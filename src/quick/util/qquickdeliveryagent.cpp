/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/private/qabstractanimationjob_p.h>
#include <QtQuick/private/qquickdrag_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickhoverhandler_p.h>
#include <QtQuick/private/qquickpointerhandler_p.h>
#include <QtQuick/private/qquickpointerhandler_p_p.h>
#include <QtQuick/private/qquickprofiler_p.h>
#include <QtQuick/private/qquickrendercontrol_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qsgrenderloop_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTouch, "qt.quick.touch")
Q_LOGGING_CATEGORY(lcTouchCmprs, "qt.quick.touch.compression")
Q_LOGGING_CATEGORY(lcTouchTarget, "qt.quick.touch.target")
Q_LOGGING_CATEGORY(lcMouse, "qt.quick.mouse")
Q_LOGGING_CATEGORY(lcMouseTarget, "qt.quick.mouse.target")
Q_LOGGING_CATEGORY(lcPtr, "qt.quick.pointer")
Q_LOGGING_CATEGORY(lcPtrGrab, "qt.quick.pointer.grab")
Q_LOGGING_CATEGORY(lcWheelTarget, "qt.quick.wheel.target")
Q_LOGGING_CATEGORY(lcGestureTarget, "qt.quick.gesture.target")
Q_LOGGING_CATEGORY(lcHoverTrace, "qt.quick.hover.trace")
Q_LOGGING_CATEGORY(lcFocus, "qt.quick.focus")

extern Q_GUI_EXPORT bool qt_sendShortcutOverrideEvent(QObject *o, ulong timestamp, int k, Qt::KeyboardModifiers mods, const QString &text = QString(), bool autorep = false, ushort count = 1);

void QQuickWindowPrivate::touchToMouseEvent(QEvent::Type type, const QEventPoint &p, const QTouchEvent *touchEvent, QMutableSinglePointEvent *mouseEvent)
{
    Q_ASSERT(QCoreApplication::testAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents));
    QMutableSinglePointEvent ret(type, touchEvent->pointingDevice(), p,
                                 (type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton),
                                 (type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton),
                                 touchEvent->modifiers(), Qt::MouseEventSynthesizedByQt);
    ret.setAccepted(true); // this now causes the persistent touchpoint to be accepted too
    *mouseEvent = ret;
}

bool QQuickWindowPrivate::checkIfDoubleTapped(ulong newPressEventTimestamp, QPoint newPressPos)
{
    bool doubleClicked = false;

    if (touchMousePressTimestamp > 0) {
        QPoint distanceBetweenPresses = newPressPos - touchMousePressPos;
        const int doubleTapDistance = QGuiApplication::styleHints()->touchDoubleTapDistance();
        doubleClicked = (qAbs(distanceBetweenPresses.x()) <= doubleTapDistance) && (qAbs(distanceBetweenPresses.y()) <= doubleTapDistance);

        if (doubleClicked) {
            ulong timeBetweenPresses = newPressEventTimestamp - touchMousePressTimestamp;
            ulong doubleClickInterval = static_cast<ulong>(QGuiApplication::styleHints()->
                    mouseDoubleClickInterval());
            doubleClicked = timeBetweenPresses < doubleClickInterval;
        }
    }
    if (doubleClicked) {
        touchMousePressTimestamp = 0;
    } else {
        touchMousePressTimestamp = newPressEventTimestamp;
        touchMousePressPos = newPressPos;
    }

    return doubleClicked;
}

QPointerEvent *QQuickWindowPrivate::eventInDelivery() const
{
    if (eventsInDelivery.isEmpty())
        return nullptr;
    return eventsInDelivery.top();
}

/*! \internal
    A helper function for the benefit of obsolete APIs like QQuickItem::grabMouse()
    that don't have the currently-being-delivered event in context.
    Returns the device the currently-being-delivered event comse from.
*/
QPointingDevicePrivate::EventPointData *QQuickWindowPrivate::mousePointData()
{
    if (eventsInDelivery.isEmpty())
        return nullptr;
    auto devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice*>(eventsInDelivery.top()->pointingDevice()));
    return devPriv->pointById(isDeliveringTouchAsMouse() ? touchMouseId : 0);
}

void QQuickWindowPrivate::cancelTouchMouseSynthesis()
{
    qCDebug(lcTouchTarget) << "id" << touchMouseId << "on" << touchMouseDevice;
    touchMouseId = -1;
    touchMouseDevice = nullptr;
}

bool QQuickWindowPrivate::deliverTouchAsMouse(QQuickItem *item, QTouchEvent *pointerEvent)
{
    Q_ASSERT(QCoreApplication::testAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents));
    auto device = pointerEvent->pointingDevice();

    // A touch event from a trackpad is likely to be followed by a mouse or gesture event, so mouse event synth is redundant
    if (device->type() == QInputDevice::DeviceType::TouchPad && device->capabilities().testFlag(QInputDevice::Capability::MouseEmulation)) {
        qCDebug(lcTouchTarget) << "skipping delivery of synth-mouse event from" << device;
        return false;
    }

    // FIXME: make this work for mouse events too and get rid of the asTouchEvent in here.
    QMutableTouchEvent event;
    QQuickItemPrivate::get(item)->localizedTouchEvent(pointerEvent, false, &event);
    if (!event.points().count())
        return false;

    // For each point, check if it is accepted, if not, try the next point.
    // Any of the fingers can become the mouse one.
    // This can happen because a mouse area might not accept an event at some point but another.
    for (auto &p : event.points()) {
        // A new touch point
        if (touchMouseId == -1 && p.state() & QEventPoint::State::Pressed) {
            QPointF pos = item->mapFromScene(p.scenePosition());

            // probably redundant, we check bounds in the calling function (matchingNewPoints)
            if (!item->contains(pos))
                break;

            qCDebug(lcTouchTarget) << device << "TP (mouse)" << Qt::hex << p.id() << "->" << item;
            QMutableSinglePointEvent mousePress;
            touchToMouseEvent(QEvent::MouseButtonPress, p, &event, &mousePress);

            // Send a single press and see if that's accepted
            QCoreApplication::sendEvent(item, &mousePress);
            event.setAccepted(mousePress.isAccepted());
            if (mousePress.isAccepted()) {
                touchMouseDevice = device;
                touchMouseId = p.id();
                const auto &pt = mousePress.point(0);
                if (!mousePress.exclusiveGrabber(pt))
                    mousePress.setExclusiveGrabber(pt, item);

                if (checkIfDoubleTapped(event.timestamp(), p.globalPosition().toPoint())) {
                    // since we synth the mouse event from from touch, we respect the
                    // QPlatformTheme::TouchDoubleTapDistance instead of QPlatformTheme::MouseDoubleClickDistance
                    QMutableSinglePointEvent mouseDoubleClick;
                    touchToMouseEvent(QEvent::MouseButtonDblClick, p, &event, &mouseDoubleClick);
                    QCoreApplication::sendEvent(item, &mouseDoubleClick);
                    event.setAccepted(mouseDoubleClick.isAccepted());
                    if (!mouseDoubleClick.isAccepted())
                        cancelTouchMouseSynthesis();
                }

                return true;
            }
            // try the next point

        // Touch point was there before and moved
        } else if (touchMouseDevice == device && p.id() == touchMouseId) {
            if (p.state() & QEventPoint::State::Updated) {
                if (touchMousePressTimestamp != 0) {
                    const int doubleTapDistance = QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::TouchDoubleTapDistance).toInt();
                    const QPoint moveDelta = p.globalPosition().toPoint() - touchMousePressPos;
                    if (moveDelta.x() >= doubleTapDistance || moveDelta.y() >= doubleTapDistance)
                        touchMousePressTimestamp = 0;   // Got dragged too far, dismiss the double tap
                }
                if (QQuickItem *mouseGrabberItem = qmlobject_cast<QQuickItem *>(pointerEvent->exclusiveGrabber(p))) {
                    QMutableSinglePointEvent me;
                    touchToMouseEvent(QEvent::MouseMove, p, &event, &me);
                    QCoreApplication::sendEvent(item, &me);
                    event.setAccepted(me.isAccepted());
                    if (me.isAccepted())
                        qCDebug(lcTouchTarget) << device << "TP (mouse)" << Qt::hex << p.id() << "->" << mouseGrabberItem;
                    return event.isAccepted();
                } else {
                    // no grabber, check if we care about mouse hover
                    // FIXME: this should only happen once, not recursively... I'll ignore it just ignore hover now.
                    // hover for touch???
                    QMutableSinglePointEvent me;
                    touchToMouseEvent(QEvent::MouseMove, p, &event, &me);
                    if (lastMousePosition.isNull())
                        lastMousePosition = me.scenePosition();
                    QPointF last = lastMousePosition;
                    lastMousePosition = me.scenePosition();

                    bool accepted = me.isAccepted();
                    bool delivered = deliverHoverEvent(contentItem, me.scenePosition(), last, me.modifiers(), me.timestamp(), accepted);
                    // take care of any exits
                    if (!delivered)
                        clearHover(me.timestamp());
                    break;
                }
            } else if (p.state() & QEventPoint::State::Released) {
                // currently handled point was released
                if (QQuickItem *mouseGrabberItem = qmlobject_cast<QQuickItem *>(pointerEvent->exclusiveGrabber(p))) {
                    QMutableSinglePointEvent me;
                    touchToMouseEvent(QEvent::MouseButtonRelease, p, &event, &me);
                    QCoreApplication::sendEvent(item, &me);

                    if (item->acceptHoverEvents() && p.globalPosition() != QGuiApplicationPrivate::lastCursorPosition) {
                        QPointF localMousePos(qInf(), qInf());
                        if (QWindow *w = item->window())
                            localMousePos = item->mapFromScene(w->mapFromGlobal(QGuiApplicationPrivate::lastCursorPosition.toPoint()));
                        QMouseEvent mm(QEvent::MouseMove, localMousePos, QGuiApplicationPrivate::lastCursorPosition,
                                       Qt::NoButton, Qt::NoButton, event.modifiers());
                        QCoreApplication::sendEvent(item, &mm);
                    }
                    if (pointerEvent->exclusiveGrabber(p) == mouseGrabberItem) // might have ungrabbed due to event
                        pointerEvent->setExclusiveGrabber(p, nullptr);

                    cancelTouchMouseSynthesis();
                    return me.isAccepted();
                }
            }
            break;
        }
    }
    return false;
}

