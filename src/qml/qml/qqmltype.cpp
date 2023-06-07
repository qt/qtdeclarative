// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmltype_p_p.h"

#include <QtQml/qjsvalue.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>

#include <private/qqmlcustomparser_p.h>
#include <private/qqmldata_p.h>
#include <private/qqmlmetatypedata_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qqmltypedata_p.h>

QT_BEGIN_NAMESPACE

QQmlTypePrivate::QQmlTypePrivate(QQmlType::RegistrationType type)
    : regType(type), iid(nullptr), revision(QTypeRevision::zero()),
    containsRevisionedAttributes(false), baseMetaObject(nullptr),
    index(-1), isSetup(false), isEnumFromCacheSetup(false), isEnumFromBaseSetup(false),
    haveSuperType(false)
{
    switch (type) {
    case QQmlType::CppType:
        extraData.cd = new QQmlCppTypeData;
        extraData.cd->allocationSize = 0;
        extraData.cd->newFunc = nullptr;
        extraData.cd->createValueTypeFunc = nullptr;
        extraData.cd->parserStatusCast = -1;
        extraData.cd->extFunc = nullptr;
        extraData.cd->extMetaObject = nullptr;
        extraData.cd->customParser = nullptr;
        extraData.cd->attachedPropertiesFunc = nullptr;
        extraData.cd->attachedPropertiesType = nullptr;
        extraData.cd->propertyValueSourceCast = -1;
        extraData.cd->propertyValueInterceptorCast = -1;
        extraData.cd->finalizerCast = -1;
        extraData.cd->registerEnumClassesUnscoped = true;
        extraData.cd->registerEnumsFromRelatedTypes = true;
        break;
    case QQmlType::SingletonType:
    case QQmlType::CompositeSingletonType:
        extraData.sd = new QQmlSingletonTypeData;
        extraData.sd->singletonInstanceInfo = nullptr;
        extraData.sd->extFunc = nullptr;
        extraData.sd->extMetaObject = nullptr;
        break;
    case QQmlType::InterfaceType:
        extraData.cd = nullptr;
        break;
    case QQmlType::CompositeType:
        extraData.fd = new QQmlCompositeTypeData;
        break;
    case QQmlType::InlineComponentType:
        extraData.id = new QQmlInlineTypeData;
        break;
    case QQmlType::SequentialContainerType:
        extraData.ld = new QQmlSequenceTypeData;
        break;
    default: qFatal("QQmlTypePrivate Internal Error.");
    }
}

QQmlTypePrivate::~QQmlTypePrivate()
{
    qDeleteAll(scopedEnums);
    for (const auto &metaObject : metaObjects)
        free(metaObject.metaObject);
    switch (regType) {
    case QQmlType::CppType:
        delete extraData.cd->customParser;
        delete extraData.cd;
        break;
    case QQmlType::SingletonType:
    case QQmlType::CompositeSingletonType:
        delete extraData.sd->singletonInstanceInfo;
        delete extraData.sd;
        break;
    case QQmlType::CompositeType:
        delete extraData.fd;
        break;
    case QQmlType::InlineComponentType:
        delete  extraData.id;
        break;
    case QQmlType::SequentialContainerType:
        delete extraData.ld;
        break;
    default: //Also InterfaceType, because it has no extra data
        break;
    }
}

QQmlType::QQmlType() = default;
QQmlType::QQmlType(const QQmlType &) = default;
QQmlType::QQmlType(QQmlType &&) = default;
QQmlType &QQmlType::operator =(const QQmlType &other) = default;
QQmlType &QQmlType::operator =(QQmlType &&other) = default;
QQmlType::QQmlType(const QQmlTypePrivate *priv) : d(priv) {}
QQmlType::~QQmlType() = default;

QHashedString QQmlType::module() const
{
    if (!d)
        return QHashedString();
    return d->module;
}

QTypeRevision QQmlType::version() const
{
    if (!d)
        return QTypeRevision();
    return d->version;
}

