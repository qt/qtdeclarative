// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqml.h"

#include <QtQml/qqmlprivate.h>

#include <private/qjsvalue_p.h>
#include <private/qqmlbuiltinfunctions_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlfinalizer_p.h>
#include <private/qqmlloggingcategorybase_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmlmetatypedata_p.h>
#include <private/qqmltype_p_p.h>
#include <private/qqmltypemodule_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/qqmlvaluetypewrapper_p.h>
#include <private/qv4alloca_p.h>
#include <private/qv4dateobject_p.h>
#include <private/qv4errorobject_p.h>
#include <private/qv4identifiertable_p.h>
#include <private/qv4lookup_p.h>
#include <private/qv4qobjectwrapper_p.h>

#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

/*!
  \headerfile <qqml.h>
  \inmodule QtQml
  \title Functions to register C++ types to QML

  This header provides a collection of functions that allow the registration of
  C++ types to QML.

  \sa {Overview - QML and C++ Integration}, qqmlintegration.h, qmltyperegistrar
*/

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

    if (!data->propertyCache)
        data->propertyCache = QQmlMetaType::propertyCache(object->metaObject());

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

/*!
    \relates <qqml.h>

    This function returns the extension object that belongs to \a base, if there is any.
    Otherwise it returns \c nullptr.

    \sa QML_EXTENDED
*/
QObject *qmlExtendedObject(QObject *base)
{
    return QQmlPrivate::qmlExtendedObject(base, 0);
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
        qWarning().nospace()
                << metaType.name()
                << " is neither a default constructible QObject, nor a default- "
                << "and copy-constructible Q_GADGET, nor a QObject marked as uncreatable.\n"
                << "You should not use it as a QML type.";
        break;
    case UnconstructibleSingleton:
        qWarning()
                << "Singleton" << metaType.name()
                << "needs to be a concrete class with either a default constructor"
                << "or, when adding a default constructor is infeasible, a public static"
                << "create(QQmlEngine *, QJSEngine *) method.";
        break;
    case NonQObjectWithAtached:
        qWarning()
                << metaType.name()
                << "is not a QObject, but has attached properties. This won't work.";
        break;
    }
}

QMetaType QQmlPrivate::compositeMetaType(
        QV4::ExecutableCompilationUnit *unit, int elementNameId)
{
    return QQmlTypePrivate::visibleQmlTypeByName(unit, elementNameId).typeId();
}

QMetaType QQmlPrivate::compositeMetaType(
        QV4::ExecutableCompilationUnit *unit, const QString &elementName)
{
    return QQmlTypePrivate::visibleQmlTypeByName(
                   unit->baseCompilationUnit(), elementName, unit->engine->typeLoader())
            .typeId();
}

QMetaType QQmlPrivate::compositeListMetaType(
        QV4::ExecutableCompilationUnit *unit, int elementNameId)
{
    return QQmlTypePrivate::visibleQmlTypeByName(unit, elementNameId).qListTypeId();
}

QMetaType QQmlPrivate::compositeListMetaType(
        QV4::ExecutableCompilationUnit *unit, const QString &elementName)
{
    return QQmlTypePrivate::visibleQmlTypeByName(
                   unit->baseCompilationUnit(), elementName, unit->engine->typeLoader())
            .qListTypeId();
}

/*!
  \relates <qqml.h>
  \since 5.8

  This function registers the \a staticMetaObject and its extension
  in the QML system with the name \a qmlName in the library imported
  from \a uri having version number composed from \a versionMajor and
  \a versionMinor.

  An instance of the meta object cannot be created. An error message with
  the given \a reason is printed if the user attempts to create it.

  This function is useful for registering Q_NAMESPACE namespaces.

  Returns the QML type id.

  For example:

  //! Workaround for MOC not respecting comments and triggering an error on certain Qt macros.
  \code Q
  namespace MyNamespace {
    \1_NAMESPACE
    enum MyEnum {
        Key1,
        Key2,
    };
    \1_ENUM_NS(MyEnum)
  }

  //...
  qmlRegisterUncreatableMetaObject(MyNamespace::staticMetaObject, "io.qt", 1, 0, "MyNamespace", "Access to enums & flags only");
  \endcode

  On the QML side, you can now use the registered enums:
  \code
  Component.onCompleted: console.log(MyNamespace.Key2)
  \endcode

  \sa QML_ELEMENT, QML_NAMED_ELEMENT(), QML_UNCREATABLE()
*/
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

/*!
  \relates <qqml.h>

  Clears all stored type registrations, such as those produced with \l qmlRegisterType().

  Do not call this function while a QQmlEngine exists or behavior will be undefined.
  Any existing QQmlEngines must be deleted before calling this function.  This function
  only affects the application global cache. Delete the QQmlEngine to clear all cached
  data relating to that engine.
*/
void qmlClearTypeRegistrations() // Declared in qqml.h
{
    QQmlMetaType::clearTypeRegistrations();
    QQmlEnginePrivate::baseModulesUninitialized = true; //So the engine re-registers its types
    qmlClearEnginePlugins();
}

/*!
  \relates <qqml.h>

  This function protects a module from further modification. This can be used
  to prevent other plugins from injecting types into your module. It can also
  be a performance improvement, as it allows the engine to skip checking for
  the possibility of new types or plugins when this import is reached.

  Once qmlProtectModule has been called, a QML engine will not search for a new
  \c qmldir file to load the module anymore. It will re-use any \c qmldir files
  it has loaded before, though. Therefore, types present at this point continue
  to work. Mind that different QML engines may load different modules. The
  module protection, however, is global and affects all engines. The overhead
  of locating \c qmldir files and loading plugins may be noticeable with slow file
  systems. Therefore, protecting a module once you are sure you won't need to
  load it anymore can be a good optimization. Mind also that the module lock
  not only affects plugins but also any other qmldir directives, like \c import
  or \c prefer, as well as any composite types or scripts declared in a \c qmldir
  file.

  In addition, after this function is called, any attempt to register C++ types
  into this uri, major version combination will lead to a runtime error.

  Returns true if the module with \a uri as a \l{Identified Modules}
  {module identifier} and \a majVersion as a major version number was found
  and locked, otherwise returns false. The module must contain exported types
  in order to be found.
*/
bool qmlProtectModule(const char *uri, int majVersion)
{
    return QQmlMetaType::protectModule(QString::fromUtf8(uri),
                                       QTypeRevision::fromMajorVersion(majVersion));
}

/*!
  \since 5.9
  \relates <qqml.h>

  This function registers a module in a particular \a uri with a version specified
  in \a versionMajor and \a versionMinor.

  This can be used to make a certain module version available, even if no types
  are registered for that version. This is particularly useful for keeping the
  versions of related modules in sync.
*/

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
 * \relates <qqml.h>
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
 * \relates <qqml.h>
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
 * \relates <qqml.h>
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

/*!
  \since 5.12
  \relates <qqml.h>

  Returns the QML type id of a type that was registered with the
  name \a qmlName in a particular \a uri and a version specified in \a
  versionMajor and \a versionMinor.

  This function returns the same value as the QML type registration functions
  such as qmlRegisterType() and qmlRegisterSingletonType().

  If \a qmlName, \a uri and \a versionMajor match a registered type, but the
  specified minor version in \a versionMinor is higher, then the id of the type
  with the closest minor version is returned.

  Returns -1 if no matching type was found or one of the given parameters
  was invalid.

  \note: qmlTypeId tries to make modules available, even if they were not accessed by any
  engine yet. This can introduce overhead the first time a module is accessed. Trying to
  find types from a module which does not exist always introduces this overhead.

  \sa QML_ELEMENT, QML_NAMED_ELEMENT, QML_SINGLETON, qmlRegisterType(), qmlRegisterSingletonType()
*/
int qmlTypeId(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    auto revision = QTypeRevision::fromVersion(versionMajor, versionMinor);
    int id =  QQmlMetaType::typeId(uri, revision, qmlName);
    if (id != -1)
        return id;
    /* If the module hasn't been imported yet, we might not have the id of a
       singleton at this point. To obtain it, we need an engine in order to
       to do the resolution steps.
       This is expensive, but we assume that users don't constantly query invalid
       Types; internal code should use QQmlMetaType API.
    */
    QQmlEngine engine;
    QQmlTypeLoader *typeLoader = &QQmlEnginePrivate::get(&engine)->typeLoader;
    auto loadHelper = QQml::makeRefPointer<LoadHelper>(
            typeLoader, uri, qmlName, QQmlTypeLoader::Synchronous);
    const QQmlType type = loadHelper->type();
    if (type.availableInVersion(revision))
        return type.index();
    else
        return -1;
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

static QQmlType::SingletonInstanceInfo::ConstPtr singletonInstanceInfo(
        const QQmlPrivate::RegisterSingletonType &type)
{
    QQmlType::SingletonInstanceInfo::Ptr siinfo = QQmlType::SingletonInstanceInfo::create();
    siinfo->scriptCallback = type.scriptApi;
    siinfo->qobjectCallback = type.qObjectApi;
    siinfo->typeName = type.typeName;
    return QQmlType::SingletonInstanceInfo::ConstPtr(
            siinfo.take(), QQmlType::SingletonInstanceInfo::ConstPtr::Adopt);
}

static QQmlType::SingletonInstanceInfo::ConstPtr singletonInstanceInfo(
        const QQmlPrivate::RegisterCompositeSingletonType &type)
{
    QQmlType::SingletonInstanceInfo::Ptr siinfo = QQmlType::SingletonInstanceInfo::create();
    siinfo->url = QQmlTypeLoader::normalize(type.url);
    siinfo->typeName = type.typeName;
    return QQmlType::SingletonInstanceInfo::ConstPtr(
            siinfo.take(), QQmlType::SingletonInstanceInfo::ConstPtr::Adopt);
}

static int finalizeType(const QQmlType &dtype)
{
    if (!dtype.isValid())
        return -1;

    QQmlMetaType::registerUndeletableType(dtype);
    return dtype.index();
}

using ElementNames = QVarLengthArray<const char *, 8>;
static ElementNames classElementNames(const QMetaObject *metaObject)
{
    Q_ASSERT(metaObject);
    const char *key = "QML.Element";

    const int offset = metaObject->classInfoOffset();
    const int start = metaObject->classInfoCount() + offset - 1;

    ElementNames elementNames;

    for (int i = start; i >= offset; --i) {
        const QMetaClassInfo classInfo = metaObject->classInfo(i);
        if (qstrcmp(key, classInfo.name()) == 0) {
            const char *elementName = classInfo.value();

            if (qstrcmp(elementName, "auto") == 0) {
                const char *strippedClassName = metaObject->className();
                for (const char *c = strippedClassName; *c != '\0'; c++) {
                    if (*c == ':')
                        strippedClassName = c + 1;
                }
                elementName = strippedClassName;
            } else if (qstrcmp(elementName, "anonymous") == 0) {
                if (elementNames.isEmpty())
                    elementNames.push_back(nullptr);
                else if (elementNames[0] != nullptr)
                    qWarning() << metaObject->className() << "is both anonymous and named";
                continue;
            }

            if (!elementNames.isEmpty() && elementNames[0] == nullptr) {
                qWarning() << metaObject->className() << "is both anonymous and named";
                elementNames[0] = elementName;
            } else {
                elementNames.push_back(elementName);
            }
        }
    }

    return elementNames;
}

struct AliasRegistrar
{
    AliasRegistrar(const ElementNames *elementNames) : elementNames(elementNames) {}

    void registerAliases(int typeId)
    {
        if (elementNames) {
            for (int i = 1, end = elementNames->length(); i < end; ++i)
                otherNames.append(QString::fromUtf8(elementNames->at(i)));
            elementNames = nullptr;
        }

        for (const QString &otherName : std::as_const(otherNames))
            QQmlMetaType::registerTypeAlias(typeId, otherName);
    }

private:
    const ElementNames *elementNames;
    QVarLengthArray<QString, 8> otherNames;
};


static void doRegisterTypeAndRevisions(
        const QQmlPrivate::RegisterTypeAndRevisions &type,
        const ElementNames &elementNames)
{
    using namespace QQmlPrivate;

    const bool isValueType = !(type.typeId.flags() & QMetaType::PointerToQObject);
    const bool creatable = (elementNames[0] != nullptr || isValueType)
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

    AliasRegistrar aliasRegistrar(&elementNames);
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
            typeRevision.elementName = elementNames[0];
            typeRevision.create = creatable ? type.create : nullptr;
            typeRevision.userdata = type.userdata;
        }

        typeRevision.customParser = type.customParserFactory();
        const int id = qmlregister(TypeRegistration, &typeRevision);
        if (type.qmlTypeIds)
            type.qmlTypeIds->append(id);

        if (typeRevision.elementName)
            aliasRegistrar.registerAliases(id);

        if (sequenceRevision.metaSequence != QMetaSequence()) {
            sequenceRevision.version = typeRevision.version;
            sequenceRevision.revision = typeRevision.revision;
            const int id = QQmlPrivate::qmlregister(
                    QQmlPrivate::SequentialContainerRegistration, &sequenceRevision);
            if (type.qmlTypeIds)
                type.qmlTypeIds->append(id);
        }
    }
}

