/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQml/qqmlprivate.h>
#include "qqmlmetatype_p.h"

#include <private/qqmlproxymetaobject_p.h>
#include <private/qqmlcustomparser_p.h>
#include <private/qqmlguard_p.h>
#include <private/qhashedstring_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qbitarray.h>
#include <QtCore/qreadwritelock.h>
#include <QtCore/private/qmetaobject_p.h>

#include <qmetatype.h>
#include <qobjectdefs.h>
#include <qbytearray.h>
#include <qreadwritelock.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qvector.h>

#include <ctype.h>

QT_BEGIN_NAMESPACE

struct QQmlMetaTypeData
{
    QQmlMetaTypeData();
    ~QQmlMetaTypeData();
    QList<QQmlType *> types;
    typedef QHash<int, QQmlType *> Ids;
    Ids idToType;
    typedef QHash<QHashedStringRef,QQmlType *> Names;
    Names nameToType;
    typedef QHash<const QMetaObject *, QQmlType *> MetaObjects;
    MetaObjects metaObjectToType;
    typedef QHash<int, QQmlMetaType::StringConverter> StringConverters;
    StringConverters stringConverters;

    struct VersionedUri {
        VersionedUri()
        : majorVersion(0) {}
        VersionedUri(const QHashedString &uri, int majorVersion)
        : uri(uri), majorVersion(majorVersion) {}
        bool operator==(const VersionedUri &other) const {
            return other.majorVersion == majorVersion && other.uri == uri;
        }
        QHashedString uri;
        int majorVersion;
    };
    typedef QHash<VersionedUri, QQmlTypeModule *> TypeModules;
    TypeModules uriToModule;

    struct ModuleApiList {
        ModuleApiList() : sorted(true) {}
        QList<QQmlMetaType::ModuleApi> moduleApis;
        bool sorted;
    };
    typedef QStringHash<ModuleApiList> ModuleApis;
    ModuleApis moduleApis;
    int moduleApiCount;

    QBitArray objects;
    QBitArray interfaces;
    QBitArray lists;

    QList<QQmlPrivate::AutoParentFunction> parentFunctions;

    QSet<QString> protectedNamespaces;

    QString typeRegistrationNamespace;
    QStringList typeRegistrationFailures;
};

class QQmlTypeModulePrivate
{
public:
    QQmlTypeModulePrivate() 
    : minMinorVersion(INT_MAX), maxMinorVersion(0) {}

    QQmlMetaTypeData::VersionedUri uri;

    int minMinorVersion;
    int maxMinorVersion;

    void add(QQmlType *);

    QStringHash<QList<QQmlType *> > typeHash;
    QList<QQmlType *> types;
};

Q_GLOBAL_STATIC(QQmlMetaTypeData, metaTypeData)
Q_GLOBAL_STATIC_WITH_ARGS(QReadWriteLock, metaTypeDataLock, (QReadWriteLock::Recursive))

static uint qHash(const QQmlMetaTypeData::VersionedUri &v)
{
    return v.uri.hash() ^ qHash(v.majorVersion);
}

QQmlMetaTypeData::QQmlMetaTypeData()
: moduleApiCount(0)
{
}

QQmlMetaTypeData::~QQmlMetaTypeData()
{
    for (int i = 0; i < types.count(); ++i)
        delete types.at(i);
}

class QQmlTypePrivate
{
public:
    QQmlTypePrivate();

    void init() const;
    void initEnums() const;
    void insertEnums(const QMetaObject *metaObject) const;

    bool m_isInterface : 1;
    const char *m_iid;
    QHashedString m_module;
    QString m_name;
    QString m_elementName;
    int m_version_maj;
    int m_version_min;
    int m_typeId; int m_listId; 
    int m_revision;
    mutable bool m_containsRevisionedAttributes;
    mutable QQmlType *m_superType;

    int m_allocationSize;
    void (*m_newFunc)(void *);
    QString m_noCreationReason;

    const QMetaObject *m_baseMetaObject;
    QQmlAttachedPropertiesFunc m_attachedPropertiesFunc;
    const QMetaObject *m_attachedPropertiesType;
    int m_attachedPropertiesId;
    int m_parserStatusCast;
    int m_propertyValueSourceCast;
    int m_propertyValueInterceptorCast;
    QObject *(*m_extFunc)(QObject *);
    const QMetaObject *m_extMetaObject;
    int m_index;
    QQmlCustomParser *m_customParser;
    mutable volatile bool m_isSetup:1;
    mutable volatile bool m_isEnumSetup:1;
    mutable bool m_haveSuperType:1;
    mutable QList<QQmlProxyMetaObject::ProxyData> m_metaObjects;
    mutable QStringHash<int> m_enums;

