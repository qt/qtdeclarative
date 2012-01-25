/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include "QtQuick1/private/qdeclarativestate_p_p.h"
#include "QtQuick1/private/qdeclarativestate_p.h"

#include "QtQuick1/private/qdeclarativetransition_p.h"
#include "QtQuick1/private/qdeclarativestategroup_p.h"
#include "QtQuick1/private/qdeclarativestateoperations_p.h"
#include "QtQuick1/private/qdeclarativeanimation_p.h"
#include "QtQuick1/private/qdeclarativeanimation_p_p.h"

#include <QtDeclarative/private/qdeclarativebinding_p.h>
#include <QtDeclarative/private/qdeclarativeglobal_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE



DEFINE_BOOL_CONFIG_OPTION(stateChangeDebug, STATECHANGE_DEBUG);

QDeclarative1Action::QDeclarative1Action()
: restore(true), actionDone(false), reverseEvent(false), deletableToBinding(false), fromBinding(0), event(0),
  specifiedObject(0)
{
}

QDeclarative1Action::QDeclarative1Action(QObject *target, const QString &propertyName,
               const QVariant &value)
: restore(true), actionDone(false), reverseEvent(false), deletableToBinding(false), 
  property(target, propertyName, qmlEngine(target)), toValue(value),
  fromBinding(0), event(0),
  specifiedObject(target), specifiedProperty(propertyName)
{
    if (property.isValid())
        fromValue = property.read();
}

QDeclarative1Action::QDeclarative1Action(QObject *target, const QString &propertyName,
               QDeclarativeContext *context, const QVariant &value)
: restore(true), actionDone(false), reverseEvent(false), deletableToBinding(false),
  property(target, propertyName, context), toValue(value),
  fromBinding(0), event(0),
  specifiedObject(target), specifiedProperty(propertyName)
{
    if (property.isValid())
        fromValue = property.read();
}


QDeclarative1ActionEvent::~QDeclarative1ActionEvent()
{
}

QString QDeclarative1ActionEvent::typeName() const
{
    return QString();
}

void QDeclarative1ActionEvent::execute(Reason)
{
}

bool QDeclarative1ActionEvent::isReversable()
{
    return false;
}

void QDeclarative1ActionEvent::reverse(Reason)
{
}

bool QDeclarative1ActionEvent::changesBindings()
{
    return false;
}

void QDeclarative1ActionEvent::clearBindings()
{
}

bool QDeclarative1ActionEvent::override(QDeclarative1ActionEvent *other)
{
    Q_UNUSED(other);
    return false;
}

