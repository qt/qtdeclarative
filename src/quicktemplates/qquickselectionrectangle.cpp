// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickselectionrectangle_p.h"
#include "qquickselectionrectangle_p_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/private/qquickdraghandler_p.h>
#include <QtQuick/private/qquickhoverhandler_p.h>

#include <QtQuick/private/qquicktableview_p_p.h>

#include "qquickscrollview_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype SelectionRectangle
    \inherits Control
//!     \nativetype QQuickSelectionRectangle
    \inqmlmodule QtQuick.Controls
    \since 6.2
    \ingroup utilities
    \brief Used to select table cells inside a TableView.

    \image qtquickcontrols-selectionrectangle.png

    SelectionRectangle is used for selecting table cells in a TableView. It lets
    the user start a selection by doing a pointer drag inside the viewport, or by
    doing a long press on top of a cell.

    For a SelectionRectangle to be able to select cells, TableView must have
    an ItemSelectionModel assigned. The ItemSelectionModel will store any
    selections done on the model, and can be used for querying
    which cells that the user has selected.

    The following example shows how you can make a SelectionRectangle target
    a TableView:

    \snippet qtquickcontrols-selectionrectangle.qml 0

    \note A SelectionRectangle itself is not shown as part of a selection. Only the
    delegates (like topLeftHandle and bottomRightHandle) are used.
    You should also consider \l {Selecting items}{rendering the TableView delegate as selected}.

    \sa TableView, TableView::selectionModel, ItemSelectionModel
*/

/*!
    \qmlproperty Item QtQuick.Controls::SelectionRectangle::target

    This property holds the TableView on which the
    SelectionRectangle should act.
*/

/*!
    \qmlproperty bool QtQuick.Controls::SelectionRectangle::active
    \readonly

    This property is \c true while the user is performing a
    selection. The selection will be active from the time the
    the user starts to select, and until the selection is
    removed again, for example from tapping inside the viewport.
*/

/*!
    \qmlproperty bool QtQuick.Controls::SelectionRectangle::dragging
    \readonly

    This property is \c true whenever the user is doing a pointer drag or
    a handle drag to adjust the selection rectangle.
*/

/*!
    \qmlproperty Component QtQuick.Controls::SelectionRectangle::topLeftHandle

    This property holds the delegate that will be shown on the center of the
    top-left corner of the selection rectangle. When a handle is
    provided, the user can drag it to adjust the selection.

    The handle is not hidden by default when a selection is removed.
    Instead, this is the responsibility of the delegate, to open up for
    custom fade-out animations. The easiest way to ensure that the handle
    ends up hidden, is to simply bind \l {Item::}{visible} to the \l active
    state of the SelectionRectangle:

    \qml
    SelectionRectangle {
        topLeftHandle: Rectangle {
            width: 20
            height: 20
            visible: SelectionRectangle.control.active
        }
    }
    \endqml

    Set this property to \c null if you don't want a selection handle on the top-left.

    \sa bottomRightHandle
*/

/*!
    \qmlproperty Component QtQuick.Controls::SelectionRectangle::bottomRightHandle

    This property holds the delegate that will be shown on the center of the
    top-left corner of the selection rectangle. When a handle is
    provided, the user can drag it to adjust the selection.

    The handle is not hidden by default when a selection is removed.
    Instead, this is the responsibility of the delegate, to open up for
    custom fade-out animations. The easiest way to ensure that the handle
    ends up hidden, is to simply bind \l {Item::}{visible} to the \l active
    state of the SelectionRectangle:

    \qml
    SelectionRectangle {
        bottomRightHandle: Rectangle {
            width: 20
            height: 20
            visible: SelectionRectangle.control.active
        }
    }
    \endqml

    Set this property to \c null if you don't want a selection handle on the bottom-right.

    \sa topLeftHandle
*/

