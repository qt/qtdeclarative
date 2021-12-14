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

#include <private/qjsvalue_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmlmetatypedata_p.h>
#include <private/qqmltype_p_p.h>
#include <private/qqmltypemodule_p.h>
#include <private/qqmltypenotavailable_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/qqmlvaluetypewrapper_p.h>
#include <private/qv4lookup_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4identifiertable_p.h>
#include <private/qv4errorobject_p.h>

#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE


/*!
   \internal

   This method completes the setup of all deferred properties of \a object.
   Deferred properties are declared with
   Q_CLASSINFO("DeferredPropertyNames", "comma,separated,property,list");

   Any binding to a deferred property is not executed when the object is instantiated,
   but only when completion is requested with qmlExecuteDeferred, or by manually
   calling QQmlComponentPrivate::beginDeferred and completeDeferred.

   \sa QV4::CompiledData::Binding::IsDeferredBinding,
       QV4::CompiledData::Object::HasDeferredBindings,
       QQmlData::deferData,
       QQmlObjectCreator::setupBindings
*/
void qmlExecuteDeferred(QObject *object)
{
    QQmlData *data = QQmlData::get(object);

    if (data && !data->deferredData.isEmpty() && !data->wasDeleted(object)) {
        QQmlEnginePrivate *ep = QQmlEnginePrivate::get(data->context->engine());

        QQmlComponentPrivate::DeferredState state;
        QQmlComponentPrivate::beginDeferred(ep, object, &state);

        // Release the reference for the deferral action (we still have one from construction)
        data->releaseDeferredData();

        QQmlComponentPrivate::completeDeferred(ep, &state);
    }
}

QQmlContext *qmlContext(const QObject *obj)
{
    return QQmlEngine::contextForObject(obj);
}

QQmlEngine *qmlEngine(const QObject *obj)
{
    QQmlData *data = QQmlData::get(obj, false);
    if (!data || !data->context)
        return nullptr;
    return data->context->engine();
}

static QObject *resolveAttachedProperties(QQmlAttachedPropertiesFunc pf, QQmlData *data,
                                          QObject *object, bool create)
{
    if (!pf)
        return nullptr;

    QObject *rv = data->hasExtendedData() ? data->attachedProperties()->value(pf) : 0;
    if (rv || !create)
        return rv;

    rv = pf(object);

    if (rv)
        data->attachedProperties()->insert(pf, rv);

    return rv;
}

QQmlAttachedPropertiesFunc qmlAttachedPropertiesFunction(QObject *object,
                                                         const QMetaObject *attachedMetaObject)
{
    QQmlEngine *engine = object ? qmlEngine(object) : nullptr;
    return QQmlMetaType::attachedPropertiesFunc(engine ? QQmlEnginePrivate::get(engine) : nullptr,
                                                attachedMetaObject);
}

QObject *qmlAttachedPropertiesObject(QObject *object, QQmlAttachedPropertiesFunc func, bool create)
{
    if (!object)
        return nullptr;

    QQmlData *data = QQmlData::get(object, create);

    // Attached properties are only on objects created by QML,
    // unless explicitly requested (create==true)
    if (!data)
        return nullptr;

    return resolveAttachedProperties(func, data, object, create);
}

