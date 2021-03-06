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
#ifndef INCLUDED_SVX_PALETTEMANAGER_HXX
#define INCLUDED_SVX_PALETTEMANAGER_HXX

#include <svx/SvxColorValueSet.hxx>
#include <svx/Palette.hxx>
#include <rtl/ustring.hxx>
#include <svx/tbxcolorupdate.hxx>

#include <tools/urlobj.hxx>
#include <com/sun/star/util/XURLTransformer.hpp>
#include <com/sun/star/util/URLTransformer.hpp>

#include <deque>
#include <vector>
#include <memory>

class SVX_DLLPUBLIC PaletteManager
{
    const sal_uInt16        mnMaxRecentColors;

    sal_uInt16              mnNumOfPalettes;
    sal_uInt16              mnCurrentPalette;

    long                    mnColorCount;
    svx::ToolboxButtonColorUpdater* mpBtnUpdater;

    XColorListRef           pColorList;
    std::deque<NamedColor>  maRecentColors;
    std::vector<std::unique_ptr<Palette>> m_Palettes;

    std::function<void(const OUString&, const NamedColor&)> maColorSelectFunction;
    css::uno::Reference < css::uno::XComponentContext > m_context;
public:
    PaletteManager();
    ~PaletteManager();
    PaletteManager(const PaletteManager&) = delete;
    PaletteManager& operator=(const PaletteManager&) = delete;
    void        LoadPalettes();
    void        ReloadColorSet(SvxColorValueSet& rColorSet);
    void        ReloadColorSet(ColorValueSet& rColorSet);
    void        ReloadRecentColorSet(SvxColorValueSet& rColorSet);
    void        ReloadRecentColorSet(ColorValueSet& rColorSet);
    std::vector<OUString> GetPaletteList();
    void        SetPalette( sal_Int32 nPos );
    sal_Int32   GetPalette();
    sal_Int32   GetPaletteCount() { return mnNumOfPalettes; }
    OUString    GetPaletteName();
    OUString    GetSelectedPalettePath();

    long        GetColorCount();
    long        GetRecentColorCount();
    void        AddRecentColor(const Color& rRecentColor, const OUString& rColorName, bool bFront = true);

    void        SetBtnUpdater(svx::ToolboxButtonColorUpdater* pBtnUpdater);
    void        PopupColorPicker(weld::Window* pParent, const OUString& aCommand, const Color& rInitialColor);

    void        SetColorSelectFunction(const std::function<void(const OUString&, const NamedColor&)>& aColorSelectFunction);

    static void DispatchColorCommand(const OUString& aCommand, const NamedColor& rColor);
};

#endif // INCLUDED_SVX_PALETTEMANAGER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
