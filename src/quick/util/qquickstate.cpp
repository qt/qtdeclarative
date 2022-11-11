// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstate_p_p.h"
#include "qquickstate_p.h"

#include "qquickstategroup_p.h"
#include "qquickstatechangescript_p.h"

#include <private/qqmlglobal_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcStates, "qt.qml.states")

QQuickStateAction::QQuickStateAction()
: restore(true), actionDone(false), reverseEvent(false), deletableToBinding(false), fromBinding(nullptr), event(nullptr),
  specifiedObject(nullptr)
{
}

QQuickStateAction::QQuickStateAction(QObject *target, const QString &propertyName,
               const QVariant &value)
: restore(true), actionDone(false), reverseEvent(false), deletableToBinding(false),
  property(target, propertyName, qmlEngine(target)), toValue(value),
  fromBinding(nullptr), event(nullptr),
  specifiedObject(target), specifiedProperty(propertyName)
{
    if (property.isValid())
        fromValue = property.read();
}

QQuickStateAction::QQuickStateAction(QObject *target, const QQmlProperty &property, const QString &propertyName, const QVariant &value)
: restore(true), actionDone(false), reverseEvent(false), deletableToBinding(false),
  property(property), toValue(value),
  fromBinding(nullptr), event(nullptr),
  specifiedObject(target), specifiedProperty(propertyName)
{
    if (property.isValid())
        fromValue = property.read();
}


QQuickStateActionEvent::~QQuickStateActionEvent()
{
}

void QQuickStateActionEvent::execute()
{
}

bool QQuickStateActionEvent::isReversable()
{
    return false;
}

void QQuickStateActionEvent::reverse()
{
}

bool QQuickStateActionEvent::changesBindings()
{
    return false;
}

void QQuickStateActionEvent::clearBindings()
{
}

bool QQuickStateActionEvent::mayOverride(QQuickStateActionEvent *other)
{
    Q_UNUSED(other);
    return false;
}

