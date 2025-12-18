// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQMLPROPERTYCACHECREATOR_P_H
#define QQMLPROPERTYCACHECREATOR_P_H

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

#include <private/qqmlvaluetype_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlmetaobject_p.h>
#include <private/qqmltypedata_p.h>
#include <private/inlinecomponentutils_p.h>
#include <private/qqmlsourcecoordinate_p.h>
#include <private/qqmlsignalnames_p.h>
#include <private/qexpected_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qscopedvaluerollback.h>

#if QT_CONFIG(regularexpression)
#include <QtCore/qregularexpression.h>
#endif

#include <vector>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(invalidOverride);

inline QQmlError qQmlCompileError(const QV4::CompiledData::Location &location,
                                                  const QString &description)
{
    QQmlError error;
    error.setLine(qmlConvertSourceCoordinate<quint32, int>(location.line()));
    error.setColumn(qmlConvertSourceCoordinate<quint32, int>(location.column()));
    error.setDescription(description);
    return error;
}

struct QQmlBindingInstantiationContext {
    QQmlBindingInstantiationContext() {}
    QQmlBindingInstantiationContext(
            int referencingObjectIndex, const QV4::CompiledData::Binding *instantiatingBinding,
            const QString &instantiatingPropertyName,
            const QQmlPropertyCache::ConstPtr &referencingObjectPropertyCache);

    bool resolveInstantiatingProperty();
    QQmlPropertyCache::ConstPtr instantiatingPropertyCache() const;

    int referencingObjectIndex = -1;
    const QV4::CompiledData::Binding *instantiatingBinding = nullptr;
    QString instantiatingPropertyName;
    QQmlPropertyCache::ConstPtr referencingObjectPropertyCache;
    const QQmlPropertyData *instantiatingProperty = nullptr;
};

struct QQmlPendingGroupPropertyBindings : public QVector<QQmlBindingInstantiationContext>
{
    void resolveMissingPropertyCaches(
            QQmlPropertyCacheVector *propertyCaches) const;
};

struct QQmlPropertyCacheCreatorBase
{
    Q_DECLARE_TR_FUNCTIONS(QQmlPropertyCacheCreatorBase)
public:
    static QAtomicInt Q_AUTOTEST_EXPORT classIndexCounter;

    static QMetaType metaTypeForPropertyType(QV4::CompiledData::CommonType type)
    {
        switch (type) {
        case QV4::CompiledData::CommonType::Void:     return QMetaType();
        case QV4::CompiledData::CommonType::Var:      return QMetaType::fromType<QVariant>();
        case QV4::CompiledData::CommonType::Int:      return QMetaType::fromType<int>();
        case QV4::CompiledData::CommonType::Bool:     return QMetaType::fromType<bool>();
        case QV4::CompiledData::CommonType::Real:     return QMetaType::fromType<qreal>();
        case QV4::CompiledData::CommonType::String:   return QMetaType::fromType<QString>();
        case QV4::CompiledData::CommonType::Url:      return QMetaType::fromType<QUrl>();
        case QV4::CompiledData::CommonType::Time:     return QMetaType::fromType<QTime>();
        case QV4::CompiledData::CommonType::Date:     return QMetaType::fromType<QDate>();
        case QV4::CompiledData::CommonType::DateTime: return QMetaType::fromType<QDateTime>();
#if QT_CONFIG(regularexpression)
        case QV4::CompiledData::CommonType::RegExp:   return QMetaType::fromType<QRegularExpression>();
#else
        case QV4::CompiledData::CommonType::RegExp:   return QMetaType();
#endif
        case QV4::CompiledData::CommonType::Rect:     return QMetaType::fromType<QRectF>();
        case QV4::CompiledData::CommonType::Point:    return QMetaType::fromType<QPointF>();
        case QV4::CompiledData::CommonType::Size:     return QMetaType::fromType<QSizeF>();
        case QV4::CompiledData::CommonType::Invalid:  break;
        };
        return QMetaType {};
    }

    static QMetaType listTypeForPropertyType(QV4::CompiledData::CommonType type)
    {
        switch (type) {
        case QV4::CompiledData::CommonType::Void:     return QMetaType();
        case QV4::CompiledData::CommonType::Var:      return QMetaType::fromType<QList<QVariant>>();
        case QV4::CompiledData::CommonType::Int:      return QMetaType::fromType<QList<int>>();
        case QV4::CompiledData::CommonType::Bool:     return QMetaType::fromType<QList<bool>>();
        case QV4::CompiledData::CommonType::Real:     return QMetaType::fromType<QList<qreal>>();
        case QV4::CompiledData::CommonType::String:   return QMetaType::fromType<QList<QString>>();
        case QV4::CompiledData::CommonType::Url:      return QMetaType::fromType<QList<QUrl>>();
        case QV4::CompiledData::CommonType::Time:     return QMetaType::fromType<QList<QTime>>();
        case QV4::CompiledData::CommonType::Date:     return QMetaType::fromType<QList<QDate>>();
        case QV4::CompiledData::CommonType::DateTime: return QMetaType::fromType<QList<QDateTime>>();
#if QT_CONFIG(regularexpression)
        case QV4::CompiledData::CommonType::RegExp:   return QMetaType::fromType<QList<QRegularExpression>>();
#else
        case QV4::CompiledData::CommonType::RegExp:   return QMetaType();
#endif
        case QV4::CompiledData::CommonType::Rect:     return QMetaType::fromType<QList<QRectF>>();
        case QV4::CompiledData::CommonType::Point:    return QMetaType::fromType<QList<QPointF>>();
        case QV4::CompiledData::CommonType::Size:     return QMetaType::fromType<QList<QSizeF>>();
        case QV4::CompiledData::CommonType::Invalid:  break;
        };
        return QMetaType {};
    }