    static QHash<const QMetaObject *, int> m_attachedPropertyIds;
};

// Avoid multiple fromUtf8(), copies and hashing of the module name.
// This is only called when metaTypeDataLock is locked.
static QHashedString moduleFromUtf8(const char *module)
{
    if (!module)
        return QHashedString();

    static const char *lastModule = 0;
    static QHashedString lastModuleStr;

    // Separate plugins may have different strings at the same address
    QHashedCStringRef currentModule(module, ::strlen(module));
    if ((lastModule != module) || (lastModuleStr.hash() != currentModule.hash())) {
        lastModuleStr = QString::fromUtf8(module);
        lastModuleStr.hash();
        lastModule = module;
    }

    return lastModuleStr;
}

QHash<const QMetaObject *, int> QQmlTypePrivate::m_attachedPropertyIds;

QQmlTypePrivate::QQmlTypePrivate()
: m_isInterface(false), m_iid(0), m_typeId(0), m_listId(0), m_revision(0), m_containsRevisionedAttributes(false),
  m_superType(0), m_allocationSize(0), m_newFunc(0), m_baseMetaObject(0), m_attachedPropertiesFunc(0), 
  m_attachedPropertiesType(0), m_parserStatusCast(-1), m_propertyValueSourceCast(-1), 
  m_propertyValueInterceptorCast(-1), m_extFunc(0), m_extMetaObject(0), m_index(-1), m_customParser(0), 
  m_isSetup(false), m_isEnumSetup(false), m_haveSuperType(false)
{
}


QQmlType::QQmlType(int index, const QQmlPrivate::RegisterInterface &interface)
: d(new QQmlTypePrivate)
{
    d->m_isInterface = true;
    d->m_iid = interface.iid;
    d->m_typeId = interface.typeId;
    d->m_listId = interface.listId;
    d->m_newFunc = 0;
    d->m_index = index;
    d->m_isSetup = true;
    d->m_version_maj = 0;
    d->m_version_min = 0;
}

QQmlType::QQmlType(int index, const QQmlPrivate::RegisterType &type)
: d(new QQmlTypePrivate)
{
    d->m_module = moduleFromUtf8(type.uri);
    d->m_elementName = QString::fromUtf8(type.elementName);

    d->m_version_maj = type.versionMajor;
    d->m_version_min = type.versionMinor;
    if (type.version >= 1) // revisions added in version 1
        d->m_revision = type.revision;
    d->m_typeId = type.typeId;
    d->m_listId = type.listId;
    d->m_allocationSize = type.objectSize;
    d->m_newFunc = type.create;
    d->m_noCreationReason = type.noCreationReason;
    d->m_baseMetaObject = type.metaObject;
    d->m_attachedPropertiesFunc = type.attachedPropertiesFunction;
    d->m_attachedPropertiesType = type.attachedPropertiesMetaObject;
    if (d->m_attachedPropertiesType) {
        QHash<const QMetaObject *, int>::Iterator iter = d->m_attachedPropertyIds.find(d->m_baseMetaObject);
        if (iter == d->m_attachedPropertyIds.end())
            iter = d->m_attachedPropertyIds.insert(d->m_baseMetaObject, index);
        d->m_attachedPropertiesId = *iter;
    } else {
        d->m_attachedPropertiesId = -1;
    }
    d->m_parserStatusCast = type.parserStatusCast;
    d->m_propertyValueSourceCast = type.valueSourceCast;
    d->m_propertyValueInterceptorCast = type.valueInterceptorCast;
    d->m_extFunc = type.extensionObjectCreate;
    d->m_index = index;
    d->m_customParser = type.customParser;

    if (type.extensionMetaObject)
        d->m_extMetaObject = type.extensionMetaObject;
}

QQmlType::~QQmlType()
{
    delete d->m_customParser;
    delete d;
}

const QHashedString &QQmlType::module() const
{
    return d->m_module;
}

int QQmlType::majorVersion() const
{
    return d->m_version_maj;
}

int QQmlType::minorVersion() const
{
    return d->m_version_min;
}

bool QQmlType::availableInVersion(int vmajor, int vminor) const
{
    Q_ASSERT(vmajor >= 0 && vminor >= 0);
    return vmajor == d->m_version_maj && vminor >= d->m_version_min;
}

bool QQmlType::availableInVersion(const QHashedStringRef &module, int vmajor, int vminor) const
{
    Q_ASSERT(vmajor >= 0 && vminor >= 0);
    return module == d->m_module && vmajor == d->m_version_maj && vminor >= d->m_version_min;
}

