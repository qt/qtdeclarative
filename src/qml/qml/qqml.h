/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQML_H
#define QQML_H

#include <QtQml/qqmlprivate.h>
#include <QtQml/qjsvalue.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmetacontainer.h>
#include <QtCore/qversionnumber.h>

#define QML_VERSION     0x020000
#define QML_VERSION_STR "2.0"

#define QML_PRIVATE_NAMESPACE \
    QT_PREPEND_NAMESPACE(QQmlPrivate)

#define QML_REGISTER_TYPES_AND_REVISIONS \
    QT_PREPEND_NAMESPACE(qmlRegisterTypesAndRevisions)

#define QML_DECLARE_TYPE(TYPE) \
    Q_DECLARE_METATYPE(TYPE *) \
    Q_DECLARE_METATYPE(QQmlListProperty<TYPE>)

#define QML_DECLARE_TYPE_HASMETATYPE(TYPE) \
    Q_DECLARE_METATYPE(QQmlListProperty<TYPE>)

#define QML_DECLARE_INTERFACE(INTERFACE) \
    QML_DECLARE_TYPE(INTERFACE)

#define QML_DECLARE_INTERFACE_HASMETATYPE(INTERFACE) \
    QML_DECLARE_TYPE_HASMETATYPE(INTERFACE)

#define QML_ELEMENT \
    Q_CLASSINFO("QML.Element", "auto")

#define QML_ANONYMOUS \
    Q_CLASSINFO("QML.Element", "anonymous")