    static QV4::CompiledData::CommonType propertyTypeForMetaType(QMetaType metaType) {
        if (!metaType.isValid())
            return QV4::CompiledData::CommonType::Void;

        switch (metaType.id()) {
        case QMetaType::QVariant:
            return QV4::CompiledData::CommonType::Var;
        case QMetaType::Int:
            return QV4::CompiledData::CommonType::Int;
        case QMetaType::Bool:
            return QV4::CompiledData::CommonType::Bool;
        case QMetaType::QReal:
            return QV4::CompiledData::CommonType::Real;
        case QMetaType::QString:
            return QV4::CompiledData::CommonType::String;
        case QMetaType::QUrl:
            return QV4::CompiledData::CommonType::Url;
        case QMetaType::QTime:
            return QV4::CompiledData::CommonType::Time;
        case QMetaType::QDate:
            return QV4::CompiledData::CommonType::Date;
        case QMetaType::QDateTime:
            return QV4::CompiledData::CommonType::DateTime;
#if QT_CONFIG(regularexpression)
        case QMetaType::QRegularExpression:
            return QV4::CompiledData::CommonType::RegExp;
#endif
        case QMetaType::QRectF:
            return QV4::CompiledData::CommonType::Rect;
        case QMetaType::QPointF:
            return QV4::CompiledData::CommonType::Point;
        case QMetaType::QSizeF:
            return QV4::CompiledData::CommonType::Size;
        default:
            break;
        }
        return QV4::CompiledData::CommonType::Invalid;
    }

    static bool canCreateClassNameTypeByUrl(const QUrl &url);
    static QByteArray createClassNameTypeByUrl(const QUrl &url);
    static QByteArray createClassNameForInlineComponent(const QUrl &url);

    struct IncrementalResult {
        // valid if and only if an error occurred
        QQmlError error;
        // true if there was no error and there are still components left to process
        bool canResume = false;
        // the object index of the last processed (inline) component root.
        int processedRoot = 0;
    };

    // TODO just reuse message from the QQmlPropertyCacheCreator::maybeLog
    static QString explain(OverrideSemantics::Status reason)
    {
        switch (reason) {
        case OverrideSemantics::Status::MissingBase:
            return tr("Nothing to override. Remove \"override\" keyword");
        case OverrideSemantics::Status::OverridingFinal:
            return tr("Cannot override FINAL property");
        case OverrideSemantics::Status::OverridingNonVirtual:
            return tr("Cannot override non virtual property. Add \"virtual\" to the property of "
                      "the base object");
        default:
            return tr("unknown");
        }
    }
};

template <typename ObjectContainer>
class QQmlPropertyCacheCreator : public QQmlPropertyCacheCreatorBase
{
public:
    using CompiledObject = typename ObjectContainer::CompiledObject;
    using InlineComponent = typename std::remove_reference<decltype (*(std::declval<CompiledObject>().inlineComponentsBegin()))>::type;

    QQmlPropertyCacheCreator(QQmlPropertyCacheVector *propertyCaches,
                             QQmlPendingGroupPropertyBindings *pendingGroupPropertyBindings,
                             QQmlTypeLoader *typeLoader,
                             const ObjectContainer *objectContainer, const QQmlImports *imports,
                             const QByteArray &typeClassName);
    ~QQmlPropertyCacheCreator() { propertyCaches->seal(); }


    /*!
        \internal
        Creates the property cache for the CompiledObjects of objectContainer,
        one (inline) root component at a time.

        \note Later compiler passes might modify those property caches. Therefore,
        the actual metaobjects are not created yet.
     */
    IncrementalResult buildMetaObjectsIncrementally();

    /*!
        \internal
        Returns a valid error if the inline components of the objectContainer
        form a cycle. Otherwise an invalid error is returned
     */
    QQmlError verifyNoICCycle();

    /*!
        \internal
        Tries to creates a property cache for the CompiledObject based on the cache of a base type.
        Returns QQmlError in case of invalid overrides

        \note: Aliases are added separately in appendAliasToPropertyCache
      */
    [[nodiscard]] q23::expected<QQmlPropertyCache::Ptr, QQmlError>
    tryDeriveCacheFrom(const CompiledObject *obj, const QQmlPropertyCache::ConstPtr &baseTypeCache,
                       QByteArray dynamicClassName = QByteArray()) const;

    /*!
        \internal
        Tries to create a QQmlPropertyData based on IR of a property.
        This involves property type resolution and property flags creation

        \a notifyIndex MUST be in the signal index range (see QObjectPrivate::signalIndex()).
        This is different from QMetaMethod::methodIndex()

        return error in case of failed type resolution
    */
    [[nodiscard]] q23::expected<QQmlPropertyData, QQmlError>
    tryCreateQQmlPropertyData(const QV4::CompiledData::Property &propertyIR, int coreIndex,
                              int notifyIndex) const;

protected:
    enum class VMEMetaObjectIsRequired { Maybe, Always };

    QQmlError buildMetaObjectRecursively(int objectIndex,
                                         const QQmlBindingInstantiationContext &context,
                                         VMEMetaObjectIsRequired isVMERequired);
    QQmlPropertyCache::ConstPtr propertyCacheForObject(const CompiledObject *obj, const QQmlBindingInstantiationContext &context, QQmlError *error) const;
    QQmlError createMetaObject(int objectIndex, const CompiledObject *obj, const QQmlPropertyCache::ConstPtr &baseTypeCache);

    QMetaType metaTypeForParameter(const QV4::CompiledData::ParameterType &param,
                                   QString *customTypeName = nullptr) const;

    QString stringAt(int index) const { return objectContainer->stringAt(index); }

private:
    struct PropertyType
    {
        QMetaType metaType;
        QTypeRevision revision = QTypeRevision::zero();
        QV4::CompiledData::CommonType commonType = QV4::CompiledData::CommonType::Invalid;
    };

    [[nodiscard]] q23::expected<PropertyType, QQmlError>
    tryResolvePropertyType(const QV4::CompiledData::Property &propertyIR) const;

    // can be made a free function
    [[nodiscard]] static QQmlPropertyData::Flags
    propertyDataFlags(const QV4::CompiledData::Property &propertyIR,
                      const PropertyType &resolvedPropertyType);

protected:
    QQmlTypeLoader *const typeLoader;
    const ObjectContainer * const objectContainer;
    const QQmlImports * const imports;
    QQmlPropertyCacheVector *propertyCaches;
    QQmlPendingGroupPropertyBindings *pendingGroupPropertyBindings;
    QByteArray typeClassName; // not const as we temporarily chang it for inline components
    unsigned int currentRoot; // set to objectID of inline component root when handling inline components

