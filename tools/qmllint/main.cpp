/****************************************************************************
**
** Copyright (C) 2016 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sergio Martins <sergio.martins@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "findwarnings.h"

#include <QtQmlCompiler/private/qqmljsresourcefilemapper_p.h>

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdiriterator.h>

#if QT_CONFIG(commandlineparser)
#include <QtCore/qcommandlineparser.h>
#endif

#ifndef QT_BOOTSTRAPPED
#include <QtCore/qlibraryinfo.h>
#endif

static bool lint_file(const QString &filename, const bool silent, const QStringList &qmlImportPaths,
                      const QStringList &qmltypesFiles, const QString &resourceFile,
                      const QMap<QString, QQmlJSLogger::Option> &options, QQmlJSImporter &importer)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        if (!silent)
            qWarning() << "Failed to open file" << filename << file.error();
        return false;
    }

    QString code = QString::fromUtf8(file.readAll());
    file.close();

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    QFileInfo info(filename);
    const QString lowerSuffix = info.suffix().toLower();
    const bool isESModule = lowerSuffix == QLatin1String("mjs");
    const bool isJavaScript = isESModule || lowerSuffix == QLatin1String("js");

    lexer.setCode(code, /*lineno = */ 1, /*qmlMode=*/ !isJavaScript);
    QQmlJS::Parser parser(&engine);

    bool success = isJavaScript ? (isESModule ? parser.parseModule() : parser.parseProgram())
                                : parser.parse();

    if (!success && !silent) {
        const auto diagnosticMessages = parser.diagnosticMessages();
        for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
            qWarning().noquote() << QString::fromLatin1("%1:%2 : %3")
                                    .arg(filename).arg(m.loc.startLine).arg(m.message);
        }
    }

    if (success && !isJavaScript) {
        const auto check = [&](QQmlJSResourceFileMapper *mapper) {
            if (importer.importPaths() != qmlImportPaths)
                importer.setImportPaths(qmlImportPaths);

            importer.setResourceFileMapper(mapper);

            FindWarningVisitor v { &importer, qmltypesFiles, code, filename, silent };

            for (auto it = options.cbegin(); it != options.cend(); ++it) {
                v.logger().setCategoryDisabled(it.value().m_category, it.value().m_disabled);
                v.logger().setCategoryLevel(it.value().m_category, it.value().m_level);
            }

            parser.rootNode()->accept(&v);
            success = v.check();
        };

        if (resourceFile.isEmpty()) {
            check(nullptr);
        } else {
            QQmlJSResourceFileMapper mapper({ resourceFile });
            check(&mapper);
        }
    }

    return success;
}

