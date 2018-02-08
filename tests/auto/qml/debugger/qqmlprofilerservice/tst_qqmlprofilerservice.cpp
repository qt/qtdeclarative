/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "debugutil_p.h"
#include "qqmldebugprocess_p.h"
#include "../../../shared/util.h"

#include <private/qqmlprofilerclient_p.h>
#include <private/qqmldebugconnection_p.h>

#include <QtTest/qtest.h>
#include <private/qtestresult_p.h>
#include <QtCore/qlibraryinfo.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

class QQmlProfilerTestClient : public QQmlProfilerEventReceiver, public QQmlProfilerDefinitions
{
    Q_OBJECT

public:
    QQmlProfilerTestClient(QQmlDebugConnection *connection) :
        client(new QQmlProfilerClient(connection, this))
    {
        connect(client.data(), &QQmlProfilerClient::traceStarted,
                this, &QQmlProfilerTestClient::startTrace);
        connect(client.data(), &QQmlProfilerClient::traceFinished,
                this, &QQmlProfilerTestClient::endTrace);
    }

    void startTrace(qint64 timestamp, const QList<int> &engineIds);
    void endTrace(qint64 timestamp, const QList<int> &engineIds);

    QPointer<QQmlProfilerClient> client; // Owned by QQmlDebugTest
    QVector<QQmlProfilerEventType> types;

    QVector<QQmlProfilerEvent> qmlMessages;
    QVector<QQmlProfilerEvent> javascriptMessages;
    QVector<QQmlProfilerEvent> jsHeapMessages;
    QVector<QQmlProfilerEvent> asynchronousMessages;
    QVector<QQmlProfilerEvent> pixmapMessages;

    int numLoadedEventTypes() const override;
    void addEventType(const QQmlProfilerEventType &type) override;
    void addEvent(const QQmlProfilerEvent &event) override;
};

void QQmlProfilerTestClient::startTrace(qint64 timestamp, const QList<int> &engineIds)
{
    types.append(QQmlProfilerEventType(Event, MaximumRangeType, StartTrace));
    asynchronousMessages.append(QQmlProfilerEvent(timestamp, types.length() - 1,
                                                  engineIds.toVector()));
}

void QQmlProfilerTestClient::endTrace(qint64 timestamp, const QList<int> &engineIds)
{
    types.append(QQmlProfilerEventType(Event, MaximumRangeType, EndTrace));
    asynchronousMessages.append(QQmlProfilerEvent(timestamp, types.length() - 1,
                                                  engineIds.toVector()));
}

int QQmlProfilerTestClient::numLoadedEventTypes() const
{
    return types.length();
}

void QQmlProfilerTestClient::addEventType(const QQmlProfilerEventType &type)
{
    types.append(type);
}

void QQmlProfilerTestClient::addEvent(const QQmlProfilerEvent &event)
{
    const int typeIndex = event.typeIndex();
    QVERIFY(typeIndex < types.length());

    const QQmlProfilerEventType &type = types[typeIndex];

    switch (type.message()) {
    case Event: {
        switch (type.detailType()) {
        case StartTrace:
            QFAIL("StartTrace should not be passed on as event");
            break;
        case EndTrace:
            QFAIL("EndTrace should not be passed on as event");
            break;
        case AnimationFrame:
            asynchronousMessages.append(event);
            break;
        case Mouse:
        case Key:
            qmlMessages.append(event);
            break;
        default:
            QFAIL(qPrintable(QString::fromLatin1("Event with unknown detailType %1 received at %2.")
                             .arg(type.detailType()).arg(event.timestamp())));
            break;
        }
        break;
    }
    case RangeStart:
    case RangeData:
    case RangeLocation:
    case RangeEnd:
        QFAIL("Range stages are transmitted as part of events");
        break;
    case Complete:
        QFAIL("Complete should not be passed on as event");
        break;
    case PixmapCacheEvent:
        pixmapMessages.append(event);
        break;
    case SceneGraphFrame:
        asynchronousMessages.append(event);
        break;
    case MemoryAllocation:
        jsHeapMessages.append(event);
        break;
    case DebugMessage:
        // Unhandled
        break;
    case MaximumMessage:
        switch (type.rangeType()) {
        case Painting:
            QFAIL("QtQuick1 paint message received.");
            break;
        case Compiling:
        case Creating:
        case Binding:
        case HandlingSignal:
            qmlMessages.append(event);
            break;
        case Javascript:
            javascriptMessages.append(event);
            break;
        default:
            QFAIL(qPrintable(
                      QString::fromLatin1("Unknown range event %1 received at %2.")
                      .arg(type.rangeType()).arg(event.timestamp())));
            break;
        }
        break;
    }
}

