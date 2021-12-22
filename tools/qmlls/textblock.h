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
#ifndef TEXTBLOCK_H
#define TEXTBLOCK_H

#include <QtCore/qstring.h>

namespace Utils {

class TextDocument;
class TextBlockUserData;

class TextBlock
{
public:
    bool isValid() const;

    void setBlockNumber(int blockNumber);
    int blockNumber() const;

    void setPosition(int position);
    int position() const;

    void setLength(int length);
    int length() const;

    TextBlock next() const;
    TextBlock previous() const;

    int userState() const;
    void setUserState(int state);

    bool isVisible() const;
    void setVisible(bool visible);

    void setLineCount(int count);
    int lineCount() const;

    void setDocument(TextDocument *document);
    TextDocument *document() const;

    QString text() const;

    int revision() const;
    void setRevision(int rev);

    friend bool operator==(const TextBlock &t1, const TextBlock &t2);
    friend bool operator!=(const TextBlock &t1, const TextBlock &t2);

private:
    TextDocument *m_document = nullptr;
    int m_revision = 0;

    int m_position = 0;
    int m_length = 0;
    int m_blockNumber = -1;
};

} // namespace Utils

#endif // TEXTBLOCK_H
