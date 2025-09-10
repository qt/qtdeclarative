// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomcomments_p.h"
#include "qqmldomoutwriter_p.h"
#include "qqmldomlinewriter_p.h"
#include "qqmldomelements_p.h"
#include "qqmldomexternalitems_p.h"
#include "qqmldomastdumper_p.h"
#include "qqmldomattachedinfo_p.h"

#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljslexer_p.h>

#include <QtCore/QSet>

#include <variant>

static Q_LOGGING_CATEGORY(commentsLog, "qt.qmldom.comments", QtWarningMsg);

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

/*!
\internal
\class QQmlJS::Dom::AstComments

\brief Associates comments with AST::Node *

Comments are associated to the largest closest node with the
following algorithm:
\list
\li comments of a node can either be preComments or postComments (before
or after the element)
\li define start and end for each element, if two elements start (or end)
  at the same place the first (larger) wins.
\li associate the comments either with the element just before or
just after unless the comments is *inside* an element (meaning that
going back there is a start before finding an end, or going forward an
end is met before a start).
\li to choose between the element before or after, we look at the start
of the comment, if it is on a new line then associating it as
preComment to the element after is preferred, otherwise post comment
of the previous element (inline element).
This is the only space dependent choice, making comment assignment
quite robust
\li if the comment is intrinsically inside all elements then it is moved
to before the smallest element.
This is the largest reorganization performed, and it is still quite
small and difficult to trigger.
\li the comments are stored with the whitespace surrounding them, from
the preceding newline (and recording if a newline is required before
it) until the newline after.
This allows a better reproduction of the comments.
\endlist
*/
/*!
\class QQmlJS::Dom::CommentInfo

\brief Extracts various pieces and information out of a rawComment string

Comments store a string (rawComment) with comment characters (//,..) and spaces.
Sometime one wants just the comment, the commentcharacters, the space before the comment,....
CommentInfo gets such a raw comment string and makes the various pieces available
*/
CommentInfo::CommentInfo(QStringView rawComment, QQmlJS::SourceLocation loc)
    : rawComment(rawComment), commentLocation(loc)
{
    commentBegin = 0;
    while (commentBegin < quint32(rawComment.size()) && rawComment.at(commentBegin).isSpace()) {
        if (rawComment.at(commentBegin) == QLatin1Char('\n'))
            hasStartNewline = true;
        ++commentBegin;
    }
    if (commentBegin < quint32(rawComment.size())) {
        QString expectedEnd;
        switch (rawComment.at(commentBegin).unicode()) {
        case '/':
            commentStartStr = rawComment.mid(commentBegin, 2);
            if (commentStartStr == u"/*") {
                expectedEnd = QStringLiteral(u"*/");
            } else {
                if (commentStartStr == u"//") {
                    expectedEnd = QStringLiteral(u"\n");
                } else {
                    warnings.append(tr("Unexpected comment start %1").arg(commentStartStr));
                }
            }
            break;
        case '#':
            commentStartStr = rawComment.mid(commentBegin, 1);
            expectedEnd = QStringLiteral(u"\n");
            break;
        default:
            commentStartStr = rawComment.mid(commentBegin, 1);
            warnings.append(tr("Unexpected comment start %1").arg(commentStartStr));
            break;
        }

        commentEnd = commentBegin + commentStartStr.size();
        quint32 rawEnd = quint32(rawComment.size());
        commentContentEnd = commentContentBegin = commentEnd;
        QChar e1 = ((expectedEnd.isEmpty()) ? QChar::fromLatin1(0) : expectedEnd.at(0));
        while (commentEnd < rawEnd) {
            QChar c = rawComment.at(commentEnd);
            if (c == e1) {
                if (expectedEnd.size() > 1) {
                    if (++commentEnd < rawEnd && rawComment.at(commentEnd) == expectedEnd.at(1)) {
                        Q_ASSERT(expectedEnd.size() == 2);
                        commentEndStr = rawComment.mid(++commentEnd - 2, 2);
                        break;
                    } else {
                        commentContentEnd = commentEnd;
                    }
                } else {
                    // Comment ends with \n, treat as it is not part of the comment but post whitespace
                    commentEndStr = rawComment.mid(commentEnd - 1, 1);
                    break;
                }
            } else if (!c.isSpace()) {
                commentContentEnd = commentEnd;
            } else if (c == QLatin1Char('\n')) {
                ++nContentNewlines;
            } else if (c == QLatin1Char('\r')) {
                if (expectedEnd == QStringLiteral(u"\n")) {
                    if (commentEnd + 1 < rawEnd
                        && rawComment.at(commentEnd + 1) == QLatin1Char('\n')) {
                        ++commentEnd;
                        commentEndStr = rawComment.mid(++commentEnd - 2, 2);
                    } else {
                        commentEndStr = rawComment.mid(++commentEnd - 1, 1);
                    }
                    break;
                } else if (commentEnd + 1 == rawEnd
                           || rawComment.at(commentEnd + 1) != QLatin1Char('\n')) {
                    ++nContentNewlines;
                }
            }
            ++commentEnd;
        }

        if (commentEnd > 0
            && (rawComment.at(commentEnd - 1) == QLatin1Char('\n')
                || rawComment.at(commentEnd - 1) == QLatin1Char('\r')))
            hasEndNewline = true;
        quint32 i = commentEnd;
        while (i < rawEnd && rawComment.at(i).isSpace()) {
            if (rawComment.at(i) == QLatin1Char('\n') || rawComment.at(i) == QLatin1Char('\r'))
                hasEndNewline = true;
            ++i;
        }
        if (i < rawEnd) {
            warnings.append(tr("Non whitespace char %1 after comment end at %2")
                                    .arg(rawComment.at(i))
                                    .arg(i));
        }
    }

    // Post process comment source location
    commentLocation.offset -= commentStartStr.size();
    commentLocation.startColumn -=  commentStartStr.size();
    commentLocation.length = commentEnd - commentBegin;
}

