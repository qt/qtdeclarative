/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QML preview debug service.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlpreviewposition.h"

#include <QtGui/qwindow.h>
#include <QtGui/qscreen.h>
#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE

static const QSize availableScreenSize(const QPoint &point)
{
    if (const QScreen *screen = QGuiApplication::screenAt(point))
        return screen->availableGeometry().size();
    return QSize();
}

QQmlPreviewPosition::QQmlPreviewPosition()
    : m_settings("QtProject", "QtQmlPreview")
{
    m_savePositionTimer.setSingleShot(true);
    m_savePositionTimer.setInterval(500);
    QObject::connect(&m_savePositionTimer, &QTimer::timeout, [this]() {
        saveWindowPosition();
    });
}

void QQmlPreviewPosition::setPosition(const QPoint &point)
{
    m_hasPosition = true;
    m_lastWindowPosition = point;
    m_savePositionTimer.start();
}

void QQmlPreviewPosition::saveWindowPosition()
{
    if (m_hasPosition) {
        if (!m_settingsKey.isNull())
            m_settings.setValue(m_settingsKey, m_lastWindowPosition);

        m_settings.setValue(QLatin1String("global_lastpostion"), m_lastWindowPosition);
    }
}

void QQmlPreviewPosition::loadWindowPositionSettings(const QUrl &url)
{
    m_settingsKey = url.toString(QUrl::PreferLocalFile) + QLatin1String("_lastpostion");

    if (m_settings.contains(m_settingsKey)) {
        m_hasPosition = true;
        m_lastWindowPosition = m_settings.value(m_settingsKey).toPoint();
    }
}

void QQmlPreviewPosition::initLastSavedWindowPosition(QWindow *window)
{
    if (m_positionedWindows.contains(window))
        return;
    if (!m_hasPosition) {
        // in case there was nothing saved, we do not want to set anything
        if (!m_settings.contains(QLatin1String("global_lastpostion")))
            return;
        m_lastWindowPosition = m_settings.value(QLatin1String("global_lastpostion")).toPoint();
    }
    if (QGuiApplication::screenAt(m_lastWindowPosition))
        window->setFramePosition(m_lastWindowPosition);

    m_positionedWindows.append(window);
}

const QSize QQmlPreviewPosition::currentScreenSize(QWindow *window)
{
    return availableScreenSize(window->position());
}

QT_END_NAMESPACE
