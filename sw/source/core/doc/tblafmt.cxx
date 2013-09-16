/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <tools/resid.hxx>
#include <tools/stream.hxx>
#include <tools/shl.hxx>
#include <vcl/svapp.hxx>
#include <sfx2/docfile.hxx>
#include <svl/urihelper.hxx>
#include <svl/zforlist.hxx>
#include <svl/zformat.hxx>
#include <unotools/pathoptions.hxx>
#include <sfx2/app.hxx>
#include <svx/dialmgr.hxx>
#include <svx/dialogs.hrc>
#include <swtable.hxx>
#include <swtblfmt.hxx>
#include <com/sun/star/text/VertOrientation.hpp>
#include <swtypes.hxx>
#include <doc.hxx>
#include <poolfmt.hxx>
#include <tblafmt.hxx>
#include <cellatr.hxx>
#include <SwStyleNameMapper.hxx>
#include <hintids.hxx>
#include <fmtornt.hxx>
#include <editsh.hxx>

/*
 * XXX: BIG RED NOTICE! Changes MUST be binary file format compatible and MUST
 * be synchronized with Calc's ScAutoFormat sc/source/core/tool/autoform.cxx
 */

using ::editeng::SvxBorderLine;

// until SO5PF
const sal_uInt16 AUTOFORMAT_ID_X        = 9501;
const sal_uInt16 AUTOFORMAT_ID_358      = 9601;
const sal_uInt16 AUTOFORMAT_DATA_ID_X   = 9502;

// from SO5
//! In follow-up versions these IDs' values need to increase
const sal_uInt16 AUTOFORMAT_ID_504      = 9801;
const sal_uInt16 AUTOFORMAT_DATA_ID_504 = 9802;

const sal_uInt16 AUTOFORMAT_ID_552      = 9901;
const sal_uInt16 AUTOFORMAT_DATA_ID_552 = 9902;

// --- from 641 on: CJK and CTL font settings
const sal_uInt16 AUTOFORMAT_ID_641      = 10001;
const sal_uInt16 AUTOFORMAT_DATA_ID_641 = 10002;

// --- from 680/dr14 on: diagonal frame lines
const sal_uInt16 AUTOFORMAT_ID_680DR14      = 10011;
const sal_uInt16 AUTOFORMAT_DATA_ID_680DR14 = 10012;

// --- from 680/dr25 on: store strings as UTF-8
const sal_uInt16 AUTOFORMAT_ID_680DR25      = 10021;
const sal_uInt16 AUTOFORMAT_DATA_ID_680DR25 = 10022;

// --- from DEV300/overline2 on: overline
const sal_uInt16 AUTOFORMAT_ID_300OVRLN      = 10031;
const sal_uInt16 AUTOFORMAT_DATA_ID_300OVRLN = 10032;

// --- Bug fix to fdo#31005: Table Autoformats does not save/apply all properties (Writer and Calc)
const sal_uInt16 AUTOFORMAT_ID_31005      = 10041;
const sal_uInt16 AUTOFORMAT_DATA_ID_31005 = 10042;

// current version
const sal_uInt16 AUTOFORMAT_ID          = AUTOFORMAT_ID_31005;
const sal_uInt16 AUTOFORMAT_DATA_ID     = AUTOFORMAT_DATA_ID_31005;
const sal_uInt16 AUTOFORMAT_FILE_VERSION= SOFFICE_FILEFORMAT_50;

#define sAutoTblFmtName "autotbl.fmt"

namespace
{
    /// Begins a writer-specific data block. Call before serializing any writer-specific properties.
    sal_uInt64 BeginSwBlock(SvStream& rStream)
    {
        // We need to write down the offset of the end of the writer-specific data, so that
        // calc can skip it. We'll only have that value after writing the data, so we
        // write a placeholder value first, write the data, then jump back and write the
        // real offset.

        // Note that we explicitly use sal_uInt64 instead of sal_Size (which can be 32
        // or 64 depending on platform) to ensure 64-bit portability on this front. I don't
        // actually know if autotbl.fmt as a whole is portable, since that requires all serialization
        // logic to be written with portability in mind.
        sal_uInt64 whereToWriteEndOfSwBlock = rStream.Tell();

        sal_uInt64 endOfSwBlock = 0;
        rStream << endOfSwBlock;

        return whereToWriteEndOfSwBlock;
    }

