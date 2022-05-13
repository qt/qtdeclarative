// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef TEXTDOCUMENT_H
#define TEXTDOCUMENT_H

#include "textblock.h"

#include <QtCore/qchar.h>
#include <QtCore/qvector.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qmutex.h>

#include <optional>

namespace Utils {

class TextBlockUserData;

class TextDocument
{
public:
    TextDocument() = default;
    explicit TextDocument(const QString &text);

    TextBlock findBlockByNumber(int blockNumber) const;
    TextBlock findBlockByLineNumber(int lineNumber) const;
    QChar characterAt(int pos) const;
    int characterCount() const;
    TextBlock begin() const;
    TextBlock firstBlock() const;
    TextBlock lastBlock() const;

    std::optional<int> version() const;
    void setVersion(std::optional<int>);

    QString toPlainText() const;
    void setPlainText(const QString &text);

    bool isModified() const;
    void setModified(bool modified);

    void setUndoRedoEnabled(bool enable);

    void clear();

    void setUserState(int blockNumber, int state);
    int userState(int blockNumber) const;
    QMutex *mutex() const;

private:
    struct Block
    {
        TextBlock textBlock;
        int userState = -1;
    };

    QVector<Block> m_blocks;

    QString m_content;
    bool m_modified = false;
    std::optional<int> m_version;
    mutable QMutex m_mutex;
};
} // namespace Utils

#endif // TEXTDOCUMENT_H