int qmlRegisterUncreatableMetaObject(const QMetaObject &staticMetaObject,
                                     const char *uri, int versionMajor,
                                     int versionMinor, const char *qmlName,
                                     const QString& reason)
{
    QQmlPrivate::RegisterType type = {
        0,
        QMetaType(),
        QMetaType(),
        0,
        nullptr,
        nullptr,
        reason,
        nullptr,

        uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName, &staticMetaObject,

        QQmlAttachedPropertiesFunc(),
        nullptr,

        0,
        0,
        0,

        nullptr, nullptr,

        nullptr,
        QTypeRevision::zero()
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

void qmlClearTypeRegistrations() // Declared in qqml.h
{
    QQmlMetaType::clearTypeRegistrations();
    QQmlEnginePrivate::baseModulesUninitialized = true; //So the engine re-registers its types
    qmlClearEnginePlugins();
}

//From qqml.h
bool qmlProtectModule(const char *uri, int majVersion)
{
    return QQmlMetaType::protectModule(QString::fromUtf8(uri),
                                       QTypeRevision::fromMajorVersion(majVersion));
}

//From qqml.h
void qmlRegisterModule(const char *uri, int versionMajor, int versionMinor)
{
    QQmlMetaType::registerModule(uri, QTypeRevision::fromVersion(versionMajor, versionMinor));
}

static QQmlDirParser::Import resolveImport(const QString &uri, int importMajor, int importMinor)
{
    if (importMajor == QQmlModuleImportAuto)
        return QQmlDirParser::Import(uri, QTypeRevision(), QQmlDirParser::Import::Auto);
    else if (importMajor == QQmlModuleImportLatest)
        return QQmlDirParser::Import(uri, QTypeRevision(), QQmlDirParser::Import::Default);
    else if (importMinor == QQmlModuleImportLatest)
        return QQmlDirParser::Import(uri, QTypeRevision::fromMajorVersion(importMajor), QQmlDirParser::Import::Default);
    return QQmlDirParser::Import(uri, QTypeRevision::fromVersion(importMajor, importMinor), QQmlDirParser::Import::Default);
}

static QTypeRevision resolveModuleVersion(int moduleMajor)
{
    return moduleMajor == QQmlModuleImportModuleAny
            ? QTypeRevision()
            : QTypeRevision::fromMajorVersion(moduleMajor);
}

/*!
 * \enum QQmlModuleImportSpecialVersions
 *
 * Defines some special values that can be passed to the version arguments of
 * qmlRegisterModuleImport() and qmlUnregisterModuleImport().
 *
 * \value QQmlModuleImportModuleAny When passed as majorVersion of the base
 *                                  module, signifies that the import is to be
 *                                  applied to any version of the module.
 * \value QQmlModuleImportLatest    When passed as major or minor version of
 *                                  the imported module, signifies that the
 *                                  latest overall, or latest minor version
 *                                  of a specified major version shall be
 *                                  imported.
 * \value QQmlModuleImportAuto      When passed as major version of the imported
 *                                  module, signifies that the version of the
 *                                  base module shall be forwarded.
 */

/*!
 * Registers an implicit import for module \a uri of major version \a majorVersion.
 *
 * This has the same effect as an \c import statement in a qmldir file: Whenever
 * \a uri of version \a moduleMajor is imported, \a import of version
 * \a importMajor. \a importMinor is automatically imported, too. If
 * \a importMajor is \l QmlModuleImportLatest the latest version
 * available of that module is imported, and \a importMinor does not matter. If
 * \a importMinor is \l QmlModuleImportLatest the latest minor version of a
 * \a importMajor is chosen. If \a importMajor is \l QmlModuleImportAuto the
 * version of \a import is version of \a uri being imported, and \a importMinor
 * does not matter. If \a moduleMajor is \a QmlModuleImportModuleAny the module
 * import is applied for any major version of \a uri. For example, you may
 * specify that whenever any version of MyModule is imported, the latest version
 * of MyOtherModule should be imported. Then, the following call would be
 * appropriate:
 *
 * \code
 * qmlRegisterModuleImport("MyModule", QmlModuleImportModuleAny,
 *                         "MyOtherModule", QmlModuleImportLatest);
 * \endcode
 *
 * Or, you may specify that whenever major version 5 of "MyModule" is imported,
 * then version 3.14 of "MyOtherModule" should be imported:
 *
 * \code
 * qmlRegisterModuleImport("MyModule", 5, "MyOtherModule", 3, 14);
 * \endcode
 *
 * Finally, if you always want the same version of "MyOtherModule" to be
 * imported whenever "MyModule" is imported, specify the following:
 *
 * \code
 * qmlRegisterModuleImport("MyModule", QmlModuleImportModuleAny,
 *                         "MyOtherModule", QmlModuleImportAuto);
 * \endcode
 *
 * \sa qmlUnregisterModuleImport()
 */
void qmlRegisterModuleImport(const char *uri, int moduleMajor,
                             const char *import, int importMajor, int importMinor)
{
    QQmlMetaType::registerModuleImport(
                QString::fromUtf8(uri), resolveModuleVersion(moduleMajor),
                resolveImport(QString::fromUtf8(import), importMajor, importMinor));
}


/*!
 * Removes a module import previously registered with qmlRegisterModuleImport()
 *
 * Calling this function makes sure that \a import of version
 * \a{importMajor}.\a{importMinor} is not automatically imported anymore when
 * \a uri of version \a moduleMajor is. The version resolution works the same
 * way as with \l qmlRegisterModuleImport().
 *
 * \sa qmlRegisterModuleImport()
 */
void qmlUnregisterModuleImport(const char *uri, int moduleMajor,
                               const char *import, int importMajor, int importMinor)
{
    QQmlMetaType::unregisterModuleImport(
                QString::fromUtf8(uri), resolveModuleVersion(moduleMajor),
                resolveImport(QString::fromUtf8(import), importMajor, importMinor));
}

//From qqml.h
int qmlTypeId(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    return QQmlMetaType::typeId(uri, QTypeRevision::fromVersion(versionMajor, versionMinor), qmlName);
}

// From qqmlprivate.h
QObject *QQmlPrivate::SingletonFunctor::operator()(QQmlEngine *qeng, QJSEngine *)
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

static QVector<QTypeRevision> availableRevisions(const QMetaObject *metaObject)
{
    QVector<QTypeRevision> revisions;
    if (!metaObject)
        return revisions;
    const int propertyOffset = metaObject->propertyOffset();
    const int propertyCount = metaObject->propertyCount();
    for (int propertyIndex = propertyOffset, propertyEnd = propertyOffset + propertyCount;
         propertyIndex < propertyEnd; ++propertyIndex) {
        const QMetaProperty property = metaObject->property(propertyIndex);
        if (int revision = property.revision())
            revisions.append(QTypeRevision::fromEncodedVersion(revision));
    }
    const int methodOffset = metaObject->methodOffset();
    const int methodCount = metaObject->methodCount();
    for (int methodIndex = methodOffset, methodEnd = methodOffset + methodCount;
         methodIndex < methodEnd; ++methodIndex) {
        const QMetaMethod method = metaObject->method(methodIndex);
        if (int revision = method.revision())
            revisions.append(QTypeRevision::fromEncodedVersion(revision));
    }

    // Need to also check parent meta objects, as their revisions are inherited.
    if (const QMetaObject *superMeta = metaObject->superClass())
        revisions += availableRevisions(superMeta);

    return revisions;
}

template<typename Registration>
void assignVersions(Registration *registration, QTypeRevision revision,
                    QTypeRevision defaultVersion)
{
    const quint8 majorVersion = revision.hasMajorVersion() ? revision.majorVersion()
                                                           : defaultVersion.majorVersion();
    registration->version = revision.hasMinorVersion()
            ? QTypeRevision::fromVersion(majorVersion, revision.minorVersion())
            : QTypeRevision::fromMajorVersion(majorVersion);
    registration->revision = revision;
}

static QVector<QTypeRevision> prepareRevisions(const QMetaObject *metaObject, QTypeRevision added)
{
    auto revisions = availableRevisions(metaObject);
    revisions.append(added);
    return revisions;
}

static void uniqueRevisions(QVector<QTypeRevision> *revisions, QTypeRevision defaultVersion,
                            QTypeRevision added)
{
    bool revisionsHaveMajorVersions = false;
    for (QTypeRevision revision : QVector<QTypeRevision>(*revisions)) { // yes, copy
        // allow any minor version for each explicitly specified past major one
        if (revision.hasMajorVersion()) {
            revisionsHaveMajorVersions = true;
            if (revision.majorVersion() < defaultVersion.majorVersion())
                revisions->append(QTypeRevision::fromVersion(revision.majorVersion(), 254));
        }
    }

    if (revisionsHaveMajorVersions) {
        if (!added.hasMajorVersion()) {
            // If added in unspecified major version, assume default one.
            revisions->append(QTypeRevision::fromVersion(defaultVersion.majorVersion(),
                                                         added.minorVersion()));
        } else if (added.majorVersion() < defaultVersion.majorVersion()) {
            // If added in past major version, add .0 of default version.
            revisions->append(QTypeRevision::fromVersion(defaultVersion.majorVersion(), 0));
        }
    }

    std::sort(revisions->begin(), revisions->end());
    const auto it = std::unique(revisions->begin(), revisions->end());
    revisions->erase(it, revisions->end());
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

        QString noCreateReason;

        if (!creatable) {
            noCreateReason = QString::fromUtf8(classInfo(type.classInfoMetaObject, "QML.UncreatableReason"));
            if (noCreateReason.isEmpty())
                noCreateReason = QLatin1String("Type cannot be created in QML.");
        }

        RegisterType revisionRegistration = {
            0,
            type.typeId,
            type.listId,
            creatable ? type.objectSize : 0,
            nullptr,
            nullptr,
            noCreateReason,
            type.createValueType,
            type.uri,
            type.version,
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
            QTypeRevision()
        };

        const QTypeRevision added = revisionClassInfo(
                    type.classInfoMetaObject, "QML.AddedInVersion",
                    QTypeRevision::fromMinorVersion(0));
        const QTypeRevision removed = revisionClassInfo(
                    type.classInfoMetaObject, "QML.RemovedInVersion");
        const QList<QTypeRevision> furtherRevisions = revisionClassInfos(type.classInfoMetaObject,
                                                                        "QML.ExtraVersion");

        auto revisions = prepareRevisions(type.metaObject, added) + furtherRevisions;
        if (type.attachedPropertiesMetaObject)
            revisions += availableRevisions(type.attachedPropertiesMetaObject);
        uniqueRevisions(&revisions, type.version, added);

        for (QTypeRevision revision : revisions) {
            if (revision < added)
                continue;
            if (revision.hasMajorVersion() && revision.majorVersion() > type.version.majorVersion())
                break;

            // When removed, we still add revisions, but anonymous ones
            if (removed.isValid() && !(revision < removed)) {
                revisionRegistration.elementName = nullptr;
                revisionRegistration.create = nullptr;
            } else {
                revisionRegistration.elementName = elementName;
                revisionRegistration.create = creatable ? type.create : nullptr;
                revisionRegistration.userdata = type.userdata;
            }

            assignVersions(&revisionRegistration, revision, type.version);
            revisionRegistration.customParser = type.customParserFactory();
            const int id = qmlregister(TypeRegistration, &revisionRegistration);
            if (type.qmlTypeIds)
                type.qmlTypeIds->append(id);
        }
        break;
    }
    case SingletonAndRevisionsRegistration: {
        const RegisterSingletonTypeAndRevisions &type
                = *reinterpret_cast<RegisterSingletonTypeAndRevisions *>(data);
        const char *elementName = classElementName(type.classInfoMetaObject);
        RegisterSingletonType revisionRegistration = {
            0,
            type.uri,
            type.version,
            elementName,
            nullptr,
            type.qObjectApi,
            type.instanceMetaObject,
            type.typeId,
            type.extensionObjectCreate,
            type.extensionMetaObject,
            QTypeRevision()
        };

        const QTypeRevision added = revisionClassInfo(
                    type.classInfoMetaObject, "QML.AddedInVersion",
                    QTypeRevision::fromMinorVersion(0));
        const QTypeRevision removed = revisionClassInfo(
                    type.classInfoMetaObject, "QML.RemovedInVersion");
        const QList<QTypeRevision> furtherRevisions = revisionClassInfos(type.classInfoMetaObject,
                                                                        "QML.ExtraVersion");

        auto revisions = prepareRevisions(type.instanceMetaObject, added) + furtherRevisions;
        uniqueRevisions(&revisions, type.version, added);

        for (QTypeRevision revision : qAsConst(revisions)) {
            if (revision < added)
                continue;
            if (revision.hasMajorVersion() && revision.majorVersion() > type.version.majorVersion())
                break;

            // When removed, we still add revisions, but anonymous ones
            if (removed.isValid() && !(revision < removed)) {
                revisionRegistration.typeName = nullptr;
                revisionRegistration.qObjectApi = nullptr;
            } else {
                revisionRegistration.typeName = elementName;
                revisionRegistration.qObjectApi = type.qObjectApi;
            }

            assignVersions(&revisionRegistration, revision, type.version);
            const int id = qmlregister(SingletonRegistration, &revisionRegistration);
            if (type.qmlTypeIds)
                type.qmlTypeIds->append(id);
        }
        break;
    }
    case SequentialContainerAndRevisionsRegistration: {
        const RegisterSequentialContainerAndRevisions &type
                = *reinterpret_cast<RegisterSequentialContainerAndRevisions *>(data);
        const char *elementName = classElementName(type.classInfoMetaObject);
        RegisterSequentialContainer revisionRegistration = {
            0,
            type.uri,
            type.version,
            elementName,
            type.typeId,
            type.metaSequence,
            QTypeRevision()
        };

        const QTypeRevision added = revisionClassInfo(
                    type.classInfoMetaObject, "QML.AddedInVersion",
                    QTypeRevision::fromMinorVersion(0));
        const QTypeRevision removed = revisionClassInfo(
                    type.classInfoMetaObject, "QML.RemovedInVersion");
        QList<QTypeRevision> revisions = revisionClassInfos(type.classInfoMetaObject,
                                                            "QML.ExtraVersion");
        revisions.append(added);
        uniqueRevisions(&revisions, type.version, added);

        for (QTypeRevision revision : qAsConst(revisions)) {
            if (revision < added)
                continue;
            if (revision.hasMajorVersion() && revision.majorVersion() > type.version.majorVersion())
                break;

            // When removed, we still add revisions, but anonymous ones
            if (removed.isValid() && !(revision < removed))
                revisionRegistration.typeName = nullptr;
            else
                revisionRegistration.typeName = elementName;

            assignVersions(&revisionRegistration, revision, type.version);
            const int id = qmlregister(SequentialContainerRegistration, &revisionRegistration);
            if (type.qmlTypeIds)
                type.qmlTypeIds->append(id);
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
    case SequentialContainerRegistration:
        dtype = QQmlMetaType::registerSequentialContainer(*reinterpret_cast<RegisterSequentialContainer *>(data));
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
    case SequentialContainerRegistration:
        QQmlMetaType::unregisterSequentialContainer(data);
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
    case SequentialContainerAndRevisionsRegistration:
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
        const char *uri, int versionMajor, const QMetaObject *classInfoMetaObject,
        QVector<int> *qmlTypeIds, const QMetaObject *extension)
{
    using T = QQmlTypeNotAvailable;

    RegisterTypeAndRevisions type = {
        0,
        QMetaType::fromType<T *>(),
        QMetaType::fromType<QQmlListProperty<T>>(),
        0,
        nullptr,
        nullptr,
        nullptr,

        uri,
        QTypeRevision::fromMajorVersion(versionMajor),

        &QQmlTypeNotAvailable::staticMetaObject,
        classInfoMetaObject,

        attachedPropertiesFunc<T>(),
        attachedPropertiesMetaObject<T>(),

        StaticCastSelector<T, QQmlParserStatus>::cast(),
        StaticCastSelector<T, QQmlPropertyValueSource>::cast(),
        StaticCastSelector<T, QQmlPropertyValueInterceptor>::cast(),

        nullptr, extension, qmlCreateCustomParser<T>, qmlTypeIds
    };

    qmlregister(TypeAndRevisionsRegistration, &type);
}


QQmlEngine *AOTCompiledContext::qmlEngine() const
{
    return qmlContext ? qmlContext->engine() : nullptr;
}

QJSValue AOTCompiledContext::jsMetaType(int index) const
{
    return QJSValuePrivate::fromReturnedValue(
                compilationUnit->runtimeClasses[index]->asReturnedValue());
}

void AOTCompiledContext::setInstructionPointer(int offset) const
{
    if (auto *frame = engine->handle()->currentStackFrame)
        frame->instructionPointer = offset;
}

void AOTCompiledContext::setReturnValueUndefined() const
{
    if (auto *frame = engine->handle()->currentStackFrame) {
        Q_ASSERT(frame->isMetaTypesFrame());
        static_cast<QV4::MetaTypesStackFrame *>(frame)->setReturnValueUndefined();
    }
}

static void captureObjectProperty(
        QObject *object, const QQmlPropertyCache *propertyCache,
        const QQmlPropertyData *property, QQmlContextData *qmlContext)
{
    if (!qmlContext || property->isConstant())
        return;

    QQmlEngine *engine = qmlContext->engine();
    Q_ASSERT(engine);
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
    Q_ASSERT(ep);
    if (QQmlPropertyCapture *capture = ep->propertyCapture)
        capture->captureProperty(object, propertyCache, property);
}

static bool inherits(const QQmlPropertyCache *descendent, const QQmlPropertyCache *ancestor)
{
    for (const QQmlPropertyCache *cache = descendent; cache; cache = cache->parent()) {
        if (cache == ancestor)
            return true;
    }
    return false;
}

enum class ObjectPropertyResult { OK, NeedsInit, Deleted };

static ObjectPropertyResult loadObjectProperty(QV4::Lookup *l, QObject *object, void *target,
                               QQmlContextData *qmlContext)
{
    QQmlData *qmlData = QQmlData::get(object);
    if (!qmlData)
        return ObjectPropertyResult::NeedsInit;
    if (qmlData->isQueuedForDeletion)
        return ObjectPropertyResult::Deleted;
    Q_ASSERT(!QQmlData::wasDeleted(object));
    const QQmlPropertyCache *propertyCache = l->qobjectLookup.propertyCache;
    if (!inherits(qmlData->propertyCache, propertyCache))
        return ObjectPropertyResult::NeedsInit;
    const QQmlPropertyData *property = l->qobjectLookup.propertyData;

    const int coreIndex = property->coreIndex();
    if (qmlData->hasPendingBindingBit(coreIndex))
        qmlData->flushPendingBinding(coreIndex);

    captureObjectProperty(object, propertyCache, property, qmlContext);
    property->readProperty(object, target);
    return ObjectPropertyResult::OK;
}

static ObjectPropertyResult storeObjectProperty(QV4::Lookup *l, QObject *object, void *value)
{
    const QQmlData *qmlData = QQmlData::get(object);
    if (!qmlData)
        return ObjectPropertyResult::NeedsInit;
    if (qmlData->isQueuedForDeletion)
        return ObjectPropertyResult::Deleted;
    Q_ASSERT(!QQmlData::wasDeleted(object));
    if (!inherits(qmlData->propertyCache, l->qobjectLookup.propertyCache))
        return ObjectPropertyResult::NeedsInit;
    const QQmlPropertyData *property = l->qobjectLookup.propertyData;
    QQmlPropertyPrivate::removeBinding(object, QQmlPropertyIndex(property->coreIndex()));
    property->writeProperty(object, value, {});
    return ObjectPropertyResult::OK;
}

static bool initObjectLookup(
        const AOTCompiledContext *aotContext, QV4::Lookup *l, QObject *object, QMetaType type)
{
    QV4::Scope scope(aotContext->engine->handle());
    QV4::PropertyKey id = scope.engine->identifierTable->asPropertyKey(
                aotContext->compilationUnit->runtimeStrings[l->nameIndex]);

    Q_ASSERT(id.isString());

    QV4::ScopedString name(scope, id.asStringOrSymbol());

    Q_ASSERT(!name->equals(scope.engine->id_toString()));
    Q_ASSERT(!name->equals(scope.engine->id_destroy()));

    QQmlData *ddata = QQmlData::get(object, true);
    Q_ASSERT(ddata);
    if (ddata->isQueuedForDeletion)
        return false;

    QQmlPropertyData *property;
    if (!ddata->propertyCache) {
        property = QQmlPropertyCache::property(
                    aotContext->engine, object, name, aotContext->qmlContext, nullptr);
    } else {
        property = ddata->propertyCache->property(
                    name.getPointer(), object, aotContext->qmlContext);
    }

    if (!property)
        return false;

    const QMetaType propType = property->propType();
    if (!type.isValid()) {
        // If type is invalid, then the calling code depends on the lookup
        // to be set up in order to query the type, via lookupResultMetaType.
        // We cannot verify the type in this case.
    } else if ((type.flags() & QMetaType::IsQmlList) && (propType.flags() & QMetaType::IsQmlList)) {
        // We want to check the value types here, but we cannot easily do it.
        // Internally those are all QObject* lists, though.
    } else if (type.flags() & QMetaType::PointerToQObject) {
        // We accept any base class as type, too

        const QMetaObject *typeMetaObject = type.metaObject();
        const QMetaObject *foundMetaObject = propType.metaObject();
        if (!foundMetaObject) {
            if (QQmlEngine *engine = aotContext->qmlEngine()) {
                foundMetaObject = QQmlEnginePrivate::get(engine)->metaObjectForType(
                            propType.id()).metaObject();
            }
        }

        while (foundMetaObject) {
            if (foundMetaObject == typeMetaObject)
                break;
            foundMetaObject = foundMetaObject->superClass();
        }

        if (!foundMetaObject)
            return false;
    } else if (propType != type) {
        return false;
    }

    Q_ASSERT(ddata->propertyCache);

    QV4::setupQObjectLookup(l, ddata, property);
    return true;
}

static bool initValueLookup(QV4::Lookup *l, QV4::ExecutableCompilationUnit *compilationUnit,
                            const QMetaObject *metaObject, QMetaType type)
{
    Q_ASSERT(metaObject);
    const QByteArray name = compilationUnit->runtimeStrings[l->nameIndex]->toQString().toUtf8();
    const int coreIndex = metaObject->indexOfProperty(name.constData());
    QMetaType lookupType = metaObject->property(coreIndex).metaType();
    if (type.isValid() && lookupType != type)
        return false;
    l->qgadgetLookup.metaObject = quintptr(metaObject) + 1;
    l->qgadgetLookup.coreIndex = coreIndex;
    l->qgadgetLookup.metaType = lookupType.iface();
    return true;
}

static void amendException(QV4::ExecutionEngine *engine)
{
    const int lineNumber = engine->currentStackFrame->lineNumber();
    engine->exceptionStackTrace.front().line = lineNumber;

    QV4::Scope scope(engine);
    QV4::Scoped<QV4::ErrorObject> error(scope, *engine->exceptionValue);
    if (error) // else some other value was thrown
        error->d()->stackTrace->front().line = lineNumber;
}


bool AOTCompiledContext::captureLookup(uint index, QObject *object) const
{
    if (!object)
        return false;

    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->getter != QV4::QQmlTypeWrapper::lookupSingletonProperty
            && l->getter != QV4::Lookup::getterQObject) {
        return false;
    }

    const QQmlPropertyData *property = l->qobjectLookup.propertyData;
    QQmlData::flushPendingBinding(object, property->coreIndex());
    captureObjectProperty(object, l->qobjectLookup.propertyCache, property, qmlContext);
    return true;
}

bool AOTCompiledContext::captureQmlContextPropertyLookup(uint index) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->qmlContextPropertyGetter != QV4::QQmlContextWrapper::lookupScopeObjectProperty
            && l->qmlContextPropertyGetter != QV4::QQmlContextWrapper::lookupContextObjectProperty) {
        return false;
    }

    const QQmlPropertyData *property = l->qobjectLookup.propertyData;
    QQmlData::flushPendingBinding(qmlScopeObject, property->coreIndex());
    captureObjectProperty(qmlScopeObject, l->qobjectLookup.propertyCache, property, qmlContext);
    return true;
}

