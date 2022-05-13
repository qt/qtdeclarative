// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLJSLEXER_P_H
#define QQMLJSLEXER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmljsglobal_p.h>
#include <private/qqmljsgrammar_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qstack.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {

class Engine;
struct DiagnosticMessage;
class Directives;

class QML_PARSER_EXPORT Lexer: public QQmlJSGrammar
{
public:
    enum {
        T_ABSTRACT = T_RESERVED_WORD,
        T_BOOLEAN = T_RESERVED_WORD,
        T_BYTE = T_RESERVED_WORD,
        T_CHAR = T_RESERVED_WORD,
        T_DOUBLE = T_RESERVED_WORD,
        T_FINAL = T_RESERVED_WORD,
        T_FLOAT = T_RESERVED_WORD,
        T_GOTO = T_RESERVED_WORD,
        T_IMPLEMENTS = T_RESERVED_WORD,
        T_INT = T_RESERVED_WORD,
        T_INTERFACE = T_RESERVED_WORD,
        T_LONG = T_RESERVED_WORD,
        T_NATIVE = T_RESERVED_WORD,
        T_PACKAGE = T_RESERVED_WORD,
        T_PRIVATE = T_RESERVED_WORD,
        T_PROTECTED = T_RESERVED_WORD,
        T_SHORT = T_RESERVED_WORD,
        T_SYNCHRONIZED = T_RESERVED_WORD,
        T_THROWS = T_RESERVED_WORD,
        T_TRANSIENT = T_RESERVED_WORD,
        T_VOLATILE = T_RESERVED_WORD
    };

    enum Error {
        NoError,
        IllegalCharacter,
        IllegalNumber,
        UnclosedStringLiteral,
        IllegalEscapeSequence,
        IllegalUnicodeEscapeSequence,
        UnclosedComment,
        IllegalExponentIndicator,
        IllegalIdentifier,
        IllegalHexadecimalEscapeSequence
    };

    enum RegExpBodyPrefix {
        NoPrefix,
        EqualPrefix
    };

    enum RegExpFlag {
        RegExp_Global     = 0x01,
        RegExp_IgnoreCase = 0x02,
        RegExp_Multiline  = 0x04,
        RegExp_Unicode    = 0x08,
        RegExp_Sticky     = 0x10
    };

    enum ParseModeFlags {
        QmlMode = 0x1,
        YieldIsKeyword = 0x2,
        StaticIsKeyword = 0x4
    };

    enum class ImportState {
        SawImport,
        NoQmlImport
    };

public:
    Lexer(Engine *engine);

    int parseModeFlags() const {
        int flags = 0;
        if (qmlMode())
            flags |= QmlMode|StaticIsKeyword;
        if (yieldIsKeyWord())
            flags |= YieldIsKeyword;
        if (_staticIsKeyword)
            flags |= StaticIsKeyword;
        return flags;
    }

    bool qmlMode() const;
    bool yieldIsKeyWord() const { return _state.generatorLevel != 0; }
    void setStaticIsKeyword(bool b) { _staticIsKeyword = b; }

    QString code() const;
    void setCode(const QString &code, int lineno, bool qmlMode = true);

    int lex();

    bool scanRegExp(RegExpBodyPrefix prefix = NoPrefix);
    bool scanDirectives(Directives *directives, DiagnosticMessage *error);

    int regExpFlags() const { return _state.patternFlags; }
    QString regExpPattern() const { return _state.tokenText; }

    int tokenKind() const { return _state.tokenKind; }
    int tokenOffset() const { return _state.tokenStartPtr - _code.unicode(); }
    int tokenLength() const { return _state.tokenLength; }

    int tokenStartLine() const { return _state.tokenLine; }
    int tokenStartColumn() const { return _state.tokenColumn; }

    inline QStringView tokenSpell() const { return _state.tokenSpell; }
    inline QStringView rawString() const { return _state.rawString; }
    double tokenValue() const { return _state.tokenValue; }
    QString tokenText() const;

    Error errorCode() const;
    QString errorMessage() const;

    bool prevTerminator() const;
    bool followsClosingBrace() const;
    bool canInsertAutomaticSemicolon(int token) const;

    enum ParenthesesState {
        IgnoreParentheses,
        CountParentheses,
        BalancedParentheses
    };

    void enterGeneratorBody() { ++_state.generatorLevel; }
    void leaveGeneratorBody() { --_state.generatorLevel; }

    struct State
    {
        QString tokenText;
        QString errorMessage;
        QStringView tokenSpell;
        QStringView rawString;

        const QChar *codePtr = nullptr;
        const QChar *tokenStartPtr = nullptr;

        QChar currentChar = u'\n';
        Error errorCode = NoError;

        int currentLineNumber = 0;
        int currentColumnNumber = 0;
        double tokenValue = 0;

        // parentheses state
        ParenthesesState parenthesesState = IgnoreParentheses;
        int parenthesesCount = 0;

        // template string stack
        QStack<int> outerTemplateBraceCount;
        int bracesCount = -1;

        int stackToken = -1;

        int patternFlags = 0;
        int tokenKind = 0;
        int tokenLength = 0;
        int tokenLine = 0;
        int tokenColumn = 0;
        ImportState importState = ImportState::NoQmlImport;

        bool validTokenText = false;
        bool prohibitAutomaticSemicolon = false;
        bool restrictedKeyword = false;
        bool terminator = false;
        bool followsClosingBrace = false;
        bool delimited = true;
        bool skipLinefeed = false;
        bool handlingDirectives = false;
        int generatorLevel = 0;
    };

    const State &state() const;
    void setState(const State &state);

protected:
    static int classify(const QChar *s, int n, int parseModeFlags);

private:
    inline void scanChar();
    inline QChar peekChar();
    int scanToken();
    int scanNumber(QChar ch);
    int scanVersionNumber(QChar ch);
    enum ScanStringMode {
        SingleQuote = '\'',
        DoubleQuote = '"',
        TemplateHead = '`',
        TemplateContinuation = 0
    };
    int scanString(ScanStringMode mode);

    bool isLineTerminator() const;
    unsigned isLineTerminatorSequence() const;
    static bool isIdentLetter(QChar c);
    static bool isDecimalDigit(ushort c);
    static bool isHexDigit(QChar c);
    static bool isOctalDigit(ushort c);

    void syncProhibitAutomaticSemicolon();
    uint decodeUnicodeEscapeCharacter(bool *ok);
    QChar decodeHexEscapeCharacter(bool *ok);

private:
    Engine *_engine;

    QString _code;
    const QChar *_endPtr;
    bool _qmlMode;
    bool _staticIsKeyword = false;

    State _state;
};

} // end of namespace QQmlJS

QT_END_NAMESPACE

#endif // LEXER_H
