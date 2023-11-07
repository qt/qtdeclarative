// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qml/qqmlprivate.h"
#include "qv4engine_p.h"
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
#include <private/qv4resolvedtypereference_p.h>
#include <private/qv4objectiterator_p.h>

#include <QtQml/qqmlfile.h>
#include <QtQml/qqmlpropertymap.h>

#include <QtCore/qdir.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/QScopedValueRollback>

static_assert(QV4::CompiledData::QmlCompileHashSpace > QML_COMPILE_HASH_LENGTH);

#if defined(QML_COMPILE_HASH) && defined(QML_COMPILE_HASH_LENGTH) && QML_COMPILE_HASH_LENGTH > 0
#  ifdef Q_OS_LINUX
// Place on a separate section on Linux so it's easier to check from outside
// what the hash version is.
__attribute__((section(".qml_compile_hash")))
#  endif
const char qml_compile_hash[QV4::CompiledData::QmlCompileHashSpace] = QML_COMPILE_HASH;
static_assert(sizeof(QV4::CompiledData::Unit::libraryVersionHash) > QML_COMPILE_HASH_LENGTH,
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
    // strings need to be 0 in case a GC run happens while we're within the loop below
    runtimeStrings = (QV4::Heap::String **)calloc(stringCount, sizeof(QV4::Heap::String*));
    for (uint i = 0; i < stringCount; ++i)
        runtimeStrings[i] = engine->newString(stringAt(i));

    // zero-initialize regexps in case a GC run happens while we're within the loop below
    runtimeRegularExpressions
            = new QV4::Value[data->regexpTableSize] {};
    for (uint i = 0; i < data->regexpTableSize; ++i) {
        const CompiledData::RegExp *re = data->regexpAt(i);
        uint f = re->flags();
        const CompiledData::RegExp::Flags flags = static_cast<CompiledData::RegExp::Flags>(f);
        runtimeRegularExpressions[i] = QV4::RegExp::create(
                engine, stringAt(re->stringIndex()), flags);
    }

    if (data->lookupTableSize) {
        runtimeLookups = new QV4::Lookup[data->lookupTableSize];
        memset(runtimeLookups, 0, data->lookupTableSize * sizeof(QV4::Lookup));
        const CompiledData::Lookup *compiledLookups = data->lookupTable();
        for (uint i = 0; i < data->lookupTableSize; ++i) {
            QV4::Lookup *l = runtimeLookups + i;

            CompiledData::Lookup::Type type
                    = CompiledData::Lookup::Type(uint(compiledLookups[i].type()));
            if (type == CompiledData::Lookup::Type_Getter)
                l->getter = QV4::Lookup::getterGeneric;
            else if (type == CompiledData::Lookup::Type_Setter)
                l->setter = QV4::Lookup::setterGeneric;
            else if (type == CompiledData::Lookup::Type_GlobalGetter)
                l->globalGetter = QV4::Lookup::globalGetterGeneric;
            else if (type == CompiledData::Lookup::Type_QmlContextPropertyGetter)
                l->qmlContextPropertyGetter = QQmlContextWrapper::resolveQmlContextPropertyLookupGetter;
            l->forCall = compiledLookups[i].mode() == CompiledData::Lookup::Mode_ForCall;
            l->nameIndex = compiledLookups[i].nameIndex();
        }
    }

    if (data->jsClassTableSize) {
        // zero the regexps with calloc in case a GC run happens while we're within the loop below
        runtimeClasses
                = (QV4::Heap::InternalClass **)calloc(data->jsClassTableSize,
                                                      sizeof(QV4::Heap::InternalClass *));

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
                                        runtimeStrings[member->nameOffset()]),
                                member->isAccessor()
                                        ? QV4::Attr_Accessor
                                        : QV4::Attr_Data);
        }
    }

    runtimeFunctions.resize(data->functionTableSize);
    static bool ignoreAotCompiledFunctions
            = qEnvironmentVariableIsSet("QV4_FORCE_INTERPRETER")
            || !(engine->diskCacheOptions() & ExecutionEngine::DiskCache::AotNative);

    const QQmlPrivate::AOTCompiledFunction *aotFunction
            = ignoreAotCompiledFunctions ? nullptr : aotCompiledFunctions;

    auto advanceAotFunction = [&](int i) -> const QQmlPrivate::AOTCompiledFunction * {
        if (aotFunction) {
            if (aotFunction->functionPtr) {
                if (aotFunction->extraData == i)
                    return aotFunction++;
            } else {
                aotFunction = nullptr;
            }
        }
        return nullptr;
    };

    for (int i = 0 ;i < runtimeFunctions.size(); ++i) {
        const QV4::CompiledData::Function *compiledFunction = data->functionAt(i);
        runtimeFunctions[i] = QV4::Function::create(engine, this, compiledFunction,
                                                    advanceAotFunction(i));
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

    if (isRegistered) {
        Q_ASSERT(data && propertyCaches.count() > 0 && propertyCaches.at(/*root object*/0));
        QQmlMetaType::unregisterInternalCompositeType(this);
    }

    propertyCaches.clear();

    if (runtimeLookups) {
        for (uint i = 0; i < data->lookupTableSize; ++i)
            runtimeLookups[i].releasePropertyCache();
    }

    dependentScripts.clear();

    typeNameCache.reset();

    qDeleteAll(resolvedTypes);
    resolvedTypes.clear();

    engine = nullptr;

    delete [] runtimeLookups;
    runtimeLookups = nullptr;

    for (QV4::Function *f : std::as_const(runtimeFunctions))
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
    for (QV4::Function *f : std::as_const(runtimeFunctions))
        if (f && f->internalClass)
            f->internalClass->mark(markStack);
    for (QV4::Heap::InternalClass *c : std::as_const(runtimeBlocks))
        if (c)
            c->mark(markStack);

    for (QV4::Heap::Object *o : std::as_const(templateObjects))
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
        namedObjectCache.add(runtimeStrings[namedObject->idNameIndex], namedObject->objectId());
    }
    Q_ASSERT(!namedObjectCache.isEmpty());
    return *namedObjectsPerComponentCache.insert(componentObjectIndex, namedObjectCache);
}