QMetaType AOTCompiledContext::lookupResultMetaType(uint index) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupScopeObjectProperty
            || l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupContextObjectProperty
            || l->getter == QV4::QQmlTypeWrapper::lookupSingletonProperty
            || l->getter == QV4::Lookup::getterQObject
            || l->setter == QV4::Lookup::setterQObject) {
        return l->qobjectLookup.propertyData->propType();
    } else if (l->getter == QV4::QQmlValueTypeWrapper::lookupGetter) {
        return QMetaType(l->qgadgetLookup.metaType);
    } else if (l->getter == QV4::QQmlTypeWrapper::lookupEnumValue) {
        return QMetaType::fromType<int>();
    } else if (l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupIdObject
               || l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupType
               || l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupSingleton
               || l->getter == QV4::QObjectWrapper::lookupAttached) {
        return QMetaType::fromType<QObject *>();
    }
    return QMetaType();
}

void AOTCompiledContext::storeNameSloppy(uint nameIndex, void *value, QMetaType type) const
{
    // We don't really use any part of the lookup machinery here.
    // The QV4::Lookup is created on the stack to conveniently get the property cache, and through
    // the property cache we store a value into the property.

    QV4::Lookup l;
    l.clear();
    l.nameIndex = nameIndex;
    if (initObjectLookup(this, &l, qmlScopeObject, QMetaType())) {

        ObjectPropertyResult storeResult;
        const QMetaType propType = l.qobjectLookup.propertyData->propType();
        if (type == propType) {
            storeResult = storeObjectProperty(&l, qmlScopeObject, value);
        } else {
            QVariant var(propType);
            propType.convert(type, value, propType, var.data());
            storeResult = storeObjectProperty(&l, qmlScopeObject, var.data());
        }

        switch (storeResult) {
        case ObjectPropertyResult::NeedsInit:
            engine->handle()->throwTypeError();
            break;
        case ObjectPropertyResult::Deleted:
            engine->handle()->throwTypeError(
                        QStringLiteral("Value is null and could not be converted to an object"));
            break;
        case ObjectPropertyResult::OK:
            break;
        }
        l.qobjectLookup.propertyCache->release();
    } else {
        engine->handle()->throwTypeError();
    }
}

