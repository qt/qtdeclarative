// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "testtypes.h"

void registerTypes()
{
    qmlRegisterType<MyQmlObject>("Qt.test", 4, 6, "MyQmlObject");
}