int main(int argv, char *argc[])
{
    qSetGlobalQHashSeed(0);
    QMap<QString, QQmlJSLogger::Option> options = QQmlJSLogger::options();

    QCoreApplication app(argv, argc);
    QCoreApplication::setApplicationName("qmllint");
    QCoreApplication::setApplicationVersion("1.0");
#if QT_CONFIG(commandlineparser)
    QCommandLineParser parser;
    parser.setApplicationDescription(QLatin1String(R"(QML syntax verifier and analyzer

All warnings can be set to three levels:
    disabled - Fully disables the warning.
    info - Displays the warning but does not influence the return code.
    warning - Displays the warning and leads to a non-zero exit code if encountered.
)"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption silentOption(QStringList() << "s" << "silent",
                                    QLatin1String("Don't output syntax errors"));
    parser.addOption(silentOption);

    for (auto it = options.cbegin(); it != options.cend(); ++it) {
        QCommandLineOption option(it.key(), it.value().m_description + QStringLiteral(" (default: %1)").arg(it.value().levelToString()) , QStringLiteral("level"));
        parser.addOption(option);
    }

    // TODO: Remove after Qt 6.2
    QCommandLineOption disableCheckUnqualified(QStringList() << "no-unqualified-id",
                                               QLatin1String("Don't warn about unqualified identifiers (deprecated, please use --unqualified disable instead)"));
    parser.addOption(disableCheckUnqualified);

    QCommandLineOption disableCheckWithStatement(QStringList() << "no-with-statement",
                                                 QLatin1String("Don't warn about with statements (deprecated, please use --with-statements disable instead)"));
    parser.addOption(disableCheckWithStatement);

    QCommandLineOption disableCheckInheritanceCycle(QStringList() << "no-inheritance-cycle",
                                                    QLatin1String("Don't warn about inheritance cycles (deprecated, please use --inheritance-cycle disable instead"));
    parser.addOption(disableCheckInheritanceCycle);

    QCommandLineOption resourceOption(
                { QStringLiteral("resource") },
                QStringLiteral("Look for related files in the given resource file"),
                QStringLiteral("resource"));
    parser.addOption(resourceOption);

    QCommandLineOption qmlImportPathsOption(
            QStringList() << "I"
                          << "qmldirs",
            QLatin1String("Look for QML modules in specified directory"),
            QLatin1String("directory"));
    parser.addOption(qmlImportPathsOption);

    QCommandLineOption qmlImportNoDefault(
                QStringList() << "bare",
                QLatin1String("Do not include default import directories or the current directory. "
                              "This may be used to run qmllint on a project using a different Qt version."));
    parser.addOption(qmlImportNoDefault);

    QCommandLineOption qmltypesFilesOption(
            QStringList() << "i"
                          << "qmltypes",
            QLatin1String("Include the specified qmltypes files. By default, all qmltypes files "
                          "found in the current directory are used. When this option is set, you "
                          "have to explicitly add files from the current directory if you want "
                          "them to be used."),
            QLatin1String("qmltypes"));
    parser.addOption(qmltypesFilesOption);

    parser.addPositionalArgument(QLatin1String("files"),
                                 QLatin1String("list of qml or js files to verify"));

    parser.process(app);

    for (auto it = options.begin(); it != options.end(); ++it) {
        if (parser.isSet(it.key())) {
            const QString value = parser.value(it.key());
            auto &option = it.value();

            if (!option.setLevel(value)) {
                qWarning() << "Invalid logging level" << value << "provided for" << it.key() << "(allowed are: disable, info, warning)";
                parser.showHelp(-1);
            }
        }
    }

    const auto positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        parser.showHelp(-1);
    }

    bool silent = parser.isSet(silentOption);

    // TODO: Remove after Qt 6.2
    bool NoWarnUnqualified = parser.isSet(disableCheckUnqualified);
    bool NoWarnWithStatement = parser.isSet(disableCheckWithStatement);
    bool NoWarnInheritanceCycle = parser.isSet(disableCheckInheritanceCycle);

    if (NoWarnUnqualified) {
        options[QStringLiteral("unqualified")].m_disabled = true;
        qWarning() << "Warning: --no-unqualified-id is deprecated. See --help.";
    }

    if (NoWarnWithStatement) {
        options[QStringLiteral("with")].m_disabled = true;
        qWarning() << "Warning: --no-with-statement is deprecated. See --help.";
    }

    if (NoWarnInheritanceCycle) {
        options[QStringLiteral("inheritance-cycle")].m_disabled = true;
        qWarning() << "Warning: --no-inheritance-cycle is deprecated. See --help.";
    }

    // use host qml import path as a sane default if not explicitly disabled
    QStringList qmlImportPaths = parser.isSet(qmlImportNoDefault)
            ? QStringList {}
#   ifndef QT_BOOTSTRAPPED
            : QStringList { QLibraryInfo::path(QLibraryInfo::QmlImportsPath), QDir::currentPath() };
#   else
            : QStringList { QDir::currentPath() };
#   endif

    if (parser.isSet(qmlImportPathsOption))
        qmlImportPaths << parser.values(qmlImportPathsOption);

    QStringList qmltypesFiles;
    if (parser.isSet(qmltypesFilesOption)) {
        qmltypesFiles = parser.values(qmltypesFilesOption);
    } else {
        // If none are given explicitly, use the qmltypes files from the current directory.
        QDirIterator it(".", {"*.qmltypes"}, QDir::Files);
        while (it.hasNext()) {
            it.next();
            qmltypesFiles.append(it.fileInfo().absoluteFilePath());
        }
    }

    const QString resourceFile = parser.value(resourceOption);

#else
    bool silent = false;
    bool warnUnqualified = true;
    bool warnWithStatement = true;
    bool warnInheritanceCycle = true;
    QStringList qmlImportPahs {};
    QStringList qmltypesFiles {};
#endif
    bool success = true;
    QQmlJSImporter importer(qmlImportPaths, nullptr);

#if QT_CONFIG(commandlineparser)
    for (const QString &filename : positionalArguments)
#else
    const auto arguments = app.arguments();
    for (const QString &filename : arguments)
#endif
        success &= lint_file(filename, silent, qmlImportPaths, qmltypesFiles, resourceFile, options,
                             importer);

    return success ? 0 : -1;
}
