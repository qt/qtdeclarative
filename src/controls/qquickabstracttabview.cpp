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

#include "qquickabstracttabview_p.h"
#include "qquickabstracttabbar_p.h"
#include "qquickabstractcontainer_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype TabView
    \inherits Container
    \instantiates QQuickAbstractTabView
    \inqmlmodule QtQuick.Controls
    \ingroup tabs
    \brief A tab view control.

    TODO
*/

class QQuickAbstractTabViewPrivate : public QQuickAbstractContainerPrivate
{
public:
    QQuickAbstractTabViewPrivate() : currentIndex(0), bar(Q_NULLPTR) { }

    int currentIndex;
    QQuickAbstractTabBar *bar;
};

QQuickAbstractTabView::QQuickAbstractTabView(QQuickItem *parent) :
    QQuickAbstractContainer(*(new QQuickAbstractTabViewPrivate), parent)
{
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);
}

/*!
    \qmlproperty int QtQuickControls2::TabView::currentIndex

    TODO
*/
int QQuickAbstractTabView::currentIndex() const
{
    Q_D(const QQuickAbstractTabView);
    return d->currentIndex;
}

void QQuickAbstractTabView::setCurrentIndex(int index)
{
    Q_D(QQuickAbstractTabView);
    if (d->currentIndex != index) {
        d->currentIndex = index;
        emit currentIndexChanged(index);
    }
}

/*!
    \qmlproperty TabBar QtQuickControls2::TabView::tabBar

    TODO
*/
QQuickAbstractTabBar *QQuickAbstractTabView::tabBar() const
{
    Q_D(const QQuickAbstractTabView);
    return d->bar;
}

void QQuickAbstractTabView::setTabBar(QQuickAbstractTabBar *bar)
{
    Q_D(QQuickAbstractTabView);
    if (d->bar != bar) {
        if (d->bar) {
            disconnect(this, &QQuickAbstractTabView::currentIndexChanged, d->bar, &QQuickAbstractTabBar::setCurrentIndex);
            disconnect(d->bar, &QQuickAbstractTabBar::currentIndexChanged, this, &QQuickAbstractTabView::setCurrentIndex);
            if (d->bar->parentItem() == this)
                delete d->bar;
        }
        d->bar = bar;
        if (bar) {
            connect(this, &QQuickAbstractTabView::currentIndexChanged, d->bar, &QQuickAbstractTabBar::setCurrentIndex);
            connect(bar, &QQuickAbstractTabBar::currentIndexChanged, this, &QQuickAbstractTabView::setCurrentIndex);
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

void QQuickAbstractTabView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickAbstractTabView);
    QQuickControl::geometryChanged(newGeometry, oldGeometry);
    if (d->bar && (!d->bar->widthValid() || qFuzzyCompare(d->bar->width(), oldGeometry.width())))
        d->bar->setWidth(newGeometry.width());
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
QQuickAbstractTabView *QQuickTabAttached::view() const
{
    return m_view;
}

void QQuickTabAttached::setView(QQuickAbstractTabView *view)
{
    if (m_view != view) {
        m_view = view;
        emit viewChanged();
    }
}

QT_END_NAMESPACE
