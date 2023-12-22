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

#include <private/qv4compileddata_p.h>
#include <private/qv4identifierhash_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qintrusivelist_p.h>
#include <private/qqmlpropertycachevector_p.h>
#include <private/qqmltype_p.h>
#include <private/qqmlnullablevalue_p.h>
#include <private/qqmlmetatype_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QQmlScriptData;
class QQmlEnginePrivate;

struct InlineComponentData {

    InlineComponentData() = default;
    InlineComponentData(
            const QQmlType &qmlType, int objectIndex, int nameIndex, int totalObjectCount,
            int totalBindingCount, int totalParserStatusCount)
        : qmlType(qmlType)
        , objectIndex(objectIndex)
        , nameIndex(nameIndex)
        , totalObjectCount(totalObjectCount)
        , totalBindingCount(totalBindingCount)
        , totalParserStatusCount(totalParserStatusCount) {}

    QQmlType qmlType;
    int objectIndex = -1;
    int nameIndex = -1;
    int totalObjectCount = 0;
    int totalBindingCount = 0;
    int totalParserStatusCount = 0;
};

namespace QV4 {

// index is per-object binding index
typedef QVector<const QQmlPropertyData *> BindingPropertyData;

class CompilationUnitMapper;
class ResolvedTypeReference;
// map from name index
struct ResolvedTypeReferenceMap: public QHash<int, ResolvedTypeReference*>
{
    bool addToHash(QCryptographicHash *hash, QHash<quintptr, QByteArray> *checksums) const;
};

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

class Q_QML_PRIVATE_EXPORT ExecutableCompilationUnit final
    : public CompilationUnitRuntimeData,
      public QQmlRefCounted<ExecutableCompilationUnit>
{
    Q_DISABLE_COPY_MOVE(ExecutableCompilationUnit)
public:
    friend class QQmlRefCounted<ExecutableCompilationUnit>;
    friend class QQmlRefPointer<ExecutableCompilationUnit>;

    static QQmlRefPointer<ExecutableCompilationUnit> create(
            QQmlRefPointer<CompiledData::CompilationUnit> &&compilationUnit,
            ExecutionEngine *engine);

    QIntrusiveListNode nextCompilationUnit;
    ExecutionEngine *engine = nullptr;

    QString finalUrlString() const { return m_compilationUnit->finalUrlString(); }
    QString fileName() const { return m_compilationUnit->fileName(); }

    QUrl url() const { return m_compilationUnit->url(); }
    QUrl finalUrl() const { return m_compilationUnit->finalUrl(); }

    // QML specific fields
    QQmlPropertyCacheVector propertyCaches;
    QQmlPropertyCache::ConstPtr rootPropertyCache() const { return propertyCaches.at(/*root object*/0); }

    QQmlRefPointer<QQmlTypeNameCache> typeNameCache;

    // index is object index. This allows fast access to the
    // property data when initializing bindings, avoiding expensive
    // lookups by string (property name).
    QVector<BindingPropertyData> bindingPropertyDataPerObject;

    // mapping from component object index (CompiledData::Unit object index that points to component) to identifier hash of named objects
    // this is initialized on-demand by QQmlContextData
    QHash<int, IdentifierHash> namedObjectsPerComponentCache;
    inline IdentifierHash namedObjectsPerComponent(int componentObjectIndex);

    void finalizeCompositeType(const QQmlType &type);

    int m_totalBindingsCount = 0; // Number of bindings used in this type
    int m_totalParserStatusCount = 0; // Number of instantiated types that are QQmlParserStatus subclasses
    int m_totalObjectCount = 0; // Number of objects explicitly instantiated
    std::unique_ptr<QString> icRootName;

    int totalBindingsCount() const;
    int totalParserStatusCount() const;
    int totalObjectCount() const;

    QVector<QQmlRefPointer<QQmlScriptData>> dependentScripts;
    ResolvedTypeReferenceMap resolvedTypes;
    ResolvedTypeReference *resolvedType(int id) const { return resolvedTypes.value(id); }
    ResolvedTypeReference *resolvedType(QMetaType type) const;

    bool verifyChecksum(const CompiledData::DependentTypesHasher &dependencyHasher) const;

    QQmlType qmlTypeForComponent(const QString &inlineComponentName = QString()) const;

    QQmlType qmlType;

    QHash<QString, InlineComponentData> inlineComponentData;

    int inlineComponentId(const QString &inlineComponentName) const
    {
        for (int i = 0; i < objectCount(); ++i) {
            auto *object = objectAt(i);
            for (auto it = object->inlineComponentsBegin(), end = object->inlineComponentsEnd();
                 it != end; ++it) {
                if (stringAt(it->nameIndex) == inlineComponentName)
                    return it->objectIndex;
            }
        }
        return -1;
    }

    // --- interface for QQmlPropertyCacheCreator
    using CompiledObject = const CompiledData::Object;
    using CompiledFunction = const CompiledData::Function;
    using CompiledBinding = const CompiledData::Binding;
    enum class ListPropertyAssignBehavior { Append, Replace, ReplaceIfNotDefault };

    // Empty dummy. We don't need to do this when loading from cache.
    class IdToObjectMap
    {
    public:
        void insert(int, int) {}
        void clear() {}

        // We have already checked uniqueness of IDs when creating the CU
        bool contains(int) { return false; }
    };

    ListPropertyAssignBehavior listPropertyAssignBehavior() const
    {
        if (unitData()->flags & CompiledData::Unit::ListPropertyAssignReplace)
            return ListPropertyAssignBehavior::Replace;
        if (unitData()->flags & CompiledData::Unit::ListPropertyAssignReplaceIfNotDefault)
            return ListPropertyAssignBehavior::ReplaceIfNotDefault;
        return ListPropertyAssignBehavior::Append;
    }

    bool ignoresFunctionSignature() const
    {
        return unitData()->flags & CompiledData::Unit::FunctionSignaturesIgnored;
    }

    bool nativeMethodsAcceptThisObjects() const
    {
        return unitData()->flags & CompiledData::Unit::NativeMethodsAcceptThisObject;
    }

    bool valueTypesAreCopied() const
    {
        return unitData()->flags & CompiledData::Unit::ValueTypesCopied;
    }

    bool valueTypesAreAddressable() const
    {
        return unitData()->flags & CompiledData::Unit::ValueTypesAddressable;
    }

    bool componentsAreBound() const
    {
        return unitData()->flags & CompiledData::Unit::ComponentsBound;
    }

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

    bool isESModule() const
    {
        return unitData()->flags & CompiledData::Unit::IsESModule;
    }

    bool isSharedLibrary() const
    {
        return unitData()->flags & CompiledData::Unit::IsSharedLibrary;
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

    void markObjects(MarkStack *markStack);

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

    void populate();
    void clear();

protected:
    quint32 totalStringCount() const
    { return unitData()->stringTableSize; }

private:
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
