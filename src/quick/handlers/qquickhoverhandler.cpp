/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qquickhoverhandler_p.h"
#include <private/qquicksinglepointhandler_p_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcHoverHandler, "qt.quick.handler.hover")

/*!
    \qmltype HoverHandler
    \instantiates QQuickHoverHandler
    \inherits SinglePointHandler
    \inqmlmodule QtQuick
    \ingroup qtquick-input-handlers
    \brief Handler for mouse and tablet hover.

    HoverHandler detects a hovering mouse or tablet stylus cursor.

    A binding to the \l hovered property is the easiest way to react when the
    cursor enters or leaves the \l {PointerHandler::parent}{parent} Item.
    The \l {SinglePointHandler::point}{point} property provides more detail,
    including the cursor position. The
    \l {PointerDeviceHandler::acceptedDevices}{acceptedDevices},
    \l {PointerDeviceHandler::acceptedPointerTypes}{acceptedPointerTypes},
    and \l {PointerDeviceHandler::acceptedModifiers}{acceptedModifiers}
    properties can be used to narrow the behavior to detect hovering of
    specific kinds of devices or while holding a modifier key.

    The \l cursorShape property allows changing the cursor whenever
    \l hovered changes to \c true.

    \sa MouseArea, PointHandler
*/

QQuickHoverHandler::QQuickHoverHandler(QQuickItem *parent)
    : QQuickSinglePointHandler(parent)
{
    // Tell QQuickPointerDeviceHandler::wantsPointerEvent() to ignore button state
    d_func()->acceptedButtons = Qt::NoButton;
}

QQuickHoverHandler::~QQuickHoverHandler()
{
    if (auto parent = parentItem())
        QQuickItemPrivate::get(parent)->setHasHoverInChild(false);
}

void QQuickHoverHandler::componentComplete()
{
    parentItem()->setAcceptHoverEvents(true);
    QQuickItemPrivate::get(parentItem())->setHasHoverInChild(true);
}

bool QQuickHoverHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    QQuickEventPoint *point = event->point(0);
    if (QQuickPointerDeviceHandler::wantsPointerEvent(event) && wantsEventPoint(point) && parentContains(point)) {
        // assume this is a mouse or tablet event, so there's only one point
        setPointId(point->pointId());
        return true;
    }

    // Some hover events come from QQuickWindow::tabletEvent(). In between,
    // some hover events come from QQWindowPrivate::flushFrameSynchronousEvents(),
    // but those look like mouse events. If a particular HoverHandler instance
    // is filtering for tablet events only (e.g. by setting
    // acceptedDevices:PointerDevice.Stylus), those events should not cause
    // the hovered property to transition to false prematurely.
    // If a QQuickPointerTabletEvent caused the hovered property to become true,
    // then only another QQuickPointerTabletEvent can make it become false.
    if (!(m_hoveredTablet && event->asPointerMouseEvent()))
        setHovered(false);

    return false;
}

void QQuickHoverHandler::handleEventPoint(QQuickEventPoint *point)
{
    bool hovered = true;
    if (point->state() == QQuickEventPoint::Released &&
            point->pointerEvent()->device()->pointerType() == QQuickPointerDevice::Finger)
        hovered = false;
    else if (point->pointerEvent()->asPointerTabletEvent())
        m_hoveredTablet = true;
    setHovered(hovered);
    setPassiveGrab(point);
}

/*!
    \qmlproperty bool QtQuick::HoverHandler::hovered
    \readonly

    Holds true whenever any pointing device cursor (mouse or tablet) is within
    the bounds of the \c parent Item, extended by the
    \l {PointerHandler::margin}{margin}, if any.
*/
void QQuickHoverHandler::setHovered(bool hovered)
{
    if (m_hovered != hovered) {
        qCDebug(lcHoverHandler) << objectName() << "hovered" << m_hovered << "->" << hovered;
        m_hovered = hovered;
        if (!hovered)
            m_hoveredTablet = false;
        emit hoveredChanged();
    }
}

/*!
    \since 5.15
    \qmlproperty Qt::CursorShape QtQuick::HoverHandler::cursorShape
    This property holds the cursor shape that will appear whenever
    \l hovered is \c true and no other handler is overriding it.

    The available cursor shapes are:
    \list
    \li Qt.ArrowCursor
    \li Qt.UpArrowCursor
    \li Qt.CrossCursor
    \li Qt.WaitCursor
    \li Qt.IBeamCursor
    \li Qt.SizeVerCursor
    \li Qt.SizeHorCursor
    \li Qt.SizeBDiagCursor
    \li Qt.SizeFDiagCursor
    \li Qt.SizeAllCursor
    \li Qt.BlankCursor
    \li Qt.SplitVCursor
    \li Qt.SplitHCursor
    \li Qt.PointingHandCursor
    \li Qt.ForbiddenCursor
    \li Qt.WhatsThisCursor
    \li Qt.BusyCursor
    \li Qt.OpenHandCursor
    \li Qt.ClosedHandCursor
    \li Qt.DragCopyCursor
    \li Qt.DragMoveCursor
    \li Qt.DragLinkCursor
    \endlist

    The default value of this property is not set, which allows any active
    handler on the same \l parentItem to determine the cursor shape.
    This property can be reset to the initial condition by setting it to
    \c undefined.

    If any handler with defined \c cursorShape is
    \l {PointerHandler::active}{active}, that cursor will appear.
    Else if the HoverHandler has a defined \c cursorShape, that cursor will appear.
    Otherwise, the \l {QQuickItem::cursor()}{cursor} of \l parentItem will appear.

    \note When this property has not been set, or has been set to \c undefined,
    if you read the value it will return \c Qt.ArrowCursor.

    \sa Qt::CursorShape, QQuickItem::cursor()
*/

QT_END_NAMESPACE
