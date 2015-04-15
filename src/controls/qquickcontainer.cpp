/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickcontainer_p.h"
#include "qquickcontainer_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Container
    \inherits Control
    \instantiates QQuickContainer
    \inqmlmodule QtQuick.Controls
    \qmlabstract
    \internal
*/

QQuickContainerPrivate::QQuickContainerPrivate() :
    contentWidth(0), contentHeight(0), contentItem(Q_NULLPTR)
{
}

QQuickContainer::QQuickContainer(QQuickItem *parent) :
    QQuickControl(*(new QQuickContainerPrivate), parent)
{
}

QQuickContainer::QQuickContainer(QQuickContainerPrivate &dd, QQuickItem *parent) :
    QQuickControl(dd, parent)
{
}

/*!
    \qmlproperty real QtQuickControls2::Container::contentWidth

    TODO
*/
qreal QQuickContainer::contentWidth() const
{
    Q_D(const QQuickContainer);
    return d->contentWidth;
}

void QQuickContainer::setContentWidth(qreal width)
{
    Q_D(QQuickContainer);
    if (d->contentWidth != width) {
        d->contentWidth = width;
        emit contentWidthChanged();
    }
}

/*!
    \qmlproperty real QtQuickControls2::Container::contentHeight

    TODO
*/
qreal QQuickContainer::contentHeight() const
{
    Q_D(const QQuickContainer);
    return d->contentHeight;
}

void QQuickContainer::setContentHeight(qreal height)
{
    Q_D(QQuickContainer);
    if (d->contentHeight != height) {
        d->contentHeight = height;
        emit contentHeightChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::Container::contentItem

    TODO
*/
QQuickItem *QQuickContainer::contentItem() const
{
    Q_D(const QQuickContainer);
    return d->contentItem;
}

void QQuickContainer::setContentItem(QQuickItem *item)
{
    Q_D(QQuickContainer);
    if (d->contentItem != item) {
        contentItemChange(item, d->contentItem);
        delete d->contentItem;
        d->contentItem = item;
        if (item) {
            if (!item->parentItem())
                item->setParentItem(this);
        }
        emit contentItemChanged();
    }
}

void QQuickContainer::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_UNUSED(newItem);
    Q_UNUSED(oldItem);
}

QT_END_NAMESPACE
