/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qquickpointhandler_p.h"
#include <private/qquickwindow_p.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PointHandler
    \instantiates QQuickPointHandler
    \inqmlmodule QtQuick
    \ingroup qtquick-handlers
    \brief Handler for reacting to a single touchpoint

    PointHandler can be used to show feedback about a touchpoint or the mouse
    position, or to otherwise react to pointer events. By being only a passive
    grabber, it has the ability to keep oversight of all movements, and its
    grab cannot be stolen even when other gestures are detected and exclusive
    grabs occur.

    If your goal is orthogonal surveillance of eventpoints, an older
    alternative was QObject::installEventFilter(), but that has never been a
    built-in QtQuick feature: it requires a custom C++ QQuickItem subclass.
    PointHandler is more efficient than that, because only pointer events will
    be delivered to it, during the course of normal event delivery in
    QQuickWindow, whereas an event filter needs to filter all events of all
    types, and thus sets itself up as a potential event delivery bottleneck.

    One possible use case is to add this handler to a transparent Item which is
    on top of the rest of the scene, so that when a point is freshly pressed,
    it will be delivered to that Item and its handlers first, providing the
    opportunity to take the passive grab as early as possible. Then such an
    item (like a pane of glass over the whole UI) also becomes a good parent
    for other Items which visualize the kind of reactive feedback which must
    always be on top; and likewise it can be the parent for popups, popovers,
    dialogs and so on. For example, a declared Window can have an Item with a
    high Z value so that it stays on top. It can also be helpful for your
    main.cpp to use QQmlContext::setContextProperty() to make the "glass pane"
    accessible by ID to the entire UI, so that other Items and PointHandlers
    can be reparented to it.

    Inside a PointHandler you can declare a \l target Item, but PointHandler
    will not automatically manipulate it in any way. The target Item can bind to
    properties of the PointHandler. In this way it can follow a point's movements.

    \sa MultiPointTouchArea
*/

QQuickPointHandler::QQuickPointHandler(QObject *parent)
    : QQuickSinglePointHandler(parent)
{
    setIgnoreAdditionalPoints();
}

QQuickPointHandler::~QQuickPointHandler()
{
}

bool QQuickPointHandler::wantsEventPoint(QQuickEventPoint *pt)
{
    // On press, we want it unless a sibling of the same type also does.
    if (pt->state() == QQuickEventPoint::Pressed && QQuickSinglePointHandler::wantsEventPoint(pt)) {
        for (const QQuickPointerHandler *grabber : pt->passiveGrabbers()) {
            if (grabber && grabber->parent() == parent() &&
                    grabber->metaObject()->className() == metaObject()->className())
                return false;
        }
        return true;
    }
    // If we've already been interested in a point, stay interested, even if it has strayed outside bounds.
    return (pt->state() != QQuickEventPoint::Pressed && point().id() == pt->pointId());
}

void QQuickPointHandler::handleEventPoint(QQuickEventPoint *point)
{
    switch (point->state()) {
    case QQuickEventPoint::Pressed:
        setPassiveGrab(point);
        setActive(true);
        break;
    case QQuickEventPoint::Released:
        setActive(false);
        break;
    default:
        break;
    }
    point->setAccepted(false); // Just lurking... don't interfere with propagation
    emit translationChanged();
}

QVector2D QQuickPointHandler::translation() const
{
    return QVector2D(point().position() - point().pressPosition());
}

QT_END_NAMESPACE
