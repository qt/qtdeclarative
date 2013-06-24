/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QLibraryInfo>

#include "debugutil_p.h"
#include "qqmldebugclient.h"
#include "../../../shared/util.h"

#define STR_PORT_FROM "13773"
#define STR_PORT_TO "13783"

struct QQmlProfilerData
{
    qint64 time;
    int messageType;
    int detailType;

    //###
    QString detailData; //used by RangeData and RangeLocation
    int line;           //used by RangeLocation
    int column;         //used by RangeLocation
    int framerate;      //used by animation events
    int animationcount; //used by animation events

    QByteArray toByteArray() const;
};

class QQmlProfilerClient : public QQmlDebugClient
{
    Q_OBJECT

public:
    enum Message {
        Event,
        RangeStart,
        RangeData,
        RangeLocation,
        RangeEnd,
        Complete, // end of transmission
        PixmapCacheEvent,
        SceneGraphFrame,

        MaximumMessage
    };

    enum EventType {
        FramePaint,
        Mouse,
        Key,
        AnimationFrame,
        EndTrace,
        StartTrace,

        MaximumEventType
    };

    enum RangeType {
        Painting,
        Compiling,
        Creating,
        Binding,            //running a binding
        HandlingSignal,     //running a signal handler

        MaximumRangeType
    };

    enum PixmapEventType {
        PixmapSizeKnown,
        PixmapReferenceCountChanged,
        PixmapCacheCountChanged,
        PixmapLoadingStarted,
        PixmapLoadingFinished,
        PixmapLoadingError,

        MaximumPixmapEventType
    };

    enum SceneGraphFrameType {
        SceneGraphRendererFrame,
        SceneGraphAdaptationLayerFrame,
        SceneGraphContextFrame,
        SceneGraphRenderLoopFrame,
        SceneGraphTexturePrepare,
        SceneGraphTextureDeletion,
        SceneGraphPolishAndSync,
        SceneGraphWindowsRenderShow,
        SceneGraphWindowsAnimations,
        SceneGraphWindowsPolishFrame,

        MaximumSceneGraphFrameType
    };

    QQmlProfilerClient(QQmlDebugConnection *connection)
        : QQmlDebugClient(QLatin1String("CanvasFrameRate"), connection)
    {
    }

    QList<QQmlProfilerData> traceMessages;

    void setTraceState(bool enabled) {
        QByteArray message;
        QDataStream stream(&message, QIODevice::WriteOnly);
        stream << enabled;
        sendMessage(message);
    }

signals:
    void complete();

protected:
    void messageReceived(const QByteArray &message);
};

class tst_QQmlProfilerService : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQmlProfilerService()
        : m_process(0)
        , m_connection(0)
        , m_client(0)
    {
    }


private:
    QQmlDebugProcess *m_process;
    QQmlDebugConnection *m_connection;
    QQmlProfilerClient *m_client;

    void connect(bool block, const QString &testFile);

private slots:
    void cleanup();

    void blockingConnectWithTraceEnabled();
    void blockingConnectWithTraceDisabled();
    void nonBlockingConnect();
    void pixmapCacheData();
    void scenegraphData();
    void profileOnExit();
};

