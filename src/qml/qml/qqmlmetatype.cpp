// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlmetatype_p.h"

#include <private/qqmlmetatypedata_p.h>
#include <private/qqmltypemodule_p.h>
#include <private/qqmltype_p_p.h>
#include <private/qqmltypeloader_p.h>
#include <private/qqmlextensionplugin_p.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qv4executablecompilationunit_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qmutex.h>
#include <QtCore/qloggingcategory.h>

Q_DECLARE_LOGGING_CATEGORY(DBG_DISK_CACHE)
Q_LOGGING_CATEGORY(lcTypeRegistration, "qt.qml.typeregistration")

QT_BEGIN_NAMESPACE

CompositeMetaTypeIds CompositeMetaTypeIds::fromCompositeName(const QByteArray &name)
{
    auto ids = QQmlMetaType::registerInternalCompositeType(name);
    ids.refCount = new int;
    *ids.refCount = 1;
    return ids;
}

void CompositeMetaTypeIds::deref()
{
    Q_ASSERT(refCount);
    --*refCount;
    if (!*refCount) {
        delete refCount;
        QQmlMetaType::unregisterInternalCompositeType(*this);
        refCount = nullptr;
    }
}

CompositeMetaTypeIds::~CompositeMetaTypeIds()
{
    if (refCount)
        deref();
}


struct LockedData : private QQmlMetaTypeData
{
    friend class QQmlMetaTypeDataPtr;
};

Q_GLOBAL_STATIC(LockedData, metaTypeData)
Q_GLOBAL_STATIC(QRecursiveMutex, metaTypeDataLock)

struct ModuleUri : public QString
{
    ModuleUri(const QString &string) : QString(string) {}
    ModuleUri(const std::unique_ptr<QQmlTypeModule> &module) : QString(module->module()) {}
};

class QQmlMetaTypeDataPtr
{
    Q_DISABLE_COPY_MOVE(QQmlMetaTypeDataPtr)
public:
    QQmlMetaTypeDataPtr() : locker(metaTypeDataLock()), data(metaTypeData()) {}
    ~QQmlMetaTypeDataPtr() = default;

    QQmlMetaTypeData &operator*() { return *data; }
    QQmlMetaTypeData *operator->() { return data; }
    operator QQmlMetaTypeData *() { return data; }

    const QQmlMetaTypeData &operator*() const { return *data; }
    const QQmlMetaTypeData *operator->() const { return data; }
    operator const QQmlMetaTypeData *() const { return data; }

    bool isValid() const { return data != nullptr; }

private:
    QMutexLocker<QRecursiveMutex> locker;
    LockedData *data = nullptr;
};

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data,
                                       const QQmlPrivate::RegisterInterface &type)
{
    auto *d = new QQmlTypePrivate(QQmlType::InterfaceType);
    d->iid = type.iid;
    d->typeId = type.typeId;
    d->listId = type.listId;
    d->isSetup.storeRelease(true);
    d->module = QString::fromUtf8(type.uri);
    d->version = type.version;
    data->registerType(d);
    return d;
}

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data, const QString &elementName,
                                       const QQmlPrivate::RegisterSingletonType &type)
{
    auto *d = new QQmlTypePrivate(QQmlType::SingletonType);
    data->registerType(d);

    d->setName(QString::fromUtf8(type.uri), elementName);
    d->version = type.version;

    if (type.qObjectApi) {
        d->baseMetaObject = type.instanceMetaObject;
        d->typeId = type.typeId;
        d->revision = type.revision;
    }

    d->extraData.sd->singletonInstanceInfo = new QQmlType::SingletonInstanceInfo;
    d->extraData.sd->singletonInstanceInfo->scriptCallback = type.scriptApi;
    d->extraData.sd->singletonInstanceInfo->qobjectCallback = type.qObjectApi;
    d->extraData.sd->singletonInstanceInfo->typeName = QString::fromUtf8(type.typeName);
    d->extraData.sd->singletonInstanceInfo->instanceMetaObject
            = type.qObjectApi ? type.instanceMetaObject : nullptr;
    d->extraData.sd->extFunc = type.extensionObjectCreate;
    d->extraData.sd->extMetaObject = type.extensionMetaObject;

    return d;
}

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data, const QString &elementName,
                                       const QQmlPrivate::RegisterType &type)
{
    QQmlTypePrivate *d = new QQmlTypePrivate(QQmlType::CppType);
    data->registerType(d);
    d->setName(QString::fromUtf8(type.uri), elementName);

    d->version = type.version;
    d->revision = type.revision;
    d->typeId = type.typeId;
    d->listId = type.listId;
    d->extraData.cd->allocationSize = type.objectSize;
    d->extraData.cd->userdata = type.userdata;
    d->extraData.cd->newFunc = type.create;
    d->extraData.cd->noCreationReason = type.noCreationReason;
    d->extraData.cd->createValueTypeFunc = type.createValueType;
    d->baseMetaObject = type.metaObject;
    d->extraData.cd->attachedPropertiesFunc = type.attachedPropertiesFunction;
    d->extraData.cd->attachedPropertiesType = type.attachedPropertiesMetaObject;
    d->extraData.cd->parserStatusCast = type.parserStatusCast;
    d->extraData.cd->propertyValueSourceCast = type.valueSourceCast;
    d->extraData.cd->propertyValueInterceptorCast = type.valueInterceptorCast;
    d->extraData.cd->finalizerCast = type.has(QQmlPrivate::RegisterType::FinalizerCast)
            ? type.finalizerCast
            : -1;
    d->extraData.cd->extFunc = type.extensionObjectCreate;
    d->extraData.cd->customParser = reinterpret_cast<QQmlCustomParser *>(type.customParser);
    d->extraData.cd->registerEnumClassesUnscoped = true;
    d->extraData.cd->registerEnumsFromRelatedTypes = true;
    d->extraData.cd->constructValueType = type.has(QQmlPrivate::RegisterType::CreationMethod)
            && type.creationMethod != QQmlPrivate::ValueTypeCreationMethod::None;
    d->extraData.cd->populateValueType = type.has(QQmlPrivate::RegisterType::CreationMethod)
            && type.creationMethod == QQmlPrivate::ValueTypeCreationMethod::Structured;

    if (type.extensionMetaObject)
        d->extraData.cd->extMetaObject = type.extensionMetaObject;

    // Check if the user wants only scoped enum classes
    if (d->baseMetaObject) {
        auto indexOfUnscoped = d->baseMetaObject->indexOfClassInfo("RegisterEnumClassesUnscoped");
        if (indexOfUnscoped != -1
                && qstrcmp(d->baseMetaObject->classInfo(indexOfUnscoped).value(), "false") == 0) {
            d->extraData.cd->registerEnumClassesUnscoped = false;
        }

        auto indexOfRelated = d->baseMetaObject->indexOfClassInfo("RegisterEnumsFromRelatedTypes");
        if (indexOfRelated != -1
                && qstrcmp(d->baseMetaObject->classInfo(indexOfRelated).value(), "false") == 0) {
            d->extraData.cd->registerEnumsFromRelatedTypes = false;
        }
    }

    return d;
}

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data, const QString &elementName,
                                       const QQmlPrivate::RegisterCompositeType &type)
{
    auto *d = new QQmlTypePrivate(QQmlType::CompositeType);
    data->registerType(d);
    d->setName(QString::fromUtf8(type.uri), elementName);
    d->version = type.version;

    d->extraData.fd->url = QQmlTypeLoader::normalize(type.url);
    return d;
}

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data, const QString &elementName,
                                       const QQmlPrivate::RegisterCompositeSingletonType &type)
{
    auto *d = new QQmlTypePrivate(QQmlType::CompositeSingletonType);
    data->registerType(d);
    d->setName(QString::fromUtf8(type.uri), elementName);

    d->version = type.version;

    d->extraData.sd->singletonInstanceInfo = new QQmlType::SingletonInstanceInfo;
    d->extraData.sd->singletonInstanceInfo->url = QQmlTypeLoader::normalize(type.url);
    d->extraData.sd->singletonInstanceInfo->typeName = QString::fromUtf8(type.typeName);
    return d;
}

