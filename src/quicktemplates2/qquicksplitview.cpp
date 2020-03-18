/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
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
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicksplitview_p.h"
#include "qquicksplitview_p_p.h"
#include "qquickcontentitem_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qcborarray.h>
#include <QtCore/qcbormap.h>
#include <QtCore/qcborvalue.h>
#include <QtQml/QQmlInfo>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SplitView
    \inherits Control
//!     \instantiates QQuickSplitView
    \inqmlmodule QtQuick.Controls
    \since 5.13
    \ingroup qtquickcontrols2-containers
    \ingroup qtquickcontrols2-focusscopes
    \brief Lays out items with a draggable splitter between each item.

    SplitView is a control that lays out items horizontally or vertically with
    a draggable splitter between each item.

    SplitView supports the following attached properties on items it manages:

    \list
    \li \l{minimumWidth}{SplitView.minimumWidth}
    \li \l{minimumHeight}{SplitView.minimumHeight}
    \li \l{preferredWidth}{SplitView.preferredWidth}
    \li \l{preferredHeight}{SplitView.preferredHeight}
    \li \l{maximumWidth}{SplitView.maximumWidth}
    \li \l{maximumHeight}{SplitView.maximumHeight}
    \li \l{fillWidth}{SplitView.fillWidth} (true for only one child)
    \li \l{fillHeight}{SplitView.fillHeight} (true for only one child)
    \endlist

    In addition, each handle has the following read-only attached properties:

    \list
    \li \l{SplitHandle::hovered}{SplitHandle.hovered}
    \li \l{SplitHandle::pressed}{SplitHandle.pressed}
    \endlist

    \note Handles should be purely visual and not handle events, as it can
    interfere with their hovered and pressed states.

    The preferred size of items in a SplitView can be specified via
    \l{Item::}{implicitWidth} and \l{Item::}{implicitHeight} or
    \c SplitView.preferredWidth and \c SplitView.preferredHeight:

    \code
    SplitView {
        anchors.fill: parent

        Item {
            SplitView.preferredWidth: 50
        }

        // ...
    }
    \endcode

    For a horizontal SplitView, it's not necessary to specify the preferred
    height of each item, as they will be resized to the height of the view.
    This applies in reverse for vertical views.

    When a split handle is dragged, the \c SplitView.preferredWidth or
    \c SplitView.preferredHeight property is overwritten, depending on the
    \l orientation of the view.

    To limit the size of items in a horizontal view, use the following
    properties:

    \code
    SplitView {
        anchors.fill: parent

        Item {
            SplitView.minimumWidth: 25
            SplitView.preferredWidth: 50
            SplitView.maximumWidth: 100
        }

        // ...
    }
    \endcode

    To limit the size of items in a vertical view, use the following
    properties:

    \code
    SplitView {
        anchors.fill: parent
        orientation: Qt.Vertical

        Item {
            SplitView.minimumHeight: 25
            SplitView.preferredHeight: 50
            SplitView.maximumHeight: 100
        }

        // ...
    }
    \endcode

    There will always be one item (the fill item) in the SplitView that has
    \c SplitView.fillWidth set to \c true (or \c SplitView.fillHeight, if
    \l orientation is \c Qt.Vertical). This means that the item will get all
    leftover space when other items have been laid out. By default, the last
    visible child of the SplitView will have this set, but it can be changed by
    explicitly setting \c fillWidth to \c true on another item.

    A handle can belong to the item either on the left or top side, or on the
    right or bottom side:

    \list
    \li If the fill item is to the right: the handle belongs to the left
        item.
    \li If the fill item is on the left: the handle belongs to the right
        item.
    \endlist

    To create a SplitView with three items, and let the center item get
    superfluous space, one could do the following:

    \code
    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Rectangle {
            implicitWidth: 200
            SplitView.maximumWidth: 400
            color: "lightblue"
            Label {
                text: "View 1"
                anchors.centerIn: parent
            }
        }
        Rectangle {
            id: centerItem
            SplitView.minimumWidth: 50
            SplitView.fillWidth: true
            color: "lightgray"
            Label {
                text: "View 2"
                anchors.centerIn: parent
            }
        }
        Rectangle {
            implicitWidth: 200
            color: "lightgreen"
            Label {
                text: "View 3"
                anchors.centerIn: parent
            }
        }
    }
    \endcode

    \section1 Serializing SplitView's State

    The main purpose of SplitView is to allow users to easily configure the
    size of various UI elements. In addition, the user's preferred sizes should
    be remembered across sessions. To achieve this, the values of the \c
    SplitView.preferredWidth and \c SplitView.preferredHeight properties can be
    serialized using the \l saveState() and \l restoreState() functions:

    \qml \QtMinorVersion
    import QtQuick.Controls 2.\1
    import Qt.labs.settings 1.0

    ApplicationWindow {
        // ...

        Component.onCompleted: splitView.restoreState(settings.splitView)
        Component.onDestruction: settings.splitView = splitView.saveState()

        Settings {
            id: settings
            property var splitView
        }

        SplitView {
            id: splitView
            // ...
        }
    }
    \endqml

    Alternatively, the \l {Settings::}{value()} and \l {Settings::}{setValue()}
    functions of \l Settings can be used:

    \qml \QtMinorVersion
    import QtQuick.Controls 2.\1
    import Qt.labs.settings 1.0

    ApplicationWindow {
        // ...

        Component.onCompleted: splitView.restoreState(settings.value("ui/splitview"))
        Component.onDestruction: settings.setValue("ui/splitview", splitView.saveState())

        Settings {
            id: settings
        }

        SplitView {
            id: splitView
            // ...
        }
    }
    \endqml

    \sa SplitHandle, {Customizing SplitView}, {Container Controls}
*/

Q_LOGGING_CATEGORY(qlcQQuickSplitView, "qt.quick.controls.splitview")
Q_LOGGING_CATEGORY(qlcQQuickSplitViewMouse, "qt.quick.controls.splitview.mouse")
Q_LOGGING_CATEGORY(qlcQQuickSplitViewState, "qt.quick.controls.splitview.state")

void QQuickSplitViewPrivate::updateFillIndex()
{
    const int count = contentModel->count();
    const bool horizontal = isHorizontal();

    qCDebug(qlcQQuickSplitView) << "looking for fillWidth/Height item amongst" << count << "items";

    m_fillIndex = -1;
    int i = 0;
    int lastVisibleIndex = -1;
    for (; i < count; ++i) {
        QQuickItem *item = qobject_cast<QQuickItem*>(contentModel->object(i));
        if (!item->isVisible())
            continue;

        lastVisibleIndex = i;

        const QQuickSplitViewAttached *attached = qobject_cast<QQuickSplitViewAttached*>(
            qmlAttachedPropertiesObject<QQuickSplitView>(item, false));
        if (!attached)
            continue;

        if ((horizontal && attached->fillWidth()) || (!horizontal && attached->fillHeight())) {
            m_fillIndex = i;
            qCDebug(qlcQQuickSplitView) << "found fillWidth/Height item at index" << m_fillIndex;
            break;
        }
    }

    if (m_fillIndex == -1) {
        // If there was no item with fillWidth/fillHeight set, m_fillIndex will be -1,
        // and we'll set it to the last visible item.
        // If there was an item with fillWidth/fillHeight set, we were already done and this will be skipped.
        m_fillIndex = lastVisibleIndex != -1 ? lastVisibleIndex : count - 1;
        qCDebug(qlcQQuickSplitView) << "found no fillWidth/Height item; using last item at index" << m_fillIndex;
    }
}

