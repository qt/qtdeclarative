// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testtypes.h"
#include <QtQml/qqml.h>

void registerTypes()
{
    qmlRegisterType<TestObject>("Qt.test", 1,0, "TestObject");
}
