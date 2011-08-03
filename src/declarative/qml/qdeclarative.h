/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDECLARATIVE_H
#define QDECLARATIVE_H

#include <QtDeclarative/qdeclarativeprivate.h>
#include <QtDeclarative/qdeclarativeparserstatus.h>
#include <QtDeclarative/qdeclarativepropertyvaluesource.h>
#include <QtDeclarative/qdeclarativepropertyvalueinterceptor.h>
#include <QtDeclarative/qdeclarativelist.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_HEADER

#define QML_VERSION     0x020000
#define QML_VERSION_STR "2.0"

#define QML_DECLARE_TYPE(TYPE) \
    Q_DECLARE_METATYPE(TYPE *) \
    Q_DECLARE_METATYPE(QDeclarativeListProperty<TYPE>) 

#define QML_DECLARE_TYPE_HASMETATYPE(TYPE) \
    Q_DECLARE_METATYPE(QDeclarativeListProperty<TYPE>) 

#define QML_DECLARE_INTERFACE(INTERFACE) \
    QML_DECLARE_TYPE(INTERFACE)

#define QML_DECLARE_INTERFACE_HASMETATYPE(INTERFACE) \
    QML_DECLARE_TYPE_HASMETATYPE(INTERFACE)

enum { /* TYPEINFO flags */
    QML_HAS_ATTACHED_PROPERTIES = 0x01
};

#define QML_DECLARE_TYPEINFO(TYPE, FLAGS) \
QT_BEGIN_NAMESPACE \
template <> \
class QDeclarativeTypeInfo<TYPE > \
{ \
public: \
    enum { \
        hasAttachedProperties = (((FLAGS) & QML_HAS_ATTACHED_PROPERTIES) == QML_HAS_ATTACHED_PROPERTIES) \
    }; \
}; \
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

template<typename T>
int qmlRegisterType()
{
    QByteArray name(T::staticMetaObject.className());

    QByteArray pointerName(name + '*');
    QByteArray listName("QDeclarativeListProperty<" + name + ">");

    QDeclarativePrivate::RegisterType type = {
        0, 

        qRegisterMetaType<T *>(pointerName.constData()),
        qRegisterMetaType<QDeclarativeListProperty<T> >(listName.constData()),
        0, 0,
        QString(),

        0, 0, 0, 0, &T::staticMetaObject,

        QDeclarativePrivate::attachedPropertiesFunc<T>(),
        QDeclarativePrivate::attachedPropertiesMetaObject<T>(),

        QDeclarativePrivate::StaticCastSelector<T,QDeclarativeParserStatus>::cast(), 
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueSource>::cast(),
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueInterceptor>::cast(),

        0, 0,

        0,
        0
    };

    return QDeclarativePrivate::qmlregister(QDeclarativePrivate::TypeRegistration, &type);
}

int Q_DECLARATIVE_EXPORT qmlRegisterTypeNotAvailable(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& message);

template<typename T>
int qmlRegisterUncreatableType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason)
{
    QByteArray name(T::staticMetaObject.className());

    QByteArray pointerName(name + '*');
    QByteArray listName("QDeclarativeListProperty<" + name + ">");

    QDeclarativePrivate::RegisterType type = {
        0,

        qRegisterMetaType<T *>(pointerName.constData()),
        qRegisterMetaType<QDeclarativeListProperty<T> >(listName.constData()),
        0, 0,
        reason,

        uri, versionMajor, versionMinor, qmlName, &T::staticMetaObject,

        QDeclarativePrivate::attachedPropertiesFunc<T>(),
        QDeclarativePrivate::attachedPropertiesMetaObject<T>(),

        QDeclarativePrivate::StaticCastSelector<T,QDeclarativeParserStatus>::cast(),
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueSource>::cast(),
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueInterceptor>::cast(),

        0, 0,

        0,
        0
    };

    return QDeclarativePrivate::qmlregister(QDeclarativePrivate::TypeRegistration, &type);
}

template<typename T>
int qmlRegisterType(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    QByteArray name(T::staticMetaObject.className());

    QByteArray pointerName(name + '*');
    QByteArray listName("QDeclarativeListProperty<" + name + ">");

    QDeclarativePrivate::RegisterType type = {
        0, 

        qRegisterMetaType<T *>(pointerName.constData()),
        qRegisterMetaType<QDeclarativeListProperty<T> >(listName.constData()),
        sizeof(T), QDeclarativePrivate::createInto<T>,
        QString(),

        uri, versionMajor, versionMinor, qmlName, &T::staticMetaObject,

        QDeclarativePrivate::attachedPropertiesFunc<T>(),
        QDeclarativePrivate::attachedPropertiesMetaObject<T>(),

        QDeclarativePrivate::StaticCastSelector<T,QDeclarativeParserStatus>::cast(), 
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueSource>::cast(),
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueInterceptor>::cast(),

        0, 0,

        0,
        0
    };

    return QDeclarativePrivate::qmlregister(QDeclarativePrivate::TypeRegistration, &type);
}

