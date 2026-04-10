// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquickcontrol_p_p.h"
#include "qquickoverlay_p.h"
#include "qquickoverlay_p_p.h"
#include "qquickpopupitem_p_p.h"
#include "qquickpopup_p_p.h"
#include "qquickdrawer_p.h"
#include "qquickdrawer_p_p.h"
#include "qquickapplicationwindow_p.h"
#include <QtGui/qpainterpath.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlproperty.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/private/qquickshadereffectsource_p.h>
#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Overlay
    \inherits Item
//!     \nativetype QQuickOverlay
    \inqmlmodule QtQuick.Controls
    \since 5.10
    \brief A window overlay for popups.

    Overlay provides a layer for popups, ensuring that popups are displayed above
    other content and that the background is dimmed when a \l {Popup::}{modal} or
    \l {Popup::dim}{dimmed} popup is visible.

    The overlay is an ordinary Item that covers the entire window. It can be used
    as a visual parent to position a popup in scene coordinates.

    \include qquickoverlay-popup-parent.qdocinc

    \sa ApplicationWindow
*/

QList<QQuickPopup *> QQuickOverlayPrivate::stackingOrderPopups() const
{
    const QList<QQuickItem *> children = paintOrderChildItems();

    QList<QQuickPopup *> popups;
    popups.reserve(children.size());

    for (auto it = children.crbegin(), end = children.crend(); it != end; ++it) {
        QQuickPopup *popup = qobject_cast<QQuickPopup *>((*it)->parent());
        if (popup)
            popups += popup;
    }

    return popups;
}

QList<QQuickPopup *> QQuickOverlayPrivate::stackingOrderDrawers() const
{
    QList<QQuickPopup *> sorted(allDrawers);
    std::sort(sorted.begin(), sorted.end(), [](const QQuickPopup *one, const QQuickPopup *another) {
        return one->z() > another->z();
    });
    return sorted;
}

void QQuickOverlayPrivate::itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF &)
{
    updateGeometry();
}

void QQuickOverlayPrivate::itemRotationChanged(QQuickItem *)
{
    updateGeometry();
}

// Wheel and tablet events are not routed through the delivery agent like mouse
// and touch events, so modal popup blocking must be handled explicitly here.
// Returns true if event is consumed by a modal popup blocking its top-most target.
#if QT_CONFIG(tabletevent) || QT_CONFIG(wheelevent)
bool QQuickOverlayPrivate::eatEventIfBlockedByModal(QPointerEvent *event)
{
    Q_Q(QQuickOverlay);
    const QList<QQuickItem *> targetItems = deliveryAgentPrivate()->pointerTargets(
            window->contentItem(), event, event->point(0), false, false);
    if (targetItems.isEmpty())
        return false;

    QQuickItem *const topItem = targetItems.first();
    QQuickItem *const dimmerItem = q->property("_q_dimmerItem").value<QQuickItem *>();
    // Find the QQuickPopupItem that contains topItem, if any.
    QQuickItem *item = qobject_cast<QQuickPopupItem *>(topItem) ? topItem : nullptr;

    if (!item) {
        item = topItem;
        while ((item = item->parentItem())) {
            if (qobject_cast<QQuickPopupItem *>(item))
                break;
        }
    }

    // topItem may be a QQuickShaderEffectSource created for a popup's drop shadow layer.
    // It is a direct child of the overlay (not of the popup item), so the parent walk above
    // misses it. Resolve it to the popup item it renders so the loop below handles it correctly.
    if (!item && dimmerItem != topItem && q->isAncestorOf(topItem)) {
        if (auto *shaderEffect = qobject_cast<QQuickShaderEffectSource *>(topItem)) {
            if (auto *pi = qobject_cast<QQuickPopupItem *>(shaderEffect->sourceItem()))
                item = pi;
        }
        if (!item)
            return false;
    }

    // Eat the event if receiver topItem is not a child of a popup before
    // the first modal popup.
    for (const auto &popup : stackingOrderPopups()) {
        const QQuickItem *popupItem = popup->popupItem();
        if (!popupItem)
            continue;
        // item belongs to this popup — check shader effect source case, then stop searching
        if (popupItem == item) {
            // topItem is the popup's shader effect source (drop shadow), not the popup item
            // itself — the event landed in the shadow area outside the popup's bounds.
            // Eat it for modal popups to prevent it reaching items behind the shadow.
            if (topItem != item && popup->isModal()
                && !item->contains(item->mapFromScene(event->point(0).scenePosition()))) {
                event->accept();
                return true;
            }
            break;
        }
        // if topItem is below this modal popup in stacking order — eat the event
        if (popup->overlayEvent(topItem, event))
            return true;
    }
    return false;
}
#endif

