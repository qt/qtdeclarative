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

#ifndef QDECLARATIVEACCESSORS_P_H
#define QDECLARATIVEACCESSORS_P_H

#include <QtCore/qvector.h>
#include <QtCore/qhash.h>
#include <QtCore/QReadWriteLock>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QObject;
class QDeclarativeNotifier;

// QML "accessor properties" allow V4 and V8 to bypass Qt's meta system to read and, more
// importantly, subscribe to properties directly.  Any property that is primarily read
// from bindings is a candidate for inclusion as an accessor property.
//
// To define accessor properties, use the QML_DECLARE_PROPERTIES() and QML_DEFINE_PROPERTIES()
// macros.  The QML_DECLARE_PROPERTIES() macro is used to specify the properties, and the
// QML_DEFINE_PROPERTIES() macro to register the properties with the
// QDeclarativeAccessorProperties singleton.
//
// A class with accessor properties must also add the Q_CLASSINFO("qt_HasQmlAccessors", "true")
// tag to its declaration.  This is essential for QML to maintain internal consistency,
// and forgetting to do so will probably cause your application to qFatal() with a
// helpful reminder of this requirement.
//
// It is important that QML_DEFINE_PROPERTIES() has been called before QML ever sees
// the type with the accessor properties.  As QML_DEFINE_PROPERTIES() is idempotent, it is
// recommended to call it in the type's constructor as well as when the type is registered
// as a QML element (if it ever is).  QML_DEFINE_PROPERTIES() is a very cheap operation
// if registration has already occurred.

#define QML_DECLARE_PROPERTIES(type) \
    static volatile bool qdeclarative_accessor_properties_isregistered_ ## type = false; \
    static QDeclarativeAccessorProperties::Property qdeclarative_accessor_properties_ ## type[] =

#define QML_DEFINE_PROPERTIES(type) \
    do { \
        if (!qdeclarative_accessor_properties_isregistered_ ## type) { \
            int count = sizeof(qdeclarative_accessor_properties_ ## type) / \
                        sizeof(QDeclarativeAccessorProperties::Property); \
            QDeclarativeAccessorProperties::registerProperties(&type::staticMetaObject, count, \
                    qdeclarative_accessor_properties_ ## type);\
            qdeclarative_accessor_properties_isregistered_ ## type = true; \
        } \
    } while (false);

#define QML_PRIVATE_ACCESSOR(clazz, cpptype, name, variable) \
    static void clazz ## _ ## name ## Read(QObject *o, intptr_t, void *rv) \
    { \
        clazz ## Private *d = clazz ## Private::get(static_cast<clazz *>(o)); \
        *static_cast<cpptype *>(rv) = d->variable; \
    }

#define QML_PROPERTY_NAME(name) #name, sizeof #name - 1

class QDeclarativeAccessors
{
public:
    void (*read)(QObject *object, intptr_t property, void *output);
    void (*notifier)(QObject *object, intptr_t property, QDeclarativeNotifier **notifier);
};

namespace QDeclarativeAccessorProperties {
    struct Property {
        const char *name;
        unsigned int nameLength;
        intptr_t data;
        QDeclarativeAccessors *accessors;
    };

    struct Properties {
        inline Properties();
        Properties(Property *, int);

        bool operator==(const Properties &o) const {
            return count == o.count && properties == o.properties;
        }

        inline Property *property(const char *name);

        int count;
        Property *properties;
        quint32 nameMask;
    };

    Properties properties(const QMetaObject *);
    void Q_DECLARATIVE_EXPORT registerProperties(const QMetaObject *, int, Property *);
};

QDeclarativeAccessorProperties::Property *
QDeclarativeAccessorProperties::Properties::property(const char *name)
{
    if (count == 0)
        return 0;

    unsigned int length = strlen(name);

    Q_ASSERT(length);

    if (nameMask & (1 << qMin(31U, length - 1))) {

        for (int ii = 0; ii < count; ++ii) {
            if (properties[ii].nameLength == length && 0 == qstrcmp(name, properties[ii].name))
                return &properties[ii];
        }

    }

    return 0;
}

QDeclarativeAccessorProperties::Properties::Properties()
: count(0), properties(0), nameMask(0)
{
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDECLARATIVEACCESSORS_P_H
