// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef UNCREATABLE_H
#define UNCREATABLE_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQml/qqmlregistration.h>
#include <QtQml/qqmlengine.h>

class SingletonCreatable : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    SingletonCreatable() = delete;
    // not default constructible but has the static create method
public:
    static SingletonCreatable *create(QQmlEngine *, QJSEngine *) { return nullptr; }
};

class SingletonCreatable2 : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // is default constructible
};

class SingletonCreatable3 : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    // is default constructible and has the create method
    static SingletonCreatable3 *create(QQmlEngine *, QJSEngine *) { return nullptr; }
};

class SingletonForeign : public QObject
{
    Q_OBJECT
    SingletonForeign() = delete;
};

class SingletonLocalCreatable
{
    Q_GADGET
    QML_FOREIGN(SingletonForeign)
    QML_ELEMENT
    QML_SINGLETON
public:
    static SingletonForeign *create(QQmlEngine *, QJSEngine *) { return nullptr; }
};

class SingletonLocalUncreatable1
{
    Q_GADGET
    QML_FOREIGN(SingletonForeign)
    QML_ELEMENT
    QML_SINGLETON
    static SingletonForeign *create(QQmlEngine *, QJSEngine *) { return nullptr; }
};

class SingletonLocalUncreatable2
{
    Q_GADGET
    QML_FOREIGN(SingletonForeign)
    QML_ELEMENT
    QML_SINGLETON
};

class SingletonIncreatable : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    SingletonIncreatable() = delete;

public:
    static SingletonCreatable *create(QQmlEngine *, QJSEngine *)
    {
        return nullptr;
    } // wrong return type
};

class SingletonIncreatable2 : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    SingletonIncreatable2() = delete;

public:
    static SingletonCreatable2 *create(QQmlEngine *) { return nullptr; } // wrong argument type
};

class SingletonIncreatable3 : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    SingletonIncreatable3() = delete;

public:
    SingletonCreatable3 *create(QQmlEngine *, QJSEngine *) { return nullptr; } // should be static
};

class SingletonIncreatable4 : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    SingletonIncreatable4() = delete;
    static SingletonIncreatable4 *create(QQmlEngine *, QJSEngine *)
    {
        return nullptr;
    } // should be public
};

class BadUncreatable : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    BadUncreatable() = delete;
};

class GoodUncreatable : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    GoodUncreatable() = delete;
};

class UncreatableNeedsForeign : public QObject
{
    Q_OBJECT

    virtual void f() = 0;
};

class GoodUncreatable2
{
    Q_GADGET
    QML_FOREIGN(UncreatableNeedsForeign)
    QML_ELEMENT
    QML_UNCREATABLE("")
};

class Creatable : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    Creatable(QObject *parent = nullptr) { Q_UNUSED(parent); }; // default constructor in disguise
};

class Extension : public QObject
{
    Q_OBJECT

public:
    Extension(QObject *parent = nullptr) : QObject(parent) {}

    enum HelloWorld { Hello, Hallo, Bonjour };
    Q_ENUM(HelloWorld)
};

class Creatable2 : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(Extension)
public:
    Creatable2(QObject *parent = nullptr) { Q_UNUSED(parent); }; // default constructor in disguise
};

class SingletonIncreatableExtended : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    QML_EXTENDED(Extension)
    SingletonIncreatableExtended() = delete;
    static SingletonIncreatableExtended *create(QQmlEngine *, QJSEngine *)
    {
        return nullptr;
    } // should be public
};

class BadUncreatableExtended : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(Extension)

    BadUncreatableExtended() = delete;
};

class GoodUncreatableExtended : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    QML_EXTENDED(Extension)

    GoodUncreatableExtended() = delete;
};

class FixingBadUncreatable
{
    Q_GADGET
    QML_FOREIGN(BadUncreatable)
    QML_UNCREATABLE("")
};

class SingletonVesion0 : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // is default constructible
};

class SingletonVesion1 : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // is default constructible
};
#endif // UNCREATABLE_H
