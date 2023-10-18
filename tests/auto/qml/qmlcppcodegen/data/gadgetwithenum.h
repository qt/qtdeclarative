// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef GADGETWITHENUM_H
#define GADGETWITHENUM_H

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

class GadgetWithEnum : public QObject {
    Q_GADGET
    QML_ELEMENT

public:
    enum State {
        DISCONNECTED,
        CONNECTING,
        CONNECTED
    };
    Q_ENUM(State)
};

namespace GadgetWithEnumWrapper {
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(GadgetWithEnum)
    QML_NAMED_ELEMENT(NamespaceWithEnum)
};

struct Gadget
{
    Q_GADGET
    QML_VALUE_TYPE(gadget)

public:
    enum class Prop1 { High, low, VeryHigh, VeryLow };
    Q_ENUM(Prop1)

    enum class Prop2 { VeryHigh, High, low, VeryLow };
    Q_ENUM(Prop2)
};

class Backend : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(prop priority READ priority FINAL CONSTANT)
    Q_PROPERTY(Gadget gadget READ gadget FINAL CONSTANT)
    Q_CLASSINFO("RegisterEnumsFromRelatedTypes", "false")

public:
    enum prop { High, low, VeryHigh, VeryLow };
    Q_ENUM(prop)

    explicit Backend(QObject *parent = nullptr) : QObject(parent) {}

    prop priority() const { return m_priority; }
    Gadget gadget() const { return m_gadget; }

private:
    prop m_priority = low;
    Gadget m_gadget;
};

#endif // GADGETWITHENUM_H
