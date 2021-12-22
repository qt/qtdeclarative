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

#include "textblock.h"
#include "textdocument.h"

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
