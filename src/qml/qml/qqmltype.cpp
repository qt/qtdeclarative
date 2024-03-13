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
    : regType(type)
{
    switch (type) {
    case QQmlType::CppType:
        extraData.cppTypeData = new QQmlCppTypeData;
        extraData.cppTypeData->allocationSize = 0;
        extraData.cppTypeData->newFunc = nullptr;
        extraData.cppTypeData->createValueTypeFunc = nullptr;
        extraData.cppTypeData->parserStatusCast = -1;
        extraData.cppTypeData->extFunc = nullptr;
        extraData.cppTypeData->extMetaObject = nullptr;
        extraData.cppTypeData->customParser = nullptr;
        extraData.cppTypeData->attachedPropertiesFunc = nullptr;
        extraData.cppTypeData->attachedPropertiesType = nullptr;
        extraData.cppTypeData->propertyValueSourceCast = -1;
        extraData.cppTypeData->propertyValueInterceptorCast = -1;
        extraData.cppTypeData->finalizerCast = -1;
        extraData.cppTypeData->registerEnumClassesUnscoped = true;
        extraData.cppTypeData->registerEnumsFromRelatedTypes = true;
        break;
    case QQmlType::SingletonType:
    case QQmlType::CompositeSingletonType:
        extraData.singletonTypeData = new QQmlSingletonTypeData;
        extraData.singletonTypeData->singletonInstanceInfo = nullptr;
        extraData.singletonTypeData->extFunc = nullptr;
        extraData.singletonTypeData->extMetaObject = nullptr;
        break;
    case QQmlType::InterfaceType:
        extraData.interfaceTypeData = nullptr;
        break;
    case QQmlType::CompositeType:
        new (&extraData.compositeTypeData) QUrl();
        break;
    case QQmlType::InlineComponentType:
        new (&extraData.inlineComponentTypeData) QUrl();
        break;
    case QQmlType::SequentialContainerType:
        new (&extraData.sequentialContainerTypeData) QMetaSequence();
        break;
    default: qFatal("QQmlTypePrivate Internal Error.");
    }
}