class tst_QQmlProfilerService : public QQmlDebugTest
{
    Q_OBJECT

private:
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
        CheckFileEndsWith = 1 << 5,
        CheckNumbers      = 1 << 6,

        CheckType = CheckMessageType | CheckDetailType | CheckLine | CheckColumn | CheckFileEndsWith
    };

    ConnectResult connect(bool block, const QString &testFile, bool recordFromStart = true,
                          uint flushInterval = 0, bool restrictServices = true);
    void checkProcessTerminated();
    void checkTraceReceived();
    void checkJsHeap();
    bool verify(MessageListType type, int expectedPosition,
                const QQmlProfilerEventType &expectedType, quint32 checks,
                const QVector<qint64> &numbers);

    QList<QQmlDebugClient *> createClients() override;
    QScopedPointer<QQmlProfilerTestClient> m_client;

private slots:
    void cleanup() override;

    void connect_data();
    void connect();
    void pixmapCacheData();
    void scenegraphData();
    void profileOnExit();
    void controlFromJS();
    void signalSourceLocation();
    void javascript();
    void flushInterval();
    void translationBinding();
    void memory();

private:
    bool m_recordFromStart = true;
    bool m_flushInterval = 0;
    bool m_isComplete = false;

    // Don't use ({...}) here as MSVC will interpret that as the "QVector(int size)" ctor.
    const QVector<qint64> m_rangeStart = (QVector<qint64>() << QQmlProfilerDefinitions::RangeStart);
    const QVector<qint64> m_rangeEnd = (QVector<qint64>() << QQmlProfilerDefinitions::RangeEnd);
};

#define VERIFY(type, position, expected, checks, numbers) \
    QVERIFY(verify(type, position, expected, checks, numbers))

QQmlDebugTest::ConnectResult tst_QQmlProfilerService::connect(
        bool block, const QString &file, bool recordFromStart, uint flushInterval,
        bool restrictServices)
{
    m_recordFromStart = recordFromStart;
    m_flushInterval = flushInterval;
    m_isComplete = false;

    // ### Still using qmlscene due to QTBUG-33377
    return QQmlDebugTest::connect(
                QLibraryInfo::location(QLibraryInfo::BinariesPath) + "/qmlscene",
                restrictServices ? QQmlDebuggingEnabler::profilerServices().join(',') : QString(),
                testFile(file), block);
}

void tst_QQmlProfilerService::checkProcessTerminated()
{
    // If the process ends before connect(), we get a non-success value from connect()
    // That's not a problem as we will still receive the trace. We check that process has terminated
    // cleanly here.

    // Wait for the process to finish by itself, if that hasn't happened already
    QVERIFY(m_client);
    QVERIFY(m_client->client);
    QTRY_COMPARE(m_client->client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_process);
    QTRY_COMPARE(m_process->exitStatus(), QProcess::NormalExit);
}