bool QQuickOverlayPrivate::startDrag(QEvent *event, const QPointF &pos)
{
    Q_Q(QQuickOverlay);
    if (allDrawers.isEmpty())
        return false;

    // don't start dragging a drawer if a modal popup overlay is blocking (QTBUG-60602)
    QQuickItem *item = q->childAt(pos.x(), pos.y());
    if (item) {
        const auto popups = stackingOrderPopups();
        for (QQuickPopup *popup : popups) {
            QQuickPopupPrivate *p = QQuickPopupPrivate::get(popup);
            if (p->dimmer == item && popup->isVisible() && popup->isModal())
                return false;
        }
    }

    const QList<QQuickPopup *> drawers = stackingOrderDrawers();
    for (QQuickPopup *popup : drawers) {
        QQuickDrawer *drawer = qobject_cast<QQuickDrawer *>(popup);
        Q_ASSERT(drawer);
        QQuickDrawerPrivate *p = QQuickDrawerPrivate::get(drawer);
        if (p->startDrag(event)) {
            setMouseGrabberPopup(drawer);
            return true;
        }
    }

    return false;
}

bool QQuickOverlayPrivate::handlePress(QQuickItem *source, QEvent *event, QQuickPopup *target)
{
    if (target) {
        if (target->overlayEvent(source, event)) {
            setMouseGrabberPopup(target);
            return true;
        }
        return false;
    }

    switch (event->type()) {
    default: {
        if (mouseGrabberPopup)
            break;
#if QT_CONFIG(quicktemplates2_multitouch)
        Q_FALLTHROUGH();
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
#endif
        // allow non-modal popups to close themselves,
        // and non-dimming modal popups to block the event
        const auto popups = stackingOrderPopups();
        for (QQuickPopup *popup : popups) {
            if (popup->overlayEvent(source, event)) {
                setMouseGrabberPopup(popup);
                return true;
            }
        }
        break;
    }
    }

    event->ignore();
    return false;
}

bool QQuickOverlayPrivate::handleMove(QQuickItem *source, QEvent *event, QQuickPopup *target)
{
    if (target)
        return target->overlayEvent(source, event);
    return false;
}

bool QQuickOverlayPrivate::handleRelease(QQuickItem *source, QEvent *event, QQuickPopup *target)
{
    if (target) {
        setMouseGrabberPopup(nullptr);
        if (target->overlayEvent(source, event)) {
            setMouseGrabberPopup(nullptr);
            return true;
        }
    } else {
        const auto popups = stackingOrderPopups();
        for (QQuickPopup *popup : popups) {
            if (popup->overlayEvent(source, event))
                return true;
        }
    }
    return false;
}

bool QQuickOverlayPrivate::handleMouseEvent(QQuickItem *source, QMouseEvent *event, QQuickPopup *target)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (!target && startDrag(event, event->scenePosition()))
            return true;
        return handlePress(source, event, target);
    case QEvent::MouseMove:
        return handleMove(source, event, target ? target : mouseGrabberPopup.data());
    case QEvent::MouseButtonRelease:
        return handleRelease(source, event, target ? target : mouseGrabberPopup.data());
    default:
        break;
    }
    return false;
}

