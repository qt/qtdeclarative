/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qqml.h"

#include <QtQml/qqmlprivate.h>

#include <private/qqmlengine_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmlmetatypedata_p.h>
#include <private/qqmltype_p_p.h>
#include <private/qqmltypemodule_p_p.h>
#include <private/qqmltypenotavailable_p.h>

#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

void qmlClearTypeRegistrations() // Declared in qqml.h
{
    QQmlMetaType::clearTypeRegistrations();
    QQmlEnginePrivate::baseModulesUninitialized = true; //So the engine re-registers its types
    qmlClearEnginePlugins();
}

//From qqml.h
bool qmlProtectModule(const char *uri, int majVersion)
{
    return QQmlMetaType::protectModule(QString::fromUtf8(uri), majVersion);
}

//From qqml.h
void qmlRegisterModule(const char *uri, int versionMajor, int versionMinor)
{
    QQmlMetaType::registerModule(uri, versionMajor, versionMinor);
}

//From qqml.h
int qmlTypeId(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    return QQmlMetaType::typeId(uri, versionMajor, versionMinor, qmlName);
}

// From qqmlprivate.h
QObject *QQmlPrivate::RegisterSingletonFunctor::operator()(QQmlEngine *qeng, QJSEngine *)
{
    if (!m_object) {
        QQmlError error;
        error.setDescription(QLatin1String("The registered singleton has already been deleted. Ensure that it outlives the engine."));
        QQmlEnginePrivate::get(qeng)->warning(qeng, error);
        return nullptr;
    }

    if (qeng->thread() != m_object->thread()) {
        QQmlError error;
        error.setDescription(QLatin1String("Registered object must live in the same thread as the engine it was registered with"));
        QQmlEnginePrivate::get(qeng)->warning(qeng, error);
        return nullptr;
    }
    if (alreadyCalled) {
        QQmlError error;
        error.setDescription(QLatin1String("Singleton registered by registerSingletonInstance must only be accessed from one engine"));
        QQmlEnginePrivate::get(qeng)->warning(qeng, error);
        return nullptr;
    }
    alreadyCalled = true;
    qeng->setObjectOwnership(m_object, QQmlEngine::CppOwnership);
    return m_object;
};

static QVector<int> availableRevisions(const QMetaObject *metaObject)
{
    QVector<int> revisions;
    if (!metaObject)
        return revisions;
    const int propertyOffset = metaObject->propertyOffset();
    const int propertyCount = metaObject->propertyCount();
    for (int propertyIndex = propertyOffset, propertyEnd = propertyOffset + propertyCount;
         propertyIndex < propertyEnd; ++propertyIndex) {
        const QMetaProperty property = metaObject->property(propertyIndex);
        if (int revision = property.revision())
            revisions.append(revision);
    }
    const int methodOffset = metaObject->methodOffset();
    const int methodCount = metaObject->methodCount();
    for (int methodIndex = methodOffset, methodEnd = methodOffset + methodCount;
         methodIndex < methodEnd; ++methodIndex) {
        const QMetaMethod method = metaObject->method(methodIndex);
        if (int revision = method.revision())
            revisions.append(revision);
    }

    // Need to also check parent meta objects, as their revisions are inherited.
    if (const QMetaObject *superMeta = metaObject->superClass())
        revisions += availableRevisions(superMeta);

    return revisions;
}

