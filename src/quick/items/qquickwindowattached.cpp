/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickwindow.h"
#include "qquickitem.h"
#include "qquickwindowattached_p.h"

QT_BEGIN_NAMESPACE

// QDoc comments must go in qquickwindow.cpp to avoid overwriting the Window docs

QQuickWindowAttached::QQuickWindowAttached(QObject* attachee)
    : QObject(attachee)
    , m_window(NULL)
{
    m_attachee = qobject_cast<QQuickItem*>(attachee);
    if (m_attachee && m_attachee->window()) // It might not be in a window yet
        windowChanged(m_attachee->window());
    if (m_attachee)
        connect(m_attachee, &QQuickItem::windowChanged, this, &QQuickWindowAttached::windowChanged);
}

QWindow::Visibility QQuickWindowAttached::visibility() const
{
    return (m_window ? m_window->visibility() : QWindow::Hidden);
}

bool QQuickWindowAttached::isActive() const
{
    return (m_window ? m_window->isActive() : false);
}

QQuickItem *QQuickWindowAttached::activeFocusItem() const
{
    return (m_window ? m_window->activeFocusItem() : Q_NULLPTR);
}

QQuickItem *QQuickWindowAttached::contentItem() const
{
    return (m_window ? m_window->contentItem() : Q_NULLPTR);
}

int QQuickWindowAttached::width() const
{
    return (m_window ? m_window->width() : 0);
}

int QQuickWindowAttached::height() const
{
    return (m_window ? m_window->height() : 0);
}

void QQuickWindowAttached::windowChanged(QQuickWindow *window)
{
    if (window != m_window) {
        QQuickWindow* oldWindow = m_window;
        m_window = window;

        if (oldWindow)
            oldWindow->disconnect(this);

        if (!window)
            return; // No values to get, therefore nothing to emit

        if (!oldWindow || window->visibility() != oldWindow->visibility())
            emit visibilityChanged();
        if (!oldWindow || window->isActive() != oldWindow->isActive())
            emit activeChanged();
        if (!oldWindow || window->activeFocusItem() != oldWindow->activeFocusItem())
            emit activeFocusItemChanged();
        emit contentItemChanged();
        if (!oldWindow || window->width() != oldWindow->width())
            emit widthChanged();
        if (!oldWindow || window->height() != oldWindow->height())
            emit heightChanged();

        // QQuickWindowQmlImpl::visibilityChanged also exists, and window might even
        // be QQuickWindowQmlImpl, but that's not what we are connecting to.
        // So this is actual window state rather than a buffered or as-requested one.
        // If we used the metaobject connect syntax there would be a warning:
        // QMetaObjectPrivate::indexOfSignalRelative - QMetaObject::indexOfSignal:
        // signal visibilityChanged(QWindow::Visibility) from QQuickWindow redefined in QQuickWindowQmlImpl
        connect(window, &QQuickWindow::visibilityChanged,
                this, &QQuickWindowAttached::visibilityChanged);
        connect(window, &QQuickWindow::activeChanged,
                this, &QQuickWindowAttached::activeChanged);
        connect(window, &QQuickWindow::activeFocusItemChanged,
                this, &QQuickWindowAttached::activeFocusItemChanged);
        connect(window, &QQuickWindow::widthChanged,
                this, &QQuickWindowAttached::widthChanged);
        connect(window, &QQuickWindow::heightChanged,
                this, &QQuickWindowAttached::heightChanged);
    }
}

QT_END_NAMESPACE