void tst_QQmlProfilerService::checkTraceReceived()
{
    QTRY_VERIFY2(m_isComplete, "No trace received in time.");

    QVector<qint64> numbers;

    // must start with "StartTrace"
    QQmlProfilerEventType expected(QQmlProfilerDefinitions::Event,
                                   QQmlProfilerDefinitions::MaximumRangeType,
                                   QQmlProfilerDefinitions::StartTrace);
    VERIFY(MessageListAsynchronous, 0, expected, CheckMessageType | CheckDetailType, numbers);

    // must end with "EndTrace"
    expected = QQmlProfilerEventType(QQmlProfilerDefinitions::Event,
                                     QQmlProfilerDefinitions::MaximumRangeType,
                                     QQmlProfilerDefinitions::EndTrace);
    VERIFY(MessageListAsynchronous, m_client->asynchronousMessages.length() - 1, expected,
           CheckMessageType | CheckDetailType, numbers);
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
    foreach (const QQmlProfilerEvent &message, m_client->jsHeapMessages) {
        const qint64 amount = message.number<qint64>(0);
        const QQmlProfilerEventType &type = m_client->types.at(message.typeIndex());
        switch (type.detailType()) {
        case QV4::Profiling::HeapPage:
            allocated += amount;
            seen_alloc = true;
            break;
        case QV4::Profiling::SmallItem:
            used += amount;
            seen_small = true;
            break;
        case QV4::Profiling::LargeItem:
            allocated += amount;
            used += amount;
            seen_large = true;
            break;
        }

        QVERIFY(message.timestamp() >= lastTimestamp);
        // The heap will only be consistent after all events of the same timestamp are processed.
        if (lastTimestamp == -1) {
            lastTimestamp = message.timestamp();
            continue;
        } else if (message.timestamp() == lastTimestamp) {
            continue;
        }

        lastTimestamp = message.timestamp();

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
                                     int expectedPosition, const QQmlProfilerEventType &expected,
                                     quint32 checks, const QVector<qint64> &expectedNumbers)
{
    const QVector<QQmlProfilerEvent> *target = 0;
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
    qint64 timestamp = target->at(expectedPosition).timestamp();
    while (position > 0 && target->at(position - 1).timestamp() == timestamp)
        --position;

    QStringList warnings;

    do {
        const QQmlProfilerEvent &event = target->at(position);
        const QQmlProfilerEventType &actual = m_client->types.at(event.typeIndex());
        if ((checks & CheckMessageType) &&
                (actual.message() != expected.message()
                 || actual.rangeType() != expected.rangeType())) {
            warnings << QString::fromLatin1("%1: unexpected messageType or rangeType. "
                                            "actual: %2, %3 - expected: %4, %5")
                        .arg(position).arg(actual.message()).arg(actual.rangeType())
                        .arg(expected.message()).arg(expected.rangeType());
            continue;
        }
        if ((checks & CheckDetailType) && actual.detailType() != expected.detailType()) {
            warnings << QString::fromLatin1("%1: unexpected detailType. actual: %2 - expected: %3")
                        .arg(position).arg(actual.detailType()).arg(expected.detailType());
            continue;
        }

        const QQmlProfilerEventLocation expectedLocation = expected.location();
        const QQmlProfilerEventLocation actualLocation = actual.location();

        if ((checks & CheckLine) && actualLocation.line() != expectedLocation.line()) {
            warnings << QString::fromLatin1("%1: unexpected line. actual: %2 - expected: %3")
                        .arg(position).arg(actualLocation.line())
                        .arg(expectedLocation.line());
            continue;
        }
        if ((checks & CheckColumn) && actualLocation.column() != expectedLocation.column()) {
            warnings << QString::fromLatin1("%1: unexpected column. actual: %2 - expected: %3")
                        .arg(position).arg(actualLocation.column())
                        .arg(expectedLocation.column());
            continue;
        }
        if ((checks & CheckFileEndsWith) &&
                !actualLocation.filename().endsWith(expectedLocation.filename())) {
            warnings << QString::fromLatin1("%1: unexpected fileName. actual: %2 - expected: %3")
                        .arg(position).arg(actualLocation.filename())
                        .arg(expectedLocation.filename());
            continue;
        }

        if ((checks & CheckDataEndsWith) && !actual.data().endsWith(expected.data())) {
            warnings << QString::fromLatin1("%1: unexpected detailData. actual: %2 - expected: %3")
                       .arg(position).arg(actual.data()).arg(expected.data());
            continue;
        }

        if (checks & CheckNumbers) {
            const QVector<qint64> actualNumbers = event.numbers<QVector<qint64>>();
            if (actualNumbers != expectedNumbers) {

                QStringList expectedList;
                for (qint64 number : expectedNumbers)
                    expectedList.append(QString::number(number));
                QStringList actualList;
                for (qint64 number : actualNumbers)
                    actualList.append(QString::number(number));

                warnings << QString::fromLatin1(
                                "%1: unexpected numbers. actual [%2] - expected: [%3]")
                            .arg(position)
                            .arg(actualList.join(QLatin1String(", ")))
                            .arg(expectedList.join(QLatin1String(", ")));
                continue;
            }
        }

        return true;
    } while (target->at(++position).timestamp() == timestamp);

    foreach (const QString &message, warnings)
        qWarning() << message.toLocal8Bit().constData();

    return false;
}

QList<QQmlDebugClient *> tst_QQmlProfilerService::createClients()
{
    m_client.reset(new QQmlProfilerTestClient(m_connection));
    m_client->client->setRecording(m_recordFromStart);
    m_client->client->setFlushInterval(m_flushInterval);
    QObject::connect(m_client->client, &QQmlProfilerClient::complete,
                     this, [this](){ m_isComplete = true; });
    return QList<QQmlDebugClient *>({m_client->client});
}

void tst_QQmlProfilerService::cleanup()
{
    auto log = [this](const QQmlProfilerEvent &data, int i) {
        const QQmlProfilerEventType &type = m_client->types.at(data.typeIndex());
        const QQmlProfilerEventLocation location = type.location();
        qDebug() << i << data.timestamp() << type.message() << type.rangeType() << type.detailType()
                 << location.filename() << location.line() << location.column()
                 << data.numbers<QVector<qint64>>();
    };

    if (m_client && QTest::currentTestFailed()) {
        qDebug() << "QML Messages:" << m_client->qmlMessages.count();
        int i = 0;
        for (const QQmlProfilerEvent &data : qAsConst(m_client->qmlMessages))
            log(data, i++);

        qDebug() << " ";
        qDebug() << "JavaScript Messages:" << m_client->javascriptMessages.count();
        i = 0;

        for (const QQmlProfilerEvent &data : qAsConst(m_client->javascriptMessages))
            log(data, i++);

        qDebug() << " ";
        qDebug() << "Asynchronous Messages:" << m_client->asynchronousMessages.count();
        i = 0;
        for (const QQmlProfilerEvent &data : qAsConst(m_client->asynchronousMessages))
            log(data, i++);

        qDebug() << " ";
        qDebug() << "Pixmap Cache Messages:" << m_client->pixmapMessages.count();
        i = 0;
        for (const QQmlProfilerEvent &data : qAsConst(m_client->pixmapMessages))
            log(data, i++);

        qDebug() << " ";
        qDebug() << "Javascript Heap Messages:" << m_client->jsHeapMessages.count();
        i = 0;
        for (const QQmlProfilerEvent &data : qAsConst(m_client->jsHeapMessages))
            log(data, i++);

        qDebug() << " ";
    }

    m_client.reset();
    QQmlDebugTest::cleanup();
}

void tst_QQmlProfilerService::connect_data()
{
    QTest::addColumn<bool>("blockMode");
    QTest::addColumn<bool>("restrictMode");
    QTest::addColumn<bool>("traceEnabled");
    QTest::newRow("normal/unrestricted/disabled") << false << false << false;
    QTest::newRow("block/unrestricted/disabled") << true << false << false;
    QTest::newRow("normal/restricted/disabled") << false << true << false;
    QTest::newRow("block/restricted/disabled") << true << true << false;
    QTest::newRow("normal/unrestricted/enabled") << false << false << true;
    QTest::newRow("block/unrestricted/enabled") << true << false << true;
    QTest::newRow("normal/restricted/enabled") << false << true << true;
    QTest::newRow("block/restricted/enabled") << true << true << true;
}

void tst_QQmlProfilerService::connect()
{
    QFETCH(bool, blockMode);
    QFETCH(bool, restrictMode);
    QFETCH(bool, traceEnabled);

    QCOMPARE(connect(blockMode, "test.qml", traceEnabled, 0, restrictMode), ConnectSuccess);

    if (!traceEnabled)
        m_client->client->setRecording(true);
    m_client->client->setRecording(false);
    checkTraceReceived();
    checkJsHeap();
}

void tst_QQmlProfilerService::pixmapCacheData()
{

    QCOMPARE(connect(true, "pixmapCacheTest.qml"), ConnectSuccess);

    // Don't wait for readyReadStandardOutput before the loop. It may have already arrived.
    while (m_process->output().indexOf(QLatin1String("image loaded")) == -1 &&
           m_process->output().indexOf(QLatin1String("image error")) == -1)
        QVERIFY(QQmlDebugTest::waitForSignal(m_process, SIGNAL(readyReadStandardOutput())));

    m_client->client->setRecording(false);

    checkTraceReceived();
    checkJsHeap();

    auto createType = [](QQmlProfilerDefinitions::PixmapEventType type) {
        return QQmlProfilerEventType(QQmlProfilerDefinitions::PixmapCacheEvent,
                                     QQmlProfilerDefinitions::MaximumRangeType, type);
    };

    QVector<qint64> numbers;

    // image starting to load
    VERIFY(MessageListPixmap, 0, createType(QQmlProfilerDefinitions::PixmapLoadingStarted),
           CheckMessageType | CheckDetailType, numbers);

    // image size
    numbers = QVector<qint64>({2, 2, 1});
    VERIFY(MessageListPixmap, 1, createType(QQmlProfilerDefinitions::PixmapSizeKnown),
           CheckMessageType | CheckDetailType | CheckNumbers, numbers);

    // image loaded
    VERIFY(MessageListPixmap, 2, createType(QQmlProfilerDefinitions::PixmapLoadingFinished),
           CheckMessageType | CheckDetailType, numbers);

    // cache size
    VERIFY(MessageListPixmap, 3, createType(QQmlProfilerDefinitions::PixmapCacheCountChanged),
           CheckMessageType | CheckDetailType, numbers);
}

void tst_QQmlProfilerService::scenegraphData()
{
    QCOMPARE(connect(true, "scenegraphTest.qml"), ConnectSuccess);

    while (!m_process->output().contains(QLatin1String("tick")))
        QVERIFY(QQmlDebugTest::waitForSignal(m_process, SIGNAL(readyReadStandardOutput())));
    m_client->client->setRecording(false);

    checkTraceReceived();
    checkJsHeap();

    // Check that at least one frame was rendered.
    // There should be a SGContextFrame + SGRendererFrame + SGRenderLoopFrame sequence,
    // but we can't be sure to get the SGRenderLoopFrame in the threaded renderer.
    //
    // Since the rendering happens in a different thread, there could be other unrelated events
    // interleaved. Also, events could carry the same time stamps and be sorted in an unexpected way
    // if the clocks are acting up.
    qint64 contextFrameTime = -1;
    qint64 renderFrameTime = -1;
#if QT_CONFIG(opengl) //Software renderer doesn't have context frames
    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL)) {
        foreach (const QQmlProfilerEvent &msg, m_client->asynchronousMessages) {
            const QQmlProfilerEventType &type = m_client->types.at(msg.typeIndex());
            if (type.message() == QQmlProfilerDefinitions::SceneGraphFrame) {
                if (type.detailType() == QQmlProfilerDefinitions::SceneGraphContextFrame) {
                    contextFrameTime = msg.timestamp();
                    break;
                }
            }
        }

        QVERIFY(contextFrameTime != -1);
    }
