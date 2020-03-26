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

#include "findunqualified.h"

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qcoreapplication.h>

#if QT_CONFIG(commandlineparser)
#include <QtCore/qcommandlineparser.h>
#endif

#ifndef QT_BOOTSTRAPPED
#include <QtCore/qlibraryinfo.h>
#endif

static bool lint_file(const QString &filename, const bool silent, const bool warnUnqualied,
                      const QStringList &qmltypeDirs, const QStringList &qmltypeFiles)
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

    if (success && !isJavaScript && warnUnqualied) {
        auto root = parser.rootNode();
        FindUnqualifiedIDVisitor v { qmltypeDirs, qmltypeFiles, code, filename, silent };
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

    QCommandLineOption checkUnqualified(QStringList() << "U" << "check-unqualified",
                                        QLatin1String("Warn about unqualified identifiers"));
    parser.addOption(checkUnqualified);

    QCommandLineOption qmltypesDirsOption(
            QStringList() << "I"
                          << "qmldirs",
            QLatin1String("Look for qmltypes files in specified directory"),
            QLatin1String("directory"));
    parser.addOption(qmltypesDirsOption);

    QCommandLineOption qmltypesFilesOption(
            QStringList() << "i"
                          << "qmltypes",
            QLatin1String("Include the specified qmltypes files"),
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
    bool warnUnqualified = parser.isSet(checkUnqualified);
    // use host qml import path as a sane default if nothing else has been provided
    QStringList qmltypeDirs = parser.isSet(qmltypesDirsOption)
            ? parser.values(qmltypesDirsOption)
#   ifndef QT_BOOTSTRAPPED
            : QStringList { QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath) };
#   else
            : QStringList {};
#   endif

    if (!parser.isSet(qmltypesFilesOption))
        qmltypeDirs << ".";

    QStringList qmltypeFiles = parser.isSet(qmltypesFilesOption) ? parser.values(qmltypesFilesOption) : QStringList {};
#else
    bool silent = false;
    bool warnUnqualified = false;
    QStringList qmltypeDirs {};
    QStringList qmltypeFiles {};
#endif
    bool success = true;
#if QT_CONFIG(commandlineparser)
    for (const QString &filename : positionalArguments)
#else
    const auto arguments = app.arguments();
    for (const QString &filename : arguments)
#endif
        success &= lint_file(filename, silent, warnUnqualified, qmltypeDirs, qmltypeFiles);

    return success ? 0 : -1;
}
