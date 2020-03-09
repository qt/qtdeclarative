/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QCoreApplication>
#include <QStringList>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QHashFunctions>
#include <QSaveFile>
#include <QScopedPointer>
#include <QScopeGuard>

#include <private/qqmlirbuilder_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljslexer_p.h>

#include "resourcefilemapper.h"

#include <algorithm>

using namespace QQmlJS;

int filterResourceFile(const QString &input, const QString &output);
bool generateLoader(const QStringList &compiledFiles, const QString &output,
                    const QStringList &resourceFileMappings, QString *errorString);
QString symbolNamespaceForPath(const QString &relativePath);

QSet<QString> illegalNames;

void setupIllegalNames()
{
    for (const char **g = QV4::Compiler::Codegen::s_globalNames; *g != nullptr; ++g)
        illegalNames.insert(QString::fromLatin1(*g));
}

struct Error
{
    QString message;
    void print();
    Error augment(const QString &contextErrorMessage) const;
    void appendDiagnostics(const QString &inputFileName, const QList<QQmlJS::DiagnosticMessage> &diagnostics);
    void appendDiagnostic(const QString &inputFileName, const DiagnosticMessage &diagnostic);
};

void Error::print()
{
    fprintf(stderr, "%s\n", qPrintable(message));
}

Error Error::augment(const QString &contextErrorMessage) const
{
    Error augmented;
    augmented.message = contextErrorMessage + message;
    return augmented;
}

QString diagnosticErrorMessage(const QString &fileName, const QQmlJS::DiagnosticMessage &m)
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

void Error::appendDiagnostic(const QString &inputFileName, const DiagnosticMessage &diagnostic)
{
    if (!message.isEmpty())
        message += QLatin1Char('\n');
    message += diagnosticErrorMessage(inputFileName, diagnostic);
}

void Error::appendDiagnostics(const QString &inputFileName, const QList<DiagnosticMessage> &diagnostics)
{
    for (const QQmlJS::DiagnosticMessage &diagnostic: diagnostics)
        appendDiagnostic(inputFileName, diagnostic);
}

