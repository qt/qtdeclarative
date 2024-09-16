// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickanimator_p_p.h"
#include "qquickanimatorjob_p.h"

#include <private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Animator
    \nativetype QQuickAnimator
    \inqmlmodule QtQuick
    \since 5.2
    \ingroup qtquick-transitions-animations
    \inherits Animation
    \brief Is the base of all QML animators.

    Animator types are a special type of animation which operate
    directly on Qt Quick's scene graph, rather than the QML objects and their
    properties like regular Animation types do. This has the benefit that
    Animator based animations can animate on the \l
    {Threaded Render Loop ('threaded')}{scene graph's rendering thread} even when the
    UI thread is blocked.

    The value of the QML property will be updated after the animation has
    finished. The property is not updated while the animation is running.

    The Animator types can be used just like any other Animation type.

    \snippet qml/animators.qml mixed

    If all sub-animations of ParallelAnimation and SequentialAnimation
    are Animator types, the ParallelAnimation and SequentialAnimation will
    also be treated as an Animator and be run on the scene graph's rendering
    thread when possible.

    The Animator types can be used for animations during transitions, but
    they do not support the \l {Transition::reversible}{reversible}
    property.

    The Animator type cannot be used directly in a QML file. It exists
    to provide a set of common properties and methods, available across all the
    other animator types that inherit from it. Attempting to use the Animator
    type directly will result in an error.
 */

QQuickAnimator::QQuickAnimator(QQuickAnimatorPrivate &dd, QObject *parent)
    : QQuickAbstractAnimation(dd, parent)
{
}

QQuickAnimator::QQuickAnimator(QObject *parent)
    : QQuickAbstractAnimation(*new QQuickAnimatorPrivate, parent)
{
}

/*!
   \qmlproperty QtQuick::Item QtQuick::Animator::target

   This property holds the target item of the animator.

   \note Animator targets must be Item based types.
 */

void QQuickAnimator::setTargetItem(QQuickItem *target)
{
    Q_D(QQuickAnimator);
    if (target == d->target)
        return;
    d->target = target;
    Q_EMIT targetItemChanged(d->target);
}

QQuickItem *QQuickAnimator::targetItem() const
{
    Q_D(const QQuickAnimator);
    return d->target;
}

/*!
    \qmlproperty int QtQuick::Animator::duration
    This property holds the duration of the animation in milliseconds.

    The default value is 250.
*/
void QQuickAnimator::setDuration(int duration)
{
    Q_D(QQuickAnimator);
    if (duration == d->duration)
        return;
    d->duration = duration;
    Q_EMIT durationChanged(duration);
}

int QQuickAnimator::duration() const
{
    Q_D(const QQuickAnimator);
    return d->duration;
}

/*!
    \qmlpropertygroup QtQuick::Animator::easing
    \qmlproperty enumeration QtQuick::Animator::easing.type
    \qmlproperty real QtQuick::Animator::easing.amplitude
    \qmlproperty real QtQuick::Animator::easing.overshoot
    \qmlproperty real QtQuick::Animator::easing.period
    \qmlproperty list<real> QtQuick::Animator::easing.bezierCurve
    \include qquickanimation.cpp propertyanimation.easing
*/

void QQuickAnimator::setEasing(const QEasingCurve &easing)
{
    Q_D(QQuickAnimator);
    if (easing == d->easing)
        return;
    d->easing = easing;
    Q_EMIT easingChanged(d->easing);
}

QEasingCurve QQuickAnimator::easing() const
{
    Q_D(const QQuickAnimator);
    return d->easing;
}

/*!
    \qmlproperty real QtQuick::Animator::to
    This property holds the end value for the animation.

    If the Animator is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the end state of the
    \l Transition, or the value of the property change that triggered the
    \l Behavior.
 */

void QQuickAnimator::setTo(qreal to)
{
    Q_D(QQuickAnimator);
    if (to == d->to)
        return;
    d->toIsDefined = true;
    d->to = to;
    Q_EMIT toChanged(d->to);
}

qreal QQuickAnimator::to() const
{
    Q_D(const QQuickAnimator);
    return d->to;
}

/*!
    \qmlproperty real QtQuick::Animator::from
    This property holds the starting value for the animation.

    If the Animator is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the starting state of the
    \l Transition, or the current value of the property at the moment the
    \l Behavior is triggered.

    \sa {Animation and Transitions in Qt Quick}
*/

void QQuickAnimator::setFrom(qreal from)
{
    Q_D(QQuickAnimator);
    d->fromIsDefined = true;
    if (from == d->from)
        return;
    d->from = from;
    Q_EMIT fromChanged(d->from);
}

qreal QQuickAnimator::from() const
{
    Q_D(const QQuickAnimator);
    return d->from;
}

void QQuickAnimatorPrivate::apply(QQuickAnimatorJob *job,
                                     const QString &propertyName,
                                     QQuickStateActions &actions,
                                     QQmlProperties &modified,
                                     QObject *defaultTarget)
{

    if (actions.size()) {
        for (int i=0; i<actions.size(); ++i) {
            QQuickStateAction &action = actions[i];
            if (action.property.name() != propertyName)
                continue;
            modified << action.property;

            job->setTarget(qobject_cast<QQuickItem *>(action.property.object()));

            if (fromIsDefined)
                job->setFrom(from);
            else if (action.fromValue.isValid())
                job->setFrom(action.fromValue.toReal());
            else
                job->setFrom(action.property.read().toReal());

            if (toIsDefined)
                job->setTo(to);
            else if (action.toValue.isValid())
                job->setTo(action.toValue.toReal());
            else
                job->setTo(action.property.read().toReal());

            // This magic line is in sync with what PropertyAnimation does
            // and prevents the animation to end up in the "completeList"
            // which forces action.toValue to be written directly to
            // the item when a transition is cancelled.
            action.fromValue = action.toValue;
       }
    }

    if (modified.isEmpty()) {
        job->setTarget(target);
        if (fromIsDefined)
            job->setFrom(from);
        job->setTo(to);
    }

    if (!job->target()) {
        if (defaultProperty.object())
            job->setTarget(qobject_cast<QQuickItem *>(defaultProperty.object()));
        else
            job->setTarget(qobject_cast<QQuickItem *>(defaultTarget));
    }

    if (modified.isEmpty() && !fromIsDefined && job->target())
        job->setFrom(job->target()->property(propertyName.toLatin1()).toReal());

    job->setDuration(duration);
    job->setLoopCount(loopCount);
    job->setEasingCurve(easing);
}

QAbstractAnimationJob *QQuickAnimator::transition(QQuickStateActions &actions,
                                                  QQmlProperties &modified,
                                                  TransitionDirection direction,
                                                  QObject *defaultTarget)
{
    Q_D(QQuickAnimator);

    if (d->defaultProperty.isValid() && propertyName() != d->defaultProperty.name()) {
        qmlWarning(this) << "property name conflict: \""
            << propertyName() << "\" != \"" << d->defaultProperty.name() << "\"";
        return nullptr;
    }

    // The animation system cannot handle backwards uncontrolled animations.
    if (direction == Backward)
        return nullptr;

    QQuickAnimatorJob *job = createJob();
    if (!job)
        return nullptr;

    d->apply(job, propertyName(), actions, modified, defaultTarget);

    if (!job->target()) {
        delete job;
        return nullptr;
    }

    return job;
}

/*!
    \qmltype XAnimator
    \nativetype QQuickXAnimator
    \inqmlmodule QtQuick
    \since 5.2
    \ingroup qtquick-transitions-animations
    \inherits Animator
    \brief The XAnimator type animates the x position of an Item.

    \l{Animator} types are different from normal Animation types. When
    using an Animator, the animation can be run in the render thread
    and the property value will jump to the end when the animation is
    complete.

    The value of Item::x is updated after the animation has finished.

    The following snippet shows how to use a XAnimator together
    with a Rectangle item.

    \snippet qml/animators.qml x target

    It is also possible to use the \c on keyword to tie the
    XAnimator directly to an Item instance.

    \snippet qml/animators.qml x on


 */

QQuickXAnimator::QQuickXAnimator(QObject *parent) : QQuickAnimator(parent) {}

QQuickAnimatorJob *QQuickXAnimator::createJob() const { return new QQuickXAnimatorJob(); }

/*!
    \qmltype YAnimator
    \nativetype QQuickYAnimator
    \inqmlmodule QtQuick
    \since 5.2
    \ingroup qtquick-transitions-animations
    \inherits Animator
    \brief The YAnimator type animates the y position of an Item.

    \l{Animator} types are different from normal Animation types. When
    using an Animator, the animation can be run in the render thread
    and the property value will jump to the end when the animation is
    complete.

    The value of Item::y is updated after the animation has finished.

    The following snippet shows how to use a YAnimator together
    with a Rectangle item.

    \snippet qml/animators.qml y target

    It is also possible to use the \c on keyword to tie the
    YAnimator directly to an Item instance.

    \snippet qml/animators.qml y on


 */

QQuickYAnimator::QQuickYAnimator(QObject *parent) : QQuickAnimator(parent) {}

QQuickAnimatorJob *QQuickYAnimator::createJob() const { return new QQuickYAnimatorJob(); }

/*!
    \qmltype ScaleAnimator
    \nativetype QQuickScaleAnimator
    \inqmlmodule QtQuick
    \since 5.2
    \ingroup qtquick-transitions-animations
    \inherits Animator
    \brief The ScaleAnimator type animates the scale factor of an Item.

    \l{Animator} types are different from normal Animation types. When
    using an Animator, the animation can be run in the render thread
    and the property value will jump to the end when the animation is
    complete.

    The value of Item::scale is updated after the animation has finished.

    The following snippet shows how to use a ScaleAnimator together
    with a Rectangle item.

    \snippet qml/animators.qml scale target

    It is also possible to use the \c on keyword to tie the
    ScaleAnimator directly to an Item instance.

    \snippet qml/animators.qml scale on

    \sa Item::transformOrigin, RotationAnimator
 */

QQuickScaleAnimator::QQuickScaleAnimator(QObject *parent) : QQuickAnimator(parent) {}

QQuickAnimatorJob *QQuickScaleAnimator::createJob() const { return new QQuickScaleAnimatorJob(); }

/*!
    \qmltype OpacityAnimator
    \nativetype QQuickOpacityAnimator
    \inqmlmodule QtQuick
    \since 5.2
    \ingroup qtquick-transitions-animations
    \inherits Animator
    \brief The OpacityAnimator type animates the opacity of an Item.

    \l{Animator} types are different from normal Animation types. When
    using an Animator, the animation can be run in the render thread
    and the property value will jump to the end when the animation is
    complete.

    The value of Item::opacity is updated after the animation has finished.

    The following snippet shows how to use a OpacityAnimator together
    with a Rectangle item.

    \snippet qml/animators.qml opacity target

    It is also possible to use the \c on keyword to tie the
    OpacityAnimator directly to an Item instance.

    \snippet qml/animators.qml opacity on

 */

QQuickOpacityAnimator::QQuickOpacityAnimator(QObject *parent) : QQuickAnimator(parent) {}

QQuickAnimatorJob *QQuickOpacityAnimator::createJob() const { return new QQuickOpacityAnimatorJob(); }

/*!
    \qmltype RotationAnimator
    \nativetype QQuickRotationAnimator
    \inqmlmodule QtQuick
    \since 5.2
    \ingroup qtquick-transitions-animations
    \inherits Animator
    \brief The RotationAnimator type animates the rotation of an Item.

    \l{Animator} types are different from normal Animation types. When
    using an Animator, the animation can be run in the render thread
    and the property value will jump to the end when the animation is
    complete.

    The value of Item::rotation is updated after the animation has finished.

    The following snippet shows how to use a RotationAnimator together
    with a Rectangle item.

    \snippet qml/animators.qml rotation target

    It is also possible to use the \c on keyword to tie the
    RotationAnimator directly to the \c rotation property of an Item
    instance.

    \snippet qml/animators.qml rotation on

    \sa Item::transformOrigin, ScaleAnimator
 */

QQuickRotationAnimator::QQuickRotationAnimator(QObject *parent)
    : QQuickAnimator(*new QQuickRotationAnimatorPrivate, parent)
{
}

QQuickAnimatorJob *QQuickRotationAnimator::createJob() const {
    Q_D(const QQuickRotationAnimator);
    QQuickRotationAnimatorJob *job = new QQuickRotationAnimatorJob();
    job->setDirection(d->direction);
    return job;
}

/*!
    \qmlproperty enumeration QtQuick::RotationAnimator::direction
    This property holds the direction of the rotation.

    Possible values are:

    \value RotationAnimator.Numerical
        (default) Rotate by linearly interpolating between the two numbers.
        A rotation from 10 to 350 will rotate 340 degrees clockwise.
    \value RotationAnimator.Clockwise
        Rotate clockwise between the two values
    \value RotationAnimator.Counterclockwise
        Rotate counterclockwise between the two values
    \value RotationAnimator.Shortest
        Rotate in the direction that produces the shortest animation path.
        A rotation from 10 to 350 will rotate 20 degrees counterclockwise.
*/
void QQuickRotationAnimator::setDirection(RotationDirection dir)
{
    Q_D(QQuickRotationAnimator);
    if (d->direction == dir)
        return;
    d->direction = dir;
    Q_EMIT directionChanged(d->direction);
}

QQuickRotationAnimator::RotationDirection QQuickRotationAnimator::direction() const
{
    Q_D(const QQuickRotationAnimator);
    return d->direction;
}

#if QT_CONFIG(quick_shadereffect)
/*!
    \qmltype UniformAnimator
    \nativetype QQuickUniformAnimator
    \inqmlmodule QtQuick
    \since 5.2
    \ingroup qtquick-transitions-animations
    \inherits Animator
    \brief The UniformAnimator type animates a uniform of a ShaderEffect.

    \l{Animator} types are different from normal Animation types. When
    using an Animator, the animation can be run in the render thread
    and the property value will jump to the end when the animation is
    complete.

    The value of the QML property defining the uniform is updated after
    the animation has finished.

    The following snippet shows how to use a UniformAnimator together
    with a ShaderEffect item.

    \snippet qml/animators.qml shader target

    It is also possible to use the \c on keyword to tie the
    UniformAnimator directly to a uniform of a ShaderEffect
    instance.

    \snippet qml/animators.qml shader on

    \sa ShaderEffect, ShaderEffectSource
 */

QQuickUniformAnimator::QQuickUniformAnimator(QObject *parent)
    : QQuickAnimator(*new QQuickUniformAnimatorPrivate, parent)
{
}

/*!
   \qmlproperty string QtQuick::UniformAnimator::uniform
   This property holds the name of the uniform to animate.

   The value of the uniform must correspond to both a property
   on the target ShaderEffect and must be a uniform of type
   \c float in the fragment or vertex shader.
 */
void QQuickUniformAnimator::setUniform(const QString &uniform)
{
    Q_D(QQuickUniformAnimator);
    if (d->uniform == uniform)
        return;
    d->uniform = uniform;
    Q_EMIT uniformChanged(d->uniform);
}

QString QQuickUniformAnimator::uniform() const
{
    Q_D(const QQuickUniformAnimator);
    return d->uniform;
}

QString QQuickUniformAnimator::propertyName() const
{
    Q_D(const QQuickUniformAnimator);
    if (!d->uniform.isEmpty())
        return d->uniform;
    return d->defaultProperty.name();
}

QQuickAnimatorJob *QQuickUniformAnimator::createJob() const
{
    QString u = propertyName();
    if (u.isEmpty())
        return nullptr;

    QQuickUniformAnimatorJob *job = new QQuickUniformAnimatorJob();
    job->setUniform(u.toLatin1());
    return job;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickanimator_p.cpp"
