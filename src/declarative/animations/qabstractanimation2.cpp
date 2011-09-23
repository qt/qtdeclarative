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

/*!
    \class QAbstractAnimation2
    \ingroup animation
    \brief The QAbstractAnimation2 class is the base of all animations.
    \since 4.6

    The class defines the functions for the functionality shared by
    all animations. By inheriting this class, you can create custom
    animations that plug into the rest of the animation framework.

    The progress of an animation is given by its current time
    (currentLoopTime()), which is measured in milliseconds from the start
    of the animation (0) to its end (duration()). The value is updated
    automatically while the animation is running. It can also be set
    directly with setCurrentTime().

    At any point an animation is in one of three states:
    \l{QAbstractAnimation2::}{Running},
    \l{QAbstractAnimation2::}{Stopped}, or
    \l{QAbstractAnimation2::}{Paused}--as defined by the
    \l{QAbstractAnimation2::}{State} enum. The current state can be
    changed by calling start(), stop(), pause(), or resume(). An
    animation will always reset its \l{currentTime()}{current time}
    when it is started. If paused, it will continue with the same
    current time when resumed. When an animation is stopped, it cannot
    be resumed, but will keep its current time (until started again).
    QAbstractAnimation2 will emit stateChanged() whenever its state
    changes.

    An animation can loop any number of times by setting the loopCount
    property. When an animation's current time reaches its duration(),
    it will reset the current time and keep running. A loop count of 1
    (the default value) means that the animation will run one time.
    Note that a duration of -1 means that the animation will run until
    stopped; the current time will increase indefinitely. When the
    current time equals duration() and the animation is in its
    final loop, the \l{QAbstractAnimation2::}{Stopped} state is
    entered, and the finished() signal is emitted.

    QAbstractAnimation2 provides pure virtual functions used by
    subclasses to track the progress of the animation: duration() and
    updateCurrentTime(). The duration() function lets you report a
    duration for the animation (as discussed above). The animation
    framework calls updateCurrentTime() when current time has changed.
    By reimplementing this function, you can track the animation
    progress. Note that neither the interval between calls nor the
    number of calls to this function are defined; though, it will
    normally be 60 updates per second.

    By reimplementing updateState(), you can track the animation's
    state changes, which is particularly useful for animations that
    are not driven by time.

    \sa QVariantAnimation2, QPropertyAnimation2, QAnimationGroup2, {The Animation Framework}
*/

/*!
    \enum QAbstractAnimation2::DeletionPolicy

    \value KeepWhenStopped The animation will not be deleted when stopped.
    \value DeleteWhenStopped The animation will be automatically deleted when
    stopped.
*/

/*!
    \fn QAbstractAnimation2::finished()

    QAbstractAnimation2 emits this signal after the animation has stopped and
    has reached the end.

    This signal is emitted after stateChanged().

    \sa stateChanged()
*/

/*!
    \fn QAbstractAnimation2::stateChanged(QAbstractAnimation2::State newState, QAbstractAnimation2::State oldState)

    QAbstractAnimation2 emits this signal whenever the state of the animation has
    changed from \a oldState to \a newState. This signal is emitted after the virtual
    updateState() function is called.

    \sa updateState()
*/

/*!
    \fn QAbstractAnimation2::currentLoopChanged(int currentLoop)

    QAbstractAnimation2 emits this signal whenever the current loop
    changes. \a currentLoop is the current loop.

    \sa currentLoop(), loopCount()
*/

/*!
    \fn QAbstractAnimation2::directionChanged(QAbstractAnimation2::Direction newDirection);

    QAbstractAnimation2 emits this signal whenever the direction has been
    changed. \a newDirection is the new direction.

    \sa direction
*/

#include "private/qabstractanimation2_p.h"
#include "private/qanimationgroup2_p.h"

#include <QtCore/qdebug.h>

#include "private/qabstractanimation2_p_p.h"

#include <QtCore/qmath.h>
#include <QtCore/qthreadstorage.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qpointer.h>



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
            QAbstractAnimation2 *animation = animations.at(currentAnimationIdx);
            int elapsed = QAbstractAnimation2Private::get(animation)->totalCurrentTime
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

