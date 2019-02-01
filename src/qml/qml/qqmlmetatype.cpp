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
Q_GLOBAL_STATIC_WITH_ARGS(QMutex, metaTypeDataLock, (QMutex::Recursive))

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

private:
    QMutexLocker locker;
    LockedData *data = nullptr;
};

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
    data->urlToNonFileImportType.clear();
    data->metaObjectToType.clear();
    data->uriToModule.clear();
    data->undeletableTypes.clear();
}

int QQmlMetaType::registerAutoParentFunction(QQmlPrivate::RegisterAutoParent &autoparent)
{
    QQmlMetaTypeDataPtr data;

    data->parentFunctions.append(autoparent.function);

    return data->parentFunctions.count() - 1;
}

QQmlType QQmlMetaType::registerInterface(const QQmlPrivate::RegisterInterface &type)
{
    if (type.version > 0)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;

    QQmlType dtype(data, type);
    QQmlTypePrivate *priv = dtype.priv();
    Q_ASSERT(priv);

    data->idToType.insert(priv->typeId, priv);
    data->idToType.insert(priv->listId, priv);
    // XXX No insertMulti, so no multi-version interfaces?
    if (!priv->elementName.isEmpty())
        data->nameToType.insert(priv->elementName, priv);

    if (data->interfaces.size() <= type.typeId)
        data->interfaces.resize(type.typeId + 16);
    if (data->lists.size() <= type.listId)
        data->lists.resize(type.listId + 16);
    data->interfaces.setBit(type.typeId, true);
    data->lists.setBit(type.listId, true);

    return dtype;
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
                       const char *uri, const QString &typeName, int majorVersion = -1)
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

        if (data->typeRegistrationNamespace.isEmpty() && !nameSpace.isEmpty()) {
            // Is the target namespace protected against further registrations?
            if (data->protectedNamespaces.contains(nameSpace)) {
                QString failure(QCoreApplication::translate("qmlRegisterType",
                                                            "Cannot install %1 '%2' into protected namespace '%3'"));
                data->recordTypeRegFailure(failure.arg(registrationTypeString(typeType)).arg(typeName).arg(nameSpace));
                return false;
            }
        } else if (majorVersion >= 0) {
            QQmlMetaTypeData::VersionedUri versionedUri;
            versionedUri.uri = nameSpace;
            versionedUri.majorVersion = majorVersion;
            if (QQmlTypeModule* qqtm = data->uriToModule.value(versionedUri, 0)){
                if (QQmlTypeModulePrivate::get(qqtm)->locked){
                    QString failure(QCoreApplication::translate("qmlRegisterType",
                                                                "Cannot install %1 '%2' into protected module '%3' version '%4'"));
                    data->recordTypeRegFailure(failure.arg(registrationTypeString(typeType)).arg(typeName).arg(nameSpace).arg(majorVersion));
                    return false;
                }
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
        module = new QQmlTypeModule;
        module->d->uri = versionedUri;
        data->uriToModule.insert(versionedUri, module);
    }
    return module;
}

// NOTE: caller must hold a QMutexLocker on "data"
void addTypeToData(QQmlTypePrivate *type, QQmlMetaTypeData *data)
{
    Q_ASSERT(type);

    if (!type->elementName.isEmpty())
        data->nameToType.insertMulti(type->elementName, type);

    if (type->baseMetaObject)
        data->metaObjectToType.insertMulti(type->baseMetaObject, type);

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
        module->d->add(type);
    }
}

QQmlType QQmlMetaType::registerType(const QQmlPrivate::RegisterType &type)
{
    QQmlMetaTypeDataPtr data;

    QString elementName = QString::fromUtf8(type.elementName);
    if (!checkRegistration(QQmlType::CppType, data, type.uri, elementName, type.versionMajor))
        return QQmlType();

    QQmlType dtype(data, elementName, type);

    addTypeToData(dtype.priv(), data);
    if (!type.typeId)
        data->idToType.insert(dtype.typeId(), dtype.priv());

    return dtype;
}

