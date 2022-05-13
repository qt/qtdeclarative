// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef INTERFACES_H
#define INTERFACES_H

#include <QtQml/qqml.h>
#include <QTest>

struct Interface {
    // non-virtual, non-QObject interfaces are not supported
    virtual ~Interface() {};
};

QT_BEGIN_NAMESPACE
#define MyInterface_iid "io.qt.bugreports.Interface"
Q_DECLARE_INTERFACE(Interface, MyInterface_iid);
QT_END_NAMESPACE

class A : public QObject, public Interface {
    Q_OBJECT
    QML_IMPLEMENTS_INTERFACES(Interface)
};

class B : public QObject, Interface {
    Q_OBJECT
    QML_IMPLEMENTS_INTERFACES(Interface)
};

class C : public QObject {
    Q_OBJECT
};

struct Interface2
{
    Q_GADGET
    QML_INTERFACE
public:
    // non-virtual, non-QObject interfaces are not supported
    virtual ~Interface2() {};
};

QT_BEGIN_NAMESPACE
#define MyInterface2_iid "io.qt.bugreports.Interface2"
Q_DECLARE_INTERFACE(Interface2, MyInterface2_iid);
QT_END_NAMESPACE

class A2 : public QObject, public Interface2
{
    Q_OBJECT
    QML_ELEMENT
    QML_IMPLEMENTS_INTERFACES(Interface2)
};

class B2 : public QObject, Interface2 {
    Q_OBJECT
    QML_ELEMENT
    QML_IMPLEMENTS_INTERFACES(Interface2)
};

class C2 : public QObject {
    Q_OBJECT
    QML_ELEMENT
};

class InterfaceConsumer : public QObject {
    Q_OBJECT
    Q_PROPERTY(Interface *i READ interface WRITE setInterface NOTIFY interfaceChanged)
    Q_PROPERTY(int testValue READ testValue NOTIFY testValueChanged)

public:

    Interface* interface() const
    {
        return m_interface;
    }
    void setInterface(Interface* interface)
    {
        QObject* object = dynamic_cast<A*>(interface); // we know that we only get an A
        QVERIFY(object);
        m_testValue = object->property("i").toInt();
        emit testValueChanged();
        if (m_interface == interface)
            return;

        m_interface = interface;
        emit interfaceChanged();
    }

    int testValue() {
        return m_testValue;
    }

signals:
    void interfaceChanged();
    void testValueChanged();

private:
    Interface* m_interface = nullptr;
    int m_testValue = 0;
};


class InterfaceConsumer2 : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Interface2 *i READ interface WRITE setInterface NOTIFY interfaceChanged)
    Q_PROPERTY(int testValue READ testValue NOTIFY testValueChanged)

    QML_ELEMENT

public:

    Interface2* interface() const
    {
        return m_interface;
    }
    void setInterface(Interface2* interface2)
    {
        QObject* object = dynamic_cast<QObject*>(interface2);
        m_testValue = object->property("i").toInt();
        emit testValueChanged();
        if (m_interface == interface2)
            return;

        m_interface = interface2;
        emit interfaceChanged();
    }

    int testValue() {
        return m_testValue;
    }

signals:
    void interfaceChanged();
    void testValueChanged();

private:
    Interface2 *m_interface = nullptr;
    int m_testValue = 0;
};

#endif // INTERFACES_H
