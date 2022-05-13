// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickdesignersupportpropertychanges_p.h"

#include <private/qquickpropertychanges_p.h>
#include <private/qquickstateoperations_p.h>

QT_BEGIN_NAMESPACE

void QQuickDesignerSupportPropertyChanges::attachToState(QObject *propertyChanges)
{
    QQuickPropertyChanges *propertyChange = qobject_cast<QQuickPropertyChanges*>(propertyChanges);

    if (!propertyChange)
        return;

    propertyChange->attachToState();
}

QObject *QQuickDesignerSupportPropertyChanges::targetObject(QObject *propertyChanges)
{
    QQuickPropertyChanges *propertyChange = qobject_cast<QQuickPropertyChanges*>(propertyChanges);

    if (!propertyChange)
        return nullptr;

    return propertyChange->object();
}

void QQuickDesignerSupportPropertyChanges::removeProperty(QObject *propertyChanges, const QQuickDesignerSupport::PropertyName &propertyName)
{
    QQuickPropertyChanges *propertyChange = qobject_cast<QQuickPropertyChanges*>(propertyChanges);

    if (!propertyChange)
        return;

    propertyChange->removeProperty(QString::fromUtf8(propertyName));
}

QVariant QQuickDesignerSupportPropertyChanges::getProperty(QObject *propertyChanges,
                                                     const QQuickDesignerSupport::PropertyName &propertyName)
{
    QQuickPropertyChanges *propertyChange = qobject_cast<QQuickPropertyChanges*>(propertyChanges);

    if (!propertyChange)
        return QVariant();

    return propertyChange->property(QString::fromUtf8(propertyName));
}

void QQuickDesignerSupportPropertyChanges::changeValue(QObject *propertyChanges,
                                                 const QQuickDesignerSupport::PropertyName &propertyName,
                                                 const QVariant &value)
{
    QQuickPropertyChanges *propertyChange = qobject_cast<QQuickPropertyChanges*>(propertyChanges);

    if (!propertyChange)
        return;

    propertyChange->changeValue(QString::fromUtf8(propertyName), value);
}

void QQuickDesignerSupportPropertyChanges::changeExpression(QObject *propertyChanges,
                                                      const QQuickDesignerSupport::PropertyName &propertyName,
                                                      const QString &expression)
{
    QQuickPropertyChanges *propertyChange = qobject_cast<QQuickPropertyChanges*>(propertyChanges);

    if (!propertyChange)
        return;

    propertyChange->changeExpression(QString::fromUtf8(propertyName), expression);
}

QObject *QQuickDesignerSupportPropertyChanges::stateObject(QObject *propertyChanges)
{
    QQuickPropertyChanges *propertyChange = qobject_cast<QQuickPropertyChanges*>(propertyChanges);

    if (!propertyChange)
        return nullptr;

    return propertyChange->state();
}

bool QQuickDesignerSupportPropertyChanges::isNormalProperty(const QQuickDesignerSupport::PropertyName &propertyName)
{
    const QMetaObject *metaObject = &QQuickPropertyChanges::staticMetaObject;
    return (metaObject->indexOfProperty(propertyName) > 0); // 'restoreEntryValues', 'explicit'
}

void QQuickDesignerSupportPropertyChanges::detachFromState(QObject *propertyChanges)
{
    QQuickPropertyChanges *propertyChange = qobject_cast<QQuickPropertyChanges*>(propertyChanges);

    if (!propertyChange)
        return;

    propertyChange->detachFromState();
}

QT_END_NAMESPACE


