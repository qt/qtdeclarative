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

#include "private/qdeclarativestate_p.h"
#include "private/qdeclarativestategroup_p.h"
#include "private/qdeclarativestate_p_p.h"
#include "private/qdeclarativestateoperations_p.h"
#include "private/qdeclarativeanimation_p.h"
#include "private/qdeclarativeanimation_p_p.h"
#include "private/qdeclarativetransitionmanager_p_p.h"

#include "private/qparallelanimationgroup2_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmlclass Transition QDeclarativeTransition
    \inqmlmodule QtQuick 2
    \ingroup qml-animation-transition
    \brief The Transition element defines animated transitions that occur on state changes.

    A Transition defines the animations to be applied when a \l State change occurs.

    For example, the following \l Rectangle has two states: the default state, and
    an added "moved" state. In the "moved state, the rectangle's position changes
    to (50, 50).  The added Transition specifies that when the rectangle
    changes between the default and the "moved" state, any changes
    to the \c x and \c y properties should be animated, using an \c Easing.InOutQuad.

    \snippet doc/src/snippets/declarative/transition.qml 0

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

    To define multiple transitions, specify \l Item::transitions as a list:

    \snippet doc/src/snippets/declarative/transitions-list.qml list of transitions

    If multiple Transitions are specified, only a single (best-matching) Transition will be applied for any particular
    state change. In the example above, when changing to \c state1, the first transition will be used, rather
    than the more generic second transition.

    If a state change has a Transition that matches the same property as a
    \l Behavior, the Transition animation overrides the \l Behavior for that
    state change.

    \sa {QML Animation and Transitions}, {declarative/animation/states}{states example}, {qmlstates}{States}, {QtDeclarative}
*/

//ParallelAnimationWrapper allows us to do a "callback" when the animation finishes, rather than connecting
//and disconnecting signals and slots frequently
class ParallelAnimationWrapper : public QParallelAnimationGroup2
{
public:
    ParallelAnimationWrapper(QDeclarativeAbstractAnimation *animation = 0) : QParallelAnimationGroup2(animation) {}
    QDeclarativeTransitionPrivate *trans;
protected:
    virtual void updateState(QAbstractAnimation2::State newState, QAbstractAnimation2::State oldState);
};

class QDeclarativeTransitionPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeTransition)
public:
    QDeclarativeTransitionPrivate()
    : fromState(QLatin1String("*")), toState(QLatin1String("*")),
      reversed(false), reversible(false), enabled(true), manager(0)
    {
        group.trans = this;
    }

    QString fromState;
    QString toState;
    bool reversed;
    bool reversible;
    bool enabled;
    ParallelAnimationWrapper group;
    QDeclarativeTransitionManager *manager;

    void complete()
    {
        manager->complete();
    }
    static void append_animation(QDeclarativeListProperty<QDeclarativeAbstractAnimation> *list, QDeclarativeAbstractAnimation *a);
    static int animation_count(QDeclarativeListProperty<QDeclarativeAbstractAnimation> *list);
    static QDeclarativeAbstractAnimation* animation_at(QDeclarativeListProperty<QDeclarativeAbstractAnimation> *list, int pos);
    static void clear_animations(QDeclarativeListProperty<QDeclarativeAbstractAnimation> *list);
    QList<QDeclarativeAbstractAnimation *> animations;
};

void QDeclarativeTransitionPrivate::append_animation(QDeclarativeListProperty<QDeclarativeAbstractAnimation> *list, QDeclarativeAbstractAnimation *a)
{
    QDeclarativeTransition *q = static_cast<QDeclarativeTransition *>(list->object);
    q->d_func()->animations.append(a);
    a->setDisableUserControl();
}

int QDeclarativeTransitionPrivate::animation_count(QDeclarativeListProperty<QDeclarativeAbstractAnimation> *list)
{
    QDeclarativeTransition *q = static_cast<QDeclarativeTransition *>(list->object);
    return q->d_func()->animations.count();
}

QDeclarativeAbstractAnimation* QDeclarativeTransitionPrivate::animation_at(QDeclarativeListProperty<QDeclarativeAbstractAnimation> *list, int pos)
{
    QDeclarativeTransition *q = static_cast<QDeclarativeTransition *>(list->object);
    return q->d_func()->animations.at(pos);
}

void QDeclarativeTransitionPrivate::clear_animations(QDeclarativeListProperty<QDeclarativeAbstractAnimation> *list)
{
    QDeclarativeTransition *q = static_cast<QDeclarativeTransition *>(list->object);
    while (q->d_func()->animations.count()) {
        QDeclarativeAbstractAnimation *firstAnim = q->d_func()->animations.at(0);
        q->d_func()->animations.removeAll(firstAnim);
    }
}

void ParallelAnimationWrapper::updateState(QAbstractAnimation2::State newState, QAbstractAnimation2::State oldState)
{
    QParallelAnimationGroup2::updateState(newState, oldState);
    if (newState == Stopped && (duration() == -1
        || (direction() == QAbstractAnimation2::Forward && currentLoopTime() == duration())
        || (direction() == QAbstractAnimation2::Backward && currentLoopTime() == 0)))
    {
        trans->complete();
    }
}



QDeclarativeTransition::QDeclarativeTransition(QObject *parent)
    : QObject(*(new QDeclarativeTransitionPrivate), parent)
{
}

QDeclarativeTransition::~QDeclarativeTransition()
{
}

void QDeclarativeTransition::stop()
{
    Q_D(QDeclarativeTransition);
    d->group.stop();
}