bool QQmlType::availableInVersion(QTypeRevision version) const
{
    if (!d)
        return false;

    if (!version.hasMajorVersion())
        return true;

    if (version.majorVersion() != d->version.majorVersion())
        return false;

    return !version.hasMinorVersion() || version.minorVersion() >= d->version.minorVersion();
}

bool QQmlType::availableInVersion(const QHashedStringRef &module, QTypeRevision version) const
{
    if (!d || module != d->module)
        return false;

    return availableInVersion(version);
}

QQmlType QQmlTypePrivate::resolveCompositeBaseType(QQmlEnginePrivate *engine) const
{
    Q_ASSERT(isComposite());
    if (!engine)
        return QQmlType();
    QQmlRefPointer<QQmlTypeData> td(engine->typeLoader.getType(sourceUrl()));
    if (td.isNull() || !td->isComplete())
        return QQmlType();
    QV4::ExecutableCompilationUnit *compilationUnit = td->compilationUnit();
    const QMetaObject *mo = compilationUnit->rootPropertyCache()->firstCppMetaObject();
    return QQmlMetaType::qmlType(mo);
}

QQmlPropertyCache::ConstPtr QQmlTypePrivate::compositePropertyCache(
        QQmlEnginePrivate *engine) const
{
    // similar logic to resolveCompositeBaseType
    Q_ASSERT(isComposite());
    if (!engine)
        return nullptr;
    QQmlRefPointer<QQmlTypeData> td(engine->typeLoader.getType(sourceUrl()));
    if (td.isNull() || !td->isComplete())
        return nullptr;
    QV4::ExecutableCompilationUnit *compilationUnit = td->compilationUnit();
    return compilationUnit->rootPropertyCache();
}

static bool isPropertyRevisioned(const QMetaObject *mo, int index)
{
    return mo->property(index).revision();
}

void QQmlTypePrivate::init() const
{
    if (isSetup.loadAcquire())
        return;

    QMutexLocker lock(QQmlMetaType::typeRegistrationLock());
    if (isSetup.loadAcquire())
        return;

    const QMetaObject *mo = baseMetaObject;
    if (!mo) {
        // version 0 singleton type without metaobject information
        return;
    }

    auto setupExtendedMetaObject = [&](const QMetaObject *extMetaObject,
                                       QObject *(*extFunc)(QObject *)) {
        if (!extMetaObject)
            return;

        // XXX - very inefficient
        QMetaObjectBuilder builder;
        QQmlMetaType::clone(builder, extMetaObject, extMetaObject, extMetaObject,
                            extFunc ? QQmlMetaType::CloneAll : QQmlMetaType::CloneEnumsOnly);
        QMetaObject *mmo = builder.toMetaObject();
        mmo->d.superdata = mo;
        QQmlProxyMetaObject::ProxyData data = { mmo, extFunc, 0, 0 };
        metaObjects << data;
        QQmlMetaType::registerMetaObjectForType(mmo, const_cast<QQmlTypePrivate *>(this));
    };

    if (regType == QQmlType::SingletonType)
        setupExtendedMetaObject(extraData.sd->extMetaObject, extraData.sd->extFunc);
    else if (regType == QQmlType::CppType)
        setupExtendedMetaObject(extraData.cd->extMetaObject, extraData.cd->extFunc);

    metaObjects.append(QQmlMetaType::proxyData(
            mo, baseMetaObject, metaObjects.isEmpty() ? nullptr
                                                      : metaObjects.constLast().metaObject));

    for (int ii = 0; ii < metaObjects.size(); ++ii) {
        metaObjects[ii].propertyOffset =
                metaObjects.at(ii).metaObject->propertyOffset();
        metaObjects[ii].methodOffset =
                metaObjects.at(ii).metaObject->methodOffset();
    }

    // Check for revisioned details
    {
        const QMetaObject *mo = nullptr;
        if (metaObjects.isEmpty())
            mo = baseMetaObject;
        else
            mo = metaObjects.constFirst().metaObject;

        for (int ii = 0; !containsRevisionedAttributes && ii < mo->propertyCount(); ++ii) {
            if (isPropertyRevisioned(mo, ii))
                containsRevisionedAttributes = true;
        }

        for (int ii = 0; !containsRevisionedAttributes && ii < mo->methodCount(); ++ii) {
            if (mo->method(ii).revision() != 0)
                containsRevisionedAttributes = true;
        }
    }

    isSetup.storeRelease(true);
    lock.unlock();
}

