// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textdocument.h"
#include "textblock.h"

namespace Utils {

TextDocument::TextDocument(const QString &text)
{
    setPlainText(text);
}

TextBlock TextDocument::findBlockByNumber(int blockNumber) const
{
    return (blockNumber >= 0 && blockNumber < m_blocks.size())
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
    return m_content.size();
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
    while (blockStart < text.size()) {
        Block block;
        block.textBlock.setBlockNumber(blockNumber++);
        block.textBlock.setPosition(blockStart);
        block.textBlock.setDocument(this);

        int blockEnd = text.indexOf('\n', blockStart) + 1;
        if (blockEnd == 0)
            blockEnd = text.size();

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
    if (blockNumber >= 0 && blockNumber < m_blocks.size())
        m_blocks[blockNumber].userState = state;
}

int TextDocument::userState(int blockNumber) const
{
    return (blockNumber >= 0 && blockNumber < m_blocks.size()) ? m_blocks[blockNumber].userState
                                                                 : -1;
}

QMutex *TextDocument::mutex() const
{
    return &m_mutex;
}

} // namespace Utils