QQmlTypePrivate::~QQmlTypePrivate()
{
    delete enums.fetchAndStoreAcquire(nullptr);
    delete proxyMetaObjects.fetchAndStoreAcquire(nullptr);

    if (const auto &iface = typeId.iface()) {
        if (iface->metaObjectFn == &dynamicQmlMetaObject)
            QQmlMetaType::unregisterInternalCompositeType(typeId, listId);
    }

    switch (regType) {
    case QQmlType::CppType:
        delete extraData.cppTypeData->customParser;
        delete extraData.cppTypeData;
        break;
    case QQmlType::SingletonType:
    case QQmlType::CompositeSingletonType:
        extraData.singletonTypeData->singletonInstanceInfo.reset();
        delete extraData.singletonTypeData;
        break;
    case QQmlType::CompositeType:
        extraData.compositeTypeData.~QUrl();
        break;
    case QQmlType::InlineComponentType:
        extraData.inlineComponentTypeData.~QUrl();
        break;
    case QQmlType::SequentialContainerType:
        extraData.sequentialContainerTypeData.~QMetaSequence();
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

const QQmlTypePrivate::ProxyMetaObjects *QQmlTypePrivate::init() const
{
    if (const ProxyMetaObjects *result = proxyMetaObjects.loadRelaxed())
        return result;

    ProxyMetaObjects *proxies = new ProxyMetaObjects;
    auto finalize = [this, proxies]() -> const ProxyMetaObjects *{
        const ProxyMetaObjects *concurrentModification;
        if (proxyMetaObjects.testAndSetOrdered(nullptr, proxies, concurrentModification))
            return proxies;

        delete proxies;
        return concurrentModification;
    };

    const QMetaObject *mo = baseMetaObject;
    if (!mo) {
        // version 0 singleton type without metaobject information
        return finalize();
    }

    QList<QQmlProxyMetaObject::ProxyData> metaObjects;

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
        setupExtendedMetaObject(extraData.singletonTypeData->extMetaObject, extraData.singletonTypeData->extFunc);
    else if (regType == QQmlType::CppType)
        setupExtendedMetaObject(extraData.cppTypeData->extMetaObject, extraData.cppTypeData->extFunc);

    metaObjects.append(QQmlMetaType::proxyData(
            mo, baseMetaObject, metaObjects.isEmpty() ? nullptr
                                                      : metaObjects.constLast().metaObject));

    for (int ii = 0; ii < metaObjects.size(); ++ii) {
        metaObjects[ii].propertyOffset =
                metaObjects.at(ii).metaObject->propertyOffset();
        metaObjects[ii].methodOffset =
                metaObjects.at(ii).metaObject->methodOffset();
    }

    bool containsRevisionedAttributes = false;

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

    proxies->data = std::move(metaObjects);
    proxies->containsRevisionedAttributes = containsRevisionedAttributes;

    return finalize();
}

const QQmlTypePrivate::Enums *QQmlTypePrivate::initEnums(QQmlEnginePrivate *engine) const
{
    if (const Enums *result = enums.loadRelaxed())
        return result;

    QQmlPropertyCache::ConstPtr cache;
    if (isComposite()) {
        cache = compositePropertyCache(engine);
        if (!cache)
            return nullptr; // Composite type not ready, yet.
    }

    Enums *newEnums = new Enums;

    // beware: It could be a singleton type without metaobject

    if (cache)
        insertEnumsFromPropertyCache(newEnums, cache);

    if (baseMetaObject) {
        // init() can add to the metaObjects list. Therefore, check proxies->data only below
        const ProxyMetaObjects *proxies = init();
        insertEnums(
                newEnums,
                proxies->data.isEmpty() ? baseMetaObject : proxies->data.constFirst().metaObject);
    }

    const Enums *concurrentModification;
    if (enums.testAndSetOrdered(nullptr, newEnums, concurrentModification))
        return newEnums;

    delete newEnums;
    return concurrentModification;
}

void QQmlTypePrivate::insertEnums(Enums *enums, const QMetaObject *metaObject) const
{
    // Add any enum values defined by 'related' classes
    if (regType != QQmlType::CppType || extraData.cppTypeData->registerEnumsFromRelatedTypes) {
        if (const auto *related = metaObject->d.relatedMetaObjects) {
            while (const QMetaObject *relatedMetaObject = *related) {
                insertEnums(enums, relatedMetaObject);
                ++related;
            }
        }
    }

    QSet<QString> localEnums;
    const QMetaObject *localMetaObject = nullptr;

    // ### TODO (QTBUG-123294): track this at instance creation time
    auto shouldSingletonAlsoRegisterUnscoped = [&](){
        Q_ASSERT(regType == QQmlType::SingletonType);
        if (!baseMetaObject)
            return true;
        int idx = baseMetaObject->indexOfClassInfo("RegisterEnumClassesUnscoped");
        if (idx == -1)
            return true;
        if (qstrcmp(baseMetaObject->classInfo(idx).value(), "false") == 0)
            return false;
        return true;
    };

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
        const bool shouldRegisterUnscoped = !isScoped
                || (regType == QQmlType::CppType && extraData.cppTypeData->registerEnumClassesUnscoped)
                || (regType == QQmlType::SingletonType && shouldSingletonAlsoRegisterUnscoped())
        ;

        for (int jj = 0; jj < e.keyCount(); ++jj) {
            const QString key = QString::fromUtf8(e.key(jj));
            const int value = e.value(jj);
            if (shouldRegisterUnscoped) {
                if (localEnums.contains(key)) {
                    auto existingEntry = enums->enums.find(key);
                    if (existingEntry != enums->enums.end() && existingEntry.value() != value) {
                        qWarning("Previously registered enum will be overwritten due to name clash: %s.%s", metaObject->className(), key.toUtf8().constData());
                        createEnumConflictReport(metaObject, key);
                    }
                } else {
                    localEnums.insert(key);
                }
                enums->enums.insert(key, value);
            }
            if (isScoped)
                scoped->insert(key, value);
        }

        if (isScoped) {
            enums->scopedEnums << scoped;
            enums->scopedEnumIndex.insert(QString::fromUtf8(e.name()), enums->scopedEnums.size()-1);
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
        Enums *enums, const QQmlPropertyCache::ConstPtr &cache) const
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
                enums->enums.insert(value.namedValue, value.value);
                scoped->insert(value.namedValue, value.value);
            }
            enums->scopedEnums << scoped;
            enums->scopedEnumIndex.insert(enumData->name, enums->scopedEnums.size()-1);
        }
    }
    insertEnums(enums, cppMetaObject);
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
            return d->extraData.singletonTypeData->singletonInstanceInfo->typeName;
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

    QObject *rv = (QObject *)operator new(d->extraData.cppTypeData->allocationSize + additionalMemory);
    d->extraData.cppTypeData->newFunc(rv, d->extraData.cppTypeData->userdata);

    createProxy(rv);
    *memory = ((char *)rv) + d->extraData.cppTypeData->allocationSize;
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

