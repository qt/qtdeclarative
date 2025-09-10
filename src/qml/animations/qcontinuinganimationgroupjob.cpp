// Copyright (C) 2016 Jolla Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qcontinuinganimationgroupjob_p.h"
#include "private/qanimationjobutil_p.h"

QT_BEGIN_NAMESPACE

QContinuingAnimationGroupJob::QContinuingAnimationGroupJob()
{
}

QContinuingAnimationGroupJob::~QContinuingAnimationGroupJob()
{
}

void QContinuingAnimationGroupJob::updateCurrentTime(int /*currentTime*/)
{
    Q_ASSERT(!m_children.isEmpty());
    for (QAbstractAnimationJob *animation : m_children) {
        if (animation->state() == state()) {
            RETURN_IF_DELETED(animation->setCurrentTime(m_currentTime));
        }
    }
}

void QContinuingAnimationGroupJob::updateState(QAbstractAnimationJob::State newState,
                                          QAbstractAnimationJob::State oldState)
{
    QAnimationGroupJob::updateState(newState, oldState);

    switch (newState) {
    case Stopped:
        for (QAbstractAnimationJob *animation  : m_children)
            animation->stop();
        break;
    case Paused:
        for (QAbstractAnimationJob *animation  : m_children)
            if (animation->isRunning())
                animation->pause();
        break;
    case Running:
        if (m_children.isEmpty()) {
            stop();
            return;
        }
        for (QAbstractAnimationJob *animation  : m_children) {
            RETURN_IF_DELETED(resetUncontrolledAnimationFinishTime(animation));
            animation->setDirection(m_direction);
            RETURN_IF_DELETED(animation->start());
        }
        break;
    }
}

void QContinuingAnimationGroupJob::updateDirection(QAbstractAnimationJob::Direction direction)
{
    if (!isStopped()) {
        for (QAbstractAnimationJob *animation  : m_children)
            animation->setDirection(direction);
    }
}

void QContinuingAnimationGroupJob::uncontrolledAnimationFinished(QAbstractAnimationJob *animation)
{
    Q_ASSERT(animation && (animation->duration() == -1));
    int uncontrolledRunningCount = 0;

    for (QAbstractAnimationJob *child : m_children) {
        if (child == animation)
            setUncontrolledAnimationFinishTime(animation, animation->currentTime());
        else if (uncontrolledAnimationFinishTime(child) == -1)
            ++uncontrolledRunningCount;
    }

    if (uncontrolledRunningCount > 0)
        return;

    setUncontrolledAnimationFinishTime(this, currentTime());
    stop();
}

void QContinuingAnimationGroupJob::debugAnimation(QDebug d) const
{
    d << "ContinuingAnimationGroupJob(" << Qt::hex << (const void *) this << Qt::dec << ")";

    debugChildren(d);
}

QT_END_NAMESPACE

