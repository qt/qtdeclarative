// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef EVENTCAPTURER_H
#define EVENTCAPTURER_H

#include <QObject>
#include <QElapsedTimer>
#include <QEvent>
#include <QList>
#include <QPoint>
#include <QSet>

#include "capturedevent.h"

class EventCapturer : public QObject
{
    Q_OBJECT

public:
    EventCapturer(QObject *parent = nullptr);

    enum MoveEventTrimFlag
    {
        TrimNone = 0x0,
        TrimLeading = 0x1,
        TrimTrailing = 0x2,
        TrimAfterReleases = 0x4,
        TrimAll = TrimLeading | TrimTrailing | TrimAfterReleases
    };

    Q_DECLARE_FLAGS(MoveEventTrimFlags, MoveEventTrimFlag)

    void setStopCaptureKey(Qt::Key stopCaptureKey);
    void setMoveEventTrimFlags(MoveEventTrimFlags trimFlags);

    void startCapturing(QObject *eventSource, int duration);

    QSet<QEvent::Type> capturedEventTypes();
    void setCapturedEventTypes(QSet<QEvent::Type> types);

    QList<CapturedEvent> capturedEvents() const;
protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private slots:
    void stopCapturing();

private:
    void captureEvent(const QEvent *event);

    QObject *mEventSource;
    QSet<QEvent::Type> mCapturedEventTypes;
    Qt::Key mStopCaptureKey;
    MoveEventTrimFlags mMoveEventTrimFlags;
    QElapsedTimer mDelayTimer;
    QList<CapturedEvent> mEvents;
    int mDuration;
    int mLastCaptureTime;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(EventCapturer::MoveEventTrimFlags)

#endif // EVENTCAPTURER_H
