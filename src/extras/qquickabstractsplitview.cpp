/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Extras module of the Qt Toolkit.
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

#include "qquickabstractsplitview_p.h"

#include <QtQuickControls/private/qquickabstractcontainer_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractSplitViewPrivate : public QQuickAbstractContainerPrivate
{
public:
    QQuickAbstractSplitViewPrivate() : orientation(Qt::Horizontal) { }

    bool resizing;
    Qt::Orientation orientation;
};

QQuickAbstractSplitView::QQuickAbstractSplitView(QQuickItem *parent) :
    QQuickAbstractContainer(*(new QQuickAbstractSplitViewPrivate), parent)
{
}

Qt::Orientation QQuickAbstractSplitView::orientation() const
{
    Q_D(const QQuickAbstractSplitView);
    return d->orientation;
}

void QQuickAbstractSplitView::setOrientation(Qt::Orientation orientation)
{
    Q_D(QQuickAbstractSplitView);
    if (d->orientation != orientation) {
        d->orientation = orientation;
        emit orientationChanged();
    }
}

bool QQuickAbstractSplitView::isResizing() const
{
    Q_D(const QQuickAbstractSplitView);
    return d->resizing;
}

void QQuickAbstractSplitView::setResizing(bool resizing)
{
    Q_D(QQuickAbstractSplitView);
    if (d->resizing != resizing) {
        d->resizing = resizing;
        emit resizingChanged();
    }
}

void QQuickAbstractSplitView::addItem(QQuickItem *item)
{
    // TODO
    Q_UNUSED(item);
}

void QQuickAbstractSplitView::componentComplete()
{
}

QT_END_NAMESPACE
