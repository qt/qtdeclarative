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

#ifndef COMMENTASTVISITOR_H
#define COMMENTASTVISITOR_H

#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsengine_p.h>

#include <QHash>
#include <QString>
#include <QVector>

using namespace QQmlJS::AST;
using namespace QQmlJS;

struct Comment
{
    enum Location : int
    {
        Front = 1,
        Front_Inline = Front << 1,
        Back = Front_Inline << 1,
        Back_Inline = Back << 1,
        DefaultLocations = Front | Back_Inline,
        AllLocations = Front | Back | Front_Inline | Back_Inline
    } m_location = Front;

    Comment() = default;
    Comment(const QQmlJS::Engine *engine, Location location, QList<SourceLocation> srcLocations)
        : m_location(location), m_srcLocations(srcLocations) {
        for (const auto& srcLoc : srcLocations) {
            m_text += engine->code().mid(static_cast<int>(srcLoc.begin()),
                                         static_cast<int>(srcLoc.end() - srcLoc.begin())) + "\n";
        }

        m_text.chop(1);
    }

    QList<SourceLocation> m_srcLocations;

    bool isValid() const { return !m_srcLocations.isEmpty(); }
    bool isMultiline() const { return m_text.contains("\n"); }
    bool isSyntheticMultiline() const { return m_srcLocations.size() > 1; }

    bool contains(const SourceLocation& location) const {
        for (const SourceLocation& srcLoc : m_srcLocations) {
            if (srcLoc.begin() == location.begin() && srcLoc.end() == location.end())
                return true;
        }

        return false;
    }

    quint32 endLine() const
    {
        if (isSyntheticMultiline() || !isValid())
            return 0;

        return m_srcLocations[0].startLine + m_text.count(QLatin1Char('\n'));
    }

    QString m_text;
};

class CommentAstVisitor : protected Visitor
{
public:
    CommentAstVisitor(QQmlJS::Engine *engine, Node *rootNode);

    void throwRecursionDepthError() override {}

    const QHash<Node *, Comment> attachedComments() const { return m_attachedComments; }
    const QHash<quint32, Comment> listComments() const { return m_listItemComments; }
    const QHash<Node *, QVector<Comment>> orphanComments() const { return m_orphanComments; }

    bool visit(UiScriptBinding *node) override;
    bool visit(UiObjectBinding *node) override;

    bool visit(UiArrayBinding *node) override;
    void endVisit(UiArrayBinding *node) override;

    bool visit(UiObjectDefinition *node) override;
    void endVisit(UiObjectDefinition *) override;

    bool visit(UiEnumDeclaration *node) override;
    void endVisit(UiEnumDeclaration *node) override;

    bool visit(UiEnumMemberList *node) override;

    bool visit(StatementList *node) override;
    void endVisit(StatementList *node) override;

    bool visit(UiImport *node) override;
    bool visit(UiPragma *node) override;
    bool visit(UiPublicMember *node) override;
    bool visit(FunctionDeclaration *node) override;
private:
    bool isCommentAttached(const SourceLocation& location) const;

    QList<SourceLocation> findCommentsInLine(quint32 line, bool includePrevious = false) const;

    Comment findComment(SourceLocation first, SourceLocation last,
                        int locations = Comment::DefaultLocations) const;

    Comment findComment(Node *node, int locations = Comment::DefaultLocations) const;
    QVector<Comment> findOrphanComments(Node *node) const;
    void attachComment(Node *node, int locations = Comment::DefaultLocations);

    QQmlJS::Engine *m_engine;
    QHash<Node *, Comment> m_attachedComments;
    QHash<quint32, Comment> m_listItemComments;
    QHash<Node *, QVector<Comment>> m_orphanComments;
};

#endif // COMMENTASTVISITOR_H