/*!
    Ungrabs all touchpoint grabs and/or the mouse grab from the given item \a grabber.
    This should not be called when processing a release event - that's redundant.
    It is called in other cases, when the points may not be released, but the item
    nevertheless must lose its grab due to becoming disabled, invisible, etc.
    QPointerEvent::setExclusiveGrabber() calls touchUngrabEvent() when all points are released,
    but if not all points are released, it cannot be sure whether to call touchUngrabEvent()
    or not; so we have to do it here.
*/
void QQuickWindowPrivate::removeGrabber(QQuickItem *grabber, bool mouse, bool touch, bool cancel)
{
    Q_Q(QQuickWindow);
    if (eventsInDelivery.isEmpty()) {
        // do it the expensive way
        for (auto dev : knownPointingDevices) {
            auto devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(dev));
            devPriv->removeGrabber(grabber, cancel);
        }
        return;
    }
    auto eventInDelivery = eventsInDelivery.top();
    if (Q_LIKELY(mouse) && q->mouseGrabberItem() == grabber && eventInDelivery) {
        const bool fromTouch = isDeliveringTouchAsMouse();
        auto point = eventInDelivery->pointById(fromTouch ? touchMouseId : 0);
        Q_ASSERT(point);
        QQuickItem *oldGrabber = qobject_cast<QQuickItem *>(eventInDelivery->exclusiveGrabber(*point));
        qCDebug(lcMouseTarget) << "removeGrabber" << oldGrabber << "-> null";
        eventInDelivery->setExclusiveGrabber(*point, nullptr);
    }
    if (Q_LIKELY(touch)) {
        bool ungrab = false;
        const auto touchDevices = QPointingDevice::devices();
        for (auto device : touchDevices) {
            if (device->type() != QInputDevice::DeviceType::TouchScreen)
                continue;
            if (QPointingDevicePrivate::get(const_cast<QPointingDevice *>(static_cast<const QPointingDevice *>(device)))->
                    removeExclusiveGrabber(eventInDelivery, grabber))
                ungrab = true;
        }
        if (ungrab)
            grabber->touchUngrabEvent();
    }
}

/*! \internal
    Translates QEventPoint::scenePosition() in \a touchEvent to this window.

    The item-local QEventPoint::position() is updated later, not here.
*/
void QQuickWindowPrivate::translateTouchEvent(QTouchEvent *touchEvent)
{
    for (qsizetype i = 0; i != touchEvent->pointCount(); ++i) {
        auto &pt = QMutableEventPoint::from(touchEvent->point(i));
        pt.setScenePosition(pt.position());
    }
}


static inline bool windowHasFocus(QQuickWindow *win)
{
    const QWindow *focusWindow = QGuiApplication::focusWindow();
    return win == focusWindow || QQuickRenderControl::renderWindowFor(win) == focusWindow;
}

#ifdef Q_OS_WEBOS
// Temporary fix for webOS until multi-seat is implemented see QTBUG-85272
static inline bool singleWindowOnScreen(QQuickWindow *win)
{
    const QWindowList windowList = QGuiApplication::allWindows();
    for (int i = 0; i < windowList.count(); i++) {
        QWindow *ii = windowList.at(i);
        if (ii == win)
            continue;
        if (ii->screen() == win->screen())
            return false;
    }

    return true;
}
#endif

/*!
Set the focus inside \a scope to be \a item.
If the scope contains the active focus item, it will be changed to \a item.
Calls notifyFocusChangesRecur for all changed items.
*/
void QQuickWindowPrivate::setFocusInScope(QQuickItem *scope, QQuickItem *item, Qt::FocusReason reason, FocusOptions options)
{
    Q_Q(QQuickWindow);

    Q_ASSERT(item);
    Q_ASSERT(scope || item == contentItem);

    qCDebug(lcFocus) << "QQuickWindowPrivate::setFocusInScope():";
    qCDebug(lcFocus) << "    scope:" << (QObject *)scope;
    if (scope)
        qCDebug(lcFocus) << "    scopeSubFocusItem:" << (QObject *)QQuickItemPrivate::get(scope)->subFocusItem;
    qCDebug(lcFocus) << "    item:" << (QObject *)item;
    qCDebug(lcFocus) << "    activeFocusItem:" << (QObject *)activeFocusItem;

    QQuickItemPrivate *scopePrivate = scope ? QQuickItemPrivate::get(scope) : nullptr;
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);

    QQuickItem *oldActiveFocusItem = nullptr;
    QQuickItem *currentActiveFocusItem = activeFocusItem;
    QQuickItem *newActiveFocusItem = nullptr;
    bool sendFocusIn = false;

    lastFocusReason = reason;

    QVarLengthArray<QQuickItem *, 20> changed;

    // Does this change the active focus?
    if (item == contentItem || scopePrivate->activeFocus) {
        oldActiveFocusItem = activeFocusItem;
        if (item->isEnabled()) {
            newActiveFocusItem = item;
            while (newActiveFocusItem->isFocusScope()
                   && newActiveFocusItem->scopedFocusItem()
                   && newActiveFocusItem->scopedFocusItem()->isEnabled()) {
                newActiveFocusItem = newActiveFocusItem->scopedFocusItem();
            }
        } else {
            newActiveFocusItem = scope;
        }

        if (oldActiveFocusItem) {
#if QT_CONFIG(im)
            QGuiApplication::inputMethod()->commit();
#endif

            activeFocusItem = nullptr;

            QQuickItem *afi = oldActiveFocusItem;
            while (afi && afi != scope) {
                if (QQuickItemPrivate::get(afi)->activeFocus) {
                    QQuickItemPrivate::get(afi)->activeFocus = false;
                    changed << afi;
                }
                afi = afi->parentItem();
            }
        }
    }

    if (item != contentItem && !(options & DontChangeSubFocusItem)) {
        QQuickItem *oldSubFocusItem = scopePrivate->subFocusItem;
        if (oldSubFocusItem) {
            QQuickItemPrivate::get(oldSubFocusItem)->focus = false;
            changed << oldSubFocusItem;
        }

        QQuickItemPrivate::get(item)->updateSubFocusItem(scope, true);
    }

    if (!(options & DontChangeFocusProperty)) {
        if (item != contentItem
                || windowHasFocus(q)
#ifdef Q_OS_WEBOS
        // Allow focused if there is only one window in the screen where it belongs.
        // Temporary fix for webOS until multi-seat is implemented see QTBUG-85272
                || singleWindowOnScreen(q)
#endif
                ) {
            itemPrivate->focus = true;
            changed << item;
        }
    }

    if (newActiveFocusItem && contentItem->hasFocus()) {
        activeFocusItem = newActiveFocusItem;

        QQuickItemPrivate::get(newActiveFocusItem)->activeFocus = true;
        changed << newActiveFocusItem;

        QQuickItem *afi = newActiveFocusItem->parentItem();
        while (afi && afi != scope) {
            if (afi->isFocusScope()) {
                QQuickItemPrivate::get(afi)->activeFocus = true;
                changed << afi;
            }
            afi = afi->parentItem();
        }
        updateFocusItemTransform();
        sendFocusIn = true;
    }

    // Now that all the state is changed, emit signals & events
    // We must do this last, as this process may result in further changes to focus.
    if (oldActiveFocusItem) {
        QFocusEvent event(QEvent::FocusOut, reason);
        QCoreApplication::sendEvent(oldActiveFocusItem, &event);
    }

    // Make sure that the FocusOut didn't result in another focus change.
    if (sendFocusIn && activeFocusItem == newActiveFocusItem) {
        QFocusEvent event(QEvent::FocusIn, reason);
        QCoreApplication::sendEvent(newActiveFocusItem, &event);
    }

    if (activeFocusItem != currentActiveFocusItem)
        emit q->focusObjectChanged(activeFocusItem);

    if (!changed.isEmpty())
        notifyFocusChangesRecur(changed.data(), changed.count() - 1);
}

void QQuickWindowPrivate::clearFocusInScope(QQuickItem *scope, QQuickItem *item, Qt::FocusReason reason, FocusOptions options)
{
    Q_Q(QQuickWindow);

    Q_ASSERT(item);
    Q_ASSERT(scope || item == contentItem);

    qCDebug(lcFocus) << "QQuickWindowPrivate::clearFocusInScope():";
    qCDebug(lcFocus) << "    scope:" << (QObject *)scope;
    qCDebug(lcFocus) << "    item:" << (QObject *)item;
    qCDebug(lcFocus) << "    activeFocusItem:" << (QObject *)activeFocusItem;

    QQuickItemPrivate *scopePrivate = nullptr;
    if (scope) {
        scopePrivate = QQuickItemPrivate::get(scope);
        if ( !scopePrivate->subFocusItem )
            return;//No focus, nothing to do.
    }

    QQuickItem *currentActiveFocusItem = activeFocusItem;
    QQuickItem *oldActiveFocusItem = nullptr;
    QQuickItem *newActiveFocusItem = nullptr;

    lastFocusReason = reason;

    QVarLengthArray<QQuickItem *, 20> changed;

    Q_ASSERT(item == contentItem || item == scopePrivate->subFocusItem);

    // Does this change the active focus?
    if (item == contentItem || scopePrivate->activeFocus) {
        oldActiveFocusItem = activeFocusItem;
        newActiveFocusItem = scope;

#if QT_CONFIG(im)
        QGuiApplication::inputMethod()->commit();
#endif

        activeFocusItem = nullptr;

        if (oldActiveFocusItem) {
            QQuickItem *afi = oldActiveFocusItem;
            while (afi && afi != scope) {
                if (QQuickItemPrivate::get(afi)->activeFocus) {
                    QQuickItemPrivate::get(afi)->activeFocus = false;
                    changed << afi;
                }
                afi = afi->parentItem();
            }
        }
    }

    if (item != contentItem && !(options & DontChangeSubFocusItem)) {
        QQuickItem *oldSubFocusItem = scopePrivate->subFocusItem;
        if (oldSubFocusItem && !(options & DontChangeFocusProperty)) {
            QQuickItemPrivate::get(oldSubFocusItem)->focus = false;
            changed << oldSubFocusItem;
        }

        QQuickItemPrivate::get(item)->updateSubFocusItem(scope, false);

    } else if (!(options & DontChangeFocusProperty)) {
        QQuickItemPrivate::get(item)->focus = false;
        changed << item;
    }

    if (newActiveFocusItem) {
        Q_ASSERT(newActiveFocusItem == scope);
        activeFocusItem = scope;
        updateFocusItemTransform();
    }

    // Now that all the state is changed, emit signals & events
    // We must do this last, as this process may result in further changes to
    // focus.
    if (oldActiveFocusItem) {
        QFocusEvent event(QEvent::FocusOut, reason);
        QCoreApplication::sendEvent(oldActiveFocusItem, &event);
    }

    // Make sure that the FocusOut didn't result in another focus change.
    if (newActiveFocusItem && activeFocusItem == newActiveFocusItem) {
        QFocusEvent event(QEvent::FocusIn, reason);
        QCoreApplication::sendEvent(newActiveFocusItem, &event);
    }

    if (activeFocusItem != currentActiveFocusItem)
        emit q->focusObjectChanged(activeFocusItem);

    if (!changed.isEmpty())
        notifyFocusChangesRecur(changed.data(), changed.count() - 1);
}

