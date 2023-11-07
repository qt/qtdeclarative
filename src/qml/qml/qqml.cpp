// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqml.h"

#include <QtQml/qqmlprivate.h>

#include <private/qjsvalue_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmlmetatypedata_p.h>
#include <private/qqmltype_p_p.h>
#include <private/qqmltypemodule_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/qqmlvaluetypewrapper_p.h>
#include <private/qv4lookup_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4identifiertable_p.h>
#include <private/qv4errorobject_p.h>
#include <private/qqmlbuiltinfunctions_p.h>
#include <private/qqmlfinalizer_p.h>
#include <private/qqmlloggingcategory_p.h>

#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQml);
Q_DECLARE_LOGGING_CATEGORY(lcJs);

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

    if (!data
            || !data->context
            || !data->context->engine()
            || data->deferredData.isEmpty()
            || data->wasDeleted(object)) {
        return;
    }

    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(data->context->engine());

    QQmlComponentPrivate::DeferredState state;
    QQmlComponentPrivate::beginDeferred(ep, object, &state);

    // Release the reference for the deferral action (we still have one from construction)
    data->releaseDeferredData();

    QQmlComponentPrivate::completeDeferred(ep, &state);
}

QQmlContext *qmlContext(const QObject *obj)
{
    return QQmlEngine::contextForObject(obj);
}

