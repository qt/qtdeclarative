/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef INTERFACES_H
#define INTERFACES_H

#include <QtQml/qqml.h>

struct Interface {
};

QT_BEGIN_NAMESPACE
#define MyInterface_iid "io.qt.bugreports.Interface"
Q_DECLARE_INTERFACE(Interface, MyInterface_iid);
QT_END_NAMESPACE

class A : public QObject, Interface {
    Q_OBJECT
    Q_INTERFACES(Interface)
};

class B : public QObject, Interface {
    Q_OBJECT
    Q_INTERFACES(Interface)
};

class C : public QObject {
    Q_OBJECT
};

struct Interface2
{
    Q_GADGET
    QML_INTERFACE
};

QT_BEGIN_NAMESPACE
#define MyInterface2_iid "io.qt.bugreports.Interface2"
Q_DECLARE_INTERFACE(Interface2, MyInterface2_iid);
QT_END_NAMESPACE

class A2 : public QObject, Interface2 {
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(Interface2)
};

class B2 : public QObject, Interface2 {
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(Interface2)
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
        QObject* object = reinterpret_cast<QObject*>(interface);
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
        QObject* object = reinterpret_cast<QObject*>(interface2);
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
