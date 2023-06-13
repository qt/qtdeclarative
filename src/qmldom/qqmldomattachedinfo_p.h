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
template<typename TreePtr>
class AttachedInfoLookupResult
{
public:
    TreePtr foundTree;
    Path lookupPath; // relative path used to reach result
    std::optional<Path> rootTreePath; // path of the root TreePath
    std::optional<Path> foundTreePath;
    operator bool() { return bool(foundTree); }
    template<typename T>
    AttachedInfoLookupResult<std::shared_ptr<T>> as() const
    {
        AttachedInfoLookupResult<std::shared_ptr<T>> res;
        res.foundTree = std::static_pointer_cast<T>(foundTree);
        res.lookupPath = lookupPath;
        res.rootTreePath = rootTreePath;
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
    enum class FindOption {
        None = 0,
        SetRootTreePath = 0x1,
        SetFoundTreePath = 0x2,
        Default = 0x3
    };
    Q_DECLARE_FLAGS(FindOptions, FindOption)
    Q_FLAG(FindOptions)

    constexpr static DomType kindValue = DomType::AttachedInfo;
    using Ptr = std::shared_ptr<AttachedInfo>;

    DomType kind() const override { return kindValue; }
    Path canonicalPath(DomItem &self) const override { return self.m_ownerPath; }
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;

    AttachedInfo::Ptr makeCopy(DomItem &self) const
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
    static AttachedInfoLookupResult<Ptr>
    findAttachedInfo(DomItem &item, QStringView treeFieldName,
                     FindOptions options = AttachedInfo::FindOption::None);
    static Ptr treePtr(DomItem &item, QStringView fieldName)
    {
        return findAttachedInfo(item, fieldName, FindOption::None).foundTree;
    }

    DomItem itemAtPath(DomItem &self, Path p, PathType pType = PathType::Relative) const
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

    DomItem infoAtPath(DomItem &self, Path p, PathType pType = PathType::Relative) const
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
    virtual DomItem infoItem(DomItem &self) = 0;
    DomItem infoItem(DomItem &self) const
    {
        return const_cast<AttachedInfo *>(this)->infoItem(self);
    }
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
Q_DECLARE_OPERATORS_FOR_FLAGS(AttachedInfo::FindOptions)

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

    static AttachedInfoLookupResult<Ptr> findAttachedInfo(DomItem &item, QStringView fieldName,
                                                          AttachedInfo::FindOptions options)
    {
        return AttachedInfo::findAttachedInfo(item, fieldName, options)
                .template as<AttachedInfoT>();
    }
    static Ptr treePtr(DomItem &item, QStringView fieldName)
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
    DomItem infoItem(DomItem &self) override { return self.wrapField(Fields::infoItem, m_info); }

    Ptr makeCopy(DomItem &self) const
    {
        return std::static_pointer_cast<AttachedInfoT>(doCopy(self));
    }

    Ptr parent() const { return std::static_pointer_cast<AttachedInfoT>(AttachedInfo::parent()); }

    const Info &info() const { return m_info; }
    Info &info() { return m_info; }
protected:
    std::shared_ptr<OwningItem> doCopy(DomItem &) const override
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
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor);
    void ensureCommentLocations(QList<QString> keys);

    static Tree createTree(Path basePath);
    static Tree ensure(Tree base, Path basePath,
                       AttachedInfo::PathType pType = AttachedInfo::PathType::Relative);
    static Tree find(Tree self, Path p,
                     AttachedInfo::PathType pType = AttachedInfo::PathType::Relative)
    {
        return AttachedInfoT<FileLocations>::find(self, p, pType);
    }

    // returns the path looked up and the found tree when looking for the info attached to item
    static AttachedInfoLookupResult<Tree>
    findAttachedInfo(DomItem &item,
                     AttachedInfo::FindOptions options = AttachedInfo::FindOption::Default);
    static FileLocations::Tree treeOf(DomItem &);
    static const FileLocations *fileLocationsOf(DomItem &);

    static void updateFullLocation(Tree fLoc, SourceLocation loc);
    static void addRegion(Tree fLoc, QString locName, SourceLocation loc);
    static void addRegion(Tree fLoc, QStringView locName, SourceLocation loc);

    SourceLocation fullRegion;
    QMap<QString, SourceLocation> regions;
    QMap<QString, QList<SourceLocation>> preCommentLocations;
    QMap<QString, QList<SourceLocation>> postCommentLocations;
};

class QMLDOM_EXPORT UpdatedScriptExpression
{
    Q_GADGET
public:
    using Tree = std::shared_ptr<AttachedInfoT<UpdatedScriptExpression>>;
    constexpr static DomType kindValue = DomType::UpdatedScriptExpression;
    DomType kind() const { return kindValue; }
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor);

    static Tree createTree(Path basePath);
    static Tree ensure(Tree base, Path basePath, AttachedInfo::PathType pType);

    // returns the path looked up and the found tree when looking for the info attached to item
    static AttachedInfoLookupResult<Tree>
    findAttachedInfo(DomItem &item,
                     AttachedInfo::FindOptions options = AttachedInfo::FindOption::Default);
    // convenience: find FileLocations::Tree attached to the given item
    static Tree treePtr(DomItem &);
    // convenience: find FileLocations* attached to the given item (if there is one)
    static const UpdatedScriptExpression *exprPtr(DomItem &);

    static bool visitTree(Tree base, function_ref<bool(Path, Tree)> visitor,
                          Path basePath = Path());

    std::shared_ptr<ScriptExpression> expr;
};

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QMLDOMATTACHEDINFO_P_H
