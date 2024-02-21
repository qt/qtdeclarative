// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMCOMMENTS_P_H
#define QQMLDOMCOMMENTS_P_H

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

#include "qqmldom_fwd_p.h"
#include "qqmldomconstants_p.h"
#include "qqmldomfunctionref_p.h"
#include "qqmldomitem_p.h"
#include "qqmldomattachedinfo_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsengine_p.h>

#include <QtCore/QMultiMap>
#include <QtCore/QHash>
#include <QtCore/QStack>
#include <QtCore/QCoreApplication>

#include <memory>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT CommentInfo
{
    Q_DECLARE_TR_FUNCTIONS(CommentInfo)
public:
    CommentInfo(QStringView);

    QStringView preWhitespace() const { return rawComment.mid(0, commentBegin); }

    QStringView comment() const { return rawComment.mid(commentBegin, commentEnd - commentBegin); }

    QStringView commentContent() const
    {
        return rawComment.mid(commentContentBegin, commentContentEnd - commentContentEnd);
    }

    QStringView postWhitespace() const
    {
        return rawComment.mid(commentEnd, rawComment.size() - commentEnd);
    }

    quint32 commentBegin = 0;
    quint32 commentEnd = 0;
    quint32 commentContentBegin = 0;
    quint32 commentContentEnd = 0;
    QStringView commentStartStr;
    QStringView commentEndStr;
    bool hasStartNewline = false;
    bool hasEndNewline = false;
    int nContentNewlines = 0;
    QStringView rawComment;
    QStringList warnings;
};

class QMLDOM_EXPORT Comment
{
public:
    constexpr static DomType kindValue = DomType::Comment;
    DomType kind() const { return kindValue; }

    Comment(QString c, int newlinesBefore = 1)
        : m_commentStr(c), m_comment(m_commentStr), m_newlinesBefore(newlinesBefore)
    {
    }
    Comment(QStringView c, int newlinesBefore = 1) : m_comment(c), m_newlinesBefore(newlinesBefore)
    {
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const;
    int newlinesBefore() const { return m_newlinesBefore; }
    void setNewlinesBefore(int n) { m_newlinesBefore = n; }
    QStringView rawComment() const { return m_comment; }
    CommentInfo info() const { return CommentInfo(m_comment); }
    void write(OutWriter &lw, SourceLocation *commentLocation = nullptr) const;

    friend bool operator==(const Comment &c1, const Comment &c2)
    {
        return c1.m_newlinesBefore == c2.m_newlinesBefore && c1.m_comment == c2.m_comment;
    }
    friend bool operator!=(const Comment &c1, const Comment &c2) { return !(c1 == c2); }

private:
    QString m_commentStr;
    QStringView m_comment;
    int m_newlinesBefore;
};

class QMLDOM_EXPORT CommentedElement
{
public:
    constexpr static DomType kindValue = DomType::CommentedElement;
    DomType kind() const { return kindValue; }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const;
    void writePre(OutWriter &lw, QList<SourceLocation> *locations = nullptr) const;
    void writePost(OutWriter &lw, QList<SourceLocation> *locations = nullptr) const;
    QMultiMap<quint32, const QList<Comment> *> commentGroups(SourceLocation elLocation) const;

    friend bool operator==(const CommentedElement &c1, const CommentedElement &c2)
    {
        return c1.preComments == c2.preComments && c1.postComments == c2.postComments;
    }
    friend bool operator!=(const CommentedElement &c1, const CommentedElement &c2)
    {
        return !(c1 == c2);
    }

    QList<Comment> preComments;
    QList<Comment> postComments;
};

class QMLDOM_EXPORT RegionComments
{
public:
    constexpr static DomType kindValue = DomType::RegionComments;
    DomType kind() const { return kindValue; }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const;

    friend bool operator==(const RegionComments &c1, const RegionComments &c2)
    {
        return c1.regionComments == c2.regionComments;
    }
    friend bool operator!=(const RegionComments &c1, const RegionComments &c2)
    {
        return !(c1 == c2);
    }