bool AOTCompiledContext::callQmlContextPropertyLookup(
        uint index, void **args, const QMetaType *types, int argc) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue thisObject(scope);
    QV4::ScopedFunctionObject function(
                scope, l->qmlContextPropertyGetter(l, scope.engine, thisObject));
    if (!function) {
        scope.engine->throwTypeError(
                    QStringLiteral("Property '%1' of object [null] is not a function").arg(
                        compilationUnit->runtimeStrings[l->nameIndex]->toQString()));
        return false;
    }

    function->call(thisObject, args, types, argc);
    return !scope.engine->hasException;
}

void AOTCompiledContext::initCallQmlContextPropertyLookup(uint index) const
{
    Q_UNUSED(index);
    Q_ASSERT(engine->hasError());
    amendException(engine->handle());
}

bool AOTCompiledContext::loadContextIdLookup(uint index, void *target) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    int objectId = -1;
    QQmlContextData *context = nullptr;
    Q_ASSERT(qmlContext);

    if (l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupIdObject) {
        objectId = l->qmlContextIdObjectLookup.objectId;
        context = qmlContext;
    } else if (l->qmlContextPropertyGetter
               == QV4::QQmlContextWrapper::lookupIdObjectInParentContext) {
        QV4::Scope scope(engine->handle());
        QV4::ScopedString name(scope, compilationUnit->runtimeStrings[l->nameIndex]);
        for (context = qmlContext; context; context = context->parent().data()) {
            objectId = context->propertyIndex(name);
            if (objectId != -1 && objectId < context->numIdValues())
                break;
        }
    } else {
        return false;
    }

    Q_ASSERT(objectId >= 0);
    Q_ASSERT(context != nullptr);
    QQmlEnginePrivate *engine = QQmlEnginePrivate::get(qmlEngine());
    if (QQmlPropertyCapture *capture = engine->propertyCapture)
        capture->captureProperty(context->idValueBindings(objectId));
    *static_cast<QObject **>(target) = context->idValue(objectId);
    return true;
}

