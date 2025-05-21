// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickoverlay_p.h"
#include "qquickpopuppositioner_p_p.h"
#include "qquickpopupanchors_p.h"
#include "qquickpopupitem_p_p.h"
#include "qquickpopupwindow_p_p.h"
#include "qquickpopup_p_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcPopupPositioner, "qt.quick.controls.popuppositioner")

static const QQuickItemPrivate::ChangeTypes AncestorChangeTypes = QQuickItemPrivate::Geometry
                                                                  | QQuickItemPrivate::Parent
                                                                  | QQuickItemPrivate::Children;

static const QQuickItemPrivate::ChangeTypes ItemChangeTypes = QQuickItemPrivate::Geometry
                                                             | QQuickItemPrivate::Parent;

QQuickPopupPositioner::QQuickPopupPositioner(QQuickPopup *popup)
    : m_popup(popup)
{
}

QQuickPopupPositioner::~QQuickPopupPositioner()
{
    if (m_parentItem) {
        QQuickItemPrivate::get(m_parentItem)->removeItemChangeListener(this, ItemChangeTypes);
        removeAncestorListeners(m_parentItem->parentItem());
    }
}

QQuickPopup *QQuickPopupPositioner::popup() const
{
    return m_popup;
}

QQuickItem *QQuickPopupPositioner::parentItem() const
{
    return m_parentItem;
}

void QQuickPopupPositioner::setParentItem(QQuickItem *parent)
{
    if (m_parentItem == parent)
        return;

    if (m_parentItem) {
        QQuickItemPrivate::get(m_parentItem)->removeItemChangeListener(this, ItemChangeTypes);
        removeAncestorListeners(m_parentItem->parentItem());
    }

    m_parentItem = parent;

    if (!parent)
        return;

    QQuickItemPrivate::get(parent)->addItemChangeListener(this, ItemChangeTypes);
    addAncestorListeners(parent->parentItem());
    // Store the scale property so the end result of any transition that could effect the scale
    // does not influence the top left of the final popup, so it doesn't appear to flip from one
    // position to another as a result
    m_popupScale = m_popup->popupItem()->scale();
    if (m_popup->popupItem()->isVisible())
        QQuickPopupPrivate::get(m_popup)->reposition();
}

