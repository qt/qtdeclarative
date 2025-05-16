// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qdebug.h>
#include <QtGui/private/qevent_p.h>
#include <QtGui/private/qeventpoint_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/private/qabstractanimationjob_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtQuick/private/qquickhoverhandler_p.h>
#include <QtQuick/private/qquickpointerhandler_p_p.h>
#if QT_CONFIG(quick_draganddrop)
#include <QtQuick/private/qquickdrag_p.h>
#endif
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickprofiler_p.h>
#include <QtQuick/private/qquickrendercontrol_p.h>
#include <QtQuick/private/qquickwindow_p.h>

#include <QtCore/qpointer.h>

#include <memory>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTouch, "qt.quick.touch")
Q_STATIC_LOGGING_CATEGORY(lcTouchCmprs, "qt.quick.touch.compression")
Q_LOGGING_CATEGORY(lcTouchTarget, "qt.quick.touch.target")
Q_LOGGING_CATEGORY(lcMouse, "qt.quick.mouse")
Q_STATIC_LOGGING_CATEGORY(lcMouseTarget, "qt.quick.mouse.target")
Q_STATIC_LOGGING_CATEGORY(lcTablet, "qt.quick.tablet")
Q_LOGGING_CATEGORY(lcPtr, "qt.quick.pointer")
Q_STATIC_LOGGING_CATEGORY(lcPtrLoc, "qt.quick.pointer.localization")
Q_STATIC_LOGGING_CATEGORY(lcWheelTarget, "qt.quick.wheel.target")
Q_LOGGING_CATEGORY(lcHoverTrace, "qt.quick.hover.trace")
Q_LOGGING_CATEGORY(lcFocus, "qt.quick.focus")
Q_STATIC_LOGGING_CATEGORY(lcContextMenu, "qt.quick.contextmenu")

extern Q_GUI_EXPORT bool qt_sendShortcutOverrideEvent(QObject *o, ulong timestamp, int k, Qt::KeyboardModifiers mods, const QString &text = QString(), bool autorep = false, ushort count = 1);

bool QQuickDeliveryAgentPrivate::subsceneAgentsExist(false);
QQuickDeliveryAgent *QQuickDeliveryAgentPrivate::currentEventDeliveryAgent(nullptr);

static bool allowSyntheticRightClick()
{
    static int allowRightClick = -1;
    if (allowRightClick < 0) {
        bool ok = false;
        allowRightClick = qEnvironmentVariableIntValue("QT_QUICK_ALLOW_SYNTHETIC_RIGHT_CLICK", &ok);
        if (!ok)
            allowRightClick = 1; // user didn't opt out
    }
    return allowRightClick != 0;
}

void QQuickDeliveryAgentPrivate::touchToMouseEvent(QEvent::Type type, const QEventPoint &p, const QTouchEvent *touchEvent, QMutableSinglePointEvent *mouseEvent)
{
    Q_ASSERT(QCoreApplication::testAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents));
    QMutableSinglePointEvent ret(type, touchEvent->pointingDevice(), p,
                                 (type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton),
                                 (type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton),
                                 touchEvent->modifiers(), Qt::MouseEventSynthesizedByQt);
    ret.setAccepted(true); // this now causes the persistent touchpoint to be accepted too
    ret.setTimestamp(touchEvent->timestamp());
    *mouseEvent = ret;
    // It's very important that the recipient of the event shall be able to see that
    // this "mouse" event actually comes from a touch device.
    Q_ASSERT(mouseEvent->device() == touchEvent->device());
    if (Q_UNLIKELY(mouseEvent->device()->type() == QInputDevice::DeviceType::Mouse))
        qWarning() << "Unexpected: synthesized an indistinguishable mouse event" << mouseEvent;
}

/*!
    Returns \c false if the time constraint for detecting a double-click is violated.
*/
bool QQuickDeliveryAgentPrivate::isWithinDoubleClickInterval(ulong timeInterval)
{
    return timeInterval < static_cast<ulong>(QGuiApplication::styleHints()->mouseDoubleClickInterval());
}

/*!
    Returns \c false if the spatial constraint for detecting a touchscreen double-tap is violated.
*/
bool QQuickDeliveryAgentPrivate::isWithinDoubleTapDistance(const QPoint &distanceBetweenPresses)
{
    auto square = [](qint64 v) { return v * v; };
    return square(distanceBetweenPresses.x()) + square(distanceBetweenPresses.y()) <
            square(QGuiApplication::styleHints()->touchDoubleTapDistance());
}

bool QQuickDeliveryAgentPrivate::checkIfDoubleTapped(ulong newPressEventTimestamp, const QPoint &newPressPos)
{
    const bool doubleClicked = isDeliveringTouchAsMouse() &&
            isWithinDoubleTapDistance(newPressPos - touchMousePressPos) &&
            isWithinDoubleClickInterval(newPressEventTimestamp - touchMousePressTimestamp);
    if (doubleClicked) {
        touchMousePressTimestamp = 0;
    } else {
        touchMousePressTimestamp = newPressEventTimestamp;
        touchMousePressPos = newPressPos;
    }
    return doubleClicked;
}

void QQuickDeliveryAgentPrivate::resetIfDoubleTapPrevented(const QEventPoint &pressedPoint)
{
    if (touchMousePressTimestamp > 0 &&
            (!isWithinDoubleTapDistance(pressedPoint.globalPosition().toPoint() - touchMousePressPos) ||
             !isWithinDoubleClickInterval(pressedPoint.timestamp() - touchMousePressTimestamp))) {
        touchMousePressTimestamp = 0;
        touchMousePressPos = QPoint();
    }
}

/*! \internal
    \deprecated events are handled by methods in which the event is an argument

    Accessor for use by legacy methods such as QQuickItem::grabMouse(),
    QQuickItem::ungrabMouse(), and QQuickItem::grabTouchPoints() which
    are not given sufficient context to do the grabbing.
    We should remove eventsInDelivery in Qt 7.
*/
QPointerEvent *QQuickDeliveryAgentPrivate::eventInDelivery() const
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
QPointingDevicePrivate::EventPointData *QQuickDeliveryAgentPrivate::mousePointData()
{
    if (eventsInDelivery.isEmpty())
        return nullptr;
    auto devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice*>(eventsInDelivery.top()->pointingDevice()));
    return devPriv->pointById(isDeliveringTouchAsMouse() ? touchMouseId : 0);
}

void QQuickDeliveryAgentPrivate::cancelTouchMouseSynthesis()
{
    qCDebug(lcTouchTarget) << "id" << touchMouseId << "on" << touchMouseDevice;
    touchMouseId = -1;
    touchMouseDevice = nullptr;
}

