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

#include "qquickabstractcontainer_p.h"
#include "qquickabstractcontainer_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Container
    \inherits Control
    \instantiates QQuickAbstractContainer
    \inqmlmodule QtQuick.Controls
    \qmlabstract
    \internal
*/

QQuickAbstractContainerPrivate::QQuickAbstractContainerPrivate() :
    contentWidth(0), contentHeight(0), contentItem(Q_NULLPTR)
{
}

QQuickAbstractContainer::QQuickAbstractContainer(QQuickItem *parent) :
    QQuickControl(*(new QQuickAbstractContainerPrivate), parent)
{
}

QQuickAbstractContainer::QQuickAbstractContainer(QQuickAbstractContainerPrivate &dd, QQuickItem *parent) :
    QQuickControl(dd, parent)
{
}

/*!
    \qmlproperty real QtQuickControls2::Container::contentWidth

    TODO
*/
qreal QQuickAbstractContainer::contentWidth() const
{
    Q_D(const QQuickAbstractContainer);
    return d->contentWidth;
}

void QQuickAbstractContainer::setContentWidth(qreal width)
{
    Q_D(QQuickAbstractContainer);
    if (d->contentWidth != width) {
        d->contentWidth = width;
        emit contentWidthChanged();
    }
}

/*!
    \qmlproperty real QtQuickControls2::Container::contentHeight

    TODO
*/
qreal QQuickAbstractContainer::contentHeight() const
{
    Q_D(const QQuickAbstractContainer);
    return d->contentHeight;
}

void QQuickAbstractContainer::setContentHeight(qreal height)
{
    Q_D(QQuickAbstractContainer);
    if (d->contentHeight != height) {
        d->contentHeight = height;
        emit contentHeightChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::Container::contentItem

    TODO
*/
QQuickItem *QQuickAbstractContainer::contentItem() const
{
    Q_D(const QQuickAbstractContainer);
    return d->contentItem;
}

void QQuickAbstractContainer::setContentItem(QQuickItem *item)
{
    Q_D(QQuickAbstractContainer);
    if (d->contentItem != item) {
        contentItemChange(d->contentItem, item);
        delete d->contentItem;
        d->contentItem = item;
        if (item) {
            if (!item->parentItem())
                item->setParentItem(this);
        }
        emit contentItemChanged();
    }
}

void QQuickAbstractContainer::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_UNUSED(newItem);
    Q_UNUSED(oldItem);
}

QT_END_NAMESPACE
