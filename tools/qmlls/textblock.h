// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
