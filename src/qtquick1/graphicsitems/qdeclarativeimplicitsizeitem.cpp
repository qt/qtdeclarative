/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include "QtQuick1/private/qdeclarativeimplicitsizeitem_p.h"
#include "QtQuick1/private/qdeclarativeimplicitsizeitem_p_p.h"

QT_BEGIN_NAMESPACE



void QDeclarative1ImplicitSizeItemPrivate::implicitWidthChanged()
{
    Q_Q(QDeclarative1ImplicitSizeItem);
    emit q->implicitWidthChanged();
}

void QDeclarative1ImplicitSizeItemPrivate::implicitHeightChanged()
{
    Q_Q(QDeclarative1ImplicitSizeItem);
    emit q->implicitHeightChanged();
}

QDeclarative1ImplicitSizeItem::QDeclarative1ImplicitSizeItem(QDeclarativeItem *parent)
    : QDeclarativeItem(*(new QDeclarative1ImplicitSizeItemPrivate), parent)
{
}

QDeclarative1ImplicitSizeItem::QDeclarative1ImplicitSizeItem(QDeclarative1ImplicitSizeItemPrivate &dd, QDeclarativeItem *parent)
    : QDeclarativeItem(dd, parent)
{
}


void QDeclarative1ImplicitSizePaintedItemPrivate::implicitWidthChanged()
{
    Q_Q(QDeclarative1ImplicitSizePaintedItem);
    emit q->implicitWidthChanged();
}

void QDeclarative1ImplicitSizePaintedItemPrivate::implicitHeightChanged()
{
    Q_Q(QDeclarative1ImplicitSizePaintedItem);
    emit q->implicitHeightChanged();
}

QDeclarative1ImplicitSizePaintedItem::QDeclarative1ImplicitSizePaintedItem(QDeclarativeItem *parent)
    : QDeclarative1PaintedItem(*(new QDeclarative1ImplicitSizePaintedItemPrivate), parent)
{
}

QDeclarative1ImplicitSizePaintedItem::QDeclarative1ImplicitSizePaintedItem(QDeclarative1ImplicitSizePaintedItemPrivate &dd, QDeclarativeItem *parent)
    : QDeclarative1PaintedItem(dd, parent)
{
}



QT_END_NAMESPACE
