// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "testactivitycommunicator.h"

using namespace QtJniTypes;

TestActivityCommunicator::TestActivityCommunicator(QObject *parent)
    : QObject{ parent },
      m_activity(TestActivity::callStaticMethod<TestActivity>("instance")),
      m_view(m_activity.callMethod<TestView>("testView"))
{
}

bool TestActivityCommunicator::getBoolProperty(const QString &name)
{
    return m_view.callMethod<Boolean>(getPropertyGetterName(name)).callMethod<bool>("booleanValue");
}

int TestActivityCommunicator::getIntProperty(const QString &name)
{
    return m_view.callMethod<Integer>(getPropertyGetterName(name)).callMethod<int>("intValue");
}

double TestActivityCommunicator::getDoubleProperty(const QString &name)
{
    return m_view.callMethod<Double>(getPropertyGetterName(name)).callMethod<double>("doubleValue");
}

QString TestActivityCommunicator::getStringProperty(const QString &name)
{
    return m_view.callMethod<String>(getPropertyGetterName(name)).toString();
}

void TestActivityCommunicator::setBoolProperty(const QString &name, bool value)
{
    m_view.callMethod<void>(getPropertySetterName(name), Boolean::construct(value));
}

void TestActivityCommunicator::setIntProperty(const QString &name, int value)
{
    m_view.callMethod<void>(getPropertySetterName(name), Integer::construct(value));
}

void TestActivityCommunicator::setDoubleProperty(const QString &name, double value)
{
    m_view.callMethod<void>(getPropertySetterName(name), Double::construct(value));
}

void TestActivityCommunicator::setStringProperty(const QString &name, const QString &value)
{
    m_view.callMethod<void>(getPropertySetterName(name), value);
}

QByteArray TestActivityCommunicator::getPropertyGetterName(const QString &name)
{
    return QByteArray("get") + name.at(0).toUpper().toLatin1() + name.mid(1).toLatin1();
}

QByteArray TestActivityCommunicator::getPropertySetterName(const QString &name)
{
    return QByteArray("set") + name.at(0).toUpper().toLatin1() + name.mid(1).toLatin1();
}