static void doRegisterSingletonAndRevisions(
        const QQmlPrivate::RegisterSingletonTypeAndRevisions &type,
        const ElementNames &elementNames)
{
    using namespace QQmlPrivate;

    RegisterSingletonType revisionRegistration = {
        0,
        type.uri,
        type.version,
        elementNames[0],
        nullptr,
        type.qObjectApi,
        type.instanceMetaObject,
        type.typeId,
        type.extensionObjectCreate,
        type.extensionMetaObject,
        QTypeRevision()
    };
    const QQmlType::SingletonInstanceInfo::ConstPtr siinfo
            = singletonInstanceInfo(revisionRegistration);

    const QTypeRevision added = revisionClassInfo(
            type.classInfoMetaObject, "QML.AddedInVersion",
            QTypeRevision::fromVersion(type.version.majorVersion(), 0));
    const QTypeRevision removed = revisionClassInfo(
            type.classInfoMetaObject, "QML.RemovedInVersion");
    const QList<QTypeRevision> furtherRevisions = revisionClassInfos(type.classInfoMetaObject,
                                                                     "QML.ExtraVersion");

    auto revisions = prepareRevisions(type.instanceMetaObject, added) + furtherRevisions;
    uniqueRevisions(&revisions, type.version, added);

    AliasRegistrar aliasRegistrar(&elementNames);
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
            revisionRegistration.typeName = elementNames[0];
            revisionRegistration.qObjectApi = type.qObjectApi;
        }

        const int id = finalizeType(
                QQmlMetaType::registerSingletonType(revisionRegistration, siinfo));
        if (type.qmlTypeIds)
            type.qmlTypeIds->append(id);

        if (revisionRegistration.typeName)
            aliasRegistrar.registerAliases(id);
    }
}

