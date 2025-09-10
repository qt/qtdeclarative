// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qpauseanimationjob_p.h"

QT_BEGIN_NAMESPACE

QPauseAnimationJob::QPauseAnimationJob(int duration)
    : m_duration(duration)
{
    m_isPause = true;
}

QPauseAnimationJob::~QPauseAnimationJob()
{
}

int QPauseAnimationJob::duration() const
{
    return m_duration;
}

void QPauseAnimationJob::setDuration(int msecs)
{
    m_duration = msecs;
}

void QPauseAnimationJob::updateCurrentTime(int)
{
}

void QPauseAnimationJob::debugAnimation(QDebug d) const
{
    d << "PauseAnimationJob(" << Qt::hex << (const void *) this << Qt::dec << ")" << "duration:" << m_duration;
}

QT_END_NAMESPACE