    QQmlBindingInstantiationContext m_context;
    std::vector<InlineComponent> allICs;
    std::vector<icutils::Node> nodesSorted;
    std::vector<icutils::Node>::reverse_iterator nodeIt = nodesSorted.rbegin();
    bool hasCycle = false;
};

template <typename ObjectContainer>
inline QQmlPropertyCacheCreator<ObjectContainer>::QQmlPropertyCacheCreator(
        QQmlPropertyCacheVector *propertyCaches,
        QQmlPendingGroupPropertyBindings *pendingGroupPropertyBindings,
        QQmlTypeLoader *typeLoader,
        const ObjectContainer *objectContainer, const QQmlImports *imports,
        const QByteArray &typeClassName)
    : typeLoader(typeLoader)
    , objectContainer(objectContainer)
    , imports(imports)
    , propertyCaches(propertyCaches)
    , pendingGroupPropertyBindings(pendingGroupPropertyBindings)
    , typeClassName(typeClassName)
    , currentRoot(-1)
{
    propertyCaches->resetAndResize(objectContainer->objectCount());

    using namespace icutils;

    // get a list of all inline components

    for (int i=0; i != objectContainer->objectCount(); ++i) {
        const CompiledObject *obj = objectContainer->objectAt(i);
        for (auto it = obj->inlineComponentsBegin(); it != obj->inlineComponentsEnd(); ++it) {
            allICs.push_back(*it);
        }
    }

    // create a graph on inline components referencing inline components
    std::vector<icutils::Node> nodes;
    nodes.resize(allICs.size());
    std::iota(nodes.begin(), nodes.end(), 0);
    AdjacencyList adjacencyList;
    adjacencyList.resize(nodes.size());
    fillAdjacencyListForInlineComponents(objectContainer, adjacencyList, nodes, allICs);

    nodesSorted = topoSort(nodes, adjacencyList, hasCycle);
    nodeIt = nodesSorted.rbegin();
}

template <typename ObjectContainer>
inline QQmlError QQmlPropertyCacheCreator<ObjectContainer>::verifyNoICCycle()
{
    if (hasCycle) {
        QQmlError diag;
        diag.setDescription(QLatin1String("Inline components form a cycle!"));
        return diag;
    }
    return {};
}

template <typename ObjectContainer>
inline  QQmlPropertyCacheCreatorBase::IncrementalResult
QQmlPropertyCacheCreator<ObjectContainer>::buildMetaObjectsIncrementally()
{
    // needs to be checked with verifyNoICCycle before this function is called
    Q_ASSERT(!hasCycle);

    // create meta objects for inline components before compiling actual root component
    if (nodeIt != nodesSorted.rend()) {
        const auto &ic = allICs[nodeIt->index()];
        QV4::ResolvedTypeReference *typeRef = objectContainer->resolvedType(ic.nameIndex);
        Q_ASSERT(propertyCaches->at(ic.objectIndex).isNull());
        Q_ASSERT(typeRef->typePropertyCache().isNull()); // not set yet

        QByteArray icTypeName { objectContainer->stringAt(ic.nameIndex).toUtf8() };
        QScopedValueRollback<QByteArray> nameChange {typeClassName, icTypeName};
        QScopedValueRollback<unsigned int> rootChange {currentRoot, ic.objectIndex};
        ++nodeIt;
        QQmlError diag = buildMetaObjectRecursively(ic.objectIndex, m_context, VMEMetaObjectIsRequired::Always);
        if (diag.isValid()) {
            return {diag, false, 0};
        }
        typeRef->setTypePropertyCache(propertyCaches->at(ic.objectIndex));
        Q_ASSERT(!typeRef->typePropertyCache().isNull());
        return { QQmlError(), true, int(ic.objectIndex) };
    }

    auto diag = buildMetaObjectRecursively(/*root object*/0, m_context, VMEMetaObjectIsRequired::Maybe);
    return {diag, false, 0};
}

