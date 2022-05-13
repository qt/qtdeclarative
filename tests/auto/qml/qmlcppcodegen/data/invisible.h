// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef INVISIBLE_H
#define INVISIBLE_H

#include <QtCore/qobject.h>
#include <QtQmlIntegration/qqmlintegration.h>

class Invisible : public QObject
{
public:
    Invisible(QObject *parent = nullptr) : QObject(parent) {}
};

class SingletonModel : public Invisible
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    SingletonModel(QObject *parent = nullptr) : Invisible(parent) {}
};

class AttachedAttached : public Invisible
{
    Q_OBJECT
public:
    AttachedAttached(QObject *parent = nullptr) : Invisible(parent) {}
};

class AttachedObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(AttachedAttached)
public:
    static AttachedAttached *qmlAttachedProperties(QObject *object)
    {
        return new AttachedAttached(object);
    }
};

class DerivedFromInvisible : public Invisible
{
    Q_OBJECT
    QML_ELEMENT
};

#endif // INVISIBLE_H
