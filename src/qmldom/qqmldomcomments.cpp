// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
CommentInfo::CommentInfo(QStringView rawComment) : rawComment(rawComment)
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
bool Comment::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
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

bool CommentedElement::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvWrapField(visitor, Fields::preComments, preComments);
    cont = cont && self.dvWrapField(visitor, Fields::postComments, postComments);
    return cont;
}

void CommentedElement::writePre(OutWriter &lw, QList<SourceLocation> *locs) const
{
    if (locs)
        locs->resize(preComments.size());
    int i = 0;
    for (const Comment &c : preComments)
        c.write(lw, (locs ? &((*locs)[i++]) : nullptr));
}

void CommentedElement::writePost(OutWriter &lw, QList<SourceLocation> *locs) const
{
    if (locs)
        locs->resize(postComments.size());
    int i = 0;
    for (const Comment &c : postComments)
        c.write(lw, (locs ? &((*locs)[i++]) : nullptr));
}

/*!
\brief Given the SourceLocation of the current element returns the comments associated with the
start and end of item

The map uses an index that is based on 2*the location. Thus for every location l it is possible
to have two indexes: 2*l (just before) and 2*l+1 (just after).
This allows to attach comments to indexes representing either just before or after any location
*/
QMultiMap<quint32, const QList<Comment> *>
CommentedElement::commentGroups(SourceLocation elLocation) const
{
    return QMultiMap<quint32, const QList<Comment> *>(
            { { elLocation.begin() * 2, &preComments },
              { elLocation.end() * 2 + 1, &postComments } });
}

using namespace QQmlJS::AST;

class RegionRef
{
public:
    Path path; // store the MutableDomItem instead?
    QString regionName;
};

// internal class to keep a reference either to an AST::Node* or a region of a DomItem and the
// size of that region
class ElementRef
{
public:
    ElementRef(AST::Node *node, quint32 size) : element(node), size(size) { }
    ElementRef(Path path, QString region, quint32 size)
        : element(RegionRef { path, region }), size(size)
    {
    }
    operator bool() const
    {
        return (element.index() == 0 && std::get<0>(element)) || element.index() == 1 || size != 0;
    }
    ElementRef() = default;

    std::variant<AST::Node *, RegionRef> element;
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
    void addItemRanges(DomItem item, FileLocations::Tree itemLocations, Path currentP);

    void throwRecursionDepthError() override { }

    static const QSet<int> kindsToSkip();

    bool preVisit(Node *n) override
    {
        if (!kindsToSkip().contains(n->kind)) {
            quint32 start = n->firstSourceLocation().begin();
            quint32 end = n->lastSourceLocation().end();
            if (!starts.contains(start))
                starts.insert(start, { n, end - start });
            if (!ends.contains(end))
                ends.insert(end, { n, end - start });
        }
        return true;
    }

    QQmlJS::Engine *engine;
    FileLocations::Tree rootItemLocations;
    QMap<quint32, ElementRef> starts;
    QMap<quint32, ElementRef> ends;
};

void AstRangesVisitor::addNodeRanges(AST::Node *rootNode)
{
    AST::Node::accept(rootNode, this);
}

void AstRangesVisitor::addItemRanges(DomItem item, FileLocations::Tree itemLocations, Path currentP)
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
            if (!starts.contains(startI))
                starts.insert(startI, { currentP, it.key(), quint32(endI - startI) });
            if (!ends.contains(endI))
                ends.insert(endI, { currentP, it.key(), endI - startI });
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

/*!
\class QQmlJS::Dom::AstComments
\brief Stores the comments associated with javascript AST::Node pointers
*/

bool AstComments::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = self.dvItemField(visitor, Fields::commentedElements, [this, &self]() {
        return self.subMapItem(Map(
                self.pathFromOwner().field(Fields::commentedElements),
                [this](DomItem &map, QString key) {
                    bool ok;
                    // we expose the comments as map just for debugging purposes,
                    // as key we use the address hex value as key (keys must be strings)
                    quintptr v = key.split(QLatin1Char('_')).last().toULong(&ok, 16);
                    // recover the actual key, and check if it is in the map
                    AST::Node *n = reinterpret_cast<AST::Node *>(v);
                    if (ok && m_commentedElements.contains(n))
                        return map.wrap(PathEls::Key(key), m_commentedElements[n]);
                    return DomItem();
                },
                [this](DomItem &) {
                    QSet<QString> res;
                    for (AST::Node *n : m_commentedElements.keys()) {
                        QString name;
                        if (n)
                            name = QString::number(n->kind); // we should add mapping to
                                                             // string for this
                        res.insert(name + QStringLiteral(u"_") + QString::number(quintptr(n), 16));
                    }
                    return res;
                },
                QLatin1String("CommentedElements")));
    });
    return cont;
}