// Ensure that ListElement objects keep all property assignments in their string form
static void annotateListElements(QmlIR::Document *document)
{
    QStringList listElementNames;

    for (const QV4::CompiledData::Import *import : qAsConst(document->imports)) {
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

    for (QmlIR::Object *object : qAsConst(document->objects)) {
        if (!listElementNames.contains(document->stringAt(object->inheritedTypeNameIndex)))
            continue;
        for (QmlIR::Binding *binding = object->firstBinding(); binding; binding = binding->next) {
            if (binding->type != QV4::CompiledData::Binding::Type_Script)
                continue;
            binding->stringIndex = document->registerString(object->bindingAsString(document, binding->value.compiledScriptIndex));
        }
    }
}

static bool checkArgumentsObjectUseInSignalHandlers(const QmlIR::Document &doc, Error *error)
{
    for (QmlIR::Object *object: qAsConst(doc.objects)) {
        for (auto binding = object->bindingsBegin(); binding != object->bindingsEnd(); ++binding) {
            if (binding->type != QV4::CompiledData::Binding::Type_Script)
                continue;
            const QString propName =  doc.stringAt(binding->propertyNameIndex);
            if (!propName.startsWith(QLatin1String("on"))
                || propName.length() < 3
                || !propName.at(2).isUpper())
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

using SaveFunction = std::function<bool(const QV4::CompiledData::SaveableUnitPointer &, QString *)>;

static bool compileQmlFile(const QString &inputFileName, SaveFunction saveFunction, Error *error)
{
    QmlIR::Document irDocument(/*debugMode*/false);

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

    {
        QmlIR::IRBuilder irBuilder(illegalNames);
        if (!irBuilder.generateFromQml(sourceCode, inputFileName, &irDocument)) {
            error->appendDiagnostics(inputFileName, irBuilder.errors);
            return false;
        }
    }

    annotateListElements(&irDocument);

    {
        QmlIR::JSCodeGen v4CodeGen(&irDocument, illegalNames);
        for (QmlIR::Object *object: qAsConst(irDocument.objects)) {
            if (object->functionsAndExpressions->count == 0)
                continue;
            QList<QmlIR::CompiledFunctionOrExpression> functionsToCompile;
            for (QmlIR::CompiledFunctionOrExpression *foe = object->functionsAndExpressions->first; foe; foe = foe->next)
                functionsToCompile << *foe;
            const QVector<int> runtimeFunctionIndices = v4CodeGen.generateJSCodeForFunctionsAndBindings(functionsToCompile);
            if (v4CodeGen.hasError()) {
                error->appendDiagnostic(inputFileName, v4CodeGen.error());
                return false;
            }

            QQmlJS::MemoryPool *pool = irDocument.jsParserEngine.pool();
            object->runtimeFunctionIndices.allocate(pool, runtimeFunctionIndices);
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
        QV4::CompiledData::SaveableUnitPointer saveable(irDocument.javaScriptCompilationUnit.data,
                                                        saveFlags);
        if (!saveFunction(saveable, &error->message))
            return false;
    }
    return true;
}

static bool compileJSFile(const QString &inputFileName, const QString &inputFileUrl, SaveFunction saveFunction, Error *error)
{
    QV4::CompiledData::CompilationUnit unit;

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
        if (!unit.unitData())
            return false;
    } else {
        QmlIR::Document irDocument(/*debugMode*/false);

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
            QmlIR::JSCodeGen v4CodeGen(&irDocument, illegalNames);
            v4CodeGen.generateFromProgram(inputFileName, inputFileUrl, sourceCode, program,
                                          &irDocument.jsModule, QV4::Compiler::ContextType::ScriptImportedByQML);
            if (v4CodeGen.hasError()) {
                error->appendDiagnostic(inputFileName, v4CodeGen.error());
                return false;
            }

            // Precompiled files are relocatable and the final location will be set when loading.
            irDocument.jsModule.fileName.clear();
            irDocument.jsModule.finalUrl.clear();

            irDocument.javaScriptCompilationUnit = v4CodeGen.generateCompilationUnit(/*generate unit*/false);
            QmlIR::QmlUnitGenerator generator;
            generator.generate(irDocument);
            unit = std::move(irDocument.javaScriptCompilationUnit);
        }
    }

    return saveFunction(QV4::CompiledData::SaveableUnitPointer(unit.data), &error->message);
}

static bool saveUnitAsCpp(const QString &inputFileName, const QString &outputFileName,
                          const QV4::CompiledData::SaveableUnitPointer &unit,
                          QString *errorString)
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

    if (!writeStr(QByteArrayLiteral("namespace QmlCacheGeneratedCode {\nnamespace ")))
        return false;

    if (!writeStr(symbolNamespaceForPath(inputFileName).toUtf8()))
        return false;

    if (!writeStr(QByteArrayLiteral(" {\nextern const unsigned char qmlData alignas(16) [] = {\n")))
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



    if (!writeStr("};\n}\n}\n"))
        return false;

#if QT_CONFIG(temporaryfile)
    if (!f.commit()) {
        *errorString = f.errorString();
        return false;
    }
#endif

    return true;
}

int main(int argc, char **argv)
{
    // Produce reliably the same output for the same input by disabling QHash's random seeding.
    qSetGlobalQHashSeed(0);

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("qmlcachegen"));
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption filterResourceFileOption(QStringLiteral("filter-resource-file"), QCoreApplication::translate("main", "Filter out QML/JS files from a resource file that can be cached ahead of time instead"));
    parser.addOption(filterResourceFileOption);
    QCommandLineOption resourceFileMappingOption(QStringLiteral("resource-file-mapping"), QCoreApplication::translate("main", "Path from original resource file to new one"), QCoreApplication::translate("main", "old-name:new-name"));
    parser.addOption(resourceFileMappingOption);
    QCommandLineOption resourceOption(QStringLiteral("resource"), QCoreApplication::translate("main", "Qt resource file that might later contain one of the compiled files"), QCoreApplication::translate("main", "resource-file-name"));
    parser.addOption(resourceOption);
    QCommandLineOption resourcePathOption(QStringLiteral("resource-path"), QCoreApplication::translate("main", "Qt resource file path corresponding to the file being compiled"), QCoreApplication::translate("main", "resource-path"));
    parser.addOption(resourcePathOption);

    QCommandLineOption outputFileOption(QStringLiteral("o"), QCoreApplication::translate("main", "Output file name"), QCoreApplication::translate("main", "file name"));
    parser.addOption(outputFileOption);

    parser.addPositionalArgument(QStringLiteral("[qml file]"),
            QStringLiteral("QML source file to generate cache for."));

    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    parser.process(app);

    enum Output {
        GenerateCpp,
        GenerateCacheFile,
        GenerateLoader
    } target = GenerateCacheFile;

    QString outputFileName;
    if (parser.isSet(outputFileOption))
        outputFileName = parser.value(outputFileOption);

    if (outputFileName.endsWith(QLatin1String(".cpp"))) {
        target = GenerateCpp;
        if (outputFileName.endsWith(QLatin1String("qmlcache_loader.cpp")))
            target = GenerateLoader;
    }

    const QStringList sources = parser.positionalArguments();
    if (sources.isEmpty()){
        parser.showHelp();
    } else if (sources.count() > 1 && target != GenerateLoader) {
        fprintf(stderr, "%s\n", qPrintable(QStringLiteral("Too many input files specified: '") + sources.join(QStringLiteral("' '")) + QLatin1Char('\'')));
        return EXIT_FAILURE;
    }

    const QString inputFile = sources.first();
    if (outputFileName.isEmpty())
        outputFileName = inputFile + QLatin1Char('c');

    if (parser.isSet(filterResourceFileOption)) {
        return filterResourceFile(inputFile, outputFileName);
    }

    if (target == GenerateLoader) {
        ResourceFileMapper mapper(sources);

        Error error;
        if (!generateLoader(mapper.qmlCompilerFiles(), outputFileName,
                            parser.values(resourceFileMappingOption), &error.message)) {
            error.augment(QLatin1String("Error generating loader stub: ")).print();
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    QString inputFileUrl = inputFile;

    SaveFunction saveFunction;
    if (target == GenerateCpp) {
        ResourceFileMapper fileMapper(parser.values(resourceOption));
        QString inputResourcePath = parser.value(resourcePathOption);

        if (!inputResourcePath.isEmpty() && !fileMapper.isEmpty()) {
            fprintf(stderr, "--%s and --%s are mutually exclusive.\n",
                    qPrintable(resourcePathOption.names().first()),
                    qPrintable(resourceOption.names().first()));
            return EXIT_FAILURE;
        }

        // If the user didn't specify the resource path corresponding to the file on disk being
        // compiled, try to determine it from the resource file, if one was supplied.
        if (inputResourcePath.isEmpty()) {
            const QStringList resourcePaths = fileMapper.resourcePaths(inputFile);
            if (resourcePaths.isEmpty()) {
                fprintf(stderr, "No resource path for file: %s\n", qPrintable(inputFile));
                return EXIT_FAILURE;
            }

            if (resourcePaths.size() != 1) {
                fprintf(stderr, "Multiple resource paths for file %s. "
                                "Use the --%s option to disambiguate:\n",
                        qPrintable(inputFile),
                        qPrintable(resourcePathOption.names().first()));
                for (const QString &resourcePath: resourcePaths)
                    fprintf(stderr, "\t%s\n", qPrintable(resourcePath));
                return EXIT_FAILURE;
            }

            inputResourcePath = resourcePaths.first();
        }

        inputFileUrl = QStringLiteral("qrc://") + inputResourcePath;

        saveFunction = [inputResourcePath, outputFileName](
                               const QV4::CompiledData::SaveableUnitPointer &unit,
                               QString *errorString) {
            return saveUnitAsCpp(inputResourcePath, outputFileName, unit, errorString);
        };

    } else {
        saveFunction = [outputFileName](const QV4::CompiledData::SaveableUnitPointer &unit,
                                        QString *errorString) {
            return unit.saveToDisk<char>(
                    [&outputFileName, errorString](const char *data, quint32 size) {
                        return QV4::CompiledData::SaveableUnitPointer::writeDataToFile(
                                outputFileName, data, size, errorString);
            });
        };
    }

    setupIllegalNames();


    if (inputFile.endsWith(QLatin1String(".qml"))) {
        Error error;
        if (!compileQmlFile(inputFile, saveFunction, &error)) {
            error.augment(QLatin1String("Error compiling qml file: ")).print();
            return EXIT_FAILURE;
        }
    } else if (inputFile.endsWith(QLatin1String(".js")) || inputFile.endsWith(QLatin1String(".mjs"))) {
        Error error;
        if (!compileJSFile(inputFile, inputFileUrl, saveFunction, &error)) {
            error.augment(QLatin1String("Error compiling js file: ")).print();
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "Ignoring %s input file as it is not QML source code - maybe remove from QML_FILES?\n", qPrintable(inputFile));
    }

    return EXIT_SUCCESS;
}
