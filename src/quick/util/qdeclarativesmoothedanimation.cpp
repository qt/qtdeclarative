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

#include "qdeclarativesmoothedanimation_p.h"
#include "qdeclarativesmoothedanimation_p_p.h"

#include "qdeclarativeanimation_p_p.h"

#include <qdeclarativeproperty.h>
#include <private/qdeclarativeproperty_p.h>

#include <private/qdeclarativeglobal_p.h>

#include <QtCore/qdebug.h>

#include <math.h>

#define DELAY_STOP_TIMER_INTERVAL 32

QT_BEGIN_NAMESPACE


QSmoothedAnimationTimer::QSmoothedAnimationTimer(QDeclarativeRefPointer<QSmoothedAnimation> animation, QObject *parent)
    : QTimer(parent)
    , m_animation(animation)
{
    connect(this, SIGNAL(timeout()), this, SLOT(stopAnimation()));
}

QSmoothedAnimationTimer::~QSmoothedAnimationTimer()
{
}

void QSmoothedAnimationTimer::stopAnimation()
{
    m_animation->stop();
}

QSmoothedAnimation::QSmoothedAnimation()
    : QAbstractAnimation2(), to(0), velocity(200), userDuration(-1), maximumEasingTime(-1),
      reversingMode(QDeclarativeSmoothedAnimation::Eased), initialVelocity(0),
      trackVelocity(0), initialValue(0), invert(false), finalDuration(-1), lastTime(0),
      useDelta(false), delayedStopTimer(new QSmoothedAnimationTimer(this))
{
    delayedStopTimer->setInterval(DELAY_STOP_TIMER_INTERVAL);
    delayedStopTimer->setSingleShot(true);
}

QSmoothedAnimation::QSmoothedAnimation(const QSmoothedAnimation &other)
    : QAbstractAnimation2(other)
    , to(other.to)
    , velocity(other.velocity)
    , userDuration(other.userDuration)
    , maximumEasingTime(other.maximumEasingTime)
    , reversingMode(other.reversingMode)
    , initialVelocity(other.initialVelocity)
    , trackVelocity(other.trackVelocity)
    , initialValue(other.initialValue)
    , invert(other.invert)
    , finalDuration(other.finalDuration)
    , lastTime(other.lastTime)
    , useDelta(other.useDelta)
    , delayedStopTimer(new QSmoothedAnimationTimer(this))
{
    delayedStopTimer->setInterval(DELAY_STOP_TIMER_INTERVAL);
    delayedStopTimer->setSingleShot(true);
}

QSmoothedAnimation::~QSmoothedAnimation()
{
    delete delayedStopTimer;
}

void QSmoothedAnimation::restart()
{
    initialVelocity = trackVelocity;
    if (state() != QAbstractAnimation2::Running) {
        useDelta = false;
        start();
    } else {
        //we are joining a new wrapper group, our times need to be restarted
        useDelta = true;
        init();
        lastTime = 0;
    }
}

void QSmoothedAnimation::updateState(QAbstractAnimation2::State newState, QAbstractAnimation2::State /*oldState*/)
{
    if (newState == QAbstractAnimation2::Running)
        init();
}

void QSmoothedAnimation::delayedStop()
{
    if (!delayedStopTimer->isActive())
        delayedStopTimer->start();
}

int QSmoothedAnimation::duration() const
{
    return -1;
}

