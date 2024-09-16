// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "testattachedtype.h"

TestTypeAttached::TestTypeAttached(QObject *parent) : QObject(parent), m_count(0)
{
    ++creationCount;
}

int TestTypeAttached::creationCount = 0;

int TestTypeAttached::getAttachedCount() const
{
    return m_count.value();
}

void TestTypeAttached::setAttachedCount(int v)
{
    m_count.setValue(v);
    emit attachedCountChanged();
}

QBindable<int> TestTypeAttached::bindableAttachedCount()
{
    return QBindable<int>(&m_count);
}

int TestTypeAttached::getAttachedFormula() const
{
    return m_formula.value();
}

void TestTypeAttached::setAttachedFormula(int v)
{
    m_formula.setValue(v);
}

QBindable<int> TestTypeAttached::bindableAttachedFormula()
{
    return QBindable<int>(&m_formula);
}

QObject *TestTypeAttached::getAttachedObject() const
{
    return m_object.value();
}
void TestTypeAttached::setAttachedObject(QObject *v)
{
    m_object.setValue(v);
}
QBindable<QObject *> TestTypeAttached::bindableAttachedObject()
{
    return QBindable<QObject *>(&m_object);
}

TestType::TestType(QObject *parent) : QObject(parent) { }

TestTypeAttached *TestType::qmlAttachedProperties(QObject *parent)
{
    return new TestTypeAttached(parent);
}
