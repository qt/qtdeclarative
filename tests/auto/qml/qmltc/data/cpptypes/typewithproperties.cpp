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

#include "typewithproperties.h"

double TypeWithProperties::a() const
{
    return m_a;
}
QString TypeWithProperties::b() const
{
    return m_b;
}
QVariant TypeWithProperties::c() const
{
    return m_c;
}
int TypeWithProperties::d() const
{
    return m_d;
}

QJSValue TypeWithProperties::jsvalue() const
{
    return m_jsvalue;
}

void TypeWithProperties::setA(double a_)
{
    m_a = a_;
}
void TypeWithProperties::setB(const QString &b_)
{
    if (m_b != b_) {
        m_b = b_;
        Q_EMIT bChanged();
    }
}
void TypeWithProperties::setC(const QVariant &c_)
{
    if (m_c != c_) {
        m_c = c_;
        Q_EMIT cWeirdSignal(c_);
    }
}
void TypeWithProperties::setD(int d_)
{
    if (m_d != d_) {
        m_d = d_;
        Q_EMIT dSignal(u"d changed"_qs, d_);
    }
}
void TypeWithProperties::setJsValue(const QJSValue &value)
{
    m_jsvalue = value;
}

QBindable<double> TypeWithProperties::bindableA()
{
    return QBindable<double>(&m_a);
}
QBindable<int> TypeWithProperties::bindableD()
{
    return QBindable<int>(&m_d);
}
QBindable<QJSValue> TypeWithProperties::bindableJsValue()
{
    return QBindable<QJSValue>(&m_jsvalue);
}