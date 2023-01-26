// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomcodeformatter_p.h"

#include <QLoggingCategory>
#include <QMetaEnum>

static Q_LOGGING_CATEGORY(formatterLog, "qt.qmldom.formatter", QtWarningMsg);

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

using StateType = FormatTextStatus::StateType;
using State = FormatTextStatus::State;

State FormatTextStatus::state(int belowTop) const
{
    if (belowTop < states.size())
        return states.at(states.size() - 1 - belowTop);
    else
        return State();
}

QString FormatTextStatus::stateToString(StateType type)
{
    const QMetaEnum &metaEnum =
            staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("StateType"));
    return QString::fromUtf8(metaEnum.valueToKey(int(type)));
}

void FormatPartialStatus::enterState(StateType newState)
{
    int savedIndentDepth = currentIndent;
    defaultOnEnter(newState, &currentIndent, &savedIndentDepth);
    currentStatus.pushState(newState, savedIndentDepth);
    qCDebug(formatterLog) << "enter state" << FormatTextStatus::stateToString(newState);

    if (newState == StateType::BracketOpen)
        enterState(StateType::BracketElementStart);
}

void FormatPartialStatus::leaveState(bool statementDone)
{
    Q_ASSERT(currentStatus.size() > 1);
    if (currentStatus.state().type == StateType::TopmostIntro)
        return;

    // restore indent depth
    State poppedState = currentStatus.popState();
    currentIndent = poppedState.savedIndentDepth;

    StateType topState = currentStatus.state().type;

    qCDebug(formatterLog) << "left state" << FormatTextStatus::stateToString(poppedState.type)
                          << ", now in state" << FormatTextStatus::stateToString(topState);

    // if statement is done, may need to leave recursively
    if (statementDone) {
        if (topState == StateType::IfStatement) {
            if (poppedState.type != StateType::MaybeElse)
                enterState(StateType::MaybeElse);
            else
                leaveState(true);
        } else if (topState == StateType::ElseClause) {
            // leave the else *and* the surrounding if, to prevent another else
            leaveState(false);
            leaveState(true);
        } else if (topState == StateType::TryStatement) {
            if (poppedState.type != StateType::MaybeCatchOrFinally
                && poppedState.type != StateType::FinallyStatement) {
                enterState(StateType::MaybeCatchOrFinally);
            } else {
                leaveState(true);
            }
        } else if (!FormatTextStatus::isExpressionEndState(topState)) {
            leaveState(true);
        }
    }
}

void FormatPartialStatus::turnIntoState(StateType newState)
{
    leaveState(false);
    enterState(newState);
}

const Token &FormatPartialStatus::tokenAt(int idx) const
{
    static const Token empty;
    if (idx < 0 || idx >= lineTokens.size())
        return empty;
    else
        return lineTokens.at(idx);
}

int FormatPartialStatus::column(int index) const
{
    if (index > line.size())
        index = line.size();
    IndentInfo indent(QStringView(line).mid(0, index), options.tabSize, indentOffset);
    return indent.column;
}

QStringView FormatPartialStatus::tokenText(const Token &token) const
{
    return line.mid(token.begin(), token.length);
}

bool FormatPartialStatus::tryInsideExpression(bool alsoExpression)
{
    StateType newState = StateType::Invalid;
    const int kind = tokenAt(tokenIndex).lexKind;
    switch (kind) {
    case QQmlJSGrammar::T_LPAREN:
        newState = StateType::ParenOpen;
        break;
    case QQmlJSGrammar::T_LBRACKET:
        newState = StateType::BracketOpen;
        break;
    case QQmlJSGrammar::T_LBRACE:
        newState = StateType::ObjectliteralOpen;
        break;
    case QQmlJSGrammar::T_FUNCTION:
        newState = StateType::FunctionStart;
        break;
    case QQmlJSGrammar::T_QUESTION:
        newState = StateType::TernaryOp;
        break;
    }

    if (newState != StateType::Invalid) {
        if (alsoExpression)
            enterState(StateType::Expression);
        enterState(newState);
        return true;
    }

    return false;
}