void QQmlTypePrivate::initEnums(QQmlEnginePrivate *engine) const
{
    QQmlPropertyCache::ConstPtr cache = (!isEnumFromCacheSetup.loadAcquire() && isComposite())
            ? compositePropertyCache(engine)
            : QQmlPropertyCache::ConstPtr();

    // beware: It could be a singleton type without metaobject
    const QMetaObject *metaObject = !isEnumFromBaseSetup.loadAcquire()
            ? baseMetaObject
            : nullptr;

    if (!cache && !metaObject)
        return;

    init(); // init() can add to the metaObjects list. Therefore, check metaObjects only below

    QMutexLocker lock(QQmlMetaType::typeRegistrationLock());

    if (cache) {
        insertEnumsFromPropertyCache(cache);
        isEnumFromCacheSetup.storeRelease(true);
    }

    if (metaObject) {
        insertEnums(metaObjects.isEmpty() ? baseMetaObject : metaObjects.constFirst().metaObject);
        isEnumFromBaseSetup.storeRelease(true);
    }
}

void QQmlTypePrivate::insertEnums(const QMetaObject *metaObject) const
{
    // Add any enum values defined by 'related' classes
    if (regType != QQmlType::CppType || extraData.cd->registerEnumsFromRelatedTypes) {
        if (const auto *related = metaObject->d.relatedMetaObjects) {
            while (const QMetaObject *relatedMetaObject = *related) {
                insertEnums(relatedMetaObject);
                ++related;
            }
        }
    }

    QSet<QString> localEnums;
    const QMetaObject *localMetaObject = nullptr;

    // Add any enum values defined by this class, overwriting any inherited values
    for (int ii = 0; ii < metaObject->enumeratorCount(); ++ii) {
        QMetaEnum e = metaObject->enumerator(ii);
        const bool isScoped = e.isScoped();
        QStringHash<int> *scoped = isScoped ? new QStringHash<int>() : nullptr;

    // We allow enums in sub-classes to overwrite enums from base-classes, such as
    // ListView.Center (from enum PositionMode) overwriting Item.Center (from enum TransformOrigin).
    // This is acceptable because the _use_ of the enum from the QML side requires qualification
    // anyway, i.e. ListView.Center vs. Item.Center.
        // However if a class defines two enums with the same value, then that must produce a warning
        // because it represents a valid conflict.
        if (e.enclosingMetaObject() != localMetaObject) {
            localEnums.clear();
            localMetaObject = e.enclosingMetaObject();
        }

        for (int jj = 0; jj < e.keyCount(); ++jj) {
            const QString key = QString::fromUtf8(e.key(jj));
            const int value = e.value(jj);
            if (!isScoped || (regType == QQmlType::CppType && extraData.cd->registerEnumClassesUnscoped)) {
                if (localEnums.contains(key)) {
                    auto existingEntry = enums.find(key);
                    if (existingEntry != enums.end() && existingEntry.value() != value) {
                        qWarning("Previously registered enum will be overwritten due to name clash: %s.%s", metaObject->className(), key.toUtf8().constData());
                        createEnumConflictReport(metaObject, key);
                    }
                } else {
                    localEnums.insert(key);
                }
                enums.insert(key, value);
            }
            if (isScoped)
                scoped->insert(key, value);
        }

        if (isScoped) {
            scopedEnums << scoped;
            scopedEnumIndex.insert(QString::fromUtf8(e.name()), scopedEnums.size()-1);
        }
    }
}