void AOTCompiledContext::initLoadContextIdLookup(uint index) const
{
    Q_ASSERT(!engine->hasError());
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedString name(scope, compilationUnit->runtimeStrings[l->nameIndex]);
    const QQmlRefPointer<QQmlContextData> ownContext = qmlContext;
    for (auto context = ownContext; context; context = context->parent()) {
        const int propertyIdx = context->propertyIndex(name);
        if (propertyIdx == -1 || propertyIdx >= context->numIdValues())
            continue;

        if (context.data() == ownContext.data()) {
            l->qmlContextIdObjectLookup.objectId = propertyIdx;
            l->qmlContextPropertyGetter = QV4::QQmlContextWrapper::lookupIdObject;
        } else {
            l->qmlContextPropertyGetter = QV4::QQmlContextWrapper::lookupIdObjectInParentContext;
        }

        return;
    }

    Q_UNREACHABLE();
}

bool AOTCompiledContext::callObjectPropertyLookup(
        uint index, QObject *object, void **args, const QMetaType *types, int argc) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue thisObject(scope, QV4::QObjectWrapper::wrap(scope.engine, object));
    QV4::ScopedFunctionObject function(scope, l->getter(l, engine->handle(), thisObject));
    if (!function) {
        scope.engine->throwTypeError(
                    QStringLiteral("Property '%1' of object [object Object] is not a function")
                    .arg(compilationUnit->runtimeStrings[l->nameIndex]->toQString()));
        return false;
    }

    function->call(thisObject, args, types, argc);
    return !scope.engine->hasException;
}

