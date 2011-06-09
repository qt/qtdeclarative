/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "sgselectiontool.h"

#include "sgviewinspector.h"

#include <QtGui/QMouseEvent>
#include <QtDeclarative/QSGView>
#include <QtDeclarative/QSGItem>
#include <QtDeclarative/QSGPaintedItem>
#include <QtDeclarative/private/qsgitem_p.h>

namespace QmlJSDebugger {

/*
 * Returns the first visible item at the given position, or 0 when no such
 * child exists.
 */
static QSGItem *itemAt(QSGItem *item, const QPointF &pos, QSGItem *overlay)
{
    if (item == overlay)
        return 0;

    if (!item->isVisible() || item->opacity() == 0.0)
        return 0;

    if (item->flags() & QSGItem::ItemClipsChildrenToShape) {
        if (!QRectF(0, 0, item->width(), item->height()).contains(pos))
            return 0;
    }

    QList<QSGItem *> children = QSGItemPrivate::get(item)->paintOrderChildItems();
    for (int i = children.count() - 1; i >= 0; --i) {
        QSGItem *child = children.at(i);
        if (QSGItem *betterCandidate = itemAt(child, item->mapToItem(child, pos), overlay))
            return betterCandidate;
    }

    if (!(item->flags() & QSGItem::ItemHasContents))
        return 0;

    if (!QRectF(0, 0, item->width(), item->height()).contains(pos))
        return 0;

    return item;
}


class SGHoverHighlight : public QSGPaintedItem
{
public:
    SGHoverHighlight(QSGItem *parent) : QSGPaintedItem(parent)
    {
        setZ(1); // hover highlight on top of selection indicator
    }

    void paint(QPainter *painter)
    {
        painter->setPen(QPen(QColor(0, 22, 159)));
        painter->drawRect(QRect(1, 1, width() - 3, height() - 3));
        painter->setPen(QColor(158, 199, 255));
        painter->drawRect(QRect(0, 0, width() - 1, height() - 1));
    }
};


SGSelectionTool::SGSelectionTool(SGViewInspector *inspector) :
    AbstractTool(inspector),
    m_hoverHighlight(new SGHoverHighlight(inspector->overlay()))
{
}

void SGSelectionTool::leaveEvent(QEvent *)
{
    m_hoverHighlight->setVisible(false);
}

void SGSelectionTool::mousePressEvent(QMouseEvent *event)
{
    SGViewInspector *sgInspector = static_cast<SGViewInspector*>(inspector());
    QSGItem *root = sgInspector->view()->rootItem();
    QPointF mappedPos = root->mapFromScene(event->pos());
    QSGItem *item = itemAt(root, mappedPos, sgInspector->overlay());
    if (item && item != root)
        sgInspector->setSelectedItems(QList<QSGItem*>() << item);
}

void SGSelectionTool::hoverMoveEvent(QMouseEvent *event)
{
    SGViewInspector *sgInspector = static_cast<SGViewInspector*>(inspector());
    QSGItem *root = sgInspector->view()->rootItem();
    QPointF mappedPos = root->mapFromScene(event->pos());
    QSGItem *item = itemAt(root, mappedPos, sgInspector->overlay());
    if (!item || item == root) {
        m_hoverHighlight->setVisible(false);
        return;
    }

    m_hoverHighlight->setSize(QSizeF(item->width(), item->height()));
    m_hoverHighlight->setPos(root->mapFromItem(item->parentItem(), item->pos()));
    m_hoverHighlight->setVisible(true);
}

} // namespace QmlJSDebugger
