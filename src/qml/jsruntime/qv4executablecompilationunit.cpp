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
#include <private/qqmltypewrapper_p.h>
#include <private/qv4resolvedtypereference_p.h>
#include <private/qv4objectiterator_p.h>

#include <QtQml/qqmlpropertymap.h>

#include <QtCore/qfileinfo.h>
#include <QtCore/qcryptographichash.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

ExecutableCompilationUnit::ExecutableCompilationUnit() = default;

ExecutableCompilationUnit::ExecutableCompilationUnit(
        QQmlRefPointer<CompiledData::CompilationUnit> &&compilationUnit)
    : m_compilationUnit(std::move(compilationUnit))
{
    constants = m_compilationUnit->constants;
}

ExecutableCompilationUnit::~ExecutableCompilationUnit()
{
    if (engine)
        clear();
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

void ExecutableCompilationUnit::populate()
{
    /* In general, we should use QV4::Scope whenever we allocate heap objects, and employ write barriers
       for member variables pointing to heap objects. However, ExecutableCompilationUnit is special, as it
       is always part of the root set. So instead of using scopde allocations and write barriers, we use a
       slightly different approach: We temporarily block the gc from running. Afterwards, at the end of the
       function we check whether the gc was already running, and mark the ExecutableCompilationUnit. This
       ensures that all the newly allocated objects of the compilation unit will be marked in turn.
       If the gc was not running, we don't have to do anything, because everything will be marked when the
       gc starts marking the root set at the start of a run.
     */
    const CompiledData::Unit *data = m_compilationUnit->data;
    GCCriticalSection<ExecutableCompilationUnit> criticalSection(engine, this);

    Q_ASSERT(!runtimeStrings);
    Q_ASSERT(engine);
    Q_ASSERT(data);
    const quint32 stringCount = totalStringCount();
    runtimeStrings = (QV4::Heap::String **)calloc(stringCount, sizeof(QV4::Heap::String*));
    for (uint i = 0; i < stringCount; ++i)
        runtimeStrings[i] = engine->newString(stringAt(i));

    runtimeRegularExpressions
            = new QV4::Value[data->regexpTableSize];
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
            = ignoreAotCompiledFunctions ? nullptr : m_compilationUnit->aotCompiledFunctions;

    auto advanceAotFunction = [&](int i) -> const QQmlPrivate::AOTCompiledFunction * {
        if (aotFunction) {
            if (aotFunction->functionPtr) {
                if (aotFunction->functionIndex == i)
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
}

Heap::Object *ExecutableCompilationUnit::templateObjectAt(int index) const
{
    const CompiledData::Unit *data = m_compilationUnit->data;
    Q_ASSERT(data);
    Q_ASSERT(engine);

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

void ExecutableCompilationUnit::clear()
{
    delete [] imports;
    imports = nullptr;

    if (runtimeLookups) {
        const uint lookupTableSize = unitData()->lookupTableSize;
        for (uint i = 0; i < lookupTableSize; ++i)
            runtimeLookups[i].releasePropertyCache();
    }

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

void ExecutableCompilationUnit::markObjects(QV4::MarkStack *markStack) const
{
    const CompiledData::Unit *data = m_compilationUnit->data;

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

    if (Heap::Base *v = m_valueOrModule.heapObject())
        v->mark(markStack);
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

QQmlRefPointer<ExecutableCompilationUnit> ExecutableCompilationUnit::create(
        QQmlRefPointer<CompiledData::CompilationUnit> &&compilationUnit, ExecutionEngine *engine)
{
    auto result = QQmlRefPointer<ExecutableCompilationUnit>(
            new ExecutableCompilationUnit(std::move(compilationUnit)),
            QQmlRefPointer<ExecutableCompilationUnit>::Adopt);
    result->engine = engine;
    return result;
}

Heap::Module *ExecutableCompilationUnit::instantiate()
{
    const CompiledData::Unit *data = m_compilationUnit->data;

    if (isESModule() && module())
        return module();

    if (data->indexOfRootFunction < 0)
        return nullptr;

    Q_ASSERT(engine);
    if (!runtimeStrings)
        populate();

    Scope scope(engine);
    Scoped<Module> module(scope, engine->memoryManager->allocate<Module>(engine, this));

    if (isESModule())
        setModule(module->d());

    const QStringList moduleRequests = m_compilationUnit->moduleRequests();
    for (const QString &request: moduleRequests) {
        const QUrl url(request);
        const auto dependentModuleUnit = engine->loadModule(url, this);
        if (engine->hasException)
            return nullptr;
        if (dependentModuleUnit)
            dependentModuleUnit->instantiate();
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

        if (const auto module = engine->loadModule(url, this)) {
            const Value *valuePtr = module->resolveExport(importName);
            if (!valuePtr) {
                QString referenceErrorMessage = QStringLiteral("Unable to resolve import reference ");
                referenceErrorMessage += importName->toQString();
                QV4::ScopedValue compiledValue(scope, module->value());
                engine->throwReferenceError(
                        referenceErrorMessage, fileName(),
                        entry.location.line(), entry.location.column());
                return nullptr;
            }
            imports[i] = valuePtr;
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
        if (auto dependentModule = engine->loadModule(urlAt(entry.moduleRequest), this)) {
            ScopedString importName(scope, runtimeStrings[entry.importName]);
            if (!dependentModule->resolveExport(importName)) {
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

    const CompiledData::Unit *data = m_compilationUnit->data;

    Q_ASSERT(data);
    Q_ASSERT(engine);

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
        if (auto dependentModule = engine->loadModule(request, this)) {
            ScopedString importName(scope, runtimeStrings[indirectExport->importName]);
            return dependentModule->resolveExportRecursively(importName, resolveSet);
        }
        return nullptr;
    }

    if (exportName->toQString() == QLatin1String("default"))
        return nullptr;

    const Value *starResolution = nullptr;

    for (uint i = 0; i < data->starExportEntryTableSize; ++i) {
        const CompiledData::ExportEntry &entry = data->starExportEntryTable()[i];
        QUrl request = urlAt(entry.moduleRequest);
        const Value *resolution = nullptr;
        if (auto dependentModule = engine->loadModule(request, this))
            resolution = dependentModule->resolveExportRecursively(exportName, resolveSet);

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

    const CompiledData::Unit *data = m_compilationUnit->data;

    Q_ASSERT(data);
    Q_ASSERT(engine);

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
        if (auto dependentModule = engine->loadModule(urlAt(entry.moduleRequest), this)) {
            dependentModule->getExportedNamesRecursively(
                    names, exportNameSet, /*includeDefaultExport*/false);
        }
    }
}

void ExecutableCompilationUnit::evaluate()
{
    Q_ASSERT(engine);

    QV4::Scope scope(engine);
    QV4::Scoped<Module> mod(scope, module());
    mod->evaluate();
}

void ExecutableCompilationUnit::evaluateModuleRequests()
{
    Q_ASSERT(engine);

    const QStringList moduleRequests = m_compilationUnit->moduleRequests();
    for (const QString &request: moduleRequests) {
        auto dependentModule = engine->loadModule(QUrl(request), this);

        if (engine->hasException)
            return;

        Q_ASSERT(dependentModule);
        dependentModule->evaluate();
        if (engine->hasException)
            return;
    }
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
    return m_compilationUnit->bindingValueAsString(binding);
}

QString ExecutableCompilationUnit::translateFrom(TranslationDataIndex index) const
{
#if !QT_CONFIG(translation)
    return QString();
#else
    const CompiledData::TranslationData &translation = unitData()->translations()[index.index];

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
    QByteArray context;
    if (hasContext) {
        context = stringAt(translation.contextIndex).toUtf8();
    } else {
        auto pragmaTranslationContext = unitData()->translationContextIndex();
        context = stringAt(*pragmaTranslationContext).toUtf8();
        context = context.isEmpty() ? fileContext() : context;
    }

    QByteArray comment = stringAt(translation.commentIndex).toUtf8();
    QByteArray text = stringAt(translation.stringIndex).toUtf8();
    return QCoreApplication::translate(context, text, comment, translation.number);
#endif
}

Heap::Module *ExecutableCompilationUnit::module() const
{
    if (const Module *m = m_valueOrModule.as<QV4::Module>())
        return m->d();
    return nullptr;
}

void ExecutableCompilationUnit::setModule(Heap::Module *module)
{
    m_valueOrModule = module;
}

} // namespace QV4

QT_END_NAMESPACE