    /// Ends a writer-specific data block. Call after serializing writer-specific properties.
    /// Closes a corresponding BeginSwBlock call.
    void EndSwBlock(SvStream& rStream, sal_uInt64 whereToWriteEndOfSwBlock)
    {
        sal_uInt64 endOfSwBlock = rStream.Tell();
        rStream.Seek(whereToWriteEndOfSwBlock);
        rStream << endOfSwBlock;
        rStream.Seek(endOfSwBlock);
    }

    /**
    Helper class for writer-specific blocks. Begins a writer-specific block on construction,
    and closes it on destruction.

    See also: BeginSwBlock and EndSwBlock.
    */
    class WriterSpecificAutoFormatBlock : ::boost::noncopyable
    {
    public:
        WriterSpecificAutoFormatBlock(SvStream &rStream) : _rStream(rStream)
        {
            _whereToWriteEndOfBlock = BeginSwBlock(rStream);
        }

        ~WriterSpecificAutoFormatBlock()
        {
            EndSwBlock(_rStream, _whereToWriteEndOfBlock);
        }

    private:
        SvStream &_rStream;
        sal_uInt64 _whereToWriteEndOfBlock;
    };

    /// Checks whether a writer-specific block exists (i.e. size is not zero)
    bool WriterSpecificBlockExists(SvStream &stream)
    {
        sal_uInt64 endOfSwBlock = 0;
        stream >> endOfSwBlock;

        // end-of-block pointing to itself indicates a zero-size block.
        return endOfSwBlock != stream.Tell();
    }
}


// Struct with version numbers of the Items

struct SwAfVersions
{
public:
    sal_uInt16 nFontVersion;
    sal_uInt16 nFontHeightVersion;
    sal_uInt16 nWeightVersion;
    sal_uInt16 nPostureVersion;
    sal_uInt16 nUnderlineVersion;
    sal_uInt16 nOverlineVersion;
    sal_uInt16 nCrossedOutVersion;
    sal_uInt16 nContourVersion;
    sal_uInt16 nShadowedVersion;
    sal_uInt16 nColorVersion;
    sal_uInt16 nBoxVersion;
    sal_uInt16 nLineVersion;
    sal_uInt16 nBrushVersion;

    sal_uInt16 nAdjustVersion;
    sal_uInt16 m_nTextOrientationVersion;
    sal_uInt16 m_nVerticalAlignmentVersion;

    sal_uInt16 nHorJustifyVersion;
    sal_uInt16 nVerJustifyVersion;
    sal_uInt16 nOrientationVersion;
    sal_uInt16 nMarginVersion;
    sal_uInt16 nBoolVersion;
    sal_uInt16 nInt32Version;
    sal_uInt16 nRotateModeVersion;

    sal_uInt16 nNumFmtVersion;

    SwAfVersions();
    void Load( SvStream& rStream, sal_uInt16 nVer );
};

SwAfVersions::SwAfVersions() :
    nFontVersion(0),
    nFontHeightVersion(0),
    nWeightVersion(0),
    nPostureVersion(0),
    nUnderlineVersion(0),
    nOverlineVersion(0),
    nCrossedOutVersion(0),
    nContourVersion(0),
    nShadowedVersion(0),
    nColorVersion(0),
    nBoxVersion(0),
    nLineVersion(0),
    nBrushVersion(0),
    nAdjustVersion(0),
    m_nTextOrientationVersion(0),
    m_nVerticalAlignmentVersion(0),
    nHorJustifyVersion(0),
    nVerJustifyVersion(0),
    nOrientationVersion(0),
    nMarginVersion(0),
    nBoolVersion(0),
    nInt32Version(0),
    nRotateModeVersion(0),
    nNumFmtVersion(0)
{
}

void SwAfVersions::Load( SvStream& rStream, sal_uInt16 nVer )
{
    rStream >> nFontVersion;
    rStream >> nFontHeightVersion;
    rStream >> nWeightVersion;
    rStream >> nPostureVersion;
    rStream >> nUnderlineVersion;
    if ( nVer >= AUTOFORMAT_ID_300OVRLN )
        rStream >> nOverlineVersion;
    rStream >> nCrossedOutVersion;
    rStream >> nContourVersion;
    rStream >> nShadowedVersion;
    rStream >> nColorVersion;
    rStream >> nBoxVersion;
    if ( nVer >= AUTOFORMAT_ID_680DR14 )
        rStream >> nLineVersion;
    rStream >> nBrushVersion;
    rStream >> nAdjustVersion;
    if (nVer >= AUTOFORMAT_ID_31005 && WriterSpecificBlockExists(rStream))
    {
        rStream >> m_nTextOrientationVersion;
        rStream >> m_nVerticalAlignmentVersion;
    }

    rStream >> nHorJustifyVersion;
    rStream >> nVerJustifyVersion;
    rStream >> nOrientationVersion;
    rStream >> nMarginVersion;
    rStream >> nBoolVersion;
    if ( nVer >= AUTOFORMAT_ID_504 )
    {
        rStream >> nInt32Version;
        rStream >> nRotateModeVersion;
    }
    rStream >> nNumFmtVersion;
}

