/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "private/qabstractanimation2_p.h"
#include "private/qanimationgroup2_p.h"
#include "private/qdeclarativeanimation_p.h"

#define DEFAULT_TIMER_INTERVAL 16
#define STARTSTOP_TIMER_DELAY 0

QT_BEGIN_NAMESPACE

#ifndef QT_NO_THREAD
Q_GLOBAL_STATIC(QThreadStorage<QUnifiedTimer2 *>, unifiedTimer)
#endif

QUnifiedTimer2::QUnifiedTimer2() :
    QObject(), defaultDriver(this), lastTick(0), timingInterval(DEFAULT_TIMER_INTERVAL),
    currentAnimationIdx(0), insideTick(false), consistentTiming(false), slowMode(false),
    slowdownFactor(5.0f), isPauseTimerActive(false), runningLeafAnimations(0)
{
    time.invalidate();
    driver = &defaultDriver;
}


QUnifiedTimer2 *QUnifiedTimer2::instance(bool create)
{
    QUnifiedTimer2 *inst;
#ifndef QT_NO_THREAD
    if (create && !unifiedTimer()->hasLocalData()) {
        inst = new QUnifiedTimer2;
        unifiedTimer()->setLocalData(inst);
    } else {
        inst = unifiedTimer()->localData();
    }
#else
    static QUnifiedTimer2 unifiedTimer;
    inst = &unifiedTimer;
#endif
    return inst;
}

QUnifiedTimer2 *QUnifiedTimer2::instance()
{
    return instance(true);
}

void QUnifiedTimer2::ensureTimerUpdate()
{
    QUnifiedTimer2 *inst = QUnifiedTimer2::instance(false);
    if (inst && inst->isPauseTimerActive)
        inst->updateAnimationsTime(-1);
}

void QUnifiedTimer2::updateAnimationsTime(qint64 timeStep)
{
    //setCurrentTime can get this called again while we're the for loop. At least with pauseAnimations
    if(insideTick)
        return;

    qint64 totalElapsed = timeStep >= 0 ? timeStep : time.elapsed();

    // ignore consistentTiming in case the pause timer is active
    int delta = (consistentTiming && !isPauseTimerActive) ?
                        timingInterval : totalElapsed - lastTick;
    if (slowMode) {
        if (slowdownFactor > 0)
            delta = qRound(delta / slowdownFactor);
        else
            delta = 0;
    }

    lastTick = totalElapsed;

    //we make sure we only call update time if the time has actually changed
    //it might happen in some cases that the time doesn't change because events are delayed
    //when the CPU load is high
    if (delta) {
        insideTick = true;
        for (currentAnimationIdx = 0; currentAnimationIdx < animations.count(); ++currentAnimationIdx) {
            QAbstractAnimation2* animation = animations.at(currentAnimationIdx);
            int elapsed = animation->currentTime()
                          + (animation->direction() == QAbstractAnimation2::Forward ? delta : -delta);
            animation->setCurrentTime(elapsed);
        }
        insideTick = false;
        currentAnimationIdx = 0;
    }
}

void QUnifiedTimer2::updateAnimationTimer()
{
    QUnifiedTimer2 *inst = QUnifiedTimer2::instance(false);
    if (inst)
        inst->restartAnimationTimer();
}

void QUnifiedTimer2::restartAnimationTimer()
{
    if (runningLeafAnimations == 0 && !runningPauseAnimations.isEmpty()) {
        int closestTimeToFinish = closestPauseAnimationTimeToFinish();
        if (closestTimeToFinish < 0) {
            qDebug() << runningPauseAnimations;
            qDebug() << closestPauseAnimationTimeToFinish();
        }
        driver->stop();
        animationTimer.start(closestTimeToFinish, this);
        isPauseTimerActive = true;
    } else if (!driver->isRunning() || isPauseTimerActive) {
        driver->start();
        isPauseTimerActive = false;
    } else if (runningLeafAnimations == 0)
        driver->stop();
}

void QUnifiedTimer2::setTimingInterval(int interval)
{
    timingInterval = interval;

    if (driver->isRunning() && !isPauseTimerActive) {
        //we changed the timing interval
        driver->stop();
        driver->start();
    }
}


