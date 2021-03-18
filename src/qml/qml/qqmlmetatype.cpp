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

#include "qqmlmetatype_p.h"

#include <private/qqmlmetatypedata_p.h>
#include <private/qqmltypemodule_p_p.h>
#include <private/qqmltype_p_p.h>
#include <private/qqmltypeloader_p.h>
#include <private/qqmlextensionplugin_p.h>
#include <private/qv4executablecompilationunit_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qmutex.h>
#include <QtCore/qloggingcategory.h>

Q_DECLARE_LOGGING_CATEGORY(DBG_DISK_CACHE)

QT_BEGIN_NAMESPACE

struct LockedData : private QQmlMetaTypeData
{
    friend class QQmlMetaTypeDataPtr;
};

Q_GLOBAL_STATIC(LockedData, metaTypeData)
Q_GLOBAL_STATIC(QRecursiveMutex, metaTypeDataLock)

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
    QMutexLocker locker;
    LockedData *data = nullptr;
};

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data,
                                       const QQmlPrivate::RegisterInterface &type)
{
    auto *d = new QQmlTypePrivate(QQmlType::InterfaceType);
    d->iid = type.iid;
    d->typeId = type.typeId;
    d->listId = type.listId;
    d->isSetup = true;
    d->version_min = 0;
    if (type.version > 0) {
        d->module = QString::fromUtf8(type.uri);
        d->version_maj = type.versionMajor;
    } else {
        d->version_maj = 0;
    }
    data->registerType(d);
    return d;
}

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data, const QString &elementName,
                                       const QQmlPrivate::RegisterSingletonType &type)
{
    auto *d = new QQmlTypePrivate(QQmlType::SingletonType);
    data->registerType(d);

    d->setName(QString::fromUtf8(type.uri), elementName);
    d->version_maj = type.versionMajor;
    d->version_min = type.versionMinor;

    if (type.qobjectApi || (type.version >= 3 && type.generalizedQobjectApi)) {
        if (type.version >= 1) // static metaobject added in version 1
            d->baseMetaObject = type.instanceMetaObject;
        if (type.version >= 2) // typeId added in version 2
            d->typeId = type.typeId;
        if (type.version >= 2) // revisions added in version 2
            d->revision = type.revision;
    }

    d->extraData.sd->singletonInstanceInfo = new QQmlType::SingletonInstanceInfo;
    d->extraData.sd->singletonInstanceInfo->scriptCallback = type.scriptApi;
    if (type.version >= 3) {
        d->extraData.sd->singletonInstanceInfo->qobjectCallback = type.generalizedQobjectApi;
    } else {
        d->extraData.sd->singletonInstanceInfo->qobjectCallback = type.qobjectApi;
    }
    d->extraData.sd->singletonInstanceInfo->typeName = QString::fromUtf8(type.typeName);
    d->extraData.sd->singletonInstanceInfo->instanceMetaObject
            = ((type.qobjectApi || (type.version >= 3 && type.generalizedQobjectApi) ) && type.version >= 1) ? type.instanceMetaObject : nullptr;

    return d;
}

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data, const QString &elementName,
                                       const QQmlPrivate::RegisterType &type)
{
    QQmlTypePrivate *d = new QQmlTypePrivate(QQmlType::CppType);
    data->registerType(d);
    d->setName(QString::fromUtf8(type.uri), elementName);

    d->version_maj = type.versionMajor;
    d->version_min = type.versionMinor;
    if (type.version >= 1) // revisions added in version 1
        d->revision = type.revision;
    d->typeId = type.typeId;
    d->listId = type.listId;
    d->extraData.cd->allocationSize = type.objectSize;
    d->extraData.cd->newFunc = type.create;
    d->extraData.cd->noCreationReason = type.noCreationReason;
    d->baseMetaObject = type.metaObject;
    d->extraData.cd->attachedPropertiesFunc = type.attachedPropertiesFunction;
    d->extraData.cd->attachedPropertiesType = type.attachedPropertiesMetaObject;
    d->extraData.cd->parserStatusCast = type.parserStatusCast;
    d->extraData.cd->propertyValueSourceCast = type.valueSourceCast;
    d->extraData.cd->propertyValueInterceptorCast = type.valueInterceptorCast;
    d->extraData.cd->extFunc = type.extensionObjectCreate;
    d->extraData.cd->customParser = reinterpret_cast<QQmlCustomParser *>(type.customParser);
    d->extraData.cd->registerEnumClassesUnscoped = true;

    if (type.extensionMetaObject)
        d->extraData.cd->extMetaObject = type.extensionMetaObject;

    // Check if the user wants only scoped enum classes
    if (d->baseMetaObject) {
        auto indexOfClassInfo = d->baseMetaObject->indexOfClassInfo("RegisterEnumClassesUnscoped");
        if (indexOfClassInfo != -1 && QString::fromUtf8(d->baseMetaObject->classInfo(indexOfClassInfo).value()) == QLatin1String("false"))
            d->extraData.cd->registerEnumClassesUnscoped = false;
    }

    return d;
}

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data, const QString &elementName,
                                       const QQmlPrivate::RegisterCompositeType &type)
{
    auto *d = new QQmlTypePrivate(QQmlType::CompositeType);
    data->registerType(d);
    d->setName(QString::fromUtf8(type.uri), elementName);
    d->version_maj = type.versionMajor;
    d->version_min = type.versionMinor;

    d->extraData.fd->url = QQmlTypeLoader::normalize(type.url);
    return d;
}

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data, const QString &elementName,
                                       const QQmlPrivate::RegisterCompositeSingletonType &type)
{
    auto *d = new QQmlTypePrivate(QQmlType::CompositeSingletonType);
    data->registerType(d);
    d->setName(QString::fromUtf8(type.uri), elementName);

    d->version_maj = type.versionMajor;
    d->version_min = type.versionMinor;

    d->extraData.sd->singletonInstanceInfo = new QQmlType::SingletonInstanceInfo;
    d->extraData.sd->singletonInstanceInfo->url = QQmlTypeLoader::normalize(type.url);
    d->extraData.sd->singletonInstanceInfo->typeName = QString::fromUtf8(type.typeName);
    return d;
}

