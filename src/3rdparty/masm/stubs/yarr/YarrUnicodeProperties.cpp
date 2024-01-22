// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "config.h"
#include "yarr/YarrUnicodeProperties.h"
#include "qchar.h"

#include "yarr/Yarr.h"
#include "yarr/YarrPattern.h"

using namespace WTF;

namespace JSC { namespace Yarr {

std::optional<BuiltInCharacterClassID> unicodeMatchPropertyValue(WTF::String unicodePropertyName, WTF::String unicodePropertyValue)
{
    Q_UNUSED(unicodePropertyName);
    Q_UNUSED(unicodePropertyValue);
    return std::nullopt;
}

std::optional<BuiltInCharacterClassID> unicodeMatchProperty(WTF::String unicodePropertyValue)
{
    Q_UNUSED(unicodePropertyValue);
    return std::nullopt;
}

std::unique_ptr<CharacterClass> createUnicodeCharacterClassFor(BuiltInCharacterClassID unicodeClassID)
{
    Q_UNUSED(unicodeClassID);
    return nullptr;
}

} } // namespace JSC::Yarr