#define READ( aItem, aItemType, nVers )\
    pNew = aItem.Create(rStream, nVers ); \
    aItem = *(aItemType*)pNew; \
    delete pNew;

sal_Bool SwTableBoxFmt::Load( SvStream& rStream, const SwAfVersions& rVersions, sal_uInt16 nVer )
{
    SfxPoolItem* pNew;

    SvxFontItem aFont = SvxFontItem( *(SvxFontItem*)GetDfltAttr( RES_CHRATR_FONT ) );
    READ( aFont, SvxFontItem, rVersions.nFontVersion )
    if( rStream.GetStreamCharSet() == aFont.GetCharSet() )
        aFont.SetCharSet( ::osl_getThreadTextEncoding() );
    SetFont( aFont );

    SvxFontHeightItem aHeight = SvxFontHeightItem( 240, 100, RES_CHRATR_FONTSIZE );
    READ( aHeight, SvxFontHeightItem, rVersions.nFontHeightVersion )
    SetHeight( aHeight );

    SvxWeightItem aWeight = SvxWeightItem( WEIGHT_NORMAL, RES_CHRATR_WEIGHT );
    READ( aWeight, SvxWeightItem, rVersions.nWeightVersion )
    SetWeight( aWeight );

    SvxPostureItem aPosture = SvxPostureItem( ITALIC_NONE, RES_CHRATR_POSTURE );
    READ( aPosture, SvxPostureItem, rVersions.nPostureVersion )
    SetPosture( aPosture );

    // --- from 641 on: CJK and CTL font settings
    if( AUTOFORMAT_DATA_ID_641 <= nVer )
    {
        SvxFontItem aCJKFont = SvxFontItem( *(SvxFontItem*)GetDfltAttr( RES_CHRATR_FONT ) );
        READ( aCJKFont, SvxFontItem, rVersions.nFontVersion )
        SetCJKFont( aCJKFont );

        SvxFontHeightItem aCJKHeight = SvxFontHeightItem( 240, 100, RES_CHRATR_FONTSIZE );
        READ( aCJKHeight, SvxFontHeightItem, rVersions.nFontHeightVersion )
        SetCJKHeight( aCJKHeight );

        SvxWeightItem aCJKWeight = SvxWeightItem( WEIGHT_NORMAL, RES_CHRATR_WEIGHT );
        READ( aCJKWeight, SvxWeightItem, rVersions.nWeightVersion )
        SetCJKWeight( aCJKWeight );

        SvxPostureItem aCJKPosture = SvxPostureItem( ITALIC_NONE, RES_CHRATR_POSTURE );
        READ( aCJKPosture, SvxPostureItem, rVersions.nPostureVersion )
        SetCJKPosture( aCJKPosture );


        SvxFontItem aCTLFont = SvxFontItem( *(SvxFontItem*)GetDfltAttr( RES_CHRATR_FONT ) );
        READ( aCTLFont, SvxFontItem, rVersions.nFontVersion )
        SetCTLFont( aCTLFont );

        SvxFontHeightItem aCTLHeight = SvxFontHeightItem( 240, 100, RES_CHRATR_FONTSIZE );
        READ( aCTLHeight, SvxFontHeightItem, rVersions.nFontHeightVersion )
        SetCTLHeight( aCTLHeight );

        SvxWeightItem aCTLWeight = SvxWeightItem( WEIGHT_NORMAL, RES_CHRATR_WEIGHT );
        READ( aCTLWeight, SvxWeightItem, rVersions.nWeightVersion )
        SetCTLWeight( aCTLWeight );

        SvxPostureItem aCTLPosture = SvxPostureItem( ITALIC_NONE, RES_CHRATR_POSTURE );
        READ( aCTLPosture, SvxPostureItem, rVersions.nPostureVersion )
        SetCTLPosture( aCTLPosture );
    }

    SvxUnderlineItem aUnderline = SvxUnderlineItem( UNDERLINE_NONE, RES_CHRATR_UNDERLINE );
    READ( aUnderline, SvxUnderlineItem, rVersions.nUnderlineVersion )
    SetUnderline( aUnderline );

    if( nVer >= AUTOFORMAT_DATA_ID_300OVRLN )
    {
        SvxOverlineItem aOverline = SvxOverlineItem( UNDERLINE_NONE, RES_CHRATR_OVERLINE );
        READ( aOverline, SvxOverlineItem, rVersions.nOverlineVersion )
        SetOverline( aOverline );
    }

    SvxCrossedOutItem aCrossedOut = SvxCrossedOutItem( STRIKEOUT_NONE, RES_CHRATR_CROSSEDOUT );
    READ( aCrossedOut, SvxCrossedOutItem, rVersions.nCrossedOutVersion )
    SetCrossedOut( aCrossedOut );

    SvxContourItem aContour = SvxContourItem( sal_False, RES_CHRATR_CONTOUR );
    READ( aContour, SvxContourItem, rVersions.nContourVersion )
    SetContour( aContour );

    SvxShadowedItem aShadowed = SvxShadowedItem( sal_False, RES_CHRATR_SHADOWED );
    READ( aShadowed, SvxShadowedItem, rVersions.nShadowedVersion )
    SetShadowed( aShadowed );

    SvxColorItem aColor = SvxColorItem( RES_CHRATR_COLOR );
    READ( aColor, SvxColorItem, rVersions.nColorVersion )
    SetColor( aColor );

    SvxBoxItem aBox = SvxBoxItem( RES_BOX );
    READ( aBox, SvxBoxItem, rVersions.nBoxVersion )
    SetBox( aBox );

    // Unused, but needed to be loaded to not mess the other styles
    if( nVer >= AUTOFORMAT_DATA_ID_680DR14 )
    {
        SvxLineItem aTLBR( 0 );
        READ( aTLBR, SvxLineItem, rVersions.nLineVersion)

        SvxLineItem aBLTR( 0 );
        READ( aBLTR, SvxLineItem, rVersions.nLineVersion)
    }
    // <- close

    SvxBrushItem aBackground = SvxBrushItem( RES_BACKGROUND );
    READ( aBackground, SvxBrushItem, rVersions.nBrushVersion )
    SetBackground( aBackground );

    SvxAdjustItem aAdjust = SvxAdjustItem( SVX_ADJUST_LEFT, RES_PARATR_ADJUST );
    pNew = aAdjust.Create( rStream, rVersions.nAdjustVersion );
    SetAdjust( *(SvxAdjustItem*)pNew );
    delete pNew;

    if (nVer >= AUTOFORMAT_DATA_ID_31005 && WriterSpecificBlockExists(rStream))
    {
        SvxFrameDirectionItem aTextOrientation = SvxFrameDirectionItem( FRMDIR_ENVIRONMENT, RES_FRAMEDIR );
        READ( aTextOrientation, SvxFrameDirectionItem, rVersions.m_nTextOrientationVersion )
        SetTextOrientation( aTextOrientation );

        SwFmtVertOrient aVerticalAlignment = SwFmtVertOrient( 0, com::sun::star::text::VertOrientation::NONE, com::sun::star::text::RelOrientation::FRAME );
        READ( aVerticalAlignment, SwFmtVertOrient, rVersions.m_nVerticalAlignmentVersion );
        SetVerticalAlignment( aVerticalAlignment );
    }

    // Unused, but needed to be loaded to not mess the other styles
    SvxHorJustifyItem aHorJustify( SVX_HOR_JUSTIFY_STANDARD, 0 );
    READ( aHorJustify, SvxHorJustifyItem, rVersions.nHorJustifyVersion )

    SvxVerJustifyItem aVerJustify( SVX_VER_JUSTIFY_STANDARD, 0 );
    READ( aVerJustify, SvxVerJustifyItem, rVersions.nVerJustifyVersion )

    SvxOrientationItem aOrientation( SVX_ORIENTATION_STANDARD, 0 );
    READ( aOrientation, SvxOrientationItem, rVersions.nOrientationVersion )

    SvxMarginItem aMargin( 0 );
    READ( aMargin, SvxMarginItem, rVersions.nMarginVersion )

    SfxBoolItem aLinebreak( 0 );
    pNew = aLinebreak.Create(rStream, rVersions.nBoolVersion );
    aLinebreak.SetValue( ((SfxBoolItem*)pNew)->GetValue() );
    delete pNew;

    if ( nVer >= AUTOFORMAT_DATA_ID_504 )
    {
        SfxInt32Item aRotateAngle( 0 );
        pNew = aRotateAngle.Create( rStream, rVersions.nInt32Version );
        aRotateAngle.SetValue( ((SfxInt32Item*)pNew)->GetValue() );
        delete pNew;

        SvxRotateModeItem aRotateMode( SVX_ROTATE_MODE_STANDARD, 0 );
        pNew = aRotateMode.Create( rStream, rVersions.nRotateModeVersion );
        aRotateMode.SetValue( ((SvxRotateModeItem*)pNew)->GetValue() );
        delete pNew;
    }
    // <- close

    if( 0 == rVersions.nNumFmtVersion )
    {
        sal_uInt16 eSys, eLge;
        // --- from 680/dr25 on: store strings as UTF-8
        CharSet eCharSet = (nVer >= AUTOFORMAT_ID_680DR25) ? RTL_TEXTENCODING_UTF8 : rStream.GetStreamCharSet();
        sNumFmtString = rStream.ReadUniOrByteString( eCharSet );
        rStream >> eSys >> eLge;
        eSysLanguage = (LanguageType) eSys;
        eNumFmtLanguage = (LanguageType) eLge;
        if ( eSysLanguage == LANGUAGE_SYSTEM )      // from old versions (Calc)
            eSysLanguage = ::GetAppLanguage();
    }

    return 0 == rStream.GetError();
}

