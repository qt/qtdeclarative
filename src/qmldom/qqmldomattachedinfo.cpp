/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**/
#include "qqmldomattachedinfo_p.h"

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
bool FileLocations::iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem &)> visitor)
{
    bool cont = true;
    QString codeStr = self.fileObject().field(Fields::code).value().toString();
    auto loc2str = [codeStr](SourceLocation loc) {
        return QStringView(codeStr).mid(loc.offset, loc.length);
    };
    cont = cont && self.subDataField(Fields::fullRegion, locationToData(fullRegion)).visit(visitor);
    cont = cont && self.subMap(
        Map::fromMapRef<SourceLocation>(
            self.pathFromOwner().field(Fields::regions), regions, [&loc2str](const DomItem &map, Path key, SourceLocation &el){
                return map.subDataPath(key, locationToData(el, loc2str(el))).item;
            })).visit(visitor);
    cont = cont && self.subMap(
        Map::fromMapRef<QList<SourceLocation>>(
            self.pathFromOwner().field(Fields::preCommentLocations), preCommentLocations,
            [&loc2str](const DomItem &map, Path key, QList<SourceLocation> &el){
                return map.subList(List::fromQListRef<SourceLocation>(
                                       map.pathFromOwner().path(key), el,
                                       [&loc2str](const DomItem &list, Path idx, SourceLocation &el){
                                           return list.subDataPath(idx, locationToData(el, loc2str(el))).item;
                                       })).item;
            })).visit(visitor);
    cont = cont && self.subMap(
        Map::fromMapRef<QList<SourceLocation>>(
            self.pathFromOwner().field(Fields::postCommentLocations), postCommentLocations,
            [&loc2str](const DomItem &map, Path key, QList<SourceLocation> &el){
                return map.subList(List::fromQListRef<SourceLocation>(
                                map.pathFromOwner().path(key), el,
                                [&loc2str](const DomItem &list, Path idx, SourceLocation &el){
                                    return list.subDataPath(idx, locationToData(el, loc2str(el))).item;
                                })).item;
            })).visit(visitor);
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
FileLocations::findAttachedInfo(const DomItem &item, AttachedInfo::FindOptions options)
{
    return AttachedInfoT<FileLocations>::findAttachedInfo(item, Fields::fileLocationsTree, options);
}

FileLocations::Tree FileLocations::treePtr(const DomItem &item)
{
    return AttachedInfoT<FileLocations>::treePtr(item, Fields::fileLocationsTree);
}

const FileLocations *FileLocations::fileLocationsPtr(const DomItem &item)
{
    if (FileLocations::Tree t = treePtr(item))
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

bool AttachedInfo::iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem &)> visitor)
{
    bool cont = true;
    if (Ptr p = parent())
        cont = cont && Subpath{Path::Field(Fields::parent), DomItem(self.m_top, p, self.m_ownerPath.dropTail(2), p.get())}.visit(visitor);
    cont = cont && self.subDataField(Fields::path, path().toString()).visit(visitor);
    cont = cont && self.subMap(
                Map(Path::Field(Fields::subItems),
                    [this](const DomItem &map, QString key){
        Path p = Path::fromString(key);
        return map.copy(m_subItems.value(p), map.canonicalPath().key(key));
    },[this](const DomItem &) {
        QSet<QString> res;
        for (auto p : m_subItems.keys())
            res.insert(p.toString());
        return res;
    }, QLatin1String("AttachedInfo"))).visit(visitor);
    cont = cont && Subpath{Path::Field(Fields::infoItem), infoItem(self)}.visit(visitor);
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
AttachedInfo::findAttachedInfo(const DomItem &item, QStringView fieldName,
                               AttachedInfo::FindOptions options)
{
    Path p;
    DomItem fLoc = item.field(fieldName);
    if (!fLoc) {
        // owner or container.owner should be a file, so this works, but we could simply use the
        // canonical path, and PathType::Canonical instead...
        DomItem o = item.owner();
        p = item.pathFromOwner();
        DomItem fLoc = o.field(fieldName);
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
    if (options & FindOption::SetRootTreePath)
        res.rootTreePath = fLoc.canonicalPath();
    return res;
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
