#ifndef QV4SYNTAXCHECKER_P_H
#define QV4SYNTAXCHECKER_P_H

#include <private/qqmljslexer_p.h>
#include <private/qqmljsengine_p.h>

#include <QtCore/QVector>
#include <QtCore/QString>
#include <QtCore/QList>

namespace QQmlJS {

class SyntaxChecker: Lexer
{
public:
    SyntaxChecker();

    QString text() const;
    void clearText();
    void appendText(const QString &text);

    bool canEvaluate();

private:
    Engine m_engine;
    QVector<int> m_stateStack;
    QList<int> m_tokens;
    QString m_code;
};

} // end of QQmlJS namespace

#endif // QV4SYNTAXCHECKER_P_H
