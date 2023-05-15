// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
#include <QLibraryInfo>
#include <QLoggingCategory>

#include <private/qqmlirbuilder_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsresourcefilemapper_p.h>
#include <private/qqmljsloadergenerator_p.h>
#include <private/qqmljscompiler_p.h>
#include <private/qresourcerelocater_p.h>

#include <algorithm>

static bool argumentsFromCommandLineAndFile(QStringList& allArguments, const QStringList &arguments)
{
    allArguments.reserve(arguments.size());
    for (const QString &argument : arguments) {
        // "@file" doesn't start with a '-' so we can't use QCommandLineParser for it
        if (argument.startsWith(QLatin1Char('@'))) {
            QString optionsFile = argument;
            optionsFile.remove(0, 1);
            if (optionsFile.isEmpty()) {
                fprintf(stderr, "The @ option requires an input file");
                return false;
            }
            QFile f(optionsFile);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                fprintf(stderr, "Cannot open options file specified with @");
                return false;
            }
            while (!f.atEnd()) {
                QString line = QString::fromLocal8Bit(f.readLine().trimmed());
                if (!line.isEmpty())
                    allArguments << line;
            }
        } else {
            allArguments << argument;
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    // Produce reliably the same output for the same input by disabling QHash's random seeding.
    QHashSeed::setDeterministicGlobalSeed();

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("qmlcachegen"));
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption bareOption(QStringLiteral("bare"), QCoreApplication::translate("main", "Do not include default import directories. This may be used to run qmlcachegen on a project using a different Qt version."));
    parser.addOption(bareOption);
    QCommandLineOption filterResourceFileOption(QStringLiteral("filter-resource-file"), QCoreApplication::translate("main", "Filter out QML/JS files from a resource file that can be cached ahead of time instead"));
    parser.addOption(filterResourceFileOption);
    QCommandLineOption resourceFileMappingOption(QStringLiteral("resource-file-mapping"), QCoreApplication::translate("main", "Path from original resource file to new one"), QCoreApplication::translate("main", "old-name=new-name"));
    parser.addOption(resourceFileMappingOption);
    QCommandLineOption resourceOption(QStringLiteral("resource"), QCoreApplication::translate("main", "Qt resource file that might later contain one of the compiled files"), QCoreApplication::translate("main", "resource-file-name"));
    parser.addOption(resourceOption);
    QCommandLineOption resourcePathOption(QStringLiteral("resource-path"), QCoreApplication::translate("main", "Qt resource file path corresponding to the file being compiled"), QCoreApplication::translate("main", "resource-path"));
    parser.addOption(resourcePathOption);
    QCommandLineOption resourceNameOption(QStringLiteral("resource-name"),
                                                QCoreApplication::translate("main", "Required to generate qmlcache_loader without qrc files. This is the name of the Qt resource the input files belong to."),
                                                QCoreApplication::translate("main", "compiled-file-list"));
    parser.addOption(resourceNameOption);
    QCommandLineOption directCallsOption(QStringLiteral("direct-calls"), QCoreApplication::translate("main", "This option is ignored."));
    directCallsOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(directCallsOption);
    QCommandLineOption importsOption(
                QStringLiteral("i"),
                QCoreApplication::translate("main", "Import extra qmldir"),
                QCoreApplication::translate("main", "qmldir file"));
    parser.addOption(importsOption);
    QCommandLineOption importPathOption(
                QStringLiteral("I"),
                QCoreApplication::translate("main", "Look for QML modules in specified directory"),
                QCoreApplication::translate("main", "import directory"));
    parser.addOption(importPathOption);
    QCommandLineOption onlyBytecode(
                QStringLiteral("only-bytecode"),
                QCoreApplication::translate(
                    "main", "Generate only byte code for bindings and functions, no C++ code"));
    parser.addOption(onlyBytecode);
    QCommandLineOption verboseOption(
            QStringLiteral("verbose"),
            QCoreApplication::translate("main", "Output compile warnings"));
    parser.addOption(verboseOption);

    QCommandLineOption outputFileOption(QStringLiteral("o"), QCoreApplication::translate("main", "Output file name"), QCoreApplication::translate("main", "file name"));
    parser.addOption(outputFileOption);

    parser.addPositionalArgument(QStringLiteral("[qml file]"),
            QStringLiteral("QML source file to generate cache for."));

    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);


    QStringList arguments;
    if (!argumentsFromCommandLineAndFile(arguments, app.arguments()))
        return EXIT_FAILURE;

    parser.process(arguments);

    enum Output {
        GenerateCpp,
        GenerateCacheFile,
        GenerateLoader,
        GenerateLoaderStandAlone,
    } target = GenerateCacheFile;

    QString outputFileName;
    if (parser.isSet(outputFileOption))
        outputFileName = parser.value(outputFileOption);

    if (outputFileName.endsWith(QLatin1String(".cpp"))) {
        target = GenerateCpp;
        if (outputFileName.endsWith(QLatin1String("qmlcache_loader.cpp")))
            target = GenerateLoader;
    }

    if (target == GenerateLoader && parser.isSet(resourceNameOption))
        target = GenerateLoaderStandAlone;

    const QStringList sources = parser.positionalArguments();
    if (sources.isEmpty()){
        parser.showHelp();
    } else if (sources.size() > 1 && (target != GenerateLoader && target != GenerateLoaderStandAlone)) {
        fprintf(stderr, "%s\n", qPrintable(QStringLiteral("Too many input files specified: '") + sources.join(QStringLiteral("' '")) + QLatin1Char('\'')));
        return EXIT_FAILURE;
    }

    const QString inputFile = !sources.isEmpty() ? sources.first() : QString();
    if (outputFileName.isEmpty())
        outputFileName = inputFile + QLatin1Char('c');

    if (parser.isSet(filterResourceFileOption))
        return qRelocateResourceFile(inputFile, outputFileName);

    if (target == GenerateLoader) {
        QQmlJSResourceFileMapper mapper(sources);

        QQmlJSCompileError error;
        if (!qQmlJSGenerateLoader(
                    mapper.resourcePaths(QQmlJSResourceFileMapper::allQmlJSFilter()),
                    outputFileName, parser.values(resourceFileMappingOption), &error.message)) {
            error.augment(QLatin1String("Error generating loader stub: ")).print();
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    if (target == GenerateLoaderStandAlone) {
        QQmlJSCompileError error;
        if (!qQmlJSGenerateLoader(sources, outputFileName,
                                  parser.values(resourceNameOption), &error.message)) {
            error.augment(QLatin1String("Error generating loader stub: ")).print();
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    QString inputFileUrl = inputFile;

    QQmlJSSaveFunction saveFunction;
    QQmlJSResourceFileMapper fileMapper(parser.values(resourceOption));
    QString inputResourcePath = parser.value(resourcePathOption);

    // If the user didn't specify the resource path corresponding to the file on disk being
    // compiled, try to determine it from the resource file, if one was supplied.
    if (inputResourcePath.isEmpty()) {
        const QStringList resourcePaths = fileMapper.resourcePaths(
                    QQmlJSResourceFileMapper::localFileFilter(inputFile));
        if (target == GenerateCpp && resourcePaths.isEmpty()) {
            fprintf(stderr, "No resource path for file: %s\n", qPrintable(inputFile));
            return EXIT_FAILURE;
        }

        if (resourcePaths.size() == 1) {
            inputResourcePath = resourcePaths.first();
        } else if (target == GenerateCpp) {
            fprintf(stderr, "Multiple resource paths for file %s. "
                            "Use the --%s option to disambiguate:\n",
                    qPrintable(inputFile),
                    qPrintable(resourcePathOption.names().first()));
            for (const QString &resourcePath: resourcePaths)
                fprintf(stderr, "\t%s\n", qPrintable(resourcePath));
            return EXIT_FAILURE;
        }
    }

    if (target == GenerateCpp) {
        inputFileUrl = QStringLiteral("qrc://") + inputResourcePath;
        saveFunction = [inputResourcePath, outputFileName](
                               const QV4::CompiledData::SaveableUnitPointer &unit,
                               const QQmlJSAotFunctionMap &aotFunctions,
                               QString *errorString) {
            return qSaveQmlJSUnitAsCpp(inputResourcePath, outputFileName, unit, aotFunctions,
                                       errorString);
        };

    } else {
        saveFunction = [outputFileName](const QV4::CompiledData::SaveableUnitPointer &unit,
                                        const QQmlJSAotFunctionMap &aotFunctions,
                                        QString *errorString) {
            Q_UNUSED(aotFunctions);
            return unit.saveToDisk<char>(
                    [&outputFileName, errorString](const char *data, quint32 size) {
                        return QV4::CompiledData::SaveableUnitPointer::writeDataToFile(
                                outputFileName, data, size, errorString);
            });
        };
    }

    if (inputFile.endsWith(QLatin1String(".qml"))) {
        QQmlJSCompileError error;
        if (target != GenerateCpp || inputResourcePath.isEmpty() || parser.isSet(onlyBytecode)) {
            if (!qCompileQmlFile(inputFile, saveFunction, nullptr, &error,
                                 /* storeSourceLocation */ false)) {
                error.augment(QStringLiteral("Error compiling qml file: ")).print();
                return EXIT_FAILURE;
            }
        } else {
            QStringList importPaths;

            if (parser.isSet(resourceOption)) {
                importPaths.append(QLatin1String(":/qt-project.org/imports"));
                importPaths.append(QLatin1String(":/qt/qml"));
            };

            if (parser.isSet(importPathOption))
                importPaths.append(parser.values(importPathOption));

            if (!parser.isSet(bareOption))
                importPaths.append(QLibraryInfo::path(QLibraryInfo::QmlImportsPath));

            QQmlJSImporter importer(
                        importPaths, parser.isSet(resourceOption) ? &fileMapper : nullptr);
            QQmlJSLogger logger;

            // Always trigger the qFatal() on "pragma Strict" violations.
            logger.setCategoryLevel(qmlCompiler, QtWarningMsg);
            logger.setCategoryIgnored(qmlCompiler, false);
            logger.setCategoryFatal(qmlCompiler, true);

            if (!parser.isSet(verboseOption))
                logger.setSilent(true);

            QQmlJSAotCompiler cppCodeGen(
                        &importer, u':' + inputResourcePath, parser.values(importsOption), &logger);

            if (!qCompileQmlFile(inputFile, saveFunction, &cppCodeGen, &error,
                                 /* storeSourceLocation */ true)) {
                error.augment(QStringLiteral("Error compiling qml file: ")).print();
                return EXIT_FAILURE;
            }

            QList<QQmlJS::DiagnosticMessage> warnings = importer.takeGlobalWarnings();

            if (!warnings.isEmpty()) {
                logger.log(QStringLiteral("Type warnings occurred while compiling file:"),
                           qmlImport, QQmlJS::SourceLocation());
                logger.processMessages(warnings, qmlImport);
            }
        }
    } else if (inputFile.endsWith(QLatin1String(".js")) || inputFile.endsWith(QLatin1String(".mjs"))) {
        QQmlJSCompileError error;
        if (!qCompileJSFile(inputFile, inputFileUrl, saveFunction, &error)) {
            error.augment(QLatin1String("Error compiling js file: ")).print();
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "Ignoring %s input file as it is not QML source code - maybe remove from QML_FILES?\n", qPrintable(inputFile));
    }

    return EXIT_SUCCESS;
}