/*
This method is "over generalized" to allow us to (potentially) register more types of things in
the future without adding exported symbols.
*/
int QQmlPrivate::qmlregister(RegistrationType type, void *data)
{
    switch (type) {
    case AutoParentRegistration:
        return QQmlMetaType::registerAutoParentFunction(
                *reinterpret_cast<RegisterAutoParent *>(data));
    case QmlUnitCacheHookRegistration:
        return QQmlMetaType::registerUnitCacheHook(
                *reinterpret_cast<RegisterQmlUnitCacheHook *>(data));
    case TypeAndRevisionsRegistration: {
        const RegisterTypeAndRevisions &type = *reinterpret_cast<RegisterTypeAndRevisions *>(data);
        if (type.structVersion > 1 && type.forceAnonymous) {
            doRegisterTypeAndRevisions(type, {nullptr});
        } else {
            const ElementNames names = classElementNames(type.classInfoMetaObject);
            if (names.isEmpty()) {
                qWarning().nospace() << "Missing QML.Element class info for "
                                     << type.classInfoMetaObject->className();
            } else {
                doRegisterTypeAndRevisions(type, names);
            }

        }
        break;
    }
    case SingletonAndRevisionsRegistration: {
        const RegisterSingletonTypeAndRevisions &type
                = *reinterpret_cast<RegisterSingletonTypeAndRevisions *>(data);
        const ElementNames names = classElementNames(type.classInfoMetaObject);
        if (names.isEmpty()) {
            qWarning().nospace() << "Missing QML.Element class info for "
                                 << type.classInfoMetaObject->className();
        } else {
            doRegisterSingletonAndRevisions(type, names);
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
        return finalizeType(
                QQmlMetaType::registerType(*reinterpret_cast<RegisterType *>(data)));
    case InterfaceRegistration:
        return finalizeType(
                QQmlMetaType::registerInterface(*reinterpret_cast<RegisterInterface *>(data)));
    case SingletonRegistration:
        return finalizeType(QQmlMetaType::registerSingletonType(
                *reinterpret_cast<RegisterSingletonType *>(data),
                singletonInstanceInfo(*reinterpret_cast<RegisterSingletonType *>(data))));
    case CompositeRegistration:
        return finalizeType(QQmlMetaType::registerCompositeType(
                *reinterpret_cast<RegisterCompositeType *>(data)));
    case CompositeSingletonRegistration:
        return finalizeType(QQmlMetaType::registerCompositeSingletonType(
                *reinterpret_cast<RegisterCompositeSingletonType *>(data),
                singletonInstanceInfo(*reinterpret_cast<RegisterCompositeSingletonType *>(data))));
    case SequentialContainerRegistration:
        return finalizeType(QQmlMetaType::registerSequentialContainer(
                *reinterpret_cast<RegisterSequentialContainer *>(data)));
    default:
        return -1;
    }

    return -1;
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

/*!
  \relates <qqml.h>

  This function registers a type in the QML system with the name \a qmlName, in the type namespace imported from \a uri having the
  version number composed from \a versionMajor and \a versionMinor, but any attempt to instantiate the type
  will produce the given error \a message.

  Normally, the types exported by a plugin should be fixed. However, if a C++ type is not available, you should
  at least "reserve" the QML type name, and give the user of the unavailable type a meaningful error message.

  Returns the QML type id.

  Example:

  \code
  #ifdef NO_GAMES_ALLOWED
  qmlRegisterTypeNotAvailable("MinehuntCore", 0, 1, "Game", "Get back to work, slacker!");
  #else
  qmlRegisterType<MinehuntGame>("MinehuntCore", 0, 1, "Game");
  #endif
  \endcode

  This will cause any QML which imports the "MinehuntCore" type namespace and attempts to use the type to produce an error message:
  \code
  fun.qml: Get back to work, slacker!
     Game {
     ^
  \endcode

  Without this, a generic "Game is not a type" message would be given.

  \sa QML_UNAVAILABLE, qmlRegisterUncreatableType(),
      {Choosing the Correct Integration Method Between C++ and QML}
*/
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

struct LookupNotInitialized {};

QObject *AOTCompiledContext::thisObject() const
{
    return static_cast<QV4::MetaTypesStackFrame *>(engine->handle()->currentStackFrame)
            ->thisObject();
}

QQmlEngine *AOTCompiledContext::qmlEngine() const
{
    return engine->handle()->qmlEngine();
}

static QQmlPropertyCapture *propertyCapture(const AOTCompiledContext *aotContext)
{
    QQmlEngine *engine = aotContext->qmlEngine();
    return engine ? QQmlEnginePrivate::get(aotContext->qmlEngine())->propertyCapture : nullptr;
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

static void captureFallbackProperty(
        QObject *object, int coreIndex, int notifyIndex, bool isConstant,
        const AOTCompiledContext *aotContext)
{
    if (isConstant)
        return;

    if (QQmlPropertyCapture *capture = propertyCapture(aotContext))
        capture->captureProperty(object, coreIndex, notifyIndex);
}

static void captureObjectProperty(
        QObject *object, const QQmlPropertyCache *propertyCache,
        const QQmlPropertyData *property, const AOTCompiledContext *aotContext)
{
    if (property->isConstant())
        return;

    if (QQmlPropertyCapture *capture = propertyCapture(aotContext))
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

enum class PropertyResult { OK, NeedsInit, Deleted };

struct ObjectPropertyQmlData
{
    QQmlData *qmlData;
    PropertyResult result;
};

template<bool StrictType>
ObjectPropertyQmlData findObjectPropertyQmlData(QV4::Lookup *lookup, QObject *object)
{
    QQmlData *qmlData = QQmlData::get(object);
    if (!qmlData)
        return {qmlData, PropertyResult::NeedsInit};
    if (qmlData->isQueuedForDeletion)
        return {qmlData, PropertyResult::Deleted};
    Q_ASSERT(!QQmlData::wasDeleted(object));
    const QQmlPropertyCache *propertyCache = lookup->qobjectLookup.propertyCache;
    if (StrictType) {
        if (qmlData->propertyCache.data() != propertyCache)
            return {qmlData, PropertyResult::NeedsInit};
    } else if (!inherits(qmlData->propertyCache.data(), propertyCache)) {
        return {qmlData, PropertyResult::NeedsInit};
    }
    return {qmlData, PropertyResult::OK};
}

template<bool StrictType = false>
PropertyResult loadObjectProperty(
        QV4::Lookup *lookup, QObject *object, void *target, const AOTCompiledContext *aotContext)
{
    const ObjectPropertyQmlData data = findObjectPropertyQmlData<StrictType>(lookup, object);
    if (data.result != PropertyResult::OK)
        return data.result;

    const QQmlPropertyData *propertyData = lookup->qobjectLookup.propertyData;
    const int coreIndex = propertyData->coreIndex();
    if (data.qmlData->hasPendingBindingBit(coreIndex))
        data.qmlData->flushPendingBinding(coreIndex);

    captureObjectProperty(object, lookup->qobjectLookup.propertyCache, propertyData, aotContext);
    propertyData->readProperty(object, target);
    return PropertyResult::OK;
}

template<bool StrictType = false>
PropertyResult writeBackObjectProperty(QV4::Lookup *lookup, QObject *object, void *source)
{
    const ObjectPropertyQmlData data = findObjectPropertyQmlData<StrictType>(lookup, object);
    if (data.result != PropertyResult::OK)
        return data.result;

    lookup->qobjectLookup.propertyData->writeProperty(object, source, {});
    return PropertyResult::OK;
}

struct FallbackPropertyQmlData
{
    QQmlData *qmlData;
    const QMetaObject *metaObject;
    PropertyResult result;
};

static FallbackPropertyQmlData findFallbackPropertyQmlData(QV4::Lookup *lookup, QObject *object)
{
    QQmlData *qmlData = QQmlData::get(object);
    if (qmlData && qmlData->isQueuedForDeletion)
        return {qmlData, nullptr, PropertyResult::Deleted};

    Q_ASSERT(!QQmlData::wasDeleted(object));

    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(lookup->qobjectFallbackLookup.metaObject - 1);
    if (!metaObject || metaObject != object->metaObject())
        return {qmlData, nullptr, PropertyResult::NeedsInit};

    return {qmlData, metaObject, PropertyResult::OK};
}

static PropertyResult loadFallbackProperty(
        QV4::Lookup *lookup, QObject *object, void *target, const AOTCompiledContext *aotContext)
{
    const FallbackPropertyQmlData data = findFallbackPropertyQmlData(lookup, object);
    if (data.result != PropertyResult::OK)
        return data.result;

    const int coreIndex = lookup->qobjectFallbackLookup.coreIndex;
    if (data.qmlData && data.qmlData->hasPendingBindingBit(coreIndex))
        data.qmlData->flushPendingBinding(coreIndex);

    captureFallbackProperty(object, coreIndex, lookup->qobjectFallbackLookup.notifyIndex,
                            lookup->qobjectFallbackLookup.isConstant, aotContext);

    void *a[] = { target, nullptr };
    data.metaObject->metacall(object, QMetaObject::ReadProperty, coreIndex, a);

    return PropertyResult::OK;
}

static PropertyResult writeBackFallbackProperty(QV4::Lookup *lookup, QObject *object, void *source)
{
    const FallbackPropertyQmlData data = findFallbackPropertyQmlData(lookup, object);
    if (data.result != PropertyResult::OK)
        return data.result;

    void *a[] = { source, nullptr };
    data.metaObject->metacall(
            object, QMetaObject::WriteProperty, lookup->qobjectFallbackLookup.coreIndex, a);

    return PropertyResult::OK;
}

PropertyResult loadObjectAsVariant(
        QV4::Lookup *lookup, QObject *object, void *target, const AOTCompiledContext *aotContext)
{
    QVariant *variant = static_cast<QVariant *>(target);
    const QMetaType propType = lookup->qobjectLookup.propertyData->propType();
    if (propType == QMetaType::fromType<QVariant>())
        return loadObjectProperty<true>(lookup, object, variant, aotContext);

    *variant = QVariant(propType);
    return loadObjectProperty<true>(lookup, object, variant->data(), aotContext);
}

PropertyResult writeBackObjectAsVariant(QV4::Lookup *lookup, QObject *object, void *source)
{
    QVariant *variant = static_cast<QVariant *>(source);
    const QMetaType propType = lookup->qobjectLookup.propertyData->propType();
    if (propType == QMetaType::fromType<QVariant>())
        return writeBackObjectProperty<true>(lookup, object, variant);

    Q_ASSERT(variant->metaType() == propType);
    return writeBackObjectProperty<true>(lookup, object, variant->data());
}

PropertyResult loadFallbackAsVariant(
        QV4::Lookup *lookup, QObject *object, void *target, const AOTCompiledContext *aotContext)
{
    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(lookup->qobjectFallbackLookup.metaObject - 1);
    Q_ASSERT(metaObject);

    QVariant *variant = static_cast<QVariant *>(target);
    const QMetaType propType = metaObject->property(lookup->qobjectFallbackLookup.coreIndex).metaType();
    if (propType == QMetaType::fromType<QVariant>())
        return loadFallbackProperty(lookup, object, variant, aotContext);

    *variant = QVariant(propType);
    return loadFallbackProperty(lookup, object, variant->data(), aotContext);
}

PropertyResult writeBackFallbackAsVariant(QV4::Lookup *lookup, QObject *object, void *source)
{
    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(lookup->qobjectFallbackLookup.metaObject - 1);
    Q_ASSERT(metaObject);

    QVariant *variant = static_cast<QVariant *>(source);
    const QMetaType propType = metaObject->property(lookup->qobjectFallbackLookup.coreIndex).metaType();
    if (propType == QMetaType::fromType<QVariant>())
        return writeBackFallbackProperty(lookup, object, variant);

    Q_ASSERT(variant->metaType() == propType);
    return writeBackFallbackProperty(lookup, object, variant->data());
}

template<bool StrictType, typename Op>
static PropertyResult changeObjectProperty(QV4::Lookup *lookup, QObject *object, Op op)
{
    const ObjectPropertyQmlData data = findObjectPropertyQmlData<StrictType>(lookup, object);
    if (data.result != PropertyResult::OK)
        return data.result;

    const QQmlPropertyData *property = lookup->qobjectLookup.propertyData;
    QQmlPropertyPrivate::removeBinding(object, QQmlPropertyIndex(property->coreIndex()));
    op(property);
    return PropertyResult::OK;
}

template<bool StrictType = false>
static PropertyResult resetObjectProperty(
        QV4::Lookup *l, QObject *object, QV4::ExecutionEngine *v4)
{
    return changeObjectProperty<StrictType>(l, object, [&](const QQmlPropertyData *property) {
        if (property->isResettable()) {
            property->resetProperty(object, {});
        } else {
            v4->throwError(
                    QLatin1String("Cannot assign [undefined] to ") +
                    QLatin1String(property->propType().name()));
        }
    });
}

template<bool StrictType = false>
static PropertyResult storeObjectProperty(QV4::Lookup *l, QObject *object, void *value)
{
    return changeObjectProperty<StrictType>(l, object, [&](const QQmlPropertyData *property) {
        property->writeProperty(object, value, {});
    });
}

template<typename Op>
static PropertyResult changeFallbackProperty(QV4::Lookup *lookup, QObject *object, Op op)
{
    const FallbackPropertyQmlData data = findFallbackPropertyQmlData(lookup, object);
    if (data.result != PropertyResult::OK)
        return data.result;

    const int coreIndex = lookup->qobjectFallbackLookup.coreIndex;
    QQmlPropertyPrivate::removeBinding(object, QQmlPropertyIndex(coreIndex));

    op(data.metaObject, coreIndex);
    return PropertyResult::OK;
}

static PropertyResult storeFallbackProperty(QV4::Lookup *l, QObject *object, void *value)
{
    return changeFallbackProperty(l, object, [&](const QMetaObject *metaObject, int coreIndex) {
        void *args[] = { value, nullptr };
        metaObject->metacall(object, QMetaObject::WriteProperty, coreIndex, args);
    });
}

static PropertyResult resetFallbackProperty(
        QV4::Lookup *l, QObject *object, const QMetaProperty *property, QV4::ExecutionEngine *v4)
{
    return changeFallbackProperty(l, object, [&](const QMetaObject *metaObject, int coreIndex) {
        if (property->isResettable()) {
            void *args[] = { nullptr };
            metaObject->metacall(object, QMetaObject::ResetProperty, coreIndex, args);
        } else {
            v4->throwError(
                    QLatin1String("Cannot assign [undefined] to ") +
                    QLatin1String(property->typeName()));
        }
    });
}

static bool isEnumUnderlyingType(QMetaType enumType, QMetaType numberType)
{
    // You can pass the underlying type of an enum.
    // We don't want to check for the actual underlying type because
    // moc and qmltyperegistrar are not very precise about it. Especially
    // the long and longlong types can be ambiguous.

    const bool isUnsigned = enumType.flags() & QMetaType::IsUnsignedEnumeration;
    switch (enumType.sizeOf()) {
    case 1:
        return isUnsigned
                ? numberType == QMetaType::fromType<quint8>()
                : numberType == QMetaType::fromType<qint8>();
    case 2:
        return isUnsigned
                ? numberType == QMetaType::fromType<ushort>()
                : numberType == QMetaType::fromType<short>();
    case 4:
        // The default type, if moc doesn't know the actual enum type, is int.
        // However, the compiler can still decide to encode the enum in uint.
        // Therefore, we also accept int for uint enums.
        // TODO: This is technically UB.
        return isUnsigned
                ? (numberType == QMetaType::fromType<int>()
                   || numberType == QMetaType::fromType<uint>())
                : numberType == QMetaType::fromType<int>();
    case 8:
        return isUnsigned
                ? numberType == QMetaType::fromType<qulonglong>()
                : numberType == QMetaType::fromType<qlonglong>();
    }

    return false;
}

static bool canHoldVoid(QMetaType type)
{
    // We cannot directly store void, but we can put it into QVariant or QJSPrimitiveValue
    return !type.isValid()
            || type == QMetaType::fromType<QVariant>()
            || type == QMetaType::fromType<QJSPrimitiveValue>();
}

static bool isTypeCompatible(QMetaType source, QMetaType target)
{
    if (source == target)
        return true;

    if ((source.flags() & QMetaType::IsQmlList)
               && (target.flags() & QMetaType::IsQmlList)) {
        // We want to check the value types here, but we cannot easily do it.
        // Internally those are all QObject* lists, though.
        return true;
    }

    if (target.flags() & QMetaType::PointerToQObject) {
        // We accept any derived class, too

        const QMetaObject *targetMetaObject = target.metaObject();
        const QMetaObject *sourceMetaObject = source.metaObject();
        if (!sourceMetaObject)
            sourceMetaObject = QQmlMetaType::metaObjectForType(source).metaObject();

        while (sourceMetaObject && sourceMetaObject != targetMetaObject)
            sourceMetaObject = sourceMetaObject->superClass();

        return sourceMetaObject != nullptr;
    }

    if (target.flags() & QMetaType::IsEnumeration)
        return isEnumUnderlyingType(target, source);

    if (source.flags() & QMetaType::IsEnumeration)
        return isEnumUnderlyingType(source, target);

    if (!source.isValid())
        return canHoldVoid(target);

    return false;
}

static PropertyResult storeObjectAsVariant(
        QV4::ExecutionEngine *v4, QV4::Lookup *lookup, QObject *object, void *value)
{
    QVariant *variant = static_cast<QVariant *>(value);
    const QMetaType propType = lookup->qobjectLookup.propertyData->propType();
    if (propType == QMetaType::fromType<QVariant>())
        return storeObjectProperty<true>(lookup, object, variant);

    if (!variant->isValid())
        return resetObjectProperty<true>(lookup, object, v4);

    const QMetaType variantMetaType = variant->metaType();

    if (isTypeCompatible(variantMetaType, propType))
        return storeObjectProperty<true>(lookup, object, variant->data());

    QVariant converted(propType);
    QV4::Scope scope(v4);
    QV4::ScopedValue val(scope, v4->fromVariant(*variant));
    if (v4->metaTypeFromJS(val, propType, converted.data())
            || QMetaType::convert(
                variantMetaType, variant->constData(), propType, converted.data())) {
        return storeObjectProperty<true>(lookup, object, converted.data());
    }

    v4->throwError(
            QLatin1String("Cannot assign ") + QLatin1String(variantMetaType.name())
            + QLatin1String(" to ") + QLatin1String(propType.name()));

    return PropertyResult::NeedsInit;
}

static PropertyResult storeFallbackAsVariant(
        QV4::ExecutionEngine *v4, QV4::Lookup *lookup, QObject *object, void *value)
{
    QVariant *variant = static_cast<QVariant *>(value);

    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(lookup->qobjectFallbackLookup.metaObject - 1);
    Q_ASSERT(metaObject);

    const QMetaProperty property = metaObject->property(lookup->qobjectFallbackLookup.coreIndex);
    const QMetaType propType = property.metaType();
    if (propType == QMetaType::fromType<QVariant>())
        return storeFallbackProperty(lookup, object, variant);

    if (!variant->isValid())
        return resetFallbackProperty(lookup, object, &property, v4);

    if (isTypeCompatible(variant->metaType(), propType))
        return storeFallbackProperty(lookup, object, variant->data());

    QVariant converted(propType);
    QV4::Scope scope(v4);
    QV4::ScopedValue val(scope, v4->fromVariant(*variant));
    if (v4->metaTypeFromJS(val, propType, converted.data())
            || QMetaType::convert(
                variant->metaType(), variant->constData(), propType, converted.data())) {
        return storeFallbackProperty(lookup, object, converted.data());
    }

    return PropertyResult::NeedsInit;
}

enum class ObjectLookupResult {
    Failure,
    Object,
    Fallback,
};

static ObjectLookupResult initObjectLookup(
        const AOTCompiledContext *aotContext, QV4::Lookup *lookup, QObject *object)
{
    QV4::Scope scope(aotContext->engine->handle());
    QV4::PropertyKey id = scope.engine->identifierTable->asPropertyKey(
                aotContext->compilationUnit->runtimeStrings[lookup->nameIndex]);

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

    if (!property) {
        const QMetaObject *metaObject = object->metaObject();
        if (!metaObject)
            return ObjectLookupResult::Failure;

        const int coreIndex = metaObject->indexOfProperty(
                    name->toQStringNoThrow().toUtf8().constData());
        if (coreIndex < 0)
            return ObjectLookupResult::Failure;

        const QMetaProperty property = metaObject->property(coreIndex);

        lookup->releasePropertyCache();
        // & 1 to tell the gc that this is not heap allocated; see markObjects in qv4lookup_p.h
        lookup->qobjectFallbackLookup.metaObject = quintptr(metaObject) + 1;
        lookup->qobjectFallbackLookup.coreIndex = coreIndex;
        lookup->qobjectFallbackLookup.notifyIndex =
                QMetaObjectPrivate::signalIndex(property.notifySignal());
        lookup->qobjectFallbackLookup.isConstant = property.isConstant() ? 1 : 0;
        return ObjectLookupResult::Fallback;
    }

    Q_ASSERT(ddata->propertyCache);

    QV4::setupQObjectLookup(lookup, ddata, property);

    return ObjectLookupResult::Object;
}

enum class LookupType { Concrete, Variant};
template<QV4::Lookup::Call ObjectCall, QV4::Lookup::Call FallbackCall, LookupType Type>
void initObjectLookup(const AOTCompiledContext *aotContext, uint index, QObject *object)
{
    QV4::ExecutionEngine *v4 = aotContext->engine->handle();
    if (v4->hasException) {
        v4->amendException();
        return;
    }

    QV4::Lookup *lookup = aotContext->compilationUnit->runtimeLookups + index;
    switch (initObjectLookup(aotContext, lookup, object)) {
    case ObjectLookupResult::Object:
        lookup->call = ObjectCall;
        lookup->asVariant = (Type == LookupType::Variant);
        break;
    case ObjectLookupResult::Fallback:
        lookup->call = FallbackCall;
        lookup->asVariant = (Type == LookupType::Variant);
        break;
    case ObjectLookupResult::Failure:
        v4->throwTypeError();
        break;
    }

    return;
}


static void initValueLookup(
        QV4::Lookup *lookup, QV4::ExecutableCompilationUnit *compilationUnit,
        const QMetaObject *metaObject)
{
    Q_ASSERT(metaObject);
    const QByteArray name = compilationUnit->runtimeStrings[lookup->nameIndex]->toQString().toUtf8();
    const int coreIndex = metaObject->indexOfProperty(name.constData());
    QMetaType lookupType = metaObject->property(coreIndex).metaType();
    lookup->qgadgetLookup.metaObject = quintptr(metaObject) + 1;
    lookup->qgadgetLookup.coreIndex = coreIndex;
    lookup->qgadgetLookup.metaType = lookupType.iface();
}

bool AOTCompiledContext::captureLookup(uint index, QObject *object) const
{
    if (!object)
        return false;

    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    switch (lookup->call) {
    case QV4::Lookup::Call::GetterSingletonProperty:
    case QV4::Lookup::Call::GetterQObjectProperty: {
        const QQmlPropertyData *property = lookup->qobjectLookup.propertyData;
        QQmlData::flushPendingBinding(object, property->coreIndex());
        captureObjectProperty(object, lookup->qobjectLookup.propertyCache, property, this);
        return true;
    }
    case QV4::Lookup::Call::GetterQObjectPropertyFallback: {
        const int coreIndex = lookup->qobjectFallbackLookup.coreIndex;
        QQmlData::flushPendingBinding(object, coreIndex);
        captureFallbackProperty(
                    object, coreIndex, lookup->qobjectFallbackLookup.notifyIndex,
                    lookup->qobjectFallbackLookup.isConstant, this);
        return true;
    }
    default:
        break;
    }

    return false;
}

bool AOTCompiledContext::captureQmlContextPropertyLookup(uint index) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    switch (lookup->call) {
    case QV4::Lookup::Call::ContextGetterScopeObjectProperty: {
        const QQmlPropertyData *property = lookup->qobjectLookup.propertyData;
        QQmlData::flushPendingBinding(qmlScopeObject, property->coreIndex());
        captureObjectProperty(qmlScopeObject, lookup->qobjectLookup.propertyCache, property, this);
        return true;
    }
    case QV4::Lookup::Call::ContextGetterScopeObjectPropertyFallback: {
        const int coreIndex = lookup->qobjectFallbackLookup.coreIndex;
        QQmlData::flushPendingBinding(qmlScopeObject, coreIndex);
        captureFallbackProperty(qmlScopeObject, coreIndex, lookup->qobjectFallbackLookup.notifyIndex,
                                lookup->qobjectFallbackLookup.isConstant, this);
        return true;
    }
    default:
        break;
    }

    return false;
}

void AOTCompiledContext::captureTranslation() const
{
    if (QQmlPropertyCapture *capture = propertyCapture(this))
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
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    switch (lookup->call) {
    case QV4::Lookup::Call::ContextGetterScopeObjectProperty:
    case QV4::Lookup::Call::GetterSingletonProperty:
    case QV4::Lookup::Call::GetterQObjectProperty:
        return lookup->qobjectLookup.propertyData->propType();
    case QV4::Lookup::Call::GetterValueTypeProperty:
        return QMetaType(lookup->qgadgetLookup.metaType);
    case QV4::Lookup::Call::GetterEnumValue:
        return QMetaType(lookup->qmlEnumValueLookup.metaType);
    case QV4::Lookup::Call::ContextGetterIdObject:
    case QV4::Lookup::Call::ContextGetterType:
    case QV4::Lookup::Call::ContextGetterSingleton:
    case QV4::Lookup::Call::GetterQObjectAttached:
        return QMetaType::fromType<QObject *>();
    case QV4::Lookup::Call::GetterQObjectPropertyFallback:
    case QV4::Lookup::Call::ContextGetterScopeObjectPropertyFallback: {
        const QMetaObject *metaObject
                = reinterpret_cast<const QMetaObject *>(lookup->qobjectFallbackLookup.metaObject - 1);
        const int coreIndex = lookup->qobjectFallbackLookup.coreIndex;
        return metaObject->property(coreIndex).metaType();
    }
    case QV4::Lookup::Call::GetterQObjectMethod:
    case QV4::Lookup::Call::GetterQObjectMethodFallback:
    case QV4::Lookup::Call::ContextGetterScopeObjectMethod:
        switch (lookup->qobjectMethodLookup.method->index) {
        case QV4::QObjectMethod::DestroyMethod:
            return QMetaType::fromType<void>();
        case QV4::QObjectMethod::ToStringMethod:
            return QMetaType::fromType<QString>();
        default:
            return lookup->qobjectMethodLookup.propertyData->propType();
        }
    default:
        break;
    }

    return QMetaType::fromType<LookupNotInitialized>();
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

    const auto unwrapVariant = [&]() {
        if (type == QMetaType::fromType<QVariant>()) {
            QVariant *variant = static_cast<QVariant *>(value);
            type = variant->metaType();
            value = variant->data();
        }
    };

    QV4::Lookup lookup;
    memset(&lookup, 0, sizeof(QV4::Lookup));
    lookup.nameIndex = nameIndex;
    lookup.forCall = false;
    PropertyResult storeResult = PropertyResult::NeedsInit;
    QMetaType propType;

    switch (initObjectLookup(this, &lookup, qmlScopeObject)) {
    case ObjectLookupResult::Object: {
        propType = lookup.qobjectLookup.propertyData->propType();
        if (isTypeCompatible(type, propType)) {
            storeResult = storeObjectProperty(&lookup, qmlScopeObject, value);
        } else if (isUndefined(value, type)) {

            // NB: In order to have a meaningful reset() here, the type needs to be a wrapper type
            //     that can hold undefined. For example QVariant. The caller must not unwrap it.

            storeResult = resetObjectProperty(&lookup, qmlScopeObject, engine->handle());
        } else {

            // Unwrap any QVariant so that we get a meaningful conversion below.
            unwrapVariant();

            QVariant var(propType);
            QV4::ExecutionEngine *v4 = engine->handle();
            QV4::Scope scope(v4);
            QV4::ScopedValue val(scope, v4->metaTypeToJS(type, value));
            if (v4->metaTypeFromJS(val, propType, var.data())
                    || QMetaType::convert(type, value, propType, var.data())) {
                storeResult = storeObjectProperty(&lookup, qmlScopeObject, var.data());
            }
        }

        lookup.qobjectLookup.propertyCache->release();
        break;
    }
    case ObjectLookupResult::Fallback: {
        const QMetaObject *metaObject
                = reinterpret_cast<const QMetaObject *>(lookup.qobjectFallbackLookup.metaObject - 1);
        const QMetaProperty property = metaObject->property(lookup.qobjectFallbackLookup.coreIndex);
        propType = property.metaType();
        if (isTypeCompatible(type, propType)) {
            storeResult = storeFallbackProperty(&lookup, qmlScopeObject, value);
        } else if (isUndefined(value, type)) {

            // NB: In order to have a meaningful reset() here, the type needs to be a wrapper type
            //     that can hold undefined. For example QVariant. The caller must not unwrap it.
            storeResult = resetFallbackProperty(
                    &lookup, qmlScopeObject, &property, engine->handle());

        } else {

            // Unwrap any QVariant so that we get a meaningful conversion below.
            unwrapVariant();

            QVariant var(propType);
            QV4::ExecutionEngine *v4 = engine->handle();
            QV4::Scope scope(v4);
            QV4::ScopedValue val(scope, v4->metaTypeToJS(type, value));
            if (v4->metaTypeFromJS(val, propType, var.data())
                    || QMetaType::convert(type, value, propType, var.data())) {
                storeResult = storeFallbackProperty(&lookup, qmlScopeObject, var.data());
            }
        }
        break;
    }
    case ObjectLookupResult::Failure:
        engine->handle()->throwTypeError();
        return;
    }

    switch (storeResult) {
    case PropertyResult::NeedsInit: {
        const QString error = QLatin1String("Cannot assign ") +
                QLatin1String(type.name()) +
                QLatin1String(" to ") +
                QLatin1String(propType.name());
        engine->throwError(error);
        break;
    }
    case PropertyResult::Deleted:
        engine->handle()->throwTypeError(
                    QStringLiteral("Value is null and could not be converted to an object"));
        break;
    case PropertyResult::OK:
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
        // turns out to be a QQmlLoggingCategoryBase at run time.
        if (QQmlLoggingCategoryBase *qQmlLoggingCategory
                = qobject_cast<QQmlLoggingCategoryBase *>(wrapper)) {
            const QLoggingCategory *loggingCategory = qQmlLoggingCategory->category();
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
        int ctorIndex, void **args) const
{
    return QQmlValueTypeProvider::constructValueType(
            resultMetaType, resultMetaObject, ctorIndex, args);
}

QDateTime AOTCompiledContext::constructDateTime(double timestamp) const
{
    return QV4::DateObject::timestampToDateTime(timestamp);
}

QDateTime AOTCompiledContext::constructDateTime(const QString &string) const
{
    return QV4::DateObject::stringToDateTime(string, engine->handle());
}

QDateTime AOTCompiledContext::constructDateTime(
        double year, double month, double day, double hours,
        double minutes, double seconds, double msecs) const
{
    return constructDateTime(QV4::DateObject::componentsToTimestamp(
            year, month, day, hours, minutes, seconds, msecs, engine->handle()));
}

static QMetaType jsTypedFunctionArgument(
        const QQmlType &type, const QV4::CompiledData::ParameterType &parameter)
{
    return parameter.isList() ? type.qListTypeId() : type.typeId();
}

static bool callQObjectMethodWithTypes(
        QV4::ExecutionEngine *engine, QV4::Lookup *lookup, QObject *thisObject, void **args,
        QMetaType *types, int argc)
{
    QV4::Scope scope(engine);
    QV4::Scoped<QV4::QObjectMethod> function(scope, lookup->qobjectMethodLookup.method);
    Q_ASSERT(function);
    function->call(thisObject, args, types, argc);
    return !scope.hasException();
}

static bool callQObjectMethodAsVariant(
        QV4::ExecutionEngine *engine, QV4::Lookup *lookup,
        QObject *thisObject, void **args, int argc)
{
    // We need to re-fetch the method on every call because it can be shadowed.

    QV4::Scope scope(engine);
    QV4::ScopedValue wrappedObject(scope, QV4::QObjectWrapper::wrap(scope.engine, thisObject));
    QV4::ScopedFunctionObject function(scope, lookup->getter(scope.engine, wrappedObject));
    Q_ASSERT(function);
    Q_ASSERT(lookup->asVariant); // The getter mustn't reset the isVariant flag

    Q_ALLOCA_VAR(QMetaType, types, (argc + 1) * sizeof(QMetaType));
    std::fill(types, types + argc + 1, QMetaType::fromType<QVariant>());

    function->call(thisObject, args, types, argc);
    return !scope.hasException();
}

static bool callQObjectMethod(
        QV4::ExecutionEngine *engine, QV4::Lookup *lookup,
        QObject *thisObject, void **args, int argc)
{
    Q_ASSERT(!lookup->asVariant);

    Q_ALLOCA_VAR(QMetaType, types, (argc + 1) * sizeof(QMetaType));
    if (const QQmlPropertyData *propertyData = lookup->qobjectMethodLookup.propertyData) {
        const QMetaMethod method = propertyData->metaMethod();
        Q_ASSERT(argc == method.parameterCount());
        types[0] = method.returnMetaType();
        for (int i = 0; i < argc; ++i)
            types[i + 1] = method.parameterMetaType(i);

        return callQObjectMethodWithTypes(engine, lookup, thisObject, args, types, argc);
    }

    QV4::Scope scope(engine);
    QV4::Scoped<QV4::QObjectMethod> method(scope, lookup->qobjectMethodLookup.method);
    Q_ASSERT(method);
    switch (method->methodIndex()) {
    case QV4::QObjectMethod::DestroyMethod:
        return method->method_destroy(
                engine, thisObject, argc > 0 ?  *static_cast<int *>(args[1]) : 0);
    case QV4::QObjectMethod::ToStringMethod: {
        QString result = QV4::QObjectWrapper::objectToString(
                engine, thisObject ? thisObject->metaObject() : method->d()->metaObject(),
                thisObject);
        if (void *returnValue = args[0])
            *static_cast<QString *>(returnValue) = std::move(result);
        return true;
    }
    default:
        break;
    }
    return false;
}

static bool callArrowFunction(
        QV4::ExecutionEngine *engine, QV4::ArrowFunction *function,
        QObject *thisObject, void **args, int argc)
{
    QV4::Function *v4Function = function->function();
    Q_ASSERT(v4Function);
    Q_ASSERT(v4Function->nFormals == argc);

    switch (v4Function->kind) {
    case QV4::Function::AotCompiled: {
        const auto *types = v4Function->aotCompiledFunction.types.data();
        function->call(thisObject, args, types, argc);
        return !engine->hasException;
    }
    case QV4::Function::JsTyped: {
        const auto *compiledFunction = v4Function->compiledFunction;
        const QV4::CompiledData::Parameter *formals
                = v4Function->compiledFunction->formalsTable();

        Q_ALLOCA_VAR(QMetaType, types, (argc + 1) * sizeof(QMetaType));
        types[0] = jsTypedFunctionArgument(
                v4Function->jsTypedFunction.types[0], compiledFunction->returnType);
        for (qsizetype i = 0; i != argc; ++i) {
            types[i + 1] = jsTypedFunctionArgument(
                    v4Function->jsTypedFunction.types[i + 1], formals[i].type);
        }

        function->call(thisObject, args, types, argc);
        return !engine->hasException;
    }
    case QV4::Function::JsUntyped: {
        // We can call untyped functions if we're not expecting a specific return value and don't
        // have to pass any arguments. The compiler verifies this.
        Q_ASSERT(argc == 0);
        QMetaType variantType = QMetaType::fromType<QVariant>();
        function->call(thisObject, args, &variantType, 0);
        return !engine->hasException;
    }
    case QV4::Function::Eval:
        break;
    }

    Q_UNREACHABLE_RETURN(false);
}

static bool callArrowFunctionAsVariant(
        QV4::ExecutionEngine *engine, QV4::ArrowFunction *function,
        QObject *thisObject, void **args, int argc)
{
    QV4::Function *v4Function = function->function();
    Q_ASSERT(v4Function);

    switch (v4Function->kind) {
    case QV4::Function::JsUntyped:
        // We cannot assert anything here because the method can be shadowed.
        // That's why we wrap everything in QVariant.
    case QV4::Function::AotCompiled:
    case QV4::Function::JsTyped: {
        Q_ALLOCA_VAR(QMetaType, types, (argc + 1) * sizeof(QMetaType));
        std::fill(types, types + argc + 1, QMetaType::fromType<QVariant>());
        function->call(thisObject, args, types, argc);
        return !engine->hasException;
    }
    case QV4::Function::Eval:
        break;
    }

    Q_UNREACHABLE_RETURN(false);
}

bool AOTCompiledContext::callQmlContextPropertyLookup(uint index, void **args, int argc) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;

    if (lookup->call == QV4::Lookup::Call::ContextGetterScopeObjectMethod)
        return callQObjectMethod(engine->handle(), lookup, qmlScopeObject, args, argc);

    const auto doCall = [&](auto &&call) {
        QV4::Scope scope(engine->handle());
        QV4::ScopedValue undefined(scope);
        QV4::Scoped<QV4::ArrowFunction> function(
                scope, lookup->contextGetter(scope.engine, undefined));
        Q_ASSERT(function);
        return call(scope.engine, function, qmlScopeObject, args, argc);
    };

    if (lookup->call == QV4::Lookup::Call::ContextGetterScopeObjectProperty)
        return doCall(&callArrowFunction);

    return false;
}

enum MatchScore { NoMatch, VariantMatch, ExactMatch, };

static MatchScore resolveQObjectMethodOverload(
        QV4::QObjectMethod *method, QV4::Lookup *lookup, int relativeMethodIndex)
{
    Q_ASSERT(lookup->qobjectMethodLookup.method.get() == method->d());

    const auto *d = method->d();
    const int methodCount = d->methodCount;

    if (relativeMethodIndex == -1 && methodCount == 1) {
        // QML-declared signals do not have a meaningful method index and cannot be overloaded.
        // They still show up as QObjectMethod rather than ArrowFunction. If they didn't, we
        // wouldn't have to care.
        Q_ASSERT(d->methods[0].metaMethod().methodType() == QMetaMethod::Signal);
        lookup->qobjectMethodLookup.propertyData = d->methods;
        return ExactMatch;
    }

    for (int i = 0, end = d->methodCount; i != end; ++i) {
        const QMetaMethod metaMethod = d->methods[i].metaMethod();
        if (metaMethod.relativeMethodIndex() != relativeMethodIndex)
            continue;

        lookup->qobjectMethodLookup.propertyData = d->methods + i;
        return ExactMatch;
    }

    return NoMatch;
}

static bool tryEnsureMethodsCache(QV4::QObjectMethod *method, QObject *object)
{
    QV4::Heap::QObjectMethod *d = method->d();
    switch (method->methodIndex()) {
    case QV4::QObjectMethod::DestroyMethod:
    case QV4::QObjectMethod::ToStringMethod:
        return false;
    default:
        break;
    }

    d->ensureMethodsCache(object->metaObject());
    return true;
}

void AOTCompiledContext::initCallQmlContextPropertyLookup(uint index, int relativeMethodIndex) const
{
    if (engine->hasError()) {
        engine->handle()->amendException();
        return;
    }

    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue thisObject(scope);
    QV4::ScopedFunctionObject function(
                scope, lookup->contextGetter(scope.engine, thisObject));
    if (auto *method = function->as<QV4::QObjectMethod>()) {
        Q_ASSERT(lookup->call == QV4::Lookup::Call::ContextGetterScopeObjectMethod);
        if (tryEnsureMethodsCache(method, qmlScopeObject)) {
            const auto match = resolveQObjectMethodOverload(method, lookup, relativeMethodIndex);
            Q_ASSERT(match == ExactMatch);
        }
        return;
    }

    if (function->as<QV4::ArrowFunction>()) {
        // Can't have overloads of JavaScript functions.
        Q_ASSERT(lookup->call == QV4::Lookup::Call::ContextGetterScopeObjectProperty);
        return;
    }

    scope.engine->throwTypeError(
            QStringLiteral("Property '%1' of object [null] is not a function").arg(
                    compilationUnit->runtimeStrings[lookup->nameIndex]->toQString()));
}

bool AOTCompiledContext::loadContextIdLookup(uint index, void *target) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    int objectId = -1;
    QQmlContextData *context = nullptr;
    Q_ASSERT(qmlContext);

    switch (lookup->call) {
    case QV4::Lookup::Call::ContextGetterIdObject:
        objectId = lookup->qmlContextIdObjectLookup.objectId;
        context = qmlContext;
        break;
    case QV4::Lookup::Call::ContextGetterIdObjectInParentContext: {
        QV4::Scope scope(engine->handle());
        QV4::ScopedString name(scope, compilationUnit->runtimeStrings[lookup->nameIndex]);
        for (context = qmlContext; context; context = context->parent().data()) {
            objectId = context->propertyIndex(name);
            if (objectId != -1 && objectId < context->numIdValues())
                break;
        }
        break;
    }
    default:
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
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedString name(scope, compilationUnit->runtimeStrings[lookup->nameIndex]);
    const QQmlRefPointer<QQmlContextData> ownContext = qmlContext;
    for (auto context = ownContext; context; context = context->parent()) {
        const int propertyIdx = context->propertyIndex(name);
        if (propertyIdx == -1 || propertyIdx >= context->numIdValues())
            continue;

        if (context.data() == ownContext.data()) {
            lookup->qmlContextIdObjectLookup.objectId = propertyIdx;
            lookup->call = QV4::Lookup::Call::ContextGetterIdObject;
        } else {
            lookup->call = QV4::Lookup::Call::ContextGetterIdObjectInParentContext;
        }

        return;
    }

    Q_UNREACHABLE();
}

bool AOTCompiledContext::callObjectPropertyLookup(
        uint index, QObject *object, void **args, int argc) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;

    if (!object) {
        engine->handle()->throwTypeError(QStringLiteral("Cannot call method '%1' of null")
                .arg(compilationUnit->runtimeStrings[lookup->nameIndex]->toQString()));
        return false;
    }

    switch (lookup->call) {
    case QV4::Lookup::Call::GetterQObjectMethod:
    case QV4::Lookup::Call::GetterQObjectMethodFallback:
        return lookup->asVariant
                ? callQObjectMethodAsVariant(engine->handle(), lookup, object, args, argc)
                : callQObjectMethod(engine->handle(), lookup, object, args, argc);
    case QV4::Lookup::Call::GetterQObjectProperty:
    case QV4::Lookup::Call::GetterQObjectPropertyFallback: {
        const bool asVariant = lookup->asVariant;
        // Here we always retrieve a fresh method via the getter. No need to re-init.
        QV4::Scope scope(engine->handle());
        QV4::ScopedValue thisObject(scope, QV4::QObjectWrapper::wrap(scope.engine, object));
        QV4::Scoped<QV4::ArrowFunction> function(scope, lookup->getter(scope.engine, thisObject));
        Q_ASSERT(function);
        Q_ASSERT(lookup->asVariant == asVariant); // The getter mustn't touch the asVariant bit
        return asVariant
                ? callArrowFunctionAsVariant(scope.engine, function, qmlScopeObject, args, argc)
                : callArrowFunction(scope.engine, function, qmlScopeObject, args, argc);
    }
    default:
        break;
    }

    return false;
}

void AOTCompiledContext::initCallObjectPropertyLookupAsVariant(uint index, QObject *object) const
{
    if (engine->hasError()) {
        engine->handle()->amendException();
        return;
    }

    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue thisObject(scope, QV4::QObjectWrapper::wrap(scope.engine, object));
    QV4::ScopedFunctionObject function(scope, lookup->getter(scope.engine, thisObject));
    if (auto *method = function->as<QV4::QObjectMethod>()) {
        tryEnsureMethodsCache(method, object);
        lookup->asVariant = true;
        return;
    }

    if (function->as<QV4::ArrowFunction>()) {
        // Can't have overloads of JavaScript functions.
        lookup->asVariant = true;
        return;
    }

    scope.engine->throwTypeError(
            QStringLiteral("Property '%1' of object [object Object] is not a function")
                    .arg(compilationUnit->runtimeStrings[lookup->nameIndex]->toQString()));
}

void AOTCompiledContext::initCallObjectPropertyLookup(
        uint index, QObject *object, int relativeMethodIndex) const
{
    if (engine->hasError()) {
        engine->handle()->amendException();
        return;
    }

    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue thisObject(scope, QV4::QObjectWrapper::wrap(scope.engine, object));
    QV4::ScopedFunctionObject function(scope, lookup->getter(scope.engine, thisObject));
    if (auto *method = function->as<QV4::QObjectMethod>()) {
        if (tryEnsureMethodsCache(method, object)) {
            const auto match = resolveQObjectMethodOverload(method, lookup, relativeMethodIndex);
            Q_ASSERT(match == ExactMatch);
        }
        return;
    }

    if (function->as<QV4::ArrowFunction>()) {
        // Can't have overloads of JavaScript functions.
        return;
    }

    scope.engine->throwTypeError(
            QStringLiteral("Property '%1' of object [object Object] is not a function")
                    .arg(compilationUnit->runtimeStrings[lookup->nameIndex]->toQString()));
}

bool AOTCompiledContext::loadGlobalLookup(uint index, void *target) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    if (!lookup->protoLookup.metaType)
        return false;
    QV4::Scope scope(engine->handle());
    QV4::ScopedValue val(scope, lookup->globalGetter(engine->handle()));
    if (!QV4::ExecutionEngine::metaTypeFromJS(
                val,
                QMetaType(lookup->protoLookup.metaType), target)) {
        engine->handle()->throwTypeError();
        return false;
    }
    return true;
}

void AOTCompiledContext::initLoadGlobalLookup(uint index, QMetaType type) const
{
    if (engine->hasError()) {
        engine->handle()->amendException();
        return;
    }

    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    lookup->protoLookup.metaType = type.iface();
}

bool AOTCompiledContext::loadScopeObjectPropertyLookup(uint index, void *target) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;

    if (!qmlScopeObject) {
        engine->handle()->throwReferenceError(
                compilationUnit->runtimeStrings[lookup->nameIndex]->toQString());
        return false;
    }

    PropertyResult result = PropertyResult::NeedsInit;
    switch (lookup->call) {
    case QV4::Lookup::Call::ContextGetterScopeObjectProperty:
        result = loadObjectProperty(lookup, qmlScopeObject, target, this);
        break;
    case QV4::Lookup::Call::ContextGetterScopeObjectPropertyFallback:
        result = loadFallbackProperty(lookup, qmlScopeObject, target, this);
        break;
    default:
        return false;
    }

    switch (result) {
    case PropertyResult::NeedsInit:
        return false;
    case PropertyResult::Deleted:
        engine->handle()->throwTypeError(
                    QStringLiteral("Cannot read property '%1' of null")
                    .arg(compilationUnit->runtimeStrings[lookup->nameIndex]->toQString()));
        return false;
    case PropertyResult::OK:
        return true;
    }

    Q_UNREACHABLE_RETURN(false);
}

bool AOTCompiledContext::writeBackScopeObjectPropertyLookup(uint index, void *source) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;

    PropertyResult result = PropertyResult::NeedsInit;
    switch (lookup->call) {
    case QV4::Lookup::Call::ContextGetterScopeObjectProperty:
        result = writeBackObjectProperty(lookup, qmlScopeObject, source);
        break;
    case QV4::Lookup::Call::ContextGetterScopeObjectPropertyFallback:
        result = writeBackFallbackProperty(lookup, qmlScopeObject, source);
        break;
    default:
        return false;
    }

    switch (result) {
    case PropertyResult::NeedsInit:
        return false;
    case PropertyResult::Deleted: // Silently omit the write back. Same as interpreter
    case PropertyResult::OK:
        return true;
    }

    Q_UNREACHABLE_RETURN(false);
}



void AOTCompiledContext::initLoadScopeObjectPropertyLookup(uint index) const
{
    initObjectLookup<
            QV4::Lookup::Call::ContextGetterScopeObjectProperty,
            QV4::Lookup::Call::ContextGetterScopeObjectPropertyFallback,
            LookupType::Concrete>(this, index, qmlScopeObject);
}

bool AOTCompiledContext::loadSingletonLookup(uint index, void *target) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());

    if (lookup->call == QV4::Lookup::Call::ContextGetterSingleton) {
        QV4::Scoped<QV4::QQmlTypeWrapper> wrapper(
                    scope, lookup->qmlContextSingletonLookup.singletonObject);

        // We don't handle non-QObject singletons (as those can't be declared in qmltypes anyway)
        Q_ASSERT(wrapper);
        *static_cast<QObject **>(target) = wrapper->object();
        return true;
    }

    return false;
}

