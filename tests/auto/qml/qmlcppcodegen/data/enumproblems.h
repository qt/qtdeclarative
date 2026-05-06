// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef ENUMPROBLEMS_H
#define ENUMPROBLEMS_H

#include <QObject>
#include <QtCore/qflags.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>

class Foo : public QObject {
    Q_OBJECT

public:
    enum Type {
        Unknown,
        Fighter,
        Component
    };
    Q_ENUM(Type)

    explicit Foo(Foo::Type type, QObject *parent = nullptr) : QObject(parent), m_type(type) {}

    Type type() const { return m_type; }

private:
    Type m_type = Type::Unknown;
};

namespace FooWrapper {
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(Foo)
    QML_NAMED_ELEMENT(Foo)
};


class FooThingWrapper {
    Q_GADGET
    QML_FOREIGN(Foo)
    QML_NAMED_ELEMENT(FooThing)
    QML_UNCREATABLE("nope")
};


class FooFactory : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(T8 t8 READ t8 CONSTANT FINAL)
    Q_PROPERTY(T16 t16 READ t16 CONSTANT FINAL)

public:
    enum T8: qint8 {
        A, B, C
    };
    Q_ENUM(T8)

    enum T16: qint16 {
        D = 500, E, F
    };
    Q_ENUM(T16)

    T8 t8() const { return C; }
    T16 t16() const { return E; }

    Q_INVOKABLE Foo* get(Foo::Type type) const { return new Foo(type); }
};

class ControlFlags : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Flag Container Class")
public:

    enum Option {
        ControlA = 0x1,
        ControlB = 0x2,
        Both = ControlA | ControlB
    };

    Q_DECLARE_FLAGS(Options, Option)
    Q_FLAG(Option)
};

class ScopedEnum : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Data)
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

public:
    enum class DType {
        A = 27, B
    };
    Q_ENUM(DType)

    enum EType {
        C = 7, D
    };
    Q_ENUM(EType)
};

class UnscopedEnum : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Data2)
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "true")

public:
    enum class DType {
        A = 26, B
    };
    Q_ENUM(DType)

    enum EType {
        C = 6, D
    };
    Q_ENUM(EType)
};

using uint_myown_t = decltype(75 - 12);

class UnknownUnderlyingType : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Bla : uint_myown_t {
        Yo = 11,
    };
    Q_ENUM(Bla)
};

class OldEnum : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Foos foos READ foos WRITE setFoos NOTIFY foosChanged)

    Q_ENUMS(Foo)
    QML_ELEMENT
public:
    enum Foo { None = 0, Bar = 13 };

    Q_DECLARE_FLAGS(Foos, Foo)
    Q_FLAGS(Foos)

    Foos foos() { return m_foos; }
    void setFoos(Foos foos)
    {
        if (foos != m_foos) {
            m_foos = foos;
            emit foosChanged();
        }
    }

Q_SIGNALS:
    void foosChanged();

private:
    Foos m_foos = None;
};

#endif // ENUMPROBLEMS_H
