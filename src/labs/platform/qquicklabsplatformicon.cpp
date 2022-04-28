/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
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
******************************************************************************/

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