void QQuickPopupPositioner::reposition()
{
    auto p = QQuickPopupPrivate::get(popup());
    QQuickPopupItem *popupItem = static_cast<QQuickPopupItem *>(m_popup->popupItem());

    if (p->usePopupWindow()) {
        repositionPopupWindow();
        return;
    }

    if (!popupItem->isVisible())
        return;

    if (m_positioning) {
        popupItem->polish();
        return;
    }

    qCDebug(lcPopupPositioner) << "reposition called for" << m_popup;

    const qreal w = popupItem->width() * m_popupScale;
    const qreal h = popupItem->height() * m_popupScale;
    const qreal iw = popupItem->implicitWidth() * m_popupScale;
    const qreal ih = popupItem->implicitHeight() * m_popupScale;

    bool widthAdjusted = false;
    bool heightAdjusted = false;

    const QQuickItem *centerInParent = p->anchors ? p->getAnchors()->centerIn() : nullptr;
    const QQuickOverlay *centerInOverlay = qobject_cast<const QQuickOverlay*>(centerInParent);
    QRectF rect(!centerInParent ? p->allowHorizontalMove ? p->x : popupItem->x() : 0,
                !centerInParent ? p->allowVerticalMove ? p->y : popupItem->y() : 0,
                !p->hasWidth && iw > 0 ? iw : w, !p->hasHeight && ih > 0 ? ih : h);
    bool relaxEdgeConstraint = p->relaxEdgeConstraint;
    if (m_parentItem) {
        // m_parentItem is the parent that the popup should open in,
        // and popupItem()->parentItem() is the overlay, so the mapToItem() calls below
        // effectively map the rect to scene coordinates.

        // Animations can cause reposition() to get called when m_parentItem no longer has a window.
        if (!m_parentItem->window())
            return;

        if (centerInParent) {
            if (centerInParent != parentItem() && !centerInOverlay) {
                qmlWarning(m_popup) << "Popup can only be centered within its immediate parent or Overlay.overlay";
                return;
            }

            if (centerInOverlay) {
                rect.moveCenter(QPointF(qRound(centerInOverlay->width() / 2.0), qRound(centerInOverlay->height() / 2.0)));
                // Popup cannot be moved outside window bounds when its centered with overlay
                relaxEdgeConstraint = false;
            } else {
                const QPointF parentItemCenter = QPointF(qRound(m_parentItem->width() / 2), qRound(m_parentItem->height() / 2));
                rect.moveCenter(m_parentItem->mapToItem(popupItem->parentItem(), parentItemCenter));
            }
        } else {
            rect.moveTopLeft(m_parentItem->mapToItem(popupItem->parentItem(), rect.topLeft()));
        }

        // The overlay is assumed to fully cover the window's contents, although the overlay's geometry
        // might not always equal the window's geometry (for example, if the window's contents are rotated).
        QQuickOverlay *overlay = QQuickOverlay::overlay(p->window);
        if (overlay) {
            qreal boundsWidth = overlay->width();
            qreal boundsHeight = overlay->height();

            // QTBUG-126843: On some platforms, the overlay's geometry is not yet available at the instant
            // when Component.completed() is emitted. Fall back to the window's geometry for this edge case.
            if (Q_UNLIKELY(boundsWidth <= 0)) {
                boundsWidth = p->window->width();
                boundsHeight = p->window->height();
            }

            const QMarginsF margins = p->getMargins();
            QRectF bounds(qMax<qreal>(0.0, margins.left()),
                          qMax<qreal>(0.0, margins.top()),
                          boundsWidth - qMax<qreal>(0.0, margins.left()) - qMax<qreal>(0.0, margins.right()),
                          boundsHeight - qMax<qreal>(0.0, margins.top()) - qMax<qreal>(0.0, margins.bottom()));

            // if the popup doesn't fit horizontally inside the window, try flipping it around (left <-> right)
            if (p->allowHorizontalFlip && (rect.left() < bounds.left() || rect.right() > bounds.right())) {
                const QPointF newTopLeft(m_parentItem->width() - p->x - rect.width(), p->y);
                const QRectF flipped(m_parentItem->mapToItem(popupItem->parentItem(), newTopLeft),
                                     rect.size());
                if (flipped.intersected(bounds).width() > rect.intersected(bounds).width())
                    rect.moveLeft(flipped.left());
            }

            // if the popup doesn't fit vertically inside the window, try flipping it around (above <-> below)
            if (p->allowVerticalFlip && (rect.top() < bounds.top() || rect.bottom() > bounds.bottom())) {
                const QPointF newTopLeft(p->x, m_parentItem->height() - p->y - rect.height());
                const QRectF flipped(m_parentItem->mapToItem(popupItem->parentItem(), newTopLeft),
                                     rect.size());
                if (flipped.intersected(bounds).height() > rect.intersected(bounds).height())
                    rect.moveTop(flipped.top());
            }

            // push inside the margins if specified
            if (p->allowVerticalMove) {
                if (margins.top() >= 0 && rect.top() < bounds.top())
                    rect.moveTop(margins.top());
                if (margins.bottom() >= 0 && rect.bottom() > bounds.bottom())
                    rect.moveBottom(bounds.bottom());
            }
            if (p->allowHorizontalMove) {
                if (margins.left() >= 0 && rect.left() < bounds.left())
                    rect.moveLeft(margins.left());
                if (margins.right() >= 0 && rect.right() > bounds.right())
                    rect.moveRight(bounds.right());
            }

            if (iw > 0 && (rect.left() < bounds.left() || rect.right() > bounds.right())) {
                // neither the flipped or pushed geometry fits inside the window, choose
                // whichever side (left vs. right) fits larger part of the popup
                if (p->allowHorizontalMove && p->allowHorizontalFlip) {
                    if (rect.left() < bounds.left() && bounds.left() + rect.width() <= bounds.right())
                        rect.moveLeft(bounds.left());
                    else if (rect.right() > bounds.right() && bounds.right() - rect.width() >= bounds.left())
                        rect.moveRight(bounds.right());
                }

                // as a last resort, adjust the width to fit the window
                // Negative margins don't require resize as popup not pushed within
                // the boundary. But otherwise, retain existing behavior of resizing
                // for items, such as menus, which enables flip.
                if (p->allowHorizontalResize) {
                    if ((margins.left() >= 0 || !relaxEdgeConstraint)
                            && (rect.left() < bounds.left())) {
                        rect.setLeft(bounds.left());
                        widthAdjusted = true;
                    }
                    if ((margins.right() >= 0 || !relaxEdgeConstraint)
                            && (rect.right() > bounds.right())) {
                        rect.setRight(bounds.right());
                        widthAdjusted = true;
                    }
                }
            } else if (iw > 0 && rect.left() >= bounds.left() && rect.right() <= bounds.right()
                       && iw != w) {
                // restore original width
                rect.setWidth(iw);
                widthAdjusted = true;
            }

            if (ih > 0 && (rect.top() < bounds.top() || rect.bottom() > bounds.bottom())) {
                // neither the flipped or pushed geometry fits inside the window, choose
                // whichever side (above vs. below) fits larger part of the popup
                if (p->allowVerticalMove && p->allowVerticalFlip) {
                    if (rect.top() < bounds.top() && bounds.top() + rect.height() <= bounds.bottom())
                        rect.moveTop(bounds.top());
                    else if (rect.bottom() > bounds.bottom() && bounds.bottom() - rect.height() >= bounds.top())
                        rect.moveBottom(bounds.bottom());
                }

                // as a last resort, adjust the height to fit the window
                // Negative margins don't require resize as popup not pushed within
                // the boundary. But otherwise, retain existing behavior of resizing
                // for items, such as menus, which enables flip.
                if (p->allowVerticalResize) {
                    if ((margins.top() >= 0 || !relaxEdgeConstraint)
                            && (rect.top() < bounds.top())) {
                        rect.setTop(bounds.top());
                        heightAdjusted = true;
                    }
                    if ((margins.bottom() >= 0 || !relaxEdgeConstraint)
                            && (rect.bottom() > bounds.bottom())) {
                        rect.setBottom(bounds.bottom());
                        heightAdjusted = true;
                    }
                }
            } else if (ih > 0 && rect.top() >= bounds.top() && rect.bottom() <= bounds.bottom()
                       && ih != h) {
                // restore original height
                rect.setHeight(ih);
                heightAdjusted = true;
            }
        }
    }

    m_positioning = true;

    const QPointF windowPos = rect.topLeft();
    popupItem->setPosition(windowPos);

    // If the popup was assigned a parent, rect will be in scene coordinates,
    // so we need to map its top left back to item coordinates.
    // However, if centering within the overlay, the coordinates will be relative
    // to the window, so we don't need to do anything.
    // The same applies to popups that are in their own dedicated window.
    if (m_parentItem && !centerInOverlay)
        p->setEffectivePosFromWindowPos(m_parentItem->mapFromScene(windowPos));
    else
        p->setEffectivePosFromWindowPos(windowPos);

    if (!p->hasWidth && widthAdjusted && rect.width() > 0) {
        popupItem->setWidth(rect.width() / m_popupScale);
        // The popup doesn't have an explicit width, so we should respect that by not
        // making our call above an explicit assignment. If we don't, the popup won't
        // resize after being repositioned in some cases.
        QQuickItemPrivate::get(popupItem)->widthValidFlag = false;
    }
    if (!p->hasHeight && heightAdjusted && rect.height() > 0) {
        popupItem->setHeight(rect.height() / m_popupScale);
        QQuickItemPrivate::get(popupItem)->heightValidFlag = false;
    }
    m_positioning = false;

    qCDebug(lcPopupPositioner) << "- new popupItem geometry:"
        << popupItem->x() << popupItem->y() << popupItem->width() << popupItem->height();
}

