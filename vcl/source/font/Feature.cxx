/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include <vcl/font/Feature.hxx>
#include <strings.hrc>
#include <svdata.hxx>

namespace vcl
{
namespace font
{
OUString featureCodeAsString(sal_uInt32 nFeature)
{
    std::vector<sal_Char> aString(5, 0);
    aString[0] = sal_Char(nFeature >> 24 & 0xff);
    aString[1] = sal_Char(nFeature >> 16 & 0xff);
    aString[2] = sal_Char(nFeature >> 8 & 0xff);
    aString[3] = sal_Char(nFeature >> 0 & 0xff);

    return OStringToOUString(aString.data(), RTL_TEXTENCODING_ASCII_US);
}

// FeatureParameter

FeatureParameter::FeatureParameter(sal_uInt32 nCode, OUString aDescription)
    : m_nCode(nCode)
    , m_sDescription(aDescription)
    , m_pDescriptionID(nullptr)
{
}

FeatureParameter::FeatureParameter(sal_uInt32 nCode, const char* pDescriptionID)
    : m_nCode(nCode)
    , m_pDescriptionID(pDescriptionID)
{
}

OUString FeatureParameter::getDescription() const
{
    OUString aReturnString;

    if (m_pDescriptionID)
        aReturnString = VclResId(m_pDescriptionID);
    else if (!m_sDescription.isEmpty())
        aReturnString = m_sDescription;

    return aReturnString;
}

sal_uInt32 FeatureParameter::getCode() const { return m_nCode; }

// FeatureDefinition

FeatureDefinition::FeatureDefinition()
    : m_nCode(0)
    , m_pDescriptionID(nullptr)
    , m_eType(FeatureParameterType::BOOL)
{
}

FeatureDefinition::FeatureDefinition(sal_uInt32 nCode, OUString const& rDescription,
                                     FeatureParameterType eType,
                                     std::vector<FeatureParameter> const& rEnumParameters)
    : m_nCode(nCode)
    , m_sDescription(rDescription)
    , m_pDescriptionID(nullptr)
    , m_eType(eType)
    , m_aEnumParameters(rEnumParameters)
{
}

FeatureDefinition::FeatureDefinition(sal_uInt32 nCode, const char* pDescriptionID,
                                     OUString const& rNumericPart)
    : m_nCode(nCode)
    , m_pDescriptionID(pDescriptionID)
    , m_sNumericPart(rNumericPart)
    , m_eType(FeatureParameterType::BOOL)
{
}

FeatureDefinition::FeatureDefinition(sal_uInt32 nCode, const char* pDescriptionID,
                                     std::vector<FeatureParameter> aEnumParameters)
    : m_nCode(nCode)
    , m_pDescriptionID(pDescriptionID)
    , m_eType(FeatureParameterType::ENUM)
    , m_aEnumParameters(aEnumParameters)
{
}

const std::vector<FeatureParameter>& FeatureDefinition::getEnumParameters() const
{
    return m_aEnumParameters;
}

OUString FeatureDefinition::getDescription() const
{
    if (m_pDescriptionID)
    {
        OUString sTranslatedDescription = VclResId(m_pDescriptionID);
        if (!m_sNumericPart.isEmpty())
            return sTranslatedDescription.replaceFirst("%1", m_sNumericPart);
        return sTranslatedDescription;
    }
    else if (!m_sDescription.isEmpty())
    {
        return m_sDescription;
    }
    else
    {
        return vcl::font::featureCodeAsString(m_nCode);
    }
}

sal_uInt32 FeatureDefinition::getCode() const { return m_nCode; }

FeatureParameterType FeatureDefinition::getType() const { return m_eType; }

FeatureDefinition::operator bool() const { return m_nCode != 0; }

} // end font namespace
} // end vcl namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