bool QSmoothedAnimation::recalc()
{
    s = to - initialValue;
    vi = initialVelocity;

    s = (invert? -1.0: 1.0) * s;

    if (userDuration > 0 && velocity > 0) {
        tf = s / velocity;
        if (tf > (userDuration / 1000.)) tf = (userDuration / 1000.);
    } else if (userDuration > 0) {
        tf = userDuration / 1000.;
    } else if (velocity > 0) {
        tf = s / velocity;
    } else {
        return false;
    }

    finalDuration = ceil(tf * 1000.0);

    if (maximumEasingTime == 0) {
        a = 0;
        d = 0;
        tp = 0;
        td = tf;
        vp = velocity;
        sp = 0;
        sd = s;
    } else if (maximumEasingTime != -1 && tf > (maximumEasingTime / 1000.)) {
        qreal met = maximumEasingTime / 1000.;
        /*       tp|       |td
         * vp_      _______
         *         /       \
         * vi_    /         \
         *                   \
         *                    \   _ 0
         *       |ta|      |ta|
         */
        qreal ta = met / 2.;
        a = (s - (vi * tf - 0.5 * vi * ta)) / (tf * ta - ta * ta);

        vp = vi + a * ta;
        d = vp / ta;
        tp = ta;
        sp = vi * ta + 0.5 * a * tp * tp;
        sd = sp + vp * (tf - 2 * ta);
        td = tf - ta;
    } else {
        qreal c1 = 0.25 * tf * tf;
        qreal c2 = 0.5 * vi * tf - s;
        qreal c3 = -0.25 * vi * vi;

        qreal a1 = (-c2 + sqrt(c2 * c2 - 4 * c1 * c3)) / (2. * c1);

        qreal tp1 = 0.5 * tf - 0.5 * vi / a1;
        qreal vp1 = a1 * tp1 + vi;

        qreal sp1 = 0.5 * a1 * tp1 * tp1 + vi * tp1;

        a = a1;
        d = a1;
        tp = tp1;
        td = tp1;
        vp = vp1;
        sp = sp1;
        sd = sp1;
    }
    return true;
}

qreal QSmoothedAnimation::easeFollow(qreal time_seconds)
{
    qreal value;
    if (time_seconds < tp) {
        trackVelocity = vi + time_seconds * a;
        value = 0.5 * a * time_seconds * time_seconds + vi * time_seconds;
    } else if (time_seconds < td) {
        time_seconds -= tp;
        trackVelocity = vp;
        value = sp + time_seconds * vp;
    } else if (time_seconds < tf) {
        time_seconds -= td;
        trackVelocity = vp - time_seconds * a;
        value = sd - 0.5 * d * time_seconds * time_seconds + vp * time_seconds;
    } else {
        trackVelocity = 0;
        value = s;
        delayedStop();
    }

    // to normalize 's' between [0..1], divide 'value' by 's'
    return value;
}

void QSmoothedAnimation::updateCurrentTime(int t)
{
    qreal time_seconds = useDelta ? qreal(QUnifiedTimer2::instance()->currentDelta()) / 1000. : qreal(t - lastTime) / 1000.;
    if (useDelta)
        useDelta = false;

    qreal value = easeFollow(time_seconds);
    value *= (invert? -1.0: 1.0);
    QDeclarativePropertyPrivate::write(target, initialValue + value,
                                       QDeclarativePropertyPrivate::BypassInterceptor
                                       | QDeclarativePropertyPrivate::DontRemoveBinding);
}

void QSmoothedAnimation::init()
{
    if (velocity == 0) {
        stop();
        return;
    }

    if (delayedStopTimer->isActive())
        delayedStopTimer->stop();

    initialValue = target.read().toReal();
    lastTime = this->currentTime();

    if (to == initialValue) {
        stop();
        return;
    }

    bool hasReversed = trackVelocity != 0. &&
                      ((!invert) == ((initialValue - to) > 0));

    if (hasReversed) {
        switch (reversingMode) {
            default:
            case QDeclarativeSmoothedAnimation::Eased:
                initialVelocity = -trackVelocity;
                break;
            case QDeclarativeSmoothedAnimation::Sync:
                QDeclarativePropertyPrivate::write(target, to,
                                                   QDeclarativePropertyPrivate::BypassInterceptor
                                                   | QDeclarativePropertyPrivate::DontRemoveBinding);
                trackVelocity = 0;
                stop();
                return;
            case QDeclarativeSmoothedAnimation::Immediate:
                initialVelocity = 0;
                break;
        }
    }

    trackVelocity = initialVelocity;

    invert = (to < initialValue);

    if (!recalc()) {
        QDeclarativePropertyPrivate::write(target, to,
                                           QDeclarativePropertyPrivate::BypassInterceptor
                                           | QDeclarativePropertyPrivate::DontRemoveBinding);
        stop();
        return;
    }
}

