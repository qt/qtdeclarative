// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstategroup_p.h"

#include "qquicktransition_p.h"

#include <private/qqmlbinding_p.h>
#include <private/qqmlglobal_p.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvector.h>

#include <private/qobject_p.h>
#include <qqmlinfo.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_DECLARE_LOGGING_CATEGORY(lcStates)

class QQuickStateGroupPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickStateGroup)
public:
    QQuickStateGroupPrivate()
    : nullState(nullptr), componentComplete(true),
      ignoreTrans(false), applyingState(false), unnamedCount(0) {}

    QString currentState;
    QQuickState *nullState;

    static void append_state(QQmlListProperty<QQuickState> *list, QQuickState *state);
    static qsizetype count_state(QQmlListProperty<QQuickState> *list);
    static QQuickState *at_state(QQmlListProperty<QQuickState> *list, qsizetype index);
    static void clear_states(QQmlListProperty<QQuickState> *list);
    static void replace_states(QQmlListProperty<QQuickState> *list, qsizetype index, QQuickState *state);
    static void removeLast_states(QQmlListProperty<QQuickState> *list);

    static void append_transition(QQmlListProperty<QQuickTransition> *list, QQuickTransition *state);
    static qsizetype count_transitions(QQmlListProperty<QQuickTransition> *list);
    static QQuickTransition *at_transition(QQmlListProperty<QQuickTransition> *list, qsizetype index);
    static void clear_transitions(QQmlListProperty<QQuickTransition> *list);

    QList<QQuickState *> states;
    QList<QQuickTransition *> transitions;

    bool componentComplete;
    bool ignoreTrans;
    bool applyingState;
    int unnamedCount;

    QQuickTransition *findTransition(const QString &from, const QString &to);
    void setCurrentStateInternal(const QString &state, bool = false);
    bool updateAutoState();
};

/*!
   \qmltype StateGroup
    \instantiates QQuickStateGroup
    \inqmlmodule QtQuick
   \ingroup qtquick-states
   \brief Provides built-in state support for non-Item types.

   Item (and all derived types) provides built in support for states and transitions
   via its \l{Item::state}{state}, \l{Item::states}{states} and \l{Item::transitions}{transitions} properties. StateGroup provides an easy way to
   use this support in other (non-Item-derived) types.

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

   \sa {Qt Quick States}{Qt Quick States}, {Animation and Transitions in Qt Quick}{Transitions}, {Qt QML}
*/

QQuickStateGroup::QQuickStateGroup(QObject *parent)
    : QObject(*(new QQuickStateGroupPrivate), parent)
{
}

QQuickStateGroup::~QQuickStateGroup()
{
    Q_D(const QQuickStateGroup);
    for (int i = 0; i < d->states.size(); ++i) {
        if (d->states.at(i))
            d->states.at(i)->setStateGroup(nullptr);
    }
    if (d->nullState)
        d->nullState->setStateGroup(nullptr);
}

QList<QQuickState *> QQuickStateGroup::states() const
{
    Q_D(const QQuickStateGroup);
    return d->states;
}

/*!
  \qmlproperty list<State> QtQuick::StateGroup::states
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

  \sa {Qt Quick States}{Qt Quick States}
*/
QQmlListProperty<QQuickState> QQuickStateGroup::statesProperty()
{
    Q_D(QQuickStateGroup);
    return QQmlListProperty<QQuickState>(this, &d->states,
                                         &QQuickStateGroupPrivate::append_state,
                                         &QQuickStateGroupPrivate::count_state,
                                         &QQuickStateGroupPrivate::at_state,
                                         &QQuickStateGroupPrivate::clear_states,
                                         &QQuickStateGroupPrivate::replace_states,
                                         &QQuickStateGroupPrivate::removeLast_states);
}

void QQuickStateGroupPrivate::append_state(QQmlListProperty<QQuickState> *list, QQuickState *state)
{
    QQuickStateGroup *_this = static_cast<QQuickStateGroup *>(list->object);
    _this->d_func()->states.append(state);
    if (state)
        state->setStateGroup(_this);
}

qsizetype QQuickStateGroupPrivate::count_state(QQmlListProperty<QQuickState> *list)
{
    QQuickStateGroup *_this = static_cast<QQuickStateGroup *>(list->object);
    return _this->d_func()->states.size();
}

QQuickState *QQuickStateGroupPrivate::at_state(QQmlListProperty<QQuickState> *list, qsizetype index)
{
    QQuickStateGroup *_this = static_cast<QQuickStateGroup *>(list->object);
    return _this->d_func()->states.at(index);
}

void QQuickStateGroupPrivate::clear_states(QQmlListProperty<QQuickState> *list)
{
    QQuickStateGroup *_this = static_cast<QQuickStateGroup *>(list->object);
    _this->d_func()->setCurrentStateInternal(QString(), true);
    for (qsizetype i = 0; i < _this->d_func()->states.size(); ++i) {
        if (_this->d_func()->states.at(i))
            _this->d_func()->states.at(i)->setStateGroup(nullptr);
    }
    _this->d_func()->states.clear();
}