/*!
    \qmlproperty enumeration QtQuick.Controls::SelectionRectangle::selectionMode

    This property holds when a selection should start.

    \value SelectionRectangle.Drag A selection will start by doing a pointer drag inside the viewport
    \value SelectionRectangle.PressAndHold A selection will start by doing a press and hold on top of a cell
    \value SelectionRectangle.Auto SelectionRectangle will choose which mode to
        use based on the \l target and the input device in use. This normally
        means \c Drag when using a mouse, and \c PressAndHold on a touchscreen.
        However, \c Drag will only be used if it doesn't conflict with flicking.
        One way to avoid conflict is to disable mouse-drag flicking by setting
        \l {Flickable::}{acceptedButtons} to \c Qt.NoButton. In that case, the
        mouse wheel or touchpad scrolling gesture continues to work, touchscreen
        flicking continues to work, and touchscreen selection can be started by
        press-and-hold. Another way is to set \l {Flickable::}{interactive} to
        \c false, which disables flicking and scrolling altogether (perhaps
        this should be done only temporarily, in some mode in your UI).
        Yet another way is to place the TableView inside a ScrollView (where
        flicking, by default, is off for mouse events).

    The default value is \c Auto.
*/

/*!
    \qmlattachedproperty SelectionRectangle QtQuick.Controls::SelectionRectangle::control

    This attached property holds the SelectionRectangle that manages the delegate instance.
    It is attached to each handle instance.
*/

/*!
    \qmlattachedproperty bool QtQuick.Controls::SelectionRectangle::dragging

    This attached property will be \c true if the user is dragging on the handle.
    It is attached to each handle instance.
*/