/*!
\class QQmlJS::Dom::Comment

\brief Represents a comment

Comments are not needed for execute the program, so they are aimed to the programmer,
and have few functions: explaining code, adding extra info/context (who did write,
when licensing,...) or  disabling code.
Being for the programmer and being non functional it is difficult to treat them properly.
So preserving them as much as possible is the best course of action.

To acheive this comment is represented by
\list
\li newlinesBefore: the number of newlines before the comment, to preserve spacing between
comments (the extraction routines limit this to 2 at most, i.e. a single empty line) \li
rawComment: a string with the actual comment including whitespace before and after and the
comment characters (whitespace before is limited to spaces/tabs to preserve indentation or
spacing just before starting the comment) \endlist The rawComment is a bit annoying if one wants
to change the comment, or extract information from it. For this reason info gives access to the
various elements of it: the comment characters #, // or /
*, the space before it, and the actual comment content.

the comments are stored with the whitespace surrounding them, from
the preceding newline (and recording if a newline is required before
it) until the newline after.

A comment has methods to write it out again (write) and expose it to the Dom
(iterateDirectSubpaths).
*/

/*!
\brief Expose attributes to the Dom
*/
bool Comment::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    cont = cont && self.dvValueField(visitor, Fields::rawComment, rawComment());
    cont = cont && self.dvValueField(visitor, Fields::newlinesBefore, newlinesBefore());
    return cont;
}

void Comment::write(OutWriter &lw, SourceLocation *commentLocation) const
{
    if (newlinesBefore())
        lw.ensureNewline(newlinesBefore());
    CommentInfo cInfo = info();
    lw.ensureSpace(cInfo.preWhitespace());
    QStringView cBody = cInfo.comment();
    PendingSourceLocationId cLoc = lw.lineWriter.startSourceLocation(commentLocation);
    lw.write(cBody.mid(0, 1));
    bool indentOn = lw.indentNextlines;
    lw.indentNextlines = false;
    lw.write(cBody.mid(1));
    lw.indentNextlines = indentOn;
    lw.lineWriter.endSourceLocation(cLoc);
    lw.write(cInfo.postWhitespace());
}

/*!
\class QQmlJS::Dom::CommentedElement
\brief Keeps the comment associated with an element

A comment can be attached to an element (that is always a range of the file with a start and
end) only in two ways: it can precede the region (preComments), or follow it (postComments).
*/

/*!
\class QQmlJS::Dom::RegionComments
\brief Keeps the comments associated with a DomItem

A DomItem can be more complex that just a start/end, it can have multiple regions, for example
a return or a function token might define a region.
The empty string is the region that represents the whole element.

Every region has a name, and should be written out using the OutWriter.writeRegion (or
startRegion/ EndRegion). Region comments keeps a mapping containing them.
*/

