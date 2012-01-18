/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qdeclarativespringanimation_p.h"

#include "qdeclarativeanimation_p_p.h"
#include <private/qdeclarativeproperty_p.h>
#include "private/qparallelanimationgroupjob_p.h"

#include <QtCore/qdebug.h>

#include <private/qobject_p.h>

#include <limits.h>
#include <math.h>

QT_BEGIN_NAMESPACE

class QDeclarativeSpringAnimationPrivate;
class Q_AUTOTEST_EXPORT QSpringAnimation : public QAbstractAnimationJob
{
    Q_DISABLE_COPY(QSpringAnimation)
public:
    QSpringAnimation(QDeclarativeSpringAnimationPrivate * = 0);

    ~QSpringAnimation();
    int duration() const;
    void restart();
    void init();

    qreal currentValue;
    qreal to;
    qreal velocity;
    int startTime;
    int dura;
    int lastTime;
    enum Mode {
        Track,
        Velocity,
        Spring
    };
    Mode mode;
    QDeclarativeProperty target;

    qreal velocityms;
    qreal maxVelocity;
    qreal mass;
    qreal spring;
    qreal damping;
    qreal epsilon;
    qreal modulus;

    bool useMass : 1;
    bool haveModulus : 1;
    bool useDelta : 1;
    typedef QHash<QDeclarativeProperty, QSpringAnimation*> ActiveAnimationHash;

protected:
    virtual void updateCurrentTime(int time);
    virtual void updateState(QAbstractAnimationJob::State, QAbstractAnimationJob::State);

private:
    QDeclarativeSpringAnimationPrivate *animationTemplate;
};

QSpringAnimation::QSpringAnimation(QDeclarativeSpringAnimationPrivate *priv)
    : QAbstractAnimationJob()
    , currentValue(0)
    , to(0)
    , velocity(0)
    , startTime(0)
    , dura(0)
    , lastTime(0)
    , mode(Track)
    , velocityms(0)
    , maxVelocity(0)
    , mass(1.0)
    , spring(0.)
    , damping(0.)
    , epsilon(0.01)
    , modulus(0.0)
    , useMass(false)
    , haveModulus(false)
    , useDelta(false)
    , animationTemplate(priv)
{
}

int QSpringAnimation::duration() const
{
    return dura;
}
void QSpringAnimation::restart()
{
    if (isRunning()) {
        useDelta = true;
        init();
    } else {
        useDelta = false;
        //init() will be triggered when group starts
    }
}

void QSpringAnimation::init()
{
    lastTime = 0;
}

