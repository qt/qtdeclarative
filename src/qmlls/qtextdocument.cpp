// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtextdocument_p.h"
#include "qtextblock_p.h"

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

    const auto appendToBlocks = [this](int blockNumber, int start, int length) {
        Block block;
        block.textBlock.setBlockNumber(blockNumber);
        block.textBlock.setPosition(start);
        block.textBlock.setDocument(this);
        block.textBlock.setLength(length);
        m_blocks.append(block);
    };

    int blockStart = 0;
    int blockNumber = -1;
    while (blockStart < text.size()) {
        int blockEnd = text.indexOf(u'\n', blockStart) + 1;
        if (blockEnd == 0)
            blockEnd = text.size();
        appendToBlocks(++blockNumber, blockStart, blockEnd - blockStart);
        blockStart = blockEnd;
    }
    // Add an empty block if the text ends with \n. This is required for retrieving
    // the actual line of the text editor if requested, for example, in findBlockByNumber.
    // Consider a case with text aa\nbb\n\n. You are on 4th line of the text editor and even
    // if it is an empty line, we introduce a text block for it to maybe use later.
    if (text.endsWith(u'\n'))
        appendToBlocks(++blockNumber, blockStart, 0);
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
