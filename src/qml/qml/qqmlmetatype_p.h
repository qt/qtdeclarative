// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLMETATYPE_P_H
#define QQMLMETATYPE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmldirparser_p.h>
#include <private/qqmlmetaobject_p.h>
#include <private/qqmlproxymetaobject_p.h>
#include <private/qqmltype_p.h>
#include <private/qtqmlglobal_p.h>

QT_BEGIN_NAMESPACE

class QQmlTypeModule;
class QRecursiveMutex;
class QQmlError;
class QQmlValueType;

namespace QV4 { class ExecutableCompilationUnit; }

struct CompositeMetaTypeIds
{
private:
    int *refCount = nullptr;
    void deref();
    void ref()
    {
        Q_ASSERT(refCount);
        ++*refCount;
    }
public:
    CompositeMetaTypeIds() = default;
    CompositeMetaTypeIds(QMetaType id, QMetaType listId) : id(id), listId(listId) {}
    CompositeMetaTypeIds(const CompositeMetaTypeIds &other)
        : refCount(other.refCount), id(other.id), listId(other.listId)
    {
        if (refCount)
            ref();
    }
    CompositeMetaTypeIds(CompositeMetaTypeIds &&other)
        : refCount(other.refCount), id(other.id), listId(other.listId)
    {
        other.refCount = nullptr;
    }
    CompositeMetaTypeIds &operator=(const CompositeMetaTypeIds &other)
    {
        if (refCount)
            deref();
        refCount = other.refCount;
        id = other.id;
        listId = other.listId;
        if (refCount)
            ref();
        return *this;
    }
    CompositeMetaTypeIds &operator=(CompositeMetaTypeIds &&other)
    {
        if (refCount)
            deref();
        refCount = other.refCount;
        id = other.id;
        listId = other.listId;
        other.refCount = nullptr;
        return *this;
    }
    ~CompositeMetaTypeIds();
    static CompositeMetaTypeIds fromCompositeName(const QByteArray &name);
public:
    QMetaType id;
    QMetaType listId;
    bool isValid() const { return id.isValid() && listId.isValid(); }
};

class Q_QML_PRIVATE_EXPORT QQmlMetaType
{
    friend struct CompositeMetaTypeIds;
    friend class QQmlDesignerMetaObject;

    static CompositeMetaTypeIds registerInternalCompositeType(const QByteArray &className);
    static void unregisterInternalCompositeType(const CompositeMetaTypeIds &typeIds);

public:
    enum class RegistrationResult {
        Success,
        Failure,
        NoRegistrationFunction
    };

    static QQmlType registerType(const QQmlPrivate::RegisterType &type);
    static QQmlType registerInterface(const QQmlPrivate::RegisterInterface &type);
    static QQmlType registerSingletonType(const QQmlPrivate::RegisterSingletonType &type);
    static QQmlType registerCompositeSingletonType(const QQmlPrivate::RegisterCompositeSingletonType &type);
    static QQmlType registerCompositeType(const QQmlPrivate::RegisterCompositeType &type);
    static RegistrationResult registerPluginTypes(QObject *instance, const QString &basePath,
                                                  const QString &uri, const QString &typeNamespace,
                                                  QTypeRevision version, QList<QQmlError> *errors);
    static QQmlType typeForUrl(const QString &urlString, const QHashedStringRef& typeName,
                               bool isCompositeSingleton, QList<QQmlError> *errors,
                               QTypeRevision version = QTypeRevision());

    static void unregisterType(int type);

    static void registerMetaObjectForType(const QMetaObject *metaobject, QQmlTypePrivate *type);

    static void registerModule(const char *uri, QTypeRevision version);
    static bool protectModule(const QString &uri, QTypeRevision version,
                              bool weakProtectAllVersions = false);

    static void registerModuleImport(const QString &uri, QTypeRevision version,
                                     const QQmlDirParser::Import &import);
    static void unregisterModuleImport(const QString &uri, QTypeRevision version,
                                       const QQmlDirParser::Import &import);
    static QList<QQmlDirParser::Import> moduleImports(const QString &uri, QTypeRevision version);

