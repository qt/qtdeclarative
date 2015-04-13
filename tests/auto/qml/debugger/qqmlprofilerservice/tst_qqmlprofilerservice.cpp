/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
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
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    QQmlProfilerData(int messageType = 0, int detailType = 0, const QString &detailData = QString())
        : messageType(messageType), detailType(detailType), detailData(detailData) {}

    qint64 time;
    int messageType;
    int detailType;

    //###
    QString detailData; //used by RangeData and RangeLocation
    int line;           //used by RangeLocation
    int column;         //used by RangeLocation
    int framerate;      //used by animation events
    int animationcount; //used by animation events
    qint64 amount;      //used by heap events

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
        MemoryAllocation,

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
        Javascript,

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

    enum MemoryType {
        HeapPage,
        LargeItem,
        SmallItem
    };

    QQmlProfilerClient(QQmlDebugConnection *connection)
        : QQmlDebugClient(QLatin1String("CanvasFrameRate"), connection)
    {
    }

    QList<QQmlProfilerData> qmlMessages;
    QList<QQmlProfilerData> javascriptMessages;
    QList<QQmlProfilerData> jsHeapMessages;
    QList<QQmlProfilerData> asynchronousMessages;
    QList<QQmlProfilerData> pixmapMessages;

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

    enum MessageListType {
        MessageListQML,
        MessageListJavaScript,
        MessageListJsHeap,
        MessageListAsynchronous,
        MessageListPixmap
    };

    enum CheckType {
        CheckMessageType  = 1 << 0,
        CheckDetailType   = 1 << 1,
        CheckLine         = 1 << 2,
        CheckColumn       = 1 << 3,
        CheckDataEndsWith = 1 << 4,

        CheckAll = CheckMessageType | CheckDetailType | CheckLine | CheckColumn | CheckDataEndsWith
    };

    void connect(bool block, const QString &testFile);
    void checkTraceReceived();
    void checkJsHeap();
    bool verify(MessageListType type, int expectedPosition, const QQmlProfilerData &expected,
                quint32 checks);

private slots:
    void cleanup();

    void blockingConnectWithTraceEnabled();
    void blockingConnectWithTraceDisabled();
    void nonBlockingConnect();
    void pixmapCacheData();
    void scenegraphData();
    void profileOnExit();
    void controlFromJS();
    void signalSourceLocation();
    void javascript();
};

