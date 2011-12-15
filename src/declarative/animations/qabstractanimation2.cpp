/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qdebug.h>
#include <QtCore/qmath.h>
#include <QtCore/qthreadstorage.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qpointer.h>

#include <QtCore/private/qabstractanimation_p.h>
#include "private/qabstractanimation2_p.h"
#include "private/qanimationgroup2_p.h"

#define DEFAULT_TIMER_INTERVAL 16

QT_BEGIN_NAMESPACE

#ifndef QT_NO_THREAD
Q_GLOBAL_STATIC(QThreadStorage<QDeclarativeAnimationTimer *>, animationTimer)
#endif

QDeclarativeAnimationTimer::QDeclarativeAnimationTimer() :
    QAbstractAnimationTimer(), lastTick(0), lastDelta(0),
    currentAnimationIdx(0), insideTick(false),
    startAnimationPending(false), stopTimerPending(false),
    runningLeafAnimations(0)
{
}

QDeclarativeAnimationTimer *QDeclarativeAnimationTimer::instance(bool create)
{
    QDeclarativeAnimationTimer *inst;
#ifndef QT_NO_THREAD
    if (create && !animationTimer()->hasLocalData()) {
        inst = new QDeclarativeAnimationTimer;
        animationTimer()->setLocalData(inst);
    } else {
        inst = animationTimer() ? animationTimer()->localData() : 0;
    }
#else
    static QAnimationTimer unifiedTimer;
    inst = &unifiedTimer;
#endif
    return inst;
}

QDeclarativeAnimationTimer *QDeclarativeAnimationTimer::instance()
{
    return instance(true);
}

void QDeclarativeAnimationTimer::ensureTimerUpdate()
{
    QDeclarativeAnimationTimer *inst = QDeclarativeAnimationTimer::instance(false);
    QUnifiedTimer *instU = QUnifiedTimer::instance(false);
    if (instU && inst && inst->isPaused)
        instU->updateAnimationTimers(-1);
}

void QDeclarativeAnimationTimer::updateAnimationsTime(qint64 delta)
{
    //setCurrentTime can get this called again while we're the for loop. At least with pauseAnimations
    if (insideTick)
        return;

    lastTick += delta;
    lastDelta = delta;

    //we make sure we only call update time if the time has actually changed
    //it might happen in some cases that the time doesn't change because events are delayed
    //when the CPU load is high
    if (delta) {
        insideTick = true;
        for (currentAnimationIdx = 0; currentAnimationIdx < animations.count(); ++currentAnimationIdx) {
            QAbstractAnimation2 *animation = animations.at(currentAnimationIdx);
            int elapsed = animation->m_totalCurrentTime
                          + (animation->direction() == QAbstractAnimation2::Forward ? delta : -delta);
            animation->setCurrentTime(elapsed);
        }
        insideTick = false;
        currentAnimationIdx = 0;
    }
}

void QDeclarativeAnimationTimer::updateAnimationTimer()
{
    QDeclarativeAnimationTimer *inst = QDeclarativeAnimationTimer::instance(false);
    if (inst)
        inst->restartAnimationTimer();
}

void QDeclarativeAnimationTimer::restartAnimationTimer()
{
    if (runningLeafAnimations == 0 && !runningPauseAnimations.isEmpty())
        QUnifiedTimer::pauseAnimationTimer(this, closestPauseAnimationTimeToFinish());
    else if (isPaused)
        QUnifiedTimer::resumeAnimationTimer(this);
    else if (!isRegistered)
        QUnifiedTimer::startAnimationTimer(this);
}

void QDeclarativeAnimationTimer::startAnimations()
{
    startAnimationPending = false;
    //force timer to update, which prevents large deltas for our newly added animations
    if (!animations.isEmpty())
        QUnifiedTimer::instance()->updateAnimationTimers(-1);

    //we transfer the waiting animations into the "really running" state
    animations += animationsToStart;
    animationsToStart.clear();
    if (!animations.isEmpty())
        restartAnimationTimer();
}