    static int typeId(const char *uri, QTypeRevision version, const char *qmlName);

    static void registerUndeletableType(const QQmlType &dtype);

    static QList<QString> qmlTypeNames();
    static QList<QQmlType> qmlTypes();
    static QList<QQmlType> qmlSingletonTypes();
    static QList<QQmlType> qmlAllTypes();

    static QQmlType qmlType(const QString &qualifiedName, QTypeRevision version);
    static QQmlType qmlType(const QHashedStringRef &name, const QHashedStringRef &module, QTypeRevision version);
    static QQmlType qmlType(const QMetaObject *);
    static QQmlType qmlType(const QMetaObject *metaObject, const QHashedStringRef &module, QTypeRevision version);
    static QQmlType qmlTypeById(int qmlTypeId);

    static QQmlType qmlType(QMetaType metaType);
    static QQmlType qmlListType(QMetaType metaType);

    static QQmlType qmlType(const QUrl &unNormalizedUrl, bool includeNonFileImports = false);
    static QQmlType inlineComponentType(const QQmlType &containingType, const QString &name);
    static void associateInlineComponent(const QQmlType &containingType, const QString &name, const CompositeMetaTypeIds &metaTypeIds, QQmlType existingType);

    static QQmlPropertyCache::ConstPtr propertyCache(
            QObject *object, QTypeRevision version = QTypeRevision());
    static QQmlPropertyCache::ConstPtr propertyCache(
            const QMetaObject *metaObject, QTypeRevision version = QTypeRevision());
    static QQmlPropertyCache::ConstPtr propertyCache(
            const QQmlType &type, QTypeRevision version);

    // These methods may be called from the loader thread
    static QQmlMetaObject rawMetaObjectForType(QMetaType metaType);
    static QQmlMetaObject metaObjectForType(QMetaType metaType);
    static QQmlPropertyCache::ConstPtr propertyCacheForType(QMetaType metaType);
    static QQmlPropertyCache::ConstPtr rawPropertyCacheForType(QMetaType metaType);
    static QQmlPropertyCache::ConstPtr rawPropertyCacheForType(
            QMetaType metaType, QTypeRevision version);

    static void freeUnusedTypesAndCaches();

    static QMetaProperty defaultProperty(const QMetaObject *);
    static QMetaProperty defaultProperty(QObject *);
    static QMetaMethod defaultMethod(const QMetaObject *);
    static QMetaMethod defaultMethod(QObject *);

    static QObject *toQObject(const QVariant &, bool *ok = nullptr);

    static QMetaType listValueType(QMetaType type);
    static QQmlAttachedPropertiesFunc attachedPropertiesFunc(QQmlEnginePrivate *,
                                                             const QMetaObject *);
    static bool isInterface(QMetaType type);
    static const char *interfaceIId(QMetaType type);
    static bool isList(QMetaType type);

    static QTypeRevision latestModuleVersion(const QString &uri);
    static bool isStronglyLockedModule(const QString &uri, QTypeRevision version);
    static QTypeRevision matchingModuleVersion(const QString &module, QTypeRevision version);
    static QQmlTypeModule *typeModule(const QString &uri, QTypeRevision version);

    static QList<QQmlPrivate::AutoParentFunction> parentFunctions();

    enum class CachedUnitLookupError {
        NoError,
        NoUnitFound,
        VersionMismatch,
        NotFullyTyped
    };

    enum CacheMode { RejectAll, AcceptUntyped, RequireFullyTyped };
    static const QQmlPrivate::CachedQmlUnit *findCachedCompilationUnit(
            const QUrl &uri, CacheMode mode, CachedUnitLookupError *status);

    // used by tst_qqmlcachegen.cpp
    static void prependCachedUnitLookupFunction(QQmlPrivate::QmlUnitCacheLookupFunction handler);
    static void removeCachedUnitLookupFunction(QQmlPrivate::QmlUnitCacheLookupFunction handler);

