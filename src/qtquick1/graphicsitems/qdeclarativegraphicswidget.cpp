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

#include "QtQuick1/private/qdeclarativegraphicswidget_p.h"
#include "QtQuick1/private/qdeclarativeanchors_p.h"
#include "QtQuick1/private/qdeclarativeitem_p.h"
#include "QtQuick1/private/qdeclarativeanchors_p_p.h"

QT_BEGIN_NAMESPACE



class QDeclarative1GraphicsWidgetPrivate : public QObjectPrivate {
    Q_DECLARE_PUBLIC(QDeclarative1GraphicsWidget)
public :
    QDeclarative1GraphicsWidgetPrivate() :
        _anchors(0), _anchorLines(0)
    {}
    QDeclarativeItemPrivate::AnchorLines *anchorLines() const;
    QDeclarative1Anchors *_anchors;
    mutable QDeclarativeItemPrivate::AnchorLines *_anchorLines;
};

QDeclarative1GraphicsWidget::QDeclarative1GraphicsWidget(QObject *parent) :
    QObject(*new QDeclarative1GraphicsWidgetPrivate, parent)
{
}
QDeclarative1GraphicsWidget::~QDeclarative1GraphicsWidget()
{
    Q_D(QDeclarative1GraphicsWidget);
    delete d->_anchorLines; d->_anchorLines = 0;
    delete d->_anchors; d->_anchors = 0;
}

QDeclarative1Anchors *QDeclarative1GraphicsWidget::anchors()
{
    Q_D(QDeclarative1GraphicsWidget);
    if (!d->_anchors)
        d->_anchors = new QDeclarative1Anchors(static_cast<QGraphicsObject *>(parent()));
    return d->_anchors;
}

QDeclarativeItemPrivate::AnchorLines *QDeclarative1GraphicsWidgetPrivate::anchorLines() const
{
    Q_Q(const QDeclarative1GraphicsWidget);
    if (!_anchorLines)
        _anchorLines = new QDeclarativeItemPrivate::AnchorLines(static_cast<QGraphicsObject *>(q->parent()));
    return _anchorLines;
}

QDeclarative1AnchorLine QDeclarative1GraphicsWidget::left() const
{
    Q_D(const QDeclarative1GraphicsWidget);
    return d->anchorLines()->left;
}

QDeclarative1AnchorLine QDeclarative1GraphicsWidget::right() const
{
    Q_D(const QDeclarative1GraphicsWidget);
    return d->anchorLines()->right;
}

QDeclarative1AnchorLine QDeclarative1GraphicsWidget::horizontalCenter() const
{
    Q_D(const QDeclarative1GraphicsWidget);
    return d->anchorLines()->hCenter;
}

QDeclarative1AnchorLine QDeclarative1GraphicsWidget::top() const
{
    Q_D(const QDeclarative1GraphicsWidget);
    return d->anchorLines()->top;
}

QDeclarative1AnchorLine QDeclarative1GraphicsWidget::bottom() const
{
    Q_D(const QDeclarative1GraphicsWidget);
    return d->anchorLines()->bottom;
}

QDeclarative1AnchorLine QDeclarative1GraphicsWidget::verticalCenter() const
{
    Q_D(const QDeclarative1GraphicsWidget);
    return d->anchorLines()->vCenter;
}



QT_END_NAMESPACE

#include <moc_qdeclarativegraphicswidget_p.cpp>