using QmlContextPropertyGetter
    = QV4::ReturnedValue (*)(QV4::Lookup *lookup, QV4::ExecutionEngine *engine, QV4::Value *thisObject);

template<QV4::Lookup::Call call>
static void initTypeWrapperLookup(
        const AOTCompiledContext *context, QV4::Lookup *lookup, uint importNamespace)
{
    Q_ASSERT(!context->engine->hasError());
    if (importNamespace != AOTCompiledContext::InvalidStringId) {
        QV4::Scope scope(context->engine->handle());
        QV4::ScopedString import(scope, context->compilationUnit->runtimeStrings[importNamespace]);

        QQmlTypeLoader *typeLoader = scope.engine->typeLoader();
        Q_ASSERT(typeLoader);
        if (const QQmlImportRef *importRef
                = context->qmlContext->imports()->query(import, typeLoader).importNamespace) {

            QV4::Scoped<QV4::QQmlTypeWrapper> wrapper(
                        scope, QV4::QQmlTypeWrapper::create(
                            scope.engine, nullptr, context->qmlContext->imports(), importRef));

            // This is not a contextGetter since we actually load from the namespace.
            wrapper = lookup->getter(context->engine->handle(), wrapper);

            // In theory, the getter may have populated the lookup's property cache.
            lookup->releasePropertyCache();

            lookup->call = call;
            switch (call) {
            case QV4::Lookup::Call::ContextGetterSingleton:
                lookup->qmlContextSingletonLookup.singletonObject.set(scope.engine, wrapper->heapObject());
                break;
            case QV4::Lookup::Call::ContextGetterType:
                lookup->qmlTypeLookup.qmlTypeWrapper.set(scope.engine, wrapper->heapObject());
                break;
            default:
                break;
            }
            return;
        }
        scope.engine->throwTypeError();
    } else {
        QV4::ExecutionEngine *v4 = context->engine->handle();
        lookup->contextGetter(v4, nullptr);
        if (lookup->call != call) {
            const QString error
                    = QLatin1String(call == QV4::Lookup::Call::ContextGetterSingleton
                        ? "%1 was a singleton at compile time, "
                          "but is not a singleton anymore."
                        : "%1 was not a singleton at compile time, "
                          "but is a singleton now.")
                    .arg(context->compilationUnit->runtimeStrings[lookup->nameIndex]->toQString());
            v4->throwTypeError(error);
        }
    }
}