void QQuickWindowPrivate::clearFocusObject()
{
    if (activeFocusItem == contentItem)
        return;

    clearFocusInScope(contentItem, QQuickItemPrivate::get(contentItem)->subFocusItem, Qt::OtherFocusReason);
}

void QQuickWindowPrivate::notifyFocusChangesRecur(QQuickItem **items, int remaining)
{
    QPointer<QQuickItem> item(*items);

    if (remaining)
        notifyFocusChangesRecur(items + 1, remaining - 1);

    if (item) {
        QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);

        if (itemPrivate->notifiedFocus != itemPrivate->focus) {
            itemPrivate->notifiedFocus = itemPrivate->focus;
            emit item->focusChanged(itemPrivate->focus);
        }

        if (item && itemPrivate->notifiedActiveFocus != itemPrivate->activeFocus) {
            itemPrivate->notifiedActiveFocus = itemPrivate->activeFocus;
            itemPrivate->itemChange(QQuickItem::ItemActiveFocusHasChanged, itemPrivate->activeFocus);
            emit item->activeFocusChanged(itemPrivate->activeFocus);
        }
    }
}

bool QQuickWindowPrivate::clearHover(ulong timestamp)
{
    Q_Q(QQuickWindow);
    if (hoverItems.isEmpty())
        return false;

    QPointF pos = q->mapFromGlobal(QGuiApplicationPrivate::lastCursorPosition.toPoint());

    bool accepted = false;
    for (QQuickItem* item : qAsConst(hoverItems)) {
        accepted = sendHoverEvent(QEvent::HoverLeave, item, pos, pos, QGuiApplication::keyboardModifiers(), timestamp, true) || accepted;
    }
    hoverItems.clear();
    return accepted;
}

/*! \reimp */
bool QQuickWindow::event(QEvent *e)
{
    Q_D(QQuickWindow);

    switch (e->type()) {

    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        QTouchEvent *touch = static_cast<QTouchEvent*>(e);
        d->handleTouchEvent(touch);
        if (Q_LIKELY(QCoreApplication::testAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents))) {
            // we consume all touch events ourselves to avoid duplicate
            // mouse delivery by QtGui mouse synthesis
            e->accept();
        }
        return true;
    }
        break;
    case QEvent::TouchCancel:
        // return in order to avoid the QWindow::event below
        return d->deliverTouchCancelEvent(static_cast<QTouchEvent*>(e));
        break;
    case QEvent::Enter: {
        if (!d->contentItem)
            return false;
        QEnterEvent *enter = static_cast<QEnterEvent*>(e);
        bool accepted = enter->isAccepted();
        bool delivered = d->deliverHoverEvent(d->contentItem, enter->scenePosition(), d->lastMousePosition,
            QGuiApplication::keyboardModifiers(), 0L, accepted);
        d->lastMousePosition = enter->scenePosition();
        enter->setAccepted(accepted);
#if QT_CONFIG(cursor)
        d->updateCursor(mapFromGlobal(QCursor::pos()));
#endif
        return delivered;
    }
        break;
    case QEvent::Leave:
        d->clearHover();
        d->lastMousePosition = QPointF();
        break;
#if QT_CONFIG(quick_draganddrop)
    case QEvent::DragEnter:
    case QEvent::DragLeave:
    case QEvent::DragMove:
    case QEvent::Drop:
        d->deliverDragEvent(d->dragGrabber, e);
        break;
#endif
    case QEvent::WindowDeactivate:
        d->handleWindowDeactivate();
        break;
    case QEvent::PlatformSurface:
        if ((static_cast<QPlatformSurfaceEvent *>(e))->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            // Ensure that the rendering thread is notified before
            // the QPlatformWindow is destroyed.
            if (d->windowManager)
                d->windowManager->hide(this);
        }
        break;
    case QEvent::FocusAboutToChange:
#if QT_CONFIG(im)
        if (d->activeFocusItem)
            qGuiApp->inputMethod()->commit();
#endif
        break;
    case QEvent::UpdateRequest: {
        if (d->windowManager)
            d->windowManager->handleUpdateRequest(this);
        break;
    }
#if QT_CONFIG(gestures)
    case QEvent::NativeGesture:
        d->deliverSinglePointEventUntilAccepted(static_cast<QPointerEvent *>(e));
        break;
#endif
    case QEvent::ShortcutOverride:
        if (d->activeFocusItem)
            QCoreApplication::sendEvent(d->activeFocusItem, e);
        return true;
    case QEvent::LanguageChange:
        if (d->contentItem)
            QCoreApplication::sendEvent(d->contentItem, e);
        break;
    case QEvent::InputMethod:
    case QEvent::InputMethodQuery:
        {
            QQuickItem *target = d->activeFocusItem;
            // while an input method delivers the event, this window might still be inactive
            if (!target) {
                target = d->contentItem;
                if (!target || !target->isEnabled())
                    break;
                // see setFocusInScope for a similar loop
                while (target->isFocusScope() && target->scopedFocusItem() && target->scopedFocusItem()->isEnabled())
                    target = target->scopedFocusItem();
            }
            if (target) {
                QCoreApplication::sendEvent(target, e);
                return true;
            }
        }
        break;
    default:
        break;
    }

    if (e->type() == QEvent::Type(QQuickWindowPrivate::FullUpdateRequest))
        update();
    else if (e->type() == QEvent::Type(QQuickWindowPrivate::TriggerContextCreationFailure))
        d->windowManager->handleContextCreationFailure(this);

    return QWindow::event(e);
}

void QQuickWindowPrivate::deliverKeyEvent(QKeyEvent *e)
{
    if (activeFocusItem) {
        QQuickItem *item = activeFocusItem;

        // In case of generated event, trigger ShortcutOverride event
        if (e->type() == QEvent::KeyPress && e->spontaneous() == false)
                qt_sendShortcutOverrideEvent(item, e->timestamp(),
                                         e->key(), e->modifiers(), e->text(),
                                         e->isAutoRepeat(), e->count());

        e->accept();
        QCoreApplication::sendEvent(item, e);
        while (!e->isAccepted() && (item = item->parentItem())) {
            e->accept();
            QCoreApplication::sendEvent(item, e);
        }
    }
}

/*! \internal
    Make a copy of any type of QPointerEvent, and optionally localize it
    by setting its first point's local position() if \a transformedLocalPos is given.

    \note some subclasses of QSinglePointEvent, such as QWheelEvent, add extra storage.
    This function doesn't yet support cloning all of those; it can be extended if needed.
*/
QPointerEvent *QQuickWindowPrivate::clonePointerEvent(QPointerEvent *event, std::optional<QPointF> transformedLocalPos)
{
    QPointerEvent *ret = event->clone();
    QMutableEventPoint &point = QMutableEventPoint::from(ret->point(0));
    point.detach();
    point.setTimestamp(event->timestamp());
    if (transformedLocalPos)
        point.setPosition(*transformedLocalPos);

    return ret;
}

void QQuickWindowPrivate::deliverToPassiveGrabbers(const QVector<QPointer <QObject> > &passiveGrabbers,
                                                   QPointerEvent *pointerEvent)
{
    const QVector<QObject *> &eventDeliveryTargets =
            QQuickPointerHandlerPrivate::deviceDeliveryTargets(pointerEvent->device());
    QVarLengthArray<QPair<QQuickItem *, bool>, 4> sendFilteredPointerEventResult;
    hasFiltered.clear();
    for (auto o : passiveGrabbers) {
        QQuickPointerHandler *handler = qobject_cast<QQuickPointerHandler *>(o);
        // a null pointer in passiveGrabbers is unlikely, unless the grabbing handler was deleted dynamically
        if (Q_LIKELY(handler) && !eventDeliveryTargets.contains(handler)) {
            bool alreadyFiltered = false;
            QQuickItem *par = handler->parentItem();

            // see if we already have sent a filter event to the parent
            auto it = std::find_if(sendFilteredPointerEventResult.begin(), sendFilteredPointerEventResult.end(),
                                        [par](const QPair<QQuickItem *, bool> &pair) { return pair.first == par; });
            if (it != sendFilteredPointerEventResult.end()) {
                // Yes, the event was already filtered to that parent, do not call it again but use
                // the result of the previous call to determine if we should call the handler.
                alreadyFiltered = it->second;
            } else {
                alreadyFiltered = sendFilteredPointerEvent(pointerEvent, par);
                sendFilteredPointerEventResult << qMakePair(par, alreadyFiltered);
            }
            if (!alreadyFiltered) {
                localizePointerEvent(pointerEvent, handler->parentItem());
                handler->handlePointerEvent(pointerEvent);
            }
        }
    }
}

