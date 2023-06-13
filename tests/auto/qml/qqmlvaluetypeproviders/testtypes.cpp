// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "testtypes.h"

QList<Padding::LogEntry> Padding::log;

void registerTypes()
{
    qmlRegisterType<MyTypeObject>("Test", 1, 0, "MyTypeObject");
    qmlRegisterTypesAndRevisions<ConstructibleValueType>("Test", 1);
    qmlRegisterTypesAndRevisions<ConstructibleFromQReal>("Test", 1);
    qmlRegisterTypesAndRevisions<StructuredValueType>("Test", 1);
    qmlRegisterTypesAndRevisions<ForeignAnonymousStructuredValueType>("Test", 1);
    qmlRegisterTypesAndRevisions<Padding>("Test", 1);
    qmlRegisterTypesAndRevisions<MyItem>("Test", 1);
}