void QQmlMetaType::clone(QMetaObjectBuilder &builder, const QMetaObject *mo,
                         const QMetaObject *ignoreStart, const QMetaObject *ignoreEnd)
{
    // Set classname
    builder.setClassName(ignoreEnd->className());

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

    // Clone Q_PROPERTY
    for (int ii = mo->propertyOffset(); ii < mo->propertyCount(); ++ii) {
        QMetaProperty property = mo->property(ii);

        int otherIndex = ignoreEnd->indexOfProperty(property.name());
        if (otherIndex >= ignoreStart->propertyOffset() + ignoreStart->propertyCount()) {
            builder.addProperty(QByteArray("__qml_ignore__") + property.name(), QByteArray("void"));
            // Skip
        } else {
            builder.addProperty(property);
        }
    }

    // Clone Q_METHODS
    for (int ii = mo->methodOffset(); ii < mo->methodCount(); ++ii) {
        QMetaMethod method = mo->method(ii);

        // More complex - need to search name
        QByteArray name = method.name();


        bool found = false;

        for (int ii = ignoreStart->methodOffset() + ignoreStart->methodCount();
             !found && ii < ignoreEnd->methodOffset() + ignoreEnd->methodCount();
             ++ii) {

            QMetaMethod other = ignoreEnd->method(ii);

            found = name == other.name();
        }

        QMetaMethodBuilder m = builder.addMethod(method);
        if (found) // SKIP
            m.setAccess(QMetaMethod::Private);
    }

    // Clone Q_ENUMS
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

void QQmlMetaType::qmlInsertModuleRegistration(const QString &uri, int majorVersion,
                                               void (*registerFunction)())
{
    const QQmlMetaTypeData::VersionedUri versionedUri(uri, majorVersion);
    QQmlMetaTypeDataPtr data;
    if (data->moduleTypeRegistrationFunctions.contains(versionedUri))
        qFatal("Cannot add multiple registrations for %s %d", qPrintable(uri), majorVersion);
    else
        data->moduleTypeRegistrationFunctions.insert(versionedUri, registerFunction);
}

void QQmlMetaType::qmlRemoveModuleRegistration(const QString &uri, int majorVersion)
{
    const QQmlMetaTypeData::VersionedUri versionedUri(uri, majorVersion);
    QQmlMetaTypeDataPtr data;

    if (!data.isValid())
        return; // shutdown/deletion race. Not a problem.

    if (!data->moduleTypeRegistrationFunctions.contains(versionedUri))
        qFatal("Cannot remove multiple registrations for %s %d", qPrintable(uri), majorVersion);
    else
        data->moduleTypeRegistrationFunctions.remove(versionedUri);
}

bool QQmlMetaType::qmlRegisterModuleTypes(const QString &uri, int majorVersion)
{
    QQmlMetaTypeDataPtr data;
    return data->registerModuleTypes(QQmlMetaTypeData::VersionedUri(uri, majorVersion));
}

/*!
   \internal
    Method is only used to in tst_qqmlenginecleanup.cpp to test whether all
    types have been removed from qmlLists after shutdown of QQmlEngine
 */
int QQmlMetaType::qmlRegisteredListTypeCount()
{
    QQmlMetaTypeDataPtr data;
    return data->qmlLists.count();
}

void QQmlMetaType::clearTypeRegistrations()
{
    //Only cleans global static, assumed no running engine
    QQmlMetaTypeDataPtr data;

    for (QQmlMetaTypeData::TypeModules::const_iterator i = data->uriToModule.constBegin(), cend = data->uriToModule.constEnd(); i != cend; ++i)
        delete *i;

    data->types.clear();
    data->idToType.clear();
    data->nameToType.clear();
    data->urlToType.clear();
    data->typePropertyCaches.clear();
    data->urlToNonFileImportType.clear();
    data->metaObjectToType.clear();
    data->uriToModule.clear();
    data->undeletableTypes.clear();
}

int QQmlMetaType::registerAutoParentFunction(const QQmlPrivate::RegisterAutoParent &autoparent)
{
    QQmlMetaTypeDataPtr data;

    data->parentFunctions.append(autoparent.function);

    return data->parentFunctions.count() - 1;
}

void QQmlMetaType::unregisterAutoParentFunction(const QQmlPrivate::AutoParentFunction &function)
{
    QQmlMetaTypeDataPtr data;
    data->parentFunctions.removeOne(function);
}

QQmlType QQmlMetaType::registerInterface(const QQmlPrivate::RegisterInterface &type)
{
    if (type.version > 1)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;
    QQmlTypePrivate *priv = createQQmlType(data, type);
    Q_ASSERT(priv);

    data->idToType.insert(priv->typeId, priv);
    data->idToType.insert(priv->listId, priv);

    if (data->interfaces.size() <= type.typeId)
        data->interfaces.resize(type.typeId + 16);
    if (data->lists.size() <= type.listId)
        data->lists.resize(type.listId + 16);
    data->interfaces.setBit(type.typeId, true);
    data->lists.setBit(type.listId, true);

    return QQmlType(priv);
}

QString registrationTypeString(QQmlType::RegistrationType typeType)
{
    QString typeStr;
    if (typeType == QQmlType::CppType)
        typeStr = QStringLiteral("element");
    else if (typeType == QQmlType::SingletonType)
        typeStr = QStringLiteral("singleton type");
    else if (typeType == QQmlType::CompositeSingletonType)
        typeStr = QStringLiteral("composite singleton type");
    else
        typeStr = QStringLiteral("type");
    return typeStr;
}

// NOTE: caller must hold a QMutexLocker on "data"
bool checkRegistration(QQmlType::RegistrationType typeType, QQmlMetaTypeData *data,
                       const char *uri, const QString &typeName, int majorVersion)
{
    if (!typeName.isEmpty()) {
        if (typeName.at(0).isLower()) {
            QString failure(QCoreApplication::translate("qmlRegisterType", "Invalid QML %1 name \"%2\"; type names must begin with an uppercase letter"));
            data->recordTypeRegFailure(failure.arg(registrationTypeString(typeType)).arg(typeName));
            return false;
        }

        int typeNameLen = typeName.length();
        for (int ii = 0; ii < typeNameLen; ++ii) {
            if (!(typeName.at(ii).isLetterOrNumber() || typeName.at(ii) == '_')) {
                QString failure(QCoreApplication::translate("qmlRegisterType", "Invalid QML %1 name \"%2\""));
                data->recordTypeRegFailure(failure.arg(registrationTypeString(typeType)).arg(typeName));
                return false;
            }
        }
    }

    if (uri && !typeName.isEmpty()) {
        QString nameSpace = QString::fromUtf8(uri);
        QQmlMetaTypeData::VersionedUri versionedUri;
        versionedUri.uri = nameSpace;
        versionedUri.majorVersion = majorVersion;
        if (QQmlTypeModule* qqtm = data->uriToModule.value(versionedUri, 0)){
            if (qqtm->isLocked()){
                QString failure(QCoreApplication::translate("qmlRegisterType",
                                                            "Cannot install %1 '%2' into protected module '%3' version '%4'"));
                data->recordTypeRegFailure(failure.arg(registrationTypeString(typeType)).arg(typeName).arg(nameSpace).arg(majorVersion));
                return false;
            }
        }
    }

    return true;
}

// NOTE: caller must hold a QMutexLocker on "data"
QQmlTypeModule *getTypeModule(const QHashedString &uri, int majorVersion, QQmlMetaTypeData *data)
{
    QQmlMetaTypeData::VersionedUri versionedUri(uri, majorVersion);
    QQmlTypeModule *module = data->uriToModule.value(versionedUri);
    if (!module) {
        module = new QQmlTypeModule(versionedUri.uri, versionedUri.majorVersion);
        data->uriToModule.insert(versionedUri, module);
    }
    return module;
}

// NOTE: caller must hold a QMutexLocker on "data"
void addTypeToData(QQmlTypePrivate *type, QQmlMetaTypeData *data)
{
    Q_ASSERT(type);

    if (!type->elementName.isEmpty())
        data->nameToType.insert(type->elementName, type);

    if (type->baseMetaObject)
        data->metaObjectToType.insert(type->baseMetaObject, type);

    if (type->typeId) {
        data->idToType.insert(type->typeId, type);
        if (data->objects.size() <= type->typeId)
            data->objects.resize(type->typeId + 16);
        data->objects.setBit(type->typeId, true);
    }

    if (type->listId) {
        if (data->lists.size() <= type->listId)
            data->lists.resize(type->listId + 16);
        data->lists.setBit(type->listId, true);
        data->idToType.insert(type->listId, type);
    }

    if (!type->module.isEmpty()) {
        const QHashedString &mod = type->module;

        QQmlTypeModule *module = getTypeModule(mod, type->version_maj, data);
        Q_ASSERT(module);
        module->add(type);
    }
}

QQmlType QQmlMetaType::registerType(const QQmlPrivate::RegisterType &type)
{
    QQmlMetaTypeDataPtr data;

    QString elementName = QString::fromUtf8(type.elementName);
    if (!checkRegistration(QQmlType::CppType, data, type.uri, elementName, type.versionMajor))
        return QQmlType();

    QQmlTypePrivate *priv = createQQmlType(data, elementName, type);

    addTypeToData(priv, data);
    if (!type.typeId)
        data->idToType.insert(priv->typeId, priv);

    return QQmlType(priv);
}

QQmlType QQmlMetaType::registerSingletonType(const QQmlPrivate::RegisterSingletonType &type)
{
    QQmlMetaTypeDataPtr data;

    QString typeName = QString::fromUtf8(type.typeName);
    if (!checkRegistration(QQmlType::SingletonType, data, type.uri, typeName, type.versionMajor))
        return QQmlType();

    QQmlTypePrivate *priv = createQQmlType(data, typeName, type);

    addTypeToData(priv, data);

    return QQmlType(priv);
}

QQmlType QQmlMetaType::registerCompositeSingletonType(const QQmlPrivate::RegisterCompositeSingletonType &type)
{
    // Assumes URL is absolute and valid. Checking of user input should happen before the URL enters type.
    QQmlMetaTypeDataPtr data;

    QString typeName = QString::fromUtf8(type.typeName);
    bool fileImport = false;
    if (*(type.uri) == '\0')
        fileImport = true;
    if (!checkRegistration(QQmlType::CompositeSingletonType, data, fileImport ? nullptr : type.uri,
                           typeName, type.versionMajor)) {
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
    // Assumes URL is absolute and valid. Checking of user input should happen before the URL enters type.
    QQmlMetaTypeDataPtr data;

    QString typeName = QString::fromUtf8(type.typeName);
    bool fileImport = false;
    if (*(type.uri) == '\0')
        fileImport = true;
    if (!checkRegistration(QQmlType::CompositeType, data, fileImport?nullptr:type.uri, typeName, type.versionMajor))
        return QQmlType();

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

    int ptr_type = QMetaType::registerNormalizedType(ptr,
                                                     QtMetaTypePrivate::QMetaTypeFunctionHelper<QObject*>::Destruct,
                                                     QtMetaTypePrivate::QMetaTypeFunctionHelper<QObject*>::Construct,
                                                     sizeof(QObject*),
                                                     static_cast<QFlags<QMetaType::TypeFlag> >(QtPrivate::QMetaTypeTypeFlags<QObject*>::Flags),
                                                     nullptr);
    int lst_type = QMetaType::registerNormalizedType(lst,
                                                     QtMetaTypePrivate::QMetaTypeFunctionHelper<QQmlListProperty<QObject> >::Destruct,
                                                     QtMetaTypePrivate::QMetaTypeFunctionHelper<QQmlListProperty<QObject> >::Construct,
                                                     sizeof(QQmlListProperty<QObject>),
                                                     static_cast<QFlags<QMetaType::TypeFlag> >(QtPrivate::QMetaTypeTypeFlags<QQmlListProperty<QObject> >::Flags),
                                                     static_cast<QMetaObject*>(nullptr));

    QQmlMetaTypeDataPtr data;
    data->qmlLists.insert(lst_type, ptr_type);

    return {ptr_type, lst_type};
}

void QQmlMetaType::unregisterInternalCompositeType(const CompositeMetaTypeIds &typeIds)
{
    QQmlMetaTypeDataPtr data;
    data->qmlLists.remove(typeIds.listId);

    QMetaType::unregisterType(typeIds.id);
    QMetaType::unregisterType(typeIds.listId);
}

int QQmlMetaType::registerUnitCacheHook(
        const QQmlPrivate::RegisterQmlUnitCacheHook &hookRegistration)
{
    if (hookRegistration.version > 0)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;
    data->lookupCachedQmlUnit << hookRegistration.lookupCachedQmlUnit;
    return 0;
}

bool QQmlMetaType::protectModule(const QString &uri, int majVersion)
{
    QQmlMetaTypeDataPtr data;

    QQmlMetaTypeData::VersionedUri versionedUri;
    versionedUri.uri = uri;
    versionedUri.majorVersion = majVersion;

    if (QQmlTypeModule* qqtm = data->uriToModule.value(versionedUri, 0)) {
        qqtm->lock();
        return true;
    }
    return false;
}

void QQmlMetaType::registerModule(const char *uri, int versionMajor, int versionMinor)
{
    QQmlMetaTypeDataPtr data;

    QQmlTypeModule *module = getTypeModule(QString::fromUtf8(uri), versionMajor, data);
    Q_ASSERT(module);

    module->addMinorVersion(versionMinor);
}

int QQmlMetaType::typeId(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    QQmlMetaTypeDataPtr data;

    QQmlTypeModule *module = getTypeModule(QString::fromUtf8(uri), versionMajor, data);
    if (!module)
        return -1;

    QQmlType type = module->type(QHashedStringRef(QString::fromUtf8(qmlName)), versionMinor);
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
                                           int majorVersion)
{
    // Has any type previously been installed to this namespace?
    QHashedString nameSpace(uri);
    for (const QQmlType &type : data->types) {
        if (type.module() == nameSpace && type.majorVersion() == majorVersion)
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


bool QQmlMetaType::registerPluginTypes(QObject *instance, const QString &basePath,
                                       const QString &uri, const QString &typeNamespace, int vmaj,
                                       QList<QQmlError> *errors)
{
    if (!typeNamespace.isEmpty() && typeNamespace != uri) {
        // This is an 'identified' module
        // The namespace for type registrations must match the URI for locating the module
        if (errors) {
            QQmlError error;
            error.setDescription(
                    QStringLiteral("Module namespace '%1' does not match import URI '%2'")
                            .arg(typeNamespace).arg(uri));
            errors->prepend(error);
        }
        return false;
    }

    QStringList failures;
    QQmlMetaTypeDataPtr data;
    {
        QQmlMetaTypeRegistrationFailureRecorder failureRecorder(data, &failures);
        if (!typeNamespace.isEmpty()) {
            // This is an 'identified' module
            if (namespaceContainsRegistrations(data, typeNamespace, vmaj)) {
                // Other modules have already installed to this namespace
                if (errors) {
                    QQmlError error;
                    error.setDescription(QStringLiteral("Namespace '%1' has already been used "
                                                        "for type registration")
                                                 .arg(typeNamespace));
                    errors->prepend(error);
                }
                return false;
            }
        } else {
            // This is not an identified module - provide a warning
            qWarning().nospace() << qPrintable(
                    QStringLiteral("Module '%1' does not contain a module identifier directive - "
                                   "it cannot be protected from external registrations.").arg(uri));
        }

        if (!qobject_cast<QQmlEngineExtensionInterface *>(instance)) {
            QQmlTypesExtensionInterface *iface = qobject_cast<QQmlTypesExtensionInterface *>(instance);
            if (!iface) {
                if (errors) {
                    QQmlError error;
                    // Also does not implement QQmlTypesExtensionInterface, but we want to discourage that.
                    error.setDescription(QStringLiteral("Module loaded for URI '%1' does not implement "
                                                        "QQmlEngineExtensionInterface").arg(typeNamespace));
                    errors->prepend(error);
                }
                return false;
            }

            if (auto *plugin = qobject_cast<QQmlExtensionPlugin *>(instance)) {
                // basepath should point to the directory of the module, not the plugin file itself:
                QQmlExtensionPluginPrivate::get(plugin)->baseUrl
                        = QQmlImports::urlFromLocalFileOrQrcOrUrl(basePath);
            }

            const QByteArray bytes = uri.toUtf8();
            const char *moduleId = bytes.constData();
            iface->registerTypes(moduleId);
        }

        data->registerModuleTypes(QQmlMetaTypeData::VersionedUri(uri, vmaj));

        if (!failures.isEmpty()) {
            if (errors) {
                for (const QString &failure : qAsConst(failures)) {
                    QQmlError error;
                    error.setDescription(failure);
                    errors->prepend(error);
                }
            }
            return false;
        }
    }

    return true;
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
                                  int majorVersion, int minorVersion)
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
    if (checkRegistration(registrationType, data, nullptr, typeName, majorVersion)) {
        auto *priv = new QQmlTypePrivate(registrationType);
        priv->setName(QString(), typeName);
        priv->version_maj = majorVersion;
        priv->version_min = minorVersion;

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
        error.setDescription(failures.join('\n'));
        errors->prepend(error);
    } else {
        qWarning("%s", failures.join('\n').toLatin1().constData());
    }
    return QQmlType();
}

QRecursiveMutex *QQmlMetaType::typeRegistrationLock()
{
    return metaTypeDataLock();
}

/*
    Returns true if a module \a uri of any version is installed.
*/
bool QQmlMetaType::isAnyModule(const QString &uri)
{
    QQmlMetaTypeDataPtr data;

    for (QQmlMetaTypeData::TypeModules::ConstIterator iter = data->uriToModule.cbegin();
         iter != data->uriToModule.cend(); ++iter) {
        if ((*iter)->module() == uri)
            return true;
    }

    return false;
}

/*
    Returns true if a module \a uri of this version is installed and locked;
*/
bool QQmlMetaType::isLockedModule(const QString &uri, int majVersion)
{
    QQmlMetaTypeDataPtr data;

    QQmlMetaTypeData::VersionedUri versionedUri;
    versionedUri.uri = uri;
    versionedUri.majorVersion = majVersion;
    if (QQmlTypeModule* qqtm = data->uriToModule.value(versionedUri, 0))
        return qqtm->isLocked();
    return false;
}

/*
    Returns true if any type or API has been registered for the given \a module with at least
    versionMajor.versionMinor, or if types have been registered for \a module with at most
    versionMajor.versionMinor.

    So if only 4.7 and 4.9 have been registered, 4.7,4.8, and 4.9 are valid, but not 4.6 nor 4.10.
*/
bool QQmlMetaType::isModule(const QString &module, int versionMajor, int versionMinor)
{
    Q_ASSERT(versionMajor >= 0 && versionMinor >= 0);
    QQmlMetaTypeDataPtr data;

    // first, check Types
    QQmlTypeModule *tm =
        data->uriToModule.value(QQmlMetaTypeData::VersionedUri(module, versionMajor));
    if (tm && tm->minimumMinorVersion() <= versionMinor && tm->maximumMinorVersion() >= versionMinor)
        return true;

    return false;
}

QQmlTypeModule *QQmlMetaType::typeModule(const QString &uri, int majorVersion)
{
    QQmlMetaTypeDataPtr data;
    return data->uriToModule.value(QQmlMetaTypeData::VersionedUri(uri, majorVersion));
}

QList<QQmlPrivate::AutoParentFunction> QQmlMetaType::parentFunctions()
{
    QQmlMetaTypeDataPtr data;
    return data->parentFunctions;
}

QObject *QQmlMetaType::toQObject(const QVariant &v, bool *ok)
{
    if (!isQObject(v.userType())) {
        if (ok) *ok = false;
        return nullptr;
    }

    if (ok) *ok = true;

    return *(QObject *const *)v.constData();
}

bool QQmlMetaType::isQObject(int userType)
{
    if (userType == QMetaType::QObjectStar)
        return true;

    QQmlMetaTypeDataPtr data;
    return userType >= 0 && userType < data->objects.size() && data->objects.testBit(userType);
}

/*
    Returns the item type for a list of type \a id.
 */
int QQmlMetaType::listType(int id)
{
    QQmlMetaTypeDataPtr data;
    QHash<int, int>::ConstIterator iter = data->qmlLists.constFind(id);
    if (iter != data->qmlLists.cend())
        return *iter;
    QQmlTypePrivate *type = data->idToType.value(id);
    if (type && type->listId == id)
        return type->typeId;
    else
        return 0;
}

#if QT_DEPRECATED_SINCE(5, 14)
int QQmlMetaType::attachedPropertiesFuncId(QQmlEnginePrivate *engine, const QMetaObject *mo)
{
    QQmlMetaTypeDataPtr data;

    for (auto it = data->metaObjectToType.constFind(mo), end = data->metaObjectToType.constEnd();
         it != end && it.key() == mo; ++it) {
        if (const QQmlTypePrivate *type = it.value()) {
            if (const QQmlTypePrivate *base = type->attachedPropertiesBase(engine))
                return base->index;
        }
    }
    return -1;
}

QQmlAttachedPropertiesFunc QQmlMetaType::attachedPropertiesFuncById(QQmlEnginePrivate *engine, int id)
{
    if (id < 0)
        return nullptr;
    QQmlMetaTypeDataPtr data;
    return data->types.at(id).attachedPropertiesFunction(engine);
}
#endif

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

QQmlMetaType::TypeCategory QQmlMetaType::typeCategory(int userType)
{
    if (userType < 0)
        return Unknown;
    if (userType == QMetaType::QObjectStar)
        return Object;

    QQmlMetaTypeDataPtr data;
    if (data->qmlLists.contains(userType))
        return List;
    else if (userType < data->objects.size() && data->objects.testBit(userType))
        return Object;
    else if (userType < data->lists.size() && data->lists.testBit(userType))
        return List;
    else
        return Unknown;
}

/*!
    See qmlRegisterInterface() for information about when this will return true.
*/
bool QQmlMetaType::isInterface(int userType)
{
    const QQmlMetaTypeDataPtr data;
    return userType >= 0 && userType < data->interfaces.size() && data->interfaces.testBit(userType);
}

const char *QQmlMetaType::interfaceIId(int userType)
{

    QQmlTypePrivate *typePrivate = nullptr;
    {
        QQmlMetaTypeDataPtr data;
        typePrivate = data->idToType.value(userType);
    }

    QQmlType type(typePrivate);
    if (type.isInterface() && type.typeId() == userType)
        return type.interfaceIId();
    else
        return nullptr;
}

bool QQmlMetaType::isList(int userType)
{
    const QQmlMetaTypeDataPtr data;
    if (data->qmlLists.contains(userType))
        return true;
    return userType >= 0 && userType < data->lists.size() && data->lists.testBit(userType);
}

/*!
    A custom string convertor allows you to specify a function pointer that
    returns a variant of \a type. For example, if you have written your own icon
    class that you want to support as an object property assignable in QML:

    \code
    int type = qRegisterMetaType<SuperIcon>("SuperIcon");
    QML::addCustomStringConvertor(type, &SuperIcon::pixmapFromString);
    \endcode

    The function pointer must be of the form:
    \code
    QVariant (*StringConverter)(const QString &);
    \endcode
 */
void QQmlMetaType::registerCustomStringConverter(int type, StringConverter converter)
{
    QQmlMetaTypeDataPtr data;
    if (data->stringConverters.contains(type))
        return;
    data->stringConverters.insert(type, converter);
}

/*!
    Return the custom string converter for \a type, previously installed through
    registerCustomStringConverter()
 */
QQmlMetaType::StringConverter QQmlMetaType::customStringConverter(int type)
{
    const QQmlMetaTypeDataPtr data;
    return data->stringConverters.value(type);
}

/*!
    Returns the type (if any) of URI-qualified named \a qualifiedName and version specified
    by \a version_major and \a version_minor.
*/
QQmlType QQmlMetaType::qmlType(const QString &qualifiedName, int version_major, int version_minor)
{
    int slash = qualifiedName.indexOf(QLatin1Char('/'));
    if (slash <= 0)
        return QQmlType();

    QHashedStringRef module(qualifiedName.constData(), slash);
    QHashedStringRef name(qualifiedName.constData() + slash + 1, qualifiedName.length() - slash - 1);

    return qmlType(name, module, version_major, version_minor);
}

/*!
    Returns the type (if any) of \a name in \a module and version specified
    by \a version_major and \a version_minor.
*/
QQmlType QQmlMetaType::qmlType(const QHashedStringRef &name, const QHashedStringRef &module, int version_major, int version_minor)
{
    Q_ASSERT(version_major >= 0 && version_minor >= 0);
    const QQmlMetaTypeDataPtr data;

    QQmlMetaTypeData::Names::ConstIterator it = data->nameToType.constFind(name);
    while (it != data->nameToType.cend() && it.key() == name) {
        QQmlType t(*it);
        // XXX version_major<0 just a kludge for QQmlPropertyPrivate::initProperty
        if (version_major < 0 || module.isEmpty() || t.availableInVersion(module, version_major,version_minor))
            return t;
        ++it;
    }

    return QQmlType();
}

/*!
    Returns the type (if any) that corresponds to the \a metaObject.  Returns null if no
    type is registered.
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
QQmlType QQmlMetaType::qmlType(const QMetaObject *metaObject, const QHashedStringRef &module, int version_major, int version_minor)
{
    Q_ASSERT(version_major >= 0 && version_minor >= 0);
    const QQmlMetaTypeDataPtr data;

    QQmlMetaTypeData::MetaObjects::const_iterator it = data->metaObjectToType.constFind(metaObject);
    while (it != data->metaObjectToType.cend() && it.key() == metaObject) {
        QQmlType t(*it);
        if (version_major < 0 || module.isEmpty() || t.availableInVersion(module, version_major,version_minor))
            return t;
        ++it;
    }

    return QQmlType();
}

/*!
    Returns the type (if any) that corresponds to \a typeId.  Depending on \a category, the
    \a typeId is interpreted either as QVariant::Type or as QML type id returned by one of the
    qml type registration functions.  Returns null if no type is registered.
*/
QQmlType QQmlMetaType::qmlType(int typeId, TypeIdCategory category)
{
    const QQmlMetaTypeDataPtr data;

    if (category == TypeIdCategory::MetaType) {
        QQmlTypePrivate *type = data->idToType.value(typeId);
        if (type && type->typeId == typeId)
            return QQmlType(type);
    } else if (category == TypeIdCategory::QmlType) {
        QQmlType type = data->types.value(typeId);
        if (type.isValid())
            return type;
    }
    return QQmlType();
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

QQmlPropertyCache *QQmlMetaType::propertyCache(const QMetaObject *metaObject, int minorVersion, bool doRef)
{
    QQmlMetaTypeDataPtr data; // not const: the cache is created on demand
    auto ret =  data->propertyCache(metaObject, minorVersion);
    if (doRef)
        return ret.take();
    else
        return ret.data();
}

QQmlPropertyCache *QQmlMetaType::propertyCache(const QQmlType &type, int minorVersion)
{
    QQmlMetaTypeDataPtr data; // not const: the cache is created on demand
    return data->propertyCache(type, minorVersion);
}

void QQmlMetaType::unregisterType(int typeIndex)
{
    QQmlMetaTypeDataPtr data;
    const QQmlType type = data->types.value(typeIndex);
    if (const QQmlTypePrivate *d = type.priv()) {
        removeQQmlTypePrivate(data->idToType, d);
        removeQQmlTypePrivate(data->nameToType, d);
        removeQQmlTypePrivate(data->urlToType, d);
        removeQQmlTypePrivate(data->urlToNonFileImportType, d);
        removeQQmlTypePrivate(data->metaObjectToType, d);
        for (auto & module : data->uriToModule)
            module->remove(d);
        data->clearPropertyCachesForMinorVersion(typeIndex);
        data->types[typeIndex] = QQmlType();
        data->undeletableTypes.remove(type);
    }
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
            if (d && d->count() == 1) {
                deletedAtLeastOneType = true;

                removeQQmlTypePrivate(data->idToType, d);
                removeQQmlTypePrivate(data->nameToType, d);
                removeQQmlTypePrivate(data->urlToType, d);
                removeQQmlTypePrivate(data->urlToNonFileImportType, d);
                removeQQmlTypePrivate(data->metaObjectToType, d);

                for (auto &module : data->uriToModule)
                    module->remove(d);

                data->clearPropertyCachesForMinorVersion(d->index);
                *it = QQmlType();
            } else {
                ++it;
            }
        }
    } while (deletedAtLeastOneType);

    bool deletedAtLeastOneCache;
    do {
        deletedAtLeastOneCache = false;
        QHash<const QMetaObject *, QQmlPropertyCache *>::Iterator it = data->propertyCaches.begin();
        while (it != data->propertyCaches.end()) {

            if ((*it)->count() == 1) {
                QQmlPropertyCache *pc = nullptr;
                qSwap(pc, *it);
                it = data->propertyCaches.erase(it);
                pc->release();
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
    names.reserve(data->nameToType.count());
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
    for (QQmlTypePrivate *t : data->nameToType)
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
    for (const auto t : qAsConst(data->nameToType)) {
        QQmlType type(t);
        if (type.isSingleton())
            retn.append(type);
    }
    return retn;
}

const QV4::CompiledData::Unit *QQmlMetaType::findCachedCompilationUnit(const QUrl &uri, CachedUnitLookupError *status)
{
    const QQmlMetaTypeDataPtr data;

    for (const auto lookup : qAsConst(data->lookupCachedQmlUnit)) {
        if (const QQmlPrivate::CachedQmlUnit *unit = lookup(uri)) {
            QString error;
            if (!QV4::ExecutableCompilationUnit::verifyHeader(unit->qmlData, QDateTime(), &error)) {
                qCDebug(DBG_DISK_CACHE) << "Error loading pre-compiled file " << uri << ":" << error;
                if (status)
                    *status = CachedUnitLookupError::VersionMismatch;
                return nullptr;
            }
            if (status)
                *status = CachedUnitLookupError::NoError;
            return unit->qmlData;
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
            typeName = typeName.leftRef(marker) + QLatin1Char('*');
            type = QQmlMetaType::qmlType(QMetaType::type(typeName.toLatin1()));
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

    while (mo) {
        QQmlTypePrivate *t = data->metaObjectToType.value(mo);
        if (t) {
            if (t->regType == QQmlType::CppType) {
                if (t->extraData.cd->extFunc) {
                    QMetaObjectBuilder builder;
                    clone(builder, t->extraData.cd->extMetaObject, t->baseMetaObject, baseMetaObject);
                    builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);
                    QMetaObject *mmo = builder.toMetaObject();
                    mmo->d.superdata = baseMetaObject;
                    if (!metaObjects.isEmpty())
                        metaObjects.constLast().metaObject->d.superdata = mmo;
                    else if (lastMetaObject)
                        lastMetaObject->d.superdata = mmo;
                    QQmlProxyMetaObject::ProxyData data = { mmo, t->extraData.cd->extFunc, 0, 0 };
                    metaObjects << data;
                }
            }
        }
        mo = mo->d.superdata;
    }

    return metaObjects;
}

QT_END_NAMESPACE