/*
    Resizes split items according to their preferred size and any constraints.

    If a split item is being resized due to a split handle being dragged,
    it will be resized accordingly.

    Items that aren't visible are skipped.
*/
void QQuickSplitViewPrivate::layoutResizeSplitItems(qreal &usedWidth, qreal &usedHeight, int &indexBeingResizedDueToDrag)
{
    const int count = contentModel->count();
    const bool horizontal = isHorizontal();
    for (int index = 0; index < count; ++index) {
        QQuickItem *item = qobject_cast<QQuickItem*>(contentModel->object(index));
        if (!item->isVisible()) {
            // The item is not visible, so skip it.
            qCDebug(qlcQQuickSplitView).nospace() << "  - " << index << ": split item " << item
                << " at index " << index << " is not visible; skipping it and its handles (if any)";
            continue;
        }

        const QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
        QQuickSplitViewAttached *attached = qobject_cast<QQuickSplitViewAttached*>(
            qmlAttachedPropertiesObject<QQuickSplitView>(item, false));
        const auto sizeData = effectiveSizeData(itemPrivate, attached);

        const bool resizeLeftItem = m_fillIndex > m_pressedHandleIndex;
        // True if any handle is pressed.
        const bool isAHandlePressed = m_pressedHandleIndex != -1;
        // True if this particular item is being resized as a result of a handle being dragged.
        const bool isBeingResized = isAHandlePressed && ((resizeLeftItem && index == m_pressedHandleIndex)
            || (!resizeLeftItem && index == m_nextVisibleIndexAfterPressedHandle));
        if (isBeingResized) {
            indexBeingResizedDueToDrag = index;
            qCDebug(qlcQQuickSplitView).nospace() << "  - " << index << ": dragging handle for item";
        }

        const qreal size = horizontal ? width : height;
        qreal requestedSize = 0;
        if (isBeingResized) {
            // Don't let the mouse go past either edge of the SplitView.
            const qreal clampedMousePos = horizontal
                ? qBound(qreal(0.0), m_mousePos.x(), width)
                : qBound(qreal(0.0), m_mousePos.y(), height);

            // We also need to ensure that the item's edge doesn't go too far
            // out and hence give the item more space than is available.
            const int firstIndex = resizeLeftItem ? m_nextVisibleIndexAfterPressedHandle : 0;
            const int lastIndex = resizeLeftItem ? contentModel->count() - 1 : m_pressedHandleIndex;
            const qreal accumulated = accumulatedSize(firstIndex, lastIndex);

            const qreal mousePosRelativeToLeftHandleEdge = horizontal
                ? m_pressPos.x() - m_handlePosBeforePress.x()
                : m_pressPos.y() - m_handlePosBeforePress.y();

            const QQuickItem *pressedHandleItem = m_handleItems.at(m_pressedHandleIndex);
            const qreal pressedHandleSize = horizontal ? pressedHandleItem->width() : pressedHandleItem->height();

            if (resizeLeftItem) {
                // The handle shouldn't cross other handles, so use the right edge of
                // the first handle to the left as the left edge.
                qreal leftEdge = 0;
                if (m_pressedHandleIndex - 1 >= 0) {
                    const QQuickItem *leftHandle = m_handleItems.at(m_pressedHandleIndex - 1);
                    leftEdge = horizontal
                        ? leftHandle->x() + leftHandle->width()
                        : leftHandle->y() + leftHandle->height();
                }

                // The mouse can be clicked anywhere in the handle, and if we don't account for
                // its position within the handle, the handle will jump when dragged.
                const qreal pressedHandlePos = clampedMousePos - mousePosRelativeToLeftHandleEdge;

                const qreal rightStop = size - accumulated - pressedHandleSize;
                qreal leftStop = qMax(leftEdge, pressedHandlePos);
                // qBound() doesn't care if min is greater than max, but we do.
                if (leftStop > rightStop)
                    leftStop = rightStop;
                const qreal newHandlePos = qBound(leftStop, pressedHandlePos, rightStop);
                const qreal newItemSize = newHandlePos - leftEdge;

                // Modify the preferredWidth, otherwise the original implicitWidth/preferredWidth
                // will be used on the next layout (when it's no longer being resized).
                if (!attached) {
                    // Force the attached object to be created since we rely on it.
                    attached = qobject_cast<QQuickSplitViewAttached*>(
                        qmlAttachedPropertiesObject<QQuickSplitView>(item, true));
                }

                /*
                    Users could conceivably respond to size changes in items by setting attached
                    SplitView properties:

                        onWidthChanged: if (width < 10) secondItem.SplitView.preferredWidth = 100

                    We handle this by doing another layout after the current layout if the
                    attached/implicit size properties are set during this layout. However, we also
                    need to set preferredWidth/Height here (for reasons mentioned in the comment above),
                    but we don't want this to count as a request for a delayed layout, so we guard against it.
                */
                m_ignoreNextLayoutRequest = true;

                if (horizontal)
                    attached->setPreferredWidth(newItemSize);
                else
                    attached->setPreferredHeight(newItemSize);

                // We still need to use requestedWidth in the setWidth() call below,
                // because sizeData has already been calculated and now contains an old
                // effectivePreferredWidth value.
                requestedSize = newItemSize;

                qCDebug(qlcQQuickSplitView).nospace() << "  - " << index << ": resized (dragged) " << item
                    << " (clampedMousePos=" << clampedMousePos
                    << " pressedHandlePos=" << pressedHandlePos
                    << " accumulated=" << accumulated
                    << " leftEdge=" << leftEdge
                    << " leftStop=" << leftStop
                    << " rightStop=" << rightStop
                    << " newHandlePos=" << newHandlePos
                    << " newItemSize=" << newItemSize << ")";
            } else { // Resizing the item on the right.
                // The handle shouldn't cross other handles, so use the left edge of
                // the first handle to the right as the right edge.
                qreal rightEdge = size;
                if (m_nextVisibleIndexAfterPressedHandle < m_handleItems.size()) {
                    const QQuickItem *rightHandle = m_handleItems.at(m_nextVisibleIndexAfterPressedHandle);
                    rightEdge = horizontal ? rightHandle->x() : rightHandle->y();
                }

                // The mouse can be clicked anywhere in the handle, and if we don't account for
                // its position within the handle, the handle will jump when dragged.
                const qreal pressedHandlePos = clampedMousePos - mousePosRelativeToLeftHandleEdge;

                const qreal leftStop = accumulated - pressedHandleSize;
                qreal rightStop = qMin(rightEdge - pressedHandleSize, pressedHandlePos);
                // qBound() doesn't care if min is greater than max, but we do.
                if (rightStop < leftStop)
                    rightStop = leftStop;
                const qreal newHandlePos = qBound(leftStop, pressedHandlePos, rightStop);
                const qreal newItemSize = rightEdge - (newHandlePos + pressedHandleSize);

                // Modify the preferredWidth, otherwise the original implicitWidth/preferredWidth
                // will be used on the next layout (when it's no longer being resized).
                if (!attached) {
                    // Force the attached object to be created since we rely on it.
                    attached = qobject_cast<QQuickSplitViewAttached*>(
                        qmlAttachedPropertiesObject<QQuickSplitView>(item, true));
                }

                m_ignoreNextLayoutRequest = true;

                if (horizontal)
                    attached->setPreferredWidth(newItemSize);
                else
                    attached->setPreferredHeight(newItemSize);

                // We still need to use requestedSize in the setWidth()/setHeight() call below,
                // because sizeData has already been calculated and now contains an old
                // effectivePreferredWidth/Height value.
                requestedSize = newItemSize;

                qCDebug(qlcQQuickSplitView).nospace() << "  - " << index << ": resized (dragged) " << item
                    << " (clampedMousePos=" << clampedMousePos
                    << " pressedHandlePos=" << pressedHandlePos
                    << " accumulated=" << accumulated
                    << " leftEdge=" << rightEdge
                    << " leftStop=" << leftStop
                    << " rightStop=" << rightStop
                    << " newHandlePos=" << newHandlePos
                    << " newItemSize=" << newItemSize << ")";
            }
        } else if (index != m_fillIndex) {
            // No handle is being dragged and we're not the fill item,
            // so set our preferred size as we normally would.
            requestedSize = horizontal
                ? sizeData.effectivePreferredWidth : sizeData.effectivePreferredHeight;
        }

        if (index != m_fillIndex) {
            if (horizontal) {
                item->setWidth(qBound(
                    sizeData.effectiveMinimumWidth,
                    requestedSize,
                    sizeData.effectiveMaximumWidth));
                item->setHeight(height);
            } else {
                item->setWidth(width);
                item->setHeight(qBound(
                    sizeData.effectiveMinimumHeight,
                    requestedSize,
                    sizeData.effectiveMaximumHeight));
            }

            qCDebug(qlcQQuickSplitView).nospace() << "  - " << index << ": resized split item " << item
                << " (effective"
                << " minW=" << sizeData.effectiveMinimumWidth
                << ", minH=" << sizeData.effectiveMinimumHeight
                << ", prfW=" << sizeData.effectivePreferredWidth
                << ", prfH=" << sizeData.effectivePreferredHeight
                << ", maxW=" << sizeData.effectiveMaximumWidth
                << ", maxH=" << sizeData.effectiveMaximumHeight << ")";

            // Keep track of how much space has been used so far.
            if (horizontal)
                usedWidth += item->width();
            else
                usedHeight += item->height();
        } else if (indexBeingResizedDueToDrag != m_fillIndex) {
            // The fill item is resized afterwards, outside of the loop.
            qCDebug(qlcQQuickSplitView).nospace() << "  - " << index << ": skipping fill item as we resize it last";
        }

        // Also account for the size of the handle for this item (if any).
        // We do this for the fill item too, which is why it's outside of the check above.
        if (index < count - 1 && m_handle) {
            QQuickItem *handleItem = m_handleItems.at(index);
            // The handle for an item that's not visible will usually already be skipped
            // with the item visibility check higher up, but if the view looks like this
            // [ visible ] | [ visible (fill) ] | [ hidden ]
            //                                  ^
            //                               hidden
            // and we're iterating over the second item (which is visible but has no handle),
            // we need to add an extra check for it to avoid it still taking up space.
            if (handleItem->isVisible()) {
                if (horizontal) {
                    qCDebug(qlcQQuickSplitView).nospace() << "  - " << index
                        << ": handle takes up " << handleItem->width() << " width";
                    usedWidth += handleItem->width();
                } else {
                    qCDebug(qlcQQuickSplitView).nospace() << "  - " << index
                        << ": handle takes up " << handleItem->height() << " height";
                    usedHeight += handleItem->height();
                }
            } else {
                qCDebug(qlcQQuickSplitView).nospace() << "  - " << index << ": handle is not visible; skipping it";
            }
        }
    }
}

