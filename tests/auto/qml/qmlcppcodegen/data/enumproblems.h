// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

public:
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

#endif // ENUMPROBLEMS_H
