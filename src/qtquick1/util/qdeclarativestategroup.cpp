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

#include "QtQuick1/private/qdeclarativestategroup_p.h"

#include "QtQuick1/private/qdeclarativetransition_p.h"
#include "QtQuick1/private/qdeclarativestate_p_p.h"

#include <QtDeclarative/private/qdeclarativebinding_p.h>
#include <QtDeclarative/private/qdeclarativeglobal_p.h>

#include <QtCore/qstringbuilder.h>
#include <QtCore/qdebug.h>

#include <private/qobject_p.h>
#include <QtDeclarative/qdeclarativeinfo.h>

QT_BEGIN_NAMESPACE



DEFINE_BOOL_CONFIG_OPTION(stateChangeDebug, STATECHANGE_DEBUG);

class QDeclarative1StateGroupPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1StateGroup)
public:
    QDeclarative1StateGroupPrivate()
    : nullState(0), componentComplete(true),
      ignoreTrans(false), applyingState(false), unnamedCount(0) {}

    QString currentState;
    QDeclarative1State *nullState;

    static void append_state(QDeclarativeListProperty<QDeclarative1State> *list, QDeclarative1State *state);
    static int count_state(QDeclarativeListProperty<QDeclarative1State> *list);
    static QDeclarative1State *at_state(QDeclarativeListProperty<QDeclarative1State> *list, int index);
    static void clear_states(QDeclarativeListProperty<QDeclarative1State> *list);

    static void append_transition(QDeclarativeListProperty<QDeclarative1Transition> *list, QDeclarative1Transition *state);
    static int count_transitions(QDeclarativeListProperty<QDeclarative1Transition> *list);
    static QDeclarative1Transition *at_transition(QDeclarativeListProperty<QDeclarative1Transition> *list, int index);
    static void clear_transitions(QDeclarativeListProperty<QDeclarative1Transition> *list);

    QList<QDeclarative1State *> states;
    QList<QDeclarative1Transition *> transitions;

    bool componentComplete;
    bool ignoreTrans;
    bool applyingState;
    int unnamedCount;

    QDeclarative1Transition *findTransition(const QString &from, const QString &to);
    void setCurrentStateInternal(const QString &state, bool = false);
    bool updateAutoState();
};

/*!
   \qmlclass StateGroup QDeclarative1StateGroup
    \inqmlmodule QtQuick 1
   \ingroup qml-state-elements
   \since QtQuick 1.0
   \brief The StateGroup element provides state support for non-Item elements.

   Item (and all derived elements) provides built in support for states and transitions
   via its \l{Item::state}{state}, \l{Item::states}{states} and \l{Item::transitions}{transitions} properties. StateGroup provides an easy way to
   use this support in other (non-Item-derived) elements.

   \qml
   MyCustomObject {
       StateGroup {
           id: myStateGroup
           states: State {
               name: "state1"
               // ...
           }
           transitions: Transition {
               // ...
           }
       }

       onSomethingHappened: myStateGroup.state = "state1";
   }
   \endqml

   \sa {qmlstate}{States} {QML Animation and Transitions}{Transitions}, {QtDeclarative}
*/

QDeclarative1StateGroup::QDeclarative1StateGroup(QObject *parent)
    : QObject(*(new QDeclarative1StateGroupPrivate), parent)
{
}

QDeclarative1StateGroup::~QDeclarative1StateGroup()
{
    Q_D(const QDeclarative1StateGroup);
    for (int i = 0; i < d->states.count(); ++i)
        d->states.at(i)->setStateGroup(0);
}

QList<QDeclarative1State *> QDeclarative1StateGroup::states() const
{
    Q_D(const QDeclarative1StateGroup);
    return d->states;
}

/*!
  \qmlproperty list<State> QtQuick1::StateGroup::states
  This property holds a list of states defined by the state group.

  \qml
  StateGroup {
      states: [
          State {
              // State definition...
          },
          State {
              // ...
          }
          // Other states...
      ]
  }
  \endqml

  \sa {qmlstate}{States}
*/
QDeclarativeListProperty<QDeclarative1State> QDeclarative1StateGroup::statesProperty()
{
    Q_D(QDeclarative1StateGroup);
    return QDeclarativeListProperty<QDeclarative1State>(this, &d->states, &QDeclarative1StateGroupPrivate::append_state,
                                                       &QDeclarative1StateGroupPrivate::count_state,
                                                       &QDeclarative1StateGroupPrivate::at_state,
                                                       &QDeclarative1StateGroupPrivate::clear_states);
}

void QDeclarative1StateGroupPrivate::append_state(QDeclarativeListProperty<QDeclarative1State> *list, QDeclarative1State *state)
{
    QDeclarative1StateGroup *_this = static_cast<QDeclarative1StateGroup *>(list->object);
    if (state) {
        _this->d_func()->states.append(state);
        state->setStateGroup(_this);
    }

}