QDeclarative1StateOperation::QDeclarative1StateOperation(QObjectPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

/*!
    \qmlclass State QDeclarative1State
    \inqmlmodule QtQuick 1
    \ingroup qml-state-elements
    \since QtQuick 1.0
    \brief The State element defines configurations of objects and properties.

    A \e state is a set of batched changes from the default configuration.

    All items have a default state that defines the default configuration of objects
    and property values. New states can be defined by adding State items to the \l {Item::states}{states} property to
    allow items to switch between different configurations. These configurations
    can, for example, be used to apply different sets of property values or execute
    different scripts.

    The following example displays a single \l Rectangle. In the default state, the rectangle
    is colored black. In the "clicked" state, a PropertyChanges element changes the
    rectangle's color to red. Clicking within the MouseArea toggles the rectangle's state
    between the default state and the "clicked" state, thus toggling the color of the
    rectangle between black and red.

    \snippet doc/src/snippets/qtquick1/state.qml 0

    Notice the default state is referred to using an empty string ("").

    States are commonly used together with \l{QML Animation and Transitions}{Transitions} to provide
    animations when state changes occur.

    \note Setting the state of an object from within another state of the same object is
    not allowed.

    \sa {declarative/animation/states}{states example}, {qmlstates}{States},
    {QML Animation and Transitions}{Transitions}, QtDeclarative
*/
QDeclarative1State::QDeclarative1State(QObject *parent)
: QObject(*(new QDeclarative1StatePrivate), parent)
{
    Q_D(QDeclarative1State);
    d->transitionManager.setState(this);
}

QDeclarative1State::~QDeclarative1State()
{
    Q_D(QDeclarative1State);
    if (d->group)
        d->group->removeState(this);
}

/*!
    \qmlproperty string QtQuick1::State::name
    This property holds the name of the state.

    Each state should have a unique name within its item.
*/
QString QDeclarative1State::name() const
{
    Q_D(const QDeclarative1State);
    return d->name;
}

void QDeclarative1State::setName(const QString &n)
{
    Q_D(QDeclarative1State);
    d->name = n;
    d->named = true;
}

bool QDeclarative1State::isNamed() const
{
    Q_D(const QDeclarative1State);
    return d->named;
}

bool QDeclarative1State::isWhenKnown() const
{
    Q_D(const QDeclarative1State);
    return d->when != 0;
}

/*!
    \qmlproperty bool QtQuick1::State::when
    This property holds when the state should be applied.

    This should be set to an expression that evaluates to \c true when you want the state to
    be applied. For example, the following \l Rectangle changes in and out of the "hidden"
    state when the \l MouseArea is pressed:

    \snippet doc/src/snippets/qtquick1/state-when.qml 0

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
QDeclarativeBinding *QDeclarative1State::when() const
{
    Q_D(const QDeclarative1State);
    return d->when;
}

void QDeclarative1State::setWhen(QDeclarativeBinding *when)
{
    Q_D(QDeclarative1State);
    d->when = when;
    if (d->group)
        d->group->updateAutoState();
}

/*!
    \qmlproperty string QtQuick1::State::extend
    This property holds the state that this state extends.

    When a state extends another state, it inherits all the changes of that state.

    The state being extended is treated as the base state in regards to
    the changes specified by the extending state.
*/
QString QDeclarative1State::extends() const
{
    Q_D(const QDeclarative1State);
    return d->extends;
}

void QDeclarative1State::setExtends(const QString &extends)
{
    Q_D(QDeclarative1State);
    d->extends = extends;
}

/*!
    \qmlproperty list<Change> QtQuick1::State::changes
    This property holds the changes to apply for this state
    \default

    By default these changes are applied against the default state. If the state
    extends another state, then the changes are applied against the state being
    extended.
*/
QDeclarativeListProperty<QDeclarative1StateOperation> QDeclarative1State::changes()
{
    Q_D(QDeclarative1State);
    return QDeclarativeListProperty<QDeclarative1StateOperation>(this, &d->operations, QDeclarative1StatePrivate::operations_append,
                                              QDeclarative1StatePrivate::operations_count, QDeclarative1StatePrivate::operations_at,
                                              QDeclarative1StatePrivate::operations_clear);
}

int QDeclarative1State::operationCount() const
{
    Q_D(const QDeclarative1State);
    return d->operations.count();
}

QDeclarative1StateOperation *QDeclarative1State::operationAt(int index) const
{
    Q_D(const QDeclarative1State);
    return d->operations.at(index);
}

QDeclarative1State &QDeclarative1State::operator<<(QDeclarative1StateOperation *op)
{
    Q_D(QDeclarative1State);
    d->operations.append(QDeclarative1StatePrivate::OperationGuard(op, &d->operations));
    return *this;
}

void QDeclarative1StatePrivate::complete()
{
    Q_Q(QDeclarative1State);

    for (int ii = 0; ii < reverting.count(); ++ii) {
        for (int jj = 0; jj < revertList.count(); ++jj) {
            if (revertList.at(jj).property() == reverting.at(ii)) {
                revertList.removeAt(jj);
                break;
            }
        }
    }
    reverting.clear();

    emit q->completed();
}

// Generate a list of actions for this state.  This includes coelescing state
// actions that this state "extends"
QDeclarative1StateOperation::ActionList
QDeclarative1StatePrivate::generateActionList(QDeclarative1StateGroup *group) const
{
    QDeclarative1StateOperation::ActionList applyList;
    if (inState)
        return applyList;

    // Prevent "extends" recursion
    inState = true;

    if (!extends.isEmpty()) {
        QList<QDeclarative1State *> states = group->states();
        for (int ii = 0; ii < states.count(); ++ii)
            if (states.at(ii)->name() == extends) {
                qmlExecuteDeferred(states.at(ii));
                applyList = static_cast<QDeclarative1StatePrivate*>(states.at(ii)->d_func())->generateActionList(group);
            }
    }

    foreach(QDeclarative1StateOperation *op, operations)
        applyList << op->actions();

    inState = false;
    return applyList;
}

QDeclarative1StateGroup *QDeclarative1State::stateGroup() const
{
    Q_D(const QDeclarative1State);
    return d->group;
}

void QDeclarative1State::setStateGroup(QDeclarative1StateGroup *group)
{
    Q_D(QDeclarative1State);
    d->group = group;
}

void QDeclarative1State::cancel()
{
    Q_D(QDeclarative1State);
    d->transitionManager.cancel();
}

void QDeclarative1Action::deleteFromBinding()
{
    if (fromBinding) {
        QDeclarativePropertyPrivate::setBinding(property, 0);
        fromBinding->destroy();
        fromBinding = 0;
    }
}

bool QDeclarative1State::containsPropertyInRevertList(QObject *target, const QString &name) const
{
    Q_D(const QDeclarative1State);

    if (isStateActive()) {
        QListIterator<QDeclarative1SimpleAction> revertListIterator(d->revertList);

        while (revertListIterator.hasNext()) {
            const QDeclarative1SimpleAction &simpleAction = revertListIterator.next();
            if (simpleAction.specifiedObject() == target && simpleAction.specifiedProperty() == name)
                return true;
        }
    }

    return false;
}

bool QDeclarative1State::changeValueInRevertList(QObject *target, const QString &name, const QVariant &revertValue)
{
    Q_D(QDeclarative1State);

    if (isStateActive()) {
        QMutableListIterator<QDeclarative1SimpleAction> revertListIterator(d->revertList);

        while (revertListIterator.hasNext()) {
            QDeclarative1SimpleAction &simpleAction = revertListIterator.next();
            if (simpleAction.specifiedObject() == target && simpleAction.specifiedProperty() == name) {
                    simpleAction.setValue(revertValue);
                    return true;
            }
        }
    }

    return false;
}

bool QDeclarative1State::changeBindingInRevertList(QObject *target, const QString &name, QDeclarativeAbstractBinding *binding)
{
    Q_D(QDeclarative1State);

    if (isStateActive()) {
        QMutableListIterator<QDeclarative1SimpleAction> revertListIterator(d->revertList);

        while (revertListIterator.hasNext()) {
            QDeclarative1SimpleAction &simpleAction = revertListIterator.next();
            if (simpleAction.specifiedObject() == target && simpleAction.specifiedProperty() == name) {
                if (simpleAction.binding())
                    simpleAction.binding()->destroy();

                simpleAction.setBinding(binding);
                return true;
            }
        }
    }

    return false;
}

bool QDeclarative1State::removeEntryFromRevertList(QObject *target, const QString &name)
{
    Q_D(QDeclarative1State);

    if (isStateActive()) {
        QMutableListIterator<QDeclarative1SimpleAction> revertListIterator(d->revertList);

        while (revertListIterator.hasNext()) {
            QDeclarative1SimpleAction &simpleAction = revertListIterator.next();
            if (simpleAction.property().object() == target && simpleAction.property().name() == name) {
                QDeclarativeAbstractBinding *oldBinding = QDeclarativePropertyPrivate::binding(simpleAction.property());
                if (oldBinding) {
                    QDeclarativePropertyPrivate::setBinding(simpleAction.property(), 0);
                    oldBinding->destroy();
                }

                simpleAction.property().write(simpleAction.value());
                if (simpleAction.binding())
                    QDeclarativePropertyPrivate::setBinding(simpleAction.property(), simpleAction.binding());

                revertListIterator.remove();
                return true;
            }
        }
    }

    return false;
}

void QDeclarative1State::addEntryToRevertList(const QDeclarative1Action &action)
{
    Q_D(QDeclarative1State);

    QDeclarative1SimpleAction simpleAction(action);

    d->revertList.append(simpleAction);
}

void QDeclarative1State::removeAllEntriesFromRevertList(QObject *target)
{
     Q_D(QDeclarative1State);

     if (isStateActive()) {
         QMutableListIterator<QDeclarative1SimpleAction> revertListIterator(d->revertList);

         while (revertListIterator.hasNext()) {
             QDeclarative1SimpleAction &simpleAction = revertListIterator.next();
             if (simpleAction.property().object() == target) {
                 QDeclarativeAbstractBinding *oldBinding = QDeclarativePropertyPrivate::binding(simpleAction.property());
                 if (oldBinding) {
                     QDeclarativePropertyPrivate::setBinding(simpleAction.property(), 0);
                     oldBinding->destroy();
                 }

                 simpleAction.property().write(simpleAction.value());
                 if (simpleAction.binding())
                     QDeclarativePropertyPrivate::setBinding(simpleAction.property(), simpleAction.binding());

                 revertListIterator.remove();
             }
         }
     }
}

void QDeclarative1State::addEntriesToRevertList(const QList<QDeclarative1Action> &actionList)
{
    Q_D(QDeclarative1State);
    if (isStateActive()) {
        QList<QDeclarative1SimpleAction> simpleActionList;

        QListIterator<QDeclarative1Action> actionListIterator(actionList);
        while(actionListIterator.hasNext()) {
            const QDeclarative1Action &action = actionListIterator.next();
            QDeclarative1SimpleAction simpleAction(action);
            action.property.write(action.toValue);
            if (!action.toBinding.isNull()) {
                QDeclarativeAbstractBinding *oldBinding = QDeclarativePropertyPrivate::binding(simpleAction.property());
                if (oldBinding)
                    QDeclarativePropertyPrivate::setBinding(simpleAction.property(), 0);
                QDeclarativePropertyPrivate::setBinding(simpleAction.property(), action.toBinding.data(), QDeclarativePropertyPrivate::DontRemoveBinding);
            }

            simpleActionList.append(simpleAction);
        }

        d->revertList.append(simpleActionList);
    }
}

QVariant QDeclarative1State::valueInRevertList(QObject *target, const QString &name) const
{
    Q_D(const QDeclarative1State);

    if (isStateActive()) {
        QListIterator<QDeclarative1SimpleAction> revertListIterator(d->revertList);

        while (revertListIterator.hasNext()) {
            const QDeclarative1SimpleAction &simpleAction = revertListIterator.next();
            if (simpleAction.specifiedObject() == target && simpleAction.specifiedProperty() == name)
                return simpleAction.value();
        }
    }

    return QVariant();
}

QDeclarativeAbstractBinding *QDeclarative1State::bindingInRevertList(QObject *target, const QString &name) const
{
    Q_D(const QDeclarative1State);

    if (isStateActive()) {
        QListIterator<QDeclarative1SimpleAction> revertListIterator(d->revertList);

        while (revertListIterator.hasNext()) {
            const QDeclarative1SimpleAction &simpleAction = revertListIterator.next();
            if (simpleAction.specifiedObject() == target && simpleAction.specifiedProperty() == name)
                return simpleAction.binding();
        }
    }

    return 0;
}

bool QDeclarative1State::isStateActive() const
{
    return stateGroup() && stateGroup()->state() == name();
}

void QDeclarative1State::apply(QDeclarative1StateGroup *group, QDeclarative1Transition *trans, QDeclarative1State *revert)
{
    Q_D(QDeclarative1State);

    qmlExecuteDeferred(this);

    cancel();
    if (revert)
        revert->cancel();
    d->revertList.clear();
    d->reverting.clear();

    if (revert) {
        QDeclarative1StatePrivate *revertPrivate =
            static_cast<QDeclarative1StatePrivate*>(revert->d_func());
        d->revertList = revertPrivate->revertList;
        revertPrivate->revertList.clear();
    }

    // List of actions caused by this state
    QDeclarative1StateOperation::ActionList applyList = d->generateActionList(group);

    // List of actions that need to be reverted to roll back (just) this state
    QDeclarative1StatePrivate::SimpleActionList additionalReverts;
    // First add the reverse of all the applyList actions
    for (int ii = 0; ii < applyList.count(); ++ii) {
        QDeclarative1Action &action = applyList[ii];

        if (action.event) {
            if (!action.event->isReversable())
                continue;
            bool found = false;
            for (int jj = 0; jj < d->revertList.count(); ++jj) {
                QDeclarative1ActionEvent *event = d->revertList.at(jj).event();
                if (event && event->typeName() == action.event->typeName()) {
                    if (action.event->override(event)) {
                        found = true;

                        if (action.event != d->revertList.at(jj).event() && action.event->needsCopy()) {
                            action.event->copyOriginals(d->revertList.at(jj).event());

                            QDeclarative1SimpleAction r(action);
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
                QDeclarative1SimpleAction r(action);
                additionalReverts << r;
            }
        } else {
            bool found = false;
            action.fromBinding = QDeclarativePropertyPrivate::binding(action.property);

            for (int jj = 0; jj < d->revertList.count(); ++jj) {
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
                    QDeclarative1SimpleAction r(action);
                    additionalReverts << r;
                }
            }
        }
    }

    // Any reverts from a previous state that aren't carried forth
    // into this state need to be translated into apply actions
    for (int ii = 0; ii < d->revertList.count(); ++ii) {
        bool found = false;
        if (d->revertList.at(ii).event()) {
            QDeclarative1ActionEvent *event = d->revertList.at(ii).event();
            if (!event->isReversable())
                continue;
            for (int jj = 0; !found && jj < applyList.count(); ++jj) {
                const QDeclarative1Action &action = applyList.at(jj);
                if (action.event && action.event->typeName() == event->typeName()) {
                    if (action.event->override(event))
                        found = true;
                }
            }
        } else {
            for (int jj = 0; !found && jj < applyList.count(); ++jj) {
                const QDeclarative1Action &action = applyList.at(jj);
                if (action.property == d->revertList.at(ii).property())
                    found = true;
            }
        }
        if (!found) {
            QVariant cur = d->revertList.at(ii).property().read();
            QDeclarativeAbstractBinding *delBinding = 
                QDeclarativePropertyPrivate::setBinding(d->revertList.at(ii).property(), 0);
            if (delBinding)
                delBinding->destroy();

            QDeclarative1Action a;
            a.property = d->revertList.at(ii).property();
            a.fromValue = cur;
            a.toValue = d->revertList.at(ii).value();
            a.toBinding = QDeclarativeAbstractBinding::getPointer(d->revertList.at(ii).binding());
            a.specifiedObject = d->revertList.at(ii).specifiedObject();
            a.specifiedProperty = d->revertList.at(ii).specifiedProperty();
            a.event = d->revertList.at(ii).event();
            a.reverseEvent = d->revertList.at(ii).reverseEvent();
            if (a.event && a.event->isRewindable())
                a.event->saveCurrentValues();
            applyList << a;
            // Store these special reverts in the reverting list
            d->reverting << d->revertList.at(ii).property();
        }
    }
    // All the local reverts now become part of the ongoing revertList
    d->revertList << additionalReverts;

#ifndef QT_NO_DEBUG_STREAM
    // Output for debugging
    if (stateChangeDebug()) {
        foreach(const QDeclarative1Action &action, applyList) {
            if (action.event)
                qWarning() << "    QDeclarative1Action event:" << action.event->typeName();
            else
                qWarning() << "    QDeclarative1Action:" << action.property.object()
                           << action.property.name() << "From:" << action.fromValue 
                           << "To:" << action.toValue;
        }
    }
#endif

    d->transitionManager.transition(applyList, trans);
}

QDeclarative1StateOperation::ActionList QDeclarative1StateOperation::actions()
{
    return ActionList();
}

QDeclarative1State *QDeclarative1StateOperation::state() const
{
    Q_D(const QDeclarative1StateOperation);
    return d->m_state;
}

void QDeclarative1StateOperation::setState(QDeclarative1State *state)
{
    Q_D(QDeclarative1StateOperation);
    d->m_state = state;
}



QT_END_NAMESPACE