SwTableFmt* SwTableFmt::Load( SvStream& rStream, const SwAfVersions& rVersions, SwDoc* pDoc )
{
    SwTableFmt* pRet = NULL;
    sal_Bool bRet = 0 == rStream.GetError();

    sal_uInt16  nVal = 0;
    rStream >> nVal;
    bRet = 0 == rStream.GetError();

    if( bRet && (nVal == AUTOFORMAT_DATA_ID_X ||
            (AUTOFORMAT_DATA_ID_504 <= nVal && nVal <= AUTOFORMAT_DATA_ID)) )
    {
        sal_Bool b;
        // --- from 680/dr25 on: store strings as UTF-8
        CharSet eCharSet = (nVal >= AUTOFORMAT_ID_680DR25) ? RTL_TEXTENCODING_UTF8 : rStream.GetStreamCharSet();
        OUString aName = rStream.ReadUniOrByteString( eCharSet );
        sal_uInt16 nStrResId  = USHRT_MAX;
        if( AUTOFORMAT_DATA_ID_552 <= nVal )
        {
            rStream >> nStrResId;
            sal_uInt16 nId = RID_SVXSTR_TBLAFMT_BEGIN + nStrResId;
            if( RID_SVXSTR_TBLAFMT_BEGIN <= nId &&
                nId < RID_SVXSTR_TBLAFMT_END )
            {
                aName = SVX_RESSTR( nId );
            }
            else
                nStrResId = USHRT_MAX;
        }

        pRet = pDoc->GetTableStyles()->MakeStyle( aName );
        pRet->nStrResId = nStrResId;

        // No longer needed, but still read to not misread other data
        rStream >> b;
        rStream >> b;
        rStream >> b;
        rStream >> b;
        rStream >> b;
        rStream >> b;
        // <- close

        if (nVal >= AUTOFORMAT_DATA_ID_31005 && WriterSpecificBlockExists(rStream))
        {
            SfxPoolItem* pNew = 0;

            SvxFmtBreakItem aBreak = SvxFmtBreakItem( SVX_BREAK_NONE, RES_BREAK );
            READ( aBreak, SvxFmtBreakItem, AUTOFORMAT_FILE_VERSION );
            pRet->SetBreak( aBreak );

            SwFmtPageDesc aPageDesc;
            READ( aPageDesc, SwFmtPageDesc, AUTOFORMAT_FILE_VERSION );
            pRet->SetPageDesc( aPageDesc );

            SvxFmtKeepItem aKeepWithNextPara = SvxFmtKeepItem( sal_False, RES_KEEP );
            READ( aKeepWithNextPara, SvxFmtKeepItem, AUTOFORMAT_FILE_VERSION );
            pRet->SetKeepWithNextPara( aKeepWithNextPara );

            sal_uInt16 aRepeatHeading;
            sal_Bool bLayoutSplit;
            sal_Bool bRowSplit;
            sal_Bool bCollapsingBorders;
            rStream >> aRepeatHeading >> bLayoutSplit >> bRowSplit >> bCollapsingBorders;
            pRet->SetRepeatHeading( aRepeatHeading );
            pRet->SetRowSplit( bRowSplit );
            pRet->SetLayoutSplit( bLayoutSplit );
            pRet->SetCollapsingBorders( bCollapsingBorders );

            SvxShadowItem aShadow = SvxShadowItem( RES_SHADOW );
            READ( aShadow, SvxShadowItem, AUTOFORMAT_FILE_VERSION );
            pRet->SetShadow( aShadow );
        }

        bRet = 0 == rStream.GetError();

        for( sal_uInt8 i = 0; bRet && i < 16; ++i )
        {
            SwTableBoxFmt* pFmt = pDoc->MakeTableBoxFmt();

            bRet = pFmt->Load( rStream, rVersions, nVal );
            if( bRet )
                pRet->SetBoxFmt( *pFmt, i );
            else
            {
                delete pFmt;
                break;
            }
        }
    }
    if ( !bRet )
    {
        delete pRet;
        pRet = NULL;
    }

    return pRet;
}

