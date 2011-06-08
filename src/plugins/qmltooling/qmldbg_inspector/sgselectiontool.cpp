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
#include <QtDeclarative/private/qsgitem_p.h>
#include <QtDeclarative/private/qsgrectangle_p.h>

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


SGSelectionTool::SGSelectionTool(SGViewInspector *inspector) :
    SGAbstractTool(inspector),
    m_hoverHighlight(new QSGRectangle(inspector->overlay()))
{
    m_hoverHighlight->border()->setColor(QColor(64, 128, 255));
    m_hoverHighlight->setColor(Qt::transparent);
}

void SGSelectionTool::leaveEvent(QEvent *)
{
    m_hoverHighlight->setVisible(false);
}

void SGSelectionTool::hoverMoveEvent(QMouseEvent *event)
{
    QSGItem *root = inspector()->view()->rootItem();
    QPointF mappedPos = root->mapFromScene(event->pos());
    QSGItem *item = itemAt(root, mappedPos, inspector()->overlay());
    if (!item || item == root) {
        m_hoverHighlight->setVisible(false);
        return;
    }

    m_hoverHighlight->setSize(QSizeF(item->width() - 1, item->height() - 1));
    m_hoverHighlight->setPos(root->mapFromItem(item->parentItem(), item->pos()));
    m_hoverHighlight->setVisible(true);
}

} // namespace QmlJSDebugger
