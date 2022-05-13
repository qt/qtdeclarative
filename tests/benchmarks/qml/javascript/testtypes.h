// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TESTTYPES_H
#define TESTTYPES_H

#include <QtCore/qobject.h>

class TestObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int intValue READ intValue);
    Q_PROPERTY(QString stringValue READ stringValue);

public:
    TestObject() : m_string("Hello world!") {}

    int intValue() const { return 13; }
    QString stringValue() const { return m_string; }

private:
    QString m_string;
};

void registerTypes();

#endif // TESTTYPES_H
