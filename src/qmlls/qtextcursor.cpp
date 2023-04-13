// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtextcursor_p.h"
#include "qtextdocument_p.h"
#include "qtextblock_p.h"

namespace Utils {

class TextFrame;
class TextTable;
class TextTableCell;

TextCursor::TextCursor(TextDocument *document) : m_document(document) { }

bool TextCursor::movePosition(TextCursor::MoveOperation op, TextCursor::MoveMode mode, int n)
{
    Q_UNUSED(n);
    switch (op) {
    case NoMove:
        return true;
    case Start:
        m_position = 0;
        break;
    case PreviousCharacter:
        while (--n >= 0) {
            if (m_position == 0)
                return false;
            --m_position;
        }
        break;
    case End:
        m_position = m_document->characterCount();
        break;
    case NextCharacter:
        while (--n >= 0) {
            if (m_position == m_document->characterCount())
                return false;
            ++m_position;
        }
        break;
    }

    if (mode == MoveAnchor)
        m_anchor = m_position;

    return false;
}

int TextCursor::position() const
{
    return m_position;
}

void TextCursor::setPosition(int pos, Utils::TextCursor::MoveMode mode)
{
    m_position = pos;
    if (mode == MoveAnchor)
        m_anchor = pos;
}

QString TextCursor::selectedText() const
{
    return m_document->toPlainText().mid(qMin(m_position, m_anchor), qAbs(m_position - m_anchor));
}

void TextCursor::clearSelection()
{
    m_anchor = m_position;
}

TextDocument *TextCursor::document() const
{
    return m_document;
}

void TextCursor::insertText(const QString &text)
{
    const QString orig = m_document->toPlainText();
    const QString left = orig.left(qMin(m_position, m_anchor));
    const QString right = orig.mid(qMax(m_position, m_anchor));
    m_document->setPlainText(left + text + right);
}

TextBlock TextCursor::block() const
{
    TextBlock current = m_document->firstBlock();
    while (current.isValid()) {
        if (current.position() <= position()
            && current.position() + current.length() > current.position())
            break;
        current = current.next();
    }
    return current;
}

int TextCursor::positionInBlock() const
{
    return m_position - block().position();
}

int TextCursor::blockNumber() const
{
    return block().blockNumber();
}

void TextCursor::removeSelectedText()
{
    insertText(QString());
}

int TextCursor::selectionEnd() const
{
    return qMax(m_position, m_anchor);
}

bool TextCursor::isNull() const
{
    return m_document == nullptr;
}

} // namespace Utils