bool FormatPartialStatus::tryStatement()
{
    Token t = tokenAt(tokenIndex);
    const int kind = t.lexKind;
    switch (kind) {
    case QQmlJSGrammar::T_AUTOMATIC_SEMICOLON:
    case QQmlJSGrammar::T_COMPATIBILITY_SEMICOLON:
    case QQmlJSGrammar::T_SEMICOLON:
        enterState(StateType::EmptyStatement);
        leaveState(true);
        return true;
    case QQmlJSGrammar::T_BREAK:
    case QQmlJSGrammar::T_CONTINUE:
        enterState(StateType::BreakcontinueStatement);
        return true;
    case QQmlJSGrammar::T_THROW:
        enterState(StateType::ThrowStatement);
        enterState(StateType::Expression);
        return true;
    case QQmlJSGrammar::T_RETURN:
        enterState(StateType::ReturnStatement);
        enterState(StateType::Expression);
        return true;
    case QQmlJSGrammar::T_WHILE:
    case QQmlJSGrammar::T_FOR:
    case QQmlJSGrammar::T_CATCH:
        enterState(StateType::StatementWithCondition);
        return true;
    case QQmlJSGrammar::T_SWITCH:
        enterState(StateType::SwitchStatement);
        return true;
    case QQmlJSGrammar::T_IF:
        enterState(StateType::IfStatement);
        return true;
    case QQmlJSGrammar::T_DO:
        enterState(StateType::DoStatement);
        enterState(StateType::Substatement);
        return true;
    case QQmlJSGrammar::T_CASE:
    case QQmlJSGrammar::T_DEFAULT:
        enterState(StateType::CaseStart);
        return true;
    case QQmlJSGrammar::T_TRY:
        enterState(StateType::TryStatement);
        return true;
    case QQmlJSGrammar::T_LBRACE:
        enterState(StateType::JsblockOpen);
        return true;
    case QQmlJSGrammar::T_VAR:
    case QQmlJSGrammar::T_PLUS_PLUS:
    case QQmlJSGrammar::T_MINUS_MINUS:
    case QQmlJSGrammar::T_IMPORT:
    case QQmlJSGrammar::T_SIGNAL:
    case QQmlJSGrammar::T_ON:
    case QQmlJSGrammar::T_AS:
    case QQmlJSGrammar::T_PROPERTY:
    case QQmlJSGrammar::T_REQUIRED:
    case QQmlJSGrammar::T_READONLY:
    case QQmlJSGrammar::T_FUNCTION:
    case QQmlJSGrammar::T_FUNCTION_STAR:
    case QQmlJSGrammar::T_NUMERIC_LITERAL:
    case QQmlJSGrammar::T_LPAREN:
        enterState(StateType::Expression);
        // look at the token again
        tokenIndex -= 1;
        return true;
    default:
        if (Token::lexKindIsIdentifier(kind)) {
            enterState(StateType::ExpressionOrLabel);
            return true;
        } else if (Token::lexKindIsDelimiter(kind) || Token::lexKindIsStringType(kind)) {
            enterState(StateType::Expression);
            // look at the token again
            tokenIndex -= 1;
            return true;
        }
    }
    return false;
}

void FormatPartialStatus::dump() const
{
    qCDebug(formatterLog) << "Current token index" << tokenIndex;
    qCDebug(formatterLog) << "Current state:";
    foreach (const State &s, currentStatus.states) {
        qCDebug(formatterLog) << FormatTextStatus::stateToString(s.type) << s.savedIndentDepth;
    }
    qCDebug(formatterLog) << "Current lexerState:" << currentStatus.lexerState.state;
    qCDebug(formatterLog) << "Current indent:" << currentIndent;
}