QQuickSelectionRectanglePrivate::QQuickSelectionRectanglePrivate()
    : QQuickControlPrivate()
{
    m_tapHandler = new QQuickTapHandler();
    m_dragHandler = new QQuickDragHandler();
    m_dragHandler->setTarget(nullptr);

    QObject::connect(&m_scrollTimer, &QTimer::timeout, [&]{
        if (m_topLeftHandle && m_draggedHandle == m_topLeftHandle.data())
            m_selectable->setSelectionStartPos(m_scrollToPoint);
        else
            m_selectable->setSelectionEndPos(m_scrollToPoint);
        updateHandles();
        const QSizeF dist = m_selectable->scrollTowardsPoint(m_scrollToPoint, m_scrollSpeed);
        m_scrollToPoint.rx() += dist.width() > 0 ? m_scrollSpeed.width() : -m_scrollSpeed.width();
        m_scrollToPoint.ry() += dist.height() > 0 ? m_scrollSpeed.height() : -m_scrollSpeed.height();
        m_scrollSpeed = QSizeF(qAbs(dist.width() * 0.007), qAbs(dist.height() * 0.007));
    });

    QObject::connect(m_tapHandler, &QQuickTapHandler::pressedChanged, [this]() {
        Q_Q(QQuickSelectionRectangle);

        if (!m_tapHandler->isPressed()) {
            // Deactivate the selection rectangle when the tap handler
            // is released and there's no selection
            if (q->active() && !m_selectable->hasSelection())
                updateActiveState(false);
            return;
        }
        if (m_effectiveSelectionMode != QQuickSelectionRectangle::Drag)
            return;

        const QPointF pos = m_tapHandler->point().pressPosition();
        const auto modifiers = m_tapHandler->point().modifiers();
        if (modifiers & ~(Qt::ControlModifier | Qt::ShiftModifier))
            return;

        // A selection rectangle is not valid when its width or height is 0
        const auto isSelectionRectValid = [](const QRectF &selectionRect) -> bool {
            return ((selectionRect.width() != 0) || (selectionRect.height() != 0));
        };

        if (modifiers & Qt::ShiftModifier) {
            // Extend the selection towards the pressed cell. If there is no
            // existing selection, start a new selection from the current item
            // to the pressed item.
            if (!m_active) {
                if (!m_selectable->startSelection(pos, modifiers))
                    return;
                m_selectable->setSelectionStartPos(QPoint{-1, -1});
            }
            m_selectable->setSelectionEndPos(pos);
            // Don't update and activate the selection handlers when
            // the size of m_selectable's selection rectangle is 0.
            if (!isSelectionRectValid(m_selectable->selectionRectangle()))
                return;
            updateHandles();
            updateActiveState(true);
        } else if (modifiers & Qt::ControlModifier) {
            // Select a single cell, but keep the old selection (unless
            // m_selectable->startSelection(pos. modifiers) returns false, which
            // it will if selectionMode only allows a single selection).
            if (handleUnderPos(pos) != nullptr) {
                // Don't allow press'n'hold to start a new
                // selection if it started on top of a handle.
                return;
            }

            if (!m_selectable->startSelection(pos, modifiers))
                return;
            m_selectable->setSelectionStartPos(pos);
            m_selectable->setSelectionEndPos(pos);
            // Don't update and activate the selection handlers when
            // the size of m_selectable's selection rectangle is 0.
            if (!isSelectionRectValid(m_selectable->selectionRectangle()))
                return;
            updateHandles();
            updateActiveState(true);
        }
    });

    QObject::connect(m_tapHandler, &QQuickTapHandler::longPressed, [this]() {
        if (m_tapHandler->point().device()->type() == QInputDevice::DeviceType::TouchScreen &&
            m_selectionMode == QQuickSelectionRectangle::Auto) {
            const QQuickTableView *tableview = qobject_cast<QQuickTableView *>(m_target);
            if (tableview && !tableview->isInteractive())
                return; // you can select by touch drag, so don't allow touch long-press
            // otherwise, touch long-press is allowed
        } else if (m_effectiveSelectionMode != QQuickSelectionRectangle::PressAndHold) {
            return;
        }

        const QPointF pos = m_tapHandler->point().pressPosition();
        const auto modifiers = m_tapHandler->point().modifiers();
        if (handleUnderPos(pos) != nullptr) {
            // Don't allow press'n'hold to start a new
            // selection if it started on top of a handle.
            return;
        }

        // A selection rectangle is not valid when its width or height is 0
        const auto isSelectionRectValid = [](const QRectF &selectionRect) -> bool {
            return ((selectionRect.width() != 0) || (selectionRect.height() != 0));
        };

        if (modifiers == Qt::ShiftModifier) {
            // Extend the selection towards the pressed cell. If there is no
            // existing selection, start a new selection from the current item
            // to the pressed item.
            if (!m_active) {
                if (!m_selectable->startSelection(pos, modifiers))
                    return;
                m_selectable->setSelectionStartPos(QPoint{-1, -1});
            }
            m_selectable->setSelectionEndPos(pos);
            if (!isSelectionRectValid(m_selectable->selectionRectangle()))
                return;
            updateHandles();
            updateActiveState(true);
        } else {
            // Select a single cell. m_selectable->startSelection() will decide
            // if the existing selection should also be cleared.
            if (!m_selectable->startSelection(pos, modifiers))
                return;
            m_selectable->setSelectionStartPos(pos);
            m_selectable->setSelectionEndPos(pos);
            if (!isSelectionRectValid(m_selectable->selectionRectangle()))
                return;
            updateHandles();
            updateActiveState(true);
        }
    });

    QObject::connect(m_dragHandler, &QQuickDragHandler::activeChanged, [this]() {
        Q_ASSERT(m_effectiveSelectionMode == QQuickSelectionRectangle::Drag);
        const QPointF startPos = m_dragHandler->centroid().pressPosition();
        const QPointF dragPos = m_dragHandler->centroid().position();
        const auto modifiers = m_dragHandler->centroid().modifiers();
        if (modifiers & ~(Qt::ControlModifier | Qt::ShiftModifier))
            return;

        // A selection rectangle is not valid when its width or height is 0
        const auto isSelectionRectValid = [](const QRectF &selectionRect) -> bool {
            return ((selectionRect.width() != 0) || (selectionRect.height() != 0));
        };

        if (m_dragHandler->active()) {
            // Start a new selection unless there is an active selection
            // already, and one of the relevant modifiers are being held.
            // In that case we continue to extend the active selection instead.
            const bool modifiersHeld = modifiers & (Qt::ControlModifier | Qt::ShiftModifier);
            if (!m_active || !modifiersHeld) {
                if (!m_selectable->startSelection(startPos, modifiers))
                    return;
                m_selectable->setSelectionStartPos(startPos);
            }
            m_selectable->setSelectionEndPos(dragPos);
            m_draggedHandle = nullptr;
            if (!isSelectionRectValid(m_selectable->selectionRectangle()))
                return;
            updateHandles();
            updateActiveState(true);
            updateDraggingState(true);
        } else {
            m_scrollTimer.stop();
            m_selectable->normalizeSelection();
            updateDraggingState(false);
        }
    });

    QObject::connect(m_dragHandler, &QQuickDragHandler::centroidChanged, [this]() {
        if (!m_dragging)
            return;
        const QPointF pos = m_dragHandler->centroid().position();
        m_selectable->setSelectionEndPos(pos);
        updateHandles();
        scrollTowardsPos(pos);
    });
}

