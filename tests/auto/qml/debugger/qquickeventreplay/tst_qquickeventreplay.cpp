// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qqmldebugprocess_p.h>
#include <debugutil_p.h>

#include <private/qqmldebugconnection_p.h>
#include <private/qquickeventreplayclient_p.h>
#include <private/qqmlprofilerclient_p.h>

#include <QtCore/qlibraryinfo.h>
#include <QtCore/qpointer.h>

class EventReceiver : public QQmlProfilerEventReceiver
{
public:
    qsizetype numLoadedEventTypes() const final { return m_eventTypes.size(); }
    qsizetype numLoadedEvents() const final { return m_events.size(); }
    void addEventType(const QQmlProfilerEventType &type) final;
    void addEvent(const QQmlProfilerEvent &event) final;
    void clear() final;
    void clearAll();

    QList<QQmlProfilerEventType> m_eventTypes;
    QList<QQmlProfilerEvent> m_events;
};

void EventReceiver::addEventType(const QQmlProfilerEventType &type)
{
    m_eventTypes.append(type);
}

void EventReceiver::addEvent(const QQmlProfilerEvent &event)
{
    const QQmlProfilerEventType &type = m_eventTypes[event.typeIndex()];
    switch (type.message()) {
    case Event:
        switch (type.detailType()) {
        case Mouse:
        case Key:
            m_events.append(event);
            break;
        default:
            QFAIL("Only input events should be recorded");
            break;
        }
        break;
    default:
        QFAIL("Only input events should be recorded");
        break;
    }
}

void EventReceiver::clear()
{
    m_events.clear();
}

void EventReceiver::clearAll()
{
    m_events.clear();
    m_eventTypes.clear();
}

class tst_QQuickEventReplay : public QQmlDebugTest
{
    Q_OBJECT
public:
    tst_QQuickEventReplay();

private Q_SLOTS:
    void receiveEvent();
    void replayEvent();

private:
    ConnectResult startQmlProcess(
            const QString &qmlFile, QStringList environmentVariables = QStringList());
    QList<QQmlDebugClient *> createClients() final;

    EventReceiver m_receiver;
    QPointer<QQmlProfilerClient> m_profilerClient;
    QPointer<QQuickEventReplayClient> m_replayClient;
};

tst_QQuickEventReplay::tst_QQuickEventReplay()
    : QQmlDebugTest(QT_QMLTEST_DATADIR)
{
}

QQmlDebugTest::ConnectResult tst_QQuickEventReplay::startQmlProcess(
        const QString &qmlFile, QStringList environmentVariables)
{
    return QQmlDebugTest::connectTo(
            QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/qml",
            QStringLiteral("EventReplay,CanvasFrameRate,EngineControl"), testFile(qmlFile), true,
            environmentVariables);
}

QList<QQmlDebugClient *> tst_QQuickEventReplay::createClients()
{
    m_receiver.clearAll();
    m_replayClient = new QQuickEventReplayClient(m_connection);
    m_profilerClient = new QQmlProfilerClient(m_connection, &m_receiver, 1 << ProfileInputEvents);
    m_profilerClient->setRecording(true);
    return QList<QQmlDebugClient *>({m_replayClient, m_profilerClient});
}

void tst_QQuickEventReplay::receiveEvent()
{
    startQmlProcess("record.qml");
    QTRY_VERIFY(m_process->output().contains("clicked"));
    m_profilerClient->setRecording(false);
    QTRY_COMPARE(m_receiver.numLoadedEvents(), 5);

    // Reset the timestamps so that we can compare them
    for (auto &event : m_receiver.m_events)
        event.setTimestamp(0);

    QCOMPARE(m_receiver.m_eventTypes, QList<QQmlProfilerEventType>({
        { Event, MaximumRangeType, Key, QQmlProfilerEventLocation(), QString(), QString() },
        { Event, MaximumRangeType, Mouse, QQmlProfilerEventLocation(), QString(), QString() },
    }));

    QCOMPARE(m_receiver.m_events, QList<QQmlProfilerEvent>({
        { 0ll, 0, QList<int>({InputKeyPress, Qt::Key_Q, Qt::NoModifier}) },
        { 0ll, 0, QList<int>({InputKeyRelease, Qt::Key_Q, Qt::NoModifier}) },
        { 0ll, 1, QList<int>({InputMouseMove, 12, 13}) },
        { 0ll, 1, QList<int>({InputMousePress, Qt::LeftButton, Qt::LeftButton}) },
        { 0ll, 1, QList<int>({InputMouseRelease, Qt::LeftButton, Qt::NoButton}) },
    }));
}

void tst_QQuickEventReplay::replayEvent()
{
    startQmlProcess("replay.qml");
    QTRY_VERIFY(m_process->output().contains("window active"));

    const QQmlProfilerEventType mouseType {Event, MaximumRangeType, Mouse};
    m_replayClient->sendEvent(mouseType, {
        0ll, 0, QList<int>({InputMouseMove, 12, 13})
    });
    m_replayClient->sendEvent(mouseType, {
        1ll, 0, QList<int>({InputMousePress, Qt::LeftButton, Qt::LeftButton})
    });
    m_replayClient->sendEvent(mouseType, {
        2ll, 0, QList<int>({InputMouseRelease, Qt::LeftButton, Qt::NoButton})
    });

    QTRY_VERIFY(m_process->output().contains("mouse 12 13"));

    const QQmlProfilerEventType keyType {Event, MaximumRangeType, Key};
    m_replayClient->sendEvent(keyType, {
        3ll, 0, QList<int>({InputKeyPress, Qt::Key_2, Qt::NoModifier})
    });
    m_replayClient->sendEvent(keyType, {
        4ll, 0, QList<int>({InputKeyRelease, Qt::Key_2, Qt::NoModifier})
    });

    QTRY_VERIFY(m_process->output().contains(
            QLatin1String("key %1 pressed").arg(QString::number(Qt::Key_2))));
    QTRY_VERIFY(m_process->output().contains(
            QLatin1String("key %1 released").arg(QString::number(Qt::Key_2))));
}


QTEST_MAIN(tst_QQuickEventReplay)

#include "tst_qquickeventreplay.moc"