void QUnifiedTimer2::registerAnimation(QAbstractAnimation2 *animation, bool isTopLevel)
{
    QUnifiedTimer2 *inst = instance(true); //we create the instance if needed
    inst->registerRunningAnimation(animation);
    if (isTopLevel) {
        Q_ASSERT(!QAbstractAnimation2Private::get(animation)->hasRegisteredTimer);
        QAbstractAnimation2Private::get(animation)->hasRegisteredTimer = true;
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

        if (!QAbstractAnimation2Private::get(animation)->hasRegisteredTimer)
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
            inst->animationsToStart.removeOne(animation);
        }
    }
    QAbstractAnimation2Private::get(animation)->hasRegisteredTimer = false;
}

void QUnifiedTimer2::registerRunningAnimation(QAbstractAnimation2 *animation)
{
    if (QAbstractAnimation2Private::get(animation)->isGroup)
        return;

    if (QAbstractAnimation2Private::get(animation)->isPause) {
        runningPauseAnimations << animation;
    } else
        runningLeafAnimations++;
}

void QUnifiedTimer2::unregisterRunningAnimation(QAbstractAnimation2 *animation)
{
    if (QAbstractAnimation2Private::get(animation)->isGroup)
        return;

    if (QAbstractAnimation2Private::get(animation)->isPause)
        runningPauseAnimations.removeOne(animation);
    else
        runningLeafAnimations--;
    Q_ASSERT(runningLeafAnimations >= 0);
}

int QUnifiedTimer2::closestPauseAnimationTimeToFinish()
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



void QAbstractAnimation2Private::setState(QAbstractAnimation2::State newState)
{
    Q_Q(QAbstractAnimation2);
    if (state == newState)
        return;

    if (loopCount == 0)
        return;

    QAbstractAnimation2::State oldState = state;
    int oldCurrentTime = currentTime;
    int oldCurrentLoop = currentLoop;
    QAbstractAnimation2::Direction oldDirection = direction;

    // check if we should Rewind
    if ((newState == QAbstractAnimation2::Paused || newState == QAbstractAnimation2::Running)
        && oldState == QAbstractAnimation2::Stopped) {
            //here we reset the time if needed
            //we don't call setCurrentTime because this might change the way the animation
            //behaves: changing the state or changing the current value
            totalCurrentTime = currentTime = (direction == QAbstractAnimation2::Forward) ?
                0 : (loopCount == -1 ? q->duration() : q->totalDuration());
    }

    state = newState;
    QWeakPointer<QAbstractAnimation2> guard(q);

    //(un)registration of the animation must always happen before calls to
    //virtual function (updateState) to ensure a correct state of the timer
    bool isTopLevel = !group || group->state() == QAbstractAnimation2::Stopped;
    if (oldState == QAbstractAnimation2::Running) {
        if (newState == QAbstractAnimation2::Paused && hasRegisteredTimer)
            QUnifiedTimer2::ensureTimerUpdate();
        //the animation, is not running any more
        QUnifiedTimer2::unregisterAnimation(q);
    } else if (newState == QAbstractAnimation2::Running) {
        QUnifiedTimer2::registerAnimation(q, isTopLevel);
    }

    q->updateState(newState, oldState);
    if (!guard || newState != state) //this is to be safe if updateState changes the state
        return;

    // Notify state change
    emit q->stateChanged(newState, oldState);
    if (!guard || newState != state) //this is to be safe if updateState changes the state
        return;

    switch (state) {
    case QAbstractAnimation2::Paused:
        break;
    case QAbstractAnimation2::Running:
        {

            // this ensures that the value is updated now that the animation is running
            if (oldState == QAbstractAnimation2::Stopped) {
                if (isTopLevel) {
                    // currentTime needs to be updated if pauseTimer is active
                    QUnifiedTimer2::ensureTimerUpdate();
                    q->setCurrentTime(totalCurrentTime);
                }
            }
        }
        break;
    case QAbstractAnimation2::Stopped:
        // Leave running state.
        int dura = q->duration();

        if (deleteWhenStopped)
            q->deleteLater();

        if (dura == -1 || loopCount < 0
            || (oldDirection == QAbstractAnimation2::Forward && (oldCurrentTime * (oldCurrentLoop + 1)) == (dura * loopCount))
            || (oldDirection == QAbstractAnimation2::Backward && oldCurrentTime == 0)) {
                emit q->finished();
        }
        break;
    }
}