    static QRecursiveMutex *typeRegistrationLock();

    static QString prettyTypeName(const QObject *object);

    template <typename QQmlTypeContainer>
    static void removeQQmlTypePrivate(QQmlTypeContainer &container,
                                      const QQmlTypePrivate *reference)
    {
        for (typename QQmlTypeContainer::iterator it = container.begin(); it != container.end();) {
            if (*it == reference)
                it = container.erase(it);
            else
                ++it;
        }
    }

    template <typename InlineComponentContainer>
    static void removeFromInlineComponents(
        InlineComponentContainer &container, const QQmlTypePrivate *reference)
    {
        for (auto it = container.begin(), end = container.end(); it != end;) {
            if (it.key().containingType == reference)
                it = container.erase(it);
            else
                ++it;
        }
    }

    static void registerTypeAlias(int typeId, const QString &name);

    static int registerAutoParentFunction(const QQmlPrivate::RegisterAutoParent &autoparent);
    static void unregisterAutoParentFunction(const QQmlPrivate::AutoParentFunction &function);

    static QQmlType registerSequentialContainer(
            const QQmlPrivate::RegisterSequentialContainer &sequenceRegistration);
    static void unregisterSequentialContainer(int id);

    static int registerUnitCacheHook(const QQmlPrivate::RegisterQmlUnitCacheHook &hookRegistration);
    static void clearTypeRegistrations();

    static QList<QQmlProxyMetaObject::ProxyData> proxyData(const QMetaObject *mo,
                                                           const QMetaObject *baseMetaObject,
                                                           QMetaObject *lastMetaObject);

    enum ClonePolicy {
        CloneAll, // default
        CloneEnumsOnly, // skip properties and methods
    };
    static void clone(QMetaObjectBuilder &builder, const QMetaObject *mo,
                      const QMetaObject *ignoreStart, const QMetaObject *ignoreEnd,
                      ClonePolicy policy);

    static void qmlInsertModuleRegistration(const QString &uri, void (*registerFunction)());
    static void qmlRemoveModuleRegistration(const QString &uri);

    static bool qmlRegisterModuleTypes(const QString &uri);

    static bool isValueType(QMetaType type);
    static QQmlValueType *valueType(QMetaType metaType);
    static const QMetaObject *metaObjectForValueType(QMetaType type);
    static const QMetaObject *metaObjectForValueType(const QQmlType &qmlType)
    {
        // Prefer the extension meta object, if any.
        // Extensions allow registration of non-gadget value types.
        if (const QMetaObject *extensionMetaObject = qmlType.extensionMetaObject()) {
            // This may be a namespace even if the original metaType isn't.
            // You can do such things with QML_FOREIGN declarations.
            if (extensionMetaObject->metaType().flags() & QMetaType::IsGadget)
                return extensionMetaObject;
        }

        if (const QMetaObject *qmlTypeMetaObject = qmlType.metaObject()) {
            // This may be a namespace even if the original metaType isn't.
            // You can do such things with QML_FOREIGN declarations.
            if (qmlTypeMetaObject->metaType().flags() & QMetaType::IsGadget)
                return qmlTypeMetaObject;
        }

        return nullptr;
    }

    static QQmlPropertyCache::ConstPtr findPropertyCacheInCompositeTypes(QMetaType t);
    static void registerInternalCompositeType(QV4::ExecutableCompilationUnit *compilationUnit);
    static void unregisterInternalCompositeType(QV4::ExecutableCompilationUnit *compilationUnit);
    static QV4::ExecutableCompilationUnit *obtainExecutableCompilationUnit(QMetaType type);
};

Q_DECLARE_TYPEINFO(QQmlMetaType, Q_RELOCATABLE_TYPE);

// used in QQmlListMetaType to tag the metatpye
inline const QMetaObject *dynamicQmlListMarker(const QtPrivate::QMetaTypeInterface *) {
    return nullptr;
};