void AOTCompiledContext::initLoadSingletonLookup(uint index, uint importNamespace) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    initTypeWrapperLookup<QV4::Lookup::Call::ContextGetterSingleton>(this, lookup, importNamespace);
}

bool AOTCompiledContext::loadAttachedLookup(uint index, QObject *object, void *target) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    if (lookup->call != QV4::Lookup::Call::GetterQObjectAttached)
        return false;

    QV4::Scope scope(engine->handle());
    QV4::Scoped<QV4::QQmlTypeWrapper> wrapper(scope, lookup->qmlTypeLookup.qmlTypeWrapper);
    Q_ASSERT(wrapper);
    *static_cast<QObject **>(target) = qmlAttachedPropertiesObject(
                object, wrapper->d()->type().attachedPropertiesFunction(
                    QQmlEnginePrivate::get(qmlEngine())));
    return true;
}

void AOTCompiledContext::initLoadAttachedLookup(
        uint index, uint importNamespace, QObject *object) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    QV4::Scope scope(engine->handle());
    QV4::ScopedString name(scope, compilationUnit->runtimeStrings[lookup->nameIndex]);

    QQmlType type;
    QQmlTypeLoader *typeLoader = scope.engine->typeLoader();
    Q_ASSERT(typeLoader);
    if (importNamespace != InvalidStringId) {
        QV4::ScopedString import(scope, compilationUnit->runtimeStrings[importNamespace]);
        if (const QQmlImportRef *importRef
                = qmlContext->imports()->query(import, typeLoader).importNamespace) {
            type = qmlContext->imports()->query(name, importRef, typeLoader).type;
        }
    } else {
        type = qmlContext->imports()->query<QQmlImport::AllowRecursion>(name, typeLoader).type;
    }

    if (!type.isValid()) {
        scope.engine->throwTypeError();
        return;
    }

    QV4::Scoped<QV4::QQmlTypeWrapper> wrapper(
                scope, QV4::QQmlTypeWrapper::create(scope.engine, object, type,
                                                    QV4::Heap::QQmlTypeWrapper::ExcludeEnums));

    lookup->qmlTypeLookup.qmlTypeWrapper.set(scope.engine, wrapper->d());
    lookup->call = QV4::Lookup::Call::GetterQObjectAttached;
}