template<typename T, int metaObjectRevision>
int qmlRegisterType(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    QByteArray name(T::staticMetaObject.className());

    QByteArray pointerName(name + '*');
    QByteArray listName("QDeclarativeListProperty<" + name + ">");

    QDeclarativePrivate::RegisterType type = {
        1,

        qRegisterMetaType<T *>(pointerName.constData()),
        qRegisterMetaType<QDeclarativeListProperty<T> >(listName.constData()),
        sizeof(T), QDeclarativePrivate::createInto<T>,
        QString(),

        uri, versionMajor, versionMinor, qmlName, &T::staticMetaObject,

        QDeclarativePrivate::attachedPropertiesFunc<T>(),
        QDeclarativePrivate::attachedPropertiesMetaObject<T>(),

        QDeclarativePrivate::StaticCastSelector<T,QDeclarativeParserStatus>::cast(),
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueSource>::cast(),
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueInterceptor>::cast(),

        0, 0,

        0,
        metaObjectRevision
    };

    return QDeclarativePrivate::qmlregister(QDeclarativePrivate::TypeRegistration, &type);
}

template<typename T, int metaObjectRevision>
int qmlRegisterRevision(const char *uri, int versionMajor, int versionMinor)
{
    QByteArray name(T::staticMetaObject.className());

    QByteArray pointerName(name + '*');
    QByteArray listName("QDeclarativeListProperty<" + name + ">");

    QDeclarativePrivate::RegisterType type = {
        1,

        qRegisterMetaType<T *>(pointerName.constData()),
        qRegisterMetaType<QDeclarativeListProperty<T> >(listName.constData()),
        sizeof(T), QDeclarativePrivate::createInto<T>,
        QString(),

        uri, versionMajor, versionMinor, 0, &T::staticMetaObject,

        QDeclarativePrivate::attachedPropertiesFunc<T>(),
        QDeclarativePrivate::attachedPropertiesMetaObject<T>(),

        QDeclarativePrivate::StaticCastSelector<T,QDeclarativeParserStatus>::cast(),
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueSource>::cast(),
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueInterceptor>::cast(),

        0, 0,

        0,
        metaObjectRevision
    };

    return QDeclarativePrivate::qmlregister(QDeclarativePrivate::TypeRegistration, &type);
}


template<typename T, typename E>
int qmlRegisterExtendedType()
{
    QByteArray name(T::staticMetaObject.className());

    QByteArray pointerName(name + '*');
    QByteArray listName("QDeclarativeListProperty<" + name + ">");

    QDeclarativePrivate::RegisterType type = {
        0, 

        qRegisterMetaType<T *>(pointerName.constData()),
        qRegisterMetaType<QDeclarativeListProperty<T> >(listName.constData()),
        0, 0,
        QString(),

        0, 0, 0, 0, &T::staticMetaObject,

        QDeclarativePrivate::attachedPropertiesFunc<T>(),
        QDeclarativePrivate::attachedPropertiesMetaObject<T>(),

        QDeclarativePrivate::StaticCastSelector<T,QDeclarativeParserStatus>::cast(), 
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueSource>::cast(),
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueInterceptor>::cast(),

        QDeclarativePrivate::createParent<E>, &E::staticMetaObject,

        0,
        0
    };

    return QDeclarativePrivate::qmlregister(QDeclarativePrivate::TypeRegistration, &type);
}