bool QQuickDeliveryAgentPrivate::deliverTouchAsMouse(QQuickItem *item, QTouchEvent *pointerEvent)
{
    Q_Q(QQuickDeliveryAgent);
    Q_ASSERT(QCoreApplication::testAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents));
    auto device = pointerEvent->pointingDevice();

    // A touch event from a trackpad is likely to be followed by a mouse or gesture event, so mouse event synth is redundant
    if (device->type() == QInputDevice::DeviceType::TouchPad && device->capabilities().testFlag(QInputDevice::Capability::MouseEmulation)) {
        qCDebug(lcTouchTarget) << q << "skipping delivery of synth-mouse event from" << device;
        return false;
    }

    // FIXME: make this work for mouse events too and get rid of the asTouchEvent in here.
    QMutableTouchEvent event;
    QQuickItemPrivate::get(item)->localizedTouchEvent(pointerEvent, false, &event);
    if (!event.points().size())
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

            qCDebug(lcTouchTarget) << q << device << "TP (mouse)" << Qt::hex << p.id() << "->" << item;
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
                    if (!isWithinDoubleTapDistance(p.globalPosition().toPoint() - touchMousePressPos))
                        touchMousePressTimestamp = 0;   // Got dragged too far, dismiss the double tap
                }
                if (QQuickItem *mouseGrabberItem = qmlobject_cast<QQuickItem *>(pointerEvent->exclusiveGrabber(p))) {
                    QMutableSinglePointEvent me;
                    touchToMouseEvent(QEvent::MouseMove, p, &event, &me);
                    QCoreApplication::sendEvent(item, &me);
                    event.setAccepted(me.isAccepted());
                    if (me.isAccepted())
                        qCDebug(lcTouchTarget) << q << device << "TP (mouse)" << Qt::hex << p.id() << "->" << mouseGrabberItem;
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

                    deliverHoverEvent(me.scenePosition(), last, me.modifiers(), me.timestamp());
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
                            localMousePos = item->mapFromScene(w->mapFromGlobal(QGuiApplicationPrivate::lastCursorPosition));
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
void QQuickDeliveryAgentPrivate::removeGrabber(QQuickItem *grabber, bool mouse, bool touch, bool cancel)
{
    Q_Q(QQuickDeliveryAgent);
    if (eventsInDelivery.isEmpty()) {
        // do it the expensive way
        for (auto dev : knownPointingDevices) {
            auto devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(dev));
            devPriv->removeGrabber(grabber, cancel);
        }
        return;
    }
    auto eventInDelivery = eventsInDelivery.top();
    if (Q_LIKELY(mouse) && eventInDelivery) {
        auto epd = mousePointData();
        if (epd && epd->exclusiveGrabber == grabber && epd->exclusiveGrabberContext.data() == q) {
            QQuickItem *oldGrabber = qobject_cast<QQuickItem *>(epd->exclusiveGrabber);
            qCDebug(lcMouseTarget) << "removeGrabber" << oldGrabber << "-> null";
            eventInDelivery->setExclusiveGrabber(epd->eventPoint, nullptr);
        }
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

/*!
    \internal

    Clears all exclusive and passive grabs for the points in \a pointerEvent.

    We never allow any kind of grab to persist after release, unless we're waiting
    for a synth event from QtGui (as with most tablet events), so for points that
    are fully released, the grab is cleared.

    Called when QQuickWindow::event dispatches events, or when the QQuickOverlay
    has filtered an event so that it bypasses normal delivery.
*/
void QQuickDeliveryAgentPrivate::clearGrabbers(QPointerEvent *pointerEvent)
{
    if (pointerEvent->isEndEvent()
        && !(isTabletEvent(pointerEvent)
             && (qApp->testAttribute(Qt::AA_SynthesizeMouseForUnhandledTabletEvents)
                 || QWindowSystemInterfacePrivate::TabletEvent::platformSynthesizesMouse))) {
        if (pointerEvent->isSinglePointEvent()) {
            if (static_cast<QSinglePointEvent *>(pointerEvent)->buttons() == Qt::NoButton) {
                auto &firstPt = pointerEvent->point(0);
                pointerEvent->setExclusiveGrabber(firstPt, nullptr);
                pointerEvent->clearPassiveGrabbers(firstPt);
            }
        } else {
            for (auto &point : pointerEvent->points()) {
                if (point.state() == QEventPoint::State::Released) {
                    pointerEvent->setExclusiveGrabber(point, nullptr);
                    pointerEvent->clearPassiveGrabbers(point);
                }
            }
        }
    }
}

/*! \internal
    Translates QEventPoint::scenePosition() in \a touchEvent to this window.

    The item-local QEventPoint::position() is updated later, not here.
*/
void QQuickDeliveryAgentPrivate::translateTouchEvent(QTouchEvent *touchEvent)
{
    for (qsizetype i = 0; i != touchEvent->pointCount(); ++i) {
        auto &pt = touchEvent->point(i);
        QMutableEventPoint::setScenePosition(pt, pt.position());
    }
}


static inline bool windowHasFocus(QQuickWindow *win)
{
    const QWindow *focusWindow = QGuiApplication::focusWindow();
    return win == focusWindow || QQuickRenderControlPrivate::isRenderWindowFor(win, focusWindow) || !focusWindow;
}

static QQuickItem *findFurthestFocusScopeAncestor(QQuickItem *item)
{
    QQuickItem *parentItem = item->parentItem();

    if (parentItem && parentItem->flags() & QQuickItem::ItemIsFocusScope)
        return findFurthestFocusScopeAncestor(parentItem);

    return item;
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
void QQuickDeliveryAgentPrivate::setFocusInScope(QQuickItem *scope, QQuickItem *item,
                                                 Qt::FocusReason reason, FocusOptions options)
{
    Q_Q(QQuickDeliveryAgent);
    Q_ASSERT(item);
    Q_ASSERT(scope || item == rootItem);

    qCDebug(lcFocus) << q << "focus" << item << "in scope" << scope;
    if (scope)
        qCDebug(lcFocus) << "    scopeSubFocusItem:" << QQuickItemPrivate::get(scope)->subFocusItem;

    QQuickItemPrivate *scopePrivate = scope ? QQuickItemPrivate::get(scope) : nullptr;
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);

    QQuickItem *oldActiveFocusItem = nullptr;
    QQuickItem *currentActiveFocusItem = activeFocusItem;
    QQuickItem *newActiveFocusItem = nullptr;
    bool sendFocusIn = false;

    lastFocusReason = reason;

    QVarLengthArray<QQuickItem *, 20> changed;

    // Does this change the active focus?
    if (item == rootItem || scopePrivate->activeFocus) {
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

    if (item != rootItem && !(options & DontChangeSubFocusItem)) {
        QQuickItem *oldSubFocusItem = scopePrivate->subFocusItem;
        if (oldSubFocusItem) {
            QQuickItemPrivate *priv = QQuickItemPrivate::get(oldSubFocusItem);
            priv->focus = false;
            priv->notifyChangeListeners(QQuickItemPrivate::Focus, &QQuickItemChangeListener::itemFocusChanged, oldSubFocusItem, reason);
            changed << oldSubFocusItem;
        }

        QQuickItemPrivate::get(item)->updateSubFocusItem(scope, true);
    }

    if (!(options & DontChangeFocusProperty)) {
        if (item != rootItem || windowHasFocus(rootItem->window())
#ifdef Q_OS_WEBOS
        // Allow focused if there is only one window in the screen where it belongs.
        // Temporary fix for webOS until multi-seat is implemented see QTBUG-85272
                || singleWindowOnScreen(rootItem->window())
#endif
                ) {
            itemPrivate->focus = true;
            itemPrivate->notifyChangeListeners(QQuickItemPrivate::Focus, &QQuickItemChangeListener::itemFocusChanged, item, reason);
            changed << item;
        }
    }

    if (newActiveFocusItem && (rootItem->hasFocus() || (rootItem->window()->type() == Qt::Popup))) {
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
        emit rootItem->window()->focusObjectChanged(activeFocusItem);

    if (!changed.isEmpty())
        notifyFocusChangesRecur(changed.data(), changed.size() - 1, reason);
    if (isSubsceneAgent) {
        auto da = QQuickWindowPrivate::get(rootItem->window())->deliveryAgent;
        qCDebug(lcFocus) << "    delegating setFocusInScope to" << da;

        // When setting subFocusItem, hierarchy is important. Each focus ancestor's
        // subFocusItem must be its nearest descendant with focus. Changing the rootItem's
        // subFocusItem to 'item' here would make 'item' the subFocusItem of all ancestor
        // focus scopes up until root item.
        // That is why we should avoid altering subFocusItem until having traversed
        // all the focus hierarchy.
        QQuickItem *ancestorFS = findFurthestFocusScopeAncestor(item);
        if (ancestorFS != item)
            options |= QQuickDeliveryAgentPrivate::DontChangeSubFocusItem;
        QQuickWindowPrivate::get(rootItem->window())->deliveryAgentPrivate()->setFocusInScope(da->rootItem(), item, reason, options);
    }
    if (oldActiveFocusItem == activeFocusItem)
        qCDebug(lcFocus) << "    activeFocusItem remains" << activeFocusItem << "in" << q;
    else
        qCDebug(lcFocus) << "    activeFocusItem" << oldActiveFocusItem << "->" << activeFocusItem << "in" << q;
}

void QQuickDeliveryAgentPrivate::clearFocusInScope(QQuickItem *scope, QQuickItem *item, Qt::FocusReason reason, FocusOptions options)
{
    Q_ASSERT(item);
    Q_ASSERT(scope || item == rootItem);
    Q_Q(QQuickDeliveryAgent);
    qCDebug(lcFocus) << q << "clear focus" << item << "in scope" << scope;

    QQuickItemPrivate *scopePrivate = nullptr;
    if (scope) {
        scopePrivate = QQuickItemPrivate::get(scope);
        if ( !scopePrivate->subFocusItem )
            return; // No focus, nothing to do.
    }

    QQuickItem *currentActiveFocusItem = activeFocusItem;
    QQuickItem *oldActiveFocusItem = nullptr;
    QQuickItem *newActiveFocusItem = nullptr;

    lastFocusReason = reason;

    QVarLengthArray<QQuickItem *, 20> changed;

    Q_ASSERT(item == rootItem || item == scopePrivate->subFocusItem);

    // Does this change the active focus?
    if (item == rootItem || scopePrivate->activeFocus) {
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

    if (item != rootItem && !(options & DontChangeSubFocusItem)) {
        QQuickItem *oldSubFocusItem = scopePrivate->subFocusItem;
        if (oldSubFocusItem && !(options & DontChangeFocusProperty)) {
            QQuickItemPrivate *priv = QQuickItemPrivate::get(oldSubFocusItem);
            priv->focus = false;
            priv->notifyChangeListeners(QQuickItemPrivate::Focus, &QQuickItemChangeListener::itemFocusChanged, oldSubFocusItem, reason);
            changed << oldSubFocusItem;
        }

        QQuickItemPrivate::get(item)->updateSubFocusItem(scope, false);

    } else if (!(options & DontChangeFocusProperty)) {
        QQuickItemPrivate *priv = QQuickItemPrivate::get(item);
        priv->focus = false;
        priv->notifyChangeListeners(QQuickItemPrivate::Focus, &QQuickItemChangeListener::itemFocusChanged, item, reason);
        changed << item;
    }

    if (newActiveFocusItem) {
        Q_ASSERT(newActiveFocusItem == scope);
        activeFocusItem = scope;
        updateFocusItemTransform();
    }

    // Now that all the state is changed, emit signals & events
    // We must do this last, as this process may result in further changes to focus.
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
        emit rootItem->window()->focusObjectChanged(activeFocusItem);

    if (!changed.isEmpty())
        notifyFocusChangesRecur(changed.data(), changed.size() - 1, reason);
    if (isSubsceneAgent) {
        auto da = QQuickWindowPrivate::get(rootItem->window())->deliveryAgent;
        qCDebug(lcFocus) << "    delegating clearFocusInScope to" << da;
        QQuickWindowPrivate::get(rootItem->window())->deliveryAgentPrivate()->clearFocusInScope(da->rootItem(), item, reason, options);
    }
    if (oldActiveFocusItem == activeFocusItem)
        qCDebug(lcFocus) << "activeFocusItem remains" << activeFocusItem << "in" << q;
    else
        qCDebug(lcFocus) << "    activeFocusItem" << oldActiveFocusItem << "->" << activeFocusItem << "in" << q;
}

void QQuickDeliveryAgentPrivate::clearFocusObject()
{
    if (activeFocusItem == rootItem)
        return;

    clearFocusInScope(rootItem, QQuickItemPrivate::get(rootItem)->subFocusItem, Qt::OtherFocusReason);
}

void QQuickDeliveryAgentPrivate::notifyFocusChangesRecur(QQuickItem **items, int remaining, Qt::FocusReason reason)
{
    QPointer<QQuickItem> item(*items);

    if (item) {
        QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);

        if (itemPrivate->notifiedFocus != itemPrivate->focus) {
            itemPrivate->notifiedFocus = itemPrivate->focus;
            itemPrivate->notifyChangeListeners(QQuickItemPrivate::Focus, &QQuickItemChangeListener::itemFocusChanged, item, reason);
            emit item->focusChanged(itemPrivate->focus);
        }

        if (item && itemPrivate->notifiedActiveFocus != itemPrivate->activeFocus) {
            itemPrivate->notifiedActiveFocus = itemPrivate->activeFocus;
            itemPrivate->itemChange(QQuickItem::ItemActiveFocusHasChanged, bool(itemPrivate->activeFocus));
            itemPrivate->notifyChangeListeners(QQuickItemPrivate::Focus, &QQuickItemChangeListener::itemFocusChanged, item, reason);
            emit item->activeFocusChanged(itemPrivate->activeFocus);
        }
    }

    if (remaining)
        notifyFocusChangesRecur(items + 1, remaining - 1, reason);
}

bool QQuickDeliveryAgentPrivate::clearHover(ulong timestamp)
{
    if (hoverItems.isEmpty())
        return false;

    QQuickWindow *window = rootItem->window();
    if (!window)
        return false;

    const QPointF lastPos = window->mapFromGlobal(QGuiApplicationPrivate::lastCursorPosition);
    const auto modifiers = QGuiApplication::keyboardModifiers();

    // while we don't modify hoveritems directly in the loop, the delivery of the event
    // is expected to reset the stored ID for each cleared item, and items might also
    // be removed from the map in response to event delivery.
    // So we don't want to iterate over a const version of hoverItems here (it would be
    // misleading), but still use const_iterators to avoid  premature detach and constant
    // ref-count-checks.
    for (auto it = hoverItems.cbegin(); it != hoverItems.cend(); ++it) {
        if (const auto &item = it.key()) {
            deliverHoverEventToItem(item, lastPos, lastPos, modifiers, timestamp, HoverChange::Clear);
            Q_ASSERT(([this, item]{
                const auto &it2 = std::as_const(hoverItems).find(item);
                return it2 == hoverItems.cend() || it2.value() == 0;
            }()));
        }
    }

    return true;
}

void QQuickDeliveryAgentPrivate::updateFocusItemTransform()
{
#if QT_CONFIG(im)
    if (activeFocusItem && QGuiApplication::focusObject() == activeFocusItem) {
        QQuickItemPrivate *focusPrivate = QQuickItemPrivate::get(activeFocusItem);
        QGuiApplication::inputMethod()->setInputItemTransform(focusPrivate->itemToWindowTransform());
        QGuiApplication::inputMethod()->setInputItemRectangle(QRectF(0, 0, focusPrivate->width, focusPrivate->height));
        activeFocusItem->updateInputMethod(Qt::ImInputItemClipRectangle);
    }
#endif
}

/*!
    Returns the item that should get active focus when the
    root focus scope gets active focus.
*/
QQuickItem *QQuickDeliveryAgentPrivate::focusTargetItem() const
{
    if (activeFocusItem)
        return activeFocusItem;

    Q_ASSERT(rootItem);
    QQuickItem *targetItem = rootItem;

    while (targetItem->isFocusScope()
            && targetItem->scopedFocusItem()
            && targetItem->scopedFocusItem()->isEnabled()) {
        targetItem = targetItem->scopedFocusItem();
    }

    return targetItem;
}

/*! \internal
    If called during event delivery, returns the agent that is delivering the
    event, without checking whether \a item is reachable from there.
    Otherwise returns QQuickItemPrivate::deliveryAgent() (the delivery agent for
    the narrowest subscene containing \a item), or \c null if \a item is \c null.
*/
QQuickDeliveryAgent *QQuickDeliveryAgentPrivate::currentOrItemDeliveryAgent(const QQuickItem *item)
{
    if (currentEventDeliveryAgent)
        return currentEventDeliveryAgent;
    if (item)
        return QQuickItemPrivate::get(const_cast<QQuickItem *>(item))->deliveryAgent();
    return nullptr;
}

/*! \internal
    QQuickDeliveryAgent delivers events to a tree of Qt Quick Items, beginning
    with the given root item, which is usually QQuickWindow::rootItem() but
    may alternatively be embedded into a Qt Quick 3D scene or something else.
*/
QQuickDeliveryAgent::QQuickDeliveryAgent(QQuickItem *rootItem)
    : QObject(*new QQuickDeliveryAgentPrivate(rootItem), rootItem)
{
}

QQuickDeliveryAgent::~QQuickDeliveryAgent()
{
}

QQuickDeliveryAgent::Transform::~Transform()
{
}

/*! \internal
    Get the QQuickRootItem or subscene root item on behalf of which
    this delivery agent was constructed to handle events.
*/
QQuickItem *QQuickDeliveryAgent::rootItem() const
{
    Q_D(const QQuickDeliveryAgent);
    return d->rootItem;
}

/*! \internal
    Returns the object that was set in setSceneTransform(): a functor that
    transforms from scene coordinates in the parent scene to scene coordinates
    within this DA's subscene, or \c null if none was set.
*/
QQuickDeliveryAgent::Transform *QQuickDeliveryAgent::sceneTransform() const
{
    Q_D(const QQuickDeliveryAgent);
    return d->sceneTransform;
}

/*! \internal
    QQuickDeliveryAgent takes ownership of the given \a transform, which
    encapsulates the ability to transform parent scene coordinates to rootItem
    (subscene) coordinates.
*/
void QQuickDeliveryAgent::setSceneTransform(QQuickDeliveryAgent::Transform *transform)
{
    Q_D(QQuickDeliveryAgent);
    if (d->sceneTransform == transform)
        return;
    qCDebug(lcPtr) << this << d->sceneTransform << "->" << transform;
    if (d->sceneTransform)
        delete d->sceneTransform;
    d->sceneTransform = transform;
}

/*!
    Handle \a ev on behalf of this delivery agent's window or subscene.

    This is the usual main entry point for every incoming event:
    QQuickWindow::event() and QQuick3DViewport::forwardEventToSubscenes()
    both call this function.
*/
bool QQuickDeliveryAgent::event(QEvent *ev)
{
    Q_D(QQuickDeliveryAgent);
    d->currentEventDeliveryAgent = this;
    auto cleanup = qScopeGuard([d] { d->currentEventDeliveryAgent = nullptr; });

    switch (ev->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove: {
        QMouseEvent *me = static_cast<QMouseEvent*>(ev);
        d->handleMouseEvent(me);
        break;
    }
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove: {
        QHoverEvent *he = static_cast<QHoverEvent*>(ev);
        bool accepted = d->deliverHoverEvent(he->scenePosition(),
                                              he->points().first().sceneLastPosition(),
                                              he->modifiers(), he->timestamp());
        d->lastMousePosition = he->scenePosition();
        he->setAccepted(accepted);
#if QT_CONFIG(cursor)
        QQuickWindowPrivate::get(d->rootItem->window())->updateCursor(d->sceneTransform ?
            d->sceneTransform->map(he->scenePosition()) : he->scenePosition(), d->rootItem);
#endif
        return accepted;
    }
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        QTouchEvent *touch = static_cast<QTouchEvent*>(ev);
        d->handleTouchEvent(touch);
        if (Q_LIKELY(QCoreApplication::testAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents))) {
            // we consume all touch events ourselves to avoid duplicate
            // mouse delivery by QtGui mouse synthesis
            ev->accept();
        }
        break;
    }
    case QEvent::TouchCancel:
        // return in order to avoid the QWindow::event below
        return d->deliverTouchCancelEvent(static_cast<QTouchEvent*>(ev));
        break;
    case QEvent::Enter: {
        if (!d->rootItem)
            return false;
        QEnterEvent *enter = static_cast<QEnterEvent*>(ev);
        const auto scenePos = enter->scenePosition();
        bool accepted = d->deliverHoverEvent(scenePos,
                                              enter->points().first().sceneLastPosition(),
                                              enter->modifiers(), enter->timestamp());
        d->lastMousePosition = scenePos;
        // deliverHoverEvent() constructs QHoverEvents: check that EPD didn't end up with corrupted scenePos
        Q_ASSERT(enter->scenePosition() == scenePos);
        enter->setAccepted(accepted);
#if QT_CONFIG(cursor)
        QQuickWindowPrivate::get(d->rootItem->window())->updateCursor(enter->scenePosition(), d->rootItem);
#endif
        return accepted;
    }
    case QEvent::Leave:
        d->clearHover();
        d->lastMousePosition = QPointF();
        break;
#if QT_CONFIG(quick_draganddrop)
    case QEvent::DragEnter:
    case QEvent::DragLeave:
    case QEvent::DragMove:
    case QEvent::Drop:
        d->deliverDragEvent(d->dragGrabber, ev);
        break;
#endif
    case QEvent::FocusAboutToChange:
#if QT_CONFIG(im)
        if (d->activeFocusItem)
            qGuiApp->inputMethod()->commit();
#endif
        break;
#if QT_CONFIG(gestures)
    case QEvent::NativeGesture:
        d->deliverSinglePointEventUntilAccepted(static_cast<QPointerEvent *>(ev));
        break;
#endif
    case QEvent::ShortcutOverride:
        d->deliverKeyEvent(static_cast<QKeyEvent *>(ev));
        break;
    case QEvent::InputMethod:
    case QEvent::InputMethodQuery:
        {
            QQuickItem *target = d->focusTargetItem();
            if (target)
                QCoreApplication::sendEvent(target, ev);
        }
        break;
#if QT_CONFIG(wheelevent)
    case QEvent::Wheel: {
        auto event = static_cast<QWheelEvent *>(ev);
        qCDebug(lcMouse) << event;

        //if the actual wheel event was accepted, accept the compatibility wheel event and return early
        if (d->lastWheelEventAccepted && event->angleDelta().isNull() && event->phase() == Qt::ScrollUpdate)
            return true;

        event->ignore();
        Q_QUICK_INPUT_PROFILE(QQuickProfiler::Mouse, QQuickProfiler::InputMouseWheel,
                              event->angleDelta().x(), event->angleDelta().y());
        d->deliverSinglePointEventUntilAccepted(event);
        d->lastWheelEventAccepted = event->isAccepted();
        break;
    }
#endif
#if QT_CONFIG(tabletevent)
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
        {
            auto *tabletEvent = static_cast<QTabletEvent *>(ev);
            d->deliverPointerEvent(tabletEvent); // visits HoverHandlers too (unlike the mouse event case)
#if QT_CONFIG(cursor)
            QQuickWindowPrivate::get(d->rootItem->window())->updateCursor(tabletEvent->scenePosition(), d->rootItem);
#endif
        }
        break;
#endif
#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu:
        d->deliverContextMenuEvent(static_cast<QContextMenuEvent *>(ev));
        break;
#endif
    default:
        return false;
    }

    return true;
}

void QQuickDeliveryAgentPrivate::deliverKeyEvent(QKeyEvent *e)
{
    if (activeFocusItem) {
        const bool keyPress = (e->type() == QEvent::KeyPress);
        switch (e->type()) {
        case QEvent::KeyPress:
            Q_QUICK_INPUT_PROFILE(QQuickProfiler::Key, QQuickProfiler::InputKeyPress, e->key(), e->modifiers());
            break;
        case QEvent::KeyRelease:
            Q_QUICK_INPUT_PROFILE(QQuickProfiler::Key, QQuickProfiler::InputKeyRelease, e->key(), e->modifiers());
            break;
        default:
            break;
        }

        QQuickItem *item = activeFocusItem;

        // In case of generated event, trigger ShortcutOverride event
        if (keyPress && e->spontaneous() == false)
                qt_sendShortcutOverrideEvent(item, e->timestamp(),
                                         e->key(), e->modifiers(), e->text(),
                                         e->isAutoRepeat(), e->count());

        do {
            e->accept();
            QCoreApplication::sendEvent(item, e);
        } while (!e->isAccepted() && (item = item->parentItem()));
    }
}

QQuickDeliveryAgentPrivate::QQuickDeliveryAgentPrivate(QQuickItem *root) :
    QObjectPrivate(),
    rootItem(root),
    // a plain QQuickItem can be a subscene root; a QQuickRootItem always belongs directly to a QQuickWindow
    isSubsceneAgent(!qmlobject_cast<QQuickRootItem *>(rootItem))
{
#if QT_CONFIG(quick_draganddrop)
    dragGrabber = new QQuickDragGrabber;
#endif
    if (isSubsceneAgent)
        subsceneAgentsExist = true;
}

QQuickDeliveryAgentPrivate::~QQuickDeliveryAgentPrivate()
{
#if QT_CONFIG(quick_draganddrop)
    delete dragGrabber;
    dragGrabber = nullptr;
#endif
    delete sceneTransform;
}

/*! \internal
    Make a copy of any type of QPointerEvent, and optionally localize it
    by setting its first point's local position() if \a transformedLocalPos is given.

    \note some subclasses of QSinglePointEvent, such as QWheelEvent, add extra storage.
    This function doesn't yet support cloning all of those; it can be extended if needed.
*/
QPointerEvent *QQuickDeliveryAgentPrivate::clonePointerEvent(QPointerEvent *event, std::optional<QPointF> transformedLocalPos)
{
    QPointerEvent *ret = event->clone();
    QEventPoint &point = ret->point(0);
    QMutableEventPoint::detach(point);
    QMutableEventPoint::setTimestamp(point, event->timestamp());
    if (transformedLocalPos)
        QMutableEventPoint::setPosition(point, *transformedLocalPos);

    return ret;
}

void QQuickDeliveryAgentPrivate::deliverToPassiveGrabbers(const QVector<QPointer <QObject> > &passiveGrabbers,
                                                          QPointerEvent *pointerEvent)
{
    const QVector<QObject *> &eventDeliveryTargets =
            QQuickPointerHandlerPrivate::deviceDeliveryTargets(pointerEvent->device());
    QVarLengthArray<QPair<QQuickItem *, bool>, 4> sendFilteredPointerEventResult;
    hasFiltered.clear();
    for (QObject *grabberObject : passiveGrabbers) {
        // a null pointer in passiveGrabbers is unlikely, unless the grabbing handler was deleted dynamically
        if (Q_UNLIKELY(!grabberObject))
            continue;
        // a passiveGrabber might be an item or a handler
        if (QQuickPointerHandler *handler = qobject_cast<QQuickPointerHandler *>(grabberObject)) {
            if (handler && !eventDeliveryTargets.contains(handler)) {
                bool alreadyFiltered = false;
                QQuickItem *par = handler->parentItem();

                // see if we already have sent a filter event to the parent
                auto it = std::find_if(sendFilteredPointerEventResult.begin(), sendFilteredPointerEventResult.end(),
                                       [par](const QPair<QQuickItem *, bool> &pair) { return pair.first == par; });
                if (it != sendFilteredPointerEventResult.end()) {
                    // Yes, the event was sent to that parent for filtering: do not call it again, but use
                    // the result of the previous call to determine whether we should call the handler.
                    alreadyFiltered = it->second;
                } else if (par) {
                    alreadyFiltered = sendFilteredPointerEvent(pointerEvent, par);
                    sendFilteredPointerEventResult << qMakePair(par, alreadyFiltered);
                }
                if (!alreadyFiltered) {
                    if (par)
                        localizePointerEvent(pointerEvent, par);
                    handler->handlePointerEvent(pointerEvent);
                }
            }
        } else if (QQuickItem *grabberItem = static_cast<QQuickItem *>(grabberObject)) {
            // don't steal the grab if input should remain with the exclusive grabber only
            if (QQuickItem *excGrabber = static_cast<QQuickItem *>(pointerEvent->exclusiveGrabber(pointerEvent->point(0)))) {
                if ((isMouseEvent(pointerEvent) && excGrabber->keepMouseGrab())
                 || (isTouchEvent(pointerEvent) && excGrabber->keepTouchGrab())) {
                    return;
                }
            }
            localizePointerEvent(pointerEvent, grabberItem);
            QCoreApplication::sendEvent(grabberItem, pointerEvent);
            pointerEvent->accept();
        }
    }
}

bool QQuickDeliveryAgentPrivate::sendHoverEvent(QEvent::Type type, QQuickItem *item,
                                      const QPointF &scenePos, const QPointF &lastScenePos,
                                      Qt::KeyboardModifiers modifiers, ulong timestamp)
{
    auto itemPrivate = QQuickItemPrivate::get(item);
    const auto transform = itemPrivate->windowToItemTransform();
    auto globalPos = item->mapToGlobal(scenePos);
    QHoverEvent hoverEvent(type, scenePos, globalPos, transform.map(lastScenePos), modifiers);
    hoverEvent.setTimestamp(timestamp);
    hoverEvent.setAccepted(true);
    QEventPoint &point = hoverEvent.point(0);
    QMutableEventPoint::setPosition(point, transform.map(scenePos));
    QMutableEventPoint::setGlobalLastPosition(point, item->mapToGlobal(lastScenePos));

    hasFiltered.clear();
    if (sendFilteredMouseEvent(&hoverEvent, item, item->parentItem()))
        return true;

    QCoreApplication::sendEvent(item, &hoverEvent);

    return hoverEvent.isAccepted();
}

/*! \internal
    Delivers a hover event at \a scenePos to the whole scene or subscene
    that this DeliveryAgent is responsible for.  Returns \c true if
    delivery is "done".
*/
// TODO later: specify the device in case of multi-mouse scenario, or mouse and tablet both in use
bool QQuickDeliveryAgentPrivate::deliverHoverEvent(
        const QPointF &scenePos, const QPointF &lastScenePos,
        Qt::KeyboardModifiers modifiers, ulong timestamp)
{
    // The first time this function is called, hoverItems is empty.
    // We then call deliverHoverEventRecursive from the rootItem, and
    // populate the list with all the children and grandchildren that
    // we find that should receive hover events (in addition to sending
    // hover events to them and their HoverHandlers). We also set the
    // hoverId for each item to the currentHoverId.
    // The next time this function is called, we bump currentHoverId,
    // and call deliverHoverEventRecursive once more.
    // When that call returns, the list will contain the items that
    // were hovered the first time, as well as the items that were hovered
    // this time. But only the items that were hovered this time
    // will have their hoverId equal to currentHoverId; the ones we didn't
    // visit will still have an old hoverId. We can therefore go through the
    // list at the end of this function and look for items with an old hoverId,
    // remove them from the list, and update their state accordingly.

    const bool subtreeHoverEnabled = QQuickItemPrivate::get(rootItem)->subtreeHoverEnabled;
    const bool itemsWasHovered = !hoverItems.isEmpty();

    if (!subtreeHoverEnabled && !itemsWasHovered)
        return false;

    currentHoverId++;

    if (subtreeHoverEnabled) {
        hoveredLeafItemFound = false;
        QQuickPointerHandlerPrivate::deviceDeliveryTargets(QPointingDevice::primaryPointingDevice()).clear();
        deliverHoverEventRecursive(rootItem, scenePos, lastScenePos, modifiers, timestamp);
    }

    // Prune the list for items that are no longer hovered
    for (auto it = hoverItems.begin(); it != hoverItems.end();) {
        const auto &[item, hoverId] = *it;
        if (hoverId == currentHoverId) {
            // Still being hovered
            it++;
        } else {
            // No longer hovered. If hoverId is 0, it means that we have sent a HoverLeave
            // event to the item already, and it can just be removed from the list. Note that
            // the item can have been deleted as well.
            if (item && hoverId != 0)
                deliverHoverEventToItem(item, scenePos, lastScenePos, modifiers, timestamp, HoverChange::Clear);
            it = hoverItems.erase(it);
        }
    }

    const bool itemsAreHovered = !hoverItems.isEmpty();
    return itemsWasHovered || itemsAreHovered;
}

/*! \internal
    Delivers a hover event at \a scenePos to \a item and all its children.
    The children get it first. As soon as any item allows the event to remain
    accepted, recursion stops. Returns \c true in that case, or \c false if the
    event is rejected.

    Each item that has hover enabled (from setAcceptHoverEvents()) has the
    QQuickItemPrivate::hoverEnabled flag set. This only controls whether we
    should send hover events to the item itself. (HoverHandlers no longer set
    this flag.) When an item has hoverEnabled set, all its ancestors have the
    QQuickItemPrivate::subtreeHoverEnabled set. This function will
    follow the subtrees that have subtreeHoverEnabled by recursing into each
    child with that flag set. And for each child (in addition to the item
    itself) that also has hoverEnabled set, we call deliverHoverEventToItem()
    to actually deliver the event to it. The item can then choose to accept or
    reject the event. This is only for control over whether we stop propagation
    or not: an item can reject the event, but at the same time be hovered (and
    therefore in hoverItems). By accepting the event, the item will effectivly
    end up as the only one hovered. Any other HoverHandler that may be a child
    of an item that is stacked underneath, will not. Note that since siblings
    can overlap, there can be more than one leaf item under the mouse.

    Note that HoverHandler doesn't set the hoverEnabled flag on the parent item.
    But still, adding a HoverHandler to an item will set its subtreeHoverEnabled flag.
    So all the propagation logic described above will otherwise be the same.
    But the hoverEnabled flag can be used to resolve if subtreeHoverEnabled is on
    because the application explicitly requested it (setAcceptHoverEvents()), or
    indirectly, because the item has HoverHandlers.

    For legacy reasons (Qt 6.1), as soon as we find a leaf item that has hover
    enabled, and therefore receives the event, we stop recursing into the remaining
    siblings (even if the event was ignored). This means that we only allow hover
    events to propagate up the direct parent-child hierarchy, and not to siblings.
    However, if the first candidate HoverHandler is disabled, delivery continues
    to the next one, which may be a sibling (QTBUG-106548).
*/
bool QQuickDeliveryAgentPrivate::deliverHoverEventRecursive(
        QQuickItem *item, const QPointF &scenePos, const QPointF &lastScenePos,
        Qt::KeyboardModifiers modifiers, ulong timestamp)
{

    const QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    const QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();

    for (int ii = children.size() - 1; ii >= 0; --ii) {
        QQuickItem *child = children.at(ii);
        const QQuickItemPrivate *childPrivate = QQuickItemPrivate::get(child);

        if (!child->isVisible() || childPrivate->culled)
            continue;
        if (!childPrivate->subtreeHoverEnabled)
            continue;
        if (childPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
            const QPointF localPos = child->mapFromScene(scenePos);
            if (!child->contains(localPos))
                continue;
        }

        // Recurse into the child
        const bool accepted = deliverHoverEventRecursive(child, scenePos, lastScenePos, modifiers, timestamp);
        if (accepted) {
            // Stop propagation / recursion
            return true;
        }
        if (hoveredLeafItemFound) {
            // Don't propagate to siblings, only to ancestors
            break;
        }
    }

    // All decendants have been visited.
    // Now deliver the event to the item
    return deliverHoverEventToItem(item, scenePos, lastScenePos, modifiers, timestamp, HoverChange::Set);
}

/*! \internal
    Delivers a hover event at \a scenePos to \a item and its HoverHandlers if any.
    Returns \c true if the event remains accepted, \c false if rejected.

    If \a clearHover is \c true, it will be sent as a QEvent::HoverLeave event,
    and the item and its handlers are expected to transition into their non-hovered
    states even if the position still indicates that the mouse is inside.
*/
bool QQuickDeliveryAgentPrivate::deliverHoverEventToItem(
        QQuickItem *item, const QPointF &scenePos, const QPointF &lastScenePos,
        Qt::KeyboardModifiers modifiers, ulong timestamp, HoverChange hoverChange)
{
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    const QPointF localPos = item->mapFromScene(scenePos);
    const QPointF globalPos = item->mapToGlobal(localPos);
    const bool isHovering = item->contains(localPos);
    const auto hoverItemIterator = hoverItems.find(item);
    const bool wasHovering = hoverItemIterator != hoverItems.end() && hoverItemIterator.value() != 0;

    qCDebug(lcHoverTrace) << "item:" << item << "scene pos:" << scenePos << "localPos:" << localPos
                          << "wasHovering:" << wasHovering << "isHovering:" << isHovering;

    bool accepted = false;

    // Start by sending out enter/move/leave events to the item.
    // Note that hoverEnabled only controls if we should send out hover events to the
    // item itself. HoverHandlers are not included, and are dealt with separately below.
    if (itemPrivate->hoverEnabled && isHovering && hoverChange == HoverChange::Set) {
        // Add the item to the list of hovered items (if it doesn't exist there
        // from before), and update hoverId to mark that it's (still) hovered.
        // Also set hoveredLeafItemFound, so that only propagate in a straight
        // line towards the root from now on.
        hoveredLeafItemFound = true;
        if (hoverItemIterator != hoverItems.end())
            hoverItemIterator.value() = currentHoverId;
        else
            hoverItems[item] = currentHoverId;

        if (wasHovering)
            accepted = sendHoverEvent(QEvent::HoverMove, item, scenePos, lastScenePos, modifiers, timestamp);
        else
            accepted = sendHoverEvent(QEvent::HoverEnter, item, scenePos, lastScenePos, modifiers, timestamp);
    } else if (wasHovering) {
        // A leave should never stop propagation
        hoverItemIterator.value() = 0;
        sendHoverEvent(QEvent::HoverLeave, item, scenePos, lastScenePos, modifiers, timestamp);
    }

    if (!itemPrivate->hasPointerHandlers())
        return accepted;

    // Next, send out hover events to the hover handlers.
    // If the item didn't accept the hover event, 'accepted' is now false.
    // Otherwise it's true, and then it should stay the way regardless of
    // whether or not the hoverhandlers themselves are hovered.
    // Note that since a HoverHandler can have a margin, a HoverHandler
    // can be hovered even if the item itself is not.

    if (hoverChange == HoverChange::Clear) {
        // Note: a leave should never stop propagation
        QHoverEvent hoverEvent(QEvent::HoverLeave, scenePos, globalPos, lastScenePos, modifiers);
        hoverEvent.setTimestamp(timestamp);

        for (QQuickPointerHandler *h : itemPrivate->extra->pointerHandlers) {
            if (QQuickHoverHandler *hh = qmlobject_cast<QQuickHoverHandler *>(h)) {
                if (!hh->isHovered())
                    continue;
                hoverEvent.setAccepted(true);
                QCoreApplication::sendEvent(hh, &hoverEvent);
            }
        }
    } else {
        QMouseEvent hoverEvent(QEvent::MouseMove, localPos, scenePos, globalPos, Qt::NoButton, Qt::NoButton, modifiers);
        hoverEvent.setTimestamp(timestamp);

        for (QQuickPointerHandler *h : itemPrivate->extra->pointerHandlers) {
            if (QQuickHoverHandler *hh = qmlobject_cast<QQuickHoverHandler *>(h)) {
                if (!hh->enabled())
                    continue;
                hoverEvent.setAccepted(true);
                hh->handlePointerEvent(&hoverEvent);
                if (hh->isHovered()) {
                    // Mark the whole item as updated, even if only the handler is
                    // actually in a hovered state (because of HoverHandler.margins)
                    hoveredLeafItemFound = true;
                    if (hoverItemIterator != hoverItems.end())
                        hoverItemIterator.value() = currentHoverId;
                    else
                        hoverItems[item] = currentHoverId;
                    if (hh->isBlocking()) {
                        qCDebug(lcHoverTrace) << "skipping rest of hover delivery due to blocking" << hh;
                        accepted = true;
                        break;
                    }
                }
            }
        }
    }

    return accepted;
}

// Simple delivery of non-mouse, non-touch Pointer Events: visit the items and handlers
// in the usual reverse-paint-order until propagation is stopped
bool QQuickDeliveryAgentPrivate::deliverSinglePointEventUntilAccepted(QPointerEvent *event)
{
    Q_ASSERT(event->points().size() == 1);
    QQuickPointerHandlerPrivate::deviceDeliveryTargets(event->pointingDevice()).clear();
    QEventPoint &point = event->point(0);
    QVector<QQuickItem *> targetItems = pointerTargets(rootItem, event, point, false, false);
    point.setAccepted(false);

    // Let passive grabbers see the event. This must be done before we deliver the
    // event to the target and to handlers that might stop event propagation.
    // Passive grabbers cannot stop event delivery.
    for (const auto &passiveGrabber : event->passiveGrabbers(point)) {
        if (auto *grabberItem = qobject_cast<QQuickItem *>(passiveGrabber)) {
            if (targetItems.contains(grabberItem))
                continue;
            localizePointerEvent(event, grabberItem);
            QCoreApplication::sendEvent(grabberItem, event);
        }
    }
    // Maintain the invariant that items receive input events in accepted state.
    // A passive grabber might have explicitly ignored the event.
    event->accept();

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

bool QQuickDeliveryAgentPrivate::deliverTouchCancelEvent(QTouchEvent *event)
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

void QQuickDeliveryAgentPrivate::deliverDelayedTouchEvent()
{
    // Deliver and delete delayedTouch.
    // Set delayedTouch to nullptr before delivery to avoid redelivery in case of
    // event loop recursions (e.g if it the touch starts a dnd session).
    std::unique_ptr<QTouchEvent> e(std::move(delayedTouch));
    qCDebug(lcTouchCmprs) << "delivering" << e.get();
    compressedTouchCount = 0;
    deliverPointerEvent(e.get());
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
void QQuickDeliveryAgentPrivate::handleWindowDeactivate(QQuickWindow *win)
{
    Q_Q(QQuickDeliveryAgent);
    qCDebug(lcFocus) << "deactivated" << win->title();
    const auto inputDevices = QInputDevice::devices();
    for (auto device : inputDevices) {
        if (auto pointingDevice = qobject_cast<const QPointingDevice *>(device)) {
            auto devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(pointingDevice));
            for (auto epd : devPriv->activePoints.values()) {
                if (!epd.exclusiveGrabber.isNull()) {
                    bool relevant = false;
                    if (QQuickItem *item = qmlobject_cast<QQuickItem *>(epd.exclusiveGrabber.data()))
                        relevant = (item->window() == win);
                    else if (QQuickPointerHandler *handler = qmlobject_cast<QQuickPointerHandler *>(epd.exclusiveGrabber.data())) {
                        if (handler->parentItem())
                            relevant = (handler->parentItem()->window() == win && epd.exclusiveGrabberContext.data() == q);
                        else
                            // a handler with no Item parent probably has a 3D Model parent.
                            // TODO actually check the window somehow
                            relevant = true;
                    }
                    if (relevant)
                        devPriv->setExclusiveGrabber(nullptr, epd.eventPoint, nullptr);
                }
                // For now, we don't clearPassiveGrabbers(), just in case passive grabs
                // can be useful to keep monitoring the mouse even after window deactivation.
            }
        }
    }
}

void QQuickDeliveryAgentPrivate::handleWindowHidden(QQuickWindow *win)
{
    qCDebug(lcFocus)  << "hidden" << win->title();
    clearHover();
    lastMousePosition = QPointF();
}

bool QQuickDeliveryAgentPrivate::allUpdatedPointsAccepted(const QPointerEvent *ev)
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
void QQuickDeliveryAgentPrivate::localizePointerEvent(QPointerEvent *ev, const QQuickItem *dest)
{
    for (int i = 0; i < ev->pointCount(); ++i) {
        auto &point = ev->point(i);
        QMutableEventPoint::setPosition(point, dest->mapFromScene(point.scenePosition()));
        qCDebug(lcPtrLoc) << ev->type() << "@" << point.scenePosition() << "to"
                          << dest << "@" << dest->mapToScene(QPointF()) << "->" << point;
    }
}

QList<QObject *> QQuickDeliveryAgentPrivate::exclusiveGrabbers(QPointerEvent *ev)
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

bool QQuickDeliveryAgentPrivate::anyPointGrabbed(const QPointerEvent *ev)
{
    for (const QEventPoint &point : ev->points()) {
        if (ev->exclusiveGrabber(point) || !ev->passiveGrabbers(point).isEmpty())
            return true;
    }
    return false;
}

bool QQuickDeliveryAgentPrivate::allPointsGrabbed(const QPointerEvent *ev)
{
    for (const auto &point : ev->points()) {
        if (!ev->exclusiveGrabber(point) && ev->passiveGrabbers(point).isEmpty())
            return false;
    }
    return true;
}

bool QQuickDeliveryAgentPrivate::isMouseEvent(const QPointerEvent *ev)
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

bool QQuickDeliveryAgentPrivate::isMouseOrWheelEvent(const QPointerEvent *ev)
{
    return isMouseEvent(ev) || ev->type() == QEvent::Wheel;
}

bool QQuickDeliveryAgentPrivate::isHoverEvent(const QPointerEvent *ev)
{
    switch (ev->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverMove:
    case QEvent::HoverLeave:
        return true;
    default:
        return false;
    }
}

bool QQuickDeliveryAgentPrivate::isTouchEvent(const QPointerEvent *ev)
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

bool QQuickDeliveryAgentPrivate::isTabletEvent(const QPointerEvent *ev)
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

bool QQuickDeliveryAgentPrivate::isEventFromMouseOrTouchpad(const QPointerEvent *ev)
{
    const auto devType = ev->device()->type();
    return devType == QInputDevice::DeviceType::Mouse ||
           devType == QInputDevice::DeviceType::TouchPad;
}

bool QQuickDeliveryAgentPrivate::isSynthMouse(const QPointerEvent *ev)
{
    return (!isEventFromMouseOrTouchpad(ev) && isMouseEvent(ev));
}

/*!
    Returns \c true if \a dev is a type of device that only sends
    QSinglePointEvents.
*/
bool QQuickDeliveryAgentPrivate::isSinglePointDevice(const QInputDevice *dev)
{
    switch (dev->type()) {
    case QInputDevice::DeviceType::Mouse:
    case QInputDevice::DeviceType::TouchPad:
    case QInputDevice::DeviceType::Puck:
    case QInputDevice::DeviceType::Stylus:
    case QInputDevice::DeviceType::Airbrush:
        return true;
    case QInputDevice::DeviceType::TouchScreen:
    case QInputDevice::DeviceType::Keyboard:
    case QInputDevice::DeviceType::Unknown:
    case QInputDevice::DeviceType::AllDevices:
        return false;
    }
    return false;
}

QQuickPointingDeviceExtra *QQuickDeliveryAgentPrivate::deviceExtra(const QInputDevice *device)
{
    QInputDevicePrivate *devPriv = QInputDevicePrivate::get(const_cast<QInputDevice *>(device));
    if (devPriv->qqExtra)
        return static_cast<QQuickPointingDeviceExtra *>(devPriv->qqExtra);
    auto extra = new QQuickPointingDeviceExtra;
    devPriv->qqExtra = extra;
    QObject::connect(device, &QObject::destroyed, [devPriv]() {
        delete static_cast<QQuickPointingDeviceExtra *>(devPriv->qqExtra);
        devPriv->qqExtra = nullptr;
    });
    return extra;
}

/*!
    \internal
    This function is called from handleTouchEvent() in case a series of touch
    events containing only \c Updated and \c Stationary points arrives within a
    short period of time. (Some touchscreens are more "jittery" than others.)

    It would be a waste of CPU time to deliver events and have items in the
    scene getting modified more often than once per frame; so here we try to
    coalesce the series of updates into a single event containing all updates
    that occur within one frame period, and deliverDelayedTouchEvent() is
    called from flushFrameSynchronousEvents() to send that single event. This
    is the reason why touch compression lives here so far, instead of in a
    lower layer: the render loop updates the scene in sync with the screen's
    vsync, and flushFrameSynchronousEvents() is called from there (for example
    from QSGThreadedRenderLoop::polishAndSync(), and equivalent places in other
    render loops). It would be preferable to move this code down to a lower
    level eventually, though, because it's not fundamentally a Qt Quick concern.

    This optimization can be turned off by setting the environment variable
    \c QML_NO_TOUCH_COMPRESSION.

    Returns \c true if "done", \c false if the caller needs to finish the
    \a event delivery.
*/
bool QQuickDeliveryAgentPrivate::compressTouchEvent(QTouchEvent *event)
{
    // If this is a subscene agent, don't store any events, because
    // flushFrameSynchronousEvents() is only called on the window's DA.
    if (isSubsceneAgent)
        return false;

    QEventPoint::States states = event->touchPointStates();
    if (states.testFlag(QEventPoint::State::Pressed) || states.testFlag(QEventPoint::State::Released)) {
        qCDebug(lcTouchCmprs) << "no compression" << event;
        // we can only compress an event that doesn't include any pressed or released points
        return false;
    }

    if (!delayedTouch) {
        delayedTouch.reset(new QMutableTouchEvent(event->type(), event->pointingDevice(), event->modifiers(), event->points()));
        delayedTouch->setTimestamp(event->timestamp());
        for (qsizetype i = 0; i < delayedTouch->pointCount(); ++i) {
            auto &tp = delayedTouch->point(i);
            QMutableEventPoint::detach(tp);
        }
        ++compressedTouchCount;
        qCDebug(lcTouchCmprs) << "delayed" << compressedTouchCount << delayedTouch.get();
        if (QQuickWindow *window = rootItem->window())
            window->maybeUpdate();
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
                QMutableEventPoint::setState(tpts[i], QEventPoint::State::Updated);
        }

        // matching touch event? then give delayedTouch a merged set of touchpoints
        if (!mismatch) {
            // have to create a new event because QMutableTouchEvent::setTouchPoints() is missing
            // TODO optimize, or move event compression elsewhere
            delayedTouch.reset(new QMutableTouchEvent(event->type(), event->pointingDevice(), event->modifiers(), tpts));
            delayedTouch->setTimestamp(event->timestamp());
            for (qsizetype i = 0; i < delayedTouch->pointCount(); ++i) {
                auto &tp = delayedTouch->point(i);
                QMutableEventPoint::detach(tp);
            }
            ++compressedTouchCount;
            qCDebug(lcTouchCmprs) << "coalesced" << compressedTouchCount << delayedTouch.get();
            if (QQuickWindow *window = rootItem->window())
                window->maybeUpdate();
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
void QQuickDeliveryAgentPrivate::handleTouchEvent(QTouchEvent *event)
{
    Q_Q(QQuickDeliveryAgent);
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

    qCDebug(lcTouch) << q << event;

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

/*!
    Handle \a event on behalf of this delivery agent's window or subscene.
*/
void QQuickDeliveryAgentPrivate::handleMouseEvent(QMouseEvent *event)
{
    Q_Q(QQuickDeliveryAgent);
    // We generally don't want OS-synthesized mouse events, because Qt Quick does its own touch->mouse synthesis.
    // But if the platform converts long-press to right-click, it's ok to react to that,
    // unless the user has opted out by setting QT_QUICK_ALLOW_SYNTHETIC_RIGHT_CLICK=0.
    if (event->source() == Qt::MouseEventSynthesizedBySystem &&
            !(event->button() == Qt::RightButton && allowSyntheticRightClick())) {
        event->accept();
        return;
    }
    qCDebug(lcMouse) << q << event;

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
        QQuickWindowPrivate::get(rootItem->window())->updateCursor(event->scenePosition());
#endif
        break;
    case QEvent::MouseButtonDblClick:
        Q_QUICK_INPUT_PROFILE(QQuickProfiler::Mouse, QQuickProfiler::InputMouseDoubleClick,
                              event->button(), event->buttons());
        deliverPointerEvent(event);
        break;
    case QEvent::MouseMove: {
        Q_QUICK_INPUT_PROFILE(QQuickProfiler::Mouse, QQuickProfiler::InputMouseMove,
                              event->position().x(), event->position().y());

        const QPointF last = lastMousePosition.isNull() ? event->scenePosition() : lastMousePosition;
        lastMousePosition = event->scenePosition();
        qCDebug(lcHoverTrace) << q << "mouse pos" << last << "->" << lastMousePosition;
        if (!event->points().size() || !event->exclusiveGrabber(event->point(0))) {
            bool accepted = deliverHoverEvent(event->scenePosition(), last, event->modifiers(), event->timestamp());
            event->setAccepted(accepted);
        }
        deliverPointerEvent(event);
#if QT_CONFIG(cursor)
        // The pointer event could result in a cursor change (reaction), so update it afterwards.
        QQuickWindowPrivate::get(rootItem->window())->updateCursor(event->scenePosition());
#endif
        break;
    }
    default:
        Q_ASSERT(false);
        break;
    }
}

/*! \internal
    Flush events before a frame is rendered in \a win.

    This is here because of compressTouchEvent(): we need to ensure that
    coalesced touch events are actually delivered in time to cause the desired
    reactions of items and their handlers. And then since it was introduced
    because of that, we started using this function for once-per-frame hover
    events too, to take care of changing hover state when an item animates
    under the mouse cursor at a time that the mouse cursor is not moving.

    This is done before QQuickItem::updatePolish() is called on all the items
    that requested polishing.
*/
void QQuickDeliveryAgentPrivate::flushFrameSynchronousEvents(QQuickWindow *win)
{
    Q_Q(QQuickDeliveryAgent);
    QQuickDeliveryAgent *deliveringAgent = QQuickDeliveryAgentPrivate::currentEventDeliveryAgent;
    QQuickDeliveryAgentPrivate::currentEventDeliveryAgent = q;

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
    if (frameSynchronousHoverEnabled && !win->mouseGrabberItem() &&
            !lastMousePosition.isNull() && QQuickWindowPrivate::get(win)->dirtyItemList) {
        qCDebug(lcHoverTrace) << q << "delivering frame-sync hover to root @" << lastMousePosition;
        if (deliverHoverEvent(lastMousePosition, lastMousePosition, QGuiApplication::keyboardModifiers(), 0)) {
#if QT_CONFIG(cursor)
            QQuickWindowPrivate::get(rootItem->window())->updateCursor(
                    sceneTransform ? sceneTransform->map(lastMousePosition) : lastMousePosition, rootItem);
#endif
        }

        qCDebug(lcHoverTrace) << q << "frame-sync hover delivery done";
    }
#else
    Q_UNUSED(win);
#endif
    if (Q_UNLIKELY(QQuickDeliveryAgentPrivate::currentEventDeliveryAgent &&
                   QQuickDeliveryAgentPrivate::currentEventDeliveryAgent != q))
        qCWarning(lcPtr, "detected interleaved frame-sync and actual events");
    QQuickDeliveryAgentPrivate::currentEventDeliveryAgent = deliveringAgent;
}

/*! \internal
    React to the fact that \a grabber underwent a grab \a transition
    while an item or handler was handling \a point from \a event.
    I.e. handle the QPointingDevice::grabChanged() signal.

    This notifies the relevant items and/or pointer handlers, and
    does cleanup when grabs are lost or relinquished.
*/
void QQuickDeliveryAgentPrivate::onGrabChanged(QObject *grabber, QPointingDevice::GrabTransition transition,
                                               const QPointerEvent *event, const QEventPoint &point)
{
    Q_Q(QQuickDeliveryAgent);
    const bool grabGained = (transition == QPointingDevice::GrabTransition::GrabExclusive ||
                             transition == QPointingDevice::GrabTransition::GrabPassive);

    // note: event can be null, if the signal was emitted from QPointingDevicePrivate::removeGrabber(grabber)
    if (auto *handler = qmlobject_cast<QQuickPointerHandler *>(grabber)) {
        if (handler->parentItem()) {
            auto itemPriv = QQuickItemPrivate::get(handler->parentItem());
            if (itemPriv->deliveryAgent() == q) {
                handler->onGrabChanged(handler, transition, const_cast<QPointerEvent *>(event),
                                       const_cast<QEventPoint &>(point));
            }
            if (grabGained) {
                // An item that is NOT a subscene root needs to track whether it got a grab via a subscene delivery agent,
                // whereas the subscene root item already knows it has its own DA.
                if (isSubsceneAgent && (!itemPriv->extra.isAllocated() || !itemPriv->extra->subsceneDeliveryAgent))
                    itemPriv->maybeHasSubsceneDeliveryAgent = true;
            }
        } else if (!isSubsceneAgent) {
            handler->onGrabChanged(handler, transition, const_cast<QPointerEvent *>(event),
                                   const_cast<QEventPoint &>(point));
        }
    } else if (auto *grabberItem = qmlobject_cast<QQuickItem *>(grabber)) {
        switch (transition) {
        case QPointingDevice::CancelGrabExclusive:
        case QPointingDevice::UngrabExclusive:
            if (isDeliveringTouchAsMouse() || isSinglePointDevice(point.device())) {
                // If an EventPoint from the mouse or the synth-mouse or from any
                // mouse-like device is ungrabbed, call QQuickItem::mouseUngrabEvent().
                QMutableSinglePointEvent e(QEvent::UngrabMouse, point.device(), point);
                hasFiltered.clear();
                if (!sendFilteredMouseEvent(&e, grabberItem, grabberItem->parentItem())) {
                    lastUngrabbed = grabberItem;
                    grabberItem->mouseUngrabEvent();
                }
            } else {
                // Multi-point event: call QQuickItem::touchUngrabEvent() only if
                // all eventpoints are released or cancelled.
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
                    grabberItem->touchUngrabEvent();
            }
            break;
        default:
            break;
        }
        auto *itemPriv = QQuickItemPrivate::get(grabberItem);
        // An item that is NOT a subscene root needs to track whether it got a grab via a subscene delivery agent,
        // whereas the subscene root item already knows it has its own DA.
        if (isSubsceneAgent && grabGained && (!itemPriv->extra.isAllocated() || !itemPriv->extra->subsceneDeliveryAgent))
            itemPriv->maybeHasSubsceneDeliveryAgent = true;
    }

    if (currentEventDeliveryAgent == q && event && event->device()) {
        switch (transition) {
        case QPointingDevice::GrabPassive: {
            auto epd = QPointingDevicePrivate::get(const_cast<QPointingDevice*>(event->pointingDevice()))->queryPointById(point.id());
            Q_ASSERT(epd);
            QPointingDevicePrivate::setPassiveGrabberContext(epd, grabber, q);
            qCDebug(lcPtr) << "remembering that" << q << "handles point" << point.id() << "after" << transition;
        } break;
        case QPointingDevice::GrabExclusive: {
            auto epd = QPointingDevicePrivate::get(const_cast<QPointingDevice*>(event->pointingDevice()))->queryPointById(point.id());
            Q_ASSERT(epd);
            epd->exclusiveGrabberContext = q;
            qCDebug(lcPtr) << "remembering that" << q << "handles point" << point.id() << "after" << transition;
        } break;
        case QPointingDevice::CancelGrabExclusive:
        case QPointingDevice::UngrabExclusive:
            // taken care of in QPointingDevicePrivate::setExclusiveGrabber(,,nullptr), removeExclusiveGrabber()
            break;
        case QPointingDevice::UngrabPassive:
        case QPointingDevice::CancelGrabPassive:
            // taken care of in QPointingDevicePrivate::removePassiveGrabber(), clearPassiveGrabbers()
            break;
        case QPointingDevice::OverrideGrabPassive:
            // not in use at this time
            break;
        }
    }
}

/*! \internal
    Called when a QPointingDevice is detected, to ensure that the
    QPointingDevice::grabChanged() signal is connected to
    QQuickDeliveryAgentPrivate::onGrabChanged().

    \c knownPointingDevices is maintained only to track signal connections, and
    should not be used for other purposes. The usual place to get a list of all
    devices is QInputDevice::devices().
*/
void QQuickDeliveryAgentPrivate::ensureDeviceConnected(const QPointingDevice *dev)
{
    Q_Q(QQuickDeliveryAgent);
    if (knownPointingDevices.contains(dev))
        return;
    knownPointingDevices.append(dev);
    connect(dev, &QPointingDevice::grabChanged, this, &QQuickDeliveryAgentPrivate::onGrabChanged);
    QObject::connect(dev, &QObject::destroyed, q, [this, dev] {this->knownPointingDevices.removeAll(dev);});
}

/*! \internal
    The entry point for delivery of \a event after determining that it \e is a
    pointer event, and either does not need to be coalesced in
    compressTouchEvent(), or already has been.

    When it returns, event delivery is done.
*/
void QQuickDeliveryAgentPrivate::deliverPointerEvent(QPointerEvent *event)
{
    Q_Q(QQuickDeliveryAgent);
    if (isTabletEvent(event))
        qCDebug(lcTablet) << q << event;

    // If users spin the eventloop as a result of event delivery, we disable
    // event compression and send events directly. This is because we consider
    // the usecase a bit evil, but we at least don't want to lose events.
    ++pointerEventRecursionGuard;
    eventsInDelivery.push(event);

    // So far this is for use in Qt Quick 3D: if a QEventPoint is grabbed,
    // updates get delivered here pretty directly, bypassing picking; but we need to
    // be able to map the 2D viewport coordinate to a 2D coordinate within
    // d->rootItem, a 2D scene that has been arbitrarily mapped onto a 3D object.
    QVarLengthArray<QPointF, 16> originalScenePositions;
    if (sceneTransform) {
        originalScenePositions.resize(event->pointCount());
        for (int i = 0; i < event->pointCount(); ++i) {
            auto &pt = event->point(i);
            originalScenePositions[i] = pt.scenePosition();
            QMutableEventPoint::setScenePosition(pt, sceneTransform->map(pt.scenePosition()));
            qCDebug(lcPtrLoc) << q << event->type() << pt.id() << "transformed scene pos" << pt.scenePosition();
        }
    } else if (isSubsceneAgent) {
        qCDebug(lcPtrLoc) << q << event->type() << "no scene transform set";
    }

    skipDelivery.clear();
    QQuickPointerHandlerPrivate::deviceDeliveryTargets(event->pointingDevice()).clear();
    if (sceneTransform)
        qCDebug(lcPtr) << q << "delivering with" << sceneTransform << event;
    else
        qCDebug(lcPtr) << q << "delivering" << event;
    for (int i = 0; i < event->pointCount(); ++i)
        event->point(i).setAccepted(false);

    if (event->isBeginEvent()) {
        ensureDeviceConnected(event->pointingDevice());
        if (event->type() == QEvent::MouseButtonPress && rootItem->window()
            && static_cast<QSinglePointEvent *>(event)->button() == Qt::RightButton) {
            QQuickWindowPrivate::get(rootItem->window())->rmbContextMenuEventEnabled = true;
        }
        if (!deliverPressOrReleaseEvent(event))
            event->setAccepted(false);
    }
    if (!allUpdatedPointsAccepted(event))
        deliverUpdatedPoints(event);
    if (event->isEndEvent())
        deliverPressOrReleaseEvent(event, true);

    // failsafe: never allow touch->mouse synthesis to persist after all touchpoints are released,
    // or after the touchmouse is released
    if (isTouchEvent(event) && touchMouseId >= 0) {
        if (static_cast<QTouchEvent *>(event)->touchPointStates() == QEventPoint::State::Released) {
            cancelTouchMouseSynthesis();
        } else {
            auto touchMousePoint = event->pointById(touchMouseId);
            if (touchMousePoint && touchMousePoint->state() == QEventPoint::State::Released)
                cancelTouchMouseSynthesis();
        }
    }

    eventsInDelivery.pop();
    if (sceneTransform) {
        for (int i = 0; i < event->pointCount(); ++i)
            QMutableEventPoint::setScenePosition(event->point(i), originalScenePositions.at(i));
    }
    --pointerEventRecursionGuard;
    lastUngrabbed = nullptr;
}

/*! \internal
    Returns a list of all items that are spatially relevant to receive \a event
    occurring at \a itemPos relative to \a item, starting with \a item and
    recursively checking all the children.
    \list
        \li If QQuickItem::clip() is \c true \e and \a itemPos is outside of
        QQuickItem::clipRect(), its children are also omitted. (We stop the
        recursion, because any clipped-off portions of children under \a itemPos
        are invisible.)
        \li Ignore any item in a subscene that "belongs to" a different
        DeliveryAgent. (In current practice, this only happens in 2D scenes in
        Qt Quick 3D.)
        \li Ignore any item for which the given \a predicate returns \c false;
        include any item for which the predicate returns \c true.
    \endlist
*/
// FIXME: should this be iterative instead of recursive?
QVector<QQuickItem *> QQuickDeliveryAgentPrivate::eventTargets(QQuickItem *item, const QEvent *event, QPointF scenePos,
                                                               qxp::function_ref<std::optional<bool> (QQuickItem *, const QEvent *)> predicate) const
{
    QVector<QQuickItem *> targets;
    auto itemPrivate = QQuickItemPrivate::get(item);
    const auto itemPos = item->mapFromScene(scenePos);
    bool relevant = item->contains(itemPos);
    // if the item clips, we can potentially return early
    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        if (!item->clipRect().contains(itemPos))
            return targets;
    }
    QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();
    const std::optional<bool> override = predicate(item, event);
    if (override.has_value())
        relevant = override.value();
    if (relevant) {
        auto it = std::lower_bound(children.begin(), children.end(), 0,
           [](auto lhs, auto rhs) -> bool { return lhs->z() < rhs; });
        children.insert(it, item);
    }

    for (int ii = children.size() - 1; ii >= 0; --ii) {
        QQuickItem *child = children.at(ii);
        auto childPrivate = QQuickItemPrivate::get(child);
        if (!child->isVisible() || !child->isEnabled() || childPrivate->culled ||
                (child != item && childPrivate->extra.isAllocated() && childPrivate->extra->subsceneDeliveryAgent))
            continue;

        if (child == item)
            targets << child;
        else
            targets << eventTargets(child, event, scenePos, predicate);
    }

    return targets;
}

/*! \internal
    Returns a list of all items that are spatially relevant to receive \a event
    occurring at \a point, starting with \a item and recursively checking all
    the children.
    \list
        \li If an item has pointer handlers, call
        QQuickPointerHandler::wantsEventPoint()
        on every handler to decide whether the item is eligible.
        \li Otherwise, if \a checkMouseButtons is \c true, it means we are
        finding targets for a mouse event, so no item for which
        acceptedMouseButtons() is NoButton will be added.
        \li Otherwise, if \a checkAcceptsTouch is \c true, it means we are
        finding targets for a touch event, so either acceptTouchEvents() must
        return true \e or it must accept a synthesized mouse event. I.e. if
        acceptTouchEvents() returns false, it gets added only if
        acceptedMouseButtons() is true.
        \li If QQuickItem::clip() is \c true \e and the \a point is outside of
        QQuickItem::clipRect(), its children are also omitted. (We stop the
        recursion, because any clipped-off portions of children under \a point
        are invisible.)
        \li Ignore any item in a subscene that "belongs to" a different
        DeliveryAgent. (In current practice, this only happens in 2D scenes in
        Qt Quick 3D.)
    \endlist

    The list returned from this function is the list of items that will be
    "visited" when delivering any event for which QPointerEvent::isBeginEvent()
    is \c true.
*/
QVector<QQuickItem *> QQuickDeliveryAgentPrivate::pointerTargets(QQuickItem *item, const QPointerEvent *event, const QEventPoint &point,
                                                                 bool checkMouseButtons, bool checkAcceptsTouch) const
{
    auto predicate = [point, checkMouseButtons, checkAcceptsTouch](QQuickItem *item, const QEvent *ev) -> std::optional<bool> {
        const QPointerEvent *event = static_cast<const QPointerEvent *>(ev);
        auto itemPrivate = QQuickItemPrivate::get(item);
        if (itemPrivate->hasPointerHandlers()) {
            if (itemPrivate->anyPointerHandlerWants(event, point))
                return true;
        } else {
            if (checkMouseButtons && item->acceptedMouseButtons() == Qt::NoButton)
                return false;
            if (checkAcceptsTouch && !(item->acceptTouchEvents() || item->acceptedMouseButtons()))
                return false;
        }

        return std::nullopt;
    };

    return eventTargets(item, event, point.scenePosition(), predicate);
}

/*! \internal
    Returns a joined list consisting of the items in \a list1 and \a list2.
    \a list1 has priority; common items come last.
*/
QVector<QQuickItem *> QQuickDeliveryAgentPrivate::mergePointerTargets(const QVector<QQuickItem *> &list1, const QVector<QQuickItem *> &list2) const
{
    QVector<QQuickItem *> targets = list1;
    // start at the end of list2
    // if item not in list, append it
    // if item found, move to next one, inserting before the last found one
    int insertPosition = targets.size();
    for (int i = list2.size() - 1; i >= 0; --i) {
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
void QQuickDeliveryAgentPrivate::deliverUpdatedPoints(QPointerEvent *event)
{
    Q_Q(const QQuickDeliveryAgent);
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
            // Filtering via QQuickItem::childMouseEventFilter() is only possible
            // if the handler's parent is an Item.  It could be a QQ3D object.
            if (receiver) {
                hasFiltered.clear();
                if (sendFilteredPointerEvent(event, receiver))
                    done = true;
                localizePointerEvent(event, receiver);
            }
            handler->handlePointerEvent(event);
        }
        if (done)
            break;
        // If the grabber is an item or the grabbing handler didn't handle it,
        // then deliver the event to the item (which may have multiple handlers).
        hasFiltered.clear();
        if (receiver)
            deliverMatchingPointsToItem(receiver, true, event);
    }

    // Deliver to each eventpoint's passive grabbers (but don't visit any handler more than once)
    for (auto &point : event->points()) {
        auto epd = QPointingDevicePrivate::get(event->pointingDevice())->queryPointById(point.id());
        if (Q_UNLIKELY(!epd)) {
            qWarning() << "point is not in activePoints" << point;
            continue;
        }
        QList<QPointer<QObject>> relevantPassiveGrabbers;
        for (int i = 0; i < epd->passiveGrabbersContext.size(); ++i) {
            if (epd->passiveGrabbersContext.at(i).data() == q)
                relevantPassiveGrabbers << epd->passiveGrabbers.at(i);
        }
        if (!relevantPassiveGrabbers.isEmpty())
            deliverToPassiveGrabbers(relevantPassiveGrabbers, event);

        // Ensure that HoverHandlers are updated, in case no items got dirty so far and there's no update request
        if (event->type() == QEvent::TouchUpdate) {
            for (const auto &[item, id] : hoverItems) {
                if (item) {
                    bool res = deliverHoverEventToItem(item, point.scenePosition(), point.sceneLastPosition(),
                                                       event->modifiers(), event->timestamp(), HoverChange::Set);
                    // if the event was accepted, then the item's ID must be valid
                    Q_ASSERT(!res || hoverItems.value(item));
                }
            }
        }
    }

    if (done)
        return;

    // If some points weren't grabbed, deliver only to non-grabber PointerHandlers in reverse paint order
    if (!allPointsGrabbed(event)) {
        QVector<QQuickItem *> targetItems;
        for (auto &point : event->points()) {
            // Presses were delivered earlier; not the responsibility of deliverUpdatedTouchPoints.
            // Don't find handlers for points that are already grabbed by an Item (such as Flickable).
            if (point.state() == QEventPoint::Pressed || qmlobject_cast<QQuickItem *>(event->exclusiveGrabber(point)))
                continue;
            QVector<QQuickItem *> targetItemsForPoint = pointerTargets(rootItem, event, point, false, false);
            if (targetItems.size()) {
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
            if (allPointsGrabbed(event))
                break;
        }
    }
}

/*! \internal
    Deliver a pointer \a event containing newly pressed or released QEventPoints.
    If \a handlersOnly is \c true, skip the items and just deliver to Pointer Handlers
    (via QQuickItemPrivate::handlePointerEvent()).

    For the sake of determinism, this function first builds the list
    \c targetItems by calling pointerTargets() on the root item. That is, the
    list of items to "visit" is determined at the beginning, and will not be
    affected if items reparent, hide, or otherwise try to make themselves
    eligible or ineligible during delivery. (Avoid bugs due to ugly
    just-in-time tricks in JS event handlers, filters etc.)

    Whenever a touch gesture is in progress, and another touchpoint is pressed,
    or an existing touchpoint is released, we "start over" with delivery:
    that's why this function is called whenever the event \e contains newly
    pressed or released points. It's not necessary for a handler or an item to
    greedily grab all touchpoints just in case a valid gesture might start.
    QQuickMultiPointHandler::wantsPointerEvent() can calmly return \c false if
    the number of points is less than QQuickMultiPointHandler::minimumPointCount(),
    because it knows it will be asked again if the number of points increases.

    When \a handlersOnly is \c false, \a event visits the items in \c targetItems
    via QQuickItem::event(). We have to call sendFilteredPointerEvent()
    before visiting each item, just in case a Flickable (or some other
    parent-filter) will decide to intercept the event. But we also have to be
    very careful never to let the same Flickable filter the same event twice,
    because when Flickable decides to intercept, it lets the child item have
    that event, and then grabs the next event. That allows you to drag a
    Slider, DragHandler or whatever inside a ListView delegate: if you're
    dragging in the correct direction for the draggable child, it can use
    QQuickItem::setKeepMouseGrab(), QQuickItem::setKeepTouchGrab() or
    QQuickPointerHandler::grabPermissions() to prevent Flickable from
    intercepting during filtering, only if it actually \e has the exclusive
    grab already when Flickable attempts to take it. Typically, both the
    Flickable and the child are checking the same drag threshold, so the
    child must have a chance to grab and \e keep the grab before Flickable
    gets a chance to steal it, even though Flickable actually sees the
    event first during filtering.
*/
bool QQuickDeliveryAgentPrivate::deliverPressOrReleaseEvent(QPointerEvent *event, bool handlersOnly)
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
        // Regardless whether a touchpoint could later result in a synth-mouse event:
        // if the double-tap time or space constraint has been violated,
        // reset state to prevent a double-click event.
        if (isTouch && point.state() == QEventPoint::Pressed)
            resetIfDoubleTapPrevented(point);
        QVector<QQuickItem *> targetItemsForPoint = pointerTargets(rootItem, event, point, !isTouch, isTouch);
        if (targetItems.size()) {
            targetItems = mergePointerTargets(targetItems, targetItemsForPoint);
        } else {
            targetItems = targetItemsForPoint;
        }
    }

    QVector<QPointer<QQuickItem>> safeTargetItems(targetItems.begin(), targetItems.end());

    for (auto &item : safeTargetItems) {
        if (item.isNull())
            continue;
        // failsafe: when items get into a subscene somehow, ensure that QQuickItemPrivate::deliveryAgent() can find it
        if (isSubsceneAgent)
            QQuickItemPrivate::get(item)->maybeHasSubsceneDeliveryAgent = true;

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

/*! \internal
    Deliver \a pointerEvent to \a item and its handlers, if any.
    If \a handlersOnly is \c true, skip QQuickItem::event() and just visit its
    handlers via QQuickItemPrivate::handlePointerEvent().

    This function exists just to de-duplicate the common code between
    deliverPressOrReleaseEvent() and deliverUpdatedPoints().
*/
void QQuickDeliveryAgentPrivate::deliverMatchingPointsToItem(QQuickItem *item, bool isGrabber, QPointerEvent *pointerEvent, bool handlersOnly)
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
    if (pointerEvent->type() != QEvent::MouseButtonDblClick)
        itemPrivate->handlePointerEvent(pointerEvent);

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
    // TODO: remove isGrabber then?
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
                    // Normally we don't need item->mouseUngrabEvent() here, because QQuickDeliveryAgentPrivate::onGrabChanged does it.
                    // However, if one item accepted the mouse event, it expects to have the grab and be in "pressed" state,
                    // because accepting implies grabbing.  But before it actually gets the grab, another item could steal it.
                    // In that case, onGrabChanged() does NOT notify the item that accepted the event that it's not getting the grab after all.
                    // So after ensuring that it's not redundant, we send a notification here, for that case (QTBUG-55325).
                    if (item != lastUngrabbed) {
                        item->mouseUngrabEvent();
                        lastUngrabbed = item;
                    }
                } else if (item->isEnabled() && item->isVisible() && point.state() == QEventPoint::State::Pressed) {
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
    itemPrivate->localizedTouchEvent(static_cast<QTouchEvent *>(pointerEvent), false, &touchEvent);
    if (touchEvent.type() == QEvent::None)
        return;  // no points inside this item

    if (item->acceptTouchEvents()) {
        qCDebug(lcTouch) << "considering delivering" << &touchEvent << " to " << item;

        // Deliver the touch event to the given item
        qCDebug(lcTouch) << "actually delivering" << &touchEvent << " to " << item;
        QCoreApplication::sendEvent(item, &touchEvent);
        eventAccepted = touchEvent.isAccepted();
    } else {
        // If the touch event wasn't accepted, synthesize a mouse event and see if the item wants it.
        if (Q_LIKELY(QCoreApplication::testAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents)) &&
                !eventAccepted && (itemPrivate->acceptedMouseButtons() & Qt::LeftButton))
            deliverTouchAsMouse(item, &touchEvent);
        return;
    }

    Q_ASSERT(item->acceptTouchEvents()); // else we would've returned early above
    if (eventAccepted) {
        bool isPressOrRelease = pointerEvent->isBeginEvent() || pointerEvent->isEndEvent();
        for (int i = 0; i < touchEvent.pointCount(); ++i) {
            auto &point = touchEvent.point(i);
            // legacy-style delivery: if the item doesn't reject the event, that means it handled ALL the points
            point.setAccepted();
            // but don't let the root of a subscene implicitly steal the grab from some other item (such as one of its children)
            if (isPressOrRelease && !(itemPrivate->deliveryAgent() && pointerEvent->exclusiveGrabber(point)))
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
void QQuickDeliveryAgentPrivate::deliverDragEvent(QQuickDragGrabber *grabber, QEvent *event)
{
    QObject *formerTarget = grabber->target();
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
            grabber->ignoreList().clear();
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
            event->setAccepted(deliverDragEvent(grabber, rootItem, &enterEvent, &currentGrabItems,
                                                formerTarget));

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
        event->setAccepted(deliverDragEvent(grabber, rootItem, &enterEvent));
    } else {
        grabber->ignoreList().clear();
    }
}

bool QQuickDeliveryAgentPrivate::deliverDragEvent(
        QQuickDragGrabber *grabber, QQuickItem *item, QDragMoveEvent *event,
        QVarLengthArray<QQuickItem *, 64> *currentGrabItems, QObject *formerTarget)
{
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    if (!item->isVisible() || !item->isEnabled() || QQuickItemPrivate::get(item)->culled)
        return false;
    QPointF p = item->mapFromScene(event->position().toPoint());
    bool itemContained = item->contains(p);

    const int itemIndex = grabber->ignoreList().indexOf(item);
    if (!itemContained) {
        if (itemIndex >= 0)
            grabber->ignoreList().remove(itemIndex);

        if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape)
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
    for (int ii = children.size() - 1; ii >= 0; --ii) {
        if (children.at(ii)->z() < 0)
            continue;
        if (deliverDragEvent(grabber, children.at(ii), &enterEvent, currentGrabItems, formerTarget))
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
            if (event->type() == QEvent::DragEnter) {
                if (formerTarget) {
                    QQuickItem *formerTargetItem = qobject_cast<QQuickItem *>(formerTarget);
                    if (formerTargetItem && currentGrabItems) {
                        QDragLeaveEvent leaveEvent;
                        QCoreApplication::sendEvent(formerTarget, &leaveEvent);

                        // Remove the item from the currentGrabItems so a leave event won't be generated
                        // later on
                        currentGrabItems->removeAll(formerTarget);
                    }
                } else if (itemIndex >= 0) {
                    return false;
                }
            }

            QDragMoveEvent translatedEvent(p.toPoint(), event->possibleActions(), event->mimeData(),
                                           event->buttons(), event->modifiers(), event->type());
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
                } else if (itemIndex < 0) {
                    grabber->ignoreList().append(item);
                }
            } else {
                return true;
            }
        }
    }

    // Check children behind this item if this item or any higher children have not accepted
    for (int ii = children.size() - 1; ii >= 0; --ii) {
        if (children.at(ii)->z() >= 0)
            continue;
        if (deliverDragEvent(grabber, children.at(ii), &enterEvent, currentGrabItems, formerTarget))
            return true;
    }

    return false;
}
#endif // quick_draganddrop

/*! \internal
    Allow \a filteringParent to filter \a event on behalf of \a receiver, via
    QQuickItem::childMouseEventFilter(). This happens right \e before we would
    send \a event to \a receiver.

    Returns \c true only if \a event has been intercepted (by \a filteringParent
    or some other filtering ancestor) and should \e not be sent to \a receiver.
*/
bool QQuickDeliveryAgentPrivate::sendFilteredPointerEvent(QPointerEvent *event, QQuickItem *receiver, QQuickItem *filteringParent)
{
    return sendFilteredPointerEventImpl(event, receiver, filteringParent ? filteringParent : receiver->parentItem());
}

/*! \internal
    The recursive implementation of sendFilteredPointerEvent().
*/
bool QQuickDeliveryAgentPrivate::sendFilteredPointerEventImpl(QPointerEvent *event, QQuickItem *receiver, QQuickItem *filteringParent)
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
                // TODO should not be necessary? because QQuickDeliveryAgentPrivate::deliverMatchingPointsToItem() does it
                QMutableTouchEvent filteringParentTouchEvent;
                QQuickItemPrivate::get(receiver)->localizedTouchEvent(static_cast<QTouchEvent *>(event), true, &filteringParentTouchEvent);
                if (filteringParentTouchEvent.type() != QEvent::None) {
                    qCDebug(lcTouch) << "letting parent" << filteringParent << "filter for" << receiver << &filteringParentTouchEvent;
                    filtered = filteringParent->childMouseEventFilter(receiver, &filteringParentTouchEvent);
                    if (filtered) {
                        qCDebug(lcTouch) << "touch event intercepted by childMouseEventFilter of " << filteringParent;
                        event->setAccepted(filteringParentTouchEvent.isAccepted());
                        skipDelivery.append(filteringParent);
                        if (event->isAccepted()) {
                            for (auto point : filteringParentTouchEvent.points()) {
                                const QQuickItem *exclusiveGrabber = qobject_cast<const QQuickItem *>(event->exclusiveGrabber(point));
                                if (!exclusiveGrabber || !exclusiveGrabber->keepTouchGrab())
                                    event->setExclusiveGrabber(point, filteringParent);
                            }
                        }
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
                                filtered = filteringParent->childMouseEventFilter(receiver, &mouseEvent);
                                if (filtered) {
                                    qCDebug(lcTouch) << "touch event intercepted as synth mouse event by childMouseEventFilter of " << filteringParent;
                                    event->setAccepted(mouseEvent.isAccepted());
                                    skipDelivery.append(filteringParent);
                                    if (event->isAccepted() && event->isBeginEvent()) {
                                        qCDebug(lcTouchTarget) << "TP (mouse)" << Qt::hex << tp.id() << "->" << filteringParent;
                                        filteringParentTouchEvent.setExclusiveGrabber(tp, filteringParent);
                                        touchMouseUnset = false; // We want to leave touchMouseId and touchMouseDevice set
                                        filteringParent->grabMouse();
                                    }
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

/*! \internal
    Allow \a filteringParent to filter \a event on behalf of \a receiver, via
    QQuickItem::childMouseEventFilter(). This happens right \e before we would
    send \a event to \a receiver.

    Returns \c true only if \a event has been intercepted (by \a filteringParent
    or some other filtering ancestor) and should \e not be sent to \a receiver.

    Unlike sendFilteredPointerEvent(), this version does not synthesize a
    mouse event from touch (presumably it's already an actual mouse event).
*/
bool QQuickDeliveryAgentPrivate::sendFilteredMouseEvent(QEvent *event, QQuickItem *receiver, QQuickItem *filteringParent)
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

/*! \internal
    Returns \c true if the movement delta \a d in pixels along the \a axis
    exceeds \a startDragThreshold if it is set, or QStyleHints::startDragDistance();
    \e or, if QEventPoint::velocity() of \a event exceeds QStyleHints::startDragVelocity().

    \sa QQuickPointerHandlerPrivate::dragOverThreshold()
*/
bool QQuickDeliveryAgentPrivate::dragOverThreshold(qreal d, Qt::Axis axis, QMouseEvent *event, int startDragThreshold)
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

/*! \internal
    Returns \c true if the movement delta \a d in pixels along the \a axis
    exceeds \a startDragThreshold if it is set, or QStyleHints::startDragDistance();
    \e or, if QEventPoint::velocity() of \a tp exceeds QStyleHints::startDragVelocity().

    \sa QQuickPointerHandlerPrivate::dragOverThreshold()
*/
bool QQuickDeliveryAgentPrivate::dragOverThreshold(qreal d, Qt::Axis axis, const QEventPoint &tp, int startDragThreshold)
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

/*! \internal
    Returns \c true if the movement \a delta in pixels exceeds QStyleHints::startDragDistance().

    \sa QQuickDeliveryAgentPrivate::dragOverThreshold()
*/
bool QQuickDeliveryAgentPrivate::dragOverThreshold(QVector2D delta)
{
    int threshold = qApp->styleHints()->startDragDistance();
    return qAbs(delta.x()) > threshold || qAbs(delta.y()) > threshold;
}

/*!
    \internal
    Returns all items that could potentially want \a event.

    (Similar to \l pointerTargets(), necessary because QContextMenuEvent is not
    a QPointerEvent.)
*/
QVector<QQuickItem *> QQuickDeliveryAgentPrivate::contextMenuTargets(QQuickItem *item, const QContextMenuEvent *event) const
{
    auto predicate = [](QQuickItem *, const QEvent *) -> std::optional<bool> {
        return std::nullopt;
    };

    return eventTargets(item, event, event->pos(), predicate);
}

/*!
    \internal

    Based on \l deliverPointerEvent().
*/
void QQuickDeliveryAgentPrivate::deliverContextMenuEvent(QContextMenuEvent *event)
{
    skipDelivery.clear();
    QVector<QQuickItem *> targetItems = contextMenuTargets(rootItem, event);
    qCDebug(lcContextMenu) << "delivering context menu event" << event << "to" << targetItems.size() << "target item(s)";
    QVector<QPointer<QQuickItem>> safeTargetItems(targetItems.begin(), targetItems.end());
    for (auto &item : safeTargetItems) {
        qCDebug(lcContextMenu) << "- attempting to deliver to" << item;
        if (item.isNull())
            continue;
        // failsafe: when items get into a subscene somehow, ensure that QQuickItemPrivate::deliveryAgent() can find it
        if (isSubsceneAgent)
            QQuickItemPrivate::get(item)->maybeHasSubsceneDeliveryAgent = true;

        QCoreApplication::sendEvent(item, event);
        if (event->isAccepted())
            return;
    }
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QQuickDeliveryAgent *da)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    if (!da) {
        debug << "QQuickDeliveryAgent(0)";
        return debug;
    }

    debug << "QQuickDeliveryAgent(";
    if (!da->objectName().isEmpty())
        debug << da->objectName() << ' ';
    auto root = da->rootItem();
    if (Q_LIKELY(root)) {
        debug << "root=" << root->metaObject()->className();
        if (!root->objectName().isEmpty())
            debug << ' ' << root->objectName();
    } else {
        debug << "root=0";
    }
    debug << ')';
    return debug;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickdeliveryagent_p.cpp"
