/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "private/qparallelanimationgroupjob_p.h"
#include "private/qanimationjobutil_p.h"

QT_BEGIN_NAMESPACE

QParallelAnimationGroupJob::QParallelAnimationGroupJob()
    : QAnimationGroupJob()
    , m_previousLoop(0)
    , m_previousCurrentTime(0)
{
}

QParallelAnimationGroupJob::~QParallelAnimationGroupJob()
{
}

int QParallelAnimationGroupJob::duration() const
{
    int ret = 0;

    for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling()) {
        int currentDuration = animation->totalDuration();
        if (currentDuration == -1)
            return -1; // Undetermined length
        ret = qMax(ret, currentDuration);
    }

    return ret;
}

void QParallelAnimationGroupJob::updateCurrentTime(int /*currentTime*/)
{
    if (!firstChild())
        return;

    if (m_currentLoop > m_previousLoop) {
        // simulate completion of the loop
        int dura = duration();
        if (dura < 0) {
            // For an uncontrolled parallel group, we need to simulate the end of running animations.
            // As uncontrolled animation finish time is already reset for this next loop, we pick the
            // longest of the known stop times.
            for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling()) {
                int currentDuration = animation->totalDuration();
                if (currentDuration >= 0)
                    dura = qMax(dura, currentDuration);
            }
        }
        if (dura > 0) {
            for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling()) {
                if (!animation->isStopped())
                    RETURN_IF_DELETED(animation->setCurrentTime(dura));   // will stop
            }
        }
    } else if (m_currentLoop < m_previousLoop) {
        // simulate completion of the loop seeking backwards
        for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling()) {
            //we need to make sure the animation is in the right state
            //and then rewind it
            applyGroupState(animation);
            RETURN_IF_DELETED(animation->setCurrentTime(0));
            animation->stop();
        }
    }

    // finally move into the actual time of the current loop
    for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling()) {
        const int dura = animation->totalDuration();
        //if the loopcount is bigger we should always start all animations
        if (m_currentLoop > m_previousLoop
            //if we're at the end of the animation, we need to start it if it wasn't already started in this loop
            //this happens in Backward direction where not all animations are started at the same time
            || shouldAnimationStart(animation, m_previousCurrentTime > dura /*startIfAtEnd*/)) {
            applyGroupState(animation);
        }

        if (animation->state() == state()) {
            RETURN_IF_DELETED(animation->setCurrentTime(m_currentTime));
            if (dura > 0 && m_currentTime > dura)
                animation->stop();
        }
    }
    m_previousLoop = m_currentLoop;
    m_previousCurrentTime = m_currentTime;
}

void QParallelAnimationGroupJob::updateState(QAbstractAnimationJob::State newState,
                                          QAbstractAnimationJob::State oldState)
{
    QAnimationGroupJob::updateState(newState, oldState);

    switch (newState) {
    case Stopped:
        for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling())
            animation->stop();
        break;
    case Paused:
        for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling())
            if (animation->isRunning())
                animation->pause();
        break;
    case Running:
        for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling()) {
            if (oldState == Stopped) {
                animation->stop();
                m_previousLoop = m_direction == Forward ? 0 : m_loopCount - 1;
            }
            resetUncontrolledAnimationFinishTime(animation);
            animation->setDirection(m_direction);
            if (shouldAnimationStart(animation, oldState == Stopped))
                animation->start();
        }
        break;
    }
}

bool QParallelAnimationGroupJob::shouldAnimationStart(QAbstractAnimationJob *animation, bool startIfAtEnd) const
{
    const int dura = animation->totalDuration();

    if (dura == -1)
        return uncontrolledAnimationFinishTime(animation) == -1;

    if (startIfAtEnd)
        return m_currentTime <= dura;
    if (m_direction == Forward)
        return m_currentTime < dura;
    else //direction == Backward
        return m_currentTime && m_currentTime <= dura;
}

void QParallelAnimationGroupJob::applyGroupState(QAbstractAnimationJob *animation)
{
    switch (m_state)
    {
    case Running:
        animation->start();
        break;
    case Paused:
        animation->pause();
        break;
    case Stopped:
    default:
        break;
    }
}

void QParallelAnimationGroupJob::updateDirection(QAbstractAnimationJob::Direction direction)
{
    //we need to update the direction of the current animation
    if (!isStopped()) {
        for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling()) {
            animation->setDirection(direction);
        }
    } else {
        if (direction == Forward) {
            m_previousLoop = 0;
            m_previousCurrentTime = 0;
        } else {
            // Looping backwards with loopCount == -1 does not really work well...
            m_previousLoop = (m_loopCount == -1 ? 0 : m_loopCount - 1);
            m_previousCurrentTime = duration();
        }
    }
}

void QParallelAnimationGroupJob::uncontrolledAnimationFinished(QAbstractAnimationJob *animation)
{
    Q_ASSERT(animation && (animation->duration() == -1 || animation->loopCount() < 0));
    int uncontrolledRunningCount = 0;

    for (QAbstractAnimationJob *child = firstChild(); child; child = child->nextSibling()) {
        if (child == animation) {
            setUncontrolledAnimationFinishTime(animation, animation->currentTime());
        } else if (child->duration() == -1 || child->loopCount() < 0) {
            if (uncontrolledAnimationFinishTime(child) == -1)
                ++uncontrolledRunningCount;
        }
    }

    if (uncontrolledRunningCount > 0)
        return;

    int maxDuration = 0;
    bool running = false;
    for (QAbstractAnimationJob *job = firstChild(); job; job = job->nextSibling()) {
        if (job->state() == Running)
            running = true;
        maxDuration = qMax(maxDuration, job->totalDuration());
    }

    setUncontrolledAnimationFinishTime(this, qMax(maxDuration + m_currentLoopStartTime, currentTime()));

    if (!running
            && ((m_direction == Forward && m_currentLoop == m_loopCount -1)
                || (m_direction == Backward && m_currentLoop == 0))) {
        stop();
    }
}

void QParallelAnimationGroupJob::debugAnimation(QDebug d) const
{
    d << "ParallelAnimationGroupJob(" << Qt::hex << (const void *) this << Qt::dec << ")";

    debugChildren(d);
}

QT_END_NAMESPACE