QQmlEngine *qmlEngine(const QObject *obj)
{
    QQmlData *data = QQmlData::get(obj);
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

QObject *qmlExtendedObject(QObject *object)
{
    return QQmlPrivate::qmlExtendedObject(object, 0);
}

QObject *QQmlPrivate::qmlExtendedObject(QObject *object, int index)
{
    if (!object)
        return nullptr;

    void *result = nullptr;
    QObjectPrivate *d = QObjectPrivate::get(object);
    if (!d->metaObject)
        return nullptr;

    const int id = d->metaObject->metaCall(
                object, QMetaObject::CustomCall,
                QQmlProxyMetaObject::extensionObjectId(index), &result);
    if (id != QQmlProxyMetaObject::extensionObjectId(index))
        return nullptr;

    return static_cast<QObject *>(result);
}

void QQmlPrivate::qmlRegistrationWarning(
        QQmlPrivate::QmlRegistrationWarning warning, QMetaType metaType)
{
    switch (warning) {
    case UnconstructibleType:
        qWarning()
                << metaType.name()
                << "is neither a QObject, nor default- and copy-constructible, nor uncreatable."
                << "You should not use it as a QML type.";
        break;
    case UnconstructibleSingleton:
        qWarning()
                << "Singleton" << metaType.name()
                << "needs either a default constructor or, when adding a default"
                << "constructor is infeasible, a public static"
                << "create(QQmlEngine *, QJSEngine *) method.";
        break;
    case NonQObjectWithAtached:
        qWarning()
                << metaType.name()
                << "is not a QObject, but has attached properties. This won't work.";
        break;
    }
}

int qmlRegisterUncreatableMetaObject(const QMetaObject &staticMetaObject,
                                     const char *uri, int versionMajor,
                                     int versionMinor, const char *qmlName,
                                     const QString& reason)
{
    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
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

        -1,
        -1,
        -1,

        nullptr, nullptr,

        nullptr,
        QTypeRevision::zero(),
        -1,
        QQmlPrivate::ValueTypeCreationMethod::None
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
 * \relates QQmlEngine
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
 * \relates QQmlEngine
 * Registers a qmldir-import for module \a uri of major version \a moduleMajor.
 *
 * This has the same effect as an \c import statement in a qmldir file: Whenever
 * \a uri of version \a moduleMajor is imported, \a import of version
 * \a importMajor. \a importMinor is automatically imported, too. If
 * \a importMajor is \l QQmlModuleImportLatest the latest version
 * available of that module is imported, and \a importMinor does not matter. If
 * \a importMinor is \l QQmlModuleImportLatest the latest minor version of a
 * \a importMajor is chosen. If \a importMajor is \l QQmlModuleImportAuto the
 * version of \a import is version of \a uri being imported, and \a importMinor
 * does not matter. If \a moduleMajor is \l QQmlModuleImportModuleAny the module
 * import is applied for any major version of \a uri. For example, you may
 * specify that whenever any version of MyModule is imported, the latest version
 * of MyOtherModule should be imported. Then, the following call would be
 * appropriate:
 *
 * \code
 * qmlRegisterModuleImport("MyModule", QQmlModuleImportModuleAny,
 *                         "MyOtherModule", QQmlModuleImportLatest);
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
 * qmlRegisterModuleImport("MyModule", QQmlModuleImportModuleAny,
 *                         "MyOtherModule", QQmlModuleImportAuto);
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
 * \relates QQmlEngine
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

static bool checkSingletonInstance(QQmlEngine *engine, QObject *instance)
{
    if (!instance) {
        QQmlError error;
        error.setDescription(QStringLiteral("The registered singleton has already been deleted. "
                                            "Ensure that it outlives the engine."));
        QQmlEnginePrivate::get(engine)->warning(engine, error);
        return false;
    }

    if (engine->thread() != instance->thread()) {
        QQmlError error;
        error.setDescription(QStringLiteral("Registered object must live in the same thread "
                                            "as the engine it was registered with"));
        QQmlEnginePrivate::get(engine)->warning(engine, error);
        return false;
    }

    return true;
}

// From qqmlprivate.h
#if QT_DEPRECATED_SINCE(6, 3)
QObject *QQmlPrivate::SingletonFunctor::operator()(QQmlEngine *qeng, QJSEngine *)
{
    if (!checkSingletonInstance(qeng, m_object))
        return nullptr;

    if (alreadyCalled) {
        QQmlError error;
        error.setDescription(QStringLiteral("Singleton registered by registerSingletonInstance "
                                            "must only be accessed from one engine"));
        QQmlEnginePrivate::get(qeng)->warning(qeng, error);
        return nullptr;
    }

    alreadyCalled = true;
    QJSEngine::setObjectOwnership(m_object, QQmlEngine::CppOwnership);
    return m_object;
};
#endif

QObject *QQmlPrivate::SingletonInstanceFunctor::operator()(QQmlEngine *qeng, QJSEngine *)
{
    if (!checkSingletonInstance(qeng, m_object))
        return nullptr;

    if (!m_engine) {
        m_engine = qeng;
        QJSEngine::setObjectOwnership(m_object, QQmlEngine::CppOwnership);
    } else if (m_engine != qeng) {
        QQmlError error;
        error.setDescription(QLatin1String("Singleton registered by registerSingletonInstance must only be accessed from one engine"));
        QQmlEnginePrivate::get(qeng)->warning(qeng, error);
        return nullptr;
    }

    return m_object;
};

static QVector<QTypeRevision> availableRevisions(const QMetaObject *metaObject)
{
    QVector<QTypeRevision> revisions;
    if (!metaObject)
        return revisions;
    const int propertyOffset = metaObject->propertyOffset();
    const int propertyCount = metaObject->propertyCount();
    for (int coreIndex = propertyOffset, propertyEnd = propertyOffset + propertyCount;
         coreIndex < propertyEnd; ++coreIndex) {
        const QMetaProperty property = metaObject->property(coreIndex);
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
        const char *elementName = (type.structVersion > 1 && type.forceAnonymous)
                ? nullptr
                : classElementName(type.classInfoMetaObject);
        const bool isValueType = !(type.typeId.flags() & QMetaType::PointerToQObject);
        const bool creatable = (elementName != nullptr || isValueType)
                && boolClassInfo(type.classInfoMetaObject, "QML.Creatable", true);

        QString noCreateReason;
        ValueTypeCreationMethod creationMethod = ValueTypeCreationMethod::None;

        if (!creatable) {
            noCreateReason = QString::fromUtf8(
                        classInfo(type.classInfoMetaObject, "QML.UncreatableReason"));
            if (noCreateReason.isEmpty())
                noCreateReason = QLatin1String("Type cannot be created in QML.");
        } else if (isValueType) {
            const char *method = classInfo(type.classInfoMetaObject, "QML.CreationMethod");
            if (qstrcmp(method, "structured") == 0)
                creationMethod = ValueTypeCreationMethod::Structured;
            else if (qstrcmp(method, "construct") == 0)
                creationMethod = ValueTypeCreationMethod::Construct;
        }

        RegisterType typeRevision = {
            QQmlPrivate::RegisterType::CurrentVersion,
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
            QTypeRevision(),
            type.structVersion > 0 ? type.finalizerCast : -1,
            creationMethod
        };

        QQmlPrivate::RegisterSequentialContainer sequenceRevision = {
            0,
            type.uri,
            type.version,
            nullptr,
            type.listId,
            type.structVersion > 1 ? type.listMetaSequence : QMetaSequence(),
            QTypeRevision(),
        };

        const QTypeRevision added = revisionClassInfo(
                    type.classInfoMetaObject, "QML.AddedInVersion",
                    QTypeRevision::fromVersion(type.version.majorVersion(), 0));
        const QTypeRevision removed = revisionClassInfo(
                    type.classInfoMetaObject, "QML.RemovedInVersion");
        const QList<QTypeRevision> furtherRevisions = revisionClassInfos(type.classInfoMetaObject,
                                                                        "QML.ExtraVersion");

        auto revisions = prepareRevisions(type.metaObject, added) + furtherRevisions;
        if (type.attachedPropertiesMetaObject)
            revisions += availableRevisions(type.attachedPropertiesMetaObject);
        uniqueRevisions(&revisions, type.version, added);

        for (QTypeRevision revision : revisions) {
            if (revision.hasMajorVersion() && revision.majorVersion() > type.version.majorVersion())
                break;

            assignVersions(&typeRevision, revision, type.version);

            // When removed or before added, we still add revisions, but anonymous ones
            if (typeRevision.version < added
                    || (removed.isValid() && !(typeRevision.version < removed))) {
                typeRevision.elementName = nullptr;
                typeRevision.create = nullptr;
                typeRevision.userdata = nullptr;
            } else {
                typeRevision.elementName = elementName;
                typeRevision.create = creatable ? type.create : nullptr;
                typeRevision.userdata = type.userdata;
            }

            typeRevision.customParser = type.customParserFactory();
            const int id = qmlregister(TypeRegistration, &typeRevision);
            if (type.qmlTypeIds)
                type.qmlTypeIds->append(id);

            if (sequenceRevision.metaSequence != QMetaSequence()) {
                sequenceRevision.version = typeRevision.version;
                sequenceRevision.revision = typeRevision.revision;
                const int id = QQmlPrivate::qmlregister(
                            QQmlPrivate::SequentialContainerRegistration, &sequenceRevision);
                if (type.qmlTypeIds)
                    type.qmlTypeIds->append(id);
            }
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
                    QTypeRevision::fromVersion(type.version.majorVersion(), 0));
        const QTypeRevision removed = revisionClassInfo(
                    type.classInfoMetaObject, "QML.RemovedInVersion");
        const QList<QTypeRevision> furtherRevisions = revisionClassInfos(type.classInfoMetaObject,
                                                                        "QML.ExtraVersion");

        auto revisions = prepareRevisions(type.instanceMetaObject, added) + furtherRevisions;
        uniqueRevisions(&revisions, type.version, added);

        for (QTypeRevision revision : std::as_const(revisions)) {
            if (revision.hasMajorVersion() && revision.majorVersion() > type.version.majorVersion())
                break;

            assignVersions(&revisionRegistration, revision, type.version);

            // When removed or before added, we still add revisions, but anonymous ones
            if (revisionRegistration.version < added
                    || (removed.isValid() && !(revisionRegistration.version < removed))) {
                revisionRegistration.typeName = nullptr;
                revisionRegistration.qObjectApi = nullptr;
            } else {
                revisionRegistration.typeName = elementName;
                revisionRegistration.qObjectApi = type.qObjectApi;
            }

            const int id = qmlregister(SingletonRegistration, &revisionRegistration);
            if (type.qmlTypeIds)
                type.qmlTypeIds->append(id);
        }
        break;
    }
    case SequentialContainerAndRevisionsRegistration: {
        const RegisterSequentialContainerAndRevisions &type
                = *reinterpret_cast<RegisterSequentialContainerAndRevisions *>(data);
        RegisterSequentialContainer revisionRegistration = {
            0,
            type.uri,
            type.version,
            nullptr,
            type.typeId,
            type.metaSequence,
            QTypeRevision()
        };

        const QTypeRevision added = revisionClassInfo(
                    type.classInfoMetaObject, "QML.AddedInVersion",
                    QTypeRevision::fromMinorVersion(0));
        QList<QTypeRevision> revisions = revisionClassInfos(
                    type.classInfoMetaObject, "QML.ExtraVersion");
        revisions.append(added);
        uniqueRevisions(&revisions, type.version, added);

        for (QTypeRevision revision : std::as_const(revisions)) {
            if (revision < added)
                continue;
            if (revision.hasMajorVersion() && revision.majorVersion() > type.version.majorVersion())
                break;

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

QList<QTypeRevision> QQmlPrivate::revisionClassInfos(const QMetaObject *metaObject,
                                                     const char *key)
{
    QList<QTypeRevision> revisions;
    for (int index = indexOfOwnClassInfo(metaObject, key); index != -1;
         index = indexOfOwnClassInfo(metaObject, key, index - 1)) {
        revisions.push_back(QTypeRevision::fromEncodedVersion(
                                QLatin1StringView(metaObject->classInfo(index).value()).toInt()));
    }
    return revisions;
}

int qmlRegisterTypeNotAvailable(
        const char *uri, int versionMajor, int versionMinor,
        const char *qmlName, const QString &message)
{
    return qmlRegisterUncreatableType<QQmlTypeNotAvailable>(
                uri, versionMajor, versionMinor, qmlName, message);
}

namespace QQmlPrivate {
template<>
void qmlRegisterTypeAndRevisions<QQmlTypeNotAvailable, void>(
        const char *uri, int versionMajor, const QMetaObject *classInfoMetaObject,
        QVector<int> *qmlTypeIds, const QMetaObject *extension, bool)
{
    using T = QQmlTypeNotAvailable;

    RegisterTypeAndRevisions type = {
        3,
        QmlMetaType<T>::self(),
        QmlMetaType<T>::list(),
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

        nullptr,
        extension,
        qmlCreateCustomParser<T>,
        qmlTypeIds,
        QQmlPrivate::StaticCastSelector<T, QQmlFinalizerHook>::cast(),
        false,
        QmlMetaType<T>::sequence(),
    };

    qmlregister(TypeAndRevisionsRegistration, &type);
}

QObject *AOTCompiledContext::thisObject() const
{
    return static_cast<QV4::MetaTypesStackFrame *>(engine->handle()->currentStackFrame)
            ->thisObject();
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

static QQmlPropertyCapture *propertyCapture(const QQmlContextData *qmlContext)
{
    if (!qmlContext)
        return nullptr;

    QQmlEngine *engine = qmlContext->engine();
    Q_ASSERT(engine);
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
    Q_ASSERT(ep);
    return ep->propertyCapture;
}

static void captureFallbackProperty(
        QObject *object, int coreIndex, int notifyIndex, bool isConstant,
        const QQmlContextData *qmlContext)
{
    if (isConstant)
        return;

    if (QQmlPropertyCapture *capture = propertyCapture(qmlContext))
        capture->captureProperty(object, coreIndex, notifyIndex);
}

static void captureObjectProperty(
        QObject *object, const QQmlPropertyCache *propertyCache,
        const QQmlPropertyData *property, QQmlContextData *qmlContext)
{
    if (property->isConstant())
        return;

    if (QQmlPropertyCapture *capture = propertyCapture(qmlContext))
        capture->captureProperty(object, propertyCache, property);
}

static bool inherits(const QQmlPropertyCache *descendent, const QQmlPropertyCache *ancestor)
{
    for (const QQmlPropertyCache *cache = descendent; cache; cache = cache->parent().data()) {
        if (cache == ancestor)
            return true;
    }
    return false;
}

enum class ObjectPropertyResult { OK, NeedsInit, Deleted };

template<bool StrictType = false>
ObjectPropertyResult loadObjectProperty(
        QV4::Lookup *l, QObject *object, void *target, QQmlContextData *qmlContext)
{
    QQmlData *qmlData = QQmlData::get(object);
    if (!qmlData)
        return ObjectPropertyResult::NeedsInit;
    if (qmlData->isQueuedForDeletion)
        return ObjectPropertyResult::Deleted;
    Q_ASSERT(!QQmlData::wasDeleted(object));
    const QQmlPropertyCache *propertyCache = l->qobjectLookup.propertyCache;
    if (StrictType) {
        if (qmlData->propertyCache.data() != propertyCache)
            return ObjectPropertyResult::NeedsInit;
    } else if (!inherits(qmlData->propertyCache.data(), propertyCache)) {
        return ObjectPropertyResult::NeedsInit;
    }
    const QQmlPropertyData *property = l->qobjectLookup.propertyData;

    const int coreIndex = property->coreIndex();
    if (qmlData->hasPendingBindingBit(coreIndex))
        qmlData->flushPendingBinding(coreIndex);

    captureObjectProperty(object, propertyCache, property, qmlContext);
    property->readProperty(object, target);
    return ObjectPropertyResult::OK;
}

static ObjectPropertyResult loadFallbackProperty(
        QV4::Lookup *l, QObject *object, void *target, QQmlContextData *qmlContext)
{
    QQmlData *qmlData = QQmlData::get(object);
    if (qmlData && qmlData->isQueuedForDeletion)
        return ObjectPropertyResult::Deleted;

    Q_ASSERT(!QQmlData::wasDeleted(object));

    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(l->qobjectFallbackLookup.metaObject - 1);
    if (!metaObject || metaObject != object->metaObject())
        return ObjectPropertyResult::NeedsInit;

    const int coreIndex = l->qobjectFallbackLookup.coreIndex;
    if (qmlData && qmlData->hasPendingBindingBit(coreIndex))
        qmlData->flushPendingBinding(coreIndex);

    captureFallbackProperty(object, coreIndex, l->qobjectFallbackLookup.notifyIndex,
                            l->qobjectFallbackLookup.isConstant, qmlContext);

    void *a[] = { target, nullptr };
    metaObject->metacall(object, QMetaObject::ReadProperty, coreIndex, a);

    return ObjectPropertyResult::OK;
}

ObjectPropertyResult loadObjectAsVariant(
        QV4::Lookup *l, QObject *object, void *target, QQmlContextData *qmlContext)
{
    QVariant *variant = static_cast<QVariant *>(target);
    const QMetaType propType = l->qobjectLookup.propertyData->propType();
    if (propType == QMetaType::fromType<QVariant>())
        return loadObjectProperty<true>(l, object, variant, qmlContext);

    *variant = QVariant(propType);
    return loadObjectProperty<true>(l, object, variant->data(), qmlContext);
}

ObjectPropertyResult loadFallbackAsVariant(
        QV4::Lookup *l, QObject *object, void *target, QQmlContextData *qmlContext)
{
    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(l->qobjectFallbackLookup.metaObject - 1);
    Q_ASSERT(metaObject);

    QVariant *variant = static_cast<QVariant *>(target);
    const QMetaType propType = metaObject->property(l->qobjectFallbackLookup.coreIndex).metaType();
    if (propType == QMetaType::fromType<QVariant>())
        return loadFallbackProperty(l, object, variant, qmlContext);

    *variant = QVariant(propType);
    return loadFallbackProperty(l, object, variant->data(), qmlContext);
}

template<bool StrictType, typename Op>
static ObjectPropertyResult changeObjectProperty(QV4::Lookup *l, QObject *object, Op op)
{
    const QQmlData *qmlData = QQmlData::get(object);
    if (!qmlData)
        return ObjectPropertyResult::NeedsInit;
    if (qmlData->isQueuedForDeletion)
        return ObjectPropertyResult::Deleted;
    Q_ASSERT(!QQmlData::wasDeleted(object));
    if (StrictType) {
        if (qmlData->propertyCache.data() != l->qobjectLookup.propertyCache)
            return ObjectPropertyResult::NeedsInit;
    } else if (!inherits(qmlData->propertyCache.data(), l->qobjectLookup.propertyCache)) {
        return ObjectPropertyResult::NeedsInit;
    }
    const QQmlPropertyData *property = l->qobjectLookup.propertyData;
    QQmlPropertyPrivate::removeBinding(object, QQmlPropertyIndex(property->coreIndex()));
    op(property);
    return ObjectPropertyResult::OK;
}

template<bool StrictType = false>
static ObjectPropertyResult resetObjectProperty(QV4::Lookup *l, QObject *object)
{
    return changeObjectProperty<StrictType>(l, object, [&](const QQmlPropertyData *property) {
        property->resetProperty(object, {});
    });
}

template<bool StrictType = false>
static ObjectPropertyResult storeObjectProperty(QV4::Lookup *l, QObject *object, void *value)
{
    return changeObjectProperty<StrictType>(l, object, [&](const QQmlPropertyData *property) {
        property->writeProperty(object, value, {});
    });
}

template<typename Op>
static ObjectPropertyResult changeFallbackProperty(QV4::Lookup *l, QObject *object, Op op)
{
    const QQmlData *qmlData = QQmlData::get(object);
    if (qmlData && qmlData->isQueuedForDeletion)
        return ObjectPropertyResult::Deleted;
    Q_ASSERT(!QQmlData::wasDeleted(object));

    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(l->qobjectFallbackLookup.metaObject - 1);
    if (!metaObject || metaObject != object->metaObject())
        return ObjectPropertyResult::NeedsInit;

    const int coreIndex = l->qobjectFallbackLookup.coreIndex;
    QQmlPropertyPrivate::removeBinding(object, QQmlPropertyIndex(coreIndex));

    op(metaObject, coreIndex);
    return ObjectPropertyResult::OK;
}

static ObjectPropertyResult storeFallbackProperty(QV4::Lookup *l, QObject *object, void *value)
{
    return changeFallbackProperty(l, object, [&](const QMetaObject *metaObject, int coreIndex) {
        void *args[] = { value, nullptr };
        metaObject->metacall(object, QMetaObject::WriteProperty, coreIndex, args);
    });
}

static ObjectPropertyResult resetFallbackProperty(QV4::Lookup *l, QObject *object)
{
    return changeFallbackProperty(l, object, [&](const QMetaObject *metaObject, int coreIndex) {
        void *args[] = { nullptr };
        metaObject->metacall(object, QMetaObject::ResetProperty, coreIndex, args);
    });
}

static bool isTypeCompatible(QMetaType lookupType, QMetaType propertyType)
{
    if (!lookupType.isValid()) {
        // If type is invalid, then the calling code depends on the lookup
        // to be set up in order to query the type, via lookupResultMetaType.
        // We cannot verify the type in this case.
    } else if ((lookupType.flags() & QMetaType::IsQmlList)
               && (propertyType.flags() & QMetaType::IsQmlList)) {
        // We want to check the value types here, but we cannot easily do it.
        // Internally those are all QObject* lists, though.
    } else if (lookupType.flags() & QMetaType::PointerToQObject) {
        // We accept any base class as type, too

        const QMetaObject *typeMetaObject = lookupType.metaObject();
        const QMetaObject *foundMetaObject = propertyType.metaObject();
        if (!foundMetaObject)
            foundMetaObject = QQmlMetaType::metaObjectForType(propertyType).metaObject();

        while (foundMetaObject && foundMetaObject != typeMetaObject)
            foundMetaObject = foundMetaObject->superClass();

        if (!foundMetaObject)
            return false;
    } else if (propertyType.flags() & QMetaType::IsEnumeration) {
        if (propertyType == lookupType)
            return true;

        // You can pass the underlying type of an enum.
        // We don't want to check for the actual underlying type because
        // moc and qmltyperegistrar are not very precise about it. Especially
        // the long and longlong types can be ambiguous.

        const bool isUnsigned = propertyType.flags() & QMetaType::IsUnsignedEnumeration;
        switch (propertyType.sizeOf()) {
        case 1:
            return isUnsigned
                    ? lookupType == QMetaType::fromType<quint8>()
                    : lookupType == QMetaType::fromType<qint8>();
        case 2:
            return isUnsigned
                    ? lookupType == QMetaType::fromType<ushort>()
                    : lookupType == QMetaType::fromType<short>();
        case 4:
            // The default type, if moc doesn't know the actual enum type, is int.
            // However, the compiler can still decide to encode the enum in uint.
            // Therefore, we also accept int for uint enums.
            // TODO: This is technically UB.
            return isUnsigned
                    ? (lookupType == QMetaType::fromType<int>()
                       || lookupType == QMetaType::fromType<uint>())
                    : lookupType == QMetaType::fromType<int>();
        case 8:
            return isUnsigned
                    ? lookupType == QMetaType::fromType<qulonglong>()
                    : lookupType == QMetaType::fromType<qlonglong>();
        }

        return false;
    } else if (propertyType != lookupType) {
        return false;
    }
    return true;
}

static ObjectPropertyResult storeObjectAsVariant(
        QV4::ExecutionEngine *v4, QV4::Lookup *l, QObject *object, void *value)
{
    QVariant *variant = static_cast<QVariant *>(value);
    const QMetaType propType = l->qobjectLookup.propertyData->propType();
    if (propType == QMetaType::fromType<QVariant>())
        return storeObjectProperty<true>(l, object, variant);

    if (!variant->isValid())
        return resetObjectProperty<true>(l, object);

    if (isTypeCompatible(variant->metaType(), propType))
        return storeObjectProperty<true>(l, object, variant->data());

    QVariant converted(propType);
    v4->metaTypeFromJS(v4->fromVariant(*variant), propType, converted.data());
    return storeObjectProperty<true>(l, object, converted.data());
}

static ObjectPropertyResult storeFallbackAsVariant(
        QV4::ExecutionEngine *v4, QV4::Lookup *l, QObject *object, void *value)
{
    QVariant *variant = static_cast<QVariant *>(value);

    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(l->qobjectFallbackLookup.metaObject - 1);
    Q_ASSERT(metaObject);

    const QMetaType propType = metaObject->property(l->qobjectFallbackLookup.coreIndex).metaType();
    if (propType == QMetaType::fromType<QVariant>())
        return storeFallbackProperty(l, object, variant);

    if (!propType.isValid())
        return resetFallbackProperty(l, object);

    if (isTypeCompatible(variant->metaType(), propType))
        return storeFallbackProperty(l, object, variant->data());

    QVariant converted(propType);
    v4->metaTypeFromJS(v4->fromVariant(*variant), propType, converted.data());
    return storeFallbackProperty(l, object, converted.data());
}

enum class ObjectLookupResult {
    Failure,
    Object,
    Fallback,
    ObjectAsVariant,
    FallbackAsVariant,
};

static ObjectLookupResult initObjectLookup(
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
        return ObjectLookupResult::Failure;

    const QQmlPropertyData *property;
    if (!ddata->propertyCache) {
        property = QQmlPropertyCache::property(object, name, aotContext->qmlContext, nullptr);
    } else {
        property = ddata->propertyCache->property(
                    name.getPointer(), object, aotContext->qmlContext);
    }

    const bool doVariantLookup = type == QMetaType::fromType<QVariant>();
    if (!property) {
        const QMetaObject *metaObject = object->metaObject();
        if (!metaObject)
            return ObjectLookupResult::Failure;

        const int coreIndex = metaObject->indexOfProperty(
                    name->toQStringNoThrow().toUtf8().constData());
        if (coreIndex < 0)
            return ObjectLookupResult::Failure;

        const QMetaProperty property = metaObject->property(coreIndex);
        if (!doVariantLookup && !isTypeCompatible(type, property.metaType()))
            return ObjectLookupResult::Failure;

        l->releasePropertyCache();
        // & 1 to tell the gc that this is not heap allocated; see markObjects in qv4lookup_p.h
        l->qobjectFallbackLookup.metaObject = quintptr(metaObject) + 1;
        l->qobjectFallbackLookup.coreIndex = coreIndex;
        l->qobjectFallbackLookup.notifyIndex =
                QMetaObjectPrivate::signalIndex(property.notifySignal());
        l->qobjectFallbackLookup.isConstant = property.isConstant() ? 1 : 0;
        return doVariantLookup
                ? ObjectLookupResult::FallbackAsVariant
                : ObjectLookupResult::Fallback;
    }

    if (!doVariantLookup && !isTypeCompatible(type, property->propType()))
        return ObjectLookupResult::Failure;

    Q_ASSERT(ddata->propertyCache);

    QV4::setupQObjectLookup(l, ddata, property);

    return doVariantLookup
            ? ObjectLookupResult::ObjectAsVariant
            : ObjectLookupResult::Object;
}

static bool initValueLookup(QV4::Lookup *l, QV4::ExecutableCompilationUnit *compilationUnit,
                            const QMetaObject *metaObject, QMetaType type)
{
    Q_ASSERT(metaObject);
    const QByteArray name = compilationUnit->runtimeStrings[l->nameIndex]->toQString().toUtf8();
    const int coreIndex = metaObject->indexOfProperty(name.constData());
    QMetaType lookupType = metaObject->property(coreIndex).metaType();
    if (!isTypeCompatible(type, lookupType))
        return false;
    l->qgadgetLookup.metaObject = quintptr(metaObject) + 1;
    l->qgadgetLookup.coreIndex = coreIndex;
    l->qgadgetLookup.metaType = lookupType.iface();
    return true;
}

static void amendException(QV4::ExecutionEngine *engine)
{
    const int missingLineNumber = engine->currentStackFrame->missingLineNumber();
    const int lineNumber = engine->currentStackFrame->lineNumber();
    Q_ASSERT(missingLineNumber != lineNumber);

    auto amendStackTrace = [&](QV4::StackTrace *stackTrace) {
        for (auto it = stackTrace->begin(), end = stackTrace->end(); it != end; ++it) {
            if (it->line == missingLineNumber) {
                it->line = lineNumber;
                break;
            }
        }
    };

    amendStackTrace(&engine->exceptionStackTrace);

    QV4::Scope scope(engine);
    QV4::Scoped<QV4::ErrorObject> error(scope, *engine->exceptionValue);
    if (error) // else some other value was thrown
        amendStackTrace(error->d()->stackTrace);
}


bool AOTCompiledContext::captureLookup(uint index, QObject *object) const
{
    if (!object)
        return false;

    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->getter == QV4::QQmlTypeWrapper::lookupSingletonProperty
            || l->getter == QV4::Lookup::getterQObject
            || l->getter == QV4::Lookup::getterQObjectAsVariant) {
        const QQmlPropertyData *property = l->qobjectLookup.propertyData;
        QQmlData::flushPendingBinding(object, property->coreIndex());
        captureObjectProperty(object, l->qobjectLookup.propertyCache, property, qmlContext);
        return true;
    }

    if (l->getter == QV4::Lookup::getterFallback
        || l->getter == QV4::Lookup::getterFallbackAsVariant) {
        const int coreIndex = l->qobjectFallbackLookup.coreIndex;
        QQmlData::flushPendingBinding(object, coreIndex);
        captureFallbackProperty(
                    object, coreIndex, l->qobjectFallbackLookup.notifyIndex,
                    l->qobjectFallbackLookup.isConstant, qmlContext);
        return true;
    }

    return false;
}

bool AOTCompiledContext::captureQmlContextPropertyLookup(uint index) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupScopeObjectProperty
            && l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupContextObjectProperty) {
        const QQmlPropertyData *property = l->qobjectLookup.propertyData;
        QQmlData::flushPendingBinding(qmlScopeObject, property->coreIndex());
        captureObjectProperty(qmlScopeObject, l->qobjectLookup.propertyCache, property, qmlContext);
        return true;
    }

    if (l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupScopeFallbackProperty) {
        const int coreIndex = l->qobjectFallbackLookup.coreIndex;
        QQmlData::flushPendingBinding(qmlScopeObject, coreIndex);
        captureFallbackProperty(qmlScopeObject, coreIndex, l->qobjectFallbackLookup.notifyIndex,
                                l->qobjectFallbackLookup.isConstant, qmlContext);
        return true;
    }

    return false;
}

void AOTCompiledContext::captureTranslation() const
{
    if (QQmlPropertyCapture *capture = propertyCapture(qmlContext))
        capture->captureTranslation();
}

QString AOTCompiledContext::translationContext() const
{
#if QT_CONFIG(translation)
    return QV4::GlobalExtensions::currentTranslationContext(engine->handle());
#else
    return QString();
#endif
}

QMetaType AOTCompiledContext::lookupResultMetaType(uint index) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupScopeObjectProperty
            || l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupContextObjectProperty
            || l->getter == QV4::QQmlTypeWrapper::lookupSingletonProperty
            || l->getter == QV4::Lookup::getterQObject
            || l->setter == QV4::Lookup::setterQObject
            || l->getter == QV4::Lookup::getterQObjectAsVariant
            || l->setter == QV4::Lookup::setterQObjectAsVariant) {
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
    } else if (l->getter == QV4::Lookup::getterFallback
               || l->setter == QV4::Lookup::setterFallback
               || l->getter == QV4::Lookup::getterFallbackAsVariant
               || l->setter == QV4::Lookup::setterFallbackAsVariant
               || l->qmlContextPropertyGetter
                    == QV4::QQmlContextWrapper::lookupScopeFallbackProperty) {
        const QMetaObject *metaObject
                = reinterpret_cast<const QMetaObject *>(l->qobjectFallbackLookup.metaObject - 1);
        const int coreIndex = l->qobjectFallbackLookup.coreIndex;
        return metaObject->property(coreIndex).metaType();
    }
    return QMetaType();
}

static bool isUndefined(const void *value, QMetaType type)
{
    if (type == QMetaType::fromType<QVariant>())
        return !static_cast<const QVariant *>(value)->isValid();
    if (type == QMetaType::fromType<QJSValue>())
        return static_cast<const QJSValue *>(value)->isUndefined();
    if (type == QMetaType::fromType<QJSPrimitiveValue>()) {
        return static_cast<const QJSPrimitiveValue *>(value)->type()
                == QJSPrimitiveValue::Undefined;
    }
    return false;
}

void AOTCompiledContext::storeNameSloppy(uint nameIndex, void *value, QMetaType type) const
{
    // We don't really use any part of the lookup machinery here.
    // The QV4::Lookup is created on the stack to conveniently get the property cache, and through
    // the property cache we store a value into the property.

    QV4::Lookup l;
    memset(&l, 0, sizeof(QV4::Lookup));
    l.nameIndex = nameIndex;
    l.forCall = false;
    ObjectPropertyResult storeResult = ObjectPropertyResult::NeedsInit;
    switch (initObjectLookup(this, &l, qmlScopeObject, QMetaType())) {
    case ObjectLookupResult::ObjectAsVariant:
    case ObjectLookupResult::Object: {
        const QMetaType propType = l.qobjectLookup.propertyData->propType();
        if (isTypeCompatible(type, propType)) {
            storeResult = storeObjectProperty(&l, qmlScopeObject, value);
        } else if (isUndefined(value, type)) {
            storeResult = resetObjectProperty(&l, qmlScopeObject);
        } else {
            QVariant var(propType);
            QV4::ExecutionEngine *v4 = engine->handle();
            v4->metaTypeFromJS(v4->metaTypeToJS(type, value), propType, var.data());
            storeResult = storeObjectProperty(&l, qmlScopeObject, var.data());
        }

        l.qobjectLookup.propertyCache->release();
        break;
    }
    case ObjectLookupResult::FallbackAsVariant:
    case ObjectLookupResult::Fallback: {
        const QMetaObject *metaObject
                = reinterpret_cast<const QMetaObject *>(l.qobjectFallbackLookup.metaObject - 1);
        const QMetaType propType
                = metaObject->property(l.qobjectFallbackLookup.coreIndex).metaType();
        if (isTypeCompatible(type, propType)) {
            storeResult = storeFallbackProperty(&l, qmlScopeObject, value);
        } else if (isUndefined(value, type)) {
            storeResult = resetFallbackProperty(&l, qmlScopeObject);
        } else {
            QVariant var(propType);
            QV4::ExecutionEngine *v4 = engine->handle();
            v4->metaTypeFromJS(v4->metaTypeToJS(type, value), propType, var.data());
            storeResult = storeFallbackProperty(&l, qmlScopeObject, var.data());
        }
        break;
    }
    case ObjectLookupResult::Failure:
        engine->handle()->throwTypeError();
        return;
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
}

QJSValue AOTCompiledContext::javaScriptGlobalProperty(uint nameIndex) const
{
    QV4::Scope scope(engine->handle());
    QV4::ScopedString name(scope, compilationUnit->runtimeStrings[nameIndex]);
    QV4::ScopedObject global(scope, scope.engine->globalObject);
    return QJSValuePrivate::fromReturnedValue(global->get(name->toPropertyKey()));
}

const QLoggingCategory *AOTCompiledContext::resolveLoggingCategory(QObject *wrapper, bool *ok) const
{
    if (wrapper) {
        // We have to check this here because you may pass a plain QObject that only
        // turns out to be a QQmlLoggingCategory at run time.
        if (QQmlLoggingCategory *qQmlLoggingCategory
                = qobject_cast<QQmlLoggingCategory *>(wrapper)) {
            QLoggingCategory *loggingCategory = qQmlLoggingCategory->category();
            *ok = true;
            if (!loggingCategory) {
                engine->handle()->throwError(
                            QStringLiteral("A QmlLoggingCatgory was provided without a valid name"));
            }
            return loggingCategory;
        }
    }

    *ok = false;
    return qmlEngine() ? &lcQml() : &lcJs();
}

void AOTCompiledContext::writeToConsole(
        QtMsgType type, const QString &message, const QLoggingCategory *loggingCategory) const
{
    Q_ASSERT(loggingCategory->isEnabled(type));

    const QV4::CppStackFrame *frame = engine->handle()->currentStackFrame;
    Q_ASSERT(frame);

    const QByteArray source(frame->source().toUtf8());
    const QByteArray function(frame->function().toUtf8());
    QMessageLogger logger(source.constData(), frame->lineNumber(),
                          function.constData(), loggingCategory->categoryName());

    switch (type) {
    case QtDebugMsg:
        logger.debug("%s", qUtf8Printable(message));
        break;
    case QtInfoMsg:
        logger.info("%s", qUtf8Printable(message));
        break;
    case QtWarningMsg:
        logger.warning("%s", qUtf8Printable(message));
        break;
    case QtCriticalMsg:
        logger.critical("%s", qUtf8Printable(message));
        break;
    default:
        break;
    }
}

QVariant AOTCompiledContext::constructValueType(
        QMetaType resultMetaType, const QMetaObject *resultMetaObject,
        int ctorIndex, void *ctorArg) const
{
    return QQmlValueTypeProvider::constructValueType(
                resultMetaType, resultMetaObject, ctorIndex, ctorArg);
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

    function->call(qmlScopeObject, args, types, argc);
    return !scope.hasException();
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

    function->call(object, args, types, argc);
    return !scope.hasException();
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

    function->call(nullptr, args, types, argc);
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
    if (!QV4::ExecutionEngine::metaTypeFromJS(l->globalGetter(l, engine->handle()), type, target)) {
        engine->handle()->throwTypeError();
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

    ObjectPropertyResult result = ObjectPropertyResult::NeedsInit;
    if (l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupScopeObjectProperty)
        result = loadObjectProperty(l, qmlScopeObject, target, qmlContext);
    else if (l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupScopeFallbackProperty)
        result = loadFallbackProperty(l, qmlScopeObject, target, qmlContext);
    else
        return false;

    switch (result) {
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

    Q_UNREACHABLE_RETURN(false);
}

void AOTCompiledContext::initLoadScopeObjectPropertyLookup(uint index, QMetaType type) const
{
    QV4::ExecutionEngine *v4 = engine->handle();
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;

    if (v4->hasException) {
        amendException(v4);
        return;
    }

    switch (initObjectLookup(this, l, qmlScopeObject, type)) {
    case ObjectLookupResult::ObjectAsVariant:
    case ObjectLookupResult::Object:
        l->qmlContextPropertyGetter = QV4::QQmlContextWrapper::lookupScopeObjectProperty;
        break;
    case ObjectLookupResult::FallbackAsVariant:
    case ObjectLookupResult::Fallback:
        l->qmlContextPropertyGetter = QV4::QQmlContextWrapper::lookupScopeFallbackProperty;
        break;
    case ObjectLookupResult::Failure:
        v4->throwTypeError();
        break;
    }
}

bool AOTCompiledContext::loadSingletonLookup(uint index, void *target) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());

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

using QmlContextPropertyGetter
    = QV4::ReturnedValue (*)(QV4::Lookup *l, QV4::ExecutionEngine *engine, QV4::Value *thisObject);

template<QmlContextPropertyGetter qmlContextPropertyGetter>
static void initTypeWrapperLookup(
        const AOTCompiledContext *context, QV4::Lookup *l, uint importNamespace)
{
    Q_ASSERT(!context->engine->hasError());
    if (importNamespace != AOTCompiledContext::InvalidStringId) {
        QV4::Scope scope(context->engine->handle());
        QV4::ScopedString import(scope, context->compilationUnit->runtimeStrings[importNamespace]);
        if (const QQmlImportRef *importRef
                = context->qmlContext->imports()->query(import).importNamespace) {
            QV4::Scoped<QV4::QQmlTypeWrapper> wrapper(
                        scope, QV4::QQmlTypeWrapper::create(
                            scope.engine, nullptr, context->qmlContext->imports(), importRef));
            wrapper = l->qmlContextPropertyGetter(l, context->engine->handle(), wrapper);
            l->qmlContextPropertyGetter = qmlContextPropertyGetter;
            if (qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupSingleton)
                l->qmlContextSingletonLookup.singletonObject = wrapper->heapObject();
            else if (qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupType)
                l->qmlTypeLookup.qmlTypeWrapper = wrapper->heapObject();
            return;
        }
        scope.engine->throwTypeError();
    } else {
        QV4::ExecutionEngine *v4 = context->engine->handle();
        l->qmlContextPropertyGetter(l, v4, nullptr);
        if (l->qmlContextPropertyGetter != qmlContextPropertyGetter) {
            const QString error
                    = QLatin1String(qmlContextPropertyGetter
                                    == QV4::QQmlContextWrapper::lookupSingleton
                        ? "%1 was a singleton at compile time, "
                          "but is not a singleton anymore."
                        : "%1 was not a singleton at compile time, "
                          "but is a singleton now.")
                    .arg(context->compilationUnit->runtimeStrings[l->nameIndex]->toQString());
            v4->throwTypeError(error);
        }
    }
}

void AOTCompiledContext::initLoadSingletonLookup(uint index, uint importNamespace) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    initTypeWrapperLookup<QV4::QQmlContextWrapper::lookupSingleton>(this, l, importNamespace);
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

void AOTCompiledContext::initLoadAttachedLookup(
        uint index, uint importNamespace, QObject *object) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedString name(scope, compilationUnit->runtimeStrings[l->nameIndex]);

    QQmlType type;
    if (importNamespace != InvalidStringId) {
        QV4::ScopedString import(scope, compilationUnit->runtimeStrings[importNamespace]);
        if (const QQmlImportRef *importRef = qmlContext->imports()->query(import).importNamespace)
            type = qmlContext->imports()->query(name, importRef).type;
    } else {
        type = qmlContext->imports()->query<QQmlImport::AllowRecursion>(name).type;
    }

    if (!type.isValid()) {
        scope.engine->throwTypeError();
        return;
    }

    QV4::Scoped<QV4::QQmlTypeWrapper> wrapper(
                scope, QV4::QQmlTypeWrapper::create(scope.engine, object, type,
                                                    QV4::Heap::QQmlTypeWrapper::ExcludeEnums));

    l->qmlTypeLookup.qmlTypeWrapper = wrapper->d();
    l->getter = QV4::QObjectWrapper::lookupAttached;
}

bool AOTCompiledContext::loadTypeLookup(uint index, void *target) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->qmlContextPropertyGetter != QV4::QQmlContextWrapper::lookupType)
        return false;

    const QV4::Heap::QQmlTypeWrapper *typeWrapper = static_cast<const QV4::Heap::QQmlTypeWrapper *>(
                l->qmlTypeLookup.qmlTypeWrapper);
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(qmlEngine());

    QMetaType metaType = typeWrapper->type().typeId();
    if (!metaType.isValid()) {
        metaType = ep->typeLoader.getType(typeWrapper->type().sourceUrl())
                ->compilationUnit()->typeIds.id;
    }

    *static_cast<const QMetaObject **>(target)
            = QQmlMetaType::metaObjectForType(metaType).metaObject();
    return true;
}

void AOTCompiledContext::initLoadTypeLookup(uint index, uint importNamespace) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    initTypeWrapperLookup<QV4::QQmlContextWrapper::lookupType>(this, l, importNamespace);
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

    ObjectPropertyResult result = ObjectPropertyResult::NeedsInit;
    if (l->getter == QV4::Lookup::getterQObject)
        result = loadObjectProperty(l, object, target, qmlContext);
    else if (l->getter == QV4::Lookup::getterFallback)
        result = loadFallbackProperty(l, object, target, qmlContext);
    else if (l->getter == QV4::Lookup::getterQObjectAsVariant)
        result = loadObjectAsVariant(l, object, target, qmlContext);
    else if (l->getter == QV4::Lookup::getterFallbackAsVariant)
        result = loadFallbackAsVariant(l, object, target, qmlContext);
    else
        return false;

    switch (result) {
    case ObjectPropertyResult::Deleted:
        return doThrow();
    case ObjectPropertyResult::NeedsInit:
        return false;
    case ObjectPropertyResult::OK:
        return true;
    }

    Q_UNREACHABLE_RETURN(false);
}

