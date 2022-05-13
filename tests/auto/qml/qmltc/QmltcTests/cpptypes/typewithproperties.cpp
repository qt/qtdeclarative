// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "typewithproperties.h"

using namespace Qt::StringLiterals;

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
        Q_EMIT dSignal(u"d changed"_s, d_);
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
