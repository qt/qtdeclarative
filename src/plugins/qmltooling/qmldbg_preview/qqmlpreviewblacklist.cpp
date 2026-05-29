// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpreviewblacklist.h"

QT_BEGIN_NAMESPACE

void QQmlPreviewBlacklist::blacklist(const QString &path)
{
    if (!path.isEmpty())
        m_root.insert(path, 0);
}

void QQmlPreviewBlacklist::whitelist(const QString &path)
{
    if (!path.isEmpty())
        m_root.remove(path, 0);
}

bool QQmlPreviewBlacklist::isBlacklisted(const QString &path) const
{
    return path.isEmpty() ? true : m_root.findPrefix(path, 0) == Node::MatchedLeaf;
}

void QQmlPreviewBlacklist::clear()
{
    m_root = Node();
}

QQmlPreviewBlacklist::Node::Node()
{
}

QQmlPreviewBlacklist::Node::Node(const QQmlPreviewBlacklist::Node &other) :
    m_mine(other.m_mine), m_isLeaf(other.m_isLeaf)
{
    for (auto it = other.m_next.begin(), end = other.m_next.end(); it != end; ++it)
        m_next.insert(it.key(), new Node(**it));
}

QQmlPreviewBlacklist::Node::Node(QQmlPreviewBlacklist::Node &&other) noexcept
{
    m_mine.swap(other.m_mine);
    m_next.swap(other.m_next);
    m_isLeaf = other.m_isLeaf;
}

QQmlPreviewBlacklist::Node::~Node()
{
    qDeleteAll(m_next);
}

QQmlPreviewBlacklist::Node &QQmlPreviewBlacklist::Node::operator=(
        const QQmlPreviewBlacklist::Node &other)
{
    if (&other != this) {
        m_mine = other.m_mine;
        for (auto it = other.m_next.begin(), end = other.m_next.end(); it != end; ++it)
            m_next.insert(it.key(), new Node(**it));
        m_isLeaf = other.m_isLeaf;
    }
    return *this;
}

QQmlPreviewBlacklist::Node &QQmlPreviewBlacklist::Node::operator=(
        QQmlPreviewBlacklist::Node &&other) noexcept
{
    if (&other != this) {
        m_mine.swap(other.m_mine);
        m_next.swap(other.m_next);
        m_isLeaf = other.m_isLeaf;
    }
    return *this;
}

void QQmlPreviewBlacklist::Node::split(QString::iterator it, QString::iterator end)
{
    QString existing;
    existing.resize(end - it - 1);
    std::copy(it + 1, end, existing.begin());

    Node *node = new Node(existing, m_next, m_isLeaf);
    m_next.clear();
    m_next.insert(*it, node);
    m_mine.resize(it - m_mine.begin());
    m_isLeaf = false;
}

void QQmlPreviewBlacklist::Node::insert(const QString &path, int offset)
{
    for (auto it = m_mine.begin(), end = m_mine.end(); it != end; ++it) {
        if (offset == path.size()) {
            split(it, end);
            m_isLeaf = true;
            return;
        }

        if (path.at(offset) != *it) {
            split(it, end);

            QString inserted;
            inserted.resize(path.size() - offset - 1);
            std::copy(path.begin() + offset + 1, path.end(), inserted.begin());
            m_next.insert(path.at(offset), new Node(inserted));
            return;
        }

        ++offset;
    }

    if (offset == path.size()) {
        m_isLeaf = true;
        return;
    }

    Node *&node = m_next[path.at(offset++)];
    if (node == nullptr) {
        QString inserted;
        inserted.resize(path.size() - offset);
        std::copy(path.begin() + offset, path.end(), inserted.begin());
        node = new Node(inserted);
    } else {
        node->insert(path, offset);
    }
}

void QQmlPreviewBlacklist::Node::remove(const QString &path, int offset)
{
    for (auto it = m_mine.begin(), end = m_mine.end(); it != end; ++it) {
        if (offset == path.size() || path.at(offset) != *it) {
            split(it, end);
            return;
        }
        ++offset;
    }

    m_isLeaf = false;
    if (offset == path.size())
        return;

    Node *&node = m_next[path.at(offset++)];
    if (node) {
        node->remove(path, offset);
    } else {
        QString inserted;
        inserted.resize(path.size() - offset);
        std::copy(path.begin() + offset, path.end(), inserted.begin());
        node = new Node(inserted, {}, false);
    }
}

QQmlPreviewBlacklist::Node::PrefixResult QQmlPreviewBlacklist::Node::findPrefix(
        const QString &path, int offset) const
{
    if (offset == path.size()) {
        if (!m_mine.isEmpty())
            return Unmatched;
        return m_isLeaf ? MatchedLeaf : MatchedBranch;
    }

    for (auto it = m_mine.begin(), end = m_mine.end(); it != end; ++it) {
        if (path.at(offset) != *it)
            return Unmatched;

        if (++offset == path.size()) {
            if (++it != end)
                return Unmatched;
            return m_isLeaf ? MatchedLeaf : MatchedBranch;
        }
    }

    const QChar c = path.at(offset);
    const auto it = m_next.find(c);
    if (it != m_next.end()) {
        const PrefixResult result = (*it)->findPrefix(path, offset + 1);
        if (result != Unmatched)
            return result;
    }

    if (c == '/')
        return m_isLeaf ? MatchedLeaf : MatchedBranch;

    return Unmatched;
}

QQmlPreviewBlacklist::Node::Node(const QString &mine,
                                 const QHash<QChar, QQmlPreviewBlacklist::Node *> &next,
                                 bool isLeaf)
    : m_mine(mine), m_next(next), m_isLeaf(isLeaf)
{
}

QT_END_NAMESPACE