void QQmlMetaType::clone(QMetaObjectBuilder &builder, const QMetaObject *mo,
                         const QMetaObject *ignoreStart, const QMetaObject *ignoreEnd,
                         QQmlMetaType::ClonePolicy policy)
{
    // Set classname
    builder.setClassName(mo->className());

    // Clone Q_CLASSINFO
    for (int ii = mo->classInfoOffset(); ii < mo->classInfoCount(); ++ii) {
        QMetaClassInfo info = mo->classInfo(ii);

        int otherIndex = ignoreEnd->indexOfClassInfo(info.name());
        if (otherIndex >= ignoreStart->classInfoOffset() + ignoreStart->classInfoCount()) {
            // Skip
        } else {
            builder.addClassInfo(info.name(), info.value());
        }
    }

    if (policy != QQmlMetaType::CloneEnumsOnly) {
        // Clone Q_METHODS - do this first to avoid duplicating the notify signals.
        for (int ii = mo->methodOffset(); ii < mo->methodCount(); ++ii) {
            QMetaMethod method = mo->method(ii);

            // More complex - need to search name
            QByteArray name = method.name();

            bool found = false;

            for (int ii = ignoreStart->methodOffset() + ignoreStart->methodCount();
                 !found && ii < ignoreEnd->methodOffset() + ignoreEnd->methodCount(); ++ii) {

                QMetaMethod other = ignoreEnd->method(ii);

                found = name == other.name();
            }

            QMetaMethodBuilder m = builder.addMethod(method);
            if (found) // SKIP
                m.setAccess(QMetaMethod::Private);
        }

        // Clone Q_PROPERTY
        for (int ii = mo->propertyOffset(); ii < mo->propertyCount(); ++ii) {
            QMetaProperty property = mo->property(ii);

            int otherIndex = ignoreEnd->indexOfProperty(property.name());
            if (otherIndex >= ignoreStart->propertyOffset() + ignoreStart->propertyCount()) {
                builder.addProperty(QByteArray("__qml_ignore__") + property.name(),
                                    QByteArray("void"));
                // Skip
            } else {
                builder.addProperty(property);
            }
        }
    }

    // Clone enums registered with the metatype system
    for (int ii = mo->enumeratorOffset(); ii < mo->enumeratorCount(); ++ii) {
        QMetaEnum enumerator = mo->enumerator(ii);

        int otherIndex = ignoreEnd->indexOfEnumerator(enumerator.name());
        if (otherIndex >= ignoreStart->enumeratorOffset() + ignoreStart->enumeratorCount()) {
            // Skip
        } else {
            builder.addEnumerator(enumerator);
        }
    }
}

void QQmlMetaType::qmlInsertModuleRegistration(const QString &uri, void (*registerFunction)())
{
    QQmlMetaTypeDataPtr data;
    if (data->moduleTypeRegistrationFunctions.contains(uri))
        qFatal("Cannot add multiple registrations for %s", qPrintable(uri));
    else
        data->moduleTypeRegistrationFunctions.insert(uri, registerFunction);
}

void QQmlMetaType::qmlRemoveModuleRegistration(const QString &uri)
{
    QQmlMetaTypeDataPtr data;

    if (!data.isValid())
        return; // shutdown/deletion race. Not a problem.

    if (!data->moduleTypeRegistrationFunctions.contains(uri))
        qFatal("Cannot remove multiple registrations for %s", qPrintable(uri));
    else
        data->moduleTypeRegistrationFunctions.remove(uri);
}

bool QQmlMetaType::qmlRegisterModuleTypes(const QString &uri)
{
    QQmlMetaTypeDataPtr data;
    return data->registerModuleTypes(uri);
}

void QQmlMetaType::clearTypeRegistrations()
{
    //Only cleans global static, assumed no running engine
    QQmlMetaTypeDataPtr data;

    data->uriToModule.clear();
    data->types.clear();
    data->idToType.clear();
    data->nameToType.clear();
    data->urlToType.clear();
    data->typePropertyCaches.clear();
    data->urlToNonFileImportType.clear();
    data->metaObjectToType.clear();
    data->undeletableTypes.clear();
    data->propertyCaches.clear();
    data->inlineComponentTypes.clear();
}

void QQmlMetaType::registerTypeAlias(int typeIndex, const QString &name)
{
    QQmlMetaTypeDataPtr data;
    const QQmlType type = data->types.value(typeIndex);
    const QQmlTypePrivate *priv = type.priv();
    data->nameToType.insert(name, priv);
}

int QQmlMetaType::registerAutoParentFunction(const QQmlPrivate::RegisterAutoParent &function)
{
    if (function.structVersion > 1)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;

    data->parentFunctions.append(function.function);

    return data->parentFunctions.size() - 1;
}

void QQmlMetaType::unregisterAutoParentFunction(const QQmlPrivate::AutoParentFunction &function)
{
    QQmlMetaTypeDataPtr data;
    data->parentFunctions.removeOne(function);
}

QQmlType QQmlMetaType::registerInterface(const QQmlPrivate::RegisterInterface &type)
{
    if (type.structVersion > 1)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;
    QQmlTypePrivate *priv = createQQmlType(data, type);
    Q_ASSERT(priv);


    data->idToType.insert(priv->typeId.id(), priv);
    data->idToType.insert(priv->listId.id(), priv);

    data->interfaces.insert(type.typeId.id());

    return QQmlType(priv);
}

static QString registrationTypeString(QQmlType::RegistrationType typeType)
{
    QString typeStr;
    if (typeType == QQmlType::CppType)
        typeStr = QStringLiteral("element");
    else if (typeType == QQmlType::SingletonType)
        typeStr = QStringLiteral("singleton type");
    else if (typeType == QQmlType::CompositeSingletonType)
        typeStr = QStringLiteral("composite singleton type");
    else if (typeType == QQmlType::SequentialContainerType)
        typeStr = QStringLiteral("sequential container type");
    else
        typeStr = QStringLiteral("type");
    return typeStr;
}

// NOTE: caller must hold a QMutexLocker on "data"
static bool checkRegistration(
        QQmlType::RegistrationType typeType, QQmlMetaTypeData *data, const char *uri,
        const QString &typeName, QTypeRevision version, QMetaType::TypeFlags flags)
{
    if (!typeName.isEmpty()) {
        if (typeName.at(0).isLower() && (flags & QMetaType::PointerToQObject)) {
            QString failure(QCoreApplication::translate("qmlRegisterType", "Invalid QML %1 name \"%2\"; type names must begin with an uppercase letter"));
            data->recordTypeRegFailure(failure.arg(registrationTypeString(typeType), typeName));
            return false;
        }

        if (typeName.at(0).isUpper()
                && (flags & (QMetaType::IsGadget | QMetaType::PointerToGadget))) {
            qCWarning(lcTypeRegistration).noquote()
                    << QCoreApplication::translate(
                           "qmlRegisterType",
                           "Invalid QML %1 name \"%2\"; "
                           "value type names should begin with a lowercase letter")
                       .arg(registrationTypeString(typeType), typeName);
        }

        // There can also be types that aren't even gadgets, and there can be types for namespaces.
        // We cannot check those, but namespaces should be uppercase.

        int typeNameLen = typeName.size();
        for (int ii = 0; ii < typeNameLen; ++ii) {
            if (!(typeName.at(ii).isLetterOrNumber() || typeName.at(ii) == u'_')) {
                QString failure(QCoreApplication::translate("qmlRegisterType", "Invalid QML %1 name \"%2\""));
                data->recordTypeRegFailure(failure.arg(registrationTypeString(typeType), typeName));
                return false;
            }
        }
    }

    if (uri && !typeName.isEmpty()) {
        QString nameSpace = QString::fromUtf8(uri);
        QQmlTypeModule *qqtm = data->findTypeModule(nameSpace, version);
        if (qqtm && qqtm->lockLevel() != QQmlTypeModule::LockLevel::Open) {
            QString failure(QCoreApplication::translate(
                                "qmlRegisterType",
                                "Cannot install %1 '%2' into protected module '%3' version '%4'"));
            data->recordTypeRegFailure(failure
                                       .arg(registrationTypeString(typeType), typeName, nameSpace)
                                       .arg(version.majorVersion()));
            return false;
        }
    }

    return true;
}

