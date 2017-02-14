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

#include <private/qqmlirbuilder_p.h>
#include <private/qv4isel_moth_p.h>
#include <private/qqmljsparser_p.h>

QT_BEGIN_NAMESPACE
extern Q_CORE_EXPORT QBasicAtomicInt qt_qhash_seed;

namespace QV4 { namespace JIT {
Q_QML_EXPORT QV4::EvalISelFactory *createISelForArchitecture(const QString &architecture);
} }

QT_END_NAMESPACE

struct Error
{
    QString message;
    void print();
    Error augment(const QString &contextErrorMessage) const;
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

static bool compileQmlFile(const QString &inputFileName, QV4::EvalISelFactory *iselFactory, Error *error)
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
        irDocument.jsModule.sourceTimeStamp = QFileInfo(f).lastModified().toMSecsSinceEpoch();
    }

    {
        QSet<QString> illegalNames; // ####
        QmlIR::IRBuilder irBuilder(illegalNames);
        if (!irBuilder.generateFromQml(sourceCode, inputFileName, &irDocument)) {
            for (const QQmlJS::DiagnosticMessage &parseError: qAsConst(irBuilder.errors)) {
                if (!error->message.isEmpty())
                    error->message += QLatin1Char('\n');
                error->message += diagnosticErrorMessage(inputFileName, parseError);
            }
            return false;
        }
    }

    {
        QmlIR::JSCodeGen v4CodeGen(inputFileName, irDocument.code, &irDocument.jsModule, &irDocument.jsParserEngine, irDocument.program, /*import cache*/0, &irDocument.jsGenerator.stringTable);
        for (QmlIR::Object *object: qAsConst(irDocument.objects)) {
            if (object->functionsAndExpressions->count == 0)
                continue;
            QList<QmlIR::CompiledFunctionOrExpression> functionsToCompile;
            for (QmlIR::CompiledFunctionOrExpression *foe = object->functionsAndExpressions->first; foe; foe = foe->next) {
                foe->disableAcceleratedLookups = true;
                functionsToCompile << *foe;
            }
            const QVector<int> runtimeFunctionIndices = v4CodeGen.generateJSCodeForFunctionsAndBindings(functionsToCompile);
            QList<QQmlJS::DiagnosticMessage> jsErrors = v4CodeGen.errors();
            if (!jsErrors.isEmpty()) {
                for (const QQmlJS::DiagnosticMessage &e: qAsConst(jsErrors)) {
                    if (!error->message.isEmpty())
                        error->message += QLatin1Char('\n');
                    error->message += diagnosticErrorMessage(inputFileName, e);
                }
                return false;
            }

            QQmlJS::MemoryPool *pool = irDocument.jsParserEngine.pool();
            object->runtimeFunctionIndices.allocate(pool, runtimeFunctionIndices);
        }

        QmlIR::QmlUnitGenerator generator;

        // ### translation binding simplification

        QV4::ExecutableAllocator allocator;
        QScopedPointer<QV4::EvalInstructionSelection> isel(iselFactory->create(/*engine*/nullptr, &allocator, &irDocument.jsModule, &irDocument.jsGenerator));
        // Disable lookups in non-standalone (aka QML) mode
        isel->setUseFastLookups(false);
        irDocument.javaScriptCompilationUnit = isel->compile(/*generate unit*/false);
        // ###
        QV4::CompiledData::ResolvedTypeReferenceMap dummyDependencies;
        QV4::CompiledData::Unit *unit = generator.generate(irDocument, /*engine*/nullptr, dummyDependencies);
        unit->flags |= QV4::CompiledData::Unit::StaticData;
        unit->flags |= QV4::CompiledData::Unit::PendingTypeCompilation;
        irDocument.javaScriptCompilationUnit->data = unit;

        if (!irDocument.javaScriptCompilationUnit->saveToDisk(inputFileName, &error->message))
            return false;

        free(unit);
    }
    return true;
}