template<typename F>
void processInlinComponentType(
    const QQmlType &type, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
    F &&populateIcData)
{
    if (type.isInlineComponentType()) {
        QString icRootName;
        if (compilationUnit->icRootName) {
            icRootName = type.elementName();
            std::swap(*compilationUnit->icRootName, icRootName);
        } else {
            compilationUnit->icRootName = std::make_unique<QString>(type.elementName());
        }

        populateIcData();

        if (icRootName.isEmpty())
            compilationUnit->icRootName.reset();
        else
            std::swap(*compilationUnit->icRootName, icRootName);
    } else {
        populateIcData();
    }
}

void ExecutableCompilationUnit::finalizeCompositeType(CompositeMetaTypeIds types)
{
    // Add to type registry of composites
    if (propertyCaches.needsVMEMetaObject(/*root object*/0)) {
        // typeIds is only valid for types that have references to themselves.
        if (!types.isValid())
            types = CompositeMetaTypeIds::fromCompositeName(rootPropertyCache()->className());
        typeIds = types;
        QQmlMetaType::registerInternalCompositeType(this);

    } else {
        const QV4::CompiledData::Object *obj = objectAt(/*root object*/0);
        auto *typeRef = resolvedTypes.value(obj->inheritedTypeNameIndex);
        Q_ASSERT(typeRef);
        if (const auto compilationUnit = typeRef->compilationUnit()) {
            typeIds = compilationUnit->typeIds;
        } else {
            const auto type = typeRef->type();
            typeIds = CompositeMetaTypeIds{ type.typeId(), type.qListTypeId() };
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
    NodeList nodes;
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
        const auto &ic = allICs.at(nodeIt->index());
        const int lastICRoot = ic.objectIndex;
        for (int i = ic.objectIndex; i<objectCount(); ++i) {
            const QV4::CompiledData::Object *obj = objectAt(i);
            bool leftCurrentInlineComponent
                    = (i != lastICRoot
                            && obj->hasFlag(QV4::CompiledData::Object::IsInlineComponentRoot))
                        || !obj->hasFlag(QV4::CompiledData::Object::IsPartOfInlineComponent);
            if (leftCurrentInlineComponent)
                break;
            const QString lastICRootName = stringAt(ic.nameIndex);
            inlineComponentData[lastICRootName].totalBindingCount += obj->nBindings;

            if (auto *typeRef = resolvedTypes.value(obj->inheritedTypeNameIndex)) {
                const auto type = typeRef->type();
                if (type.isValid() && type.parserStatusCast() != -1)
                    ++inlineComponentData[lastICRootName].totalParserStatusCount;

                ++inlineComponentData[lastICRootName].totalObjectCount;
                if (const auto compilationUnit = typeRef->compilationUnit()) {
                    // if the type is an inline component type, we have to extract the information from it
                    // This requires that inline components are visited in the correct order
                    processInlinComponentType(type, compilationUnit, [&]() {
                        auto &icData = inlineComponentData[lastICRootName];
                        icData.totalBindingCount += compilationUnit->totalBindingsCount();
                        icData.totalParserStatusCount += compilationUnit->totalParserStatusCount();
                        icData.totalObjectCount += compilationUnit->totalObjectCount();
                    });
                }
            }
        }
    }
    int bindingCount = 0;
    int parserStatusCount = 0;
    int objectCount = 0;
    for (quint32 i = 0, count = this->objectCount(); i < count; ++i) {
        const QV4::CompiledData::Object *obj = objectAt(i);
        if (obj->hasFlag(QV4::CompiledData::Object::IsPartOfInlineComponent))
            continue;

        bindingCount += obj->nBindings;
        if (auto *typeRef = resolvedTypes.value(obj->inheritedTypeNameIndex)) {
            const auto type = typeRef->type();
            if (type.isValid() && type.parserStatusCast() != -1)
                ++parserStatusCount;
            ++objectCount;
            if (const auto compilationUnit = typeRef->compilationUnit()) {
                processInlinComponentType(type, compilationUnit, [&](){
                    bindingCount += compilationUnit->totalBindingsCount();
                    parserStatusCount += compilationUnit->totalParserStatusCount();
                    objectCount += compilationUnit->totalObjectCount();
                });
            }
        }
    }

    m_totalBindingsCount = bindingCount;
    m_totalParserStatusCount = parserStatusCount;
    m_totalObjectCount = objectCount;
}

int ExecutableCompilationUnit::totalBindingsCount() const {
    if (!icRootName)
        return m_totalBindingsCount;
    return inlineComponentData[*icRootName].totalBindingCount;
}

int ExecutableCompilationUnit::totalObjectCount() const {
    if (!icRootName)
        return m_totalObjectCount;
    return inlineComponentData[*icRootName].totalObjectCount;
}

ResolvedTypeReference *ExecutableCompilationUnit::resolvedType(QMetaType type) const
{
    for (ResolvedTypeReference *ref : std::as_const(resolvedTypes)) {
        if (ref->type().typeId() == type)
            return ref;
    }
    return nullptr;
}

int ExecutableCompilationUnit::totalParserStatusCount() const {
    if (!icRootName)
        return m_totalParserStatusCount;
    return inlineComponentData[*icRootName].totalParserStatusCount;
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

CompositeMetaTypeIds ExecutableCompilationUnit::typeIdsForComponent(
    const QString &inlineComponentName) const
{
    if (inlineComponentName.isEmpty())
        return typeIds;
    return inlineComponentData[inlineComponentName].typeIds;
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
        const QUrl url(request);
        const auto dependentModuleUnit = engine->loadModule(url, this);
        if (engine->hasException)
            return nullptr;
        if (dependentModuleUnit.compiled)
            dependentModuleUnit.compiled->instantiate(engine);
    }

    ScopedString importName(scope);

    const uint importCount = data->importEntryTableSize;
    if (importCount > 0) {
        imports = new const StaticValue *[importCount];
        memset(imports, 0, importCount * sizeof(StaticValue *));
    }
    for (uint i = 0; i < importCount; ++i) {
        const CompiledData::ImportEntry &entry = data->importEntryTable()[i];
        QUrl url = urlAt(entry.moduleRequest);
        importName = runtimeStrings[entry.importName];

        const auto module = engine->loadModule(url, this);
        if (module.compiled) {
            const Value *valuePtr = module.compiled->resolveExport(importName);
            if (!valuePtr) {
                QString referenceErrorMessage = QStringLiteral("Unable to resolve import reference ");
                referenceErrorMessage += importName->toQString();
                engine->throwReferenceError(
                        referenceErrorMessage, fileName(),
                        entry.location.line(), entry.location.column());
                return nullptr;
            }
            imports[i] = valuePtr;
        } else if (Value *value = module.native) {
            const QString name = importName->toQString();
            if (value->isNullOrUndefined()) {
                QString errorMessage = name;
                errorMessage += QStringLiteral(" from ");
                errorMessage += url.toString();
                errorMessage += QStringLiteral(" is null");
                engine->throwError(errorMessage);
                return nullptr;
            }

            if (name == QStringLiteral("default")) {
                imports[i] = value;
            } else {
                url.setFragment(name);
                const auto fragment = engine->moduleForUrl(url, this);
                if (fragment.native) {
                    imports[i] = fragment.native;
                } else {
                    Scope scope(this->engine);
                    ScopedObject o(scope, value);
                    if (!o) {
                        QString referenceErrorMessage = QStringLiteral("Unable to resolve import reference ");
                        referenceErrorMessage += name;
                        referenceErrorMessage += QStringLiteral(" because ");
                        referenceErrorMessage += url.toString(QUrl::RemoveFragment);
                        referenceErrorMessage += QStringLiteral(" is not an object");
                        engine->throwReferenceError(
                                referenceErrorMessage, fileName(),
                                entry.location.line(), entry.location.column());
                        return nullptr;
                    }

                    const ScopedPropertyKey key(scope, scope.engine->identifierTable->asPropertyKey(name));
                    const ScopedValue result(scope, o->get(key));
                    imports[i] = engine->registerNativeModule(url, result);
                }
            }
        }
    }

    const auto throwReferenceError = [&](const CompiledData::ExportEntry &entry, const QString &importName) {
        QString referenceErrorMessage = QStringLiteral("Unable to resolve re-export reference ");
        referenceErrorMessage += importName;
        engine->throwReferenceError(
                referenceErrorMessage, fileName(),
                entry.location.line(), entry.location.column());
    };

    for (uint i = 0; i < data->indirectExportEntryTableSize; ++i) {
        const CompiledData::ExportEntry &entry = data->indirectExportEntryTable()[i];
        auto dependentModule = engine->loadModule(urlAt(entry.moduleRequest), this);
        ScopedString importName(scope, runtimeStrings[entry.importName]);
        if (const auto dependentModuleUnit = dependentModule.compiled) {
            if (!dependentModuleUnit->resolveExport(importName)) {
                throwReferenceError(entry, importName->toQString());
                return nullptr;
            }
        } else if (const auto native = dependentModule.native) {
            ScopedObject o(scope, native);
            const ScopedPropertyKey key(scope, scope.engine->identifierTable->asPropertyKey(importName));
            const ScopedValue result(scope, o->get(key));
            if (result->isUndefined()) {
                throwReferenceError(entry, importName->toQString());
                return nullptr;
            }
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
        QUrl request = urlAt(indirectExport->moduleRequest);
        auto dependentModule = engine->loadModule(request, this);
        ScopedString importName(scope, runtimeStrings[indirectExport->importName]);
        if (dependentModule.compiled) {
            return dependentModule.compiled->resolveExportRecursively(importName, resolveSet);
        } else if (dependentModule.native) {
            if (exportName->toQString() == QLatin1String("*"))
                return dependentModule.native;
            if (exportName->toQString() == QLatin1String("default"))
                return nullptr;

            request.setFragment(importName->toQString());
            const auto fragment = engine->moduleForUrl(request);
            if (fragment.native)
                return fragment.native;

            ScopedObject o(scope, dependentModule.native);
            if (o)
                return engine->registerNativeModule(request, o->get(importName));

            return nullptr;
        } else {
            return nullptr;
        }
    }

    if (exportName->toQString() == QLatin1String("default"))
        return nullptr;

    const Value *starResolution = nullptr;

    for (uint i = 0; i < data->starExportEntryTableSize; ++i) {
        const CompiledData::ExportEntry &entry = data->starExportEntryTable()[i];
        QUrl request = urlAt(entry.moduleRequest);
        auto dependentModule = engine->loadModule(request, this);
        const Value *resolution = nullptr;
        if (dependentModule.compiled) {
            resolution = dependentModule.compiled->resolveExportRecursively(
                        exportName, resolveSet);
        } else if (dependentModule.native) {
            if (exportName->toQString() == QLatin1String("*")) {
                resolution = dependentModule.native;
            } else if (exportName->toQString() != QLatin1String("default")) {
                request.setFragment(exportName->toQString());
                const auto fragment = engine->moduleForUrl(request);
                if (fragment.native) {
                    resolution = fragment.native;
                } else {
                    ScopedObject o(scope, dependentModule.native);
                    if (o)
                        resolution = engine->registerNativeModule(request, o->get(exportName));
                }
            }
        }

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
        auto dependentModule = engine->loadModule(urlAt(entry.moduleRequest), this);
        if (dependentModule.compiled) {
            dependentModule.compiled->getExportedNamesRecursively(
                        names, exportNameSet, /*includeDefaultExport*/false);
        } else if (dependentModule.native) {
            Scope scope(engine);
            ScopedObject o(scope, dependentModule.native);
            ObjectIterator iterator(scope, o, ObjectIterator::EnumerableOnly);
            while (true) {
                ScopedValue val(scope, iterator.nextPropertyNameAsString());
                if (val->isNull())
                    break;
                append(val->toQString());
            }
        }
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
        auto dependentModule = engine->loadModule(QUrl(request), this);
        if (dependentModule.native)
            continue;

        if (engine->hasException)
            return;

        Q_ASSERT(dependentModule.compiled);
        dependentModule.compiled->evaluate();
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
    auto cacheFile = std::make_unique<CompilationUnitMapper>();

    const QStringList cachePaths = { sourcePath + QLatin1Char('c'), localCacheFilePath(url) };
    for (const QString &cachePath : cachePaths) {
        CompiledData::Unit *mappedUnit = cacheFile->get(cachePath, sourceTimeStamp, errorString);
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

        if (data->sourceFileIndex != 0) {
            if (data->sourceFileIndex >= data->stringTableSize + dynamicStrings.size()) {
                *errorString = QStringLiteral("QML source file index is invalid.");
                continue;
            }
            if (sourcePath != QQmlFile::urlToLocalFileOrQrc(stringAt(data->sourceFileIndex))) {
                *errorString = QStringLiteral("QML source file has moved to a different location.");
                continue;
            }
        }

        dataPtrRevert.dismiss();
        free(const_cast<CompiledData::Unit*>(oldDataPtr));
        backingFile = std::move(cacheFile);
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
        const QString cachePath = localCacheFilePath(unitUrl);
        if (CompiledData::SaveableUnitPointer::writeDataToFile(
                    cachePath, data, size, errorString)) {
            CompilationUnitMapper::invalidate(cachePath);
            return true;
        }

        return false;
    });
}

/*!
    \internal
    This function creates a temporary key vector and sorts it to guarantuee a stable
    hash. This is used to calculate a check-sum on dependent meta-objects.
 */
bool ResolvedTypeReferenceMap::addToHash(
        QCryptographicHash *hash, QHash<quintptr, QByteArray> *checksums) const
{
    std::vector<int> keys (size());
    int i = 0;
    for (auto it = constBegin(), end = constEnd(); it != end; ++it) {
        keys[i] = it.key();
        ++i;
    }
    std::sort(keys.begin(), keys.end());
    for (int key: keys) {
        if (!this->operator[](key)->addToHash(hash, checksums))
            return false;
    }

    return true;
}

QString ExecutableCompilationUnit::bindingValueAsString(const CompiledData::Binding *binding) const
{
#if QT_CONFIG(translation)
    using namespace CompiledData;
    bool byId = false;
    switch (binding->type()) {
    case Binding::Type_TranslationById:
        byId = true;
        Q_FALLTHROUGH();
    case Binding::Type_Translation: {
        return translateFrom({ binding->value.translationDataIndex, byId });
    }
    default:
        break;
    }
#endif
    return CompilationUnit::bindingValueAsString(binding);
}

QString ExecutableCompilationUnit::translateFrom(TranslationDataIndex index) const
{
#if !QT_CONFIG(translation)
    return QString();
#else
    const CompiledData::TranslationData &translation = data->translations()[index.index];

    if (index.byId) {
        QByteArray id = stringAt(translation.stringIndex).toUtf8();
        return qtTrId(id.constData(), translation.number);
    }

    const auto fileContext = [this]() {
        // This code must match that in the qsTr() implementation
        const QString &path = fileName();
        int lastSlash = path.lastIndexOf(QLatin1Char('/'));

        QStringView context = (lastSlash > -1)
                ? QStringView{ path }.mid(lastSlash + 1, path.size() - lastSlash - 5)
                : QStringView();
        return context.toUtf8();
    };

    const bool hasContext
            = translation.contextIndex != QV4::CompiledData::TranslationData::NoContextIndex;
    QByteArray comment = stringAt(translation.commentIndex).toUtf8();
    QByteArray text = stringAt(translation.stringIndex).toUtf8();
    return QCoreApplication::translate(
            hasContext ? stringAt(translation.contextIndex).toUtf8() : fileContext(),
            text, comment, translation.number);
#endif
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

#if defined(QML_COMPILE_HASH) && defined(QML_COMPILE_HASH_LENGTH) && QML_COMPILE_HASH_LENGTH > 0
    if (qstrncmp(qml_compile_hash, unit->libraryVersionHash, QML_COMPILE_HASH_LENGTH) != 0) {
        *errorString = QStringLiteral("QML compile hashes don't match. Found %1 expected %2")
                .arg(QString::fromLatin1(
                         QByteArray(unit->libraryVersionHash, QML_COMPILE_HASH_LENGTH)
                         .toPercentEncoding()),
                     QString::fromLatin1(
                         QByteArray(qml_compile_hash, QML_COMPILE_HASH_LENGTH)
                         .toPercentEncoding()));
        return false;
    }
#else
#error "QML_COMPILE_HASH must be defined for the build of QtDeclarative to ensure version checking for cache files"
#endif
    return true;
}

} // namespace QV4

QT_END_NAMESPACE
