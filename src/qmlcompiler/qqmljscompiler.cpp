// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljscompiler_p.h"

#include <private/qqmlirbuilder_p.h>
#include <private/qqmljsbasicblocks_p.h>
#include <private/qqmljscodegenerator_p.h>
#include <private/qqmljscompilerstats_p.h>
#include <private/qqmljsfunctioninitializer_p.h>
#include <private/qqmljsimportvisitor_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsloadergenerator_p.h>
#include <private/qqmljsoptimizations_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsshadowcheck_p.h>
#include <private/qqmljsstoragegeneralizer_p.h>
#include <private/qqmljstypepropagator_p.h>

#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qloggingcategory.h>

#include <QtQml/private/qqmlsignalnames_p.h>

#include <limits>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcAotCompiler, "qt.qml.compiler.aot", QtFatalMsg);

static const int FileScopeCodeIndex = -1;

static QSet<QString> getIllegalNames()
{
    QSet<QString> illegalNames;
    for (const char **g = QV4::Compiler::Codegen::s_globalNames; *g != nullptr; ++g)
        illegalNames.insert(QString::fromLatin1(*g));
    return illegalNames;
}

Q_GLOBAL_STATIC_WITH_ARGS(QSet<QString>, illegalNames, (getIllegalNames()));


void QQmlJSCompileError::print()
{
    fprintf(stderr, "%s\n", qPrintable(message));
}

QQmlJSCompileError QQmlJSCompileError::augment(const QString &contextErrorMessage) const
{
    QQmlJSCompileError augmented;
    augmented.message = contextErrorMessage + message;
    return augmented;
}

static QString diagnosticErrorMessage(const QString &fileName, const QQmlJS::DiagnosticMessage &m)
{
    QString message;
    message = fileName + QLatin1Char(':') + QString::number(m.loc.startLine) + QLatin1Char(':');
    if (m.loc.startColumn > 0)
        message += QString::number(m.loc.startColumn) + QLatin1Char(':');

    if (m.isError())
        message += QLatin1String(" error: ");
    else
        message += QLatin1String(" warning: ");
    message += m.message;
    return message;
}

void QQmlJSCompileError::appendDiagnostic(const QString &inputFileName,
                                          const QQmlJS::DiagnosticMessage &diagnostic)
{
    if (!message.isEmpty())
        message += QLatin1Char('\n');
    message += diagnosticErrorMessage(inputFileName, diagnostic);
}

void QQmlJSCompileError::appendDiagnostics(const QString &inputFileName,
                                           const QList<QQmlJS::DiagnosticMessage> &diagnostics)
{
    for (const QQmlJS::DiagnosticMessage &diagnostic: diagnostics)
        appendDiagnostic(inputFileName, diagnostic);
}

// Ensure that ListElement objects keep all property assignments in their string form
static void annotateListElements(QmlIR::Document *document)
{
    QStringList listElementNames;

    for (const QV4::CompiledData::Import *import : std::as_const(document->imports)) {
        const QString uri = document->stringAt(import->uriIndex);
        if (uri != QStringLiteral("QtQml.Models") && uri != QStringLiteral("QtQuick"))
            continue;

         QString listElementName = QStringLiteral("ListElement");
         const QString qualifier = document->stringAt(import->qualifierIndex);
         if (!qualifier.isEmpty()) {
             listElementName.prepend(QLatin1Char('.'));
             listElementName.prepend(qualifier);
         }
         listElementNames.append(listElementName);
    }

    if (listElementNames.isEmpty())
        return;

    for (QmlIR::Object *object : std::as_const(document->objects)) {
        if (!listElementNames.contains(document->stringAt(object->inheritedTypeNameIndex)))
            continue;
        for (QmlIR::Binding *binding = object->firstBinding(); binding; binding = binding->next) {
            if (binding->type() != QV4::CompiledData::Binding::Type_Script)
                continue;
            binding->stringIndex = document->registerString(object->bindingAsString(document, binding->value.compiledScriptIndex));
        }
    }
}

