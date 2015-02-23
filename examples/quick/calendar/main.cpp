/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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

#include <QtGui>
#include <QtQml>
#include <QtSql>

class Event : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER name NOTIFY nameChanged)
    Q_PROPERTY(QDateTime start MEMBER start NOTIFY startChanged)
    Q_PROPERTY(QDateTime end MEMBER end NOTIFY endChanged)

public:
    explicit Event(const QString &name, QObject *parent = 0) : QObject(parent), name(name) { }

    QString name;
    QDateTime start;
    QDateTime end;

signals:
    void nameChanged();
    void startChanged();
    void endChanged();
};

class SqlEventModel : public QSqlQueryModel
{
    Q_OBJECT
    Q_PROPERTY(QDate min READ min CONSTANT)
    Q_PROPERTY(QDate max READ max CONSTANT)

public:
    SqlEventModel(QObject *parent = 0) : QSqlQueryModel(parent) { }

    QDate min() const
    {
        QSqlQuery query(QStringLiteral("SELECT MIN(start) FROM Event"));
        if (query.next())
            return QDateTime::fromMSecsSinceEpoch(query.value(0).toLongLong()).date();
        return QDate();
    }

    QDate max() const
    {
        QSqlQuery query(QStringLiteral("SELECT MAX(end) FROM Event"));
        if (query.next())
            return QDateTime::fromMSecsSinceEpoch(query.value(0).toLongLong()).date();
        return QDate();
    }

    Q_INVOKABLE QList<QObject*> eventsForDate(const QDate &date)
    {
        qint64 from = QDateTime(date, QTime(0, 0)).toMSecsSinceEpoch();
        qint64 to = QDateTime(date, QTime(23, 59)).toMSecsSinceEpoch();

        QSqlQuery query;
        if (!query.exec(QStringLiteral("SELECT * FROM Event WHERE start <= %1 AND end >= %2").arg(to).arg(from)))
            qFatal("Query failed");

        QList<QObject*> events;
        while (query.next()) {
            Event *event = new Event(query.value("name").toString(), this);
            event->start = QDateTime::fromMSecsSinceEpoch(query.value("start").toLongLong());
            event->end = QDateTime::fromMSecsSinceEpoch(query.value("end").toLongLong());
            events.append(event);
        }
        return events;
    }
};

// create an in-memory SQLITE database
static bool addEvent(QSqlQuery* query, const QString &name, const QDateTime &start, qint64 duration = 0)
{
    QDateTime end = start.addSecs(duration);
    return query->exec(QStringLiteral("insert into Event values('%1', %2, %3)").arg(name)
                                                                               .arg(start.toMSecsSinceEpoch())
                                                                               .arg(end.toMSecsSinceEpoch()));
}

static void createDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    if (!db.open()) {
        qFatal("Cannot open database");
        return;
    }

    QSqlQuery query;
    query.exec("create table if not exists Event (name TEXT, start BIGINT, end BIGINT)");

    const QDate current = QDate::currentDate();
    addEvent(&query, "Job interview", QDateTime(current.addDays(-19), QTime(12, 0)));
    addEvent(&query, "Grocery shopping", QDateTime(current.addDays(-14), QTime(18, 0)));
    addEvent(&query, "Ice skating", QDateTime(current.addDays(-14), QTime(20, 0)), 5400);
    addEvent(&query, "Dentist's appointment", QDateTime(current.addDays(-8), QTime(14, 0)), 1800);
    addEvent(&query, "Cross-country skiing", QDateTime(current.addDays(1), QTime(19, 30)), 3600);
    addEvent(&query, "Conference", QDateTime(current.addDays(10), QTime(9, 0)), 432000);
    addEvent(&query, "Hairdresser", QDateTime(current.addDays(19), QTime(13, 0)));
    addEvent(&query, "Doctor's appointment", QDateTime(current.addDays(21), QTime(16, 0)));
    addEvent(&query, "Vacation", QDateTime(current.addDays(35), QTime(0, 0)), 604800);
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    createDatabase();
    qmlRegisterType<SqlEventModel>("io.qt.examples.calendar", 1, 0, "SqlEventModel");
    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    return app.exec();
}

#include "main.moc"