template <typename ObjectContainer>
q23::expected<QQmlPropertyCache::Ptr, QQmlError>
QQmlPropertyCacheCreator<ObjectContainer>::tryDeriveCacheFrom(
        const CompiledObject *obj, const QQmlPropertyCache::ConstPtr &baseTypeCache,
        QByteArray dynamicClassName) const
{
    QQmlPropertyCache::Ptr cache = baseTypeCache->copyAndReserve(
            obj->propertyCount() + obj->aliasCount(),
            obj->functionCount() + obj->propertyCount() + obj->aliasCount() + obj->signalCount(),
            obj->signalCount() + obj->propertyCount() + obj->aliasCount(), obj->enumCount());
    cache->_dynamicClassName = std::move(dynamicClassName);

    int effectiveMethodIndex = cache->methodIndexCacheStart;

    // For property change signal override detection.
    // We prepopulate a set of signal names which already exist in the object,
    // and throw an error if there is a signal/method defined as an override.
    // TODO: Remove AllowOverride once we can. No override should be allowed.
    enum class AllowOverride { No, Yes };
    QHash<QString, AllowOverride> seenSignals{
        { QStringLiteral("destroyed"), AllowOverride::No },
        { QStringLiteral("parentChanged"), AllowOverride::No },
        { QStringLiteral("objectNameChanged"), AllowOverride::No }
    };
    const QQmlPropertyCache *parentCache = cache.data();
    while ((parentCache = parentCache->parent().data())) {
        if (int pSigCount = parentCache->signalCount()) {
            int pSigOffset = parentCache->signalOffset();
            for (int i = pSigOffset; i < pSigCount; ++i) {
                const QQmlPropertyData *currPSig = parentCache->signal(i);
                // XXX TODO: find a better way to get signal name from the property data :-/
                for (QQmlPropertyCache::StringCache::ConstIterator iter =
                             parentCache->stringCache.begin();
                     iter != parentCache->stringCache.end(); ++iter) {
                    if (currPSig == (*iter).second) {
                        if (currPSig->isOverridableSignal()) {
                            const qsizetype oldSize = seenSignals.size();
                            AllowOverride &entry = seenSignals[iter.key()];
                            if (seenSignals.size() != oldSize)
                                entry = AllowOverride::Yes;
                        } else {
                            seenSignals[iter.key()] = AllowOverride::No;
                        }

                        break;
                    }
                }
            }
        }
    }

    // Set up notify signals for properties - first normal, then alias
    auto p = obj->propertiesBegin();
    auto pend = obj->propertiesEnd();
    for (; p != pend; ++p) {
        auto flags = QQmlPropertyData::defaultSignalFlags();

        const QString changedSigName =
                QQmlSignalNames::propertyNameToChangedSignalName(stringAt(p->nameIndex()));
        seenSignals[changedSigName] = AllowOverride::No;

        cache->appendSignal(changedSigName, flags, effectiveMethodIndex++);
    }

    auto a = obj->aliasesBegin();
    auto aend = obj->aliasesEnd();
    for (; a != aend; ++a) {
        auto flags = QQmlPropertyData::defaultSignalFlags();

        const QString changedSigName =
                QQmlSignalNames::propertyNameToChangedSignalName(stringAt(a->nameIndex()));
        seenSignals[changedSigName] = AllowOverride::No;

        cache->appendSignal(changedSigName, flags, effectiveMethodIndex++);
    }

    auto e = obj->enumsBegin();
    auto eend = obj->enumsEnd();
    for (; e != eend; ++e) {
        const int enumValueCount = e->enumValueCount();
        QVector<QQmlEnumValue> values;
        values.reserve(enumValueCount);

        auto enumValue = e->enumValuesBegin();
        auto end = e->enumValuesEnd();
        for (; enumValue != end; ++enumValue)
            values.append(QQmlEnumValue(stringAt(enumValue->nameIndex), enumValue->value));

        cache->appendEnum(stringAt(e->nameIndex), values);
    }

    // Dynamic signals
    auto s = obj->signalsBegin();
    auto send = obj->signalsEnd();
    for (; s != send; ++s) {
        const int paramCount = s->parameterCount();

        QList<QByteArray> names;
        names.reserve(paramCount);
        QVarLengthArray<QMetaType, 10> paramTypes(paramCount);

        if (paramCount) {

            int i = 0;
            auto param = s->parametersBegin();
            auto end = s->parametersEnd();
            for (; param != end; ++param, ++i) {
                names.append(stringAt(param->nameIndex).toUtf8());

                QString customTypeName;
                QMetaType type = metaTypeForParameter(param->type, &customTypeName);
                if (!type.isValid())
                    return q23::make_unexpected(qQmlCompileError(
                            s->location,
                            QQmlPropertyCacheCreatorBase::tr("Invalid signal parameter type: %1")
                                    .arg(customTypeName)));

                paramTypes[i] = type;
            }
        }

        auto flags = QQmlPropertyData::defaultSignalFlags();
        if (paramCount)
            flags.setHasArguments(true);

        QString signalName = stringAt(s->nameIndex);
        const auto it = seenSignals.find(signalName);
        if (it == seenSignals.end()) {
            seenSignals[signalName] = AllowOverride::No;
        } else {
            // TODO: Remove the AllowOverride::Yes branch once we can.
            QQmlError message = qQmlCompileError(
                    s->location,
                    QQmlPropertyCacheCreatorBase::tr(
                            "Duplicate signal name: "
                            "invalid override of property change signal or superclass signal"));
            switch (*it) {
            case AllowOverride::No:
                return q23::make_unexpected(std::move(message));
            case AllowOverride::Yes:
                message.setUrl(objectContainer->url());
                qCWarning(invalidOverride).noquote() << message.toString();
                *it = AllowOverride::No; // No further overriding allowed.
                break;
            }
        }
        cache->appendSignal(signalName, flags, effectiveMethodIndex++,
                            paramCount ? paramTypes.constData() : nullptr, names);
    }

    // Dynamic slots
    auto function = objectContainer->objectFunctionsBegin(obj);
    auto fend = objectContainer->objectFunctionsEnd(obj);
    for (; function != fend; ++function) {
        auto flags = QQmlPropertyData::defaultSlotFlags();

        const QString slotName = stringAt(function->nameIndex);
        const auto it = seenSignals.constFind(slotName);
        if (it != seenSignals.constEnd()) {
            // TODO: Remove the AllowOverride::Yes branch once we can.
            QQmlError message = qQmlCompileError(
                    function->location,
                    QQmlPropertyCacheCreatorBase::tr(
                            "Duplicate method name: "
                            "invalid override of property change signal or superclass signal"));
            switch (*it) {
            case AllowOverride::No:
                return q23::make_unexpected(std::move(message));
            case AllowOverride::Yes:
                message.setUrl(objectContainer->url());
                qCWarning(invalidOverride).noquote() << message.toString();
                break;
            }
        }
        // Note: we don't append slotName to the seenSignals list, since we don't
        // protect against overriding change signals or methods with properties.

        QList<QByteArray> parameterNames;
        QVector<QMetaType> parameterTypes;
        auto formal = function->formalsBegin();
        auto end = function->formalsEnd();
        for (; formal != end; ++formal) {
            flags.setHasArguments(true);
            parameterNames << stringAt(formal->nameIndex).toUtf8();
            QMetaType type = metaTypeForParameter(formal->type);
            if (!type.isValid())
                type = QMetaType::fromType<QVariant>();
            parameterTypes << type;
        }

        QMetaType returnType = metaTypeForParameter(function->returnType);
        if (!returnType.isValid())
            returnType = QMetaType::fromType<QVariant>();

        cache->appendMethod(slotName, flags, effectiveMethodIndex++, returnType, parameterNames,
                            parameterTypes);
    }

    // Dynamic properties
    int propertyIndex = cache->propertyIndexCacheStart;
    int notifyIndex = cache->signalHandlerIndexCacheStart;
    int objPropertyIdx = 0;
    p = obj->propertiesBegin();
    pend = obj->propertiesEnd();
    for (; p != pend; ++p, ++objPropertyIdx, propertyIndex++, notifyIndex++) {
        auto propertyDataOrError = tryCreateQQmlPropertyData(*p, propertyIndex, notifyIndex);
        if (!propertyDataOrError.has_value()) {
            return q23::make_unexpected(propertyDataOrError.error());
        }

        QString propertyName = stringAt(p->nameIndex());
        if (!obj->hasAliasAsDefaultProperty()
            && objPropertyIdx == obj->indexOfDefaultPropertyOrAlias)
            cache->_defaultPropertyName = propertyName;

        const auto &appendResult =
                cache->appendPropertyAttr(propertyName, std::move(propertyDataOrError).value());
        if (!appendResult) {
            return q23::make_unexpected(
                    qQmlCompileError(p->location, explain(appendResult.error())));
        }
    }
    return cache;
};