bool CommentedElement::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    cont = cont && self.dvWrapField(visitor, Fields::preComments, m_preComments);
    cont = cont && self.dvWrapField(visitor, Fields::postComments, m_postComments);
    return cont;
}

void CommentedElement::writePre(OutWriter &lw, QList<SourceLocation> *locs) const
{
    if (locs)
        locs->resize(m_preComments.size());
    int i = 0;
    for (const Comment &c : m_preComments)
        c.write(lw, (locs ? &((*locs)[i++]) : nullptr));
}

void CommentedElement::writePost(OutWriter &lw, QList<SourceLocation> *locs) const
{
    if (locs)
        locs->resize(m_postComments.size());
    int i = 0;
    for (const Comment &c : m_postComments)
        c.write(lw, (locs ? &((*locs)[i++]) : nullptr));
}

using namespace QQmlJS::AST;

class RegionRef
{
public:
    Path path; // store the MutableDomItem instead?
    FileLocationRegion regionName;
};

class NodeRef
{
public:
    AST::Node *node = nullptr;
    CommentAnchor commentAnchor;
};

// internal class to keep a reference either to an AST::Node* or a region of a DomItem and the
// size of that region
class ElementRef
{
public:
    ElementRef(AST::Node *node, quint32 size, CommentAnchor commentAnchor)
        : element(NodeRef{ node, commentAnchor }), size(size)
    {
    }
    ElementRef(const Path &path, FileLocationRegion region, quint32 size)
        : element(RegionRef{ path, region }), size(size)
    {
    }
    operator bool() const
    {
        return (element.index() == 0 && std::get<0>(element).node) || element.index() == 1
                || size != 0;
    }
    ElementRef() = default;

    std::variant<NodeRef, RegionRef> element;
    quint32 size = 0;
};

/*!
\class QQmlJS::Dom::VisitAll
\brief A vistor that visits all the AST:Node

The default visitor does not necessarily visit all nodes, because some part
of the AST are typically handled manually. This visitor visits *all* AST
elements contained.

Note: Subclasses should take care to call the parent (i.e. this) visit/endVisit
methods when overriding them, to guarantee that all element are really visited
*/

/*!
returns a set with all Ui* Nodes (i.e. the top level non javascript Qml)
*/
QSet<int> VisitAll::uiKinds()
{
    static QSet<int> res({ AST::Node::Kind_UiObjectMemberList, AST::Node::Kind_UiArrayMemberList,
                           AST::Node::Kind_UiParameterList,    AST::Node::Kind_UiHeaderItemList,
                           AST::Node::Kind_UiEnumMemberList,   AST::Node::Kind_UiAnnotationList,

                           AST::Node::Kind_UiArrayBinding,     AST::Node::Kind_UiImport,
                           AST::Node::Kind_UiObjectBinding,    AST::Node::Kind_UiObjectDefinition,
                           AST::Node::Kind_UiInlineComponent,  AST::Node::Kind_UiObjectInitializer,
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
                           AST::Node::Kind_UiPragmaValueList,
#endif
                           AST::Node::Kind_UiPragma,           AST::Node::Kind_UiProgram,
                           AST::Node::Kind_UiPublicMember,     AST::Node::Kind_UiQualifiedId,
                           AST::Node::Kind_UiScriptBinding,    AST::Node::Kind_UiSourceElement,
                           AST::Node::Kind_UiEnumDeclaration,  AST::Node::Kind_UiVersionSpecifier,
                           AST::Node::Kind_UiRequired,         AST::Node::Kind_UiAnnotation });
    return res;
}

// internal private class to set all the starts/ends of the nodes/regions
class AstRangesVisitor final : protected VisitAll
{
public:
    AstRangesVisitor() = default;

    void addNodeRanges(AST::Node *rootNode);
    void addItemRanges(
            const DomItem &item, const FileLocations::Tree &itemLocations, const Path &currentP);

    void throwRecursionDepthError() override { }

    static const QSet<int> kindsToSkip();
    static bool shouldSkipRegion(const DomItem &item, FileLocationRegion region);

    void addSourceLocations(Node *n, quint32 start, quint32 end, CommentAnchor commentAnchor)
    {
        if (!starts.contains(start))
            starts.insert(start, { n, end - start, commentAnchor });
        if (!ends.contains(end))
            ends.insert(end, { n, end - start, commentAnchor });
    }
    void addSourceLocations(Node *n)
    {
        addSourceLocations(n, n->firstSourceLocation().begin(), n->lastSourceLocation().end(),
                           CommentAnchor{});
    }
    void addSourceLocations(Node *n, const SourceLocation &sourceLocation)
    {
        if (!sourceLocation.isValid())
            return;
        addSourceLocations(n, sourceLocation.begin(), sourceLocation.end(),
                           CommentAnchor::from(sourceLocation));
    }

