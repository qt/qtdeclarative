// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltccommandlineutils.h"
#include "qmltcvisitor.h"
#include "qmltctyperesolver.h"

#include "qmltccompiler.h"

#include <private/qqmljscompiler_p.h>
#include <private/qqmljsresourcefilemapper_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qurl.h>
#include <QtCore/qhashfunctions.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qcommandlineparser.h>
#include <QtCore/qregularexpression.h>

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsdiagnosticmessage_p.h>

#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE

using namespace Qt::StringLiterals;

void setupLogger(QQmlJSLogger &logger) // prepare logger to work with compiler
{
    for (const QQmlJS::LoggerCategory &category : logger.categories()) {
        if (category.id() == qmlUnusedImports)
            continue;
        logger.setCategoryLevel(category.id(), QtCriticalMsg);
        logger.setCategoryIgnored(category.id(), false);
    }
}

int main(int argc, char **argv)
{
    // Produce reliably the same output for the same input by disabling QHash's
    // random seeding.
    QHashSeed::setDeterministicGlobalSeed();
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(u"qmltc"_s);
    QCoreApplication::setApplicationVersion(QStringLiteral(QT_VERSION_STR));

    // command-line parsing:
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption bareOption {
        u"bare"_s,
        QCoreApplication::translate(
                "main", "Do not include default import directories. This may be used to run "
                        "qmltc on a project using a different Qt version.")
    };
    parser.addOption(bareOption);

    QCommandLineOption importPathOption {
        u"I"_s, QCoreApplication::translate("main", "Look for QML modules in specified directory"),
        QCoreApplication::translate("main", "import directory")
    };
    parser.addOption(importPathOption);
    QCommandLineOption qmldirOption {
        u"i"_s, QCoreApplication::translate("main", "Include extra qmldir files"),
        QCoreApplication::translate("main", "qmldir file")
    };
    parser.addOption(qmldirOption);
    QCommandLineOption outputCppOption {
        u"impl"_s, QCoreApplication::translate("main", "Generated C++ source file path"),
        QCoreApplication::translate("main", "cpp path")
    };
    parser.addOption(outputCppOption);
    QCommandLineOption outputHOption {
        u"header"_s, QCoreApplication::translate("main", "Generated C++ header file path"),
        QCoreApplication::translate("main", "h path")
    };
    parser.addOption(outputHOption);
    QCommandLineOption resourceOption {
        u"resource"_s,
        QCoreApplication::translate(
                "main", "Qt resource file that might later contain one of the compiled files"),
        QCoreApplication::translate("main", "resource file name")
    };
    parser.addOption(resourceOption);
    QCommandLineOption metaResourceOption {
        u"meta-resource"_s,
        QCoreApplication::translate("main", "Qt meta information file (in .qrc format)"),
        QCoreApplication::translate("main", "meta file name")
    };
    parser.addOption(metaResourceOption);
    QCommandLineOption namespaceOption {
        u"namespace"_s, QCoreApplication::translate("main", "Namespace of the generated C++ code"),
        QCoreApplication::translate("main", "namespace")
    };
    parser.addOption(namespaceOption);
    QCommandLineOption exportOption{ u"export"_s,
                                     QCoreApplication::translate(
                                             "main", "Export macro used in the generated C++ code"),
                                     QCoreApplication::translate("main", "export") };
    parser.addOption(exportOption);
    QCommandLineOption exportIncludeOption{
        u"exportInclude"_s,
        QCoreApplication::translate(
                "main", "Header defining the export macro to be used in the generated C++ code"),
        QCoreApplication::translate("main", "exportInclude")
    };
    parser.addOption(exportIncludeOption);

    parser.process(app);

    const QStringList sources = parser.positionalArguments();
    if (sources.size() != 1) {
        if (sources.isEmpty()) {
            parser.showHelp();
        } else {
            fprintf(stderr, "%s\n",
                    qPrintable(u"Too many input files specified: '"_s + sources.join(u"' '"_s)
                               + u'\''));
        }
        return EXIT_FAILURE;
    }
    const QString inputFile = sources.first();

    QString url = parseUrlArgument(inputFile);
    if (url.isNull())
        return EXIT_FAILURE;
    if (!url.endsWith(u".qml")) {
        fprintf(stderr, "Non-QML file passed as input\n");
        return EXIT_FAILURE;
    }

    static QRegularExpression nameChecker(u"^[a-zA-Z_][a-zA-Z0-9_]*\\.qml$"_s);
    if (auto match = nameChecker.match(QUrl(url).fileName()); !match.hasMatch()) {
        fprintf(stderr,
                "The given QML filename is unsuited for type compilation: the name must consist of "
                "letters, digits and underscores, starting with "
                "a letter or an underscore and ending in '.qml'!\n");
        return EXIT_FAILURE;
    }

    QString sourceCode = loadUrl(url);
    if (sourceCode.isEmpty())
        return EXIT_FAILURE;

    QString implicitImportDirectory = getImplicitImportDirectory(url);
    if (implicitImportDirectory.isEmpty())
        return EXIT_FAILURE;

    QStringList importPaths;

    if (parser.isSet(resourceOption)) {
        importPaths.append(QLatin1String(":/qt-project.org/imports"));
        importPaths.append(QLatin1String(":/qt/qml"));
    };

    if (parser.isSet(importPathOption))
        importPaths.append(parser.values(importPathOption));

    if (!parser.isSet(bareOption))
        importPaths.append(QLibraryInfo::path(QLibraryInfo::QmlImportsPath));

    QStringList qmldirFiles = parser.values(qmldirOption);

    QString outputCppFile;
    if (!parser.isSet(outputCppOption)) {
        outputCppFile = url.first(url.size() - 3) + u"cpp"_s;
    } else {
        outputCppFile = parser.value(outputCppOption);
    }

    QString outputHFile;
    if (!parser.isSet(outputHOption)) {
        outputHFile = url.first(url.size() - 3) + u"h"_s;
    } else {
        outputHFile = parser.value(outputHOption);
    }

    if (!parser.isSet(resourceOption)) {
        fprintf(stderr, "No resource paths for file: %s\n", qPrintable(inputFile));
        return EXIT_FAILURE;
    }

    // main logic:
    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(sourceCode, /*lineno = */ 1);
    QQmlJS::Parser qmlParser(&engine);
    if (!qmlParser.parse()) {
        const auto diagnosticMessages = qmlParser.diagnosticMessages();
        for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
            fprintf(stderr, "%s\n",
                    qPrintable(QStringLiteral("%1:%2:%3: %4")
                                       .arg(inputFile)
                                       .arg(m.loc.startLine)
                                       .arg(m.loc.startColumn)
                                       .arg(m.message)));
        }
        return EXIT_FAILURE;
    }

    const QStringList resourceFiles = parser.values(resourceOption);
    QQmlJSResourceFileMapper mapper(resourceFiles);
    const QStringList metaResourceFiles = parser.values(metaResourceOption);
    QQmlJSResourceFileMapper metaDataMapper(metaResourceFiles);

    const auto firstQml = [](const QStringList &paths) {
        auto it = std::find_if(paths.cbegin(), paths.cend(),
                               [](const QString &x) { return x.endsWith(u".qml"_s); });
        if (it == paths.cend())
            return QString();
        return *it;
    };
    // verify that we can map current file to qrc (then use the qrc path later)
    const QStringList paths = mapper.resourcePaths(QQmlJSResourceFileMapper::localFileFilter(url));
    if (paths.isEmpty()) {
        fprintf(stderr, "Failed to find a resource path for file: %s\n", qPrintable(inputFile));
        return EXIT_FAILURE;
    } else if (paths.size() > 1) {
        bool good = !firstQml(paths).isEmpty();
        good &= std::any_of(paths.cbegin(), paths.cend(),
                            [](const QString &x) { return x.endsWith(u".h"_s); });
        if (!good || paths.size() > 2) {
            fprintf(stderr, "Unexpected resource paths for file: %s\n", qPrintable(inputFile));
            return EXIT_FAILURE;
        }
    }

    QmltcCompilerInfo info;
    info.outputCppFile = parser.value(outputCppOption);
    info.outputHFile = parser.value(outputHOption);
    info.resourcePath = firstQml(paths);
    info.outputNamespace = parser.value(namespaceOption);
    info.exportMacro = parser.value(exportOption);
    info.exportInclude = parser.value(exportIncludeOption);

    if (info.outputCppFile.isEmpty()) {
        fprintf(stderr, "An output C++ file is required. Pass one using --impl");
        return EXIT_FAILURE;
    }
    if (info.outputHFile.isEmpty()) {
        fprintf(stderr, "An output C++ header file is required. Pass one using --header");
        return EXIT_FAILURE;
    }

    QQmlJSImporter importer { importPaths, &mapper };
    importer.setMetaDataMapper(&metaDataMapper);
    auto createQmltcVisitor = [](const QQmlJSScope::Ptr &root, QQmlJSImporter *importer,
                                 QQmlJSLogger *logger, const QString &implicitImportDirectory,
                                 const QStringList &qmldirFiles) -> QQmlJSImportVisitor * {
        return new QmltcVisitor(root, importer, logger, implicitImportDirectory, qmldirFiles);
    };
    importer.setImportVisitorCreator(createQmltcVisitor);

    QQmlJSLogger logger;
    logger.setFileName(url);
    logger.setCode(sourceCode);
    setupLogger(logger);

    QmltcVisitor visitor(QQmlJSScope::create(), &importer, &logger,
                         QQmlJSImportVisitor::implicitImportDirectory(url, &mapper), qmldirFiles);
    visitor.setMode(QmltcVisitor::Compile);
    QmltcTypeResolver typeResolver { &importer };
    typeResolver.init(&visitor, qmlParser.rootNode());

    if (logger.hasErrors())
        return EXIT_FAILURE;

    QList<QQmlJS::DiagnosticMessage> warnings = importer.takeGlobalWarnings();
    if (!warnings.isEmpty()) {
        logger.log(QStringLiteral("Type warnings occurred while compiling file:"), qmlImport,
                   QQmlJS::SourceLocation());
        logger.processMessages(warnings, qmlImport);
        // Log_Import is critical for the compiler
        return EXIT_FAILURE;
    }

    QmltcCompiler compiler(url, &typeResolver, &visitor, &logger);
    compiler.compile(info);

    if (logger.hasErrors())
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