size_t SwTableFmtTbl::size() const
{
    return m_TableStyles.size();
}

SwTableFmt const* SwTableFmtTbl::operator[](size_t const i) const
{
    return m_TableStyles[i];
}
SwTableFmt      * SwTableFmtTbl::operator[](size_t const i)
{
    return m_TableStyles[i];
}

SwTableFmt* SwTableFmtTbl::MakeStyle( OUString sName )
{
    SwTableFmt* pStyle = new SwTableFmt( m_pDoc->GetAttrPool(), sName, m_pDoc->GetDfltFrmFmt() );
    m_TableStyles.push_back( pStyle );
    m_pDoc->SetModified();

    return pStyle;
}

SwTableFmt* SwTableFmtTbl::FindStyle( OUString sName )
{
    SwTableFmt* pFnd = 0;
    for( sal_uInt16 n = 0; n < m_TableStyles.size(); n++ )
    {
        if( m_TableStyles[ n ]->GetName() == sName )
        {
            pFnd = m_TableStyles[ n ];
            break;
        }
    }

    return pFnd;
}

void SwTableFmtTbl::InsertStyle(size_t const i, SwTableFmt * pFmt)
{
    m_TableStyles.insert(m_TableStyles.begin() + i, pFmt);
}

void SwTableFmtTbl::EraseStyle(size_t const i)
{
    m_TableStyles.erase(m_TableStyles.begin() + i);
}

