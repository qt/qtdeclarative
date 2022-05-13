// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickweeknumbermodel_p.h"

#include <QtCore/private/qabstractitemmodel_p.h>
#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE

class QQuickWeekNumberModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QQuickWeekNumberModel)

public:
    QQuickWeekNumberModelPrivate() : month(-1), year(-1), weekNumbers{}
    {
        QDate date = QDate::currentDate();
        init(date.month(), date.year(), locale);
        month = date.month();
        year = date.year();
    }

    void init(int month, int year, const QLocale &locale = QLocale());
    static QDate calculateFirst(int month, int year, const QLocale &locale);

    int month;
    int year;
    QLocale locale;
    int weekNumbers[6];
};

void QQuickWeekNumberModelPrivate::init(int m, int y, const QLocale &l)
{
    Q_Q(QQuickWeekNumberModel);
    if (m == month && y == year && l.firstDayOfWeek() == locale.firstDayOfWeek())
        return;

    // The actual first (1st) day of the month.
    QDate firstDayOfMonthDate(y, m, 1);
    int difference = ((firstDayOfMonthDate.dayOfWeek() - l.firstDayOfWeek()) + 7) % 7;
    // The first day to display should never be the 1st of the month, as we want some days from
    // the previous month to be visible.
    if (difference == 0)
        difference += 7;

    for (int i = 0; i < 6; ++i)
        weekNumbers[i] = firstDayOfMonthDate.addDays(i * 7 - difference).weekNumber();

    if (q) // null at construction
        emit q->dataChanged(q->index(0, 0), q->index(5, 0));
}

QQuickWeekNumberModel::QQuickWeekNumberModel(QObject *parent) :
    QAbstractListModel(*(new QQuickWeekNumberModelPrivate), parent)
{
}

int QQuickWeekNumberModel::month() const
{
    Q_D(const QQuickWeekNumberModel);
    return d->month;
}

void QQuickWeekNumberModel::setMonth(int month)
{
    Q_D(QQuickWeekNumberModel);
    if (d->month != month) {
        d->init(month, d->year, d->locale);
        d->month = month;
        emit monthChanged();
    }
}

int QQuickWeekNumberModel::year() const
{
    Q_D(const QQuickWeekNumberModel);
    return d->year;
}

void QQuickWeekNumberModel::setYear(int year)
{
    Q_D(QQuickWeekNumberModel);
    if (d->year != year) {
        d->init(d->month, year, d->locale);
        d->year = year;
        emit yearChanged();
    }
}

QLocale QQuickWeekNumberModel::locale() const
{
    Q_D(const QQuickWeekNumberModel);
    return d->locale;
}

void QQuickWeekNumberModel::setLocale(const QLocale &locale)
{
    Q_D(QQuickWeekNumberModel);
    if (d->locale != locale) {
        d->init(d->month, d->year, locale);
        d->locale = locale;
        emit localeChanged();
    }
}

int QQuickWeekNumberModel::weekNumberAt(int index) const
{
    Q_D(const QQuickWeekNumberModel);
    if (index < 0 || index > 5)
        return -1;
    return d->weekNumbers[index];
}

int QQuickWeekNumberModel::indexOf(int weekNumber) const
{
    Q_D(const QQuickWeekNumberModel);
    if (weekNumber < d->weekNumbers[0] || weekNumber > d->weekNumbers[5])
        return -1;
    return weekNumber - d->weekNumbers[0];
}

QVariant QQuickWeekNumberModel::data(const QModelIndex &index, int role) const
{
    if (role == WeekNumberRole) {
        int weekNumber = weekNumberAt(index.row());
        if (weekNumber != -1)
            return weekNumber;
    }
    return QVariant();
}

int QQuickWeekNumberModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 6;
}

QHash<int, QByteArray> QQuickWeekNumberModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[WeekNumberRole] = QByteArrayLiteral("weekNumber");
    return roles;
}

QT_END_NAMESPACE

#include "moc_qquickweeknumbermodel_p.cpp"
