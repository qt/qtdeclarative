// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "timemodel.h"

void TimeModel::timerEvent(QTimerEvent *)
{
    const QTime now = QTime::currentTime();
    if (now.second() == 59
            && now.minute() == m_time.value().minute()
            && now.hour() == m_time.value().hour()) {
        // just missed time tick over, force it, wait extra 0.5 seconds
        timer.start(60500, this);
    } else {
        timer.start(60000 - m_time.value().second() * 1000, this);
    }
    m_time = now;
}

TimeModel::TimeModel(QObject *parent)
    : QObject(parent)
    , m_time(QTime::currentTime())
{
    m_minute.setBinding([this]() { return m_time.value().minute(); });
    m_hour.setBinding([this]() { return m_time.value().hour(); });
    timer.start(60000 - m_time.value().second() * 1000, this);
}