void SwTableFmtTbl::MoveStyle(size_t const target, size_t source)
{
    m_TableStyles.insert( m_TableStyles.begin() + target, m_TableStyles[ source ] );
    m_TableStyles.erase( m_TableStyles.begin() + source );
}

SwTableFmtTbl::~SwTableFmtTbl()
{
    for( sal_uInt16 n = 0; n < m_TableStyles.size(); n++ )
    {
        delete m_TableStyles[ n ];
        m_TableStyles.erase( m_TableStyles.begin() + n );
    }
}

SwTableFmtTbl::SwTableFmtTbl(SwDoc* pDoc)
    : m_pDoc( pDoc)
{
    OUString sNm;
    sNm = SwStyleNameMapper::GetUIName( RES_POOLCOLL_STANDARD, sNm );
    SwTableFmt* pNewTableStyle = MakeStyle( sNm );

    SwTableBoxFmt* pNewBoxFmt = pDoc->MakeTableBoxFmt();

    sal_uInt8 i;

    Color aColor( COL_BLUE );
    SvxBrushItem aBrushItem( aColor, RES_BACKGROUND );
    pNewBoxFmt->SetBackground( aBrushItem );
    pNewBoxFmt->SetColor( SvxColorItem(Color( COL_WHITE ), RES_CHRATR_COLOR) );

    for( i = 0; i < 4; ++i )
        pNewTableStyle->SetBoxFmt( *pNewBoxFmt, i );

    // 70% gray
    aBrushItem.SetColor( RGB_COLORDATA( 0x4d, 0x4d, 0x4d ) );
    pNewBoxFmt->SetBackground( aBrushItem );
    for( i = 4; i <= 12; i += 4 )
        pNewTableStyle->SetBoxFmt( *pNewBoxFmt, i );

    // 20% gray
    aBrushItem.SetColor( RGB_COLORDATA( 0xcc, 0xcc, 0xcc ) );
    pNewBoxFmt->SetBackground( aBrushItem );
    aColor.SetColor( COL_BLACK );
    pNewBoxFmt->SetColor( SvxColorItem( aColor, RES_CHRATR_COLOR) );
    for( i = 7; i <= 15; i += 4 )
        pNewTableStyle->SetBoxFmt( *pNewBoxFmt, i );
    for( i = 13; i <= 14; ++i )
        pNewTableStyle->SetBoxFmt( *pNewBoxFmt, i );

    aBrushItem.SetColor( Color( COL_WHITE ) );
    pNewBoxFmt->SetBackground( aBrushItem );
    for( i = 5; i <= 6; ++i )
        pNewTableStyle->SetBoxFmt( *pNewBoxFmt, i );
    for( i = 9; i <= 10; ++i )
        pNewTableStyle->SetBoxFmt( *pNewBoxFmt, i );


    SvxBoxItem aBox( RES_BOX );
    aBox.SetDistance( 55 );
    SvxBorderLine aLn( &aColor, DEF_LINE_WIDTH_0 );
    aBox.SetLine( &aLn, BOX_LINE_LEFT );
    aBox.SetLine( &aLn, BOX_LINE_BOTTOM );

    for( i = 0; i <= 15; ++i )
    {
        aBox.SetLine( i <= 3 ? &aLn : 0, BOX_LINE_TOP );
        aBox.SetLine( (3 == ( i & 3 )) ? &aLn : 0, BOX_LINE_RIGHT );
        pNewTableStyle->GetBoxFmt( i )->SetBox( aBox );
    }
}

