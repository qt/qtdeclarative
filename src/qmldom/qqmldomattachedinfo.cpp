// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqmldom_fwd_p.h"
#include "qqmldomlinewriter_p.h"
#include "qqmldomelements_p.h"
#include "qqmldompath_p.h"

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

using namespace Qt::StringLiterals;


/*!
\internal
\class QQmlJS::Dom::FileLocations
\brief Represents and maintains a mapping between elements and their location in a file

The location information is attached to the element it refers to via AttachedInfo
There are static methods to simplify the handling of the tree of AttachedInfo.

Attributes:
\list
\li fullRegion: A location guaranteed to include this element, its comments, and all its sub elements
\li regions: a map with locations of regions of this element, the empty string is the default region
    of this element
\li preCommentLocations: locations of the comments before this element
\li postCommentLocations: locations of the comments after this element
\endlist

\sa QQmlJs::Dom::AttachedInfo
*/
bool FileLocations::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    cont = cont && self.dvValueLazyField(visitor, Fields::fullRegion, [this]() {
        return sourceLocationToQCborValue(fullRegion);
    });
    cont = cont && self.dvItemField(visitor, Fields::regions, [this, &self]() -> DomItem {
        const Path pathFromOwner = self.pathFromOwner().field(Fields::regions);
        auto map = Map::fromFileRegionMap(pathFromOwner, regions);
        return self.subMapItem(map);
    });
    cont = cont
            && self.dvItemField(visitor, Fields::preCommentLocations, [this, &self]() -> DomItem {
                   const Path pathFromOwner =
                           self.pathFromOwner().field(Fields::preCommentLocations);
                   auto map = Map::fromFileRegionListMap(pathFromOwner, preCommentLocations);
                   return self.subMapItem(map);
               });
    cont = cont
            && self.dvItemField(visitor, Fields::postCommentLocations, [this, &self]() -> DomItem {
                   const Path pathFromOwner =
                           self.pathFromOwner().field(Fields::postCommentLocations);
                   auto map = Map::fromFileRegionListMap(pathFromOwner, postCommentLocations);
                   return self.subMapItem(map);
               });
    return cont;
}

void FileLocations::ensureCommentLocations(const QList<FileLocationRegion> &keys)
{
    for (FileLocationRegion k : keys) {
        preCommentLocations[k];
        postCommentLocations[k];
    }
}

FileLocations::Tree FileLocations::createTree(Path basePath){
    return AttachedInfoT<FileLocations>::createTree(basePath);
}

FileLocations::Tree FileLocations::ensure(FileLocations::Tree base, Path basePath, AttachedInfo::PathType pType){
    return AttachedInfoT<FileLocations>::ensure(base, basePath, pType);
}

/*!
\internal
Allows to query information about the FileLocations::Tree obtained from item, such as path of
the Tree root in the Dom, the path of this item's Tree in the Dom, and so on.

\note You can use \c{qDebug() << item.path(FileLocations::findAttachedInfo(item).foundTreePath)} or
\c{item.path(FileLocations::findAttachedInfo(item).foundTreePath).toString()} to print out the Tree
of item, for example, as Tree's cannot be printed when outside the Dom.
*/
AttachedInfoLookupResult<FileLocations::Tree>
FileLocations::findAttachedInfo(const DomItem &item)
{
    return AttachedInfoT<FileLocations>::findAttachedInfo(item, Fields::fileLocationsTree);
}

/*!
   \internal
   Returns the tree corresponding to a DomItem.
 */
FileLocations::Tree FileLocations::treeOf(const DomItem &item)
{
    return findAttachedInfo(item).foundTree;
}

/*!
   \internal
   Returns the filelocation Info corresponding to a DomItem.
 */
const FileLocations *FileLocations::fileLocationsOf(const DomItem &item)
{
    if (FileLocations::Tree t = treeOf(item))
        return &(t->info());
    return nullptr;
}

void FileLocations::updateFullLocation(const FileLocations::Tree &fLoc, SourceLocation loc)
{
    Q_ASSERT(fLoc);
    if (loc != SourceLocation()) {
        FileLocations::Tree p = fLoc;
        while (p) {
            SourceLocation &l = p->info().fullRegion;
            if (loc.begin() < l.begin() || loc.end() > l.end()) {
                l = combine(l, loc);
                p->info().regions[MainRegion] = l;
            } else {
                break;
            }
            p = p->parent();
        }
    }
}

void FileLocations::addRegion(const FileLocations::Tree &fLoc, FileLocationRegion region,
                              SourceLocation loc)
{
    Q_ASSERT(fLoc);
    fLoc->info().regions[region] = loc;
    updateFullLocation(fLoc, loc);
}

SourceLocation FileLocations::region(const FileLocations::Tree &fLoc, FileLocationRegion region)
{
    Q_ASSERT(fLoc);
    const auto &regions = fLoc->info().regions;
    if (auto it = regions.constFind(region); it != regions.constEnd() && it->isValid()) {
        return *it;
    }

    if (region == MainRegion)
        return fLoc->info().fullRegion;

    return SourceLocation{};
}

/*!
\internal
\class QQmlJS::Dom::AttachedInfo
\brief Attached info creates a tree to attach extra info to DomItems

Normally one uses the template AttachedInfoT<SpecificInfoToAttach>

static methods
Attributes:
\list
\li parent: parent AttachedInfo in tree (might be empty)
\li subItems: subItems of the tree (path -> AttachedInfo)
\li infoItem: the attached information
\endlist

\sa QQmlJs::Dom::AttachedInfo
*/

