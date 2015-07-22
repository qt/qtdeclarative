/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