// NOTE: caller must hold a QMutexLocker on "data"
static QQmlTypeModule *getTypeModule(
        const QHashedString &uri, QTypeRevision version, QQmlMetaTypeData *data)
{
    if (QQmlTypeModule *module = data->findTypeModule(uri, version))
        return module;
    return data->addTypeModule(std::make_unique<QQmlTypeModule>(uri, version.majorVersion()));
}

// NOTE: caller must hold a QMutexLocker on "data"
static void addTypeToData(QQmlTypePrivate *type, QQmlMetaTypeData *data)
{
    Q_ASSERT(type);

    if (!type->elementName.isEmpty())
        data->nameToType.insert(type->elementName, type);

    if (type->baseMetaObject)
        data->metaObjectToType.insert(type->baseMetaObject, type);

    if (type->regType == QQmlType::SequentialContainerType) {
        if (type->listId.isValid())
            data->idToType.insert(type->listId.id(), type);
    } else {
        if (type->typeId.isValid())
            data->idToType.insert(type->typeId.id(), type);

        if (type->listId.flags().testFlag(QMetaType::IsQmlList))
            data->idToType.insert(type->listId.id(), type);
    }

    if (!type->module.isEmpty()) {
        const QHashedString &mod = type->module;

        QQmlTypeModule *module = getTypeModule(mod, type->version, data);
        Q_ASSERT(module);
        module->add(type);
    }
}

QQmlType QQmlMetaType::registerType(const QQmlPrivate::RegisterType &type)
{
    if (type.structVersion > int(QQmlPrivate::RegisterType::CurrentVersion))
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;

    QString elementName = QString::fromUtf8(type.elementName);
    if (!checkRegistration(QQmlType::CppType, data, type.uri, elementName, type.version,
                           QMetaType(type.typeId).flags())) {
        return QQmlType();
    }

    QQmlTypePrivate *priv = createQQmlType(data, elementName, type);
    addTypeToData(priv, data);

    return QQmlType(priv);
}

QQmlType QQmlMetaType::registerSingletonType(const QQmlPrivate::RegisterSingletonType &type)
{
    if (type.structVersion > 1)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;

    QString typeName = QString::fromUtf8(type.typeName);
    if (!checkRegistration(QQmlType::SingletonType, data, type.uri, typeName, type.version,
                           QMetaType(type.typeId).flags())) {
        return QQmlType();
    }

    QQmlTypePrivate *priv = createQQmlType(data, typeName, type);

    addTypeToData(priv, data);

    return QQmlType(priv);
}

QQmlType QQmlMetaType::registerCompositeSingletonType(const QQmlPrivate::RegisterCompositeSingletonType &type)
{
    if (type.structVersion > 1)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    // Assumes URL is absolute and valid. Checking of user input should happen before the URL enters type.
    QQmlMetaTypeDataPtr data;

    QString typeName = QString::fromUtf8(type.typeName);
    bool fileImport = false;
    if (*(type.uri) == '\0')
        fileImport = true;
    if (!checkRegistration(QQmlType::CompositeSingletonType, data, fileImport ? nullptr : type.uri,
                           typeName, type.version, {})) {
        return QQmlType();
    }

    QQmlTypePrivate *priv = createQQmlType(data, typeName, type);
    addTypeToData(priv, data);

    QQmlMetaTypeData::Files *files = fileImport ? &(data->urlToType) : &(data->urlToNonFileImportType);
    files->insert(QQmlTypeLoader::normalize(type.url), priv);

    return QQmlType(priv);
}

QQmlType QQmlMetaType::registerCompositeType(const QQmlPrivate::RegisterCompositeType &type)
{
    if (type.structVersion > 1)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    // Assumes URL is absolute and valid. Checking of user input should happen before the URL enters type.
    QQmlMetaTypeDataPtr data;

    QString typeName = QString::fromUtf8(type.typeName);
    bool fileImport = false;
    if (*(type.uri) == '\0')
        fileImport = true;
    if (!checkRegistration(QQmlType::CompositeType, data, fileImport?nullptr:type.uri, typeName,
                           type.version, {})) {
        return QQmlType();
    }

    QQmlTypePrivate *priv = createQQmlType(data, typeName, type);
    addTypeToData(priv, data);

    QQmlMetaTypeData::Files *files = fileImport ? &(data->urlToType) : &(data->urlToNonFileImportType);
    files->insert(QQmlTypeLoader::normalize(type.url), priv);

    return QQmlType(priv);
}

CompositeMetaTypeIds QQmlMetaType::registerInternalCompositeType(const QByteArray &className)
{
    QByteArray ptr = className + '*';
    QByteArray lst = "QQmlListProperty<" + className + '>';

    QMetaType ptr_type(new QQmlMetaTypeInterface(ptr));
    QMetaType lst_type(new QQmlListMetaTypeInterface(lst, ptr_type.iface()));

    // Retrieve the IDs once, so that the types are added to QMetaType's custom type registry.
    ptr_type.id();
    lst_type.id();

    return {ptr_type, lst_type};
}

void QQmlMetaType::unregisterInternalCompositeType(const CompositeMetaTypeIds &typeIds)
{
    QMetaType metaType(typeIds.id);
    QMetaType listMetaType(typeIds.listId);

    // This may be called from delayed dtors on shutdown when the data is already gone.
    QQmlMetaTypeDataPtr data;
    if (data.isValid()) {
        if (QQmlValueType *vt = data->metaTypeToValueType.take(metaType.id()))
            delete vt;
        if (QQmlValueType *vt = data->metaTypeToValueType.take(listMetaType.id()))
            delete vt;
    }

    QMetaType::unregisterMetaType(metaType);
    QMetaType::unregisterMetaType(listMetaType);
    delete static_cast<const QQmlMetaTypeInterface *>(metaType.iface());
    delete static_cast<const QQmlListMetaTypeInterface *>(listMetaType.iface());
}

int QQmlMetaType::registerUnitCacheHook(
        const QQmlPrivate::RegisterQmlUnitCacheHook &hookRegistration)
{
    if (hookRegistration.structVersion > 1)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;
    data->lookupCachedQmlUnit << hookRegistration.lookupCachedQmlUnit;
    return 0;
}