template <typename ObjectContainer>
q23::expected<QQmlPropertyData, QQmlError>
QQmlPropertyCacheCreator<ObjectContainer>::tryCreateQQmlPropertyData(
        const QV4::CompiledData::Property &propertyIR, int coreIndex, int notifyIndex) const
{
    const auto propertyTypeOrError = tryResolvePropertyType(propertyIR);
    if (!propertyTypeOrError.has_value()) {
        return q23::make_unexpected(propertyTypeOrError.error());
    }
    const auto propertyType = propertyTypeOrError.value();

    QQmlPropertyData propertyData;
    propertyData.setPropType(propertyType.metaType);
    propertyData.setCoreIndex(coreIndex);
    propertyData.setNotifyIndex(notifyIndex);
    propertyData.setFlags(QQmlPropertyCacheCreator::propertyDataFlags(propertyIR, propertyType));
    propertyData.setTypeVersion(propertyType.revision);
    return propertyData;
}

template <typename ObjectContainer>
inline QQmlError QQmlPropertyCacheCreator<ObjectContainer>::buildMetaObjectRecursively(int objectIndex, const QQmlBindingInstantiationContext &context, VMEMetaObjectIsRequired isVMERequired)
{
    auto isAddressable = [](const QUrl &url) {
        const QString fileName = url.fileName();
        return !fileName.isEmpty() && fileName.front().isUpper();
    };

    const CompiledObject *obj = objectContainer->objectAt(objectIndex);
    bool needVMEMetaObject = isVMERequired == VMEMetaObjectIsRequired::Always
            || obj->propertyCount() != 0 || obj->aliasCount() != 0 || obj->signalCount() != 0
            || obj->functionCount() != 0 || obj->enumCount() != 0
            || obj->inlineComponentCount() != 0
            || ((obj->hasFlag(QV4::CompiledData::Object::IsComponent)
                 || (objectIndex == 0 && isAddressable(objectContainer->url())))
                && !objectContainer->resolvedType(obj->inheritedTypeNameIndex)->isFullyDynamicType());

    if (!needVMEMetaObject) {
        auto binding = obj->bindingsBegin();
        auto end = obj->bindingsEnd();
        for ( ; binding != end; ++binding) {
            if (binding->type() == QV4::CompiledData::Binding::Type_Object
                    && (binding->flags() & QV4::CompiledData::Binding::IsOnAssignment)) {
                // If the on assignment is inside a group property, we need to distinguish between QObject based
                // group properties and value type group properties. For the former the base type is derived from
                // the property that references us, for the latter we only need a meta-object on the referencing object
                // because interceptors can't go to the shared value type instances.
                if (context.instantiatingProperty && QQmlMetaType::isValueType(context.instantiatingProperty->propType())) {
                    if (!propertyCaches->needsVMEMetaObject(context.referencingObjectIndex)) {
                        const CompiledObject *obj = objectContainer->objectAt(context.referencingObjectIndex);
                        auto *typeRef = objectContainer->resolvedType(obj->inheritedTypeNameIndex);
                        Q_ASSERT(typeRef);
                        QQmlPropertyCache::ConstPtr baseTypeCache = typeRef->createPropertyCache();
                        QQmlError error = baseTypeCache
                            ? createMetaObject(context.referencingObjectIndex, obj, baseTypeCache)
                            : qQmlCompileError(binding->location, QQmlPropertyCacheCreatorBase::tr(
                                    "Type cannot be used for 'on' assignment"));
                        if (error.isValid())
                            return error;
                    }
                } else {
                    // On assignments are implemented using value interceptors, which require a VME meta object.
                    needVMEMetaObject = true;
                }
                break;
            }
        }
    }

    QQmlPropertyCache::ConstPtr baseTypeCache;
    {
        QQmlError error;
        baseTypeCache = propertyCacheForObject(obj, context, &error);
        if (error.isValid())
            return error;
    }

    if (baseTypeCache) {
        if (needVMEMetaObject) {
            QQmlError error = createMetaObject(objectIndex, obj, baseTypeCache);
            if (error.isValid())
                return error;
        } else {
            propertyCaches->set(objectIndex, baseTypeCache);
        }
    }

    QQmlPropertyCache::ConstPtr thisCache = propertyCaches->at(objectIndex);
    auto binding = obj->bindingsBegin();
    auto end = obj->bindingsEnd();
    for (; binding != end; ++binding) {
        switch (binding->type()) {
        case QV4::CompiledData::Binding::Type_Object:
        case QV4::CompiledData::Binding::Type_GroupProperty:
        case QV4::CompiledData::Binding::Type_AttachedProperty:
            // We can always resolve object, group, and attached properties.
            break;
        default:
            // Everything else is of no interest here.
            continue;
        }

        QQmlBindingInstantiationContext context(
                    objectIndex, &(*binding), stringAt(binding->propertyNameIndex), thisCache);

        // Binding to group property where we failed to look up the type of the
        // property? Possibly a group property that is an alias that's not resolved yet.
        // Let's attempt to resolve it after we're done with the aliases and fill in the
        // propertyCaches entry then.
        if (!thisCache || !context.resolveInstantiatingProperty())
            pendingGroupPropertyBindings->append(context);

        QQmlError error = buildMetaObjectRecursively(
                    binding->value.objectIndex, context, VMEMetaObjectIsRequired::Maybe);
        if (error.isValid())
            return error;
    }

    QQmlError noError;
    return noError;
}

