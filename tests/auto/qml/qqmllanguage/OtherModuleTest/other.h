// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef OTHER_H
#define OTHER_H

#include <qqmlregistration.h>
#include <QQmlEngine>

namespace YepNamespaceB {
class YepAttached : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    YepAttached(QObject *parent) : QObject(parent) { }
    Q_INVOKABLE QString s() const { return QStringLiteral("OtherModuleTest Attached Type"); }
};
class Yep : public QObject
{
    Q_OBJECT
    QML_ATTACHED(YepAttached)
    QML_ELEMENT

public:
    static YepAttached *qmlAttachedProperties(QObject *object) { return new YepAttached(object); }
};

class YepSingleton : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    Q_INVOKABLE QString s() const { return QStringLiteral("OtherModuleTest Singleton"); }
};

class MyObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    Q_INVOKABLE QString s() const { return QStringLiteral("OtherModuleTest"); }
};
} // namespace YepNamespaceB

#endif // OTHER_H