/*!
    \qmlclass SmoothedAnimation QDeclarativeSmoothedAnimation
    \inqmlmodule QtQuick 2
    \ingroup qml-animation-transition
    \inherits NumberAnimation
    \brief The SmoothedAnimation element allows a property to smoothly track a value.

    A SmoothedAnimation animates a property's value to a set target value
    using an ease in/out quad easing curve.  When the target value changes,
    the easing curves used to animate between the old and new target values
    are smoothly spliced together to create a smooth movement to the new
    target value that maintains the current velocity.

    The follow example shows one \l Rectangle tracking the position of another
    using SmoothedAnimation. The green rectangle's \c x and \c y values are
    bound to those of the red rectangle. Whenever these values change, the
    green rectangle smoothly animates to its new position:

    \snippet doc/src/snippets/declarative/smoothedanimation.qml 0

    A SmoothedAnimation can be configured by setting the \l velocity at which the
    animation should occur, or the \l duration that the animation should take.
    If both the \l velocity and \l duration are specified, the one that results in
    the quickest animation is chosen for each change in the target value.

    For example, animating from 0 to 800 will take 4 seconds if a velocity
    of 200 is set, will take 8 seconds with a duration of 8000 set, and will
    take 4 seconds with both a velocity of 200 and a duration of 8000 set.
    Animating from 0 to 20000 will take 10 seconds if a velocity of 200 is set,
    will take 8 seconds with a duration of 8000 set, and will take 8 seconds
    with both a velocity of 200 and a duration of 8000 set.

    The default velocity of SmoothedAnimation is 200 units/second.  Note that if the range of the
    value being animated is small, then the velocity will need to be adjusted
    appropriately.  For example, the opacity of an item ranges from 0 - 1.0.
    To enable a smooth animation in this range the velocity will need to be
    set to a value such as 0.5 units/second.  Animating from 0 to 1.0 with a velocity
    of 0.5 will take 2000 ms to complete.

    Like any other animation element, a SmoothedAnimation can be applied in a
    number of ways, including transitions, behaviors and property value
    sources. The \l {QML Animation and Transitions} documentation shows a
    variety of methods for creating animations.

    \sa SpringAnimation, NumberAnimation, {QML Animation and Transitions}, {declarative/animation/basics}{Animation basics example}
*/

QDeclarativeSmoothedAnimation::QDeclarativeSmoothedAnimation(QObject *parent)
: QDeclarativeNumberAnimation(*(new QDeclarativeSmoothedAnimationPrivate), parent)
{
}

QDeclarativeSmoothedAnimation::~QDeclarativeSmoothedAnimation()
{

}

QDeclarativeSmoothedAnimationPrivate::QDeclarativeSmoothedAnimationPrivate()
    : anim(0)
{
    anim.take(new QSmoothedAnimation);
}

QDeclarativeSmoothedAnimationPrivate::~QDeclarativeSmoothedAnimationPrivate()
{
}

void QDeclarativeSmoothedAnimationPrivate::updateRunningAnimations()
{
    foreach (const QDeclarativeRefPointer<QSmoothedAnimation> &ease, activeAnimations.values()){
        ease->maximumEasingTime = anim->maximumEasingTime;
        ease->reversingMode = anim->reversingMode;
        ease->velocity = anim->velocity;
        ease->userDuration = anim->userDuration;
        ease->init();
    }
}

QAbstractAnimation2Pointer QDeclarativeSmoothedAnimation::transition(QDeclarativeStateActions &actions,
                                               QDeclarativeProperties &modified,
                                               TransitionDirection direction)
{
    Q_UNUSED(direction);
    Q_D(QDeclarativeSmoothedAnimation);

    QDeclarativeStateActions dataActions = QDeclarativePropertyAnimation::createTransitionActions(actions, modified);

    QDeclarativeRefPointer<QParallelAnimationGroup2> wrapperGroup;
    wrapperGroup.take(new QParallelAnimationGroup2());

    if (!dataActions.isEmpty()) {
        QSet<QAbstractAnimation2Pointer> anims;
        for (int i = 0; i < dataActions.size(); i++) {
            QDeclarativeRefPointer<QSmoothedAnimation> ease;
            bool needsRestart;
            if (!d->activeAnimations.contains(dataActions[i].property)) {
                ease.take(new QSmoothedAnimation());
                d->activeAnimations.insert(dataActions[i].property, ease);
                ease->target = dataActions[i].property;
                needsRestart = false;
            } else {
                ease = d->activeAnimations.value(dataActions[i].property);
                needsRestart = true;
            }
            wrapperGroup->addAnimation(QAbstractAnimation2Pointer(ease));

            ease->to = dataActions[i].toValue.toReal();

            // copying public members from main value holder animation
            ease->maximumEasingTime = d->anim->maximumEasingTime;
            ease->reversingMode = d->anim->reversingMode;
            ease->velocity = d->anim->velocity;
            ease->userDuration = d->anim->userDuration;

            ease->initialVelocity = ease->trackVelocity;

            if (needsRestart)
                ease->restart();
            anims.insert(QAbstractAnimation2Pointer(ease));
        }

        foreach (const QDeclarativeRefPointer<QSmoothedAnimation> &ease, d->activeAnimations.values()){
            if (!anims.contains(ease.data()))
                d->activeAnimations.remove(ease->target);
        }
    }
    return QAbstractAnimation2Pointer(wrapperGroup);
}