bool QQuickWindowPrivate::sendHoverEvent(QEvent::Type type, QQuickItem *item,
                                      const QPointF &scenePos, const QPointF &lastScenePos,
                                      Qt::KeyboardModifiers modifiers, ulong timestamp,
                                      bool accepted)
{
    const QTransform transform = QQuickItemPrivate::get(item)->windowToItemTransform();

    //create copy of event
    QHoverEvent hoverEvent(type, transform.map(scenePos), transform.map(lastScenePos), modifiers);
    hoverEvent.setTimestamp(timestamp);
    hoverEvent.setAccepted(accepted);

    hasFiltered.clear();
    if (sendFilteredMouseEvent(&hoverEvent, item, item->parentItem()))
        return true;

    QCoreApplication::sendEvent(item, &hoverEvent);

    return hoverEvent.isAccepted();
}

// TODO later: specify the device in case of multi-mouse scenario, or mouse and tablet both in use
bool QQuickWindowPrivate::deliverHoverEvent(QQuickItem *item, const QPointF &scenePos, const QPointF &lastScenePos,
                                         Qt::KeyboardModifiers modifiers, ulong timestamp, bool &accepted)
{
    Q_Q(QQuickWindow);
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);

    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(scenePos);
        if (!item->contains(p))
            return false;
    }

    if (Q_UNLIKELY(lcHoverTrace().isDebugEnabled())) {
        if (lastScenePos == scenePos)
            qCDebug(lcHoverTrace) << scenePos << "(unchanged)" << item << "subtreeHoverEnabled" << itemPrivate->subtreeHoverEnabled << "in window" << windowTitle;
        else
            qCDebug(lcHoverTrace) << lastScenePos << "->" << scenePos << item << "subtreeHoverEnabled" << itemPrivate->subtreeHoverEnabled << "in window" << windowTitle;
    }
    if (itemPrivate->subtreeHoverEnabled) {
        QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();
        for (int ii = children.count() - 1; ii >= 0; --ii) {
            QQuickItem *child = children.at(ii);
            if (!child->isVisible() || QQuickItemPrivate::get(child)->culled)
                continue;
            if (!child->isEnabled() && !QQuickItemPrivate::get(child)->subtreeHoverEnabled)
                continue;
            if (deliverHoverEvent(child, scenePos, lastScenePos, modifiers, timestamp, accepted))
                return true;
        }
    }

    if (itemPrivate->hasPointerHandlers()) {
        const QPointF localPos = item->mapFromScene(scenePos);
        QMouseEvent hoverEvent(QEvent::MouseMove, localPos, scenePos, q->mapToGlobal(scenePos), Qt::NoButton, Qt::NoButton, modifiers);
        hoverEvent.setTimestamp(timestamp);
        hoverEvent.setAccepted(true);
        for (QQuickPointerHandler *h : itemPrivate->extra->pointerHandlers)
            if (QQuickHoverHandler *hh = qmlobject_cast<QQuickHoverHandler *>(h))
                hh->handlePointerEvent(&hoverEvent);
    }

    if (itemPrivate->hoverEnabled) {
        QPointF p = item->mapFromScene(scenePos);
        if (item->contains(p)) {
            if (!hoverItems.isEmpty() && hoverItems.at(0) == item) {
                //move
                accepted = sendHoverEvent(QEvent::HoverMove, item, scenePos, lastScenePos, modifiers, timestamp, accepted);
            } else {
                QList<QQuickItem *> itemsToHover;
                QQuickItem* parent = item;
                itemsToHover << item;
                while ((parent = parent->parentItem()))
                    itemsToHover << parent;

                // Leaving from previous hovered items until we reach the item or one of its ancestors.
                while (!hoverItems.isEmpty() && !itemsToHover.contains(hoverItems.at(0))) {
                    QQuickItem *hoverLeaveItem = hoverItems.takeFirst();
                    sendHoverEvent(QEvent::HoverLeave, hoverLeaveItem, scenePos, lastScenePos, modifiers, timestamp, accepted);
                }

                if (!hoverItems.isEmpty() && hoverItems.at(0) == item) {//Not entering a new Item
                    // ### Shouldn't we send moves for the parent items as well?
                    accepted = sendHoverEvent(QEvent::HoverMove, item, scenePos, lastScenePos, modifiers, timestamp, accepted);
                } else {
                    // Enter items that are not entered yet.
                    int startIdx = -1;
                    if (!hoverItems.isEmpty())
                        startIdx = itemsToHover.indexOf(hoverItems.at(0)) - 1;
                    if (startIdx == -1)
                        startIdx = itemsToHover.count() - 1;

                    for (int i = startIdx; i >= 0; i--) {
                        QQuickItem *itemToHover = itemsToHover.at(i);
                        QQuickItemPrivate *itemToHoverPrivate = QQuickItemPrivate::get(itemToHover);
                        // The item may be about to be deleted or reparented to another window
                        // due to another hover event delivered in this function. If that is the
                        // case, sending a hover event here will cause a crash or other bad
                        // behavior when the leave event is generated. Checking
                        // itemToHoverPrivate->window here prevents that case.
                        if (itemToHoverPrivate->window == q && itemToHoverPrivate->hoverEnabled) {
                            hoverItems.prepend(itemToHover);
                            sendHoverEvent(QEvent::HoverEnter, itemToHover, scenePos, lastScenePos, modifiers, timestamp, accepted);
                        }
                    }
                }
            }
            return true;
        }
    }

    return false;
}

// Simple delivery of non-mouse, non-touch Pointer Events: visit the items and handlers
// in the usual reverse-paint-order until propagation is stopped
bool QQuickWindowPrivate::deliverSinglePointEventUntilAccepted(QPointerEvent *event)
{
    Q_ASSERT(event->points().count() == 1);
    QQuickPointerHandlerPrivate::deviceDeliveryTargets(event->pointingDevice()).clear();
    QEventPoint &point = event->point(0);
    QVector<QQuickItem *> targetItems = pointerTargets(contentItem, event, point, false, false);
    point.setAccepted(false);

    for (QQuickItem *item : targetItems) {
        QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
        localizePointerEvent(event, item);
        // Let Pointer Handlers have the first shot
        itemPrivate->handlePointerEvent(event);
        if (point.isAccepted())
            return true;
        event->accept();
        QCoreApplication::sendEvent(item, event);
        if (event->isAccepted()) {
            qCDebug(lcWheelTarget) << event << "->" << item;
            return true;
        }
    }

    return false; // it wasn't handled
}

bool QQuickWindowPrivate::deliverTouchCancelEvent(QTouchEvent *event)
{
    qCDebug(lcTouch) << event;

    // An incoming TouchCancel event will typically not contain any points,
    // but sendTouchCancelEvent() adds the points that have grabbers to the event.
    // Deliver it to all items and handlers that have active touches.
    const_cast<QPointingDevicePrivate *>(QPointingDevicePrivate::get(event->pointingDevice()))->
            sendTouchCancelEvent(event);
    cancelTouchMouseSynthesis();
    return true;
}

void QQuickWindowPrivate::deliverDelayedTouchEvent()
{
    // Deliver and delete delayedTouch.
    // Set delayedTouch to nullptr before delivery to avoid redelivery in case of
    // event loop recursions (e.g if it the touch starts a dnd session).
    QScopedPointer<QTouchEvent> e(delayedTouch.take());
    qCDebug(lcTouchCmprs) << "delivering" << e.data();
    deliverPointerEvent(e.data());
}

/*! \internal
    The handler for the QEvent::WindowDeactivate event, and also when
    Qt::ApplicationState tells us the application is no longer active.
    It clears all exclusive grabs of items and handlers whose window is this one,
    for all known pointing devices.

    The QEvent is not passed into this function because in the first case it's
    just a plain QEvent with no extra data, and because the application state
    change is delivered via a signal rather than an event.
*/
void QQuickWindowPrivate::handleWindowDeactivate()
{
    Q_Q(QQuickWindow);
    qCDebug(lcFocus) << "deactivated" << windowTitle;
    const auto inputDevices = QInputDevice::devices();
    for (auto device : inputDevices) {
        if (auto pointingDevice = qobject_cast<const QPointingDevice *>(device)) {
            auto devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(pointingDevice));
            for (auto epd : devPriv->activePoints.values()) {
                if (!epd.exclusiveGrabber.isNull()) {
                    bool relevant = false;
                    if (QQuickItem *item = qmlobject_cast<QQuickItem *>(epd.exclusiveGrabber.data()))
                        relevant = (item->window() == q);
                    else if (QQuickPointerHandler *handler = qmlobject_cast<QQuickPointerHandler *>(epd.exclusiveGrabber.data()))
                        relevant = (handler->parentItem()->window() == q);
                    if (relevant)
                        devPriv->setExclusiveGrabber(nullptr, epd.eventPoint, nullptr);
                }
                // For now, we don't clearPassiveGrabbers(), just in case passive grabs
                // can be useful to keep monitoring the mouse even after window deactivation.
            }
        }
    }
}

bool QQuickWindowPrivate::allUpdatedPointsAccepted(const QPointerEvent *ev)
{
    for (auto &point : ev->points()) {
        if (point.state() != QEventPoint::State::Pressed && !point.isAccepted())
            return false;
    }
    return true;
}

/*! \internal
    Localize \a ev for delivery to \a dest.

    Unlike QMutableTouchEvent::localized(), this modifies the QEventPoint
    instances in \a ev, which is more efficient than making a copy.
*/
void QQuickWindowPrivate::localizePointerEvent(QPointerEvent *ev, const QQuickItem *dest)
{
    for (int i = 0; i < ev->pointCount(); ++i) {
        auto &point = QMutableEventPoint::from(ev->point(i));
        QMutableEventPoint::from(point).setPosition(dest->mapFromScene(point.scenePosition()));
    }
}

QList<QObject *> QQuickWindowPrivate::exclusiveGrabbers(QPointerEvent *ev)
{
    QList<QObject *> result;
    for (const QEventPoint &point : ev->points()) {
        if (QObject *grabber = ev->exclusiveGrabber(point)) {
            if (!result.contains(grabber))
                result << grabber;
        }
    }
    return result;
}

bool QQuickWindowPrivate::isMouseEvent(const QPointerEvent *ev)
{
    switch (ev->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
        return true;
    default:
        return false;
    }
}

bool QQuickWindowPrivate::isTouchEvent(const QPointerEvent *ev)
{
    switch (ev->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        return true;
    default:
        return false;
    }
}

