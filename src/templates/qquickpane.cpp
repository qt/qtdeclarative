/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#include "qquickpane_p.h"
#include "qquickpane_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Pane
    \inherits Control
    \instantiates QQuickPane
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-containers
    \brief A pane control.

    Pane provides a background color that matches with the application style
    and theme. Pane does not provide a layout of its own, but requires you to
    position its contents, for instance by creating a \l RowLayout or a
    \l ColumnLayout.

    If only a single item is used within a Pane, it will resize to fit the
    implicit size of its contained item. This makes it particularly suitable
    for use together with layouts.

    \image qtlabscontrols-pane.png

    \snippet qtlabscontrols-pane.qml 1

    \labs

    \sa {Customizing Pane}, {Container Controls}
*/

QQuickPanePrivate::QQuickPanePrivate() : contentWidth(0), contentHeight(0)
{
}

QQuickPane::QQuickPane(QQuickItem *parent) :
    QQuickControl(*(new QQuickPanePrivate), parent)
{
    setFlag(QQuickItem::ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::AllButtons);
}

QQuickPane::QQuickPane(QQuickPanePrivate &dd, QQuickItem *parent) :
    QQuickControl(dd, parent)
{
    setFlag(QQuickItem::ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::AllButtons);
}

/*!
    \qmlproperty real Qt.labs.controls::Pane::contentWidth

    This property holds the content width. It is used for calculating the
    total implicit width of the pane.

    \note If only a single item is used within the pane, the implicit width
          of its contained item is used as the content width.
*/
qreal QQuickPane::contentWidth() const
{
    Q_D(const QQuickPane);
    return d->contentWidth;
}

void QQuickPane::setContentWidth(qreal width)
{
    Q_D(QQuickPane);
    if (d->contentWidth != width) {
        d->contentWidth = width;
        emit contentWidthChanged();
    }
}

/*!
    \qmlproperty real Qt.labs.controls::Pane::contentHeight

    This property holds the content height. It is used for calculating the
    total implicit height of the pane.

    \note If only a single item is used within the pane, the implicit height
          of its contained item is used as the content height.
*/
qreal QQuickPane::contentHeight() const
{
    Q_D(const QQuickPane);
    return d->contentHeight;
}

void QQuickPane::setContentHeight(qreal height)
{
    Q_D(QQuickPane);
    if (d->contentHeight != height) {
        d->contentHeight = height;
        emit contentHeightChanged();
    }
}

/*!
    \qmlproperty list<Object> Qt.labs.controls::Pane::contentData
    \default

    This property holds the list of content data.

    \sa Item::data
*/
QQmlListProperty<QObject> QQuickPane::contentData()
{
    Q_D(QQuickPane);
    return QQmlListProperty<QObject>(d->contentItem, Q_NULLPTR,
                                     QQuickItemPrivate::data_append,
                                     QQuickItemPrivate::data_count,
                                     QQuickItemPrivate::data_at,
                                     QQuickItemPrivate::data_clear);
}

/*!
    \qmlproperty list<Item> Qt.labs.controls::Pane::contentChildren

    This property holds the list of content children.

    \sa Item::children
*/
QQmlListProperty<QQuickItem> QQuickPane::contentChildren()
{
    Q_D(QQuickPane);
    return QQmlListProperty<QQuickItem>(d->contentItem, Q_NULLPTR,
                                        QQuickItemPrivate::children_append,
                                        QQuickItemPrivate::children_count,
                                        QQuickItemPrivate::children_at,
                                        QQuickItemPrivate::children_clear);
}

void QQuickPane::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    QQuickControl::contentItemChange(newItem, oldItem);
    if (oldItem)
        disconnect(oldItem, &QQuickItem::childrenChanged, this, &QQuickPane::contentChildrenChanged);
    if (newItem)
        connect(newItem, &QQuickItem::childrenChanged, this, &QQuickPane::contentChildrenChanged);
    emit contentChildrenChanged();
}

#ifndef QT_NO_ACCESSIBILITY
QAccessible::Role QQuickPane::accessibleRole() const
{
    return QAccessible::Pane;
}
#endif

QT_END_NAMESPACE