void QQmlProfilerClient::messageReceived(const QByteArray &message)
{
    QByteArray msg = message;
    QDataStream stream(&msg, QIODevice::ReadOnly);


    QQmlProfilerData data;
    data.time = -2;
    data.messageType = -1;
    data.detailType = -1;
    data.line = -1;
    data.framerate = -1;
    data.animationcount = -1;

    stream >> data.time >> data.messageType;

    QVERIFY(data.time >= -1);

    switch (data.messageType) {
    case (QQmlProfilerClient::Event): {
        stream >> data.detailType;

        switch (data.detailType) {
        case QQmlProfilerClient::AnimationFrame: {
            stream >> data.framerate >> data.animationcount;
            QVERIFY(data.framerate != -1);
            QVERIFY(data.animationcount != -1);
            break;
        }
        case QQmlProfilerClient::FramePaint:
        case QQmlProfilerClient::Mouse:
        case QQmlProfilerClient::Key:
        case QQmlProfilerClient::StartTrace:
        case QQmlProfilerClient::EndTrace:
            break;
        default: {
            QString failMsg = QString("Unknown event type:") + data.detailType;
            QFAIL(qPrintable(failMsg));
            break;
        }
        }
        break;
    }
    case QQmlProfilerClient::Complete: {
        emit complete();
        return;
    }
    case QQmlProfilerClient::RangeStart: {
        stream >> data.detailType;
        QVERIFY(data.detailType >= 0 && data.detailType < QQmlProfilerClient::MaximumRangeType);
        break;
    }
    case QQmlProfilerClient::RangeEnd: {
        stream >> data.detailType;
        QVERIFY(data.detailType >= 0 && data.detailType < QQmlProfilerClient::MaximumRangeType);
        break;
    }
    case QQmlProfilerClient::RangeData: {
        stream >> data.detailType >> data.detailData;
        QVERIFY(data.detailType >= 0 && data.detailType < QQmlProfilerClient::MaximumRangeType);
        break;
    }
    case QQmlProfilerClient::RangeLocation: {
        stream >> data.detailType >> data.detailData >> data.line >> data.column;
        QVERIFY(data.detailType >= 0 && data.detailType < QQmlProfilerClient::MaximumRangeType);
        QVERIFY(data.line >= -2);
        break;
    }
    case QQmlProfilerClient::PixmapCacheEvent: {
        stream >> data.detailType >> data.detailData;
        if (data.detailType == QQmlProfilerClient::PixmapSizeKnown)
            stream >> data.line >> data.column;
        if (data.detailType == QQmlProfilerClient::PixmapReferenceCountChanged)
            stream >> data.animationcount;
        if (data.detailType == QQmlProfilerClient::PixmapCacheCountChanged)
            stream >> data.animationcount;
        break;
    }
    case QQmlProfilerClient::SceneGraphFrame: {
        stream >> data.detailType;
        qint64 subtime_1, subtime_2, subtime_3, subtime_4, subtime_5;
        int glyphCount;
        switch (data.detailType) {
        // RendererFrame: preprocessTime, updateTime, bindingTime, renderTime
        case QQmlProfilerClient::SceneGraphRendererFrame: stream >> subtime_1 >> subtime_2 >> subtime_3 >> subtime_4; break;
            // AdaptationLayerFrame: glyphCount, glyphRenderTime, glyphStoreTime
        case QQmlProfilerClient::SceneGraphAdaptationLayerFrame: stream >> glyphCount >> subtime_2 >> subtime_3; break;
            // ContextFrame: compiling material time
        case QQmlProfilerClient::SceneGraphContextFrame: stream >> subtime_1; break;
            // RenderLoop: syncTime, renderTime, swapTime
        case QQmlProfilerClient::SceneGraphRenderLoopFrame: stream >> subtime_1 >> subtime_2 >> subtime_3; break;
            // TexturePrepare: bind, convert, swizzle, upload, mipmap
        case QQmlProfilerClient::SceneGraphTexturePrepare: stream >> subtime_1 >> subtime_2 >> subtime_3 >> subtime_4 >> subtime_5; break;
            // TextureDeletion: deletionTime
        case QQmlProfilerClient::SceneGraphTextureDeletion: stream >> subtime_1; break;
            // PolishAndSync: polishTime, waitTime, syncTime, animationsTime,
        case QQmlProfilerClient::SceneGraphPolishAndSync: stream >> subtime_1 >> subtime_2 >> subtime_3 >> subtime_4; break;
            // WindowsRenderLoop: GL time, make current time, SceneGraph time
        case QQmlProfilerClient::SceneGraphWindowsRenderShow: stream >> subtime_1 >> subtime_2 >> subtime_3; break;
            // WindowsAnimations: update time
        case QQmlProfilerClient::SceneGraphWindowsAnimations: stream >> subtime_1; break;
            // WindowsRenderWindow: polish time
        case QQmlProfilerClient::SceneGraphWindowsPolishFrame: stream >> subtime_1; break;
        }
        break;
    }
    default:
        QString failMsg = QString("Unknown message type:") + data.messageType;
        QFAIL(qPrintable(failMsg));
        break;
    }
    QVERIFY(stream.atEnd());
    traceMessages.append(data);
}