// returns the nearest _registered_ super class
QQmlType *QQmlType::superType() const
{
    if (!d->m_haveSuperType) {
        const QMetaObject *mo = d->m_baseMetaObject->superClass();
        while (mo && !d->m_superType) {
            d->m_superType = QQmlMetaType::qmlType(mo, d->m_module, d->m_version_maj, d->m_version_min);
            mo = mo->superClass();
        }
        d->m_haveSuperType = true;
    }

    return d->m_superType;
}

static void clone(QMetaObjectBuilder &builder, const QMetaObject *mo, 
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

static bool isPropertyRevisioned(const QMetaObject *mo, int index)
{
    int i = index;
    i -= mo->propertyOffset();
    if (i < 0 && mo->d.superdata)
        return isPropertyRevisioned(mo->d.superdata, index);

    const QMetaObjectPrivate *mop = reinterpret_cast<const QMetaObjectPrivate*>(mo->d.data);
    if (i >= 0 && i < mop->propertyCount) {
        int handle = mop->propertyData + 3*i;
        int flags = mo->d.data[handle + 2];

        return (flags & Revisioned);
    }

    return false;
}

void QQmlTypePrivate::init() const
{
    if (m_isSetup) return;

    QWriteLocker lock(metaTypeDataLock());
    if (m_isSetup)
        return;

    // Setup extended meta object
    // XXX - very inefficient
    const QMetaObject *mo = m_baseMetaObject;
    if (m_extFunc) {
        QMetaObjectBuilder builder;
        clone(builder, m_extMetaObject, m_extMetaObject, m_extMetaObject);
        builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);
        QMetaObject *mmo = builder.toMetaObject();
        mmo->d.superdata = mo;
        QQmlProxyMetaObject::ProxyData data = { mmo, m_extFunc, 0, 0 };
        m_metaObjects << data;
    }

    mo = mo->d.superdata;
    while(mo) {
        QQmlType *t = metaTypeData()->metaObjectToType.value(mo);
        if (t) {
            if (t->d->m_extFunc) {
                QMetaObjectBuilder builder;
                clone(builder, t->d->m_extMetaObject, t->d->m_baseMetaObject, m_baseMetaObject);
                builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);
                QMetaObject *mmo = builder.toMetaObject();
                mmo->d.superdata = m_baseMetaObject;
                if (!m_metaObjects.isEmpty())
                    m_metaObjects.last().metaObject->d.superdata = mmo;
                QQmlProxyMetaObject::ProxyData data = { mmo, t->d->m_extFunc, 0, 0 };
                m_metaObjects << data;
            }
        }
        mo = mo->d.superdata;
    }

    for (int ii = 0; ii < m_metaObjects.count(); ++ii) {
        m_metaObjects[ii].propertyOffset =
            m_metaObjects.at(ii).metaObject->propertyOffset();
        m_metaObjects[ii].methodOffset =
            m_metaObjects.at(ii).metaObject->methodOffset();
    }
    
    // Check for revisioned details
    {
        const QMetaObject *mo = 0;
        if (m_metaObjects.isEmpty())
            mo = m_baseMetaObject;
        else
            mo = m_metaObjects.first().metaObject;

        for (int ii = 0; !m_containsRevisionedAttributes && ii < mo->propertyCount(); ++ii) {
            if (isPropertyRevisioned(mo, ii))
                m_containsRevisionedAttributes = true;
        }

        for (int ii = 0; !m_containsRevisionedAttributes && ii < mo->methodCount(); ++ii) {
            if (mo->method(ii).revision() != 0)
                m_containsRevisionedAttributes = true;
        }
    }

    m_isSetup = true;
    lock.unlock();
}

void QQmlTypePrivate::initEnums() const
{
    if (m_isEnumSetup) return;

    init();

    QWriteLocker lock(metaTypeDataLock());
    if (m_isEnumSetup) return;

    insertEnums(m_baseMetaObject);

    m_isEnumSetup = true;
}

void QQmlTypePrivate::insertEnums(const QMetaObject *metaObject) const
{
    // Add any enum values defined by 'related' classes
    if (metaObject->d.relatedMetaObjects) {
        const QMetaObject **related = metaObject->d.relatedMetaObjects;
        if (related) {
            while (*related)
                insertEnums(*related++);
        }
    }

    // Add any enum values defined by this class, overwriting any inherited values
    for (int ii = 0; ii < metaObject->enumeratorCount(); ++ii) {
        QMetaEnum e = metaObject->enumerator(ii);
        for (int jj = 0; jj < e.keyCount(); ++jj)
            m_enums.insert(QString::fromUtf8(e.key(jj)), e.value(jj));
    }
}

QByteArray QQmlType::typeName() const
{
    if (d->m_baseMetaObject)
        return d->m_baseMetaObject->className();
    else
        return QByteArray();
}

