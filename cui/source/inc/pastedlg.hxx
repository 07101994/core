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

#ifndef INCLUDED_CUI_SOURCE_INC_PASTEDLG_HXX
#define INCLUDED_CUI_SOURCE_INC_PASTEDLG_HXX

#include <map>
#include <sot/formats.hxx>
#include <tools/globname.hxx>
#include <svtools/transfer.hxx>
#include <vcl/weld.hxx>

struct TransferableObjectDescriptor;
class TransferableDataHelper;

class SvPasteObjectDialog : public weld::GenericDialogController
{
    std::map< SotClipboardFormatId, OUString > aSupplementMap;
    SvGlobalName    aObjClassName;
    OUString        aObjName;

    std::unique_ptr<weld::Label> m_xFtObjectSource;
    std::unique_ptr<weld::TreeView> m_xLbInsertList;
    std::unique_ptr<weld::Button> m_xOKButton;

    weld::TreeView& ObjectLB() { return *m_xLbInsertList; }

    void            SelectObject();
    DECL_LINK(SelectHdl, weld::TreeView&, void);
    DECL_LINK(DoubleClickHdl, weld::TreeView&, void);

public:
    SvPasteObjectDialog(weld::Window* pParent);

    void        Insert( SotClipboardFormatId nFormat, const OUString & rFormatName );
    void        SetObjName( const SvGlobalName & rClass, const OUString & rObjName );
    SotClipboardFormatId GetFormat( const TransferableDataHelper& aHelper);
};

#endif // INCLUDED_CUI_SOURCE_INC_PASTEDLG_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