void AstComments::collectComments(MutableDomItem &item)
{
    if (std::shared_ptr<ScriptExpression> scriptPtr = item.ownerAs<ScriptExpression>()) {
        DomItem itemItem = item.item();
        return collectComments(scriptPtr->engine(), scriptPtr->ast(), scriptPtr->astComments(),
                               item, FileLocations::treeOf(itemItem));
    } else if (std::shared_ptr<QmlFile> qmlFilePtr = item.ownerAs<QmlFile>()) {
        return collectComments(qmlFilePtr->engine(), qmlFilePtr->ast(), qmlFilePtr->astComments(),
                               item, qmlFilePtr->fileLocationsTree());
    } else {
        qCWarning(commentsLog)
                << "collectComments works with QmlFile and ScriptExpression, not with"
                << item.internalKindStr();
    }
}

/*!
\brief
Collects and associates comments with javascript AST::Node pointers and MutableDomItem in
rootItem
*/
void AstComments::collectComments(std::shared_ptr<Engine> engine, AST::Node *n,
                                  std::shared_ptr<AstComments> ccomm, MutableDomItem rootItem,
                                  FileLocations::Tree rootItemLocations)
{
    if (!n)
        return;
    AstRangesVisitor ranges;
    ranges.addItemRanges(rootItem.item(), rootItemLocations, Path());
    ranges.addNodeRanges(n);
    QStringView code = engine->code();
    QHash<AST::Node *, CommentedElement> &commentedElements = ccomm->m_commentedElements;
    quint32 lastPostCommentPostEnd = 0;
    for (SourceLocation cLoc : engine->comments()) {
        // collect whitespace before and after cLoc -> iPre..iPost contains whitespace,
        // do not add newline before, but add the one after
        quint32 iPre = cLoc.begin();
        int preNewline = 0;
        int postNewline = 0;
        QStringView commentStartStr;
        while (iPre > 0) {
            QChar c = code.at(iPre - 1);
            if (!c.isSpace()) {
                if (commentStartStr.isEmpty() && (c == QLatin1Char('*') || c == QLatin1Char('/'))
                    && iPre - 1 > 0 && code.at(iPre - 2) == QLatin1Char('/')) {
                    commentStartStr = code.mid(iPre - 2, 2);
                    --iPre;
                } else {
                    break;
                }
            } else if (c == QLatin1Char('\n') || c == QLatin1Char('\r')) {
                preNewline = 1;
                // possibly add an empty line if it was there (but never more than one)
                int i = iPre - 1;
                if (c == QLatin1Char('\n') && i > 0 && code.at(i - 1) == QLatin1Char('\r'))
                    --i;
                while (i > 0 && code.at(--i).isSpace()) {
                    c = code.at(i);
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

        qsizetype iPost = cLoc.end();
        while (iPost < code.size()) {
            QChar c = code.at(iPost);
            if (!c.isSpace()) {
                if (!commentStartStr.isEmpty() && commentStartStr.at(1) == QLatin1Char('*')
                    && c == QLatin1Char('*') && iPost + 1 < code.size()
                    && code.at(iPost + 1) == QLatin1Char('/')) {
                    commentStartStr = QStringView();
                    ++iPost;
                } else {
                    break;
                }
            } else {
                if (c == QLatin1Char('\n')) {
                    ++postNewline;
                    if (iPost + 1 < code.size() && code.at(iPost + 1) == QLatin1Char('\n')) {
                        ++iPost;
                        ++postNewline;
                    }
                } else if (c == QLatin1Char('\r')) {
                    if (iPost + 1 < code.size() && code.at(iPost + 1) == QLatin1Char('\n')) {
                        ++iPost;
                        ++postNewline;
                    }
                }
            }
            ++iPost;
            if (postNewline > 1)
                break;
        }

        ElementRef commentEl;
        bool pre = true;
        auto iStart = ranges.starts.lowerBound(cLoc.begin());
        auto iEnd = ranges.ends.lowerBound(cLoc.begin());
        Q_ASSERT(!ranges.ends.isEmpty() && !ranges.starts.isEmpty());

        auto checkElementBefore = [&]() {
            if (commentEl)
                return;
            // prefer post comment attached to preceding element
            auto preEnd = iEnd;
            auto preStart = iStart;
            if (preEnd != ranges.ends.begin()) {
                --preEnd;
                if (iStart == ranges.starts.begin() || (--preStart).key() < preEnd.key()) {
                    // iStart == begin should never happen
                    // check that we do not have operators (or in general other things) between
                    // preEnd and this because inserting a newline too ealy might invalidate the
                    // expression (think a + //comment\n b  ==> a // comment\n + b), in this
                    // case attaching as preComment of iStart (b in the example) should be
                    // preferred as it is safe
                    quint32 i = iPre;
                    while (i != 0 && code.at(--i).isSpace())
                        ;
                    if (i <= preEnd.key() || i < lastPostCommentPostEnd
                        || iEnd == ranges.ends.end()) {
                        commentEl = preEnd.value();
                        pre = false;
                        lastPostCommentPostEnd = iPost + 1; // ensure the previous check works
                        // with multiple post comments
                    }
                }
            }
        };
        auto checkElementAfter = [&]() {
            if (commentEl)
                return;
            if (iStart != ranges.starts.end()) {
                // try to add a pre comment of following element
                if (iEnd == ranges.ends.end() || iEnd.key() > iStart.key()) {
                    // there is no end of element before iStart begins
                    // associate the comment as preComment of iStart
                    // (btw iEnd == end should never happen here)
                    commentEl = iStart.value();
                    return;
                }
            }
            if (iStart == ranges.starts.begin()) {
                Q_ASSERT(iStart != ranges.starts.end());
                // we are before the first node (should be handled already by previous case)
                commentEl = iStart.value();
            }
        };
        auto checkInsideEl = [&]() {
            if (commentEl)
                return;
            auto preIStart = iStart;
            if (iStart == ranges.starts.begin()) {
                commentEl = iStart.value(); // checkElementAfter should have handled this
                return;
            } else {
                --preIStart;
            }
            // we are inside a node, actually inside both n1 and n2 (which might be the same)
            // add to pre of the smallest between n1 and n2.
            // This is needed because if there are multiple nodes starting/ending at the same
            // place we store only the first (i.e. largest)
            ElementRef n1 = preIStart.value();
            ElementRef n2 = iEnd.value();
            if (n1.size > n2.size)
                commentEl = n2;
            else
                commentEl = n1;
        };
        if (!preNewline) {
            checkElementBefore();
            checkElementAfter();
        } else {
            checkElementAfter();
            checkElementBefore();
        }
        if (!commentEl)
            checkInsideEl();
        if (!commentEl) {
            qCWarning(commentsLog) << "Could not assign comment at" << locationToData(cLoc)
                                   << "adding before root node";
            if (rootItem && (rootItemLocations || !n)) {
                commentEl.element = RegionRef { Path(), QString() };
                commentEl.size =
                        rootItemLocations->info()
                                .regions.value(QString(), rootItemLocations->info().fullRegion)
                                .length;
                // attach to rootItem
            } else if (n) {
                commentEl.element = n;
                commentEl.size = n->lastSourceLocation().end() - n->firstSourceLocation().begin();
            }
        }

        Comment comment(code.mid(iPre, iPost - iPre), preNewline);
        if (commentEl.element.index() == 0 && std::get<0>(commentEl.element)) {
            CommentedElement &cEl = commentedElements[std::get<0>(commentEl.element)];
            if (pre)
                cEl.preComments.append(comment);
            else
                cEl.postComments.append(comment);
        } else if (commentEl.element.index() == 1) {
            DomItem rComments = rootItem.item()
                                        .path(std::get<1>(commentEl.element).path)
                                        .field(Fields::comments);
            if (RegionComments *rCommentsPtr = rComments.mutableAs<RegionComments>()) {
                if (pre)
                    rCommentsPtr->addPreComment(comment, std::get<1>(commentEl.element).regionName);
                else
                    rCommentsPtr->addPostComment(comment,
                                                 std::get<1>(commentEl.element).regionName);
            } else {
                Q_ASSERT(false);
            }
        } else {
            qCWarning(commentsLog)
                    << "Failed: no item or node to attach comment" << comment.rawComment();
        }
    }
}

// internal class to collect all comments in a node or its subnodes
class CommentCollectorVisitor : protected VisitAll
{
public:
    CommentCollectorVisitor(AstComments *comments, AST::Node *n) : comments(comments)
    {
        AST::Node::accept(n, this);
    }

    void throwRecursionDepthError() override { }

    bool preVisit(Node *n) override
    {
        auto &cEls = comments->commentedElements();
        if (cEls.contains(n))
            nodeComments += cEls[n].commentGroups(
                    combine(n->firstSourceLocation(), n->lastSourceLocation()));
        return true;
    }

    AstComments *comments;
    QMultiMap<quint32, const QList<Comment> *> nodeComments;
};

/*!
\brief low level method returns all comments in a node (including its subnodes)

The comments are roughly ordered in the order they appear in the file.
Multiple values are in reverse order if the index is even.
*/
QMultiMap<quint32, const QList<Comment> *> AstComments::allCommentsInNode(AST::Node *n)
{
    CommentCollectorVisitor v(this, n);
    return v.nodeComments;
}

bool RegionComments::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    if (!regionComments.isEmpty())
        cont = cont && self.dvWrapField(visitor, Fields::regionComments, regionComments);
    return cont;
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