const QString &QQmlType::elementName() const
{
    return d->m_elementName;
}

const QString &QQmlType::qmlTypeName() const
{
    if (d->m_name.isEmpty()) {
        if (!d->m_module.isEmpty())
            d->m_name = static_cast<QString>(d->m_module) + QLatin1Char('/') + d->m_elementName;
        else
            d->m_name = d->m_elementName;
    }

    return d->m_name;
}

QObject *QQmlType::create() const
{
    d->init();

    QObject *rv = (QObject *)operator new(d->m_allocationSize);
    d->m_newFunc(rv);

    if (rv && !d->m_metaObjects.isEmpty())
        (void)new QQmlProxyMetaObject(rv, &d->m_metaObjects);

    return rv;
}

void QQmlType::create(QObject **out, void **memory, size_t additionalMemory) const
{
    d->init();

    QObject *rv = (QObject *)operator new(d->m_allocationSize + additionalMemory);
    d->m_newFunc(rv);

    if (rv && !d->m_metaObjects.isEmpty())
        (void)new QQmlProxyMetaObject(rv, &d->m_metaObjects);

    *out = rv;
    *memory = ((char *)rv) + d->m_allocationSize;
}

QQmlCustomParser *QQmlType::customParser() const
{
    return d->m_customParser;
}

QQmlType::CreateFunc QQmlType::createFunction() const
{
    return d->m_newFunc;
}

QString QQmlType::noCreationReason() const
{
    return d->m_noCreationReason;
}

int QQmlType::createSize() const
{
    return d->m_allocationSize;
}

bool QQmlType::isCreatable() const
{
    return d->m_newFunc != 0;
}

bool QQmlType::isExtendedType() const
{
    d->init();

    return !d->m_metaObjects.isEmpty();
}

bool QQmlType::isInterface() const
{
    return d->m_isInterface;
}

int QQmlType::typeId() const
{
    return d->m_typeId;
}

int QQmlType::qListTypeId() const
{
    return d->m_listId;
}

const QMetaObject *QQmlType::metaObject() const
{
    d->init();

    if (d->m_metaObjects.isEmpty())
        return d->m_baseMetaObject;
    else
        return d->m_metaObjects.first().metaObject;

}

const QMetaObject *QQmlType::baseMetaObject() const
{
    return d->m_baseMetaObject;
}

bool QQmlType::containsRevisionedAttributes() const
{
    d->init();

    return d->m_containsRevisionedAttributes;
}

int QQmlType::metaObjectRevision() const
{
    return d->m_revision;
}

QQmlAttachedPropertiesFunc QQmlType::attachedPropertiesFunction() const
{
    return d->m_attachedPropertiesFunc;
}

const QMetaObject *QQmlType::attachedPropertiesType() const
{
    return d->m_attachedPropertiesType;
}

/*
This is the id passed to qmlAttachedPropertiesById().  This is different from the index
for the case that a single class is registered under two or more names (eg. Item in 
Qt 4.7 and QtQuick 1.0).
*/
int QQmlType::attachedPropertiesId() const
{
    return d->m_attachedPropertiesId;
}

int QQmlType::parserStatusCast() const
{
    return d->m_parserStatusCast;
}

int QQmlType::propertyValueSourceCast() const
{
    return d->m_propertyValueSourceCast;
}

int QQmlType::propertyValueInterceptorCast() const
{
    return d->m_propertyValueInterceptorCast;
}

const char *QQmlType::interfaceIId() const
{
    return d->m_iid;
}

int QQmlType::index() const
{
    return d->m_index;
}

int QQmlType::enumValue(const QHashedStringRef &name, bool *ok) const
{
    Q_ASSERT(ok);
    *ok = true;

    d->initEnums();

    int *rv = d->m_enums.value(name);
    if (rv)
        return *rv;

    *ok = false;
    return -1;
}

int QQmlType::enumValue(const QHashedCStringRef &name, bool *ok) const
{
    Q_ASSERT(ok);
    *ok = true;

    d->initEnums();

    int *rv = d->m_enums.value(name);
    if (rv)
        return *rv;

    *ok = false;
    return -1;
}

int QQmlType::enumValue(const QHashedV8String &name, bool *ok) const
{
    Q_ASSERT(ok);
    *ok = true;

    d->initEnums();

    int *rv = d->m_enums.value(name);
    if (rv)
        return *rv;

    *ok = false;
    return -1;
}

QQmlTypeModule::QQmlTypeModule()
: d(new QQmlTypeModulePrivate)
{
}

QQmlTypeModule::~QQmlTypeModule()
{
    delete d; d = 0;
}

QString QQmlTypeModule::module() const
{
    return d->uri.uri;
}

