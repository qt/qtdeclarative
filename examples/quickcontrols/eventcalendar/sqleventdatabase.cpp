// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "sqleventdatabase.h"

#include <QDebug>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

SqlEventDatabase::SqlEventDatabase()
{
    createConnection();
}

QVector<Event> SqlEventDatabase::eventsForDate(const QDate &date)
{
    const QString queryStr = QString::fromLatin1("SELECT * FROM Event WHERE '%1' >= startDate AND '%1' <= endDate").arg(date.toString("yyyy-MM-dd"));
    QSqlQuery query(queryStr);
    if (!query.exec()) {
        qWarning() << "SQL query failed";
        return {};
    }

    QVector<Event> events;
    while (query.next()) {
        Event event;
        event.name = query.value("name").toString();

        QDateTime startDate;
        startDate.setDate(query.value("startDate").toDate());
        startDate.setTime(QTime(0, 0).addSecs(query.value("startTime").toInt()));
        event.startDate = startDate;

        QDateTime endDate;
        endDate.setDate(query.value("endDate").toDate());
        endDate.setTime(QTime(0, 0).addSecs(query.value("endTime").toInt()));
        event.endDate = endDate;

        events.append(event);
    }
    return events;
}

/*
    Defines a helper function to open a connection to an
    in-memory SQLITE database and to create a table.
*/
void SqlEventDatabase::createConnection()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    if (!db.open()) {
        qFatal("Cannot open database");
        return;
    }

    QSqlQuery query;
    const QString year = QDate::currentDate().toString("yyyy");
    const QString month = QDate::currentDate().toString("MM");
    // Keep the example up-to-date by making the events fall in the current year and month.
    // We store the time as seconds because it's easier to query.
    query.exec("create table Event (name TEXT, startDate DATE, startTime INT, endDate DATE, endTime INT)");
    query.exec(QString::fromLatin1("insert into Event values('Grocery shopping', '%1-%2-01', 36000, '%1-%2-01', 39600)").arg(year, month));
    query.exec(QString::fromLatin1("insert into Event values('Ice skating', '%1-%2-01', 57600, '%1-%2-01', 61200)").arg(year, month));
    query.exec(QString::fromLatin1("insert into Event values('Doctor''s appointment', '%1-%2-15', 57600, '%1-%2-15', 63000)").arg(year, month));
    query.exec(QString::fromLatin1("insert into Event values('Conference', '%1-%2-24', 32400, '%1-%2-28', 61200)").arg(year, month));

    return;
}
