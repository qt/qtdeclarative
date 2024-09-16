// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "debugutil_p.h"
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <private/qqmldebugclient_p.h>
#include <private/qqmldebugconnection_p.h>
#include <private/qpacket_p.h>
#include <private/qqmlenginecontrolclient_p.h>

#include <QtTest/qtest.h>
#include <QtCore/qlibraryinfo.h>

class QQmlEngineBlocker : public QObject
{
    Q_OBJECT
public:
    QQmlEngineBlocker(QQmlEngineControlClient *parent);

public slots:
    void blockEngine(int engineId, const QString &name);
};

QQmlEngineBlocker::QQmlEngineBlocker(QQmlEngineControlClient *parent)
     : QObject(parent)
{
    connect(parent, &QQmlEngineControlClient::engineAboutToBeAdded,
            this, &QQmlEngineBlocker::blockEngine);
    connect(parent, &QQmlEngineControlClient::engineAboutToBeRemoved,
            this, &QQmlEngineBlocker::blockEngine);
}

void QQmlEngineBlocker::blockEngine(int engineId, const QString &name)
{
    Q_UNUSED(name);
    static_cast<QQmlEngineControlClient *>(parent())->blockEngine(engineId);
}

class tst_QQmlEngineControl : public QQmlDebugTest
{
    Q_OBJECT

public:
    tst_QQmlEngineControl();

private:
    ConnectResult connectTo(const QString &testFile, bool restrictServices);
    QList<QQmlDebugClient *> createClients() override;

    void engine_data();
    QPointer<QQmlEngineControlClient> m_client;

private slots:
    void startEngine_data();
    void startEngine();
    void stopEngine_data();
    void stopEngine();
};

tst_QQmlEngineControl::tst_QQmlEngineControl()
    : QQmlDebugTest(QT_QMLTEST_DATADIR)
{
}

QQmlDebugTest::ConnectResult tst_QQmlEngineControl::connectTo(const QString &file,
                                                            bool restrictServices)
{
    return QQmlDebugTest::connectTo(QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/qmlscene",
                                  restrictServices ? QStringLiteral("EngineControl") : QString(),
                                  testFile(file), true);
}

QList<QQmlDebugClient *> tst_QQmlEngineControl::createClients()
{
    m_client = new QQmlEngineControlClient(m_connection);
    new QQmlEngineBlocker(m_client);
    return QList<QQmlDebugClient *>({m_client});
}

void tst_QQmlEngineControl::engine_data()
{
    QTest::addColumn<bool>("restrictMode");
    QTest::newRow("unrestricted") << false;
    QTest::newRow("restricted") << true;
}

void tst_QQmlEngineControl::startEngine_data()
{
    engine_data();
}

void tst_QQmlEngineControl::startEngine()
{
    QFETCH(bool, restrictMode);
    QCOMPARE(connectTo("test.qml", restrictMode), ConnectSuccess);

    QTRY_VERIFY(!m_client->blockedEngines().empty());
    m_client->releaseEngine(m_client->blockedEngines().last());
    QVERIFY(m_client->blockedEngines().isEmpty());

    QVERIFY2(QQmlDebugTest::waitForSignal(m_client, SIGNAL(engineAdded(int,QString))),
             "No engine start message received in time.");

    QVERIFY(m_client->blockedEngines().isEmpty());
}

void tst_QQmlEngineControl::stopEngine_data()
{
    engine_data();
}

void tst_QQmlEngineControl::stopEngine()
{
    QFETCH(bool, restrictMode);

    QCOMPARE(connectTo("exit.qml", restrictMode), ConnectSuccess);

    QTRY_VERIFY(!m_client->blockedEngines().empty());
    m_client->releaseEngine(m_client->blockedEngines().last());
    QVERIFY(m_client->blockedEngines().isEmpty());

    QVERIFY2(QQmlDebugTest::waitForSignal(m_client, SIGNAL(engineAdded(int,QString))),
             "No engine start message received in time.");
    QVERIFY(m_client->blockedEngines().isEmpty());

    QVERIFY2(QQmlDebugTest::waitForSignal(m_client, SIGNAL(engineAboutToBeRemoved(int,QString))),
             "No engine about to stop message received in time.");
    m_client->releaseEngine(m_client->blockedEngines().last());
    QVERIFY(m_client->blockedEngines().isEmpty());
    QVERIFY2(QQmlDebugTest::waitForSignal(m_client, SIGNAL(engineRemoved(int,QString))),
             "No engine stop message received in time.");
    QVERIFY(m_client->blockedEngines().isEmpty());
}

QTEST_MAIN(tst_QQmlEngineControl)

#include "tst_qqmlenginecontrol.moc"