#define QML_NAMED_ELEMENT(NAME) \
    Q_CLASSINFO("QML.Element", #NAME)

#define QML_VALUE_TYPE(NAME) \
    Q_CLASSINFO("QML.Element", #NAME) \
    QML_UNCREATABLE("Value types cannot be created.")

#define QML_UNCREATABLE(REASON) \
    Q_CLASSINFO("QML.Creatable", "false") \
    Q_CLASSINFO("QML.UncreatableReason", REASON)

#define QML_SINGLETON \
    Q_CLASSINFO("QML.Singleton", "true") \
    enum class QmlIsSingleton {yes = true}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlSingleton; \
    template<typename T, typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *);

#define QML_SEQUENTIAL_CONTAINER(VALUE_TYPE) \
    Q_CLASSINFO("QML.Sequence", #VALUE_TYPE) \
    using QmlSequenceValueType = VALUE_TYPE; \
    enum class QmlIsSequence {yes = true}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlSequence; \
    template<typename T, typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *);

#define QML_ADDED_IN_MINOR_VERSION(VERSION) \
    Q_CLASSINFO("QML.AddedInVersion", Q_REVISION(VERSION))

#define QML_ADDED_IN_VERSION(MAJOR, MINOR) \
    Q_CLASSINFO("QML.AddedInVersion", Q_REVISION(MAJOR, MINOR))

#define QML_REMOVED_IN_MINOR_VERSION(VERSION) \
    Q_CLASSINFO("QML.RemovedInVersion", Q_REVISION(VERSION))

#define QML_REMOVED_IN_VERSION(MAJOR, MINOR) \
    Q_CLASSINFO("QML.RemovedInVersion", Q_REVISION(MAJOR, MINOR))

#define QML_ATTACHED(ATTACHED_TYPE) \
    Q_CLASSINFO("QML.Attached", #ATTACHED_TYPE) \
    using QmlAttachedType = ATTACHED_TYPE; \
    template<class, class, bool> friend struct QML_PRIVATE_NAMESPACE::QmlAttached; \
    template<class> friend struct QML_PRIVATE_NAMESPACE::QmlAttachedAccessor;

#define QML_EXTENDED(EXTENDED_TYPE) \
    Q_CLASSINFO("QML.Extended", #EXTENDED_TYPE) \
    using QmlExtendedType = EXTENDED_TYPE; \
    template<class, class> friend struct QML_PRIVATE_NAMESPACE::QmlExtended; \
    template<typename T, typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *);

#define QML_EXTENDED_NAMESPACE(EXTENDED_NAMESPACE) \
    Q_CLASSINFO("QML.Extended", #EXTENDED_NAMESPACE) \
    static constexpr const QMetaObject *qmlExtendedNamespace() { return &EXTENDED_NAMESPACE::staticMetaObject; } \
    template<class, class> friend struct QML_PRIVATE_NAMESPACE::QmlExtendedNamespace; \
    template<typename T, typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *);

#define QML_FOREIGN(FOREIGN_TYPE) \
    Q_CLASSINFO("QML.Foreign", #FOREIGN_TYPE) \
    using QmlForeignType = FOREIGN_TYPE; \
    template<class, class> friend struct QML_PRIVATE_NAMESPACE::QmlResolved; \
    template<typename T, typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *);

#define QML_FOREIGN_NAMESPACE(FOREIGN_NAMESPACE) \
    Q_CLASSINFO("QML.Foreign", #FOREIGN_NAMESPACE)

#define QML_INTERFACE \
    Q_CLASSINFO("QML.Element", "anonymous") \
    enum class QmlIsInterface {yes = true}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlInterface; \
    template<typename T, typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *);

#define QML_IMPLEMENTS_INTERFACES(INTERFACES) \
    Q_INTERFACES(INTERFACES) \
    enum class QmlIsInterface {yes = false}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlInterface;

#define QML_UNAVAILABLE \
    QML_FOREIGN(QQmlTypeNotAvailable)

enum { /* TYPEINFO flags */
    QML_HAS_ATTACHED_PROPERTIES = 0x01
};

#define QML_DECLARE_TYPEINFO(TYPE, FLAGS) \
QT_BEGIN_NAMESPACE \
template <> \
class QQmlTypeInfo<TYPE > \
{ \
public: \
    enum { \
        hasAttachedProperties = (((FLAGS) & QML_HAS_ATTACHED_PROPERTIES) == QML_HAS_ATTACHED_PROPERTIES) \
    }; \
}; \
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

void Q_QML_EXPORT qmlClearTypeRegistrations();

template<class T>
QQmlCustomParser *qmlCreateCustomParser();

template<typename T>
int qmlRegisterAnonymousType(const char *uri, int versionMajor)
{
    QQmlPrivate::RegisterType type = {
        0,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        0,
        nullptr, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, 0), nullptr,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

int Q_QML_EXPORT qmlRegisterTypeNotAvailable(const char *uri, int versionMajor, int versionMinor,
                                             const char *qmlName, const QString& message);

template<typename T>
int qmlRegisterUncreatableType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason)
{
    QQmlPrivate::RegisterType type = {
        0,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        0,
        nullptr,
        nullptr,
        reason,
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, int metaObjectRevision>
int qmlRegisterUncreatableType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason)
{
    QQmlPrivate::RegisterType type = {
        0,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        0,
        nullptr,
        nullptr,
        reason,
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        QTypeRevision::fromMinorVersion(metaObjectRevision)
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, typename E>
int qmlRegisterExtendedUncreatableType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason)
{
    QQmlAttachedPropertiesFunc attached = QQmlPrivate::attachedPropertiesFunc<E>();
    const QMetaObject * attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<E>();
    if (!attached) {
        attached = QQmlPrivate::attachedPropertiesFunc<T>();
        attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<T>();
    }

    QQmlPrivate::RegisterType type = {
        0,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        0,
        nullptr,
        nullptr,
        reason,
        QQmlPrivate::ValueType<T, E>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        attached,
        attachedMetaObject,

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        QQmlPrivate::ExtendedType<E>::createParent, QQmlPrivate::ExtendedType<E>::staticMetaObject(),

        nullptr,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, typename E, int metaObjectRevision>
int qmlRegisterExtendedUncreatableType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason)
{
    QQmlAttachedPropertiesFunc attached = QQmlPrivate::attachedPropertiesFunc<E>();
    const QMetaObject * attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<E>();
    if (!attached) {
        attached = QQmlPrivate::attachedPropertiesFunc<T>();
        attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<T>();
    }

    QQmlPrivate::RegisterType type = {
        0,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        0,
        nullptr,
        nullptr,
        reason,
        QQmlPrivate::ValueType<T, E>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        attached,
        attachedMetaObject,

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        QQmlPrivate::ExtendedType<E>::createParent, QQmlPrivate::ExtendedType<E>::staticMetaObject(),

        nullptr,
        QTypeRevision::fromMinorVersion(metaObjectRevision)
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

Q_QML_EXPORT int qmlRegisterUncreatableMetaObject(const QMetaObject &staticMetaObject, const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason);

template<typename T>
int qmlRegisterType(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    QQmlPrivate::RegisterType type = {
        0,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, int metaObjectRevision>
int qmlRegisterType(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    QQmlPrivate::RegisterType type = {
        0,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        QTypeRevision::fromMinorVersion(metaObjectRevision)
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, int metaObjectRevision>
int qmlRegisterRevision(const char *uri, int versionMajor, int versionMinor)
{
    QQmlPrivate::RegisterType type = {
        0,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), nullptr,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        QTypeRevision::fromMinorVersion(metaObjectRevision)
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, typename E>
int qmlRegisterExtendedType(const char *uri, int versionMajor)
{
    QQmlPrivate::RegisterType type = {
        0,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        0,
        nullptr,
        nullptr,
        QString(),
        QQmlPrivate::ValueType<T, E>::create,

        uri, QTypeRevision::fromVersion(versionMajor, 0), nullptr,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        QQmlPrivate::ExtendedType<E>::createParent, QQmlPrivate::ExtendedType<E>::staticMetaObject(),

        nullptr,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, typename E>
int qmlRegisterExtendedType(const char *uri, int versionMajor, int versionMinor,
                            const char *qmlName)
{
    QQmlAttachedPropertiesFunc attached = QQmlPrivate::attachedPropertiesFunc<E>();
    const QMetaObject * attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<E>();
    if (!attached) {
        attached = QQmlPrivate::attachedPropertiesFunc<T>();
        attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<T>();
    }

    QQmlPrivate::RegisterType type = {
        0,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, E>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        attached,
        attachedMetaObject,

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        QQmlPrivate::ExtendedType<E>::createParent, QQmlPrivate::ExtendedType<E>::staticMetaObject(),

        nullptr,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T>
int qmlRegisterInterface(const char *uri, int versionMajor)
{
    QQmlPrivate::RegisterInterface qmlInterface = {
        0,
        // An interface is not a QObject itself but is typically casted to one.
        // Therefore, we still want the pointer.
        QMetaType::fromType<T *>(),
        QMetaType::fromType<QQmlListProperty<T> >(),
        qobject_interface_iid<T *>(),

        uri,
        QTypeRevision::fromVersion(versionMajor, 0)
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::InterfaceRegistration, &qmlInterface);
}

template<typename T>
int qmlRegisterCustomType(const char *uri, int versionMajor, int versionMinor,
                          const char *qmlName, QQmlCustomParser *parser)
{
    QQmlPrivate::RegisterType type = {
        0,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        parser,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, int metaObjectRevision>
int qmlRegisterCustomType(const char *uri, int versionMajor, int versionMinor,
                          const char *qmlName, QQmlCustomParser *parser)
{
    QQmlPrivate::RegisterType type = {
        0,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, void>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        parser,
        QTypeRevision::fromMinorVersion(metaObjectRevision)
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T, typename E>
int qmlRegisterCustomExtendedType(const char *uri, int versionMajor, int versionMinor,
                          const char *qmlName, QQmlCustomParser *parser)
{
    QQmlAttachedPropertiesFunc attached = QQmlPrivate::attachedPropertiesFunc<E>();
    const QMetaObject * attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<E>();
    if (!attached) {
        attached = QQmlPrivate::attachedPropertiesFunc<T>();
        attachedMetaObject = QQmlPrivate::attachedPropertiesMetaObject<T>();
    }

    QQmlPrivate::RegisterType type = {
        0,
        QQmlPrivate::QmlMetaType<T>::self(),
        QQmlPrivate::QmlMetaType<T>::list(),
        sizeof(T), QQmlPrivate::Constructors<T>::createInto, nullptr,
        QString(),
        QQmlPrivate::ValueType<T, E>::create,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),

        attached,
        attachedMetaObject,

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        QQmlPrivate::ExtendedType<E>::createParent, QQmlPrivate::ExtendedType<E>::staticMetaObject(),

        parser,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

class QQmlContext;
class QQmlEngine;
class QJSValue;
class QJSEngine;

Q_QML_EXPORT void qmlExecuteDeferred(QObject *);
Q_QML_EXPORT QQmlContext *qmlContext(const QObject *);
Q_QML_EXPORT QQmlEngine *qmlEngine(const QObject *);
Q_QML_EXPORT QQmlAttachedPropertiesFunc qmlAttachedPropertiesFunction(QObject *,
                                                                      const QMetaObject *);
Q_QML_EXPORT QObject *qmlAttachedPropertiesObject(QObject *, QQmlAttachedPropertiesFunc func,
                                                  bool create = true);

//The C++ version of protected namespaces in qmldir
Q_QML_EXPORT bool qmlProtectModule(const char* uri, int majVersion);
Q_QML_EXPORT void qmlRegisterModule(const char *uri, int versionMajor, int versionMinor);

enum QQmlModuleImportSpecialVersions: int {
    QQmlModuleImportModuleAny = -1,
    QQmlModuleImportLatest = -1,
    QQmlModuleImportAuto = -2
};

Q_QML_EXPORT void qmlRegisterModuleImport(const char *uri, int moduleMajor,
                                          const char *import,
                                          int importMajor = QQmlModuleImportLatest,
                                          int importMinor = QQmlModuleImportLatest);
Q_QML_EXPORT void qmlUnregisterModuleImport(const char *uri, int moduleMajor,
                                            const char *import,
                                            int importMajor = QQmlModuleImportLatest,
                                            int importMinor = QQmlModuleImportLatest);

template<typename T>
QObject *qmlAttachedPropertiesObject(const QObject *obj, bool create = true)
{
    // We don't need a concrete object to resolve the function. As T is a C++ type, it and all its
    // super types should be registered as CppType (or not at all). We only need the object and its
    // QML engine to resolve composite types. Therefore, the function is actually a static property
    // of the C++ type system and we can cache it here for improved performance on further lookups.
    static const auto func = qmlAttachedPropertiesFunction(nullptr, &T::staticMetaObject);
    return qmlAttachedPropertiesObject(const_cast<QObject *>(obj), func, create);
}

inline int qmlRegisterSingletonType(const char *uri, int versionMajor, int versionMinor, const char *typeName,
                                QJSValue (*callback)(QQmlEngine *, QJSEngine *))
{
    QQmlPrivate::RegisterSingletonType api = {
        0,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), typeName,

        callback,
        nullptr, nullptr, QMetaType(),
        nullptr, nullptr,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::SingletonRegistration, &api);
}

template <typename T>
inline int qmlRegisterSingletonType(
        const char *uri, int versionMajor, int versionMinor,  const char *typeName,
        QObject *(*callback)(QQmlEngine *, QJSEngine *))
{
    QQmlPrivate::RegisterSingletonType api = {
        0,
        uri,
        QTypeRevision::fromVersion(versionMajor, versionMinor),
        typeName,
        nullptr,
        callback,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),
        QQmlPrivate::QmlMetaType<T>::self(),
        nullptr, nullptr,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::SingletonRegistration, &api);
}

#ifdef Q_QDOC
template <typename T>
int qmlRegisterSingletonType(const char *uri, int versionMajor, int versionMinor, const char *typeName, std::function<QObject*(QQmlEngine *, QJSEngine *)> callback)
#else
template <typename T, typename F, typename std::enable_if<std::is_convertible<F, std::function<QObject *(QQmlEngine *, QJSEngine *)>>::value
                                                 && !std::is_convertible<F, QObject *(*)(QQmlEngine *, QJSEngine *)>::value, void>::type* = nullptr>
inline int qmlRegisterSingletonType(const char *uri, int versionMajor, int versionMinor, const char *typeName,
                                    F&& callback)
#endif
{
    QQmlPrivate::RegisterSingletonType api = {
        0,
        uri,
        QTypeRevision::fromVersion(versionMajor, versionMinor),
        typeName,
        nullptr,
        callback,
        QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),
        QQmlPrivate::QmlMetaType<T>::self(),
        nullptr, nullptr,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::SingletonRegistration, &api);
}

#ifdef Q_QDOC
int qmlRegisterSingletonInstance(const char *uri, int versionMajor, int versionMinor, const char *typeName, QObject *cppObject)
#else
template<typename T>
inline auto qmlRegisterSingletonInstance(const char *uri, int versionMajor, int versionMinor,
                                         const char *typeName, T *cppObject) -> typename std::enable_if<std::is_base_of<QObject, T>::value, int>::type
#endif
{
    QQmlPrivate::SingletonFunctor registrationFunctor;
    registrationFunctor.m_object = cppObject;
    return qmlRegisterSingletonType<T>(uri, versionMajor, versionMinor, typeName, registrationFunctor);
}

inline int qmlRegisterSingletonType(const QUrl &url, const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    if (url.isRelative()) {
        // User input check must go here, because QQmlPrivate::qmlregister is also used internally for composite types
        qWarning("qmlRegisterSingletonType requires absolute URLs.");
        return 0;
    }

    QQmlPrivate::RegisterCompositeSingletonType type = {
        0,
        url,
        uri,
        QTypeRevision::fromVersion(versionMajor, versionMinor),
        qmlName
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::CompositeSingletonRegistration, &type);
}

inline int qmlRegisterType(const QUrl &url, const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    if (url.isRelative()) {
        // User input check must go here, because QQmlPrivate::qmlregister is also used internally for composite types
        qWarning("qmlRegisterType requires absolute URLs.");
        return 0;
    }

    QQmlPrivate::RegisterCompositeType type = {
        0,
        url,
        uri,
        QTypeRevision::fromVersion(versionMajor, versionMinor),
        qmlName
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::CompositeRegistration, &type);
}

template<typename Container>
inline int qmlRegisterAnonymousSequentialContainer(const char *uri, int versionMajor)
{
    QQmlPrivate::RegisterSequentialContainer type = {
        0,
        uri,
        QTypeRevision::fromMajorVersion(versionMajor),
        nullptr,
        QMetaType::fromType<Container>(),
        QMetaSequence::fromContainer<Container>(),
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::SequentialContainerRegistration, &type);
}

template<class T, class Resolved, class Extended, bool Singleton, bool Interface, bool Sequence>
struct QmlTypeAndRevisionsRegistration;

template<class T, class Resolved, class Extended>
struct QmlTypeAndRevisionsRegistration<T, Resolved, Extended, false, false, false> {
    static void registerTypeAndRevisions(const char *uri, int versionMajor, QList<int> *qmlTypeIds,
                                         const QMetaObject *extension)
    {
        QQmlPrivate::qmlRegisterTypeAndRevisions<Resolved, Extended>(
                    uri, versionMajor, QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),
                    qmlTypeIds, extension);
    }
};

template<class T, class Resolved>
struct QmlTypeAndRevisionsRegistration<T, Resolved, void, false, false, true> {
    static void registerTypeAndRevisions(const char *uri, int versionMajor, QList<int> *qmlTypeIds,
                                         const QMetaObject *)
    {
        QQmlPrivate::qmlRegisterSequenceAndRevisions<Resolved>(
                    uri, versionMajor, QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),
                    qmlTypeIds);
    }
};

template<class T, class Resolved, class Extended>
struct QmlTypeAndRevisionsRegistration<T, Resolved, Extended, true, false, false> {
    static void registerTypeAndRevisions(const char *uri, int versionMajor, QList<int> *qmlTypeIds,
                                         const QMetaObject *extension)
    {
        QQmlPrivate::qmlRegisterSingletonAndRevisions<Resolved, Extended, T>(
                    uri, versionMajor, QQmlPrivate::StaticMetaObject<T>::staticMetaObject(),
                    qmlTypeIds, extension);
    }
};

template<class T, class Resolved>
struct QmlTypeAndRevisionsRegistration<T, Resolved, void, false, true, false> {
    static void registerTypeAndRevisions(const char *uri, int versionMajor, QList<int> *qmlTypeIds,
                                         const QMetaObject *)
    {
        const int id = qmlRegisterInterface<Resolved>(uri, versionMajor);
        if (qmlTypeIds)
            qmlTypeIds->append(id);
    }
};

template<typename T = void, typename... Args>
void qmlRegisterTypesAndRevisions(const char *uri, int versionMajor,
                                  QList<int> *qmlTypeIds = nullptr);

template<typename T, typename... Args>
void qmlRegisterTypesAndRevisions(const char *uri, int versionMajor, QList<int> *qmlTypeIds)
{
    QmlTypeAndRevisionsRegistration<
            T, typename QQmlPrivate::QmlResolved<T>::Type,
            typename QQmlPrivate::QmlExtended<T>::Type,
            QQmlPrivate::QmlSingleton<T>::Value,
            QQmlPrivate::QmlInterface<T>::Value,
            QQmlPrivate::QmlSequence<T>::Value>
            ::registerTypeAndRevisions(uri, versionMajor, qmlTypeIds,
                                       QQmlPrivate::QmlExtendedNamespace<T>::metaObject());
    qmlRegisterTypesAndRevisions<Args...>(uri, versionMajor, qmlTypeIds);
}

template<>
inline void qmlRegisterTypesAndRevisions<>(const char *, int, QList<int> *)
{
}

inline void qmlRegisterNamespaceAndRevisions(const QMetaObject *metaObject,
                                             const char *uri, int versionMajor,
                                             QList<int> *qmlTypeIds = nullptr,
                                             const QMetaObject *classInfoMetaObject = nullptr)
{
    QQmlPrivate::RegisterTypeAndRevisions type = {
        0,
        QMetaType(),
        QMetaType(),
        0,
        nullptr,
        nullptr,
        nullptr,

        uri,
        QTypeRevision::fromMajorVersion(versionMajor),

        metaObject,
        (classInfoMetaObject ? classInfoMetaObject : metaObject),

        nullptr,
        nullptr,

        -1,
        -1,
        -1,

        nullptr,
        nullptr,

        &qmlCreateCustomParser<void>,
        qmlTypeIds
    };

    qmlregister(QQmlPrivate::TypeAndRevisionsRegistration, &type);
}

int Q_QML_EXPORT qmlTypeId(const char *uri, int versionMajor, int versionMinor, const char *qmlName);

QT_END_NAMESPACE

#endif // QQML_H
