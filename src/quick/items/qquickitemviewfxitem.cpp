// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickitemviewfxitem_p_p.h"
#include "qquickitem_p.h"
#include "qquickitemview_p_p.h"

QT_BEGIN_NAMESPACE

QQuickItemViewFxItem::QQuickItemViewFxItem(QQuickItem *item, bool ownItem, QQuickItemChangeListener* changeListener)
    : item(item)
    , changeListener(changeListener)
    , transitionableItem(nullptr)
    , ownItem(ownItem)
    , releaseAfterTransition(false)
    , trackGeom(false)
{
}

QQuickItemViewFxItem::~QQuickItemViewFxItem()
{
    delete transitionableItem;
    transitionableItem = nullptr;

    if (ownItem && item) {
        trackGeometry(false);
        item->setParentItem(0);
        item->deleteLater();
    }
}

qreal QQuickItemViewFxItem::itemX() const
{
    return transitionableItem ? transitionableItem->itemX() : (item ? item->x() : 0);
}

qreal QQuickItemViewFxItem::itemY() const
{
    return transitionableItem ? transitionableItem->itemY() : (item ? item->y() : 0);
}

void QQuickItemViewFxItem::moveTo(const QPointF &pos, bool immediate)
{
    if (transitionableItem)
        transitionableItem->moveTo(pos, immediate);
    else if (item)
        item->setPosition(pos);
}

void QQuickItemViewFxItem::setVisible(bool visible)
{
    if (!visible && transitionableItem && transitionableItem->transitionScheduledOrRunning())
        return;
    if (item)
        QQuickItemPrivate::get(item)->setCulled(!visible);
}

void QQuickItemViewFxItem::trackGeometry(bool track)
{
    if (track) {
        if (!trackGeom) {
            if (item) {
                QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
                itemPrivate->addItemChangeListener(changeListener, QQuickItemPrivate::Geometry);
            }
            trackGeom = true;
        }
    } else {
        if (trackGeom) {
            if (item) {
                QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
                itemPrivate->removeItemChangeListener(changeListener, QQuickItemPrivate::Geometry);
            }
            trackGeom = false;
        }
    }
}

QRectF QQuickItemViewFxItem::geometry() const
{
    return QRectF(item->position(), item->size());
}

void QQuickItemViewFxItem::setGeometry(const QRectF &geometry)
{
    item->setPosition(geometry.topLeft());
    item->setSize(geometry.size());
}

QQuickItemViewTransitioner::TransitionType QQuickItemViewFxItem::scheduledTransitionType() const
{
    return transitionableItem ? transitionableItem->nextTransitionType : QQuickItemViewTransitioner::NoTransition;
}

bool QQuickItemViewFxItem::transitionScheduledOrRunning() const
{
    return transitionableItem ? transitionableItem->transitionScheduledOrRunning() : false;
}

bool QQuickItemViewFxItem::transitionRunning() const
{
    return transitionableItem ? transitionableItem->transitionRunning() : false;
}

bool QQuickItemViewFxItem::isPendingRemoval() const
{
    return transitionableItem ? transitionableItem->isPendingRemoval() : false;
}

void QQuickItemViewFxItem::transitionNextReposition(QQuickItemViewTransitioner *transitioner, QQuickItemViewTransitioner::TransitionType type, bool asTarget)
{
    if (!transitioner)
        return;
    if (!transitionableItem)
        transitionableItem = new QQuickItemViewTransitionableItem(item);
    transitioner->transitionNextReposition(transitionableItem, type, asTarget);
}

bool QQuickItemViewFxItem::prepareTransition(QQuickItemViewTransitioner *transitioner, const QRectF &viewBounds)
{
    return transitionableItem ? transitionableItem->prepareTransition(transitioner, index, viewBounds) : false;
}

void QQuickItemViewFxItem::startTransition(QQuickItemViewTransitioner *transitioner)
{
    if (transitionableItem)
        transitionableItem->startTransition(transitioner, index);
}

QT_END_NAMESPACE