void QSpringAnimation::updateCurrentTime(int time)
{
    if (mode == Track) {
        stop();
        return;
    }

    int elapsed = useDelta ? QDeclarativeAnimationTimer::instance()->currentDelta() : time - lastTime;
    if (useDelta)
        useDelta = false;

    if (!elapsed)
        return;

    int count = elapsed / 16;

    if (mode == Spring) {
        if (elapsed < 16) // capped at 62fps.
            return;
        lastTime = time - (elapsed - count * 16);
    } else {
        lastTime = time;
    }

    qreal srcVal = to;

    bool stopped = false;

    if (haveModulus) {
        currentValue = fmod(currentValue, modulus);
        srcVal = fmod(srcVal, modulus);
    }
    if (mode == Spring) {
        // Real men solve the spring DEs using RK4.
        // We'll do something much simpler which gives a result that looks fine.
        for (int i = 0; i < count; ++i) {
            qreal diff = srcVal - currentValue;
            if (haveModulus && qAbs(diff) > modulus / 2) {
                if (diff < 0)
                    diff += modulus;
                else
                    diff -= modulus;
            }
            if (useMass)
                velocity = velocity + (spring * diff - damping * velocity) / mass;
            else
                velocity = velocity + spring * diff - damping * velocity;
            if (maxVelocity > 0.) {
                // limit velocity
                if (velocity > maxVelocity)
                    velocity = maxVelocity;
                else if (velocity < -maxVelocity)
                    velocity = -maxVelocity;
            }
            currentValue += velocity * 16.0 / 1000.0;
            if (haveModulus) {
                currentValue = fmod(currentValue, modulus);
                if (currentValue < 0.0)
                    currentValue += modulus;
            }
        }
        if (qAbs(velocity) < epsilon && qAbs(srcVal - currentValue) < epsilon) {
            velocity = 0.0;
            currentValue = srcVal;
            stopped = true;
        }
    } else {
        qreal moveBy = elapsed * velocityms;
        qreal diff = srcVal - currentValue;
        if (haveModulus && qAbs(diff) > modulus / 2) {
            if (diff < 0)
                diff += modulus;
            else
                diff -= modulus;
        }
        if (diff > 0) {
            currentValue += moveBy;
            if (haveModulus)
                currentValue = fmod(currentValue, modulus);
        } else {
            currentValue -= moveBy;
            if (haveModulus && currentValue < 0.0)
                currentValue = fmod(currentValue, modulus) + modulus;
        }
        if (lastTime - startTime >= dura) {
            currentValue = to;
            stopped = true;
        }
    }

    qreal old_to = to;

    QDeclarativePropertyPrivate::write(target, currentValue,
                                       QDeclarativePropertyPrivate::BypassInterceptor |
                                       QDeclarativePropertyPrivate::DontRemoveBinding);

    if (stopped && old_to == to) // do not stop if we got restarted
        stop();
}

void QSpringAnimation::updateState(QAbstractAnimationJob::State newState, QAbstractAnimationJob::State /*oldState*/)
{
    if (newState == QAbstractAnimationJob::Running)
        init();
}


class QDeclarativeSpringAnimationPrivate : public QDeclarativePropertyAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeSpringAnimation)
public:
    QDeclarativeSpringAnimationPrivate()
    : QDeclarativePropertyAnimationPrivate()
    , velocityms(0)
    , maxVelocity(0)
    , mass(1.0)
    , spring(0.)
    , damping(0.)
    , epsilon(0.01)
    , modulus(0.0)
    , useMass(false)
    , haveModulus(false)
    , mode(QSpringAnimation::Track)
    {}

    void updateMode();
    qreal velocityms;
    qreal maxVelocity;
    qreal mass;
    qreal spring;
    qreal damping;
    qreal epsilon;
    qreal modulus;

    bool useMass : 1;
    bool haveModulus : 1;
    QSpringAnimation::Mode mode;

    QSpringAnimation::ActiveAnimationHash activeAnimations;
};

QSpringAnimation::~QSpringAnimation()
{
    if (animationTemplate) {
        QSpringAnimation::ActiveAnimationHash::iterator it =
                animationTemplate->activeAnimations.find(target);
        if (it != animationTemplate->activeAnimations.end() && it.value() == this)
            animationTemplate->activeAnimations.erase(it);
    }
}

void QDeclarativeSpringAnimationPrivate::updateMode()
{
    if (spring == 0. && maxVelocity == 0.)
        mode = QSpringAnimation::Track;
    else if (spring > 0.)
        mode = QSpringAnimation::Spring;
    else {
        mode = QSpringAnimation::Velocity;
        QSpringAnimation::ActiveAnimationHash::iterator it;
        for (it = activeAnimations.begin(); it != activeAnimations.end(); ++it) {
            QSpringAnimation *animation = *it;
            animation->startTime = 0;
            qreal dist = qAbs(animation->currentValue - animation->to);
            if (haveModulus && dist > modulus / 2)
                dist = modulus - fmod(dist, modulus);
            animation->dura = dist / velocityms;
        }
    }
}