/*!
    Constructs the QAbstractAnimation2 base class, and passes \a parent to
    QObject's constructor.

    \sa QVariantAnimation2, QAnimationGroup2
*/
QAbstractAnimation2::QAbstractAnimation2(QObject *parent)
    : QObject(*new QAbstractAnimation2Private, 0)
{
    // Allow auto-add on reparent
    setParent(parent);
}

/*!
    \internal
*/
QAbstractAnimation2::QAbstractAnimation2(QAbstractAnimation2Private &dd, QObject *parent)
    : QObject(dd, 0)
{
    // Allow auto-add on reparent
   setParent(parent);
}

/*!
    Stops the animation if it's running, then destroys the
    QAbstractAnimation2. If the animation is part of a QAnimationGroup2, it is
    automatically removed before it's destroyed.
*/
QAbstractAnimation2::~QAbstractAnimation2()
{
    Q_D(QAbstractAnimation2);
    //we can't call stop here. Otherwise we get pure virtual calls
    if (d->state != Stopped) {
        QAbstractAnimation2::State oldState = d->state;
        d->state = Stopped;
        emit stateChanged(oldState, d->state);
        if (oldState == QAbstractAnimation2::Running)
            QUnifiedTimer2::unregisterAnimation(this);
    }
}

/*!
    \property QAbstractAnimation2::state
    \brief state of the animation.

    This property describes the current state of the animation. When the
    animation state changes, QAbstractAnimation2 emits the stateChanged()
    signal.
*/
QAbstractAnimation2::State QAbstractAnimation2::state() const
{
    Q_D(const QAbstractAnimation2);
    return d->state;
}

/*!
    If this animation is part of a QAnimationGroup2, this function returns a
    pointer to the group; otherwise, it returns 0.

    \sa QAnimationGroup2::addAnimation()
*/
QAnimationGroup2 *QAbstractAnimation2::group() const
{
    Q_D(const QAbstractAnimation2);
    return d->group;
}

/*!
    \enum QAbstractAnimation2::State

    This enum describes the state of the animation.

    \value Stopped The animation is not running. This is the initial state
    of QAbstractAnimation2, and the state QAbstractAnimation2 reenters when finished. The current
    time remain unchanged until either setCurrentTime() is
    called, or the animation is started by calling start().

    \value Paused The animation is paused (i.e., temporarily
    suspended). Calling resume() will resume animation activity.

    \value Running The animation is running. While control is in the event
    loop, QAbstractAnimation2 will update its current time at regular intervals,
    calling updateCurrentTime() when appropriate.

    \sa state(), stateChanged()
*/

/*!
    \enum QAbstractAnimation2::Direction

    This enum describes the direction of the animation when in \l Running state.

    \value Forward The current time of the animation increases with time (i.e.,
    moves from 0 and towards the end / duration).

    \value Backward The current time of the animation decreases with time (i.e.,
    moves from the end / duration and towards 0).

    \sa direction
*/

/*!
    \property QAbstractAnimation2::direction
    \brief the direction of the animation when it is in \l Running
    state.

    This direction indicates whether the time moves from 0 towards the
    animation duration, or from the value of the duration and towards 0 after
    start() has been called.

    By default, this property is set to \l Forward.
*/
QAbstractAnimation2::Direction QAbstractAnimation2::direction() const
{
    Q_D(const QAbstractAnimation2);
    return d->direction;
}
void QAbstractAnimation2::setDirection(Direction direction)
{
    Q_D(QAbstractAnimation2);
    if (d->direction == direction)
        return;

    if (state() == Stopped) {
        if (direction == Backward) {
            d->currentTime = duration();
            d->currentLoop = d->loopCount - 1;
        } else {
            d->currentTime = 0;
            d->currentLoop = 0;
        }
    }

    // the commands order below is important: first we need to setCurrentTime with the old direction,
    // then update the direction on this and all children and finally restart the pauseTimer if needed
    if (d->hasRegisteredTimer)
        QUnifiedTimer2::ensureTimerUpdate();

    d->direction = direction;
    updateDirection(direction);

    if (d->hasRegisteredTimer)
        // needed to update the timer interval in case of a pause animation
        QUnifiedTimer2::updateAnimationTimer();

    emit directionChanged(direction);
}