void FormatPartialStatus::handleTokens()
{
    auto enter = [this](StateType newState) { this->enterState(newState); };

    auto leave = [this](bool statementDone = false) { this->leaveState(statementDone); };

    auto turnInto = [this](StateType newState) { this->turnIntoState(newState); };

    qCDebug(formatterLog) << "Starting to look at " << line;

    for (; tokenIndex < lineTokens.size();) {
        Token currentToken = tokenAt(tokenIndex);
        const int kind = currentToken.lexKind;

        qCDebug(formatterLog) << "Token: " << tokenText(currentToken);

        if (Token::lexKindIsComment(kind)
            && currentStatus.state().type != StateType::MultilineCommentCont
            && currentStatus.state().type != StateType::MultilineCommentStart) {
            tokenIndex += 1;
            continue;
        }

        switch (currentStatus.state().type) {
        case StateType::TopmostIntro:
            switch (kind) {
            case QQmlJSGrammar::T_IDENTIFIER:
                enter(StateType::ObjectdefinitionOrJs);
                continue;
            case QQmlJSGrammar::T_IMPORT:
                enter(StateType::TopQml);
                continue;
            case QQmlJSGrammar::T_LBRACE:
                enter(StateType::TopJs);
                enter(StateType::Expression);
                continue; // if a file starts with {, it's likely json
            default:
                enter(StateType::TopJs);
                continue;
            }
            break;

        case StateType::TopQml:
            switch (kind) {
            case QQmlJSGrammar::T_IMPORT:
                enter(StateType::ImportStart);
                break;
            case QQmlJSGrammar::T_IDENTIFIER:
                enter(StateType::BindingOrObjectdefinition);
                break;
            default:
                if (Token::lexKindIsIdentifier(kind))
                    enter(StateType::BindingOrObjectdefinition);
                break;
            }
            break;

        case StateType::TopJs:
            tryStatement();
            break;

        case StateType::ObjectdefinitionOrJs:
            switch (kind) {
            case QQmlJSGrammar::T_DOT:
                break;
            case QQmlJSGrammar::T_LBRACE:
                turnInto(StateType::BindingOrObjectdefinition);
                continue;
            default:
                if (!Token::lexKindIsIdentifier(kind) || !line.at(currentToken.begin()).isUpper()) {
                    turnInto(StateType::TopJs);
                    continue;
                }
            }
            break;

        case StateType::ImportStart:
            enter(StateType::ImportMaybeDotOrVersionOrAs);
            break;

        case StateType::ImportMaybeDotOrVersionOrAs:
            switch (kind) {
            case QQmlJSGrammar::T_DOT:
                turnInto(StateType::ImportDot);
                break;
            case QQmlJSGrammar::T_AS:
                turnInto(StateType::ImportAs);
                break;
            case QQmlJSGrammar::T_NUMERIC_LITERAL:
            case QQmlJSGrammar::T_VERSION_NUMBER:
                turnInto(StateType::ImportMaybeAs);
                break;
            default:
                leave();
                leave();
                continue;
            }
            break;

        case StateType::ImportMaybeAs:
            switch (kind) {
            case QQmlJSGrammar::T_AS:
                turnInto(StateType::ImportAs);
                break;
            default:
                leave();
                leave();
                continue;
            }
            break;

        case StateType::ImportDot:
            if (Token::lexKindIsIdentifier(kind)) {
                turnInto(StateType::ImportMaybeDotOrVersionOrAs);
            } else {
                leave();
                leave();
                continue;
            }
            break;

        case StateType::ImportAs:
            if (Token::lexKindIsIdentifier(kind)) {
                leave();
                leave();
            }
            break;

        case StateType::BindingOrObjectdefinition:
            switch (kind) {
            case QQmlJSGrammar::T_COLON:
                enter(StateType::BindingAssignment);
                break;
            case QQmlJSGrammar::T_LBRACE:
                enter(StateType::ObjectdefinitionOpen);
                break;
            }
            break;

        case StateType::BindingAssignment:
            switch (kind) {
            case QQmlJSGrammar::T_AUTOMATIC_SEMICOLON:
            case QQmlJSGrammar::T_COMPATIBILITY_SEMICOLON:
            case QQmlJSGrammar::T_SEMICOLON:
                leave(true);
                break;
            case QQmlJSGrammar::T_IF:
                enter(StateType::IfStatement);
                break;
            case QQmlJSGrammar::T_WITH:
                enter(StateType::StatementWithCondition);
                break;
            case QQmlJSGrammar::T_TRY:
                enter(StateType::TryStatement);
                break;
            case QQmlJSGrammar::T_SWITCH:
                enter(StateType::SwitchStatement);
                break;
            case QQmlJSGrammar::T_LBRACE:
                enter(StateType::JsblockOpen);
                break;
            case QQmlJSGrammar::T_ON:
            case QQmlJSGrammar::T_AS:
            case QQmlJSGrammar::T_IMPORT:
            case QQmlJSGrammar::T_SIGNAL:
            case QQmlJSGrammar::T_PROPERTY:
            case QQmlJSGrammar::T_REQUIRED:
            case QQmlJSGrammar::T_READONLY:
            case QQmlJSGrammar::T_IDENTIFIER:
                enter(StateType::ExpressionOrObjectdefinition);
                break;

                // error recovery
            case QQmlJSGrammar::T_RBRACKET:
            case QQmlJSGrammar::T_RPAREN:
                leave(true);
                break;

            default:
                enter(StateType::Expression);
                continue;
            }
            break;

        case StateType::ObjectdefinitionOpen:
            switch (kind) {
            case QQmlJSGrammar::T_RBRACE:
                leave(true);
                break;
            case QQmlJSGrammar::T_DEFAULT:
            case QQmlJSGrammar::T_READONLY:
                enter(StateType::PropertyModifiers);
                break;
            case QQmlJSGrammar::T_PROPERTY:
                enter(StateType::PropertyStart);
                break;
            case QQmlJSGrammar::T_REQUIRED:
                enter(StateType::RequiredProperty);
                break;
            case QQmlJSGrammar::T_COMPONENT:
                enter(StateType::ComponentStart);
                break;
            case QQmlJSGrammar::T_FUNCTION:
            case QQmlJSGrammar::T_FUNCTION_STAR:
                enter(StateType::FunctionStart);
                break;
            case QQmlJSGrammar::T_SIGNAL:
                enter(StateType::SignalStart);
                break;
            case QQmlJSGrammar::T_ENUM:
                enter(StateType::EnumStart);
                break;
            case QQmlJSGrammar::T_ON:
            case QQmlJSGrammar::T_AS:
            case QQmlJSGrammar::T_IMPORT:
                enter(StateType::BindingOrObjectdefinition);
                break;
            default:
                if (Token::lexKindIsIdentifier(kind))
                    enter(StateType::BindingOrObjectdefinition);
                break;
            }
            break;

        case StateType::PropertyModifiers:
            switch (kind) {
            case QQmlJSGrammar::T_PROPERTY:
                turnInto(StateType::PropertyStart);
                break;
            case QQmlJSGrammar::T_DEFAULT:
            case QQmlJSGrammar::T_READONLY:
                break;
            case QQmlJSGrammar::T_REQUIRED:
                turnInto(StateType::RequiredProperty);
                break;
            default:
                leave(true);
                break;
            }
            break;

        case StateType::PropertyStart:
            switch (kind) {
            case QQmlJSGrammar::T_COLON:
                enter(StateType::BindingAssignment);
                break; // oops, was a binding
            case QQmlJSGrammar::T_VAR:
            case QQmlJSGrammar::T_IDENTIFIER:
                enter(StateType::PropertyName);
                break;
            default:
                if (Token::lexKindIsIdentifier(kind) && tokenText(currentToken) == u"list") {
                    enter(StateType::PropertyListOpen);
                } else {
                    leave(true);
                    continue;
                }
            }
            break;

        case StateType::RequiredProperty:
            switch (kind) {
            case QQmlJSGrammar::T_PROPERTY:
                turnInto(StateType::PropertyStart);
                break;
            case QQmlJSGrammar::T_DEFAULT:
            case QQmlJSGrammar::T_READONLY:
                turnInto(StateType::PropertyModifiers);
                break;
            case QQmlJSGrammar::T_IDENTIFIER:
                leave(true);
                break;
            default:
                leave(true);
                continue;
            }
            break;

        case StateType::ComponentStart:
            switch (kind) {
            case QQmlJSGrammar::T_IDENTIFIER:
                turnInto(StateType::ComponentName);
                break;
            default:
                leave(true);
                continue;
            }
            break;

        case StateType::ComponentName:
            switch (kind) {
            case QQmlJSGrammar::T_COLON:
                enter(StateType::BindingAssignment);
                break;
            default:
                leave(true);
                continue;
            }
            break;

        case StateType::PropertyName:
            turnInto(StateType::PropertyMaybeInitializer);
            break;

        case StateType::PropertyListOpen: {
            const QStringView tok = tokenText(currentToken);
            if (tok == u">")
                turnInto(StateType::PropertyName);
            break;
        }
        case StateType::PropertyMaybeInitializer:
            switch (kind) {
            case QQmlJSGrammar::T_COLON:
                turnInto(StateType::BindingAssignment);
                break;
            default:
                leave(true);
                continue;
            }
            break;

        case StateType::EnumStart:
            switch (kind) {
            case QQmlJSGrammar::T_LBRACE:
                enter(StateType::ObjectliteralOpen);
                break;
            }
            break;

        case StateType::SignalStart:
            switch (kind) {
            case QQmlJSGrammar::T_COLON:
                enter(StateType::BindingAssignment);
                break; // oops, was a binding
            default:
                enter(StateType::SignalMaybeArglist);
                break;
            }
            break;

        case StateType::SignalMaybeArglist:
            switch (kind) {
            case QQmlJSGrammar::T_LPAREN:
                turnInto(StateType::SignalArglistOpen);
                break;
            default:
                leave(true);
                continue;
            }
            break;

        case StateType::SignalArglistOpen:
            switch (kind) {
            case QQmlJSGrammar::T_RPAREN:
                leave(true);
                break;
            }
            break;

        case StateType::FunctionStart:
            switch (kind) {
            case QQmlJSGrammar::T_LPAREN:
                enter(StateType::FunctionArglistOpen);
                break;
            }
            break;

        case StateType::FunctionArglistOpen:
            switch (kind) {
            case QQmlJSGrammar::T_COLON:
                enter(StateType::TypeAnnotation);
                break;
            case QQmlJSGrammar::T_RPAREN:
                turnInto(StateType::FunctionArglistClosed);
                break;
            }
            break;

        case StateType::FunctionArglistClosed:
            switch (kind) {
            case QQmlJSGrammar::T_COLON:
                enter(StateType::TypeAnnotation);
                break;
            case QQmlJSGrammar::T_LBRACE:
                turnInto(StateType::JsblockOpen);
                break;
            default:
                leave(true);
                continue; // error recovery
            }
            break;

        case StateType::TypeAnnotation:
            switch (kind) {
            case QQmlJSGrammar::T_IDENTIFIER:
            case QQmlJSGrammar::T_DOT:
                break;
            case QQmlJSGrammar::T_LT:
                turnInto(StateType::TypeParameter);
                break;
            default:
                leave();
                continue; // error recovery
            }
            break;

        case StateType::TypeParameter:
            switch (kind) {
            case QQmlJSGrammar::T_LT:
                enter(StateType::TypeParameter);
                break;
            case QQmlJSGrammar::T_GT:
                leave();
                break;
            }
            break;

        case StateType::ExpressionOrObjectdefinition:
            switch (kind) {
            case QQmlJSGrammar::T_DOT:
                break; // need to become an objectdefinition_open in cases like "width: Qt.Foo
                // {"
            case QQmlJSGrammar::T_LBRACE:
                turnInto(StateType::ObjectdefinitionOpen);
                break;

                // propagate 'leave' from expression state
            case QQmlJSGrammar::T_RBRACKET:
            case QQmlJSGrammar::T_RPAREN:
                leave();
                continue;

            default:
                if (Token::lexKindIsIdentifier(kind))
                    break; // need to become an objectdefinition_open in cases like "width:
                // Qt.Foo
                enter(StateType::Expression);
                continue; // really? identifier and more tokens might already be gone
            }
            break;

        case StateType::ExpressionOrLabel:
            switch (kind) {
            case QQmlJSGrammar::T_COLON:
                turnInto(StateType::LabelledStatement);
                break;

                // propagate 'leave' from expression state
            case QQmlJSGrammar::T_RBRACKET:
            case QQmlJSGrammar::T_RPAREN:
                leave();
                continue;

            default:
                enter(StateType::Expression);
                continue;
            }
            break;

        case StateType::TernaryOp:
            if (kind == QQmlJSGrammar::T_COLON) {
                enter(StateType::TernaryOpAfterColon);
                enter(StateType::ExpressionContinuation);
                break;
            }
            Q_FALLTHROUGH();
        case StateType::TernaryOpAfterColon:
        case StateType::Expression:
            if (tryInsideExpression(false))
                break;
            switch (kind) {
            case QQmlJSGrammar::T_COMMA:
                leave(true);
                break;
            case QQmlJSGrammar::T_RBRACKET:
            case QQmlJSGrammar::T_RPAREN:
                leave();
                continue;
            case QQmlJSGrammar::T_RBRACE:
                leave(true);
                continue;
            case QQmlJSGrammar::T_AUTOMATIC_SEMICOLON:
            case QQmlJSGrammar::T_COMPATIBILITY_SEMICOLON:
            case QQmlJSGrammar::T_SEMICOLON:
                leave(true);
                break;
            default:
                if (Token::lexKindIsDelimiter(kind))
                    enter(StateType::ExpressionContinuation);
                break;
            }
            break;

        case StateType::ExpressionContinuation:
            leave();
            continue;

        case StateType::ExpressionMaybeContinuation:
            switch (kind) {
            case QQmlJSGrammar::T_QUESTION:
            case QQmlJSGrammar::T_LBRACKET:
            case QQmlJSGrammar::T_LPAREN:
            case QQmlJSGrammar::T_LBRACE:
                leave();
                continue;
            default:
                leave(!Token::lexKindIsDelimiter(kind));
                continue;
            }
            break;

        case StateType::ParenOpen:
            if (tryInsideExpression(false))
                break;
            switch (kind) {
            case QQmlJSGrammar::T_RPAREN:
                leave();
                break;
            }
            break;

        case StateType::BracketOpen:
            if (tryInsideExpression(false))
                break;
            switch (kind) {
            case QQmlJSGrammar::T_COMMA:
                enter(StateType::BracketElementStart);
                break;
            case QQmlJSGrammar::T_RBRACKET:
                leave();
                break;
            }
            break;

        case StateType::ObjectliteralOpen:
            if (tryInsideExpression(false))
                break;
            switch (kind) {
            case QQmlJSGrammar::T_COLON:
                enter(StateType::ObjectliteralAssignment);
                break;
            case QQmlJSGrammar::T_RBRACKET:
            case QQmlJSGrammar::T_RPAREN:
                leave();
                continue; // error recovery
            case QQmlJSGrammar::T_RBRACE:
                leave(true);
                break;
            }
            break;

            // pretty much like expression, but ends with , or }
        case StateType::ObjectliteralAssignment:
            if (tryInsideExpression(false))
                break;
            switch (kind) {
            case QQmlJSGrammar::T_COMMA:
                leave();
                break;
            case QQmlJSGrammar::T_RBRACKET:
            case QQmlJSGrammar::T_RPAREN:
                leave();
                continue; // error recovery
            case QQmlJSGrammar::T_RBRACE:
                leave();
                continue; // so we also leave objectliteral_open
            default:
                if (Token::lexKindIsDelimiter(kind))
                    enter(StateType::ExpressionContinuation);
                break;
            }
            break;

        case StateType::BracketElementStart:
            if (Token::lexKindIsIdentifier(kind)) {
                turnInto(StateType::BracketElementMaybeObjectdefinition);
            } else {
                leave();
                continue;
            }
            break;

        case StateType::BracketElementMaybeObjectdefinition:
            switch (kind) {
            case QQmlJSGrammar::T_LBRACE:
                turnInto(StateType::ObjectdefinitionOpen);
                break;
            default:
                leave();
                continue;
            }
            break;

        case StateType::JsblockOpen:
        case StateType::SubstatementOpen:
            if (tryStatement())
                break;
            switch (kind) {
            case QQmlJSGrammar::T_RBRACE:
                leave(true);
                break;
            }
            break;

        case StateType::LabelledStatement:
            if (tryStatement())
                break;
            leave(true); // error recovery
            break;

        case StateType::Substatement:
            // prefer substatement_open over block_open
            if (kind != QQmlJSGrammar::T_LBRACE) {
                if (tryStatement())
                    break;
            }
            switch (kind) {
            case QQmlJSGrammar::T_LBRACE:
                turnInto(StateType::SubstatementOpen);
                break;
            }
            break;

        case StateType::IfStatement:
            switch (kind) {
            case QQmlJSGrammar::T_LPAREN:
                enter(StateType::ConditionOpen);
                break;
            default:
                leave(true);
                break; // error recovery
            }
            break;

        case StateType::MaybeElse:
            switch (kind) {
            case QQmlJSGrammar::T_ELSE:
                turnInto(StateType::ElseClause);
                enter(StateType::Substatement);
                break;
            default:
                leave(true);
                continue;
            }
            break;

        case StateType::MaybeCatchOrFinally:
            switch (kind) {
            case QQmlJSGrammar::T_CATCH:
                turnInto(StateType::CatchStatement);
                break;
            case QQmlJSGrammar::T_FINALLY:
                turnInto(StateType::FinallyStatement);
                break;
            default:
                leave(true);
                continue;
            }
            break;

        case StateType::ElseClause:
            // ### shouldn't happen
            dump();
            Q_ASSERT(false);
            leave(true);
            break;

        case StateType::ConditionOpen:
            if (tryInsideExpression(false))
                break;
            switch (kind) {
            case QQmlJSGrammar::T_RPAREN:
                turnInto(StateType::Substatement);
                break;
            }
            break;

        case StateType::SwitchStatement:
        case StateType::CatchStatement:
        case StateType::StatementWithCondition:
            switch (kind) {
            case QQmlJSGrammar::T_LPAREN:
                enter(StateType::StatementWithConditionParenOpen);
                break;
            default:
                leave(true);
            }
            break;

        case StateType::StatementWithConditionParenOpen:
            if (tryInsideExpression(false))
                break;
            switch (kind) {
            case QQmlJSGrammar::T_RPAREN:
                turnInto(StateType::Substatement);
                break;
            }
            break;

        case StateType::TryStatement:
        case StateType::FinallyStatement:
            switch (kind) {
            case QQmlJSGrammar::T_LBRACE:
                enter(StateType::JsblockOpen);
                break;
            default:
                leave(true);
                break;
            }
            break;

        case StateType::DoStatement:
            switch (kind) {
            case QQmlJSGrammar::T_WHILE:
                break;
            case QQmlJSGrammar::T_LPAREN:
                enter(StateType::DoStatementWhileParenOpen);
                break;
            default:
                leave(true);
                continue; // error recovery
            }
            break;

        case StateType::DoStatementWhileParenOpen:
            if (tryInsideExpression(false))
                break;
            switch (kind) {
            case QQmlJSGrammar::T_RPAREN:
                leave();
                leave(true);
                break;
            }
            break;

        case StateType::BreakcontinueStatement:
            if (Token ::lexKindIsIdentifier(kind)) {
                leave(true);
            } else {
                leave(true);
                continue; // try again
            }
            break;

        case StateType::CaseStart:
            switch (kind) {
            case QQmlJSGrammar::T_COLON:
                turnInto(StateType::CaseCont);
                break;
            }
            break;

        case StateType::CaseCont:
            if (kind != QQmlJSGrammar::T_CASE && kind != QQmlJSGrammar::T_DEFAULT && tryStatement())
                break;
            switch (kind) {
            case QQmlJSGrammar::T_RBRACE:
                leave();
                continue;
            case QQmlJSGrammar::T_DEFAULT:
            case QQmlJSGrammar::T_CASE:
                leave();
                continue;
            }
            break;

        case StateType::MultilineCommentStart:
        case StateType::MultilineCommentCont:
            if (!Token::lexKindIsComment(kind)) {
                leave();
                continue;
            } else if (tokenIndex == lineTokens.size() - 1
                       && !currentStatus.lexerState.isMultiline()) {
                leave();
            } else if (tokenIndex == 0) {
                // to allow enter/leave to update the indentDepth
                turnInto(StateType::MultilineCommentCont);
            }
            break;

        default:
            qWarning() << "Unhandled state" << currentStatus.state().typeStr();
            break;
        } // end of state switch

        ++tokenIndex;
    }

    StateType topState = currentStatus.state().type;

    // if there's no colon on the same line, it's not a label
    if (topState == StateType::ExpressionOrLabel)
        enterState(StateType::Expression);
    // if not followed by an identifier on the same line, it's done
    else if (topState == StateType::BreakcontinueStatement)
        leaveState(true);

    topState = currentStatus.state().type;

    // some states might be continued on the next line
    if (topState == StateType::Expression || topState == StateType::ExpressionOrObjectdefinition
        || topState == StateType::ObjectliteralAssignment
        || topState == StateType::TernaryOpAfterColon) {
        enterState(StateType::ExpressionMaybeContinuation);
    }
    // multi-line comment start?
    if (topState != StateType::MultilineCommentStart && topState != StateType::MultilineCommentCont
        && currentStatus.lexerState.state.tokenKind == QQmlJSGrammar::T_PARTIAL_COMMENT) {
        enterState(StateType::MultilineCommentStart);
    }
    currentStatus.finalIndent = currentIndent;
}

