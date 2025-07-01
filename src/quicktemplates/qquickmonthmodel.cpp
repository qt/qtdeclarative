// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmonthmodel_p.h"

#include <QtCore/private/qabstractitemmodel_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qtimezone.h>

namespace {
    static const int daysInAWeek = 7;
    static const int weeksOnACalendarMonth = 6;
    static const int daysOnACalendarMonth = daysInAWeek * weeksOnACalendarMonth;
}

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcMonthModel, "qt.quick.controls.monthmodel")

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
    QVector<QDateTime> dates;
    QDate today;
};

bool QQuickMonthModelPrivate::populate(int m, int y, const QLocale &l, bool force)
{
    Q_Q(QQuickMonthModel);
    if (!force && m == month && y == year && l.firstDayOfWeek() == locale.firstDayOfWeek())
        return false;

    // The actual first (1st) day of the month.
    const QDate firstDayOfMonthDate = QDate(y, m, 1);
    // QDate is converted to local time when converted to a JavaScript Date,
    // so if we stored our dates as QDates, it's possible that the date provided
    // to delegates will be wrong in certain timezones:
    // e.g. 00:00 UTC converted to UTC-8 is 16:00 the day before.
    // To account for this, we store our dates as local QDateTimes.
    const QDateTime firstDayOfMonthDateTime = firstDayOfMonthDate.startOfDay();
    int difference = ((firstDayOfMonthDate.dayOfWeek() - l.firstDayOfWeek()) + 7) % 7;
    // The first day to display should never be the 1st of the month, as we want some days from
    // the previous month to be visible.
    if (difference == 0)
        difference += 7;
    const QDateTime firstDateToDisplay = firstDayOfMonthDateTime.addDays(-difference);

    today = QDate::currentDate();
    for (int i = 0; i < daysOnACalendarMonth; ++i)
        dates[i] = firstDateToDisplay.addDays(i);

    q->setTitle(l.standaloneMonthName(m) + QStringLiteral(" ") + QString::number(y));

    qCDebug(lcMonthModel) << "populated model for month" << m << "year" << y << "locale" << locale
        << "firstDayOfMonthDateTime" << firstDayOfMonthDateTime;

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

QDateTime QQuickMonthModel::dateAt(int index) const
{
    Q_D(const QQuickMonthModel);
    return d->dates.value(index);
}

int QQuickMonthModel::indexOf(QDateTime date) const
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
        const QDateTime dateTime = d->dates.at(index.row());
        // As mentioned in populate, we store dates whose time is adjusted
        // by the timezone offset, so we need to convert back to local time
        // to get the correct date if the conversion to JavaScript's Date
        // isn't being done for us.
        const QDate date = d->dates.at(index.row()).toLocalTime().date();
        switch (role) {
        case DateRole:
            return dateTime;
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
