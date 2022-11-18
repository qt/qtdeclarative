// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SQLEVENTDATABASE_H
#define SQLEVENTDATABASE_H

#include <QObject>
#include <QtQml>
#include <QVector>

#include "event.h"

class SqlEventDatabase : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(EventDatabase)
    QML_UNCREATABLE("EventDatabase should not be created in QML")

public:
    SqlEventDatabase();

    QVector<Event> eventsForDate(const QDate &date);

private:
    static void createConnection();
};

#endif // SQLEVENTDATABASE_H