template<typename T, typename E>
int qmlRegisterExtendedType(const char *uri, int versionMajor, int versionMinor, 
                            const char *qmlName)
{
    QByteArray name(T::staticMetaObject.className());

    QByteArray pointerName(name + '*');
    QByteArray listName("QDeclarativeListProperty<" + name + ">");

    QDeclarativeAttachedPropertiesFunc attached = QDeclarativePrivate::attachedPropertiesFunc<E>();
    const QMetaObject * attachedMetaObject = QDeclarativePrivate::attachedPropertiesMetaObject<E>(); 
    if (!attached) {
        attached = QDeclarativePrivate::attachedPropertiesFunc<T>();
        attachedMetaObject = QDeclarativePrivate::attachedPropertiesMetaObject<T>();
    }

    QDeclarativePrivate::RegisterType type = {
        0, 

        qRegisterMetaType<T *>(pointerName.constData()),
        qRegisterMetaType<QDeclarativeListProperty<T> >(listName.constData()),
        sizeof(T), QDeclarativePrivate::createInto<T>,
        QString(),

        uri, versionMajor, versionMinor, qmlName, &T::staticMetaObject,

        attached,
        attachedMetaObject,

        QDeclarativePrivate::StaticCastSelector<T,QDeclarativeParserStatus>::cast(), 
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueSource>::cast(),
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueInterceptor>::cast(),

        QDeclarativePrivate::createParent<E>, &E::staticMetaObject,

        0,
        0
    };

    return QDeclarativePrivate::qmlregister(QDeclarativePrivate::TypeRegistration, &type);
}

template<typename T>
int qmlRegisterInterface(const char *typeName)
{
    QByteArray name(typeName);

    QByteArray pointerName(name + '*');
    QByteArray listName("QDeclarativeListProperty<" + name + ">");

    QDeclarativePrivate::RegisterInterface qmlInterface = {
        0,

        qRegisterMetaType<T *>(pointerName.constData()),
        qRegisterMetaType<QDeclarativeListProperty<T> >(listName.constData()),

        qobject_interface_iid<T *>()
    };

    return QDeclarativePrivate::qmlregister(QDeclarativePrivate::InterfaceRegistration, &qmlInterface);
}

template<typename T>
int qmlRegisterCustomType(const char *uri, int versionMajor, int versionMinor, 
                          const char *qmlName, QDeclarativeCustomParser *parser)
{
    QByteArray name(T::staticMetaObject.className());

    QByteArray pointerName(name + '*');
    QByteArray listName("QDeclarativeListProperty<" + name + ">");

    QDeclarativePrivate::RegisterType type = {
        0, 

        qRegisterMetaType<T *>(pointerName.constData()),
        qRegisterMetaType<QDeclarativeListProperty<T> >(listName.constData()),
        sizeof(T), QDeclarativePrivate::createInto<T>,
        QString(),

        uri, versionMajor, versionMinor, qmlName, &T::staticMetaObject,

        QDeclarativePrivate::attachedPropertiesFunc<T>(),
        QDeclarativePrivate::attachedPropertiesMetaObject<T>(),

        QDeclarativePrivate::StaticCastSelector<T,QDeclarativeParserStatus>::cast(), 
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueSource>::cast(),
        QDeclarativePrivate::StaticCastSelector<T,QDeclarativePropertyValueInterceptor>::cast(),

        0, 0,

        parser,
        0
    };

    return QDeclarativePrivate::qmlregister(QDeclarativePrivate::TypeRegistration, &type);
}

class QDeclarativeContext;
class QDeclarativeEngine;
class QJSValue;
class QJSEngine;
Q_DECLARATIVE_EXPORT void qmlExecuteDeferred(QObject *);
Q_DECLARATIVE_EXPORT QDeclarativeContext *qmlContext(const QObject *);
Q_DECLARATIVE_EXPORT QDeclarativeEngine *qmlEngine(const QObject *);
Q_DECLARATIVE_EXPORT QObject *qmlAttachedPropertiesObjectById(int, const QObject *, bool create = true);
Q_DECLARATIVE_EXPORT QObject *qmlAttachedPropertiesObject(int *, const QObject *, const QMetaObject *, bool create);

template<typename T>
QObject *qmlAttachedPropertiesObject(const QObject *obj, bool create = true)
{
    static int idx = -1;
    return qmlAttachedPropertiesObject(&idx, obj, &T::staticMetaObject, create);
}

// For the use of QtQuick1 module
Q_DECLARATIVE_EXPORT void qmlRegisterBaseTypes(const char *uri, int versionMajor, int versionMinor);