void QQuickSelectionRectanglePrivate::scrollTowardsPos(const QPointF &pos)
{
    m_scrollToPoint = pos;
    if (m_scrollTimer.isActive())
        return;

    const QSizeF dist = m_selectable->scrollTowardsPoint(m_scrollToPoint, m_scrollSpeed);
    if (!dist.isNull())
        m_scrollTimer.start(1);
}

QQuickItem *QQuickSelectionRectanglePrivate::handleUnderPos(const QPointF &pos)
{
    const auto handlerTarget = m_selectable->selectionPointerHandlerTarget();
    if (m_topLeftHandle) {
        const QPointF localPos = m_topLeftHandle->mapFromItem(handlerTarget, pos);
        if (m_topLeftHandle->contains(localPos))
            return m_topLeftHandle.data();
    }

    if (m_bottomRightHandle) {
        const QPointF localPos = m_bottomRightHandle->mapFromItem(handlerTarget, pos);
        if (m_bottomRightHandle->contains(localPos))
            return m_bottomRightHandle.data();
    }

    return nullptr;
}

void QQuickSelectionRectanglePrivate::updateDraggingState(bool dragging)
{
    if (dragging != m_dragging) {
        m_dragging = dragging;
        emit q_func()->draggingChanged();
    }

    if (auto attached = getAttachedObject(m_draggedHandle))
        attached->setDragging(dragging);
}

void QQuickSelectionRectanglePrivate::updateActiveState(bool active)
{
    if (active == m_active)
        return;

    m_active = active;

    if (const auto tableview = qobject_cast<QQuickTableView *>(m_target)) {
        if (active) {
            // If the position of rows and columns changes, we'll need to reposition the handles
            connect(tableview, &QQuickTableView::layoutChanged, this, &QQuickSelectionRectanglePrivate::updateHandles);
        } else {
            disconnect(tableview, &QQuickTableView::layoutChanged, this, &QQuickSelectionRectanglePrivate::updateHandles);
        }
    }

    emit q_func()->activeChanged();
}

