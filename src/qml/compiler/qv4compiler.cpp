// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qv4compiler_p.h>
#include <qv4codegen_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qv4staticvalue_p.h>
#include <private/qv4alloca_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsast_p.h>
#include <private/qml_compile_hash_p.h>
#include <private/qqmlirbuilder_p.h>
#include <QCryptographicHash>
#include <QtEndian>

// Efficient implementation that takes advantage of powers of two.

QT_BEGIN_NAMESPACE
namespace QtPrivate { // Disambiguate from WTF::roundUpToMultipleOf
static inline size_t roundUpToMultipleOf(size_t divisor, size_t x)
{
    Q_ASSERT(divisor && !(divisor & (divisor - 1)));
    const size_t remainderMask = divisor - 1;
    return (x + remainderMask) & ~remainderMask;
}
}
QT_END_NAMESPACE

QV4::Compiler::StringTableGenerator::StringTableGenerator()
{
    clear();
}

int QV4::Compiler::StringTableGenerator::registerString(const QString &str)
{
    Q_ASSERT(!frozen);
    QHash<QString, int>::ConstIterator it = stringToId.constFind(str);
    if (it != stringToId.cend())
        return *it;
    stringToId.insert(str, strings.size());
    strings.append(str);
    stringDataSize += QV4::CompiledData::String::calculateSize(str);
    return strings.size() - 1;
}

int QV4::Compiler::StringTableGenerator::getStringId(const QString &string) const
{
    Q_ASSERT(stringToId.contains(string));
    return stringToId.value(string);
}

void QV4::Compiler::StringTableGenerator::clear()
{
    strings.clear();
    stringToId.clear();
    stringDataSize = 0;
    frozen = false;
}

void QV4::Compiler::StringTableGenerator::initializeFromBackingUnit(const QV4::CompiledData::Unit *unit)
{
    clear();
    for (uint i = 0; i < unit->stringTableSize; ++i)
        registerString(unit->stringAtInternal(i));
    backingUnitTableSize = unit->stringTableSize;
    stringDataSize = 0;
}

void QV4::Compiler::StringTableGenerator::serialize(CompiledData::Unit *unit)
{
    char *dataStart = reinterpret_cast<char *>(unit);
    quint32_le *stringTable = reinterpret_cast<quint32_le *>(dataStart + unit->offsetToStringTable);
    char *stringData = reinterpret_cast<char *>(stringTable)
                       + QtPrivate::roundUpToMultipleOf(8, unit->stringTableSize * sizeof(uint));
    for (int i = backingUnitTableSize ; i < strings.size(); ++i) {
        const int index = i - backingUnitTableSize;
        stringTable[index] = stringData - dataStart;
        const QString &qstr = strings.at(i);

        QV4::CompiledData::String *s = reinterpret_cast<QV4::CompiledData::String *>(stringData);
        Q_ASSERT(reinterpret_cast<uintptr_t>(s) % alignof(QV4::CompiledData::String) == 0);
        Q_ASSERT(qstr.size() >= 0);
        s->size = qstr.size();

        ushort *uc = reinterpret_cast<ushort *>(reinterpret_cast<char *>(s) + sizeof(*s));
        qToLittleEndian<ushort>(qstr.constData(), s->size, uc);
        uc[s->size] = 0;

        stringData += QV4::CompiledData::String::calculateSize(qstr);
    }
}

void QV4::Compiler::JSUnitGenerator::generateUnitChecksum(QV4::CompiledData::Unit *unit)
{
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    QCryptographicHash hash(QCryptographicHash::Md5);

    const int checksummableDataOffset
            = offsetof(QV4::CompiledData::Unit, md5Checksum) + sizeof(unit->md5Checksum);

    const char *dataPtr = reinterpret_cast<const char *>(unit) + checksummableDataOffset;
    hash.addData({dataPtr, qsizetype(unit->unitSize - checksummableDataOffset)});

    QByteArray checksum = hash.result();
    Q_ASSERT(checksum.size() == sizeof(unit->md5Checksum));
    memcpy(unit->md5Checksum, checksum.constData(), sizeof(unit->md5Checksum));
#else
    memset(unit->md5Checksum, 0, sizeof(unit->md5Checksum));
#endif
}

