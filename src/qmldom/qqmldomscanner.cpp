// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmldomscanner_p.h"
#include "qqmldomerrormessage_p.h"
#include "qqmljsgrammar_p.h"

#include <QtCore/QMetaEnum>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace QQmlJS::Dom;

static void addLexToken(QList<Token> &tokens, int tokenKind, QQmlJS::Lexer &lexer,
                        bool &regexpMayFollow)
{
    switch (tokenKind) {
    case QQmlJSGrammar::T_DIVIDE_:
    case QQmlJSGrammar::T_DIVIDE_EQ:
        if (lexer.state().currentChar.isSpace()) {
            regexpMayFollow = false;
            break;
        }
        if (regexpMayFollow) {
            QQmlJS::Lexer::RegExpBodyPrefix prefix;
            if (tokenKind == QQmlJSGrammar::T_DIVIDE_)
                prefix = QQmlJS::Lexer::NoPrefix;
            else
                prefix = QQmlJS::Lexer::EqualPrefix;
            if (lexer.scanRegExp(prefix)) {
                regexpMayFollow = false;
                break;
            } else {
                qCWarning(domLog) << "lexing error scannign regexp in" << lexer.code()
                                  << lexer.errorCode() << lexer.errorMessage();
            }
            break;
        } else if (tokenKind == QQmlJSGrammar::T_DIVIDE_) {
            regexpMayFollow = true;
        }
        Q_FALLTHROUGH();

    case QQmlJSGrammar::T_AND:
    case QQmlJSGrammar::T_AND_AND:
    case QQmlJSGrammar::T_AND_EQ:
    case QQmlJSGrammar::T_ARROW:
    case QQmlJSGrammar::T_EQ:
    case QQmlJSGrammar::T_EQ_EQ:
    case QQmlJSGrammar::T_EQ_EQ_EQ:
    case QQmlJSGrammar::T_GE:
    case QQmlJSGrammar::T_GT:
    case QQmlJSGrammar::T_GT_GT:
    case QQmlJSGrammar::T_GT_GT_EQ:
    case QQmlJSGrammar::T_GT_GT_GT:
    case QQmlJSGrammar::T_GT_GT_GT_EQ:
    case QQmlJSGrammar::T_LE:
    case QQmlJSGrammar::T_LT:
    case QQmlJSGrammar::T_LT_LT:
    case QQmlJSGrammar::T_LT_LT_EQ:
    case QQmlJSGrammar::T_MINUS:
    case QQmlJSGrammar::T_MINUS_EQ:
    case QQmlJSGrammar::T_MINUS_MINUS:
    case QQmlJSGrammar::T_NOT:
    case QQmlJSGrammar::T_NOT_EQ:
    case QQmlJSGrammar::T_NOT_EQ_EQ:
    case QQmlJSGrammar::T_OR:
    case QQmlJSGrammar::T_OR_EQ:
    case QQmlJSGrammar::T_OR_OR:
    case QQmlJSGrammar::T_PLUS:
    case QQmlJSGrammar::T_PLUS_EQ:
    case QQmlJSGrammar::T_PLUS_PLUS:
    case QQmlJSGrammar::T_QUESTION:
    case QQmlJSGrammar::T_QUESTION_DOT:
    case QQmlJSGrammar::T_QUESTION_QUESTION:
    case QQmlJSGrammar::T_REMAINDER:
    case QQmlJSGrammar::T_REMAINDER_EQ:
    case QQmlJSGrammar::T_STAR:
    case QQmlJSGrammar::T_STAR_EQ:
    case QQmlJSGrammar::T_STAR_STAR:
    case QQmlJSGrammar::T_STAR_STAR_EQ:
    case QQmlJSGrammar::T_TILDE:
    case QQmlJSGrammar::T_XOR:
    case QQmlJSGrammar::T_XOR_EQ:

    case QQmlJSGrammar::T_AT:

    case QQmlJSGrammar::T_AUTOMATIC_SEMICOLON:
    case QQmlJSGrammar::T_COMPATIBILITY_SEMICOLON:
    case QQmlJSGrammar::T_SEMICOLON:

    case QQmlJSGrammar::T_COLON:
    case QQmlJSGrammar::T_COMMA:
    case QQmlJSGrammar::T_LBRACE:
    case QQmlJSGrammar::T_LBRACKET:
    case QQmlJSGrammar::T_LPAREN:

    case QQmlJSGrammar::T_ELLIPSIS:
        regexpMayFollow = true;
        break;

    case QQmlJSGrammar::T_FUNCTION:
        // might contain a space at the end...
        tokens.append(Token(lexer.tokenStartColumn() - 1,
                            lexer.tokenLength() - ((lexer.tokenText().endsWith(u' ')) ? 1 : 0),
                            tokenKind));
        return;

    case QQmlJSGrammar::T_DOT:
    case QQmlJSGrammar::T_RBRACE:
    case QQmlJSGrammar::T_RBRACKET:
    case QQmlJSGrammar::T_RPAREN:
        regexpMayFollow = false;
        break;

        // template used to expand to a string plus a delimiter for the ${ and }, now
        // we use a + as delimiter
    case QQmlJSGrammar::T_TEMPLATE_HEAD:
        regexpMayFollow = true;
        tokens.append(Token(lexer.tokenStartColumn() - 1, lexer.tokenLength() - 2, tokenKind));
        tokens.append(Token(lexer.tokenStartColumn() + lexer.tokenLength() - 3, 2,
                            QQmlJSGrammar::T_PLUS));
        return;
    case QQmlJSGrammar::T_TEMPLATE_MIDDLE:
        regexpMayFollow = true;
        tokens.append(Token(lexer.tokenStartColumn() - 1, 1, QQmlJSGrammar::T_PLUS));
        tokens.append(Token(lexer.tokenStartColumn(), lexer.tokenLength() - 3, tokenKind));
        tokens.append(Token(lexer.tokenStartColumn() + lexer.tokenLength() - 3, 2,
                            QQmlJSGrammar::T_PLUS));
        return;
    case QQmlJSGrammar::T_TEMPLATE_TAIL:
        regexpMayFollow = true;
        tokens.append(Token(lexer.tokenStartColumn() - 1, 1, QQmlJSGrammar::T_PLUS));
        tokens.append(Token(lexer.tokenStartColumn(), lexer.tokenLength() - 1, tokenKind));
        return;
    case QQmlJSGrammar::T_PARTIAL_TEMPLATE_MIDDLE:
        regexpMayFollow = true;
        tokens.append(Token(lexer.tokenStartColumn() - 1, 1, QQmlJSGrammar::T_PLUS));
        tokens.append(Token(lexer.tokenStartColumn(), lexer.tokenLength() - 1, tokenKind));
        return;
    case QQmlJSGrammar::T_MULTILINE_STRING_LITERAL:
    case QQmlJSGrammar::T_NO_SUBSTITUTION_TEMPLATE:
    case QQmlJSGrammar::T_STRING_LITERAL:
    case QQmlJSGrammar::T_PARTIAL_SINGLE_QUOTE_STRING_LITERAL:
    case QQmlJSGrammar::T_PARTIAL_DOUBLE_QUOTE_STRING_LITERAL:
    case QQmlJSGrammar::T_PARTIAL_TEMPLATE_HEAD:
        regexpMayFollow = (tokenKind == QQmlJSGrammar::T_TEMPLATE_MIDDLE
                           || tokenKind == QQmlJSGrammar::T_TEMPLATE_HEAD);
        break;

    case QQmlJSGrammar::T_VERSION_NUMBER:
        if (lexer.state().currentChar == u'.') {
            int offset = lexer.tokenStartColumn() - 1;
            int length = lexer.tokenLength();
            tokenKind = lexer.lex();
            Q_ASSERT(tokenKind == QQmlJSGrammar::T_DOT);
            tokenKind = lexer.lex();
            Q_ASSERT(tokenKind == QQmlJSGrammar::T_VERSION_NUMBER);
            length += 1 + lexer.tokenLength();
            tokens.append(Token(offset, length, QQmlJSGrammar::T_NUMERIC_LITERAL));
            return;
        }
        break;

    default:
        break;
    }
    // avoid newline (on multiline comments/strings)
    qsizetype len = lexer.code().size();
    if (lexer.code().endsWith(u'\n'))
        --len;
    len -= lexer.tokenStartColumn() - 1;
    if (len < 0)
        len = 0;
    if (lexer.tokenLength() < len)
        len = lexer.tokenLength();
    tokens.append(Token(lexer.tokenStartColumn() - 1, len, tokenKind));
}

