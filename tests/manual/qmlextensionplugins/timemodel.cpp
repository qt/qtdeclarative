// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "timemodel.h"

int TimeModel::instances=0;
MinuteTimer *TimeModel::timer=nullptr;

void MinuteTimer::start()
{
    if (!timer.isActive()) {
        time = QTime::currentTime();
        timer.start(60000-time.second()*1000, this);
    }
}

void MinuteTimer::stop()
{
    timer.stop();
}

void MinuteTimer::timerEvent(QTimerEvent *)
{
    QTime now = QTime::currentTime();
    if (now.second() == 59 && now.minute() == time.minute() && now.hour() == time.hour()) {
        // just missed time tick over, force it, wait extra 0.5 seconds
        time = time.addSecs(60);
        timer.start(60500, this);
    } else {
        time = now;
        timer.start(60000-time.second()*1000, this);
    }
    emit timeChanged();
}

TimeModel::TimeModel(QObject *parent) : QObject(parent)
{
    if (++instances == 1) {
        if (!timer)
            timer = new MinuteTimer(QCoreApplication::instance());
        connect(timer, &MinuteTimer::timeChanged, this, &TimeModel::timeChanged);
        timer->start();
    }
}

TimeModel::~TimeModel()
{
    if (--instances == 0) {
        timer->stop();
    }
}
