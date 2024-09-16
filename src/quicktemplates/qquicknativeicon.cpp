// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicknativeicon_p.h"

QT_BEGIN_NAMESPACE

QUrl QQuickNativeIcon::source() const
{
    return m_source;
}

void QQuickNativeIcon::setSource(const QUrl& source)
{
    m_source = source;
}

QString QQuickNativeIcon::name() const
{
    return m_name;
}

void QQuickNativeIcon::setName(const QString& name)
{
    m_name = name;
}

bool QQuickNativeIcon::isMask() const
{
    return m_mask;
}

void QQuickNativeIcon::setMask(bool mask)
{
    m_mask = mask;
}

bool QQuickNativeIcon::operator==(const QQuickNativeIcon &other) const
{
    return m_source == other.m_source && m_name == other.m_name && m_mask == other.m_mask;
}

bool QQuickNativeIcon::operator!=(const QQuickNativeIcon &other) const
{
    return !(*this == other);
}

QT_END_NAMESPACE

#include "moc_qquicknativeicon_p.cpp"