template <typename ObjectContainer>
inline QQmlPropertyCache::ConstPtr QQmlPropertyCacheCreator<ObjectContainer>::propertyCacheForObject(const CompiledObject *obj, const QQmlBindingInstantiationContext &context, QQmlError *error) const
{
    if (context.instantiatingProperty) {
        return context.instantiatingPropertyCache();
    } else if (obj->inheritedTypeNameIndex != 0) {
        auto *typeRef = objectContainer->resolvedType(obj->inheritedTypeNameIndex);
        Q_ASSERT(typeRef);

        if (typeRef->isFullyDynamicType()) {
            if (obj->propertyCount() > 0 || obj->aliasCount() > 0) {
                *error = qQmlCompileError(obj->location, QQmlPropertyCacheCreatorBase::tr("Fully dynamic types cannot declare new properties."));
                return nullptr;
            }
            if (obj->signalCount() > 0) {
                *error = qQmlCompileError(obj->location, QQmlPropertyCacheCreatorBase::tr("Fully dynamic types cannot declare new signals."));
                return nullptr;
            }
            if (obj->functionCount() > 0) {
                *error = qQmlCompileError(obj->location, QQmlPropertyCacheCreatorBase::tr("Fully Dynamic types cannot declare new functions."));
                return nullptr;
            }
        }

        if (QQmlPropertyCache::ConstPtr propertyCache = typeRef->createPropertyCache())
            return propertyCache;
        *error = qQmlCompileError(
            obj->location,
            QQmlPropertyCacheCreatorBase::tr("Type '%1' cannot declare new members.")
                .arg(stringAt(obj->inheritedTypeNameIndex)));
        return nullptr;
    } else if (const QV4::CompiledData::Binding *binding = context.instantiatingBinding) {
        if (binding->isAttachedProperty()) {
            auto *typeRef = objectContainer->resolvedType(
                    binding->propertyNameIndex);
            Q_ASSERT(typeRef);
            QQmlType qmltype = typeRef->type();
            if (!qmltype.isValid()) {
                imports->resolveType(
                        typeLoader, stringAt(binding->propertyNameIndex),
                        &qmltype, nullptr, nullptr);
            }

            const QMetaObject *attachedMo = qmltype.attachedPropertiesType(typeLoader);
            if (!attachedMo) {
                *error = qQmlCompileError(binding->location, QQmlPropertyCacheCreatorBase::tr("Non-existent attached object"));
                return nullptr;
            }
            return QQmlMetaType::propertyCache(attachedMo);
        }
    }
    return nullptr;
}

template <typename ObjectContainer>
inline QQmlError QQmlPropertyCacheCreator<ObjectContainer>::createMetaObject(
        int objectIndex, const CompiledObject *obj,
        const QQmlPropertyCache::ConstPtr &baseTypeCache)
{
    const auto dynamicClassName = [this](int objectIndex, const auto &baseTypeCache) -> QByteArray {
        const auto isComponentRoot = objectIndex == 0 /*root object*/
                || objectIndex == int(currentRoot) /*root object of IC*/;
        if (isComponentRoot && !typeClassName.isEmpty()) {
            return typeClassName;
        }
        return QByteArray(QQmlMetaObject(baseTypeCache).className())
                .append("_QML_")
                .append(QByteArray::number(classIndexCounter.fetchAndAddRelaxed(1)));
    };

    const auto cacheOrError =
            tryDeriveCacheFrom(obj, baseTypeCache, dynamicClassName(objectIndex, baseTypeCache));
    if (!cacheOrError.has_value()) {
        return cacheOrError.error();
    }
    auto &cache = cacheOrError.value();

    using ListPropertyAssignBehavior = typename ObjectContainer::ListPropertyAssignBehavior;
    switch (objectContainer->listPropertyAssignBehavior()) {
    case ListPropertyAssignBehavior::ReplaceIfNotDefault:
        cache->_listPropertyAssignBehavior = "ReplaceIfNotDefault";
        break;
    case ListPropertyAssignBehavior::Replace:
        cache->_listPropertyAssignBehavior = "Replace";
        break;
    case ListPropertyAssignBehavior::Append:
        break;
    }

    propertyCaches->setOwn(objectIndex, cache);
    propertyCaches->setNeedsVMEMetaObject(objectIndex);
    return QQmlError();
}

template <typename ObjectContainer>
inline QMetaType QQmlPropertyCacheCreator<ObjectContainer>::metaTypeForParameter(
        const QV4::CompiledData::ParameterType &param, QString *customTypeName) const
{
    const quint32 typeId = param.typeNameIndexOrCommonType();
    if (param.indexIsCommonType()) {
        // built-in type
        if (param.isList())
            return listTypeForPropertyType(QV4::CompiledData::CommonType(typeId));
        return metaTypeForPropertyType(QV4::CompiledData::CommonType(typeId));
    }

    // lazily resolved type
    const QString typeName = stringAt(param.typeNameIndexOrCommonType());
    if (customTypeName)
        *customTypeName = typeName;
    QQmlType qmltype;
    bool selfReference = false;
    if (!imports->resolveType(
                typeLoader, typeName, &qmltype, nullptr, nullptr, nullptr,
                QQmlType::AnyRegistrationType, &selfReference))
        return QMetaType();

    if (!qmltype.isComposite()) {
        const QMetaType typeId = param.isList() ? qmltype.qListTypeId() : qmltype.typeId();
        if (!typeId.isValid() && qmltype.isInlineComponentType()) {
            const QQmlType qmlType = objectContainer->qmlTypeForComponent(qmltype.elementName());
            return param.isList() ? qmlType.qListTypeId() : qmlType.typeId();
        } else {
            return typeId;
        }
    }

    if (selfReference) {
        const QQmlType qmlType = objectContainer->qmlTypeForComponent();
        return param.isList() ? qmlType.qListTypeId() : qmlType.typeId();
    }

    return param.isList() ? qmltype.qListTypeId() : qmltype.typeId();
}

