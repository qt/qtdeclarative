
#include "qv4syntaxchecker_p.h"

using namespace QQmlJS;

SyntaxChecker::SyntaxChecker()
    : Lexer(&m_engine)
    , m_stateStack(128)
{
}

void QQmlJS::SyntaxChecker::clearText()
{
    m_code.clear();
    m_tokens.clear();
}

void SyntaxChecker::appendText(const QString &text)
{
    m_code += text;
}

QString SyntaxChecker::text() const
{
    return m_code;
}

bool SyntaxChecker::canEvaluate()
{
    int yyaction = 0;
    int yytoken = -1;
    int yytos = -1;

    setCode(m_code, 1);

    m_tokens.clear();
    m_tokens.append(T_FEED_JS_PROGRAM);

    do {
        if (++yytos == m_stateStack.size())
            m_stateStack.resize(m_stateStack.size() * 2);

        m_stateStack[yytos] = yyaction;

again:
        if (yytoken == -1 && action_index[yyaction] != -TERMINAL_COUNT) {
            if (m_tokens.isEmpty())
                yytoken = lex();
            else
                yytoken = m_tokens.takeFirst();
        }

        yyaction = t_action(yyaction, yytoken);
        if (yyaction > 0) {
            if (yyaction == ACCEPT_STATE) {
                --yytos;
                return true;
            }
            yytoken = -1;
        } else if (yyaction < 0) {
            const int ruleno = -yyaction - 1;
            yytos -= rhs[ruleno];
            yyaction = nt_action(m_stateStack[yytos], lhs[ruleno] - TERMINAL_COUNT);
        }
    } while (yyaction);

    const int errorState = m_stateStack[yytos];
    if (t_action(errorState, T_AUTOMATIC_SEMICOLON) && canInsertAutomaticSemicolon(yytoken)) {
        yyaction = errorState;
        m_tokens.prepend(yytoken);
        yytoken = T_SEMICOLON;
        goto again;
    }

    if (yytoken != EOF_SYMBOL)
        return true;

    return false;
}
