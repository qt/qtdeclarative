// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/debugutil_p.h"
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <private/qqmldebugconnection_p.h>
#include <private/qqmlenginedebugclient_p.h>
#include <private/qqmlinspectorclient_p.h>

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtNetwork/qhostaddress.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qthread.h>
#include <QtCore/qlibraryinfo.h>

class tst_QQmlEngineDebugInspectorIntegration : public QQmlDebugTest
{
    Q_OBJECT

public:
    tst_QQmlEngineDebugInspectorIntegration();

private:
    ConnectResult runAndConnect(bool restrictServices);
    QList<QQmlDebugClient *> createClients() override;

    QQmlEngineDebugObjectReference findRootObject();

    QPointer<QQmlInspectorClient> m_inspectorClient;
    QPointer<QQmlEngineDebugClient> m_engineDebugClient;
    QPointer<QQmlInspectorResultRecipient> m_recipient;

private slots:
    void connect_data();
    void connect();
    void objectLocationLookup();
    void select();
    void createObject();
    void moveObject();
    void destroyObject();
};

QQmlEngineDebugObjectReference tst_QQmlEngineDebugInspectorIntegration::findRootObject()
{
    bool success = false;
    m_engineDebugClient->queryAvailableEngines(&success);

    if (!QQmlDebugTest::waitForSignal(m_engineDebugClient, SIGNAL(result())))
        return QQmlEngineDebugObjectReference();

    m_engineDebugClient->queryRootContexts(m_engineDebugClient->engines()[0], &success);
    if (!QQmlDebugTest::waitForSignal(m_engineDebugClient, SIGNAL(result())))
        return QQmlEngineDebugObjectReference();

    int count = m_engineDebugClient->rootContext().contexts.size();
    m_engineDebugClient->queryObject(
                m_engineDebugClient->rootContext().contexts[count - 1].objects[0], &success);
    if (!QQmlDebugTest::waitForSignal(m_engineDebugClient, SIGNAL(result())))
        return QQmlEngineDebugObjectReference();
    return m_engineDebugClient->object();
}

tst_QQmlEngineDebugInspectorIntegration::tst_QQmlEngineDebugInspectorIntegration()
    : QQmlDebugTest(QT_QMLTEST_DATADIR)
{
}

QQmlDebugTest::ConnectResult tst_QQmlEngineDebugInspectorIntegration::runAndConnect(bool restrictServices)
{
    return QQmlDebugTest::connectTo(
                QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/qml",
                restrictServices ? QStringLiteral("QmlDebugger,QmlInspector") : QString(),
                testFile("qtquick2.qml"), true);
}

QList<QQmlDebugClient *> tst_QQmlEngineDebugInspectorIntegration::createClients()
{
    m_inspectorClient = new QQmlInspectorClient(m_connection);
    m_engineDebugClient = new QQmlEngineDebugClient(m_connection);
    m_recipient = new QQmlInspectorResultRecipient(m_inspectorClient);
    QObject::connect(m_inspectorClient.data(), &QQmlInspectorClient::responseReceived,
                     m_recipient.data(), &QQmlInspectorResultRecipient::recordResponse);
    return QList<QQmlDebugClient *>({m_inspectorClient, m_engineDebugClient});
}

void tst_QQmlEngineDebugInspectorIntegration::connect_data()
{
    QTest::addColumn<bool>("restrictMode");
    QTest::newRow("unrestricted") << false;
    QTest::newRow("restricted") << true;
}

void tst_QQmlEngineDebugInspectorIntegration::connect()
{
    QFETCH(bool, restrictMode);
    QCOMPARE(runAndConnect(restrictMode), ConnectSuccess);
}

void tst_QQmlEngineDebugInspectorIntegration::objectLocationLookup()
{
    QCOMPARE(runAndConnect(true), ConnectSuccess);

    bool success = false;
    const QQmlEngineDebugObjectReference rootObject = findRootObject();
    QVERIFY(rootObject.debugId != -1);
    const QString fileName = QFileInfo(rootObject.source.url.toString()).fileName();
    int lineNumber = rootObject.source.lineNumber;
    int columnNumber = rootObject.source.columnNumber;
    m_engineDebugClient->queryObjectsForLocation(fileName, lineNumber,
                                        columnNumber, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_engineDebugClient, SIGNAL(result())));

    for (const QQmlEngineDebugObjectReference &child : rootObject.children) {
        success = false;
        lineNumber = child.source.lineNumber;
        columnNumber = child.source.columnNumber;
        m_engineDebugClient->queryObjectsForLocation(fileName, lineNumber,
                                       columnNumber, &success);
        QVERIFY(success);
        QVERIFY(QQmlDebugTest::waitForSignal(m_engineDebugClient, SIGNAL(result())));
    }
}