QV4::Compiler::JSUnitGenerator::JSUnitGenerator(QV4::Compiler::Module *module)
    : module(module)
{
    // Make sure the empty string always gets index 0
    registerString(QString());
}

int QV4::Compiler::JSUnitGenerator::registerGetterLookup(const QString &name, LookupMode mode)
{
    return registerGetterLookup(registerString(name), mode);
}

static QV4::CompiledData::Lookup::Mode lookupMode(QV4::Compiler::JSUnitGenerator::LookupMode mode)
{
    return mode == QV4::Compiler::JSUnitGenerator::LookupForCall
            ? QV4::CompiledData::Lookup::Mode_ForCall
            : QV4::CompiledData::Lookup::Mode_ForStorage;
}

int QV4::Compiler::JSUnitGenerator::registerGetterLookup(int nameIndex, LookupMode mode)
{
    lookups << CompiledData::Lookup(
                   CompiledData::Lookup::Type_Getter, lookupMode(mode), nameIndex);
    return lookups.size() - 1;
}

int QV4::Compiler::JSUnitGenerator::registerSetterLookup(const QString &name)
{
    return registerSetterLookup(registerString(name));
}

int QV4::Compiler::JSUnitGenerator::registerSetterLookup(int nameIndex)
{
    lookups << CompiledData::Lookup(
                   CompiledData::Lookup::Type_Setter,
                   CompiledData::Lookup::Mode_ForStorage, nameIndex);
    return lookups.size() - 1;
}

int QV4::Compiler::JSUnitGenerator::registerGlobalGetterLookup(int nameIndex, LookupMode mode)
{
    lookups << CompiledData::Lookup(
                   CompiledData::Lookup::Type_GlobalGetter, lookupMode(mode), nameIndex);
    return lookups.size() - 1;
}

int QV4::Compiler::JSUnitGenerator::registerQmlContextPropertyGetterLookup(
        int nameIndex, LookupMode mode)
{
    lookups << CompiledData::Lookup(
                   CompiledData::Lookup::Type_QmlContextPropertyGetter, lookupMode(mode),
                   nameIndex);
    return lookups.size() - 1;
}

int QV4::Compiler::JSUnitGenerator::registerRegExp(QQmlJS::AST::RegExpLiteral *regexp)
{
    quint32 flags = 0;
    if (regexp->flags & QQmlJS::Lexer::RegExp_Global)
        flags |= CompiledData::RegExp::RegExp_Global;
    if (regexp->flags &  QQmlJS::Lexer::RegExp_IgnoreCase)
        flags |= CompiledData::RegExp::RegExp_IgnoreCase;
    if (regexp->flags &  QQmlJS::Lexer::RegExp_Multiline)
        flags |= CompiledData::RegExp::RegExp_Multiline;
    if (regexp->flags &  QQmlJS::Lexer::RegExp_Unicode)
        flags |= CompiledData::RegExp::RegExp_Unicode;
    if (regexp->flags &  QQmlJS::Lexer::RegExp_Sticky)
        flags |= CompiledData::RegExp::RegExp_Sticky;

    regexps.append(CompiledData::RegExp(flags, registerString(regexp->pattern.toString())));
    return regexps.size() - 1;
}

int QV4::Compiler::JSUnitGenerator::registerConstant(QV4::ReturnedValue v)
{
    int idx = constants.indexOf(v);
    if (idx >= 0)
        return idx;
    constants.append(v);
    return constants.size() - 1;
}

QV4::ReturnedValue QV4::Compiler::JSUnitGenerator::constant(int idx) const
{
    return constants.at(idx);
}

int QV4::Compiler::JSUnitGenerator::registerJSClass(const QStringList &members)
{
    // ### re-use existing class definitions.

    const int size = CompiledData::JSClass::calculateSize(members.size());
    jsClassOffsets.append(jsClassData.size());
    const int oldSize = jsClassData.size();
    jsClassData.resize(jsClassData.size() + size);
    memset(jsClassData.data() + oldSize, 0, size);

    CompiledData::JSClass *jsClass = reinterpret_cast<CompiledData::JSClass*>(jsClassData.data() + oldSize);
    jsClass->nMembers = members.size();
    CompiledData::JSClassMember *member = reinterpret_cast<CompiledData::JSClassMember*>(jsClass + 1);

    for (const auto &name : members) {
        member->set(registerString(name), false);
        ++member;
    }

    return jsClassOffsets.size() - 1;
}

