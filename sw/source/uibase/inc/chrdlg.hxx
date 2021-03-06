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
#ifndef INCLUDED_SW_SOURCE_UIBASE_INC_CHRDLG_HXX
#define INCLUDED_SW_SOURCE_UIBASE_INC_CHRDLG_HXX

#include <sfx2/tabdlg.hxx>
#include <vcl/group.hxx>
#include <vcl/fixed.hxx>
#include <vcl/edit.hxx>
#include <vcl/combobox.hxx>
#include <vcl/button.hxx>
#include <vcl/lstbox.hxx>
#include "chrdlgmodes.hxx"

class SwView;
class SvxMacroItem;

class SwCharDlg: public SfxTabDialog
{
    SwView&   m_rView;
    SwCharDlgMode m_nDialogMode;

    sal_uInt16 m_nCharStdId;
    sal_uInt16 m_nCharExtId;
    sal_uInt16 m_nCharPosId;
    sal_uInt16 m_nCharTwoId;
    sal_uInt16 m_nCharUrlId;
    sal_uInt16 m_nCharBgdId;
    sal_uInt16 m_nCharBrdId;

public:
    SwCharDlg(vcl::Window* pParent, SwView& pVw, const SfxItemSet& rCoreSet,
              SwCharDlgMode nDialogMode, const OUString* pFormatStr);

    virtual ~SwCharDlg() override;

    virtual void PageCreated( sal_uInt16 nId, SfxTabPage &rPage ) override;
};

class SwCharURLPage : public SfxTabPage
{
    SvxMacroItem*       pINetItem;
    bool                bModified;

    std::unique_ptr<weld::Entry> m_xURLED;
    std::unique_ptr<weld::Label> m_xTextFT;
    std::unique_ptr<weld::Entry> m_xTextED;
    std::unique_ptr<weld::Entry> m_xNameED;
    std::unique_ptr<weld::ComboBoxText> m_xTargetFrameLB;
    std::unique_ptr<weld::Button> m_xURLPB;
    std::unique_ptr<weld::Button> m_xEventPB;
    std::unique_ptr<weld::ComboBoxText> m_xVisitedLB;
    std::unique_ptr<weld::ComboBoxText> m_xNotVisitedLB;
    std::unique_ptr<weld::Widget> m_xCharStyleContainer;

    DECL_LINK(InsertFileHdl, weld::Button&, void);
    DECL_LINK(EventHdl, weld::Button&, void);

public:
    SwCharURLPage(TabPageParent pParent, const SfxItemSet& rSet);

    virtual ~SwCharURLPage() override;
    virtual void dispose() override;
    static VclPtr<SfxTabPage> Create(TabPageParent pParent,
                                     const SfxItemSet* rAttrSet);

    virtual bool        FillItemSet( SfxItemSet* rSet ) override;
    virtual void        Reset( const SfxItemSet* rSet ) override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
