/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
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