static bool checkArgumentsObjectUseInSignalHandlers(const QmlIR::Document &doc,
                                                    QQmlJSCompileError *error)
{
    for (QmlIR::Object *object: std::as_const(doc.objects)) {
        for (auto binding = object->bindingsBegin(); binding != object->bindingsEnd(); ++binding) {
            if (binding->type() != QV4::CompiledData::Binding::Type_Script)
                continue;
            const QString propName =  doc.stringAt(binding->propertyNameIndex);
            if (!QQmlSignalNames::isHandlerName(propName))
                continue;
            auto compiledFunction = doc.jsModule.functions.value(object->runtimeFunctionIndices.at(binding->value.compiledScriptIndex));
            if (!compiledFunction)
                continue;
            if (compiledFunction->usesArgumentsObject == QV4::Compiler::Context::ArgumentsObjectUsed) {
                error->message = QLatin1Char(':') + QString::number(compiledFunction->line) + QLatin1Char(':');
                if (compiledFunction->column > 0)
                    error->message += QString::number(compiledFunction->column) + QLatin1Char(':');

                error->message += QLatin1String(" error: The use of eval() or the use of the arguments object in signal handlers is\n"
                                                "not supported when compiling qml files ahead of time. That is because it's ambiguous if \n"
                                                "any signal parameter is called \"arguments\". Similarly the string passed to eval might use\n"
                                                "\"arguments\". Unfortunately we cannot distinguish between it being a parameter or the\n"
                                                "JavaScript arguments object at this point.\n"
                                                "Consider renaming the parameter of the signal if applicable or moving the code into a\n"
                                                "helper function.");
                return false;
            }
        }
    }
    return true;
}

class BindingOrFunction
{
public:
    BindingOrFunction(const QmlIR::Binding &b) : m_binding(&b) {}
    BindingOrFunction(const QmlIR::Function &f) : m_function(&f) {}

    friend bool operator<(const BindingOrFunction &lhs, const BindingOrFunction &rhs)
    {
        return lhs.index() < rhs.index();
    }

    const QmlIR::Binding *binding() const { return m_binding; }
    const QmlIR::Function *function() const { return m_function; }

    quint32 index() const
    {
        return m_binding
                ? m_binding->value.compiledScriptIndex
                : (m_function
                   ? m_function->index
                   : std::numeric_limits<quint32>::max());
    }

private:
    const QmlIR::Binding *m_binding = nullptr;
    const QmlIR::Function *m_function = nullptr;
};

bool qCompileQmlFile(const QString &inputFileName, QQmlJSSaveFunction saveFunction,
                     QQmlJSAotCompiler *aotCompiler, QQmlJSCompileError *error,
                     bool storeSourceLocation, QV4::Compiler::CodegenWarningInterface *interface,
                     const QString *fileContents)
{
    QmlIR::Document irDocument(QString(), QString(), /*debugMode*/false);
    return qCompileQmlFile(irDocument, inputFileName, saveFunction, aotCompiler, error,
                           storeSourceLocation, interface, fileContents);
}