bool Token::lexKindIsDelimiter(int kind)
{
    switch (kind) {
    case QQmlJSGrammar::T_AND:
    case QQmlJSGrammar::T_AND_AND:
    case QQmlJSGrammar::T_AND_EQ:
    case QQmlJSGrammar::T_ARROW:
    case QQmlJSGrammar::T_EQ:
    case QQmlJSGrammar::T_EQ_EQ:
    case QQmlJSGrammar::T_EQ_EQ_EQ:
    case QQmlJSGrammar::T_GE:
    case QQmlJSGrammar::T_GT:
    case QQmlJSGrammar::T_GT_GT:
    case QQmlJSGrammar::T_GT_GT_EQ:
    case QQmlJSGrammar::T_GT_GT_GT:
    case QQmlJSGrammar::T_GT_GT_GT_EQ:
    case QQmlJSGrammar::T_LE:
    case QQmlJSGrammar::T_LT:
    case QQmlJSGrammar::T_LT_LT:
    case QQmlJSGrammar::T_LT_LT_EQ:
    case QQmlJSGrammar::T_MINUS:
    case QQmlJSGrammar::T_MINUS_EQ:
    case QQmlJSGrammar::T_NOT:
    case QQmlJSGrammar::T_NOT_EQ:
    case QQmlJSGrammar::T_NOT_EQ_EQ:
    case QQmlJSGrammar::T_OR:
    case QQmlJSGrammar::T_OR_EQ:
    case QQmlJSGrammar::T_OR_OR:
    case QQmlJSGrammar::T_PLUS:
    case QQmlJSGrammar::T_PLUS_EQ:
    case QQmlJSGrammar::T_QUESTION:
    case QQmlJSGrammar::T_QUESTION_DOT:
    case QQmlJSGrammar::T_QUESTION_QUESTION:
    case QQmlJSGrammar::T_REMAINDER:
    case QQmlJSGrammar::T_REMAINDER_EQ:
    case QQmlJSGrammar::T_STAR:
    case QQmlJSGrammar::T_STAR_EQ:
    case QQmlJSGrammar::T_STAR_STAR:
    case QQmlJSGrammar::T_STAR_STAR_EQ:
    case QQmlJSGrammar::T_TILDE:
    case QQmlJSGrammar::T_XOR:
    case QQmlJSGrammar::T_XOR_EQ:
    case QQmlJSGrammar::T_AT:
    case QQmlJSGrammar::T_COMMA:
    case QQmlJSGrammar::T_COLON:
    case QQmlJSGrammar::T_LPAREN:
    case QQmlJSGrammar::T_DIVIDE_:
    case QQmlJSGrammar::T_DIVIDE_EQ:
        return true;
    default:
        break;
    }
    return false;
}