void QQuickStateGroupPrivate::replace_states(QQmlListProperty<QQuickState> *list, qsizetype index, QQuickState *state)
{
    auto *self = static_cast<QQuickStateGroup *>(list->object);
    auto *d = self->d_func();
    auto *oldState = d->states.at(index);
    if (oldState != state) {
        if (oldState)
            oldState->setStateGroup(nullptr);
        state->setStateGroup(self);
        d->states.replace(index, state);
        if (!oldState || d->currentState == oldState->name())
            d->setCurrentStateInternal(state->name(), true);
    }
}

void QQuickStateGroupPrivate::removeLast_states(QQmlListProperty<QQuickState> *list)
{
    auto *d = static_cast<QQuickStateGroup *>(list->object)->d_func();
    if (d->currentState == d->states.last()->name())
        d->setCurrentStateInternal(d->states.size() > 1 ? d->states.first()->name() : QString(), true);
    d->states.last()->setStateGroup(nullptr);
    d->states.removeLast();
}

/*!
  \qmlproperty list<Transition> QtQuick::StateGroup::transitions
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

  \sa {Animation and Transitions in Qt Quick}{Transitions}
*/
QQmlListProperty<QQuickTransition> QQuickStateGroup::transitionsProperty()
{
    Q_D(QQuickStateGroup);
    return QQmlListProperty<QQuickTransition>(this, &d->transitions, &QQuickStateGroupPrivate::append_transition,
                                                       &QQuickStateGroupPrivate::count_transitions,
                                                       &QQuickStateGroupPrivate::at_transition,
                                                       &QQuickStateGroupPrivate::clear_transitions);
}

void QQuickStateGroupPrivate::append_transition(QQmlListProperty<QQuickTransition> *list, QQuickTransition *trans)
{
    QQuickStateGroup *_this = static_cast<QQuickStateGroup *>(list->object);
    if (trans)
        _this->d_func()->transitions.append(trans);
}

qsizetype QQuickStateGroupPrivate::count_transitions(QQmlListProperty<QQuickTransition> *list)
{
    QQuickStateGroup *_this = static_cast<QQuickStateGroup *>(list->object);
    return _this->d_func()->transitions.size();
}

QQuickTransition *QQuickStateGroupPrivate::at_transition(QQmlListProperty<QQuickTransition> *list, qsizetype index)
{
    QQuickStateGroup *_this = static_cast<QQuickStateGroup *>(list->object);
    return _this->d_func()->transitions.at(index);
}

void QQuickStateGroupPrivate::clear_transitions(QQmlListProperty<QQuickTransition> *list)
{
    QQuickStateGroup *_this = static_cast<QQuickStateGroup *>(list->object);
    _this->d_func()->transitions.clear();
}

/*!
  \qmlproperty string QtQuick::StateGroup::state

  This property holds the name of the current state of the state group.

  This property is often used in scripts to change between states. For
  example:

  \qml
  function toggle() {
      if (button.state == 'On')
          button.state = 'Off';
      else
          button.state = 'On';
  }
  \endqml

  If the state group is in its base state (i.e. no explicit state has been
  set), \c state will be a blank string. Likewise, you can return a
  state group to its base state by setting its current state to \c ''.

  \sa {Qt Quick States}{Qt Quick States}
*/
QString QQuickStateGroup::state() const
{
    Q_D(const QQuickStateGroup);
    return d->currentState;
}

void QQuickStateGroup::setState(const QString &state)
{
    Q_D(QQuickStateGroup);
    if (d->currentState == state)
        return;

    d->setCurrentStateInternal(state);
}

void QQuickStateGroup::classBegin()
{
    Q_D(QQuickStateGroup);
    d->componentComplete = false;
}

