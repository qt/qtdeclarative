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
