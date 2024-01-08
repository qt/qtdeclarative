// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4EXECUTABLECOMPILATIONUNIT_P_H
#define QV4EXECUTABLECOMPILATIONUNIT_P_H

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

#include <private/qintrusivelist_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmlnullablevalue_p.h>
#include <private/qqmlpropertycachevector_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qqmltype_p.h>
#include <private/qqmltypenamecache_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qv4identifierhash_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QQmlScriptData;
class QQmlEnginePrivate;

namespace QV4 {

class CompilationUnitMapper;

struct CompilationUnitRuntimeData
{
    Heap::String **runtimeStrings = nullptr; // Array

    // pointers either to data->constants() or little-endian memory copy.
    // We keep this member twice so that the JIT can access it via standard layout.
    const StaticValue *constants = nullptr;

    QV4::StaticValue *runtimeRegularExpressions = nullptr;
    Heap::InternalClass **runtimeClasses = nullptr;
    const StaticValue **imports = nullptr;

    QV4::Lookup *runtimeLookups = nullptr;
    QVector<QV4::Function *> runtimeFunctions;
    QVector<QV4::Heap::InternalClass *> runtimeBlocks;
    mutable QVector<QV4::Heap::Object *> templateObjects;
};

static_assert(std::is_standard_layout_v<CompilationUnitRuntimeData>);
static_assert(offsetof(CompilationUnitRuntimeData, runtimeStrings) == 0);
static_assert(offsetof(CompilationUnitRuntimeData, constants) == sizeof(QV4::Heap::String **));
static_assert(offsetof(CompilationUnitRuntimeData, runtimeRegularExpressions) == offsetof(CompilationUnitRuntimeData, constants) + sizeof(const StaticValue *));
static_assert(offsetof(CompilationUnitRuntimeData, runtimeClasses) == offsetof(CompilationUnitRuntimeData, runtimeRegularExpressions) + sizeof(const StaticValue *));
static_assert(offsetof(CompilationUnitRuntimeData, imports) == offsetof(CompilationUnitRuntimeData, runtimeClasses) + sizeof(const StaticValue *));

class Q_QML_EXPORT ExecutableCompilationUnit final
    : public CompilationUnitRuntimeData,
      public QQmlRefCounted<ExecutableCompilationUnit>
{
    Q_DISABLE_COPY_MOVE(ExecutableCompilationUnit)
public:
    friend class QQmlRefCounted<ExecutableCompilationUnit>;
    friend class QQmlRefPointer<ExecutableCompilationUnit>;

    QIntrusiveListNode nextCompilationUnit;
    ExecutionEngine *engine = nullptr;

    QString finalUrlString() const { return m_compilationUnit->finalUrlString(); }
    QString fileName() const { return m_compilationUnit->fileName(); }

    QUrl url() const { return m_compilationUnit->url(); }
    QUrl finalUrl() const { return m_compilationUnit->finalUrl(); }

    QQmlRefPointer<QQmlTypeNameCache> typeNameCache() const
    {
        return m_compilationUnit->typeNameCache;
    }

    void setTypeNameCache(const QQmlRefPointer<QQmlTypeNameCache> &typeNameCache)
    {
        m_compilationUnit->typeNameCache = typeNameCache;
    }

    const QQmlPropertyCacheVector *propertyCachesPtr() const
    {
        return &m_compilationUnit->propertyCaches;
    }

    QQmlPropertyCacheVector *propertyCachesPtr()
    {
        return &m_compilationUnit->propertyCaches;
    }

    QQmlPropertyCache::ConstPtr rootPropertyCache() const
    {
        return m_compilationUnit->rootPropertyCache();
    }

    // mapping from component object index (CompiledData::Unit object index that points to component) to identifier hash of named objects
    // this is initialized on-demand by QQmlContextData
    QHash<int, IdentifierHash> namedObjectsPerComponentCache;
    inline IdentifierHash namedObjectsPerComponent(int componentObjectIndex);

    void finalizeCompositeType(const QQmlType &type);

    const QString *icRootName() const { return m_compilationUnit->icRootName.get(); }
    QString *icRootName() { return m_compilationUnit->icRootName.get(); }
    void setIcRootName(std::unique_ptr<QString> &&icRootName)
    {
        m_compilationUnit->icRootName = std::move(icRootName);
    }

    int totalBindingsCount() const { return m_compilationUnit->totalBindingsCount(); }
    int totalParserStatusCount() const { return m_compilationUnit->totalParserStatusCount(); }
    int totalObjectCount() const { return m_compilationUnit->totalObjectCount(); }
    QVector<QQmlRefPointer<QQmlScriptData>> dependentScripts;

    ResolvedTypeReference *resolvedType(int id) const
    {
        return m_compilationUnit->resolvedType(id);
    }

    ResolvedTypeReference *resolvedType(QMetaType type) const
    {
        return m_compilationUnit->resolvedType(type);
    }

    void setResolvedTypes(const CompiledData::ResolvedTypeReferenceMap &resolvedTypes)
    {
        m_compilationUnit->resolvedTypes = resolvedTypes;
    }

    bool verifyChecksum(const CompiledData::DependentTypesHasher &dependencyHasher) const
    {
        return m_compilationUnit->verifyChecksum(dependencyHasher);
    }

    QQmlType qmlTypeForComponent(const QString &inlineComponentName = QString()) const
    {
        return m_compilationUnit->qmlTypeForComponent(inlineComponentName);
    }

    QQmlType qmlType() const { return m_compilationUnit->qmlType; }

    QMetaType metaType() const { return qmlType().typeId(); }

    int inlineComponentId(const QString &inlineComponentName) const
    {
        return m_compilationUnit->inlineComponentId(inlineComponentName);
    }

    // --- interface for QQmlPropertyCacheCreator
    using CompiledObject = const CompiledData::Object;
    using CompiledFunction = const CompiledData::Function;
    using CompiledBinding = const CompiledData::Binding;

    // Empty dummy. We don't need to do this when loading from cache.
    class IdToObjectMap
    {
    public:
        void insert(int, int) {}
        void clear() {}

        // We have already checked uniqueness of IDs when creating the CU
        bool contains(int) { return false; }
    };

    using ListPropertyAssignBehavior = CompiledData::CompilationUnit::ListPropertyAssignBehavior;
    ListPropertyAssignBehavior listPropertyAssignBehavior() const
    {
        return m_compilationUnit->listPropertyAssignBehavior();
    }

    bool nativeMethodsAcceptThisObjects() const
    {
        return m_compilationUnit->nativeMethodsAcceptThisObjects();
    }

    bool ignoresFunctionSignature() const { return m_compilationUnit->ignoresFunctionSignature(); }
    bool valueTypesAreCopied() const { return m_compilationUnit->valueTypesAreCopied(); }
    bool valueTypesAreAddressable() const { return m_compilationUnit->valueTypesAreAddressable(); }
    bool componentsAreBound() const { return m_compilationUnit->componentsAreBound(); }
    bool isESModule() const { return m_compilationUnit->isESModule(); }
    bool isSharedLibrary() const { return m_compilationUnit->isSharedLibrary(); }

    int objectCount() const { return qmlData()->nObjects; }
    const CompiledObject *objectAt(int index) const
    {
        return qmlData()->objectAt(index);
    }

    int importCount() const { return m_compilationUnit->importCount(); }
    const CompiledData::Import *importAt(int index) const
    {
        return m_compilationUnit->importAt(index);
    }

    Heap::Object *templateObjectAt(int index) const;

    struct FunctionIterator
    {
        FunctionIterator(const CompiledData::Unit *unit, const CompiledObject *object, int index)
            : unit(unit), object(object), index(index) {}
        const CompiledData::Unit *unit;
        const CompiledObject *object;
        int index;

        const CompiledFunction *operator->() const
        {
            return unit->functionAt(object->functionOffsetTable()[index]);
        }

        void operator++() { ++index; }
        bool operator==(const FunctionIterator &rhs) const { return index == rhs.index; }
        bool operator!=(const FunctionIterator &rhs) const { return index != rhs.index; }
    };

    FunctionIterator objectFunctionsBegin(const CompiledObject *object) const
    {
        return FunctionIterator(unitData(), object, 0);
    }

    FunctionIterator objectFunctionsEnd(const CompiledObject *object) const
    {
        return FunctionIterator(unitData(), object, object->nFunctions);
    }

    Heap::Module *instantiate();
    const Value *resolveExport(QV4::String *exportName)
    {
        QVector<ResolveSetEntry> resolveSet;
        return resolveExportRecursively(exportName, &resolveSet);
    }

    QStringList exportedNames() const
    {
        QStringList names;
        QVector<const ExecutableCompilationUnit*> exportNameSet;
        getExportedNamesRecursively(&names, &exportNameSet);
        names.sort();
        auto last = std::unique(names.begin(), names.end());
        names.erase(last, names.end());
        return names;
    }

    void evaluate();
    void evaluateModuleRequests();

    void markObjects(MarkStack *markStack) const;

    QString bindingValueAsString(const CompiledData::Binding *binding) const;
    double bindingValueAsNumber(const CompiledData::Binding *binding) const
    {
        return m_compilationUnit->bindingValueAsNumber(binding);
    }
    QString bindingValueAsScriptString(const CompiledData::Binding *binding) const
    {
        return m_compilationUnit->bindingValueAsScriptString(binding);
    }

    struct TranslationDataIndex
    {
        uint index;
        bool byId;
    };

    QString translateFrom(TranslationDataIndex index) const;

    static bool verifyHeader(const CompiledData::Unit *unit, QDateTime expectedSourceTimeStamp,
                             QString *errorString);

    Heap::Module *module() const { return m_module; }
    void setModule(Heap::Module *module) { m_module = module; }

    const CompiledData::Unit *unitData() const { return m_compilationUnit->data; }
    const CompiledData::QmlUnit *qmlData() const { return m_compilationUnit->qmlData; }

    QString stringAt(uint index) const { return m_compilationUnit->stringAt(index); }

    const CompiledData::BindingPropertyData *bindingPropertyDataPerObjectAt(
            qsizetype objectIndex) const
    {
        return &m_compilationUnit->bindingPropertyDataPerObject.at(objectIndex);
    }

    QQmlRefPointer<QV4::CompiledData::CompilationUnit> baseCompilationUnit() const
    {
        return m_compilationUnit;
    }

    QV4::Function *rootFunction()
    {
        if (!runtimeStrings)
            populate();

        const auto *data = unitData();
        return data->indexOfRootFunction != -1
                ? runtimeFunctions[data->indexOfRootFunction]
                : nullptr;
    }

    const QHash<QString, CompiledData::InlineComponentData> &inlineComponentData() const
    {
        return m_compilationUnit->inlineComponentData;
    }

    void setInlineComponentData(
            const QHash<QString, CompiledData::InlineComponentData> &inlineComponentData)
    {
        m_compilationUnit->inlineComponentData = inlineComponentData;
    }

    void populate();
    void clear();

protected:
    quint32 totalStringCount() const
    { return unitData()->stringTableSize; }

private:
    friend struct ExecutionEngine;

    QQmlRefPointer<CompiledData::CompilationUnit> m_compilationUnit;
    Heap::Module *m_module = nullptr;

    struct ResolveSetEntry
    {
        ResolveSetEntry() {}
        ResolveSetEntry(ExecutableCompilationUnit *module, QV4::String *exportName)
            : module(module), exportName(exportName) {}
        ExecutableCompilationUnit *module = nullptr;
        QV4::String *exportName = nullptr;
    };

    ExecutableCompilationUnit();
    ExecutableCompilationUnit(QQmlRefPointer<CompiledData::CompilationUnit> &&compilationUnit);
    ~ExecutableCompilationUnit();

    static QQmlRefPointer<ExecutableCompilationUnit> create(
            QQmlRefPointer<CompiledData::CompilationUnit> &&compilationUnit,
            ExecutionEngine *engine);

    const Value *resolveExportRecursively(QV4::String *exportName,
                                          QVector<ResolveSetEntry> *resolveSet);

    QUrl urlAt(int index) const { return QUrl(stringAt(index)); }

    Q_NEVER_INLINE IdentifierHash createNamedObjectsPerComponent(int componentObjectIndex);
    const CompiledData::ExportEntry *lookupNameInExportTable(
            const CompiledData::ExportEntry *firstExportEntry, int tableSize,
            QV4::String *name) const;

    void getExportedNamesRecursively(
            QStringList *names, QVector<const ExecutableCompilationUnit *> *exportNameSet,
            bool includeDefaultExport = true) const;
};

IdentifierHash ExecutableCompilationUnit::namedObjectsPerComponent(int componentObjectIndex)
{
    auto it = namedObjectsPerComponentCache.find(componentObjectIndex);
    if (Q_UNLIKELY(it == namedObjectsPerComponentCache.end()))
        return createNamedObjectsPerComponent(componentObjectIndex);
    Q_ASSERT(!it->isEmpty());
    return *it;
}

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4EXECUTABLECOMPILATIONUNIT_P_H
