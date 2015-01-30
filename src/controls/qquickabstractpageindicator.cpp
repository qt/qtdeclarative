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

#include "qquickabstractpageindicator_p.h"
#include "qquickabstractcontainer_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype PageIndicator
    \inherits Control
    \instantiates QQuickAbstractPageIndicator
    \inqmlmodule QtQuick.Controls
    \ingroup indicators
    \brief A page indicator.

    TODO
*/

class QQuickAbstractPageIndicatorPrivate : public QQuickAbstractContainerPrivate
{
public:
    QQuickAbstractPageIndicatorPrivate() : count(0), currentIndex(0) { }

    int count;
    int currentIndex;
};

QQuickAbstractPageIndicator::QQuickAbstractPageIndicator(QQuickItem *parent) :
    QQuickAbstractContainer(*(new QQuickAbstractPageIndicatorPrivate), parent)
{
}

/*!
    \qmlproperty int QtQuickControls2::PageIndicator::count

    TODO
*/
int QQuickAbstractPageIndicator::count() const
{
    Q_D(const QQuickAbstractPageIndicator);
    return d->count;
}

void QQuickAbstractPageIndicator::setCount(int count)
{
    Q_D(QQuickAbstractPageIndicator);
    if (d->count != count) {
        d->count = count;
        emit countChanged();
    }
}

/*!
    \qmlproperty int QtQuickControls2::PageIndicator::currentIndex

    TODO
*/
int QQuickAbstractPageIndicator::currentIndex() const
{
    Q_D(const QQuickAbstractPageIndicator);
    return d->currentIndex;
}

void QQuickAbstractPageIndicator::setCurrentIndex(int index)
{
    Q_D(QQuickAbstractPageIndicator);
    if (d->currentIndex != index) {
        d->currentIndex = index;
        emit currentIndexChanged();
    }
}

QT_END_NAMESPACE