void QQuickPopupPositioner::repositionPopupWindow()
{
    auto *p = QQuickPopupPrivate::get(popup());
    QQuickPopupItem *popupItem = static_cast<QQuickPopupItem *>(m_popup->popupItem());

    QPointF requestedPos(p->x, p->y);
    // Shift the window position a bit back, so that the top-left of the
    // background frame ends up at the requested position.
    QPointF windowPos = requestedPos - p->windowInsetsTopLeft();

    if (!p->popupWindow || !p->parentItem) {
        // If we don't have a popupWindow, set a temporary effective pos. Otherwise
        // wait for a callback to QQuickPopupWindow::handlePopupPositionChangeFromWindowSystem()
        // from setting p->popupWindow->setPosition() below.
        p->setEffectivePosFromWindowPos(windowPos);
        return;
    }

    const QQuickItem *centerInParent = p->anchors ? p->getAnchors()->centerIn() : nullptr;
    const QQuickOverlay *centerInOverlay = qobject_cast<const QQuickOverlay *>(centerInParent);
    bool skipFittingStep = false;

    if (centerInOverlay) {
        windowPos = QPoint(qRound((centerInOverlay->width() - p->popupItem->width()) / 2.0),
                           qRound((centerInOverlay->height() - p->popupItem->height()) / 2.0));
        skipFittingStep = true;
    } else if (centerInParent == p->parentItem) {
        windowPos = QPoint(qRound((p->parentItem->width() - p->popupItem->width()) / 2.0),
                           qRound((p->parentItem->height() - p->popupItem->height()) / 2.0));
        skipFittingStep = true;
    } else if (centerInParent)
        qmlWarning(popup()) << "Popup can only be centered within its immediate parent or Overlay.overlay";

    const QPointF globalCoords = centerInOverlay ? centerInOverlay->mapToGlobal(windowPos.x(), windowPos.y())
                                                 : p->parentItem->mapToGlobal(windowPos.x(), windowPos.y());
    QRectF rect = { globalCoords.x(), globalCoords.y(), popupItem->width(), popupItem->height() };

    // QTBUG-99618: On wayland, we can't use QWindow::mapToGlobal(), and should use a xdg_positioner instead.
    static bool isWayland = QGuiApplication::platformName().startsWith(QLatin1String("wayland"));
    if (!skipFittingStep && !isWayland) {
        const QScreen *screenAtPopupPosition = QGuiApplication::screenAt(globalCoords.toPoint());
        const QScreen *screen = screenAtPopupPosition ? screenAtPopupPosition : QGuiApplication::primaryScreen();
        const QRectF bounds = screen->availableGeometry().toRectF();

        // When flipping menus, we need to take both the overlap and padding into account.
        const qreal overlap = popup()->property("overlap").toReal();
        qreal padding = 0;
        qreal scale = 1.0;
        if (const QQuickPopup *parentPopup = qobject_cast<QQuickPopup *>(popup()->parent())) {
            padding = parentPopup->leftPadding();
            scale = parentPopup->scale();
        }

        if (p->allowHorizontalFlip && (rect.left() < bounds.left() || rect.right() > bounds.right()))
            rect.moveLeft(rect.left() - requestedPos.x() - rect.width() + overlap * scale - padding);

        if (p->allowVerticalFlip && (rect.top() < bounds.top() || rect.bottom() > bounds.bottom()))
            rect.moveTop(rect.top() - requestedPos.y() - rect.height() + overlap * scale);

        if (rect.left() < bounds.left() || rect.right() > bounds.right()) {
            if (p->allowHorizontalMove) {
                if (rect.left() < bounds.left() && bounds.left() + rect.width() <= bounds.right())
                    rect.moveLeft(bounds.left());
                else if (rect.right() > bounds.right() && bounds.right() - rect.width() >= bounds.left())
                    rect.moveRight(bounds.right());
            }
        }
        if (rect.top() < bounds.top() || rect.bottom() > bounds.bottom()) {
            if (p->allowVerticalMove) {
                if (rect.top() < bounds.top() && bounds.top() + rect.height() <= bounds.bottom())
                    rect.moveTop(bounds.top());
                else if (rect.bottom() > bounds.bottom() && bounds.bottom() - rect.height() >= bounds.top())
                    rect.moveBottom(bounds.bottom());
            }
        }
    }

    p->popupWindow->setPosition(rect.x(), rect.y());
    p->popupItem->setPosition(p->windowInsetsTopLeft());
}

