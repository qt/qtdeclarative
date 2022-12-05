// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include "moc_qquickdayofweekmodel_p.cpp"