/*
    Resizes the fill item by giving it the remaining space
    after all other items have been resized.

    Items that aren't visible are skipped.
*/
void QQuickSplitViewPrivate::layoutResizeFillItem(QQuickItem *fillItem,
    qreal &usedWidth, qreal &usedHeight, int indexBeingResizedDueToDrag)
{
    // Only bother resizing if it it's visible. Also, if it's being resized due to a drag,
    // then we've already set its size in layoutResizeSplitItems(), so no need to do it here.
    if (!fillItem->isVisible() || indexBeingResizedDueToDrag == m_fillIndex) {
        qCDebug(qlcQQuickSplitView).nospace() << m_fillIndex << ":  - fill item " << fillItem
            << " is not visible or was already resized due to a drag;"
            << " skipping it and its handles (if any)";
        return;
    }

    const QQuickItemPrivate *fillItemPrivate = QQuickItemPrivate::get(fillItem);
    const QQuickSplitViewAttached *attached = qobject_cast<QQuickSplitViewAttached*>(
        qmlAttachedPropertiesObject<QQuickSplitView>(fillItem, false));
    const auto fillSizeData = effectiveSizeData(fillItemPrivate, attached);
    if (isHorizontal()) {
        fillItem->setWidth(qBound(
            fillSizeData.effectiveMinimumWidth,
            width - usedWidth,
            fillSizeData.effectiveMaximumWidth));
        fillItem->setHeight(height);
    } else {
        fillItem->setWidth(width);
        fillItem->setHeight(qBound(
            fillSizeData.effectiveMinimumHeight,
            height - usedHeight,
            fillSizeData.effectiveMaximumHeight));
    }

    qCDebug(qlcQQuickSplitView).nospace() << "  - " << m_fillIndex
        << ": resized split fill item " << fillItem << " (effective"
        << " minW=" << fillSizeData.effectiveMinimumWidth
        << ", minH=" << fillSizeData.effectiveMinimumHeight
        << ", maxW=" << fillSizeData.effectiveMaximumWidth
        << ", maxH=" << fillSizeData.effectiveMaximumHeight << ")";
}

/*
    Positions items by laying them out in a row or column.

    Items that aren't visible are skipped.
*/
void QQuickSplitViewPrivate::layoutPositionItems(const QQuickItem *fillItem)
{
    const bool horizontal = isHorizontal();
    const int count = contentModel->count();
    qreal usedWidth = 0;
    qreal usedHeight = 0;

    for (int i = 0; i < count; ++i) {
        QQuickItem *item = qobject_cast<QQuickItem*>(contentModel->object(i));
        if (!item->isVisible()) {
            qCDebug(qlcQQuickSplitView).nospace() << "  - " << i << ": split item " << item
                << " is not visible; skipping it and its handles (if any)";
            continue;
        }

        // Position the item.
        if (horizontal) {
            item->setX(usedWidth);
            item->setY(0);
        } else {
            item->setX(0);
            item->setY(usedHeight);
        }

        // Keep track of how much space has been used so far.
        if (horizontal)
            usedWidth += item->width();
        else
            usedHeight += item->height();

        if (Q_UNLIKELY(qlcQQuickSplitView().isDebugEnabled())) {
            const QQuickItemPrivate *fillItemPrivate = QQuickItemPrivate::get(fillItem);
            const QQuickSplitViewAttached *attached = qobject_cast<QQuickSplitViewAttached*>(
                qmlAttachedPropertiesObject<QQuickSplitView>(fillItem, false));
            const auto sizeData = effectiveSizeData(fillItemPrivate, attached);
            qCDebug(qlcQQuickSplitView).nospace() << "  - " << i << ": positioned "
                << (i == m_fillIndex ? "fill item " : "item ") << item << " (effective"
                << " minW=" << sizeData.effectiveMinimumWidth
                << ", minH=" << sizeData.effectiveMinimumHeight
                << ", prfW=" << sizeData.effectivePreferredWidth
                << ", prfH=" << sizeData.effectivePreferredHeight
                << ", maxW=" << sizeData.effectiveMaximumWidth
                << ", maxH=" << sizeData.effectiveMaximumHeight << ")";
        }

        // Position  the handle for this item (if any).
        if (i < count - 1 && m_handle) {
            // Position the handle.
            QQuickItem *handleItem = m_handleItems.at(i);
            handleItem->setX(horizontal ? usedWidth : 0);
            handleItem->setY(horizontal ? 0 : usedHeight);

            if (horizontal)
                usedWidth += handleItem->width();
            else
                usedHeight += handleItem->height();

            qCDebug(qlcQQuickSplitView).nospace() << "  - " << i << ": positioned handle " << handleItem;
        }
    }
}

void QQuickSplitViewPrivate::requestLayout()
{
    Q_Q(QQuickSplitView);
    q->polish();
}

void QQuickSplitViewPrivate::layout()
{
    if (!componentComplete)
        return;

    if (m_layingOut)
        return;

    const int count = contentModel->count();
    if (count <= 0)
        return;

    Q_ASSERT_X(m_fillIndex < count, Q_FUNC_INFO, qPrintable(
        QString::fromLatin1("m_fillIndex is %1 but our count is %2").arg(m_fillIndex).arg(count)));

    Q_ASSERT_X(!m_handle || m_handleItems.size() == count - 1, Q_FUNC_INFO, qPrintable(QString::fromLatin1(
        "Expected %1 handle items, but there are %2").arg(count - 1).arg(m_handleItems.size())));

    // We allow mouse events to instantly trigger layouts, whereas with e.g.
    // attached properties being set, we require a delayed layout.
    // To prevent recursive calls during mouse events, we need this guard.
    QBoolBlocker guard(m_layingOut, true);

    const bool horizontal = isHorizontal();
    qCDebug(qlcQQuickSplitView) << "laying out" << count << "split items"
        << (horizontal ? "horizontally" : "vertically") << "in SplitView" << q_func();

    qreal usedWidth = 0;
    qreal usedHeight = 0;
    int indexBeingResizedDueToDrag = -1;

    qCDebug(qlcQQuickSplitView) << "  resizing:";

    // First, resize the items. We need to do this first because otherwise fill
    // items would take up all of the remaining space as soon as they are encountered.
    layoutResizeSplitItems(usedWidth, usedHeight, indexBeingResizedDueToDrag);

    qCDebug(qlcQQuickSplitView).nospace()
        << "  - (remaining width=" << width - usedWidth
        << " remaining height=" << height - usedHeight << ")";

    // Give the fill item the remaining space.
    QQuickItem *fillItem = qobject_cast<QQuickItem*>(contentModel->object(m_fillIndex));
    layoutResizeFillItem(fillItem, usedWidth, usedHeight, indexBeingResizedDueToDrag);

    qCDebug(qlcQQuickSplitView) << "  positioning:";

    // Position the items.
    layoutPositionItems(fillItem);

    qCDebug(qlcQQuickSplitView).nospace() << "finished layouting";
}

void QQuickSplitViewPrivate::createHandles()
{
    Q_ASSERT(m_handle);
    // A handle only makes sense if there are two items on either side.
    if (contentModel->count() <= 1)
        return;

    // Create new handle items if there aren't enough.
    const int count = contentModel->count() - 1;
    qCDebug(qlcQQuickSplitView) << "creating" << count << "handles";
    m_handleItems.reserve(count);
    for (int i = 0; i < count; ++i)
        createHandleItem(i);
}

void QQuickSplitViewPrivate::createHandleItem(int index)
{
    Q_Q(QQuickSplitView);
    if (contentModel->count() <= 1)
        return;

    qCDebug(qlcQQuickSplitView) << "- creating handle for split item at index" << index
        << "from handle component" << m_handle;

    // If we don't use the correct context, it won't be possible to refer to
    // the control's id from within the delegate.
    QQmlContext *creationContext = m_handle->creationContext();
    // The component might not have been created in QML, in which case
    // the creation context will be null and we have to create it ourselves.
    if (!creationContext)
        creationContext = qmlContext(q);
    QQmlContext *context = new QQmlContext(creationContext, q);
    context->setContextObject(q);
    QQuickItem *handleItem = qobject_cast<QQuickItem*>(m_handle->beginCreate(context));
    if (handleItem) {
        qCDebug(qlcQQuickSplitView) << "- successfully created handle item" << handleItem << "for split item at index" << index;

        // Insert the item to our list of items *before* its parent is set to us,
        // so that we can avoid it being added as a content item by checking
        // if it is in the list in isContent().
        m_handleItems.insert(index, handleItem);

        handleItem->setParentItem(q);

        m_handle->completeCreate();
        resizeHandle(handleItem);
    }
}

void QQuickSplitViewPrivate::removeExcessHandles()
{
    int excess = m_handleItems.size() - qMax(0, contentModel->count() - 1);
    qCDebug(qlcQQuickSplitView) << "removing" << excess << "excess handles from the end of our list";
    for (; excess > 0; --excess) {
        QQuickItem *handleItem = m_handleItems.takeLast();
        delete handleItem;
    }
}

qreal QQuickSplitViewPrivate::accumulatedSize(int firstIndex, int lastIndex) const
{
    qreal size = 0.0;
    const bool horizontal = isHorizontal();
    for (int i = firstIndex; i <= lastIndex; ++i) {
        QQuickItem *item = qobject_cast<QQuickItem*>(contentModel->object(i));
        if (item->isVisible()) {
            if (i != m_fillIndex) {
                size += horizontal ? item->width() : item->height();
            } else {
                // If the fill item has a minimum size specified, we must respect it.
                const QQuickSplitViewAttached *attached = qobject_cast<QQuickSplitViewAttached*>(
                    qmlAttachedPropertiesObject<QQuickSplitView>(item, false));
                if (attached) {
                    const QQuickSplitViewAttachedPrivate *attachedPrivate
                        = QQuickSplitViewAttachedPrivate::get(attached);
                    if (horizontal && attachedPrivate->m_isMinimumWidthSet)
                        size += attachedPrivate->m_minimumWidth;
                    else if (!horizontal && attachedPrivate->m_isMinimumHeightSet)
                        size += attachedPrivate->m_minimumHeight;
                }
            }
        }

        // Only add the handle's width if there's actually a handle for this split item index.
        if (i < lastIndex || lastIndex < contentModel->count() - 1) {
            const QQuickItem *handleItem = m_handleItems.at(i);
            if (handleItem->isVisible())
                size += horizontal ? handleItem->width() : handleItem->height();
        }
    }
    return size;
}