template <typename ObjectContainer>
inline auto QQmlPropertyCacheCreator<ObjectContainer>::tryResolvePropertyType(
        const QV4::CompiledData::Property &propertyIR) const
        -> q23::expected<PropertyType, QQmlError>
{
    using namespace Qt::StringLiterals;
    PropertyType propertyType;
    propertyType.commonType = propertyIR.commonType();

    if (propertyType.commonType != QV4::CompiledData::CommonType::Invalid) {
        // common type
        propertyType.metaType = propertyIR.isList()
                ? listTypeForPropertyType(propertyType.commonType)
                : metaTypeForPropertyType(propertyType.commonType);
        return propertyType;
    }

    // needs to be resolved
    // can be extracted and passed to tryResolvePropertyType as a function object to simplify
    // dependency injection (imports / typeLoader)
    Q_ASSERT(!propertyIR.isCommonType());
    QQmlType qmltype;
    bool selfReference = false;
    QList<QQmlError> errors;
    const QString typeName = stringAt(propertyIR.commonTypeOrTypeNameIndex());
    if (!imports->resolveType(typeLoader, typeName, &qmltype, nullptr, nullptr, &errors,
                              QQmlType::AnyRegistrationType, &selfReference)) {
        Q_ASSERT(!errors.isEmpty());

        if (typeName == "list"_L1 && errors[0].description() == "is not a type"_L1) {
            errors[0].setDescription(errors[0].description().append(
                    ". It requires an element type argument (eg. list<int>)"_L1));
        }

        return q23::make_unexpected(
                qQmlCompileError(propertyIR.location, typeName + u' ' + errors[0].description()));
    }

    Q_ASSERT(qmltype.isValid());

    propertyType.metaType = propertyIR.isList() ? qmltype.qListTypeId() : qmltype.typeId();

    if (!qmltype.isComposite() && !qmltype.isInlineComponentType())
        propertyType.revision = qmltype.version();

    return propertyType;
}

template <typename ObjectContainer>
inline QQmlPropertyData::Flags QQmlPropertyCacheCreator<ObjectContainer>::propertyDataFlags(
        const QV4::CompiledData::Property &propertyIR, const PropertyType &resolvedPropertyType)
{
    QQmlPropertyData::Flags flags;

    // set type flag
    if (propertyIR.isList()) {
        flags.setType(QQmlPropertyData::Flags::QListType);
    } else if (resolvedPropertyType.commonType == QV4::CompiledData::CommonType::Var) {
        flags.setType(QQmlPropertyData::Flags::VarPropertyType);
    } else if (resolvedPropertyType.metaType.flags().testFlag(QMetaType::PointerToQObject)) {
        flags.setType(QQmlPropertyData::Flags::QObjectDerivedType);
    }

    // set attributes
    if (!propertyIR.isReadOnly()
        && !resolvedPropertyType.metaType.flags().testFlag(QMetaType::IsQmlList))
        flags.setIsWritable(true);
    if (propertyIR.isFinal())
        flags.setIsFinal(true);
    if (propertyIR.isVirtual())
        flags.setIsVirtual(true);
    if (propertyIR.isOverride())
        flags.setDoesOverride(true);

    return flags;
}

template <typename ObjectContainer, typename CompiledObject>
int objectForId(const ObjectContainer *objectContainer, const CompiledObject &component, int id)
{
    for (quint32 i = 0, count = component.namedObjectsInComponentCount(); i < count; ++i) {
        const int candidateIndex = component.namedObjectsInComponentTable()[i];
        const CompiledObject &candidate = *objectContainer->objectAt(candidateIndex);
        if (candidate.objectId() == id)
            return candidateIndex;
    }
    return -1;
}

template <typename ObjectContainer>
class QQmlPropertyCacheAliasCreator
{
public:
    typedef typename ObjectContainer::CompiledObject CompiledObject;

    QQmlPropertyCacheAliasCreator(
            QQmlPropertyCacheVector *propertyCaches, const ObjectContainer *objectContainer);
    QQmlError appendAliasToPropertyCache(
            const CompiledObject &component, const QV4::CompiledData::Alias &alias, int objectIndex,
            int aliasIndex, int encodedMetaPropertyIndex);

private:
    QQmlError propertyDataForAlias(
            const CompiledObject &component, const QV4::CompiledData::Alias &alias, QMetaType *type,
            QTypeRevision *version, QQmlPropertyData::Flags *propertyFlags,
            int targetPropertyIndex);

    QQmlPropertyCacheVector *propertyCaches;
    const ObjectContainer *objectContainer;
};

template <typename ObjectContainer>
inline QQmlPropertyCacheAliasCreator<ObjectContainer>::QQmlPropertyCacheAliasCreator(
        QQmlPropertyCacheVector *propertyCaches, const ObjectContainer *objectContainer)
    : propertyCaches(propertyCaches)
    , objectContainer(objectContainer)
{
}