int QQmlTypeModule::majorVersion() const
{
    return d->uri.majorVersion;
}

int QQmlTypeModule::minimumMinorVersion() const
{
    return d->minMinorVersion;
}

int QQmlTypeModule::maximumMinorVersion() const
{
    return d->maxMinorVersion;
}

void QQmlTypeModulePrivate::add(QQmlType *type)
{
    minMinorVersion = qMin(minMinorVersion, type->minorVersion());
    maxMinorVersion = qMax(maxMinorVersion, type->minorVersion());

    QList<QQmlType *> &list = typeHash[type->elementName()];
    for (int ii = 0; ii < list.count(); ++ii) {
        if (list.at(ii)->minorVersion() < type->minorVersion()) {
            list.insert(ii, type);
            return;
        }
    }
    list.append(type);
}

QQmlType *QQmlTypeModule::type(const QHashedStringRef &name, int minor)
{
    QReadLocker lock(metaTypeDataLock());

    QList<QQmlType *> *types = d->typeHash.value(name);
    if (!types) return 0;

    for (int ii = 0; ii < types->count(); ++ii)
        if (types->at(ii)->minorVersion() <= minor)
            return types->at(ii);

    return 0;
}

QQmlType *QQmlTypeModule::type(const QHashedV8String &name, int minor)
{
    QReadLocker lock(metaTypeDataLock());

    QList<QQmlType *> *types = d->typeHash.value(name);
    if (!types) return 0;

    for (int ii = 0; ii < types->count(); ++ii)
        if (types->at(ii)->minorVersion() <= minor)
            return types->at(ii);

    return 0;
}


QQmlTypeModuleVersion::QQmlTypeModuleVersion()
: m_module(0), m_minor(0)
{
}

QQmlTypeModuleVersion::QQmlTypeModuleVersion(QQmlTypeModule *module, int minor)
: m_module(module), m_minor(minor)
{
    Q_ASSERT(m_module);
    Q_ASSERT(m_minor >= 0);
}

QQmlTypeModuleVersion::QQmlTypeModuleVersion(const QQmlTypeModuleVersion &o)
: m_module(o.m_module), m_minor(o.m_minor)
{
}

QQmlTypeModuleVersion &QQmlTypeModuleVersion::operator=(const QQmlTypeModuleVersion &o)
{
    m_module = o.m_module;
    m_minor = o.m_minor;
    return *this;
}

QQmlTypeModule *QQmlTypeModuleVersion::module() const
{
    return m_module;
}

int QQmlTypeModuleVersion::minorVersion() const
{
    return m_minor;
}

QQmlType *QQmlTypeModuleVersion::type(const QHashedStringRef &name) const
{
    if (m_module) return m_module->type(name, m_minor);
    else return 0;
}

QQmlType *QQmlTypeModuleVersion::type(const QHashedV8String &name) const
{
    if (m_module) return m_module->type(name, m_minor);
    else return 0;
}


int registerAutoParentFunction(QQmlPrivate::RegisterAutoParent &autoparent)
{
    QWriteLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();

    data->parentFunctions.append(autoparent.function);

    return data->parentFunctions.count() - 1;
}

int registerInterface(const QQmlPrivate::RegisterInterface &interface)
{
    if (interface.version > 0) 
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QWriteLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();

    int index = data->types.count();

    QQmlType *type = new QQmlType(index, interface);

    data->types.append(type);
    data->idToType.insert(type->typeId(), type);
    data->idToType.insert(type->qListTypeId(), type);
    // XXX No insertMulti, so no multi-version interfaces?
    if (!type->elementName().isEmpty())
        data->nameToType.insert(type->elementName(), type);

    if (data->interfaces.size() <= interface.typeId)
        data->interfaces.resize(interface.typeId + 16);
    if (data->lists.size() <= interface.listId)
        data->lists.resize(interface.listId + 16);
    data->interfaces.setBit(interface.typeId, true);
    data->lists.setBit(interface.listId, true);

    return index;
}

