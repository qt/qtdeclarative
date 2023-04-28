// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TIMEMODEL_H
#define TIMEMODEL_H

#include <QtQml/qqml.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qbasictimer.h>
#include <QtCore/qcoreapplication.h>

// Implements a "TimeModel" class with hour and minute properties
// that change on-the-minute yet efficiently sleep the rest
// of the time.

class MinuteTimer : public QObject
{
    Q_OBJECT
public:
    MinuteTimer(QObject *parent) : QObject(parent) {}

    void start();
    void stop();

    int hour() const { return time.hour(); }
    int minute() const { return time.minute(); }

signals:
    void timeChanged();

protected:
    void timerEvent(QTimerEvent *) override;

private:
    QTime time;
    QBasicTimer timer;
};

//![0]
class TimeModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int hour READ hour NOTIFY timeChanged)
    Q_PROPERTY(int minute READ minute NOTIFY timeChanged)
    QML_NAMED_ELEMENT(Time)
//![0]

public:
    TimeModel(QObject *parent=nullptr);
    ~TimeModel() override;

    int minute() const { return timer->minute(); }
    int hour() const { return timer->hour(); }

signals:
    void timeChanged();

private:
    QTime t;
    static MinuteTimer *timer;
    static int instances;
};

#endif // TIMEMODEL_H