bool Token::lexKindIsQmlReserved(int kind)
{
    switch (kind) {
    case QQmlJSGrammar::T_AS:
    case QQmlJSGrammar::T_IMPORT:
    case QQmlJSGrammar::T_SIGNAL:
    case QQmlJSGrammar::T_PROPERTY:
    case QQmlJSGrammar::T_READONLY:
    case QQmlJSGrammar::T_COMPONENT:
    case QQmlJSGrammar::T_REQUIRED:
    case QQmlJSGrammar::T_ON:
    case QQmlJSGrammar::T_ENUM:
        return true;
    default:
        break;
    }
    return false;
}

bool Token::lexKindIsComment(int kind)
{
    switch (kind) {
    case QQmlJSGrammar::T_COMMENT:
    case QQmlJSGrammar::T_PARTIAL_COMMENT:
        return true;
    default:
        break;
    }
    return false;
}

bool Token::lexKindIsJSKeyword(int kind)
{
    switch (kind) {
    case QQmlJSGrammar::T_BREAK:
    case QQmlJSGrammar::T_CASE:
    case QQmlJSGrammar::T_CATCH:
    case QQmlJSGrammar::T_CLASS:
    case QQmlJSGrammar::T_CONST:
    case QQmlJSGrammar::T_CONTINUE:
    case QQmlJSGrammar::T_DEBUGGER:
    case QQmlJSGrammar::T_DEFAULT:
    case QQmlJSGrammar::T_DELETE:
    case QQmlJSGrammar::T_DO:
    case QQmlJSGrammar::T_ELSE:
    case QQmlJSGrammar::T_ENUM:
    case QQmlJSGrammar::T_EXPORT:
    case QQmlJSGrammar::T_EXTENDS:
    case QQmlJSGrammar::T_FALSE:
    case QQmlJSGrammar::T_FINALLY:
    case QQmlJSGrammar::T_FOR:
    case QQmlJSGrammar::T_FROM:
    case QQmlJSGrammar::T_GET:
    case QQmlJSGrammar::T_IF:
    case QQmlJSGrammar::T_IN:
    case QQmlJSGrammar::T_INSTANCEOF:
    case QQmlJSGrammar::T_LET:
    case QQmlJSGrammar::T_NEW:
    case QQmlJSGrammar::T_RETURN:
    case QQmlJSGrammar::T_SUPER:
    case QQmlJSGrammar::T_SWITCH:
    case QQmlJSGrammar::T_THEN:
    case QQmlJSGrammar::T_THIS:
    case QQmlJSGrammar::T_THROW:
    case QQmlJSGrammar::T_VOID:
    case QQmlJSGrammar::T_WHILE:
    case QQmlJSGrammar::T_WITH:
    case QQmlJSGrammar::T_YIELD:
    case QQmlJSGrammar::T_VAR:
    case QQmlJSGrammar::T_FUNCTION:
        return true;
    default:
        break;
    }
    return false;
}

