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

#include "testgroupedtype.h"

TestTypeGrouped::TestTypeGrouped(QObject *parent) : QObject(parent), m_count(0) { }

int TestTypeGrouped::getCount() const
{
    return m_count.value();
}

void TestTypeGrouped::setCount(int v)
{
    m_count.setValue(v);
    emit countChanged();
}

QBindable<int> TestTypeGrouped::bindableCount()
{
    return QBindable<int>(&m_count);
}

int TestTypeGrouped::getFormula() const
{
    return m_formula.value();
}

void TestTypeGrouped::setFormula(int v)
{
    m_formula.setValue(v);
}

QBindable<int> TestTypeGrouped::bindableFormula()
{
    return QBindable<int>(&m_formula);
}

QObject *TestTypeGrouped::getObject() const
{
    return m_object.value();
}

void TestTypeGrouped::setObject(QObject *v)
{
    m_object.setValue(v);
}

QBindable<QObject *> TestTypeGrouped::bindableObject()
{
    return QBindable<QObject *>(&m_object);
}

QString TestTypeGrouped::getStr() const
{
    return m_str;
}

void TestTypeGrouped::setStr(const QString &s)
{
    m_str = s;
    emit strChanged();
}

QmlGroupPropertyTestType::QmlGroupPropertyTestType(QObject *parent) : QObject(parent) { }

TestTypeGrouped *QmlGroupPropertyTestType::getGroup()
{
    return &m_group;
}