void AOTCompiledContext::initCallObjectPropertyLookup(uint index) const
{
    Q_UNUSED(index);
    Q_ASSERT(engine->hasError());
    amendException(engine->handle());
}

bool AOTCompiledContext::callGlobalLookup(
        uint index, void **args, const QMetaType *types, int argc) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedFunctionObject function(scope, l->globalGetter(l, scope.engine));
    if (!function) {
        scope.engine->throwTypeError(
                    QStringLiteral("Property '%1' of object [null] is not a function")
                    .arg(compilationUnit->runtimeStrings[l->nameIndex]->toQString()));
        return false;
    }

    QV4::ScopedValue thisObject(scope, QV4::Encode::undefined());
    function->call(thisObject, args, types, argc);
    return true;
}

void AOTCompiledContext::initCallGlobalLookup(uint index) const
{
    Q_UNUSED(index);
    Q_ASSERT(engine->hasError());
    amendException(engine->handle());
}

bool AOTCompiledContext::loadGlobalLookup(uint index, void *target, QMetaType type) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::ExecutionEngine *v4 = engine->handle();
    if (!v4->metaTypeFromJS(l->globalGetter(l, engine->handle()), type, target)) {
        v4->throwTypeError();
        return false;
    }
    return true;
}

void AOTCompiledContext::initLoadGlobalLookup(uint index) const
{
    Q_UNUSED(index);
    Q_ASSERT(engine->hasError());
    amendException(engine->handle());
}

