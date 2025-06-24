// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktransition_p.h"

#include "qquickstate_p.h"
#include "qquickstate_p_p.h"
#include "qquickstatechangescript_p.h"
#include "qquickanimation_p.h"
#include "qquickanimation_p_p.h"
#include "qquicktransitionmanager_p_p.h"

#include <private/qquickanimatorjob_p.h>

#include "private/qparallelanimationgroupjob_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Transition
    \nativetype QQuickTransition
    \inqmlmodule QtQuick
    \ingroup qtquick-transitions-animations
    \brief Defines animated transitions that occur on state changes.

    A Transition defines the animations to be applied when a \l State change occurs.

    For example, the following \l Rectangle has two states: the default state, and
    an added "moved" state. In the "moved state, the rectangle's position changes
    to (50, 50).  The added Transition specifies that when the rectangle
    changes between the default and the "moved" state, any changes
    to the \c x and \c y properties should be animated, using an \c Easing.InOutQuad.

    \snippet qml/transition.qml 0

    Notice the example does not require \l{PropertyAnimation::}{to} and
    \l{PropertyAnimation::}{from} values for the NumberAnimation. As a convenience,
    these properties are automatically set to the values of \c x and \c y before
    and after the state change; the \c from values are provided by
    the current values of \c x and \c y, and the \c to values are provided by
    the PropertyChanges object. If you wish, you can provide \l{PropertyAnimation::}{to} and
    \l{PropertyAnimation::}{from} values anyway to override the default values.

    By default, a Transition's animations are applied for any state change in the
    parent item. The  Transition \l {Transition::}{from} and \l {Transition::}{to}
    values can be set to restrict the animations to only be applied when changing
    from one particular state to another.

    Top-level animations within a transition are run in parallel. To run them
    sequentially, define them within a SequentialAnimation:

    \snippet qml/transition-reversible.qml sequential animations

    To define multiple Transitions, specify \l Item::transitions as a list:

    \snippet qml/transitions-list.qml list of transitions

    If multiple Transitions are specified, only a single (best-matching)
    Transition will be applied for any particular state change. In the
    example above, if the Rectangle enters a state other than \c "middleRight"
    or \c "bottomLeft", the third Transition will be carried out, meaning the
    icon will be moved to the starting point.

    If a state change has a Transition that matches the same property as a
    \l Behavior, the Transition animation overrides the \l Behavior for that
    state change.

    \sa {Animation and Transitions in Qt Quick}, {Qt Quick Examples - Animation#States}{States example}, {Qt Quick States}, {Qt Qml}
*/

//ParallelAnimationWrapper allows us to do a "callback" when the animation finishes, rather than connecting
//and disconnecting signals and slots frequently
class ParallelAnimationWrapper : public QParallelAnimationGroupJob
{
public:
    ParallelAnimationWrapper() : QParallelAnimationGroupJob() {}
    QQuickTransitionManager *manager;

protected:
    void updateState(QAbstractAnimationJob::State newState, QAbstractAnimationJob::State oldState) override;
};

class QQuickTransitionPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickTransition)
public:
    QQuickTransitionPrivate()
    : fromState(QLatin1String("*")), toState(QLatin1String("*"))
    , runningInstanceCount(0), state(QAbstractAnimationJob::Stopped)
    , reversed(false), reversible(false), enabled(true)
    {
    }

    static QQuickTransitionPrivate *get(QQuickTransition *q) { return q->d_func(); }
    void animationStateChanged(QAbstractAnimationJob::State newState);

    QString fromState;
    QString toState;
    quint32 runningInstanceCount;
    QAbstractAnimationJob::State state;
    bool reversed;
    bool reversible;
    bool enabled;
protected:

    static void append_animation(QQmlListProperty<QQuickAbstractAnimation> *list, QQuickAbstractAnimation *a);
    static qsizetype animation_count(QQmlListProperty<QQuickAbstractAnimation> *list);
    static QQuickAbstractAnimation* animation_at(QQmlListProperty<QQuickAbstractAnimation> *list, qsizetype pos);
    static void clear_animations(QQmlListProperty<QQuickAbstractAnimation> *list);
    static void removeLast_animation(QQmlListProperty<QQuickAbstractAnimation> *list);
    static void replace_animation(
            QQmlListProperty<QQuickAbstractAnimation> *list, qsizetype pos,
            QQuickAbstractAnimation *a);

    QList<QPointer<QQuickAbstractAnimation>> animations;
};

void QQuickTransitionPrivate::append_animation(QQmlListProperty<QQuickAbstractAnimation> *list, QQuickAbstractAnimation *a)
{
    QQuickTransition *q = static_cast<QQuickTransition *>(list->object);
    q->d_func()->animations.append(a);
    if (a)
        a->setDisableUserControl();
}

qsizetype QQuickTransitionPrivate::animation_count(QQmlListProperty<QQuickAbstractAnimation> *list)
{
    QQuickTransition *q = static_cast<QQuickTransition *>(list->object);
    return q->d_func()->animations.size();
}