bool QQuickOverlayPrivate::handleHoverEvent(QQuickItem *source, QHoverEvent *event, QQuickPopup *target)
{
    switch (event->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverMove:
    case QEvent::HoverLeave:
        if (target)
            return target->overlayEvent(source, event);
        return false;
    default:
        Q_UNREACHABLE(); // function must only be called on hover events
        break;
    }
    return false;
}

#if QT_CONFIG(quicktemplates2_multitouch)
bool QQuickOverlayPrivate::handleTouchEvent(QQuickItem *source, QTouchEvent *event, QQuickPopup *target)
{
    bool handled = false;
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        for (const QTouchEvent::TouchPoint &point : event->points()) {
            switch (point.state()) {
            case QEventPoint::Pressed:
                if (!target && startDrag(event, point.scenePosition()))
                    handled = true;
                else
                    handled |= handlePress(source, event, target);
                break;
            case QEventPoint::Updated:
                handled |= handleMove(source, event, target ? target : mouseGrabberPopup.data());
                break;
            case QEventPoint::Released:
                handled |= handleRelease(source, event, target ? target : mouseGrabberPopup.data());
                break;
            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    return handled;
}
#endif

void QQuickOverlayPrivate::addPopup(QQuickPopup *popup)
{
    Q_Q(QQuickOverlay);
    allPopups += popup;
    if (QQuickDrawer *drawer = qobject_cast<QQuickDrawer *>(popup)) {
        allDrawers += drawer;
        q->setVisible(!allDrawers.isEmpty() || !q->childItems().isEmpty());
    }
}

void QQuickOverlayPrivate::removePopup(QQuickPopup *popup)
{
    Q_Q(QQuickOverlay);
    allPopups.removeOne(popup);
    if (allDrawers.removeOne(popup))
        q->setVisible(!allDrawers.isEmpty() || !q->childItems().isEmpty());
}

void QQuickOverlayPrivate::setMouseGrabberPopup(QQuickPopup *popup)
{
    if (popup && !popup->isVisible())
        popup = nullptr;
    mouseGrabberPopup = popup;
}

void QQuickOverlayPrivate::updateGeometry()
{
    Q_Q(QQuickOverlay);
    if (!window || !window->contentItem())
        return;

    const QSizeF size = window->contentItem()->size();
    const QPointF pos(-(size.width() - window->size().width()) / 2,
                      -(size.height() - window->size().height()) / 2);

    q->setSize(size);
    q->setPosition(pos);
}

QQuickOverlay::QQuickOverlay(QQuickItem *parent)
    : QQuickItem(*(new QQuickOverlayPrivate), parent)
{
    Q_D(QQuickOverlay);
    setZ(1000001); // DefaultWindowDecoration+1
    setAcceptedMouseButtons(Qt::AllButtons);
#if QT_CONFIG(quicktemplates2_multitouch)
    setAcceptTouchEvents(true);
#endif
    setFiltersChildMouseEvents(true);
    setVisible(false);

    if (parent) {
        d->updateGeometry();
        QQuickItemPrivate::get(parent)->addItemChangeListener(d, QQuickItemPrivate::Geometry);
        if (QQuickWindow *window = parent->window()) {
            window->installEventFilter(this);
            if (QQuickItem *contentItem = window->contentItem())
                QQuickItemPrivate::get(contentItem)->addItemChangeListener(d, QQuickItemPrivate::Rotation);
        }
    }
}

QQuickOverlay::~QQuickOverlay()
{
    Q_D(QQuickOverlay);
    if (QQuickItem *parent = parentItem()) {
        QQuickItemPrivate::get(parent)->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
        if (QQuickWindow *window = parent->window()) {
            if (QQuickItem *contentItem = window->contentItem())
                QQuickItemPrivate::get(contentItem)->removeItemChangeListener(d, QQuickItemPrivate::Rotation);
        }
    }
}

QQmlComponent *QQuickOverlay::modal() const
{
    Q_D(const QQuickOverlay);
    return d->modal;
}

void QQuickOverlay::setModal(QQmlComponent *modal)
{
    Q_D(QQuickOverlay);
    if (d->modal == modal)
        return;

    d->modal = modal;
    emit modalChanged();
}

QQmlComponent *QQuickOverlay::modeless() const
{
    Q_D(const QQuickOverlay);
    return d->modeless;
}

void QQuickOverlay::setModeless(QQmlComponent *modeless)
{
    Q_D(QQuickOverlay);
    if (d->modeless == modeless)
        return;

    d->modeless = modeless;
    emit modelessChanged();
}

QQuickOverlay *QQuickOverlay::overlay(QQuickWindow *window, QQuickItem *parent)
{
    if (!window)
        return nullptr;

    const char *name = "_q_QQuickOverlay";

    if (QQuickItemPrivate::customOverlayRequested) {
        while (parent) {
            if (QQuickItemPrivate::get(parent)->customOverlay) {
                QQuickOverlay *overlay = parent->property(name).value<QQuickOverlay *>();
                if (!overlay) {
                    overlay = new QQuickOverlay(parent);
                    parent->setProperty(name, QVariant::fromValue(overlay));
                }
                return overlay;
            }
            parent = parent->parentItem();
        }
    }
    QQuickOverlay *overlay = window->property(name).value<QQuickOverlay *>();
    if (!overlay) {
        QQuickItem *content = window->contentItem();
        // Do not re-create the overlay if the window is being destroyed
        // and thus, its content item no longer has a window associated.
        if (content && content->window()) {
            overlay = new QQuickOverlay(window->contentItem());
            window->setProperty(name, QVariant::fromValue(overlay));
        }
    }
    return overlay;
}

QQuickOverlayAttached *QQuickOverlay::qmlAttachedProperties(QObject *object)
{
    return new QQuickOverlayAttached(object);
}

void QQuickOverlay::itemChange(ItemChange change, const ItemChangeData &data)
{
    Q_D(QQuickOverlay);
    QQuickItem::itemChange(change, data);

    if (change == ItemChildAddedChange || change == ItemChildRemovedChange) {
        setVisible(!d->allDrawers.isEmpty() || !childItems().isEmpty());
        if (data.item->parent() == d->mouseGrabberPopup)
            d->setMouseGrabberPopup(nullptr);
    }
}

void QQuickOverlay::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickOverlay);
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    for (QQuickPopup *popup : std::as_const(d->allPopups))
        QQuickPopupPrivate::get(popup)->resizeDimmer();
}