void QUnifiedTimer2::timerEvent(QTimerEvent *event)
{
    //in the case of consistent timing we make sure the orders in which events come is always the same
   //for that purpose we do as if the startstoptimer would always fire before the animation timer
    if ((consistentTiming && startStopAnimationTimer.isActive()) ||
        event->timerId() == startStopAnimationTimer.timerId()) {
        startStopAnimationTimer.stop();

        //we transfer the waiting animations into the "really running" state
        animations += animationsToStart;
        animationsToStart.clear();
        if (animations.isEmpty()) {
            animationTimer.stop();
            isPauseTimerActive = false;
            // invalidate the start reference time
            time.invalidate();
        } else {
            restartAnimationTimer();
            if (!time.isValid()) {
                lastTick = 0;
                time.start();
            }
        }
    }

    if (event->timerId() == animationTimer.timerId()) {
        // update current time on all top level animations
        updateAnimationsTime(-1);
        restartAnimationTimer();
    }
}

void QUnifiedTimer2::registerAnimation(QAbstractAnimation2* animation, bool isTopLevel)
{
    QUnifiedTimer2 *inst = instance(true); //we create the instance if needed
    inst->registerRunningAnimation(animation);
    if (isTopLevel) {
        Q_ASSERT(!animation->m_hasRegisteredTimer);
        animation->m_hasRegisteredTimer = true;
        inst->animationsToStart << animation;
        if (!inst->startStopAnimationTimer.isActive())
            inst->startStopAnimationTimer.start(STARTSTOP_TIMER_DELAY, inst);
    }
}

void QUnifiedTimer2::unregisterAnimation(QAbstractAnimation2 *animation)
{
    QUnifiedTimer2 *inst = QUnifiedTimer2::instance(false);
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

            if (inst->animations.isEmpty() && !inst->startStopAnimationTimer.isActive())
                inst->startStopAnimationTimer.start(STARTSTOP_TIMER_DELAY, inst);
        } else {
            if (inst->animationsToStart.contains(animation)) {
                inst->animationsToStart.removeOne(animation);
            }
        }
    }
    animation->m_hasRegisteredTimer = false;
}

void QUnifiedTimer2::registerRunningAnimation(QAbstractAnimation2* animation)
{
    if (animation->m_isGroup)
        return;

    if (animation->m_isPause) {
        runningPauseAnimations << animation;
    } else
        runningLeafAnimations++;
}

void QUnifiedTimer2::unregisterRunningAnimation(QAbstractAnimation2 *animation)
{
    if (animation->m_isGroup)
        return;

    if (animation->m_isPause && runningPauseAnimations.contains(animation)) {
        runningPauseAnimations.removeOne(animation);
    } else if (animations.contains(animation) || animationsToStart.contains(animation))
        runningLeafAnimations--;
    Q_ASSERT(runningLeafAnimations >= 0);
}

