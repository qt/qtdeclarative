// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4script_p.h"
#include <private/qv4mm_p.h>
#include "qv4function_p.h"
#include "qv4context_p.h"
#include "qv4debugging_p.h"
#include "qv4scopedvalue_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlsourcecoordinate_p.h>
#include <private/qv4profiling_p.h>
#include <qv4runtimecodegen_p.h>

#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QScopedValueRollback>

using namespace QV4;
using namespace QQmlJS;

Script::Script(ExecutionEngine *v4, QmlContext *qml, const QQmlRefPointer<ExecutableCompilationUnit> &compilationUnit)
    : line(1), column(0), context(v4->rootContext()), strictMode(false), inheritContext(true), parsed(false)
    , compilationUnit(compilationUnit), parseAsBinding(true)
{
    if (qml)
        qmlContext.set(v4, *qml);

    parsed = true;

    vmFunction.set(v4,
                   compilationUnit ? compilationUnit->rootFunction() : nullptr);
}

Script::~Script()
{
}

void Script::parse()
{
    if (parsed)
        return;

    parsed = true;

    ExecutionEngine *v4 = context->engine();
    Scope valueScope(v4);

    QV4::Compiler::Module module(sourceFile, sourceFile, v4->debugger() != nullptr);

    if (sourceCode.startsWith(QLatin1String("function("))) {
        static const int snippetLength = 70;
        qWarning() << "Warning: Using function expressions as statements in scripts is not compliant with the ECMAScript specification:\n"
                   << (QStringView{sourceCode}.left(snippetLength) + QLatin1String("..."))
                   << "\nThis will throw a syntax error in Qt 5.12. If you want a function expression, surround it by parentheses.";
    }

    Engine ee, *engine = &ee;
    Lexer lexer(engine);
    lexer.setCode(sourceCode, line, parseAsBinding);
    Parser parser(engine);

    const bool parsed = parser.parseProgram();

    const auto diagnosticMessages = parser.diagnosticMessages();
    for (const DiagnosticMessage &m : diagnosticMessages) {
        if (m.isError()) {
            valueScope.engine->throwSyntaxError(m.message, sourceFile, m.loc.startLine, m.loc.startColumn);
            return;
        } else {
            qWarning() << sourceFile << ':' << m.loc.startLine << ':' << m.loc.startColumn
                      << ": warning: " << m.message;
        }
    }

    if (parsed) {
        using namespace AST;
        Program *program = AST::cast<Program *>(parser.rootNode());
        if (!program) {
            // if parsing was successful, and we have no program, then
            // we're done...:
            return;
        }

        QV4::Compiler::JSUnitGenerator jsGenerator(&module);
        RuntimeCodegen cg(v4, &jsGenerator, strictMode);
        if (inheritContext)
            cg.setUseFastLookups(false);
        cg.generateFromProgram(sourceCode, program, &module, contextType);
        if (v4->hasException)
            return;

        compilationUnit = v4->insertCompilationUnit(cg.generateCompilationUnit());
        vmFunction.set(v4, compilationUnit->rootFunction());
    }

    if (!vmFunction) {
        // ### FIX file/line number
        ScopedObject error(valueScope, v4->newSyntaxErrorObject(QStringLiteral("Syntax error")));
        v4->throwError(error);
    }
}

ReturnedValue Script::run(const QV4::Value *thisObject)
{
    if (!parsed)
        parse();
    if (!vmFunction)
        return Encode::undefined();

    QV4::ExecutionEngine *engine = context->engine();
    QV4::Scope valueScope(engine);

    if (qmlContext.isUndefined()) {
        QScopedValueRollback<Function*> savedGlobalCode(engine->globalCode, vmFunction);

        return vmFunction->call(thisObject ? thisObject : engine->globalObject, nullptr, 0,
                                context);
    } else {
        Scoped<QmlContext> qml(valueScope, qmlContext.value());
        return vmFunction->call(thisObject, nullptr, 0, qml);
    }
}

