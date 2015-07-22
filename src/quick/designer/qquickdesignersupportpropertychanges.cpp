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
        return 0;

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
        return 0;

    return propertyChange->state();
}

bool QQuickDesignerSupportPropertyChanges::isNormalProperty(const QQuickDesignerSupport::PropertyName &propertyName)
{
    QMetaObject metaObject = QQuickPropertyChanges::staticMetaObject;

    return (metaObject.indexOfProperty(propertyName) > 0); // 'restoreEntryValues', 'explicit'
}

void QQuickDesignerSupportPropertyChanges::detachFromState(QObject *propertyChanges)
{
    QQuickPropertyChanges *propertyChange = qobject_cast<QQuickPropertyChanges*>(propertyChanges);

    if (!propertyChange)
        return;

    propertyChange->detachFromState();
}

QT_END_NAMESPACE