bool AOTCompiledContext::loadTypeLookup(uint index, void *target) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    if (lookup->call != QV4::Lookup::Call::ContextGetterType)
        return false;

    const QV4::Heap::QQmlTypeWrapper *typeWrapper = static_cast<const QV4::Heap::QQmlTypeWrapper *>(
                lookup->qmlTypeLookup.qmlTypeWrapper.get());

    QMetaType metaType = typeWrapper->type().typeId();
    *static_cast<const QMetaObject **>(target)
            = QQmlMetaType::metaObjectForType(metaType).metaObject();
    return true;
}

void AOTCompiledContext::initLoadTypeLookup(uint index, uint importNamespace) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    initTypeWrapperLookup<QV4::Lookup::Call::ContextGetterType>(this, lookup, importNamespace);
}

bool AOTCompiledContext::getObjectLookup(uint index, QObject *object, void *target) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    const auto doThrow = [&]() {
        engine->handle()->throwTypeError(
                    QStringLiteral("Cannot read property '%1' of null")
                    .arg(compilationUnit->runtimeStrings[lookup->nameIndex]->toQString()));
        return false;
    };

    if (!object)
        return doThrow();

    PropertyResult result = PropertyResult::NeedsInit;
    switch (lookup->call) {
    case QV4::Lookup::Call::GetterQObjectProperty:
        result = lookup->asVariant
                ? loadObjectAsVariant(lookup, object, target, this)
                : loadObjectProperty(lookup, object, target, this);
        break;
    case QV4::Lookup::Call::GetterQObjectPropertyFallback:
        result = lookup->asVariant
                ? loadFallbackAsVariant(lookup, object, target, this)
                : loadFallbackProperty(lookup, object, target, this);
        break;
    default:
        return false;
    }

    switch (result) {
    case PropertyResult::Deleted:
        return doThrow();
    case PropertyResult::NeedsInit:
        return false;
    case PropertyResult::OK:
        return true;
    }

    Q_UNREACHABLE_RETURN(false);
}

bool AOTCompiledContext::writeBackObjectLookup(uint index, QObject *object, void *source) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    if (!object)
        return true;

    PropertyResult result = PropertyResult::NeedsInit;
    switch (lookup->call) {
    case QV4::Lookup::Call::GetterQObjectProperty:
        result = lookup->asVariant
                ? writeBackObjectAsVariant(lookup, object, source)
                : writeBackObjectProperty(lookup, object, source);
        break;
    case QV4::Lookup::Call::GetterQObjectPropertyFallback:
        result = lookup->asVariant
                ? writeBackFallbackAsVariant(lookup, object, source)
                : writeBackFallbackProperty(lookup, object, source);
        break;
    default:
        return false;
    }

    switch (result) {
    case PropertyResult::NeedsInit:
        return false;
    case PropertyResult::Deleted: // Silently omit the write back
    case PropertyResult::OK:
        return true;
    }

    Q_UNREACHABLE_RETURN(false);
}

void AOTCompiledContext::initGetObjectLookup(uint index, QObject *object) const
{
    initObjectLookup<
            QV4::Lookup::Call::GetterQObjectProperty,
            QV4::Lookup::Call::GetterQObjectPropertyFallback,
            LookupType::Concrete>(this, index, object);
}

void AOTCompiledContext::initGetObjectLookupAsVariant(uint index, QObject *object) const
{
    initObjectLookup<
            QV4::Lookup::Call::GetterQObjectProperty,
            QV4::Lookup::Call::GetterQObjectPropertyFallback,
            LookupType::Variant>(this, index, object);
}

bool AOTCompiledContext::getValueLookup(uint index, void *value, void *target) const
{
    Q_ASSERT(value);

    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    if (lookup->call != QV4::Lookup::Call::GetterValueTypeProperty)
        return false;

    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(lookup->qgadgetLookup.metaObject - 1);
    Q_ASSERT(metaObject);

    void *args[] = { target, nullptr };
    metaObject->d.static_metacall(
                reinterpret_cast<QObject*>(value), QMetaObject::ReadProperty,
                lookup->qgadgetLookup.coreIndex, args);
    return true;
}

bool AOTCompiledContext::writeBackValueLookup(uint index, void *value, void *source) const
{
    Q_ASSERT(value);

    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    if (lookup->call != QV4::Lookup::Call::GetterValueTypeProperty)
        return false;

    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(lookup->qgadgetLookup.metaObject - 1);
    Q_ASSERT(metaObject);

    void *args[] = { source, nullptr };
    metaObject->d.static_metacall(
            reinterpret_cast<QObject*>(value), QMetaObject::WriteProperty,
            lookup->qgadgetLookup.coreIndex, args);
    return true;
}

void AOTCompiledContext::initGetValueLookup(uint index, const QMetaObject *metaObject) const
{
    Q_ASSERT(!engine->hasError());
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    initValueLookup(lookup, compilationUnit, metaObject);
    lookup->call = QV4::Lookup::Call::GetterValueTypeProperty;
}

bool AOTCompiledContext::getEnumLookup(uint index, void *target) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    if (lookup->call != QV4::Lookup::Call::GetterEnumValue)
        return false;
    const bool isUnsigned
            = lookup->qmlEnumValueLookup.metaType->flags & QMetaType::IsUnsignedEnumeration;
    const QV4::ReturnedValue encoded = lookup->qmlEnumValueLookup.encodedEnumValue;
    switch (lookup->qmlEnumValueLookup.metaType->size) {
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
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    if (!metaObject) {
        engine->handle()->throwTypeError(
                    QStringLiteral("Cannot read property '%1' of undefined")
                    .arg(QString::fromUtf8(enumValue)));
        return;
    }
    const int enumIndex = metaObject->indexOfEnumerator(enumerator);
    const QMetaEnum metaEnum = metaObject->enumerator(enumIndex);
    lookup->qmlEnumValueLookup.encodedEnumValue = metaEnum.keyToValue(enumValue);
    lookup->qmlEnumValueLookup.metaType = metaEnum.metaType().iface();
    lookup->call = QV4::Lookup::Call::GetterEnumValue;
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

    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    PropertyResult result = PropertyResult::NeedsInit;
    switch (lookup->call) {
    case QV4::Lookup::Call::SetterQObjectProperty:
        result = lookup->asVariant
                ? storeObjectAsVariant(engine->handle(), lookup, object, value)
                : storeObjectProperty(lookup, object, value);
        break;
    case QV4::Lookup::Call::SetterQObjectPropertyFallback:
        result = lookup->asVariant
                ? storeFallbackAsVariant(engine->handle(), lookup, object, value)
                : storeFallbackProperty(lookup, object, value);
        break;
    default:
        return false;
    }

    switch (result) {
    case PropertyResult::Deleted:
        return doThrow();
    case PropertyResult::NeedsInit:
        return false;
    case PropertyResult::OK:
        return true;
    }

    Q_UNREACHABLE_RETURN(false);
}

void AOTCompiledContext::initSetObjectLookup(uint index, QObject *object) const
{
    initObjectLookup<
            QV4::Lookup::Call::SetterQObjectProperty,
            QV4::Lookup::Call::SetterQObjectPropertyFallback,
            LookupType::Concrete>(this, index, object);
}

