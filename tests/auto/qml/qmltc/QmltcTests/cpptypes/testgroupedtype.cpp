// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

QmlGeneralizedGroupPropertyTestType::QmlGeneralizedGroupPropertyTestType(QObject *parent)
    : QObject(parent)
{
}

TestTypeGrouped *QmlGeneralizedGroupPropertyTestType::getGroup()
{
    return &m_group;
}
