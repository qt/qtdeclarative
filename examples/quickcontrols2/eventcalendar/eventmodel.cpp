/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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