/*!
    \qmlproperty enumeration QtQuick2::SmoothedAnimation::reversingMode

    Sets how the SmoothedAnimation behaves if an animation direction is reversed.

    Possible values are:

    \list
    \o SmoothedAnimation.Eased (default) - the animation will smoothly decelerate, and then reverse direction
    \o SmoothedAnimation.Immediate - the animation will immediately begin accelerating in the reverse direction, beginning with a velocity of 0
    \o SmoothedAnimation.Sync - the property is immediately set to the target value
    \endlist
*/
QDeclarativeSmoothedAnimation::ReversingMode QDeclarativeSmoothedAnimation::reversingMode() const
{
    Q_D(const QDeclarativeSmoothedAnimation);
    return (QDeclarativeSmoothedAnimation::ReversingMode) d->anim->reversingMode;
}

void QDeclarativeSmoothedAnimation::setReversingMode(ReversingMode m)
{
    Q_D(QDeclarativeSmoothedAnimation);
    if (d->anim->reversingMode == m)
        return;

    d->anim->reversingMode = m;
    emit reversingModeChanged();
    d->updateRunningAnimations();
}

/*!
    \qmlproperty int QtQuick2::SmoothedAnimation::duration

    This property holds the animation duration, in msecs, used when tracking the source.

    Setting this to -1 (the default) disables the duration value.

    If the velocity value and the duration value are both enabled, then the animation will
    use whichever gives the shorter duration.
*/
int QDeclarativeSmoothedAnimation::duration() const
{
    Q_D(const QDeclarativeSmoothedAnimation);
    return d->anim->userDuration;
}

void QDeclarativeSmoothedAnimation::setDuration(int duration)
{
    Q_D(QDeclarativeSmoothedAnimation);
    if (duration != -1)
        QDeclarativeNumberAnimation::setDuration(duration);
    if(duration == d->anim->userDuration)
        return;
    d->anim->userDuration = duration;
    d->updateRunningAnimations();
}

qreal QDeclarativeSmoothedAnimation::velocity() const
{
    Q_D(const QDeclarativeSmoothedAnimation);
    return d->anim->velocity;
}

/*!
    \qmlproperty real QtQuick2::SmoothedAnimation::velocity

    This property holds the average velocity allowed when tracking the 'to' value.

    The default velocity of SmoothedAnimation is 200 units/second.

    Setting this to -1 disables the velocity value.

    If the velocity value and the duration value are both enabled, then the animation will
    use whichever gives the shorter duration.
*/
void QDeclarativeSmoothedAnimation::setVelocity(qreal v)
{
    Q_D(QDeclarativeSmoothedAnimation);
    if (d->anim->velocity == v)
        return;

    d->anim->velocity = v;
    emit velocityChanged();
    d->updateRunningAnimations();
}

/*!
    \qmlproperty int QtQuick2::SmoothedAnimation::maximumEasingTime

    This property specifies the maximum time, in msecs, any "eases" during the follow should take.
    Setting this property causes the velocity to "level out" after at a time.  Setting
    a negative value reverts to the normal mode of easing over the entire animation
    duration.

    The default value is -1.
*/
int QDeclarativeSmoothedAnimation::maximumEasingTime() const
{
    Q_D(const QDeclarativeSmoothedAnimation);
    return d->anim->maximumEasingTime;
}

void QDeclarativeSmoothedAnimation::setMaximumEasingTime(int v)
{
    Q_D(QDeclarativeSmoothedAnimation);
    if(v == d->anim->maximumEasingTime)
        return;
    d->anim->maximumEasingTime = v;
    emit maximumEasingTimeChanged();
    d->updateRunningAnimations();
}

QT_END_NAMESPACE