    bool preVisit(Node *n) override
    {
        if (!kindsToSkip().contains(n->kind)) {
            addSourceLocations(n);
        }
        return true;
    }

    using VisitAll::visit;
    bool visit(CaseClause *caseClause) override
    {
        // special case: case clauses can have comments attached to their `:` token
        addSourceLocations(caseClause, caseClause->colonToken);
        return true;
    }

    bool visit(ClassDeclaration *classDeclaration) override
    {
        // special case: class declarations can have comments attached to their `identifier` token
        addSourceLocations(classDeclaration, classDeclaration->identifierToken);
        addSourceLocations(classDeclaration, classDeclaration->lbraceToken);
        addSourceLocations(classDeclaration, classDeclaration->rbraceToken);
        return true;
    }

    bool visit(FormalParameterList *list) override
    {
        if (!list->commaToken.isValid())
            return true;

        // Comments are first attached to some previous element, if possible. Therefore, attach
        // comments in FormalParameterList to comments so that they don't "jump over commas" during
        // formatting.
        addSourceLocations(list, list->commaToken);
        return true;
    }

    bool visit(FunctionExpression *fExpr) override
    {
        addSourceLocations(fExpr, fExpr->identifierToken);
        addSourceLocations(fExpr, fExpr->lparenToken);
        addSourceLocations(fExpr, fExpr->rparenToken);
        addSourceLocations(fExpr, fExpr->lbraceToken);
        addSourceLocations(fExpr, fExpr->rbraceToken);
        return true;
    }

    bool visit(FunctionDeclaration *fExpr) override
    {
        return visit(static_cast<FunctionExpression *>(fExpr));
    }

    QMap<quint32, ElementRef> starts;
    QMap<quint32, ElementRef> ends;
};

void AstRangesVisitor::addNodeRanges(AST::Node *rootNode)
{
    AST::Node::accept(rootNode, this);
}

void AstRangesVisitor::addItemRanges(
        const DomItem &item, const FileLocations::Tree &itemLocations, const Path &currentP)
{
    if (!itemLocations) {
        if (item)
            qCWarning(commentsLog) << "reached item" << item.canonicalPath() << "without locations";
        return;
    }
    DomItem comments = item.field(Fields::comments);
    if (comments) {
        auto regs = itemLocations->info().regions;
        for (auto it = regs.cbegin(), end = regs.cend(); it != end; ++it) {
            quint32 startI = it.value().begin();
            quint32 endI = it.value().end();

            if (!shouldSkipRegion(item, it.key())) {
                if (!starts.contains(startI))
                    starts.insert(startI, { currentP, it.key(), quint32(endI - startI) });
                if (!ends.contains(endI))
                    ends.insert(endI, { currentP, it.key(), endI - startI });
            }
        }
    }
    {
        auto subMaps = itemLocations->subItems();
        for (auto it = subMaps.begin(), end = subMaps.end(); it != end; ++it) {
            addItemRanges(item.path(it.key()),
                          std::static_pointer_cast<AttachedInfoT<FileLocations>>(it.value()),
                          currentP.path(it.key()));
        }
    }
}

const QSet<int> AstRangesVisitor::kindsToSkip()
{
    static QSet<int> res = QSet<int>({
                                             AST::Node::Kind_ArgumentList,
                                             AST::Node::Kind_ElementList,
                                             AST::Node::Kind_FormalParameterList,
                                             AST::Node::Kind_ImportsList,
                                             AST::Node::Kind_ExportsList,
                                             AST::Node::Kind_PropertyDefinitionList,
                                             AST::Node::Kind_StatementList,
                                             AST::Node::Kind_VariableDeclarationList,
                                             AST::Node::Kind_ClassElementList,
                                             AST::Node::Kind_PatternElementList,
                                             AST::Node::Kind_PatternPropertyList,
                                             AST::Node::Kind_TypeArgument,
                                     })
                                   .unite(VisitAll::uiKinds());
    return res;
}