QQuickAbstractAnimation* QQuickTransitionPrivate::animation_at(QQmlListProperty<QQuickAbstractAnimation> *list, qsizetype pos)
{
    QQuickTransition *q = static_cast<QQuickTransition *>(list->object);
    return q->d_func()->animations.at(pos);
}

void QQuickTransitionPrivate::clear_animations(QQmlListProperty<QQuickAbstractAnimation> *list)
{
    QQuickTransition *q = static_cast<QQuickTransition *>(list->object);
    q->d_func()->animations.clear();
}

void QQuickTransitionPrivate::removeLast_animation(QQmlListProperty<QQuickAbstractAnimation> *list)
{
    QQuickTransition *q = static_cast<QQuickTransition *>(list->object);
    q->d_func()->animations.removeLast();
}

void QQuickTransitionPrivate::replace_animation(
        QQmlListProperty<QQuickAbstractAnimation> *list, qsizetype pos, QQuickAbstractAnimation *a)
{
    QQuickTransition *q = static_cast<QQuickTransition *>(list->object);
    QQuickTransitionPrivate *d = q->d_func();
    if (d->animations.length() <= pos)
        d->animations.resize(pos + 1, nullptr);
    d->animations[pos] = a;
    if (a)
        a->setDisableUserControl();
}

void QQuickTransitionInstance::animationStateChanged(QAbstractAnimationJob *, QAbstractAnimationJob::State newState, QAbstractAnimationJob::State)
{
    if (!m_transition)
        return;

    QQuickTransitionPrivate *transition = QQuickTransitionPrivate::get(m_transition);
    transition->animationStateChanged(newState);
}

void QQuickTransitionPrivate::animationStateChanged(QAbstractAnimationJob::State newState)
{
    Q_Q(QQuickTransition);

    if (newState == QAbstractAnimationJob::Running) {
        runningInstanceCount++;
        if (runningInstanceCount == 1)
            emit q->runningChanged();
    } else if (newState == QAbstractAnimationJob::Stopped) {
        runningInstanceCount--;
        if (runningInstanceCount == 0)
            emit q->runningChanged();
    }
}

void ParallelAnimationWrapper::updateState(QAbstractAnimationJob::State newState, QAbstractAnimationJob::State oldState)
{
    QParallelAnimationGroupJob::updateState(newState, oldState);
    if (newState == Stopped && (duration() == -1
        || (direction() == QAbstractAnimationJob::Forward && currentLoopTime() == duration())
        || (direction() == QAbstractAnimationJob::Backward && currentLoopTime() == 0)))
    {
         manager->complete();
    }
}

QQuickTransitionInstance::QQuickTransitionInstance(QQuickTransition *transition, QAbstractAnimationJob *anim)
    : m_transition(transition)
    , m_anim(anim)
{
    anim->addAnimationChangeListener(this, QAbstractAnimationJob::StateChange);
}

QQuickTransitionInstance::~QQuickTransitionInstance()
{
    removeStateChangeListener();
    delete m_anim;
}

void QQuickTransitionInstance::start()
{
    if (m_anim)
        m_anim->start();
}

void QQuickTransitionInstance::stop()
{
    if (m_anim)
        m_anim->stop();
}

void QQuickTransitionInstance::complete()
{
    if (m_anim)
        m_anim->complete();
}

bool QQuickTransitionInstance::isRunning() const
{
    return m_anim && m_anim->state() == QAbstractAnimationJob::Running;
}

QQuickTransition::QQuickTransition(QObject *parent)
    : QObject(*(new QQuickTransitionPrivate), parent)
{
}

QQuickTransition::~QQuickTransition()
{
}

void QQuickTransition::setReversed(bool r)
{
    Q_D(QQuickTransition);
    d->reversed = r;
}

QQuickTransitionInstance *QQuickTransition::prepare(QQuickStateOperation::ActionList &actions,
                            QList<QQmlProperty> &after,
                            QQuickTransitionManager *manager,
                            QObject *defaultTarget)
{
    Q_D(QQuickTransition);

    qmlExecuteDeferred(this);

    ParallelAnimationWrapper *group = new ParallelAnimationWrapper();
    group->manager = manager;

    QQuickAbstractAnimation::TransitionDirection direction = d->reversed ? QQuickAbstractAnimation::Backward : QQuickAbstractAnimation::Forward;
    int start = d->reversed ? d->animations.size() - 1 : 0;
    int end = d->reversed ? -1 : d->animations.size();

    for (int i = start; i != end; d->reversed ? --i : ++i) {
        QQuickAbstractAnimation *anim = d->animations.at(i);
        if (!anim)
            continue;

        QAbstractAnimationJob *job = anim->transition(actions, after, direction, defaultTarget);
        if (!job)
            continue;

        if (anim->threadingModel() == QQuickAbstractAnimation::RenderThread)
            job = new QQuickAnimatorProxyJob(job, anim);

        d->reversed ? group->prependAnimation(job) : group->appendAnimation(job);
    }

    group->setDirection(d->reversed ? QAbstractAnimationJob::Backward : QAbstractAnimationJob::Forward);

    QQuickTransitionInstance *wrapper = new QQuickTransitionInstance(this, group);
    return wrapper;
}