#define VERIFY(type, position, expected, checks) QVERIFY(verify(type, position, expected, checks))

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

    switch (data.messageType) {
    case (QQmlProfilerClient::Event): {
        stream >> data.detailType;

        switch (data.detailType) {
        case QQmlProfilerClient::AnimationFrame: {
            int threadId;
            stream >> data.framerate >> data.animationcount >> threadId;
            QVERIFY(threadId >= 0);
            QVERIFY(data.framerate != -1);
            QVERIFY(data.animationcount != -1);
            break;
        }
        case QQmlProfilerClient::FramePaint:
        case QQmlProfilerClient::Mouse:
        case QQmlProfilerClient::Key:
            break;
        case QQmlProfilerClient::EndTrace:
        case QQmlProfilerClient::StartTrace: {
            int engineId = -1;
            if (!stream.atEnd()) {
                stream >> engineId;
                QVERIFY(engineId >= 0);
            }
            break;
        }
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
    case QQmlProfilerClient::MemoryAllocation: {
        stream >> data.detailType;
        stream >> data.amount;
        break;
    }
    default:
        QString failMsg = QString("Unknown message type:") + data.messageType;
        QFAIL(qPrintable(failMsg));
        break;
    }
    QVERIFY(stream.atEnd());
    if (data.messageType == QQmlProfilerClient::PixmapCacheEvent)
        pixmapMessages.append(data);
    else if (data.messageType == QQmlProfilerClient::SceneGraphFrame ||
            data.messageType == QQmlProfilerClient::Event)
        asynchronousMessages.append(data);
    else if (data.messageType == QQmlProfilerClient::MemoryAllocation)
        jsHeapMessages.append(data);
    else if (data.detailType == QQmlProfilerClient::Javascript)
        javascriptMessages.append(data);
    else
        qmlMessages.append(data);
}

void tst_QQmlProfilerService::connect(bool block, const QString &testFile)
{
    // ### Still using qmlscene due to QTBUG-33377
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

    m_connection = new QQmlDebugConnection();
    m_client = new QQmlProfilerClient(m_connection);

    const int port = m_process->debugPort();
    m_connection->connectToHost(QLatin1String("127.0.0.1"), port);
}

void tst_QQmlProfilerService::checkTraceReceived()
{
    QVERIFY2(QQmlDebugTest::waitForSignal(m_client, SIGNAL(complete())), "No trace received in time.");

    // must start with "StartTrace"
    QQmlProfilerData expected(QQmlProfilerClient::Event, QQmlProfilerClient::StartTrace);
    VERIFY(MessageListAsynchronous, 0, expected, CheckMessageType | CheckDetailType);

    // must end with "EndTrace"
    expected.detailType = QQmlProfilerClient::EndTrace;
    VERIFY(MessageListAsynchronous, m_client->asynchronousMessages.length() - 1, expected,
           CheckMessageType | CheckDetailType);
}

void tst_QQmlProfilerService::checkJsHeap()
{
    QVERIFY2(m_client->jsHeapMessages.count() > 0, "no JavaScript heap messages received");

    bool seen_alloc = false;
    bool seen_small = false;
    bool seen_large = false;
    qint64 allocated = 0;
    qint64 used = 0;
    qint64 lastTimestamp = -1;
    foreach (const QQmlProfilerData &message, m_client->jsHeapMessages) {
        switch (message.detailType) {
        case QQmlProfilerClient::HeapPage:
            allocated += message.amount;
            seen_alloc = true;
            break;
        case QQmlProfilerClient::SmallItem:
            used += message.amount;
            seen_small = true;
            break;
        case QQmlProfilerClient::LargeItem:
            allocated += message.amount;
            used += message.amount;
            seen_large = true;
            break;
        }

        QVERIFY(message.time >= lastTimestamp);
        // The heap will only be consistent after all events of the same timestamp are processed.
        if (lastTimestamp == -1) {
            lastTimestamp = message.time;
            continue;
        } else if (message.time == lastTimestamp) {
            continue;
        }

        lastTimestamp = message.time;

        QVERIFY2(used >= 0, QString::fromLatin1("Negative memory usage seen: %1")
                 .arg(used).toUtf8().constData());

        QVERIFY2(allocated >= 0, QString::fromLatin1("Negative memory allocation seen: %1")
                 .arg(allocated).toUtf8().constData());

        QVERIFY2(used <= allocated,
                 QString::fromLatin1("More memory usage than allocation seen: %1 > %2")
                 .arg(used).arg(allocated).toUtf8().constData());
    }

    QVERIFY2(seen_alloc, "No heap allocation seen");
    QVERIFY2(seen_small, "No small item seen");
    QVERIFY2(seen_large, "No large item seen");
}

bool tst_QQmlProfilerService::verify(tst_QQmlProfilerService::MessageListType type,
                                     int expectedPosition, const QQmlProfilerData &expected,
                                     quint32 checks)
{
    QList<QQmlProfilerData> *target = 0;
    switch (type) {
        case MessageListQML:          target = &(m_client->qmlMessages); break;
        case MessageListJavaScript:   target = &(m_client->javascriptMessages); break;
        case MessageListJsHeap:       target = &(m_client->jsHeapMessages); break;
        case MessageListAsynchronous: target = &(m_client->asynchronousMessages); break;
        case MessageListPixmap:       target = &(m_client->pixmapMessages); break;
    }

    if (target->length() <= expectedPosition) {
        qWarning() << "Not enough events. expected position:" << expectedPosition
                   << "length:" << target->length();
        return false;
    }

    uint position = expectedPosition;
    qint64 timestamp = target->at(expectedPosition).time;
    while (position > 0 && target->at(position - 1).time == timestamp)
        --position;

    QStringList warnings;

    do {
        const QQmlProfilerData &actual = target->at(position);
        if ((checks & CheckMessageType) &&  actual.messageType != expected.messageType) {
            warnings << QString::fromLatin1("%1: unexpected messageType. actual: %2 - expected: %3")
                       .arg(position).arg(actual.messageType).arg(expected.messageType);
            continue;
        }
        if ((checks & CheckDetailType) && actual.detailType != expected.detailType) {
            warnings << QString::fromLatin1("%1: unexpected detailType. actual: %2 - expected: %3")
                       .arg(position).arg(actual.detailType).arg(expected.detailType);
            continue;
        }
        if ((checks & CheckLine) && actual.line != expected.line) {
            warnings << QString::fromLatin1("%1: unexpected line. actual: %2 - expected: %3")
                       .arg(position).arg(actual.line).arg(expected.line);
            continue;
        }
        if ((checks & CheckColumn) && actual.column != expected.column) {
            warnings << QString::fromLatin1("%1: unexpected column. actual: %2 - expected: %3")
                       .arg(position).arg(actual.column).arg(expected.column);
            continue;
        }
        if ((checks & CheckDataEndsWith) && !actual.detailData.endsWith(expected.detailData)) {
            warnings << QString::fromLatin1("%1: unexpected detailData. actual: %2 - expected: %3")
                       .arg(position).arg(actual.detailData).arg(expected.detailData);
            continue;
        }
        return true;
    } while (target->at(++position).time == timestamp);

    foreach (const QString &message, warnings)
        qWarning() << message.toLocal8Bit().constData();

    return false;
}

void tst_QQmlProfilerService::cleanup()
{
    if (QTest::currentTestFailed()) {
        qDebug() << "QML Messages:" << m_client->qmlMessages.count();
        int i = 0;
        foreach (const QQmlProfilerData &data, m_client->qmlMessages) {
            qDebug() << i++ << data.time << data.messageType << data.detailType << data.detailData
                     << data.line << data.column;
        }
        qDebug() << " ";
        qDebug() << "JavaScript Messages:" << m_client->javascriptMessages.count();
        i = 0;
        foreach (const QQmlProfilerData &data, m_client->javascriptMessages) {
            qDebug() << i++ << data.time << data.messageType << data.detailType << data.detailData
                     << data.line << data.column;
        }
        qDebug() << " ";
        qDebug() << "Asynchronous Messages:" << m_client->asynchronousMessages.count();
        i = 0;
        foreach (const QQmlProfilerData &data, m_client->asynchronousMessages) {
            qDebug() << i++ << data.time << data.messageType << data.detailType << data.detailData
                     << data.framerate << data.animationcount << data.line << data.column;
        }
        qDebug() << " ";
        qDebug() << "Pixmap Cache Messages:" << m_client->pixmapMessages.count();
        i = 0;
        foreach (const QQmlProfilerData &data, m_client->pixmapMessages) {
            qDebug() << i++ << data.time << data.messageType << data.detailType << data.detailData
                     << data.line << data.column;
        }
        qDebug() << " ";
        qDebug() << "Javascript Heap Messages:" << m_client->jsHeapMessages.count();
        i = 0;
        foreach (const QQmlProfilerData &data, m_client->jsHeapMessages) {
            qDebug() << i++ << data.time << data.messageType << data.detailType;
        }
        qDebug() << " ";
        qDebug() << "Process State:" << (m_process ? m_process->state() : QLatin1String("null"));
        qDebug() << "Application Output:" << (m_process ? m_process->output() : QLatin1String("null"));
        qDebug() << "Connection State:" << (m_connection ? m_connection->stateString() : QLatin1String("null"));
        qDebug() << "Client State:" << (m_client ? m_client->stateString() : QLatin1String("null"));
    }
    delete m_process;
    m_process = 0;
    delete m_client;
    m_client = 0;
    delete m_connection;
    m_connection = 0;
}

void tst_QQmlProfilerService::blockingConnectWithTraceEnabled()
{
    connect(true, "test.qml");
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(true);
    m_client->setTraceState(false);
    checkTraceReceived();
    checkJsHeap();
}

void tst_QQmlProfilerService::blockingConnectWithTraceDisabled()
{
    connect(true, "test.qml");
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(false);
    m_client->setTraceState(true);
    m_client->setTraceState(false);
    checkTraceReceived();
    checkJsHeap();
}

void tst_QQmlProfilerService::nonBlockingConnect()
{
    connect(false, "test.qml");
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(true);
    m_client->setTraceState(false);
    checkTraceReceived();
    checkJsHeap();
}

void tst_QQmlProfilerService::pixmapCacheData()
{
    connect(true, "pixmapCacheTest.qml");
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(true);
    QVERIFY(QQmlDebugTest::waitForSignal(m_process, SIGNAL(readyReadStandardOutput())));

    while (m_process->output().indexOf(QLatin1String("image loaded")) == -1 &&
           m_process->output().indexOf(QLatin1String("image error")) == -1)
        QVERIFY(QQmlDebugTest::waitForSignal(m_process, SIGNAL(readyReadStandardOutput())));

    m_client->setTraceState(false);

    checkTraceReceived();
    checkJsHeap();

    QQmlProfilerData expected(QQmlProfilerClient::PixmapCacheEvent);

    // image starting to load
    expected.detailType = QQmlProfilerClient::PixmapLoadingStarted;
    VERIFY(MessageListPixmap, 0, expected, CheckMessageType | CheckDetailType);

    // image size
    expected.detailType = QQmlProfilerClient::PixmapSizeKnown;
    expected.line = expected.column = 2; // width and height, in fact
    VERIFY(MessageListPixmap, 1, expected,
           CheckMessageType | CheckDetailType | CheckLine | CheckColumn);

    // image loaded
    expected.detailType = QQmlProfilerClient::PixmapLoadingFinished;
    VERIFY(MessageListPixmap, 2, expected, CheckMessageType | CheckDetailType);

    // cache size
    expected.detailType = QQmlProfilerClient::PixmapCacheCountChanged;
    VERIFY(MessageListPixmap, 3, expected, CheckMessageType | CheckDetailType);
}

void tst_QQmlProfilerService::scenegraphData()
{
    connect(true, "scenegraphTest.qml");
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(true);

    while (!m_process->output().contains(QLatin1String("tick")))
        QVERIFY(QQmlDebugTest::waitForSignal(m_process, SIGNAL(readyReadStandardOutput())));
    m_client->setTraceState(false);

    checkTraceReceived();
    checkJsHeap();

    // check that at least one frame was rendered
    // there should be a SGPolishAndSync + SGRendererFrame + SGRenderLoopFrame sequence
    // (though we can't be sure to get the SGRenderLoopFrame in the threaded renderer)
    //
    // since the rendering happens in a different thread, there could be other unrelated events
    // interleaved. Also, events could carry the same time stamps and be sorted in an unexpected way
    // if the clocks are acting up.
    qint64 contextFrameTime = -1;
    qint64 renderFrameTime = -1;

    foreach (const QQmlProfilerData &msg, m_client->asynchronousMessages) {
        if (msg.messageType == QQmlProfilerClient::SceneGraphFrame) {
            if (msg.detailType == QQmlProfilerClient::SceneGraphContextFrame) {
                contextFrameTime = msg.time;
                break;
            }
        }
    }

    QVERIFY(contextFrameTime != -1);

    foreach (const QQmlProfilerData &msg, m_client->asynchronousMessages) {
        if (msg.detailType == QQmlProfilerClient::SceneGraphRendererFrame) {
            QVERIFY(msg.time >= contextFrameTime);
            renderFrameTime = msg.time;
            break;
        }
    }

    QVERIFY(renderFrameTime != -1);

    foreach (const QQmlProfilerData &msg, m_client->asynchronousMessages) {
        if (msg.detailType == QQmlProfilerClient::SceneGraphRenderLoopFrame) {
            QVERIFY(msg.time >= renderFrameTime);
            break;
        }
    }
}

void tst_QQmlProfilerService::profileOnExit()
{
    connect(true, "exit.qml");
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(true);

    checkTraceReceived();
    checkJsHeap();
}

void tst_QQmlProfilerService::controlFromJS()
{
    connect(true, "controlFromJS.qml");
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(false);
    checkTraceReceived();
    checkJsHeap();
}

void tst_QQmlProfilerService::signalSourceLocation()
{
    connect(true, "signalSourceLocation.qml");
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(true);
    while (!(m_process->output().contains(QLatin1String("500"))))
        QVERIFY(QQmlDebugTest::waitForSignal(m_process, SIGNAL(readyReadStandardOutput())));
    m_client->setTraceState(false);
    checkTraceReceived();
    checkJsHeap();

    QQmlProfilerData expected(QQmlProfilerClient::RangeLocation,
                              QQmlProfilerClient::HandlingSignal,
                              QLatin1String("signalSourceLocation.qml"));
    expected.line = 8;
    expected.column = 28;
    VERIFY(MessageListQML, 13, expected, CheckAll);

    expected.line = 7;
    expected.column = 21;
    VERIFY(MessageListQML, 15, expected, CheckAll);
}

void tst_QQmlProfilerService::javascript()
{
    connect(true, "javascript.qml");
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setTraceState(true);
    while (!(m_process->output().contains(QLatin1String("done"))))
        QVERIFY(QQmlDebugTest::waitForSignal(m_process, SIGNAL(readyReadStandardOutput())));
    m_client->setTraceState(false);
    checkTraceReceived();
    checkJsHeap();

    QQmlProfilerData expected(QQmlProfilerClient::RangeStart, QQmlProfilerClient::Javascript);
    VERIFY(MessageListJavaScript, 6, expected, CheckMessageType | CheckDetailType);

    expected.messageType = QQmlProfilerClient::RangeLocation;
    expected.detailData = QLatin1String("javascript.qml");
    expected.line = 4;
    expected.column = 5;
    VERIFY(MessageListJavaScript, 7, expected, CheckAll);

    expected.messageType = QQmlProfilerClient::RangeData;
    expected.detailData = QLatin1String("something");
    VERIFY(MessageListJavaScript, 8, expected,
           CheckMessageType | CheckDetailType | CheckDataEndsWith);

    expected.messageType = QQmlProfilerClient::RangeEnd;
    VERIFY(MessageListJavaScript, 21, expected, CheckMessageType | CheckDetailType);
}

QTEST_MAIN(tst_QQmlProfilerService)

#include "tst_qqmlprofilerservice.moc"
