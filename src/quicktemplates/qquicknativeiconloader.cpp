// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicknativeiconloader_p.h"

#include <QtCore/qobject.h>
#include <QtCore/qmetaobject.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

QQuickNativeIconLoader::QQuickNativeIconLoader(int slot, QObject *parent)
    : m_parent(parent),
      m_slot(slot),
      m_enabled(false)
{
    Q_ASSERT(slot != -1 && parent);
}

bool QQuickNativeIconLoader::isEnabled() const
{
    return m_enabled;
}

void QQuickNativeIconLoader::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (m_enabled)
        loadIcon();
}

QIcon QQuickNativeIconLoader::toQIcon() const
{
    const QIcon fallback = QPixmap::fromImage(image());
    return QIcon::fromTheme(m_icon.name(), fallback);
}

QQuickIcon QQuickNativeIconLoader::icon() const
{
    return m_icon;
}

void QQuickNativeIconLoader::setIcon(const QQuickIcon &icon)
{
    m_icon = icon;
    if (m_enabled)
        loadIcon();
}

void QQuickNativeIconLoader::loadIcon()
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