static bool compileJSFile(const QString &inputFileName, QV4::EvalISelFactory *iselFactory, Error *error)
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
        irDocument.jsModule.sourceTimeStamp = QFileInfo(f).lastModified().toMSecsSinceEpoch();
    }

    QQmlJS::Engine *engine = &irDocument.jsParserEngine;
    QmlIR::ScriptDirectivesCollector directivesCollector(engine, &irDocument.jsGenerator);
    QQmlJS::Directives *oldDirs = engine->directives();
    engine->setDirectives(&directivesCollector);

    QQmlJS::AST::Program *program = nullptr;

    {
        QQmlJS::Lexer lexer(engine);
         lexer.setCode(sourceCode, /*line*/1, /*parseAsBinding*/false);
         QQmlJS::Parser parser(engine);

         bool parsed = parser.parseProgram();

         for (const QQmlJS::DiagnosticMessage &parseError: parser.diagnosticMessages()) {
             if (!error->message.isEmpty())
                 error->message += QLatin1Char('\n');
             error->message += diagnosticErrorMessage(inputFileName, parseError);
         }

         if (!parsed) {
             engine->setDirectives(oldDirs);
             return false;
         }

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
        QmlIR::JSCodeGen v4CodeGen(inputFileName, irDocument.code, &irDocument.jsModule, &irDocument.jsParserEngine, irDocument.program, /*import cache*/0, &irDocument.jsGenerator.stringTable);
        v4CodeGen.generateFromProgram(inputFileName, sourceCode, program, &irDocument.jsModule, QQmlJS::Codegen::GlobalCode);
        QList<QQmlJS::DiagnosticMessage> jsErrors = v4CodeGen.errors();
        if (!jsErrors.isEmpty()) {
            for (const QQmlJS::DiagnosticMessage &e: qAsConst(jsErrors)) {
                if (!error->message.isEmpty())
                    error->message += QLatin1Char('\n');
                error->message += diagnosticErrorMessage(inputFileName, e);
            }
            engine->setDirectives(oldDirs);
            return false;
        }

        QmlIR::QmlUnitGenerator generator;

        // ### translation binding simplification

        QScopedPointer<QV4::EvalInstructionSelection> isel(iselFactory->create(/*engine*/nullptr, /*executable allocator*/nullptr, &irDocument.jsModule, &irDocument.jsGenerator));
        // Disable lookups in non-standalone (aka QML) mode
        isel->setUseFastLookups(false);
        irDocument.javaScriptCompilationUnit = isel->compile(/*generate unit*/false);
        // ###
        QV4::CompiledData::ResolvedTypeReferenceMap dummyDependencies;
        QV4::CompiledData::Unit *unit = generator.generate(irDocument, /*engine*/nullptr, dummyDependencies);
        unit->flags |= QV4::CompiledData::Unit::StaticData;
        irDocument.javaScriptCompilationUnit->data = unit;

        if (!irDocument.javaScriptCompilationUnit->saveToDisk(inputFileName, &error->message)) {
            engine->setDirectives(oldDirs);
            return false;
        }

        free(unit);
    }
    engine->setDirectives(oldDirs);
    return true;
}

int main(int argc, char **argv)
{
    // Produce reliably the same output for the same input by disabling QHash's random seeding.
    qt_qhash_seed.testAndSetRelaxed(-1, 0);

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("qmlcachegen"));
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption targetArchitectureOption(QStringLiteral("target-architecture"), QCoreApplication::translate("main", "Target architecture"), QCoreApplication::translate("main", "architecture"));
    parser.addOption(targetArchitectureOption);

    parser.addPositionalArgument(QStringLiteral("[qml file]"),
            QStringLiteral("QML source file to generate cache for."));

    parser.process(app);

    const QStringList sources = parser.positionalArguments();
    if (sources.count() > 1) {
        fprintf(stderr, "%s\n", qPrintable(QStringLiteral("Too many input files specified: '") + sources.join(QStringLiteral("' '")) + QLatin1Char('\'')));
        return EXIT_FAILURE;
    }
    const QString inputFile = sources.first();

    QScopedPointer<QV4::EvalISelFactory> isel;
    const QString targetArchitecture = parser.value(targetArchitectureOption);

    isel.reset(QV4::JIT::createISelForArchitecture(targetArchitecture));

    if (!isel)
        isel.reset(new QV4::Moth::ISelFactory);

    Error error;

    if (inputFile.endsWith(QLatin1String(".qml"))) {
        if (!compileQmlFile(inputFile, isel.data(), &error)) {
            error.augment(QLatin1String("Error compiling qml file: ")).print();
            return EXIT_FAILURE;
        }
    } else if (inputFile.endsWith(QLatin1String(".js"))) {
        if (!compileJSFile(inputFile, isel.data(), &error)) {
            error.augment(QLatin1String("Error compiling qml file: ")).print();
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "Ignoring %s input file as it is not QML source code - maybe remove from QML_FILES?\n", qPrintable(inputFile));    }


    return EXIT_SUCCESS;
}