int registerType(const QQmlPrivate::RegisterType &type)
{
    if (type.elementName) {
        for (int ii = 0; type.elementName[ii]; ++ii) {
            if (!isalnum(type.elementName[ii])) {
                qWarning("qmlRegisterType(): Invalid QML element name \"%s\"", type.elementName);
                return -1;
            }
        }
    }

    QWriteLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();

    if (type.uri && type.elementName) {
        QString nameSpace = moduleFromUtf8(type.uri);

        if (!data->typeRegistrationNamespace.isEmpty()) {
            // We can only install types into the registered namespace
            if (nameSpace != data->typeRegistrationNamespace) {
                QString failure(QCoreApplication::translate("qmlRegisterType",
                                                            "Cannot install element '%1' into unregistered namespace '%2'"));
                data->typeRegistrationFailures.append(failure.arg(QString::fromUtf8(type.elementName)).arg(nameSpace));
                return -1;
            }
        } else if (data->typeRegistrationNamespace != nameSpace) {
            // Is the target namespace protected against further registrations?
            if (data->protectedNamespaces.contains(nameSpace)) {
                QString failure(QCoreApplication::translate("qmlRegisterType",
                                                            "Cannot install element '%1' into protected namespace '%2'"));
                data->typeRegistrationFailures.append(failure.arg(QString::fromUtf8(type.elementName)).arg(nameSpace));
                return -1;
            }
        }
    }

    int index = data->types.count();

    QQmlType *dtype = new QQmlType(index, type);

    data->types.append(dtype);
    data->idToType.insert(dtype->typeId(), dtype);
    if (dtype->qListTypeId()) data->idToType.insert(dtype->qListTypeId(), dtype);

    if (!dtype->elementName().isEmpty())
        data->nameToType.insertMulti(dtype->elementName(), dtype);

    data->metaObjectToType.insertMulti(dtype->baseMetaObject(), dtype);

    if (data->objects.size() <= type.typeId)
        data->objects.resize(type.typeId + 16);
    if (data->lists.size() <= type.listId)
        data->lists.resize(type.listId + 16);
    data->objects.setBit(type.typeId, true);
    if (type.listId) data->lists.setBit(type.listId, true);

    if (!dtype->module().isEmpty()) {
        const QHashedString &mod = dtype->module();

        QQmlMetaTypeData::VersionedUri versionedUri(mod, type.versionMajor);
        QQmlTypeModule *module = data->uriToModule.value(versionedUri);
        if (!module) {
            module = new QQmlTypeModule;
            module->d->uri = versionedUri;
            data->uriToModule.insert(versionedUri, module);
        }
        module->d->add(dtype);
    }

    return index;
}

int registerModuleApi(const QQmlPrivate::RegisterModuleApi &api)
{
    QWriteLocker lock(metaTypeDataLock());

    QQmlMetaTypeData *data = metaTypeData();
    QString uri = QString::fromUtf8(api.uri);
    QQmlMetaType::ModuleApi import;
    import.major = api.versionMajor;
    import.minor = api.versionMinor;
    import.script = api.scriptApi;
    import.qobject = api.qobjectApi;
    import.instanceMetaObject = (api.qobjectApi && api.version >= 1) ? api.instanceMetaObject : 0; // BC with version 0.

    int index = data->moduleApiCount++;

    QQmlMetaTypeData::ModuleApiList *apiList = data->moduleApis.value(uri);
    if (!apiList) {
        QQmlMetaTypeData::ModuleApiList apis;
        apis.moduleApis << import;
        data->moduleApis.insert(uri, apis);
    } else {
        apiList->moduleApis << import;
        apiList->sorted = false;
    }

    return index;
}


/*
This method is "over generalized" to allow us to (potentially) register more types of things in
the future without adding exported symbols.
*/
int QQmlPrivate::qmlregister(RegistrationType type, void *data)
{
    if (type == TypeRegistration) {
        return registerType(*reinterpret_cast<RegisterType *>(data));
    } else if (type == InterfaceRegistration) {
        return registerInterface(*reinterpret_cast<RegisterInterface *>(data));
    } else if (type == AutoParentRegistration) {
        return registerAutoParentFunction(*reinterpret_cast<RegisterAutoParent *>(data));
    } else if (type == ModuleApiRegistration) {
        return registerModuleApi(*reinterpret_cast<RegisterModuleApi *>(data));
    }
    return -1;
}

bool QQmlMetaType::namespaceContainsRegistrations(const QString &uri)
{
    QQmlMetaTypeData *data = metaTypeData();

    // Has any type previously been installed to this namespace?
    QHashedString nameSpace(uri);
    foreach (const QQmlType *type, data->types)
        if (type->module() == nameSpace)
            return true;

    return false;
}

void QQmlMetaType::protectNamespace(const QString &uri)
{
    QQmlMetaTypeData *data = metaTypeData();

    data->protectedNamespaces.insert(uri);
}

void QQmlMetaType::setTypeRegistrationNamespace(const QString &uri)
{
    QQmlMetaTypeData *data = metaTypeData();

    data->typeRegistrationNamespace = uri;
    data->typeRegistrationFailures.clear();
}

QStringList QQmlMetaType::typeRegistrationFailures()
{
    QQmlMetaTypeData *data = metaTypeData();

    return data->typeRegistrationFailures;
}

QReadWriteLock *QQmlMetaType::typeRegistrationLock()
{
    return metaTypeDataLock();
}

