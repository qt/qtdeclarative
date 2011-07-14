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

#include "QtQuick1/private/qdeclarativespringanimation_p.h"

#include "QtQuick1/private/qdeclarativeanimation_p_p.h"
#include <QtDeclarative/private/qdeclarativeproperty_p.h>

#include <QtCore/qdebug.h>

#include <private/qobject_p.h>

#include <limits.h>
#include <math.h>

QT_BEGIN_NAMESPACE




class QDeclarative1SpringAnimationPrivate : public QDeclarative1PropertyAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1SpringAnimation)
public:


    struct SpringAnimation {
        SpringAnimation()
            : currentValue(0), to(0), velocity(0), start(0), duration(0) {}
        qreal currentValue;
        qreal to;
        qreal velocity;
        int start;
        int duration;
    };
    QHash<QDeclarativeProperty, SpringAnimation> activeAnimations;

    qreal maxVelocity;
    qreal velocityms;
    int lastTime;
    qreal mass;
    qreal spring;
    qreal damping;
    qreal epsilon;
    qreal modulus;

    bool useMass : 1;
    bool haveModulus : 1;

    enum Mode {
        Track,
        Velocity,
        Spring
    };
    Mode mode;

    QDeclarative1SpringAnimationPrivate()
          : maxVelocity(0), velocityms(0), lastTime(0)
          , mass(1.0), spring(0.), damping(0.), epsilon(0.01)
          , modulus(0.0), useMass(false), haveModulus(false)
          , mode(Track), clock(0)
    { }

    void tick(int time);
    bool animate(const QDeclarativeProperty &property, SpringAnimation &animation, int elapsed);
    void updateMode();

    typedef QTickAnimationProxy_1<QDeclarative1SpringAnimationPrivate, &QDeclarative1SpringAnimationPrivate::tick> Clock;
    Clock *clock;
};

void QDeclarative1SpringAnimationPrivate::tick(int time)
{
    if (mode == Track) {
        clock->stop();
        return;
    }
    int elapsed = time - lastTime;
    if (!elapsed)
        return;

    if (mode == Spring) {
        if (elapsed < 16) // capped at 62fps.
            return;
        int count = elapsed / 16;
        lastTime = time - (elapsed - count * 16);
    } else {
        lastTime = time;
    }

    QMutableHashIterator<QDeclarativeProperty, SpringAnimation> it(activeAnimations);
    while (it.hasNext()) {
        it.next();
        if (animate(it.key(), it.value(), elapsed))
            it.remove();
    }

    if (activeAnimations.isEmpty())
        clock->stop();
}

bool QDeclarative1SpringAnimationPrivate::animate(const QDeclarativeProperty &property, SpringAnimation &animation, int elapsed)
{
    qreal srcVal = animation.to;

    bool stop = false;

    if (haveModulus) {
        animation.currentValue = fmod(animation.currentValue, modulus);
        srcVal = fmod(srcVal, modulus);
    }
    if (mode == Spring) {
        // Real men solve the spring DEs using RK4.
        // We'll do something much simpler which gives a result that looks fine.
        int count = elapsed / 16;
        for (int i = 0; i < count; ++i) {
            qreal diff = srcVal - animation.currentValue;
            if (haveModulus && qAbs(diff) > modulus / 2) {
                if (diff < 0)
                    diff += modulus;
                else
                    diff -= modulus;
            }
            if (useMass)
                animation.velocity = animation.velocity + (spring * diff - damping * animation.velocity) / mass;
            else
                animation.velocity = animation.velocity + spring * diff - damping * animation.velocity;
            if (maxVelocity > 0.) {
                // limit velocity
                if (animation.velocity > maxVelocity)
                    animation.velocity = maxVelocity;
                else if (animation.velocity < -maxVelocity)
                    animation.velocity = -maxVelocity;
            }
            animation.currentValue += animation.velocity * 16.0 / 1000.0;
            if (haveModulus) {
                animation.currentValue = fmod(animation.currentValue, modulus);
                if (animation.currentValue < 0.0)
                    animation.currentValue += modulus;
            }
        }
        if (qAbs(animation.velocity) < epsilon && qAbs(srcVal - animation.currentValue) < epsilon) {
            animation.velocity = 0.0;
            animation.currentValue = srcVal;
            stop = true;
        }
    } else {
        qreal moveBy = elapsed * velocityms;
        qreal diff = srcVal - animation.currentValue;
        if (haveModulus && qAbs(diff) > modulus / 2) {
            if (diff < 0)
                diff += modulus;
            else
                diff -= modulus;
        }
        if (diff > 0) {
            animation.currentValue += moveBy;
            if (haveModulus)
                animation.currentValue = fmod(animation.currentValue, modulus);
        } else {
            animation.currentValue -= moveBy;
            if (haveModulus && animation.currentValue < 0.0)
                animation.currentValue = fmod(animation.currentValue, modulus) + modulus;
        }
        if (lastTime - animation.start >= animation.duration) {
            animation.currentValue = animation.to;
            stop = true;
        }
    }

    qreal old_to = animation.to;

    QDeclarativePropertyPrivate::write(property, animation.currentValue,
                                       QDeclarativePropertyPrivate::BypassInterceptor |
                                       QDeclarativePropertyPrivate::DontRemoveBinding);

    return (stop && old_to == animation.to); // do not stop if we got restarted
}