bool qCompileQmlFile(QmlIR::Document &irDocument, const QString &inputFileName,
                     QQmlJSSaveFunction saveFunction, QQmlJSAotCompiler *aotCompiler,
                     QQmlJSCompileError *error, bool storeSourceLocation,
                     QV4::Compiler::CodegenWarningInterface *interface, const QString *fileContents)
{
    QString sourceCode;

    if (fileContents != nullptr) {
        sourceCode = *fileContents;
    } else {
        QFile f(inputFileName);
        if (!f.open(QIODevice::ReadOnly)) {
            error->message = QLatin1String("Error opening ") + inputFileName + QLatin1Char(':') + f.errorString();
            return false;
        }
        sourceCode = QString::fromUtf8(f.readAll());
        if (f.error() != QFileDevice::NoError) {
            error->message = QLatin1String("Error reading from ") + inputFileName + QLatin1Char(':') + f.errorString();
            return false;
        }
    }

    {
        QmlIR::IRBuilder irBuilder(*illegalNames());
        if (!irBuilder.generateFromQml(sourceCode, inputFileName, &irDocument)) {
            error->appendDiagnostics(inputFileName, irBuilder.errors);
            return false;
        }
    }

    annotateListElements(&irDocument);
    QQmlJSAotFunctionMap aotFunctionsByIndex;

    {
        QmlIR::JSCodeGen v4CodeGen(&irDocument, *illegalNames(), interface, storeSourceLocation);

        if (aotCompiler)
            aotCompiler->setDocument(&v4CodeGen, &irDocument);

        QHash<QmlIR::Object *, QmlIR::Object *> effectiveScopes;
        for (QmlIR::Object *object: std::as_const(irDocument.objects)) {
            if (object->functionsAndExpressions->count == 0 && object->bindingCount() == 0)
                continue;

            if (!v4CodeGen.generateRuntimeFunctions(object)) {
                Q_ASSERT(v4CodeGen.hasError());
                error->appendDiagnostic(inputFileName, v4CodeGen.error());
                return false;
            }

            if (!aotCompiler)
                continue;

            QmlIR::Object *scope = object;
            for (auto it = effectiveScopes.constFind(scope), end = effectiveScopes.constEnd();
                 it != end; it = effectiveScopes.constFind(scope)) {
                scope = *it;
            }

            aotCompiler->setScope(object, scope);
            aotFunctionsByIndex[FileScopeCodeIndex] = aotCompiler->globalCode();

            std::vector<BindingOrFunction> bindingsAndFunctions;
            bindingsAndFunctions.reserve(object->bindingCount() + object->functionCount());

            std::copy(object->bindingsBegin(), object->bindingsEnd(),
                      std::back_inserter(bindingsAndFunctions));
            std::copy(object->functionsBegin(), object->functionsEnd(),
                      std::back_inserter(bindingsAndFunctions));

            QList<QmlIR::CompiledFunctionOrExpression> functionsToCompile;
            for (QmlIR::CompiledFunctionOrExpression *foe = object->functionsAndExpressions->first;
                 foe; foe = foe->next) {
                functionsToCompile << *foe;
            }

            // AOT-compile bindings and functions in the same order as above so that the runtime
            // class indices match
            auto contextMap = v4CodeGen.module()->contextMap;
            std::sort(bindingsAndFunctions.begin(), bindingsAndFunctions.end());
            std::for_each(bindingsAndFunctions.begin(), bindingsAndFunctions.end(),
                          [&](const BindingOrFunction &bindingOrFunction) {
                std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage> result;
                if (const auto *binding = bindingOrFunction.binding()) {
                    switch (binding->type()) {
                    case QmlIR::Binding::Type_AttachedProperty:
                    case QmlIR::Binding::Type_GroupProperty:
                        effectiveScopes.insert(
                                    irDocument.objects.at(binding->value.objectIndex), scope);
                        return;
                    case QmlIR::Binding::Type_Boolean:
                    case QmlIR::Binding::Type_Number:
                    case QmlIR::Binding::Type_String:
                    case QmlIR::Binding::Type_Null:
                    case QmlIR::Binding::Type_Object:
                    case QmlIR::Binding::Type_Translation:
                    case QmlIR::Binding::Type_TranslationById:
                        return;
                    default:
                        break;
                    }

                    Q_ASSERT(quint32(functionsToCompile.size()) > binding->value.compiledScriptIndex);
                    const auto &functionToCompile
                            = functionsToCompile[binding->value.compiledScriptIndex];
                    auto *parentNode = functionToCompile.parentNode;
                    Q_ASSERT(parentNode);
                    Q_ASSERT(contextMap.contains(parentNode));
                    QV4::Compiler::Context *context = contextMap.take(parentNode);
                    Q_ASSERT(context);

                    auto *node = functionToCompile.node;
                    Q_ASSERT(node);

                    if (context->returnsClosure) {
                        QQmlJS::AST::Node *inner
                                = QQmlJS::AST::cast<QQmlJS::AST::ExpressionStatement *>(
                                    node)->expression;
                        Q_ASSERT(inner);
                        QV4::Compiler::Context *innerContext = contextMap.take(inner);
                        Q_ASSERT(innerContext);
                        qCDebug(lcAotCompiler) << "Compiling signal handler for"
                                               << irDocument.stringAt(binding->propertyNameIndex);
                        std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage> innerResult
                                = aotCompiler->compileBinding(innerContext, *binding, inner);

                        if (auto *error = std::get_if<QQmlJS::DiagnosticMessage>(&innerResult)) {
                            qCDebug(lcAotCompiler) << "Compilation failed:"
                                                   << diagnosticErrorMessage(inputFileName, *error);
                        } else if (auto *func = std::get_if<QQmlJSAotFunction>(&innerResult)) {
                            qCDebug(lcAotCompiler) << "Generated code:" << func->code;
                            aotFunctionsByIndex[innerContext->functionIndex] = *func;
                        }
                    }

                    qCDebug(lcAotCompiler) << "Compiling binding for property"
                                           << irDocument.stringAt(binding->propertyNameIndex);
                    result = aotCompiler->compileBinding(context, *binding, node);
                } else if (const auto *function = bindingOrFunction.function()) {
                    Q_ASSERT(quint32(functionsToCompile.size()) > function->index);
                    auto *node = functionsToCompile[function->index].node;
                    Q_ASSERT(node);
                    Q_ASSERT(contextMap.contains(node));
                    QV4::Compiler::Context *context = contextMap.take(node);
                    Q_ASSERT(context);

                    const QString functionName = irDocument.stringAt(function->nameIndex);
                    qCDebug(lcAotCompiler) << "Compiling function" << functionName;
                    result = aotCompiler->compileFunction(context, functionName, node);
                } else {
                    Q_UNREACHABLE();
                }

                if (auto *error = std::get_if<QQmlJS::DiagnosticMessage>(&result)) {
                    qCDebug(lcAotCompiler) << "Compilation failed:"
                                           << diagnosticErrorMessage(inputFileName, *error);
                } else if (auto *func = std::get_if<QQmlJSAotFunction>(&result)) {
                    qCDebug(lcAotCompiler) << "Generated code:" << func->code;
                    aotFunctionsByIndex[object->runtimeFunctionIndices[bindingOrFunction.index()]] =
                            *func;
                }
            });
        }

        if (!checkArgumentsObjectUseInSignalHandlers(irDocument, error)) {
            *error = error->augment(inputFileName);
            return false;
        }

        QmlIR::QmlUnitGenerator generator;
        irDocument.javaScriptCompilationUnit = v4CodeGen.generateCompilationUnit(/*generate unit*/false);
        generator.generate(irDocument);

        const quint32 saveFlags
                = QV4::CompiledData::Unit::StaticData
                | QV4::CompiledData::Unit::PendingTypeCompilation;
        QV4::CompiledData::SaveableUnitPointer saveable(
                irDocument.javaScriptCompilationUnit->unitData(), saveFlags);
        if (!saveFunction(saveable, aotFunctionsByIndex, &error->message))
            return false;
    }
    return true;
}