/*
    Returns true if a module \a uri of any version is installed.
*/
bool QQmlMetaType::isAnyModule(const QString &uri)
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();

    // first, check Types
    for (QQmlMetaTypeData::TypeModules::ConstIterator iter = data->uriToModule.begin();
         iter != data->uriToModule.end(); ++iter) {
        if ((*iter)->module() == uri)
            return true;
    }

    // then, check ModuleApis
    QQmlMetaTypeData::ModuleApiList *apiList = data->moduleApis.value(uri);
    if (apiList)
        return true;

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
    QReadLocker lock(metaTypeDataLock());

    QQmlMetaTypeData *data = metaTypeData();

    // first, check Types
    QQmlTypeModule *tm = 
        data->uriToModule.value(QQmlMetaTypeData::VersionedUri(module, versionMajor));
    if (tm && tm->minimumMinorVersion() <= versionMinor && tm->maximumMinorVersion() >= versionMinor)
        return true;

    // then, check ModuleApis
    QQmlMetaTypeData::ModuleApiList *apiList = data->moduleApis.value(module);
    if (apiList) {
        foreach (const QQmlMetaType::ModuleApi &mApi, apiList->moduleApis) {
            if (mApi.major == versionMajor && mApi.minor == versionMinor) // XXX is this correct?
                return true;
        }
    }

    return false;
}

QQmlTypeModule *QQmlMetaType::typeModule(const QString &uri, int majorVersion)
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();
    return data->uriToModule.value(QQmlMetaTypeData::VersionedUri(uri, majorVersion));
}

QList<QQmlPrivate::AutoParentFunction> QQmlMetaType::parentFunctions()
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();
    return data->parentFunctions;
}

static bool operator<(const QQmlMetaType::ModuleApi &lhs, const QQmlMetaType::ModuleApi &rhs)
{
    return lhs.major < rhs.major || (lhs.major == rhs.major && lhs.minor < rhs.minor);
}

QQmlMetaType::ModuleApi
QQmlMetaType::moduleApi(const QString &uri, int versionMajor, int versionMinor)
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();

    QQmlMetaTypeData::ModuleApiList *apiList = data->moduleApis.value(uri);
    if (!apiList)
        return ModuleApi();

    if (apiList->sorted == false) {
        qSort(apiList->moduleApis.begin(), apiList->moduleApis.end());
        apiList->sorted = true;
    }

    for (int ii = apiList->moduleApis.count() - 1; ii >= 0; --ii) {
        const ModuleApi &import = apiList->moduleApis.at(ii);
        if (import.major == versionMajor && import.minor <= versionMinor)
            return import;
    }

    return ModuleApi();
}

QHash<QString, QList<QQmlMetaType::ModuleApi> > QQmlMetaType::moduleApis()
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();

    QHash<QString, QList<ModuleApi> > moduleApis;
    QStringHash<QQmlMetaTypeData::ModuleApiList>::ConstIterator it = data->moduleApis.begin();
    for (; it != data->moduleApis.end(); ++it)
        moduleApis[it.key()] = it.value().moduleApis;

    return moduleApis;
}

QObject *QQmlMetaType::toQObject(const QVariant &v, bool *ok)
{
    if (!isQObject(v.userType())) {
        if (ok) *ok = false;
        return 0;
    }

    if (ok) *ok = true;

    return *(QObject **)v.constData();
}

bool QQmlMetaType::isQObject(int userType)
{
    if (userType == QMetaType::QObjectStar)
        return true;

    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();
    return userType >= 0 && userType < data->objects.size() && data->objects.testBit(userType);
}

/*
    Returns the item type for a list of type \a id.
 */
int QQmlMetaType::listType(int id)
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();
    QQmlType *type = data->idToType.value(id);
    if (type && type->qListTypeId() == id)
        return type->typeId();
    else
        return 0;
}

int QQmlMetaType::attachedPropertiesFuncId(const QMetaObject *mo)
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();

    QQmlType *type = data->metaObjectToType.value(mo);
    if (type && type->attachedPropertiesFunction())
        return type->attachedPropertiesId();
    else
        return -1;
}

QQmlAttachedPropertiesFunc QQmlMetaType::attachedPropertiesFuncById(int id)
{
    if (id < 0)
        return 0;
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();
    return data->types.at(id)->attachedPropertiesFunction();
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

    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();
    if (userType < data->objects.size() && data->objects.testBit(userType))
        return Object;
    else if (userType < data->lists.size() && data->lists.testBit(userType))
        return List;
    else
        return Unknown;
}

bool QQmlMetaType::isInterface(int userType)
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();
    return userType >= 0 && userType < data->interfaces.size() && data->interfaces.testBit(userType);
}