/*!
    \property QAbstractAnimation2::duration
    \brief the duration of the animation.

    If the duration is -1, it means that the duration is undefined.
    In this case, loopCount is ignored.
*/

/*!
    \property QAbstractAnimation2::loopCount
    \brief the loop count of the animation

    This property describes the loop count of the animation as an integer.
    By default this value is 1, indicating that the animation
    should run once only, and then stop. By changing it you can let the
    animation loop several times. With a value of 0, the animation will not
    run at all, and with a value of -1, the animation will loop forever
    until stopped.
    It is not supported to have loop on an animation that has an undefined
    duration. It will only run once.
*/
int QAbstractAnimation2::loopCount() const
{
    Q_D(const QAbstractAnimation2);
    return d->loopCount;
}
void QAbstractAnimation2::setLoopCount(int loopCount)
{
    Q_D(QAbstractAnimation2);
    d->loopCount = loopCount;
}

/*!
    \property QAbstractAnimation2::currentLoop
    \brief the current loop of the animation

    This property describes the current loop of the animation. By default,
    the animation's loop count is 1, and so the current loop will
    always be 0. If the loop count is 2 and the animation runs past its
    duration, it will automatically rewind and restart at current time 0, and
    current loop 1, and so on.

    When the current loop changes, QAbstractAnimation2 emits the
    currentLoopChanged() signal.
*/
int QAbstractAnimation2::currentLoop() const
{
    Q_D(const QAbstractAnimation2);
    return d->currentLoop;
}

/*!
    \fn virtual int QAbstractAnimation2::duration() const = 0

    This pure virtual function returns the duration of the animation, and
    defines for how long QAbstractAnimation2 should update the current
    time. This duration is local, and does not include the loop count.

    A return value of -1 indicates that the animation has no defined duration;
    the animation should run forever until stopped. This is useful for
    animations that are not time driven, or where you cannot easily predict
    its duration (e.g., event driven audio playback in a game).

    If the animation is a parallel QAnimationGroup2, the duration will be the longest
    duration of all its animations. If the animation is a sequential QAnimationGroup2,
    the duration will be the sum of the duration of all its animations.
    \sa loopCount
*/

/*!
    Returns the total and effective duration of the animation, including the
    loop count.

    \sa duration(), currentTime
*/
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

/*!
    Returns the current time inside the current loop. It can go from 0 to duration().

    \sa duration(), currentTime
*/

int QAbstractAnimation2::currentLoopTime() const
{
    Q_D(const QAbstractAnimation2);
    return d->currentTime;
}

/*!
    \property QAbstractAnimation2::currentTime
    \brief the current time and progress of the animation

    This property describes the animation's current time. You can change the
    current time by calling setCurrentTime, or you can call start() and let
    the animation run, setting the current time automatically as the animation
    progresses.

    The animation's current time starts at 0, and ends at totalDuration().

    \sa loopCount, currentLoopTime()
 */
int QAbstractAnimation2::currentTime() const
{
    Q_D(const QAbstractAnimation2);
    return d->totalCurrentTime;
}
void QAbstractAnimation2::setCurrentTime(int msecs)
{
    Q_D(QAbstractAnimation2);
    msecs = qMax(msecs, 0);

    // Calculate new time and loop.
    int dura = duration();
    int totalDura = dura <= 0 ? dura : ((d->loopCount < 0) ? -1 : dura * d->loopCount);
    if (totalDura != -1)
        msecs = qMin(totalDura, msecs);
    d->totalCurrentTime = msecs;

    // Update new values.
    int oldLoop = d->currentLoop;
    d->currentLoop = ((dura <= 0) ? 0 : (msecs / dura));
    if (d->currentLoop == d->loopCount) {
        //we're at the end
        d->currentTime = qMax(0, dura);
        d->currentLoop = qMax(0, d->loopCount - 1);
    } else {
        if (d->direction == Forward) {
            d->currentTime = (dura <= 0) ? msecs : (msecs % dura);
        } else {
            d->currentTime = (dura <= 0) ? msecs : ((msecs - 1) % dura) + 1;
            if (d->currentTime == dura)
                --d->currentLoop;
        }
    }

    updateCurrentTime(d->currentTime);
    if (d->currentLoop != oldLoop)
        emit currentLoopChanged(d->currentLoop);

    // All animations are responsible for stopping the animation when their
    // own end state is reached; in this case the animation is time driven,
    // and has reached the end.
    if ((d->direction == Forward && d->totalCurrentTime == totalDura)
        || (d->direction == Backward && d->totalCurrentTime == 0)) {
        stop();
    }
}

