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

#include "qquickimplicitsizeitem_p.h"
#include "qquickimplicitsizeitem_p_p.h"

QT_BEGIN_NAMESPACE

void QQuickImplicitSizeItemPrivate::implicitWidthChanged()
{
    Q_Q(QQuickImplicitSizeItem);
    emit q->implicitWidthChanged();
}

void QQuickImplicitSizeItemPrivate::implicitHeightChanged()
{
    Q_Q(QQuickImplicitSizeItem);
    emit q->implicitHeightChanged();
}

QQuickImplicitSizeItem::QQuickImplicitSizeItem(QQuickItem *parent)
    : QQuickItem(*(new QQuickImplicitSizeItemPrivate), parent)
{
}

QQuickImplicitSizeItem::QQuickImplicitSizeItem(QQuickImplicitSizeItemPrivate &dd, QQuickItem *parent)
    : QQuickItem(dd, parent)
{
}


void QQuickImplicitSizePaintedItemPrivate::implicitWidthChanged()
{
    Q_Q(QQuickImplicitSizePaintedItem);
    emit q->implicitWidthChanged();
}

void QQuickImplicitSizePaintedItemPrivate::implicitHeightChanged()
{
    Q_Q(QQuickImplicitSizePaintedItem);
    emit q->implicitHeightChanged();
}

QQuickImplicitSizePaintedItem::QQuickImplicitSizePaintedItem(QQuickItem *parent)
    : QQuickPaintedItem(*(new QQuickImplicitSizePaintedItemPrivate), parent)
{
}

QQuickImplicitSizePaintedItem::QQuickImplicitSizePaintedItem(QQuickImplicitSizePaintedItemPrivate &dd, QQuickItem *parent)
    : QQuickPaintedItem(dd, parent)
{
}

QT_END_NAMESPACE