void QQuickOverlay::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickOverlay);
    d->handleMouseEvent(this, event);
}

void QQuickOverlay::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickOverlay);
    d->handleMouseEvent(this, event);
}

void QQuickOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickOverlay);
    d->handleMouseEvent(this, event);
}

#if QT_CONFIG(quicktemplates2_multitouch)
void QQuickOverlay::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickOverlay);
    d->handleTouchEvent(this, event);
}
#endif

void QQuickOverlay::handleWheelAndDnDEvents(QEvent *event)
{
    Q_D(QQuickOverlay);
    if (d->mouseGrabberPopup) {
        d->mouseGrabberPopup->overlayEvent(this, event);
        return;
    } else {
        const auto popups = d->stackingOrderPopups();
        for (QQuickPopup *popup : popups) {
            if (popup->overlayEvent(this, event))
                return;
        }
    }
    event->ignore();
}

#if QT_CONFIG(wheelevent)
void QQuickOverlay::wheelEvent(QWheelEvent *event)
{
    handleWheelAndDnDEvents(event);
}
#endif

#if QT_CONFIG(quick_draganddrop)
void QQuickOverlay::dragEnterEvent(QDragEnterEvent *event)
{
    handleWheelAndDnDEvents(event);
}

void QQuickOverlay::dragMoveEvent(QDragMoveEvent *event)
{
    handleWheelAndDnDEvents(event);
}

