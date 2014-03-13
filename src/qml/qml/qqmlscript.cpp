/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlscript_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsmemorypool_p.h>
#include <private/qqmljsastvisitor_p.h>
#include <private/qqmljsast_p.h>

#include <QStack>
#include <QStringList>
#include <QCoreApplication>
#include <QtDebug>

QT_BEGIN_NAMESPACE

using namespace QQmlJS;
using namespace QQmlScript;

static void replaceWithSpace(QString &str, int idx, int n)
{
    QChar *data = str.data() + idx;
    const QChar space(QLatin1Char(' '));
    for (int ii = 0; ii < n; ++ii)
        *data++ = space;
}

static QQmlScript::LocationSpan
locationFromLexer(const QQmlJS::Lexer &lex, int startLine, int startColumn, int startOffset)
{
    QQmlScript::LocationSpan l;

    l.start.line = startLine; l.start.column = startColumn;
    l.end.line = lex.tokenEndLine(); l.end.column = lex.tokenEndColumn();
    l.range.offset = startOffset;
    l.range.length = lex.tokenOffset() + lex.tokenLength() - startOffset;

    return l;
}

/*
Searches for ".pragma <value>" declarations within \a script.  Currently supported pragmas
are:
    library
*/
QQmlScript::Object::ScriptBlock::Pragmas QQmlScript::Parser::extractPragmas(QString &script)
{
    QQmlScript::Object::ScriptBlock::Pragmas rv = QQmlScript::Object::ScriptBlock::None;

    const QString pragma(QLatin1String("pragma"));
    const QString library(QLatin1String("library"));

    QQmlJS::Lexer l(0);
    l.setCode(script, 0);

    int token = l.lex();

    while (true) {
        if (token != QQmlJSGrammar::T_DOT)
            return rv;

        int startOffset = l.tokenOffset();
        int startLine = l.tokenStartLine();

        token = l.lex();

        if (token != QQmlJSGrammar::T_PRAGMA ||
            l.tokenStartLine() != startLine ||
            script.mid(l.tokenOffset(), l.tokenLength()) != pragma)
            return rv;

        token = l.lex();

        if (token != QQmlJSGrammar::T_IDENTIFIER ||
            l.tokenStartLine() != startLine)
            return rv;

        QString pragmaValue = script.mid(l.tokenOffset(), l.tokenLength());
        int endOffset = l.tokenLength() + l.tokenOffset();

        token = l.lex();
        if (l.tokenStartLine() == startLine)
            return rv;

        if (pragmaValue == library) {
            rv |= QQmlScript::Object::ScriptBlock::Shared;
            replaceWithSpace(script, startOffset, endOffset - startOffset);
        } else {
            return rv;
        }
    }
    return rv;
}

#define CHECK_LINE if (l.tokenStartLine() != startLine) return rv;
#define CHECK_TOKEN(t) if (token != QQmlJSGrammar:: t) return rv;

static const int uriTokens[] = {
    QQmlJSGrammar::T_IDENTIFIER,
    QQmlJSGrammar::T_PROPERTY,
    QQmlJSGrammar::T_SIGNAL,
    QQmlJSGrammar::T_READONLY,
    QQmlJSGrammar::T_ON,
    QQmlJSGrammar::T_BREAK,
    QQmlJSGrammar::T_CASE,
    QQmlJSGrammar::T_CATCH,
    QQmlJSGrammar::T_CONTINUE,
    QQmlJSGrammar::T_DEFAULT,
    QQmlJSGrammar::T_DELETE,
    QQmlJSGrammar::T_DO,
    QQmlJSGrammar::T_ELSE,
    QQmlJSGrammar::T_FALSE,
    QQmlJSGrammar::T_FINALLY,
    QQmlJSGrammar::T_FOR,
    QQmlJSGrammar::T_FUNCTION,
    QQmlJSGrammar::T_IF,
    QQmlJSGrammar::T_IN,
    QQmlJSGrammar::T_INSTANCEOF,
    QQmlJSGrammar::T_NEW,
    QQmlJSGrammar::T_NULL,
    QQmlJSGrammar::T_RETURN,
    QQmlJSGrammar::T_SWITCH,
    QQmlJSGrammar::T_THIS,
    QQmlJSGrammar::T_THROW,
    QQmlJSGrammar::T_TRUE,
    QQmlJSGrammar::T_TRY,
    QQmlJSGrammar::T_TYPEOF,
    QQmlJSGrammar::T_VAR,
    QQmlJSGrammar::T_VOID,
    QQmlJSGrammar::T_WHILE,
    QQmlJSGrammar::T_CONST,
    QQmlJSGrammar::T_DEBUGGER,
    QQmlJSGrammar::T_RESERVED_WORD,
    QQmlJSGrammar::T_WITH,

    QQmlJSGrammar::EOF_SYMBOL
};
static inline bool isUriToken(int token)
{
    const int *current = uriTokens;
    while (*current != QQmlJSGrammar::EOF_SYMBOL) {
        if (*current == token)
            return true;
        ++current;
    }
    return false;
}

static void extractVersion(QStringRef string, int *maj, int *min)
{
    *maj = -1; *min = -1;

    if (!string.isEmpty()) {

        int dot = string.indexOf(QLatin1Char('.'));

        if (dot < 0) {
            *maj = string.toInt();
            *min = 0;
        } else {
            *maj = string.left(dot).toInt();
            *min = string.mid(dot + 1).toInt();
        }
    }
}