/*!
    Starts the animation. The \a policy argument says whether or not the
    animation should be deleted when it's done. When the animation starts, the
    stateChanged() signal is emitted, and state() returns Running. When control
    reaches the event loop, the animation will run by itself, periodically
    calling updateCurrentTime() as the animation progresses.

    If the animation is currently stopped or has already reached the end,
    calling start() will rewind the animation and start again from the beginning.
    When the animation reaches the end, the animation will either stop, or
    if the loop level is more than 1, it will rewind and continue from the beginning.

    If the animation is already running, this function does nothing.

    \sa stop(), state()
*/
void QAbstractAnimation2::start(DeletionPolicy policy)
{
    Q_D(QAbstractAnimation2);
    if (d->state == Running)
        return;
    d->deleteWhenStopped = policy;
    d->setState(Running);
}

/*!
    Stops the animation. When the animation is stopped, it emits the stateChanged()
    signal, and state() returns Stopped. The current time is not changed.

    If the animation stops by itself after reaching the end (i.e.,
    currentLoopTime() == duration() and currentLoop() > loopCount() - 1), the
    finished() signal is emitted.

    \sa start(), state()
 */
void QAbstractAnimation2::stop()
{
    Q_D(QAbstractAnimation2);

    if (d->state == Stopped)
        return;

    d->setState(Stopped);
}

/*!
    Pauses the animation. When the animation is paused, state() returns Paused.
    The value of currentTime will remain unchanged until resume() or start()
    is called. If you want to continue from the current time, call resume().

    \sa start(), state(), resume()
 */
void QAbstractAnimation2::pause()
{
    Q_D(QAbstractAnimation2);
    if (d->state == Stopped) {
        qWarning("QAbstractAnimation2::pause: Cannot pause a stopped animation");
        return;
    }

    d->setState(Paused);
}

/*!
    Resumes the animation after it was paused. When the animation is resumed,
    it emits the resumed() and stateChanged() signals. The currenttime is not
    changed.

    \sa start(), pause(), state()
 */
void QAbstractAnimation2::resume()
{
    Q_D(QAbstractAnimation2);
    if (d->state != Paused) {
        qWarning("QAbstractAnimation2::resume: "
                 "Cannot resume an animation that is not paused");
        return;
    }

    d->setState(Running);
}

/*!
    If \a paused is true, the animation is paused.
    If \a paused is false, the animation is resumed.

    \sa state(), pause(), resume()
*/
void QAbstractAnimation2::setPaused(bool paused)
{
    if (paused)
        pause();
    else
        resume();
}


/*!
    \reimp
*/
bool QAbstractAnimation2::event(QEvent *event)
{
    return QObject::event(event);
}

/*!
    \fn virtual void QAbstractAnimation2::updateCurrentTime(int currentTime) = 0;

    This pure virtual function is called every time the animation's
    \a currentTime changes.

    \sa updateState()
*/

/*!
    This virtual function is called by QAbstractAnimation2 when the state
    of the animation is changed from \a oldState to \a newState.

    \sa start(), stop(), pause(), resume()
*/
void QAbstractAnimation2::updateState(QAbstractAnimation2::State newState,
                                     QAbstractAnimation2::State oldState)
{
    Q_UNUSED(oldState);
    Q_UNUSED(newState);
}

/*!
    This virtual function is called by QAbstractAnimation2 when the direction
    of the animation is changed. The \a direction argument is the new direction.

    \sa setDirection(), direction()
*/
void QAbstractAnimation2::updateDirection(QAbstractAnimation2::Direction direction)
{
    Q_UNUSED(direction);
}


QT_END_NAMESPACE

#include "moc_qabstractanimation2_p.cpp"


