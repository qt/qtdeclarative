// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmonthmodel_p.h"

#include <QtCore/private/qabstractitemmodel_p.h>

namespace {
    static const int daysInAWeek = 7;
    static const int weeksOnACalendarMonth = 6;
    static const int daysOnACalendarMonth = daysInAWeek * weeksOnACalendarMonth;
}

QT_BEGIN_NAMESPACE

class QQuickMonthModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QQuickMonthModel)

public:
    QQuickMonthModelPrivate() : dates(daysOnACalendarMonth)
    {
        today = QDate::currentDate();
        month = today.month();
        year = today.year();
    }

    bool populate(int month, int year, const QLocale &locale, bool force = false);

    int month;
    int year;
    QString title;
    QLocale locale;
    QVector<QDate> dates;
    QDate today;
};

bool QQuickMonthModelPrivate::populate(int m, int y, const QLocale &l, bool force)
{
    Q_Q(QQuickMonthModel);
    if (!force && m == month && y == year && l.firstDayOfWeek() == locale.firstDayOfWeek())
        return false;

    // The actual first (1st) day of the month.
    QDate firstDayOfMonthDate(y, m, 1);
    int difference = ((firstDayOfMonthDate.dayOfWeek() - l.firstDayOfWeek()) + 7) % 7;
    // The first day to display should never be the 1st of the month, as we want some days from
    // the previous month to be visible.
    if (difference == 0)
        difference += 7;
    QDate firstDateToDisplay = firstDayOfMonthDate.addDays(-difference);

    today = QDate::currentDate();
    for (int i = 0; i < daysOnACalendarMonth; ++i)
        dates[i] = firstDateToDisplay.addDays(i);

    q->setTitle(l.standaloneMonthName(m) + QStringLiteral(" ") + QString::number(y));

    return true;
}

QQuickMonthModel::QQuickMonthModel(QObject *parent) :
    QAbstractListModel(*(new QQuickMonthModelPrivate), parent)
{
    Q_D(QQuickMonthModel);
    d->populate(d->month, d->year, d->locale, true);
}

int QQuickMonthModel::month() const
{
    Q_D(const QQuickMonthModel);
    return d->month;
}

void QQuickMonthModel::setMonth(int month)
{
    Q_D(QQuickMonthModel);
    if (d->month != month) {
        if (d->populate(month, d->year, d->locale))
            emit dataChanged(index(0, 0), index(daysOnACalendarMonth - 1, 0));
        d->month = month;
        emit monthChanged();
    }
}

int QQuickMonthModel::year() const
{
    Q_D(const QQuickMonthModel);
    return d->year;
}

void QQuickMonthModel::setYear(int year)
{
    Q_D(QQuickMonthModel);
    if (d->year != year) {
        if (d->populate(d->month, year, d->locale))
            emit dataChanged(index(0, 0), index(daysOnACalendarMonth - 1, 0));
        d->year = year;
        emit yearChanged();
    }
}

QLocale QQuickMonthModel::locale() const
{
    Q_D(const QQuickMonthModel);
    return d->locale;
}

void QQuickMonthModel::setLocale(const QLocale &locale)
{
    Q_D(QQuickMonthModel);
    if (d->locale != locale) {
        if (d->populate(d->month, d->year, locale))
            emit dataChanged(index(0, 0), index(daysOnACalendarMonth - 1, 0));
        d->locale = locale;
        emit localeChanged();
    }
}

QString QQuickMonthModel::title() const
{
    Q_D(const QQuickMonthModel);
    return d->title;
}

void QQuickMonthModel::setTitle(const QString &title)
{
    Q_D(QQuickMonthModel);
    if (d->title != title) {
        d->title = title;
        emit titleChanged();
    }
}

QDate QQuickMonthModel::dateAt(int index) const
{
    Q_D(const QQuickMonthModel);
    return d->dates.value(index);
}

int QQuickMonthModel::indexOf(const QDate &date) const
{
    Q_D(const QQuickMonthModel);
    if (date < d->dates.first() || date > d->dates.last())
        return -1;
    return qMax(qint64(0), d->dates.first().daysTo(date));
}

QVariant QQuickMonthModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QQuickMonthModel);
    if (index.isValid() && index.row() < daysOnACalendarMonth) {
        const QDate date = d->dates.at(index.row());
        switch (role) {
        case DateRole:
            return date;
        case DayRole:
            return date.day();
        case TodayRole:
            return date == d->today;
        case WeekNumberRole:
            return date.weekNumber();
        case MonthRole:
            return date.month() - 1;
        case YearRole:
            return date.year();
        default:
            break;
        }
    }
    return QVariant();
}

int QQuickMonthModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return daysOnACalendarMonth;
}

QHash<int, QByteArray> QQuickMonthModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[DateRole] = QByteArrayLiteral("date");
    roles[DayRole] = QByteArrayLiteral("day");
    roles[TodayRole] = QByteArrayLiteral("today");
    roles[WeekNumberRole] = QByteArrayLiteral("weekNumber");
    roles[MonthRole] = QByteArrayLiteral("month");
    roles[YearRole] = QByteArrayLiteral("year");
    return roles;
}

QT_END_NAMESPACE

#include "moc_qquickmonthmodel_p.cpp"