bool Token::lexKindIsIdentifier(int kind)
{
    switch (kind) {
    case QQmlJSGrammar::T_IDENTIFIER:
    case QQmlJSGrammar::T_COMPONENT:
    case QQmlJSGrammar::T_REQUIRED:
    case QQmlJSGrammar::T_AS:
    case QQmlJSGrammar::T_PRAGMA:
    case QQmlJSGrammar::T_IMPORT:
    case QQmlJSGrammar::T_RESERVED_WORD:
    case QQmlJSGrammar::T_SET:
    case QQmlJSGrammar::T_SIGNAL:
    case QQmlJSGrammar::T_PROPERTY:
    case QQmlJSGrammar::T_PUBLIC:
    case QQmlJSGrammar::T_READONLY:
    case QQmlJSGrammar::T_NULL:
    case QQmlJSGrammar::T_OF:
    case QQmlJSGrammar::T_ON:
    case QQmlJSGrammar::T_STATIC:
    case QQmlJSGrammar::T_TRUE:
    case QQmlJSGrammar::T_TRY:
    case QQmlJSGrammar::T_TYPEOF:
    case QQmlJSGrammar::T_WITHOUTAS:
    case QQmlJSGrammar::T_FROM:
        return true;
    default:
        break;
    }
    return false;
}