void tst_QQmlProfilerService::connect(bool block, const QString &testFile)
{
    const QString executable = QLibraryInfo::location(QLibraryInfo::BinariesPath) + "/qmlscene";
    QStringList arguments;

    if (block)
        arguments << QString("-qmljsdebugger=port:" STR_PORT_FROM "," STR_PORT_TO ",block");
    else
        arguments << QString("-qmljsdebugger=port:" STR_PORT_FROM "," STR_PORT_TO );

    arguments << QQmlDataTest::instance()->testFile(testFile);

    m_process = new QQmlDebugProcess(executable, this);
    m_process->start(QStringList() << arguments);
    QVERIFY2(m_process->waitForSessionStart(), "Could not launch application, or did not get 'Waiting for connection'.");

    QQmlDebugConnection *m_connection = new QQmlDebugConnection();
    m_client = new QQmlProfilerClient(m_connection);

    const int port = m_process->debugPort();
    m_connection->connectToHost(QLatin1String("127.0.0.1"), port);
}

void tst_QQmlProfilerService::cleanup()
{
    if (QTest::currentTestFailed()) {
        qDebug() << "Process State:" << m_process->state();
        qDebug() << "Application Output:" << m_process->output();
    }
    delete m_process;
    delete m_connection;
    delete m_client;
}

void tst_QQmlProfilerService::blockingConnectWithTraceEnabled()
{
    connect(true, "test.qml");
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(true);
    m_client->setTraceState(false);
    QVERIFY2(QQmlDebugTest::waitForSignal(m_client, SIGNAL(complete())), "No trace received in time.");

    QVERIFY(m_client->traceMessages.count());
    // must start with "StartTrace"
    QCOMPARE(m_client->traceMessages.first().messageType, (int)QQmlProfilerClient::Event);
    QCOMPARE(m_client->traceMessages.first().detailType, (int)QQmlProfilerClient::StartTrace);

    // must end with "EndTrace"
    QCOMPARE(m_client->traceMessages.last().messageType, (int)QQmlProfilerClient::Event);
    QCOMPARE(m_client->traceMessages.last().detailType, (int)QQmlProfilerClient::EndTrace);
}

void tst_QQmlProfilerService::blockingConnectWithTraceDisabled()
{
    connect(true, "test.qml");
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(false);
    m_client->setTraceState(true);
    m_client->setTraceState(false);
    QVERIFY2(QQmlDebugTest::waitForSignal(m_client, SIGNAL(complete())), "No trace received in time.");

    QVERIFY(m_client->traceMessages.count());

    // must start with "StartTrace"
    QCOMPARE(m_client->traceMessages.first().messageType, (int)QQmlProfilerClient::Event);
    QCOMPARE(m_client->traceMessages.first().detailType, (int)QQmlProfilerClient::StartTrace);

    // must end with "EndTrace"
    QCOMPARE(m_client->traceMessages.last().messageType, (int)QQmlProfilerClient::Event);
    QCOMPARE(m_client->traceMessages.last().detailType, (int)QQmlProfilerClient::EndTrace);
}

void tst_QQmlProfilerService::nonBlockingConnect()
{
    connect(false, "test.qml");
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(true);
    m_client->setTraceState(false);
    QVERIFY2(QQmlDebugTest::waitForSignal(m_client, SIGNAL(complete())), "No trace received in time.");

    // must start with "StartTrace"
    QCOMPARE(m_client->traceMessages.first().messageType, (int)QQmlProfilerClient::Event);
    QCOMPARE(m_client->traceMessages.first().detailType, (int)QQmlProfilerClient::StartTrace);

    // must end with "EndTrace"
    QCOMPARE(m_client->traceMessages.last().messageType, (int)QQmlProfilerClient::Event);
    QCOMPARE(m_client->traceMessages.last().detailType, (int)QQmlProfilerClient::EndTrace);
}

