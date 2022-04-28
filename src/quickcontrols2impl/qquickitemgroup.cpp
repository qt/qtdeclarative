/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qquickitemgroup_p.h"

#include <QtQuick/private/qquickimplicitsizeitem_p_p.h>

QT_BEGIN_NAMESPACE

QQuickItemGroup::QQuickItemGroup(QQuickItem *parent)
    : QQuickImplicitSizeItem(*(new QQuickImplicitSizeItemPrivate), parent)
{
}

QQuickItemGroup::~QQuickItemGroup()
{
    const auto children = childItems();
    for (QQuickItem *child : children)
        unwatch(child);
}

void QQuickItemGroup::watch(QQuickItem *item)
{
    QQuickItemPrivate::get(item)->addItemChangeListener(this, QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
}

void QQuickItemGroup::unwatch(QQuickItem *item)
{
    QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
}

QSizeF QQuickItemGroup::calculateImplicitSize() const
{
    qreal width = 0;
    qreal height = 0;
    const auto children = childItems();
    for (QQuickItem *child : children) {
        width = qMax(width, child->implicitWidth());
        height = qMax(height, child->implicitHeight());
    }
    return QSizeF(width, height);
}

void QQuickItemGroup::updateImplicitSize()
{
    QSizeF size = calculateImplicitSize();
    setImplicitSize(size.width(), size.height());
}

void QQuickItemGroup::itemChange(ItemChange change, const ItemChangeData &data)
{
    QQuickImplicitSizeItem::itemChange(change, data);
    switch (change) {
    case ItemChildAddedChange:
        watch(data.item);
        data.item->setSize(QSizeF(width(), height()));
        updateImplicitSize();
        break;
    case ItemChildRemovedChange:
        unwatch(data.item);
        updateImplicitSize();
        break;
    default:
        break;
    }
}

void QQuickItemGroup::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickImplicitSizeItem::geometryChange(newGeometry, oldGeometry);

    if (newGeometry.size() != oldGeometry.size()) {
        const auto children = childItems();
        for (QQuickItem *child : children)
            child->setSize(newGeometry.size());
    }
}

void QQuickItemGroup::itemImplicitWidthChanged(QQuickItem *)
{
    setImplicitWidth(calculateImplicitSize().width());
}

void QQuickItemGroup::itemImplicitHeightChanged(QQuickItem *)
{
    setImplicitHeight(calculateImplicitSize().height());
}

QT_END_NAMESPACE

#include "moc_qquickitemgroup_p.cpp"