int QV4::Compiler::JSUnitGenerator::registerTranslation(const QV4::CompiledData::TranslationData &translation)
{
    translations.append(translation);
    return translations.size() - 1;
}

QV4::CompiledData::Unit *QV4::Compiler::JSUnitGenerator::generateUnit(GeneratorOption option)
{
    const auto registerTypeStrings = [this](QQmlJS::AST::Type *type) {
        if (!type)
            return;

        if (type->typeArgument) {
            registerString(type->typeArgument->toString());
            registerString(type->typeId->toString());
        }
        registerString(type->toString());
    };

    registerString(module->fileName);
    registerString(module->finalUrl);
    for (Context *f : std::as_const(module->functions)) {
        registerString(f->name);
        registerTypeStrings(f->returnType);
        for (int i = 0; i < f->arguments.size(); ++i) {
            registerString(f->arguments.at(i).id);
            if (const QQmlJS::AST::TypeAnnotation *annotation
                    = f->arguments.at(i).typeAnnotation.data()) {
                registerTypeStrings(annotation->type);
            }
        }
        for (int i = 0; i < f->locals.size(); ++i)
            registerString(f->locals.at(i));
    }
    for (Context *c : std::as_const(module->blocks)) {
        for (int i = 0; i < c->locals.size(); ++i)
            registerString(c->locals.at(i));
    }
    {
        const auto registerExportEntry = [this](const Compiler::ExportEntry &entry) {
            registerString(entry.exportName);
            registerString(entry.moduleRequest);
            registerString(entry.importName);
            registerString(entry.localName);
        };
        std::for_each(module->localExportEntries.constBegin(), module->localExportEntries.constEnd(), registerExportEntry);
        std::for_each(module->indirectExportEntries.constBegin(), module->indirectExportEntries.constEnd(), registerExportEntry);
        std::for_each(module->starExportEntries.constBegin(), module->starExportEntries.constEnd(), registerExportEntry);
    }
    {
        for (const auto &entry: module->importEntries) {
            registerString(entry.moduleRequest);
            registerString(entry.importName);
            registerString(entry.localName);
        }

        for (const QString &request: module->moduleRequests)
            registerString(request);
    }

    Q_ALLOCA_VAR(quint32_le, blockClassAndFunctionOffsets, (module->functions.size() + module->classes.size() + module->templateObjects.size() + module->blocks.size()) * sizeof(quint32_le));
    uint jsClassDataOffset = 0;

    char *dataPtr;
    CompiledData::Unit *unit;
    {
        QV4::CompiledData::Unit tempHeader = generateHeader(option, blockClassAndFunctionOffsets, &jsClassDataOffset);
        dataPtr = reinterpret_cast<char *>(malloc(tempHeader.unitSize));
        memset(dataPtr, 0, tempHeader.unitSize);
        memcpy(&unit, &dataPtr, sizeof(CompiledData::Unit*));
        memcpy(unit, &tempHeader, sizeof(tempHeader));
    }

    memcpy(dataPtr + unit->offsetToFunctionTable, blockClassAndFunctionOffsets, unit->functionTableSize * sizeof(quint32_le));
    memcpy(dataPtr + unit->offsetToClassTable, blockClassAndFunctionOffsets + unit->functionTableSize, unit->classTableSize * sizeof(quint32_le));
    memcpy(dataPtr + unit->offsetToTemplateObjectTable, blockClassAndFunctionOffsets + unit->functionTableSize + unit->classTableSize, unit->templateObjectTableSize * sizeof(quint32_le));
    memcpy(dataPtr + unit->offsetToBlockTable, blockClassAndFunctionOffsets + unit->functionTableSize + unit->classTableSize + unit->templateObjectTableSize, unit->blockTableSize * sizeof(quint32_le));

    for (int i = 0; i < module->functions.size(); ++i) {
        Context *function = module->functions.at(i);
        if (function == module->rootContext)
            unit->indexOfRootFunction = i;

        writeFunction(dataPtr + blockClassAndFunctionOffsets[i], function);
    }

    for (int i = 0; i < module->classes.size(); ++i) {
        const Class &c = module->classes.at(i);

        writeClass(dataPtr + blockClassAndFunctionOffsets[i + module->functions.size()], c);
    }

    for (int i = 0; i < module->templateObjects.size(); ++i) {
        const TemplateObject &t = module->templateObjects.at(i);

        writeTemplateObject(dataPtr + blockClassAndFunctionOffsets[i + module->functions.size() + module->classes.size()], t);
    }

    for (int i = 0; i < module->blocks.size(); ++i) {
        Context *block = module->blocks.at(i);

        writeBlock(dataPtr + blockClassAndFunctionOffsets[i + module->classes.size() + module->templateObjects.size() + module->functions.size()], block);
    }

    CompiledData::Lookup *lookupsToWrite = reinterpret_cast<CompiledData::Lookup*>(dataPtr + unit->offsetToLookupTable);
    for (const CompiledData::Lookup &l : std::as_const(lookups))
        *lookupsToWrite++ = l;

    CompiledData::RegExp *regexpTable = reinterpret_cast<CompiledData::RegExp *>(dataPtr + unit->offsetToRegexpTable);
    if (regexps.size())
        memcpy(regexpTable, regexps.constData(), regexps.size() * sizeof(*regexpTable));

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    ReturnedValue *constantTable = reinterpret_cast<ReturnedValue *>(dataPtr + unit->offsetToConstantTable);
    if (constants.size())
        memcpy(constantTable, constants.constData(), constants.size() * sizeof(ReturnedValue));
#else
    quint64_le *constantTable = reinterpret_cast<quint64_le *>(dataPtr + unit->offsetToConstantTable);
    for (int i = 0; i < constants.count(); ++i)
        constantTable[i] = constants.at(i);
#endif

    {
        if (jsClassData.size())
            memcpy(dataPtr + jsClassDataOffset, jsClassData.constData(), jsClassData.size());

        // write js classes and js class lookup table
        quint32_le *jsClassOffsetTable = reinterpret_cast<quint32_le *>(dataPtr + unit->offsetToJSClassTable);
        for (int i = 0; i < jsClassOffsets.size(); ++i)
            jsClassOffsetTable[i] = jsClassDataOffset + jsClassOffsets.at(i);
    }

    if (translations.size()) {
        memcpy(dataPtr + unit->offsetToTranslationTable, translations.constData(), translations.size() * sizeof(CompiledData::TranslationData));
    }

    {
        const auto populateExportEntryTable = [this, dataPtr](const QVector<Compiler::ExportEntry> &table, quint32_le offset) {
            CompiledData::ExportEntry *entryToWrite = reinterpret_cast<CompiledData::ExportEntry *>(dataPtr + offset);
            for (const Compiler::ExportEntry &entry: table) {
                entryToWrite->exportName = getStringId(entry.exportName);
                entryToWrite->moduleRequest = getStringId(entry.moduleRequest);
                entryToWrite->importName = getStringId(entry.importName);
                entryToWrite->localName = getStringId(entry.localName);
                entryToWrite->location = entry.location;
                entryToWrite++;
            }
        };
        populateExportEntryTable(module->localExportEntries, unit->offsetToLocalExportEntryTable);
        populateExportEntryTable(module->indirectExportEntries, unit->offsetToIndirectExportEntryTable);
        populateExportEntryTable(module->starExportEntries, unit->offsetToStarExportEntryTable);
    }

    {
        CompiledData::ImportEntry *entryToWrite = reinterpret_cast<CompiledData::ImportEntry *>(dataPtr + unit->offsetToImportEntryTable);
        for (const Compiler::ImportEntry &entry: module->importEntries) {
            entryToWrite->moduleRequest = getStringId(entry.moduleRequest);
            entryToWrite->importName = getStringId(entry.importName);
            entryToWrite->localName = getStringId(entry.localName);
            entryToWrite->location = entry.location;
            entryToWrite++;
        }
    }

    {
        quint32_le *moduleRequestEntryToWrite = reinterpret_cast<quint32_le *>(dataPtr + unit->offsetToModuleRequestTable);
        for (const QString &moduleRequest: module->moduleRequests) {
            *moduleRequestEntryToWrite = getStringId(moduleRequest);
            moduleRequestEntryToWrite++;
        }
    }

    // write strings and string table
    if (option == GenerateWithStringTable)
        stringTable.serialize(unit);

    generateUnitChecksum(unit);

    return unit;
}