bool AttachedInfo::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    if (Ptr p = parent())
        cont = cont && self.dvItemField(visitor, Fields::parent, [&self, p]() {
            return self.copy(p, self.m_ownerPath.dropTail(2), p.get());
        });
    cont = cont
            && self.dvValueLazyField(visitor, Fields::path, [this]() { return path().toString(); });
    cont = cont && self.dvItemField(visitor, Fields::subItems, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::subItems),
                [this](const DomItem &map, QString key) {
                    Path p = Path::fromString(key);
                    return map.copy(m_subItems.value(p), map.canonicalPath().key(key));
                },
                [this](const DomItem &) {
                    QSet<QString> res;
                    for (const auto &p : m_subItems.keys())
                        res.insert(p.toString());
                    return res;
                },
                QLatin1String("AttachedInfo")));
    });
    cont = cont && self.dvItemField(visitor, Fields::infoItem, [&self, this]() {
        return infoItem(self);
    });
    return cont;
}

AttachedInfo::AttachedInfo(const AttachedInfo &o):
    OwningItem(o),
    m_parent(o.m_parent)
{
}

/*!
  \brief
  Returns that the AttachedInfo corresponding to the given path, creating it if it does not exists.

  The path might be either a relative path or a canonical path, as specified by the PathType
*/
AttachedInfo::Ptr AttachedInfo::ensure(AttachedInfo::Ptr self, Path path, AttachedInfo::PathType pType){
    switch (pType) {
    case PathType::Canonical: {
        if (!path)
            return nullptr;
        Q_ASSERT(self);
        Path removed = path.mid(0, self->path().length());
        Q_ASSERT(removed == self->path());
        path = path.mid(self->path().length());
    } break;
    case PathType::Relative:
        Q_ASSERT(self);
        break;
    }
    Ptr res = self;
    for (const auto &p : path) {
        if (AttachedInfo::Ptr subEl = res->m_subItems.value(p)) {
            res = subEl;
        } else {
            AttachedInfo::Ptr newEl = res->instantiate(res, p);
            res->m_subItems.insert(p, newEl);
            res = newEl;
        }
    }
    return res;
}

AttachedInfo::Ptr AttachedInfo::find(AttachedInfo::Ptr self, Path p, AttachedInfo::PathType pType){
    if (pType == PathType::Canonical) {
        if (!self) return nullptr;
        Path removed = p.mid(0, self->path().length());
        if (removed != self->path())
            return nullptr;
        p = p.dropFront(self->path().length());
    }
    AttachedInfo::Ptr res = self;
    Path rest = p;
    while (rest) {
        if (!res)
            break;
        res = res->m_subItems.value(rest.head());
        rest = rest.dropFront();
    }
    return res;
}

AttachedInfoLookupResult<AttachedInfo::Ptr>
AttachedInfo::findAttachedInfo(const DomItem &item, QStringView fieldName)
{
    Path p;
    DomItem fLoc = item.field(fieldName);
    if (!fLoc) {
        // owner or container.owner should be a file, so this works, but we could simply use the
        // canonical path, and PathType::Canonical instead...
        DomItem o = item.owner();
        p = item.pathFromOwner();
        fLoc = o.field(fieldName);
        while (!fLoc && o) {
            DomItem c = o.container();
            p = c.pathFromOwner().path(o.canonicalPath().last()).path(p);
            o = c.owner();
            fLoc = o.field(fieldName);
        }
    }
    AttachedInfoLookupResult<AttachedInfo::Ptr> res;
    res.lookupPath = p;
    if (AttachedInfo::Ptr fLocPtr = fLoc.ownerAs<AttachedInfo>())
        if (AttachedInfo::Ptr foundTree =
                    AttachedInfo::find(fLocPtr, p, AttachedInfo::PathType::Relative))
            res.foundTree = foundTree;
    res.rootTreePath = fLoc.canonicalPath();

    res.foundTreePath = res.rootTreePath;
    for (Path pEl : res.lookupPath)
        res.foundTreePath = res.foundTreePath.field(Fields::subItems).key(pEl.toString());
    return res;
}

bool UpdatedScriptExpression::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    cont = cont && self.dvWrapField(visitor, Fields::expr, expr);
    return cont;
}

UpdatedScriptExpression::Tree UpdatedScriptExpression::createTree(Path basePath)
{
    return AttachedInfoT<UpdatedScriptExpression>::createTree(basePath);
}

UpdatedScriptExpression::Tree UpdatedScriptExpression::ensure(UpdatedScriptExpression::Tree base,
                                                              Path basePath,
                                                              AttachedInfo::PathType pType)
{
    return AttachedInfoT<UpdatedScriptExpression>::ensure(base, basePath, pType);
}

AttachedInfoLookupResult<UpdatedScriptExpression::Tree>
UpdatedScriptExpression::findAttachedInfo(const DomItem &item)
{
    return AttachedInfoT<UpdatedScriptExpression>::findAttachedInfo(
            item, Fields::updatedScriptExpressions);
}

UpdatedScriptExpression::Tree UpdatedScriptExpression::treePtr(const DomItem &item)
{
    return AttachedInfoT<UpdatedScriptExpression>::treePtr(item, Fields::updatedScriptExpressions);
}

const UpdatedScriptExpression *UpdatedScriptExpression::exprPtr(const DomItem &item)
{
    if (UpdatedScriptExpression::Tree t = treePtr(item))
        return &(t->info());
    return nullptr;
}

bool UpdatedScriptExpression::visitTree(Tree base, function_ref<bool(Path, Tree)> visitor,
                                        Path basePath)
{
    return AttachedInfoT<UpdatedScriptExpression>::visitTree(base, visitor, basePath);
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#include "moc_qqmldomattachedinfo_p.cpp"
