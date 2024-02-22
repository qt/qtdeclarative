// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CONVERTQJSPRIMITIVEVALUETOINTEGRAL_H
#define CONVERTQJSPRIMITIVEVALUETOINTEGRAL_H

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

class Moo485 : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int uid READ uid CONSTANT FINAL)

public:
    explicit Moo485(QObject *parent = nullptr) : QObject(parent) { }
    int uid() const { return 4711; }
};

class Foo485 : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(quint16 uid MEMBER m_uid FINAL)

public:
    explicit Foo485(QObject *parent = nullptr) : QObject(parent) { }
    quint16 m_uid = 0;
};

#endif // CONVERTQJSPRIMITIVEVALUETOINTEGRAL_H