// adjusts the indentation of the current line based on the status of the previous one, and what
// it starts with
int indentForLineStartingWithToken(const FormatTextStatus &oldStatus, const FormatOptions &,
                                   int tokenKind)
{
    State topState = oldStatus.state();
    State previousState = oldStatus.state(1);
    int indentDepth = oldStatus.finalIndent;

    // keep user-adjusted indent in multiline comments
    if (topState.type == StateType::MultilineCommentStart
        || topState.type == StateType::MultilineCommentCont) {
        if (!Token::lexKindIsInvalid(tokenKind))
            return -1;
    }
    // don't touch multi-line strings at all
    if (oldStatus.lexerState.state.tokenKind == QQmlJSGrammar::T_PARTIAL_DOUBLE_QUOTE_STRING_LITERAL
        || oldStatus.lexerState.state.tokenKind == QQmlJSGrammar::T_PARTIAL_SINGLE_QUOTE_STRING_LITERAL
        || oldStatus.lexerState.state.tokenKind == QQmlJSGrammar::T_PARTIAL_TEMPLATE_HEAD
        || oldStatus.lexerState.state.tokenKind == QQmlJSGrammar::T_PARTIAL_TEMPLATE_MIDDLE) {
        return -1;
    }

    switch (tokenKind) {
    case QQmlJSGrammar::T_LBRACE:
        if (topState.type == StateType::Substatement
            || topState.type == StateType::BindingAssignment
            || topState.type == StateType::CaseCont) {
            return topState.savedIndentDepth;
        }
        break;
    case QQmlJSGrammar::T_RBRACE: {
        if (topState.type == StateType::JsblockOpen && previousState.type == StateType::CaseCont) {
            return previousState.savedIndentDepth;
        }
        for (int i = 0; oldStatus.state(i).type != StateType::TopmostIntro; ++i) {
            const StateType type = oldStatus.state(i).type;
            if (type == StateType::ObjectdefinitionOpen || type == StateType::JsblockOpen
                || type == StateType::SubstatementOpen || type == StateType::ObjectliteralOpen) {
                return oldStatus.state(i).savedIndentDepth;
            }
        }
        break;
    }
    case QQmlJSGrammar::T_RBRACKET:
        for (int i = 0; oldStatus.state(i).type != StateType::TopmostIntro; ++i) {
            const StateType type = oldStatus.state(i).type;
            if (type == StateType::BracketOpen) {
                return oldStatus.state(i).savedIndentDepth;
            }
        }
        break;
    case QQmlJSGrammar::T_LBRACKET:
    case QQmlJSGrammar::T_LPAREN:
        if (topState.type == StateType::ExpressionMaybeContinuation)
            return topState.savedIndentDepth;
        break;
    case QQmlJSGrammar::T_ELSE:
        if (topState.type == StateType::MaybeElse) {
            return oldStatus.state(1).savedIndentDepth;
        } else if (topState.type == StateType::ExpressionMaybeContinuation) {
            bool hasElse = false;
            for (int i = 1; oldStatus.state(i).type != StateType::TopmostIntro; ++i) {
                const StateType type = oldStatus.state(i).type;
                if (type == StateType::ElseClause)
                    hasElse = true;
                if (type == StateType::IfStatement) {
                    if (hasElse) {
                        hasElse = false;
                    } else {
                        return oldStatus.state(i).savedIndentDepth;
                    }
                }
            }
        }
        break;
    case QQmlJSGrammar::T_CATCH:
    case QQmlJSGrammar::T_FINALLY:
        if (topState.type == StateType::MaybeCatchOrFinally)
            return oldStatus.state(1).savedIndentDepth;
        break;
    case QQmlJSGrammar::T_COLON:
        if (topState.type == StateType::TernaryOp)
            return indentDepth - 2;
        break;
    case QQmlJSGrammar::T_QUESTION:
        if (topState.type == StateType::ExpressionMaybeContinuation)
            return topState.savedIndentDepth;
        break;

    case QQmlJSGrammar::T_DEFAULT:
    case QQmlJSGrammar::T_CASE:
        for (int i = 0; oldStatus.state(i).type != StateType::TopmostIntro; ++i) {
            const StateType type = oldStatus.state(i).type;
            if (type == StateType::SwitchStatement || type == StateType::CaseCont) {
                return oldStatus.state(i).savedIndentDepth;
            } else if (type == StateType::TopmostIntro) {
                break;
            }
        }
        break;
    default:
        if (Token::lexKindIsDelimiter(tokenKind)
            && topState.type == StateType::ExpressionMaybeContinuation)
            return topState.savedIndentDepth;

        break;
    }
    return indentDepth;
}