/*!
    \qmlclass SpringAnimation QDeclarativeSpringAnimation
    \inqmlmodule QtQuick 2
    \ingroup qml-animation-transition
    \inherits NumberAnimation

    \brief The SpringAnimation element allows a property to track a value in a spring-like motion.

    SpringAnimation mimics the oscillatory behavior of a spring, with the appropriate \l spring constant to
    control the acceleration and the \l damping to control how quickly the effect dies away.

    You can also limit the maximum \l velocity of the animation.

    The following \l Rectangle moves to the position of the mouse using a
    SpringAnimation when the mouse is clicked. The use of the \l Behavior
    on the \c x and \c y values indicates that whenever these values are
    changed, a SpringAnimation should be applied.

    \snippet doc/src/snippets/declarative/springanimation.qml 0

    Like any other animation element, a SpringAnimation can be applied in a
    number of ways, including transitions, behaviors and property value
    sources. The \l {QML Animation and Transitions} documentation shows a
    variety of methods for creating animations.

    \sa SmoothedAnimation, {QML Animation and Transitions}, {declarative/animation/basics}{Animation basics example}, {declarative/toys/clocks}{Clocks example}
*/

QDeclarativeSpringAnimation::QDeclarativeSpringAnimation(QObject *parent)
: QDeclarativeNumberAnimation(*(new QDeclarativeSpringAnimationPrivate),parent)
{
}

QDeclarativeSpringAnimation::~QDeclarativeSpringAnimation()
{
}

/*!
    \qmlproperty real QtQuick2::SpringAnimation::velocity

    This property holds the maximum velocity allowed when tracking the source.

    The default value is 0 (no maximum velocity).
*/

qreal QDeclarativeSpringAnimation::velocity() const
{
    Q_D(const QDeclarativeSpringAnimation);
    return d->maxVelocity;
}

void QDeclarativeSpringAnimation::setVelocity(qreal velocity)
{
    Q_D(QDeclarativeSpringAnimation);
    d->maxVelocity = velocity;
    d->velocityms = velocity / 1000.0;
    d->updateMode();
}

/*!
    \qmlproperty real QtQuick2::SpringAnimation::spring

    This property describes how strongly the target is pulled towards the
    source. The default value is 0 (that is, the spring-like motion is disabled).

    The useful value range is 0 - 5.0.

    When this property is set and the \l velocity value is greater than 0,
    the \l velocity limits the maximum speed.
*/
qreal QDeclarativeSpringAnimation::spring() const
{
    Q_D(const QDeclarativeSpringAnimation);
    return d->spring;
}

void QDeclarativeSpringAnimation::setSpring(qreal spring)
{
    Q_D(QDeclarativeSpringAnimation);
    d->spring = spring;
    d->updateMode();
}

/*!
    \qmlproperty real QtQuick2::SpringAnimation::damping
    This property holds the spring damping value.

    This value describes how quickly the spring-like motion comes to rest.
    The default value is 0.

    The useful value range is 0 - 1.0. The lower the value, the faster it
    comes to rest.
*/
qreal QDeclarativeSpringAnimation::damping() const
{
    Q_D(const QDeclarativeSpringAnimation);
    return d->damping;
}

void QDeclarativeSpringAnimation::setDamping(qreal damping)
{
    Q_D(QDeclarativeSpringAnimation);
    if (damping > 1.)
        damping = 1.;

    d->damping = damping;
}


/*!
    \qmlproperty real QtQuick2::SpringAnimation::epsilon
    This property holds the spring epsilon.

    The epsilon is the rate and amount of change in the value which is close enough
    to 0 to be considered equal to zero. This will depend on the usage of the value.
    For pixel positions, 0.25 would suffice. For scale, 0.005 will suffice.

    The default is 0.01. Tuning this value can provide small performance improvements.
*/
qreal QDeclarativeSpringAnimation::epsilon() const
{
    Q_D(const QDeclarativeSpringAnimation);
    return d->epsilon;
}

void QDeclarativeSpringAnimation::setEpsilon(qreal epsilon)
{
    Q_D(QDeclarativeSpringAnimation);
    d->epsilon = epsilon;
}

