// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TESTTYPES_H
#define TESTTYPES_H

#include <QtCore/QObject>
#include <QtQml/qqml.h>

class A : public QObject
{
    Q_OBJECT

    QML_ELEMENT
    QML_ADDED_IN_VERSION(5, 0)
    QML_REMOVED_IN_VERSION(6, 0)
};

class B : public QObject
{
    Q_OBJECT

    QML_ELEMENT
    QML_ADDED_IN_VERSION(5, 0)
};

class C : public QObject
{
    Q_OBJECT

    QML_ELEMENT
    QML_REMOVED_IN_VERSION(6, 0)
};

class D : public QObject
{
    Q_OBJECT

    QML_ELEMENT
};
#endif // TESTTYPES_H