int QDeclarative1StateGroupPrivate::count_state(QDeclarativeListProperty<QDeclarative1State> *list)
{
    QDeclarative1StateGroup *_this = static_cast<QDeclarative1StateGroup *>(list->object);
    return _this->d_func()->states.count();
}

QDeclarative1State *QDeclarative1StateGroupPrivate::at_state(QDeclarativeListProperty<QDeclarative1State> *list, int index)
{
    QDeclarative1StateGroup *_this = static_cast<QDeclarative1StateGroup *>(list->object);
    return _this->d_func()->states.at(index);
}

void QDeclarative1StateGroupPrivate::clear_states(QDeclarativeListProperty<QDeclarative1State> *list)
{
    QDeclarative1StateGroup *_this = static_cast<QDeclarative1StateGroup *>(list->object);
    _this->d_func()->setCurrentStateInternal(QString(), true);
    for (int i = 0; i < _this->d_func()->states.count(); ++i) {
        _this->d_func()->states.at(i)->setStateGroup(0);
    }
    _this->d_func()->states.clear();
}

/*!
  \qmlproperty list<Transition> QtQuick1::StateGroup::transitions
  This property holds a list of transitions defined by the state group.

  \qml
  StateGroup {
      transitions: [
          Transition {
            // ...
          },
          Transition {
            // ...
          }
          // ...
      ]
  }
  \endqml

  \sa {QML Animation and Transitions}{Transitions}
*/
QDeclarativeListProperty<QDeclarative1Transition> QDeclarative1StateGroup::transitionsProperty()
{
    Q_D(QDeclarative1StateGroup);
    return QDeclarativeListProperty<QDeclarative1Transition>(this, &d->transitions, &QDeclarative1StateGroupPrivate::append_transition,
                                                       &QDeclarative1StateGroupPrivate::count_transitions,
                                                       &QDeclarative1StateGroupPrivate::at_transition,
                                                       &QDeclarative1StateGroupPrivate::clear_transitions);
}

void QDeclarative1StateGroupPrivate::append_transition(QDeclarativeListProperty<QDeclarative1Transition> *list, QDeclarative1Transition *trans)
{
    QDeclarative1StateGroup *_this = static_cast<QDeclarative1StateGroup *>(list->object);
    if (trans)
        _this->d_func()->transitions.append(trans);
}

int QDeclarative1StateGroupPrivate::count_transitions(QDeclarativeListProperty<QDeclarative1Transition> *list)
{
    QDeclarative1StateGroup *_this = static_cast<QDeclarative1StateGroup *>(list->object);
    return _this->d_func()->transitions.count();
}

QDeclarative1Transition *QDeclarative1StateGroupPrivate::at_transition(QDeclarativeListProperty<QDeclarative1Transition> *list, int index)
{
    QDeclarative1StateGroup *_this = static_cast<QDeclarative1StateGroup *>(list->object);
    return _this->d_func()->transitions.at(index);
}

void QDeclarative1StateGroupPrivate::clear_transitions(QDeclarativeListProperty<QDeclarative1Transition> *list)
{
    QDeclarative1StateGroup *_this = static_cast<QDeclarative1StateGroup *>(list->object);
    _this->d_func()->transitions.clear();
}

/*!
  \qmlproperty string QtQuick1::StateGroup::state

  This property holds the name of the current state of the state group.

  This property is often used in scripts to change between states. For
  example:

  \js
  function toggle() {
      if (button.state == 'On')
          button.state = 'Off';
      else
          button.state = 'On';
  }
  \endjs

  If the state group is in its base state (i.e. no explicit state has been
  set), \c state will be a blank string. Likewise, you can return a
  state group to its base state by setting its current state to \c ''.

  \sa {qmlstates}{States}
*/
QString QDeclarative1StateGroup::state() const
{
    Q_D(const QDeclarative1StateGroup);
    return d->currentState;
}

void QDeclarative1StateGroup::setState(const QString &state)
{
    Q_D(QDeclarative1StateGroup);
    if (d->currentState == state)
        return;

    d->setCurrentStateInternal(state);
}

void QDeclarative1StateGroup::classBegin()
{
    Q_D(QDeclarative1StateGroup);
    d->componentComplete = false;
}

void QDeclarative1StateGroup::componentComplete()
{
    Q_D(QDeclarative1StateGroup);
    d->componentComplete = true;

    for (int ii = 0; ii < d->states.count(); ++ii) {
        QDeclarative1State *state = d->states.at(ii);
        if (!state->isNamed())
            state->setName(QLatin1String("anonymousState") % QString::number(++d->unnamedCount));
    }

    if (d->updateAutoState()) {
        return;
    } else if (!d->currentState.isEmpty()) {
        QString cs = d->currentState;
        d->currentState.clear();
        d->setCurrentStateInternal(cs, true);
    }
}

/*!
    Returns true if the state was changed, otherwise false.
*/
bool QDeclarative1StateGroup::updateAutoState()
{
    Q_D(QDeclarative1StateGroup);
    return d->updateAutoState();
}