bool qCompileJSFile(
        const QString &inputFileName, const QString &inputFileUrl, QQmlJSSaveFunction saveFunction,
        QQmlJSCompileError *error)
{
    Q_UNUSED(inputFileUrl);

    QQmlRefPointer<QV4::CompiledData::CompilationUnit> unit;

    QString sourceCode;
    {
        QFile f(inputFileName);
        if (!f.open(QIODevice::ReadOnly)) {
            error->message = QLatin1String("Error opening ") + inputFileName + QLatin1Char(':') + f.errorString();
            return false;
        }
        sourceCode = QString::fromUtf8(f.readAll());
        if (f.error() != QFileDevice::NoError) {
            error->message = QLatin1String("Error reading from ") + inputFileName + QLatin1Char(':') + f.errorString();
            return false;
        }
    }

    const bool isModule = inputFileName.endsWith(QLatin1String(".mjs"));
    if (isModule) {
        QList<QQmlJS::DiagnosticMessage> diagnostics;
        // Precompiled files are relocatable and the final location will be set when loading.
        QString url;
        unit = QV4::Compiler::Codegen::compileModule(/*debugMode*/false, url, sourceCode,
                                                     QDateTime(), &diagnostics);
        error->appendDiagnostics(inputFileName, diagnostics);
        if (!unit || !unit->unitData())
            return false;
    } else {
        QmlIR::Document irDocument(QString(), QString(), /*debugMode*/false);

        QQmlJS::Engine *engine = &irDocument.jsParserEngine;
        QmlIR::ScriptDirectivesCollector directivesCollector(&irDocument);
        QQmlJS::Directives *oldDirs = engine->directives();
        engine->setDirectives(&directivesCollector);
        auto directivesGuard = qScopeGuard([engine, oldDirs]{
            engine->setDirectives(oldDirs);
        });

        QQmlJS::AST::Program *program = nullptr;

        {
            QQmlJS::Lexer lexer(engine);
            lexer.setCode(sourceCode, /*line*/1, /*parseAsBinding*/false);
            QQmlJS::Parser parser(engine);

            bool parsed = parser.parseProgram();

            error->appendDiagnostics(inputFileName, parser.diagnosticMessages());

            if (!parsed)
                return false;

            program = QQmlJS::AST::cast<QQmlJS::AST::Program*>(parser.rootNode());
            if (!program) {
                lexer.setCode(QStringLiteral("undefined;"), 1, false);
                parsed = parser.parseProgram();
                Q_ASSERT(parsed);
                program = QQmlJS::AST::cast<QQmlJS::AST::Program*>(parser.rootNode());
                Q_ASSERT(program);
            }
        }

        {
            QmlIR::JSCodeGen v4CodeGen(&irDocument, *illegalNames());
            v4CodeGen.generateFromProgram(
                    sourceCode, program, &irDocument.jsModule,
                    QV4::Compiler::ContextType::ScriptImportedByQML);
            if (v4CodeGen.hasError()) {
                error->appendDiagnostic(inputFileName, v4CodeGen.error());
                return false;
            }

            // Precompiled files are relocatable and the final location will be set when loading.
            Q_ASSERT(irDocument.jsModule.fileName.isEmpty());
            Q_ASSERT(irDocument.jsModule.finalUrl.isEmpty());

            irDocument.javaScriptCompilationUnit = v4CodeGen.generateCompilationUnit(/*generate unit*/false);
            QmlIR::QmlUnitGenerator generator;
            generator.generate(irDocument);
            unit = std::move(irDocument.javaScriptCompilationUnit);
        }
    }

    QQmlJSAotFunctionMap empty;
    return saveFunction(
            QV4::CompiledData::SaveableUnitPointer(unit->unitData()), empty, &error->message);
}