void AOTCompiledContext::initGetObjectLookup(uint index, QObject *object, QMetaType type) const
{
    QV4::ExecutionEngine *v4 = engine->handle();
    if (v4->hasException) {
        amendException(v4);
    } else {
        QV4::Lookup *l = compilationUnit->runtimeLookups + index;
        switch (initObjectLookup(this, l, object, type)) {
        case ObjectLookupResult::Object:
            l->getter = QV4::Lookup::getterQObject;
            break;
        case ObjectLookupResult::ObjectAsVariant:
            l->getter = QV4::Lookup::getterQObjectAsVariant;
            break;
        case ObjectLookupResult::Fallback:
            l->getter = QV4::Lookup::getterFallback;
            break;
        case ObjectLookupResult::FallbackAsVariant:
            l->getter = QV4::Lookup::getterFallbackAsVariant;
            break;
        case ObjectLookupResult::Failure:
            engine->handle()->throwTypeError();
            break;
        }
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

bool AOTCompiledContext::getEnumLookup(uint index, void *target) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->getter != QV4::QQmlTypeWrapper::lookupEnumValue)
        return false;
    const bool isUnsigned
            = l->qmlEnumValueLookup.metaType->flags & QMetaType::IsUnsignedEnumeration;
    const QV4::ReturnedValue encoded = l->qmlEnumValueLookup.encodedEnumValue;
    switch (l->qmlEnumValueLookup.metaType->size) {
    case 1:
        if (isUnsigned)
            *static_cast<quint8 *>(target) = encoded;
        else
            *static_cast<qint8 *>(target) = encoded;
        return true;
    case 2:
        if (isUnsigned)
            *static_cast<quint16 *>(target) = encoded;
        else
            *static_cast<qint16 *>(target) = encoded;
        return true;
    case 4:
        if (isUnsigned)
            *static_cast<quint32 *>(target) = encoded;
        else
            *static_cast<qint32 *>(target) = encoded;
        return true;
    case 8:
        if (isUnsigned)
            *static_cast<quint64 *>(target) = encoded;
        else
            *static_cast<qint64 *>(target) = encoded;
        return true;
    default:
        break;
    }

    return false;
}