/*!
    \qmlproperty real QtQuick2::SpringAnimation::modulus
    This property holds the modulus value. The default value is 0.

    Setting a \a modulus forces the target value to "wrap around" at the modulus.
    For example, setting the modulus to 360 will cause a value of 370 to wrap around to 10.
*/
qreal QDeclarativeSpringAnimation::modulus() const
{
    Q_D(const QDeclarativeSpringAnimation);
    return d->modulus;
}

void QDeclarativeSpringAnimation::setModulus(qreal modulus)
{
    Q_D(QDeclarativeSpringAnimation);
    if (d->modulus != modulus) {
        d->haveModulus = modulus != 0.0;
        d->modulus = modulus;
        d->updateMode();
        emit modulusChanged();
    }
}

/*!
    \qmlproperty real QtQuick2::SpringAnimation::mass
    This property holds the "mass" of the property being moved.

    The value is 1.0 by default.

    A greater mass causes slower movement and a greater spring-like
    motion when an item comes to rest.
*/
qreal QDeclarativeSpringAnimation::mass() const
{
    Q_D(const QDeclarativeSpringAnimation);
    return d->mass;
}

void QDeclarativeSpringAnimation::setMass(qreal mass)
{
    Q_D(QDeclarativeSpringAnimation);
    if (d->mass != mass && mass > 0.0) {
        d->useMass = mass != 1.0;
        d->mass = mass;
        emit massChanged();
    }
}

QAbstractAnimationJob* QDeclarativeSpringAnimation::transition(QDeclarativeStateActions &actions,
                                                                   QDeclarativeProperties &modified,
                                                                   TransitionDirection direction)
{
    Q_D(QDeclarativeSpringAnimation);
    Q_UNUSED(direction);

    QParallelAnimationGroupJob *wrapperGroup = new QParallelAnimationGroupJob();

    QDeclarativeStateActions dataActions = QDeclarativeNumberAnimation::createTransitionActions(actions, modified);
    if (!dataActions.isEmpty()) {
        QSet<QAbstractAnimationJob*> anims;
        for (int i = 0; i < dataActions.size(); ++i) {
            QSpringAnimation *animation;
            bool needsRestart = false;
            const QDeclarativeProperty &property = dataActions.at(i).property;
            if (d->activeAnimations.contains(property)) {
                animation = d->activeAnimations[property];
                needsRestart = true;
            } else {
                animation = new QSpringAnimation();
                d->activeAnimations.insert(property, animation);
                animation->target = property;
            }
            wrapperGroup->appendAnimation(initInstance(animation));

            animation->to = dataActions.at(i).toValue.toReal();
            animation->startTime = 0;
            animation->velocityms = d->velocityms;
            animation->mass = d->mass;
            animation->spring = d->spring;
            animation->damping = d->damping;
            animation->epsilon = d->epsilon;
            animation->modulus = d->modulus;
            animation->useMass = d->useMass;
            animation->haveModulus = d->haveModulus;
            animation->mode = d->mode;
            animation->dura = -1;
            animation->maxVelocity = d->maxVelocity;

            if (d->fromIsDefined)
                animation->currentValue = dataActions.at(i).fromValue.toReal();
            else
                animation->currentValue = property.read().toReal();
            if (animation->mode == QSpringAnimation::Velocity) {
                qreal dist = qAbs(animation->currentValue - animation->to);
                if (d->haveModulus && dist > d->modulus / 2)
                    dist = d->modulus - fmod(dist, d->modulus);
                animation->dura = dist / animation->velocityms;
            }

            if (needsRestart)
                animation->restart();
            anims.insert(animation);
        }
        foreach (QSpringAnimation *anim, d->activeAnimations.values()){
            if (!anims.contains(anim))
                d->activeAnimations.remove(anim->target);
        }
    }
    return wrapperGroup;
}

QT_END_NAMESPACE