bool QQuickWindowPrivate::isTabletEvent(const QPointerEvent *ev)
{
    switch (ev->type()) {
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
    case QEvent::TabletEnterProximity:
    case QEvent::TabletLeaveProximity:
        return true;
    default:
        return false;
    }
}

bool QQuickWindowPrivate::compressTouchEvent(QTouchEvent *event)
{
    Q_Q(QQuickWindow);
    QEventPoint::States states = event->touchPointStates();
    if (states.testFlag(QEventPoint::State::Pressed) || states.testFlag(QEventPoint::State::Released)) {
        // we can only compress an event that doesn't include any pressed or released points
        return false;
    }

    if (!delayedTouch) {
        delayedTouch.reset(new QMutableTouchEvent(event->type(), event->pointingDevice(), event->modifiers(), event->points()));
        delayedTouch->setTimestamp(event->timestamp());
        qCDebug(lcTouchCmprs) << "delayed" << delayedTouch.data();
        if (renderControl)
            QQuickRenderControlPrivate::get(renderControl)->maybeUpdate();
        else if (windowManager)
            windowManager->maybeUpdate(q);
        return true;
    }

    // check if this looks like the last touch event
    if (delayedTouch->type() == event->type() &&
            delayedTouch->device() == event->device() &&
            delayedTouch->modifiers() == event->modifiers() &&
            delayedTouch->pointCount() == event->pointCount())
    {
        // possible match.. is it really the same?
        bool mismatch = false;

        auto tpts = event->points();
        for (qsizetype i = 0; i < event->pointCount(); ++i) {
            const auto &tp = tpts.at(i);
            const auto &tpDelayed = delayedTouch->point(i);
            if (tp.id() != tpDelayed.id()) {
                mismatch = true;
                break;
            }

            if (tpDelayed.state() == QEventPoint::State::Updated && tp.state() == QEventPoint::State::Stationary)
                QMutableEventPoint::from(tpts[i]).setState(QEventPoint::State::Updated);
        }

        // matching touch event? then give delayedTouch a merged set of touchpoints
        if (!mismatch) {
            // have to create a new event because QMutableTouchEvent::setTouchPoints() is missing
            // TODO optimize, or move event compression elsewhere
            delayedTouch.reset(new QMutableTouchEvent(event->type(), event->pointingDevice(), event->modifiers(), tpts));
            delayedTouch->setTimestamp(event->timestamp());
            qCDebug(lcTouchCmprs) << "coalesced" << delayedTouch.data();
            return true;
        }
    }

    // merging wasn't possible, so deliver the delayed event first, and then delay this one
    deliverDelayedTouchEvent();
    delayedTouch.reset(new QMutableTouchEvent(event->type(), event->pointingDevice(),
                                       event->modifiers(), event->points()));
    delayedTouch->setTimestamp(event->timestamp());
    return true;
}

// entry point for touch event delivery:
// - translate the event to window coordinates
// - compress the event instead of delivering it if applicable
// - call deliverTouchPoints to actually dispatch the points
void QQuickWindowPrivate::handleTouchEvent(QTouchEvent *event)
{
    translateTouchEvent(event);
    // TODO remove: touch and mouse should be independent until we come to touch->mouse synth
    if (event->pointCount()) {
        auto &point = event->point(0);
        if (point.state() == QEventPoint::State::Released) {
            lastMousePosition = QPointF();
        } else {
            lastMousePosition = point.position();
        }
    }

    qCDebug(lcTouch) << event;

    static bool qquickwindow_no_touch_compression = qEnvironmentVariableIsSet("QML_NO_TOUCH_COMPRESSION");

    if (qquickwindow_no_touch_compression || pointerEventRecursionGuard) {
        deliverPointerEvent(event);
        return;
    }

    if (!compressTouchEvent(event)) {
        if (delayedTouch) {
            deliverDelayedTouchEvent();
            qCDebug(lcTouchCmprs) << "resuming delivery" << event;
        }
        deliverPointerEvent(event);
    }
}

void QQuickWindowPrivate::handleMouseEvent(QMouseEvent *event)
{
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        event->accept();
        return;
    }
    qCDebug(lcMouse) << event;

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        Q_QUICK_INPUT_PROFILE(QQuickProfiler::Mouse, QQuickProfiler::InputMousePress, event->button(),
                              event->buttons());
        deliverPointerEvent(event);
        break;
    case QEvent::MouseButtonRelease:
        Q_QUICK_INPUT_PROFILE(QQuickProfiler::Mouse, QQuickProfiler::InputMouseRelease, event->button(),
                              event->buttons());
        deliverPointerEvent(event);
#if QT_CONFIG(cursor)
        updateCursor(event->scenePosition());
#endif
        break;
    case QEvent::MouseButtonDblClick:
        Q_QUICK_INPUT_PROFILE(QQuickProfiler::Mouse, QQuickProfiler::InputMouseDoubleClick,
                              event->button(), event->buttons());
        if (allowDoubleClick)
            deliverPointerEvent(event);
        break;
    case QEvent::MouseMove: {
        Q_QUICK_INPUT_PROFILE(QQuickProfiler::Mouse, QQuickProfiler::InputMouseMove,
                              event->position().x(), event->position().y());

        qCDebug(lcHoverTrace) << this;

    #if QT_CONFIG(cursor)
        updateCursor(event->scenePosition());
    #endif
        if (!event->points().count() || !event->exclusiveGrabber(event->point(0))) {
            QPointF last = lastMousePosition.isNull() ? event->scenePosition() : lastMousePosition;
            lastMousePosition = event->scenePosition();

            bool accepted = event->isAccepted();
            bool delivered = deliverHoverEvent(contentItem, event->scenePosition(), last, event->modifiers(), event->timestamp(), accepted);
            if (!delivered) {
                //take care of any exits
                accepted = clearHover(event->timestamp());
            }
            event->setAccepted(accepted);
        }
        deliverPointerEvent(event);
        break;
    }
    default:
        Q_ASSERT(false);
        break;
    }
}

void QQuickWindowPrivate::flushFrameSynchronousEvents()
{
    Q_Q(QQuickWindow);

    if (delayedTouch) {
        deliverDelayedTouchEvent();

        // Touch events which constantly start animations (such as a behavior tracking
        // the mouse point) need animations to start.
        QQmlAnimationTimer *ut = QQmlAnimationTimer::instance();
        if (ut && ut->hasStartAnimationPending())
            ut->startAnimations();
    }

    // In webOS we already have the alternative to the issue that this
    // wanted to address and thus skipping this part won't break anything.
#if !defined(Q_OS_WEBOS)
    // Once per frame, if any items are dirty, send a synthetic hover,
    // in case items have changed position, visibility, etc.
    // For instance, during animation (including the case of a ListView
    // whose delegates contain MouseAreas), a MouseArea needs to know
    // whether it has moved into a position where it is now under the cursor.
    // TODO do this for each known mouse device or come up with a different strategy
    if (!q->mouseGrabberItem() && !lastMousePosition.isNull() && dirtyItemList) {
        bool accepted = false;
        bool delivered = deliverHoverEvent(contentItem, lastMousePosition, lastMousePosition, QGuiApplication::keyboardModifiers(), 0, accepted);
        if (!delivered)
            clearHover(); // take care of any exits
    }
#endif
}

void QQuickWindowPrivate::onGrabChanged(QObject *grabber, QPointingDevice::GrabTransition transition,
                                        const QPointerEvent *event, const QEventPoint &point)
{
    qCDebug(lcPtrGrab) << grabber << transition << event << point;
    // note: event can be null, if the signal was emitted from QPointingDevicePrivate::removeGrabber(grabber)
    if (auto *handler = qmlobject_cast<QQuickPointerHandler *>(grabber)) {
        handler->onGrabChanged(handler, transition, const_cast<QPointerEvent *>(event),
                               const_cast<QEventPoint &>(point));
    } else {
        switch (transition) {
        case QPointingDevice::CancelGrabExclusive:
        case QPointingDevice::UngrabExclusive:
            if (auto *item = qmlobject_cast<QQuickItem *>(grabber)) {
                bool filtered = false;
                if (isDeliveringTouchAsMouse() ||
                        point.device()->type() == QInputDevice::DeviceType::Mouse ||
                        point.device()->type() == QInputDevice::DeviceType::TouchPad) {
                    QMutableSinglePointEvent e(QEvent::UngrabMouse, point.device(), point);
                    hasFiltered.clear();
                    filtered = sendFilteredMouseEvent(&e, item, item->parentItem());
                    if (!filtered) {
                        lastUngrabbed = item;
                        item->mouseUngrabEvent();
                    }
                }
                if (point.device()->type() == QInputDevice::DeviceType::TouchScreen) {
                    bool allReleasedOrCancelled = true;
                    if (transition == QPointingDevice::UngrabExclusive && event) {
                        for (const auto &pt : event->points()) {
                            if (pt.state() != QEventPoint::State::Released) {
                                allReleasedOrCancelled = false;
                                break;
                            }
                        }
                    }
                    if (allReleasedOrCancelled)
                        item->touchUngrabEvent();
                }
            }
            break;
        default:
            break;
        }
    }
}

void QQuickWindowPrivate::ensureDeviceConnected(const QPointingDevice *dev)
{
    if (knownPointingDevices.contains(dev))
        return;
    knownPointingDevices.append(dev);
    connect(dev, &QPointingDevice::grabChanged, this, &QQuickWindowPrivate::onGrabChanged);
}

