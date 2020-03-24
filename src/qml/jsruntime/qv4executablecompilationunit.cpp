/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "qv4executablecompilationunit_p.h"

#include <private/qv4engine_p.h>
#include <private/qv4regexp_p.h>
#include <private/qv4lookup_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qv4identifiertable_p.h>
#include <private/qv4objectproto_p.h>
#include <private/qqmlengine_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qqmlvaluetypewrapper_p.h>
#include <private/qqmlscriptdata_p.h>
#include <private/qv4module_p.h>
#include <private/qv4compilationunitmapper_p.h>
#include <private/qml_compile_hash_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/inlinecomponentutils_p.h>

#include <QtQml/qqmlfile.h>
#include <QtQml/qqmlpropertymap.h>

#include <QtCore/qdir.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/QScopedValueRollback>

#if defined(QML_COMPILE_HASH)
#  ifdef Q_OS_LINUX
// Place on a separate section on Linux so it's easier to check from outside
// what the hash version is.
__attribute__((section(".qml_compile_hash")))
#  endif
const char qml_compile_hash[48 + 1] = QML_COMPILE_HASH;
static_assert(sizeof(QV4::CompiledData::Unit::libraryVersionHash) >= QML_COMPILE_HASH_LENGTH + 1,
              "Compile hash length exceeds reserved size in data structure. Please adjust and bump the format version");
#else
#  error "QML_COMPILE_HASH must be defined for the build of QtDeclarative to ensure version checking for cache files"
#endif

QT_BEGIN_NAMESPACE