static const char *funcHeaderCode = R"(
    [](const QQmlPrivate::AOTCompiledContext *aotContext, void **argv) {
Q_UNUSED(aotContext)
Q_UNUSED(argv)
)";

bool qSaveQmlJSUnitAsCpp(const QString &inputFileName, const QString &outputFileName, const QV4::CompiledData::SaveableUnitPointer &unit, const QQmlJSAotFunctionMap &aotFunctions, QString *errorString)
{
#if QT_CONFIG(temporaryfile)
    QSaveFile f(outputFileName);
#else
    QFile f(outputFileName);
#endif
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *errorString = f.errorString();
        return false;
    }

    auto writeStr = [&f, errorString](const QByteArray &data) {
        if (f.write(data) != data.size()) {
            *errorString = f.errorString();
            return false;
        }
        return true;
    };

    if (!writeStr("// "))
        return false;

    if (!writeStr(inputFileName.toUtf8()))
        return false;

    if (!writeStr("\n"))
        return false;

    if (!writeStr("#include <QtQml/qqmlprivate.h>\n"))
        return false;

    if (!aotFunctions.isEmpty()) {
        QStringList includes;

        for (const auto &function : aotFunctions)
            includes.append(function.includes);

        std::sort(includes.begin(), includes.end());
        const auto end = std::unique(includes.begin(), includes.end());
        for (auto it = includes.begin(); it != end; ++it) {
            if (!writeStr(QStringLiteral("#include <%1>\n").arg(*it).toUtf8()))
                return false;
        }
    }

    if (!writeStr(QByteArrayLiteral("namespace QmlCacheGeneratedCode {\nnamespace ")))
        return false;

    if (!writeStr(qQmlJSSymbolNamespaceForPath(inputFileName).toUtf8()))
        return false;

    if (!writeStr(QByteArrayLiteral(" {\nextern const unsigned char qmlData alignas(16) [];\n"
                                    "extern const unsigned char qmlData alignas(16) [] = {\n")))
        return false;

    unit.saveToDisk<uchar>([&writeStr](const uchar *begin, quint32 size) {
        QByteArray hexifiedData;
        {
            QTextStream stream(&hexifiedData);
            const uchar *end = begin + size;
            stream << Qt::hex;
            int col = 0;
            for (const uchar *data = begin; data < end; ++data, ++col) {
                if (data > begin)
                    stream << ',';
                if (col % 8 == 0) {
                    stream << '\n';
                    col = 0;
                }
                stream << "0x" << *data;
            }
            stream << '\n';
        }
        return writeStr(hexifiedData);
    });



    if (!writeStr("};\n"))
        return false;

    // Suppress the following warning generated by MSVC 2019:
    //     "the usage of 'QJSNumberCoercion::toInteger' requires the compiler to capture 'this'
    //      but the current default capture mode does not allow it"
    // You clearly don't have to capture 'this' in order to call 'QJSNumberCoercion::toInteger'.
    // TODO: Remove when we don't have to support MSVC 2019 anymore. Mind the QT_WARNING_POP below.
    if (!writeStr("QT_WARNING_PUSH\nQT_WARNING_DISABLE_MSVC(4573)\n"))
        return false;

    writeStr(aotFunctions[FileScopeCodeIndex].code.toUtf8().constData());
    if (aotFunctions.size() <= 1) {
        // FileScopeCodeIndex is always there, but it may be the only one.
        writeStr("extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];\n"
                 "extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[] = { { 0, 0, nullptr, nullptr } };\n");
    } else {
        writeStr("extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];\n"
                 "extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[] = {\n");

        QString footer = QStringLiteral("}\n");

        for (QQmlJSAotFunctionMap::ConstIterator func = aotFunctions.constBegin(),
             end = aotFunctions.constEnd();
             func != end; ++func) {

            if (func.key() == FileScopeCodeIndex)
                continue;

            const QString function = QString::fromUtf8(funcHeaderCode) + func.value().code + footer;

            writeStr(QStringLiteral("{ %1, %2, [](QV4::ExecutableCompilationUnit *contextUnit, "
                                    "QMetaType *argTypes) {\n%3}, %4 },")
                     .arg(func.key())
                     .arg(func->numArguments)
                     .arg(func->signature, function)
                     .toUtf8().constData());
        }

        // Conclude the list with a nullptr
        writeStr("{ 0, 0, nullptr, nullptr }");
        writeStr("};\n");
    }

    if (!writeStr("QT_WARNING_POP\n"))
        return false;

    if (!writeStr("}\n}\n"))
        return false;

