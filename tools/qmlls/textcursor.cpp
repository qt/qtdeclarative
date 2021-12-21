/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include "textcursor.h"
#include "textdocument.h"
#include "textblock.h"

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

}
