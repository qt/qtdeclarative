// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicklabsplatformicon_p.h"

QT_BEGIN_NAMESPACE

QUrl QQuickLabsPlatformIcon::source() const
{
    return m_source;
}

void QQuickLabsPlatformIcon::setSource(const QUrl& source)
{
    m_source = source;
}

QString QQuickLabsPlatformIcon::name() const
{
    return m_name;
}

void QQuickLabsPlatformIcon::setName(const QString& name)
{
    m_name = name;
}

bool QQuickLabsPlatformIcon::isMask() const
{
    return m_mask;
}

void QQuickLabsPlatformIcon::setMask(bool mask)
{
    m_mask = mask;
}

bool QQuickLabsPlatformIcon::operator==(const QQuickLabsPlatformIcon &other) const
{
    return m_source == other.m_source && m_name == other.m_name && m_mask == other.m_mask;
}

bool QQuickLabsPlatformIcon::operator!=(const QQuickLabsPlatformIcon &other) const
{
    return !(*this == other);
}

QT_END_NAMESPACE

#include "moc_qquicklabsplatformicon_p.cpp"
