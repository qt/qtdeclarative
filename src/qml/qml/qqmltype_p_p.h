// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTYPE_P_P_H
#define QQMLTYPE_P_P_H

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

#include <private/qqmltype_p.h>
#include <private/qstringhash_p.h>
#include <private/qqmlproxymetaobject_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qqmlmetatype_p.h>

#include <QAtomicInteger>

QT_BEGIN_NAMESPACE

class QQmlTypePrivate final : public QQmlRefCounted<QQmlTypePrivate>
{
    Q_DISABLE_COPY_MOVE(QQmlTypePrivate)
public:
    struct ProxyMetaObjects
    {
        ~ProxyMetaObjects()
        {
            for (const QQmlProxyMetaObject::ProxyData &metaObject : data)
                free(metaObject.metaObject);
        }

        QList<QQmlProxyMetaObject::ProxyData> data;
        bool containsRevisionedAttributes = false;
    };

    struct Enums
    {
        ~Enums() { qDeleteAll(scopedEnums); }

        QStringHash<int> enums;
        QStringHash<int> scopedEnumIndex; // maps from enum name to index in scopedEnums
        QList<QStringHash<int> *> scopedEnums;
    };

    QQmlTypePrivate(QQmlType::RegistrationType type);

    const ProxyMetaObjects *init() const;

    QUrl sourceUrl() const
    {
        switch (regType) {
        case QQmlType::CompositeType:
            return extraData.compositeTypeData;
        case QQmlType::CompositeSingletonType:
            return extraData.singletonTypeData->singletonInstanceInfo->url;
        case QQmlType::InlineComponentType:
            return extraData.inlineComponentTypeData;
        default:
            return QUrl();
        }
    }

    const QQmlTypePrivate *attachedPropertiesBase(QQmlEnginePrivate *engine) const
    {
        for (const QQmlTypePrivate *d = this; d; d = d->resolveCompositeBaseType(engine).d.data()) {
            if (d->regType == QQmlType::CppType)
                return d->extraData.cppTypeData->attachedPropertiesType ? d : nullptr;

            if (d->regType != QQmlType::CompositeType)
                return nullptr;
        }
        return nullptr;
    }

    bool isComposite() const
    {
        return regType == QQmlType::CompositeType || regType == QQmlType::CompositeSingletonType;
    }

    bool isValueType() const
    {
        return regType == QQmlType::CppType && !(typeId.flags() & QMetaType::PointerToQObject);
    }

    QQmlType resolveCompositeBaseType(QQmlEnginePrivate *engine) const;
    QQmlPropertyCache::ConstPtr compositePropertyCache(QQmlEnginePrivate *engine) const;

    struct QQmlCppTypeData
    {
        int allocationSize;
        void (*newFunc)(void *, void *);
        void *userdata = nullptr;
        QString noCreationReason;
        QVariant (*createValueTypeFunc)(const QJSValue &);
        int parserStatusCast;
        QObject *(*extFunc)(QObject *);
        const QMetaObject *extMetaObject;
        QQmlCustomParser *customParser;
        QQmlAttachedPropertiesFunc attachedPropertiesFunc;
        const QMetaObject *attachedPropertiesType;
        int propertyValueSourceCast;
        int propertyValueInterceptorCast;
        int finalizerCast;
        bool registerEnumClassesUnscoped;
        bool registerEnumsFromRelatedTypes;
        bool constructValueType;
        bool populateValueType;
    };

    struct QQmlSingletonTypeData
    {
        QQmlType::SingletonInstanceInfo::ConstPtr singletonInstanceInfo;
        QObject *(*extFunc)(QObject *);
        const QMetaObject *extMetaObject;
    };

    int index = -1;

    union extraData {
        extraData() {}  // QQmlTypePrivate() does the actual construction.
        ~extraData() {} // ~QQmlTypePrivate() does the actual destruction.

        QQmlCppTypeData *cppTypeData;
        QQmlSingletonTypeData *singletonTypeData;
        QUrl compositeTypeData;
        QUrl inlineComponentTypeData;
        QMetaSequence sequentialContainerTypeData;
        const char *interfaceTypeData;
    } extraData;
    static_assert(sizeof(extraData) == sizeof(void *));

    QHashedString module;
    QString name;
    QString elementName;
    QMetaType typeId;
    QMetaType listId;
    QQmlType::RegistrationType regType;
    QTypeRevision version;
    QTypeRevision revision = QTypeRevision::zero();
    const QMetaObject *baseMetaObject = nullptr;

    void setName(const QString &uri, const QString &element);