qreal effectiveMinimumWidth(const QQuickSplitViewAttachedPrivate *attachedPrivate)
{
    return attachedPrivate && attachedPrivate->m_isMinimumWidthSet ? attachedPrivate->m_minimumWidth : 0;
}

qreal effectiveMinimumHeight(const QQuickSplitViewAttachedPrivate *attachedPrivate)
{
    return attachedPrivate && attachedPrivate->m_isMinimumHeightSet ? attachedPrivate->m_minimumHeight : 0;
}

qreal effectivePreferredWidth(const QQuickSplitViewAttachedPrivate *attachedPrivate,
    const QQuickItemPrivate *itemPrivate)
{
    return attachedPrivate && attachedPrivate->m_isPreferredWidthSet
        ? attachedPrivate->m_preferredWidth : itemPrivate->implicitWidth;
}

qreal effectivePreferredHeight(const QQuickSplitViewAttachedPrivate *attachedPrivate,
    const QQuickItemPrivate *itemPrivate)
{
    return attachedPrivate && attachedPrivate->m_isPreferredHeightSet
        ? attachedPrivate->m_preferredHeight : itemPrivate->implicitHeight;
}

qreal effectiveMaximumWidth(const QQuickSplitViewAttachedPrivate *attachedPrivate)
{
    return attachedPrivate && attachedPrivate->m_isMaximumWidthSet
        ? attachedPrivate->m_maximumWidth : std::numeric_limits<qreal>::infinity();
}

qreal effectiveMaximumHeight(const QQuickSplitViewAttachedPrivate *attachedPrivate)
{
    return attachedPrivate && attachedPrivate->m_isMaximumHeightSet
        ? attachedPrivate->m_maximumHeight : std::numeric_limits<qreal>::infinity();
}

// We don't just take an index, because the item and attached properties object
// will both be used outside of this function by calling code, so save some
// time by not accessing them twice.
QQuickSplitViewPrivate::EffectiveSizeData QQuickSplitViewPrivate::effectiveSizeData(
    const QQuickItemPrivate *itemPrivate, const QQuickSplitViewAttached *attached) const
{
    EffectiveSizeData data;
    const QQuickSplitViewAttachedPrivate *attachedPrivate = attached ? QQuickSplitViewAttachedPrivate::get(attached) : nullptr;
    data.effectiveMinimumWidth = effectiveMinimumWidth(attachedPrivate);
    data.effectiveMinimumHeight = effectiveMinimumHeight(attachedPrivate);
    data.effectivePreferredWidth = effectivePreferredWidth(attachedPrivate, itemPrivate);
    data.effectivePreferredHeight = effectivePreferredHeight(attachedPrivate, itemPrivate);
    data.effectiveMaximumWidth = effectiveMaximumWidth(attachedPrivate);
    data.effectiveMaximumHeight = effectiveMaximumHeight(attachedPrivate);
    return data;
}

int QQuickSplitViewPrivate::handleIndexForSplitIndex(int splitIndex) const
{
    // If it's the first and only item in the view, it doesn't have a handle,
    // so return -1: splitIndex (0) - 1.
    // If it's the last item in the view, it doesn't have a handle, so use
    // the handle for the previous item.
    return splitIndex == contentModel->count() - 1 ? splitIndex - 1 : splitIndex;
}

void QQuickSplitViewPrivate::destroyHandles()
{
    qCDebug(qlcQQuickSplitView) << "destroying" << m_handleItems.size() << "handles";
    qDeleteAll(m_handleItems);
    m_handleItems.clear();
}

void QQuickSplitViewPrivate::resizeHandle(QQuickItem *handleItem)
{
    const bool horizontal = isHorizontal();
    handleItem->setWidth(horizontal ? handleItem->implicitWidth() : width);
    handleItem->setHeight(horizontal ? height : handleItem->implicitHeight());
}

void QQuickSplitViewPrivate::resizeHandles()
{
    for (QQuickItem *handleItem : m_handleItems)
        resizeHandle(handleItem);
}

void QQuickSplitViewPrivate::updateHandleVisibilities()
{
    // If this is the first item that is visible, we won't have any
    // handles yet, because we don't create a handle if we only have one item.
    if (m_handleItems.isEmpty())
        return;

    // If the visibility/children change makes any item the last (right/bottom-most)
    // visible item, we don't want to display a handle for it either:
    // [ visible (fill) ] | [ hidden ] | [ hidden ]
    //                    ^            ^
    //                 hidden        hidden
    const int count = contentModel->count();
    int lastVisibleItemIndex = -1;
    for (int i = count - 1; i >= 0; --i) {
        const QQuickItem *item = qobject_cast<QQuickItem*>(contentModel->object(i));
        if (item->isVisible()) {
            lastVisibleItemIndex = i;
            break;
        }
    }

    for (int i = 0; i < count - 1; ++i) {
        const QQuickItem *item = qobject_cast<QQuickItem*>(contentModel->object(i));
        QQuickItem *handleItem = m_handleItems.at(i);
        if (i != lastVisibleItemIndex)
            handleItem->setVisible(item->isVisible());
        else
            handleItem->setVisible(false);
        qCDebug(qlcQQuickSplitView) << "set visible property of handle" << handleItem << "at index"
            << i << "to" << handleItem->isVisible();
    }
}

void QQuickSplitViewPrivate::updateHoveredHandle(QQuickItem *hoveredItem)
{
    Q_Q(QQuickSplitView);
    qCDebug(qlcQQuickSplitViewMouse) << "updating hovered handle after" << hoveredItem << "was hovered";

    const int oldHoveredHandleIndex = m_hoveredHandleIndex;
    m_hoveredHandleIndex = m_handleItems.indexOf(hoveredItem);
    if (m_hoveredHandleIndex == oldHoveredHandleIndex)
        return;

    // First, clear the hovered flag of any previously-hovered handle.
    if (oldHoveredHandleIndex != -1) {
        QQuickItem *oldHoveredHandle = m_handleItems.at(oldHoveredHandleIndex);
        QQuickSplitHandleAttached *oldHoveredHandleAttached = qobject_cast<QQuickSplitHandleAttached*>(
            qmlAttachedPropertiesObject<QQuickSplitHandleAttached>(oldHoveredHandle, true));
        QQuickSplitHandleAttachedPrivate::get(oldHoveredHandleAttached)->setHovered(false);
        qCDebug(qlcQQuickSplitViewMouse) << "handle item at index" << oldHoveredHandleIndex << "is no longer hovered";
    }

    if (m_hoveredHandleIndex != -1) {
        QQuickSplitHandleAttached *handleAttached = qobject_cast<QQuickSplitHandleAttached*>(
            qmlAttachedPropertiesObject<QQuickSplitHandleAttached>(hoveredItem, true));
        QQuickSplitHandleAttachedPrivate::get(handleAttached)->setHovered(true);
        qCDebug(qlcQQuickSplitViewMouse) << "handle item at index" << m_hoveredHandleIndex << "is now hovered";
    } else {
        qCDebug(qlcQQuickSplitViewMouse) << "either there is no hovered item or" << hoveredItem << "is not a handle";
    }

#if QT_CONFIG(cursor)
    if (m_hoveredHandleIndex != -1)
        q->setCursor(m_orientation == Qt::Horizontal ? Qt::SplitHCursor : Qt::SplitVCursor);
    else
        q->setCursor(Qt::ArrowCursor);
#endif
}

void QQuickSplitViewPrivate::setResizing(bool resizing)
{
    Q_Q(QQuickSplitView);
    if (resizing == m_resizing)
        return;

    m_resizing = resizing;
    emit q->resizingChanged();
}

bool QQuickSplitViewPrivate::isHorizontal() const
{
    return m_orientation == Qt::Horizontal;
}

QQuickItem *QQuickSplitViewPrivate::getContentItem()
{
    Q_Q(QQuickSplitView);
    if (QQuickItem *item = QQuickContainerPrivate::getContentItem())
        return item;

    return new QQuickContentItem(q);
}

