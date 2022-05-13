// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpointhandler_p.h"
#include <private/qquickwindow_p.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PointHandler
    \instantiates QQuickPointHandler
    \inherits SinglePointHandler
    \inqmlmodule QtQuick
    \ingroup qtquick-input-handlers
    \brief Handler for reacting to a single touchpoint.

    PointHandler can be used to show feedback about a touchpoint or the mouse
    position, or to otherwise react to pointer events.

    When a press event occurs, each instance of PointHandler chooses a
    single point which is not yet "taken" at that moment: if the press
    occurs within the bounds of the \l {PointerHandler::parent}, and
    no sibling PointHandler within the same \l {PointerHandler::parent}
    has yet acquired a passive grab on that point, and if the other
    constraints such as \l {PointerDeviceHandler::acceptedButtons}{acceptedButtons}, \l {PointerDeviceHandler::acceptedDevices}{acceptedDevices} etc.
    are satisfied, it's
    eligible, and the PointHandler then acquires a passive grab. In
    this way, the \l {PointerHandler::parent} acts like an exclusive
    group: there can be multiple instances of PointHandler, and the
    set of pressed touchpoints will be distributed among them. Each
    PointHandler which has chosen a point to track has its \l active
    property \c true. It then continues to track its chosen point
    until release: the properties of the \l point will be kept
    up-to-date. Any Item can bind to these properties, and thereby
    follow the point's movements.

    By being only a passive grabber, it has the ability to keep independent
    oversight of all movements. The passive grab cannot be stolen or overridden
    even when other gestures are detected and exclusive grabs occur.

    If your goal is orthogonal surveillance of eventpoints, an older
    alternative was QObject::installEventFilter(), but that has never been a
    built-in QtQuick feature: it requires some C++ code, such as a QQuickItem
    subclass. PointHandler is more efficient than that, because only pointer
    events will be delivered to it, during the course of normal event delivery
    in QQuickWindow; whereas an event filter needs to filter all QEvents of all
    types, and thus sets itself up as a potential event delivery bottleneck.

    One possible use case is to add this handler to a transparent Item which is
    on top of the rest of the scene (by having a high \l{Item::z} {z} value),
    so that when a point is freshly pressed, it will be delivered to that Item
    and its handlers first, providing the opportunity to take the passive grab
    as early as possible. Such an item (like a pane of glass over the whole UI)
    can be a convenient parent for other Items which visualize the kind of reactive
    feedback which must always be on top; and likewise it can be the parent for
    popups, popovers, dialogs and so on. If it will be used in that way, it can
    be helpful for your main.cpp to use QQmlContext::setContextProperty() to
    make the "glass pane" accessible by ID to the entire UI, so that other
    Items and PointHandlers can be reparented to it.

    \snippet pointerHandlers/pointHandler.qml 0

    Like all input handlers, a PointHandler has a \l target property, which
    may be used as a convenient place to put a point-tracking Item; but
    PointHandler will not automatically manipulate the \c target item in any way.
    You need to use bindings to make it react to the \l point.

    \note On macOS, PointHandler does not react to the trackpad by default.
    That is because macOS can provide either native gesture recognition, or raw
    touchpoints, but not both. We prefer to use the native gesture event in
    PinchHandler, so we do not want to disable it by enabling touch. However
    MultiPointTouchArea does enable touch, thus disabling native gesture
    recognition within the entire window; so it's an alternative if you only
    want to react to all the touchpoints but do not require the smooth
    native-gesture experience.

    \sa MultiPointTouchArea
*/

QQuickPointHandler::QQuickPointHandler(QQuickItem *parent)
    : QQuickSinglePointHandler(parent)
{
    setIgnoreAdditionalPoints();
}

bool QQuickPointHandler::wantsEventPoint(const QPointerEvent *event, const QEventPoint &point)
{
    // On press, we want it unless a sibling of the same type also does.
    if (point.state() == QEventPoint::Pressed && QQuickSinglePointHandler::wantsEventPoint(event, point)) {
        for (const QObject *grabber : event->passiveGrabbers(point)) {
            if (grabber && grabber->parent() == parent() &&
                    grabber->metaObject()->className() == metaObject()->className())
                return false;
        }
        return true;
    }
    // If we've already been interested in a point, stay interested, even if it has strayed outside bounds.
    return (point.state() != QEventPoint::Pressed &&
            QQuickSinglePointHandler::point().id() == point.id());
}

void QQuickPointHandler::handleEventPoint(QPointerEvent *event, QEventPoint &point)
{
    switch (point.state()) {
    case QEventPoint::Pressed:
        if (QQuickDeliveryAgentPrivate::isTouchEvent(event) ||
                (static_cast<const QSinglePointEvent *>(event)->buttons() & acceptedButtons()) != Qt::NoButton) {
            setPassiveGrab(event, point);
            setActive(true);
        }
        break;
    case QEventPoint::Released:
        if (QQuickDeliveryAgentPrivate::isTouchEvent(event) ||
                (static_cast<const QSinglePointEvent *>(event)->buttons() & acceptedButtons()) == Qt::NoButton)
            setActive(false);
        break;
    default:
        break;
    }
    point.setAccepted(false); // Just lurking... don't interfere with propagation
    emit translationChanged();
    QQuickSinglePointHandler::handleEventPoint(event, point);
}

QVector2D QQuickPointHandler::translation() const
{
    return QVector2D(point().position() - point().pressPosition());
}

QT_END_NAMESPACE

#include "moc_qquickpointhandler_p.cpp"
