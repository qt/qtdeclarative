/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QCoreApplication>
#include <QFile>

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>

#if QT_CONFIG(commandlineparser)
#include <QCommandLineParser>
#endif

#include "commentastvisitor.h"
#include "dumpastvisitor.h"
#include "restructureastvisitor.h"

bool parseFile(const QString& filename, bool inplace, bool verbose, bool sortImports, bool force, const QString& newline)
{
    QFile file(filename);

    if (!file.open(QIODevice::Text | QIODevice::ReadOnly)) {
        qWarning().noquote() << "Failed to open" << filename << "for reading.";
        return false;
    }

    QString code = QString::fromUtf8(file.readAll());
    file.close();

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    lexer.setCode(code, 1, true);
    QQmlJS::Parser parser(&engine);

    bool success = parser.parse();

    if (!success) {
        const auto diagnosticMessages = parser.diagnosticMessages();
        for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
            qWarning().noquote() << QString::fromLatin1("%1:%2 : %3")
                                    .arg(filename).arg(m.loc.startLine).arg(m.message);
        }

        qWarning().noquote() << "Failed to parse" << filename;
        return false;
    }

    // Try to attach comments to AST nodes
    CommentAstVisitor comment(&engine, parser.rootNode());

    if (verbose)
        qWarning().noquote() << comment.attachedComments().size() << "comment(s) attached.";

    if (verbose) {
        int orphaned = 0;

        for (const auto& orphanList : comment.orphanComments().values())
            orphaned += orphanList.size();

        qWarning().noquote() << orphaned << "comments are orphans.";
    }

    if (verbose && sortImports)
        qWarning().noquote() << "Sorting imports";

    // Do the actual restructuring
    RestructureAstVisitor restructure(parser.rootNode(), sortImports);

    // Turn AST back into source code
    if (verbose)
        qWarning().noquote() << "Dumping" << filename;

    DumpAstVisitor dump(&engine, parser.rootNode(), &comment);

    QString dumpCode = dump.toString();

    lexer.setCode(dumpCode, 1, true);

    bool dumpSuccess = parser.parse();

    if (!dumpSuccess) {
        if (verbose) {
            const auto diagnosticMessages = parser.diagnosticMessages();
            for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
              qWarning().noquote() << QString::fromLatin1("<formatted>:%2 : %3")
                                      .arg(m.loc.startLine).arg(m.message);
            }
        }

        qWarning().noquote() << "Failed to parse formatted code.";
    }

    if (dump.error() || !dumpSuccess) {
        if (force) {
            qWarning().noquote() << "An error has occurred. The output may not be reliable.";
        } else {
            qWarning().noquote() << "An error has occurred. Aborting.";
            return false;
        }
   }


    const bool native = newline == "native";

    if (!native) {
        if (newline == "macos") {
            dumpCode = dumpCode.replace("\n","\r");
        } else if (newline == "windows") {
            dumpCode = dumpCode.replace("\n", "\r\n");
        } else if (newline == "unix") {
            // Nothing needs to be done for unix line-endings
        } else {
            qWarning().noquote() << "Unknown line ending type" << newline;
            return false;
        }
    }

    if (inplace) {
        if (verbose)
            qWarning().noquote() << "Writing to file" << filename;

        if (!file.open(native ? QIODevice::WriteOnly | QIODevice::Text : QIODevice::WriteOnly))
        {
            qWarning().noquote() << "Failed to open" << filename << "for writing";
            return false;
        }

        file.write(dumpCode.toUtf8());
        file.close();
    } else {
        QFile out;
        out.open(stdout, QIODevice::WriteOnly);
        out.write(dumpCode.toUtf8());
    }

    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("qmlformat");
    QCoreApplication::setApplicationVersion("1.0");

    bool success = true;
#if QT_CONFIG(commandlineparser)
    QCommandLineParser parser;
    parser.setApplicationDescription("Formats QML files according to the QML Coding Conventions.");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption(QCommandLineOption({"V", "verbose"},
                     QStringLiteral("Verbose mode. Outputs more detailed information.")));

    parser.addOption(QCommandLineOption({"n", "no-sort"},
                     QStringLiteral("Do not sort imports.")));

    parser.addOption(QCommandLineOption({"i", "inplace"},
                     QStringLiteral("Edit file in-place instead of outputting to stdout.")));

    parser.addOption(QCommandLineOption({"f", "force"},
                     QStringLiteral("Continue even if an error has occurred.")));

    parser.addOption(QCommandLineOption({"l", "newline"},
                     QStringLiteral("Override the new line format to use (native macos unix windows)."),
                     "newline", "native"));

    parser.addPositionalArgument("filenames", "files to be processed by qmlformat");

    parser.process(app);

    const auto positionalArguments = parser.positionalArguments();

    if (positionalArguments.isEmpty())
        parser.showHelp(-1);

    if (!parser.isSet("inplace") && parser.value("newline") != "native") {
        qWarning() << "Error: The -l option can only be used with -i";
        return -1;
    }

    for (const QString& file: parser.positionalArguments()) {
        if (!parseFile(file, parser.isSet("inplace"), parser.isSet("verbose"), !parser.isSet("no-sort"), parser.isSet("force"), parser.value("newline")))
            success = false;
    }
#endif

    return success ? 0 : 1;
}