const char *QQmlMetaType::interfaceIId(int userType)
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();
    QQmlType *type = data->idToType.value(userType);
    lock.unlock();
    if (type && type->isInterface() && type->typeId() == userType)
        return type->interfaceIId();
    else
        return 0;
}

bool QQmlMetaType::isList(int userType)
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();
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
    QWriteLocker lock(metaTypeDataLock());

    QQmlMetaTypeData *data = metaTypeData();
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
    QReadLocker lock(metaTypeDataLock());

    QQmlMetaTypeData *data = metaTypeData();
    return data->stringConverters.value(type);
}

/*!
    Returns the type (if any) of URI-qualified named \a qualifiedName and version specified
    by \a version_major and \a version_minor.
*/
QQmlType *QQmlMetaType::qmlType(const QString &qualifiedName, int version_major, int version_minor)
{
    int slash = qualifiedName.indexOf(QLatin1Char('/'));
    if (slash <= 0)
        return 0;

    QHashedStringRef module(qualifiedName.constData(), slash);
    QHashedStringRef name(qualifiedName.constData() + slash + 1, qualifiedName.length() - slash - 1);

    return qmlType(name, module, version_major, version_minor);
}

/*!
    Returns the type (if any) of \a name in \a module and version specified
    by \a version_major and \a version_minor.
*/
QQmlType *QQmlMetaType::qmlType(const QHashedStringRef &name, const QHashedStringRef &module, int version_major, int version_minor)
{
    Q_ASSERT(version_major >= 0 && version_minor >= 0);
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();

    QQmlMetaTypeData::Names::ConstIterator it = data->nameToType.find(name);
    while (it != data->nameToType.end() && it.key() == name) {
        // XXX version_major<0 just a kludge for QQmlPropertyPrivate::initProperty
        if (version_major < 0 || (*it)->availableInVersion(module, version_major,version_minor))
            return (*it);
        ++it;
    }

    return 0;
}

/*!
    Returns the type (if any) that corresponds to the \a metaObject.  Returns null if no
    type is registered.
*/
QQmlType *QQmlMetaType::qmlType(const QMetaObject *metaObject)
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();

    return data->metaObjectToType.value(metaObject);
}

/*!
    Returns the type (if any) that corresponds to the \a metaObject in version specified
    by \a version_major and \a version_minor in module specified by \a uri.  Returns null if no
    type is registered.
*/
QQmlType *QQmlMetaType::qmlType(const QMetaObject *metaObject, const QHashedStringRef &module, int version_major, int version_minor)
{
    Q_ASSERT(version_major >= 0 && version_minor >= 0);
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();

    QQmlMetaTypeData::MetaObjects::const_iterator it = data->metaObjectToType.find(metaObject);
    while (it != data->metaObjectToType.end() && it.key() == metaObject) {
        QQmlType *t = *it;
        if (version_major < 0 || t->availableInVersion(module, version_major,version_minor))
            return t;
        ++it;
    }

    return 0;
}

/*!
    Returns the type (if any) that corresponds to the QVariant::Type \a userType.  
    Returns null if no type is registered.
*/
QQmlType *QQmlMetaType::qmlType(int userType)
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();

    QQmlType *type = data->idToType.value(userType);
    if (type && type->typeId() == userType)
        return type;
    else
        return 0;
}

/*!
    Returns the list of registered QML type names.
*/
QList<QString> QQmlMetaType::qmlTypeNames()
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();

    QList<QString> names;
    QQmlMetaTypeData::Names::ConstIterator it = data->nameToType.begin();
    while (it != data->nameToType.end()) {
        names += (*it)->qmlTypeName();
        ++it;
    }

    return names;
}

/*!
    Returns the list of registered QML types.
*/
QList<QQmlType*> QQmlMetaType::qmlTypes()
{
    QReadLocker lock(metaTypeDataLock());
    QQmlMetaTypeData *data = metaTypeData();

    return data->nameToType.values();
}

int QQmlMetaType::QQuickAnchorLineMetaTypeId()
{
    static int id = 0;
    if (!id) {
        id = QMetaType::type("QQuickAnchorLine");
    }
    return id;
}

QQmlMetaType::CompareFunction QQmlMetaType::anchorLineCompareFunction = 0;

void QQmlMetaType::setQQuickAnchorLineCompareFunction(CompareFunction fun)
{
    anchorLineCompareFunction = fun;
}

bool QQmlMetaType::QQuickAnchorLineCompare(const void *p1, const void *p2)
{
    Q_ASSERT(anchorLineCompareFunction != 0);
    return anchorLineCompareFunction(p1, p2);
}

QT_END_NAMESPACE
