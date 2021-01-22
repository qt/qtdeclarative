/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qqmltypemoduleversion_p.h"

#include <private/qqmltype_p.h>
#include <private/qqmltypemodule_p.h>

QT_BEGIN_NAMESPACE

QQmlTypeModuleVersion::QQmlTypeModuleVersion()
    : m_module(nullptr), m_minor(0)
{
}

QQmlTypeModuleVersion::QQmlTypeModuleVersion(QQmlTypeModule *module, int minor)
    : m_module(module), m_minor(minor)
{
    Q_ASSERT(m_module);
    Q_ASSERT(m_minor >= 0);
}

QQmlTypeModuleVersion::QQmlTypeModuleVersion(const QQmlTypeModuleVersion &o)
    : m_module(o.m_module), m_minor(o.m_minor)
{
}

QQmlTypeModuleVersion &QQmlTypeModuleVersion::operator=(const QQmlTypeModuleVersion &o)
{
    m_module = o.m_module;
    m_minor = o.m_minor;
    return *this;
}

QQmlTypeModule *QQmlTypeModuleVersion::module() const
{
    return m_module;
}

int QQmlTypeModuleVersion::minorVersion() const
{
    return m_minor;
}

QQmlType QQmlTypeModuleVersion::type(const QHashedStringRef &name) const
{
    if (!m_module)
        return QQmlType();
    return m_module->type(name, m_minor);
}

QQmlType QQmlTypeModuleVersion::type(const QV4::String *name) const
{
    if (!m_module)
        return QQmlType();
    return m_module->type(name, m_minor);
}

QT_END_NAMESPACE