void QQuickSplitViewPrivate::handlePress(const QPointF &point)
{
    Q_Q(QQuickSplitView);
    QQuickContainerPrivate::handlePress(point);

    QQuickItem *pressedItem = q->childAt(point.x(), point.y());
    const int pressedHandleIndex = m_handleItems.indexOf(pressedItem);
    if (pressedHandleIndex != -1) {
        m_pressedHandleIndex = pressedHandleIndex;
        m_pressPos = point;
        m_mousePos = point;

        const QQuickItem *leftOrTopItem = qobject_cast<QQuickItem*>(contentModel->object(m_pressedHandleIndex));
        // Find the first item to the right/bottom of this one that is visible.
        QQuickItem *rightOrBottomItem = nullptr;
        m_nextVisibleIndexAfterPressedHandle = -1;
        for (int i = m_pressedHandleIndex + 1; i < contentModel->count(); ++i) {
            auto nextItem = qobject_cast<QQuickItem*>(contentModel->object(i));
            if (nextItem->isVisible()) {
                rightOrBottomItem = nextItem;
                m_nextVisibleIndexAfterPressedHandle = i;
                break;
            }
        }
        Q_ASSERT_X(rightOrBottomItem, Q_FUNC_INFO, qPrintable(QString::fromLatin1(
            "Failed to find a visible item to the right/bottom of the one that was pressed at index %1; this shouldn't happen")
                .arg(m_pressedHandleIndex)));

        const bool isHorizontal = m_orientation == Qt::Horizontal;
        m_leftOrTopItemSizeBeforePress = isHorizontal ? leftOrTopItem->width() : leftOrTopItem->height();
        m_rightOrBottomItemSizeBeforePress = isHorizontal ? rightOrBottomItem->width() : rightOrBottomItem->height();
        m_handlePosBeforePress = pressedItem->position();

        // Avoid e.g. Flickable stealing our drag if we're inside it.
        q->setKeepMouseGrab(true);

        // Force the attached object to be created since we rely on it.
        QQuickSplitHandleAttached *handleAttached = qobject_cast<QQuickSplitHandleAttached*>(
            qmlAttachedPropertiesObject<QQuickSplitHandleAttached>(pressedItem, true));
        QQuickSplitHandleAttachedPrivate::get(handleAttached)->setPressed(true);

        setResizing(true);

        qCDebug(qlcQQuickSplitViewMouse).nospace() << "handled press -"
            << " left/top index=" << m_pressedHandleIndex << ","
            << " size before press=" << m_leftOrTopItemSizeBeforePress << ","
            << " item=" << leftOrTopItem
            << " right/bottom index=" << m_nextVisibleIndexAfterPressedHandle << ","
            << " size before press=" << m_rightOrBottomItemSizeBeforePress
            << " item=" << rightOrBottomItem;
    }
}

void QQuickSplitViewPrivate::handleMove(const QPointF &point)
{
    QQuickContainerPrivate::handleMove(point);

    if (m_pressedHandleIndex != -1) {
        m_mousePos = point;
        // Don't request layouts for input events because we want
        // resizing to be as responsive and smooth as possible.
        updatePolish();
    }
}

void QQuickSplitViewPrivate::handleRelease(const QPointF &point)
{
    Q_Q(QQuickSplitView);
    QQuickContainerPrivate::handleRelease(point);

    if (m_pressedHandleIndex != -1) {
        QQuickItem *pressedHandle = m_handleItems.at(m_pressedHandleIndex);
        QQuickSplitHandleAttached *handleAttached = qobject_cast<QQuickSplitHandleAttached*>(
            qmlAttachedPropertiesObject<QQuickSplitHandleAttached>(pressedHandle, true));
        QQuickSplitHandleAttachedPrivate::get(handleAttached)->setPressed(false);
    }

    setResizing(false);

    m_pressedHandleIndex = -1;
    m_pressPos = QPointF();
    m_mousePos = QPointF();
    m_handlePosBeforePress = QPointF();
    m_leftOrTopItemSizeBeforePress = 0.0;
    m_rightOrBottomItemSizeBeforePress = 0.0;
    q->setKeepMouseGrab(false);
}

void QQuickSplitViewPrivate::itemVisibilityChanged(QQuickItem *item)
{
    const int itemIndex = contentModel->indexOf(item, nullptr);
    Q_ASSERT(itemIndex != -1);

    qCDebug(qlcQQuickSplitView) << "visible property of split item"
        << item << "at index" << itemIndex << "changed to" << item->isVisible();

    // The visibility of an item just changed, so we need to update the visibility
    // of the corresponding handle (if one exists).

    const int handleIndex = handleIndexForSplitIndex(itemIndex);
    if (handleIndex != -1) {
        QQuickItem *handleItem = m_handleItems.at(handleIndex);
        handleItem->setVisible(item->isVisible());

        qCDebug(qlcQQuickSplitView) << "set visible property of handle item"
            << handleItem << "at index" << handleIndex << "to" << item->isVisible();
    }

    updateHandleVisibilities();
    updateFillIndex();
    requestLayout();
}

void QQuickSplitViewPrivate::itemImplicitWidthChanged(QQuickItem *)
{
    requestLayout();
}

void QQuickSplitViewPrivate::itemImplicitHeightChanged(QQuickItem *)
{
    requestLayout();
}

void QQuickSplitViewPrivate::updatePolish()
{
    layout();
}

QQuickSplitViewPrivate *QQuickSplitViewPrivate::get(QQuickSplitView *splitView)
{
    return splitView->d_func();
}

QQuickSplitView::QQuickSplitView(QQuickItem *parent)
    : QQuickContainer(*(new QQuickSplitViewPrivate), parent)
{
    Q_D(QQuickSplitView);
    d->changeTypes |= QQuickItemPrivate::Visibility;

    setAcceptedMouseButtons(Qt::LeftButton);
    setFiltersChildMouseEvents(true);
}

QQuickSplitView::QQuickSplitView(QQuickSplitViewPrivate &dd, QQuickItem *parent)
    : QQuickContainer(dd, parent)
{
    Q_D(QQuickSplitView);
    d->changeTypes |= QQuickItemPrivate::Visibility;

    setAcceptedMouseButtons(Qt::LeftButton);
    setFiltersChildMouseEvents(true);
}

QQuickSplitView::~QQuickSplitView()
{
    Q_D(QQuickSplitView);
    for (int i = 0; i < d->contentModel->count(); ++i) {
        QQuickItem *item = qobject_cast<QQuickItem*>(d->contentModel->object(i));
        d->removeImplicitSizeListener(item);
    }
}

/*!
    \qmlproperty enumeration QtQuick.Controls::SplitView::orientation

    This property holds the orientation of the SplitView.

    The orientation determines how the split items are laid out:

    Possible values:
    \value Qt.Horizontal The items are laid out horizontally (default).
    \value Qt.Vertical The items are laid out vertically.
*/
Qt::Orientation QQuickSplitView::orientation() const
{
    Q_D(const QQuickSplitView);
    return d->m_orientation;
}

