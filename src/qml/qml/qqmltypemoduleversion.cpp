// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmltypemoduleversion_p.h"

QT_BEGIN_NAMESPACE

QQmlTypeModuleVersion::QQmlTypeModuleVersion()
    : m_module(nullptr), m_minor(0)
{
}

QQmlTypeModuleVersion::QQmlTypeModuleVersion(QQmlTypeModule *module, QTypeRevision version)
    : m_module(module), m_minor(version.minorVersion())
{
    Q_ASSERT(m_module);
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

QT_END_NAMESPACE