void QQmlTypePrivate::createListOfPossibleConflictingItems(const QMetaObject *metaObject, QList<EnumInfo> &enumInfoList, QStringList path) const
{
    path.append(QString::fromUtf8(metaObject->className()));

    if (metaObject->d.relatedMetaObjects) {
        const auto *related = metaObject->d.relatedMetaObjects;
        if (related) {
            while (*related)
                createListOfPossibleConflictingItems(*related++, enumInfoList, path);
        }
    }

    for (int ii = 0; ii < metaObject->enumeratorCount(); ++ii) {
        const auto e = metaObject->enumerator(ii);

        for (int jj = 0; jj < e.keyCount(); ++jj) {
            const QString key = QString::fromUtf8(e.key(jj));

            EnumInfo enumInfo;
            enumInfo.metaObjectName = QString::fromUtf8(metaObject->className());
            enumInfo.enumName = QString::fromUtf8(e.name());
            enumInfo.enumKey = key;
            enumInfo.scoped = e.isScoped();
            enumInfo.path = path;
            enumInfo.metaEnumScope = QString::fromUtf8(e.scope());
            enumInfoList.append(enumInfo);
        }
    }
}

void QQmlTypePrivate::createEnumConflictReport(const QMetaObject *metaObject, const QString &conflictingKey) const
{
    QList<EnumInfo> enumInfoList;

    if (baseMetaObject) // prefer baseMetaObject if available
        metaObject = baseMetaObject;

    if (!metaObject) { // If there is no metaObject at all return early
        qWarning() << "No meta object information available. Skipping conflict analysis.";
        return;
    }

    createListOfPossibleConflictingItems(metaObject, enumInfoList, QStringList());

    qWarning().noquote() << QLatin1String("Possible conflicting items:");
    // find items with conflicting key
    for (const auto &i : std::as_const(enumInfoList)) {
        if (i.enumKey == conflictingKey)
        qWarning().noquote().nospace() << "    " << i.metaObjectName << "." << i.enumName << "." << i.enumKey << " from scope "
                                           << i.metaEnumScope << " injected by " << i.path.join(QLatin1String("->"));
    }
}

void QQmlTypePrivate::insertEnumsFromPropertyCache(
        const QQmlPropertyCache::ConstPtr &cache) const
{
    const QMetaObject *cppMetaObject = cache->firstCppMetaObject();

    for (const QQmlPropertyCache *currentCache = cache.data();
         currentCache && currentCache->metaObject() != cppMetaObject;
         currentCache = currentCache->parent().data()) {

        int count = currentCache->qmlEnumCount();
        for (int ii = 0; ii < count; ++ii) {
            QStringHash<int> *scoped = new QStringHash<int>();
            QQmlEnumData *enumData = currentCache->qmlEnum(ii);

            for (int jj = 0; jj < enumData->values.size(); ++jj) {
                const QQmlEnumValue &value = enumData->values.at(jj);
                enums.insert(value.namedValue, value.value);
                scoped->insert(value.namedValue, value.value);
            }
            scopedEnums << scoped;
            scopedEnumIndex.insert(enumData->name, scopedEnums.size()-1);
        }
    }
    insertEnums(cppMetaObject);
}

void QQmlTypePrivate::setContainingType(QQmlType *containingType)
{
    Q_ASSERT(regType == QQmlType::InlineComponentType);
    extraData.id->containingType = containingType->d.data();
}

void QQmlTypePrivate::setName(const QString &uri, const QString &element)
{
    module = uri;
    elementName = element;
    name = uri.isEmpty() ? element : (uri + QLatin1Char('/') + element);
}

QByteArray QQmlType::typeName() const
{
    if (d) {
        if (d->regType == SingletonType || d->regType == CompositeSingletonType)
            return d->extraData.sd->singletonInstanceInfo->typeName.toUtf8();
        else if (d->baseMetaObject)
            return d->baseMetaObject->className();
    }
    return QByteArray();
}

QString QQmlType::elementName() const
{
    if (!d)
        return QString();
    return d->elementName;
}

QString QQmlType::qmlTypeName() const
{
    if (!d)
        return QString();
    return d->name;
}

/*!
   \internal
   Allocates and initializes an object if the type is creatable.
   Returns a pointer to the object, or nullptr if the type was
   not creatable.
 */
QObject *QQmlType::create() const
{
    void *unused;
    return create(&unused, 0);
}

