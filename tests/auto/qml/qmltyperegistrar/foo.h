// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef FOO_H
#define FOO_H

#include <QtCore/qobject.h>
#include <qqml.h>

class Bbb : public QObject
{
    Q_OBJECT
public:
    Bbb(QObject *parent = nullptr) : QObject(parent) {}
};

class Ccc : public QObject
{
    Q_OBJECT
public:
    Ccc(QObject *parent) : QObject(parent) {}
};

class Ooo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(Bbb)
    QML_ATTACHED(Ccc);
public:
    Q_INVOKABLE void blah();

    static Ccc *qmlAttachedProperties(QObject *o) { return new Ccc(o); }
};

namespace Foo {
Q_NAMESPACE
QML_ELEMENT

enum Bar {
    A, B, C
};

Q_ENUM_NS(Bar);
}


#endif // FOO_H
