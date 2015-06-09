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

#include "qquicktabview_p.h"
#include "qquicktabbar_p.h"
#include "qquickcontainer_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype TabView
    \inherits Container
    \instantiates QQuickTabView
    \inqmlmodule QtQuick.Controls
    \ingroup tabs
    \brief A tab view control.

    TODO
*/

class QQuickTabViewPrivate : public QQuickContainerPrivate
{
public:
    QQuickTabViewPrivate() : currentIndex(0), bar(Q_NULLPTR) { }

    int currentIndex;
    QQuickTabBar *bar;
};

QQuickTabView::QQuickTabView(QQuickItem *parent) :
    QQuickContainer(*(new QQuickTabViewPrivate), parent)
{
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);
}

/*!
    \qmlproperty int QtQuickControls2::TabView::currentIndex

    TODO
*/
int QQuickTabView::currentIndex() const
{
    Q_D(const QQuickTabView);
    return d->currentIndex;
}

void QQuickTabView::setCurrentIndex(int index)
{
    Q_D(QQuickTabView);
    if (d->currentIndex != index) {
        d->currentIndex = index;
        emit currentIndexChanged(index);
    }
}

/*!
    \qmlproperty TabBar QtQuickControls2::TabView::tabBar

    TODO
*/
QQuickTabBar *QQuickTabView::tabBar() const
{
    Q_D(const QQuickTabView);
    return d->bar;
}

void QQuickTabView::setTabBar(QQuickTabBar *bar)
{
    Q_D(QQuickTabView);
    if (d->bar != bar) {
        if (d->bar) {
            disconnect(this, &QQuickTabView::currentIndexChanged, d->bar, &QQuickTabBar::setCurrentIndex);
            disconnect(d->bar, &QQuickTabBar::currentIndexChanged, this, &QQuickTabView::setCurrentIndex);
            if (d->bar->parentItem() == this)
                delete d->bar;
        }
        d->bar = bar;
        if (bar) {
            connect(this, &QQuickTabView::currentIndexChanged, d->bar, &QQuickTabBar::setCurrentIndex);
            connect(bar, &QQuickTabBar::currentIndexChanged, this, &QQuickTabView::setCurrentIndex);
            if (!bar->parentItem())
                bar->setParentItem(this);

            int barIndex = bar->currentIndex();
            if (d->currentIndex != barIndex) {
                if (d->currentIndex != -1)
                    bar->setCurrentIndex(d->currentIndex);
                else if (barIndex != -1)
                    setCurrentIndex(barIndex);
            }
        }
        emit tabBarChanged();
    }
}

void QQuickTabView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTabView);
    QQuickContainer::geometryChanged(newGeometry, oldGeometry);
    if (d->bar) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(d->bar);
        if (!p->widthValid) {
            d->bar->setWidth(newGeometry.width());
            p->widthValid = false;
        }
    }
}

/*!
    \qmltype Tab
    \inherits QtObject
    \instantiates QQuickTabAttached
    \inqmlmodule QtQuick.Controls
    \ingroup tabs
    \brief TODO

    TODO
*/

QQuickTabAttached::QQuickTabAttached(QObject *parent) :
    QObject(parent), m_index(-1), m_visible(true), m_view(Q_NULLPTR)
{
}

QQuickTabAttached *QQuickTabAttached::qmlAttachedProperties(QObject *object)
{
    return new QQuickTabAttached(object);
}

/*!
    \qmlattachedproperty int QtQuickControls2::Tab::index

    TODO
*/
int QQuickTabAttached::index() const
{
    return m_index;
}

void QQuickTabAttached::setIndex(int index)
{
    if (m_index != index) {
        m_index = index;
        emit indexChanged();
    }
}

/*!
    \qmlattachedproperty string QtQuickControls2::Tab::title

    TODO
*/
QString QQuickTabAttached::title() const
{
    return m_title;
}

void QQuickTabAttached::setTitle(const QString &title)
{
    if (m_title != title) {
        m_title = title;
        emit titleChanged();
    }
}

/*!
    \qmlattachedproperty bool QtQuickControls2::Tab::visible

    TODO
*/
bool QQuickTabAttached::isVisible() const
{
    return m_visible;
}

void QQuickTabAttached::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        emit visibleChanged();
    }
}

/*!
    \qmlattachedproperty TabView QtQuickControls2::Tab::view

    TODO
*/
QQuickTabView *QQuickTabAttached::view() const
{
    return m_view;
}

void QQuickTabAttached::setView(QQuickTabView *view)
{
    if (m_view != view) {
        m_view = view;
        emit viewChanged();
    }
}

QT_END_NAMESPACE