int QUnifiedTimer2::closestPauseAnimationTimeToFinish()
{
    int closestTimeToFinish = INT_MAX;
    for (int i = 0; i < runningPauseAnimations.size(); ++i) {
        QAbstractAnimation2* animation = runningPauseAnimations.at(i);
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


void QUnifiedTimer2::installAnimationDriver(QAnimationDriver2 *d)
{
    if (driver != &defaultDriver) {
        qWarning("QUnifiedTimer2: animation driver already installed...");
        return;
    }

    if (driver->isRunning()) {
        driver->stop();
        d->start();
    }

    driver = d;

}


void QUnifiedTimer2::uninstallAnimationDriver(QAnimationDriver2 *d)
{
    if (driver != d) {
        qWarning("QUnifiedTimer2: trying to uninstall a driver that is not installed...");
        return;
    }

    driver = &defaultDriver;

    if (d->isRunning()) {
        d->stop();
        driver->start();
    }
}

/*!
    Returns true if \a d is the currently installed animation driver
    and is not the default animation driver (which can never be uninstalled).
*/
bool QUnifiedTimer2::canUninstallAnimationDriver(QAnimationDriver2 *d)
{
    return d == driver && driver != &defaultDriver;
}


/*!
   \class QAnimationDriver2

   \brief The QAnimationDriver2 class is used to exchange the mechanism that drives animations.

   The default animation system is driven by a timer that fires at regular intervals.
   In some scenarios, it is better to drive the animation based on other synchronization
   mechanisms, such as the vertical refresh rate of the screen.

   \internal
 */

QAnimationDriver2::QAnimationDriver2(QObject *parent)
    : QObject(*(new QAnimationDriver2Private), parent)
{
}

QAnimationDriver2::QAnimationDriver2(QAnimationDriver2Private &dd, QObject *parent)
    : QObject(dd, parent)
{
}

QAnimationDriver2::~QAnimationDriver2()
{
    QUnifiedTimer2 *timer = QUnifiedTimer2::instance(true);
    if (timer->canUninstallAnimationDriver(this))
        uninstall();
}



/*!
    Advances the animation based to the specified \a timeStep. This function should
    be continuously called by the driver subclasses while the animation is running.

    If \a timeStep is positive, it will be used as the current time in the
    calculations; otherwise, the current clock time will be used.
 */

void QAnimationDriver2::advanceAnimation(qint64 timeStep)
{
    QUnifiedTimer2 *instance = QUnifiedTimer2::instance();

    // update current time on all top level animations
    instance->updateAnimationsTime(timeStep);
    instance->restartAnimationTimer();
}



/*!
    Advances the animation. This function should be continously called
    by the driver while the animation is running.
 */

void QAnimationDriver2::advance()
{
    advanceAnimation(-1);
}



/*!
    Installs this animation driver. The animation driver is thread local and
    will only apply for the thread its installed in.
 */

void QAnimationDriver2::install()
{
    QUnifiedTimer2 *timer = QUnifiedTimer2::instance(true);
    timer->installAnimationDriver(this);
}



/*!
    Uninstalls this animation driver.
 */

void QAnimationDriver2::uninstall()
{
    QUnifiedTimer2 *timer = QUnifiedTimer2::instance(true);
    timer->uninstallAnimationDriver(this);
}

bool QAnimationDriver2::isRunning() const
{
    return d_func()->running;
}


void QAnimationDriver2::start()
{
    Q_D(QAnimationDriver2);
    if (!d->running) {
        emit started();
        d->running = true;
    }
}


void QAnimationDriver2::stop()
{
    Q_D(QAnimationDriver2);
    if (d->running) {
        emit stopped();
        d->running = false;
    }
}


/*!
    \fn qint64 QAnimationDriver2::elapsed() const

    Returns the number of milliseconds since the animations was started.
 */

qint64 QAnimationDriver2::elapsed() const
{
    return QUnifiedTimer2::instance()->time.elapsed();
}

/*!
    \fn QAnimationDriver2::started()

    This signal is emitted by the animation framework to notify the driver
    that continous animation has started.

    \internal
 */

/*!
    \fn QAnimationDriver2::stopped()

    This signal is emitted by the animation framework to notify the driver
    that continous animation has stopped.

    \internal
 */

/*!
   The default animation driver just spins the timer...
 */
QDefaultAnimationDriver2::QDefaultAnimationDriver2(QUnifiedTimer2 *timer)
    : QAnimationDriver2(0), m_unified_timer(timer)
{
    connect(this, SIGNAL(started()), this, SLOT(startTimer()));
    connect(this, SIGNAL(stopped()), this, SLOT(stopTimer()));
}

void QDefaultAnimationDriver2::timerEvent(QTimerEvent *e)
{
    Q_ASSERT(e->timerId() == m_timer.timerId());
    Q_UNUSED(e); // if the assertions are disabled
    advance();
}

void QDefaultAnimationDriver2::startTimer()
{
    m_timer.start(m_unified_timer->timingInterval, this);
}

void QDefaultAnimationDriver2::stopTimer()
{
    m_timer.stop();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void QAbstractAnimation2::setState(QAbstractAnimation2::State newState)
{
    if (m_state == newState)
        return;

    if (m_loopCount == 0)
        return;

    QAbstractAnimation2::State oldState = m_state;
    int oldCurrentTime = m_currentTime;
    int oldCurrentLoop = m_currentLoop;
    QAbstractAnimation2::Direction oldDirection = m_direction;

    // check if we should Rewind
    if ((newState == QAbstractAnimation2::Paused || newState == QAbstractAnimation2::Running)
        && oldState == QAbstractAnimation2::Stopped) {
            //here we reset the time if needed
            //we don't call setCurrentTime because this might change the way the animation
            //behaves: changing the state or changing the current value
            m_totalCurrentTime = m_currentTime = (m_direction == QAbstractAnimation2::Forward) ?
                0 : (m_loopCount == -1 ? duration() : totalDuration());
    }

    m_state = newState;
    //(un)registration of the animation must always happen before calls to
    //virtual function (updateState) to ensure a correct state of the timer
    bool isTopLevel = !m_group || m_group->state() == QAbstractAnimation2::Stopped;
    if (oldState == QAbstractAnimation2::Running) {
        if (newState == QAbstractAnimation2::Paused && m_hasRegisteredTimer)
            QUnifiedTimer2::ensureTimerUpdate();
        //the animation, is not running any more
        QUnifiedTimer2::unregisterAnimation(this);
    } else if (newState == QAbstractAnimation2::Running) {
        QUnifiedTimer2::registerAnimation(this, isTopLevel);
    }

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
    case QAbstractAnimation2::Paused:
        break;
    case QAbstractAnimation2::Running:
        {

            // this ensures that the value is updated now that the animation is running
            if (oldState == QAbstractAnimation2::Stopped) {
                if (isTopLevel) {
                    // currentTime needs to be updated if pauseTimer is active
                    QUnifiedTimer2::ensureTimerUpdate();
                    setCurrentTime(m_totalCurrentTime);
                }
            }
        }
        break;
    case QAbstractAnimation2::Stopped:
        // Leave running state.
        int dura = duration();

        if (dura == -1 || m_loopCount < 0
            || (oldDirection == QAbstractAnimation2::Forward && (oldCurrentTime * (oldCurrentLoop + 1)) == (dura * m_loopCount))
            || (oldDirection == QAbstractAnimation2::Backward && oldCurrentTime == 0)) {
               finished();
        }
        break;
    }
}


QAbstractAnimation2::QAbstractAnimation2(QDeclarativeAbstractAnimation* animation)
    : QDeclarativeRefCount()
    , m_loopCount(1)
    , m_isPause(false)
    , m_isGroup(false)
    , m_group(0)
    , m_animationGuard(animation)
    , m_type(QAbstractAnimation2::DefaultAnimation)
    , m_direction(QAbstractAnimation2::Forward)
    , m_state(QAbstractAnimation2::Stopped)
    , m_totalCurrentTime(0)
    , m_currentTime(0)
    , m_currentLoop(0)
    , m_hasRegisteredTimer(false)
{
}


QAbstractAnimation2::QAbstractAnimation2(const QAbstractAnimation2& other)
    : m_loopCount(other.m_loopCount)
    , m_isPause(other.m_isPause)
    , m_group(other.m_group)
    , m_animationGuard(0)
    , m_type(other.m_type)
    , m_direction(other.m_direction)
    , m_state(QAbstractAnimation2::Stopped)
    , m_totalCurrentTime(0)
    , m_currentTime(0)
    , m_currentLoop(0)
    , m_hasRegisteredTimer(false)
{
}

QAbstractAnimation2::AnimationType QAbstractAnimation2::type() const
{
    return QAbstractAnimation2::DefaultAnimation;
}

QDeclarativeAbstractAnimation* QAbstractAnimation2::animation() const
{
    return qobject_cast<QDeclarativeAbstractAnimation*>(m_animationGuard);
}

void QAbstractAnimation2::setAnimation(QObject *animation)
{
    m_animationGuard = animation;
}

QAbstractAnimation2::~QAbstractAnimation2()
{
    //we can't call stop here. Otherwise we get pure virtual calls
    if (m_state != Stopped) {
        QAbstractAnimation2::State oldState = m_state;
        m_state = Stopped;
        stateChanged(oldState, m_state);
    }

    if (m_group)
        m_group->removeAnimation(this);
    QUnifiedTimer2::unregisterAnimation(this);
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
        QUnifiedTimer2::ensureTimerUpdate();

    m_direction = direction;
    updateDirection(direction);

    if (m_hasRegisteredTimer)
        // needed to update the timer interval in case of a pause animation
        QUnifiedTimer2::updateAnimationTimer();

    directionChanged(direction);
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

void QAbstractAnimation2::setPaused(bool paused)
{
    if (paused)
        pause();
    else
        resume();
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
    for (int ii = 0; ii < m_finishedSlots.count(); ++ii) {
        QPair<QDeclarativeGuard<QObject>, int> slot = m_finishedSlots.at(ii);
        QObject *obj = slot.first;
        if (obj) {
            void *args[] = { 0 };
            QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod,
                                  slot.second, args);
        }
    }

    if (!m_animationGuard.isNull())
        animation()->timelineComplete();

    if (group() && (duration() == -1 || loopCount() < 0)) {
        //this is an uncontrolled animation, need to notify the group animation we are finished
        group()->uncontrolledAnimationFinished(this);
    }
}

void QAbstractAnimation2::stateChanged(QAbstractAnimation2::State newState, QAbstractAnimation2::State oldState)
{
    for (int ii = 0; ii < m_stateChangedSlots.count(); ++ii) {
        QPair<QDeclarativeGuard<QObject>, int> slot = m_stateChangedSlots.at(ii);
        QObject *obj = slot.first;
        if (obj) {
            void *args[] = { 0, &newState, &oldState };
            QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod,
                                  slot.second, args);
        }
    }
}

void QAbstractAnimation2::currentLoopChanged(int currentLoop)
{
    for (int ii = 0; ii < m_currentLoopChangedSlots.count(); ++ii) {
        QPair<QDeclarativeGuard<QObject>, int> slot = m_currentLoopChangedSlots.at(ii);
        QObject *obj = slot.first;
        if (obj) {
            void *args[] = { 0, &currentLoop };
            QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod,
                                  slot.second, args);
        }
    }
}

