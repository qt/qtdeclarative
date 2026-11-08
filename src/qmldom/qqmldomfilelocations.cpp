// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqmldom_fwd_p.h"
#include "qqmldompath_p.h"
#include "qqmldomfilelocations_p.h"

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

using namespace Qt::StringLiterals;

/*!
\internal
\namespace QQmlJS::Dom::FileLocations
\brief Provides entities to maintain mappings between elements and their location in a file

The location information is associated with the element it refers to via ::Node
There are free functions to simplify the handling of the FileLocations tree.
*/
namespace FileLocations {

/*!
\internal
\namespace QQmlJS::Dom::FileLocations::Info
\brief Contains region information about the item

Attributes:
\list
\li fullRegion: A location guaranteed to include this element and all its sub elements
\li regions: a map with locations of regions of this element, the empty string is the default region
 of this element
\endlist
*/
bool Info::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    cont = cont && self.invokeVisitorOnLazyField(visitor, Fields::fullRegion, [this]() {
        return sourceLocationToQCborValue(fullRegion);
    });
    cont = cont && visitor(Fields::regions, [this, &self]() -> DomItem {
               const Path pathFromOwner = self.pathFromOwner().withField(Fields::regions);
               auto map = Map::fromFileRegionMap(pathFromOwner, regions);
               return self.subMapItem(map);
           });
    return cont;
}

Tree createTree(const Path &basePath)
{
    return Node::instantiate(nullptr, basePath);
}

Tree ensure(const Tree &base, const Path &basePath)
{
    Path relative = basePath;
    Tree res = base;
    for (const auto &p : std::as_const(relative)) {
        res = res->insertOrReturnChildAt(p);
    }
    return res;
}

Tree find(const Tree &self, const Path &p)
{
    Tree res = self;
    for (const Path &component : p) {
        if (!res)
            break;
        res = res->subItems().value(component);
    }
    return res;
}

bool visitTree(const Tree &base, function_ref<bool(const Path &, const Tree &)> visitor,
               const Path &basePath)
{
    if (base) {
        Path pNow = basePath.withPath(base->path());
        if (!visitor(pNow, base)) {
            return false;
        }
        for (const auto &childNode : base->subItems()) {
            if (!visitTree(childNode, visitor, pNow))
                return false;
        }
    }
    return true;
}

QString canonicalPathForTesting(const Tree &base)
{
    QString result;
    for (auto *it = base.get(); it; it = it->parent().get()) {
        result.prepend(it->path().toString());
    }
    return result;
}

/*!
   \internal
   Returns the tree corresponding to a DomItem.
 */
Tree treeOf(const DomItem &item)
{
    Path p;
    DomItem fLoc = item.field(Fields::fileLocationsTree);
    if (!fLoc) {
        // owner or container.owner should be a file, so this works, but we could simply use the
        // canonical path, and PathType::Canonical instead...
        DomItem o = item.owner();
        p = item.pathFromOwner();
        fLoc = o.field(Fields::fileLocationsTree);
        while (!fLoc && o) {
            DomItem c = o.container();
            p = c.pathFromOwner().withPath(o.canonicalPath().last()).withPath(p);
            o = c.owner();
            fLoc = o.field(Fields::fileLocationsTree);
        }
    }
    if (Node::Ptr fLocPtr = fLoc.ownerAs<Node>())
        return find(fLocPtr, p);
    return {};
}

static void updateFullLocation(const Tree &fLoc, SourceLocation loc)
{
    Q_ASSERT(fLoc);
    if (loc != SourceLocation()) {
        Tree p = fLoc;
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

// Adding a new region to file location regions might break down qmlformat because
// comments might be linked to new region undesirably. We might need to add an
// exception to AstRangesVisitor::shouldSkipRegion when confronted those cases.
// beware of shrinking and reassigning regions (QTBUG-131288)
void addRegion(const Tree &fLoc, FileLocationRegion region, SourceLocation loc)
{
    Q_ASSERT(fLoc);
    fLoc->info().regions[region] = loc;
    updateFullLocation(fLoc, loc);
}

SourceLocation region(const Tree &fLoc, FileLocationRegion region)
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
\class QQmlJS::Dom::FileLocations::Node
\brief Represents a Node of FileLocations tree

Attributes:
\list
\li parent: parent Node in tree (might be empty)
\li subItems: subItems of the tree (path -> Node)
\li infoItem: actual FileLocations::Info with regions
\endlist

\sa QQmlJs::Dom::Node
*/

Node::Ptr Node::instantiate(const Ptr &parent, const Path &p)
{
    return std::shared_ptr<Node>(new Node(parent, p));
}

bool Node::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    if (const Ptr p = parent()) {
        cont = cont && visitor(Fields::parent, [&self, &p]() {
                   return self.copy(p, self.m_ownerPath.dropTail(2), p.get());
               });
    }
    cont = cont && self.invokeVisitorOnLazyField(visitor, Fields::path, [this]() {
        return path().toString();
    });
    cont = cont && visitor(Fields::subItems, [this, &self]() {
               // keyed by Path::toString() to avoid reparsing the path in Map::key()
               QMap<QString, Path> byName;
               for (auto it = m_subItems.keyBegin(), end = m_subItems.keyEnd(); it != end; ++it)
                   byName.insert(it->toString(), *it);
               return self.subMapItem(Map(
                       Path::fromField(Fields::subItems),
                       [this, byName](const DomItem &map, const QString &name) {
                           auto it = byName.find(name);
                           if (it == byName.end())
                               return DomItem();
                           return map.copy(m_subItems.value(*it),
                                           map.canonicalPath().withKey(name));
                       },
                       [byName](const DomItem &) {
                           return QSet<QString>(byName.keyBegin(), byName.keyEnd());
                       },
                       QLatin1String("Node")));
           });
    cont = cont && visitor(Fields::infoItem, [&self, this]() {
               return self.wrap(Fields::infoItem, m_info);
           });
    return cont;
}

Node::Ptr Node::insertOrReturnChildAt(const Path &path)
{
    if (Node::Ptr subEl = m_subItems.value(path)) {
        return subEl;
    }
    return m_subItems.insert(path, Node::instantiate(shared_from_this(), path)).value();
}

std::shared_ptr<OwningItem> Node::doCopy(const DomItem &) const
{
    return std::shared_ptr<Node>(new Node(*this));
}

} // namespace FileLocations
} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