#if QT_CONFIG(temporaryfile)
    if (!f.commit()) {
        *errorString = f.errorString();
        return false;
    }
#endif

    return true;
}

QQmlJSAotCompiler::QQmlJSAotCompiler(
        QQmlJSImporter *importer, const QString &resourcePath, const QStringList &qmldirFiles,
        QQmlJSLogger *logger)
    : m_typeResolver(importer)
    , m_resourcePath(resourcePath)
    , m_qmldirFiles(qmldirFiles)
    , m_importer(importer)
    , m_logger(logger)
{
}

void QQmlJSAotCompiler::setDocument(
        const QmlIR::JSCodeGen *codegen, const QmlIR::Document *irDocument)
{
    Q_UNUSED(codegen);
    m_document = irDocument;
    const QFileInfo resourcePathInfo(m_resourcePath);
    if (m_logger->filePath().isEmpty())
        m_logger->setFilePath(resourcePathInfo.fileName());
    m_logger->setCode(irDocument->code);
    m_unitGenerator = &irDocument->jsGenerator;
    QQmlJSScope::Ptr target = QQmlJSScope::create();
    QQmlJSImportVisitor visitor(target, m_importer, m_logger,
                                resourcePathInfo.canonicalPath() + u'/',
                                m_qmldirFiles);
    m_typeResolver.init(&visitor, irDocument->program);
}

