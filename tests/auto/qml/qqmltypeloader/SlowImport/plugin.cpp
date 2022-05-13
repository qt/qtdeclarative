// Copyright (C) 2016 Canonical Limited and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plugin.h"

#include "slow.h"
#include <qqml.h>

void SlowPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("Slow"));
    qmlRegisterType<SlowStuff>(uri, 1, 0, "SlowStuff");
}