/*!
   This function may be used to register a module API provider \a callback in a particular \a uri
   with a version specified in \a versionMajor and \a versionMinor.

   Installing a module API into a uri allows developers to provide arbitrary functionality
   (methods and properties) in a namespace that doesn't necessarily contain elements.

   A module API may be either a QObject or a QJSValue.  Only one module API provider
   may be registered into any given namespace (combination of \a uri, \a majorVersion and \a minorVersion).
   This function should be used to register a module API provider function which returns a QJSValue as a module API.

   \e NOTE: QJSValue module API properties will \e not trigger binding re-evaluation if changed.

   Usage:
   \code
   // first, define the module API provider function (callback).
   static QJSValue *example_qjsvalue_module_api_provider(QDeclarativeEngine *engine, QJSEngine *scriptEngine)
   {
       Q_UNUSED(engine)

       static int seedValue = 5;
       QJSValue example = scriptEngine->newObject();
       example.setProperty("someProperty", seedValue++);
       return example;
   }

   // second, register the module API provider with QML by calling this function in an initialization function.
   ...
   qmlRegisterModuleApi("Qt.example.qjsvalueApi", 1, 0, example_qjsvalue_module_api_provider);
   ...
   \endcode

   In order to use the registered module API in QML, you must import the module API.
   \qml
   import QtQuick 2.0
   import Qt.example.qjsvalueApi 1.0 as ExampleApi
   Item {
       id: root
       property int someValue: ExampleApi.someProperty
   }
   \endqml
  */
inline int qmlRegisterModuleApi(const char *uri, int versionMajor, int versionMinor,
                                QJSValue (*callback)(QDeclarativeEngine *, QJSEngine *))
{
    QDeclarativePrivate::RegisterModuleApi api = {
        0,

        uri, versionMajor, versionMinor,

        callback, 0
    };

    return QDeclarativePrivate::qmlregister(QDeclarativePrivate::ModuleApiRegistration, &api);
}

/*!
   This function may be used to register a module API provider \a callback in a particular \a uri
   with a version specified in \a versionMajor and \a versionMinor.

   Installing a module API into a uri allows developers to provide arbitrary functionality
   (methods and properties) in a namespace that doesn't necessarily contain elements.

   A module API may be either a QObject or a QJSValue.  Only one module API provider
   may be registered into any given namespace (combination of \a uri, \a majorVersion and \a minorVersion).
   This function should be used to register a module API provider function which returns a QObject as a module API.

   Usage:
   \code
   // first, define your QObject which provides the functionality.
   class ModuleApiExample : public QObject
   {
       Q_OBJECT
       Q_PROPERTY (int someProperty READ someProperty WRITE setSomeProperty NOTIFY somePropertyChanged)

   public:
       ModuleApiExample(QObject* parent = 0)
           : QObject(parent), m_someProperty(0)
       {
       }

       ~ModuleApiExample() {}

       Q_INVOKABLE int doSomething() { setSomeProperty(5); return m_someProperty; }

       int someProperty() const { return m_someProperty; }
       void setSomeProperty(int val) { m_someProperty = val; emit somePropertyChanged(val); }

   signals:
       void somePropertyChanged(int newValue);

   private:
       int m_someProperty;
   };

   // second, define the module API provider function (callback).
   static QObject *example_qobject_module_api_provider(QDeclarativeEngine *engine, QJSEngine *scriptEngine)
   {
       Q_UNUSED(engine)
       Q_UNUSED(scriptEngine)

       ModuleApiExample *example = new ModuleApiExample();
       return example;
   }

   // third, register the module API provider with QML by calling this function in an initialization function.
   ...
   qmlRegisterModuleApi("Qt.example.qobjectApi", 1, 0, example_qobject_module_api_provider);
   ...
   \endcode

   In order to use the registered module API in QML, you must import the module API.
   \qml
   import QtQuick 2.0
   import Qt.example.qobjectApi 1.0 as ExampleApi
   Item {
       id: root
       property int someValue: ExampleApi.someProperty

       Component.onCompleted: {
           someValue = ExampleApi.doSomething()
       }
   }
   \endqml
  */
inline int qmlRegisterModuleApi(const char *uri, int versionMajor, int versionMinor,
                                QObject *(*callback)(QDeclarativeEngine *, QJSEngine *))
{
    QDeclarativePrivate::RegisterModuleApi api = {
        0,

        uri, versionMajor, versionMinor,

        0, callback
    };

    return QDeclarativePrivate::qmlregister(QDeclarativePrivate::ModuleApiRegistration, &api);
}

// Enable debugging before any QDeclarativeEngine is created
struct Q_DECLARATIVE_EXPORT QDeclarativeDebuggingEnabler
{
    QDeclarativeDebuggingEnabler();
};

// Execute code in constructor before first QDeclarativeEngine is instantiated
#if defined(QT_DECLARATIVE_DEBUG)
static QDeclarativeDebuggingEnabler qmlEnableDebuggingHelper;
#endif

QT_END_NAMESPACE

QML_DECLARE_TYPE(QObject)
Q_DECLARE_METATYPE(QVariant)

QT_END_HEADER

#endif // QDECLARATIVE_H