QQmlType QQmlMetaType::registerSingletonType(const QQmlPrivate::RegisterSingletonType &type)
{
    QQmlMetaTypeDataPtr data;

    QString typeName = QString::fromUtf8(type.typeName);
    if (!checkRegistration(QQmlType::SingletonType, data, type.uri, typeName, type.versionMajor))
        return QQmlType();

    QQmlType dtype(data, typeName, type);

    addTypeToData(dtype.priv(), data);

    return dtype;
}

QQmlType QQmlMetaType::registerCompositeSingletonType(const QQmlPrivate::RegisterCompositeSingletonType &type)
{
    // Assumes URL is absolute and valid. Checking of user input should happen before the URL enters type.
    QQmlMetaTypeDataPtr data;

    QString typeName = QString::fromUtf8(type.typeName);
    bool fileImport = false;
    if (*(type.uri) == '\0')
        fileImport = true;
    if (!checkRegistration(QQmlType::CompositeSingletonType, data, fileImport ? nullptr : type.uri, typeName))
        return QQmlType();

    QQmlType dtype(data, typeName, type);

    addTypeToData(dtype.priv(), data);

    QQmlMetaTypeData::Files *files = fileImport ? &(data->urlToType) : &(data->urlToNonFileImportType);
    files->insertMulti(QQmlTypeLoader::normalize(type.url), dtype.priv());

    return dtype;
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

    QQmlType dtype(data, typeName, type);
    addTypeToData(dtype.priv(), data);

    QQmlMetaTypeData::Files *files = fileImport ? &(data->urlToType) : &(data->urlToNonFileImportType);
    files->insertMulti(QQmlTypeLoader::normalize(type.url), dtype.priv());

    return dtype;
}

void QQmlMetaType::registerInternalCompositeType(QV4::CompiledData::CompilationUnit *compilationUnit)
{
    QByteArray name = compilationUnit->rootPropertyCache()->className();

    QByteArray ptr = name + '*';
    QByteArray lst = "QQmlListProperty<" + name + '>';

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

    compilationUnit->metaTypeId = ptr_type;
    compilationUnit->listMetaTypeId = lst_type;

    QQmlMetaTypeDataPtr data;
    data->qmlLists.insert(lst_type, ptr_type);
}

