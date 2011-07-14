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

#include "QtQuick1/private/qdeclarativestate_p.h"
#include "QtQuick1/private/qdeclarativestategroup_p.h"
#include "QtQuick1/private/qdeclarativestate_p_p.h"
#include "QtQuick1/private/qdeclarativestateoperations_p.h"
#include "QtQuick1/private/qdeclarativeanimation_p.h"
#include "QtQuick1/private/qdeclarativeanimation_p_p.h"
#include "QtQuick1/private/qdeclarativetransitionmanager_p_p.h"

#include <QParallelAnimationGroup>

QT_BEGIN_NAMESPACE



/*!
    \qmlclass Transition QDeclarative1Transition
    \inqmlmodule QtQuick 1
    \ingroup qml-animation-transition
    \since QtQuick 1.0
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

//ParallelAnimationWrapper_1 allows us to do a "callback" when the animation finishes, rather than connecting
//and disconnecting signals and slots frequently
class ParallelAnimationWrapper_1 : public QParallelAnimationGroup
{
    Q_OBJECT
public:
    ParallelAnimationWrapper_1(QObject *parent = 0) : QParallelAnimationGroup(parent) {}
    QDeclarative1TransitionPrivate *trans;
protected:
    virtual void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);
};

class QDeclarative1TransitionPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1Transition)
public:
    QDeclarative1TransitionPrivate()
    : fromState(QLatin1String("*")), toState(QLatin1String("*")),
      reversed(false), reversible(false), endState(0)
    {
        group.trans = this;
    }

    QString fromState;
    QString toState;
    bool reversed;
    bool reversible;
    ParallelAnimationWrapper_1 group;
    QDeclarative1TransitionManager *endState;

    void complete()
    {
        endState->complete();
    }
    static void append_animation(QDeclarativeListProperty<QDeclarative1AbstractAnimation> *list, QDeclarative1AbstractAnimation *a);
    static int animation_count(QDeclarativeListProperty<QDeclarative1AbstractAnimation> *list);
    static QDeclarative1AbstractAnimation* animation_at(QDeclarativeListProperty<QDeclarative1AbstractAnimation> *list, int pos);
    static void clear_animations(QDeclarativeListProperty<QDeclarative1AbstractAnimation> *list);
    QList<QDeclarative1AbstractAnimation *> animations;
};

void QDeclarative1TransitionPrivate::append_animation(QDeclarativeListProperty<QDeclarative1AbstractAnimation> *list, QDeclarative1AbstractAnimation *a)
{
    QDeclarative1Transition *q = static_cast<QDeclarative1Transition *>(list->object);
    q->d_func()->animations.append(a);
    q->d_func()->group.addAnimation(a->qtAnimation());
    a->setDisableUserControl();
}

int QDeclarative1TransitionPrivate::animation_count(QDeclarativeListProperty<QDeclarative1AbstractAnimation> *list)
{
    QDeclarative1Transition *q = static_cast<QDeclarative1Transition *>(list->object);
    return q->d_func()->animations.count();
}

QDeclarative1AbstractAnimation* QDeclarative1TransitionPrivate::animation_at(QDeclarativeListProperty<QDeclarative1AbstractAnimation> *list, int pos)
{
    QDeclarative1Transition *q = static_cast<QDeclarative1Transition *>(list->object);
    return q->d_func()->animations.at(pos);
}

void QDeclarative1TransitionPrivate::clear_animations(QDeclarativeListProperty<QDeclarative1AbstractAnimation> *list)
{
    QDeclarative1Transition *q = static_cast<QDeclarative1Transition *>(list->object);
    while (q->d_func()->animations.count()) {
        QDeclarative1AbstractAnimation *firstAnim = q->d_func()->animations.at(0);
        q->d_func()->group.removeAnimation(firstAnim->qtAnimation());
        q->d_func()->animations.removeAll(firstAnim);
    }
}

void ParallelAnimationWrapper_1::updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
{
    QParallelAnimationGroup::updateState(newState, oldState);
    if (newState == Stopped && (duration() == -1
        || (direction() == QAbstractAnimation::Forward && currentLoopTime() == duration())
        || (direction() == QAbstractAnimation::Backward && currentLoopTime() == 0)))
    {
        trans->complete();
    }
}



QDeclarative1Transition::QDeclarative1Transition(QObject *parent)
    : QObject(*(new QDeclarative1TransitionPrivate), parent)
{
}

QDeclarative1Transition::~QDeclarative1Transition()
{
}

void QDeclarative1Transition::stop()
{
    Q_D(QDeclarative1Transition);
    d->group.stop();
}

void QDeclarative1Transition::setReversed(bool r)
{
    Q_D(QDeclarative1Transition);
    d->reversed = r;
}

void QDeclarative1Transition::prepare(QDeclarative1StateOperation::ActionList &actions,
                            QList<QDeclarativeProperty> &after,
                            QDeclarative1TransitionManager *endState)
{
    Q_D(QDeclarative1Transition);

    qmlExecuteDeferred(this);

    if (d->reversed) {
        for (int ii = d->animations.count() - 1; ii >= 0; --ii) {
            d->animations.at(ii)->transition(actions, after, QDeclarative1AbstractAnimation::Backward);
        }
    } else {
        for (int ii = 0; ii < d->animations.count(); ++ii) {
            d->animations.at(ii)->transition(actions, after, QDeclarative1AbstractAnimation::Forward);
        }
    }

    d->endState = endState;
    d->group.setDirection(d->reversed ? QAbstractAnimation::Backward : QAbstractAnimation::Forward);
    d->group.start();
}

/*!
    \qmlproperty string QtQuick1::Transition::from
    \qmlproperty string QtQuick1::Transition::to

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

    \sa reversible
*/
QString QDeclarative1Transition::fromState() const
{
    Q_D(const QDeclarative1Transition);
    return d->fromState;
}