void QAbstractAnimation2::directionChanged(QAbstractAnimation2::Direction direction)
{
    for (int ii = 0; ii < m_directionChangedSlots.count(); ++ii) {
        QPair<QDeclarativeGuard<QObject>, int> slot = m_directionChangedSlots.at(ii);
        QObject *obj = slot.first;
        if (obj) {
            void *args[] = { 0, &direction };
            QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod,
                                  slot.second, args);
        }
    }
}


void QAbstractAnimation2::registerFinished(QObject* object, const char* method)
{
    if (object && object != animation()) {
        m_finishedSlots.append(qMakePair(QDeclarativeGuard<QObject>(object)
                              , object->metaObject()->indexOfSlot(method)));
    }
}

void QAbstractAnimation2::registerStateChanged(QObject* object, const char* method)
{
    if (object) {
        m_stateChangedSlots.append(qMakePair(QDeclarativeGuard<QObject>(object)
                                  , object->metaObject()->indexOfSlot(method)));
    }
}

void QAbstractAnimation2::registerCurrentLoopChanged(QObject* object, const char* method)
{
    if (object) {
        m_currentLoopChangedSlots.append(qMakePair(QDeclarativeGuard<QObject>(object)
                                        , object->metaObject()->indexOfSlot(method)));
    }
}

void QAbstractAnimation2::registerDirectionChanged(QObject* object, const char* method)
{
    if (object) {
        m_directionChangedSlots.append(qMakePair(QDeclarativeGuard<QObject>(object)
                                      , object->metaObject()->indexOfSlot(method)));
    }
}

uint qHash(const QAbstractAnimation2Pointer& value)
{
    return qHash(value.data());
}

QT_END_NAMESPACE

#include "moc_qabstractanimation2_p.cpp"
