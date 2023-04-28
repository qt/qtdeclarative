// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqmldomlinewriter_p.h"
#include "qqmldomelements_p.h"

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

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
bool FileLocations::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
#ifdef QmlDomAddCodeStr
    bool hasCode = false;
    QString codeStr = self.fileObject().field(Fields::code).value().toString();
    auto loc2str = [&self, &codeStr](SourceLocation loc) {
        if (loc.offset < codeStr.length() && loc.end() <= codeStr.length())
            return QStringView(codeStr).mid(loc.offset, loc.length);
        return QStringView();
    };
#else
    auto loc2str = [](SourceLocation) { return QStringView(); };
#endif
    cont = cont && self.dvValueLazyField(visitor, Fields::fullRegion, [this]() {
        return locationToData(fullRegion);
    });
    cont = cont && self.dvItemField(visitor, Fields::regions, [this, &self, &loc2str]() {
        return self.subMapItem(Map::fromMapRef<SourceLocation>(
                self.pathFromOwner().field(Fields::regions), regions,
                [&loc2str](DomItem &map, const PathEls::PathComponent &key, SourceLocation &el) {
                    return map.subLocationItem(key, el, loc2str(el));
                }));
    });
    cont = cont
            && self.dvItemField(visitor, Fields::preCommentLocations, [this, &self, &loc2str]() {
                   return self.subMapItem(Map::fromMapRef<QList<SourceLocation>>(
                           self.pathFromOwner().field(Fields::preCommentLocations),
                           preCommentLocations,
                           [&loc2str](DomItem &map, const PathEls::PathComponent &key,
                                      QList<SourceLocation> &el) {
                               return map.subListItem(List::fromQListRef<SourceLocation>(
                                       map.pathFromOwner().appendComponent(key), el,
                                       [&loc2str](DomItem &list, const PathEls::PathComponent &idx,
                                                  SourceLocation &el) {
                                           return list.subLocationItem(idx, el, loc2str(el));
                                       }));
                           }));
               });
    cont = cont
            && self.dvItemField(visitor, Fields::postCommentLocations, [this, &self, &loc2str]() {
                   return self.subMapItem(Map::fromMapRef<QList<SourceLocation>>(
                           self.pathFromOwner().field(Fields::postCommentLocations),
                           postCommentLocations,
                           [&loc2str](DomItem &map, const PathEls::PathComponent &key,
                                      QList<SourceLocation> &el) {
                               return map.subListItem(List::fromQListRef<SourceLocation>(
                                       map.pathFromOwner().appendComponent(key), el,
                                       [&loc2str](DomItem &list, const PathEls::PathComponent &idx,
                                                  SourceLocation &el) {
                                           return list.subLocationItem(idx, el, loc2str(el));
                                       }));
                           }));
               });
    return cont;
}

void FileLocations::ensureCommentLocations(QList<QString> keys)
{
    for (auto k : keys) {
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

AttachedInfoLookupResult<FileLocations::Tree>
FileLocations::findAttachedInfo(DomItem &item, AttachedInfo::FindOptions options)
{
    return AttachedInfoT<FileLocations>::findAttachedInfo(item, Fields::fileLocationsTree, options);
}

/*!
   \internal
   Returns the tree corresponding to a DomItem.
 */
FileLocations::Tree FileLocations::treeOf(DomItem &item)
{
    return AttachedInfoT<FileLocations>::treePtr(item, Fields::fileLocationsTree);
}

/*!
   \internal
   Returns the filelocation Info corresponding to a DomItem.
 */
const FileLocations *FileLocations::fileLocationsOf(DomItem &item)
{
    if (FileLocations::Tree t = treeOf(item))
        return &(t->info());
    return nullptr;
}

void FileLocations::updateFullLocation(FileLocations::Tree fLoc, SourceLocation loc) {
    Q_ASSERT(fLoc);
    if (loc != SourceLocation()) {
        FileLocations::Tree p = fLoc;
        while (p) {
            SourceLocation &l = p->info().fullRegion;
            if (loc.begin() < l.begin() || loc.end() > l.end())
                l = combine(l, loc);
            else
                break;
            p = p->parent();
        }
    }
}

void FileLocations::addRegion(FileLocations::Tree fLoc, QString locName, SourceLocation loc) {
    Q_ASSERT(fLoc);
    fLoc->info().regions[locName] = loc;
    updateFullLocation(fLoc, loc);
}

void FileLocations::addRegion(FileLocations::Tree fLoc, QStringView locName, SourceLocation loc) {
    addRegion(fLoc, locName.toString(), loc);
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

bool AttachedInfo::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
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
                [this](DomItem &map, QString key) {
                    Path p = Path::fromString(key);
                    return map.copy(m_subItems.value(p), map.canonicalPath().key(key));
                },
                [this](DomItem &) {
                    QSet<QString> res;
                    for (auto p : m_subItems.keys())
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
    for (auto p : path) {
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
AttachedInfo::findAttachedInfo(DomItem &item, QStringView fieldName,
                               AttachedInfo::FindOptions options)
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
    if (options & (FindOption::SetRootTreePath | FindOption::SetFoundTreePath))
        res.rootTreePath = fLoc.canonicalPath();
    if (options & FindOption::SetFoundTreePath) {
        Path foundTreePath = res.rootTreePath.value();
        if (res.lookupPath) {
            foundTreePath = foundTreePath.key(res.lookupPath.head().toString());
            for (Path pEl : res.lookupPath.mid(1))
                foundTreePath = foundTreePath.field(Fields::subItems).key(pEl.toString());
        }
        res.foundTreePath = foundTreePath;
    }
    return res;
}

bool UpdatedScriptExpression::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
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
UpdatedScriptExpression::findAttachedInfo(DomItem &item, AttachedInfo::FindOptions options)
{
    return AttachedInfoT<UpdatedScriptExpression>::findAttachedInfo(
            item, Fields::updatedScriptExpressions, options);
}

UpdatedScriptExpression::Tree UpdatedScriptExpression::treePtr(DomItem &item)
{
    return AttachedInfoT<UpdatedScriptExpression>::treePtr(item, Fields::updatedScriptExpressions);
}

const UpdatedScriptExpression *UpdatedScriptExpression::exprPtr(DomItem &item)
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
