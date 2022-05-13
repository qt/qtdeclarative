// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "testtypes.h"

void registerTypes()
{
    qmlRegisterType<MyTypeObject>("Test", 1, 0, "MyTypeObject");
    qmlRegisterType<MyConstantValueSource>("Test", 1, 0, "MyConstantValueSource");
    qmlRegisterType<MyOffsetValueInterceptor>("Test", 1, 0, "MyOffsetValueInterceptor");
    qmlRegisterType<MyColorObject>("Test", 1, 0, "MyColorObject");
    qmlRegisterType<MyColorInterceptor>("Test", 1, 0, "MyColorInterceptor");
    qmlRegisterType<MyFloatSetInterceptor>("Test", 1, 0, "MyFloatSetInterceptor");
    qmlRegisterType<MyFloatIgnoreInterceptor>("Test", 1, 0, "MyFloatIgnoreInterceptor");
}
