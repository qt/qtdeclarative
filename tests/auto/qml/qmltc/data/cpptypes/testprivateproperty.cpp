/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