Function *Script::function()
{
    if (!parsed)
        parse();
    return vmFunction;
}

QQmlRefPointer<QV4::CompiledData::CompilationUnit> Script::precompile(
        QV4::Compiler::Module *module, QQmlJS::Engine *jsEngine,
        Compiler::JSUnitGenerator *unitGenerator, const QString &fileName,
        const QString &source, QList<QQmlError> *reportedErrors,
        QV4::Compiler::ContextType contextType)
{
    using namespace QV4::Compiler;
    using namespace QQmlJS::AST;

    Lexer lexer(jsEngine);
    lexer.setCode(source, /*line*/1, /*qml mode*/false);
    Parser parser(jsEngine);

    parser.parseProgram();

    QList<QQmlError> errors = QQmlEnginePrivate::qmlErrorFromDiagnostics(fileName, parser.diagnosticMessages());
    if (!errors.isEmpty()) {
        if (reportedErrors)
            *reportedErrors << errors;
        return nullptr;
    }

    Program *program = AST::cast<Program *>(parser.rootNode());
    if (!program) {
        // if parsing was successful, and we have no program, then
        // we're done...:
        return nullptr;
    }

    Codegen cg(unitGenerator, /*strict mode*/false);
    cg.generateFromProgram(source, program, module, contextType);
    if (cg.hasError()) {
        if (reportedErrors) {
            const auto v4Error = cg.error();
            QQmlError error;
            error.setUrl(cg.url());
            error.setLine(qmlConvertSourceCoordinate<quint32, int>(v4Error.loc.startLine));
            error.setColumn(qmlConvertSourceCoordinate<quint32, int>(v4Error.loc.startColumn));
            error.setDescription(v4Error.message);
            reportedErrors->append(error);
        }
        return nullptr;
    }

    return cg.generateCompilationUnit(/*generate unit data*/false);
}

Script *Script::createFromFileOrCache(ExecutionEngine *engine, QmlContext *qmlContext, const QString &fileName, const QUrl &originalUrl, QString *error)
{
    if (error)
        error->clear();

    QQmlMetaType::CachedUnitLookupError cacheError = QQmlMetaType::CachedUnitLookupError::NoError;
    const ExecutionEngine::DiskCacheOptions options = engine->diskCacheOptions();
    if (const QQmlPrivate::CachedQmlUnit *cachedUnit
            = (options & ExecutionEngine::DiskCache::Aot)
                ? QQmlMetaType::findCachedCompilationUnit(
                    originalUrl,
                    (options & ExecutionEngine::DiskCache::AotByteCode)
                        ? QQmlMetaType::AcceptUntyped
                        : QQmlMetaType::RequireFullyTyped,
                    &cacheError)
                : nullptr) {
        QQmlRefPointer<QV4::ExecutableCompilationUnit> jsUnit
                = engine->insertCompilationUnit(
                    QQml::makeRefPointer<QV4::CompiledData::CompilationUnit>(
                        cachedUnit->qmlData, cachedUnit->aotCompiledFunctions));
        return new QV4::Script(engine, qmlContext, jsUnit);
    }

    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) {
            if (cacheError == QQmlMetaType::CachedUnitLookupError::VersionMismatch)
                *error = originalUrl.toString() + QString::fromUtf8(" was compiled ahead of time with an incompatible version of Qt and the original source code cannot be found. Please recompile");
            else
                *error = QString::fromUtf8("Error opening source file %1: %2").arg(originalUrl.toString()).arg(f.errorString());
        }
        return nullptr;
    }

    QByteArray data = f.readAll();
    QString sourceCode = QString::fromUtf8(data);

    auto result = new QV4::Script(engine, qmlContext, /*parseAsBinding*/false, sourceCode, originalUrl.toString());
    result->contextType = QV4::Compiler::ContextType::ScriptImportedByQML;
    result->parse();
    return result;
}
