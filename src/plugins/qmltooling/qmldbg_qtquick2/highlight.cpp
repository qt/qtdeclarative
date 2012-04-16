/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "highlight.h"

#include <QtGui/QPainter>
#include <QtQuick/QQuickCanvas>

namespace QmlJSDebugger {
namespace QtQuick2 {

Highlight::Highlight(QQuickItem *item, QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setItem(item);
}

void Highlight::setItem(QQuickItem *item)
{
    if (m_item)
        m_item->disconnect(this);

    if (item) {
        connect(item, SIGNAL(xChanged()), SLOT(adjust()));
        connect(item, SIGNAL(yChanged()), SLOT(adjust()));
        connect(item, SIGNAL(widthChanged()), SLOT(adjust()));
        connect(item, SIGNAL(heightChanged()), SLOT(adjust()));
        connect(item, SIGNAL(rotationChanged()), SLOT(adjust()));
        connect(item, SIGNAL(transformOriginChanged(TransformOrigin)),
                SLOT(adjust()));
    }
    QQuickCanvas *view = item->canvas();
    QQuickItem * rootItem = view->rootItem();
    if (rootItem) {
        connect(rootItem, SIGNAL(xChanged()), SLOT(adjust()));
        connect(rootItem, SIGNAL(yChanged()), SLOT(adjust()));
        connect(rootItem, SIGNAL(widthChanged()), SLOT(adjust()));
        connect(rootItem, SIGNAL(heightChanged()), SLOT(adjust()));
        connect(rootItem, SIGNAL(rotationChanged()), SLOT(adjust()));
        connect(rootItem, SIGNAL(transformOriginChanged(TransformOrigin)),
                SLOT(adjust()));
    }
    m_item = item;
    adjust();
}

void Highlight::adjust()
{
    if (!m_item)
        return;

    bool success = false;
    m_transform = m_item->itemTransform(0, &success);
    if (!success)
        m_transform = QTransform();

    setSize(QSizeF(m_item->width(), m_item->height()));
    qreal scaleFactor = 1;
    QPointF originOffset = QPointF(0,0);
    QQuickCanvas *view = m_item->canvas();
    if (view->rootItem()) {
        scaleFactor = view->rootItem()->scale();
        originOffset -= view->rootItem()->pos();
    }
    // The scale transform for the overlay needs to be cancelled
    // as the Item's transform which will be applied to the painter
    // takes care of it.
    parentItem()->setScale(1/scaleFactor);
    setPos(originOffset);
    setContentsSize(view->size());
    update();
}


void HoverHighlight::paint(QPainter *painter)
{
    if (!item())
        return;

    painter->save();
    painter->setTransform(transform());
    painter->setPen(QColor(108, 141, 221));
    painter->drawRect(QRect(0, 0, item()->width() - 1, item()->height() - 1));
    painter->restore();
}


void SelectionHighlight::paint(QPainter *painter)
{
    if (!item())
        return;

    painter->save();
    painter->setTransform(transform());
    if (item()->height() >= 10 && item()->width() >= 10) {
        QColor colorHighlight = Qt::green;
        painter->fillRect(QRectF(0, 0, item()->width(), 5), colorHighlight);
        painter->fillRect(QRectF(0, item()->height()-5, item()->width(), 5), colorHighlight);
        painter->fillRect(QRectF(0, 5, 5, item()->height() - 10), colorHighlight);
        painter->fillRect(QRectF(item()->width()-5, 5, 5, item()->height() - 10), colorHighlight);
    }
    painter->setPen(QPen(QColor(0, 22, 159)));
    painter->drawRect(QRect(1, 1, item()->width() - 3, item()->height() - 3));
    painter->setPen(QColor(158, 199, 255));
    painter->drawRect(QRect(0, 0, item()->width() - 1, item()->height() - 1));
    painter->restore();
}

} // namespace QtQuick2
} // namespace QmlJSDebugger
