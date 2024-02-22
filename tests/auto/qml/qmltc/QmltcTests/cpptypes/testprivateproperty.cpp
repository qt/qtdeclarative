// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "private/testprivateproperty_p.h"

PrivatePropertyType::PrivatePropertyType(QObject *parent)
    : QObject(*(new PrivatePropertyTypePrivate), parent)
{
}

ValueTypeGroup PrivatePropertyTypePrivate::vt() const
{
    return m_vt;
}
void PrivatePropertyTypePrivate::setVt(ValueTypeGroup vt)
{
    m_vt = vt;
}

int PrivatePropertyTypePrivate::smth() const
{
    return m_smth;
}
void PrivatePropertyTypePrivate::setSmth(int s)
{
    m_smth = s;
}
QBindable<int> PrivatePropertyTypePrivate::bindableSmth()
{
    return QBindable<int>(&m_smth);
}

QString PrivatePropertyTypePrivate::foo() const
{
    return m_foo;
}
void PrivatePropertyTypePrivate::setFoo(const QString &foo)
{
    Q_Q(PrivatePropertyType);
    m_foo = foo;
    emit q->fooChanged();
}

TestTypeGrouped *PrivatePropertyTypePrivate::getGroup()
{
    return &m_group;
}