QQmlType::SingletonInstanceInfo::ConstPtr QQmlType::singletonInstanceInfo() const
{
    if (!d)
        return {};
    if (d->regType != SingletonType && d->regType != CompositeSingletonType)
        return {};
    return d->extraData.singletonTypeData->singletonInstanceInfo;
}

QQmlCustomParser *QQmlType::customParser() const
{
    if (!d)
        return nullptr;
    if (d->regType != CppType)
        return nullptr;
    return d->extraData.cppTypeData->customParser;
}

QQmlType::CreateValueTypeFunc QQmlType::createValueTypeFunction() const
{
    if (!d || d->regType != CppType)
        return nullptr;
    return d->extraData.cppTypeData->createValueTypeFunc;
}

bool QQmlType::canConstructValueType() const
{
    if (!d || d->regType != CppType)
        return false;
    return d->extraData.cppTypeData->constructValueType;
}

bool QQmlType::canPopulateValueType() const
{
    if (!d || d->regType != CppType)
        return false;
    return d->extraData.cppTypeData->populateValueType;
}

QQmlType::CreateFunc QQmlType::createFunction() const
{
    if (!d || d->regType != CppType)
        return nullptr;
    return d->extraData.cppTypeData->newFunc;
}

QString QQmlType::noCreationReason() const
{
    if (!d || d->regType != CppType)
        return QString();
    return d->extraData.cppTypeData->noCreationReason;
}

bool QQmlType::isCreatable() const
{
    return d && d->regType == CppType && d->extraData.cppTypeData->newFunc;
}

QQmlType::ExtensionFunc QQmlType::extensionFunction() const
{
    if (!d)
        return nullptr;

    switch (d->regType) {
    case CppType:
        return d->extraData.cppTypeData->extFunc;
    case SingletonType:
        return d->extraData.singletonTypeData->extFunc;
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
        return d->extraData.cppTypeData->extMetaObject;
    case SingletonType:
        return d->extraData.singletonTypeData->extMetaObject;
    default:
        return nullptr;
    }
}

bool QQmlType::isExtendedType() const
{
    return d && !d->init()->data.isEmpty();
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
    return d && d->regType == SingletonType && d->extraData.singletonTypeData->singletonInstanceInfo->qobjectCallback;
}

bool QQmlType::isQJSValueSingleton() const
{
    return d && d->regType == SingletonType && d->extraData.singletonTypeData->singletonInstanceInfo->scriptCallback;
}

bool QQmlType::isSequentialContainer() const
{
    return d && d->regType == SequentialContainerType;
}

bool QQmlType::isValueType() const
{
    return d && d->isValueType();
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
    return isSequentialContainer() ? d->extraData.sequentialContainerTypeData : QMetaSequence();
}

const QMetaObject *QQmlType::metaObject() const
{
    return d ? d->metaObject() : nullptr;
}