void QQuickSplitView::setOrientation(Qt::Orientation orientation)
{
    Q_D(QQuickSplitView);
    if (orientation == d->m_orientation)
        return;

    d->m_orientation = orientation;
    d->resizeHandles();
    d->requestLayout();
    emit orientationChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::SplitView::resizing
    \readonly

    This property is \c true when the user is resizing
    split items by dragging on the splitter handles.
*/
bool QQuickSplitView::isResizing() const
{
    Q_D(const QQuickSplitView);
    return d->m_resizing;
}

/*!
    \qmlproperty Component QtQuick.Controls::SplitView::handle

    This property holds the handle component.

    An instance of this component will be instantiated \c {count - 1}
    times, as long as \c count is greater than than \c {1}.

    The following table explains how each handle will be resized
    depending on the orientation of the split view:

    \table
        \header
            \li Orientation
            \li Handle Width
            \li Handle Height
        \row
            \li \c Qt.Horizontal
            \li \c implicitWidth
            \li The \c height of the SplitView.
        \row
            \li \c Qt.Vertical
            \li The \c width of the SplitView.
            \li \c implicitHeight
    \endtable

    \sa {Customizing SplitView}
*/
QQmlComponent *QQuickSplitView::handle()
{
    Q_D(const QQuickSplitView);
    return d->m_handle;
}

void QQuickSplitView::setHandle(QQmlComponent *handle)
{
    Q_D(QQuickSplitView);
    if (handle == d->m_handle)
        return;

    qCDebug(qlcQQuickSplitView) << "setting handle" << handle;

    if (d->m_handle)
        d->destroyHandles();

    d->m_handle = handle;

    if (d->m_handle) {
        d->createHandles();
        d->updateHandleVisibilities();
    }

    d->requestLayout();

    emit handleChanged();
}

bool QQuickSplitView::isContent(QQuickItem *item) const
{
    Q_D(const QQuickSplitView);
    if (!qmlContext(item))
        return false;

    if (QQuickItemPrivate::get(item)->isTransparentForPositioner())
        return false;

    return !d->m_handleItems.contains(item);
}

QQuickSplitViewAttached *QQuickSplitView::qmlAttachedProperties(QObject *object)
{
    return new QQuickSplitViewAttached(object);
}

/*!
    \qmlmethod var QtQuick.Controls::SplitView::saveState()

    Saves the preferred sizes of split items into a byte array and returns it.

    \sa {Serializing SplitView's State}, restoreState()
*/
QVariant QQuickSplitView::saveState()
{
    Q_D(QQuickSplitView);
    qCDebug(qlcQQuickSplitViewState) << "saving state for split items in" << this;

    // Save the preferred sizes of each split item.
    QCborArray cborArray;
    for (int i = 0; i < d->contentModel->count(); ++i) {
        const QQuickItem *item = qobject_cast<QQuickItem*>(d->contentModel->object(i));
        const QQuickSplitViewAttached *attached = qobject_cast<QQuickSplitViewAttached*>(
            qmlAttachedPropertiesObject<QQuickSplitView>(item, false));
        // Don't serialise stuff if we don't need to. If a split item was given a preferred
        // size in QML or it was dragged, it will have an attached object and either
        // m_isPreferredWidthSet or m_isPreferredHeightSet (or both) will be true,
        // so items without these can be skipped. We write the index of each item
        // that has data so that we know which item to set it on when restoring.
        if (!attached)
            continue;

        const QQuickSplitViewAttachedPrivate *attachedPrivate = QQuickSplitViewAttachedPrivate::get(attached);
        if (!attachedPrivate->m_isPreferredWidthSet && !attachedPrivate->m_isPreferredHeightSet)
            continue;

        QCborMap cborMap;
        cborMap[QLatin1String("index")] = i;
        if (attachedPrivate->m_isPreferredWidthSet) {
            cborMap[QLatin1String("preferredWidth")] = static_cast<double>(attachedPrivate->m_preferredWidth);

            qCDebug(qlcQQuickSplitViewState).nospace() << "- wrote preferredWidth of "
                << attachedPrivate->m_preferredWidth << " for split item " << item << " at index " << i;
        }
        if (attachedPrivate->m_isPreferredHeightSet) {
            cborMap[QLatin1String("preferredHeight")] = static_cast<double>(attachedPrivate->m_preferredHeight);

            qCDebug(qlcQQuickSplitViewState).nospace() << "- wrote preferredHeight of "
                << attachedPrivate->m_preferredHeight << " for split item " << item << " at index " << i;
        }

        cborArray.append(cborMap);
    }

    const QByteArray byteArray = cborArray.toCborValue().toCbor();
    qCDebug(qlcQQuickSplitViewState) << "the resulting byte array is:" << byteArray;
    return QVariant(byteArray);
}

/*!
    \qmlmethod bool QtQuick.Controls::SplitView::restoreState(state)

    Reads the preferred sizes from \a state and applies them to the split items.

    Returns \c true if the state was successfully restored, otherwise \c false.

    \sa {Serializing SplitView's State}, saveState()
*/
bool QQuickSplitView::restoreState(const QVariant &state)
{
    const QByteArray cborByteArray = state.toByteArray();
    Q_D(QQuickSplitView);
    if (cborByteArray.isEmpty())
        return false;

    QCborParserError parserError;
    const QCborValue cborValue(QCborValue::fromCbor(cborByteArray, &parserError));
    if (parserError.error != QCborError::NoError) {
        qmlWarning(this) << "Error reading SplitView state:" << parserError.errorString();
        return false;
    }

    qCDebug(qlcQQuickSplitViewState) << "restoring state for split items of" << this
        << "from the following string:" << state;

    const QCborArray cborArray(cborValue.toArray());
    const int ourCount = d->contentModel->count();
    // This could conceivably happen if items were removed from the SplitView since the state was last saved.
    if (cborArray.size() > ourCount) {
        qmlWarning(this) << "Error reading SplitView state: expected "
            << ourCount << " or less split items but got " << cborArray.size();
        return false;
    }

    for (auto it = cborArray.constBegin(); it != cborArray.constEnd(); ++it) {
        QCborMap cborMap(it->toMap());
        const int splitItemIndex = cborMap.value(QLatin1String("index")).toInteger();
        const bool isPreferredWidthSet = cborMap.contains(QLatin1String("preferredWidth"));
        const bool isPreferredHeightSet = cborMap.contains(QLatin1String("preferredHeight"));

        QQuickItem *item = qobject_cast<QQuickItem*>(d->contentModel->object(splitItemIndex));
        // If the split item does not have a preferred size specified in QML, it could still have
        // been resized via dragging before it was saved. In this case, it won't have an
        // attached object upon application startup, so we create it.
        QQuickSplitViewAttached *attached = qobject_cast<QQuickSplitViewAttached*>(
            qmlAttachedPropertiesObject<QQuickSplitView>(item, true));
        if (isPreferredWidthSet) {
            const qreal preferredWidth = cborMap.value(QLatin1String("preferredWidth")).toDouble();
            attached->setPreferredWidth(preferredWidth);
        }
        if (isPreferredHeightSet) {
            const qreal preferredHeight = cborMap.value(QLatin1String("preferredHeight")).toDouble();
            attached->setPreferredHeight(preferredHeight);
        }

        const QQuickSplitViewAttachedPrivate *attachedPrivate = QQuickSplitViewAttachedPrivate::get(attached);
        qCDebug(qlcQQuickSplitViewState).nospace()
            << "- restored the following state for split item " << item << " at index " << splitItemIndex
            << ": preferredWidthSet=" << attachedPrivate->m_isPreferredWidthSet
            << " preferredWidth=" << attachedPrivate->m_preferredWidth
            << " preferredHeightSet=" << attachedPrivate->m_isPreferredHeightSet
            << " preferredHeight=" << attachedPrivate->m_preferredHeight;
    }

    return true;
}

void QQuickSplitView::componentComplete()
{
    Q_D(QQuickSplitView);
    QQuickControl::componentComplete();
    d->resizeHandles();
    d->updateFillIndex();
    d->updatePolish();
}

void QQuickSplitView::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(QQuickSplitView);
    QQuickContainer::hoverMoveEvent(event);

    QQuickItem *hoveredItem = childAt(event->pos().x(), event->pos().y());
    d->updateHoveredHandle(hoveredItem);
}

bool QQuickSplitView::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    Q_D(QQuickSplitView);
    qCDebug(qlcQQuickSplitViewMouse) << "childMouseEventFilter called with" << item << event;
    if (event->type() != QEvent::HoverEnter)
        return false;

    // If a child item received a hover enter event, then it means our handle is no longer hovered.
    // Handles should be purely visual and not accept hover events,
    // so we should never get hover events for them here.
    d->updateHoveredHandle(nullptr);
    return false;
}

void QQuickSplitView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickSplitView);
    QQuickControl::geometryChanged(newGeometry, oldGeometry);
    d->resizeHandles();
    d->requestLayout();
}

void QQuickSplitView::itemAdded(int index, QQuickItem *item)
{
    Q_D(QQuickSplitView);
    if (QQuickItemPrivate::get(item)->isTransparentForPositioner())
        return;

    const int count = d->contentModel->count();
    qCDebug(qlcQQuickSplitView).nospace() << "split item " << item << " added at index " << index
        << "; there are now " << count << " items";

    QQuickSplitViewAttached *attached = qobject_cast<QQuickSplitViewAttached*>(
        qmlAttachedPropertiesObject<QQuickSplitView>(item, false));
    if (attached)
        QQuickSplitViewAttachedPrivate::get(attached)->setView(this);

    // Only need to add handles if we have more than one split item.
    if (count > 1) {
        // If the item was added at the end, it shouldn't get a handle;
        // the handle always goes to the split item on the left.
        d->createHandleItem(index < count - 1 ? index : index - 1);
    }

    d->addImplicitSizeListener(item);

    d->updateHandleVisibilities();
    d->updateFillIndex();
    d->requestLayout();
}

void QQuickSplitView::itemMoved(int index, QQuickItem *item)
{
    Q_D(QQuickSplitView);
    if (QQuickItemPrivate::get(item)->isTransparentForPositioner())
        return;

    qCDebug(qlcQQuickSplitView) << "split item" << item << "moved to index" << index;

    d->updateHandleVisibilities();
    d->updateFillIndex();
    d->requestLayout();
}