namespace QV4 {

ExecutableCompilationUnit::ExecutableCompilationUnit() = default;

ExecutableCompilationUnit::ExecutableCompilationUnit(
        CompiledData::CompilationUnit &&compilationUnit)
    : CompiledData::CompilationUnit(std::move(compilationUnit))
{}

ExecutableCompilationUnit::~ExecutableCompilationUnit()
{
    unlink();
}

QString ExecutableCompilationUnit::localCacheFilePath(const QUrl &url)
{
    static const QByteArray envCachePath = qgetenv("QML_DISK_CACHE_PATH");

    const QString localSourcePath = QQmlFile::urlToLocalFileOrQrc(url);
    const QString cacheFileSuffix = QFileInfo(localSourcePath + QLatin1Char('c')).completeSuffix();
    QCryptographicHash fileNameHash(QCryptographicHash::Sha1);
    fileNameHash.addData(localSourcePath.toUtf8());
    QString directory = envCachePath.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1String("/qmlcache/")
            : QString::fromLocal8Bit(envCachePath) + QLatin1String("/");
    QDir::root().mkpath(directory);
    return directory + QString::fromUtf8(fileNameHash.result().toHex()) + QLatin1Char('.') + cacheFileSuffix;
}

static QString toString(QV4::ReturnedValue v)
{
    Value val = Value::fromReturnedValue(v);
    QString result;
    if (val.isInt32())
        result = QLatin1String("int ");
    else if (val.isDouble())
        result = QLatin1String("double ");
    if (val.isEmpty())
        result += QLatin1String("empty");
    else
        result += val.toQStringNoThrow();
    return result;
}

static void dumpConstantTable(const StaticValue *constants, uint count)
{
    QDebug d = qDebug();
    d.nospace() << Qt::right;
    for (uint i = 0; i < count; ++i) {
        d << qSetFieldWidth(8) << i << qSetFieldWidth(0) << ":    "
          << toString(constants[i].asReturnedValue()).toUtf8().constData() << "\n";
    }
}

QV4::Function *ExecutableCompilationUnit::linkToEngine(ExecutionEngine *engine)
{
    this->engine = engine;
    engine->compilationUnits.insert(this);

    Q_ASSERT(!runtimeStrings);
    Q_ASSERT(data);
    const quint32 stringCount = totalStringCount();
    runtimeStrings = (QV4::Heap::String **)malloc(stringCount * sizeof(QV4::Heap::String*));
    // memset the strings to 0 in case a GC run happens while we're within the loop below
    memset(runtimeStrings, 0, stringCount * sizeof(QV4::Heap::String*));
    for (uint i = 0; i < stringCount; ++i)
        runtimeStrings[i] = engine->newString(stringAt(i));

    runtimeRegularExpressions
            = new QV4::Value[data->regexpTableSize];
    // memset the regexps to 0 in case a GC run happens while we're within the loop below
    memset(runtimeRegularExpressions, 0,
           data->regexpTableSize * sizeof(QV4::Value));
    for (uint i = 0; i < data->regexpTableSize; ++i) {
        const CompiledData::RegExp *re = data->regexpAt(i);
        uint f = re->flags;
        const CompiledData::RegExp::Flags flags = static_cast<CompiledData::RegExp::Flags>(f);
        runtimeRegularExpressions[i] = QV4::RegExp::create(
                engine, stringAt(re->stringIndex), flags);
    }

    if (data->lookupTableSize) {
        runtimeLookups = new QV4::Lookup[data->lookupTableSize];
        memset(runtimeLookups, 0, data->lookupTableSize * sizeof(QV4::Lookup));
        const CompiledData::Lookup *compiledLookups = data->lookupTable();
        for (uint i = 0; i < data->lookupTableSize; ++i) {
            QV4::Lookup *l = runtimeLookups + i;

            CompiledData::Lookup::Type type
                    = CompiledData::Lookup::Type(uint(compiledLookups[i].type_and_flags));
            if (type == CompiledData::Lookup::Type_Getter)
                l->getter = QV4::Lookup::getterGeneric;
            else if (type == CompiledData::Lookup::Type_Setter)
                l->setter = QV4::Lookup::setterGeneric;
            else if (type == CompiledData::Lookup::Type_GlobalGetter)
                l->globalGetter = QV4::Lookup::globalGetterGeneric;
            else if (type == CompiledData::Lookup::Type_QmlContextPropertyGetter)
                l->qmlContextPropertyGetter = QQmlContextWrapper::resolveQmlContextPropertyLookupGetter;
            l->nameIndex = compiledLookups[i].nameIndex;
        }
    }

    if (data->jsClassTableSize) {
        runtimeClasses
                = (QV4::Heap::InternalClass **)malloc(data->jsClassTableSize
                                                      * sizeof(QV4::Heap::InternalClass *));
        // memset the regexps to 0 in case a GC run happens while we're within the loop below
        memset(runtimeClasses, 0,
               data->jsClassTableSize * sizeof(QV4::Heap::InternalClass *));
        for (uint i = 0; i < data->jsClassTableSize; ++i) {
            int memberCount = 0;
            const CompiledData::JSClassMember *member
                    = data->jsClassAt(i, &memberCount);
            runtimeClasses[i]
                    = engine->internalClasses(QV4::ExecutionEngine::Class_Object);
            for (int j = 0; j < memberCount; ++j, ++member)
                runtimeClasses[i]
                        = runtimeClasses[i]->addMember(
                                engine->identifierTable->asPropertyKey(
                                        runtimeStrings[member->nameOffset]),
                                member->isAccessor
                                        ? QV4::Attr_Accessor
                                        : QV4::Attr_Data);
        }
    }

    runtimeFunctions.resize(data->functionTableSize);
    for (int i = 0 ;i < runtimeFunctions.size(); ++i) {
        const QV4::CompiledData::Function *compiledFunction = data->functionAt(i);
        runtimeFunctions[i] = QV4::Function::create(engine, this, compiledFunction);
    }

    Scope scope(engine);
    Scoped<InternalClass> ic(scope);

    runtimeBlocks.resize(data->blockTableSize);
    for (int i = 0 ;i < runtimeBlocks.size(); ++i) {
        const QV4::CompiledData::Block *compiledBlock = data->blockAt(i);
        ic = engine->internalClasses(EngineBase::Class_CallContext);

        // first locals
        const quint32_le *localsIndices = compiledBlock->localsTable();
        for (quint32 j = 0; j < compiledBlock->nLocals; ++j)
            ic = ic->addMember(
                    engine->identifierTable->asPropertyKey(runtimeStrings[localsIndices[j]]),
                    Attr_NotConfigurable);
        runtimeBlocks[i] = ic->d();
    }

    static const bool showCode = qEnvironmentVariableIsSet("QV4_SHOW_BYTECODE");
    if (showCode) {
        qDebug() << "=== Constant table";
        dumpConstantTable(constants, data->constantTableSize);
        qDebug() << "=== String table";
        for (uint i = 0, end = totalStringCount(); i < end; ++i)
            qDebug() << "    " << i << ":" << runtimeStrings[i]->toQString();
        qDebug() << "=== Closure table";
        for (uint i = 0; i < data->functionTableSize; ++i)
            qDebug() << "    " << i << ":" << runtimeFunctions[i]->name()->toQString();
        qDebug() << "root function at index "
                 << (data->indexOfRootFunction != -1
                             ? data->indexOfRootFunction : 0);
    }

    if (data->indexOfRootFunction != -1)
        return runtimeFunctions[data->indexOfRootFunction];
    else
        return nullptr;
}

Heap::Object *ExecutableCompilationUnit::templateObjectAt(int index) const
{
    Q_ASSERT(index < int(data->templateObjectTableSize));
    if (!templateObjects.size())
        templateObjects.resize(data->templateObjectTableSize);
    Heap::Object *o = templateObjects.at(index);
    if (o)
        return o;

    // create the template object
    Scope scope(engine);
    const CompiledData::TemplateObject *t = data->templateObjectAt(index);
    Scoped<ArrayObject> a(scope, engine->newArrayObject(t->size));
    Scoped<ArrayObject> raw(scope, engine->newArrayObject(t->size));
    ScopedValue s(scope);
    for (uint i = 0; i < t->size; ++i) {
        s = runtimeStrings[t->stringIndexAt(i)];
        a->arraySet(i, s);
        s = runtimeStrings[t->rawStringIndexAt(i)];
        raw->arraySet(i, s);
    }

    ObjectPrototype::method_freeze(engine->functionCtor(), nullptr, raw, 1);
    a->defineReadonlyProperty(QStringLiteral("raw"), raw);
    ObjectPrototype::method_freeze(engine->functionCtor(), nullptr, a, 1);

    templateObjects[index] = a->objectValue()->d();
    return templateObjects.at(index);
}

void ExecutableCompilationUnit::unlink()
{
    if (engine)
        nextCompilationUnit.remove();

    if (isRegisteredWithEngine) {
        Q_ASSERT(data && propertyCaches.count() > 0 && propertyCaches.at(/*root object*/0));
        if (qmlEngine)
            qmlEngine->unregisterInternalCompositeType(this);
        QQmlMetaType::unregisterInternalCompositeType({metaTypeId, listMetaTypeId});
        isRegisteredWithEngine = false;
    }

    propertyCaches.clear();

    if (runtimeLookups) {
        for (uint i = 0; i < data->lookupTableSize; ++i) {
            QV4::Lookup &l = runtimeLookups[i];
            if (l.getter == QV4::QObjectWrapper::lookupGetter
                    || l.getter == QQmlTypeWrapper::lookupSingletonProperty) {
                if (QQmlPropertyCache *pc = l.qobjectLookup.propertyCache)
                    pc->release();
            } else if (l.getter == QQmlValueTypeWrapper::lookupGetter
                       || l.getter == QQmlTypeWrapper::lookupSingletonProperty) {
                if (QQmlPropertyCache *pc = l.qgadgetLookup.propertyCache)
                    pc->release();
            }

            if (l.qmlContextPropertyGetter == QQmlContextWrapper::lookupScopeObjectProperty
                    || l.qmlContextPropertyGetter == QQmlContextWrapper::lookupContextObjectProperty) {
                if (QQmlPropertyCache *pc = l.qobjectLookup.propertyCache)
                    pc->release();
            }
        }
    }

    dependentScripts.clear();

    typeNameCache = nullptr;

    qDeleteAll(resolvedTypes);
    resolvedTypes.clear();

    engine = nullptr;
    qmlEngine = nullptr;

    delete [] runtimeLookups;
    runtimeLookups = nullptr;

    for (QV4::Function *f : qAsConst(runtimeFunctions))
        f->destroy();
    runtimeFunctions.clear();

    free(runtimeStrings);
    runtimeStrings = nullptr;
    delete [] runtimeRegularExpressions;
    runtimeRegularExpressions = nullptr;
    free(runtimeClasses);
    runtimeClasses = nullptr;
}

void ExecutableCompilationUnit::markObjects(QV4::MarkStack *markStack)
{
    if (runtimeStrings) {
        for (uint i = 0, end = totalStringCount(); i < end; ++i)
            if (runtimeStrings[i])
                runtimeStrings[i]->mark(markStack);
    }
    if (runtimeRegularExpressions) {
        for (uint i = 0; i < data->regexpTableSize; ++i)
            Value::fromStaticValue(runtimeRegularExpressions[i]).mark(markStack);
    }
    if (runtimeClasses) {
        for (uint i = 0; i < data->jsClassTableSize; ++i)
            if (runtimeClasses[i])
                runtimeClasses[i]->mark(markStack);
    }
    for (QV4::Function *f : qAsConst(runtimeFunctions))
        if (f && f->internalClass)
            f->internalClass->mark(markStack);
    for (QV4::Heap::InternalClass *c : qAsConst(runtimeBlocks))
        if (c)
            c->mark(markStack);

    for (QV4::Heap::Object *o : qAsConst(templateObjects))
        if (o)
            o->mark(markStack);

    if (runtimeLookups) {
        for (uint i = 0; i < data->lookupTableSize; ++i)
            runtimeLookups[i].markObjects(markStack);
    }

    if (auto mod = module())
        mod->mark(markStack);
}

IdentifierHash ExecutableCompilationUnit::createNamedObjectsPerComponent(int componentObjectIndex)
{
    IdentifierHash namedObjectCache(engine);
    const CompiledData::Object *component = objectAt(componentObjectIndex);
    const quint32_le *namedObjectIndexPtr = component->namedObjectsInComponentTable();
    for (quint32 i = 0; i < component->nNamedObjectsInComponent; ++i, ++namedObjectIndexPtr) {
        const CompiledData::Object *namedObject = objectAt(*namedObjectIndexPtr);
        namedObjectCache.add(runtimeStrings[namedObject->idNameIndex], namedObject->id);
    }
    return *namedObjectsPerComponentCache.insert(componentObjectIndex, namedObjectCache);
}

void ExecutableCompilationUnit::finalizeCompositeType(QQmlEnginePrivate *qmlEngine, CompositeMetaTypeIds typeIds)
{
    this->qmlEngine = qmlEngine;

    // Add to type registry of composites
    if (propertyCaches.needsVMEMetaObject(/*root object*/0)) {
        // typeIds is only valid for types that have references to themselves.
        if (!typeIds.isValid())
            typeIds = QQmlMetaType::registerInternalCompositeType(rootPropertyCache()->className());
        metaTypeId = typeIds.id;
        listMetaTypeId = typeIds.listId;
        qmlEngine->registerInternalCompositeType(this);

    } else {
        const QV4::CompiledData::Object *obj = objectAt(/*root object*/0);
        auto *typeRef = resolvedTypes.value(obj->inheritedTypeNameIndex);
        Q_ASSERT(typeRef);
        if (const auto compilationUnit = typeRef->compilationUnit()) {
            metaTypeId = compilationUnit->metaTypeId;
            listMetaTypeId = compilationUnit->listMetaTypeId;
        } else {
            metaTypeId = typeRef->type.typeId();
            listMetaTypeId = typeRef->type.qListTypeId();
        }
    }

    // Collect some data for instantiation later.
    using namespace  icutils;
    std::vector<QV4::CompiledData::InlineComponent> allICs {};
    for (int i=0; i != objectCount(); ++i) {
        const CompiledObject *obj = objectAt(i);
        for (auto it = obj->inlineComponentsBegin(); it != obj->inlineComponentsEnd(); ++it) {
            allICs.push_back(*it);
        }
    }
    std::vector<Node> nodes;
    nodes.resize(allICs.size());
    std::iota(nodes.begin(), nodes.end(), 0);
    AdjacencyList adjacencyList;
    adjacencyList.resize(nodes.size());
    fillAdjacencyListForInlineComponents(this, adjacencyList, nodes, allICs);
    bool hasCycle = false;
    auto nodesSorted = topoSort(nodes, adjacencyList, hasCycle);
    Q_ASSERT(!hasCycle); // would have already been discovered by qqmlpropertycachcecreator

    // We need to first iterate over all inline components, as the containing component might create instances of them
    // and in that case we need to add its object count
    for (auto nodeIt = nodesSorted.rbegin(); nodeIt != nodesSorted.rend(); ++nodeIt) {
        const auto &ic = allICs.at(nodeIt->index);
        int lastICRoot = ic.objectIndex;
        for (int i = ic.objectIndex; i<objectCount(); ++i) {
            const QV4::CompiledData::Object *obj = objectAt(i);
            bool leftCurrentInlineComponent =
                       (i != lastICRoot && obj->flags & QV4::CompiledData::Object::IsInlineComponentRoot)
                    || !(obj->flags & QV4::CompiledData::Object::InPartOfInlineComponent);
            if (leftCurrentInlineComponent)
                break;
            inlineComponentData[lastICRoot].totalBindingCount += obj->nBindings;

            if (auto *typeRef = resolvedTypes.value(obj->inheritedTypeNameIndex)) {
                if (typeRef->type.isValid() && typeRef->type.parserStatusCast() != -1)
                    ++inlineComponentData[lastICRoot].totalParserStatusCount;

                ++inlineComponentData[lastICRoot].totalObjectCount;
                if (const auto compilationUnit = typeRef->compilationUnit()) {
                    // if the type is an inline component type, we have to extract the information from it
                    // This requires that inline components are visited in the correct order
                    auto icRoot = compilationUnit->icRoot;
                    if (typeRef->type.isInlineComponentType()) {
                        icRoot = typeRef->type.inlineComponendId();
                    }
                    QScopedValueRollback<int> rollback {compilationUnit->icRoot, icRoot};
                    inlineComponentData[lastICRoot].totalBindingCount += compilationUnit->totalBindingsCount();
                    inlineComponentData[lastICRoot].totalParserStatusCount += compilationUnit->totalParserStatusCount();
                    inlineComponentData[lastICRoot].totalObjectCount += compilationUnit->totalObjectCount();
                }
            }
        }
    }
    int bindingCount = 0;
    int parserStatusCount = 0;
    int objectCount = 0;
    for (quint32 i = 0, count = this->objectCount(); i < count; ++i) {
        const QV4::CompiledData::Object *obj = objectAt(i);
        if (obj->flags & QV4::CompiledData::Object::InPartOfInlineComponent) {
            continue;
        }
        bindingCount += obj->nBindings;
        if (auto *typeRef = resolvedTypes.value(obj->inheritedTypeNameIndex)) {
            if (typeRef->type.isValid() && typeRef->type.parserStatusCast() != -1)
                ++parserStatusCount;
            ++objectCount;
            if (const auto compilationUnit = typeRef->compilationUnit()) {
                auto icRoot = compilationUnit->icRoot;
                if (typeRef->type.isInlineComponentType()) {
                    icRoot = typeRef->type.inlineComponendId();
                }
                QScopedValueRollback<int> rollback {compilationUnit->icRoot, icRoot};
                bindingCount += compilationUnit->totalBindingsCount();
                parserStatusCount += compilationUnit->totalParserStatusCount();
                objectCount += compilationUnit->totalObjectCount();
            }
        }
    }

    m_totalBindingsCount = bindingCount;
    m_totalParserStatusCount = parserStatusCount;
    m_totalObjectCount = objectCount;
}

int ExecutableCompilationUnit::totalBindingsCount() const {
    if (icRoot == -1)
        return m_totalBindingsCount;
    return inlineComponentData[icRoot].totalBindingCount;
}

int ExecutableCompilationUnit::totalObjectCount() const {
    if (icRoot == -1)
        return m_totalObjectCount;
    return inlineComponentData[icRoot].totalObjectCount;
}

int ExecutableCompilationUnit::totalParserStatusCount() const {
    if (icRoot == -1)
        return m_totalParserStatusCount;
    return inlineComponentData[icRoot].totalParserStatusCount;
}

bool ExecutableCompilationUnit::verifyChecksum(const CompiledData::DependentTypesHasher &dependencyHasher) const
{
    if (!dependencyHasher) {
        for (size_t i = 0; i < sizeof(data->dependencyMD5Checksum); ++i) {
            if (data->dependencyMD5Checksum[i] != 0)
                return false;
        }
        return true;
    }
    const QByteArray checksum = dependencyHasher();
    return checksum.size() == sizeof(data->dependencyMD5Checksum)
            && memcmp(data->dependencyMD5Checksum, checksum.constData(),
                      sizeof(data->dependencyMD5Checksum)) == 0;
}

CompositeMetaTypeIds ExecutableCompilationUnit::typeIdsForComponent(int objectid) const
{
    if (objectid == 0)
        return {metaTypeId, listMetaTypeId};
    return inlineComponentData[objectid].typeIds;
}

QStringList ExecutableCompilationUnit::moduleRequests() const
{
    QStringList requests;
    requests.reserve(data->moduleRequestTableSize);
    for (uint i = 0; i < data->moduleRequestTableSize; ++i)
        requests << stringAt(data->moduleRequestTable()[i]);
    return requests;
}

Heap::Module *ExecutableCompilationUnit::instantiate(ExecutionEngine *engine)
{
    if (isESModule() && module())
        return module();

    if (data->indexOfRootFunction < 0)
        return nullptr;

    if (!this->engine)
        linkToEngine(engine);

    Scope scope(engine);
    Scoped<Module> module(scope, engine->memoryManager->allocate<Module>(engine, this));

    if (isESModule())
        setModule(module->d());

    for (const QString &request: moduleRequests()) {
        auto dependentModuleUnit = engine->loadModule(QUrl(request), this);
        if (engine->hasException)
            return nullptr;
        dependentModuleUnit->instantiate(engine);
    }

    ScopedString importName(scope);

    const uint importCount = data->importEntryTableSize;
    if (importCount > 0) {
        imports = new const StaticValue *[importCount];
        memset(imports, 0, importCount * sizeof(StaticValue *));
    }
    for (uint i = 0; i < importCount; ++i) {
        const CompiledData::ImportEntry &entry = data->importEntryTable()[i];
        auto dependentModuleUnit = engine->loadModule(urlAt(entry.moduleRequest), this);
        importName = runtimeStrings[entry.importName];
        const Value *valuePtr = dependentModuleUnit->resolveExport(importName);
        if (!valuePtr) {
            QString referenceErrorMessage = QStringLiteral("Unable to resolve import reference ");
            referenceErrorMessage += importName->toQString();
            engine->throwReferenceError(referenceErrorMessage, fileName(), entry.location.line, entry.location.column);
            return nullptr;
        }
        imports[i] = valuePtr;
    }

    for (uint i = 0; i < data->indirectExportEntryTableSize; ++i) {
        const CompiledData::ExportEntry &entry = data->indirectExportEntryTable()[i];
        auto dependentModuleUnit = engine->loadModule(urlAt(entry.moduleRequest), this);
        if (!dependentModuleUnit)
            return nullptr;

        ScopedString importName(scope, runtimeStrings[entry.importName]);
        if (!dependentModuleUnit->resolveExport(importName)) {
            QString referenceErrorMessage = QStringLiteral("Unable to resolve re-export reference ");
            referenceErrorMessage += importName->toQString();
            engine->throwReferenceError(referenceErrorMessage, fileName(), entry.location.line, entry.location.column);
            return nullptr;
        }
    }

    return module->d();
}

const Value *ExecutableCompilationUnit::resolveExportRecursively(
        QV4::String *exportName, QVector<ResolveSetEntry> *resolveSet)
{
    if (!module())
        return nullptr;

    for (const auto &entry: *resolveSet)
        if (entry.module == this && entry.exportName->isEqualTo(exportName))
            return nullptr;

    (*resolveSet) << ResolveSetEntry(this, exportName);

    if (exportName->toQString() == QLatin1String("*"))
        return &module()->self;

    Scope scope(engine);

    if (auto localExport = lookupNameInExportTable(
                data->localExportEntryTable(), data->localExportEntryTableSize, exportName)) {
        ScopedString localName(scope, runtimeStrings[localExport->localName]);
        uint index = module()->scope->internalClass->indexOfValueOrGetter(localName->toPropertyKey());
        if (index == UINT_MAX)
            return nullptr;
        if (index >= module()->scope->locals.size)
            return &(imports[index - module()->scope->locals.size]->asValue<Value>());
        return &module()->scope->locals[index];
    }

    if (auto indirectExport = lookupNameInExportTable(
                data->indirectExportEntryTable(), data->indirectExportEntryTableSize, exportName)) {
        auto dependentModuleUnit = engine->loadModule(urlAt(indirectExport->moduleRequest), this);
        if (!dependentModuleUnit)
            return nullptr;
        ScopedString importName(scope, runtimeStrings[indirectExport->importName]);
        return dependentModuleUnit->resolveExportRecursively(importName, resolveSet);
    }


    if (exportName->toQString() == QLatin1String("default"))
        return nullptr;

    const Value *starResolution = nullptr;

    for (uint i = 0; i < data->starExportEntryTableSize; ++i) {
        const CompiledData::ExportEntry &entry = data->starExportEntryTable()[i];
        auto dependentModuleUnit = engine->loadModule(urlAt(entry.moduleRequest), this);
        if (!dependentModuleUnit)
            return nullptr;

        const Value *resolution = dependentModuleUnit->resolveExportRecursively(exportName, resolveSet);
        // ### handle ambiguous
        if (resolution) {
            if (!starResolution) {
                starResolution = resolution;
                continue;
            }
            if (resolution != starResolution)
                return nullptr;
        }
    }

    return starResolution;
}

const CompiledData::ExportEntry *ExecutableCompilationUnit::lookupNameInExportTable(
        const CompiledData::ExportEntry *firstExportEntry, int tableSize, QV4::String *name) const
{
    const CompiledData::ExportEntry *lastExportEntry = firstExportEntry + tableSize;
    auto matchingExport = std::lower_bound(firstExportEntry, lastExportEntry, name, [this](const CompiledData::ExportEntry &lhs, QV4::String *name) {
        return stringAt(lhs.exportName) < name->toQString();
    });
    if (matchingExport == lastExportEntry || stringAt(matchingExport->exportName) != name->toQString())
        return nullptr;
    return matchingExport;
}

void ExecutableCompilationUnit::getExportedNamesRecursively(
        QStringList *names, QVector<const ExecutableCompilationUnit*> *exportNameSet,
        bool includeDefaultExport) const
{
    if (exportNameSet->contains(this))
        return;
    exportNameSet->append(this);

    const auto append = [names, includeDefaultExport](const QString &name) {
        if (!includeDefaultExport && name == QLatin1String("default"))
            return;
        names->append(name);
    };

    for (uint i = 0; i < data->localExportEntryTableSize; ++i) {
        const CompiledData::ExportEntry &entry = data->localExportEntryTable()[i];
        append(stringAt(entry.exportName));
    }

    for (uint i = 0; i < data->indirectExportEntryTableSize; ++i) {
        const CompiledData::ExportEntry &entry = data->indirectExportEntryTable()[i];
        append(stringAt(entry.exportName));
    }

    for (uint i = 0; i < data->starExportEntryTableSize; ++i) {
        const CompiledData::ExportEntry &entry = data->starExportEntryTable()[i];
        auto dependentModuleUnit = engine->loadModule(urlAt(entry.moduleRequest), this);
        if (!dependentModuleUnit)
            return;
        dependentModuleUnit->getExportedNamesRecursively(names, exportNameSet, /*includeDefaultExport*/false);
    }
}

void ExecutableCompilationUnit::evaluate()
{
    QV4::Scope scope(engine);
    QV4::Scoped<Module> mod(scope, module());
    mod->evaluate();
}

void ExecutableCompilationUnit::evaluateModuleRequests()
{
    for (const QString &request: moduleRequests()) {
        auto dependentModuleUnit = engine->loadModule(QUrl(request), this);
        if (engine->hasException)
            return;
        dependentModuleUnit->evaluate();
        if (engine->hasException)
            return;
    }
}

bool ExecutableCompilationUnit::loadFromDisk(const QUrl &url, const QDateTime &sourceTimeStamp, QString *errorString)
{
    if (!QQmlFile::isLocalFile(url)) {
        *errorString = QStringLiteral("File has to be a local file.");
        return false;
    }

    const QString sourcePath = QQmlFile::urlToLocalFileOrQrc(url);
    QScopedPointer<CompilationUnitMapper> cacheFile(new CompilationUnitMapper());

    const QStringList cachePaths = { sourcePath + QLatin1Char('c'), localCacheFilePath(url) };
    for (const QString &cachePath : cachePaths) {
        CompiledData::Unit *mappedUnit = cacheFile->open(cachePath, sourceTimeStamp, errorString);
        if (!mappedUnit)
            continue;

        const CompiledData::Unit * const oldDataPtr
                = (data && !(data->flags & QV4::CompiledData::Unit::StaticData)) ? data
                                                                                     : nullptr;
        const CompiledData::Unit *oldData = data;
        auto dataPtrRevert = qScopeGuard([this, oldData](){
            setUnitData(oldData);
        });
        setUnitData(mappedUnit);

        if (data->sourceFileIndex != 0
            && sourcePath != QQmlFile::urlToLocalFileOrQrc(stringAt(data->sourceFileIndex))) {
            *errorString = QStringLiteral("QML source file has moved to a different location.");
            continue;
        }

        dataPtrRevert.dismiss();
        free(const_cast<CompiledData::Unit*>(oldDataPtr));
        backingFile.reset(cacheFile.take());
        return true;
    }

    return false;
}

bool ExecutableCompilationUnit::saveToDisk(const QUrl &unitUrl, QString *errorString)
{
    if (data->sourceTimeStamp == 0) {
        *errorString = QStringLiteral("Missing time stamp for source file");
        return false;
    }

    if (!QQmlFile::isLocalFile(unitUrl)) {
        *errorString = QStringLiteral("File has to be a local file.");
        return false;
    }

    return CompiledData::SaveableUnitPointer(unitData()).saveToDisk<char>(
            [&unitUrl, errorString](const char *data, quint32 size) {
        return CompiledData::SaveableUnitPointer::writeDataToFile(localCacheFilePath(unitUrl), data,
                                                                  size, errorString);
    });
}

/*!
Returns the property cache, if one alread exists.  The cache is not referenced.
*/
QQmlRefPointer<QQmlPropertyCache> ResolvedTypeReference::propertyCache() const
{
    if (type.isValid())
        return typePropertyCache;
    else
        return m_compilationUnit->rootPropertyCache();
}

/*!
Returns the property cache, creating one if it doesn't already exist.  The cache is not referenced.
*/
QQmlRefPointer<QQmlPropertyCache> ResolvedTypeReference::createPropertyCache(QQmlEngine *engine)
{
    if (typePropertyCache) {
        return typePropertyCache;
    } else if (type.isValid()) {
        typePropertyCache = QQmlEnginePrivate::get(engine)->cache(type.metaObject(), minorVersion);
        return typePropertyCache;
    } else {
        Q_ASSERT(m_compilationUnit);
        return m_compilationUnit->rootPropertyCache();
    }
}

bool ResolvedTypeReference::addToHash(QCryptographicHash *hash, QQmlEngine *engine)
{
    if (type.isValid() && !type.isInlineComponentType()) {
        bool ok = false;
        hash->addData(createPropertyCache(engine)->checksum(&ok));
        return ok;
    }
    if (!m_compilationUnit)
        return false;
    hash->addData(m_compilationUnit->data->md5Checksum,
                  sizeof(m_compilationUnit->data->md5Checksum));
    return true;
}

template <typename T>
bool qtTypeInherits(const QMetaObject *mo) {
    while (mo) {
        if (mo == &T::staticMetaObject)
            return true;
        mo = mo->superClass();
    }
    return false;
}

void ResolvedTypeReference::doDynamicTypeCheck()
{
    const QMetaObject *mo = nullptr;
    if (typePropertyCache)
        mo = typePropertyCache->firstCppMetaObject();
    else if (type.isValid())
        mo = type.metaObject();
    else if (m_compilationUnit)
        mo = m_compilationUnit->rootPropertyCache()->firstCppMetaObject();
    isFullyDynamicType = qtTypeInherits<QQmlPropertyMap>(mo);
}

bool ResolvedTypeReferenceMap::addToHash(QCryptographicHash *hash, QQmlEngine *engine) const
{
    for (auto it = constBegin(), end = constEnd(); it != end; ++it) {
        if (!it.value()->addToHash(hash, engine))
            return false;
    }

    return true;
}

QString ExecutableCompilationUnit::bindingValueAsString(const CompiledData::Binding *binding) const
{
    using namespace CompiledData;
    switch (binding->type) {
    case Binding::Type_Script:
    case Binding::Type_String:
        return stringAt(binding->stringIndex);
    case Binding::Type_Null:
        return QStringLiteral("null");
    case Binding::Type_Boolean:
        return binding->value.b ? QStringLiteral("true") : QStringLiteral("false");
    case Binding::Type_Number:
        return QString::number(bindingValueAsNumber(binding), 'g', QLocale::FloatingPointShortest);
    case Binding::Type_Invalid:
        return QString();
#if !QT_CONFIG(translation)
    case Binding::Type_TranslationById:
    case Binding::Type_Translation:
        return stringAt(
                data->translations()[binding->value.translationDataIndex].stringIndex);
#else
    case Binding::Type_TranslationById: {
        const TranslationData &translation
                = data->translations()[binding->value.translationDataIndex];
        QByteArray id = stringAt(translation.stringIndex).toUtf8();
        return qtTrId(id.constData(), translation.number);
    }
    case Binding::Type_Translation: {
        const TranslationData &translation
                = data->translations()[binding->value.translationDataIndex];
        // This code must match that in the qsTr() implementation
        const QString &path = fileName();
        int lastSlash = path.lastIndexOf(QLatin1Char('/'));
        QStringRef context = (lastSlash > -1) ? path.midRef(lastSlash + 1, path.length() - lastSlash - 5)
                                              : QStringRef();
        QByteArray contextUtf8 = context.toUtf8();
        QByteArray comment = stringAt(translation.commentIndex).toUtf8();
        QByteArray text = stringAt(translation.stringIndex).toUtf8();
        return QCoreApplication::translate(contextUtf8.constData(), text.constData(),
                                           comment.constData(), translation.number);
    }
#endif
    default:
        break;
    }
    return QString();
}

QString ExecutableCompilationUnit::bindingValueAsScriptString(
        const CompiledData::Binding *binding) const
{
    return (binding->type == CompiledData::Binding::Type_String)
            ? CompiledData::Binding::escapedString(stringAt(binding->stringIndex))
            : bindingValueAsString(binding);
}

bool ExecutableCompilationUnit::verifyHeader(
        const CompiledData::Unit *unit, QDateTime expectedSourceTimeStamp, QString *errorString)
{
    if (strncmp(unit->magic, CompiledData::magic_str, sizeof(unit->magic))) {
        *errorString = QStringLiteral("Magic bytes in the header do not match");
        return false;
    }

    if (unit->version != quint32(QV4_DATA_STRUCTURE_VERSION)) {
        *errorString = QString::fromUtf8("V4 data structure version mismatch. Found %1 expected %2")
                               .arg(unit->version, 0, 16).arg(QV4_DATA_STRUCTURE_VERSION, 0, 16);
        return false;
    }

    if (unit->qtVersion != quint32(QT_VERSION)) {
        *errorString = QString::fromUtf8("Qt version mismatch. Found %1 expected %2")
                               .arg(unit->qtVersion, 0, 16).arg(QT_VERSION, 0, 16);
        return false;
    }

    if (unit->sourceTimeStamp) {
        // Files from the resource system do not have any time stamps, so fall back to the application
        // executable.
        if (!expectedSourceTimeStamp.isValid())
            expectedSourceTimeStamp = QFileInfo(QCoreApplication::applicationFilePath()).lastModified();

        if (expectedSourceTimeStamp.isValid()
                && expectedSourceTimeStamp.toMSecsSinceEpoch() != unit->sourceTimeStamp) {
            *errorString = QStringLiteral("QML source file has a different time stamp than cached file.");
            return false;
        }
    }

#if defined(QML_COMPILE_HASH)
    if (qstrcmp(qml_compile_hash, unit->libraryVersionHash) != 0) {
        *errorString = QStringLiteral("QML library version mismatch. Expected compile hash does not match");
        return false;
    }
#else
#error "QML_COMPILE_HASH must be defined for the build of QtDeclarative to ensure version checking for cache files"
#endif
    return true;
}

} // namespace QV4

QT_END_NAMESPACE