inline const QMetaObject *dynamicQmlMetaObject(const QtPrivate::QMetaTypeInterface *iface) {
    return QQmlMetaType::metaObjectForType(QMetaType(iface)).metaObject();
};

// metatype interface for composite QML types
struct QQmlMetaTypeInterface : QtPrivate::QMetaTypeInterface
{
    const QByteArray name;
    QQmlMetaTypeInterface(const QByteArray &name)
        : QMetaTypeInterface {
            /*.revision=*/ QMetaTypeInterface::CurrentRevision,
            /*.alignment=*/ alignof(QObject *),
            /*.size=*/ sizeof(QObject *),
            /*.flags=*/ QtPrivate::QMetaTypeTypeFlags<QObject *>::Flags,
            /*.typeId=*/ 0,
            /*.metaObjectFn=*/ &dynamicQmlMetaObject,
            /*.name=*/ name.constData(),
            /*.defaultCtr=*/ [](const QMetaTypeInterface *, void *addr) {
                *static_cast<QObject **>(addr) = nullptr;
            },
            /*.copyCtr=*/ [](const QMetaTypeInterface *, void *addr, const void *other) {
                *static_cast<QObject **>(addr) = *static_cast<QObject *const *>(other);
            },
            /*.moveCtr=*/ [](const QMetaTypeInterface *, void *addr, void *other) {
                *static_cast<QObject **>(addr) = *static_cast<QObject **>(other);
            },
            /*.dtor=*/ [](const QMetaTypeInterface *, void *) {},
            /*.equals*/ nullptr,
            /*.lessThan*/ nullptr,
            /*.debugStream=*/ nullptr,
            /*.dataStreamOut=*/ nullptr,
            /*.dataStreamIn=*/ nullptr,
            /*.legacyRegisterOp=*/ nullptr
        }
        , name(name) { }
};

// metatype for qml list types
struct QQmlListMetaTypeInterface : QtPrivate::QMetaTypeInterface
{
    const QByteArray name;
    // if this interface is for list<type>; valueType stores the interface for type
    const QtPrivate::QMetaTypeInterface *valueType;
    QQmlListMetaTypeInterface(const QByteArray &name, const QtPrivate::QMetaTypeInterface *valueType)
        : QMetaTypeInterface {
            /*.revision=*/ QMetaTypeInterface::CurrentRevision,
            /*.alignment=*/ alignof(QQmlListProperty<QObject>),
            /*.size=*/ sizeof(QQmlListProperty<QObject>),
            /*.flags=*/ QtPrivate::QMetaTypeTypeFlags<QQmlListProperty<QObject>>::Flags,
            /*.typeId=*/ 0,
            /*.metaObjectFn=*/ &dynamicQmlListMarker,
            /*.name=*/ name.constData(),
            /*.defaultCtr=*/ [](const QMetaTypeInterface *, void *addr) {
                new (addr) QQmlListProperty<QObject> ();
            },
            /*.copyCtr=*/ [](const QMetaTypeInterface *, void *addr, const void *other) {
                new (addr) QQmlListProperty<QObject>(
                        *static_cast<const QQmlListProperty<QObject> *>(other));
            },
            /*.moveCtr=*/ [](const QMetaTypeInterface *, void *addr, void *other) {
                new (addr) QQmlListProperty<QObject>(
                        std::move(*static_cast<QQmlListProperty<QObject> *>(other)));
            },
            /*.dtor=*/ [](const QMetaTypeInterface *, void *addr) {
                static_cast<QQmlListProperty<QObject> *>(addr)->~QQmlListProperty<QObject>();
            },
            /*.equals*/ nullptr,
            /*.lessThan*/ nullptr,
            /*.debugStream=*/ nullptr,
            /*.dataStreamOut=*/ nullptr,
            /*.dataStreamIn=*/ nullptr,
            /*.legacyRegisterOp=*/ nullptr
        }
        , name(name), valueType(valueType) { }
};

QT_END_NAMESPACE

#endif // QQMLMETATYPE_P_H