void AOTCompiledContext::initSetObjectLookupAsVariant(uint index, QObject *object) const
{
    initObjectLookup<
            QV4::Lookup::Call::SetterQObjectProperty,
            QV4::Lookup::Call::SetterQObjectPropertyFallback,
            LookupType::Variant>(this, index, object);
}

static PropertyResult storeValueProperty(
        QV4::Lookup *lookup, const QMetaObject *metaObject, void *target, void *value)
{
    void *args[] = { value, nullptr };
    metaObject->d.static_metacall(
            reinterpret_cast<QObject *>(target), QMetaObject::WriteProperty,
            lookup->qgadgetLookup.coreIndex, args);
    return PropertyResult::OK;
}

static PropertyResult resetValueProperty(
        QV4::Lookup *lookup, const QMetaObject *metaObject, void *target, QV4::ExecutionEngine *v4)
{
    const QMetaProperty property = metaObject->property(lookup->qgadgetLookup.coreIndex);
    if (property.isResettable()) {
        void *args[] = { nullptr };
        metaObject->d.static_metacall(
                reinterpret_cast<QObject *>(target), QMetaObject::ResetProperty,
                lookup->qgadgetLookup.coreIndex, args);
    } else {
        v4->throwError(
                QLatin1String("Cannot assign [undefined] to ") +
                QLatin1String(property.metaType().name()));
    }

    return PropertyResult::OK;
}

static PropertyResult storeValueAsVariant(
        QV4::ExecutionEngine *v4, QV4::Lookup *lookup, const QMetaObject *metaObject,
        void *target, void *value)
{
    QVariant *variant = static_cast<QVariant *>(value);
    const QMetaType propType = metaObject->property(lookup->qgadgetLookup.coreIndex).metaType();
    if (propType == QMetaType::fromType<QVariant>())
        return storeValueProperty(lookup, metaObject, target, variant);

    if (!variant->isValid())
        return resetValueProperty(lookup, metaObject, target, v4);

    if (isTypeCompatible(variant->metaType(), propType))
        return storeValueProperty(lookup, metaObject, target, variant->data());

    QVariant converted(propType);
    QV4::Scope scope(v4);
    QV4::ScopedValue val(scope, v4->fromVariant(*variant));
    if (v4->metaTypeFromJS(val, propType, converted.data())
        || QMetaType::convert(
                variant->metaType(), variant->constData(), propType, converted.data())) {
        return storeValueProperty(lookup, metaObject, target, converted.data());
    }

    v4->throwError(
            QLatin1String("Cannot assign ") + QLatin1String(variant->metaType().name())
            + QLatin1String(" to ") + QLatin1String(propType.name()));

    return PropertyResult::NeedsInit;
}

bool AOTCompiledContext::setValueLookup(
        uint index, void *target, void *value) const
{
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    if (lookup->call != QV4::Lookup::Call::SetterValueTypeProperty)
        return false;

    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(lookup->qgadgetLookup.metaObject - 1);

    const PropertyResult result = lookup->asVariant
            ? storeValueAsVariant(engine->handle(), lookup, metaObject, target, value)
            : storeValueProperty(lookup, metaObject, target, value);

    switch (result) {
    case PropertyResult::OK:
        return true;
    case PropertyResult::NeedsInit:
        return false;
    case PropertyResult::Deleted:
        Q_UNREACHABLE();
    }

    return false;
}

template<LookupType Type>
void initValueLookup(
        const AOTCompiledContext *aotContext, uint index, const QMetaObject *metaObject)
{
    QV4::ExecutionEngine *v4 = aotContext->engine->handle();
    if (v4->hasException) {
        v4->amendException();
        return;
    }

    QV4::Lookup *lookup = aotContext->compilationUnit->runtimeLookups + index;
    initValueLookup(lookup, aotContext->compilationUnit, metaObject);
    lookup->call = QV4::Lookup::Call::SetterValueTypeProperty;
    lookup->asVariant = (Type == LookupType::Variant);
}

void AOTCompiledContext::initSetValueLookup(uint index, const QMetaObject *metaObject) const
{
    initValueLookup<LookupType::Concrete>(this, index, metaObject);
}

void AOTCompiledContext::initSetValueLookupAsVariant(
        uint index, const QMetaObject *metaObject) const
{
    initValueLookup<LookupType::Variant>(this, index, metaObject);
}

bool AOTCompiledContext::callValueLookup(uint index, void *target, void **args, int argc) const
{
    Q_UNUSED(argc);

    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    if (lookup->call != QV4::Lookup::Call::GetterValueTypeProperty)
        return false;

    Q_ASSERT(lookup->qgadgetLookup.isFunction);

    const QMetaObject *metaObject
            = reinterpret_cast<const QMetaObject *>(lookup->qgadgetLookup.metaObject - 1);

    metaObject->d.static_metacall(
            reinterpret_cast<QObject *>(target), QMetaObject::InvokeMetaMethod,
            lookup->qgadgetLookup.coreIndex, args);

    return true;
}

void AOTCompiledContext::initCallValueLookup(
        uint index, const QMetaObject *metaObject, int relativeMethodIndex) const
{
    Q_ASSERT(!engine->hasError());
    QV4::Lookup *lookup = compilationUnit->runtimeLookups + index;
    Q_ASSERT(metaObject);

    const int absoluteMethodIndex = relativeMethodIndex + metaObject->methodOffset();

    const QMetaMethod method = metaObject->method(absoluteMethodIndex);
    lookup->qgadgetLookup.metaObject = quintptr(metaObject) + 1;
    lookup->qgadgetLookup.coreIndex = absoluteMethodIndex;
    lookup->qgadgetLookup.metaType = method.returnMetaType().iface();
    lookup->qgadgetLookup.isFunction = true;
    lookup->call = QV4::Lookup::Call::GetterValueTypeProperty;
}

} // namespace QQmlPrivate

/*!
  \macro QML_DECLARE_TYPE()
  \relates <qqml.h>

  Equivalent to \c Q_DECLARE_METATYPE(TYPE *) and \c Q_DECLARE_METATYPE(QQmlListProperty<TYPE>)
*/

/*!
  \macro QML_DECLARE_TYPEINFO(Type,Flags)
  \relates <qqml.h>

  Declares additional properties of the given \a Type as described by the
  specified \a Flags.

  Current the only supported type info is \c QML_HAS_ATTACHED_PROPERTIES which
  declares that the \a Type supports \l {Attached Properties and Attached Signal Handlers}
  {attached properties}. QML_DECLARE_TYPEINFO() is not necessary if \a Type contains the
  QML_ATTACHED macro.
*/

/*!
  \fn template <typename T> int qmlRegisterType(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
  \relates <qqml.h>

  This template function registers the C++ type in the QML system with
  the name \a qmlName, in the library imported from \a uri having the
  version number composed from \a versionMajor and \a versionMinor.

  Returns the QML type id.

  There are two forms of this template function:

  \code
  template<typename T>
  int qmlRegisterType(const char *uri, int versionMajor, int versionMinor, const char *qmlName);

  template<typename T, int metaObjectRevision>
  int qmlRegisterType(const char *uri, int versionMajor, int versionMinor, const char *qmlName);
  \endcode

  The former is the standard form which registers the type \e T as a new type.
  The latter allows a particular revision of a class to be registered in
  a specified version (see \l {Type Revisions and Versions}).


  For example, this registers a C++ class \c MySliderItem as a QML type
  named \c Slider for version 1.0 of a type namespace called
  "com.mycompany.qmlcomponents":

  \code
  qmlRegisterType<MySliderItem>("com.mycompany.qmlcomponents", 1, 0, "Slider");
  \endcode

  Once this is registered, the type can be used in QML by importing the
  specified type namespace and version number:

  \qml
  import com.mycompany.qmlcomponents 1.0

  Slider {
      // ...
  }
  \endqml

  Note that it's perfectly reasonable for a library to register types to older versions
  than the actual version of the library. Indeed, it is normal for the new library to allow
  QML written to previous versions to continue to work, even if more advanced versions of
  some of its types are available.

  \sa QML_ELEMENT, QML_NAMED_ELEMENT(),
      {Choosing the Correct Integration Method Between C++ and QML}
*/

/*!
  \fn template<typename T, int metaObjectRevision> int qmlRegisterRevision(const char *uri, int versionMajor, int versionMinor)
  \relates <qqml.h>

  This template function registers the specified revision of a C++ type in the QML system with
  the library imported from \a uri having the version number composed
  from \a versionMajor and \a versionMinor.

  Returns the QML type id.

  \code
  template<typename T, int metaObjectRevision>
  int qmlRegisterRevision(const char *uri, int versionMajor, int versionMinor);
  \endcode

  This function is typically used to register the revision of a base class to
  use for the specified version of the type (see \l {Type Revisions and Versions}).
*/

/*!
  \fn template <typename T> int qmlRegisterUncreatableType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& message)
  \relates <qqml.h>

  This template function registers the C++ type in the QML system with
  the name \a qmlName, in the library imported from \a uri having the
  version number composed from \a versionMajor and \a versionMinor.

  While the type has a name and a type, it cannot be created, and the
  given error \a message will result if creation is attempted.

  This is useful where the type is only intended for providing attached properties or enum values.

  Returns the QML type id.

  \sa QML_UNCREATABLE(), qmlRegisterTypeNotAvailable(),
      {Choosing the Correct Integration Method Between C++ and QML}
*/

/*!
  \fn template <typename T, typename E> int qmlRegisterExtendedType(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
  \relates <qqml.h>

  This template function registers the C++ type and its extension object in the
  QML system with the name \a qmlName in the library imported from \a uri having
  version number composed from \a versionMajor and \a versionMinor. Properties
  not available in the main type will be searched for in the extension object.

  Returns the QML type id.

  \sa QML_EXTENDED(), qmlRegisterType(), {Registering Extension Objects}
*/

/*!
  \fn template <typename T, typename E> int qmlRegisterExtendedUncreatableType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason)
  \relates <qqml.h>

  This template function registers the C++ type and its extension
  in the QML system with the name \a qmlName in the library imported
  from \a uri having version number composed from \a versionMajor and
  \a versionMinor.

  While the type has a name and a type, it cannot be created. An error
  message with the given \a reason is printed if the user attempts to
  create an instance of this type.

  This is useful where the type is only intended for providing attached
  properties, enum values or an abstract base class with its extension.

  Returns the QML type id.

  \sa QML_EXTENDED(), QML_UNCREATABLE(), qmlRegisterUncreatableType()
*/

/*!
  \fn int qmlRegisterCustomExtendedType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, QQmlCustomParser *parser)
  \relates <qqml.h>
  \internal

  This template function registers the C++ type and its extension
  in the QML system with the name \a qmlName in the library imported
  from \a uri having version number composed from \a versionMajor and
  \a versionMinor. Properties from the C++ type or its extension that
  cannot be resolved directly by the QML system will be resolved using
  the \a parser provided.

  Returns the QML type id.

  \sa QML_ELEMENT, QML_NAMED_ELEMENT(), QML_EXTENDED()
*/

/*!
  \fn template <typename T> int qmlRegisterAnonymousType(const char *uri, int versionMajor)
  \relates <qqml.h>

  This template function registers the C++ type in the QML system as an anonymous type. The
  resulting QML type does not have a name. Therefore, instances of this type cannot be created from
  the QML system. You can, however, access instances of the type when they are exposed as properties
  of other types.

  Use this function when the type will not be referenced by name, specifically for C++ types that
  are used on the left-hand side of a property binding. To indicate to which module the type belongs
  use \a uri and \a versionMajor.

  For example, consider the following two classes:

  //! Workaround for MOC not respecting comments and triggering an error on certain Qt macros.
  \code Q
    class Bar : public QObject
    {
        \1_OBJECT
        Q_PROPERTY(QString baz READ baz WRITE setBaz NOTIFY bazChanged)

    public:
        Bar() {}

        QString baz() const { return mBaz; }

        void setBaz(const QString &baz)
        {
            if (baz == mBaz)
                return;

            mBaz = baz;
            emit bazChanged();
        }

    signals:
        void bazChanged();

    private:
        QString mBaz;
    };

    class Foo : public QObject
    {
        \1_OBJECT
        Q_PROPERTY(Bar *bar READ bar CONSTANT FINAL)

    public:
        Foo() {}

        Bar *bar() { return &mBar; }

    private:
        Bar mBar;
    };
    \endcode

  In QML, we assign a string to the \c baz property of \c bar:

    \code
    Foo {
        bar.baz: "abc"
        Component.onCompleted: print(bar.baz)
    }
    \endcode

  For the QML engine to know that the \c Bar type has a \c baz property,
  we have to make \c Bar known:

    \code
    qmlRegisterType<Foo>("App", 1, 0, "Foo");
    qmlRegisterAnonymousType<Bar>("App", 1);
    \endcode

  As the \c Foo type is instantiated in QML, it must be registered
  with the version of \l qmlRegisterType() that takes an element name.

  Returns the QML type id.

  \since 5.14
  \sa QML_ANONYMOUS, {Choosing the Correct Integration Method Between C++ and QML}
*/