void QDeclarativeAnimationTimer::stopTimer()
{
    stopTimerPending = false;
    if (animations.isEmpty()) {
        QUnifiedTimer::resumeAnimationTimer(this);
        QUnifiedTimer::stopAnimationTimer(this);
        // invalidate the start reference time
        lastTick = 0;
        lastDelta = 0;
    }
}

void QDeclarativeAnimationTimer::registerAnimation(QAbstractAnimation2 *animation, bool isTopLevel)
{
    QDeclarativeAnimationTimer *inst = instance(true); //we create the instance if needed
    inst->registerRunningAnimation(animation);
    if (isTopLevel) {
        Q_ASSERT(!animation->m_hasRegisteredTimer);
        animation->m_hasRegisteredTimer = true;
        inst->animationsToStart << animation;
        if (!inst->startAnimationPending) {
            inst->startAnimationPending = true;
            QMetaObject::invokeMethod(inst, "startAnimations", Qt::QueuedConnection);
        }
    }
}

void QDeclarativeAnimationTimer::unregisterAnimation(QAbstractAnimation2 *animation)
{
    QDeclarativeAnimationTimer *inst = QDeclarativeAnimationTimer::instance(false);
    if (inst) {
        //at this point the unified timer should have been created
        //but it might also have been already destroyed in case the application is shutting down

        inst->unregisterRunningAnimation(animation);

        if (!animation->m_hasRegisteredTimer)
            return;

        int idx = inst->animations.indexOf(animation);
        if (idx != -1) {
            inst->animations.removeAt(idx);
            // this is needed if we unregister an animation while its running
            if (idx <= inst->currentAnimationIdx)
                --inst->currentAnimationIdx;

            if (inst->animations.isEmpty() && !inst->stopTimerPending) {
                inst->stopTimerPending = true;
                QMetaObject::invokeMethod(inst, "stopTimer", Qt::QueuedConnection);
            }
        } else {
            inst->animationsToStart.removeOne(animation);
        }
    }
    animation->m_hasRegisteredTimer = false;
}

void QDeclarativeAnimationTimer::registerRunningAnimation(QAbstractAnimation2 *animation)
{
    if (animation->m_isGroup)
        return;

    if (animation->m_isPause) {
        runningPauseAnimations << animation;
    } else
        runningLeafAnimations++;
}

void QDeclarativeAnimationTimer::unregisterRunningAnimation(QAbstractAnimation2 *animation)
{
    if (animation->m_isGroup)
        return;

    if (animation->m_isPause)
        runningPauseAnimations.removeOne(animation);
    else
        runningLeafAnimations--;
    Q_ASSERT(runningLeafAnimations >= 0);
}