/*!
   \internal
   \brief Like create without arguments, but allocates some extra space after the object.
   \param memory An out-only argument. *memory will point to the start of the additionally
                 allocated memory.
   \param additionalMemory The amount of extra memory in bytes that shoudld be allocated.

   \note This function is used to allocate the QQmlData next to the object in the
   QQmlObjectCreator.

   \overload
 */
QObject *QQmlType::create(void **memory, size_t additionalMemory) const
{
    if (!d || !isCreatable())
        return nullptr;

    d->init();

    QObject *rv = (QObject *)operator new(d->extraData.cd->allocationSize + additionalMemory);
    d->extraData.cd->newFunc(rv, d->extraData.cd->userdata);

    createProxy(rv);
    *memory = ((char *)rv) + d->extraData.cd->allocationSize;
    return rv;
}

/*!
    \internal
    Like create, but also allocates memory behind the object, constructs a QQmlData there
    and lets the objects declarativeData point to the newly created QQmlData.
 */
QObject *QQmlType::createWithQQmlData() const
{
    void *ddataMemory = nullptr;
    auto instance = create(&ddataMemory, sizeof(QQmlData));
    if (!instance)
        return nullptr;
    QObjectPrivate* p = QObjectPrivate::get(instance);
    Q_ASSERT(!p->isDeletingChildren);
    if (!p->declarativeData)
        p->declarativeData = new (ddataMemory) QQmlData(QQmlData::DoesNotOwnMemory);
    return instance;
}

QQmlType::SingletonInstanceInfo *QQmlType::singletonInstanceInfo() const
{
    if (!d)
        return nullptr;
    if (d->regType != SingletonType && d->regType != CompositeSingletonType)
        return nullptr;
    return d->extraData.sd->singletonInstanceInfo;
}

QQmlCustomParser *QQmlType::customParser() const
{
    if (!d)
        return nullptr;
    if (d->regType != CppType)
        return nullptr;
    return d->extraData.cd->customParser;
}

QQmlType::CreateValueTypeFunc QQmlType::createValueTypeFunction() const
{
    if (!d || d->regType != CppType)
        return nullptr;
    return d->extraData.cd->createValueTypeFunc;
}

bool QQmlType::canConstructValueType() const
{
    if (!d || d->regType != CppType)
        return false;
    return d->extraData.cd->constructValueType;
}

bool QQmlType::canPopulateValueType() const
{
    if (!d || d->regType != CppType)
        return false;
    return d->extraData.cd->populateValueType;
}

QQmlType::CreateFunc QQmlType::createFunction() const
{
    if (!d || d->regType != CppType)
        return nullptr;
    return d->extraData.cd->newFunc;
}

QString QQmlType::noCreationReason() const
{
    if (!d || d->regType != CppType)
        return QString();
    return d->extraData.cd->noCreationReason;
}

bool QQmlType::isCreatable() const
{
    return d && d->regType == CppType && d->extraData.cd->newFunc;
}

QQmlType::ExtensionFunc QQmlType::extensionFunction() const
{
    if (!d)
        return nullptr;

    switch (d->regType) {
    case CppType:
        return d->extraData.cd->extFunc;
    case SingletonType:
        return d->extraData.sd->extFunc;
    default:
        return nullptr;
    }
}

const QMetaObject *QQmlType::extensionMetaObject() const
{
    if (!d)
        return nullptr;

    switch (d->regType) {
    case CppType:
        return d->extraData.cd->extMetaObject;
    case SingletonType:
        return d->extraData.sd->extMetaObject;
    default:
        return nullptr;
    }
}

bool QQmlType::isExtendedType() const
{
    if (!d)
        return false;
    d->init();

    return !d->metaObjects.isEmpty();
}

bool QQmlType::isSingleton() const
{
    return d && (d->regType == SingletonType || d->regType == CompositeSingletonType);
}

bool QQmlType::isInterface() const
{
    return d && d->regType == InterfaceType;
}

bool QQmlType::isComposite() const
{
    return d && d->isComposite();
}

bool QQmlType::isCompositeSingleton() const
{
    // if the outer type is a composite singleton, d->regType will indicate that even for
    // the inline component type
    // however, inline components can -at least for now- never be singletons
    // so we just do one additional check
    return d && d->regType == CompositeSingletonType && !isInlineComponentType();
}

