// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtextblock_p.h"
#include "qtextdocument_p.h"

#include <QtCore/qstring.h>

namespace Utils {

bool TextBlock::isValid() const
{
    return m_document;
}

void TextBlock::setBlockNumber(int blockNumber)
{
    m_blockNumber = blockNumber;
}

int TextBlock::blockNumber() const
{
    return m_blockNumber;
}

void TextBlock::setPosition(int position)
{
    m_position = position;
}

int TextBlock::position() const
{
    return m_position;
}

void TextBlock::setLength(int length)
{
    m_length = length;
}

int TextBlock::length() const
{
    return m_length;
}

TextBlock TextBlock::next() const
{
    return m_document->findBlockByNumber(m_blockNumber + 1);
}

TextBlock TextBlock::previous() const
{
    return m_document->findBlockByNumber(m_blockNumber - 1);
}

int TextBlock::userState() const
{
    return m_document->userState(m_blockNumber);
}

void TextBlock::setUserState(int state)
{
    m_document->setUserState(m_blockNumber, state);
}

void TextBlock::setDocument(TextDocument *document)
{
    m_document = document;
}

TextDocument *TextBlock::document() const
{
    return m_document;
}

QString TextBlock::text() const
{
    return document()->toPlainText().mid(position(), length());
}

int TextBlock::revision() const
{
    return m_revision;
}

void TextBlock::setRevision(int rev)
{
    m_revision = rev;
}

bool operator==(const TextBlock &t1, const TextBlock &t2)
{
    return t1.document() == t2.document() && t1.blockNumber() == t2.blockNumber();
}

bool operator!=(const TextBlock &t1, const TextBlock &t2)
{
    return !(t1 == t2);
}

} // namespace Utils