void QQuickOverlay::dragLeaveEvent(QDragLeaveEvent *event)
{
    handleWheelAndDnDEvents(event);
}

void QQuickOverlay::dropEvent(QDropEvent *event)
{
    handleWheelAndDnDEvents(event);
}
#endif

/*!
    \internal

    When clicking inside a scene with a single active popup, a few things can happen.
    The click can be inside the popup item, in which case, we stop filtering to allow
    normal event handling. Or the click can be outside the popup, in which case,
    it will normally be inside the overlay, or the popup's dimmer item.

    When dealing with nested popups, the second popup's dimmer (or popupItem if the dimmer is absent)
    will become an exclusive grabber to the pointerEvent, during childMouseEventFilter.
    (sometimes the overlay becomes the exclusive grabber instead, why?).
*/
bool QQuickOverlay::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    Q_D(QQuickOverlay);
    const auto popups = d->stackingOrderPopups();
    for (QQuickPopup *popup : popups) {
        QQuickPopupPrivate *p = QQuickPopupPrivate::get(popup);

        // Stop filtering overlay events when reaching a popup item or an item
        // that is inside the popup. Let the popup content handle its events.
        if (item == p->popupItem || p->popupItem->isAncestorOf(item))
            break;

        // Let the popup try closing itself when pressing or releasing over its
        // background dimming OR over another popup underneath, in case the popup
        // does not have background dimming.
        if (item == p->dimmer || !p->popupItem->isAncestorOf(item)) {
            bool handled = false;
            switch (event->type()) {
#if QT_CONFIG(quicktemplates2_multitouch)
            case QEvent::TouchBegin:
            case QEvent::TouchUpdate:
            case QEvent::TouchEnd:
                handled = d->handleTouchEvent(item, static_cast<QTouchEvent *>(event), popup);
                break;
#endif
            case QEvent::HoverEnter:
            case QEvent::HoverMove:
            case QEvent::HoverLeave:
                // If the control item has already been hovered, allow the hover leave event
                // to be processed by the same item for resetting its internal hovered state
                // instead of filtering it here.
                if (auto *control = qobject_cast<QQuickControl *>(item)) {
                    if (control->isHovered() && event->type() == QEvent::HoverLeave)
                        return false;
                }
                handled = d->handleHoverEvent(item, static_cast<QHoverEvent *>(event), popup);
                break;

            case QEvent::MouseButtonPress:
            case QEvent::MouseMove:
            case QEvent::MouseButtonRelease:
                handled = d->handleMouseEvent(item, static_cast<QMouseEvent *>(event), popup);
                break;

            default:
                break;
            }
            if (handled)
                return true;
        }
    }
    return false;
}

/*!
    \internal

    The overlay installs itself as an event filter on the window it belongs to.
    It will filter Touch, Mouse (press and release) and Wheel related events.

    Touch and MousePress events will be passed to the delivery agent for normal event propagation,
    where they will be filtered by the overlay again in QQuickOverlay::childMouseEventFilter.
    All opened popups will then be iterated, to check if the event should close the popup or not.
    Also modality is checked to determine the return type, which will cause the delivery agent to
    continue normal propagation in this second childMouseEventFilter filtering step.

    The reason for installing the eventFilter is to allow non-modal popups with CloseOnReleaseOutside
    as the closing policy to close when clicking the overlay. The delivery agent won't send the
    release event to the overlay when clicking it directly, only the press event.
*/
bool QQuickOverlay::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QQuickOverlay);
    if (!isVisible() || object != d->window)
        return false;