void QQuickPopupPositioner::itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF &)
{
    auto *popupPrivate = QQuickPopupPrivate::get(m_popup);
    if (m_parentItem && m_popup->popupItem()->isVisible() && popupPrivate->resolvedPopupType() == QQuickPopup::PopupType::Item)
        popupPrivate->reposition();
}

void QQuickPopupPositioner::itemParentChanged(QQuickItem *, QQuickItem *parent)
{
    addAncestorListeners(parent);
}

void QQuickPopupPositioner::itemChildRemoved(QQuickItem *item, QQuickItem *child)
{
    if (child == m_parentItem || child->isAncestorOf(m_parentItem))
        removeAncestorListeners(item);
}

void QQuickPopupPositioner::removeAncestorListeners(QQuickItem *item)
{
    if (item == m_parentItem)
        return;

    QQuickItem *p = item;
    while (p) {
        QQuickItemPrivate::get(p)->removeItemChangeListener(this, AncestorChangeTypes);
        p = p->parentItem();
    }
}

void QQuickPopupPositioner::addAncestorListeners(QQuickItem *item)
{
    if (item == m_parentItem)
        return;

    QQuickItem *p = item;
    while (p) {
        QQuickItemPrivate::get(p)->updateOrAddItemChangeListener(this, AncestorChangeTypes);
        p = p->parentItem();
    }
}

QT_END_NAMESPACE