void AOTCompiledContext::initGetEnumLookup(
        uint index, const QMetaObject *metaObject,
        const char *enumerator, const char *enumValue) const
{
    Q_ASSERT(!engine->hasError());
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (!metaObject) {
        engine->handle()->throwTypeError(
                    QStringLiteral("Cannot read property '%1' of undefined")
                    .arg(QString::fromUtf8(enumValue)));
        return;
    }
    const int enumIndex = metaObject->indexOfEnumerator(enumerator);
    const QMetaEnum metaEnum = metaObject->enumerator(enumIndex);
    l->qmlEnumValueLookup.encodedEnumValue = metaEnum.keyToValue(enumValue);
    l->qmlEnumValueLookup.metaType = metaEnum.metaType().iface();
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
    ObjectPropertyResult result = ObjectPropertyResult::NeedsInit;
    if (l->setter == QV4::Lookup::setterQObject)
        result = storeObjectProperty(l, object, value);
    else if (l->setter == QV4::Lookup::setterFallback)
        result = storeFallbackProperty(l, object, value);
    else if (l->setter == QV4::Lookup::setterQObjectAsVariant)
        result = storeObjectAsVariant(engine->handle(), l, object, value);
    else if (l->setter == QV4::Lookup::setterFallbackAsVariant)
        result = storeFallbackAsVariant(engine->handle(), l, object, value);
    else
        return false;

    switch (result) {
    case ObjectPropertyResult::Deleted:
        return doThrow();
    case ObjectPropertyResult::NeedsInit:
        return false;
    case ObjectPropertyResult::OK:
        return true;
    }

    Q_UNREACHABLE_RETURN(false);
}

void AOTCompiledContext::initSetObjectLookup(uint index, QObject *object, QMetaType type) const
{
    QV4::ExecutionEngine *v4 = engine->handle();
    if (v4->hasException) {
        amendException(v4);
    } else {
        QV4::Lookup *l = compilationUnit->runtimeLookups + index;
        switch (initObjectLookup(this, l, object, type)) {
        case ObjectLookupResult::Object:
            l->setter = QV4::Lookup::setterQObject;
            break;
        case ObjectLookupResult::ObjectAsVariant:
            l->setter = QV4::Lookup::setterQObjectAsVariant;
            break;
        case ObjectLookupResult::Fallback:
            l->setter = QV4::Lookup::setterFallback;
            break;
        case ObjectLookupResult::FallbackAsVariant:
            l->setter = QV4::Lookup::setterFallbackAsVariant;
            break;
        case ObjectLookupResult::Failure:
            engine->handle()->throwTypeError();
            break;
        }
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