bool Token::lexKindIsStringType(int kind)
{
    switch (kind) {
    case QQmlJSGrammar::T_PARTIAL_TEMPLATE_MIDDLE:
    case QQmlJSGrammar::T_MULTILINE_STRING_LITERAL:
    case QQmlJSGrammar::T_NO_SUBSTITUTION_TEMPLATE:
    case QQmlJSGrammar::T_STRING_LITERAL:
    case QQmlJSGrammar::T_PARTIAL_SINGLE_QUOTE_STRING_LITERAL:
    case QQmlJSGrammar::T_PARTIAL_DOUBLE_QUOTE_STRING_LITERAL:
    case QQmlJSGrammar::T_PARTIAL_TEMPLATE_HEAD:
        return true;
    default:
        break;
    }
    return false;
}

bool Token::lexKindIsInvalid(int kind)
{
    switch (kind) {
    case QQmlJSGrammar::T_NONE:
    case QQmlJSGrammar::T_EOL:
    case QQmlJSGrammar::EOF_SYMBOL:
    case QQmlJSGrammar::T_ERROR:
    case QQmlJSGrammar::T_FEED_JS_EXPRESSION:
    case QQmlJSGrammar::T_FEED_JS_MODULE:
    case QQmlJSGrammar::T_FEED_JS_SCRIPT:
    case QQmlJSGrammar::T_FEED_JS_STATEMENT:
    case QQmlJSGrammar::T_FEED_UI_OBJECT_MEMBER:
    case QQmlJSGrammar::T_FEED_UI_PROGRAM:
    case QQmlJSGrammar::REDUCE_HERE:
    case QQmlJSGrammar::T_FORCE_BLOCK:
    case QQmlJSGrammar::T_FORCE_DECLARATION:
    case QQmlJSGrammar::T_FOR_LOOKAHEAD_OK:
        return true;
    default:
        break;
    }
    return false;
}

void Token::dump(const Sink &s, QStringView line) const
{
    s(u"{");
    sinkInt(s, offset);
    s(u", ");
    sinkInt(s, length);
    s(u", Token::");
    s(QString::number(lexKind));
    s(u"}");
    QStringView value = line.mid(offset, length);
    if (!value.isEmpty()) {
        s(u":");
        sinkEscaped(s, value);
    }
}

QList<Token> Scanner::operator()(QStringView text, const Scanner::State &startState)
{
    _state = startState;
    QList<Token> tokens;

    {
        QQmlJS::Lexer lexer(nullptr, QQmlJS::Lexer::LexMode::LineByLine);
        lexer.setState(startState.state);
        QString line = text.toString();
        if (!(line.endsWith(u"\n") || line.endsWith(u"\r")))
            line += u'\n';
        lexer.setCode(line, -1, _qmlMode, QQmlJS::Lexer::CodeContinuation::Continue);
        while (true) {
            int tokenKind = lexer.lex();
            if (tokenKind == QQmlJSGrammar::T_EOL || tokenKind == QQmlJSGrammar::EOF_SYMBOL)
                break;
            addLexToken(tokens, tokenKind, lexer, _state.regexpMightFollow);
        }
        _state.state = lexer.state();
    }
    return tokens;
}

Scanner::State Scanner::state() const
{
    return _state;
}

bool Scanner::State::isMultiline() const
{
    switch (state.tokenKind) {
    case QQmlJSGrammar::T_PARTIAL_COMMENT:
    case QQmlJSGrammar::T_PARTIAL_DOUBLE_QUOTE_STRING_LITERAL:
    case QQmlJSGrammar::T_PARTIAL_SINGLE_QUOTE_STRING_LITERAL:
    case QQmlJSGrammar::T_PARTIAL_TEMPLATE_HEAD:
    case QQmlJSGrammar::T_PARTIAL_TEMPLATE_MIDDLE:
        return true;
    default:
        break;
    }
    return false;
}

bool Scanner::State::isMultilineComment() const
{
    switch (state.tokenKind) {
    case QQmlJSGrammar::T_PARTIAL_COMMENT:
        return true;
    default:
        break;
    }
    return false;
}

QT_END_NAMESPACE