// sets currentIndent to the correct indent for the current line
int FormatPartialStatus::indentLine()
{
    Q_ASSERT(currentStatus.size() >= 1);
    int firstToken = (lineTokens.isEmpty() ? QQmlJSGrammar::T_NONE : tokenAt(0).lexKind);
    int indent = indentForLineStartingWithToken(initialStatus, options, firstToken);
    recalculateWithIndent(indent);
    return indent;
}

int FormatPartialStatus::indentForNewLineAfter() const
{
    // should be just currentIndent?
    int indent = indentForLineStartingWithToken(currentStatus, options, QQmlJSGrammar::T_NONE);
    if (indent < 0)
        return currentIndent;
    return indent;
}

void FormatPartialStatus::recalculateWithIndent(int indent)
{
    if (indent >= 0) {
        indentOffset = 0;
        int i = 0;
        while (i < line.size() && line.at(i).isSpace())
            ++i;
        indentOffset = indent - column(i);
    }
    currentIndent = initialStatus.finalIndent;
    auto lexerState = currentStatus.lexerState;
    currentStatus = initialStatus;
    currentStatus.lexerState = lexerState;
    tokenIndex = 0;
    handleTokens();
}

FormatPartialStatus formatCodeLine(QStringView line, const FormatOptions &options,
                                   const FormatTextStatus &initialStatus)
{
    FormatPartialStatus status(line, options, initialStatus);

    status.handleTokens();

    return status;
}