/*! \internal
    \brief returns true if comments should skip attaching to this region
*/
bool AstRangesVisitor::shouldSkipRegion(const DomItem &item, FileLocationRegion region)
{
    switch (item.internalKind()) {
    case DomType::EnumDecl: {
        return (region == FileLocationRegion::IdentifierRegion)
                || (region == FileLocationRegion::EnumKeywordRegion);
    }
    case DomType::EnumItem: {
        return (region == FileLocationRegion::IdentifierRegion)
                || (region == FileLocationRegion::EnumValueRegion);
    }
    case DomType::QmlObject: {
        return (region == FileLocationRegion::RightBraceRegion
                || region == FileLocationRegion::LeftBraceRegion);
    }
    case DomType::Import:
    case DomType::ImportScope:
        return region == FileLocationRegion::IdentifierRegion;
    default:
        return false;
    }
    Q_UNREACHABLE_RETURN(false);
}

class CommentLinker
{
public:
    CommentLinker(QStringView code, ElementRef &commentedElement, const AstRangesVisitor &ranges, quint32 &lastPostCommentPostEnd,
                  const SourceLocation &commentLocation)
        : m_code{ code },
          m_commentedElement{ commentedElement },
          m_lastPostCommentPostEnd{ lastPostCommentPostEnd },
          m_ranges{ ranges },
          m_commentLocation { commentLocation },
          m_startElement{ m_ranges.starts.lowerBound(commentLocation.begin()) },
          m_endElement{ m_ranges.ends.lowerBound(commentLocation.end()) },
          m_spaces{findSpacesAroundComment()}
    {
    }

    void linkCommentWithElement()
    {
        if (m_spaces.preNewline < 1) {
            checkElementBeforeComment();
            checkElementAfterComment();
        } else {
            checkElementAfterComment();
            checkElementBeforeComment();
        }
        if (!m_commentedElement)
            checkElementInside();
    }

    [[nodiscard]] Comment createComment() const
    {
        const auto [preSpacesIndex, postSpacesIndex, preNewlineCount] = m_spaces;
        return Comment{ m_code.mid(preSpacesIndex, quint32(postSpacesIndex) - preSpacesIndex),
                        m_commentLocation,
                        static_cast<int>(preNewlineCount),
                        m_commentType};
    }

private:
    struct SpaceTrace
    {
        quint32 iPre;
        qsizetype iPost;
        int preNewline;
    };

    /*! \internal
        \brief Returns a Comment data
        Comment starts from the first non-newline and non-space character preceding
        the comment start characters. For example, "\n\n  // A comment  \n\n\n", we
        hold the prenewlines count (2). PostNewlines are part of the Comment structure
        but they are not regarded while writing since they could be a part of prenewlines
        of a following comment.
    */
    [[nodiscard]] SpaceTrace findSpacesAroundComment() const
    {
        quint32 iPre = m_commentLocation.begin();
        int preNewline = 0;
        int postNewline = 0;
        QStringView commentStartStr;
        while (iPre > 0) {
            QChar c = m_code.at(iPre - 1);
            if (!c.isSpace()) {
                if (commentStartStr.isEmpty() && (c == QLatin1Char('*') || c == QLatin1Char('/'))
                    && iPre - 1 > 0 && m_code.at(iPre - 2) == QLatin1Char('/')) {
                    commentStartStr = m_code.mid(iPre - 2, 2);
                    --iPre;
                } else {
                    break;
                }
            } else if (c == QLatin1Char('\n') || c == QLatin1Char('\r')) {
                preNewline = 1;
                // possibly add an empty line if it was there (but never more than one)
                int i = iPre - 1;
                if (c == QLatin1Char('\n') && i > 0 && m_code.at(i - 1) == QLatin1Char('\r'))
                    --i;
                while (i > 0 && m_code.at(--i).isSpace()) {
                    c = m_code.at(i);
                    if (c == QLatin1Char('\n') || c == QLatin1Char('\r')) {
                        ++preNewline;
                        break;
                    }
                }
                break;
            }
            --iPre;
        }
        if (iPre == 0)
            preNewline = 1;
        qsizetype iPost = m_commentLocation.end();
        while (iPost < m_code.size()) {
            QChar c = m_code.at(iPost);
            if (!c.isSpace()) {
                if (!commentStartStr.isEmpty() && commentStartStr.at(1) == QLatin1Char('*')
                    && c == QLatin1Char('*') && iPost + 1 < m_code.size()
                    && m_code.at(iPost + 1) == QLatin1Char('/')) {
                    commentStartStr = QStringView();
                    ++iPost;
                } else {
                    break;
                }
            } else {
                if (c == QLatin1Char('\n')) {
                    ++postNewline;
                    if (iPost + 1 < m_code.size() && m_code.at(iPost + 1) == QLatin1Char('\n')) {
                        ++iPost;
                        ++postNewline;
                    }
                } else if (c == QLatin1Char('\r')) {
                    if (iPost + 1 < m_code.size() && m_code.at(iPost + 1) == QLatin1Char('\n')) {
                        ++iPost;
                        ++postNewline;
                    }
                }
            }
            ++iPost;
            if (postNewline > 1)
                break;
        }

        return {iPre, iPost, preNewline};
    }