QQuickStateOperation::QQuickStateOperation(QObjectPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

/*!
    \qmltype State
    \instantiates QQuickState
    \inqmlmodule QtQuick
    \ingroup qtquick-states
    \brief Defines configurations of objects and properties.

    A \e state is a set of batched changes from the default configuration.

    All items have a default state that defines the default configuration of objects
    and property values. New states can be defined by adding State items to the \l {Item::states}{states} property to
    allow items to switch between different configurations. These configurations
    can, for example, be used to apply different sets of property values or execute
    different scripts.

    The following example displays a single \l Rectangle. In the default state, the rectangle
    is colored black. In the "clicked" state, a PropertyChanges object changes the
    rectangle's color to red. Clicking within the MouseArea toggles the rectangle's state
    between the default state and the "clicked" state, thus toggling the color of the
    rectangle between black and red.

    \snippet qml/state.qml 0

    Notice the default state is referred to using an empty string ("").

    States are commonly used together with \l{Animation and Transitions in Qt Quick}{Transitions} to provide
    animations when state changes occur.

    \note Setting the state of an object from within another state of the same object is
    not allowed.

    \sa {Qt Quick Examples - Animation#States}{States example}, {Qt Quick States},
    {Animation and Transitions in Qt Quick}{Transitions}, {Qt QML}
*/
QQuickState::QQuickState(QObject *parent)
: QObject(*(new QQuickStatePrivate), parent)
{
    Q_D(QQuickState);
    d->transitionManager.setState(this);
}

QQuickState::~QQuickState()
{
    Q_D(QQuickState);
    if (d->group)
        d->group->removeState(this);
}

/*!
    \qmlproperty string QtQuick::State::name
    This property holds the name of the state.

    Each state should have a unique name within its item.
*/
QString QQuickState::name() const
{
    Q_D(const QQuickState);
    return d->name;
}

void QQuickState::setName(const QString &n)
{
    Q_D(QQuickState);
    d->name = n;
    d->named = true;
}

bool QQuickState::isNamed() const
{
    Q_D(const QQuickState);
    return d->named;
}

bool QQuickState::isWhenKnown() const
{
    Q_D(const QQuickState);
    return d->whenKnown;
}

/*!
    \qmlproperty bool QtQuick::State::when
    This property holds when the state should be applied.

    This should be set to an expression that evaluates to \c true when you want the state to
    be applied. For example, the following \l Rectangle changes in and out of the "hidden"
    state when the \l MouseArea is pressed:

    \snippet qml/state-when.qml 0

    If multiple states in a group have \c when clauses that evaluate to \c true
    at the same time, the first matching state will be applied. For example, in
    the following snippet \c state1 will always be selected rather than
    \c state2 when sharedCondition becomes \c true.
    \qml
    Item {
        states: [
            State { name: "state1"; when: sharedCondition },
            State { name: "state2"; when: sharedCondition }
        ]
        // ...
    }
    \endqml
*/
bool QQuickState::when() const
{
    Q_D(const QQuickState);
    return d->when;
}

void QQuickState::setWhen(bool when)
{
    Q_D(QQuickState);
    d->whenKnown = true;
    d->when = when;
    if (d->group)
        d->group->updateAutoState();
}

/*!
    \qmlproperty string QtQuick::State::extend
    This property holds the state that this state extends.

    When a state extends another state, it inherits all the changes of that state.

    The state being extended is treated as the base state in regards to
    the changes specified by the extending state.
*/
QString QQuickState::extends() const
{
    Q_D(const QQuickState);
    return d->extends;
}

void QQuickState::setExtends(const QString &extends)
{
    Q_D(QQuickState);
    d->extends = extends;
}

/*!
    \qmlproperty list<Change> QtQuick::State::changes
    This property holds the changes to apply for this state
    \qmldefault

    By default these changes are applied against the default state. If the state
    extends another state, then the changes are applied against the state being
    extended.
*/
QQmlListProperty<QQuickStateOperation> QQuickState::changes()
{
    Q_D(QQuickState);
    return QQmlListProperty<QQuickStateOperation>(this, &d->operations,
                                                  QQuickStatePrivate::operations_append,
                                                  QQuickStatePrivate::operations_count,
                                                  QQuickStatePrivate::operations_at,
                                                  QQuickStatePrivate::operations_clear,
                                                  QQuickStatePrivate::operations_replace,
                                                  QQuickStatePrivate::operations_removeLast);
}

int QQuickState::operationCount() const
{
    Q_D(const QQuickState);
    return d->operations.size();
}

QQuickStateOperation *QQuickState::operationAt(int index) const
{
    Q_D(const QQuickState);
    return d->operations.at(index);
}

QQuickState &QQuickState::operator<<(QQuickStateOperation *op)
{
    Q_D(QQuickState);
    d->operations.append(QQuickStatePrivate::OperationGuard(op, &d->operations));
    return *this;
}

void QQuickStatePrivate::complete()
{
    Q_Q(QQuickState);

    for (int ii = 0; ii < reverting.size(); ++ii) {
        for (int jj = 0; jj < revertList.size(); ++jj) {
            const QQuickRevertAction &revert = reverting.at(ii);
            const QQuickSimpleAction &simple = revertList.at(jj);
            if ((revert.event && simple.event() == revert.event) ||
                simple.property() == revert.property) {
                revertList.removeAt(jj);
                break;
            }
        }
    }
    reverting.clear();

    if (group)
        group->stateAboutToComplete();
    emit q->completed();
}

// Generate a list of actions for this state.  This includes coelescing state
// actions that this state "extends"
QQuickStateOperation::ActionList
QQuickStatePrivate::generateActionList() const
{
    QQuickStateOperation::ActionList applyList;
    if (inState)
        return applyList;

    // Prevent "extends" recursion
    inState = true;

    if (!extends.isEmpty()) {
        QList<QQuickState *> states = group ? group->states() : QList<QQuickState *>();
        for (int ii = 0; ii < states.size(); ++ii)
            if (states.at(ii)->name() == extends) {
                qmlExecuteDeferred(states.at(ii));
                applyList = static_cast<QQuickStatePrivate*>(states.at(ii)->d_func())->generateActionList();
            }
    }

    for (QQuickStateOperation *op : operations)
        applyList << op->actions();

    inState = false;
    return applyList;
}

QQuickStateGroup *QQuickState::stateGroup() const
{
    Q_D(const QQuickState);
    return d->group;
}

void QQuickState::setStateGroup(QQuickStateGroup *group)
{
    Q_D(QQuickState);
    d->group = group;
}

void QQuickState::cancel()
{
    Q_D(QQuickState);
    d->transitionManager.cancel();
}

void QQuickStateAction::deleteFromBinding()
{
    if (fromBinding) {
        if (fromBinding.isAbstractPropertyBinding()) {
            QQmlPropertyPrivate::removeBinding(property);
            fromBinding = nullptr;
        }
    }
}

bool QQuickState::containsPropertyInRevertList(QObject *target, const QString &name) const
{
    Q_D(const QQuickState);

    if (isStateActive()) {
        for (const QQuickSimpleAction &simpleAction : d->revertList) {
            if (simpleAction.specifiedObject() == target && simpleAction.specifiedProperty() == name)
                return true;
        }
    }

    return false;
}

bool QQuickState::changeValueInRevertList(QObject *target, const QString &name, const QVariant &revertValue)
{
    Q_D(QQuickState);

    if (isStateActive()) {
        for (QQuickSimpleAction &simpleAction : d->revertList) {
            if (simpleAction.specifiedObject() == target && simpleAction.specifiedProperty() == name) {
                    simpleAction.setValue(revertValue);
                    return true;
            }
        }
    }

    return false;
}

bool QQuickState::changeBindingInRevertList(QObject *target, const QString &name, QQmlAnyBinding binding)
{
    Q_D(QQuickState);

    if (isStateActive()) {
        for (QQuickSimpleAction &simpleAction : d->revertList) {
            if (simpleAction.specifiedObject() == target && simpleAction.specifiedProperty() == name) {
                simpleAction.setBinding(binding);
                return true;
            }
        }
    }

    return false;
}

bool QQuickState::removeEntryFromRevertList(QObject *target, const QString &name)
{
    Q_D(QQuickState);

    if (isStateActive()) {
        for (auto it = d->revertList.begin(), end = d->revertList.end(); it != end; ++it) {
            QQuickSimpleAction &simpleAction = *it;
            if (simpleAction.property().object() == target && simpleAction.property().name() == name) {
                QQmlPropertyPrivate::removeBinding(simpleAction.property());

                simpleAction.property().write(simpleAction.value());
                if (auto binding = simpleAction.binding(); binding) {
                    QQmlProperty prop = simpleAction.property();
                    binding.installOn(prop);
                }

                d->revertList.erase(it);
                return true;
            }
        }
    }

    return false;
}

void QQuickState::addEntryToRevertList(const QQuickStateAction &action)
{
    Q_D(QQuickState);

    QQuickSimpleAction simpleAction(action);

    d->revertList.append(simpleAction);
}

void QQuickState::removeAllEntriesFromRevertList(QObject *target)
{
     Q_D(QQuickState);

     if (isStateActive()) {
         const auto actionMatchesTarget = [target](QQuickSimpleAction &simpleAction) {
             if (simpleAction.property().object() == target) {
                 QQmlPropertyPrivate::removeBinding(simpleAction.property());
                 simpleAction.property().write(simpleAction.value());
                 if (auto binding = simpleAction.binding()) {
                     QQmlProperty prop = simpleAction.property();
                     binding.installOn(prop);
                 }

                 return true;
             }
             return false;
         };

         d->revertList.erase(std::remove_if(d->revertList.begin(), d->revertList.end(),
                                            actionMatchesTarget),
                             d->revertList.end());
     }
}

void QQuickState::addEntriesToRevertList(const QList<QQuickStateAction> &actionList)
{
    Q_D(QQuickState);
    if (isStateActive()) {
        QList<QQuickSimpleAction> simpleActionList;
        simpleActionList.reserve(actionList.size());

        for (const QQuickStateAction &action : actionList) {
            QQuickSimpleAction simpleAction(action);
            action.property.write(action.toValue);
            if (auto binding = action.toBinding; binding)
                binding.installOn(action.property);

            simpleActionList.append(simpleAction);
        }

        d->revertList.append(simpleActionList);
    }
}

QVariant QQuickState::valueInRevertList(QObject *target, const QString &name) const
{
    Q_D(const QQuickState);

    if (isStateActive()) {
        for (const QQuickSimpleAction &simpleAction : d->revertList) {
            if (simpleAction.specifiedObject() == target && simpleAction.specifiedProperty() == name)
                return simpleAction.value();
        }
    }

    return QVariant();
}

QQmlAnyBinding QQuickState::bindingInRevertList(QObject *target, const QString &name) const
{
    Q_D(const QQuickState);

    if (isStateActive()) {
        for (const QQuickSimpleAction &simpleAction : d->revertList) {
            if (simpleAction.specifiedObject() == target && simpleAction.specifiedProperty() == name)
                return simpleAction.binding();
        }
    }

    return nullptr;
}

bool QQuickState::isStateActive() const
{
    return stateGroup() && stateGroup()->state() == name();
}

void QQuickState::apply(QQuickTransition *trans, QQuickState *revert)
{
    Q_D(QQuickState);

    qmlExecuteDeferred(this);

    cancel();
    if (revert)
        revert->cancel();
    d->revertList.clear();
    d->reverting.clear();

    if (revert) {
        QQuickStatePrivate *revertPrivate =
            static_cast<QQuickStatePrivate*>(revert->d_func());
        d->revertList = revertPrivate->revertList;
        revertPrivate->revertList.clear();
    }

    // List of actions caused by this state
    QQuickStateOperation::ActionList applyList = d->generateActionList();

    // List of actions that need to be reverted to roll back (just) this state
    QQuickStatePrivate::SimpleActionList additionalReverts;
    // First add the reverse of all the applyList actions
    for (int ii = 0; ii < applyList.size(); ++ii) {
        QQuickStateAction &action = applyList[ii];

        if (action.event) {
            if (!action.event->isReversable())
                continue;
            bool found = false;
            for (int jj = 0; jj < d->revertList.size(); ++jj) {
                QQuickStateActionEvent *event = d->revertList.at(jj).event();
                if (event && event->type() == action.event->type()) {
                    if (action.event->mayOverride(event)) {
                        found = true;

                        if (action.event != d->revertList.at(jj).event() && action.event->needsCopy()) {
                            action.event->copyOriginals(d->revertList.at(jj).event());

                            QQuickSimpleAction r(action);
                            additionalReverts << r;
                            d->revertList.removeAt(jj);
                            --jj;
                        } else if (action.event->isRewindable())    //###why needed?
                            action.event->saveCurrentValues();

                        break;
                    }
                }
            }
            if (!found) {
                action.event->saveOriginals();
                // Only need to revert the applyList action if the previous
                // state doesn't have a higher priority revert already
                QQuickSimpleAction r(action);
                additionalReverts << r;
            }
        } else {
            bool found = false;
            action.fromBinding = QQmlAnyBinding::ofProperty(action.property);

            for (int jj = 0; jj < d->revertList.size(); ++jj) {
                if (d->revertList.at(jj).property() == action.property) {
                    found = true;
                    if (d->revertList.at(jj).binding() != action.fromBinding) {
                        action.deleteFromBinding();
                    }
                    break;
                }
            }

            if (!found) {
                if (!action.restore) {
                    action.deleteFromBinding();;
                } else {
                    // Only need to revert the applyList action if the previous
                    // state doesn't have a higher priority revert already
                    QQuickSimpleAction r(action);
                    additionalReverts << r;
                }
            }
        }
    }

    // Any reverts from a previous state that aren't carried forth
    // into this state need to be translated into apply actions
    for (int ii = 0; ii < d->revertList.size(); ++ii) {
        bool found = false;
        if (d->revertList.at(ii).event()) {
            QQuickStateActionEvent *event = d->revertList.at(ii).event();
            if (!event->isReversable())
                continue;
            for (int jj = 0; !found && jj < applyList.size(); ++jj) {
                const QQuickStateAction &action = applyList.at(jj);
                if (action.event && action.event->type() == event->type()) {
                    if (action.event->mayOverride(event))
                        found = true;
                }
            }
        } else {
            for (int jj = 0; !found && jj < applyList.size(); ++jj) {
                const QQuickStateAction &action = applyList.at(jj);
                if (action.property == d->revertList.at(ii).property())
                    found = true;
            }
        }
        if (!found) {
            // If revert list contains bindings assigned to deleted objects, we need to
            // prevent reverting properties of those objects.
            if (d->revertList.at(ii).binding() && !d->revertList.at(ii).property().object()) {
                continue;
            }
            QVariant cur = d->revertList.at(ii).property().read();
            QQmlProperty prop = d->revertList.at(ii).property();
            QQmlAnyBinding::removeBindingFrom(prop);

            QQuickStateAction a;
            a.property = d->revertList.at(ii).property();
            a.fromValue = cur;
            a.toValue = d->revertList.at(ii).value();
            a.toBinding = d->revertList.at(ii).binding();
            a.specifiedObject = d->revertList.at(ii).specifiedObject();
            a.specifiedProperty = d->revertList.at(ii).specifiedProperty();
            a.event = d->revertList.at(ii).event();
            a.reverseEvent = d->revertList.at(ii).reverseEvent();
            if (a.event && a.event->isRewindable())
                a.event->saveCurrentValues();
            applyList << a;
            // Store these special reverts in the reverting list
            if (a.event)
                d->reverting << a.event;
            else
                d->reverting << a.property;
        }
    }
    // All the local reverts now become part of the ongoing revertList
    d->revertList << additionalReverts;

    if (lcStates().isDebugEnabled()) {
        for (const QQuickStateAction &action : std::as_const(applyList)) {
            if (action.event)
                qCDebug(lcStates) << "QQuickStateAction event:" << action.event->type();
            else
                qCDebug(lcStates) << "QQuickStateAction on" << action.property.object()
                                  << action.property.name() << "from:" << action.fromValue
                                  << "to:" << action.toValue;
        }
    }

    d->transitionManager.transition(applyList, trans);
}

QQuickStateOperation::ActionList QQuickStateOperation::actions()
{
    return ActionList();
}

QQuickState *QQuickStateOperation::state() const
{
    Q_D(const QQuickStateOperation);
    return d->m_state;
}

void QQuickStateOperation::setState(QQuickState *state)
{
    Q_D(QQuickStateOperation);
    d->m_state = state;
}

QT_END_NAMESPACE

#include "moc_qquickstate_p.cpp"