/*!
    \fn int qmlRegisterInterface(const char *typeName)
    \relates <qqml.h>

    This template function registers the C++ type in the QML system
    under the name \a typeName.

    Types registered as an interface with the engine should also
    declare themselves as an interface with the
    \l {The Meta-Object System}{meta object system}. For example:

    \code
    struct FooInterface
    {
    public:
        virtual ~FooInterface();
        virtual void doSomething() = 0;
    };

    Q_DECLARE_INTERFACE(FooInterface, "org.foo.FooInterface")
    \endcode

    When registered with the QML engine in this way, they can be used as
    property types:

    Q_PROPERTY(FooInterface *foo READ foo WRITE setFoo)

    When you assign a \l QObject sub-class to this property, the QML engine does
    the interface cast to \c FooInterface* automatically.

    Returns the QML type id.

    \sa QML_INTERFACE
*/

/*!
   \fn int qmlRegisterSingletonType(const char *uri, int versionMajor, int versionMinor, const char *typeName, std::function<QJSValue(QQmlEngine *, QJSEngine *)> callback)
   \relates <qqml.h>

   This function may be used to register a singleton type provider \a callback in a particular \a uri
   and \a typeName with a version specified in \a versionMajor and \a versionMinor.

   Installing a singleton type allows developers to provide arbitrary functionality
   (methods and properties) to a client without requiring individual instances of the type to
   be instantiated by the client.

   A singleton type may be either a QObject or a QJSValue.
   This function should be used to register a singleton type provider function which returns a QJSValue as a singleton type.

   \b{NOTE:} QJSValue singleton type properties will \b{not} trigger binding re-evaluation if changed.

   Usage:
   \code
   // First, define the singleton type provider function (callback).
   static QJSValue example_qjsvalue_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
   {
       Q_UNUSED(engine)

       static int seedValue = 5;
       QJSValue example = scriptEngine->newObject();
       example.setProperty("someProperty", seedValue++);
       return example;
   }

   // Second, register the singleton type provider with QML by calling this function in an initialization function.
   qmlRegisterSingletonType("Qt.example.qjsvalueApi", 1, 0, "MyApi", example_qjsvalue_singletontype_provider);
   \endcode

   Alternatively, you can use a C++11 lambda:

   \code
   qmlRegisterSingletonType("Qt.example.qjsvalueApi", 1, 0, "MyApi", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QJSValue {
       Q_UNUSED(engine)

       static int seedValue = 5;
       QJSValue example = scriptEngine->newObject();
       example.setProperty("someProperty", seedValue++);
       return example;
   });
   \endcode

   In order to use the registered singleton type in QML, you must import the singleton type.
   \qml
   import QtQuick 2.0
   import Qt.example.qjsvalueApi 1.0 as ExampleApi
   Item {
       id: root
       property int someValue: ExampleApi.MyApi.someProperty
   }
   \endqml

   \sa QML_SINGLETON, {Choosing the Correct Integration Method Between C++ and QML}
*/

/*!
    \fn template<typename T> QObject *qmlAttachedPropertiesObject(const QObject *attachee, bool create)
    \relates <qqml.h>

    The form of this template function is:

    \code
    template<typename T> QObject *qmlAttachedPropertiesObject(const QObject *attachee, bool create = true)
    \endcode

    This returns the attached object instance that has been attached to the specified
    \a attachee by the attaching type \e T.

    If \a create is true and type \e T is a valid attaching type, this creates and returns a new
    attached object instance.

    Returns \nullptr if type \e T is not a valid attaching type, or if \a create is false and no
    attachment object instance has previously been created for \a attachee.

    \sa QML_ATTACHED(), {Providing Attached Properties}
*/

/*!
   \fn template <typename T> int qmlRegisterSingletonType(const char *uri, int versionMajor, int versionMinor, const char *typeName, std::function<QObject*(QQmlEngine *, QJSEngine *)> callback)
   \relates <qqml.h>

   This function may be used to register a singleton type provider \a callback in a particular \a uri
   and \a typeName with a version specified in \a versionMajor and \a versionMinor.

   Installing a singleton type into a uri allows developers to provide arbitrary functionality
   (methods and properties) to clients without requiring individual instances ot the type to be
   instantiated by the client.

   A singleton type may be either a QObject or a QJSValue.
   This function should be used to register a singleton type provider function which returns a QObject
   of the given type T as a singleton type.

   A QObject singleton type may be referenced via the type name with which it was registered, and this
   typename may be used as the target in a \l Connections type or otherwise used as any other type id would.
   One exception to this is that a QObject singleton type property may not be aliased.

   \b{NOTE:} A QObject singleton type instance returned from a singleton type provider is owned by
   the QML engine unless the object has explicit QQmlEngine::CppOwnership flag set.

   Usage:
   //! Workaround for MOC not respecting comments and triggering an error on certain Qt macros.
   \code Q
   // First, define your QObject which provides the functionality.
   class SingletonTypeExample : public QObject
   {
       \1_OBJECT
       Q_PROPERTY (int someProperty READ someProperty WRITE setSomeProperty NOTIFY somePropertyChanged)

   public:
       SingletonTypeExample(QObject *parent = nullptr)
           : QObject(parent), m_someProperty(0)
       {
       }

       ~SingletonTypeExample() {}

       Q_INVOKABLE int doSomething() { setSomeProperty(5); return m_someProperty; }

       int someProperty() const { return m_someProperty; }
       void setSomeProperty(int val) { m_someProperty = val; emit somePropertyChanged(val); }

   signals:
       void somePropertyChanged(int newValue);

   private:
       int m_someProperty;
   };

   // Second, define the singleton type provider function (callback).
   static QObject *example_qobject_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
   {
       Q_UNUSED(engine)
       Q_UNUSED(scriptEngine)

       SingletonTypeExample *example = new SingletonTypeExample();
       return example;
   }

   // Third, register the singleton type provider with QML by calling this function in an initialization function.
   qmlRegisterSingletonType<SingletonTypeExample>("Qt.example.qobjectSingleton", 1, 0, "MyApi", example_qobject_singletontype_provider);
   \endcode

   Alternatively, you can use a C++11 lambda:

   \code
   qmlRegisterSingletonType<SingletonTypeExample>("Qt.example.qobjectSingleton", 1, 0, "MyApi", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
       Q_UNUSED(engine)
       Q_UNUSED(scriptEngine)

       SingletonTypeExample *example = new SingletonTypeExample();
       return example;
   });
   \endcode

   In order to use the registered singleton type in QML, you must import the singleton type.
   \qml
   import QtQuick 2.0
   import Qt.example.qobjectSingleton 1.0
   Item {
       id: root
       property int someValue: MyApi.someProperty

       Component.onCompleted: {
           someValue = MyApi.doSomething()
       }
   }
   \endqml

   \sa QML_SINGLETON, {Choosing the Correct Integration Method Between C++ and QML}
*/

/*!
   \fn int qmlRegisterSingletonType(const QUrl &url, const char *uri, int versionMajor, int versionMinor, const char *qmlName)
   \relates <qqml.h>

   This function may be used to register a singleton type with the name \a qmlName, in the library imported from \a uri having
   the version number composed from \a versionMajor and \a versionMinor. The type is defined by the QML file located at \a url.
   The url must be an absolute URL, i.e. url.isRelative() == false.

   In addition the type's QML file must have pragma Singleton statement among its import statements.

   A singleton type may be referenced via the type name with which it was registered, and this typename may be used as the
   target in a \l Connections type or otherwise used as any other type id would. One exception to this is that a singleton
   type property may not be aliased (because the singleton type name does not identify an object within the same component
   as any other item).

   Usage:
   \qml
   // First, define your QML singleton type which provides the functionality.
   pragma Singleton
   import QtQuick 2.0
   Item {
       property int testProp1: 125
   }
   \endqml

   \code
   // Second, register the QML singleton type by calling this function in an initialization function.
   qmlRegisterSingletonType(QUrl("file:///absolute/path/SingletonType.qml"), "Qt.example.qobjectSingleton", 1, 0, "RegisteredSingleton");
   \endcode

   In order to use the registered singleton type in QML, you must import the singleton type.
   \qml
   import QtQuick 2.0
   import Qt.example.qobjectSingleton 1.0
   Item {
       id: root
       property int someValue: RegisteredSingleton.testProp1
   }
   \endqml

   It is also possible to have QML singleton types registered without using the qmlRegisterSingletonType function.
   That can be done by adding a pragma Singleton statement among the imports of the type's QML file. In addition
   the type must be defined in a qmldir file with a singleton keyword and the qmldir must be imported by the QML
   files using the singleton.

   \sa QML_SINGLETON
*/

/*!
   \fn int qmlRegisterSingletonInstance(const char *uri, int versionMajor, int versionMinor, const char *typeName, QObject *cppObject)
   \relates <qqml.h>
   \since 5.14

   This function is used to register a singleton object \a cppObject, with a
   particular \a uri and \a typeName. Its version is a combination of \a
   versionMajor and \a versionMinor.

   Installing a singleton type into a URI allows you to provide arbitrary
   functionality (methods and properties) to QML code without requiring
   individual instances of the type to be instantiated by the client.

   Use this function to register an object of the given type T as a singleton
   type.

   A QObject singleton type may be referenced via the type name with which it
   was registered; in turn this type name may be used as the target in a \l
   Connections type, or like any other type ID. However, there's one
   exception: a QObject singleton type property can't be aliased because the
   singleton type name does not identify an object within the same component
   as any other item.

   \note \a cppObject must outlive the QML engine in which it is used.
   Moreover, \cppObject must have the same thread affinity as the engine. If
   you want separate singleton instances for multiple engines, you need to use
   \l {qmlRegisterSingletonType}.  See \l{Threads and QObjects} for more
   information about thread safety.

   \b{NOTE:} qmlRegisterSingleton can only be used when all types of that module are registered procedurally.

    Usage:
    //! Workaround for MOC not respecting comments and triggering an error on certain Qt macros.
    \code Q
    // First, define your QObject which provides the functionality.
    class SingletonTypeExample : public QObject
    {
        \1_OBJECT
        Q_PROPERTY(int someProperty READ someProperty WRITE setSomeProperty NOTIFY somePropertyChanged)

    public:
        explicit SingletonTypeExample(QObject* parent = nullptr) : QObject(parent) {}

        Q_INVOKABLE int doSomething()
        {
            setSomeProperty(5);
            return m_someProperty;
        }

        int someProperty() const { return m_someProperty; }
        void setSomeProperty(int val) {
            if (m_someProperty != val) {
                m_someProperty = val;
                emit somePropertyChanged(val);
            }
        }

    signals:
        void somePropertyChanged(int newValue);

    private:
        int m_someProperty = 0;
    };
    \endcode

    \code
    // Second, create an instance of the object

    // allocate example before the engine to ensure that it outlives it
    QScopedPointer<SingletonTypeExample> example(new SingletonTypeExample);
    QQmlEngine engine;

    // Third, register the singleton type provider with QML by calling this
    // function in an initialization function.
    qmlRegisterSingletonInstance("Qt.example.qobjectSingleton", 1, 0, "MyApi", example.get());
    \endcode


    In order to use the registered singleton type in QML, you must import the
    URI with the corresponding version.
    \qml
    import QtQuick 2.0
    import Qt.example.qobjectSingleton 1.0
    Item {
        id: root
        property int someValue: MyApi.someProperty

        Component.onCompleted: {
            console.log(MyApi.doSomething())
        }
    }
    \endqml

   \sa QML_SINGLETON, qmlRegisterSingletonType
 */

/*!
  \fn int qmlRegisterType(const QUrl &url, const char *uri, int versionMajor, int versionMinor, const char *qmlName);
  \relates <qqml.h>

  This function registers a type in the QML system with the name \a qmlName, in the library imported from \a uri having the
  version number composed from \a versionMajor and \a versionMinor. The type is defined by the QML file located at \a url. The
  url must be an absolute URL, i.e. url.isRelative() == false.

  Normally QML files can be loaded as types directly from other QML files, or using a qmldir file. This function allows
  registration of files to types from C++ code, such as when the type mapping needs to be procedurally determined at startup.

  Returns -1 if the registration was not successful.
*/

QT_END_NAMESPACE
