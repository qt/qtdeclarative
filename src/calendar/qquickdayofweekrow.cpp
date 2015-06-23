/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Calendar module of the Qt Toolkit.
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

#include "qquickdayofweekrow_p.h"
#include "qquickdayofweekmodel_p.h"

#include <QtQuickControls/private/qquickcontrol_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickDayOfWeekRowPrivate : public QQuickControlPrivate
{
public:
    QQuickDayOfWeekRowPrivate() : delegate(Q_NULLPTR), model(Q_NULLPTR) { }

    QVariant source;
    QQmlComponent *delegate;
    QQuickDayOfWeekModel *model;
};

QQuickDayOfWeekRow::QQuickDayOfWeekRow(QQuickItem *parent) :
    QQuickControl(*(new QQuickDayOfWeekRowPrivate), parent)
{
    Q_D(QQuickDayOfWeekRow);
    d->model = new QQuickDayOfWeekModel(this);
    d->source = QVariant::fromValue(d->model);
    connect(d->model, &QQuickDayOfWeekModel::localeChanged, this, &QQuickDayOfWeekRow::localeChanged);
}

QLocale QQuickDayOfWeekRow::locale() const
{
    Q_D(const QQuickDayOfWeekRow);
    return d->model->locale();
}

void QQuickDayOfWeekRow::setLocale(const QLocale &locale)
{
    Q_D(QQuickDayOfWeekRow);
    d->model->setLocale(locale);
}

QVariant QQuickDayOfWeekRow::source() const
{
    Q_D(const QQuickDayOfWeekRow);
    return d->source;
}

void QQuickDayOfWeekRow::setSource(const QVariant &source)
{
    Q_D(QQuickDayOfWeekRow);
    if (d->source != source) {
        d->source = source;
        emit sourceChanged();
    }
}

QQmlComponent *QQuickDayOfWeekRow::delegate() const
{
    Q_D(const QQuickDayOfWeekRow);
    return d->delegate;
}

void QQuickDayOfWeekRow::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickDayOfWeekRow);
    if (d->delegate != delegate) {
        d->delegate = delegate;
        emit delegateChanged();
    }
}

QT_END_NAMESPACE
