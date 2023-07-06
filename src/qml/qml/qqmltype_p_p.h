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
    QQmlTypePrivate(QQmlType::RegistrationType type);

    void init() const;
    void initEnums(QQmlEnginePrivate *engine) const;
    void insertEnums(const QMetaObject *metaObject) const;
    void insertEnumsFromPropertyCache(const QQmlPropertyCache::ConstPtr &cache) const;

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
        QQmlType::SingletonInstanceInfo *singletonInstanceInfo;
        QObject *(*extFunc)(QObject *);
        const QMetaObject *extMetaObject;
    };

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
    QTypeRevision revision;
    const QMetaObject *baseMetaObject;

    int index;
    mutable bool containsRevisionedAttributes;
    mutable QAtomicInteger<bool> isSetup;
    mutable QAtomicInteger<bool> isEnumFromCacheSetup;
    mutable QAtomicInteger<bool> isEnumFromBaseSetup;
    mutable QList<QQmlProxyMetaObject::ProxyData> metaObjects;
    mutable QStringHash<int> enums;
    mutable QStringHash<int> scopedEnumIndex; // maps from enum name to index in scopedEnums
    mutable QList<QStringHash<int>*> scopedEnums;

    void setName(const QString &uri, const QString &element);

private:
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

    void createListOfPossibleConflictingItems(const QMetaObject *metaObject, QList<EnumInfo> &enumInfoList, QStringList path) const;
    void createEnumConflictReport(const QMetaObject *metaObject, const QString &conflictingKey) const;
};

QT_END_NAMESPACE

#endif // QQMLTYPE_P_P_H