void QV4::Compiler::JSUnitGenerator::writeFunction(char *f, QV4::Compiler::Context *irFunction) const
{
    QV4::CompiledData::Function *function = (QV4::CompiledData::Function *)f;

    quint32 currentOffset = static_cast<quint32>(QtPrivate::roundUpToMultipleOf(8, sizeof(*function)));

    function->nameIndex = getStringId(irFunction->name);
    function->flags = 0;
    if (irFunction->isStrict)
        function->flags |= CompiledData::Function::IsStrict;
    if (irFunction->isArrowFunction)
        function->flags |= CompiledData::Function::IsArrowFunction;
    if (irFunction->isGenerator)
        function->flags |= CompiledData::Function::IsGenerator;
    if (irFunction->returnsClosure)
        function->flags |= CompiledData::Function::IsClosureWrapper;

    if (!irFunction->returnsClosure
            || irFunction->innerFunctionAccessesThis
            || irFunction->innerFunctionAccessesNewTarget) {
        // If the inner function does things with this and new.target we need to do some work in
        // the outer function. Then we shouldn't directly access the nested function.
        function->nestedFunctionIndex = std::numeric_limits<uint32_t>::max();
    } else {
        // Otherwise we can directly use the nested function.
        function->nestedFunctionIndex
                = quint32(module->functions.indexOf(irFunction->nestedContexts.first()));
    }

    function->length = irFunction->formals ? irFunction->formals->length() : 0;
    function->nFormals = irFunction->arguments.size();
    function->formalsOffset = currentOffset;
    currentOffset += function->nFormals * sizeof(CompiledData::Parameter);

    const auto idGenerator = [this](const QString &str) { return getStringId(str); };

    QmlIR::Parameter::initType(&function->returnType, idGenerator, irFunction->returnType);

    function->sizeOfLocalTemporalDeadZone = irFunction->sizeOfLocalTemporalDeadZone;
    function->sizeOfRegisterTemporalDeadZone = irFunction->sizeOfRegisterTemporalDeadZone;
    function->firstTemporalDeadZoneRegister = irFunction->firstTemporalDeadZoneRegister;

    function->nLocals = irFunction->locals.size();
    function->localsOffset = currentOffset;
    currentOffset += function->nLocals * sizeof(quint32);

    function->nLineAndStatementNumbers
            = irFunction->lineAndStatementNumberMapping.size();
    Q_ASSERT(function->lineAndStatementNumberOffset() == currentOffset);
    currentOffset += function->nLineAndStatementNumbers
            * sizeof(CompiledData::CodeOffsetToLineAndStatement);

    function->nRegisters = irFunction->registerCountInFunction;

    if (!irFunction->labelInfo.empty()) {
        function->nLabelInfos = quint32(irFunction->labelInfo.size());
        Q_ASSERT(function->labelInfosOffset() == currentOffset);
        currentOffset += function->nLabelInfos * sizeof(quint32);
    }

    function->location.set(irFunction->line, irFunction->column);

    function->codeOffset = currentOffset;
    function->codeSize = irFunction->code.size();

    // write formals
    CompiledData::Parameter *formals = (CompiledData::Parameter *)(f + function->formalsOffset);
    for (int i = 0; i < irFunction->arguments.size(); ++i) {
        auto *formal = &formals[i];
        formal->nameIndex = getStringId(irFunction->arguments.at(i).id);
        QQmlJS::AST::TypeAnnotation *annotation = irFunction->arguments.at(i).typeAnnotation.data();
        QmlIR::Parameter::initType(
                &formal->type, idGenerator, annotation ? annotation->type : nullptr);
    }

    // write locals
    quint32_le *locals = (quint32_le *)(f + function->localsOffset);
    for (int i = 0; i < irFunction->locals.size(); ++i)
        locals[i] = getStringId(irFunction->locals.at(i));

    // write line and statement numbers
    memcpy(f + function->lineAndStatementNumberOffset(),
           irFunction->lineAndStatementNumberMapping.constData(),
           irFunction->lineAndStatementNumberMapping.size()
                * sizeof(CompiledData::CodeOffsetToLineAndStatement));

    quint32_le *labels = (quint32_le *)(f + function->labelInfosOffset());
    for (unsigned u : irFunction->labelInfo) {
        *labels++ = u;
    }

    // write byte code
    memcpy(f + function->codeOffset, irFunction->code.constData(), irFunction->code.size());
}