/*!
    \qmlproperty string QtQuick::Transition::from
    \qmlproperty string QtQuick::Transition::to

    These properties indicate the state changes that trigger the transition.

    The default values for these properties is "*" (that is, any state).

    For example, the following transition has not set the \c to and \c from
    properties, so the animation is always applied when changing between
    the two states (i.e. when the mouse is pressed and released).

    \snippet qml/transition-from-to.qml 0

    If the transition was changed to this:

    \snippet qml/transition-from-to-modified.qml modified transition

    The animation would only be applied when changing from the default state
    to the "brighter" state (i.e. when the mouse is pressed, but not on release).

    Multiple \c to and \c from values can be set by using a comma-separated
    string.

    \sa reversible
*/
QString QQuickTransition::fromState() const
{
    Q_D(const QQuickTransition);
    return d->fromState;
}

void QQuickTransition::setFromState(const QString &f)
{
    Q_D(QQuickTransition);
    if (f == d->fromState)
        return;

    d->fromState = f;
    emit fromChanged();
}

/*!
    \qmlproperty bool QtQuick::Transition::reversible
    This property holds whether the transition should be automatically
    reversed when the conditions that triggered this transition are reversed.

    The default value is false.

    By default, transitions run in parallel and are applied to all state
    changes if the \l from and \l to states have not been set. In this
    situation, the transition is automatically applied when a state change
    is reversed, and it is not necessary to set this property to reverse
    the transition.

    However, if a SequentialAnimation is used, or if the \l from or \l to
    properties have been set, this property will need to be set to reverse
    a transition when a state change is reverted. For example, the following
    transition applies a sequential animation when the mouse is pressed,
    and reverses the sequence of the animation when the mouse is released:

    \snippet qml/transition-reversible.qml 0

    If the transition did not set the \c to and \c reversible values, then
    on the mouse release, the transition would play the PropertyAnimation
    before the ColorAnimation instead of reversing the sequence.
*/
bool QQuickTransition::reversible() const
{
    Q_D(const QQuickTransition);
    return d->reversible;
}

void QQuickTransition::setReversible(bool r)
{
    Q_D(QQuickTransition);
    if (r == d->reversible)
        return;

    d->reversible = r;
    emit reversibleChanged();
}

QString QQuickTransition::toState() const
{
    Q_D(const QQuickTransition);
    return d->toState;
}

void QQuickTransition::setToState(const QString &t)
{
    Q_D(QQuickTransition);
    if (t == d->toState)
        return;

    d->toState = t;
    emit toChanged();
}

/*!
    \qmlproperty bool QtQuick::Transition::enabled

    This property holds whether the Transition will be run when moving
    from the \c from state to the \c to state.

    By default a Transition is enabled.

    Note that in some circumstances disabling a Transition may cause an
    alternative Transition to be used in its place. In the following
    example, although the first Transition has been set to animate changes
    from "state1" to "state2", this transition has been disabled by setting
    \c enabled to \c false, so any such state change will actually be animated
    by the second Transition instead.

    \qml
    Item {
        states: [
            State { name: "state1" },
            State { name: "state2" }
        ]
        transitions: [
            Transition { from: "state1"; to: "state2"; enabled: false },
            Transition {
                // ...
            }
        ]
    }
    \endqml
*/

bool QQuickTransition::enabled() const
{
    Q_D(const QQuickTransition);
    return d->enabled;
}

void QQuickTransition::setEnabled(bool enabled)
{
    Q_D(QQuickTransition);
    if (d->enabled == enabled)
        return;
    d->enabled = enabled;
    emit enabledChanged();
}

/*!
    \qmlproperty bool QtQuick::Transition::running
    \readonly

    This property holds whether the transition is currently running.

    \note Unlike Animation::running, this property is read only,
    and can not be used to control the transition.
*/
bool QQuickTransition::running() const
{
    Q_D(const QQuickTransition);
    return d->runningInstanceCount;
}


/*!
    \qmlproperty list<Animation> QtQuick::Transition::animations
    \qmldefault

    This property holds a list of the animations to be run for this transition.

    \snippet qml/transition-animation.qml 0

    The top-level animations are run in parallel. To run them sequentially,
    define them within a SequentialAnimation:

    \snippet qml/transition-reversible.qml sequential animations
*/
QQmlListProperty<QQuickAbstractAnimation> QQuickTransition::animations()
{
    Q_D(QQuickTransition);
    return QQmlListProperty<QQuickAbstractAnimation>(
            this, &d->animations,
            QQuickTransitionPrivate::append_animation,
            QQuickTransitionPrivate::animation_count,
            QQuickTransitionPrivate::animation_at,
            QQuickTransitionPrivate::clear_animations,
            QQuickTransitionPrivate::replace_animation,
            QQuickTransitionPrivate::removeLast_animation);
}

QT_END_NAMESPACE

//#include <qquicktransition.moc>

#include "moc_qquicktransition_p.cpp"
