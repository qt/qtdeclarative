// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imports_plugin.h"
#include "imports.h"

#include <qqml.h>

void ImportsPlugin::registerTypes(const char *uri)
{
    // @uri dumper.imports
    qmlRegisterType<Imports>(uri, 1, 0, "Imports");
}


