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
    Q_PROPERTY(QString description MEMBER description NOTIFY descriptionChanged)
    Q_PROPERTY(QDateTime start MEMBER start NOTIFY startChanged)
    Q_PROPERTY(QDateTime end MEMBER end NOTIFY endChanged)

public:
    explicit Event(const QString &description, QObject *parent = 0) :
        QObject(parent),
        description(description)
    {

    }

    QString description;
    QDateTime start;
    QDateTime end;

signals:
    void descriptionChanged();
    void startChanged();
    void endChanged();
};

static void addEvent(const QString &description, const QDateTime &start, qint64 duration = 0)
{
    QSqlQuery query;
    QDateTime end = start.addSecs(duration);
    if (!query.exec(QStringLiteral("INSERT INTO Event (description, start, end) VALUES ('%1', %2, %3)")
        .arg(description).arg(start.toMSecsSinceEpoch()).arg(end.toMSecsSinceEpoch()))) {
        qWarning() << query.lastError();
    }
}

// create an in-memory SQLITE database
static void createDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    if (!db.open()) {
        qFatal("Cannot open database");
        return;
    }

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS Event (description TEXT, start BIGINT, end BIGINT)");

    const QDate current = QDate::currentDate();
    addEvent("Job interview", QDateTime(current.addDays(-19), QTime(12, 0)));
    addEvent("Grocery shopping", QDateTime(current.addDays(-14), QTime(18, 0)));
    addEvent("Ice skating", QDateTime(current.addDays(-14), QTime(20, 0)), 5400);
    addEvent("Dentist''s appointment", QDateTime(current.addDays(-8), QTime(14, 0)), 1800);
    addEvent("Cross-country skiing", QDateTime(current.addDays(1), QTime(19, 30)), 3600);
    addEvent("Conference", QDateTime(current.addDays(10), QTime(9, 0)), 432000);
    addEvent("Hairdresser", QDateTime(current.addDays(19), QTime(13, 0)));
    addEvent("Doctor''s appointment", QDateTime(current.addDays(21), QTime(16, 0)));
    addEvent("Vacation", QDateTime(current.addDays(35), QTime(0, 0)), 604800);
}

class SqlEventModel : public QSqlTableModel
{
    Q_OBJECT
    Q_PROPERTY(QDate min READ min CONSTANT)
    Q_PROPERTY(QDate max READ max CONSTANT)
    Q_PROPERTY(QDate date READ date WRITE setDate NOTIFY dateChanged FINAL)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)

public:
    SqlEventModel(QObject *parent = 0) :
        QSqlTableModel(parent, QSqlDatabase::database(":memory:"))
    {
        connect(this, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SIGNAL(rowCountChanged()));
        connect(this, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SIGNAL(rowCountChanged()));

        setTable("Event");
        setEditStrategy(QSqlTableModel::OnManualSubmit);
        select();

        setDate(QDate::currentDate());
    }

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

    QDate date() const
    {
        return mDate;
    }

    void setDate(const QDate &date)
    {
        if (date != mDate) {
            mDate = date;

            qint64 from = QDateTime(mDate, QTime(0, 0)).toMSecsSinceEpoch();
            qint64 to = QDateTime(mDate, QTime(23, 59)).toMSecsSinceEpoch();

            setFilter(QStringLiteral("start <= %1 AND end >= %2").arg(to).arg(from));

            emit dateChanged();
        }
    }

    enum {
        DescriptionRole = Qt::UserRole,
        StartDateRole,
        EndDateRole
    };

    QHash<int,QByteArray> roleNames() const Q_DECL_OVERRIDE
    {
        QHash<int,QByteArray> names;
        names[DescriptionRole] = "description";
        names[StartDateRole] = "start";
        names[EndDateRole] = "end";
        return names;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE
    {
        if (role < Qt::UserRole)
            return QSqlTableModel::data(index, role);

        int columnIndex = role - DescriptionRole;
        QModelIndex modelIndex = this->index(index.row(), columnIndex);
        QVariant eventData = QSqlTableModel::data(modelIndex, Qt::DisplayRole);
        if (role == DescriptionRole)
            return eventData;

        return QDateTime::fromMSecsSinceEpoch(eventData.toLongLong());
    }

    Q_INVOKABLE void addEvent(const QString &description, const QDateTime &date)
    {
        const int row = rowCount();
        insertRows(row, 1);
        setData(index(row, 0), description);
        setData(index(row, 1), date.toMSecsSinceEpoch());
        setData(index(row, 2), date.toMSecsSinceEpoch());
        submitAll();
    }

    Q_INVOKABLE void removeEvent(int modelRow)
    {
        if (modelRow < 0 || modelRow >= rowCount()) {
            qWarning() << "Invalid model row:" << modelRow;
            return;
        }

        removeRows(modelRow, 1);
        submitAll();
    }

signals:
    void dateChanged();
    void rowCountChanged();

private:
    // The date to show events for.
    QDate mDate;
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    createDatabase();
    qmlRegisterType<SqlEventModel>("io.qt.examples.calendar", 1, 0, "SqlEventModel");
    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}

#include "main.moc"
