// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "plugin.h"
#include "types.h"

#include <qqml.h>

void Plugin::registerTypes(const char *uri)
{
    // @uri dumper.ExtendedType
    qmlRegisterType<Type>(uri, 1, 0, "Type");
    qmlRegisterExtendedType<Type, ExtendedType>(uri, 1, 1, "Type");
    qmlRegisterType<DerivedType2>(uri, 1, 1, "DerivedType");
}
