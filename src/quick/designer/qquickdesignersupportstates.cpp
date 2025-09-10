// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickdesignersupportstates_p.h"

#include <private/qquickstategroup_p.h>
#include <private/qquickpropertychanges_p.h>

QT_BEGIN_NAMESPACE

bool QQuickDesignerSupportStates::isStateActive(QObject *object, QQmlContext *context)
{
    QQuickState *stateObject  = qobject_cast<QQuickState*>(object);

    if (!stateObject)
        return false;

    QQuickStateGroup *stateGroup = stateObject->stateGroup();

    QQmlProperty property(object, QLatin1String("name"), context);

    return stateObject && stateGroup && stateGroup->state() == property.read();
}

void QQuickDesignerSupportStates::activateState(QObject *object, QQmlContext *context)
{
    QQuickState *stateObject  = qobject_cast<QQuickState*>(object);

    if (!stateObject)
        return;

    QQuickStateGroup *stateGroup = stateObject->stateGroup();

    QQmlProperty property(object, QLatin1String("name"), context);

    stateGroup->setState(property.read().toString());
}

void QQuickDesignerSupportStates::deactivateState(QObject *object)
{
    QQuickState *stateObject  = qobject_cast<QQuickState*>(object);

    if (!stateObject)
        return;

    QQuickStateGroup *stateGroup = stateObject->stateGroup();

    if (stateGroup)
        stateGroup->setState(QString());
}

bool QQuickDesignerSupportStates::changeValueInRevertList(QObject *state, QObject *target,
                                                    const QQuickDesignerSupport::PropertyName &propertyName,
                                                    const QVariant &value)
{
    QQuickState *stateObject  = qobject_cast<QQuickState*>(state);

    if (!stateObject)
        return false;

    return stateObject->changeValueInRevertList(target, QString::fromUtf8(propertyName), value);
}

bool QQuickDesignerSupportStates::updateStateBinding(QObject *state, QObject *target,
                                               const QQuickDesignerSupport::PropertyName &propertyName,
                                               const QString &expression)
{
    QQuickState *stateObject  = qobject_cast<QQuickState*>(state);

    if (!stateObject)
        return false;

    return stateObject->changeValueInRevertList(target, QString::fromUtf8(propertyName), expression);
}

bool QQuickDesignerSupportStates::resetStateProperty(QObject *state, QObject *target,
                                               const QQuickDesignerSupport::PropertyName &propertyName,
                                               const QVariant & /* resetValue */)
{
    QQuickState *stateObject  = qobject_cast<QQuickState*>(state);

    if (!stateObject)
        return false;

    return stateObject->removeEntryFromRevertList(target, QString::fromUtf8(propertyName));
}

QT_END_NAMESPACE