    // tries to associate comment as a postComment to currentElement
    void checkElementBeforeComment()
    {
        if (m_commentedElement)
            return;
        // prefer post comment attached to preceding element
        auto preEnd = m_endElement;
        auto preStart = m_startElement;
        if (preEnd != m_ranges.ends.begin()) {
            --preEnd;
            if (m_startElement == m_ranges.starts.begin() || (--preStart).key() < preEnd.key()) {
                // iStart == begin should never happen
                // check that we do not have operators (or in general other things) between
                // preEnd and this because inserting a newline too ealy might invalidate the
                // expression (think a + //comment\n b  ==> a // comment\n + b), in this
                // case attaching as preComment of iStart (b in the example) should be
                // preferred as it is safe
                quint32 i = m_spaces.iPre;
                while (i != 0 && m_code.at(--i).isSpace())
                    ;
                if (i <= preEnd.key() || i < m_lastPostCommentPostEnd
                    || m_endElement == m_ranges.ends.end()) {
                    m_commentedElement = preEnd.value();
                    m_commentType = Comment::Post;
                    m_lastPostCommentPostEnd = m_spaces.iPost + 1; // ensure the previous check works
                    // with multiple post comments
                }
            }
        }
    }
    // tries to associate comment as a preComment to currentElement
    void checkElementAfterComment()
    {
        if (m_commentedElement)
            return;
        if (m_startElement != m_ranges.starts.end()) {
            // try to add a pre comment of following element
            if (m_endElement == m_ranges.ends.end() || m_endElement.key() > m_startElement.key()) {
                // there is no end of element before iStart begins
                // associate the comment as preComment of iStart
                // (btw iEnd == end should never happen here)
                m_commentedElement = m_startElement.value();
                return;
            }
        }
        if (m_startElement == m_ranges.starts.begin()) {
            Q_ASSERT(m_startElement != m_ranges.starts.end());
            // we are before the first node (should be handled already by previous case)
            m_commentedElement = m_startElement.value();
        }
    }
    void checkElementInside()
    {
        if (m_commentedElement)
            return;
        auto preStart = m_startElement;
        if (m_startElement == m_ranges.starts.begin()) {
            m_commentedElement = m_startElement.value(); // checkElementAfter should have handled this
            return;
        } else {
            --preStart;
        }
        if (m_endElement == m_ranges.ends.end()) {
            m_commentedElement = preStart.value();
            return;
        }
        // we are inside a node, actually inside both n1 and n2 (which might be the same)
        // add to pre of the smallest between n1 and n2.
        // This is needed because if there are multiple nodes starting/ending at the same
        // place we store only the first (i.e. largest)
        ElementRef n1 = preStart.value();
        ElementRef n2 = m_endElement.value();
        if (n1.size > n2.size)
            m_commentedElement = n2;
        else
            m_commentedElement = n1;
    }
private:
    QStringView m_code;
    ElementRef &m_commentedElement;
    quint32 &m_lastPostCommentPostEnd;
    Comment::CommentType m_commentType = Comment::Pre;
    const AstRangesVisitor &m_ranges;
    const SourceLocation &m_commentLocation;

    using RangesIterator = decltype(m_ranges.starts.begin());
    const RangesIterator m_startElement;
    const RangesIterator m_endElement;
    SpaceTrace m_spaces;
};

/*!
\class QQmlJS::Dom::AstComments
\brief Stores the comments associated with javascript AST::Node pointers
*/
bool AstComments::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    // TODO: QTBUG-123645
    // Revert this commit to reproduce crash with tst_qmldomitem::doNotCrashAtAstComments
    auto [pre, post] = collectPreAndPostComments();

    if (!pre.isEmpty())
        self.dvWrapField(visitor, Fields::preComments, pre);
    if (!post.isEmpty())
        self.dvWrapField(visitor, Fields::postComments, post);

    return false;
}