QQuickItem *QQuickSelectionRectanglePrivate::createHandle(QQmlComponent *delegate, Qt::Corner corner)
{
    Q_Q(QQuickSelectionRectangle);

    // Incubate the handle
    QObject *obj = delegate->beginCreate(QQmlEngine::contextForObject(q));
    QQuickItem *handleItem = qobject_cast<QQuickItem*>(obj);
    const auto handlerTarget = m_selectable->selectionPointerHandlerTarget();
    handleItem->setParentItem(handlerTarget);
    if (auto attached = getAttachedObject(handleItem))
        attached->setControl(q);
    delegate->completeCreate();
    if (handleItem->z() == 0)
        handleItem->setZ(100);

    // Add pointer handlers to it
    QQuickDragHandler *dragHandler = new QQuickDragHandler();
    dragHandler->setTarget(nullptr);
    dragHandler->setParentItem(handleItem);
    dragHandler->setGrabPermissions(QQuickPointerHandler::CanTakeOverFromAnything);

    QQuickHoverHandler *hoverHandler = new QQuickHoverHandler();
    hoverHandler->setTarget(nullptr);
    hoverHandler->setParentItem(handleItem);
#if QT_CONFIG(cursor)
    hoverHandler->setCursorShape(Qt::SizeFDiagCursor);
#endif
    hoverHandler->setBlocking(true);

    // Add a dummy TapHandler that blocks the user from being
    // able to tap on a tap handler underneath the handle.
    QQuickTapHandler *tapHandler = new QQuickTapHandler();
    tapHandler->setTarget(nullptr);
    tapHandler->setParentItem(handleItem);
    // Set a dummy gesture policy so that the tap handler
    // will get an exclusive grab already on press
    tapHandler->setGesturePolicy(QQuickTapHandler::DragWithinBounds);

    QObject::connect(dragHandler, &QQuickDragHandler::activeChanged, [this, corner, handleItem, dragHandler]() {
        if (dragHandler->active()) {
            const QPointF localPos = dragHandler->centroid().position();
            const QPointF pos = handleItem->mapToItem(handleItem->parentItem(), localPos);
            if (corner == Qt::TopLeftCorner)
                m_selectable->setSelectionStartPos(pos);
            else
                m_selectable->setSelectionEndPos(pos);

            m_draggedHandle = handleItem;
            updateHandles();
            updateDraggingState(true);
#if QT_CONFIG(cursor)
            QGuiApplication::setOverrideCursor(Qt::SizeFDiagCursor);
#endif
        } else {
            m_scrollTimer.stop();
            m_selectable->normalizeSelection();
            updateDraggingState(false);
#if QT_CONFIG(cursor)
            QGuiApplication::restoreOverrideCursor();
#endif
        }
    });

    QObject::connect(dragHandler, &QQuickDragHandler::centroidChanged, [this, corner, handleItem, dragHandler]() {
        if (!m_dragging)
            return;

        const QPointF localPos = dragHandler->centroid().position();
        const QPointF pos = handleItem->mapToItem(handleItem->parentItem(), localPos);
        if (corner == Qt::TopLeftCorner)
            m_selectable->setSelectionStartPos(pos);
        else
            m_selectable->setSelectionEndPos(pos);

        updateHandles();
        scrollTowardsPos(pos);
    });

    return handleItem;
}

void QQuickSelectionRectanglePrivate::updateHandles()
{
    const QRectF rect = m_selectable->selectionRectangle().normalized();

    if (!m_topLeftHandle && m_topLeftHandleDelegate)
        m_topLeftHandle.reset(createHandle(m_topLeftHandleDelegate, Qt::TopLeftCorner));

    if (!m_bottomRightHandle && m_bottomRightHandleDelegate)
        m_bottomRightHandle.reset(createHandle(m_bottomRightHandleDelegate, Qt::BottomRightCorner));

    if (m_topLeftHandle) {
        m_topLeftHandle->setX(rect.x() - (m_topLeftHandle->width() / 2));
        m_topLeftHandle->setY(rect.y() - (m_topLeftHandle->height() / 2));
    }

    if (m_bottomRightHandle) {
        m_bottomRightHandle->setX(rect.x() + rect.width() - (m_bottomRightHandle->width() / 2));
        m_bottomRightHandle->setY(rect.y() + rect.height() - (m_bottomRightHandle->height() / 2));
    }
}