int QDeclarativeAnimationTimer::closestPauseAnimationTimeToFinish()
{
    int closestTimeToFinish = INT_MAX;
    for (int i = 0; i < runningPauseAnimations.size(); ++i) {
        QAbstractAnimation2 *animation = runningPauseAnimations.at(i);
        int timeToFinish;

        if (animation->direction() == QAbstractAnimation2::Forward)
            timeToFinish = animation->duration() - animation->currentLoopTime();
        else
            timeToFinish = animation->currentLoopTime();

        if (timeToFinish < closestTimeToFinish)
            closestTimeToFinish = timeToFinish;
    }
    return closestTimeToFinish;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

QAbstractAnimation2::QAbstractAnimation2()
    : m_isPause(false)
    , m_isGroup(false)
    , m_loopCount(1)
    , m_group(0)
    , m_direction(QAbstractAnimation2::Forward)
    , m_state(QAbstractAnimation2::Stopped)
    , m_totalCurrentTime(0)
    , m_currentTime(0)
    , m_currentLoop(0)
    , m_hasRegisteredTimer(false)
    , m_uncontrolledFinishTime(-1)
    , m_nextSibling(0)
    , m_previousSibling(0)
{
}

QAbstractAnimation2::~QAbstractAnimation2()
{
    //we can't call stop here. Otherwise we get pure virtual calls
    if (m_state != Stopped) {
        State oldState = m_state;
        m_state = Stopped;
        stateChanged(oldState, m_state);
        if (oldState == Running)
            QDeclarativeAnimationTimer::unregisterAnimation(this);
    }

    if (m_group)
        m_group->removeAnimation(this);
}

void QAbstractAnimation2::setState(QAbstractAnimation2::State newState)
{
    if (m_state == newState)
        return;

    if (m_loopCount == 0)
        return;

    State oldState = m_state;
    int oldCurrentTime = m_currentTime;
    int oldCurrentLoop = m_currentLoop;
    Direction oldDirection = m_direction;

    // check if we should Rewind
    if ((newState == Paused || newState == Running) && oldState == Stopped) {
        //here we reset the time if needed
        //we don't call setCurrentTime because this might change the way the animation
        //behaves: changing the state or changing the current value
        m_totalCurrentTime = m_currentTime = (m_direction == Forward) ?
            0 : (m_loopCount == -1 ? duration() : totalDuration());
    }

    m_state = newState;
    //(un)registration of the animation must always happen before calls to
    //virtual function (updateState) to ensure a correct state of the timer
    bool isTopLevel = !m_group || m_group->isStopped();
    if (oldState == Running) {
        if (newState == Paused && m_hasRegisteredTimer)
            QDeclarativeAnimationTimer::ensureTimerUpdate();
        //the animation, is not running any more
        QDeclarativeAnimationTimer::unregisterAnimation(this);
    } else if (newState == Running) {
        QDeclarativeAnimationTimer::registerAnimation(this, isTopLevel);
    }

    //starting an animation qualifies as a top level loop change
    if (newState == Running && oldState == Stopped && !m_group)
        topLevelAnimationLoopChanged();

    updateState(newState, oldState);
    if (newState != m_state) //this is to be safe if updateState changes the state
        return;

    // Notify state change
    stateChanged(newState, oldState);
    if (newState != m_state) //this is to be safe if updateState changes the state
        return;

    switch (m_state) {
    case Paused:
        break;
    case Running:
        {
            // this ensures that the value is updated now that the animation is running
            if (oldState == Stopped) {
                if (isTopLevel) {
                    // currentTime needs to be updated if pauseTimer is active
                    QDeclarativeAnimationTimer::ensureTimerUpdate();
                    setCurrentTime(m_totalCurrentTime);
                }
            }
        }
        break;
    case Stopped:
        // Leave running state.
        int dura = duration();

        if (dura == -1 || m_loopCount < 0
            || (oldDirection == Forward && (oldCurrentTime * (oldCurrentLoop + 1)) == (dura * m_loopCount))
            || (oldDirection == Backward && oldCurrentTime == 0)) {
               finished();
        }
        break;
    }
}

void QAbstractAnimation2::setDirection(Direction direction)
{
    if (m_direction == direction)
        return;

    if (m_state == Stopped) {
        if (m_direction == Backward) {
            m_currentTime = duration();
            m_currentLoop = m_loopCount - 1;
        } else {
            m_currentTime = 0;
            m_currentLoop = 0;
        }
    }

    // the commands order below is important: first we need to setCurrentTime with the old direction,
    // then update the direction on this and all children and finally restart the pauseTimer if needed
    if (m_hasRegisteredTimer)
        QDeclarativeAnimationTimer::ensureTimerUpdate();

    m_direction = direction;
    updateDirection(direction);

    if (m_hasRegisteredTimer)
        // needed to update the timer interval in case of a pause animation
        QDeclarativeAnimationTimer::updateAnimationTimer();
}

void QAbstractAnimation2::setLoopCount(int loopCount)
{
    m_loopCount = loopCount;
}

int QAbstractAnimation2::totalDuration() const
{
    int dura = duration();
    if (dura <= 0)
        return dura;
    int loopcount = loopCount();
    if (loopcount < 0)
        return -1;
    return dura * loopcount;
}

void QAbstractAnimation2::setCurrentTime(int msecs)
{
    msecs = qMax(msecs, 0);
    // Calculate new time and loop.
    int dura = duration();
    int totalDura = dura <= 0 ? dura : ((m_loopCount < 0) ? -1 : dura * m_loopCount);
    if (totalDura != -1)
        msecs = qMin(totalDura, msecs);
    m_totalCurrentTime = msecs;

    // Update new values.
    int oldLoop = m_currentLoop;
    m_currentLoop = ((dura <= 0) ? 0 : (msecs / dura));
    if (m_currentLoop == m_loopCount) {
        //we're at the end
        m_currentTime = qMax(0, dura);
        m_currentLoop = qMax(0, m_loopCount - 1);
    } else {
        if (m_direction == Forward) {
            m_currentTime = (dura <= 0) ? msecs : (msecs % dura);
        } else {
            m_currentTime = (dura <= 0) ? msecs : ((msecs - 1) % dura) + 1;
            if (m_currentTime == dura)
                --m_currentLoop;
        }
    }

    if (m_currentLoop != oldLoop && !m_group)   //### verify Running as well?
        topLevelAnimationLoopChanged();

    updateCurrentTime(m_currentTime);

    if (m_currentLoop != oldLoop)
        currentLoopChanged(m_currentLoop);

    // All animations are responsible for stopping the animation when their
    // own end state is reached; in this case the animation is time driven,
    // and has reached the end.
    if ((m_direction == Forward && m_totalCurrentTime == totalDura)
        || (m_direction == Backward && m_totalCurrentTime == 0)) {
        stop();
    }
}

void QAbstractAnimation2::start()
{
    if (m_state == Running)
        return;
    setState(Running);
}

void QAbstractAnimation2::stop()
{
    if (m_state == Stopped)
        return;
    setState(Stopped);
}

void QAbstractAnimation2::pause()
{
    if (m_state == Stopped) {
        qWarning("QAbstractAnimation2::pause: Cannot pause a stopped animation");
        return;
    }

    setState(Paused);
}

void QAbstractAnimation2::resume()
{
    if (m_state != Paused) {
        qWarning("QAbstractAnimation2::resume: "
                 "Cannot resume an animation that is not paused");
        return;
    }
    setState(Running);
}

void QAbstractAnimation2::updateState(QAbstractAnimation2::State newState,
                                     QAbstractAnimation2::State oldState)
{
    Q_UNUSED(oldState);
    Q_UNUSED(newState);
}

void QAbstractAnimation2::updateDirection(QAbstractAnimation2::Direction direction)
{
    Q_UNUSED(direction);
}

void QAbstractAnimation2::finished()
{
    for (int i = 0; i < changeListeners.count(); ++i) {
        const QAbstractAnimation2::ChangeListener &change = changeListeners.at(i);
        if (change.types & QAbstractAnimation2::Completion)
            change.listener->animationFinished(this);
    }

    if (m_group && (duration() == -1 || loopCount() < 0)) {
        //this is an uncontrolled animation, need to notify the group animation we are finished
        m_group->uncontrolledAnimationFinished(this);
    }
}

void QAbstractAnimation2::stateChanged(QAbstractAnimation2::State newState, QAbstractAnimation2::State oldState)
{
    for (int i = 0; i < changeListeners.count(); ++i) {
        const QAbstractAnimation2::ChangeListener &change = changeListeners.at(i);
        if (change.types & QAbstractAnimation2::StateChange)
            change.listener->animationStateChanged(this, newState, oldState);
    }
}

void QAbstractAnimation2::currentLoopChanged(int currentLoop)
{
    Q_UNUSED(currentLoop);
    for (int i = 0; i < changeListeners.count(); ++i) {
        const QAbstractAnimation2::ChangeListener &change = changeListeners.at(i);
        if (change.types & QAbstractAnimation2::CurrentLoop)
            change.listener->animationCurrentLoopChanged(this);
    }
}

void QAbstractAnimation2::addAnimationChangeListener(QAnimation2ChangeListener *listener, QAbstractAnimation2::ChangeTypes changes)
{
    changeListeners.append(ChangeListener(listener, changes));
}

void QAbstractAnimation2::removeAnimationChangeListener(QAnimation2ChangeListener *listener, QAbstractAnimation2::ChangeTypes changes)
{
    changeListeners.removeOne(ChangeListener(listener, changes));
}


QT_END_NAMESPACE

#include "moc_qabstractanimation2_p.cpp"
