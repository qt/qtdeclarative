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

#include "qquickabstractdayofweekrow_p.h"
#include "qquickdayofweekmodel_p.h"

#include <QtQuickControls/private/qquickabstractcontainer_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractDayOfWeekRowPrivate : public QQuickAbstractContainerPrivate
{
public:
    QVariant source;
    QQuickDayOfWeekModel *model;
};

QQuickAbstractDayOfWeekRow::QQuickAbstractDayOfWeekRow(QQuickItem *parent) :
    QQuickAbstractContainer(*(new QQuickAbstractDayOfWeekRowPrivate), parent)
{
    Q_D(QQuickAbstractDayOfWeekRow);
    d->model = new QQuickDayOfWeekModel(this);
    d->source = QVariant::fromValue(d->model);
    connect(d->model, &QQuickDayOfWeekModel::localeChanged, this, &QQuickAbstractDayOfWeekRow::localeChanged);
}

QLocale QQuickAbstractDayOfWeekRow::locale() const
{
    Q_D(const QQuickAbstractDayOfWeekRow);
    return d->model->locale();
}

void QQuickAbstractDayOfWeekRow::setLocale(const QLocale &locale)
{
    Q_D(QQuickAbstractDayOfWeekRow);
    d->model->setLocale(locale);
}

QVariant QQuickAbstractDayOfWeekRow::source() const
{
    Q_D(const QQuickAbstractDayOfWeekRow);
    return d->source;
}

void QQuickAbstractDayOfWeekRow::setSource(const QVariant &source)
{
    Q_D(QQuickAbstractDayOfWeekRow);
    if (d->source != source) {
        d->source = source;
        emit sourceChanged();
    }
}

QT_END_NAMESPACE