#if QT_CONFIG(wheelevent) || QT_CONFIG(quick_draganddrop)
    auto targetItemsForEvent = [&](QEvent *){
#if QT_CONFIG(wheelevent)
        if (event->type() == QEvent::Wheel) {
            QWheelEvent *we = static_cast<QWheelEvent *>(event);
            return d->deliveryAgentPrivate()->pointerTargets(
                    d->window->contentItem(), we, we->point(0), false, false);
        }
#endif
#if QT_CONFIG(quick_draganddrop)
        bool isDnDEvent = false;
        switch (event->type()) {
        case QEvent::DragEnter:
        case QEvent::DragMove:
        case QEvent::Drop:
            isDnDEvent = true;
            break;
        default:
            break;
        }
        if (isDnDEvent) {
            QDropEvent *de = static_cast<QDropEvent *>(event);
            auto position = mapFromScene(de->position());
            return d->deliveryAgentPrivate()->eventTargets(
                    d->window->contentItem(), de, -1, position, de->position(),
                    [](QQuickItem *item, const QEvent *) -> std::optional<bool> {
                        // Only consider actual drop targets; still recurse into
                        // non-accepting items so their children can be found.
                        if (!item->flags().testFlag(QQuickItem::ItemAcceptsDrops))
                            return false;
                        return std::nullopt; // use default containment check
                    });
        }
#endif
        // Not needed so far
        Q_UNREACHABLE_RETURN(QList<QQuickItem *> {});
    };
#endif

    switch (event->type()) {
#if QT_CONFIG(quicktemplates2_multitouch)
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        if (static_cast<QTouchEvent *>(event)->touchPointStates() & QEventPoint::Pressed)
            emit pressed();
        if (static_cast<QTouchEvent *>(event)->touchPointStates() & QEventPoint::Released)
            emit released();

        // allow non-modal popups to close on touch release outside
        if (!d->mouseGrabberPopup) {
            QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
            for (const QTouchEvent::TouchPoint &point : touchEvent->points()) {
                if (point.state() == QEventPoint::Released) {
                    QQuickDeliveryAgentPrivate::translateTouchEvent(touchEvent);
                    if (d->handleRelease(d->window->contentItem(), event, nullptr))
                        break;
                }
            }
        }

        // setup currentEventDeliveryAgent like in QQuickDeliveryAgent::event
        QQuickDeliveryAgentPrivate::currentEventDeliveryAgent = d->deliveryAgent();
        d->deliveryAgentPrivate()->handleTouchEvent(static_cast<QTouchEvent *>(event));
        QQuickDeliveryAgentPrivate::currentEventDeliveryAgent = nullptr;

        // If a touch event hasn't been accepted after being delivered, there
        // were no items interested in touch events at any of the touch points.
        // Make sure to accept the touch event in order to receive the consequent
        // touch events, to be able to close non-modal popups on release outside.
        event->accept();
        // Since we eat the event, QQuickWindow::event never sees it to clean up the
        // grabber states. So we have to do so explicitly.
        d->deliveryAgentPrivate()->clearGrabbers(static_cast<QPointerEvent *>(event));
        return true;
#endif

    case QEvent::MouseButtonPress: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        // Don't filter right mouse button clicks, as it prevents ContextMenu from
        // receiving QContextMenuEvents as long as e.g. a Drawer exists, even if it's not visible.
        // This does not prevent popups from being closed with the right mouse button,
        // as mousePressEvent takes care of that.
        if (mouseEvent->button() == Qt::RightButton)
            break;

#if QT_CONFIG(quicktemplates2_multitouch)
        // do not emit pressed() twice when mouse events have been synthesized from touch events
        if (mouseEvent->source() == Qt::MouseEventNotSynthesized)
#endif
            emit pressed();

        // setup currentEventDeliveryAgent like in QQuickDeliveryAgent::event
        QQuickDeliveryAgentPrivate::currentEventDeliveryAgent = d->deliveryAgent();
        d->deliveryAgentPrivate()->handleMouseEvent(mouseEvent);
        QQuickDeliveryAgentPrivate::currentEventDeliveryAgent = nullptr;

        // If a mouse event hasn't been accepted after being delivered, there
        // was no item interested in mouse events at the mouse point. Make sure
        // to accept the mouse event in order to receive the consequent mouse
        // events, to be able to close non-modal popups on release outside.
        event->accept();
        return true;
    }
    case QEvent::MouseButtonRelease: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton)
            break;