const QMetaObject *QQmlType::metaObjectForValueType() const
{
    Q_ASSERT(d);
    return d->metaObjectForValueType();
}

const QMetaObject *QQmlType::baseMetaObject() const
{
    return d ? d->baseMetaObject : nullptr;
}

bool QQmlType::containsRevisionedAttributes() const
{
    return d && d->init()->containsRevisionedAttributes;
}

QTypeRevision QQmlType::metaObjectRevision() const
{
    return d ? d->revision : QTypeRevision();
}

QQmlAttachedPropertiesFunc QQmlType::attachedPropertiesFunction(QQmlEnginePrivate *engine) const
{
    if (const QQmlTypePrivate *base = d ? d->attachedPropertiesBase(engine) : nullptr)
        return base->extraData.cppTypeData->attachedPropertiesFunc;
    return nullptr;
}

const QMetaObject *QQmlType::attachedPropertiesType(QQmlEnginePrivate *engine) const
{
    if (const QQmlTypePrivate *base = d ? d->attachedPropertiesBase(engine) : nullptr)
        return base->extraData.cppTypeData->attachedPropertiesType;
    return nullptr;
}

int QQmlType::parserStatusCast() const
{
    if (!d || d->regType != CppType)
        return -1;
    return d->extraData.cppTypeData->parserStatusCast;
}

int QQmlType::propertyValueSourceCast() const
{
    if (!d || d->regType != CppType)
        return -1;
    return d->extraData.cppTypeData->propertyValueSourceCast;
}

int QQmlType::propertyValueInterceptorCast() const
{
    if (!d || d->regType != CppType)
        return -1;
    return d->extraData.cppTypeData->propertyValueInterceptorCast;
}

int QQmlType::finalizerCast() const
{
    if (!d || d->regType != CppType)
        return -1;
    return d->extraData.cppTypeData->finalizerCast;
}

const char *QQmlType::interfaceIId() const
{
    if (!d || d->regType != InterfaceType)
        return nullptr;
    return d->extraData.interfaceTypeData;
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
    return QQmlTypePrivate::enumValue(d, engine, name, ok);
}

int QQmlType::enumValue(QQmlEnginePrivate *engine, const QHashedCStringRef &name, bool *ok) const
{
    return QQmlTypePrivate::enumValue(d, engine, name, ok);
}

int QQmlType::enumValue(QQmlEnginePrivate *engine, const QV4::String *name, bool *ok) const
{
    return QQmlTypePrivate::enumValue(d, engine, name, ok);
}

int QQmlType::scopedEnumIndex(QQmlEnginePrivate *engine, const QV4::String *name, bool *ok) const
{
    return QQmlTypePrivate::scopedEnumIndex(d, engine, name, ok);
}

int QQmlType::scopedEnumIndex(QQmlEnginePrivate *engine, const QString &name, bool *ok) const
{
    return QQmlTypePrivate::scopedEnumIndex(d, engine, name, ok);
}

int QQmlType::scopedEnumValue(QQmlEnginePrivate *engine, int index, const QV4::String *name, bool *ok) const
{
    return QQmlTypePrivate::scopedEnumValue(d, engine, index, name, ok);
}

int QQmlType::scopedEnumValue(QQmlEnginePrivate *engine, int index, const QString &name, bool *ok) const
{
    return QQmlTypePrivate::scopedEnumValue(d, engine, index, name, ok);
}

int QQmlType::scopedEnumValue(QQmlEnginePrivate *engine, const QHashedStringRef &scopedEnumName, const QHashedStringRef &name, bool *ok) const
{
    return QQmlTypePrivate::scopedEnumValue(d, engine, scopedEnumName, name, ok);
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

void QQmlType::createProxy(QObject *instance) const
{
    const QQmlTypePrivate::ProxyMetaObjects *proxies = d->init();
    if (!proxies->data.isEmpty())
        (void)new QQmlProxyMetaObject(instance, &proxies->data);
}

QT_END_NAMESPACE