void QDeclarative1SpringAnimationPrivate::updateMode()
{
    if (spring == 0. && maxVelocity == 0.)
        mode = Track;
    else if (spring > 0.)
        mode = Spring;
    else {
        mode = Velocity;
        QHash<QDeclarativeProperty, SpringAnimation>::iterator it;
        for (it = activeAnimations.begin(); it != activeAnimations.end(); ++it) {
            SpringAnimation &animation = *it;
            animation.start = lastTime;
            qreal dist = qAbs(animation.currentValue - animation.to);
            if (haveModulus && dist > modulus / 2)
                dist = modulus - fmod(dist, modulus);
            animation.duration = dist / velocityms;
        }
    }
}

/*!
    \qmlclass SpringAnimation QDeclarative1SpringAnimation
    \inqmlmodule QtQuick 1
    \ingroup qml-animation-transition
    \inherits NumberAnimation
    \since QtQuick 1.0

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

QDeclarative1SpringAnimation::QDeclarative1SpringAnimation(QObject *parent)
: QDeclarative1NumberAnimation(*(new QDeclarative1SpringAnimationPrivate),parent)
{
    Q_D(QDeclarative1SpringAnimation);
    d->clock = new QDeclarative1SpringAnimationPrivate::Clock(d, this);
}

QDeclarative1SpringAnimation::~QDeclarative1SpringAnimation()
{
}

/*!
    \qmlproperty real QtQuick1::SpringAnimation::velocity

    This property holds the maximum velocity allowed when tracking the source.

    The default value is 0 (no maximum velocity).
*/

qreal QDeclarative1SpringAnimation::velocity() const
{
    Q_D(const QDeclarative1SpringAnimation);
    return d->maxVelocity;
}

void QDeclarative1SpringAnimation::setVelocity(qreal velocity)
{
    Q_D(QDeclarative1SpringAnimation);
    d->maxVelocity = velocity;
    d->velocityms = velocity / 1000.0;
    d->updateMode();
}

/*!
    \qmlproperty real QtQuick1::SpringAnimation::spring

    This property describes how strongly the target is pulled towards the
    source. The default value is 0 (that is, the spring-like motion is disabled).

    The useful value range is 0 - 5.0.

    When this property is set and the \l velocity value is greater than 0,
    the \l velocity limits the maximum speed.
*/
qreal QDeclarative1SpringAnimation::spring() const
{
    Q_D(const QDeclarative1SpringAnimation);
    return d->spring;
}

void QDeclarative1SpringAnimation::setSpring(qreal spring)
{
    Q_D(QDeclarative1SpringAnimation);
    d->spring = spring;
    d->updateMode();
}

/*!
    \qmlproperty real QtQuick1::SpringAnimation::damping
    This property holds the spring damping value.

    This value describes how quickly the spring-like motion comes to rest.
    The default value is 0.

    The useful value range is 0 - 1.0. The lower the value, the faster it
    comes to rest.
*/
qreal QDeclarative1SpringAnimation::damping() const
{
    Q_D(const QDeclarative1SpringAnimation);
    return d->damping;
}

