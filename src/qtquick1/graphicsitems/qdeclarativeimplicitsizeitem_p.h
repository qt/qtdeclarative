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

#ifndef QDECLARATIVEIMPLICITSIZEITEM_H
#define QDECLARATIVEIMPLICITSIZEITEM_H

#include "qdeclarativepainteditem_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QDeclarative1ImplicitSizeItemPrivate;
class Q_AUTOTEST_EXPORT QDeclarative1ImplicitSizeItem : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(qreal implicitWidth READ implicitWidth NOTIFY implicitWidthChanged REVISION 1)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight NOTIFY implicitHeightChanged REVISION 1)

public:
    QDeclarative1ImplicitSizeItem(QDeclarativeItem *parent = 0);

protected:
    QDeclarative1ImplicitSizeItem(QDeclarative1ImplicitSizeItemPrivate &dd, QDeclarativeItem *parent);

Q_SIGNALS:
    Q_REVISION(1) void implicitWidthChanged();
    Q_REVISION(1) void implicitHeightChanged();

private:
    Q_DISABLE_COPY(QDeclarative1ImplicitSizeItem)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarative1ImplicitSizeItem)
};

class QDeclarative1ImplicitSizePaintedItemPrivate;
class Q_AUTOTEST_EXPORT QDeclarative1ImplicitSizePaintedItem : public QDeclarative1PaintedItem
{
    Q_OBJECT
    Q_PROPERTY(qreal implicitWidth READ implicitWidth NOTIFY implicitWidthChanged REVISION 1)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight NOTIFY implicitHeightChanged REVISION 1)

public:
    QDeclarative1ImplicitSizePaintedItem(QDeclarativeItem *parent = 0);

protected:
    QDeclarative1ImplicitSizePaintedItem(QDeclarative1ImplicitSizePaintedItemPrivate &dd, QDeclarativeItem *parent);
    virtual void drawContents(QPainter *, const QRect &) {};

Q_SIGNALS:
    Q_REVISION(1) void implicitWidthChanged();
    Q_REVISION(1) void implicitHeightChanged();

private:
    Q_DISABLE_COPY(QDeclarative1ImplicitSizePaintedItem)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarative1ImplicitSizePaintedItem)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDECLARATIVEIMPLICITSIZEITEM_H