static_assert(int(QV4::Compiler::Class::Method::Regular) == int(QV4::CompiledData::Method::Regular), "Incompatible layout");
static_assert(int(QV4::Compiler::Class::Method::Getter) == int(QV4::CompiledData::Method::Getter), "Incompatible layout");
static_assert(int(QV4::Compiler::Class::Method::Setter) == int(QV4::CompiledData::Method::Setter), "Incompatible layout");

void QV4::Compiler::JSUnitGenerator::writeClass(char *b, const QV4::Compiler::Class &c)
{
    QV4::CompiledData::Class *cls = reinterpret_cast<QV4::CompiledData::Class *>(b);

    quint32 currentOffset = sizeof(QV4::CompiledData::Class);

    QVector<Class::Method> allMethods = c.staticMethods;
    allMethods += c.methods;

    cls->constructorFunction = c.constructorIndex;
    cls->nameIndex = c.nameIndex;
    cls->nMethods = c.methods.size();
    cls->nStaticMethods = c.staticMethods.size();
    cls->methodTableOffset = currentOffset;
    CompiledData::Method *method = reinterpret_cast<CompiledData::Method *>(b + currentOffset);

    // write methods
    for (int i = 0; i < allMethods.size(); ++i) {
        method->name = allMethods.at(i).nameIndex;
        method->type = allMethods.at(i).type;
        method->function = allMethods.at(i).functionIndex;
        ++method;
    }

    static const bool showCode = qEnvironmentVariableIsSet("QV4_SHOW_BYTECODE");
    if (showCode) {
        qDebug() << "=== Class" << stringForIndex(cls->nameIndex) << "static methods"
                 << cls->nStaticMethods << "methods" << cls->nMethods;
        qDebug() << "    constructor:" << cls->constructorFunction;
        for (uint i = 0; i < cls->nStaticMethods + cls->nMethods; ++i) {
            QDebug output = qDebug().nospace();
            output << "    " << i << ": ";
            if (i < cls->nStaticMethods)
                output << "static ";
            switch (cls->methodTable()[i].type) {
            case CompiledData::Method::Getter:
                output << "get "; break;
            case CompiledData::Method::Setter:
                output << "set "; break;
            default:
                break;
            }
            output << stringForIndex(cls->methodTable()[i].name) << " "
                   << cls->methodTable()[i].function;
        }
        qDebug().space();
    }
}