void QQuickWindowPrivate::deliverPointerEvent(QPointerEvent *event)
{
    // If users spin the eventloop as a result of event delivery, we disable
    // event compression and send events directly. This is because we consider
    // the usecase a bit evil, but we at least don't want to lose events.
    ++pointerEventRecursionGuard;
    eventsInDelivery.push(event);

    skipDelivery.clear();
    QQuickPointerHandlerPrivate::deviceDeliveryTargets(event->pointingDevice()).clear();
    qCDebug(lcPtr) << "delivering" << event;
    for (int i = 0; i < event->pointCount(); ++i)
        event->point(i).setAccepted(false);

    if (event->isBeginEvent()) {
        ensureDeviceConnected(event->pointingDevice());
        if (!deliverPressOrReleaseEvent(event))
            event->setAccepted(false);
    }
    if (!allUpdatedPointsAccepted(event))
        deliverUpdatedPoints(event);
    if (event->isEndEvent())
        deliverPressOrReleaseEvent(event, true);

    // failsafe: never allow any kind of grab to persist after release
    if (event->isEndEvent()) {
        if (isTouchEvent(event)) {
            for (int i = 0; i < event->pointCount(); ++i) {
                auto &point = event->point(i);
                if (point.state() == QEventPoint::State::Released) {
                    event->setExclusiveGrabber(point, nullptr);
                    event->clearPassiveGrabbers(point);
                }
            }
            // never allow touch->mouse synthesis to persist either
            cancelTouchMouseSynthesis();
        } else if (static_cast<QSinglePointEvent *>(event)->buttons() == Qt::NoButton) {
            auto &firstPt = event->point(0);
            event->setExclusiveGrabber(firstPt, nullptr);
            event->clearPassiveGrabbers(firstPt);
        }
    }

    eventsInDelivery.pop();
    --pointerEventRecursionGuard;
    lastUngrabbed = nullptr;
}

// check if item or any of its child items contain the point, or if any pointer handler "wants" the point
// FIXME: should this be iterative instead of recursive?
// If checkMouseButtons is true, it means we are finding targets for a mouse event, so no item for which acceptedMouseButtons() is NoButton will be added.
// If checkAcceptsTouch is true, it means we are finding targets for a touch event, so either acceptTouchEvents() must return true OR
// it must accept a synth. mouse event, thus if acceptTouchEvents() returns false but acceptedMouseButtons() is true, gets added; if not, it doesn't.
QVector<QQuickItem *> QQuickWindowPrivate::pointerTargets(QQuickItem *item, const QPointerEvent *event, const QEventPoint &point,
                                                          bool checkMouseButtons, bool checkAcceptsTouch) const
{
    QVector<QQuickItem *> targets;
    auto itemPrivate = QQuickItemPrivate::get(item);
    QPointF itemPos = item->mapFromScene(point.scenePosition());
    bool relevant = item->contains(itemPos);
    // if the item clips, we can potentially return early
    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        if (!relevant)
            return targets;
    }

    if (itemPrivate->hasPointerHandlers()) {
        if (!relevant)
            if (itemPrivate->anyPointerHandlerWants(event, point))
                relevant = true;
    } else {
        if (relevant && checkMouseButtons && item->acceptedMouseButtons() == Qt::NoButton)
            relevant = false;
        if (relevant && checkAcceptsTouch && !(item->acceptTouchEvents() || item->acceptedMouseButtons()))
            relevant = false;
    }

    QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();
    if (relevant) {
        auto it = std::lower_bound(children.begin(), children.end(), 0,
           [](auto lhs, auto rhs) -> bool { return lhs->z() < rhs; });
        children.insert(it, item);
    }

    for (int ii = children.count() - 1; ii >= 0; --ii) {
        QQuickItem *child = children.at(ii);
        auto childPrivate = QQuickItemPrivate::get(child);
        if (!child->isVisible() || !child->isEnabled() || childPrivate->culled)
            continue;

        if (child != item)
            targets << pointerTargets(child, event, point, checkMouseButtons, checkAcceptsTouch);
        else
            targets << child;
    }

    return targets;
}

// return the joined lists
// list1 has priority, common items come last
QVector<QQuickItem *> QQuickWindowPrivate::mergePointerTargets(const QVector<QQuickItem *> &list1, const QVector<QQuickItem *> &list2) const
{
    QVector<QQuickItem *> targets = list1;
    // start at the end of list2
    // if item not in list, append it
    // if item found, move to next one, inserting before the last found one
    int insertPosition = targets.length();
    for (int i = list2.length() - 1; i >= 0; --i) {
        int newInsertPosition = targets.lastIndexOf(list2.at(i), insertPosition);
        if (newInsertPosition >= 0) {
            Q_ASSERT(newInsertPosition <= insertPosition);
            insertPosition = newInsertPosition;
        }
        // check for duplicates, only insert if the item isn't there already
        if (insertPosition == targets.size() || list2.at(i) != targets.at(insertPosition))
            targets.insert(insertPosition, list2.at(i));
    }
    return targets;
}

/*! \internal
    Deliver updated points to existing grabbers.
*/
void QQuickWindowPrivate::deliverUpdatedPoints(QPointerEvent *event)
{
    bool done = false;
    const auto grabbers = exclusiveGrabbers(event);
    hasFiltered.clear();
    for (auto grabber : grabbers) {
        // The grabber is guaranteed to be either an item or a handler.
        QQuickItem *receiver = qmlobject_cast<QQuickItem *>(grabber);
        if (!receiver) {
            // The grabber is not an item? It's a handler then.  Let it have the event first.
            QQuickPointerHandler *handler = static_cast<QQuickPointerHandler *>(grabber);
            receiver = static_cast<QQuickPointerHandler *>(grabber)->parentItem();
            hasFiltered.clear();
            if (sendFilteredPointerEvent(event, receiver))
                done = true;
            localizePointerEvent(event, receiver);
            handler->handlePointerEvent(event);
        }
        if (done)
            break;
        // If the grabber is an item or the grabbing handler didn't handle it,
        // then deliver the event to the item (which may have multiple handlers).
        hasFiltered.clear();
        deliverMatchingPointsToItem(receiver, true, event);
    }

    // Deliver to each eventpoint's passive grabbers (but don't visit any handler more than once)
    for (auto &point : event->points())
        deliverToPassiveGrabbers(event->passiveGrabbers(point), event);

    if (done)
        return;

    // If some points weren't grabbed, deliver only to non-grabber PointerHandlers in reverse paint order
    if (!event->allPointsGrabbed()) {
        QVector<QQuickItem *> targetItems;
        for (auto &point : event->points()) {
            // Presses were delivered earlier; not the responsibility of deliverUpdatedTouchPoints.
            // Don't find handlers for points that are already grabbed by an Item (such as Flickable).
            if (point.state() == QEventPoint::Pressed || qmlobject_cast<QQuickItem *>(event->exclusiveGrabber(point)))
                continue;
            QVector<QQuickItem *> targetItemsForPoint = pointerTargets(contentItem, event, point, false, false);
            if (targetItems.count()) {
                targetItems = mergePointerTargets(targetItems, targetItemsForPoint);
            } else {
                targetItems = targetItemsForPoint;
            }
        }
        for (QQuickItem *item : targetItems) {
            if (grabbers.contains(item))
                continue;
            QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
            localizePointerEvent(event, item);
            itemPrivate->handlePointerEvent(event, true); // avoid re-delivering to grabbers
            if (event->allPointsGrabbed())
                break;
        }
    }
}

// Deliver an event containing newly pressed or released touch points
bool QQuickWindowPrivate::deliverPressOrReleaseEvent(QPointerEvent *event, bool handlersOnly)
{
    QVector<QQuickItem *> targetItems;
    const bool isTouch = isTouchEvent(event);
    if (isTouch && event->isBeginEvent() && isDeliveringTouchAsMouse()) {
        if (auto point = const_cast<QPointingDevicePrivate *>(QPointingDevicePrivate::get(touchMouseDevice))->queryPointById(touchMouseId)) {
            // When a second point is pressed, if the first point's existing
            // grabber was a pointer handler while a filtering parent is filtering
            // the same first point _as mouse_: we're starting over with delivery,
            // so we need to allow the second point to now be sent as a synth-mouse
            // instead of the first one, so that filtering parents (maybe even the
            // same one) can get a chance to see the second touchpoint as a
            // synth-mouse and perhaps grab it.  Ideally we would always do this
            // when a new touchpoint is pressed, but this compromise fixes
            // QTBUG-70998 and avoids breaking tst_FlickableInterop::touchDragSliderAndFlickable
            if (qobject_cast<QQuickPointerHandler *>(event->exclusiveGrabber(point->eventPoint)))
                cancelTouchMouseSynthesis();
        } else {
            qCWarning(lcTouchTarget) << "during delivery of touch press, synth-mouse ID" << Qt::hex << touchMouseId << "is missing from" << event;
        }
    }
    for (int i = 0; i < event->pointCount(); ++i) {
        auto &point = event->point(i);
        if (point.state() == QEventPoint::Pressed)
            event->clearPassiveGrabbers(point);
        QVector<QQuickItem *> targetItemsForPoint = pointerTargets(contentItem, event, point, !isTouch, isTouch);
        if (targetItems.count()) {
            targetItems = mergePointerTargets(targetItems, targetItemsForPoint);
        } else {
            targetItems = targetItemsForPoint;
        }
    }

    for (QQuickItem *item : targetItems) {
        hasFiltered.clear();
        if (!handlersOnly && sendFilteredPointerEvent(event, item)) {
            if (event->isAccepted())
                return true;
            skipDelivery.append(item);
        }

        // Do not deliverMatchingPointsTo any item for which the filtering parent already intercepted the event,
        // nor to any item which already had a chance to filter.
        if (skipDelivery.contains(item))
            continue;

        // sendFilteredPointerEvent() changed the QEventPoint::accepted() state,
        // but per-point acceptance is opt-in during normal delivery to items.
        for (int i = 0; i < event->pointCount(); ++i)
            event->point(i).setAccepted(false);

        deliverMatchingPointsToItem(item, false, event, handlersOnly);
        if (event->allPointsAccepted())
            handlersOnly = true;
    }

    return event->allPointsAccepted();
}