#if QT_CONFIG(quicktemplates2_multitouch)
        // do not emit released() twice when mouse events have been synthesized from touch events
        if (mouseEvent->source() == Qt::MouseEventNotSynthesized)
#endif
            emit released();

        // allow non-modal popups to close on mouse release outside
        if (!d->mouseGrabberPopup)
            d->handleRelease(d->window->contentItem(), event, nullptr);
        break;
    }
#if QT_CONFIG(quick_draganddrop)
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::Drop: {
        // Find which item the drag is targeting and block if a modal popup is in front.
        const QList<QQuickItem *> targetItems = targetItemsForEvent(event);
        if (targetItems.isEmpty())
            break;
        QQuickItem *const dimmerItem = property("_q_dimmerItem").value<QQuickItem *>();
        QQuickItem *const topItem = targetItems.first();
        QQuickItem *item = qobject_cast<QQuickPopupItem *>(topItem) ? topItem : nullptr;
        if (!item) {
            item = topItem;
            while ((item = item->parentItem())) {
                if (qobject_cast<QQuickPopupItem *>(item))
                    break;
            }
        }
        if (!item && dimmerItem != topItem && isAncestorOf(topItem))
            break;
        for (const auto &popup : d->stackingOrderPopups()) {
            const QQuickItem *popupItem = popup->popupItem();
            if (!popupItem)
                continue;
            if (popupItem == item)
                break;
            if (popup->overlayEvent(topItem, event))
                return true;
        }
        break;
    }
#endif
#if QT_CONFIG(wheelevent)
    case QEvent::Wheel:
        return d->eatEventIfBlockedByModal(static_cast<QWheelEvent *>(event));
#endif
#if QT_CONFIG(tabletevent)
    case QEvent::TabletMove:
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
        return d->eatEventIfBlockedByModal(static_cast<QTabletEvent *>(event));
#endif
    default:
        break;
    }

    return false;
}

class QQuickOverlayAttachedPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickOverlayAttached)

    void setWindow(QQuickWindow *newWindow);
    void setWindowAndParent(QQuickWindow *newWindow, QQuickItem *parent);

    QQuickWindow *window = nullptr;
    QQmlComponent *modal = nullptr;
    QQmlComponent *modeless = nullptr;
    QQuickItem *parentItem = nullptr;
};

void QQuickOverlayAttachedPrivate::setWindow(QQuickWindow *newWindow)
{
    setWindowAndParent(newWindow, parentItem);
}

void QQuickOverlayAttachedPrivate::setWindowAndParent(QQuickWindow *newWindow, QQuickItem *parent)
{
    Q_Q(QQuickOverlayAttached);
    if (window == newWindow)
        return;

    if (QQuickOverlay *oldOverlay = QQuickOverlay::overlay(window, parent)) {
        QObject::disconnect(oldOverlay, &QQuickOverlay::pressed, q, &QQuickOverlayAttached::pressed);
        QObject::disconnect(oldOverlay, &QQuickOverlay::released, q, &QQuickOverlayAttached::released);
    }

    if (QQuickOverlay *newOverlay = QQuickOverlay::overlay(newWindow, parent)) {
        QObject::connect(newOverlay, &QQuickOverlay::pressed, q, &QQuickOverlayAttached::pressed);
        QObject::connect(newOverlay, &QQuickOverlay::released, q, &QQuickOverlayAttached::released);
    }

    window = newWindow;
    if (parent)
        parentItem = parent;
    emit q->overlayChanged();
}