void QV4::Compiler::JSUnitGenerator::writeTemplateObject(char *b, const QV4::Compiler::TemplateObject &t)
{
    QV4::CompiledData::TemplateObject *tmpl = reinterpret_cast<QV4::CompiledData::TemplateObject *>(b);
    tmpl->size = t.strings.size();

    quint32 currentOffset = sizeof(QV4::CompiledData::TemplateObject);

    quint32_le *strings = reinterpret_cast<quint32_le *>(b + currentOffset);

    // write methods
    for (int i = 0; i < t.strings.size(); ++i)
        strings[i] = t.strings.at(i);
    strings += t.strings.size();

    for (int i = 0; i < t.rawStrings.size(); ++i)
        strings[i] = t.rawStrings.at(i);

    static const bool showCode = qEnvironmentVariableIsSet("QV4_SHOW_BYTECODE");
    if (showCode) {
        qDebug() << "=== TemplateObject size" << tmpl->size;
        for (uint i = 0; i < tmpl->size; ++i) {
            qDebug() << "    " << i << stringForIndex(tmpl->stringIndexAt(i));
            qDebug() << "        raw: " << stringForIndex(tmpl->rawStringIndexAt(i));
        }
        qDebug();
    }
}

void QV4::Compiler::JSUnitGenerator::writeBlock(char *b, QV4::Compiler::Context *irBlock) const
{
    QV4::CompiledData::Block *block = reinterpret_cast<QV4::CompiledData::Block *>(b);

    quint32 currentOffset = static_cast<quint32>(QtPrivate::roundUpToMultipleOf(8, sizeof(*block)));

    block->sizeOfLocalTemporalDeadZone = irBlock->sizeOfLocalTemporalDeadZone;
    block->nLocals = irBlock->locals.size();
    block->localsOffset = currentOffset;
    currentOffset += block->nLocals * sizeof(quint32);

    // write locals
    quint32_le *locals = (quint32_le *)(b + block->localsOffset);
    for (int i = 0; i < irBlock->locals.size(); ++i)
        locals[i] = getStringId(irBlock->locals.at(i));

    static const bool showCode = qEnvironmentVariableIsSet("QV4_SHOW_BYTECODE");
    if (showCode) {
        qDebug() << "=== Variables for block" << irBlock->blockIndex;
        for (int i = 0; i < irBlock->locals.size(); ++i)
            qDebug() << "    " << i << ":" << locals[i];
        qDebug();
    }
}

