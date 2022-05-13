// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "eventmodel.h"

#include "sqleventdatabase.h"

EventModel::EventModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

SqlEventDatabase *EventModel::eventDatabase()
{
    return m_eventDatabase;
}

void EventModel::setEventDatabase(SqlEventDatabase *eventDatabase)
{
    if (eventDatabase == m_eventDatabase)
        return;

    m_eventDatabase = eventDatabase;
    repopulate();
    emit eventDatabaseChanged();
}

QDate EventModel::date() const
{
    return m_date;
}

void EventModel::setDate(const QDate &date)
{
    if (date == m_date)
        return;

    m_date = date;
    repopulate();
    emit dateChanged();
}

int EventModel::rowCount(const QModelIndex &) const
{
    return m_events.size();
}

QVariant EventModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return QVariant();

    switch (role) {
    case NameRole: return m_events.at(index.row()).name;
    case StartDateRole: return m_events.at(index.row()).startDate;
    case EndDateRole: return m_events.at(index.row()).endDate;
    default: return QVariant();
    }
}

QHash<int, QByteArray> EventModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        { NameRole, "name" },
        { StartDateRole, "startDate" },
        { EndDateRole, "endDate" }
    };
    return roles;
}

bool EventModel::isValid() const
{
    return m_eventDatabase && !m_date.isNull();
}

void EventModel::repopulate()
{
    beginResetModel();

    if (!m_eventDatabase || m_date.isNull()) {
        m_events.clear();
        return;
    }

    m_events = m_eventDatabase->eventsForDate(m_date);

    endResetModel();
}