void QQuickSelectionRectanglePrivate::connectToTarget()
{
    // To support QuickSelectionRectangle::Auto, we need to listen for changes to the target
    if (const auto flickable = qobject_cast<QQuickFlickable *>(m_target)) {
        connect(flickable, &QQuickFlickable::interactiveChanged, this, &QQuickSelectionRectanglePrivate::updateSelectionMode);
        connect(flickable, &QQuickFlickable::acceptedButtonsChanged, this, &QQuickSelectionRectanglePrivate::updateSelectionMode);
    }

    // Add a callback function that tells if the selection was
    // modified outside of the actions taken by SelectionRectangle.
    m_selectable->setCallback([this](QQuickSelectable::CallBackFlag flag){
        switch (flag) {
        case QQuickSelectable::CallBackFlag::CancelSelection:
            // The selection is either cleared, or can no longer be
            // represented as a rectangle with two selection handles.
            updateActiveState(false);
            break;
        case QQuickSelectable::CallBackFlag::SelectionRectangleChanged:
            // The selection has changed, but the selection is still
            // rectangular and without holes.
            updateHandles();
            break;
        default:
            Q_UNREACHABLE();
        }
    });
}

void QQuickSelectionRectanglePrivate::updateSelectionMode()
{
    Q_Q(QQuickSelectionRectangle);

    const bool enabled = q->isEnabled();
    m_tapHandler->setEnabled(enabled);

    if (m_selectionMode == QQuickSelectionRectangle::Auto) {
        if (m_target && qobject_cast<QQuickScrollView *>(m_target->parentItem())) {
            // ScrollView allows flicking with touch, but not with mouse. So we do
            // the opposite here: you can drag to select with a mouse, but not with touch.
            m_effectiveSelectionMode = QQuickSelectionRectangle::Drag;
            m_dragHandler->setAcceptedDevices(QInputDevice::DeviceType::Mouse);
            m_dragHandler->setEnabled(enabled);
        } else if (const auto flickable = qobject_cast<QQuickFlickable *>(m_target)) {
            // Flickable allows flicking with mouse by default, but it can be disabled by
            // changing acceptedMouseButtons(). Setting interactive to false disables flicking
            // altogether. So we allow Drag with devices that don't conflict with flicking.
            if (enabled && (!flickable->isInteractive() ||
                            !flickable->acceptedMouseButtons().testFlag(Qt::LeftButton))) {
                m_effectiveSelectionMode = QQuickSelectionRectangle::Drag;
                m_dragHandler->setAcceptedDevices(flickable->isInteractive()
                                                          ? QInputDevice::DeviceType::Mouse
                                                          : QInputDevice::DeviceType::AllDevices);
                m_dragHandler->setEnabled(true);
            } else {
                m_effectiveSelectionMode = QQuickSelectionRectangle::PressAndHold;
                m_dragHandler->setEnabled(false);
            }
        } else {
            m_effectiveSelectionMode = QQuickSelectionRectangle::Drag;
            m_dragHandler->setAcceptedDevices(QInputDevice::DeviceType::Mouse);
            m_dragHandler->setEnabled(enabled);
        }
    } else if (m_selectionMode == QQuickSelectionRectangle::Drag) {
        m_effectiveSelectionMode = QQuickSelectionRectangle::Drag;
        m_dragHandler->setAcceptedDevices(QInputDevice::DeviceType::AllDevices);
        m_dragHandler->setEnabled(enabled);
    } else {
        m_effectiveSelectionMode = QQuickSelectionRectangle::PressAndHold;
        m_dragHandler->setEnabled(false);
    }
}

QQuickSelectionRectangleAttached *QQuickSelectionRectanglePrivate::getAttachedObject(const QObject *object) const
{
    QObject *attachedObject = qmlAttachedPropertiesObject<QQuickSelectionRectangle>(object);
    return static_cast<QQuickSelectionRectangleAttached *>(attachedObject);
}

// --------------------------------------------------------

QQuickSelectionRectangle::QQuickSelectionRectangle(QQuickItem *parent)
    : QQuickControl(*(new QQuickSelectionRectanglePrivate), parent)
{
    Q_D(QQuickSelectionRectangle);
    d->m_tapHandler->setParent(this);
    d->m_dragHandler->setParent(this);

    QObject::connect(this, &QQuickItem::enabledChanged, [=]() {
        d->m_scrollTimer.stop();
        d->updateSelectionMode();
        d->updateDraggingState(false);
        d->updateActiveState(false);
    });
}