template <typename ObjectContainer>
inline QQmlError QQmlPropertyCacheAliasCreator<ObjectContainer>::propertyDataForAlias(
        const CompiledObject &component, const QV4::CompiledData::Alias &alias, QMetaType *type,
        QTypeRevision *version, QQmlPropertyData::Flags *propertyFlags, int targetPropertyIndex)
{
    *type = QMetaType();
    bool writable = false;
    bool resettable = false;
    bool notifiesViaBindable = false;

    propertyFlags->setIsAlias(true);

    if (alias.isAliasToLocalAlias()) {
        const QV4::CompiledData::Alias *lastAlias = &alias;
        QVarLengthArray<const QV4::CompiledData::Alias *, 4> seenAliases({lastAlias});

        do {
            const int targetObjectIndex = objectForId(
                    objectContainer, component, lastAlias->targetObjectId());
            Q_ASSERT(targetObjectIndex >= 0);
            const CompiledObject *targetObject = objectContainer->objectAt(targetObjectIndex);
            Q_ASSERT(targetObject);

            auto nextAlias = targetObject->aliasesBegin();
            for (uint i = 0; i < lastAlias->localAliasIndex; ++i)
                ++nextAlias;

            const QV4::CompiledData::Alias *targetAlias = &(*nextAlias);
            if (seenAliases.contains(targetAlias)) {
                return qQmlCompileError(targetAlias->location,
                                        QQmlPropertyCacheCreatorBase::tr("Cyclic alias"));
            }

            seenAliases.append(targetAlias);
            lastAlias = targetAlias;
        } while (lastAlias->isAliasToLocalAlias());

        return propertyDataForAlias(
                component, *lastAlias, type, version, propertyFlags, targetPropertyIndex);
    }

    const int targetObjectIndex = objectForId(objectContainer, component, alias.targetObjectId());
    Q_ASSERT(targetObjectIndex >= 0);
    const CompiledObject &targetObject = *objectContainer->objectAt(targetObjectIndex);

    if (targetPropertyIndex == -1) {
        Q_ASSERT(alias.hasFlag(QV4::CompiledData::Alias::AliasPointsToPointerObject));
        auto *typeRef = objectContainer->resolvedType(targetObject.inheritedTypeNameIndex);
        if (!typeRef) {
            // Can be caused by the alias target not being a valid id or property. E.g.:
            // property alias dataValue: dataVal
            // invalidAliasComponent { id: dataVal }
            return qQmlCompileError(targetObject.location,
                                    QQmlPropertyCacheCreatorBase::tr("Invalid alias target"));
        }

        const auto referencedType = typeRef->type();
        Q_ASSERT(referencedType.isValid());
        *type = referencedType.typeId();
        if (!type->isValid() && referencedType.isInlineComponentType()) {
            *type = objectContainer->qmlTypeForComponent(referencedType.elementName()).typeId();
            Q_ASSERT(type->isValid());
        }

        *version = typeRef->version();

        propertyFlags->setType(QQmlPropertyData::Flags::QObjectDerivedType);
    } else {
        int coreIndex = QQmlPropertyIndex::fromEncoded(targetPropertyIndex).coreIndex();
        int valueTypeIndex
                = QQmlPropertyIndex::fromEncoded(targetPropertyIndex).valueTypeIndex();

        QQmlPropertyCache::ConstPtr targetCache = propertyCaches->at(targetObjectIndex);
        Q_ASSERT(targetCache);

        const QQmlPropertyData *targetProperty = targetCache->property(coreIndex);
        Q_ASSERT(targetProperty);

        const QMetaType targetPropType = targetProperty->propType();

        const auto populateWithPropertyData = [&](const QQmlPropertyData *property) {
            *type = property->propType();
            writable = property->isWritable();
            resettable = property->isResettable();
            notifiesViaBindable = property->notifiesViaBindable();

            if (property->isVarProperty())
                propertyFlags->setType(QQmlPropertyData::Flags::QVariantType);
            else
                propertyFlags->copyPropertyTypeFlags(property->flags());
        };

        // for deep aliases, valueTypeIndex is always set
        if (!QQmlMetaType::isValueType(targetPropType) && valueTypeIndex != -1) {
            // deep alias property

            QQmlPropertyCache::ConstPtr typeCache
                    = QQmlMetaType::propertyCacheForType(targetPropType);

            if (!typeCache) {
                // See if it's a half-resolved composite type
                if (const QV4::ResolvedTypeReference *typeRef
                        = objectContainer->resolvedType(targetPropType)) {
                    typeCache = typeRef->typePropertyCache();
                }
            }

            const QQmlPropertyData *typeProperty = typeCache
                    ? typeCache->property(valueTypeIndex)
                    : nullptr;
            if (typeProperty == nullptr) {
                return qQmlCompileError(
                        alias.referenceLocation,
                        QQmlPropertyCacheCreatorBase::tr("Invalid alias target"));
            }
            populateWithPropertyData(typeProperty);
        } else {
            // value type or primitive type or enum
            populateWithPropertyData(targetProperty);

            if (valueTypeIndex != -1) {
                const QMetaObject *valueTypeMetaObject
                        = QQmlMetaType::metaObjectForValueType(*type);
                const QMetaProperty valueTypeMetaProperty
                        = valueTypeMetaObject->property(valueTypeIndex);
                *type = valueTypeMetaProperty.metaType();

                // We can only write or reset the value type property if we can write
                // the value type itself.
                resettable = writable && valueTypeMetaProperty.isResettable();
                writable = writable && valueTypeMetaProperty.isWritable();

                // Do not update notifiesViaBindable. The core property counts for notifications.
                propertyFlags->setIsDeepAlias(true);
            }
        }
    }

    propertyFlags->setIsWritable(
            writable && !alias.hasFlag(QV4::CompiledData::Alias::IsReadOnly));
    propertyFlags->setIsResettable(resettable);
    propertyFlags->setIsBindable(notifiesViaBindable);
    return QQmlError();
}

template <typename ObjectContainer>
inline QQmlError QQmlPropertyCacheAliasCreator<ObjectContainer>::appendAliasToPropertyCache(
        const CompiledObject &component, const QV4::CompiledData::Alias &alias, int objectIndex,
        int aliasIndex, int encodedMetaPropertyIndex)
{
    const CompiledObject &object = *objectContainer->objectAt(objectIndex);

    Q_ASSERT(object.aliasCount() > aliasIndex);
    QMetaType type;
    QTypeRevision version = QTypeRevision::zero();
    QQmlPropertyData::Flags propertyFlags;
    QQmlError error = propertyDataForAlias(
            component, alias, &type, &version, &propertyFlags, encodedMetaPropertyIndex);
    if (error.isValid())
        return error;

    const QString propertyName = objectContainer->stringAt(alias.nameIndex());

    const QQmlPropertyCache::Ptr propertyCache = propertyCaches->ownAt(objectIndex);
    Q_ASSERT(propertyCache);

    const int effectiveSignalIndex
            = propertyCache->signalHandlerIndexCacheStart + propertyCache->propertyIndexCache.size();
    const int effectivePropertyIndex
            = propertyCache->propertyIndexCacheStart + propertyCache->propertyIndexCache.size();

    if (object.hasAliasAsDefaultProperty() && aliasIndex == object.indexOfDefaultPropertyOrAlias)
        propertyCache->_defaultPropertyName = propertyName;

    const auto &appendResult =
            propertyCache->appendAlias(propertyName, propertyFlags, effectivePropertyIndex, type,
                                       version, effectiveSignalIndex, encodedMetaPropertyIndex);
    if (!appendResult) {
        return qQmlCompileError(alias.location,
                                QQmlPropertyCacheCreatorBase::explain(appendResult.error()));
    }
    return QQmlError();
}

QT_END_NAMESPACE

#endif // QQMLPROPERTYCACHECREATOR_P_H