void tst_QQmlEngineDebugInspectorIntegration::select()
{
    QCOMPARE(runAndConnect(true), ConnectSuccess);

    const QQmlEngineDebugObjectReference rootObject = findRootObject();
    QList<int> childIds;
    int requestId = 0;
    for (const QQmlEngineDebugObjectReference &child : rootObject.children) {
        requestId = m_inspectorClient->select(QList<int>() << child.debugId);
        QTRY_COMPARE(m_recipient->lastResponseId, requestId);
        QVERIFY(m_recipient->lastResult);
        childIds << child.debugId;
    }
    requestId = m_inspectorClient->select(childIds);
    QTRY_COMPARE(m_recipient->lastResponseId, requestId);
    QVERIFY(m_recipient->lastResult);
}

void tst_QQmlEngineDebugInspectorIntegration::createObject()
{
    QCOMPARE(runAndConnect(true), ConnectSuccess);

    QString qml = QLatin1String("Rectangle {\n"
                                "  id: xxxyxxx\n"
                                "  width: 10\n"
                                "  height: 10\n"
                                "  color: \"blue\"\n"
                                "}");

    QQmlEngineDebugObjectReference rootObject = findRootObject();
    QVERIFY(rootObject.debugId != -1);
    QCOMPARE(rootObject.children.size(), 2);

    int requestId = m_inspectorClient->createObject(
                qml, rootObject.debugId, QStringList() << QLatin1String("import QtQuick 2.0"),
                QLatin1String("testcreate.qml"));
    QTRY_COMPARE(m_recipient->lastResponseId, requestId);
    QVERIFY(m_recipient->lastResult);

    rootObject = findRootObject();
    QVERIFY(rootObject.debugId != -1);
    QCOMPARE(rootObject.children.size(), 3);
    QCOMPARE(rootObject.children[2].idString, QLatin1String("xxxyxxx"));
}

void tst_QQmlEngineDebugInspectorIntegration::moveObject()
{
    QCOMPARE(runAndConnect(true), ConnectSuccess);

    QCOMPARE(m_inspectorClient->state(), QQmlDebugClient::Enabled);
    QQmlEngineDebugObjectReference rootObject = findRootObject();
    QVERIFY(rootObject.debugId != -1);
    QCOMPARE(rootObject.children.size(), 2);

    int childId = rootObject.children[0].debugId;
    int requestId = m_inspectorClient->moveObject(childId, rootObject.children[1].debugId);
    QTRY_COMPARE(m_recipient->lastResponseId, requestId);
    QVERIFY(m_recipient->lastResult);

    rootObject = findRootObject();
    QVERIFY(rootObject.debugId != -1);
    QCOMPARE(rootObject.children.size(), 1);
    bool success = false;
    m_engineDebugClient->queryObject(rootObject.children[0], &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_engineDebugClient, SIGNAL(result())));
    QCOMPARE(m_engineDebugClient->object().children.size(), 1);
    QCOMPARE(m_engineDebugClient->object().children[0].debugId, childId);
}

void tst_QQmlEngineDebugInspectorIntegration::destroyObject()
{
    QCOMPARE(runAndConnect(true), ConnectSuccess);

    QCOMPARE(m_inspectorClient->state(), QQmlDebugClient::Enabled);
    QQmlEngineDebugObjectReference rootObject = findRootObject();
    QVERIFY(rootObject.debugId != -1);
    QCOMPARE(rootObject.children.size(), 2);

    int requestId = m_inspectorClient->destroyObject(rootObject.children[0].debugId);
    QTRY_COMPARE(m_recipient->lastResponseId, requestId);
    QVERIFY(m_recipient->lastResult);

    rootObject = findRootObject();
    QVERIFY(rootObject.debugId != -1);
    QCOMPARE(rootObject.children.size(), 1);
    bool success = false;
    m_engineDebugClient->queryObject(rootObject.children[0], &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_engineDebugClient, SIGNAL(result())));
    QCOMPARE(m_engineDebugClient->object().children.size(), 0);
}

QTEST_MAIN(tst_QQmlEngineDebugInspectorIntegration)

#include "tst_qqmlenginedebuginspectorintegration.moc"
