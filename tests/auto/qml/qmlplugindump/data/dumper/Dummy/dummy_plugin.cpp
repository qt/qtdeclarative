// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dummy_plugin.h"
#include "dummy.h"

#include <qqml.h>

void DummyPlugin::registerTypes(const char *uri)
{
    // @uri dumper.dummy
    qmlRegisterType<Dummy>(uri, 1, 0, "Dummy");
}


