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

#include "qquickabstractbusyindicator_p.h"
#include "qquickcontrol_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype BusyIndicator
    \inherits Control
    \instantiates QQuickAbstractBusyIndicator
    \inqmlmodule QtQuick.Controls
    \ingroup indicators
    \brief A busy indicator.

    TODO
*/

class QQuickAbstractBusyIndicatorPrivate : public QQuickControlPrivate
{
public:
    QQuickAbstractBusyIndicatorPrivate() : running(false), indicator(Q_NULLPTR) { }

    bool running;
    QQuickItem *indicator;
};

QQuickAbstractBusyIndicator::QQuickAbstractBusyIndicator(QQuickItem *parent) :
    QQuickControl(*(new QQuickAbstractBusyIndicatorPrivate), parent)
{
}

/*!
    \qmlproperty bool QtQuickControls2::BusyIndicator::running

    TODO
*/
bool QQuickAbstractBusyIndicator::isRunning() const
{
    Q_D(const QQuickAbstractBusyIndicator);
    return d->running;
}

void QQuickAbstractBusyIndicator::setRunning(bool running)
{
    Q_D(QQuickAbstractBusyIndicator);
    if (d->running != running) {
        d->running = running;
        emit runningChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::BusyIndicator::indicator

    TODO
*/
QQuickItem *QQuickAbstractBusyIndicator::indicator() const
{
    Q_D(const QQuickAbstractBusyIndicator);
    return d->indicator;
}

void QQuickAbstractBusyIndicator::setIndicator(QQuickItem *indicator)
{
    Q_D(QQuickAbstractBusyIndicator);
    if (d->indicator != indicator) {
        delete d->indicator;
        d->indicator = indicator;
        if (indicator && !indicator->parentItem())
            indicator->setParentItem(this);
        emit indicatorChanged();
    }
}

QT_END_NAMESPACE