bool AOTCompiledContext::loadScopeObjectPropertyLookup(uint index, void *target) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->qmlContextPropertyGetter != QV4::QQmlContextWrapper::lookupScopeObjectProperty)
        return false;

    switch (loadObjectProperty(l, qmlScopeObject, target, qmlContext)) {
    case ObjectPropertyResult::NeedsInit:
        return false;
    case ObjectPropertyResult::Deleted:
        engine->handle()->throwTypeError(
                    QStringLiteral("Cannot read property '%1' of null")
                    .arg(compilationUnit->runtimeStrings[l->nameIndex]->toQString()));
        return false;
    case ObjectPropertyResult::OK:
        return true;
    }

    Q_UNREACHABLE();
    return false;
}

void AOTCompiledContext::initLoadScopeObjectPropertyLookup(uint index, QMetaType type) const
{
    QV4::ExecutionEngine *v4 = engine->handle();
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;

    if (v4->hasException)
        amendException(v4);
    else if (initObjectLookup(this, l, qmlScopeObject, type))
        l->qmlContextPropertyGetter = QV4::QQmlContextWrapper::lookupScopeObjectProperty;
    else
        v4->throwTypeError();
}

bool AOTCompiledContext::loadTypeLookup(uint index, void *target) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());

    if (l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupType) {
        // We resolve it right away. An AOT compiler, in contrast to a naive interpreter
        // should only request objects it actually needs.
        QV4::Scoped<QV4::QQmlTypeWrapper> wrapper(scope, l->qmlTypeLookup.qmlTypeWrapper);
        Q_ASSERT(wrapper);
        *static_cast<QObject **>(target) = wrapper->object();
        return true;
    }

    if (l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupSingleton) {
        QV4::Scoped<QV4::QQmlTypeWrapper> wrapper(
                    scope, l->qmlContextSingletonLookup.singletonObject);

        // We don't handle non-QObject singletons (as those can't be declared in qmltypes anyway)
        Q_ASSERT(wrapper);
        *static_cast<QObject **>(target) = wrapper->object();
        return true;
    }

    return false;
}

void AOTCompiledContext::initLoadTypeLookup(uint index) const
{
    Q_ASSERT(!engine->hasError());
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    l->qmlContextPropertyGetter(l, engine->handle(), nullptr);

    // Singleton instances can be retrieved via either lookupType or lookupSingleton
    // and both use QQmlTypeWrapper to store them.
    // TODO: Wat? Looking up the singleton instances on each access is horribly inefficient.
    //       There is plenty of space in the lookup to store the instances.

    Q_ASSERT(l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupType
             || l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupSingleton);
}

bool AOTCompiledContext::loadAttachedLookup(uint index, QObject *object, void *target) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->getter != QV4::QObjectWrapper::lookupAttached)
        return false;

    QV4::Scope scope(engine->handle());
    QV4::Scoped<QV4::QQmlTypeWrapper> wrapper(scope, l->qmlTypeLookup.qmlTypeWrapper);
    Q_ASSERT(wrapper);
    *static_cast<QObject **>(target) = qmlAttachedPropertiesObject(
                object, wrapper->d()->type().attachedPropertiesFunction(
                    QQmlEnginePrivate::get(qmlEngine())));
    return true;
}