void QQmlMetaType::unregisterInternalCompositeType(QV4::CompiledData::CompilationUnit *compilationUnit)
{
    int ptr_type = compilationUnit->metaTypeId;
    int lst_type = compilationUnit->listMetaTypeId;

    QQmlMetaTypeDataPtr data;
    data->qmlLists.remove(lst_type);

    QMetaType::unregisterType(ptr_type);
    QMetaType::unregisterType(lst_type);
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

bool QQmlMetaType::protectModule(const char *uri, int majVersion)
{
    QQmlMetaTypeDataPtr data;

    QQmlMetaTypeData::VersionedUri versionedUri;
    versionedUri.uri = QString::fromUtf8(uri);
    versionedUri.majorVersion = majVersion;

    if (QQmlTypeModule* qqtm = data->uriToModule.value(versionedUri, 0)) {
        QQmlTypeModulePrivate::get(qqtm)->locked = true;
        return true;
    }
    return false;
}

void QQmlMetaType::registerModule(const char *uri, int versionMajor, int versionMinor)
{
    QQmlMetaTypeDataPtr data;

    QQmlTypeModule *module = getTypeModule(QString::fromUtf8(uri), versionMajor, data);
    Q_ASSERT(module);

    QQmlTypeModulePrivate *p = QQmlTypeModulePrivate::get(module);
    p->minMinorVersion = qMin(p->minMinorVersion, versionMinor);
    p->maxMinorVersion = qMax(p->maxMinorVersion, versionMinor);
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

bool QQmlMetaType::namespaceContainsRegistrations(const QString &uri, int majorVersion)
{
    const QQmlMetaTypeDataPtr data;

    // Has any type previously been installed to this namespace?
    QHashedString nameSpace(uri);
    for (const QQmlType &type : data->types)
        if (type.module() == nameSpace && type.majorVersion() == majorVersion)
            return true;

    return false;
}

void QQmlMetaType::protectNamespace(const QString &uri)
{
    QQmlMetaTypeDataPtr data;
    data->protectedNamespaces.insert(uri);
}

void QQmlMetaType::setTypeRegistrationNamespace(const QString &uri)
{
    QQmlMetaTypeDataPtr data;
    data->typeRegistrationNamespace = uri;
}

QMutex *QQmlMetaType::typeRegistrationLock()
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
        return QQmlTypeModulePrivate::get(qqtm)->locked;
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

int QQmlMetaType::attachedPropertiesFuncId(QQmlEnginePrivate *engine, const QMetaObject *mo)
{
    QQmlMetaTypeDataPtr data;

    for (auto it = data->metaObjectToType.constFind(mo), end = data->metaObjectToType.constEnd();
         it != end && it.key() == mo; ++it) {
        const QQmlType type(it.value());
        if (type.attachedPropertiesFunction(engine))
            return type.attachedPropertiesId(engine);
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

QQmlPropertyCache *QQmlMetaType::propertyCache(const QMetaObject *metaObject)
{
    QQmlMetaTypeDataPtr data; // not const: the cache is created on demand
    return data->propertyCache(metaObject);
}

QQmlPropertyCache *QQmlMetaType::propertyCache(const QQmlType &type, int minorVersion)
{
    QQmlMetaTypeDataPtr data; // not const: the cache is created on demand
    return data->propertyCache(type, minorVersion);
}

void QQmlMetaType::unregisterType(int typeIndex)
{
    QQmlMetaTypeDataPtr data;
    {
        const QQmlTypePrivate *d = data->types.value(typeIndex).priv();
        if (d) {
            removeQQmlTypePrivate(data->idToType, d);
            removeQQmlTypePrivate(data->nameToType, d);
            removeQQmlTypePrivate(data->urlToType, d);
            removeQQmlTypePrivate(data->urlToNonFileImportType, d);
            removeQQmlTypePrivate(data->metaObjectToType, d);
            for (QQmlMetaTypeData::TypeModules::Iterator module = data->uriToModule.begin(); module != data->uriToModule.end(); ++module) {
                 QQmlTypeModulePrivate *modulePrivate = (*module)->priv();
                 modulePrivate->remove(d);
            }
            data->types[typeIndex] = QQmlType();
        }
    }
}

void QQmlMetaType::freeUnusedTypesAndCaches()
{
    QQmlMetaTypeDataPtr data;

    {
        bool deletedAtLeastOneType;
        do {
            deletedAtLeastOneType = false;
            QList<QQmlType>::Iterator it = data->types.begin();
            while (it != data->types.end()) {
                const QQmlTypePrivate *d = (*it).priv();
                if (d && d->refCount == 1) {
                    deletedAtLeastOneType = true;

                    removeQQmlTypePrivate(data->idToType, d);
                    removeQQmlTypePrivate(data->nameToType, d);
                    removeQQmlTypePrivate(data->urlToType, d);
                    removeQQmlTypePrivate(data->urlToNonFileImportType, d);
                    removeQQmlTypePrivate(data->metaObjectToType, d);

                    for (QQmlMetaTypeData::TypeModules::Iterator module = data->uriToModule.begin(); module != data->uriToModule.end(); ++module) {
                        QQmlTypeModulePrivate *modulePrivate = (*module)->priv();
                        modulePrivate->remove(d);
                    }

                    *it = QQmlType();
                } else {
                    ++it;
                }
            }
        } while (deletedAtLeastOneType);
    }

    {
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
            if (!unit->qmlData->verifyHeader(QDateTime(), &error)) {
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

void QQmlMetaType::startRecordingTypeRegFailures(QStringList *failures)
{
    QQmlMetaTypeDataPtr data;
    data->startRecordingTypeRegFailures(failures);
}

void QQmlMetaType::stopRecordingTypeRegFailures()
{
    QQmlMetaTypeDataPtr data;
    data->stopRecordingTypeRegFailures();
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
