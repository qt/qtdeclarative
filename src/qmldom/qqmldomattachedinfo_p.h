// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMLDOMATTACHEDINFO_P_H
#define QMLDOMATTACHEDINFO_P_H

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

#include "qqmldom_global.h"
#include "qqmldomitem_p.h"

#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {
struct AttachedInfoLookupResultBase
{
    Path lookupPath;
    Path rootTreePath;
    Path foundTreePath;
};
template<typename TreePtr>
class AttachedInfoLookupResult: public AttachedInfoLookupResultBase
{
public:
    TreePtr foundTree;

    operator bool() { return bool(foundTree); }
    template<typename T>
    AttachedInfoLookupResult<std::shared_ptr<T>> as() const
    {
        AttachedInfoLookupResult<std::shared_ptr<T>> res;
        res.AttachedInfoLookupResultBase::operator=(*this);
        res.foundTree = std::static_pointer_cast<T>(foundTree);
        return res;
    }
};

class QMLDOM_EXPORT AttachedInfo : public OwningItem  {
    Q_GADGET
public:
    enum class PathType {
        Relative,
        Canonical
    };
    Q_ENUM(PathType)

    constexpr static DomType kindValue = DomType::AttachedInfo;
    using Ptr = std::shared_ptr<AttachedInfo>;

    DomType kind() const override { return kindValue; }
    Path canonicalPath(const DomItem &self) const override { return self.m_ownerPath; }
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const override;

    AttachedInfo::Ptr makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<AttachedInfo>(doCopy(self));
    }

    Ptr parent() const { return m_parent.lock(); }
    Path path() const { return m_path; }
    void setPath(Path p) { m_path = p; }

    AttachedInfo(Ptr parent = nullptr, Path p = Path()) : m_path(p), m_parent(parent) {}
    AttachedInfo(const AttachedInfo &o);

    static Ptr ensure(Ptr self, Path path, PathType pType = PathType::Relative);
    static Ptr find(Ptr self, Path p, PathType pType = PathType::Relative);
    static AttachedInfoLookupResult<Ptr> findAttachedInfo(const DomItem &item,
                                                          QStringView treeFieldName);
    static Ptr treePtr(const DomItem &item, QStringView fieldName)
    {
        return findAttachedInfo(item, fieldName).foundTree;
    }

    DomItem itemAtPath(const DomItem &self, Path p, PathType pType = PathType::Relative) const
    {
        if (Ptr resPtr = find(self.ownerAs<AttachedInfo>(), p, pType)) {
            if (pType == PathType::Canonical)
                p = p.mid(m_path.length());
            Path resPath = self.canonicalPath();
            for (Path pEl : p) {
                resPath = resPath.field(Fields::subItems).key(pEl.toString());
            }
            return self.copy(resPtr, resPath);
        }
        return DomItem();
    }

    DomItem infoAtPath(const DomItem &self, Path p, PathType pType = PathType::Relative) const
    {
        return itemAtPath(self, p, pType).field(Fields::infoItem);
    }

    MutableDomItem ensureItemAtPath(MutableDomItem &self, Path p,
                                    PathType pType = PathType::Relative)
    {
        if (Ptr resPtr = ensure(self.ownerAs<AttachedInfo>(), p, pType)) {
            if (pType == PathType::Canonical)
                p = p.mid(m_path.length());
            Path resPath = self.canonicalPath();
            for (Path pEl : p) {
                resPath = resPath.field(Fields::subItems).key(pEl.toString());
            }
            return MutableDomItem(self.item().copy(resPtr, resPath));
        }
        return MutableDomItem();
    }

    MutableDomItem ensureInfoAtPath(MutableDomItem &self, Path p,
                                    PathType pType = PathType::Relative)
    {
        return ensureItemAtPath(self, p, pType).field(Fields::infoItem);
    }

    virtual AttachedInfo::Ptr instantiate(AttachedInfo::Ptr parent, Path p = Path()) const = 0;
    virtual DomItem infoItem(const DomItem &self) const = 0;
    QMap<Path, Ptr> subItems() const {
        return m_subItems;
    }
    void setSubItems(QMap<Path, Ptr> v) {
        m_subItems = v;
    }
protected:
    Path m_path;
    std::weak_ptr<AttachedInfo> m_parent;
    QMap<Path, Ptr> m_subItems;
};

template<typename Info>
class QMLDOM_EXPORT AttachedInfoT final : public AttachedInfo
{
public:
    constexpr static DomType kindValue = DomType::AttachedInfo;
    using Ptr = std::shared_ptr<AttachedInfoT>;
    using InfoType = Info;

    AttachedInfoT(Ptr parent = nullptr, Path p = Path()) : AttachedInfo(parent, p) {}
    AttachedInfoT(const AttachedInfoT &o):
        AttachedInfo(o),
        m_info(o.m_info)
    {
        auto end = o.m_subItems.end();
        auto i = o.m_subItems.begin();
        while (i != end) {
            m_subItems.insert(i.key(), Ptr(
                                  new AttachedInfoT(*std::static_pointer_cast<AttachedInfoT>(i.value()).get())));
        }
    }

