/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "qmltccommandlineutils.h"
#include "prototype/codegenerator.h"
#include "prototype/visitor.h"
#include "prototype/typeresolver.h"

#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljscompiler_p.h>
#include <private/qqmljsresourcefilemapper_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qurl.h>
#include <QtCore/qhashfunctions.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qlibraryinfo.h>
#if QT_CONFIG(commandlineparser)
#    include <QtCore/qcommandlineparser.h>
#endif

#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE

void setupLogger(QQmlJSLogger &logger) // prepare logger to work with compiler
{
    // TODO: support object bindings and change to setCategoryLevel(QtInfoMsg)
    logger.setCategoryError(Log_Compiler, true);
}

int main(int argc, char **argv)
{
    // Produce reliably the same output for the same input by disabling QHash's
    // random seeding.
    qSetGlobalQHashSeed(0);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(u"qmltc"_qs);
    QCoreApplication::setApplicationVersion(QStringLiteral(QT_VERSION_STR));

#if QT_CONFIG(commandlineparser)
    // command-line parsing:
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption importPathOption {
        u"I"_qs, QCoreApplication::translate("main", "Look for QML modules in specified directory"),
        QCoreApplication::translate("main", "import directory")
    };
    parser.addOption(importPathOption);
    QCommandLineOption qmldirOption {
        u"i"_qs, QCoreApplication::translate("main", "Include extra qmldir files"),
        QCoreApplication::translate("main", "qmldir file")
    };
    parser.addOption(qmldirOption);
    QCommandLineOption outputCppOption {
        u"impl"_qs, QCoreApplication::translate("main", "Generated C++ source file path"),
        QCoreApplication::translate("main", "cpp path")
    };
    parser.addOption(outputCppOption);
    QCommandLineOption outputHOption {
        u"header"_qs, QCoreApplication::translate("main", "Generated C++ header file path"),
        QCoreApplication::translate("main", "h path")
    };
    parser.addOption(outputHOption);

    QCommandLineOption resourcePathOption {
        u"resource-path"_qs,
        QCoreApplication::translate(
                "main", "Qt resource file path corresponding to the file being compiled"),
        QCoreApplication::translate("main", "resource path")
    };
    parser.addOption(resourcePathOption);
    QCommandLineOption resourceOption {
        u"resource"_qs,
        QCoreApplication::translate(
                "main", "Qt resource file that might later contain one of the compiled files"),
        QCoreApplication::translate("main", "resource file name")
    };
    parser.addOption(resourceOption);
    QCommandLineOption namespaceOption {
        u"namespace"_qs, QCoreApplication::translate("main", "Namespace of the generated C++ code"),
        QCoreApplication::translate("main", "namespace")
    };
    parser.addOption(namespaceOption);

    parser.process(app);

    const QStringList sources = parser.positionalArguments();
    if (sources.size() != 1) {
        if (sources.isEmpty()) {
            parser.showHelp();
        } else {
            fprintf(stderr, "%s\n",
                    qPrintable(u"Too many input files specified: '"_qs + sources.join(u"' '"_qs)
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

    QString sourceCode = loadUrl(url);
    if (sourceCode.isEmpty())
        return EXIT_FAILURE;

    QString implicitImportDirectory = getImplicitImportDirectory(url);
    if (implicitImportDirectory.isEmpty())
        return EXIT_FAILURE;

    QStringList importPaths = parser.values(importPathOption);
    importPaths.append(QLibraryInfo::path(QLibraryInfo::QmlImportsPath));
    QStringList qmldirFiles = parser.values(qmldirOption);

    QString outputCppFile;
    if (!parser.isSet(outputCppOption)) {
        outputCppFile = url.first(url.size() - 3) + u"cpp"_qs;
    } else {
        outputCppFile = parser.value(outputCppOption);
    }

    QString outputHFile;
    if (!parser.isSet(outputHOption)) {
        outputHFile = url.first(url.size() - 3) + u"h"_qs;
    } else {
        outputHFile = parser.value(outputHOption);
    }

    if (!parser.isSet(resourceOption) && !parser.isSet(resourcePathOption)) {
        fprintf(stderr, "No resource paths for file: %s\n", qPrintable(inputFile));
        return EXIT_FAILURE;
    }

    // main logic:
    QmlIR::Document document(false); // used by QmltcTypeResolver/QQmlJSTypeResolver
    // NB: JS unit generated here is ignored, so use noop function
    QQmlJSSaveFunction noop([](auto &&...) { return true; });
    QQmlJSCompileError error;
    if (!qCompileQmlFile(document, url, noop, nullptr, &error)) {
        error.augment(u"Error compiling qml file: "_qs).print();
        return EXIT_FAILURE;
    }

    const QStringList resourceFiles = parser.values(resourceOption);
    QQmlJSResourceFileMapper mapper(resourceFiles);

    // verify that we can map current file to qrc (then use the qrc path later)
    const QStringList paths = mapper.resourcePaths(QQmlJSResourceFileMapper::localFileFilter(url));
    QString resolvedResourcePath;
    if (paths.size() != 1) {
        if (parser.isSet(resourcePathOption)) {
            qWarning("--resource-path option is deprecated. Prefer --resource along with "
                     "automatically generated resource file");
            resolvedResourcePath = parser.value(resourcePathOption);
        } else if (paths.isEmpty()) {
            fprintf(stderr, "Failed to find a resource path for file: %s\n", qPrintable(inputFile));
            return EXIT_FAILURE;
        } else if (paths.size() > 1) {
            fprintf(stderr, "Too many (expected 1) resource paths for file: %s\n",
                    qPrintable(inputFile));
            return EXIT_FAILURE;
        }
    } else {
        resolvedResourcePath = paths.first();
    }

    Options options;
    options.outputCppFile = parser.value(outputCppOption);
    options.outputHFile = parser.value(outputHOption);
    options.resourcePath = resolvedResourcePath;
    options.outNamespace = parser.value(namespaceOption);

    QQmlJSImporter importer { importPaths, &mapper };
    QQmlJSLogger logger;
    logger.setFileName(url);
    logger.setCode(sourceCode);
    setupLogger(logger);

    QQmlJSScope::Ptr target = QQmlJSScope::create();
    Qmltc::Visitor visitor(target, &importer, &logger,
                           QQmlJSImportVisitor::implicitImportDirectory(url, &mapper), qmldirFiles);
    Qmltc::TypeResolver typeResolver { &importer };
    typeResolver.init(visitor, document.program);

    if (logger.hasWarnings() || logger.hasErrors())
        return EXIT_FAILURE;

    CodeGenerator generator(url, &logger, &document, &typeResolver);
    generator.generate(options);

    if (logger.hasWarnings() || logger.hasErrors())
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
#else
    // we need the parser at least for --resource-path option (and maybe for
    // something else in the future), so just fail here if QCommandLine parser
    // is unavailable
    fprintf(stderr,
            "qmltc requires commandlineparser feature enabled. Rebuild Qt with that feature "
            "present if you want to use this tool\n");
    return EXIT_FAILURE;
#endif
}
