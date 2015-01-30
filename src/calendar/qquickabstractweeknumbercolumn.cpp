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

#include "qquickabstractweeknumbercolumn_p.h"
#include "qquickweeknumbermodel_p.h"

#include <QtQuickControls/private/qquickabstractcontainer_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractWeekNumberColumnPrivate : public QQuickAbstractContainerPrivate
{
public:
    QVariant source;
    QQuickWeekNumberModel *model;
};

QQuickAbstractWeekNumberColumn::QQuickAbstractWeekNumberColumn(QQuickItem *parent) :
    QQuickAbstractContainer(*(new QQuickAbstractWeekNumberColumnPrivate), parent)
{
    Q_D(QQuickAbstractWeekNumberColumn);
    d->model = new QQuickWeekNumberModel(this);
    d->source = QVariant::fromValue(d->model);
    connect(d->model, &QQuickWeekNumberModel::monthChanged, this, &QQuickAbstractWeekNumberColumn::monthChanged);
    connect(d->model, &QQuickWeekNumberModel::yearChanged, this, &QQuickAbstractWeekNumberColumn::yearChanged);
    connect(d->model, &QQuickWeekNumberModel::localeChanged, this, &QQuickAbstractWeekNumberColumn::localeChanged);
}

int QQuickAbstractWeekNumberColumn::month() const
{
    Q_D(const QQuickAbstractWeekNumberColumn);
    return d->model->month();
}

void QQuickAbstractWeekNumberColumn::setMonth(int month)
{
    Q_D(QQuickAbstractWeekNumberColumn);
    d->model->setMonth(month);
}

int QQuickAbstractWeekNumberColumn::year() const
{
    Q_D(const QQuickAbstractWeekNumberColumn);
    return d->model->year();
}

void QQuickAbstractWeekNumberColumn::setYear(int year)
{
    Q_D(QQuickAbstractWeekNumberColumn);
    d->model->setYear(year);
}

QLocale QQuickAbstractWeekNumberColumn::locale() const
{
    Q_D(const QQuickAbstractWeekNumberColumn);
    return d->model->locale();
}

void QQuickAbstractWeekNumberColumn::setLocale(const QLocale &locale)
{
    Q_D(QQuickAbstractWeekNumberColumn);
    d->model->setLocale(locale);
}

QVariant QQuickAbstractWeekNumberColumn::source() const
{
    Q_D(const QQuickAbstractWeekNumberColumn);
    return d->source;
}

void QQuickAbstractWeekNumberColumn::setSource(const QVariant &source)
{
    Q_D(QQuickAbstractWeekNumberColumn);
    if (d->source != source) {
        d->source = source;
        emit sourceChanged();
    }
}

QT_END_NAMESPACE