void QQuickSplitView::itemRemoved(int index, QQuickItem *item)
{
    Q_D(QQuickSplitView);
    if (QQuickItemPrivate::get(item)->isTransparentForPositioner())
        return;

    qCDebug(qlcQQuickSplitView).nospace() << "split item " << item << " removed from index " << index
        << "; there are now " << d->contentModel->count() << " items";

    // Clear hovered/pressed handle if there are any.
    if (d->m_hoveredHandleIndex != -1 || d->m_pressedHandleIndex != -1) {
        const int handleIndex = d->m_hoveredHandleIndex != -1 ? d->m_hoveredHandleIndex : d->m_pressedHandleIndex;
        QQuickItem *itemHandle = d->m_handleItems.at(handleIndex);
        QQuickSplitHandleAttached *handleAttached = qobject_cast<QQuickSplitHandleAttached*>(
            qmlAttachedPropertiesObject<QQuickSplitHandleAttached>(itemHandle, false));
        if (handleAttached) {
            auto handleAttachedPrivate = QQuickSplitHandleAttachedPrivate::get(handleAttached);
            handleAttachedPrivate->setHovered(false);
            handleAttachedPrivate->setPressed(false);
        }

        setKeepMouseGrab(false);
        d->m_hoveredHandleIndex = -1;
        d->m_pressedHandleIndex = -1;
    }

    // Unset any attached properties since the item is no longer owned by us.
    QQuickSplitViewAttached *attached = qobject_cast<QQuickSplitViewAttached*>(
        qmlAttachedPropertiesObject<QQuickSplitView>(item, false));
    if (attached)
        QQuickSplitViewAttachedPrivate::get(attached)->setView(this);

    d->removeImplicitSizeListener(item);

    d->removeExcessHandles();
    d->updateHandleVisibilities();
    d->updateFillIndex();
    d->requestLayout();
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickSplitView::accessibleRole() const
{
    return QAccessible::Pane;
}
#endif

QQuickSplitViewAttached::QQuickSplitViewAttached(QObject *parent)
    : QObject(*(new QQuickSplitViewAttachedPrivate), parent)
{
    Q_D(QQuickSplitViewAttached);
    QQuickItem *item = qobject_cast<QQuickItem *>(parent);
    if (!item) {
        qmlWarning(parent) << "SplitView: attached properties can only be used on Items";
        return;
    }

    if (QQuickItemPrivate::get(item)->isTransparentForPositioner())
        return;

    d->m_splitItem = item;

    // Child items get added to SplitView's contentItem, so we have to ensure
    // that exists first before trying to set m_splitView.
    // Apparently, in some cases it's normal for the parent item
    // to not exist until shortly after this constructor has run.
    if (!item->parentItem())
        return;

    // This will get hit when attached SplitView properties are imperatively set
    // on an item that previously had none set, for example.
    QQuickSplitView *splitView = qobject_cast<QQuickSplitView*>(item->parentItem()->parentItem());
    if (!splitView) {
        qmlWarning(parent) << "SplitView: attached properties must be accessed through a direct child of SplitView";
        return;
    }

    d->setView(splitView);
}

/*!
    \qmlattachedproperty SplitView QtQuick.Controls::SplitView::view

    This attached property holds the split view of the item it is
    attached to, or \c null if the item is not in a split view.
*/
QQuickSplitView *QQuickSplitViewAttached::view() const
{
    Q_D(const QQuickSplitViewAttached);
    return d->m_splitView;
}

/*!
    \qmlattachedproperty real QtQuick.Controls::SplitView::minimumWidth

    This attached property controls the minimum width of the split item.
    The \l preferredWidth is bound within the \l minimumWidth and
    \l maximumWidth. A split item cannot be dragged to be smaller than
    its \c minimumWidth.

    The default value is \c 0. To reset this property to its default value,
    set it to \c undefined.

    \sa maximumWidth, preferredWidth, fillWidth, minimumHeight
*/
qreal QQuickSplitViewAttached::minimumWidth() const
{
    Q_D(const QQuickSplitViewAttached);
    return d->m_minimumWidth;
}

void QQuickSplitViewAttached::setMinimumWidth(qreal width)
{
    Q_D(QQuickSplitViewAttached);
    d->m_isMinimumWidthSet = true;
    if (qFuzzyCompare(width, d->m_minimumWidth))
        return;

    d->m_minimumWidth = width;
    d->requestLayoutView();
    emit minimumWidthChanged();
}

void QQuickSplitViewAttached::resetMinimumWidth()
{
    Q_D(QQuickSplitViewAttached);
    const qreal oldEffectiveMinimumWidth = effectiveMinimumWidth(d);

    d->m_isMinimumWidthSet = false;
    d->m_minimumWidth = -1;

    const qreal newEffectiveMinimumWidth = effectiveMinimumWidth(d);
    if (qFuzzyCompare(newEffectiveMinimumWidth, oldEffectiveMinimumWidth))
        return;

    d->requestLayoutView();
    emit minimumWidthChanged();
}

/*!
    \qmlattachedproperty real QtQuick.Controls::SplitView::minimumHeight

    This attached property controls the minimum height of the split item.
    The \l preferredHeight is bound within the \l minimumHeight and
    \l maximumHeight. A split item cannot be dragged to be smaller than
    its \c minimumHeight.

    The default value is \c 0. To reset this property to its default value,
    set it to \c undefined.

    \sa maximumHeight, preferredHeight, fillHeight, minimumWidth
*/
qreal QQuickSplitViewAttached::minimumHeight() const
{
    Q_D(const QQuickSplitViewAttached);
    return d->m_minimumHeight;
}

void QQuickSplitViewAttached::setMinimumHeight(qreal height)
{
    Q_D(QQuickSplitViewAttached);
    d->m_isMinimumHeightSet = true;
    if (qFuzzyCompare(height, d->m_minimumHeight))
        return;

    d->m_minimumHeight = height;
    d->requestLayoutView();
    emit minimumHeightChanged();
}

void QQuickSplitViewAttached::resetMinimumHeight()
{
    Q_D(QQuickSplitViewAttached);
    const qreal oldEffectiveMinimumHeight = effectiveMinimumHeight(d);

    d->m_isMinimumHeightSet = false;
    d->m_minimumHeight = -1;

    const qreal newEffectiveMinimumHeight = effectiveMinimumHeight(d);
    if (qFuzzyCompare(newEffectiveMinimumHeight, oldEffectiveMinimumHeight))
        return;

    d->requestLayoutView();
    emit minimumHeightChanged();
}

/*!
    \qmlattachedproperty real QtQuick.Controls::SplitView::preferredWidth

    This attached property controls the preferred width of the split item. The
    preferred width will be used as the size of the item, and will be bound
    within the \l minimumWidth and \l maximumWidth. If the preferred width
    is not set, the item's \l {Item::}{implicitWidth} will be used.

    When a split item is resized, the preferredWidth will be set in order
    to keep track of the new size.

    By default, this property is not set, and therefore
    \l {Item::}{implicitWidth} will be used instead. To reset this property to
    its default value, set it to \c undefined.

    \note Do not set the \l{Item::}{width} property of a split item, as it will be
    overwritten upon each layout of the SplitView.

    \sa minimumWidth, maximumWidth, fillWidth, preferredHeight
*/
qreal QQuickSplitViewAttached::preferredWidth() const
{
    Q_D(const QQuickSplitViewAttached);
    return d->m_preferredWidth;
}

void QQuickSplitViewAttached::setPreferredWidth(qreal width)
{
    Q_D(QQuickSplitViewAttached);
    d->m_isPreferredWidthSet = true;
    // Make sure that we clear this flag now, before we emit the change signals
    // which could cause another setter to be called.
    auto splitViewPrivate = d->m_splitView ? QQuickSplitViewPrivate::get(d->m_splitView) : nullptr;
    const bool ignoreNextLayoutRequest = splitViewPrivate && splitViewPrivate->m_ignoreNextLayoutRequest;
    if (splitViewPrivate)
        splitViewPrivate->m_ignoreNextLayoutRequest = false;

    if (qFuzzyCompare(width, d->m_preferredWidth))
        return;

    d->m_preferredWidth = width;

    if (!ignoreNextLayoutRequest) {
        // We are currently in the middle of performing a layout, and the user (not our internal code)
        // changed the preferred width of one of the split items, so request another layout.
        d->requestLayoutView();
    }

    emit preferredWidthChanged();
}

void QQuickSplitViewAttached::resetPreferredWidth()
{
    Q_D(QQuickSplitViewAttached);
    const qreal oldEffectivePreferredWidth = effectivePreferredWidth(
        d, QQuickItemPrivate::get(d->m_splitItem));

    d->m_isPreferredWidthSet = false;
    d->m_preferredWidth = -1;

    const qreal newEffectivePreferredWidth = effectivePreferredWidth(
        d, QQuickItemPrivate::get(d->m_splitItem));
    if (qFuzzyCompare(newEffectivePreferredWidth, oldEffectivePreferredWidth))
        return;

    d->requestLayoutView();
    emit preferredWidthChanged();
}

/*!
    \qmlattachedproperty real QtQuick.Controls::SplitView::preferredHeight

    This attached property controls the preferred height of the split item. The
    preferred height will be used as the size of the item, and will be bound
    within the \l minimumHeight and \l maximumHeight. If the preferred height
    is not set, the item's \l{Item::}{implicitHeight} will be used.

    When a split item is resized, the preferredHeight will be set in order
    to keep track of the new size.

    By default, this property is not set, and therefore
    \l{Item::}{implicitHeight} will be used instead. To reset this property to
    its default value, set it to \c undefined.

    \note Do not set the \l{Item:}{height} property of a split item, as it will be
    overwritten upon each layout of the SplitView.

    \sa minimumHeight, maximumHeight, fillHeight, preferredWidth
*/
qreal QQuickSplitViewAttached::preferredHeight() const
{
    Q_D(const QQuickSplitViewAttached);
    return d->m_preferredHeight;
}

void QQuickSplitViewAttached::setPreferredHeight(qreal height)
{
    Q_D(QQuickSplitViewAttached);
    d->m_isPreferredHeightSet = true;
    // Make sure that we clear this flag now, before we emit the change signals
    // which could cause another setter to be called.
    auto splitViewPrivate = d->m_splitView ? QQuickSplitViewPrivate::get(d->m_splitView) : nullptr;
    const bool ignoreNextLayoutRequest = splitViewPrivate && splitViewPrivate->m_ignoreNextLayoutRequest;
    if (splitViewPrivate)
        splitViewPrivate->m_ignoreNextLayoutRequest = false;

    if (qFuzzyCompare(height, d->m_preferredHeight))
        return;

    d->m_preferredHeight = height;

    if (!ignoreNextLayoutRequest) {
        // We are currently in the middle of performing a layout, and the user (not our internal code)
        // changed the preferred height of one of the split items, so request another layout.
        d->requestLayoutView();
    }

    emit preferredHeightChanged();
}

void QQuickSplitViewAttached::resetPreferredHeight()
{
    Q_D(QQuickSplitViewAttached);
    const qreal oldEffectivePreferredHeight = effectivePreferredHeight(
        d, QQuickItemPrivate::get(d->m_splitItem));

    d->m_isPreferredHeightSet = false;
    d->m_preferredHeight = -1;

    const qreal newEffectivePreferredHeight = effectivePreferredHeight(
        d, QQuickItemPrivate::get(d->m_splitItem));
    if (qFuzzyCompare(newEffectivePreferredHeight, oldEffectivePreferredHeight))
        return;

    d->requestLayoutView();
    emit preferredHeightChanged();
}

/*!
    \qmlattachedproperty real QtQuick.Controls::SplitView::maximumWidth

    This attached property controls the maximum width of the split item.
    The \l preferredWidth is bound within the \l minimumWidth and
    \l maximumWidth. A split item cannot be dragged to be larger than
    its \c maximumWidth.

    The default value is \c Infinity. To reset this property to its default
    value, set it to \c undefined.

    \sa minimumWidth, preferredWidth, fillWidth, maximumHeight
*/
qreal QQuickSplitViewAttached::maximumWidth() const
{
    Q_D(const QQuickSplitViewAttached);
    return d->m_maximumWidth;
}

void QQuickSplitViewAttached::setMaximumWidth(qreal width)
{
    Q_D(QQuickSplitViewAttached);
    d->m_isMaximumWidthSet = true;
    if (qFuzzyCompare(width, d->m_maximumWidth))
        return;

    d->m_maximumWidth = width;
    d->requestLayoutView();
    emit maximumWidthChanged();
}

void QQuickSplitViewAttached::resetMaximumWidth()
{
    Q_D(QQuickSplitViewAttached);
    const qreal oldEffectiveMaximumWidth = effectiveMaximumWidth(d);

    d->m_isMaximumWidthSet = false;
    d->m_maximumWidth = -1;

    const qreal newEffectiveMaximumWidth = effectiveMaximumWidth(d);
    if (qFuzzyCompare(newEffectiveMaximumWidth, oldEffectiveMaximumWidth))
        return;

    d->requestLayoutView();
    emit maximumWidthChanged();
}

/*!
    \qmlattachedproperty real QtQuick.Controls::SplitView::maximumHeight

    This attached property controls the maximum height of the split item.
    The \l preferredHeight is bound within the \l minimumHeight and
    \l maximumHeight. A split item cannot be dragged to be larger than
    its \c maximumHeight.

    The default value is \c Infinity. To reset this property to its default
    value, set it to \c undefined.

    \sa minimumHeight, preferredHeight, fillHeight, maximumWidth
*/
qreal QQuickSplitViewAttached::maximumHeight() const
{
    Q_D(const QQuickSplitViewAttached);
    return d->m_maximumHeight;
}

void QQuickSplitViewAttached::setMaximumHeight(qreal height)
{
    Q_D(QQuickSplitViewAttached);
    d->m_isMaximumHeightSet = true;
    if (qFuzzyCompare(height, d->m_maximumHeight))
        return;

    d->m_maximumHeight = height;
    d->requestLayoutView();
    emit maximumHeightChanged();
}

void QQuickSplitViewAttached::resetMaximumHeight()
{
    Q_D(QQuickSplitViewAttached);
    const qreal oldEffectiveMaximumHeight = effectiveMaximumHeight(d);

    d->m_isMaximumHeightSet = false;
    d->m_maximumHeight = -1;

    const qreal newEffectiveMaximumHeight = effectiveMaximumHeight(d);
    if (qFuzzyCompare(newEffectiveMaximumHeight, oldEffectiveMaximumHeight))
        return;

    d->requestLayoutView();
    emit maximumHeightChanged();
}

/*!
    \qmlattachedproperty bool QtQuick.Controls::SplitView::fillWidth

    This attached property controls whether the item takes the remaining space
    in the split view after all other items have been laid out.

    By default, the last visible child of the split view will have this set,
    but it can be changed by explicitly setting \c fillWidth to \c true on
    another item.

    The width of a split item with \c fillWidth set to \c true is still
    restricted within its \l minimumWidth and \l maximumWidth.

    \sa minimumWidth, preferredWidth, maximumWidth, fillHeight
*/
bool QQuickSplitViewAttached::fillWidth() const
{
    Q_D(const QQuickSplitViewAttached);
    return d->m_fillWidth;
}

void QQuickSplitViewAttached::setFillWidth(bool fill)
{
    Q_D(QQuickSplitViewAttached);
    d->m_isFillWidthSet = true;
    if (fill == d->m_fillWidth)
        return;

    d->m_fillWidth = fill;
    if (d->m_splitView && d->m_splitView->orientation() == Qt::Horizontal)
        QQuickSplitViewPrivate::get(d->m_splitView)->updateFillIndex();
    d->requestLayoutView();
    emit fillWidthChanged();
}

/*!
    \qmlattachedproperty bool QtQuick.Controls::SplitView::fillHeight

    This attached property controls whether the item takes the remaining space
    in the split view after all other items have been laid out.

    By default, the last visible child of the split view will have this set,
    but it can be changed by explicitly setting \c fillHeight to \c true on
    another item.

    The height of a split item with \c fillHeight set to \c true is still
    restricted within its \l minimumHeight and \l maximumHeight.

    \sa minimumHeight, preferredHeight, maximumHeight, fillWidth
*/
bool QQuickSplitViewAttached::fillHeight() const
{
    Q_D(const QQuickSplitViewAttached);
    return d->m_fillHeight;
}

void QQuickSplitViewAttached::setFillHeight(bool fill)
{
    Q_D(QQuickSplitViewAttached);
    d->m_isFillHeightSet = true;
    if (fill == d->m_fillHeight)
        return;

    d->m_fillHeight = fill;
    if (d->m_splitView && d->m_splitView->orientation() == Qt::Vertical)
        QQuickSplitViewPrivate::get(d->m_splitView)->updateFillIndex();
    d->requestLayoutView();
    emit fillHeightChanged();
}

QQuickSplitViewAttachedPrivate::QQuickSplitViewAttachedPrivate()
    : m_fillWidth(false)
    , m_fillHeight(false)
    , m_isFillWidthSet(false)
    , m_isFillHeightSet(false)
    , m_isMinimumWidthSet(false)
    , m_isMinimumHeightSet(false)
    , m_isPreferredWidthSet(false)
    , m_isPreferredHeightSet(false)
    , m_isMaximumWidthSet(false)
    , m_isMaximumHeightSet(false)
    , m_minimumWidth(0)
    , m_minimumHeight(0)
    , m_preferredWidth(-1)
    , m_preferredHeight(-1)
    , m_maximumWidth(std::numeric_limits<qreal>::infinity())
    , m_maximumHeight(std::numeric_limits<qreal>::infinity())
{
}

void QQuickSplitViewAttachedPrivate::setView(QQuickSplitView *newView)
{
    Q_Q(QQuickSplitViewAttached);
    if (newView == m_splitView)
        return;

    m_splitView = newView;
    qCDebug(qlcQQuickSplitView) << "set SplitView" << newView << "on attached object" << this;
    emit q->viewChanged();
}

void QQuickSplitViewAttachedPrivate::requestLayoutView()
{
    if (m_splitView)
        QQuickSplitViewPrivate::get(m_splitView)->requestLayout();
}

QQuickSplitViewAttachedPrivate *QQuickSplitViewAttachedPrivate::get(QQuickSplitViewAttached *attached)
{
    return attached->d_func();
}

const QQuickSplitViewAttachedPrivate *QQuickSplitViewAttachedPrivate::get(const QQuickSplitViewAttached *attached)
{
    return attached->d_func();
}

QQuickSplitHandleAttachedPrivate::QQuickSplitHandleAttachedPrivate()
    : m_hovered(false)
    , m_pressed(false)
{
}

void QQuickSplitHandleAttachedPrivate::setHovered(bool hovered)
{
    Q_Q(QQuickSplitHandleAttached);
    if (hovered == m_hovered)
        return;

    m_hovered = hovered;
    emit q->hoveredChanged();
}

void QQuickSplitHandleAttachedPrivate::setPressed(bool pressed)
{
    Q_Q(QQuickSplitHandleAttached);
    if (pressed == m_pressed)
        return;

    m_pressed = pressed;
    emit q->pressedChanged();
}

QQuickSplitHandleAttachedPrivate *QQuickSplitHandleAttachedPrivate::get(QQuickSplitHandleAttached *attached)
{
    return attached->d_func();
}

const QQuickSplitHandleAttachedPrivate *QQuickSplitHandleAttachedPrivate::get(const QQuickSplitHandleAttached *attached)
{
    return attached->d_func();
}

QQuickSplitHandleAttached::QQuickSplitHandleAttached(QObject *parent)
    : QObject(*(new QQuickSplitViewAttachedPrivate), parent)
{
}

/*!
    \qmltype SplitHandle
    \inherits QtObject
//!     \instantiates QQuickSplitHandleAttached
    \inqmlmodule QtQuick.Controls
    \since 5.13
    \brief Provides attached properties for SplitView handles.

    SplitHandle provides attached properties for \l SplitView handles.

    For split items themselves, use the attached \l SplitView properties.

    \sa SplitView
*/

/*!
    \qmlattachedproperty bool QtQuick.Controls::SplitHandle::hovered

    This attached property holds whether the split handle is hovered.

    \sa pressed
*/
bool QQuickSplitHandleAttached::isHovered() const
{
    Q_D(const QQuickSplitHandleAttached);
    return d->m_hovered;
}

/*!
    \qmlattachedproperty bool QtQuick.Controls::SplitHandle::pressed

    This attached property holds whether the split handle is pressed.

    \sa hovered
*/
bool QQuickSplitHandleAttached::isPressed() const
{
    Q_D(const QQuickSplitHandleAttached);
    return d->m_pressed;
}

QQuickSplitHandleAttached *QQuickSplitHandleAttached::qmlAttachedProperties(QObject *object)
{
    return new QQuickSplitHandleAttached(object);
}

QT_END_NAMESPACE

#include "moc_qquicksplitview_p.cpp"
