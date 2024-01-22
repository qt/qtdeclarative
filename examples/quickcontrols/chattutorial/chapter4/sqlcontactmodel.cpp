// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "sqlcontactmodel.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

static void createTable()
{
    if (QSqlDatabase::database().tables().contains(QStringLiteral("Contacts"))) {
        // The table already exists; we don't need to do anything.
        return;
    }

    QSqlQuery query;
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS 'Contacts' ("
        "   'name' TEXT NOT NULL,"
        "   PRIMARY KEY(name)"
        ")")) {
        qFatal("Failed to query database: %s", qPrintable(query.lastError().text()));
    }

    query.exec("INSERT INTO Contacts VALUES('Albert Einstein')");
    query.exec("INSERT INTO Contacts VALUES('Ernest Hemingway')");
    query.exec("INSERT INTO Contacts VALUES('Hans Gude')");
}

SqlContactModel::SqlContactModel(QObject *parent) :
    QSqlQueryModel(parent)
{
    createTable();

    QSqlQuery query;
    if (!query.exec("SELECT * FROM Contacts"))
        qFatal("Contacts SELECT query failed: %s", qPrintable(query.lastError().text()));

    setQuery(std::move(query));
    if (lastError().isValid())
        qFatal("Cannot set query on SqlContactModel: %s", qPrintable(lastError().text()));
}
