// # Copyright (C) 2023 The Qt Company Ltd.
// # SPDX-License-Identifier: BSD-3-Clause

//! [setup]
#include "setup.h"

void Setup::applicationAvailable()
{
    // custom code that doesn't require QQmlEngine
}

void Setup::qmlEngineAvailable(QQmlEngine *engine)
{
    // add import paths
}

void Setup::cleanupTestCase()
{
    // custom code to clean up before destruction starts
}
//! [setup]