CommentCollector::CommentCollector(MutableDomItem item)
    : m_rootItem{ std::move(item) },
      m_fileLocations{ FileLocations::treeOf(m_rootItem.item()) }
{
}

void CommentCollector::collectComments()
{
    if (std::shared_ptr<ScriptExpression> scriptPtr = m_rootItem.ownerAs<ScriptExpression>()) {
        return collectComments(scriptPtr->engine(), scriptPtr->ast(), scriptPtr->astComments());
    } else if (std::shared_ptr<QmlFile> qmlFilePtr = m_rootItem.ownerAs<QmlFile>()) {
        return collectComments(qmlFilePtr->engine(), qmlFilePtr->ast(), qmlFilePtr->astComments());
    } else {
        qCWarning(commentsLog)
                << "collectComments works with QmlFile and ScriptExpression, not with"
                << m_rootItem.item().internalKindStr();
    }
}

/*! \internal
    \brief Collects and associates comments with javascript AST::Node pointers
            or with MutableDomItem
*/
void CommentCollector::collectComments(
         const std::shared_ptr<Engine> &engine, AST::Node *rootNode,
         const std::shared_ptr<AstComments> &astComments)
{
    if (!rootNode)
        return;
    AstRangesVisitor ranges;
    ranges.addItemRanges(m_rootItem.item(), m_fileLocations, Path());
    ranges.addNodeRanges(rootNode);
    QStringView code = engine->code();
    quint32 lastPostCommentPostEnd = 0;
    for (const SourceLocation &commentLocation : engine->comments()) {
        // collect whitespace before and after cLoc -> iPre..iPost contains whitespace,
        // do not add newline before, but add the one after
        ElementRef elementToBeLinked;
        CommentLinker linker(code, elementToBeLinked, ranges, lastPostCommentPostEnd, commentLocation);
        linker.linkCommentWithElement();
        const auto comment = linker.createComment();

        if (!elementToBeLinked) {
            qCWarning(commentsLog) << "Could not assign comment at" << sourceLocationToQCborValue(commentLocation)
                                   << "adding before root node";
            if (m_rootItem && (m_fileLocations || !rootNode)) {
                elementToBeLinked.element = RegionRef{ Path(), MainRegion };
                elementToBeLinked.size = FileLocations::region(m_fileLocations, MainRegion).length;
            } else if (rootNode) {
                elementToBeLinked.element = NodeRef{ rootNode, CommentAnchor{} };
                elementToBeLinked.size = rootNode->lastSourceLocation().end() - rootNode->firstSourceLocation().begin();
            }
        }

        if (const auto *const commentNode = std::get_if<NodeRef>(&elementToBeLinked.element)) {
            auto commentedElement = astComments->ensureCommentForNode(commentNode->node,
                                                                      commentNode->commentAnchor);
            commentedElement->addComment(comment);
        } else if (const auto *const regionRef =
                           std::get_if<RegionRef>(&elementToBeLinked.element)) {
            DomItem currentItem = m_rootItem.item().path(regionRef->path);
            MutableDomItem regionComments = currentItem.field(Fields::comments);
            if (auto *regionCommentsPtr = regionComments.mutableAs<RegionComments>()) {
                const Path commentPath = regionCommentsPtr->addComment(comment, regionRef->regionName);

                // update file locations with the comment region
                const auto base = FileLocations::treeOf(currentItem);
                const auto fileLocations =
                        FileLocations::ensure(base, Path::Field(Fields::comments).path(commentPath),
                                              AttachedInfo::PathType::Relative);

                FileLocations::addRegion(fileLocations, MainRegion,
                                         comment.info().sourceLocation());
            } else
                Q_ASSERT(false && "Cannot attach to region comments");
        } else {
            qCWarning(commentsLog)
                    << "Failed: no item or node to attach comment" << comment.rawComment();
        }
    }
}

bool RegionComments::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    if (!m_regionComments.isEmpty()) {
        cont = cont
                && self.dvItemField(visitor, Fields::regionComments, [this, &self]() -> DomItem {
                       const Path pathFromOwner =
                               self.pathFromOwner().field(Fields::regionComments);
                       auto map = Map::fromFileRegionMap(pathFromOwner, m_regionComments);
                       return self.subMapItem(map);
                   });
    }
    return cont;
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