bool QQmlType::isQObjectSingleton() const
{
    return d && d->regType == SingletonType && d->extraData.sd->singletonInstanceInfo->qobjectCallback;
}

bool QQmlType::isQJSValueSingleton() const
{
    return d && d->regType == SingletonType && d->extraData.sd->singletonInstanceInfo->scriptCallback;
}

bool QQmlType::isSequentialContainer() const
{
    return d && d->regType == SequentialContainerType;
}

QMetaType QQmlType::typeId() const
{
    return d ? d->typeId : QMetaType{};
}

QMetaType QQmlType::qListTypeId() const
{
    return d ? d->listId : QMetaType{};
}

QMetaSequence QQmlType::listMetaSequence() const
{
    return isSequentialContainer() ? *d->extraData.ld : QMetaSequence();
}

const QMetaObject *QQmlType::metaObject() const
{
    if (!d)
        return nullptr;
    d->init();

    if (d->metaObjects.isEmpty())
        return d->baseMetaObject;
    else
        return d->metaObjects.constFirst().metaObject;

}

const QMetaObject *QQmlType::baseMetaObject() const
{
    return d ? d->baseMetaObject : nullptr;
}

bool QQmlType::containsRevisionedAttributes() const
{
    if (!d)
        return false;
    d->init();

    return d->containsRevisionedAttributes;
}

QTypeRevision QQmlType::metaObjectRevision() const
{
    return d ? d->revision : QTypeRevision();
}

QQmlAttachedPropertiesFunc QQmlType::attachedPropertiesFunction(QQmlEnginePrivate *engine) const
{
    if (const QQmlTypePrivate *base = d ? d->attachedPropertiesBase(engine) : nullptr)
        return base->extraData.cd->attachedPropertiesFunc;
    return nullptr;
}

const QMetaObject *QQmlType::attachedPropertiesType(QQmlEnginePrivate *engine) const
{
    if (const QQmlTypePrivate *base = d ? d->attachedPropertiesBase(engine) : nullptr)
        return base->extraData.cd->attachedPropertiesType;
    return nullptr;
}

int QQmlType::parserStatusCast() const
{
    if (!d || d->regType != CppType)
        return -1;
    return d->extraData.cd->parserStatusCast;
}

int QQmlType::propertyValueSourceCast() const
{
    if (!d || d->regType != CppType)
        return -1;
    return d->extraData.cd->propertyValueSourceCast;
}

int QQmlType::propertyValueInterceptorCast() const
{
    if (!d || d->regType != CppType)
        return -1;
    return d->extraData.cd->propertyValueInterceptorCast;
}

int QQmlType::finalizerCast() const
{
    if (!d || d->regType != CppType)
        return -1;
    return d->extraData.cd->finalizerCast;
}

const char *QQmlType::interfaceIId() const
{
    if (!d || d->regType != InterfaceType)
        return nullptr;
    return d->iid;
}

int QQmlType::index() const
{
    return d ? d->index : -1;
}

bool QQmlType::isInlineComponentType() const {
    return d ? d->regType == QQmlType::InlineComponentType : false;
}

QUrl QQmlType::sourceUrl() const
{
    return d ? d->sourceUrl() : QUrl();
}

int QQmlType::enumValue(QQmlEnginePrivate *engine, const QHashedStringRef &name, bool *ok) const
{
    Q_ASSERT(ok);
    if (d) {
        *ok = true;

        d->initEnums(engine);

        int *rv = d->enums.value(name);
        if (rv)
            return *rv;
    }

    *ok = false;
    return -1;
}

int QQmlType::enumValue(QQmlEnginePrivate *engine, const QHashedCStringRef &name, bool *ok) const
{
    Q_ASSERT(ok);
    if (d) {
        *ok = true;

        d->initEnums(engine);

        int *rv = d->enums.value(name);
        if (rv)
            return *rv;
    }

    *ok = false;
    return -1;
}