void QDeclarative1Transition::setFromState(const QString &f)
{
    Q_D(QDeclarative1Transition);
    if (f == d->fromState)
        return;

    d->fromState = f;
    emit fromChanged();
}

/*!
    \qmlproperty bool QtQuick1::Transition::reversible
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
bool QDeclarative1Transition::reversible() const
{
    Q_D(const QDeclarative1Transition);
    return d->reversible;
}

void QDeclarative1Transition::setReversible(bool r)
{
    Q_D(QDeclarative1Transition);
    if (r == d->reversible)
        return;

    d->reversible = r;
    emit reversibleChanged();
}

QString QDeclarative1Transition::toState() const
{
    Q_D(const QDeclarative1Transition);
    return d->toState;
}

void QDeclarative1Transition::setToState(const QString &t)
{
    Q_D(QDeclarative1Transition);
    if (t == d->toState)
        return;

    d->toState = t;
    emit toChanged();
}

/*!
    \qmlproperty list<Animation> QtQuick1::Transition::animations
    \default

    This property holds a list of the animations to be run for this transition.

    \snippet examples/declarative/toys/dynamicscene/dynamicscene.qml top-level transitions

    The top-level animations are run in parallel. To run them sequentially,
    define them within a SequentialAnimation:

    \snippet doc/src/snippets/declarative/transition-reversible.qml sequential animations
*/
QDeclarativeListProperty<QDeclarative1AbstractAnimation> QDeclarative1Transition::animations()
{
    Q_D(QDeclarative1Transition);
    return QDeclarativeListProperty<QDeclarative1AbstractAnimation>(this, &d->animations, QDeclarative1TransitionPrivate::append_animation,
                                                                   QDeclarative1TransitionPrivate::animation_count,
                                                                   QDeclarative1TransitionPrivate::animation_at,
                                                                   QDeclarative1TransitionPrivate::clear_animations);
}



QT_END_NAMESPACE

#include <qdeclarativetransition.moc>
