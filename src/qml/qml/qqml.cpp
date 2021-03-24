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
#include <private/qv4lookup_p.h>
#include <private/qv4qobjectwrapper_p.h>

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

        auto revisions = prepareRevisions(type.metaObject, added);
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

        auto revisions = prepareRevisions(type.instanceMetaObject, added);
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

        QVector<QTypeRevision> revisions = { added };
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
}

QJSValue QQmlPrivate::AOTCompiledContext::jsMetaType(int index) const
{
    return QJSValuePrivate::fromReturnedValue(
                compilationUnit->runtimeClasses[index]->asReturnedValue());
}

void QQmlPrivate::AOTCompiledContext::setInstructionPointer(int offset) const
{
    if (auto *frame = engine->handle()->currentStackFrame)
        frame->instructionPointer = offset;
}

QJSValue QQmlPrivate::AOTCompiledContext::loadQmlContextPropertyLookup(uint index) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    return QJSValuePrivate::fromReturnedValue(l->qmlContextPropertyGetter(
                                                  l, engine->handle(), nullptr));
}

bool QQmlPrivate::AOTCompiledContext::captureQmlContextPropertyLookup(uint index) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->qmlContextPropertyGetter != QV4::QQmlContextWrapper::lookupScopeObjectProperty
            && l->qmlContextPropertyGetter != QV4::QQmlContextWrapper::lookupContextObjectProperty) {
        return false;
    }

    const auto *property = l->qobjectLookup.propertyData;
    Q_ASSERT(property);
    if (property->isConstant())
        return true;

    if (QQmlEngine *engine = qmlContext->engine()) {
        QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
        if (!ep->propertyCapture)
            return true;
        ep->propertyCapture->captureProperty(qmlScopeObject, property->coreIndex(),
                                             property->notifyIndex());
    }

    return true;
}

QObject *QQmlPrivate::AOTCompiledContext::loadQmlContextPropertyIdLookup(uint index) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->qmlContextPropertyGetter == QV4::QQmlContextWrapper::lookupIdObject) {
        // Shortcut this to avoid the wrapping when returning from lookupIdObject().
        if (!qmlContext)
            return nullptr;

        const auto context = QQmlContextData::get(qmlContext);
        QQmlEnginePrivate *qmlEngine = QQmlEnginePrivate::get(context->engine());
        const int objectId = l->qmlContextIdObjectLookup.objectId;

        if (qmlEngine->propertyCapture)
            qmlEngine->propertyCapture->captureProperty(context->idValueBindings(objectId));
        return context->idValue(objectId);
    }

    QV4::Scope scope(engine->handle());
    QV4::Scoped<QV4::QObjectWrapper> o(
                scope, l->qmlContextPropertyGetter(l, scope.engine, nullptr));
    return o ? o->object() : nullptr;
}

bool QQmlPrivate::AOTCompiledContext::getObjectLookup(
        uint index, QObject *object, void *target, QMetaType type) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;

    if (!object) {
        QString message = QStringLiteral("Cannot read property '%1' of null")
                .arg(compilationUnit->runtimeStrings[l->nameIndex]->toQString());
        engine->handle()->throwTypeError(message);
        return false;
    }

    if (l->getter == QV4::QObjectWrapper::lookupGetter
            && !QQmlData::wasDeleted(object)
            && QQmlData::get(object)->propertyCache == l->qobjectLookup.propertyCache) {
        QQmlPropertyData *property = l->qobjectLookup.propertyData;
        if (property->propType() == type) {
            // We can directly read the property into the target, without conversion.
            if (!property->isConstant()) {
                if (QQmlEngine *engine = qmlContext->engine()) {
                    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
                    if (ep->propertyCapture) {
                        ep->propertyCapture->captureProperty(object, property->coreIndex(),
                                                             property->notifyIndex());
                    }
                }
            }
            property->readProperty(object, target);
            return true;
        }
    }

    QV4::Scope scope(engine->handle());
    QV4::ScopedValue o(scope, QV4::QObjectWrapper::wrap(scope.engine, object));
    QV4::ScopedValue result(scope, l->getter(l, scope.engine, o));
    if (type == QMetaType::fromType<QVariant>()) {
        // Special case QVariant in order to retain JS objects.
        // We don't want to convert JS arrays into QVariantList, for example.
        *static_cast<QVariant *>(target) = scope.engine->toVariant(result, QMetaType {});
        return true;
    }
    return scope.engine->metaTypeFromJS(result, type, target);
}