QV4::CompiledData::Unit QV4::Compiler::JSUnitGenerator::generateHeader(QV4::Compiler::JSUnitGenerator::GeneratorOption option, quint32_le *blockAndFunctionOffsets, uint *jsClassDataOffset)
{
    CompiledData::Unit unit;
    memset(&unit, 0, sizeof(unit));
    memcpy(unit.magic, CompiledData::magic_str, sizeof(unit.magic));
    unit.flags = QV4::CompiledData::Unit::IsJavascript;
    unit.flags |= module->unitFlags;
    unit.version = QV4_DATA_STRUCTURE_VERSION;
    unit.qtVersion = QT_VERSION;
    qstrcpy(unit.libraryVersionHash, QML_COMPILE_HASH);
    memset(unit.md5Checksum, 0, sizeof(unit.md5Checksum));
    memset(unit.dependencyMD5Checksum, 0, sizeof(unit.dependencyMD5Checksum));

    quint32 nextOffset = sizeof(CompiledData::Unit);

    unit.functionTableSize = module->functions.size();
    unit.offsetToFunctionTable = nextOffset;
    nextOffset += unit.functionTableSize * sizeof(uint);

    unit.classTableSize = module->classes.size();
    unit.offsetToClassTable = nextOffset;
    nextOffset += unit.classTableSize * sizeof(uint);

    unit.templateObjectTableSize = module->templateObjects.size();
    unit.offsetToTemplateObjectTable = nextOffset;
    nextOffset += unit.templateObjectTableSize * sizeof(uint);

    unit.blockTableSize = module->blocks.size();
    unit.offsetToBlockTable = nextOffset;
    nextOffset += unit.blockTableSize * sizeof(uint);

    unit.lookupTableSize = lookups.size();
    unit.offsetToLookupTable = nextOffset;
    nextOffset += unit.lookupTableSize * sizeof(CompiledData::Lookup);

    unit.regexpTableSize = regexps.size();
    unit.offsetToRegexpTable = nextOffset;
    nextOffset += unit.regexpTableSize * sizeof(CompiledData::RegExp);

    unit.constantTableSize = constants.size();

    // Ensure we load constants from well-aligned addresses into for example SSE registers.
    nextOffset = static_cast<quint32>(QtPrivate::roundUpToMultipleOf(16, nextOffset));
    unit.offsetToConstantTable = nextOffset;
    nextOffset += unit.constantTableSize * sizeof(ReturnedValue);

    unit.jsClassTableSize = jsClassOffsets.size();
    unit.offsetToJSClassTable = nextOffset;
    nextOffset += unit.jsClassTableSize * sizeof(uint);

    *jsClassDataOffset = nextOffset;
    nextOffset += jsClassData.size();

    nextOffset = static_cast<quint32>(QtPrivate::roundUpToMultipleOf(8, nextOffset));

    unit.translationTableSize = translations.size();
    unit.offsetToTranslationTable = nextOffset;
    nextOffset += unit.translationTableSize * sizeof(CompiledData::TranslationData);

    nextOffset = static_cast<quint32>(QtPrivate::roundUpToMultipleOf(8, nextOffset));

    const auto reserveExportTable = [&nextOffset](int count, quint32_le *tableSizePtr, quint32_le *offsetPtr) {
        *tableSizePtr = count;
        *offsetPtr = nextOffset;
        nextOffset += count * sizeof(CompiledData::ExportEntry);
        nextOffset = static_cast<quint32>(QtPrivate::roundUpToMultipleOf(8, nextOffset));
    };

    reserveExportTable(module->localExportEntries.size(), &unit.localExportEntryTableSize, &unit.offsetToLocalExportEntryTable);
    reserveExportTable(module->indirectExportEntries.size(), &unit.indirectExportEntryTableSize, &unit.offsetToIndirectExportEntryTable);
    reserveExportTable(module->starExportEntries.size(), &unit.starExportEntryTableSize, &unit.offsetToStarExportEntryTable);

    unit.importEntryTableSize = module->importEntries.size();
    unit.offsetToImportEntryTable = nextOffset;
    nextOffset += unit.importEntryTableSize * sizeof(CompiledData::ImportEntry);
    nextOffset = static_cast<quint32>(QtPrivate::roundUpToMultipleOf(8, nextOffset));

    unit.moduleRequestTableSize = module->moduleRequests.size();
    unit.offsetToModuleRequestTable = nextOffset;
    nextOffset += unit.moduleRequestTableSize * sizeof(uint);
    nextOffset = static_cast<quint32>(QtPrivate::roundUpToMultipleOf(8, nextOffset));

    quint32 functionSize = 0;
    for (int i = 0; i < module->functions.size(); ++i) {
        Context *f = module->functions.at(i);
        blockAndFunctionOffsets[i] = nextOffset;

        quint32 size = QV4::CompiledData::Function::calculateSize(
                    f->arguments.size(), f->locals.size(), f->lineAndStatementNumberMapping.size(),
                    f->nestedContexts.size(), int(f->labelInfo.size()), f->code.size());
        functionSize += size - f->code.size();
        nextOffset += size;
    }

    blockAndFunctionOffsets += module->functions.size();

    for (int i = 0; i < module->classes.size(); ++i) {
        const Class &c = module->classes.at(i);
        blockAndFunctionOffsets[i] = nextOffset;

        nextOffset += QV4::CompiledData::Class::calculateSize(c.staticMethods.size(), c.methods.size());
    }
    blockAndFunctionOffsets += module->classes.size();

    for (int i = 0; i < module->templateObjects.size(); ++i) {
        const TemplateObject &t = module->templateObjects.at(i);
        blockAndFunctionOffsets[i] = nextOffset;

        nextOffset += QV4::CompiledData::TemplateObject::calculateSize(t.strings.size());
    }
    blockAndFunctionOffsets += module->templateObjects.size();

    for (int i = 0; i < module->blocks.size(); ++i) {
        Context *c = module->blocks.at(i);
        blockAndFunctionOffsets[i] = nextOffset;

        nextOffset += QV4::CompiledData::Block::calculateSize(c->locals.size());
    }

    if (option == GenerateWithStringTable) {
        unit.stringTableSize = stringTable.stringCount();
        nextOffset = static_cast<quint32>(QtPrivate::roundUpToMultipleOf(8, nextOffset));
        unit.offsetToStringTable = nextOffset;
        nextOffset += stringTable.sizeOfTableAndData();
    } else {
        unit.stringTableSize = 0;
        unit.offsetToStringTable = 0;
    }
    unit.indexOfRootFunction = -1;
    unit.sourceFileIndex = getStringId(module->fileName);
    unit.finalUrlIndex = getStringId(module->finalUrl);
    unit.sourceTimeStamp = module->sourceTimeStamp.isValid() ? module->sourceTimeStamp.toMSecsSinceEpoch() : 0;
    unit.offsetToQmlUnit = 0;

    unit.unitSize = nextOffset;

    static const bool showStats = qEnvironmentVariableIsSet("QML_SHOW_UNIT_STATS");
    if (showStats) {
        qDebug() << "Generated JS unit that is" << unit.unitSize << "bytes contains:";
        qDebug() << "    " << functionSize << "bytes for non-code function data for" << unit.functionTableSize << "functions";
        qDebug() << "    " << translations.size() * sizeof(CompiledData::TranslationData) << "bytes for" << translations.size() << "translations";
    }

    return unit;
}
