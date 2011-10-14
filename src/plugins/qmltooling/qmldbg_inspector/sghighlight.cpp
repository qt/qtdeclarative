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

#include "sghighlight.h"

#include <QtGui/QPainter>

namespace QmlJSDebugger {

SGHighlight::SGHighlight(QQuickItem *item, QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setItem(item);
}

void SGHighlight::setItem(QQuickItem *item)
{
    if (m_item)
        m_item.data()->disconnect(this);

    if (item) {
        connect(item, SIGNAL(xChanged()), SLOT(adjust()));
        connect(item, SIGNAL(yChanged()), SLOT(adjust()));
        connect(item, SIGNAL(widthChanged()), SLOT(adjust()));
        connect(item, SIGNAL(heightChanged()), SLOT(adjust()));
        connect(item, SIGNAL(rotationChanged()), SLOT(adjust()));
        connect(item, SIGNAL(transformOriginChanged(TransformOrigin)),
                SLOT(adjust()));
    }

    m_item = item;
    adjust();
}

void SGHighlight::adjust()
{
    const QQuickItem *item = m_item.data();
    setSize(QSizeF(item->width(), item->height()));
    setPos(parentItem()->mapFromItem(item->parentItem(), item->pos()));
    setRotation(item->rotation());
    setTransformOrigin(item->transformOrigin());
}


void SGSelectionHighlight::paint(QPainter *painter)
{
    painter->setPen(QColor(108, 141, 221));
    painter->drawRect(QRect(0, 0, width() - 1, height() - 1));
}


void SGHoverHighlight::paint(QPainter *painter)
{
    painter->setPen(QPen(QColor(0, 22, 159)));
    painter->drawRect(QRect(1, 1, width() - 3, height() - 3));
    painter->setPen(QColor(158, 199, 255));
    painter->drawRect(QRect(0, 0, width() - 1, height() - 1));
}

} // namespace QmlJSDebugger
