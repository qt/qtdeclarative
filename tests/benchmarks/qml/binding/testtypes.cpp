// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include "testtypes.h"

void registerTypes()
{
    qmlRegisterType<MyQmlObject>("Test", 1, 0, "MyQmlObject");
}
