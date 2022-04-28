/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "extensiontypes.h"

Extension::Extension(QObject *parent) : QObject(parent) { }
int Extension::getCount() const
{
    return m_extCount;
}
void Extension::setCount(int v)
{
    m_extCount = v;
}
QBindable<int> Extension::bindableCount()
{
    return QBindable<int>(&m_extCount);
}

double Extension::getFoo() const
{
    return m_foo;
}
void Extension::setFoo(double v)
{
    m_foo = v;
}
QBindable<double> Extension::bindableFoo()
{
    return QBindable<double>(&m_foo);
}

IndirectExtension::IndirectExtension(QObject *parent) : Extension(parent) { }

TypeWithExtension::TypeWithExtension(QObject *parent) : QObject(parent)
{
    m_count = TypeWithExtension::unsetCount;
}
int TypeWithExtension::getCount() const
{
    return m_count;
}
void TypeWithExtension::setCount(int v)
{
    m_count = v;
}
QBindable<int> TypeWithExtension::bindableCount()
{
    return QBindable<int>(&m_count);
}

Extension2::Extension2(QObject *parent) : QObject(parent) { }

QString Extension2::getStr() const
{
    return m_extStr;
}
void Extension2::setStr(QString v)
{
    m_extStr = v;
}
QBindable<QString> Extension2::bindableStr()
{
    return QBindable<QString>(&m_extStr);
}

const QString TypeWithExtensionDerived::unsetStr = QStringLiteral("unset");
TypeWithExtensionDerived::TypeWithExtensionDerived(QObject *parent) : TypeWithExtension(parent)
{
    m_str = TypeWithExtensionDerived::unsetStr;
}

QString TypeWithExtensionDerived::getStr() const
{
    return m_str;
}
void TypeWithExtensionDerived::setStr(QString v)
{
    m_str = v;
}
QBindable<QString> TypeWithExtensionDerived::bindableStr()
{
    return QBindable<QString>(&m_str);
}

TypeWithExtensionNamespace::TypeWithExtensionNamespace(QObject *parent) : QObject(parent)
{
    m_count = TypeWithExtension::unsetCount;
}

int TypeWithExtensionNamespace::getCount() const
{
    return m_count;
}
void TypeWithExtensionNamespace::setCount(int v)
{
    m_count = v;
}
QBindable<int> TypeWithExtensionNamespace::bindableCount()
{
    return QBindable<int>(&m_count);
}
