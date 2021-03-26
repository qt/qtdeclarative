// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMSCANNER_P_H
#define QQMLDOMSCANNER_P_H

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

#include "qqmldom_global.h"
#include "qqmldomstringdumper_p.h"

#include <QStringList>
#include <QStringView>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsgrammar_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
enum {
    T_EOL = 2000,
    T_PARTIAL_COMMENT = 2001,
    T_PARTIAL_DOUBLE_QUOTE_STRING_LITERAL = 2002,
    T_PARTIAL_SINGLE_QUOTE_STRING_LITERAL = 2003,
    T_PARTIAL_TEMPLATE_HEAD = 2004,
    T_PARTIAL_TEMPLATE_MIDDLE = 2005,
    T_NONE = 2006
};
#else
constexpr auto T_NONE = QQmlJSGrammar::T_NONE;
constexpr auto T_EOL = QQmlJSGrammar::T_EOL;
constexpr auto T_PARTIAL_COMMENT = QQmlJSGrammar::T_PARTIAL_COMMENT;
constexpr auto T_PARTIAL_DOUBLE_QUOTE_STRING_LITERAL =
        QQmlJSGrammar::T_PARTIAL_DOUBLE_QUOTE_STRING_LITERAL;
constexpr auto T_PARTIAL_SINGLE_QUOTE_STRING_LITERAL =
        QQmlJSGrammar::T_PARTIAL_SINGLE_QUOTE_STRING_LITERAL;
constexpr auto T_PARTIAL_TEMPLATE_HEAD = QQmlJSGrammar::T_PARTIAL_TEMPLATE_HEAD;
constexpr auto T_PARTIAL_TEMPLATE_MIDDLE = QQmlJSGrammar::T_PARTIAL_TEMPLATE_MIDDLE;
#endif

class QMLDOM_EXPORT Token
{
    Q_GADGET
public:
    static bool lexKindIsDelimiter(int kind);
    static bool lexKindIsJSKeyword(int kind);
    static bool lexKindIsIdentifier(int kind);
    static bool lexKindIsStringType(int kind);
    static bool lexKindIsInvalid(int kind);
    static bool lexKindIsQmlReserved(int kind);
    static bool lexKindIsComment(int kind);

    inline Token() = default;
    inline Token(int o, int l, int lexKind) : offset(o), length(l), lexKind(lexKind) { }
    inline int begin() const { return offset; }
    inline int end() const { return offset + length; }
    void dump(Sink s, QStringView line = QStringView()) const;
    QString toString(QStringView line = QStringView()) const
    {
        return dumperToString([line, this](Sink s) { this->dump(s, line); });
    }

    static int compare(const Token &t1, const Token &t2)
    {
        if (int c = t1.offset - t2.offset)
            return c;
        if (int c = t1.length - t2.length)
            return c;
        return int(t1.lexKind) - int(t2.lexKind);
    }

    int offset = 0;
    int length = 0;
    int lexKind = T_NONE;
};

inline int operator==(const Token &t1, const Token &t2)
{
    return Token::compare(t1, t2) == 0;
}
inline int operator!=(const Token &t1, const Token &t2)
{
    return Token::compare(t1, t2) != 0;
}

class QMLDOM_EXPORT Scanner
{
public:
    struct QMLDOM_EXPORT State
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        Lexer::State state {};
#else
        struct LState
        {
            int tokenKind = {};
            friend QDebug operator<<(QDebug dbg, const LState &s)
            {
                return dbg << "tokenKind:" << s.tokenKind;
            }
        } state;
#endif
        bool regexpMightFollow = true;
        bool isMultiline() const;
        bool isMultilineComment() const;
    };

    QList<Token> operator()(QStringView text, const State &startState);
    State state() const;

private:
    bool _qmlMode = true;
    State _state;
};

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
#endif