void FormatPartialStatus::defaultOnEnter(StateType newState, int *indentDepth,
                                         int *savedIndentDepth) const
{
    const State &parentState = currentStatus.state();
    const Token &tk = tokenAt(tokenIndex);
    const int tokenPosition = column(tk.begin());
    const bool firstToken = (tokenIndex == 0);
    const bool lastToken = (tokenIndex == lineTokens.size() - 1);

    switch (newState) {
    case StateType::ObjectdefinitionOpen: {
        // special case for things like "gradient: Gradient {"
        if (parentState.type == StateType::BindingAssignment)
            *savedIndentDepth = currentStatus.state(1).savedIndentDepth;

        if (firstToken)
            *savedIndentDepth = tokenPosition;

        *indentDepth = *savedIndentDepth + options.indentSize;
        break;
    }

    case StateType::BindingOrObjectdefinition:
        if (firstToken)
            *indentDepth = *savedIndentDepth = tokenPosition;
        break;

    case StateType::BindingAssignment:
    case StateType::ObjectliteralAssignment:
        if (lastToken)
            *indentDepth = *savedIndentDepth + options.indentSize;
        else
            *indentDepth = column(tokenAt(tokenIndex + 1).begin());
        break;

    case StateType::ExpressionOrObjectdefinition:
        *indentDepth = tokenPosition;
        break;

    case StateType::ExpressionOrLabel:
        if (*indentDepth == tokenPosition)
            *indentDepth += 2 * options.indentSize;
        else
            *indentDepth = tokenPosition;
        break;

    case StateType::Expression:
        if (*indentDepth == tokenPosition) {
            // expression_or_objectdefinition doesn't want the indent
            // expression_or_label already has it
            if (parentState.type != StateType::ExpressionOrObjectdefinition
                && parentState.type != StateType::ExpressionOrLabel
                && parentState.type != StateType::BindingAssignment) {
                *indentDepth += 2 * options.indentSize;
            }
        }
        // expression_or_objectdefinition and expression_or_label have already consumed the
        // first token
        else if (parentState.type != StateType::ExpressionOrObjectdefinition
                 && parentState.type != StateType::ExpressionOrLabel) {
            *indentDepth = tokenPosition;
        }
        break;

    case StateType::ExpressionMaybeContinuation:
        // set indent depth to indent we'd get if the expression ended here
        for (int i = 1; currentStatus.state(i).type != StateType::TopmostIntro; ++i) {
            const StateType type = currentStatus.state(i).type;
            if (FormatTextStatus::isExpressionEndState(type)
                && !FormatTextStatus::isBracelessState(type)) {
                *indentDepth = currentStatus.state(i - 1).savedIndentDepth;
                break;
            }
        }
        break;

    case StateType::BracketOpen:
        if (parentState.type == StateType::Expression
            && currentStatus.state(1).type == StateType::BindingAssignment) {
            *savedIndentDepth = currentStatus.state(2).savedIndentDepth;
            *indentDepth = *savedIndentDepth + options.indentSize;
        } else if (parentState.type == StateType::ObjectliteralAssignment) {
            *savedIndentDepth = parentState.savedIndentDepth;
            *indentDepth = *savedIndentDepth + options.indentSize;
        } else if (!lastToken) {
            *indentDepth = tokenPosition + 1;
        } else {
            *indentDepth = *savedIndentDepth + options.indentSize;
        }
        break;

    case StateType::FunctionStart:
        // align to the beginning of the line
        *savedIndentDepth = *indentDepth = column(tokenAt(0).begin());
        break;

    case StateType::DoStatementWhileParenOpen:
    case StateType::StatementWithConditionParenOpen:
    case StateType::SignalArglistOpen:
    case StateType::FunctionArglistOpen:
    case StateType::ParenOpen:
        if (!lastToken)
            *indentDepth = tokenPosition + 1;
        else
            *indentDepth += options.indentSize;
        break;

    case StateType::TernaryOp:
        if (!lastToken)
            *indentDepth = tokenPosition + tk.length + 1;
        else
            *indentDepth += options.indentSize;
        break;

    case StateType::JsblockOpen:
        // closing brace should be aligned to case
        if (parentState.type == StateType::CaseCont) {
            *savedIndentDepth = parentState.savedIndentDepth;
            break;
        }
        Q_FALLTHROUGH();
    case StateType::SubstatementOpen:
        // special case for "foo: {" and "property int foo: {"
        if (parentState.type == StateType::BindingAssignment)
            *savedIndentDepth = currentStatus.state(1).savedIndentDepth;
        *indentDepth = *savedIndentDepth + options.indentSize;
        break;

    case StateType::Substatement:
        *indentDepth += options.indentSize;
        break;

    case StateType::ObjectliteralOpen:
        if (parentState.type == StateType::Expression
            || parentState.type == StateType::ObjectliteralAssignment) {
            // undo the continuation indent of the expression
            if (currentStatus.state(1).type == StateType::ExpressionOrLabel)
                *indentDepth = currentStatus.state(1).savedIndentDepth;
            else
                *indentDepth = parentState.savedIndentDepth;
            *savedIndentDepth = *indentDepth;
        }
        *indentDepth += options.indentSize;
        break;

    case StateType::StatementWithCondition:
    case StateType::TryStatement:
    case StateType::CatchStatement:
    case StateType::FinallyStatement:
    case StateType::IfStatement:
    case StateType::DoStatement:
    case StateType::SwitchStatement:
        if (firstToken || parentState.type == StateType::BindingAssignment)
            *savedIndentDepth = tokenPosition;
        // ### continuation
        *indentDepth = *savedIndentDepth; // + 2*options.indentSize;
        // special case for 'else if'
        if (!firstToken && newState == StateType::IfStatement
            && parentState.type == StateType::Substatement
            && currentStatus.state(1).type == StateType::ElseClause) {
            *indentDepth = currentStatus.state(1).savedIndentDepth;
            *savedIndentDepth = *indentDepth;
        }
        break;

    case StateType::MaybeElse:
    case StateType::MaybeCatchOrFinally: {
        // set indent to where leave(true) would put it
        int lastNonEndState = 0;
        while (!FormatTextStatus::isExpressionEndState(
                currentStatus.state(lastNonEndState + 1).type))
            ++lastNonEndState;
        *indentDepth = currentStatus.state(lastNonEndState).savedIndentDepth;
        break;
    }

    case StateType::ConditionOpen:
        // fixed extra indent when continuing 'if (', but not for 'else if ('
        if (tokenPosition <= *indentDepth + options.indentSize)
            *indentDepth += 2 * options.indentSize;
        else
            *indentDepth = tokenPosition + 1;
        break;

    case StateType::CaseStart:
        *savedIndentDepth = tokenPosition;
        break;

    case StateType::CaseCont:
        *indentDepth += options.indentSize;
        break;

    case StateType::MultilineCommentStart:
        *indentDepth = tokenPosition + 2;
        break;

    case StateType::MultilineCommentCont:
        *indentDepth = tokenPosition;
        break;
    default:
        break;
    }
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