QJSValue QQmlPrivate::AOTCompiledContext::callQmlContextPropertyLookup(
        uint index, const QJSValueList &args) const
{
    QV4::Scope scope(engine->handle());
    const int argc = args.length();
    QV4::Value *argv = scope.alloc(args.length());
    for (int i = 0; i < argc; ++i)
        argv[i] = QJSValuePrivate::convertToReturnedValue(scope.engine, args.at(i));
    return QJSValuePrivate::fromReturnedValue(QV4::Runtime::CallQmlContextPropertyLookup::call(
                                                  engine->handle(), index, argv, argc));
}

QJSValue QQmlPrivate::AOTCompiledContext::getLookup(uint index, const QJSValue &object) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;

    if (object.isNull() || object.isUndefined()) {
        QString message = QStringLiteral("Cannot read property '%1' of %2")
                .arg(compilationUnit->runtimeStrings[l->nameIndex]->toQString(), object.toString());
        return QJSValuePrivate::fromReturnedValue(engine->handle()->throwTypeError(message));
    }

    return QJSValuePrivate::fromReturnedValue(
                l->getter(l, engine->handle(),
                          QJSValuePrivate::convertToReturnedValue(engine->handle(), object)));
}

bool QQmlPrivate::AOTCompiledContext::captureLookup(uint index, QObject *object) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    if (l->getter != QV4::QQmlTypeWrapper::lookupSingletonProperty
            && l->getter != QV4::QObjectWrapper::lookupGetter) {
        return false;
    }

    const auto *property = l->qobjectLookup.propertyData;
    Q_ASSERT(property);
    if (property->isConstant())
        return true;

    if (QQmlEngine *engine = qmlContext->engine()) {
        QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
        if (!ep->propertyCapture)
            return true;
        ep->propertyCapture->captureProperty(object, property->coreIndex(),
                                             property->notifyIndex());
        return true;
    }

    return false;
}

void QQmlPrivate::AOTCompiledContext::setLookup(
        uint index, const QJSValue &object, const QJSValue &value) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue o(scope, QJSValuePrivate::convertToReturnedValue(scope.engine, object));
    if (!l->setter(l, engine->handle(), *o,
                   QJSValuePrivate::convertToReturnedValue(engine->handle(), value))) {
        engine->handle()->throwTypeError();
    }
}

QJSValue QQmlPrivate::AOTCompiledContext::callPropertyLookup(
        uint index, const QJSValue &object, const QJSValueList &args) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue o(scope, QJSValuePrivate::convertToReturnedValue(scope.engine, object));

    // ok to have the value on the stack here
    QV4::Value f = QV4::Value::fromReturnedValue(l->getter(l, engine->handle(), o));

    if (Q_UNLIKELY(!f.isFunctionObject())) {
        QString message = QStringLiteral("Property '%1' of object %2 is not a function")
                .arg(compilationUnit->runtimeStrings[l->nameIndex]->toQString(),
                     object.toString());
        engine->handle()->throwTypeError(message);
        return QJSValue();
    }

    const int argc = args.length();
    QV4::Value *argv = scope.alloc(args.length());
    for (int i = 0; i < argc; ++i)
        argv[i] = QJSValuePrivate::convertToReturnedValue(scope.engine, args.at(i));

    return QJSValuePrivate::fromReturnedValue(
                static_cast<QV4::FunctionObject &>(f).call(o, argv, argc));
}

QJSValue QQmlPrivate::AOTCompiledContext::callGlobalLookup(
        uint index, const QJSValueList &args) const
{
    QV4::Scope scope(engine->handle());
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    QV4::Value function = QV4::Value::fromReturnedValue(l->globalGetter(l, scope.engine));
    if (!function.isFunctionObject()) {
        const QString msg = QStringLiteral("Property '%1' of object [null] is not a function")
                            .arg(compilationUnit->runtimeStrings[l->nameIndex]->toQString());
        return QJSValuePrivate::fromReturnedValue(scope.engine->throwTypeError(msg));
    }

    const int argc = args.length();
    QV4::Value *argv = scope.alloc(args.length());
    for (int i = 0; i < argc; ++i)
        argv[i] = QJSValuePrivate::convertToReturnedValue(scope.engine, args.at(i));

    QV4::Value thisObject = QV4::Value::undefinedValue();
    QV4::ReturnedValue result = static_cast<QV4::FunctionObject &>(function).call(
                &thisObject, argv, argc);

    return scope.engine->hasException ? QJSValue() : QJSValuePrivate::fromReturnedValue(result);
}

QJSValue QQmlPrivate::AOTCompiledContext::loadGlobalLookup(uint index) const
{
    QV4::Lookup *l = compilationUnit->runtimeLookups + index;
    return QJSValuePrivate::fromReturnedValue(l->globalGetter(l, engine->handle()));
}

QT_END_NAMESPACE