#endif
    foreach (const QQmlProfilerEvent &msg, m_client->asynchronousMessages) {
        const QQmlProfilerEventType &type = m_client->types.at(msg.typeIndex());
        if (type.detailType() == QQmlProfilerDefinitions::SceneGraphRendererFrame) {
            QVERIFY(msg.timestamp() >= contextFrameTime);
            renderFrameTime = msg.timestamp();
            break;
        }
    }

    QVERIFY(renderFrameTime != -1);

    foreach (const QQmlProfilerEvent &msg, m_client->asynchronousMessages) {
        const QQmlProfilerEventType &type = m_client->types.at(msg.typeIndex());
        if (type.detailType() == QQmlProfilerDefinitions::SceneGraphRenderLoopFrame) {
            if (msg.timestamp() >= contextFrameTime) {
                // Make sure SceneGraphRenderLoopFrame is not between SceneGraphContextFrame and
                // SceneGraphRendererFrame. A SceneGraphRenderLoopFrame before everything else is
                // OK as the scene graph might decide to do an initial rendering.
                QVERIFY(msg.timestamp() >= renderFrameTime);
                break;
            }
        }
    }
}

void tst_QQmlProfilerService::profileOnExit()
{
    connect(true, "exit.qml");
    checkProcessTerminated();

    checkTraceReceived();
    checkJsHeap();
}