    Path addPreComment(const Comment &comment, FileLocationRegion region)
    {
        auto &preList = regionComments[region].preComments;
        index_type idx = preList.size();
        preList.append(comment);
        return Path::Field(Fields::regionComments)
                .key(fileLocationRegionName(region))
                .field(Fields::preComments)
                .index(idx);
    }

    Path addPostComment(const Comment &comment, FileLocationRegion region)
    {
        auto &postList = regionComments[region].postComments;
        index_type idx = postList.size();
        postList.append(comment);
        return Path::Field(Fields::regionComments)
                .key(fileLocationRegionName(region))
                .field(Fields::postComments)
                .index(idx);
    }

    QMap<FileLocationRegion, CommentedElement> regionComments;
};

class QMLDOM_EXPORT AstComments final : public OwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override
    {
        return std::make_shared<AstComments>(*this);
    }

public:
    constexpr static DomType kindValue = DomType::AstComments;
    DomType kind() const override { return kindValue; }
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor)  const override;
    std::shared_ptr<AstComments> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<AstComments>(doCopy(self));
    }

    Path canonicalPath(const DomItem &self) const override { return self.m_ownerPath; }
    static void collectComments(MutableDomItem &item);
    static void collectComments(
            std::shared_ptr<Engine> engine, AST::Node *n,
            std::shared_ptr<AstComments> collectComments, const MutableDomItem &rootItem,
            FileLocations::Tree rootItemLocations);
    AstComments(std::shared_ptr<Engine> e) : m_engine(e) { }
    AstComments(const AstComments &o)
        : OwningItem(o), m_engine(o.m_engine), m_commentedElements(o.m_commentedElements)
    {
    }

    const QHash<AST::Node *, CommentedElement> &commentedElements() const
    {
        return m_commentedElements;
    }
    CommentedElement *commentForNode(AST::Node *n)
    {
        if (m_commentedElements.contains(n))
            return &(m_commentedElements[n]);
        return nullptr;
    }
    QMultiMap<quint32, const QList<Comment> *> allCommentsInNode(AST::Node *n);

private:
    std::shared_ptr<Engine> m_engine;
    QHash<AST::Node *, CommentedElement> m_commentedElements;
};

class VisitAll : public AST::Visitor
{
public:
    VisitAll() = default;

    static QSet<int> uiKinds();

    void throwRecursionDepthError() override { }

    bool visit(AST::UiPublicMember *el) override
    {
        AST::Node::accept(el->annotations, this);
        AST::Node::accept(el->memberType, this);
        return true;
    }

    bool visit(AST::UiSourceElement *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    bool visit(AST::UiObjectDefinition *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    bool visit(AST::UiObjectBinding *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    bool visit(AST::UiScriptBinding *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    bool visit(AST::UiArrayBinding *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    bool visit(AST::UiParameterList *el) override
    {
        AST::Node::accept(el->type, this);
        return true;
    }

    bool visit(AST::UiQualifiedId *el) override
    {
        AST::Node::accept(el->next, this);
        return true;
    }

    bool visit(AST::UiEnumDeclaration *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    bool visit(AST::UiInlineComponent *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    void endVisit(AST::UiImport *el) override { AST::Node::accept(el->version, this); }
    void endVisit(AST::UiPublicMember *el) override { AST::Node::accept(el->parameters, this); }

    void endVisit(AST::UiParameterList *el) override
    {
        AST::Node::accept(el->next, this); // put other args at the same level as this one...
    }

    void endVisit(AST::UiEnumMemberList *el) override
    {
        AST::Node::accept(el->next,
                          this); // put other enum members at the same level as this one...
    }

    bool visit(AST::TemplateLiteral *el) override
    {
        AST::Node::accept(el->expression, this);
        return true;
    }

    void endVisit(AST::Elision *el) override
    {
        AST::Node::accept(el->next, this); // emit other elisions at the same level
    }
};
} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif // QQMLDOMCOMMENTS_P_H
