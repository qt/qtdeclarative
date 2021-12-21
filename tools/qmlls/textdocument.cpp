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

#include "textdocument.h"
#include "textblock.h"

namespace Utils {

TextDocument::TextDocument(const QString &text)
{
    setPlainText(text);
}

TextBlock TextDocument::findBlockByNumber(int blockNumber) const
{
    return (blockNumber >= 0 && blockNumber < m_blocks.length())
            ? m_blocks.at(blockNumber).textBlock
            : TextBlock();
}

TextBlock TextDocument::findBlockByLineNumber(int lineNumber) const
{
    return findBlockByNumber(lineNumber);
}

QChar TextDocument::characterAt(int pos) const
{
    return m_content.at(pos);
}

int TextDocument::characterCount() const
{
    return m_content.length();
}

TextBlock TextDocument::begin() const
{
    return m_blocks.isEmpty() ? TextBlock() : m_blocks.at(0).textBlock;
}

TextBlock TextDocument::firstBlock() const
{
    return begin();
}

TextBlock TextDocument::lastBlock() const
{
    return m_blocks.isEmpty() ? TextBlock() : m_blocks.last().textBlock;
}

std::optional<int> TextDocument::version() const
{
    return m_version;
}

void TextDocument::setVersion(std::optional<int> v)
{
    m_version = v;
}

QString TextDocument::toPlainText() const
{
    return m_content;
}

void TextDocument::setPlainText(const QString &text)
{
    m_content = text;
    m_blocks.clear();

    int blockStart = 0;
    int blockNumber = 0;
    while (blockStart < text.length()) {
        Block block;
        block.textBlock.setBlockNumber(blockNumber++);
        block.textBlock.setPosition(blockStart);
        block.textBlock.setDocument(this);

        int blockEnd = text.indexOf('\n', blockStart) + 1;
        if (blockEnd == 0)
            blockEnd = text.length();

        block.textBlock.setLength(blockEnd - blockStart);
        m_blocks.append(block);
        blockStart = blockEnd;
    }
}

bool TextDocument::isModified() const
{
    return m_modified;
}

void TextDocument::setModified(bool modified)
{
    m_modified = modified;
}

void TextDocument::setUserState(int blockNumber, int state)
{
    if (blockNumber >= 0 && blockNumber < m_blocks.length())
        m_blocks[blockNumber].userState = state;
}

int TextDocument::userState(int blockNumber) const
{
    return (blockNumber >= 0 && blockNumber < m_blocks.length()) ? m_blocks[blockNumber].userState
                                                                 : -1;
}

QMutex *TextDocument::mutex() const
{
    return &m_mutex;
}

} // namespace Utils