void tst_QQmlProfilerService::pixmapCacheData()
{
    connect(true, "pixmapCacheTest.qml");
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(true);
    QVERIFY(QQmlDebugTest::waitForSignal(m_process, SIGNAL(readyReadStandardOutput())));

    QVERIFY(m_process->output().indexOf(QLatin1String("image loaded")) != -1 ||
            m_process->output().indexOf(QLatin1String("image error")) != -1 );


    m_client->setTraceState(false);

    QVERIFY2(QQmlDebugTest::waitForSignal(m_client, SIGNAL(complete())), "No trace received in time.");
    QVERIFY(m_client->traceMessages.count());

    // must start with "StartTrace"
    QCOMPARE(m_client->traceMessages.first().messageType, (int)QQmlProfilerClient::Event);
    QCOMPARE(m_client->traceMessages.first().detailType, (int)QQmlProfilerClient::StartTrace);

    // image starting to load
    QCOMPARE(m_client->traceMessages[8].messageType, (int)QQmlProfilerClient::PixmapCacheEvent);
    QCOMPARE(m_client->traceMessages[8].detailType, (int)QQmlProfilerClient::PixmapLoadingStarted);

    // image loaded
    QCOMPARE(m_client->traceMessages[9].messageType, (int)QQmlProfilerClient::PixmapCacheEvent);
    QCOMPARE(m_client->traceMessages[9].detailType, (int)QQmlProfilerClient::PixmapLoadingFinished);

    // image size
    QCOMPARE(m_client->traceMessages[10].messageType, (int)QQmlProfilerClient::PixmapCacheEvent);
    QCOMPARE(m_client->traceMessages[10].detailType, (int)QQmlProfilerClient::PixmapSizeKnown);
    QCOMPARE(m_client->traceMessages[10].line, 2); // width
    QCOMPARE(m_client->traceMessages[10].column, 2); // height

    // cache size
    QCOMPARE(m_client->traceMessages[11].messageType, (int)QQmlProfilerClient::PixmapCacheEvent);
    QCOMPARE(m_client->traceMessages[11].detailType, (int)QQmlProfilerClient::PixmapCacheCountChanged);

    // must end with "EndTrace"
    QCOMPARE(m_client->traceMessages.last().messageType, (int)QQmlProfilerClient::Event);
    QCOMPARE(m_client->traceMessages.last().detailType, (int)QQmlProfilerClient::EndTrace);

}

void tst_QQmlProfilerService::scenegraphData()
{
    connect(true, "scenegraphTest.qml");
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(true);
    QVERIFY(QQmlDebugTest::waitForSignal(m_process, SIGNAL(readyReadStandardOutput())));
    QVERIFY(m_process->output().indexOf(QLatin1String("tick")) != -1);
    m_client->setTraceState(false);

    QVERIFY2(QQmlDebugTest::waitForSignal(m_client, SIGNAL(complete())), "No trace received in time.");
    QVERIFY(m_client->traceMessages.count());

    // check that at least one frame was rendered
    // there should be a SGPolishAndSync + SGRendererFrame + SGRenderLoopFrame sequence
    // since the rendering happens in a different thread, there could be other unrelated events interleaved
    int loopcheck = 0;
    foreach (const QQmlProfilerData &msg, m_client->traceMessages) {
        if (msg.messageType == QQmlProfilerClient::SceneGraphFrame) {
            if (loopcheck == 0 && msg.detailType == QQmlProfilerClient::SceneGraphContextFrame)
                loopcheck = 1;
            else
            if (loopcheck == 1 && msg.detailType == QQmlProfilerClient::SceneGraphRendererFrame)
                loopcheck = 2;
            else
            if (loopcheck == 2 && msg.detailType == QQmlProfilerClient::SceneGraphRenderLoopFrame)
               loopcheck = 3;
        }
    }

    QCOMPARE(loopcheck, 3);
}

void tst_QQmlProfilerService::profileOnExit()
{
    connect(true, "exit.qml");
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(true);

    QVERIFY2(QQmlDebugTest::waitForSignal(m_client, SIGNAL(complete())), "No trace received in time.");

    // must start with "StartTrace"
    QCOMPARE(m_client->traceMessages.first().messageType, (int)QQmlProfilerClient::Event);
    QCOMPARE(m_client->traceMessages.first().detailType, (int)QQmlProfilerClient::StartTrace);

    // must end with "EndTrace"
    QCOMPARE(m_client->traceMessages.last().messageType, (int)QQmlProfilerClient::Event);
    QCOMPARE(m_client->traceMessages.last().detailType, (int)QQmlProfilerClient::EndTrace);
}

QTEST_MAIN(tst_QQmlProfilerService)

#include "tst_qqmlprofilerservice.moc"