sal_Bool SwTableFmtTbl::Load()
{
    sal_Bool bRet = sal_False;
    String sNm(OUString(sAutoTblFmtName));
    SvtPathOptions aOpt;
    if( aOpt.SearchFile( sNm, SvtPathOptions::PATH_USERCONFIG ))
    {
        SfxMedium aStream( sNm, STREAM_STD_READ );
        bRet = Load( *aStream.GetInStream() );
    }
    else
        bRet = sal_False;
    return bRet;
}

sal_Bool SwTableFmtTbl::Load( SvStream& rStream )
{
    sal_Bool bRet = 0 == rStream.GetError();
    if (bRet)
    {
        // Attention: We need to read a general Header here
        sal_uInt16 nVal = 0;
        rStream >> nVal;
        bRet = 0 == rStream.GetError();

        if( bRet )
        {
            SwAfVersions aVersions;

            // Default version is 5.0, unless we detect an old format ID.
            sal_uInt16 nFileVers = SOFFICE_FILEFORMAT_50;
            if(nVal < AUTOFORMAT_ID_31005)
                nFileVers = SOFFICE_FILEFORMAT_40;

            if( nVal == AUTOFORMAT_ID_358 ||
                    (AUTOFORMAT_ID_504 <= nVal && nVal <= AUTOFORMAT_ID) )
            {
                sal_uInt8 nChrSet, nCnt;
                long nPos = rStream.Tell();
                rStream >> nCnt >> nChrSet;
                if( rStream.Tell() != sal_uLong(nPos + nCnt) )
                {
                    OSL_ENSURE( !this, "The Header contains more or newer Data" );
                    rStream.Seek( nPos + nCnt );
                }
                rStream.SetStreamCharSet( (CharSet)nChrSet );
                rStream.SetVersion( nFileVers );
            }

            if( nVal == AUTOFORMAT_ID_358 || nVal == AUTOFORMAT_ID_X ||
                    (AUTOFORMAT_ID_504 <= nVal && nVal <= AUTOFORMAT_ID) )
            {
                aVersions.Load( rStream, nVal );        // Item versions

                SwTableFmt* pNew;
                sal_uInt16 nAnz = 0;
                rStream >> nAnz;

                bRet = 0 == rStream.GetError();

                for( sal_uInt16 i = 0; i < nAnz; ++i )
                {
                    pNew = SwTableFmt::Load( rStream, aVersions, m_pDoc );
                    if( !pNew )
                    {
                        bRet = false;
                        break;
                    }
                }
            }
            else
            {
                bRet = false;
            }
        }
    }
    return bRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