    template<typename String>
    static int enumValue(
            const QQmlRefPointer<const QQmlTypePrivate> &d, QQmlEnginePrivate *engine,
            const String &name, bool *ok)
    {
        return doGetEnumValue(d, engine, [&](const QQmlTypePrivate::Enums *enums) {
            return enums->enums.value(name);
        }, ok);
    }

    template<typename String>
    static int scopedEnumIndex(
            const QQmlRefPointer<const QQmlTypePrivate> &d, QQmlEnginePrivate *engine,
            const String &name, bool *ok)
    {
        return doGetEnumValue(d, engine, [&](const QQmlTypePrivate::Enums *enums) {
            return enums->scopedEnumIndex.value(name);
        }, ok);
    }

    template<typename String>
    static int scopedEnumValue(
            const QQmlRefPointer<const QQmlTypePrivate> &d, QQmlEnginePrivate *engine, int index,
            const String &name, bool *ok)
    {
        return doGetEnumValue(d, engine, [&](const QQmlTypePrivate::Enums *enums) {
            Q_ASSERT(index > -1 && index < enums->scopedEnums.size());
            return enums->scopedEnums.at(index)->value(name);
        }, ok);
    }

    template<typename String1, typename String2>
    static int scopedEnumValue(
            const QQmlRefPointer<const QQmlTypePrivate> &d, QQmlEnginePrivate *engine,
            const String1 &scopedEnumName, const String2 &name, bool *ok)
    {
        return doGetEnumValue(d, engine, [&](const QQmlTypePrivate::Enums *enums) -> const int * {
            const int *rv = enums->scopedEnumIndex.value(scopedEnumName);
            if (!rv)
                return nullptr;

            const int index = *rv;
            Q_ASSERT(index > -1 && index < enums->scopedEnums.size());
            return enums->scopedEnums.at(index)->value(name);
        }, ok);
    }

    const QMetaObject *metaObject() const
    {
        if (isValueType())
            return metaObjectForValueType();

        const QQmlTypePrivate::ProxyMetaObjects *proxies = init();
        return proxies->data.isEmpty()
                ? baseMetaObject
                : proxies->data.constFirst().metaObject;
    }

    const QMetaObject *metaObjectForValueType() const
    {
        Q_ASSERT(isValueType());

        // Prefer the extension meta object, if any.
        // Extensions allow registration of non-gadget value types.
        if (const QMetaObject *extensionMetaObject = extraData.cppTypeData->extMetaObject) {
            // This may be a namespace even if the original metaType isn't.
            // You can do such things with QML_FOREIGN declarations.
            if (extensionMetaObject->metaType().flags() & QMetaType::IsGadget)
                return extensionMetaObject;
        }

        if (baseMetaObject) {
            // This may be a namespace even if the original metaType isn't.
            // You can do such things with QML_FOREIGN declarations.
            if (baseMetaObject->metaType().flags() & QMetaType::IsGadget)
                return baseMetaObject;
        }

        return nullptr;
    }

private:
    mutable QAtomicPointer<const ProxyMetaObjects> proxyMetaObjects;
    mutable QAtomicPointer<const Enums> enums;

    ~QQmlTypePrivate();
    friend class QQmlRefCounted<QQmlTypePrivate>;

    struct EnumInfo {
        QStringList path;
        QString metaObjectName;
        QString enumName;
        QString enumKey;
        QString metaEnumScope;
        bool scoped;
    };

    template<typename Op>
    static int doGetEnumValue(
            const QQmlRefPointer<const QQmlTypePrivate> &d, QQmlEnginePrivate *engine,
            Op &&op, bool *ok)
    {
        Q_ASSERT(ok);
        if (d) {
            if (const QQmlTypePrivate::Enums *enums = d->initEnums(engine)) {
                if (const int *rv = op(enums)) {
                    *ok = true;
                    return *rv;
                }
            }
        }

        *ok = false;
        return -1;
    }

    const Enums *initEnums(QQmlEnginePrivate *engine) const;
    void insertEnums(Enums *enums, const QMetaObject *metaObject) const;
    void insertEnumsFromPropertyCache(Enums *enums, const QQmlPropertyCache::ConstPtr &cache) const;

    void createListOfPossibleConflictingItems(const QMetaObject *metaObject, QList<EnumInfo> &enumInfoList, QStringList path) const;
    void createEnumConflictReport(const QMetaObject *metaObject, const QString &conflictingKey) const;
};

QT_END_NAMESPACE

#endif // QQMLTYPE_P_P_H