void AOTCompiledContext::initLoadAttachedLookup(uint index, QObject *object) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedString name(scope, compilationUnit->runtimeStrings[l->nameIndex]);
    QQmlTypeNameCache::Result r = qmlContext->imports()->query<QQmlImport::AllowRecursion>(name);

    if (!r.isValid() || !r.type.isValid()) {
        scope.engine->throwTypeError();
        return;
    }

    QV4::Scoped<QV4::QQmlTypeWrapper> wrapper(
                scope, QV4::QQmlTypeWrapper::create(scope.engine, object, r.type,
                                                    QV4::Heap::QQmlTypeWrapper::ExcludeEnums));

    l->qmlTypeLookup.qmlTypeWrapper = wrapper->d();
    l->getter = QV4::QObjectWrapper::lookupAttached;
}

bool AOTCompiledContext::getObjectLookup(uint index, QObject *object, void *target) const
{

    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    const auto doThrow = [&]() {
        engine->handle()->throwTypeError(
                    QStringLiteral("Cannot read property '%1' of null")
                    .arg(compilationUnit->runtimeStrings[l->nameIndex]->toQString()));
        return false;
    };

    if (!object)
        return doThrow();

    if (l->getter != QV4::Lookup::getterQObject)
        return false;

    switch (loadObjectProperty(l, object, target, qmlContext)) {
    case ObjectPropertyResult::Deleted:
        return doThrow();
    case ObjectPropertyResult::NeedsInit:
        return false;
    case ObjectPropertyResult::OK:
        return true;
    }

    Q_UNREACHABLE();
    return false;
}

void AOTCompiledContext::initGetObjectLookup(uint index, QObject *object, QMetaType type) const
{
    QV4::ExecutionEngine *v4 = engine->handle();
    if (v4->hasException) {
        amendException(v4);
    } else {
        QV4::Lookup *l = compilationUnit->runtimeLookups + index;
        if (initObjectLookup(this, l, object, type))
            l->getter = QV4::Lookup::getterQObject;
        else
            engine->handle()->throwTypeError();
    }
}

bool AOTCompiledContext::getValueLookup(uint index, void *value, void *target) const
{
    Q_ASSERT(value);

    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->getter != QV4::QQmlValueTypeWrapper::lookupGetter)
        return false;

    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(l->qgadgetLookup.metaObject - 1);
    Q_ASSERT(metaObject);

    void *args[] = { target, nullptr };
    metaObject->d.static_metacall(
                reinterpret_cast<QObject*>(value), QMetaObject::ReadProperty,
                l->qgadgetLookup.coreIndex, args);
    return true;
}

void AOTCompiledContext::initGetValueLookup(
        uint index, const QMetaObject *metaObject, QMetaType type) const
{
    Q_ASSERT(!engine->hasError());
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (initValueLookup(l, compilationUnit, metaObject, type))
        l->getter = QV4::QQmlValueTypeWrapper::lookupGetter;
    else
        engine->handle()->throwTypeError();
}

bool AOTCompiledContext::getEnumLookup(uint index, int *target) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->getter != QV4::QQmlTypeWrapper::lookupEnumValue)
        return false;
    *target = l->qmlEnumValueLookup.encodedEnumValue;
    return true;
}

void AOTCompiledContext::initGetEnumLookup(
        uint index, const QMetaObject *metaObject,
        const char *enumerator, const char *enumValue) const
{
    Q_ASSERT(!engine->hasError());
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    Q_ASSERT(metaObject);
    const int enumIndex = metaObject->indexOfEnumerator(enumerator);
    const int value = metaObject->enumerator(enumIndex).keyToValue(enumValue);
    l->qmlEnumValueLookup.encodedEnumValue = value;
    l->getter = QV4::QQmlTypeWrapper::lookupEnumValue;
}

bool AOTCompiledContext::setObjectLookup(uint index, QObject *object, void *value) const
{
    const auto doThrow = [&]() {
        engine->handle()->throwTypeError(
                    QStringLiteral("Value is null and could not be converted to an object"));
        return false;
    };

    if (!object)
        return doThrow();

    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->setter != QV4::Lookup::setterQObject)
        return false;

    switch (storeObjectProperty(l, object, value)) {
    case ObjectPropertyResult::Deleted:
        return doThrow();
    case ObjectPropertyResult::NeedsInit:
        return false;
    case ObjectPropertyResult::OK:
        return true;
    }

    Q_UNREACHABLE();
    return false;
}

void AOTCompiledContext::initSetObjectLookup(uint index, QObject *object, QMetaType type) const
{
    QV4::ExecutionEngine *v4 = engine->handle();
    if (v4->hasException) {
        amendException(v4);
    } else {
        QV4::Lookup *l = compilationUnit->runtimeLookups + index;
        if (initObjectLookup(this, l, object, type))
            l->setter = QV4::Lookup::setterQObject;
        else
            engine->handle()->throwTypeError();
    }
}

bool AOTCompiledContext::setValueLookup(
        uint index, void *target, void *value) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->setter != QV4::QQmlValueTypeWrapper::lookupSetter)
        return false;

    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(l->qgadgetLookup.metaObject - 1);

    void *args[] = { value, nullptr };
    metaObject->d.static_metacall(
                reinterpret_cast<QObject*>(target), QMetaObject::WriteProperty,
                l->qgadgetLookup.coreIndex, args);
    return true;
}

void AOTCompiledContext::initSetValueLookup(uint index, const QMetaObject *metaObject,
                                            QMetaType type) const
{
    Q_ASSERT(!engine->hasError());
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (initValueLookup(l, compilationUnit, metaObject, type))
        l->setter = QV4::QQmlValueTypeWrapper::lookupSetter;
    else
        engine->handle()->throwTypeError();
}

} // namespace QQmlPrivate

QT_END_NAMESPACE