bool QDeclarative1StateGroupPrivate::updateAutoState()
{
    Q_Q(QDeclarative1StateGroup);
    if (!componentComplete)
        return false;

    bool revert = false;
    for (int ii = 0; ii < states.count(); ++ii) {
        QDeclarative1State *state = states.at(ii);
        if (state->isWhenKnown()) {
            if (state->isNamed()) {
                if (state->when() && state->when()->evaluate().toBool()) {
                    if (stateChangeDebug()) 
                        qWarning() << "Setting auto state due to:" 
                                   << state->when()->expression();
                    if (currentState != state->name()) {
                        q->setState(state->name());
                        return true;
                    } else {
                        return false;
                    }
                } else if (state->name() == currentState) {
                    revert = true;
                }
            }
        }
    }
    if (revert) {
        bool rv = !currentState.isEmpty();
        q->setState(QString());
        return rv;
    } else {
        return false;
    }
}

QDeclarative1Transition *QDeclarative1StateGroupPrivate::findTransition(const QString &from, const QString &to)
{
    QDeclarative1Transition *highest = 0;
    int score = 0;
    bool reversed = false;
    bool done = false;

    for (int ii = 0; !done && ii < transitions.count(); ++ii) {
        QDeclarative1Transition *t = transitions.at(ii);
        for (int ii = 0; ii < 2; ++ii)
        {
            if (ii && (!t->reversible() ||
                      (t->fromState() == QLatin1String("*") && 
                       t->toState() == QLatin1String("*"))))
                break;
            QStringList fromState;
            QStringList toState;

            fromState = t->fromState().split(QLatin1Char(','));
            for (int jj = 0; jj < fromState.count(); ++jj)
                fromState[jj] = fromState.at(jj).trimmed();
            toState = t->toState().split(QLatin1Char(','));
            for (int jj = 0; jj < toState.count(); ++jj)
                toState[jj] = toState.at(jj).trimmed();
            if (ii == 1)
                qSwap(fromState, toState);
            int tScore = 0;
            if (fromState.contains(from))
                tScore += 2;
            else if (fromState.contains(QLatin1String("*")))
                tScore += 1;
            else
                continue;

            if (toState.contains(to))
                tScore += 2;
            else if (toState.contains(QLatin1String("*")))
                tScore += 1;
            else
                continue;

            if (ii == 1)
                reversed = true;
            else
                reversed = false;

            if (tScore == 4) {
                highest = t;
                done = true;
                break;
            } else if (tScore > score) {
                score = tScore;
                highest = t;
            }
        }
    }

    if (highest)
        highest->setReversed(reversed);

    return highest;
}

void QDeclarative1StateGroupPrivate::setCurrentStateInternal(const QString &state, 
                                                   bool ignoreTrans)
{
    Q_Q(QDeclarative1StateGroup);
    if (!componentComplete) {
        currentState = state;
        return;
    }

    if (applyingState) {
        qmlInfo(q) << "Can't apply a state change as part of a state definition.";
        return;
    }

    applyingState = true;

    QDeclarative1Transition *transition = ignoreTrans ? 0 : findTransition(currentState, state);
    if (stateChangeDebug()) {
        qWarning() << this << "Changing state.  From" << currentState << ". To" << state;
        if (transition)
            qWarning() << "   using transition" << transition->fromState() 
                       << transition->toState();
    }

    QDeclarative1State *oldState = 0;
    if (!currentState.isEmpty()) {
        for (int ii = 0; ii < states.count(); ++ii) {
            if (states.at(ii)->name() == currentState) {
                oldState = states.at(ii);
                break;
            }
        }
    }

    currentState = state;
    emit q->stateChanged(currentState);

    QDeclarative1State *newState = 0;
    for (int ii = 0; ii < states.count(); ++ii) {
        if (states.at(ii)->name() == currentState) {
            newState = states.at(ii);
            break;
        }
    }

    if (oldState == 0 || newState == 0) {
        if (!nullState) { nullState = new QDeclarative1State; QDeclarative_setParent_noEvent(nullState, q); }
        if (!oldState) oldState = nullState;
        if (!newState) newState = nullState;
    }

    newState->apply(q, transition, oldState);
    applyingState = false;
    if (!transition)
        static_cast<QDeclarative1StatePrivate*>(QObjectPrivate::get(newState))->complete();
}

QDeclarative1State *QDeclarative1StateGroup::findState(const QString &name) const
{
    Q_D(const QDeclarative1StateGroup);
    for (int i = 0; i < d->states.count(); ++i) {
        QDeclarative1State *state = d->states.at(i);
        if (state->name() == name)
            return state;
    }

    return 0;
}

void QDeclarative1StateGroup::removeState(QDeclarative1State *state)
{
    Q_D(QDeclarative1StateGroup);
    d->states.removeOne(state);
}



QT_END_NAMESPACE


