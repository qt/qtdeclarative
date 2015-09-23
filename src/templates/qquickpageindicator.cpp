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

#include "qquickpageindicator_p.h"
#include "qquickcontrol_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype PageIndicator
    \inherits Control
    \instantiates QQuickPageIndicator
    \inqmlmodule QtQuick.Controls
    \ingroup indicators
    \brief A page indicator control.

    PageIndicator is used to indicate the currently active page
    in a container of multiple pages. PageIndicator consists of
    delegate items that present pages.

    ### TODO: screenshot

    \code
    PageIndicator {
        count: view.count
        currentIndex: view.currentIndex
        anchors.bottom: view.bottom
        anchors.horizontalCenter: view.horizontalCenter
    }
    \endcode

    \sa SwipeView, {Customizing PageIndicator}
*/

class QQuickPageIndicatorPrivate : public QQuickControlPrivate
{
public:
    QQuickPageIndicatorPrivate() : count(0), currentIndex(0), delegate(Q_NULLPTR) { }

    int count;
    int currentIndex;
    QQmlComponent *delegate;
    QColor color;
};

QQuickPageIndicator::QQuickPageIndicator(QQuickItem *parent) :
    QQuickControl(*(new QQuickPageIndicatorPrivate), parent)
{
    setAccessibleRole(0x00000027); //QAccessible::Indicator
}

/*!
    \qmlproperty int QtQuick.Controls::PageIndicator::count

    This property holds the number of pages.
*/
int QQuickPageIndicator::count() const
{
    Q_D(const QQuickPageIndicator);
    return d->count;
}

void QQuickPageIndicator::setCount(int count)
{
    Q_D(QQuickPageIndicator);
    if (d->count != count) {
        d->count = count;
        emit countChanged();
    }
}

/*!
    \qmlproperty int QtQuick.Controls::PageIndicator::currentIndex

    This property holds the index of the current page.
*/
int QQuickPageIndicator::currentIndex() const
{
    Q_D(const QQuickPageIndicator);
    return d->currentIndex;
}

void QQuickPageIndicator::setCurrentIndex(int index)
{
    Q_D(QQuickPageIndicator);
    if (d->currentIndex != index) {
        d->currentIndex = index;
        emit currentIndexChanged();
    }
}

/*!
    \qmlproperty Component QtQuick.Controls::PageIndicator::delegate

    This property holds a delegate that presents a page.

    \sa color
*/
QQmlComponent *QQuickPageIndicator::delegate() const
{
    Q_D(const QQuickPageIndicator);
    return d->delegate;
}

void QQuickPageIndicator::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickPageIndicator);
    if (d->delegate != delegate) {
        d->delegate = delegate;
        emit delegateChanged();
    }
}

/*!
    \qmlproperty color QtQuick.Controls::PageIndicator::color

    This property holds the color of the indicator.

    \sa delegate
*/
QColor QQuickPageIndicator::color() const
{
    Q_D(const QQuickPageIndicator);
    return d->color;
}

void QQuickPageIndicator::setColor(const QColor &color)
{
    Q_D(QQuickPageIndicator);
    if (d->color != color) {
        d->color = color;
        emit colorChanged();
    }
}

QT_END_NAMESPACE