   static Ptr createTree(Path p = Path()) {
        return Ptr(new AttachedInfoT(nullptr, p));
    }

    static Ptr ensure(Ptr self, Path path, PathType pType = PathType::Relative){
        return std::static_pointer_cast<AttachedInfoT>(AttachedInfo::ensure(self, path, pType));
    }

    static Ptr find(Ptr self, Path p, PathType pType = PathType::Relative){
        return std::static_pointer_cast<AttachedInfoT>(AttachedInfo::find(self, p, pType));
    }

    static AttachedInfoLookupResult<Ptr> findAttachedInfo(const DomItem &item,
                                                          QStringView fieldName)
    {
        return AttachedInfo::findAttachedInfo(item, fieldName).template as<AttachedInfoT>();
    }
    static Ptr treePtr(const DomItem &item, QStringView fieldName)
    {
        return std::static_pointer_cast<AttachedInfoT>(AttachedInfo::treePtr(item, fieldName));
    }
    static bool visitTree(Ptr base, function_ref<bool(Path, Ptr)>visitor, Path basePath = Path()) {
        if (base) {
            Path pNow = basePath.path(base->path());
            if (visitor(pNow, base)) {
                auto it = base->m_subItems.cbegin();
                auto end = base->m_subItems.cend();
                while (it != end) {
                    if (!visitTree(std::static_pointer_cast<AttachedInfoT>(it.value()), visitor, pNow))
                        return false;
                    ++it;
                }
            } else {
                return false;
            }
        }
        return true;
    }

    AttachedInfo::Ptr instantiate(AttachedInfo::Ptr parent, Path p = Path()) const override {
        return Ptr(new AttachedInfoT(std::static_pointer_cast<AttachedInfoT>(parent), p));
    }
    DomItem infoItem(const DomItem &self) const override { return self.wrapField(Fields::infoItem, m_info); }

    Ptr makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<AttachedInfoT>(doCopy(self));
    }

    Ptr parent() const { return std::static_pointer_cast<AttachedInfoT>(AttachedInfo::parent()); }

    const Info &info() const { return m_info; }
    Info &info() { return m_info; }

    QString canonicalPathForTesting() const
    {
        QString result;
        for (auto *it = this; it; it = it->parent().get()) {
            result.prepend(it->path().toString());
        }
        return result;
    }

protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override
    {
        return Ptr(new AttachedInfoT(*this));
    }

private:
    Info m_info;
};

class QMLDOM_EXPORT FileLocations {
public:
    using Tree = std::shared_ptr<AttachedInfoT<FileLocations>>;
    constexpr static DomType kindValue = DomType::FileLocations;
    DomType kind() const {  return kindValue; }
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const;
    void ensureCommentLocations(const QList<FileLocationRegion> &keys);

    static Tree createTree(Path basePath);
    static Tree ensure(Tree base, Path basePath,
                       AttachedInfo::PathType pType = AttachedInfo::PathType::Relative);
    static Tree find(Tree self, Path p,
                     AttachedInfo::PathType pType = AttachedInfo::PathType::Relative)
    {
        return AttachedInfoT<FileLocations>::find(self, p, pType);
    }

    // returns the path looked up and the found tree when looking for the info attached to item
    static AttachedInfoLookupResult<Tree> findAttachedInfo(const DomItem &item);
    static FileLocations::Tree treeOf(const DomItem &);
    static const FileLocations *fileLocationsOf(const DomItem &);

    static void updateFullLocation(const Tree &fLoc, SourceLocation loc);
    static void addRegion(const Tree &fLoc, FileLocationRegion region, SourceLocation loc);
    static QQmlJS::SourceLocation region(const Tree &fLoc, FileLocationRegion region);

private:
    static QMetaEnum regionEnum;

public:
    SourceLocation fullRegion;
    QMap<FileLocationRegion, SourceLocation> regions;
    QMap<FileLocationRegion, QList<SourceLocation>> preCommentLocations;
    QMap<FileLocationRegion, QList<SourceLocation>> postCommentLocations;
};

class QMLDOM_EXPORT UpdatedScriptExpression
{
    Q_GADGET
public:
    using Tree = std::shared_ptr<AttachedInfoT<UpdatedScriptExpression>>;
    constexpr static DomType kindValue = DomType::UpdatedScriptExpression;
    DomType kind() const { return kindValue; }
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const;

    static Tree createTree(Path basePath);
    static Tree ensure(Tree base, Path basePath, AttachedInfo::PathType pType);

    // returns the path looked up and the found tree when looking for the info attached to item
    static AttachedInfoLookupResult<Tree>
    findAttachedInfo(const DomItem &item);
    // convenience: find FileLocations::Tree attached to the given item
    static Tree treePtr(const DomItem &);
    // convenience: find FileLocations* attached to the given item (if there is one)
    static const UpdatedScriptExpression *exprPtr(const DomItem &);

    static bool visitTree(Tree base, function_ref<bool(Path, Tree)> visitor,
                          Path basePath = Path());

    std::shared_ptr<ScriptExpression> expr;
};

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QMLDOMATTACHEDINFO_P_H
