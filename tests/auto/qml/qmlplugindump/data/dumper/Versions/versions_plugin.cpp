// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "versions_plugin.h"
#include "versions.h"

#include <qqml.h>

void VersionsPlugin::registerTypes(const char *uri)
{
    // @uri dumper.versions
    qmlRegisterType<Versions>(uri, 1, 0, "Versions");
    qmlRegisterType<Versions, 1>(uri, 1, 1, "Versions");
}


