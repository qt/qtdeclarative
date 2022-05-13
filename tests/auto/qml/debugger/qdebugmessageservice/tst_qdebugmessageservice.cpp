// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//QQmlDebugTest
#include "../shared/debugutil_p.h"

#include <private/qqmldebugclient_p.h>
#include <private/qqmldebugconnection_p.h>
#include <private/qpacket_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qlibraryinfo.h>
#include <QtTest/qtest.h>

const char *QMLFILE = "test.qml";

class QQmlDebugMsgClient;
class tst_QDebugMessageService : public QQmlDebugTest
{
    Q_OBJECT

public:
    tst_QDebugMessageService();

private slots:
    void retrieveDebugOutput();

private:
    QList<QQmlDebugClient *> createClients() override;
    QPointer<QQmlDebugMsgClient> m_client;
};

struct LogEntry {
    LogEntry(QtMsgType _type, QString _message)
        : type(_type), message(_message) {}

    QtMsgType type;
    QString message;
    int line;
    QString file;
    QString function;
    QString category;

    QString toString() const
    {
        return QString::number(type) + ": " + message + " (" + category + ")";
    }
};

bool operator==(const LogEntry &t1, const LogEntry &t2)
{
    return t1.type == t2.type && t1.message == t2.message
            && t1.line == t2.line && t1.file == t2.file
            && t1.function == t2.function && t1.category == t2.category;
}

class QQmlDebugMsgClient : public QQmlDebugClient
{
    Q_OBJECT
public:
    QQmlDebugMsgClient(QQmlDebugConnection *connection)
        : QQmlDebugClient(QLatin1String("DebugMessages"), connection)
    {
    }

    QList<LogEntry> logBuffer;

protected:
    void messageReceived(const QByteArray &data) override;

signals:
    void debugOutput();
};

void QQmlDebugMsgClient::messageReceived(const QByteArray &data)
{
    QPacket ds(connection()->currentDataStreamVersion(), data);
    QByteArray command;
    ds >> command;

    if (command == "MESSAGE") {
        int type;
        QByteArray message;
        QByteArray file;
        QByteArray function;
        QByteArray category;
        qint64 timestamp;
        int line;
        ds >> type >> message >> file >> line >> function >> category >> timestamp;
        QVERIFY(ds.atEnd());

        QVERIFY(type >= QtDebugMsg);
        QVERIFY(type <= QtInfoMsg);
        QVERIFY(timestamp > 0);

        LogEntry entry((QtMsgType)type, QString::fromUtf8(message));
        entry.line = line;
        entry.file = QString::fromUtf8(file);
        entry.function = QString::fromUtf8(function);
        entry.category = QString::fromUtf8(category);
        logBuffer << entry;
        emit debugOutput();
    } else {
        QFAIL("Unknown message");
    }
}

QList<QQmlDebugClient *> tst_QDebugMessageService::createClients()
{
    m_client = new QQmlDebugMsgClient(m_connection);
    return QList<QQmlDebugClient *>({m_client});
}

tst_QDebugMessageService::tst_QDebugMessageService()
    : QQmlDebugTest(QT_QMLTEST_DATADIR)
{
}

void tst_QDebugMessageService::retrieveDebugOutput()
{
    QCOMPARE(QQmlDebugTest::connectTo(QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/qml",
                                    QString(), testFile(QMLFILE), true), ConnectSuccess);

    QTRY_VERIFY(m_client->logBuffer.size() >= 2);

    const QString path =
            QUrl::fromLocalFile(QQmlDataTest::instance()->testFile(QMLFILE)).toString();
    LogEntry entry1(QtDebugMsg, QLatin1String("console.log"));
    entry1.line = 10;
    entry1.file = path;
    entry1.function = QLatin1String("expression for onCompleted");
    entry1.category = QLatin1String("qml");
    LogEntry entry2(QtDebugMsg, QLatin1String("console.count: 1"));
    entry2.line = 11;
    entry2.file = path;
    entry2.function = QLatin1String("expression for onCompleted");
    entry2.category = QLatin1String("default");

    QVERIFY(m_client->logBuffer.contains(entry1));
    QVERIFY(m_client->logBuffer.contains(entry2));
}

QTEST_MAIN(tst_QDebugMessageService)

#include "tst_qdebugmessageservice.moc"