void QQmlJSAotCompiler::setScope(const QmlIR::Object *object, const QmlIR::Object *scope)
{
    m_currentObject = object;
    m_currentScope = scope;
}

static bool isStrict(const QmlIR::Document *doc)
{
    for (const QmlIR::Pragma *pragma : doc->pragmas) {
        if (pragma->type == QmlIR::Pragma::Strict)
            return true;
    }
    return false;
}

QQmlJS::DiagnosticMessage QQmlJSAotCompiler::diagnose(
        const QString &message, QtMsgType type, const QQmlJS::SourceLocation &location) const
{
    if (isStrict(m_document)
            && (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg)
            && m_logger->isCategoryFatal(qmlCompiler)) {
        qFatal("%s:%d: (strict mode) %s",
               qPrintable(QFileInfo(m_resourcePath).fileName()),
               location.startLine, qPrintable(message));
    }

    // TODO: this is a special place that explicitly sets the severity through
    // logger's private function
    m_logger->log(message, qmlCompiler, location, type);

    return QQmlJS::DiagnosticMessage {
        message,
        type,
        location
    };
}

std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage> QQmlJSAotCompiler::compileBinding(
        const QV4::Compiler::Context *context, const QmlIR::Binding &irBinding,
        QQmlJS::AST::Node *astNode)
{
    QQmlJSFunctionInitializer initializer(
                &m_typeResolver, m_currentObject->location, m_currentScope->location);
    QQmlJS::DiagnosticMessage error;
    const QString name = m_document->stringAt(irBinding.propertyNameIndex);
    QQmlJSCompilePass::Function function = initializer.run(
                context, name, astNode, irBinding, &error);
    const QQmlJSAotFunction aotFunction = doCompileAndRecordAotStats(
            context, &function, &error, name, astNode->firstSourceLocation());

    if (error.isValid()) {
        // If it's a signal and the function just returns a closure, it's harmless.
        // Otherwise promote the message to warning level.
        return diagnose(error.message,
                        (function.isSignalHandler && error.type == QtDebugMsg)
                            ? QtDebugMsg
                            : QtWarningMsg,
                        error.loc);
    }

    qCDebug(lcAotCompiler()) << "includes:" << aotFunction.includes;
    qCDebug(lcAotCompiler()) << "binding code:" << aotFunction.code;
    return aotFunction;
}

std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage> QQmlJSAotCompiler::compileFunction(
        const QV4::Compiler::Context *context, const QString &name, QQmlJS::AST::Node *astNode)
{
    QQmlJSFunctionInitializer initializer(
                &m_typeResolver, m_currentObject->location, m_currentScope->location);
    QQmlJS::DiagnosticMessage error;
    QQmlJSCompilePass::Function function = initializer.run(context, name, astNode, &error);
    const QQmlJSAotFunction aotFunction = doCompileAndRecordAotStats(
            context, &function, &error, name, astNode->firstSourceLocation());

    if (error.isValid())
        return diagnose(error.message, QtWarningMsg, error.loc);

    qCDebug(lcAotCompiler()) << "includes:" << aotFunction.includes;
    qCDebug(lcAotCompiler()) << "binding code:" << aotFunction.code;
    return aotFunction;
}