void QQuickWindowPrivate::deliverMatchingPointsToItem(QQuickItem *item, bool isGrabber, QPointerEvent *pointerEvent, bool handlersOnly)
{
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
#if defined(Q_OS_ANDROID) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // QTBUG-85379
    // In QT_VERSION below 6.0.0 touchEnabled for QtQuickItems is set by default to true
    // It causes delivering touch events to Items which are not interested
    // In some cases (like using Material Style in Android) it may cause a crash
    if (itemPrivate->wasDeleted)
        return;
#endif
    localizePointerEvent(pointerEvent, item);
    bool isMouse = isMouseEvent(pointerEvent);

    // Let the Item's handlers (if any) have the event first.
    // However, double click should never be delivered to handlers.
    if (pointerEvent->type() != QEvent::MouseButtonDblClick) {
        bool wasAccepted = pointerEvent->allPointsAccepted();
        itemPrivate->handlePointerEvent(pointerEvent);
        allowDoubleClick = wasAccepted || !(isMouse && pointerEvent->isBeginEvent() && pointerEvent->allPointsAccepted());
    }
    if (handlersOnly)
        return;

    // If all points are released and the item is not the grabber, it doesn't get the event.
    // But if at least one point is still pressed, we might be in a potential gesture-takeover scenario.
    if (pointerEvent->isEndEvent() && !pointerEvent->isUpdateEvent()
            && !exclusiveGrabbers(pointerEvent).contains(item))
        return;

    // If any parent filters the event, we're done.
    if (sendFilteredPointerEvent(pointerEvent, item))
        return;

    // TODO: unite this mouse point delivery with the synthetic mouse event below
    if (isMouse) {
        auto button = static_cast<QSinglePointEvent *>(pointerEvent)->button();
        if ((isGrabber && button == Qt::NoButton) || item->acceptedMouseButtons().testFlag(button)) {
            // The only reason to already have a mouse grabber here is
            // synthetic events - flickable sends one when setPressDelay is used.
            auto oldMouseGrabber = pointerEvent->exclusiveGrabber(pointerEvent->point(0));
            pointerEvent->accept();
            if (isGrabber && sendFilteredPointerEvent(pointerEvent, item))
                return;
            localizePointerEvent(pointerEvent, item);
            QCoreApplication::sendEvent(item, pointerEvent);
            if (pointerEvent->isAccepted()) {
                auto &point = pointerEvent->point(0);
                auto mouseGrabber = pointerEvent->exclusiveGrabber(point);
                if (mouseGrabber && mouseGrabber != item && mouseGrabber != oldMouseGrabber) {
                    // Normally we don't need item->mouseUngrabEvent() here, because QQuickWindowPrivate::onGrabChanged does it.
                    // However, if one item accepted the mouse event, it expects to have the grab and be in "pressed" state,
                    // because accepting implies grabbing.  But before it actually gets the grab, another item could steal it.
                    // In that case, onGrabChanged() does NOT notify the item that accepted the event that it's not getting the grab after all.
                    // So after ensuring that it's not redundant, we send a notification here, for that case (QTBUG-55325).
                    if (item != lastUngrabbed) {
                        item->mouseUngrabEvent();
                        lastUngrabbed = item;
                    }
                } else if (item->isEnabled() && item->isVisible() && point.state() != QEventPoint::State::Released) {
                    pointerEvent->setExclusiveGrabber(point, item);
                }
                point.setAccepted(true);
            }
            return;
        }
    }

    if (!isTouchEvent(pointerEvent))
        return;

    bool eventAccepted = false;
    QMutableTouchEvent touchEvent;
    QQuickItemPrivate::get(item)->localizedTouchEvent(static_cast<QTouchEvent *>(pointerEvent), false, &touchEvent);
    if (touchEvent.type() == QEvent::None)
        return;  // no points inside this item

    if (item->acceptTouchEvents()) {
        qCDebug(lcTouch) << "considering delivering" << &touchEvent << " to " << item;

        // If any parent filters the event, we're done.
        hasFiltered.clear();
        if (sendFilteredPointerEvent(&touchEvent, item))
            return;

        // Deliver the touch event to the given item
        qCDebug(lcTouch) << "actually delivering" << &touchEvent << " to " << item;
        QCoreApplication::sendEvent(item, &touchEvent);
        eventAccepted = touchEvent.isAccepted();
    } else if (Q_LIKELY(QCoreApplication::testAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents))) {
        // If the touch event wasn't accepted, synthesize a mouse event and see if the item wants it.
        if (!eventAccepted && (itemPrivate->acceptedMouseButtons() & Qt::LeftButton)) {
            //  send mouse event
            if (deliverTouchAsMouse(item, &touchEvent))
                eventAccepted = true;
        }
    }

    if (eventAccepted) {
        // If the touch was accepted (regardless by whom or in what form),
        // update accepted new points.
        bool isPressOrRelease = pointerEvent->isBeginEvent() || pointerEvent->isEndEvent();
        for (int i = 0; i < touchEvent.pointCount(); ++i) {
            auto &point = QMutableEventPoint::from(touchEvent.point(i));
            // legacy-style delivery: if the item doesn't reject the event, that means it handled ALL the points
            point.setAccepted();
            if (isPressOrRelease)
                pointerEvent->setExclusiveGrabber(point, item);
        }
    } else {
        // But if the event was not accepted then we know this item
        // will not be interested in further updates for those touchpoint IDs either.
        for (const auto &point: touchEvent.points()) {
            if (point.state() == QEventPoint::State::Pressed) {
                if (pointerEvent->exclusiveGrabber(point) == item) {
                    qCDebug(lcTouchTarget) << "TP" << Qt::hex << point.id() << "disassociated";
                    pointerEvent->setExclusiveGrabber(point, nullptr);
                }
            }
        }
    }
}

#if QT_CONFIG(quick_draganddrop)
void QQuickWindowPrivate::deliverDragEvent(QQuickDragGrabber *grabber, QEvent *event)
{
    grabber->resetTarget();
    QQuickDragGrabber::iterator grabItem = grabber->begin();
    if (grabItem != grabber->end()) {
        Q_ASSERT(event->type() != QEvent::DragEnter);
        if (event->type() == QEvent::Drop) {
            QDropEvent *e = static_cast<QDropEvent *>(event);
            for (e->setAccepted(false); !e->isAccepted() && grabItem != grabber->end(); grabItem = grabber->release(grabItem)) {
                QPointF p = (**grabItem)->mapFromScene(e->position().toPoint());
                QDropEvent translatedEvent(
                        p.toPoint(),
                        e->possibleActions(),
                        e->mimeData(),
                        e->buttons(),
                        e->modifiers());
                QQuickDropEventEx::copyActions(&translatedEvent, *e);
                QCoreApplication::sendEvent(**grabItem, &translatedEvent);
                e->setAccepted(translatedEvent.isAccepted());
                e->setDropAction(translatedEvent.dropAction());
                grabber->setTarget(**grabItem);
            }
        }
        if (event->type() != QEvent::DragMove) {    // Either an accepted drop or a leave.
            QDragLeaveEvent leaveEvent;
            for (; grabItem != grabber->end(); grabItem = grabber->release(grabItem))
                QCoreApplication::sendEvent(**grabItem, &leaveEvent);
            return;
        } else {
            QDragMoveEvent *moveEvent = static_cast<QDragMoveEvent *>(event);

            // Used to ensure we don't send DragEnterEvents to current drop targets,
            // and to detect which current drop targets we have left
            QVarLengthArray<QQuickItem*, 64> currentGrabItems;
            for (; grabItem != grabber->end(); grabItem = grabber->release(grabItem))
                currentGrabItems.append(**grabItem);

            // Look for any other potential drop targets that are higher than the current ones
            QDragEnterEvent enterEvent(
                    moveEvent->position().toPoint(),
                    moveEvent->possibleActions(),
                    moveEvent->mimeData(),
                    moveEvent->buttons(),
                    moveEvent->modifiers());
            QQuickDropEventEx::copyActions(&enterEvent, *moveEvent);
            event->setAccepted(deliverDragEvent(grabber, contentItem, &enterEvent, &currentGrabItems));

            for (grabItem = grabber->begin(); grabItem != grabber->end(); ++grabItem) {
                int i = currentGrabItems.indexOf(**grabItem);
                if (i >= 0) {
                    currentGrabItems.remove(i);
                    // Still grabbed: send move event
                    QDragMoveEvent translatedEvent(
                            (**grabItem)->mapFromScene(moveEvent->position().toPoint()).toPoint(),
                            moveEvent->possibleActions(),
                            moveEvent->mimeData(),
                            moveEvent->buttons(),
                            moveEvent->modifiers());
                    QQuickDropEventEx::copyActions(&translatedEvent, *moveEvent);
                    QCoreApplication::sendEvent(**grabItem, &translatedEvent);
                    event->setAccepted(translatedEvent.isAccepted());
                    QQuickDropEventEx::copyActions(moveEvent, translatedEvent);
                }
            }

            // Anything left in currentGrabItems is no longer a drop target and should be sent a DragLeaveEvent
            QDragLeaveEvent leaveEvent;
            for (QQuickItem *i : currentGrabItems)
                QCoreApplication::sendEvent(i, &leaveEvent);

            return;
        }
    }
    if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
        QDragMoveEvent *e = static_cast<QDragMoveEvent *>(event);
        QDragEnterEvent enterEvent(
                e->position().toPoint(),
                e->possibleActions(),
                e->mimeData(),
                e->buttons(),
                e->modifiers());
        QQuickDropEventEx::copyActions(&enterEvent, *e);
        event->setAccepted(deliverDragEvent(grabber, contentItem, &enterEvent));
    }
}

bool QQuickWindowPrivate::deliverDragEvent(QQuickDragGrabber *grabber, QQuickItem *item, QDragMoveEvent *event, QVarLengthArray<QQuickItem*, 64> *currentGrabItems)
{
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    if (!item->isVisible() || !item->isEnabled() || QQuickItemPrivate::get(item)->culled)
        return false;
    QPointF p = item->mapFromScene(event->position().toPoint());
    bool itemContained = item->contains(p);

    if (!itemContained && itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        return false;
    }

    QDragEnterEvent enterEvent(
            event->position().toPoint(),
            event->possibleActions(),
            event->mimeData(),
            event->buttons(),
            event->modifiers());
    QQuickDropEventEx::copyActions(&enterEvent, *event);
    QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();

    // Check children in front of this item first
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        if (children.at(ii)->z() < 0)
            continue;
        if (deliverDragEvent(grabber, children.at(ii), &enterEvent, currentGrabItems))
            return true;
    }

    if (itemContained) {
        // If this item is currently grabbed, don't send it another DragEnter,
        // just grab it again if it's still contained.
        if (currentGrabItems && currentGrabItems->contains(item)) {
            grabber->grab(item);
            grabber->setTarget(item);
            return true;
        }

        if (event->type() == QEvent::DragMove || itemPrivate->flags & QQuickItem::ItemAcceptsDrops) {
            QDragMoveEvent translatedEvent(
                    p.toPoint(),
                    event->possibleActions(),
                    event->mimeData(),
                    event->buttons(),
                    event->modifiers(),
                    event->type());
            QQuickDropEventEx::copyActions(&translatedEvent, *event);
            translatedEvent.setAccepted(event->isAccepted());
            QCoreApplication::sendEvent(item, &translatedEvent);
            event->setAccepted(translatedEvent.isAccepted());
            event->setDropAction(translatedEvent.dropAction());
            if (event->type() == QEvent::DragEnter) {
                if (translatedEvent.isAccepted()) {
                    grabber->grab(item);
                    grabber->setTarget(item);
                    return true;
                }
            } else {
                return true;
            }
        }
    }

    // Check children behind this item if this item or any higher children have not accepted
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        if (children.at(ii)->z() >= 0)
            continue;
        if (deliverDragEvent(grabber, children.at(ii), &enterEvent, currentGrabItems))
            return true;
    }

    return false;
}
#endif // quick_draganddrop