void tst_QQmlProfilerService::controlFromJS()
{
    QCOMPARE(connect(true, "controlFromJS.qml", false), ConnectSuccess);

    m_client->client->setRecording(false);
    checkTraceReceived();
    checkJsHeap();
}

void tst_QQmlProfilerService::signalSourceLocation()
{
    QCOMPARE(connect(true, "signalSourceLocation.qml"), ConnectSuccess);

    while (!(m_process->output().contains(QLatin1String("500"))))
        QVERIFY(QQmlDebugTest::waitForSignal(m_process, SIGNAL(readyReadStandardOutput())));
    m_client->client->setRecording(false);
    checkTraceReceived();
    checkJsHeap();

    auto createType = [](int line, int column) {
        return QQmlProfilerEventType(
                    QQmlProfilerDefinitions::MaximumMessage,
                    QQmlProfilerDefinitions::HandlingSignal, -1,
                    QQmlProfilerEventLocation(QLatin1String("signalSourceLocation.qml"), line,
                                              column));
    };

    VERIFY(MessageListQML, 4, createType(8, 28), CheckType | CheckNumbers, m_rangeStart);
    VERIFY(MessageListQML, 6, createType(7, 21), CheckType | CheckNumbers, m_rangeEnd);
}

void tst_QQmlProfilerService::javascript()
{
    QCOMPARE(connect(true, "javascript.qml"), ConnectSuccess);

    while (!(m_process->output().contains(QLatin1String("done"))))
        QVERIFY(QQmlDebugTest::waitForSignal(m_process, SIGNAL(readyReadStandardOutput())));
    m_client->client->setRecording(false);
    checkTraceReceived();
    checkJsHeap();

    VERIFY(MessageListJavaScript, 2, QQmlProfilerEventType(QQmlProfilerDefinitions::MaximumMessage,
                                                           QQmlProfilerDefinitions::Javascript),
           CheckMessageType | CheckDetailType | CheckNumbers, m_rangeStart);

    VERIFY(MessageListJavaScript, 3,
           QQmlProfilerEventType(QQmlProfilerDefinitions::MaximumMessage,
                         QQmlProfilerDefinitions::Javascript, -1,
                         QQmlProfilerEventLocation(QLatin1String("javascript.qml"), 4, 5)),
           CheckType | CheckNumbers, m_rangeStart);

    VERIFY(MessageListJavaScript, 4, QQmlProfilerEventType(
               QQmlProfilerDefinitions::MaximumMessage, QQmlProfilerDefinitions::Javascript, -1,
               QQmlProfilerEventLocation(), QLatin1String("something")),
           CheckMessageType | CheckDetailType | CheckDataEndsWith | CheckNumbers, m_rangeStart);

    VERIFY(MessageListJavaScript, 10, QQmlProfilerEventType(QQmlProfilerDefinitions::MaximumMessage,
                                                            QQmlProfilerDefinitions::Javascript),
           CheckMessageType | CheckDetailType | CheckNumbers, m_rangeEnd);
}