QQmlJSAotFunction QQmlJSAotCompiler::globalCode() const
{
    QQmlJSAotFunction global;
    global.includes = {
        u"QtQml/qjsengine.h"_s,
        u"QtQml/qjsprimitivevalue.h"_s,
        u"QtQml/qjsvalue.h"_s,
        u"QtQml/qqmlcomponent.h"_s,
        u"QtQml/qqmlcontext.h"_s,
        u"QtQml/qqmlengine.h"_s,
        u"QtQml/qqmllist.h"_s,

        u"QtCore/qdatetime.h"_s,
        u"QtCore/qtimezone.h"_s,
        u"QtCore/qobject.h"_s,
        u"QtCore/qstring.h"_s,
        u"QtCore/qstringlist.h"_s,
        u"QtCore/qurl.h"_s,
        u"QtCore/qvariant.h"_s,

        u"type_traits"_s
    };
    return global;
}

QQmlJSAotFunction QQmlJSAotCompiler::doCompile(
        const QV4::Compiler::Context *context, QQmlJSCompilePass::Function *function,
        QQmlJS::DiagnosticMessage *error)
{
    const auto compileError = [&]() {
        Q_ASSERT(error->isValid());
        error->type = context->returnsClosure ? QtDebugMsg : QtWarningMsg;
        return QQmlJSAotFunction();
    };

    if (error->isValid())
        return compileError();

    bool basicBlocksValidationFailed = false;
    QQmlJSBasicBlocks basicBlocks(context, m_unitGenerator, &m_typeResolver, m_logger);
    auto passResult = basicBlocks.run(function, m_flags, basicBlocksValidationFailed);
    auto &[blocks, annotations] = passResult;

    QQmlJSTypePropagator propagator(m_unitGenerator, &m_typeResolver, m_logger, blocks, annotations);
    passResult = propagator.run(function, error);
    if (error->isValid())
        return compileError();

    QQmlJSShadowCheck shadowCheck(m_unitGenerator, &m_typeResolver, m_logger, blocks, annotations);
    passResult = shadowCheck.run(function, error);
    if (error->isValid())
        return compileError();

    QQmlJSOptimizations optimizer(m_unitGenerator, &m_typeResolver, m_logger, blocks, annotations,
                                  basicBlocks.objectAndArrayDefinitions());
    passResult = optimizer.run(function, error);
    if (error->isValid())
        return compileError();

    // Generalize all arguments, registers, and the return type.
    QQmlJSStorageGeneralizer generalizer(m_unitGenerator, &m_typeResolver, m_logger, blocks, annotations);
    passResult = generalizer.run(function, error);
    if (error->isValid())
        return compileError();

    QQmlJSCodeGenerator codegen(context, m_unitGenerator, &m_typeResolver, m_logger, blocks, annotations);
    QQmlJSAotFunction result = codegen.run(function, error, basicBlocksValidationFailed);
    return error->isValid() ? compileError() : result;
}

QQmlJSAotFunction QQmlJSAotCompiler::doCompileAndRecordAotStats(
        const QV4::Compiler::Context *context, QQmlJSCompilePass::Function *function,
        QQmlJS::DiagnosticMessage *error, const QString &name, QQmlJS::SourceLocation location)
{
    auto t1 = std::chrono::high_resolution_clock::now();
    QQmlJSAotFunction result;
    if (!error->isValid())
        result = doCompile(context, function, error);
    auto t2 = std::chrono::high_resolution_clock::now();

    if (QQmlJS::QQmlJSAotCompilerStats::recordAotStats()) {
        QQmlJS::AotStatsEntry entry;
        entry.codegenDuration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
        entry.functionName = name;
        entry.errorMessage = error->message;
        entry.line = location.startLine;
        entry.column = location.startColumn;
        entry.codegenSuccessful = !error->isValid();
        QQmlJS::QQmlJSAotCompilerStats::addEntry(function->qmlScope->filePath(), entry);
    }

    return result;
}

QT_END_NAMESPACE
