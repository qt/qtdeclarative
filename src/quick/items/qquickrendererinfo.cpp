/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickrendererinfo_p.h"
#include "qquickwindow.h"
#include "qquickitem.h"
#include <qsgrendererinterface.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype RendererInfo
    \instantiates QQuickRendererInfo
    \inqmlmodule QtQuick
    \ingroup qtquick-effects
    \since 5.8
    \since QtQuick 2.8
    \brief Provides information about the used Qt Quick backend

    The RendererInfo attached type provides information about the scenegraph
    backend used to render the contents of the associated window.

    If the item to which the properties are attached is not currently
    associated with any window, the properties are set to default values. When
    the associated window changes, the properties will update.

    \sa OpenGLInfo, ShaderEffect
 */

QQuickRendererInfo::QQuickRendererInfo(QQuickItem *item)
    : QObject(item)
    , m_window(0)
    , m_api(Unknown)
{
    connect(item, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(setWindow(QQuickWindow*)));
    setWindow(item->window());
}

QQuickRendererInfo *QQuickRendererInfo::qmlAttachedProperties(QObject *object)
{
    if (QQuickItem *item = qobject_cast<QQuickItem *>(object))
        return new QQuickRendererInfo(item);

    return nullptr;
}

/*!
    \qmlproperty enumeration QtQuick::RendererInfo::api

    This property describes the graphics API that is currently in use.

    The possible values are:
    \list
    \li RendererInfo.Unknown - the default value when no active scenegraph is associated with the item
    \li RendererInfo.Software - Qt Quick's software renderer based on QPainter with the raster paint engine
    \li RendererInfo.OpenGL - OpenGL or OpenGL ES
    \li RendererInfo.Direct3D12 - Direct3D 12
    \endlist
 */

void QQuickRendererInfo::updateInfo()
{
    GraphicsApi newAPI = Unknown;

    if (m_window && m_window->isSceneGraphInitialized()) {
        QSGRendererInterface *rif = m_window->rendererInterface();
        if (rif)
            newAPI = GraphicsApi(rif->graphicsApi()); // this function is safe to call on the gui/main thread too
    }

    if (m_api != newAPI) {
        m_api = newAPI;
        emit apiChanged();
    }
}

void QQuickRendererInfo::setWindow(QQuickWindow *window)
{
    if (m_window != window) {
        if (m_window) {
            disconnect(m_window, SIGNAL(sceneGraphInitialized()), this, SLOT(updateInfo()));
            disconnect(m_window, SIGNAL(sceneGraphInvalidated()), this, SLOT(updateInfo()));
        }
        if (window) {
            connect(window, SIGNAL(sceneGraphInitialized()), this, SLOT(updateInfo()));
            connect(window, SIGNAL(sceneGraphInvalidated()), this, SLOT(updateInfo()));
        }
        m_window = window;
    }
    updateInfo();
}

QT_END_NAMESPACE
