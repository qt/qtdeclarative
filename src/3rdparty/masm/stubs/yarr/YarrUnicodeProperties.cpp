/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
