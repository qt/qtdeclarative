// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLPREVIEWBLACKLIST_H
#define QQMLPREVIEWBLACKLIST_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qhash.h>
#include <QtCore/qchar.h>
#include <QtCore/qstring.h>
#include <algorithm>

QT_BEGIN_NAMESPACE

class QQmlPreviewBlacklist
{
public:
    void blacklist(const QString &path);
    void whitelist(const QString &path);
    bool isBlacklisted(const QString &path) const;
    void clear();

private:
    class Node {
    public:
        enum PrefixResult {
            MatchedLeaf,
            MatchedBranch,
            Unmatched,
        };

        Node();
        Node(const Node &other);
        Node(Node &&other) noexcept;

        ~Node();

        Node &operator=(const Node &other);
        Node &operator=(Node &&other) noexcept;

        void split(QString::iterator it, QString::iterator end);
        void insert(const QString &path, int offset);
        void remove(const QString &path, int offset);
        PrefixResult findPrefix(const QString &path, int offset) const;

    private:
        Node(const QString &mine, const QHash<QChar, Node *> &next = QHash<QChar, Node *>(),
             bool isLeaf = true);

        QString m_mine;
        QHash<QChar, Node *> m_next;
        bool m_isLeaf = false;
    };

    Node m_root;
};

QT_END_NAMESPACE

#endif // QQMLPREVIEWBLACKLIST_H