bool QQuickWindowPrivate::sendFilteredPointerEvent(QPointerEvent *event, QQuickItem *receiver, QQuickItem *filteringParent)
{
    return sendFilteredPointerEventImpl(event, receiver, filteringParent ? filteringParent : receiver->parentItem());
}

bool QQuickWindowPrivate::sendFilteredPointerEventImpl(QPointerEvent *event, QQuickItem *receiver, QQuickItem *filteringParent)
{
    if (!allowChildEventFiltering)
        return false;
    if (!filteringParent)
        return false;
    bool filtered = false;
    const bool hasHandlers = QQuickItemPrivate::get(receiver)->hasPointerHandlers();
    if (filteringParent->filtersChildMouseEvents() && !hasFiltered.contains(filteringParent)) {
        hasFiltered.append(filteringParent);
        if (isMouseEvent(event)) {
            if (receiver->acceptedMouseButtons()) {
                const bool wasAccepted = event->allPointsAccepted();
                Q_ASSERT(event->pointCount());
                localizePointerEvent(event, receiver);
                event->setAccepted(true);
                auto oldMouseGrabber = event->exclusiveGrabber(event->point(0));
                if (filteringParent->childMouseEventFilter(receiver, event)) {
                    qCDebug(lcMouse) << "mouse event intercepted by childMouseEventFilter of " << filteringParent;
                    skipDelivery.append(filteringParent);
                    filtered = true;
                    if (event->isAccepted() && event->isBeginEvent()) {
                        auto &point = event->point(0);
                        auto mouseGrabber = event->exclusiveGrabber(point);
                        if (mouseGrabber && mouseGrabber != receiver && mouseGrabber != oldMouseGrabber) {
                            receiver->mouseUngrabEvent();
                        } else {
                            event->setExclusiveGrabber(point, receiver);
                        }
                    }
                } else {
                    // Restore accepted state if the event was not filtered.
                    event->setAccepted(wasAccepted);
                }
            }
        } else if (isTouchEvent(event)) {
            const bool acceptsTouchEvents = receiver->acceptTouchEvents() || hasHandlers;
            auto device = event->device();
            if (device->type() == QInputDevice::DeviceType::TouchPad &&
                    device->capabilities().testFlag(QInputDevice::Capability::MouseEmulation)) {
                qCDebug(lcTouchTarget) << "skipping filtering of synth-mouse event from" << device;
            } else if (acceptsTouchEvents || receiver->acceptedMouseButtons()) {
                // get a touch event customized for delivery to filteringParent
                // TODO should not be necessary? because QQuickWindowPrivate::deliverMatchingPointsToItem() does it
                QMutableTouchEvent filteringParentTouchEvent;
                QQuickItemPrivate::get(receiver)->localizedTouchEvent(static_cast<QTouchEvent *>(event), true, &filteringParentTouchEvent);
                if (filteringParentTouchEvent.type() != QEvent::None) {
                    qCDebug(lcTouch) << "letting parent" << filteringParent << "filter for" << receiver << &filteringParentTouchEvent;
                    if (filteringParent->childMouseEventFilter(receiver, &filteringParentTouchEvent)) {
                        qCDebug(lcTouch) << "touch event intercepted by childMouseEventFilter of " << filteringParent;
                        skipDelivery.append(filteringParent);
                        for (auto point : filteringParentTouchEvent.points())
                            event->setExclusiveGrabber(point, filteringParent);
                        return true;
                    } else if (Q_LIKELY(QCoreApplication::testAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents)) &&
                               !filteringParent->acceptTouchEvents()) {
                        qCDebug(lcTouch) << "touch event NOT intercepted by childMouseEventFilter of " << filteringParent
                                           << "; accepts touch?" << filteringParent->acceptTouchEvents()
                                           << "receiver accepts touch?" << acceptsTouchEvents
                                           << "so, letting parent filter a synth-mouse event";
                        // filteringParent didn't filter the touch event.  Give it a chance to filter a synthetic mouse event.
                        for (auto &tp : filteringParentTouchEvent.points()) {
                            QEvent::Type t;
                            switch (tp.state()) {
                            case QEventPoint::State::Pressed:
                                t = QEvent::MouseButtonPress;
                                break;
                            case QEventPoint::State::Released:
                                t = QEvent::MouseButtonRelease;
                                break;
                            case QEventPoint::State::Stationary:
                                continue;
                            default:
                                t = QEvent::MouseMove;
                                break;
                            }

                            bool touchMouseUnset = (touchMouseId == -1);
                            // Only deliver mouse event if it is the touchMouseId or it could become the touchMouseId
                            if (touchMouseUnset || touchMouseId == tp.id()) {
                                // convert filteringParentTouchEvent (which is already transformed wrt local position, velocity, etc.)
                                // into a synthetic mouse event, and let childMouseEventFilter() have another chance with that
                                QMutableSinglePointEvent mouseEvent;
                                touchToMouseEvent(t, tp, &filteringParentTouchEvent, &mouseEvent);
                                // If a filtering item calls QQuickWindow::mouseGrabberItem(), it should
                                // report the touchpoint's grabber.  Whenever we send a synthetic mouse event,
                                // touchMouseId and touchMouseDevice must be set, even if it's only temporarily and isn't grabbed.
                                touchMouseId = tp.id();
                                touchMouseDevice = event->pointingDevice();
                                if (filteringParent->childMouseEventFilter(receiver, &mouseEvent)) {
                                    qCDebug(lcTouch) << "touch event intercepted as synth mouse event by childMouseEventFilter of " << filteringParent;
                                    skipDelivery.append(filteringParent);
                                    if (t != QEvent::MouseButtonRelease) {
                                        qCDebug(lcTouchTarget) << "TP (mouse)" << Qt::hex << tp.id() << "->" << filteringParent;
                                        filteringParentTouchEvent.setExclusiveGrabber(tp, filteringParent);
                                        touchMouseUnset = false; // We want to leave touchMouseId and touchMouseDevice set
                                        if (mouseEvent.isAccepted())
                                            filteringParent->grabMouse();
                                    }
                                    filtered = true;
                                }
                                if (touchMouseUnset)
                                    // Now that we're done sending a synth mouse event, and it wasn't grabbed,
                                    // the touchpoint is no longer acting as a synthetic mouse.  Restore previous state.
                                    cancelTouchMouseSynthesis();
                                mouseEvent.point(0).setAccepted(false); // because touchToMouseEvent() set it true
                                // Only one touchpoint can be treated as a synthetic mouse, so after childMouseEventFilter
                                // has been called once, we're done with this loop over the touchpoints.
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    return sendFilteredPointerEventImpl(event, receiver, filteringParent->parentItem()) || filtered;
}

bool QQuickWindowPrivate::sendFilteredMouseEvent(QEvent *event, QQuickItem *receiver, QQuickItem *filteringParent)
{
    if (!filteringParent)
        return false;

    QQuickItemPrivate *filteringParentPrivate = QQuickItemPrivate::get(filteringParent);
    if (filteringParentPrivate->replayingPressEvent)
        return false;

    bool filtered = false;
    if (filteringParentPrivate->filtersChildMouseEvents && !hasFiltered.contains(filteringParent)) {
        hasFiltered.append(filteringParent);
        if (filteringParent->childMouseEventFilter(receiver, event)) {
            filtered = true;
            skipDelivery.append(filteringParent);
        }
        qCDebug(lcMouseTarget) << "for" << receiver << filteringParent << "childMouseEventFilter ->" << filtered;
    }

    return sendFilteredMouseEvent(event, receiver, filteringParent->parentItem()) || filtered;
}

bool QQuickWindowPrivate::dragOverThreshold(qreal d, Qt::Axis axis, QMouseEvent *event, int startDragThreshold)
{
    QStyleHints *styleHints = QGuiApplication::styleHints();
    bool dragVelocityLimitAvailable = event->device()->capabilities().testFlag(QInputDevice::Capability::Velocity)
        && styleHints->startDragVelocity();
    bool overThreshold = qAbs(d) > (startDragThreshold >= 0 ? startDragThreshold : styleHints->startDragDistance());
    if (dragVelocityLimitAvailable) {
        QVector2D velocityVec = event->point(0).velocity();
        qreal velocity = axis == Qt::XAxis ? velocityVec.x() : velocityVec.y();
        overThreshold |= qAbs(velocity) > styleHints->startDragVelocity();
    }
    return overThreshold;
}

bool QQuickWindowPrivate::dragOverThreshold(qreal d, Qt::Axis axis, const QEventPoint &tp, int startDragThreshold)
{
    QStyleHints *styleHints = qApp->styleHints();
    bool overThreshold = qAbs(d) > (startDragThreshold >= 0 ? startDragThreshold : styleHints->startDragDistance());
    const bool dragVelocityLimitAvailable = (styleHints->startDragVelocity() > 0);
    if (!overThreshold && dragVelocityLimitAvailable) {
        qreal velocity = axis == Qt::XAxis ? tp.velocity().x() : tp.velocity().y();
        overThreshold |= qAbs(velocity) > styleHints->startDragVelocity();
    }
    return overThreshold;
}

bool QQuickWindowPrivate::dragOverThreshold(QVector2D delta)
{
    int threshold = qApp->styleHints()->startDragDistance();
    return qAbs(delta.x()) > threshold || qAbs(delta.y()) > threshold;
}

QT_END_NAMESPACE