QQmlType QQmlMetaType::registerSequentialContainer(
        const QQmlPrivate::RegisterSequentialContainer &container)
{
    if (container.structVersion > 1)
        qFatal("qmlRegisterSequenceContainer(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;

    if (!checkRegistration(QQmlType::SequentialContainerType, data, container.uri, QString(),
                           container.version, {})) {
        return QQmlType();
    }

    QQmlTypePrivate *priv = new QQmlTypePrivate(QQmlType::SequentialContainerType);

    data->registerType(priv);
    priv->setName(QString::fromUtf8(container.uri), QString());
    priv->version = container.version;
    priv->revision = container.revision;
    priv->typeId = container.metaSequence.valueMetaType();
    priv->listId = container.typeId;
    *priv->extraData.ld = container.metaSequence;

    addTypeToData(priv, data);

    return QQmlType(priv);
}

void QQmlMetaType::unregisterSequentialContainer(int id)
{
    unregisterType(id);
}

bool QQmlMetaType::protectModule(const QString &uri, QTypeRevision version,
                                 bool weakProtectAllVersions)
{
    QQmlMetaTypeDataPtr data;
    if (version.hasMajorVersion()) {
        if (QQmlTypeModule *module = data->findTypeModule(uri, version)) {
            if (!weakProtectAllVersions) {
                module->setLockLevel(QQmlTypeModule::LockLevel::Strong);
                return true;
            }
        } else {
            return false;
        }
    }

    const auto range = std::equal_range(
                data->uriToModule.begin(), data->uriToModule.end(), uri,
                std::less<ModuleUri>());

    for (auto it = range.first; it != range.second; ++it)
        (*it)->setLockLevel(QQmlTypeModule::LockLevel::Weak);

    return range.first != range.second;
}

void QQmlMetaType::registerModuleImport(const QString &uri, QTypeRevision moduleVersion,
                                        const QQmlDirParser::Import &import)
{
    QQmlMetaTypeDataPtr data;

    data->moduleImports.insert(QQmlMetaTypeData::VersionedUri(uri, moduleVersion), import);
}

void QQmlMetaType::unregisterModuleImport(const QString &uri, QTypeRevision moduleVersion,
                                          const QQmlDirParser::Import &import)
{
    QQmlMetaTypeDataPtr data;
    data->moduleImports.remove(QQmlMetaTypeData::VersionedUri(uri, moduleVersion), import);
}

QList<QQmlDirParser::Import> QQmlMetaType::moduleImports(
        const QString &uri, QTypeRevision version)
{
    QQmlMetaTypeDataPtr data;
    QList<QQmlDirParser::Import> result;

    const auto unrevisioned = data->moduleImports.equal_range(
                QQmlMetaTypeData::VersionedUri(uri, QTypeRevision()));
    for (auto it = unrevisioned.second; it != unrevisioned.first;)
        result.append(*(--it));

    if (version.hasMajorVersion()) {
        const auto revisioned = data->moduleImports.equal_range(
                    QQmlMetaTypeData::VersionedUri(uri, version));
        for (auto it = revisioned.second; it != revisioned.first;)
            result.append(*(--it));
        return result;
    }

    // Use latest module available with that URI.
    const auto begin = data->moduleImports.begin();
    auto it = unrevisioned.first;
    if (it == begin)
        return result;

    const QQmlMetaTypeData::VersionedUri latestVersion = (--it).key();
    if (latestVersion.uri != uri)
        return result;

    do {
        result += *it;
    } while (it != begin && (--it).key() == latestVersion);

    return result;
}

void QQmlMetaType::registerModule(const char *uri, QTypeRevision version)
{
    QQmlMetaTypeDataPtr data;

    QQmlTypeModule *module = getTypeModule(QString::fromUtf8(uri), version, data);
    Q_ASSERT(module);

    if (version.hasMinorVersion())
        module->addMinorVersion(version.minorVersion());
}

int QQmlMetaType::typeId(const char *uri, QTypeRevision version, const char *qmlName)
{
    QQmlMetaTypeDataPtr data;

    QQmlTypeModule *module = getTypeModule(QString::fromUtf8(uri), version, data);
    if (!module)
        return -1;

    QQmlType type = module->type(QHashedStringRef(QString::fromUtf8(qmlName)), version);
    if (!type.isValid())
        return -1;

    return type.index();
}

void QQmlMetaType::registerUndeletableType(const QQmlType &dtype)
{
    QQmlMetaTypeDataPtr data;
    data->undeletableTypes.insert(dtype);
}

static bool namespaceContainsRegistrations(const QQmlMetaTypeData *data, const QString &uri,
                                           QTypeRevision version)
{
    // Has any type previously been installed to this namespace?
    QHashedString nameSpace(uri);
    for (const QQmlType &type : data->types) {
        if (type.module() == nameSpace && type.version().majorVersion() == version.majorVersion())
            return true;
    }

    return false;
}

class QQmlMetaTypeRegistrationFailureRecorder
{
    Q_DISABLE_COPY_MOVE(QQmlMetaTypeRegistrationFailureRecorder)
public:
    QQmlMetaTypeRegistrationFailureRecorder(QQmlMetaTypeData *data, QStringList *failures)
        : data(data)
    {
        data->setTypeRegistrationFailures(failures);
    }

    ~QQmlMetaTypeRegistrationFailureRecorder()
    {
        data->setTypeRegistrationFailures(nullptr);
    }

    QQmlMetaTypeData *data = nullptr;
};


QQmlMetaType::RegistrationResult QQmlMetaType::registerPluginTypes(
        QObject *instance, const QString &basePath, const QString &uri,
        const QString &typeNamespace, QTypeRevision version, QList<QQmlError> *errors)
{
    if (!typeNamespace.isEmpty() && typeNamespace != uri) {
        // This is an 'identified' module
        // The namespace for type registrations must match the URI for locating the module
        if (errors) {
            QQmlError error;
            error.setDescription(
                    QStringLiteral("Module namespace '%1' does not match import URI '%2'")
                            .arg(typeNamespace, uri));
            errors->prepend(error);
        }
        return RegistrationResult::Failure;
    }

    QStringList failures;
    QQmlMetaTypeDataPtr data;
    {
        QQmlMetaTypeRegistrationFailureRecorder failureRecorder(data, &failures);
        if (!typeNamespace.isEmpty()) {
            // This is an 'identified' module
            if (namespaceContainsRegistrations(data, typeNamespace, version)) {
                // Other modules have already installed to this namespace
                if (errors) {
                    QQmlError error;
                    error.setDescription(QStringLiteral("Namespace '%1' has already been used "
                                                        "for type registration")
                                                 .arg(typeNamespace));
                    errors->prepend(error);
                }
                return RegistrationResult::Failure;
            }
        } else {
            // This is not an identified module - provide a warning
            qWarning().nospace() << qPrintable(
                    QStringLiteral("Module '%1' does not contain a module identifier directive - "
                                   "it cannot be protected from external registrations.").arg(uri));
        }

        if (instance && !qobject_cast<QQmlEngineExtensionInterface *>(instance)) {
            QQmlTypesExtensionInterface *iface = qobject_cast<QQmlTypesExtensionInterface *>(instance);
            if (!iface) {
                if (errors) {
                    QQmlError error;
                    // Also does not implement QQmlTypesExtensionInterface, but we want to discourage that.
                    error.setDescription(QStringLiteral("Module loaded for URI '%1' does not implement "
                                                        "QQmlEngineExtensionInterface").arg(typeNamespace));
                    errors->prepend(error);
                }
                return RegistrationResult::Failure;
            }

#if QT_DEPRECATED_SINCE(6, 3)
            if (auto *plugin = qobject_cast<QQmlExtensionPlugin *>(instance)) {
                // basepath should point to the directory of the module, not the plugin file itself:
                QQmlExtensionPluginPrivate::get(plugin)->baseUrl
                        = QQmlImports::urlFromLocalFileOrQrcOrUrl(basePath);
            }
#else
            Q_UNUSED(basePath)
#endif

            const QByteArray bytes = uri.toUtf8();
            const char *moduleId = bytes.constData();
            iface->registerTypes(moduleId);
        }

        if (failures.isEmpty() && !data->registerModuleTypes(uri))
            return RegistrationResult::NoRegistrationFunction;

        if (!failures.isEmpty()) {
            if (errors) {
                for (const QString &failure : std::as_const(failures)) {
                    QQmlError error;
                    error.setDescription(failure);
                    errors->prepend(error);
                }
            }
            return RegistrationResult::Failure;
        }
    }

    return RegistrationResult::Success;
}

/*
    \internal

    Fetches the QQmlType instance registered for \a urlString, creating a
    registration for it if it is not already registered, using the associated
    \a typeName, \a isCompositeSingleton, \a majorVersion and \a minorVersion
    details.

    Errors (if there are any) are placed into \a errors, if it is nonzero.
    Otherwise errors are printed as warnings.
*/
QQmlType QQmlMetaType::typeForUrl(const QString &urlString,
                                  const QHashedStringRef &qualifiedType,
                                  bool isCompositeSingleton, QList<QQmlError> *errors,
                                  QTypeRevision version)
{
    // ### unfortunate (costly) conversion
    const QUrl url = QQmlTypeLoader::normalize(QUrl(urlString));

    QQmlMetaTypeDataPtr data;
    {
        QQmlType ret(data->urlToType.value(url));
        if (ret.isValid() && ret.sourceUrl() == url)
            return ret;
    }
    {
        QQmlType ret(data->urlToNonFileImportType.value(url));
        if (ret.isValid() && ret.sourceUrl() == url)
            return ret;
    }

    const int dot = qualifiedType.indexOf(QLatin1Char('.'));
    const QString typeName = dot < 0
            ? qualifiedType.toString()
            : QString(qualifiedType.constData() + dot + 1, qualifiedType.length() - dot - 1);

    QStringList failures;
    QQmlMetaTypeRegistrationFailureRecorder failureRecorder(data, &failures);

    // Register the type. Note that the URI parameters here are empty; for
    // file type imports, we do not place them in a URI as we don't
    // necessarily have a good and unique one (picture a library import,
    // which may be found in multiple plugin locations on disk), but there
    // are other reasons for this too.
    //
    // By not putting them in a URI, we prevent the types from being
    // registered on a QQmlTypeModule; this is important, as once types are
    // placed on there, they cannot be easily removed, meaning if the
    // developer subsequently loads a different import (meaning different
    // types) with the same URI (using, say, a different plugin path), it is
    // very undesirable that we continue to associate the types from the
    // "old" URI with that new module.
    //
    // Not having URIs also means that the types cannot be found by name
    // etc, the only way to look them up is through QQmlImports -- for
    // better or worse.
    const QQmlType::RegistrationType registrationType = isCompositeSingleton
            ? QQmlType::CompositeSingletonType
            : QQmlType::CompositeType;
    if (checkRegistration(registrationType, data, nullptr, typeName, version, {})) {
        auto *priv = new QQmlTypePrivate(registrationType);
        priv->setName(QString(), typeName);
        priv->version = version;

        if (isCompositeSingleton) {
            priv->extraData.sd->singletonInstanceInfo = new QQmlType::SingletonInstanceInfo;
            priv->extraData.sd->singletonInstanceInfo->url = url;
            priv->extraData.sd->singletonInstanceInfo->typeName = typeName;
        } else {
            priv->extraData.fd->url = url;
        }

        data->registerType(priv);
        addTypeToData(priv, data);
        data->urlToType.insert(url, priv);
        return QQmlType(priv);
    }

    // This means that the type couldn't be found by URL, but could not be
    // registered either, meaning we most likely were passed some kind of bad
    // data.
    if (errors) {
        QQmlError error;
        error.setDescription(failures.join(u'\n'));
        errors->prepend(error);
    } else {
        qWarning("%s", failures.join(u'\n').toLatin1().constData());
    }
    return QQmlType();
}

QRecursiveMutex *QQmlMetaType::typeRegistrationLock()
{
    return metaTypeDataLock();
}

/*
    Returns the latest version of \a uri installed, or an in valid QTypeRevision().
*/
QTypeRevision QQmlMetaType::latestModuleVersion(const QString &uri)
{
    QQmlMetaTypeDataPtr data;
    auto upper = std::upper_bound(data->uriToModule.begin(), data->uriToModule.end(), uri,
                                  std::less<ModuleUri>());
    if (upper == data->uriToModule.begin())
        return QTypeRevision();

    const auto module = (--upper)->get();
    return (module->module() == uri)
            ? QTypeRevision::fromVersion(module->majorVersion(), module->maximumMinorVersion())
            : QTypeRevision();
}

/*
    Returns true if a module \a uri of this version is installed and locked;
*/
bool QQmlMetaType::isStronglyLockedModule(const QString &uri, QTypeRevision version)
{
    QQmlMetaTypeDataPtr data;

    if (QQmlTypeModule* qqtm = data->findTypeModule(uri, version))
        return qqtm->lockLevel() == QQmlTypeModule::LockLevel::Strong;
    return false;
}

/*
    Returns the best matching registered version for the given \a module. If \a version is
    the does not have a major version, returns the latest registered version. Otherwise
    chooses the same major version and checks if the minor version is within the range
    of registered minor versions. If so, retuens the original version, otherwise returns
    an invalid version. If \a version does not have a minor version, chooses the latest one.
*/
QTypeRevision QQmlMetaType::matchingModuleVersion(const QString &module, QTypeRevision version)
{
    if (!version.hasMajorVersion())
        return latestModuleVersion(module);

    QQmlMetaTypeDataPtr data;

    // first, check Types
    if (QQmlTypeModule *tm = data->findTypeModule(module, version)) {
        if (!version.hasMinorVersion())
            return QTypeRevision::fromVersion(version.majorVersion(), tm->maximumMinorVersion());

        if (tm->minimumMinorVersion() <= version.minorVersion()
                && tm->maximumMinorVersion() >= version.minorVersion()) {
            return version;
        }
    }

    return QTypeRevision();
}

QQmlTypeModule *QQmlMetaType::typeModule(const QString &uri, QTypeRevision version)
{
    QQmlMetaTypeDataPtr data;

    if (version.hasMajorVersion())
        return data->findTypeModule(uri, version);

    auto range = std::equal_range(data->uriToModule.begin(), data->uriToModule.end(),
                                  uri, std::less<ModuleUri>());

    return range.first == range.second ? nullptr : (--range.second)->get();
}

QList<QQmlPrivate::AutoParentFunction> QQmlMetaType::parentFunctions()
{
    QQmlMetaTypeDataPtr data;
    return data->parentFunctions;
}

QObject *QQmlMetaType::toQObject(const QVariant &v, bool *ok)
{
    if (!v.metaType().flags().testFlag(QMetaType::PointerToQObject)) {
        if (ok) *ok = false;
        return nullptr;
    }

    if (ok) *ok = true;

    return *(QObject *const *)v.constData();
}

/*
    Returns the item type for a list of type \a id.
 */
QMetaType QQmlMetaType::listValueType(QMetaType metaType)
{
    if (isList(metaType)) {
        const auto iface = metaType.iface();
        if (iface && iface->metaObjectFn == &dynamicQmlListMarker)
            return QMetaType(static_cast<const QQmlListMetaTypeInterface *>(iface)->valueType);
    } else if (metaType.flags() & QMetaType::PointerToQObject) {
        return QMetaType();
    }

    QQmlMetaTypeDataPtr data;
    Q_ASSERT(data);
    QQmlTypePrivate *type = data->idToType.value(metaType.id());

    if (type && type->listId == metaType)
        return type->typeId;
    else
        return QMetaType {};
}

QQmlAttachedPropertiesFunc QQmlMetaType::attachedPropertiesFunc(QQmlEnginePrivate *engine,
                                                                const QMetaObject *mo)
{
    QQmlMetaTypeDataPtr data;

    QQmlType type(data->metaObjectToType.value(mo));
    return type.attachedPropertiesFunction(engine);
}

QMetaProperty QQmlMetaType::defaultProperty(const QMetaObject *metaObject)
{
    int idx = metaObject->indexOfClassInfo("DefaultProperty");
    if (-1 == idx)
        return QMetaProperty();

    QMetaClassInfo info = metaObject->classInfo(idx);
    if (!info.value())
        return QMetaProperty();

    idx = metaObject->indexOfProperty(info.value());
    if (-1 == idx)
        return QMetaProperty();

    return metaObject->property(idx);
}

QMetaProperty QQmlMetaType::defaultProperty(QObject *obj)
{
    if (!obj)
        return QMetaProperty();

    const QMetaObject *metaObject = obj->metaObject();
    return defaultProperty(metaObject);
}

QMetaMethod QQmlMetaType::defaultMethod(const QMetaObject *metaObject)
{
    int idx = metaObject->indexOfClassInfo("DefaultMethod");
    if (-1 == idx)
        return QMetaMethod();

    QMetaClassInfo info = metaObject->classInfo(idx);
    if (!info.value())
        return QMetaMethod();

    idx = metaObject->indexOfMethod(info.value());
    if (-1 == idx)
        return QMetaMethod();

    return metaObject->method(idx);
}

QMetaMethod QQmlMetaType::defaultMethod(QObject *obj)
{
    if (!obj)
        return QMetaMethod();

    const QMetaObject *metaObject = obj->metaObject();
    return defaultMethod(metaObject);
}

/*!
    See qmlRegisterInterface() for information about when this will return true.
*/
bool QQmlMetaType::isInterface(QMetaType type)
{
    const QQmlMetaTypeDataPtr data;
    return data->interfaces.contains(type.id());
}

const char *QQmlMetaType::interfaceIId(QMetaType metaType)
{
    const QQmlMetaTypeDataPtr data;
    const QQmlType type(data->idToType.value(metaType.id()));
    return (type.isInterface() && type.typeId() == metaType) ? type.interfaceIId() : nullptr;
}

bool QQmlMetaType::isList(QMetaType type)
{
    if (type.flags().testFlag(QMetaType::IsQmlList))
        return true;
    else
        return false;
}

/*!
    Returns the type (if any) of URI-qualified named \a qualifiedName and version specified
    by \a version_major and \a version_minor.
*/
QQmlType QQmlMetaType::qmlType(const QString &qualifiedName, QTypeRevision version)
{
    int slash = qualifiedName.indexOf(QLatin1Char('/'));
    if (slash <= 0)
        return QQmlType();

    QHashedStringRef module(qualifiedName.constData(), slash);
    QHashedStringRef name(qualifiedName.constData() + slash + 1, qualifiedName.size() - slash - 1);

    return qmlType(name, module, version);
}

/*!
    \internal
    Returns the type (if any) of \a name in \a module and the specified \a version.

    If \a version has no major version, accept any version.
    If \a version has no minor version, accept any minor version.
    If \a module is empty, search in all modules and accept any version.
*/
QQmlType QQmlMetaType::qmlType(const QHashedStringRef &name, const QHashedStringRef &module,
                               QTypeRevision version)
{
    const QQmlMetaTypeDataPtr data;

    const QHashedString key(QString::fromRawData(name.constData(), name.length()), name.hash());
    QQmlMetaTypeData::Names::ConstIterator it = data->nameToType.constFind(key);
    while (it != data->nameToType.cend() && it.key() == name) {
        QQmlType t(*it);
        if (module.isEmpty() || t.availableInVersion(module, version))
            return t;
        ++it;
    }

    return QQmlType();
}

/*!
    Returns the type (if any) that corresponds to the \a metaObject. Returns an invalid type if no
    such type is registered.
*/
QQmlType QQmlMetaType::qmlType(const QMetaObject *metaObject)
{
    const QQmlMetaTypeDataPtr data;
    return QQmlType(data->metaObjectToType.value(metaObject));
}

/*!
    Returns the type (if any) that corresponds to the \a metaObject in version specified
    by \a version_major and \a version_minor in module specified by \a uri.  Returns null if no
    type is registered.
*/
QQmlType QQmlMetaType::qmlType(const QMetaObject *metaObject, const QHashedStringRef &module,
                               QTypeRevision version)
{
    const QQmlMetaTypeDataPtr data;

    const auto range = data->metaObjectToType.equal_range(metaObject);
    for (auto it = range.first; it != range.second; ++it) {
        QQmlType t(*it);
        if (module.isEmpty() || t.availableInVersion(module, version))
            return t;
    }

    return QQmlType();
}

/*!
    Returns the type (if any) that corresponds to \a qmlTypeId.
    Returns an invalid QQmlType if no such type is registered.
*/
QQmlType QQmlMetaType::qmlTypeById(int qmlTypeId)
{
    const QQmlMetaTypeDataPtr data;
    QQmlType type = data->types.value(qmlTypeId);
    if (type.isValid())
        return type;
    return QQmlType();
}

/*!
    Returns the type (if any) that corresponds to \a metaType.
    Returns an invalid QQmlType if no such type is registered.
*/
QQmlType QQmlMetaType::qmlType(QMetaType metaType)
{
    const QQmlMetaTypeDataPtr data;
    QQmlTypePrivate *type = data->idToType.value(metaType.id());
    return (type && type->typeId == metaType) ? QQmlType(type) : QQmlType();
}

QQmlType QQmlMetaType::qmlListType(QMetaType metaType)
{
    const QQmlMetaTypeDataPtr data;
    QQmlTypePrivate *type = data->idToType.value(metaType.id());
    return (type && type->listId == metaType) ? QQmlType(type) : QQmlType();
}

/*!
    Returns the type (if any) that corresponds to the given \a url in the set of
    composite types added through file imports.

    Returns null if no such type is registered.
*/
QQmlType QQmlMetaType::qmlType(const QUrl &unNormalizedUrl, bool includeNonFileImports /* = false */)
{
    const QUrl url = QQmlTypeLoader::normalize(unNormalizedUrl);
    const QQmlMetaTypeDataPtr data;

    QQmlType type(data->urlToType.value(url));
    if (!type.isValid() && includeNonFileImports)
        type = QQmlType(data->urlToNonFileImportType.value(url));

    if (type.sourceUrl() == url)
        return type;
    else
        return QQmlType();
}

QQmlType QQmlMetaType::inlineComponentType(const QQmlType &containingType, const QString &name)
{
    const QQmlMetaTypeDataPtr data;
    return data->inlineComponentTypes[InlineComponentKey {containingType.priv(), name}];
}

void QQmlMetaType::associateInlineComponent(
    const QQmlType &containingType, const QString &name,
    const CompositeMetaTypeIds &metaTypeIds, QQmlType existingType)
{
    bool const reuseExistingType = existingType.isValid();
    auto priv = reuseExistingType
                    ? const_cast<QQmlTypePrivate *>(existingType.priv())
                    : new QQmlTypePrivate { QQmlType::RegistrationType::InlineComponentType } ;
    priv->setName( QString::fromUtf8(existingType.typeName()), name);
    QUrl icUrl(existingType.sourceUrl());
    icUrl.setFragment(name);
    priv->extraData.id->url = icUrl;
    priv->extraData.id->containingType = containingType.priv();
    priv->typeId = metaTypeIds.id;
    priv->listId = metaTypeIds.listId;
    QQmlType icType(priv);

    QQmlMetaTypeDataPtr data;
    data->inlineComponentTypes.insert({containingType.priv(), name}, icType);

    if (!reuseExistingType)
        priv->release();
}

/*!
Returns a QQmlPropertyCache for \a obj if one is available.

If \a obj is null, being deleted or contains a dynamic meta object,
nullptr is returned.
*/
QQmlPropertyCache::ConstPtr QQmlMetaType::propertyCache(QObject *obj, QTypeRevision version)
{
    if (!obj || QObjectPrivate::get(obj)->metaObject || QObjectPrivate::get(obj)->wasDeleted)
        return QQmlPropertyCache::ConstPtr();
    return QQmlMetaType::propertyCache(obj->metaObject(), version);
}

QQmlPropertyCache::ConstPtr QQmlMetaType::propertyCache(
        const QMetaObject *metaObject, QTypeRevision version)
{
    QQmlMetaTypeDataPtr data; // not const: the cache is created on demand
    return data->propertyCache(metaObject, version);
}

QQmlPropertyCache::ConstPtr QQmlMetaType::propertyCache(
        const QQmlType &type, QTypeRevision version)
{
    QQmlMetaTypeDataPtr data; // not const: the cache is created on demand
    return data->propertyCache(type, version);
}

/*!
 * \internal
 *
 * Look up by type's baseMetaObject.
 */
QQmlMetaObject QQmlMetaType::rawMetaObjectForType(QMetaType metaType)
{
    const QQmlMetaTypeDataPtr data;
    if (auto composite = data->findPropertyCacheInCompositeTypes(metaType))
        return QQmlMetaObject(composite);

    const QQmlTypePrivate *type = data->idToType.value(metaType.id());
    return (type && type->typeId == metaType) ? type->baseMetaObject : nullptr;
}

/*!
 * \internal
 *
 * Look up by type's metaObject.
 */
QQmlMetaObject QQmlMetaType::metaObjectForType(QMetaType metaType)
{
    const QQmlMetaTypeDataPtr data;
    if (auto composite = data->findPropertyCacheInCompositeTypes(metaType))
        return QQmlMetaObject(composite);

    const QQmlTypePrivate *type = data->idToType.value(metaType.id());
    return (type && type->typeId == metaType)
            ? QQmlType(type).metaObject()
            : nullptr;
}

/*!
 * \internal
 *
 * Look up by type's metaObject and version.
 */
QQmlPropertyCache::ConstPtr QQmlMetaType::propertyCacheForType(QMetaType metaType)
{
    QQmlMetaTypeDataPtr data;
    if (auto composite = data->findPropertyCacheInCompositeTypes(metaType))
        return composite;

    const QQmlTypePrivate *type = data->idToType.value(metaType.id());
    if (type && type->typeId == metaType) {
        if (const QMetaObject *mo = QQmlType(type).metaObject())
            return data->propertyCache(mo, type->version);
    }

    return QQmlPropertyCache::ConstPtr();
}

/*!
 * \internal
 *
 * Look up by type's baseMetaObject and unspecified/any version.
 * TODO: Is this correct? Passing a plain QTypeRevision() rather than QTypeRevision::zero() or
 *       the actual type's version seems strange. The behavior has been in place for a while.
 */
QQmlPropertyCache::ConstPtr QQmlMetaType::rawPropertyCacheForType(QMetaType metaType)
{
    QQmlMetaTypeDataPtr data;
    if (auto composite = QQmlMetaType::findPropertyCacheInCompositeTypes(metaType))
        return composite;

    const QQmlTypePrivate *type = data->idToType.value(metaType.id());
    return (type && type->typeId == metaType)
            ? data->propertyCache(type->baseMetaObject, QTypeRevision())
            : QQmlPropertyCache::ConstPtr();
}

/*!
 * \internal
 *
 * Look up by QQmlType and version. We only fall back to lookup by metaobject if the type
 * has no revisiononed attributes here. Unspecified versions are interpreted as "any".
 */
QQmlPropertyCache::ConstPtr QQmlMetaType::rawPropertyCacheForType(
        QMetaType metaType, QTypeRevision version)
{
    QQmlMetaTypeDataPtr data;
    if (auto composite = data->findPropertyCacheInCompositeTypes(metaType))
        return composite;

    const QQmlTypePrivate *typePriv = data->idToType.value(metaType.id());
    if (!typePriv || typePriv->typeId != metaType)
        return QQmlPropertyCache::ConstPtr();

    const QQmlType type(typePriv);
    if (type.containsRevisionedAttributes()) {
        // It can only have (revisioned) properties or methods if it has a metaobject
        Q_ASSERT(type.metaObject());
        return data->propertyCache(type, version);
    }

    if (const QMetaObject *metaObject = type.metaObject())
        return data->propertyCache(metaObject, version);

    return QQmlPropertyCache::ConstPtr();
}

void QQmlMetaType::unregisterType(int typeIndex)
{
    QQmlMetaTypeDataPtr data;
    const QQmlType type = data->types.value(typeIndex);
    if (const QQmlTypePrivate *d = type.priv()) {
        if (d->regType == QQmlType::CompositeType || d->regType == QQmlType::CompositeSingletonType)
            removeFromInlineComponents(data->inlineComponentTypes, d);
        removeQQmlTypePrivate(data->idToType, d);
        removeQQmlTypePrivate(data->nameToType, d);
        removeQQmlTypePrivate(data->urlToType, d);
        removeQQmlTypePrivate(data->urlToNonFileImportType, d);
        removeQQmlTypePrivate(data->metaObjectToType, d);
        for (auto & module : data->uriToModule)
            module->remove(d);
        data->clearPropertyCachesForVersion(typeIndex);
        data->types[typeIndex] = QQmlType();
        data->undeletableTypes.remove(type);
    }
}

void QQmlMetaType::registerMetaObjectForType(const QMetaObject *metaobject, QQmlTypePrivate *type)
{
    Q_ASSERT(type);

    QQmlMetaTypeDataPtr data;
    data->metaObjectToType.insert(metaobject, type);
}

static bool hasActiveInlineComponents(const QQmlMetaTypeData *data, const QQmlTypePrivate *d)
{
    for (auto it = data->inlineComponentTypes.begin(), end = data->inlineComponentTypes.end();
         it != end; ++it) {
        if (it.key().containingType != d)
            continue;

        const QQmlTypePrivate *icPriv = it->priv();
        if (icPriv && icPriv->count() > 1)
            return true;
    }
    return false;
}

void QQmlMetaType::freeUnusedTypesAndCaches()
{
    QQmlMetaTypeDataPtr data;

    // in case this is being called during program exit, `data` might be destructed already
    if (!data.isValid())
        return;

    bool deletedAtLeastOneType;
    do {
        deletedAtLeastOneType = false;
        QList<QQmlType>::Iterator it = data->types.begin();
        while (it != data->types.end()) {
            const QQmlTypePrivate *d = (*it).priv();
            if (d && d->count() == 1 && !hasActiveInlineComponents(data, d)) {
                deletedAtLeastOneType = true;

                if (d->regType == QQmlType::CompositeType
                        || d->regType == QQmlType::CompositeSingletonType) {
                    removeFromInlineComponents(data->inlineComponentTypes, d);
                }
                removeQQmlTypePrivate(data->idToType, d);
                removeQQmlTypePrivate(data->nameToType, d);
                removeQQmlTypePrivate(data->urlToType, d);
                removeQQmlTypePrivate(data->urlToNonFileImportType, d);
                removeQQmlTypePrivate(data->metaObjectToType, d);

                for (auto &module : data->uriToModule)
                    module->remove(d);

                data->clearPropertyCachesForVersion(d->index);
                *it = QQmlType();
            } else {
                ++it;
            }
        }
    } while (deletedAtLeastOneType);

    bool deletedAtLeastOneCache;
    do {
        deletedAtLeastOneCache = false;
        auto it = data->propertyCaches.begin();
        while (it != data->propertyCaches.end()) {
            if ((*it)->count() == 1) {
                it = data->propertyCaches.erase(it);
                deletedAtLeastOneCache = true;
            } else {
                ++it;
            }
        }
    } while (deletedAtLeastOneCache);
}

/*!
    Returns the list of registered QML type names.
*/
QList<QString> QQmlMetaType::qmlTypeNames()
{
    const QQmlMetaTypeDataPtr data;

    QList<QString> names;
    names.reserve(data->nameToType.size());
    QQmlMetaTypeData::Names::ConstIterator it = data->nameToType.cbegin();
    while (it != data->nameToType.cend()) {
        QQmlType t(*it);
        names += t.qmlTypeName();
        ++it;
    }

    return names;
}

/*!
    Returns the list of registered QML types.
*/
QList<QQmlType> QQmlMetaType::qmlTypes()
{
    const QQmlMetaTypeDataPtr data;

    QList<QQmlType> types;
    for (const QQmlTypePrivate *t : data->nameToType)
        types.append(QQmlType(t));

    return types;
}

/*!
    Returns the list of all registered types.
*/
QList<QQmlType> QQmlMetaType::qmlAllTypes()
{
    const QQmlMetaTypeDataPtr data;
    return data->types;
}

/*!
    Returns the list of registered QML singleton types.
*/
QList<QQmlType> QQmlMetaType::qmlSingletonTypes()
{
    const QQmlMetaTypeDataPtr data;

    QList<QQmlType> retn;
    for (const auto t : std::as_const(data->nameToType)) {
        QQmlType type(t);
        if (type.isSingleton())
            retn.append(type);
    }
    return retn;
}

static bool isFullyTyped(const QQmlPrivate::CachedQmlUnit *unit)
{
    quint32 numTypedFunctions = 0;
    for (const QQmlPrivate::AOTCompiledFunction *function = unit->aotCompiledFunctions;
         function; ++function) {
        if (function->functionPtr)
            ++numTypedFunctions;
        else
            return false;
    }
    return numTypedFunctions == unit->qmlData->functionTableSize;
}

const QQmlPrivate::CachedQmlUnit *QQmlMetaType::findCachedCompilationUnit(
        const QUrl &uri, QQmlMetaType::CacheMode mode, CachedUnitLookupError *status)
{
    Q_ASSERT(mode != RejectAll);
    const QQmlMetaTypeDataPtr data;

    for (const auto lookup : std::as_const(data->lookupCachedQmlUnit)) {
        if (const QQmlPrivate::CachedQmlUnit *unit = lookup(uri)) {
            QString error;
            if (!QV4::ExecutableCompilationUnit::verifyHeader(unit->qmlData, QDateTime(), &error)) {
                qCDebug(DBG_DISK_CACHE) << "Error loading pre-compiled file " << uri << ":" << error;
                if (status)
                    *status = CachedUnitLookupError::VersionMismatch;
                return nullptr;
            }

            if (mode == RequireFullyTyped && !isFullyTyped(unit)) {
                qCDebug(DBG_DISK_CACHE)
                        << "Error loading pre-compiled file " << uri
                        << ": compilation unit contains functions not compiled to native code.";
                if (status)
                    *status = CachedUnitLookupError::NotFullyTyped;
                return nullptr;
            }

            if (status)
                *status = CachedUnitLookupError::NoError;
            return unit;
        }
    }

    if (status)
        *status = CachedUnitLookupError::NoUnitFound;

    return nullptr;
}

void QQmlMetaType::prependCachedUnitLookupFunction(QQmlPrivate::QmlUnitCacheLookupFunction handler)
{
    QQmlMetaTypeDataPtr data;
    data->lookupCachedQmlUnit.prepend(handler);
}

void QQmlMetaType::removeCachedUnitLookupFunction(QQmlPrivate::QmlUnitCacheLookupFunction handler)
{
    QQmlMetaTypeDataPtr data;
    data->lookupCachedQmlUnit.removeAll(handler);
}

/*!
    Returns the pretty QML type name (e.g. 'Item' instead of 'QtQuickItem') for the given object.
 */
QString QQmlMetaType::prettyTypeName(const QObject *object)
{
    QString typeName;

    if (!object)
        return typeName;

    QQmlType type = QQmlMetaType::qmlType(object->metaObject());
    if (type.isValid()) {
        typeName = type.qmlTypeName();
        const int lastSlash = typeName.lastIndexOf(QLatin1Char('/'));
        if (lastSlash != -1)
            typeName = typeName.mid(lastSlash + 1);
    }

    if (typeName.isEmpty()) {
        typeName = QString::fromUtf8(object->metaObject()->className());
        int marker = typeName.indexOf(QLatin1String("_QMLTYPE_"));
        if (marker != -1)
            typeName = typeName.left(marker);

        marker = typeName.indexOf(QLatin1String("_QML_"));
        if (marker != -1) {
            typeName = QStringView{typeName}.left(marker) + QLatin1Char('*');
            type = QQmlMetaType::qmlType(QMetaType::fromName(typeName.toUtf8()));
            if (type.isValid()) {
                QString qmlTypeName = type.qmlTypeName();
                const int lastSlash = qmlTypeName.lastIndexOf(QLatin1Char('/'));
                if (lastSlash != -1)
                    qmlTypeName = qmlTypeName.mid(lastSlash + 1);
                if (!qmlTypeName.isEmpty())
                    typeName = qmlTypeName;
            }
        }
    }

    return typeName;
}

QList<QQmlProxyMetaObject::ProxyData> QQmlMetaType::proxyData(const QMetaObject *mo,
                                                              const QMetaObject *baseMetaObject,
                                                              QMetaObject *lastMetaObject)
{
    QList<QQmlProxyMetaObject::ProxyData> metaObjects;
    mo = mo->d.superdata;

    const QQmlMetaTypeDataPtr data;

    auto createProxyMetaObject = [&](QQmlTypePrivate *This,
                                     const QMetaObject *superdataBaseMetaObject,
                                     const QMetaObject *extMetaObject,
                                     QObject *(*extFunc)(QObject *)) {
        if (!extMetaObject)
            return;

        QMetaObjectBuilder builder;
        clone(builder, extMetaObject, superdataBaseMetaObject, baseMetaObject,
              extFunc ? QQmlMetaType::CloneAll : QQmlMetaType::CloneEnumsOnly);
        QMetaObject *mmo = builder.toMetaObject();
        mmo->d.superdata = baseMetaObject;
        if (!metaObjects.isEmpty())
            metaObjects.constLast().metaObject->d.superdata = mmo;
        else if (lastMetaObject)
            lastMetaObject->d.superdata = mmo;
        QQmlProxyMetaObject::ProxyData data = { mmo, extFunc, 0, 0 };
        metaObjects << data;
        registerMetaObjectForType(mmo, This);
    };

    while (mo) {
        QQmlTypePrivate *t = data->metaObjectToType.value(mo);
        if (t) {
            if (t->regType == QQmlType::CppType) {
                createProxyMetaObject(t, t->baseMetaObject, t->extraData.cd->extMetaObject,
                                      t->extraData.cd->extFunc);
            } else if (t->regType == QQmlType::SingletonType) {
                createProxyMetaObject(t, t->baseMetaObject, t->extraData.sd->extMetaObject,
                                      t->extraData.sd->extFunc);
            }
        }
        mo = mo->d.superdata;
    }

    return metaObjects;
}

static bool isInternalType(int idx)
{
    // Qt internal types
    switch (idx) {
    case QMetaType::UnknownType:
    case QMetaType::QStringList:
    case QMetaType::QObjectStar:
    case QMetaType::VoidStar:
    case QMetaType::Nullptr:
    case QMetaType::QVariant:
    case QMetaType::QLocale:
    case QMetaType::QImage:  // scarce type, keep as QVariant
    case QMetaType::QPixmap: // scarce type, keep as QVariant
        return true;
    default:
        return false;
    }
}

bool QQmlMetaType::isValueType(QMetaType type)
{
    if (!type.isValid() || isInternalType(type.id()))
        return false;

    return valueType(type) != nullptr;
}

const QMetaObject *QQmlMetaType::metaObjectForValueType(QMetaType metaType)
{
    switch (metaType.id()) {
    case QMetaType::QPoint:
        return &QQmlPointValueType::staticMetaObject;
    case QMetaType::QPointF:
        return &QQmlPointFValueType::staticMetaObject;
    case QMetaType::QSize:
        return &QQmlSizeValueType::staticMetaObject;
    case QMetaType::QSizeF:
        return &QQmlSizeFValueType::staticMetaObject;
    case QMetaType::QRect:
        return &QQmlRectValueType::staticMetaObject;
    case QMetaType::QRectF:
        return &QQmlRectFValueType::staticMetaObject;
#if QT_CONFIG(easingcurve)
    case QMetaType::QEasingCurve:
        return &QQmlEasingValueType::staticMetaObject;
#endif
    default:
        break;
    }

    // It doesn't have to be a gadget for a QML type to exist, but we don't want to
    // call QObject pointers value types. Explicitly registered types also override
    // the implicit use of gadgets.
    if (!(metaType.flags() & QMetaType::PointerToQObject)) {
        if (const QMetaObject *mo = metaObjectForValueType(QQmlMetaType::qmlType(metaType)))
            return mo;
    }

    // If it _is_ a gadget, we can just use it.
    if (metaType.flags() & QMetaType::IsGadget)
        return metaType.metaObject();

    return nullptr;
}

QQmlValueType *QQmlMetaType::valueType(QMetaType type)
{
    QQmlMetaTypeDataPtr data;

    const auto it = data->metaTypeToValueType.constFind(type.id());
    if (it != data->metaTypeToValueType.constEnd())
        return *it;

    if (const QMetaObject *mo = metaObjectForValueType(type))
        return *data->metaTypeToValueType.insert(type.id(), new QQmlValueType(type, mo));
    return *data->metaTypeToValueType.insert(type.id(), nullptr);
}

QQmlPropertyCache::ConstPtr QQmlMetaType::findPropertyCacheInCompositeTypes(QMetaType t)
{
    const QQmlMetaTypeDataPtr data;
    return data->findPropertyCacheInCompositeTypes(t);
}

void QQmlMetaType::registerInternalCompositeType(QV4::ExecutableCompilationUnit *compilationUnit)
{
    compilationUnit->isRegistered = true;

    QQmlMetaTypeDataPtr data;

    // The QQmlCompiledData is not referenced here, but it is removed from this
    // hash in the QQmlCompiledData destructor
    data->compositeTypes.insert(compilationUnit->typeIds.id.iface(), compilationUnit);
    for (auto &&inlineData: compilationUnit->inlineComponentData)
        data->compositeTypes.insert(inlineData.typeIds.id.iface(), compilationUnit);
}

void QQmlMetaType::unregisterInternalCompositeType(QV4::ExecutableCompilationUnit *compilationUnit)
{
    compilationUnit->isRegistered = false;

    QQmlMetaTypeDataPtr data;
    data->compositeTypes.remove(compilationUnit->typeIds.id.iface());
    for (auto&& icDatum: compilationUnit->inlineComponentData)
        data->compositeTypes.remove(icDatum.typeIds.id.iface());
}

QV4::ExecutableCompilationUnit *QQmlMetaType::obtainExecutableCompilationUnit(QMetaType type)
{
    const QQmlMetaTypeDataPtr data;
    return data->compositeTypes.value(type.iface());
}

QT_END_NAMESPACE
