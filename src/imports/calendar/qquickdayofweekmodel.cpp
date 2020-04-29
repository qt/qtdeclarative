/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Calendar module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickdayofweekmodel_p.h"

#include <QtCore/private/qabstractitemmodel_p.h>

QT_BEGIN_NAMESPACE

class QQuickDayOfWeekModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QQuickDayOfWeekModel)

public:
    QLocale locale;
};

QQuickDayOfWeekModel::QQuickDayOfWeekModel(QObject *parent) :
    QAbstractListModel(*(new QQuickDayOfWeekModelPrivate), parent)
{
}

QLocale QQuickDayOfWeekModel::locale() const
{
    Q_D(const QQuickDayOfWeekModel);
    return d->locale;
}

void QQuickDayOfWeekModel::setLocale(const QLocale &locale)
{
    Q_D(QQuickDayOfWeekModel);
    if (d->locale != locale) {
        d->locale = locale;
        emit localeChanged();
        emit dataChanged(index(0, 0), index(6, 0));
    }
}

int QQuickDayOfWeekModel::dayAt(int index) const
{
    Q_D(const QQuickDayOfWeekModel);
    int day = d->locale.firstDayOfWeek() + index;
    if (day > 7)
        day -= 7;
    if (day == 7)
        day = 0; // Qt::Sunday = 7, but Sunday is 0 in JS Date
    return day;
}

QVariant QQuickDayOfWeekModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QQuickDayOfWeekModel);
    if (index.isValid() && index.row() < 7) {
        int day = dayAt(index.row());
        switch (role) {
        case DayRole:
            return day;
        case LongNameRole:
            return d->locale.standaloneDayName(day == 0 ? Qt::Sunday : day, QLocale::LongFormat);
        case ShortNameRole:
            return d->locale.standaloneDayName(day == 0 ? Qt::Sunday : day, QLocale::ShortFormat);
        case NarrowNameRole:
            return d->locale.standaloneDayName(day == 0 ? Qt::Sunday : day, QLocale::NarrowFormat);
        default:
            break;
        }
    }
    return QVariant();
}

int QQuickDayOfWeekModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 7;
}

QHash<int, QByteArray> QQuickDayOfWeekModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[DayRole] = QByteArrayLiteral("day");
    roles[LongNameRole] = QByteArrayLiteral("longName");
    roles[ShortNameRole] = QByteArrayLiteral("shortName");
    roles[NarrowNameRole] = QByteArrayLiteral("narrowName");
    return roles;
}

QT_END_NAMESPACE
