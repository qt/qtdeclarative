// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "debugutil_p.h"
#include "qqmldebugprocess_p.h"
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <private/qqmldebugclient_p.h>
#include <private/qqmldebugconnection_p.h>

#include <QtQml/qqmldebug.h>

#include <QtTest/qtest.h>
#include <QtCore/qprocess.h>
#include <QtCore/qtimer.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qmutex.h>
#include <QtCore/qlibraryinfo.h>

class tst_QQmlDebuggingEnabler : public QQmlDebugTest
{
    Q_OBJECT

public:
    tst_QQmlDebuggingEnabler();

private slots:
    void qmlscene_data();
    void qmlscene();
    void custom_data();
    void custom();

private:
    void data();
};

void tst_QQmlDebuggingEnabler::data()
{
    QTest::addColumn<QString>("connector");
    QTest::addColumn<bool>("blockMode");
    QTest::addColumn<QStringList>("services");

    const QStringList connectors({
        QLatin1String("QQmlDebugServer"),
        QLatin1String("QQmlNativeDebugConnector")
    });

    const QList<bool> blockModes({ true, false });

    const QList<QStringList> serviceLists({
        QStringList(),
        QQmlDebuggingEnabler::nativeDebuggerServices(),
        QQmlDebuggingEnabler::debuggerServices(),
        QQmlDebuggingEnabler::inspectorServices(),
        QQmlDebuggingEnabler::profilerServices(),
        QQmlDebuggingEnabler::debuggerServices() + QQmlDebuggingEnabler::inspectorServices()
    });

    for (const QString &connector : connectors) {
        for (bool blockMode : blockModes) {
            for (const QStringList &serviceList : serviceLists) {
                QString name = connector + QLatin1Char(',')
                        + QLatin1String(blockMode ? "block" : "noblock") + QLatin1Char(',')
                        + serviceList.join(QLatin1Char('-'));
                QTest::newRow(name.toUtf8().constData()) << connector << blockMode << serviceList;
            }
        }
    }
}

tst_QQmlDebuggingEnabler::tst_QQmlDebuggingEnabler()
    : QQmlDebugTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQmlDebuggingEnabler::qmlscene_data()
{
    data();
}

void tst_QQmlDebuggingEnabler::qmlscene()
{
    QFETCH(QString, connector);
    QFETCH(bool, blockMode);
    QFETCH(QStringList, services);

    m_process = new QQmlDebugProcess(
                QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/qmlscene", this);
    m_process->setMaximumBindErrors(1);
    m_process->start(QStringList()
                     << QString::fromLatin1("-qmljsdebugger=connector:%1%2%3%4")
                        .arg(connector + (connector == QLatin1String("QQmlDebugServer")
                                          ? QLatin1String(",port:5555,5565") : QString()))
                        .arg(blockMode ? QLatin1String(",block") : QString())
                        .arg(services.isEmpty() ? QString() : QString::fromLatin1(",services:"))
                        .arg(services.isEmpty() ? QString() : services.join(","))
                     << testFile(QLatin1String("test.qml")));

    if (connector == QLatin1String("QQmlDebugServer")) {
        QVERIFY(m_process->waitForSessionStart());
        m_connection = new QQmlDebugConnection();
        m_clients = QQmlDebugTest::createOtherClients(m_connection);
        m_connection->connectToHost("127.0.0.1", m_process->debugPort());
        QVERIFY(m_connection->waitForConnected());
        for (QQmlDebugClient *client : std::as_const(m_clients))
            QCOMPARE(client->state(), (services.isEmpty() || services.contains(client->name())) ?
                         QQmlDebugClient::Enabled : QQmlDebugClient::Unavailable);
    }

    QCOMPARE(m_process->state(), QProcess::Running);
    if (!blockMode) {
        QTRY_VERIFY_WITH_TIMEOUT(m_process->output().contains(
                                     QLatin1String("Component.onCompleted")), 15000);
    }
}

void tst_QQmlDebuggingEnabler::custom_data()
{
    data();
}

void tst_QQmlDebuggingEnabler::custom()
{
    QFETCH(QString, connector);
    QFETCH(bool, blockMode);
    QFETCH(QStringList, services);
    const int portFrom = 5555;
    const int portTo = 5565;

    m_process = new QQmlDebugProcess(QCoreApplication::applicationDirPath() +
                                     QLatin1String("/qqmldebuggingenablerserver"), this);
    m_process->setMaximumBindErrors(portTo - portFrom);

    QStringList args;
    if (blockMode)
        args << QLatin1String("-block");

    args << QLatin1String("-connector") << connector
         << QString::number(portFrom) << QString::number(portTo);

    if (!services.isEmpty())
        args << QLatin1String("-services") << services;

    m_process->start(args);

    if (connector == QLatin1String("QQmlDebugServer")) {
        QVERIFY(m_process->waitForSessionStart());
        m_connection = new QQmlDebugConnection();
        m_clients = QQmlDebugTest::createOtherClients(m_connection);
        m_connection->connectToHost("127.0.0.1", m_process->debugPort());
        QVERIFY(m_connection->waitForConnected());
        for (QQmlDebugClient *client : std::as_const(m_clients))
            QCOMPARE(client->state(), (services.isEmpty() || services.contains(client->name())) ?
                         QQmlDebugClient::Enabled : QQmlDebugClient::Unavailable);
    }

    QCOMPARE(m_process->state(), QProcess::Running);
    if (!blockMode) {
        QTRY_VERIFY_WITH_TIMEOUT(m_process->output().contains(QLatin1String("QQmlEngine created")),
                                 15000);
    }
}

QTEST_MAIN(tst_QQmlDebuggingEnabler)

#include "tst_qqmldebuggingenabler.moc"