int QQmlType::enumValue(QQmlEnginePrivate *engine, const QV4::String *name, bool *ok) const
{
    Q_ASSERT(ok);
    if (d) {
        *ok = true;

        d->initEnums(engine);

        int *rv = d->enums.value(name);
        if (rv)
            return *rv;
    }

    *ok = false;
    return -1;
}

int QQmlType::scopedEnumIndex(QQmlEnginePrivate *engine, const QV4::String *name, bool *ok) const
{
    Q_ASSERT(ok);
    if (d) {
        *ok = true;

        d->initEnums(engine);

        int *rv = d->scopedEnumIndex.value(name);
        if (rv)
            return *rv;
    }

    *ok = false;
    return -1;
}

int QQmlType::scopedEnumIndex(QQmlEnginePrivate *engine, const QString &name, bool *ok) const
{
    Q_ASSERT(ok);
    if (d) {
        *ok = true;

        d->initEnums(engine);

        int *rv = d->scopedEnumIndex.value(name);
        if (rv)
            return *rv;
    }

    *ok = false;
    return -1;
}

int QQmlType::scopedEnumValue(QQmlEnginePrivate *engine, int index, const QV4::String *name, bool *ok) const
{
    Q_UNUSED(engine);
    Q_ASSERT(ok);
    *ok = true;

    if (d) {
        Q_ASSERT(index > -1 && index < d->scopedEnums.size());
        int *rv = d->scopedEnums.at(index)->value(name);
        if (rv)
            return *rv;
    }

    *ok = false;
    return -1;
}

int QQmlType::scopedEnumValue(QQmlEnginePrivate *engine, int index, const QString &name, bool *ok) const
{
    Q_UNUSED(engine);
    Q_ASSERT(ok);
    *ok = true;

    if (d) {
        Q_ASSERT(index > -1 && index < d->scopedEnums.size());
        int *rv = d->scopedEnums.at(index)->value(name);
        if (rv)
            return *rv;
    }

    *ok = false;
    return -1;
}

int QQmlType::scopedEnumValue(QQmlEnginePrivate *engine, const QByteArray &scopedEnumName, const QByteArray &name, bool *ok) const
{
    Q_ASSERT(ok);
    if (d) {
        *ok = true;

        d->initEnums(engine);

        int *rv = d->scopedEnumIndex.value(QHashedCStringRef(scopedEnumName.constData(), scopedEnumName.size()));
        if (rv) {
            int index = *rv;
            Q_ASSERT(index > -1 && index < d->scopedEnums.size());
            rv = d->scopedEnums.at(index)->value(QHashedCStringRef(name.constData(), name.size()));
            if (rv)
                return *rv;
        }
    }

    *ok = false;
    return -1;
}

int QQmlType::scopedEnumValue(QQmlEnginePrivate *engine, QStringView scopedEnumName, QStringView name, bool *ok) const
{
    Q_ASSERT(ok);
    if (d) {
        *ok = true;

        d->initEnums(engine);

        int *rv = d->scopedEnumIndex.value(QHashedStringRef(scopedEnumName));
        if (rv) {
            int index = *rv;
            Q_ASSERT(index > -1 && index < d->scopedEnums.size());
            rv = d->scopedEnums.at(index)->value(QHashedStringRef(name));
            if (rv)
                return *rv;
        }
    }

    *ok = false;
    return -1;
}

void QQmlType::refHandle(const QQmlTypePrivate *priv)
{
    if (priv)
        priv->addref();
}

void QQmlType::derefHandle(const QQmlTypePrivate *priv)
{
    if (priv)
        priv->release();
}

int QQmlType::refCount(const QQmlTypePrivate *priv)
{
    if (priv)
        return priv->count();
    return -1;
}

QQmlType QQmlType::containingType() const
{
    Q_ASSERT(d && d->regType == QQmlType::RegistrationType::InlineComponentType);
    auto ret = QQmlType {d->extraData.id->containingType};
    Q_ASSERT(!ret.isInlineComponentType());
    return ret;
}

void QQmlType::createProxy(QObject *instance) const
{
    if (!d->metaObjects.isEmpty())
        (void)new QQmlProxyMetaObject(instance, &d->metaObjects);
}

QT_END_NAMESPACE
