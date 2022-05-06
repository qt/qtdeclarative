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

#include "qquicklabsplatformiconloader_p.h"

#include <QtCore/qobject.h>
#include <QtCore/qmetaobject.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

QQuickLabsPlatformIconLoader::QQuickLabsPlatformIconLoader(int slot, QObject *parent)
    : m_parent(parent),
      m_slot(slot),
      m_enabled(false)
{
    Q_ASSERT(slot != -1 && parent);
}

bool QQuickLabsPlatformIconLoader::isEnabled() const
{
    return m_enabled;
}

void QQuickLabsPlatformIconLoader::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (m_enabled)
        loadIcon();
}

QIcon QQuickLabsPlatformIconLoader::toQIcon() const
{
    QIcon fallback = QPixmap::fromImage(image());
    QIcon icon = QIcon::fromTheme(m_icon.name(), fallback);
    icon.setIsMask(m_icon.isMask());
    return icon;
}

QQuickLabsPlatformIcon QQuickLabsPlatformIconLoader::icon() const
{
    return m_icon;
}

void QQuickLabsPlatformIconLoader::setIcon(const QQuickLabsPlatformIcon& icon)
{
    m_icon = icon;
    if (m_enabled)
        loadIcon();
}

void QQuickLabsPlatformIconLoader::loadIcon()
{
    if (m_icon.source().isEmpty()) {
        clear(m_parent);
    } else {
        load(qmlEngine(m_parent), m_icon.source());
        if (m_slot != -1 && isLoading()) {
            connectFinished(m_parent, m_slot);
            m_slot = -1;
        }
    }

    if (!isLoading())
        m_parent->metaObject()->method(m_slot).invoke(m_parent);
}

QT_END_NAMESPACE
