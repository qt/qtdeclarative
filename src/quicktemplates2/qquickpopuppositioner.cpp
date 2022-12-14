// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickoverlay_p.h"
#include "qquickpopuppositioner_p_p.h"
#include "qquickpopupanchors_p.h"
#include "qquickpopupitem_p_p.h"
#include "qquickpopup_p_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcPopupPositioner, "qt.quick.controls.popuppositioner")

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
    QQuickItem *popupItem = m_popup->popupItem();
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
    QQuickPopupPrivate *p = QQuickPopupPrivate::get(m_popup);

    const QQuickItem *centerInParent = p->anchors ? p->getAnchors()->centerIn() : nullptr;
    const QQuickOverlay *centerInOverlay = qobject_cast<const QQuickOverlay*>(centerInParent);
    QRectF rect(!centerInParent ? p->allowHorizontalMove ? p->x : popupItem->x() : 0,
                !centerInParent ? p->allowVerticalMove ? p->y : popupItem->y() : 0,
                !p->hasWidth && iw > 0 ? iw : w,
                !p->hasHeight && ih > 0 ? ih : h);
    if (m_parentItem) {
        // m_parentItem is the parent that the popup should open in,
        // and popupItem()->parentItem() is the overlay, so the mapToItem() calls below
        // effectively map the rect to scene coordinates.
        if (centerInParent) {
            if (centerInParent != parentItem() && !centerInOverlay) {
                qmlWarning(m_popup) << "Popup can only be centered within its immediate parent or Overlay.overlay";
                return;
            }

            if (centerInOverlay) {
                rect.moveCenter(QPointF(qRound(centerInOverlay->width() / 2.0), qRound(centerInOverlay->height() / 2.0)));
            } else {
                const QPointF parentItemCenter = QPointF(qRound(m_parentItem->width() / 2), qRound(m_parentItem->height() / 2));
                rect.moveCenter(m_parentItem->mapToItem(popupItem->parentItem(), parentItemCenter));
            }
        } else {
            rect.moveTopLeft(m_parentItem->mapToItem(popupItem->parentItem(), rect.topLeft()));
        }

        if (p->window) {
            const QMarginsF margins = p->getMargins();
            QRectF bounds(qMax<qreal>(0.0, margins.left()),
                          qMax<qreal>(0.0, margins.top()),
                          p->window->width() - qMax<qreal>(0.0, margins.left()) - qMax<qreal>(0.0, margins.right()),
                          p->window->height() - qMax<qreal>(0.0, margins.top()) - qMax<qreal>(0.0, margins.bottom()));
            if (p->window->contentOrientation() == Qt::LandscapeOrientation || p->window->contentOrientation() == Qt::InvertedLandscapeOrientation)
                bounds = bounds.transposed();

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
                if (p->allowHorizontalResize) {
                    if (rect.left() < bounds.left()) {
                        rect.setLeft(bounds.left());
                        widthAdjusted = true;
                    }
                    if (rect.right() > bounds.right()) {
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
                if (p->allowVerticalResize) {
                    if (rect.top() < bounds.top()) {
                        rect.setTop(bounds.top());
                        heightAdjusted = true;
                    }
                    if (rect.bottom() > bounds.bottom()) {
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

    popupItem->setPosition(rect.topLeft());

    // If the popup was assigned a parent, rect will be in scene coordinates,
    // so we need to map its top left back to item coordinates.
    // However, if centering within the overlay, the coordinates will be relative
    // to the window, so we don't need to do anything.
    const QPointF effectivePos = m_parentItem && !centerInOverlay ? m_parentItem->mapFromScene(rect.topLeft()) : rect.topLeft();
    if (!qFuzzyCompare(p->effectiveX, effectivePos.x())) {
        p->effectiveX = effectivePos.x();
        emit m_popup->xChanged();
    }
    if (!qFuzzyCompare(p->effectiveY, effectivePos.y())) {
        p->effectiveY = effectivePos.y();
        emit m_popup->yChanged();
    }

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

void QQuickPopupPositioner::itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF &)
{
    if (m_parentItem && m_popup->popupItem()->isVisible())
        QQuickPopupPrivate::get(m_popup)->reposition();
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
