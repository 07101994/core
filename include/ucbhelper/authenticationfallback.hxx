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

#ifndef INCLUDED_UCBHELPER_AUTHENTICATIONFALLBACK_HXX
#define INCLUDED_UCBHELPER_AUTHENTICATIONFALLBACK_HXX

#include <rtl/ref.hxx>
#include <ucbhelper/interactionrequest.hxx>
#include <ucbhelper/ucbhelperdllapi.h>

namespace ucbhelper {

/**
  * This class implements a simple authentication interaction request used
  * when programmatically authentication cannot succeed.
  *
  * Read-only values : instructions, url
  * Read-write values: code
  */
class UCBHELPER_DLLPUBLIC AuthenticationFallbackRequest : public ucbhelper::InteractionRequest
{

    OUString m_Instructions;
    OUString m_Url;
    OUString m_Code;
public:
    /**
      * Constructor.
      *
      * @param rInstructions instructions to be followed by the user
      * @param rURL contains a URL for which authentication is requested.
      */
    AuthenticationFallbackRequest( const OUString & rInstructions,
                                 const OUString & rURL );
    void setCode( const OUString & code ) { m_Code = code; }
    OUString getCode( ) { return m_Code; }
};

} // namespace ucbhelper

#endif /* ! INCLUDED_UCBHELPER_AUTHENTICATIONFALLBACK_HXX*/

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
