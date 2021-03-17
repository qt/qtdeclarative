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
    Comment(const QQmlJS::Engine *engine, Location location, QList<QQmlJS::SourceLocation> srcLocations)
        : m_location(location), m_srcLocations(srcLocations) {
        for (const auto& srcLoc : srcLocations) {
            m_text += engine->code().mid(static_cast<int>(srcLoc.begin()),
                                         static_cast<int>(srcLoc.end() - srcLoc.begin())) + "\n";
        }

        m_text.chop(1);
    }

    QList<QQmlJS::SourceLocation> m_srcLocations;

    bool hasSheBang() const { return !m_srcLocations.isEmpty() && m_srcLocations.first().begin() == 0; }
    bool isValid() const { return !m_srcLocations.isEmpty(); }
    bool isMultiline() const { return m_text.contains("\n"); }
    bool isSyntheticMultiline() const { return m_srcLocations.size() > 1; }

    bool contains(const QQmlJS::SourceLocation& location) const {
        for (const QQmlJS::SourceLocation& srcLoc : m_srcLocations) {
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

namespace AST = QQmlJS::AST;
class CommentAstVisitor : protected QQmlJS::AST::Visitor
{
public:
    CommentAstVisitor(QQmlJS::Engine *engine, AST::Node *rootNode);

    void throwRecursionDepthError() override {}

    const QHash<AST::Node *, Comment> attachedComments() const { return m_attachedComments; }
    const QHash<quint32, Comment> listComments() const { return m_listItemComments; }
    const QHash<AST::Node *, QVector<Comment>> orphanComments() const { return m_orphanComments; }

    bool visit(AST::UiScriptBinding *node) override;
    bool visit(AST::UiObjectBinding *node) override;

    bool visit(AST::UiArrayBinding *node) override;
    void endVisit(AST::UiArrayBinding *node) override;

    bool visit(AST::UiObjectDefinition *node) override;
    void endVisit(AST::UiObjectDefinition *) override;

    bool visit(AST::UiEnumDeclaration *node) override;
    void endVisit(AST::UiEnumDeclaration *node) override;

    bool visit(AST::UiEnumMemberList *node) override;

    bool visit(AST::StatementList *node) override;
    void endVisit(AST::StatementList *node) override;

    bool visit(AST::UiImport *node) override;
    bool visit(AST::UiPragma *node) override;
    bool visit(AST::UiPublicMember *node) override;
    bool visit(AST::FunctionDeclaration *node) override;
private:
    bool isCommentAttached(const QQmlJS::SourceLocation& location) const;

    QList<QQmlJS::SourceLocation> findCommentsInLine(quint32 line, bool includePrevious = false) const;

    Comment findComment(QQmlJS::SourceLocation first, QQmlJS::SourceLocation last,
                        int locations = Comment::DefaultLocations) const;

    Comment findComment(AST::Node *node, int locations = Comment::DefaultLocations) const;
    QVector<Comment> findOrphanComments(AST::Node *node) const;
    void attachComment(AST::Node *node, int locations = Comment::DefaultLocations);

    QQmlJS::Engine *m_engine;
    QHash<AST::Node *, Comment> m_attachedComments;
    QHash<quint32, Comment> m_listItemComments;
    QHash<AST::Node *, QVector<Comment>> m_orphanComments;
};

#endif // COMMENTASTVISITOR_H