/*
This method is "over generalized" to allow us to (potentially) register more types of things in
the future without adding exported symbols.
*/
int QQmlPrivate::qmlregister(RegistrationType type, void *data)
{
    QQmlType dtype;
    switch (type) {
    case AutoParentRegistration:
        return QQmlMetaType::registerAutoParentFunction(
                *reinterpret_cast<RegisterAutoParent *>(data));
    case QmlUnitCacheHookRegistration:
        return QQmlMetaType::registerUnitCacheHook(
                *reinterpret_cast<RegisterQmlUnitCacheHook *>(data));
    case TypeAndRevisionsRegistration: {
        const RegisterTypeAndRevisions &type = *reinterpret_cast<RegisterTypeAndRevisions *>(data);
        const char *elementName = classElementName(type.classInfoMetaObject);
        const bool creatable = (elementName != nullptr)
                && boolClassInfo(type.classInfoMetaObject, "QML.Creatable", true);

        const QString noCreateReason = creatable
                ? QString()
                : QString::fromUtf8(classInfo(type.classInfoMetaObject, "QML.UncreatableReason"));
        RegisterType revisionRegistration = {
            1,
            type.typeId,
            type.listId,
            creatable ? type.objectSize : 0,
            nullptr,
            noCreateReason,
            type.uri,
            type.versionMajor,
            -1,
            nullptr,
            type.metaObject,
            type.attachedPropertiesFunction,
            type.attachedPropertiesMetaObject,
            type.parserStatusCast,
            type.valueSourceCast,
            type.valueInterceptorCast,
            type.extensionObjectCreate,
            type.extensionMetaObject,
            nullptr,
            -1
        };

        const int added = intClassInfo(type.classInfoMetaObject, "QML.AddedInMinorVersion", 0);
        const int removed = intClassInfo(type.classInfoMetaObject, "QML.RemovedInMinorVersion", -1);

        auto revisions = availableRevisions(type.metaObject);
        revisions.append(qMax(added, 0));
        if (type.attachedPropertiesMetaObject)
            revisions += availableRevisions(type.attachedPropertiesMetaObject);

        std::sort(revisions.begin(), revisions.end());
        const auto it = std::unique(revisions.begin(), revisions.end());
        revisions.erase(it, revisions.end());

        const bool typeWasRemoved = removed >= added;
        for (int revision : revisions) {
            if (revision < added)
                continue;

            // When removed, we still add revisions, but anonymous ones
            if (typeWasRemoved && revision >= removed) {
                revisionRegistration.elementName = nullptr;
                revisionRegistration.create = nullptr;
            } else {
                revisionRegistration.elementName = elementName;
                revisionRegistration.create = creatable ? type.create : nullptr;
            }

            // Equivalent of qmlRegisterRevision<T, revision>(...)
            revisionRegistration.versionMinor = revision;
            revisionRegistration.revision = revision;
            revisionRegistration.customParser = type.customParserFactory();

            qmlregister(TypeRegistration, &revisionRegistration);
        }
        break;
    }
    case SingletonAndRevisionsRegistration: {
        const RegisterSingletonTypeAndRevisions &type
                = *reinterpret_cast<RegisterSingletonTypeAndRevisions *>(data);
        const char *elementName = classElementName(type.classInfoMetaObject);
        RegisterSingletonType revisionRegistration = {
            QmlCurrentSingletonTypeRegistrationVersion,
            type.uri,
            type.versionMajor,
            -1,
            elementName,

            type.scriptApi,
            nullptr,
            type.instanceMetaObject,
            type.typeId,
            -1,

            type.generalizedQobjectApi
        };

        const int added = intClassInfo(type.classInfoMetaObject, "QML.AddedInMinorVersion", 0);
        const int removed = intClassInfo(type.classInfoMetaObject, "QML.RemovedInMinorVersion", -1);

        auto revisions = availableRevisions(type.instanceMetaObject);
        revisions.append(qMax(added, 0));

        std::sort(revisions.begin(), revisions.end());
        const auto it = std::unique(revisions.begin(), revisions.end());
        revisions.erase(it, revisions.end());

        const bool typeWasRemoved = removed >= added;
        for (int revision : qAsConst(revisions)) {
            if (revision < added)
                continue;

            // When removed, we still add revisions, but anonymous ones
            if (typeWasRemoved && revision >= removed) {
                revisionRegistration.typeName = nullptr;
                revisionRegistration.scriptApi = nullptr;
                revisionRegistration.generalizedQobjectApi = nullptr;
            } else {
                revisionRegistration.typeName = elementName;
                revisionRegistration.scriptApi = type.scriptApi;
                revisionRegistration.generalizedQobjectApi = type.generalizedQobjectApi;
            }

            // Equivalent of qmlRegisterRevision<T, revision>(...)
            revisionRegistration.versionMinor = revision;
            revisionRegistration.revision = revision;

            qmlregister(SingletonRegistration, &revisionRegistration);
        }
        break;
    }
    case TypeRegistration:
        dtype = QQmlMetaType::registerType(*reinterpret_cast<RegisterType *>(data));
        break;
    case InterfaceRegistration:
        dtype = QQmlMetaType::registerInterface(*reinterpret_cast<RegisterInterface *>(data));
        break;
    case SingletonRegistration:
        dtype = QQmlMetaType::registerSingletonType(*reinterpret_cast<RegisterSingletonType *>(data));
        break;
    case CompositeRegistration:
        dtype = QQmlMetaType::registerCompositeType(*reinterpret_cast<RegisterCompositeType *>(data));
        break;
    case CompositeSingletonRegistration:
        dtype = QQmlMetaType::registerCompositeSingletonType(*reinterpret_cast<RegisterCompositeSingletonType *>(data));
        break;
    default:
        return -1;
    }

    if (!dtype.isValid())
        return -1;

    QQmlMetaType::registerUndeletableType(dtype);
    return dtype.index();
}

void QQmlPrivate::qmlunregister(RegistrationType type, quintptr data)
{
    switch (type) {
    case AutoParentRegistration:
        QQmlMetaType::unregisterAutoParentFunction(reinterpret_cast<AutoParentFunction>(data));
        break;
    case QmlUnitCacheHookRegistration:
        QQmlMetaType::removeCachedUnitLookupFunction(
                reinterpret_cast<QmlUnitCacheLookupFunction>(data));
        break;
    case TypeRegistration:
    case InterfaceRegistration:
    case SingletonRegistration:
    case CompositeRegistration:
    case CompositeSingletonRegistration:
        QQmlMetaType::unregisterType(data);
        break;
    case TypeAndRevisionsRegistration:
    case SingletonAndRevisionsRegistration:
        // Currently unnecessary. We'd need a special data structure to hold
        // URI + majorVersion and then we'd iterate the minor versions, look up the
        // associated QQmlType objects by uri/elementName/major/minor and qmlunregister
        // each of them.
        Q_UNREACHABLE();
        break;
    }
}

namespace QQmlPrivate {
    template<>
    void qmlRegisterTypeAndRevisions<QQmlTypeNotAvailable, void>(
            const char *uri, int versionMajor, const QMetaObject *classInfoMetaObject)
    {
        using T = QQmlTypeNotAvailable;

        QML_GETTYPENAMES

        RegisterTypeAndRevisions type = {
            0,
            qRegisterNormalizedMetaType<T *>(pointerName.constData()),
            qRegisterNormalizedMetaType<QQmlListProperty<T> >(listName.constData()),
            0,
            nullptr,

            uri,
            versionMajor,

            &QQmlTypeNotAvailable::staticMetaObject,
            classInfoMetaObject,

            attachedPropertiesFunc<T>(),
            attachedPropertiesMetaObject<T>(),

            StaticCastSelector<T, QQmlParserStatus>::cast(),
            StaticCastSelector<T, QQmlPropertyValueSource>::cast(),
            StaticCastSelector<T, QQmlPropertyValueInterceptor>::cast(),

            nullptr, nullptr, qmlCreateCustomParser<T>
        };

        qmlregister(TypeAndRevisionsRegistration, &type);
    }
}

QT_END_NAMESPACE
