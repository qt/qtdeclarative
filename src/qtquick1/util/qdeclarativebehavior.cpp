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

#include "QtQuick1/private/qdeclarativebehavior_p.h"

#include "QtQuick1/private/qdeclarativeanimation_p.h"
#include "QtQuick1/private/qdeclarativetransition_p.h"

#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtDeclarative/private/qdeclarativeproperty_p.h>
#include <QtDeclarative/private/qdeclarativeguard_p.h>
#include <QtDeclarative/private/qdeclarativeengine_p.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE



class QDeclarative1BehaviorPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1Behavior)
public:
    QDeclarative1BehaviorPrivate() : animation(0), enabled(true), finalized(false)
      , blockRunningChanged(false) {}

    QDeclarativeProperty property;
    QVariant currentValue;
    QVariant targetValue;
    QDeclarativeGuard<QDeclarative1AbstractAnimation> animation;
    bool enabled;
    bool finalized;
    bool blockRunningChanged;
};

/*!
    \qmlclass Behavior QDeclarative1Behavior
    \inqmlmodule QtQuick 1
    \ingroup qml-animation-transition
    \since QtQuick 1.0
    \brief The Behavior element allows you to specify a default animation for a property change.

    A Behavior defines the default animation to be applied whenever a
    particular property value changes.

    For example, the following Behavior defines a NumberAnimation to be run
    whenever the \l Rectangle's \c width value changes. When the MouseArea
    is clicked, the \c width is changed, triggering the behavior's animation:

    \snippet doc/src/snippets/qtquick1/behavior.qml 0

    Note that a property cannot have more than one assigned Behavior. To provide
    multiple animations within a Behavior, use ParallelAnimation or
    SequentialAnimation.

    If a \l{QML States}{state change} has a \l Transition that matches the same property as a
    Behavior, the \l Transition animation overrides the Behavior for that
    state change. For general advice on using Behaviors to animate state changes, see
    \l{Using QML Behaviors with States}.

    \sa {QML Animation and Transitions}, {declarative/animation/behaviors}{Behavior example}, QtDeclarative
*/


QDeclarative1Behavior::QDeclarative1Behavior(QObject *parent)
    : QObject(*(new QDeclarative1BehaviorPrivate), parent)
{
}

QDeclarative1Behavior::~QDeclarative1Behavior()
{
}

/*!
    \qmlproperty Animation QtQuick1::Behavior::animation
    \default

    This property holds the animation to run when the behavior is triggered.
*/

QDeclarative1AbstractAnimation *QDeclarative1Behavior::animation()
{
    Q_D(QDeclarative1Behavior);
    return d->animation;
}

void QDeclarative1Behavior::setAnimation(QDeclarative1AbstractAnimation *animation)
{
    Q_D(QDeclarative1Behavior);
    if (d->animation) {
        qmlInfo(this) << tr("Cannot change the animation assigned to a Behavior.");
        return;
    }

    d->animation = animation;
    if (d->animation) {
        d->animation->setDefaultTarget(d->property);
        d->animation->setDisableUserControl();
        connect(d->animation->qtAnimation(),
                SIGNAL(stateChanged(QAbstractAnimation::State,QAbstractAnimation::State)),
                this,
                SLOT(qtAnimationStateChanged(QAbstractAnimation::State,QAbstractAnimation::State)));
    }
}


void QDeclarative1Behavior::qtAnimationStateChanged(QAbstractAnimation::State newState,QAbstractAnimation::State)
{
    Q_D(QDeclarative1Behavior);
    if (!d->blockRunningChanged)
        d->animation->notifyRunningChanged(newState == QAbstractAnimation::Running);
}


/*!
    \qmlproperty bool QtQuick1::Behavior::enabled

    This property holds whether the behavior will be triggered when the tracked
    property changes value.

    By default a Behavior is enabled.
*/

bool QDeclarative1Behavior::enabled() const
{
    Q_D(const QDeclarative1Behavior);
    return d->enabled;
}

void QDeclarative1Behavior::setEnabled(bool enabled)
{
    Q_D(QDeclarative1Behavior);
    if (d->enabled == enabled)
        return;
    d->enabled = enabled;
    emit enabledChanged();
}

void QDeclarative1Behavior::write(const QVariant &value)
{
    Q_D(QDeclarative1Behavior);
    qmlExecuteDeferred(this);
    if (!d->animation || !d->enabled || !d->finalized) {
        QDeclarativePropertyPrivate::write(d->property, value, QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
        d->targetValue = value;
        return;
    }

    if (d->animation->isRunning() && value == d->targetValue)
        return;

    d->currentValue = d->property.read();
    d->targetValue = value;

    if (d->animation->qtAnimation()->duration() != -1
            && d->animation->qtAnimation()->state() != QAbstractAnimation::Stopped) {
        d->blockRunningChanged = true;
        d->animation->qtAnimation()->stop();
    }

    QDeclarative1StateOperation::ActionList actions;
    QDeclarative1Action action;
    action.property = d->property;
    action.fromValue = d->currentValue;
    action.toValue = value;
    actions << action;

    QList<QDeclarativeProperty> after;
    d->animation->transition(actions, after, QDeclarative1AbstractAnimation::Forward);
    d->animation->qtAnimation()->start();
    d->blockRunningChanged = false;
    if (!after.contains(d->property))
        QDeclarativePropertyPrivate::write(d->property, value, QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
}

void QDeclarative1Behavior::setTarget(const QDeclarativeProperty &property)
{
    Q_D(QDeclarative1Behavior);
    d->property = property;
    d->currentValue = property.read();
    if (d->animation)
        d->animation->setDefaultTarget(property);

    QDeclarativeEnginePrivate *engPriv = QDeclarativeEnginePrivate::get(qmlEngine(this));
    engPriv->registerFinalizeCallback(this, this->metaObject()->indexOfSlot("componentFinalized()"));
}

void QDeclarative1Behavior::componentFinalized()
{
    Q_D(QDeclarative1Behavior);
    d->finalized = true;
}



QT_END_NAMESPACE
