/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "commentastvisitor.h"

CommentAstVisitor::CommentAstVisitor(QQmlJS::Engine *engine, Node *rootNode) : m_engine(engine)
{
    rootNode->accept(this);

    // Look for complete orphans that have not been attached to *any* node
    QVector<Comment> completeOrphans;

    for (const auto &comment : m_engine->comments()) {
        if (isCommentAttached(comment))
            continue;

        bool found_orphan = false;
        for (const auto &orphanList : orphanComments().values()) {
            for (const auto &orphan : orphanList) {
                if (orphan.contains(comment)) {
                    found_orphan = true;
                    break;
                }
            }

            if (found_orphan)
                break;
        }

        if (found_orphan)
            continue;

        completeOrphans.append(Comment(m_engine, Comment::Location::Front, {comment}));
    }

    m_orphanComments[nullptr] = completeOrphans;
}

QList<SourceLocation> CommentAstVisitor::findCommentsInLine(quint32 line, bool includePrevious) const
{
    QList<SourceLocation> results;
    if (line == 0)
        return results;

    for (const auto &location : m_engine->comments()) {
        Comment comment(m_engine, Comment::Location::Front, { location });
        if (line < location.startLine || line > comment.endLine())
            continue;

        if (isCommentAttached(location))
            continue;

        results.append(location);

        if (includePrevious) {
            // See if we can find any more comments above this one
            auto previous = findCommentsInLine(location.startLine - 1, true);

            // Iterate it in reverse to restore the correct order
            for (auto it = previous.rbegin(); it != previous.rend(); it++) {
                results.prepend(*it);
            }
        }

        break;
    }

    return results;
}

bool CommentAstVisitor::isCommentAttached(const SourceLocation &location) const
{
    for (const auto &value : m_attachedComments.values()) {
        if (value.contains(location))
            return true;
    }

    for (const auto &value : m_listItemComments.values()) {
        if (value.contains(location))
            return true;
    }

    // If a comment is already marked as an orphan of a Node that counts as attached too.
    for (const auto &orphanList : m_orphanComments.values()) {
        for (const auto &value : orphanList) {
            if (value.contains(location))
                return true;
        }
    }

    return false;
}

Comment CommentAstVisitor::findComment(SourceLocation first, SourceLocation last,
                                       int locations) const
{
    if (locations & Comment::Location::Front) {
        quint32 searchAt = first.startLine - 1;

        const auto comments = findCommentsInLine(searchAt, true);
        if (!comments.isEmpty())
            return Comment(m_engine, Comment::Location::Front, comments);
    }

    if (locations & Comment::Location::Front_Inline) {
        quint32 searchAt = first.startLine;

        const auto comments = findCommentsInLine(searchAt);
        if (!comments.isEmpty())
            return Comment(m_engine, Comment::Location::Front_Inline, comments);
    }

    if (locations & Comment::Location::Back_Inline) {
        quint32 searchAt = last.startLine;

        const auto comments = findCommentsInLine(searchAt);
        if (!comments.isEmpty())
            return Comment(m_engine, Comment::Location::Back_Inline, comments);
    }

    if (locations & Comment::Location::Back) {
        quint32 searchAt = last.startLine + 1;

        const auto comments = findCommentsInLine(searchAt);
        if (!comments.isEmpty())
            return Comment(m_engine, Comment::Location::Back, comments);
    }

    return Comment();

}

Comment CommentAstVisitor::findComment(Node *node, int locations) const
{
    return findComment(node->firstSourceLocation(), node->lastSourceLocation(), locations);
}

QVector<Comment> CommentAstVisitor::findOrphanComments(Node *node) const
{
    QVector<Comment> comments;

    for (auto &comment : m_engine->comments()) {
        if (isCommentAttached(comment))
            continue;

        if (comment.begin() <= node->firstSourceLocation().begin()
            || comment.end() > node->lastSourceLocation().end()) {
            continue;
        }

        comments.append(Comment(m_engine, Comment::Location::Front, {comment}));
    }

    return comments;
}

void CommentAstVisitor::attachComment(Node *node, int locations)
{
    auto comment = findComment(node, locations);

    if (comment.isValid())
        m_attachedComments[node] = comment;
}

bool CommentAstVisitor::visit(UiScriptBinding *node)
{
    attachComment(node);
    return true;
}

bool CommentAstVisitor::visit(StatementList *node)
{
    for (auto *item = node; item != nullptr; item = item->next)
        attachComment(item->statement, Comment::Front | Comment::Back_Inline);
    return true;
}

void CommentAstVisitor::endVisit(StatementList *node)
{
    m_orphanComments[node] = findOrphanComments(node);
}

bool CommentAstVisitor::visit(UiObjectBinding *node)
{
    attachComment(node, Comment::Front | Comment::Front_Inline | Comment::Back);
    return true;
}

bool CommentAstVisitor::visit(UiObjectDefinition *node)
{
    attachComment(node, Comment::Front | Comment::Front_Inline | Comment::Back);
    return true;
}

void CommentAstVisitor::endVisit(UiObjectDefinition *node)
{
    m_orphanComments[node] = findOrphanComments(node);
}

bool CommentAstVisitor::visit(UiArrayBinding *node)
{
    attachComment(node);
    return true;
}

void CommentAstVisitor::endVisit(UiArrayBinding *node)
{
    m_orphanComments[node] = findOrphanComments(node);
}

bool CommentAstVisitor::visit(UiEnumDeclaration *node)
{
    attachComment(node);
    return true;
}

void CommentAstVisitor::endVisit(UiEnumDeclaration *node)
{
    m_orphanComments[node] = findOrphanComments(node);
}

bool CommentAstVisitor::visit(UiEnumMemberList *node)
{
    for (auto *item = node; item != nullptr; item = item->next) {
        auto comment = findComment(item->memberToken,
                                   item->valueToken.isValid() ? item->valueToken : item->memberToken,
                                   Comment::Front | Comment::Back_Inline);

        if (comment.isValid())
            m_listItemComments[item->memberToken.begin()] = comment;
    }

    m_orphanComments[node] = findOrphanComments(node);

    return true;
}

bool CommentAstVisitor::visit(UiPublicMember *node)
{
    attachComment(node);
    return true;
}

bool CommentAstVisitor::visit(FunctionDeclaration *node)
{
    attachComment(node);
    return true;
}

bool CommentAstVisitor::visit(UiImport *node)
{
    attachComment(node);
    return true;
}

bool CommentAstVisitor::visit(UiPragma *node)
{
    attachComment(node);
    return true;
}