void QDeclarative1SpringAnimation::setDamping(qreal damping)
{
    Q_D(QDeclarative1SpringAnimation);
    if (damping > 1.)
        damping = 1.;

    d->damping = damping;
}


/*!
    \qmlproperty real QtQuick1::SpringAnimation::epsilon
    This property holds the spring epsilon.

    The epsilon is the rate and amount of change in the value which is close enough
    to 0 to be considered equal to zero. This will depend on the usage of the value.
    For pixel positions, 0.25 would suffice. For scale, 0.005 will suffice.

    The default is 0.01. Tuning this value can provide small performance improvements.
*/
qreal QDeclarative1SpringAnimation::epsilon() const
{
    Q_D(const QDeclarative1SpringAnimation);
    return d->epsilon;
}

void QDeclarative1SpringAnimation::setEpsilon(qreal epsilon)
{
    Q_D(QDeclarative1SpringAnimation);
    d->epsilon = epsilon;
}

/*!
    \qmlproperty real QtQuick1::SpringAnimation::modulus
    This property holds the modulus value. The default value is 0.

    Setting a \a modulus forces the target value to "wrap around" at the modulus.
    For example, setting the modulus to 360 will cause a value of 370 to wrap around to 10.
*/
qreal QDeclarative1SpringAnimation::modulus() const
{
    Q_D(const QDeclarative1SpringAnimation);
    return d->modulus;
}

void QDeclarative1SpringAnimation::setModulus(qreal modulus)
{
    Q_D(QDeclarative1SpringAnimation);
    if (d->modulus != modulus) {
        d->haveModulus = modulus != 0.0;
        d->modulus = modulus;
        d->updateMode();
        emit modulusChanged();
    }
}

/*!
    \qmlproperty real QtQuick1::SpringAnimation::mass
    This property holds the "mass" of the property being moved.

    The value is 1.0 by default.

    A greater mass causes slower movement and a greater spring-like
    motion when an item comes to rest.
*/
qreal QDeclarative1SpringAnimation::mass() const
{
    Q_D(const QDeclarative1SpringAnimation);
    return d->mass;
}

void QDeclarative1SpringAnimation::setMass(qreal mass)
{
    Q_D(QDeclarative1SpringAnimation);
    if (d->mass != mass && mass > 0.0) {
        d->useMass = mass != 1.0;
        d->mass = mass;
        emit massChanged();
    }
}

void QDeclarative1SpringAnimation::transition(QDeclarative1StateActions &actions,
                                             QDeclarativeProperties &modified,
                                             TransitionDirection direction)
{
    Q_D(QDeclarative1SpringAnimation);
    Q_UNUSED(direction);

    if (d->clock->state() != QAbstractAnimation::Running) {
        d->lastTime = 0;
    }

    QDeclarative1NumberAnimation::transition(actions, modified, direction);

    if (!d->actions)
        return;

    if (!d->actions->isEmpty()) {
        for (int i = 0; i < d->actions->size(); ++i) {
            const QDeclarativeProperty &property = d->actions->at(i).property;
            QDeclarative1SpringAnimationPrivate::SpringAnimation &animation
                    = d->activeAnimations[property];
            animation.to = d->actions->at(i).toValue.toReal();
            animation.start = d->lastTime;
            if (d->fromIsDefined)
                animation.currentValue = d->actions->at(i).fromValue.toReal();
            else
                animation.currentValue = property.read().toReal();
            if (d->mode == QDeclarative1SpringAnimationPrivate::Velocity) {
                qreal dist = qAbs(animation.currentValue - animation.to);
                if (d->haveModulus && dist > d->modulus / 2)
                    dist = d->modulus - fmod(dist, d->modulus);
                animation.duration = dist / d->velocityms;
            }
        }
    }
}


QAbstractAnimation *QDeclarative1SpringAnimation::qtAnimation()
{
    Q_D(QDeclarative1SpringAnimation);
    return d->clock;
}



QT_END_NAMESPACE
