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

#include "qdeclarativeaccessors_p.h"

#include "qdeclarativedata_p.h"
#include "qdeclarativenotifier_p.h"

QT_BEGIN_NAMESPACE

struct AccessorProperties {
    AccessorProperties();

    QReadWriteLock lock;
    QHash<const QMetaObject *, QDeclarativeAccessorProperties::Properties> properties;
};

Q_GLOBAL_STATIC(AccessorProperties, accessorProperties)

QML_PRIVATE_ACCESSOR(QObject, QString, objectName, objectName)

static void QObject_objectNameNotifier(QObject *object, intptr_t, QDeclarativeNotifier **notifier)
{
    *notifier = QDeclarativeData::get(object, true)->objectNameNotifier();
}

static QDeclarativeAccessors QObject_objectName = { QObject_objectNameRead,
                                                    QObject_objectNameNotifier };

QML_DECLARE_PROPERTIES(QObject) {
    { QML_PROPERTY_NAME(objectName), 0, &QObject_objectName }
};

static void buildNameMask(QDeclarativeAccessorProperties::Properties &properties)
{
    quint32 mask = 0;

    for (int ii = 0; ii < properties.count; ++ii) {
        Q_ASSERT(strlen(properties.properties[ii].name) == properties.properties[ii].nameLength);
        Q_ASSERT(properties.properties[ii].nameLength > 0);

        mask |= (1 << qMin(31U, properties.properties[ii].nameLength - 1));
    }

    properties.nameMask = mask;
}

AccessorProperties::AccessorProperties()
{
    // Pre-seed QObject::objectName accessor
    typedef QDeclarativeAccessorProperties::Properties P;
    properties.insert(&QObject::staticMetaObject,
                      P(qdeclarative_accessor_properties_QObject,
                        sizeof(qdeclarative_accessor_properties_QObject) /
                        sizeof(QDeclarativeAccessorProperties::Property)));
}

QDeclarativeAccessorProperties::Properties::Properties(Property *properties, int count)
: count(count), properties(properties)
{
    buildNameMask(*this);
}

QDeclarativeAccessorProperties::Properties
QDeclarativeAccessorProperties::properties(const QMetaObject *mo)
{
    AccessorProperties *This = accessorProperties();

    QReadLocker lock(&This->lock);
    return This->properties.value(mo);
}

void QDeclarativeAccessorProperties::registerProperties(const QMetaObject *mo, int count,
                                                        Property *props)
{
    Q_ASSERT(count > 0);

    Properties properties(props, count);

    AccessorProperties *This = accessorProperties();

    QWriteLocker lock(&This->lock);

    Q_ASSERT(!This->properties.contains(mo) || This->properties.value(mo) == properties);

    This->properties.insert(mo, properties);
}

QT_END_NAMESPACE