void QDeclarativeTransition::setReversed(bool r)
{
    Q_D(QDeclarativeTransition);
    d->reversed = r;
}

void QDeclarativeTransition::prepare(QDeclarativeStateOperation::ActionList &actions,
                            QList<QDeclarativeProperty> &after,
                            QDeclarativeTransitionManager *manager)
{
    Q_D(QDeclarativeTransition);

    qmlExecuteDeferred(this);

    d->group.clear();
    QDeclarativeAbstractAnimation::TransitionDirection direction = d->reversed ? QDeclarativeAbstractAnimation::Backward : QDeclarativeAbstractAnimation::Forward;
    int start = d->reversed ? d->animations.count() - 1 : 0;
    int end = d->reversed ? -1 : d->animations.count();

    QAbstractAnimation2Pointer anim;
    for (int i = start; i != end;) {
        anim = d->animations.at(i)->transition(actions, after, direction);
        if (anim)
            d->reversed ? d->group.insertAnimation(0, anim) : d->group.addAnimation(anim);
        d->reversed ? --i : ++i;
    }

    d->manager = manager;
    d->group.setDirection(d->reversed ? QAbstractAnimation2::Backward : QAbstractAnimation2::Forward);
    d->group.start();
}

/*!
    \qmlproperty string QtQuick2::Transition::from
    \qmlproperty string QtQuick2::Transition::to

    These properties indicate the state changes that trigger the transition.

    The default values for these properties is "*" (that is, any state).

    For example, the following transition has not set the \c to and \c from
    properties, so the animation is always applied when changing between
    the two states (i.e. when the mouse is pressed and released).

    \snippet doc/src/snippets/declarative/transition-from-to.qml 0

    If the transition was changed to this:

    \snippet doc/src/snippets/declarative/transition-from-to-modified.qml modified transition

    The animation would only be applied when changing from the default state to
    the "brighter" state (i.e. when the mouse is pressed, but not on release).

    Multiple \c to and \from values can be set by using a comma-separated string.

    \sa reversible
*/
QString QDeclarativeTransition::fromState() const
{
    Q_D(const QDeclarativeTransition);
    return d->fromState;
}

void QDeclarativeTransition::setFromState(const QString &f)
{
    Q_D(QDeclarativeTransition);
    if (f == d->fromState)
        return;

    d->fromState = f;
    emit fromChanged();
}

/*!
    \qmlproperty bool QtQuick2::Transition::reversible
    This property holds whether the transition should be automatically reversed when the conditions that triggered this transition are reversed.

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

    \snippet doc/src/snippets/declarative/transition-reversible.qml 0

    If the transition did not set the \c to and \c reversible values, then
    on the mouse release, the transition would play the PropertyAnimation
    before the ColorAnimation instead of reversing the sequence.
*/
bool QDeclarativeTransition::reversible() const
{
    Q_D(const QDeclarativeTransition);
    return d->reversible;
}

void QDeclarativeTransition::setReversible(bool r)
{
    Q_D(QDeclarativeTransition);
    if (r == d->reversible)
        return;

    d->reversible = r;
    emit reversibleChanged();
}

QString QDeclarativeTransition::toState() const
{
    Q_D(const QDeclarativeTransition);
    return d->toState;
}

void QDeclarativeTransition::setToState(const QString &t)
{
    Q_D(QDeclarativeTransition);
    if (t == d->toState)
        return;

    d->toState = t;
    emit toChanged();
}

/*!
    \qmlproperty bool QtQuick2::Transition::enabled

    This property holds whether the Transition will be run when moving
    from the \c from state to the \c to state.

    By default a Transition is enabled.

    Note that in some circumstances disabling a Transition may cause an
    alternative Transition to be used in its place. In the following
    example, the generic Transition will be used to animate the change
    from \c state1 to \c state2, as the more specific Transition has
    been disabled.

    \qml
    Item {
        states: [
            State { name: "state1" ... }
            State { name: "state2" ... }
        ]
        transitions: [
            Transition { from: "state1"; to: "state2"; enabled: false ... }
            Transition { ... }
        ]
    }
    \endqml
*/

bool QDeclarativeTransition::enabled() const
{
    Q_D(const QDeclarativeTransition);
    return d->enabled;
}

void QDeclarativeTransition::setEnabled(bool enabled)
{
    Q_D(QDeclarativeTransition);
    if (d->enabled == enabled)
        return;
    d->enabled = enabled;
    emit enabledChanged();
}

/*!
    \qmlproperty list<Animation> QtQuick2::Transition::animations
    \default

    This property holds a list of the animations to be run for this transition.

    \snippet examples/declarative/toys/dynamicscene/dynamicscene.qml top-level transitions

    The top-level animations are run in parallel. To run them sequentially,
    define them within a SequentialAnimation:

    \snippet doc/src/snippets/declarative/transition-reversible.qml sequential animations
*/
QDeclarativeListProperty<QDeclarativeAbstractAnimation> QDeclarativeTransition::animations()
{
    Q_D(QDeclarativeTransition);
    return QDeclarativeListProperty<QDeclarativeAbstractAnimation>(this, &d->animations, QDeclarativeTransitionPrivate::append_animation,
                                                                   QDeclarativeTransitionPrivate::animation_count,
                                                                   QDeclarativeTransitionPrivate::animation_at,
                                                                   QDeclarativeTransitionPrivate::clear_animations);
}

QT_END_NAMESPACE

//#include <qdeclarativetransition.moc>