QQuickItem *QQuickSelectionRectangle::target() const
{
    return d_func()->m_target;
}

void QQuickSelectionRectangle::setTarget(QQuickItem *target)
{
    Q_D(QQuickSelectionRectangle);
    if (d->m_target == target)
        return;

    if (d->m_selectable) {
        d->m_scrollTimer.stop();
        d->m_tapHandler->setParent(this);
        d->m_dragHandler->setParent(this);
        d->m_target->disconnect(this);
        d->m_selectable->setCallback(nullptr);
    }

    d->m_target = target;
    d->m_selectable = nullptr;

    if (d->m_target) {
        d->m_selectable = dynamic_cast<QQuickSelectable *>(QObjectPrivate::get(d->m_target.data()));
        if (!d->m_selectable)
            qmlWarning(this) << "the assigned target is not supported by the control";
    }

    if (d->m_selectable) {
        const auto handlerTarget = d->m_selectable->selectionPointerHandlerTarget();
        d->m_dragHandler->setParentItem(handlerTarget);
        d->m_tapHandler->setParentItem(handlerTarget);
        d->connectToTarget();
        d->updateSelectionMode();
    }

    emit targetChanged();
}

bool QQuickSelectionRectangle::active()
{
    return d_func()->m_active;
}

bool QQuickSelectionRectangle::dragging()
{
    return d_func()->m_dragging;
}

QQuickSelectionRectangle::SelectionMode QQuickSelectionRectangle::selectionMode() const
{
    return d_func()->m_selectionMode;
}

void QQuickSelectionRectangle::setSelectionMode(QQuickSelectionRectangle::SelectionMode selectionMode)
{
    Q_D(QQuickSelectionRectangle);
    if (d->m_selectionMode == selectionMode)
        return;

    d->m_selectionMode = selectionMode;

    if (d->m_target)
        d->updateSelectionMode();

    emit selectionModeChanged();
}

QQmlComponent *QQuickSelectionRectangle::topLeftHandle() const
{
    return d_func()->m_topLeftHandleDelegate;
}

void QQuickSelectionRectangle::setTopLeftHandle(QQmlComponent *topLeftHandle)
{
    Q_D(QQuickSelectionRectangle);
    if (d->m_topLeftHandleDelegate == topLeftHandle)
        return;

    d->m_topLeftHandleDelegate = topLeftHandle;
    emit topLeftHandleChanged();
}

QQmlComponent *QQuickSelectionRectangle::bottomRightHandle() const
{
    return d_func()->m_bottomRightHandleDelegate;
}

void QQuickSelectionRectangle::setBottomRightHandle(QQmlComponent *bottomRightHandle)
{
    Q_D(QQuickSelectionRectangle);
    if (d->m_bottomRightHandleDelegate == bottomRightHandle)
        return;

    d->m_bottomRightHandleDelegate = bottomRightHandle;
    emit bottomRightHandleChanged();
}

QQuickSelectionRectangleAttached *QQuickSelectionRectangle::qmlAttachedProperties(QObject *obj)
{
    return new QQuickSelectionRectangleAttached(obj);
}

QQuickSelectionRectangleAttached::QQuickSelectionRectangleAttached(QObject *parent)
    : QObject(parent)
{
}

QQuickSelectionRectangle *QQuickSelectionRectangleAttached::control() const
{
    return m_control;
}

void QQuickSelectionRectangleAttached::setControl(QQuickSelectionRectangle *control)
{
    if (m_control == control)
        return;

    m_control = control;
    emit controlChanged();
}

bool QQuickSelectionRectangleAttached::dragging() const
{
    return m_dragging;
}

void QQuickSelectionRectangleAttached::setDragging(bool dragging)
{
    if (m_dragging == dragging)
        return;

    m_dragging = dragging;
    emit draggingChanged();
}

QT_END_NAMESPACE

#include "moc_qquickselectionrectangle_p.cpp"