/*!
    \qmlattachedsignal QtQuick.Controls::Overlay::pressed()

    This attached signal is emitted when the overlay is pressed by the user while
    a popup is visible.

    The signal can be attached to any item, popup, or window. When attached to an
    item or a popup, the signal is only emitted if the item or popup is in a window.

    \include qquickoverlay-pressed-released.qdocinc
*/

/*!
    \qmlattachedsignal QtQuick.Controls::Overlay::released()

    This attached signal is emitted when the overlay is released by the user while
    a popup is visible.

    The signal can be attached to any item, popup, or window. When attached to an
    item or a popup, the signal is only emitted if the item or popup is in a window.

    \include qquickoverlay-pressed-released.qdocinc
*/

QQuickOverlayAttached::QQuickOverlayAttached(QObject *parent)
    : QObject(*(new QQuickOverlayAttachedPrivate), parent)
{
    Q_D(QQuickOverlayAttached);
    if (QQuickItem *item = qobject_cast<QQuickItem *>(parent)) {
        d->setWindowAndParent(item->window(), item);
        QObjectPrivate::connect(item, &QQuickItem::windowChanged, d, &QQuickOverlayAttachedPrivate::setWindow);
    } else if (QQuickPopup *popup = qobject_cast<QQuickPopup *>(parent)) {
        d->setWindowAndParent(popup->window(), popup->parentItem());
        QObjectPrivate::connect(popup, &QQuickPopup::windowChanged, d, &QQuickOverlayAttachedPrivate::setWindow);
    } else {
        d->setWindow(qobject_cast<QQuickWindow *>(parent));
    }
}

/*!
    \qmlattachedproperty Overlay QtQuick.Controls::Overlay::overlay
    \readonly

    This attached property holds the window overlay item.

    The property can be attached to any item, popup, or window. When attached to an
    item or a popup, the value is \c null if the item or popup is not in a window.
*/
QQuickOverlay *QQuickOverlayAttached::overlay() const
{
    Q_D(const QQuickOverlayAttached);
    return QQuickOverlay::overlay(d->window, d->parentItem);
}

/*!
    \qmlattachedproperty Component QtQuick.Controls::Overlay::modal

    This attached property holds a component to use as a visual item that implements
    background dimming for modal popups. It is created for and stacked below visible
    modal popups.

    The property can be attached to any popup.

    For example, to change the color of the background dimming for a modal
    popup, the following code can be used:

    \snippet qtquickcontrols-overlay-modal.qml 1

    \sa Popup::modal
*/
QQmlComponent *QQuickOverlayAttached::modal() const
{
    Q_D(const QQuickOverlayAttached);
    return d->modal;
}

void QQuickOverlayAttached::setModal(QQmlComponent *modal)
{
    Q_D(QQuickOverlayAttached);
    if (d->modal == modal)
        return;

    d->modal = modal;
    emit modalChanged();
}

/*!
    \qmlattachedproperty Component QtQuick.Controls::Overlay::modeless

    This attached property holds a component to use as a visual item that implements
    background dimming for modeless popups. It is created for and stacked below visible
    dimming popups.

    The property can be attached to any popup.

    For example, to change the color of the background dimming for a modeless
    popup, the following code can be used:

    \snippet qtquickcontrols-overlay-modeless.qml 1

    \sa Popup::dim
*/
QQmlComponent *QQuickOverlayAttached::modeless() const
{
    Q_D(const QQuickOverlayAttached);
    return d->modeless;
}

void QQuickOverlayAttached::setModeless(QQmlComponent *modeless)
{
    Q_D(QQuickOverlayAttached);
    if (d->modeless == modeless)
        return;

    d->modeless = modeless;
    emit modelessChanged();
}

QT_END_NAMESPACE

#include "moc_qquickoverlay_p.cpp"
