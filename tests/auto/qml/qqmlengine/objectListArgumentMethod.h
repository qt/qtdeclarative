// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef OBJECTLISTARGUMENTMETHOD_H
#define OBJECTLISTARGUMENTMETHOD_H

#include <QObject>
#include <qqmlregistration.h>

class ObjectListArgumentMethod : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QList<QObject *> objects READ objects CONSTANT)

public:
    QList<QObject *> objects() const { return m_objects; }

    Q_INVOKABLE int method(QList<QObject *>) { return 5; }

private:
    QList<QObject *> m_objects;
};

#endif // OBJECTLISTARGUMENTMETHOD_H
