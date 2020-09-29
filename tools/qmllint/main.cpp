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

static bool lint_file(const QString &filename, const bool silent, const bool warnUnqualified,
                      const bool warnWithStatement, const bool warnInheritanceCycle,
                      const QStringList &qmlImportPaths, const QStringList &qmltypesFiles)
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
        auto root = parser.rootNode();
        FindWarningVisitor v { qmlImportPaths, qmltypesFiles, code, filename, silent,
                               warnUnqualified, warnWithStatement, warnInheritanceCycle };
        root->accept(&v);
        success = v.check();
    }

    return success;
}

int main(int argv, char *argc[])
{
    QCoreApplication app(argv, argc);
    QCoreApplication::setApplicationName("qmllint");
    QCoreApplication::setApplicationVersion("1.0");
#if QT_CONFIG(commandlineparser)
    QCommandLineParser parser;
    parser.setApplicationDescription(QLatin1String("QML syntax verifier"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption silentOption(QStringList() << "s" << "silent",
                                    QLatin1String("Don't output syntax errors"));
    parser.addOption(silentOption);

    QCommandLineOption disableCheckUnqualified(QStringList() << "no-unqualified-id",
                                               QLatin1String("Don't warn about unqualified identifiers"));
    parser.addOption(disableCheckUnqualified);

    QCommandLineOption disableCheckWithStatement(QStringList() << "no-with-statement",
                                                 QLatin1String("Don't warn about with statements"));
    parser.addOption(disableCheckWithStatement);

    QCommandLineOption disableCheckInheritanceCycle(QStringList() << "no-inheritance-cycle",
                                                    QLatin1String("Don't warn about inheritance cycles"));

    parser.addOption(disableCheckInheritanceCycle);

    QCommandLineOption qmlImportPathsOption(
            QStringList() << "I"
                          << "qmldirs",
            QLatin1String("Look for QML modules in specified directory"),
            QLatin1String("directory"));
    parser.addOption(qmlImportPathsOption);

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

    const auto positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        parser.showHelp(-1);
    }

    bool silent = parser.isSet(silentOption);
    bool warnUnqualified = !parser.isSet(disableCheckUnqualified);
    bool warnWithStatement = !parser.isSet(disableCheckWithStatement);
    bool warnInheritanceCycle = !parser.isSet(disableCheckInheritanceCycle);

    // use host qml import path as a sane default if nothing else has been provided
    QStringList qmlImportPaths = parser.isSet(qmlImportPathsOption)
            ? parser.values(qmlImportPathsOption)
#   ifndef QT_BOOTSTRAPPED
            : QStringList { QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath), QDir::currentPath() };
#   else
            : QStringList { QDir::currentPath() };
#   endif

    QStringList qmltypesFiles;
    if (parser.isSet(qmltypesFilesOption)) {
        qmltypesFiles = parser.values(qmltypesFilesOption);
    } else {
        // If none are given explicitly, use the qmltypes files from the current directory.
        QDirIterator it(".", {"*.qmltypes"}, QDir::Files);
        while (it.hasNext())
            qmltypesFiles.append(it.fileInfo().absoluteFilePath());
    }

#else
    bool silent = false;
    bool warnUnqualified = true;
    bool warnWithStatement = true;
    bool warnInheritanceCycle = true;
    QStringList qmlImportPahs {};
    QStringList qmltypesFiles {};
#endif
    bool success = true;
#if QT_CONFIG(commandlineparser)
    for (const QString &filename : positionalArguments)
#else
    const auto arguments = app.arguments();
    for (const QString &filename : arguments)
#endif
        success &= lint_file(filename, silent, warnUnqualified, warnWithStatement,
                             warnInheritanceCycle, qmlImportPaths, qmltypesFiles);

    return success ? 0 : -1;
}