QQmlScript::Parser::JavaScriptMetaData QQmlScript::Parser::extractMetaData(QString &script, QQmlError *error)
{
    Q_ASSERT(error);

    JavaScriptMetaData rv;

    QQmlScript::Object::ScriptBlock::Pragmas &pragmas = rv.pragmas;

    const QString js(QLatin1String(".js"));
    const QString library(QLatin1String("library"));

    QQmlJS::Lexer l(0);
    l.setCode(script, 0);

    int token = l.lex();

    while (true) {
        if (token != QQmlJSGrammar::T_DOT)
            return rv;

        int startOffset = l.tokenOffset();
        int startLine = l.tokenStartLine();
        int startColumn = l.tokenStartColumn();

        QQmlError importError;
        importError.setLine(startLine + 1); // 0-based, adjust to be 1-based

        token = l.lex();

        CHECK_LINE;

        if (token == QQmlJSGrammar::T_IMPORT) {

            // .import <URI> <Version> as <Identifier>
            // .import <file.js> as <Identifier>

            token = l.lex();

            CHECK_LINE;

            if (token == QQmlJSGrammar::T_STRING_LITERAL) {

                QString file = l.tokenText();

                if (!file.endsWith(js)) {
                    importError.setDescription(QCoreApplication::translate("QQmlParser","Imported file must be a script"));
                    importError.setColumn(l.tokenStartColumn());
                    *error = importError;
                    return rv;
                }

                bool invalidImport = false;

                token = l.lex();

                if ((token != QQmlJSGrammar::T_AS) || (l.tokenStartLine() != startLine)) {
                    invalidImport = true;
                } else {
                    token = l.lex();

                    if ((token != QQmlJSGrammar::T_IDENTIFIER) || (l.tokenStartLine() != startLine))
                        invalidImport = true;
                }


                if (invalidImport) {
                    importError.setDescription(QCoreApplication::translate("QQmlParser","File import requires a qualifier"));
                    importError.setColumn(l.tokenStartColumn());
                    *error = importError;
                    return rv;
                }

                int endOffset = l.tokenLength() + l.tokenOffset();

                QString importId = script.mid(l.tokenOffset(), l.tokenLength());

                QQmlScript::LocationSpan location =
                    locationFromLexer(l, startLine, startColumn, startOffset);

                token = l.lex();

                if (!importId.at(0).isUpper() || (l.tokenStartLine() == startLine)) {
                    importError.setDescription(QCoreApplication::translate("QQmlParser","Invalid import qualifier"));
                    importError.setColumn(l.tokenStartColumn());
                    *error = importError;
                    return rv;
                }

                replaceWithSpace(script, startOffset, endOffset - startOffset);

                Import import;
                import.type = Import::Script;
                import.uri = file;
                import.qualifier = importId;
                import.location = location;

                rv.imports << import;
            } else {
                // URI
                QString uri;

                while (true) {
                    if (!isUriToken(token)) {
                        importError.setDescription(QCoreApplication::translate("QQmlParser","Invalid module URI"));
                        importError.setColumn(l.tokenStartColumn());
                        *error = importError;
                        return rv;
                    }

                    uri.append(l.tokenText());

                    token = l.lex();
                    CHECK_LINE;
                    if (token != QQmlJSGrammar::T_DOT)
                        break;

                    uri.append(QLatin1Char('.'));

                    token = l.lex();
                    CHECK_LINE;
                }

                if (token != QQmlJSGrammar::T_NUMERIC_LITERAL) {
                    importError.setDescription(QCoreApplication::translate("QQmlParser","Module import requires a version"));
                    importError.setColumn(l.tokenStartColumn());
                    *error = importError;
                    return rv;
                }

                int vmaj, vmin;
                extractVersion(QStringRef(&script, l.tokenOffset(), l.tokenLength()),
                                           &vmaj, &vmin);

                bool invalidImport = false;

                token = l.lex();

                if ((token != QQmlJSGrammar::T_AS) || (l.tokenStartLine() != startLine)) {
                    invalidImport = true;
                } else {
                    token = l.lex();

                    if ((token != QQmlJSGrammar::T_IDENTIFIER) || (l.tokenStartLine() != startLine))
                        invalidImport = true;
                }


                if (invalidImport) {
                    importError.setDescription(QCoreApplication::translate("QQmlParser","Module import requires a qualifier"));
                    importError.setColumn(l.tokenStartColumn());
                    *error = importError;
                    return rv;
                }

                int endOffset = l.tokenLength() + l.tokenOffset();

                QString importId = script.mid(l.tokenOffset(), l.tokenLength());

                QQmlScript::LocationSpan location =
                    locationFromLexer(l, startLine, startColumn, startOffset);

                token = l.lex();

                if (!importId.at(0).isUpper() || (l.tokenStartLine() == startLine)) {
                    importError.setDescription(QCoreApplication::translate("QQmlParser","Invalid import qualifier"));
                    importError.setColumn(l.tokenStartColumn());
                    *error = importError;
                    return rv;
                }

                replaceWithSpace(script, startOffset, endOffset - startOffset);

                Import import;
                import.type = Import::Library;
                import.uri = uri;
                import.majorVersion = vmaj;
                import.minorVersion = vmin;
                import.qualifier = importId;
                import.location = location;

                rv.imports << import;
            }
        } else if (token == QQmlJSGrammar::T_PRAGMA) {
            token = l.lex();

            CHECK_TOKEN(T_IDENTIFIER);
            CHECK_LINE;

            QString pragmaValue = script.mid(l.tokenOffset(), l.tokenLength());
            int endOffset = l.tokenLength() + l.tokenOffset();

            if (pragmaValue == library) {
                pragmas |= QQmlScript::Object::ScriptBlock::Shared;
                replaceWithSpace(script, startOffset, endOffset - startOffset);
            } else {
                return rv;
            }

            token = l.lex();
            if (l.tokenStartLine() == startLine)
                return rv;

        } else {
            return rv;
        }
    }
    return rv;
}

QT_END_NAMESPACE