void tst_QQmlProfilerService::flushInterval()
{
    QCOMPARE(connect(true, "timer.qml", true, 1), ConnectSuccess);

    // Make sure we get multiple messages
    QTRY_VERIFY(m_client->qmlMessages.length() > 0);
    QVERIFY(m_client->qmlMessages.length() < 100);
    QTRY_VERIFY(m_client->qmlMessages.length() > 100);

    m_client->client->setRecording(false);
    checkTraceReceived();
    checkJsHeap();
}

void tst_QQmlProfilerService::translationBinding()
{
    connect(true, "qstr.qml");
    checkProcessTerminated();

    checkTraceReceived();
    checkJsHeap();

    const QQmlProfilerEventType type(QQmlProfilerDefinitions::MaximumMessage,
                                     QQmlProfilerDefinitions::Binding);

    VERIFY(MessageListQML, 4, type, CheckDetailType | CheckMessageType | CheckNumbers,
           m_rangeStart);
    VERIFY(MessageListQML, 5, type, CheckDetailType | CheckMessageType | CheckNumbers,
           m_rangeEnd);
}

void tst_QQmlProfilerService::memory()
{
    connect(true, "memory.qml");
    checkProcessTerminated();

    checkTraceReceived();
    checkJsHeap();

    int smallItems = 0;
    for (auto message : m_client->jsHeapMessages) {
        const QQmlProfilerEventType &type = m_client->types[message.typeIndex()];
        if (type.detailType() == QV4::Profiling::SmallItem)
            ++smallItems;
    }

    QVERIFY(smallItems > 5);
}

QTEST_MAIN(tst_QQmlProfilerService)

#include "tst_qqmlprofilerservice.moc"
