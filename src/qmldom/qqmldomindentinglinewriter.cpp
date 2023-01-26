// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomindentinglinewriter_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

FormatPartialStatus &IndentingLineWriter::fStatus()
{
    if (!m_fStatusValid) {
        m_fStatus = formatCodeLine(m_currentLine, m_options.formatOptions, m_preCachedStatus);
        m_fStatusValid = true;
    }
    return m_fStatus;
}

void IndentingLineWriter::willCommit()
{
    m_preCachedStatus = fStatus().currentStatus;
}

void IndentingLineWriter::reindentAndSplit(QString eol, bool eof)
{
    bool shouldReindent = m_reindent;
indentAgain:
    // maybe re-indent
    if (shouldReindent && m_columnNr == 0) {
        setLineIndent(fStatus().indentLine());
    }
    if (!eol.isEmpty() || eof) {
        LineWriterOptions::TrailingSpace trailingSpace;
        if (!m_currentLine.isEmpty() && m_currentLine.trimmed().isEmpty()) {
            // space only line
            const Scanner::State &oldState = m_preCachedStatus.lexerState;
            if (oldState.isMultilineComment())
                trailingSpace = m_options.commentTrailingSpace;
            else if (oldState.isMultiline())
                trailingSpace = m_options.stringTrailingSpace;
            else
                trailingSpace = m_options.codeTrailingSpace;
            // in the LSP we will probably want to treat is specially if it is the line with the
            // cursor,  of if indentation of it is requested
        } else {
            const Scanner::State &currentState = fStatus().currentStatus.lexerState;
            if (currentState.isMultilineComment()) {
                trailingSpace = m_options.commentTrailingSpace;
            } else if (currentState.isMultiline()) {
                trailingSpace = m_options.stringTrailingSpace;
            } else {
                const int kind =
                        (fStatus().lineTokens.isEmpty() ? Lexer::T_EOL
                                                        : fStatus().lineTokens.last().lexKind);
                if (Token::lexKindIsComment(kind)) {
                    // a // comment...
                    trailingSpace = m_options.commentTrailingSpace;
                    Q_ASSERT(fStatus().currentStatus.state().type
                                     != FormatTextStatus::StateType::MultilineCommentCont
                             && fStatus().currentStatus.state().type
                                     != FormatTextStatus::StateType::
                                             MultilineCommentStart); // these should have been
                                                                     // handled above
                } else {
                    trailingSpace = m_options.codeTrailingSpace;
                }
            }
        }
        handleTrailingSpace(trailingSpace);
    }
    // maybe split long line
    if (m_options.maxLineLength > 0 && m_currentLine.size() > m_options.maxLineLength) {
        int possibleSplit = -1;
        if (fStatus().lineTokens.size() > 1) {
            // {}[] should already be handled (handle also here?)
            int minLen = 0;
            while (minLen < m_currentLine.size() && m_currentLine.at(minLen).isSpace())
                ++minLen;
            minLen = column(minLen) + m_options.minContentLength;
            int maxLen = qMax(minLen + m_options.strongMaxLineExtra, m_options.maxLineLength);
            std::array<QSet<int>, 2> splitSequence(
                    { QSet<int>({ // try split after ',','||','&&'
                                  QQmlJSGrammar::T_COMMA, QQmlJSGrammar::T_AND_AND,
                                  QQmlJSGrammar::T_OR_OR }),
                      QSet<int>({ // try split after '('
                                  QQmlJSGrammar::T_LPAREN }) });
            // try split after other binary operators?
            int minSplit = m_currentLine.size();
            for (const QSet<int> &splitOnToken : splitSequence) {
                for (int iToken = 0; iToken < fStatus().tokenCount(); ++iToken) {
                    const Token t = fStatus().tokenAt(iToken);
                    int tCol = column(t.end());
                    if (splitOnToken.contains(t.lexKind) && tCol > minLen) {
                        if (tCol <= maxLen && possibleSplit < t.end())
                            possibleSplit = t.end();
                        if (t.end() < minSplit)
                            minSplit = t.end();
                    }
                }
                if (possibleSplit > 0)
                    break;
            }
            if (possibleSplit == -1 && minSplit + 4 < m_currentLine.size())
                possibleSplit = minSplit;
            if (possibleSplit > 0) {
                lineChanged();
                quint32 oChange = eolToWrite().size();
                changeAtOffset(m_utf16Offset + possibleSplit, oChange, 0,
                               0); // line & col change updated in commitLine
                commitLine(eolToWrite(), TextAddType::NewlineSplit, possibleSplit);
                shouldReindent = true;
                goto indentAgain;
            }
        }
    }
    // maybe write out
    if (!eol.isEmpty() || eof)
        commitLine(eol);
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