void QQuickStateGroup::componentComplete()
{
    Q_D(QQuickStateGroup);
    d->componentComplete = true;

    QVarLengthArray<QString, 4> names;
    names.reserve(d->states.size());
    for (int ii = 0; ii < d->states.size(); ++ii) {
        QQuickState *state = d->states.at(ii);
        if (!state->isNamed())
            state->setName(QLatin1String("anonymousState") + QString::number(++d->unnamedCount));

        QString stateName = state->name();
        if (names.contains(stateName)) {
            qmlWarning(state->parent()) << "Found duplicate state name: " << stateName;
        } else {
            names.append(std::move(stateName));
        }
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
bool QQuickStateGroup::updateAutoState()
{
    Q_D(QQuickStateGroup);
    return d->updateAutoState();
}

bool QQuickStateGroupPrivate::updateAutoState()
{
    Q_Q(QQuickStateGroup);
    if (!componentComplete)
        return false;

    bool revert = false;
    for (int ii = 0; ii < states.size(); ++ii) {
        QQuickState *state = states.at(ii);
        if (state->isWhenKnown()) {
            if (state->isNamed()) {
                bool whenValue = state->when();
                const QQmlProperty whenProp(state, u"when"_s);
                const auto potentialWhenBinding = QQmlAnyBinding::ofProperty(whenProp);
                Q_ASSERT(!potentialWhenBinding.isUntypedPropertyBinding());

                // if there is a binding, the value in when might not be up-to-date at this point
                // so we manually re-evaluate the binding
                QQmlAbstractBinding *abstractBinding = potentialWhenBinding.asAbstractBinding();
                if (abstractBinding && abstractBinding->kind() == QQmlAbstractBinding::QmlBinding) {
                    QQmlBinding *binding = static_cast<QQmlBinding *>(abstractBinding);
                    if (binding->hasValidContext()) {
                        const auto boolType = QMetaType::fromType<bool>();
                        const bool isUndefined = !binding->evaluate(&whenValue, boolType);
                        if (isUndefined)
                            whenValue = false;
                    }
                }

                if (whenValue) {
                    qCDebug(lcStates) << "Setting auto state due to expression";
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

QQuickTransition *QQuickStateGroupPrivate::findTransition(const QString &from, const QString &to)
{
    QQuickTransition *highest = nullptr;
    int score = 0;
    bool reversed = false;
    bool done = false;

    for (int ii = 0; !done && ii < transitions.size(); ++ii) {
        QQuickTransition *t = transitions.at(ii);
        if (!t->enabled())
            continue;
        for (int ii = 0; ii < 2; ++ii)
        {
            if (ii && (!t->reversible() ||
                      (t->fromState() == QLatin1String("*") &&
                       t->toState() == QLatin1String("*"))))
                break;
            const QString fromStateStr = t->fromState();
            const QString toStateStr = t->toState();

            auto fromState = QStringView{fromStateStr}.split(QLatin1Char(','));
            for (int jj = 0; jj < fromState.size(); ++jj)
                fromState[jj] = fromState.at(jj).trimmed();
            auto toState = QStringView{toStateStr}.split(QLatin1Char(','));
            for (int jj = 0; jj < toState.size(); ++jj)
                toState[jj] = toState.at(jj).trimmed();
            if (ii == 1)
                qSwap(fromState, toState);
            int tScore = 0;
            const QString asterisk = QStringLiteral("*");
            if (fromState.contains(QStringView(from)))
                tScore += 2;
            else if (fromState.contains(QStringView(asterisk)))
                tScore += 1;
            else
                continue;

            if (toState.contains(QStringView(to)))
                tScore += 2;
            else if (toState.contains(QStringView(asterisk)))
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

void QQuickStateGroupPrivate::setCurrentStateInternal(const QString &state,
                                                   bool ignoreTrans)
{
    Q_Q(QQuickStateGroup);
    if (!componentComplete) {
        currentState = state;
        return;
    }

    if (applyingState) {
        qmlWarning(q) << "Can't apply a state change as part of a state definition.";
        return;
    }

    applyingState = true;

    QQuickTransition *transition = ignoreTrans ? nullptr : findTransition(currentState, state);
    if (lcStates().isDebugEnabled()) {
        qCDebug(lcStates) << this << "changing state from:" << currentState << "to:" << state;
        if (transition)
            qCDebug(lcStates) << "   using transition" << transition->fromState()
                              << transition->toState();
    }

    QQuickState *oldState = nullptr;
    if (!currentState.isEmpty()) {
        for (int ii = 0; ii < states.size(); ++ii) {
            if (states.at(ii)->name() == currentState) {
                oldState = states.at(ii);
                break;
            }
        }
    }

    currentState = state;
    emit q->stateChanged(currentState);

    QQuickState *newState = nullptr;
    for (int ii = 0; ii < states.size(); ++ii) {
        if (states.at(ii)->name() == currentState) {
            newState = states.at(ii);
            break;
        }
    }

    if (oldState == nullptr || newState == nullptr) {
        if (!nullState) {
            nullState = new QQuickState;
            QQml_setParent_noEvent(nullState, q);
            nullState->setStateGroup(q);
        }
        if (!oldState) oldState = nullState;
        if (!newState) newState = nullState;
    }

    newState->apply(transition, oldState);
    applyingState = false;  //### consider removing this (don't allow state changes in transition)
}

QQuickState *QQuickStateGroup::findState(const QString &name) const
{
    Q_D(const QQuickStateGroup);
    for (int i = 0; i < d->states.size(); ++i) {
        QQuickState *state = d->states.at(i);
        if (state->name() == name)
            return state;
    }

    return nullptr;
}

void QQuickStateGroup::removeState(QQuickState *state)
{
    Q_D(QQuickStateGroup);
    d->states.removeOne(state);
}

void QQuickStateGroup::stateAboutToComplete()
{
    Q_D(QQuickStateGroup);
    d->applyingState = false;
}

QT_END_NAMESPACE


#include "moc_qquickstategroup_p.cpp"
