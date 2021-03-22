/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#ifndef QMLDOMITEM_H
#define QMLDOMITEM_H

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
#include "qqmldom_fwd_p.h"
#include "qqmldomconstants_p.h"
#include "qqmldomfunctionref_p.h"
#include "qqmldomstringdumper_p.h"
#include "qqmldompath_p.h"
#include "qqmldomerrormessage_p.h"

#include <QtCore/QMap>
#include <QtCore/QMultiMap>
#include <QtCore/QString>
#include <QtCore/QStringView>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QMutex>
#include <QtCore/QCborValue>
#include <QtQml/private/qqmljssourcelocation_p.h>

#include <memory>
#include <typeinfo>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
// we didn't have enough 'O's to properly name everything...
namespace Dom {

class Path;

bool domTypeIsObjWrap(DomType k);
bool domTypeIsDomElement(DomType);
bool domTypeIsOwningItem(DomType);
bool domTypeIsExternalItem(DomType k);
bool domTypeIsTopItem(DomType k);
bool domTypeIsContainer(DomType k);
bool domTypeCanBeInline(DomType k);

QMLDOM_EXPORT QMap<DomType,QString> domTypeToStringMap();
QMLDOM_EXPORT QString domTypeToString(DomType k);

QMLDOM_EXPORT QCborValue locationToData(SourceLocation loc, QStringView strValue=u"");

class QMLDOM_EXPORT DomBase{
public:
    virtual ~DomBase() = default;

    // minimal overload set:
    virtual DomType kind() const = 0;
    virtual DomKind domKind() const;
    virtual Path pathFromOwner(const DomItem &self) const = 0;
    virtual Path canonicalPath(const DomItem &self) const = 0;
    virtual bool iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem &)>) = 0; // iterates the *direct* subpaths, returns false if a quick end was requested
    bool iterateDirectSubpathsConst(const DomItem &self, function_ref<bool(Path, const DomItem &)>) const; // iterates the *direct* subpaths, returns false if a quick end was requested

    virtual DomItem containingObject(const DomItem &self) const; // the DomItem corresponding to the canonicalSource source
    virtual void dump(const DomItem &, Sink sink, int indent) const;
    virtual quintptr id() const;
    virtual QString typeName() const;

    virtual QList<QString> const fields(const DomItem &self) const;
    virtual DomItem field(const DomItem &self, QStringView name) const;

    virtual index_type indexes(const DomItem &self) const;
    virtual DomItem index(const DomItem &self, index_type index) const;

    virtual QSet<QString> const keys(const DomItem &self) const;
    virtual DomItem key(const DomItem &self, QString name) const;

    virtual QString canonicalFilePath(const DomItem &self) const;
    virtual SourceLocation location(const DomItem &self) const;

    virtual QCborValue value() const {
        return QCborValue();
    }

};

inline DomKind kind2domKind(DomType k)
{
    switch (k) {
    case DomType::Empty:
        return DomKind::Empty;
    case DomType::List:
        return DomKind::List;
    case DomType::Map:
        return DomKind::Map;
    case DomType::ConstantData:
        return DomKind::Value;
    default:
        return DomKind::Object;
    }
}

class QMLDOM_EXPORT Empty: public DomBase {
public:
    constexpr static DomType kindValue = DomType::Empty;
    DomType kind() const override {  return kindValue; }

    Empty();
    Path pathFromOwner(const DomItem &self) const override;
    Path canonicalPath(const DomItem &self) const override;
    DomItem containingObject(const DomItem &self) const override;
    bool iterateDirectSubpaths(DomItem &self, function_ref<bool (Path, DomItem &)>) override;
    void dump(const DomItem &, Sink s, int indent) const override;
};

class QMLDOM_EXPORT DomElement: public DomBase {
protected:
    DomElement& operator=(const DomElement&) = default;
public:
    DomElement(Path pathFromOwner = Path(), const SourceLocation & loc = SourceLocation());
    DomElement(const DomElement &o) = default;
    Path pathFromOwner(const DomItem &self) const override;
    Path pathFromOwner() const { return m_pathFromOwner; }
    Path canonicalPath(const DomItem &self) const override;
    DomItem containingObject(const DomItem &self) const override;
    virtual void updatePathFromOwner(Path newPath);
    SourceLocation location(const DomItem &self) const override;

    SourceLocation loc;
private:
    Path m_pathFromOwner;
};

class QMLDOM_EXPORT Map: public DomElement {
public:
    constexpr static DomType kindValue = DomType::Map;
    DomType kind() const override {  return kindValue; }

    using LookupFunction = std::function<DomItem (const DomItem&, QString)>;
    using Keys = std::function<QSet<QString> (const DomItem &)>;
    Map(Path pathFromOwner, LookupFunction lookup, Keys keys, QString targetType);
    quintptr id() const override;
    bool iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem &)>) override;
    QSet<QString> const keys(const DomItem &self) const override;
    DomItem key(const DomItem &self, QString name) const override;

    template <typename T>
    static Map fromMultiMapRef(Path pathFromOwner, QMultiMap<QString,T> &mmap, std::function<DomItem(const DomItem &, Path, T&)> elWrapper);
    template <typename T>
    static Map fromMapRef(Path pathFromOwner, QMap<QString,T> &mmap, std::function<DomItem(const DomItem &, Path, T&)> elWrapper);
private:
    LookupFunction m_lookup;
    Keys m_keys;
    QString m_targetType;
};

class QMLDOM_EXPORT List: public DomElement {
public:
    constexpr static DomType kindValue = DomType::List;
    DomType kind() const override {  return kindValue; }

    using LookupFunction = std::function<DomItem (const DomItem &, index_type)>;
    using Length = std::function<index_type (const DomItem &)>;
    using IteratorFunction = std::function<bool (const DomItem &, function_ref<bool(index_type,DomItem &)>)>;

    List(Path pathFromOwner, LookupFunction lookup, Length length, IteratorFunction iterator, QString elType);
    quintptr id() const override;
    bool iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem &)>) override;
    void dump(const DomItem &, Sink s, int indent) const override;
    index_type indexes(const DomItem &self) const override;
    DomItem index(const DomItem &self, index_type index) const override;

    template<typename T>
    static List fromQList(Path pathFromOwner, QList<T> list, std::function<DomItem(const DomItem &, Path, T&)> elWrapper, ListOptions options = ListOptions::Normal);
    template<typename T>
    static List fromQListRef(Path pathFromOwner, QList<T> &list, std::function<DomItem(const DomItem &, Path, T&)> elWrapper, ListOptions options = ListOptions::Normal);
private:
    LookupFunction m_lookup;
    Length m_length;
    IteratorFunction m_iterator;
    QString m_elType;
};

class QMLDOM_EXPORT ConstantData: public DomElement {
public:
    constexpr static DomType kindValue = DomType::ConstantData;
    DomType kind() const override {  return kindValue; }

    enum class Options {
        MapIsMap,
        FirstMapIsFields
    };

    ConstantData(Path pathFromOwner, QCborValue value, Options options = Options::MapIsMap, const SourceLocation & loc = SourceLocation());
    bool iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem &)>) override;
    quintptr id() const override;
    DomKind domKind() const override;
    QCborValue value() const override { return m_value; }
    Options options() const { return m_options; }
private:
    QCborValue m_value;
    Options m_options;
};

class QMLDOM_EXPORT SimpleObjectWrap: public DomElement {
public:
    constexpr static DomType kindValue = DomType::SimpleObjectWrap;
    DomType kind() const override {  return kindValue; }

    template <typename T>
    static SimpleObjectWrap fromDataObject(
            Path pathFromOwner, T const & val,
            std::function<QCborValue(T const &)> toData,
            const SourceLocation & loc = SourceLocation(),
            DomType kind = kindValue,
            DomKind domKind = DomKind::Object,
            QString typeName = QString());

    template <typename T>
    static SimpleObjectWrap fromObjectRef(
            Path pathFromOwner, T &value,
            std::function<bool(DomItem &, T &val, function_ref<bool(Path, DomItem &)>)> directSubpathsIterate,
            const SourceLocation & loc = SourceLocation(),
            DomType kind = kindValue,
            QString typeName = QString(),
            DomKind domKind = DomKind::Object);

    bool iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem &)>) override;
    quintptr id() const override;
    QString typeName() const override { return m_typeName; }
    DomType internalKind() const { return m_kind; }
    DomKind domKind() const override { return m_domKind; }
    template <typename T>
    T const *as() const
    {
        return m_value.value<T*>();
    }
    template <typename T>
    T *mutableAs()
    {
        return m_value.value<T*>();
    }
private:
    SimpleObjectWrap(
        Path pathFromOwner, QVariant value,
        std::function<bool(DomItem &, QVariant, function_ref<bool(Path, DomItem &)>)> directSubpathsIterate,
        DomType kind = kindValue,
        DomKind domKind = DomKind::Object,
        QString typeName = QString(),
        const SourceLocation & loc = SourceLocation());

    DomType m_kind;
    DomKind m_domKind;
    QString m_typeName;
    QVariant m_value;
    std::function<bool(DomItem &, QVariant, function_ref<bool(Path, DomItem &)>)> m_directSubpathsIterate;
};

class QMLDOM_EXPORT Reference: public DomElement {
public:
    constexpr static DomType kindValue = DomType::Reference;
    DomType kind() const override {  return kindValue; }

    Reference(Path referredObject = Path(), Path pathFromOwner = Path(), const SourceLocation & loc = SourceLocation());
    quintptr id() const override;
    bool iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem &)>) override;
    DomItem field(const DomItem &self, QStringView name) const override;
    QList<QString> const fields(const DomItem &self) const override;
    index_type indexes(const DomItem &) const override {
        return 0;
    }
    DomItem index(const DomItem &, index_type) const override;
    QSet<QString> const keys(const DomItem &) const override {
        return {};
    }
    DomItem key(const DomItem &, QString) const override;

    DomItem get(const DomItem &self) const;

    Path referredObjectPath;
};

inline bool emptyChildrenVisitor(Path, const DomItem &, bool) { return true; }

class MutableDomItem;

class QMLDOM_EXPORT DomItem {
    Q_DECLARE_TR_FUNCTIONS(DomItem);
public:
    using Callback = function<void(Path, const DomItem &, const DomItem &)>;

    using InternalKind = DomType;
    using Visitor = function_ref<bool(Path, const DomItem &)>;
    using ChildrenVisitor = function_ref<bool(Path, const DomItem &, bool)>;

    static ErrorGroup domErrorGroup;
    static ErrorGroups myErrors();
    static ErrorGroups myResolveErrors();

    operator bool() const { return base()->kind() != DomType::Empty; }
    InternalKind internalKind() const {
        InternalKind res = base()->kind();
        if (res == InternalKind::SimpleObjectWrap)
            return static_cast<SimpleObjectWrap const *>(base())->internalKind();
        return res;
    }
    DomKind domKind() const {
        return base()->domKind();
    }

    Path canonicalPath() const;
    DomItem containingObject() const;
    DomItem container() const;
    DomItem component() const;
    DomItem owner() const;
    DomItem top() const;
    DomItem environment() const;
    DomItem universe() const;

    DomItem fileObject(GoTo option = GoTo::Strict) const;

    // convenience getters
    QString name() const;
    DomItem qmlChildren() const;
    DomItem annotations() const;

    bool resolve(Path path, Visitor visitor, ErrorHandler errorHandler, ResolveOptions options = ResolveOption::None, Path fullPath = Path(), QList<Path> *visitedRefs = nullptr) const;

    DomItem operator[](Path path) const;
    DomItem operator[](QStringView component) const;
    DomItem operator[](const QString &component) const;
    DomItem operator[](const char16_t *component) const { return (*this)[QStringView(component)]; } // to avoid clash with stupid builtin ptrdiff_t[DomItem&], coming from C
    DomItem operator[](index_type i) const { return index(i); }
    DomItem operator[](int i) const { return index(i); }

    DomItem path(Path p, ErrorHandler h = &defaultErrorHandler) const;
    DomItem path(QString p, ErrorHandler h = &defaultErrorHandler) const;
    DomItem path(QStringView p, ErrorHandler h = &defaultErrorHandler) const;

    QList<QString> const fields() const;
    DomItem field(QStringView name) const;

    index_type indexes() const;
    DomItem index(index_type) const;

    QSet<QString> const keys() const;
    DomItem key(QString name) const;

    bool visitChildren(Path basePath, ChildrenVisitor visitor, VisitOptions options = VisitOption::VisitAdopted, ChildrenVisitor openingVisitor = emptyChildrenVisitor, ChildrenVisitor closingVisitor = emptyChildrenVisitor) const;

    quintptr id() const { return base()->id(); }
    Path pathFromOwner() const { return base()->pathFromOwner(*this); }
    QString canonicalFilePath() const { return base()->canonicalFilePath(*this); }
    SourceLocation location() const { return base()->location(*this); }

    QCborValue value() const;

    void dumpPtr(Sink) const;
    void dump(Sink, int indent = 0) const;
    QString toString() const;

    // OwnigItem elements
    int derivedFrom() const;
    int revision() const;
    QDateTime createdAt() const;
    QDateTime frozenAt() const;
    QDateTime lastDataUpdateAt() const;

    void addError(ErrorMessage msg) const;
    ErrorHandler errorHandler() const;
    void clearErrors(ErrorGroups groups = ErrorGroups({}), bool iterate = true) const;
    // return false if a quick exit was requested
    bool iterateErrors(function_ref<bool(DomItem source, ErrorMessage msg)> visitor, bool iterate,
                       Path inPath = Path())const;

    bool iterateSubOwners(function_ref<bool(DomItem owner)> visitor) const;

    Subpath subDataField(QStringView fieldName, QCborValue value, ConstantData::Options options = ConstantData::Options::MapIsMap, const SourceLocation &loc = SourceLocation()) const;
    Subpath subDataField(QString fieldName, QCborValue value, ConstantData::Options options = ConstantData::Options::MapIsMap, const SourceLocation &loc = SourceLocation()) const;
    Subpath subDataIndex(index_type i, QCborValue value, ConstantData::Options options = ConstantData::Options::MapIsMap, const SourceLocation &loc = SourceLocation()) const;
    Subpath subDataKey(QStringView keyName, QCborValue value, ConstantData::Options options = ConstantData::Options::MapIsMap, const SourceLocation &loc = SourceLocation()) const;
    Subpath subDataKey(QString keyName, QCborValue value, ConstantData::Options options = ConstantData::Options::MapIsMap, const SourceLocation &loc = SourceLocation()) const;
    Subpath subDataPath(Path path, QCborValue value, ConstantData::Options options = ConstantData::Options::MapIsMap, const SourceLocation &loc = SourceLocation()) const;
    Subpath subReferenceField(QStringView fieldName, Path referencedObject,
                      const SourceLocation & loc = SourceLocation()) const;
    Subpath subReferenceField(QString fieldName, Path referencedObject, const SourceLocation & loc = SourceLocation()) const;
    Subpath subReferenceKey(QStringView keyName, Path referencedObject, const SourceLocation & loc = SourceLocation()) const;
    Subpath subReferenceKey(QString keyName, Path referencedObject, const SourceLocation & loc = SourceLocation()) const;
    Subpath subReferenceIndex(index_type i, Path referencedObject, const SourceLocation & loc = SourceLocation()) const;
    Subpath subReferencePath(Path subPath, Path referencedObject, const SourceLocation & loc = SourceLocation()) const;

    Subpath toSubField(QStringView fieldName) const;
    Subpath toSubField(QString fieldName) const;
    Subpath toSubKey(QStringView keyName) const;
    Subpath toSubKey(QString keyName) const;
    Subpath toSubIndex(index_type i) const;
    Subpath toSubPath(Path subPath) const;
    Subpath subList(const List &list) const;
    Subpath subMap(const Map &map) const;
    Subpath subObjectWrap(const SimpleObjectWrap &o) const;
    template <typename T>
    Subpath subWrapPath(Path p, T &obj, SourceLocation loc = SourceLocation()) const;
    template <typename T>
    Subpath subWrapField(QString p, T &obj, SourceLocation loc = SourceLocation()) const;
    template <typename T>
    Subpath subWrapField(QStringView p, T &obj, SourceLocation loc = SourceLocation()) const;
    template <typename T>
    Subpath subWrapKey(QString p, T &obj, SourceLocation loc = SourceLocation()) const;
    template <typename T>
    Subpath subWrapKey(QStringView p, T &obj, SourceLocation loc = SourceLocation()) const;
    template <typename T>
    Subpath subWrapIndex(index_type i, T &obj, SourceLocation loc = SourceLocation()) const;

    DomItem();
    DomItem(std::shared_ptr<DomEnvironment>);
    DomItem(std::shared_ptr<DomUniverse>);

    // --- start of potentially dangerous stuff, make private? ---

    std::shared_ptr<DomTop> topPtr() const;
    std::shared_ptr<OwningItem> owningItemPtr() const;

    // keep the DomItem around to ensure that it doesn't get deleted
    template <typename T, typename std::enable_if<std::is_base_of<DomBase, T>::value, bool>::type = true>
    T const*as() const {
        InternalKind k = base()->kind();
        if (k == T::kindValue)
            return static_cast<T const*>(base());
        if (k == InternalKind::SimpleObjectWrap)
            return static_cast<SimpleObjectWrap const *>(base())->as<T>();
        return nullptr;
    }

    template <typename T, typename std::enable_if<!std::is_base_of<DomBase, T>::value, bool>::type = true>
    T const*as() const {
        InternalKind k = base()->kind();
        if (k == InternalKind::SimpleObjectWrap)
            return static_cast<SimpleObjectWrap const *>(base())->as<T>();
        return nullptr;
    }

    template <typename T>
    std::shared_ptr<T> ownerAs() const;

    DomItem copy(std::shared_ptr<OwningItem> owner, Path ownerPath, DomBase *base) const;
    DomItem copy(std::shared_ptr<OwningItem> owner, Path ownerPath = Path()) const;
    DomItem copy(DomBase *base) const;
private:
    DomBase const* base() const {
        if (m_base == nullptr)
            return reinterpret_cast<DomBase const*>(&inlineEl);
        return m_base;
    }
    template <typename T, typename std::enable_if<std::is_base_of<DomBase, T>::value, bool>::type = true>
    T *mutableAs() {
        InternalKind k = base()->kind();
        if (k == T::kindValue)
            return static_cast<T*>(mutableBase());
        if (k == InternalKind::SimpleObjectWrap)
            return static_cast<SimpleObjectWrap *>(mutableBase())->mutableAs<T>();
        return nullptr;
    }

    template <typename T, typename std::enable_if<!std::is_base_of<DomBase, T>::value, bool>::type = true>
    T *mutableAs() {
        InternalKind k = base()->kind();
        if (k == InternalKind::SimpleObjectWrap)
            return static_cast<SimpleObjectWrap *>(mutableBase())->mutableAs<T>();
        return nullptr;
    }
    DomBase * mutableBase() {
        if (m_base == nullptr)
            return reinterpret_cast<DomBase*>(&inlineEl);
        return m_base;
    }
    DomItem(std::shared_ptr<DomTop> env, std::shared_ptr<OwningItem> owner, Path ownerPath, DomBase *base);
    DomItem(std::shared_ptr<DomTop> env, std::shared_ptr<OwningItem> owner, Map map);
    DomItem(std::shared_ptr<DomTop> env, std::shared_ptr<OwningItem> owner, List list);
    DomItem(std::shared_ptr<DomTop> env, std::shared_ptr<OwningItem> owner, ConstantData data);
    DomItem(std::shared_ptr<DomTop> env, std::shared_ptr<OwningItem> owner, Reference reference);
    DomItem(std::shared_ptr<DomTop> env, std::shared_ptr<OwningItem> owner, SimpleObjectWrap wrapper);
    friend class DomElement;
    friend class Map;
    friend class List;
    friend class QmlObject;
    friend class DomUniverse;
    friend class DomEnvironment;
    friend class ExternalItemInfoBase;
    friend class ConstantData;
    friend class MutableDomItem;
    friend class AttachedInfo;
    friend bool operator ==(const DomItem &, const DomItem &);
    std::shared_ptr<DomTop> m_top;
    std::shared_ptr<OwningItem> m_owner;
    Path m_ownerPath;
    DomBase *m_base;
    union InlineEl {
        // Should add optimized move ops (should be able to do a bit copy of union)
        InlineEl(): empty() { }
        InlineEl(const InlineEl &d) {
            switch (d.kind()){
            case DomType::Empty:
                Q_ASSERT((quintptr)this == (quintptr)&empty && "non C++11 compliant compiler");
                new (&empty) Empty(d.empty);
                break;
            case DomType::Map:
                Q_ASSERT((quintptr)this == (quintptr)&map && "non C++11 compliant compiler");
                new (&map) Map(d.map);
                break;
            case DomType::List:
                Q_ASSERT((quintptr)this == (quintptr)&list && "non C++11 compliant compiler");
                new (&list) List(d.list);
                break;
            case DomType::ConstantData:
                Q_ASSERT((quintptr)this == (quintptr)&data && "non C++11 compliant compiler");
                new (&data) ConstantData(d.data);
                break;
            case DomType::SimpleObjectWrap:
                Q_ASSERT((quintptr)this == (quintptr)&simpleObjectWrap && "non C++11 compliant compiler");
                new (&simpleObjectWrap) SimpleObjectWrap(d.simpleObjectWrap);
                break;
            case DomType::Reference:
                Q_ASSERT((quintptr)this == (quintptr)&reference && "non C++11 compliant compiler");
                new (&reference) Reference(d.reference);
                break;
            default:
                Q_ASSERT(false && "unexpected kind in inline element");
                break;
            }
        }
        InlineEl(const Empty &o) {
            Q_ASSERT((quintptr)this == (quintptr)&empty && "non C++11 compliant compiler");
            new (&empty) Empty(o);
        }
        InlineEl(const Map &o) {
            Q_ASSERT((quintptr)this == (quintptr)&map && "non C++11 compliant compiler");
            new (&map) Map(o);
        }
        InlineEl(const List &o){
            Q_ASSERT((quintptr)this == (quintptr)&list && "non C++11 compliant compiler");
            new (&list) List(o);
        }
        InlineEl(const ConstantData &o) {
            Q_ASSERT((quintptr)this == (quintptr)&data && "non C++11 compliant compiler");
            new (&data) ConstantData(o);
        }
        InlineEl(const SimpleObjectWrap &o) {
            Q_ASSERT((quintptr)this == (quintptr)&simpleObjectWrap && "non C++11 compliant compiler");
            new (&simpleObjectWrap) SimpleObjectWrap(o);
        }
        InlineEl(const Reference &o) {
            Q_ASSERT((quintptr)this == (quintptr)&reference && "non C++11 compliant compiler");
            new (&reference) Reference(o);
        }
        InlineEl &operator=(const InlineEl &d) {
            Q_ASSERT(this != &d);
            this->~InlineEl(); // destruct & construct new...
            new (this)InlineEl(d);
            return *this;
        }
        DomType kind() const {
            return reinterpret_cast<const DomBase*>(this)->kind();
        }
        ~InlineEl() {
            reinterpret_cast<const DomBase*>(this)->~DomBase();
        }
        Empty empty;
        Map map;
        List list;
        ConstantData data;
        SimpleObjectWrap simpleObjectWrap;
        Reference reference;
    } inlineEl;
};

bool operator ==(const DomItem &o1, const DomItem &o2);
inline bool operator !=(const DomItem &o1, const DomItem &o2) {
    return !(o1 == o2);
}

Q_DECLARE_OPERATORS_FOR_FLAGS(LoadOptions)

class Subpath {
public:
    Path path;
    DomItem item;

    bool visit(function_ref<bool(Path, DomItem &)> visitor){
        return visitor(path, item);
    }
};

template<typename T>
Map Map::fromMultiMapRef(Path pathFromOwner, QMultiMap<QString,T> &mmap, std::function<DomItem(const DomItem &, Path, T&)> elWrapper)
{
    return Map(pathFromOwner, [&mmap, elWrapper](const DomItem &self, QString key) {
        auto it = mmap.find(key);
        auto end = mmap.cend();
        if (it == end)
            return DomItem();
        else {
            QList<T *> values;
            while (it != end && it.key() == key)
                values.append(&(*it++));
            return self.subList(List::fromQList<T*>(self.pathFromOwner().key(key), values, [elWrapper](const DomItem &l, Path p,T * &el) {
                return elWrapper(l,p,*el);
            }, ListOptions::Reverse)).item;
        }
    }, [&mmap](const DomItem&){
        return QSet<QString>(mmap.keyBegin(), mmap.keyEnd());
    }, QLatin1String(typeid(T).name()));
}

template<typename T>
Map Map::fromMapRef(Path pathFromOwner, QMap<QString,T> &map,
                   std::function<DomItem(const DomItem &, Path, T&)> elWrapper)
{
    return Map(pathFromOwner, [&map, elWrapper](const DomItem &self, QString key) {
        if (!map.contains(key))
            return DomItem();
        else {
            return elWrapper(self, Path::Key(key), map[key]);
        }
    }, [&map](const DomItem&){
        return QSet<QString>(map.keyBegin(), map.keyEnd());
    }, QLatin1String(typeid(T).name()));
}

template<typename T>
List List::fromQList(Path pathFromOwner, QList<T> list, std::function<DomItem(const DomItem &, Path, T&)> elWrapper, ListOptions options)
{
    index_type len = list.length();
    if (options == ListOptions::Reverse) {
        return List(
                    pathFromOwner,
                    [list, elWrapper](const DomItem &self, index_type i) mutable {
            if (i < 0 || i >= list.length())
                return DomItem();
            return elWrapper(self, Path::Index(i), list[list.length() -i - 1]);
        }, [len](const DomItem &) {
            return len;
        }, nullptr, QLatin1String(typeid(T).name()));
    } else {
        return List(pathFromOwner,
                    [list, elWrapper](const DomItem &self, index_type i) mutable {
            if (i < 0 || i >= list.length())
                return DomItem();
            return elWrapper(self, Path::Index(i), list[i]);
        }, [len](const DomItem &) {
            return len;
        }, nullptr, QLatin1String(typeid(T).name()));
    }
}

template<typename T>
List List::fromQListRef(Path pathFromOwner, QList<T> &list, std::function<DomItem(const DomItem &, Path, T&)> elWrapper, ListOptions options)
{
    if (options == ListOptions::Reverse) {
        return List(
                    pathFromOwner,
                    [&list, elWrapper](const DomItem &self, index_type i) {
            if (i < 0 || i >= list.length())
                return DomItem();
            return elWrapper(self, Path::Index(i), list[list.length() -i - 1]);
        }, [&list](const DomItem &) {
            return list.length();
        }, nullptr, QLatin1String(typeid(T).name()));
    } else {
        return List(pathFromOwner,
                    [&list, elWrapper](const DomItem &self, index_type i) {
            if (i < 0 || i >= list.length())
                return DomItem();
            return elWrapper(self, Path::Index(i), list[i]);
        }, [&list](const DomItem &) {
            return list.length();
        }, nullptr, QLatin1String(typeid(T).name()));
    }
}

class QMLDOM_EXPORT OwningItem: public DomBase {
protected:
    virtual std::shared_ptr<OwningItem> doCopy(const DomItem &self) const = 0;
public:
    OwningItem(const OwningItem &o);
    OwningItem(int derivedFrom=0);
    OwningItem(int derivedFrom, QDateTime lastDataUpdateAt);
    static int nextRevision();

    Path canonicalPath(const DomItem &self) const override = 0;

    bool iterateDirectSubpaths(DomItem &self, function_ref<bool (Path, DomItem &)>) override;
    std::shared_ptr<OwningItem> makeCopy(const DomItem &self) {
        return doCopy(self);
    }
    Path pathFromOwner(const DomItem &self) const override;
    DomItem containingObject(const DomItem &self) const override;
    int derivedFrom() const;
    virtual int revision() const;

    QDateTime createdAt() const;
    virtual QDateTime lastDataUpdateAt() const;
    virtual void refreshedDataAt(QDateTime tNew);

    // explicit freeze handling needed?
    virtual bool frozen() const;
    virtual bool freeze();
    QDateTime frozenAt() const;

    virtual void addError(const DomItem &self, ErrorMessage msg);
    void addErrorLocal(ErrorMessage msg);
    void clearErrors(ErrorGroups groups = ErrorGroups({}));
    // return false if a quick exit was requested
    bool iterateErrors(const DomItem &self, function_ref<bool(DomItem source, ErrorMessage msg)> visitor, Path inPath = Path());
    QMultiMap<Path, ErrorMessage> localErrors() const {
        QMutexLocker l(mutex());
        return m_errors;
    }


    virtual bool iterateSubOwners(const DomItem &self, function_ref<bool(const DomItem &owner)> visitor);

    QBasicMutex *mutex() const { return &m_mutex; }
private:
    mutable QBasicMutex m_mutex;
    int m_derivedFrom;
    int m_revision;
    QDateTime m_createdAt;
    QDateTime m_lastDataUpdateAt;
    QDateTime m_frozenAt;
    QMultiMap<Path, ErrorMessage> m_errors;
};

template <typename T>
std::shared_ptr<T> DomItem::ownerAs() const {
    if (m_owner && m_owner->kind() == T::kindValue)
        return std::static_pointer_cast<T>(m_owner);
    return nullptr;
}

template <typename T>
SimpleObjectWrap SimpleObjectWrap::fromDataObject(
        Path pathFromOwner, T const & val,
        std::function<QCborValue(T const &)> toData,
        const SourceLocation &loc,
        DomType kind,
        DomKind domKind,
        QString typeName)
{
    QString objectName;
    if (!typeName.isEmpty())
        objectName = typeName;
    else if (kind != kindValue)
        objectName = domTypeToStringMap()[kind];
    else
        objectName = QLatin1String("SimpleObjectWrap<%1>").arg(QLatin1String(typeid(T).name()));
    return SimpleObjectWrap(
        pathFromOwner, QVariant::fromValue(&val),
        [toData, pathFromOwner](DomItem &self, QVariant v, function_ref<bool(Path, DomItem &)> visitor){
            ConstantData data = ConstantData(pathFromOwner, toData(*v.value<T const*>()), ConstantData::Options::FirstMapIsFields);
            return data.iterateDirectSubpaths(self, visitor);
        }, kind, domKind, objectName, loc);
}

template <typename T>
SimpleObjectWrap SimpleObjectWrap::fromObjectRef(
        Path pathFromOwner, T &value,
        std::function<bool(DomItem &, T &val, function_ref<bool(Path, DomItem &)>)> directSubpathsIterate,
        const SourceLocation &loc,
        DomType kind,
        QString typeName,
        DomKind domKind)
{
    return SimpleObjectWrap(
        pathFromOwner, QVariant::fromValue(&value),
        [directSubpathsIterate](DomItem &self, QVariant v, function_ref<bool(Path, DomItem &)> visitor){
            return directSubpathsIterate(self, *v.value<T *>(), visitor);
        },
        kind, domKind,
        ((!typeName.isEmpty()) ? typeName :
         (kind != kindValue) ? domTypeToStringMap()[kind] :
         QStringLiteral(u"SimpleObjectWrap<%1>").arg(QLatin1String(typeid(T).name()))),
        loc);
}

template <typename T>
Subpath DomItem::subWrapPath(Path p, T &obj, SourceLocation loc) const {
    return this->subObjectWrap(SimpleObjectWrap::fromObjectRef<T>(
                this->pathFromOwner().path(p),
                obj,
                [](DomItem &self, T &fDef, function_ref<bool(Path, DomItem &)> visitor) {
        return fDef.iterateDirectSubpaths(self, visitor);
    },loc,
    T::kindValue));
}

template <typename T>
Subpath DomItem::subWrapField(QString p, T &obj, SourceLocation loc) const {
    return this->subWrapPath<T>(Path::Field(p), obj, loc);
}
template <typename T>
Subpath DomItem::subWrapField(QStringView p, T &obj, SourceLocation loc) const {
    return this->subWrapPath<T>(Path::Field(p), obj, loc);
}
template <typename T>
Subpath DomItem::subWrapKey(QString p, T &obj, SourceLocation loc) const {
    return this->subWrapPath<T>(Path::Key(p), obj, loc);
}
template <typename T>
Subpath DomItem::subWrapKey(QStringView p, T &obj, SourceLocation loc) const {
    return this->subWrapPath<T>(Path::Key(p), obj, loc);
}
template <typename T>
Subpath DomItem::subWrapIndex(index_type i, T &obj, SourceLocation loc) const {
    return this->subWrapPath<T>(Path::Index(i), obj, loc);
}

// mainly for debugging purposes
class GenericObject: public DomElement {
public:
    constexpr static DomType kindValue = DomType::GenericObject;
    DomType kind() const override {  return kindValue; }

    GenericObject(Path pathFromOwner = Path(), const SourceLocation & loc = SourceLocation(),
                  QMap<QString, GenericObject> subObjects = {},
                  QMap<QString, QCborValue> subValues = {}):
        DomElement(pathFromOwner, loc), subObjects(subObjects), subValues(subValues) {}

    GenericObject copy() const;
    std::pair<QString, GenericObject> asStringPair() const;

    bool iterateDirectSubpaths(DomItem &self, function_ref<bool (Path, DomItem &)>) override;

    QMap<QString, GenericObject> subObjects;
    QMap<QString, QCborValue> subValues;
};

// mainly for debugging purposes
class GenericOwner: public OwningItem {
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &self) const override;
public:
    constexpr static DomType kindValue = DomType::GenericOwner;
    DomType kind() const override {  return kindValue; }

    GenericOwner(Path pathFromTop = Path(), int derivedFrom = 0,
                 QMap<QString, GenericObject> subObjects = {},
                 QMap<QString, QCborValue> subValues = {}):
        OwningItem(derivedFrom), pathFromTop(pathFromTop), subObjects(subObjects),
        subValues(subValues)
    {}

    GenericOwner(Path pathFromTop, int derivedFrom, QDateTime dataRefreshedAt,
                 QMap<QString, GenericObject> subObjects = {},
                 QMap<QString, QCborValue> subValues = {}):
        OwningItem(derivedFrom, dataRefreshedAt), pathFromTop(pathFromTop), subObjects(subObjects),
        subValues(subValues)
    {}

    GenericOwner(const GenericOwner &o);

    std::shared_ptr<GenericOwner> makeCopy(const DomItem &self);
    Path canonicalPath(const DomItem &self) const override;

    bool iterateDirectSubpaths(DomItem &self, function_ref<bool (Path, DomItem &)>) override;

    Path pathFromTop;
    QMap<QString, GenericObject> subObjects;
    QMap<QString, QCborValue> subValues;
};

QDebug operator<<(QDebug debug, const DomItem &c);


class MutableDomItem {
public:
    operator bool() const {
        return m_owner && base();
    }
    DomType internalKind() const {
        return base().internalKind();
    }
    DomKind domKind() const { return kind2domKind(internalKind()); }

    Path canonicalPath() const
    {
        return m_owner.canonicalPath().path(m_pathFromOwner);
    }
    MutableDomItem containingObject() const {
        if (m_pathFromOwner)
            return MutableDomItem(m_owner, m_pathFromOwner.split().pathToSource);
        else {
            DomItem cObj = m_owner.containingObject();
            return MutableDomItem(cObj.owner(), (domTypeIsOwningItem(cObj.internalKind()) ? Path() :cObj.pathFromOwner()));
        }
    }

    MutableDomItem container() const {
        if (m_pathFromOwner)
            return MutableDomItem(m_owner, m_pathFromOwner.dropTail());
        else {
            return MutableDomItem(base().container());
        }
    }

    MutableDomItem component() const {
        return MutableDomItem{base().component()};
    }
    MutableDomItem owner() const {
        return MutableDomItem(m_owner);
    }
    MutableDomItem top() const {
        return MutableDomItem(base().top());
    }
    MutableDomItem environment() const {
        return MutableDomItem(base().environment());
    }
    MutableDomItem universe() const {
        return MutableDomItem(base().universe());
    }
    Path pathFromOwner() const {
        return m_pathFromOwner;
    }
    MutableDomItem operator[](const Path &path) const {
        return MutableDomItem(base()[path]);
    }
    MutableDomItem operator[](QStringView component) const {
        return MutableDomItem(base()[component]);
    }
    MutableDomItem operator[](const QString &component) const {
        return MutableDomItem(base()[component]);
    }
    MutableDomItem operator[](const char16_t *component) const {
        // to avoid clash with stupid builtin ptrdiff_t[MutableDomItem&], coming from C
        return MutableDomItem(base()[QStringView(component)]);
    }
    MutableDomItem operator[](index_type i) const {
        return MutableDomItem(base().index(i));
    }

    MutableDomItem path(const Path &p) const {
        return MutableDomItem(base().path(p));
    }
    MutableDomItem path(const QString &p) const {
        return path(Path::fromString(p));
    }
    MutableDomItem path(QStringView p) const {
        return path(Path::fromString(p));
    }

    QList<QString> const fields() const {
        return base().fields();
    }
    MutableDomItem field(QStringView name) const {
        return MutableDomItem(base().field(name));
    }
    index_type indexes() const {
        return base().indexes();
    }
    MutableDomItem index(index_type i) const {
        return MutableDomItem(base().index(i));
    }

    QSet<QString> const keys() const {
        return base().keys();
    }
    MutableDomItem key(QString name) const {
        return MutableDomItem(base().key(name));
    }

    QString canonicalFilePath() const { return base().canonicalFilePath(); }
    SourceLocation location() const { return base().location(); }

    QCborValue value() const {
        return base().value();
    }

    void dump(Sink sink, int indent = 0) const {
        return base().dump(sink, indent);
    }
    QString toString() const {
        return base().toString();
    }

    // convenience getters
    QString name() const;
    MutableDomItem qmlChildren() const {
        return MutableDomItem(base().qmlChildren());
    }
    MutableDomItem annotations() const {
        return MutableDomItem(base().annotations());
    }

    QMultiMap<QString, RequiredProperty> extraRequired() const;


//    // OwnigItem elements
    int derivedFrom() const {
        return m_owner.derivedFrom();
    }
    int revision() const {
        return m_owner.revision();
    }
    QDateTime createdAt() const {
        return m_owner.createdAt();
    }
    QDateTime frozenAt() const {
        return m_owner.frozenAt();
    }
    QDateTime lastDataUpdateAt() const {
        return m_owner.lastDataUpdateAt();
    }

    void addError(ErrorMessage msg) const {
        base().addError(msg);
    }
    ErrorHandler errorHandler() const;

    MutableDomItem() = default;
    MutableDomItem(DomItem owner, Path pathFromOwner):
        m_owner(owner), m_pathFromOwner(pathFromOwner)
    {}
    MutableDomItem(DomItem item):
        m_owner(item.owner()), m_pathFromOwner(item.pathFromOwner())
    {}

    std::shared_ptr<DomTop> topPtr() const {
        return m_owner.topPtr();
    }
    std::shared_ptr<OwningItem> owningItemPtr() const {
        return m_owner.owningItemPtr();
    }

    template <typename T>
    T const*as() const {
        return base().as<T>();
    }

    template <typename T>
    T *mutableAs() {
        Q_ASSERT(!m_owner.owningItemPtr()->frozen());
        return base().mutableAs<T>();
    }

    template <typename T>
    std::shared_ptr<T> ownerAs() const {
        return m_owner.ownerAs<T>();
    }
    // it is dangerous to assume it stays valid when updates are preformed...
    DomItem base() const {
        return m_owner.path(m_pathFromOwner);
    }
private:
    DomItem m_owner;
    Path m_pathFromOwner;
};

QDebug operator<<(QDebug debug, const MutableDomItem &c);

template <typename K, typename T>
Path insertUpdatableElementInMultiMap(Path mapPathFromOwner, QMultiMap<K, T> &mmap, K key, const T&value) {
    mmap.insert(key, value);
    auto it = mmap.find(key);
    auto it2 = it;
    int nVal = 0;
    while (it2 != mmap.end() && it2.key() == key) {
       ++nVal;
        ++it2;
    }
    Path newPath = mapPathFromOwner.key(key).index(nVal-1);
    T &newComp = *it;
    newComp.updatePathFromOwner(newPath);
    return newPath;
}

template <typename T>
Path appendUpdatableElementInQList(Path listPathFromOwner, QList<T> &list, const T&value) {
    int idx = list.length();
    list.append(value);
    Path newPath = listPathFromOwner.index(idx);
    list[idx].updatePathFromOwner(newPath);
    return newPath;
}


template <typename T, typename K = QString>
void updatePathFromOwnerMultiMap(QMultiMap<K, T> &mmap, Path newPath)
{
    auto it = mmap.begin();
    auto end = mmap.end();
    index_type i = 0;
    K name;
    QList<T*> els;
    while (it != end) {
        if (i > 0 && name != it.key()) {
            Path pName = newPath.key(QString(name));
            foreach (T *el, els)
                el->updatePathFromOwner(pName.index(--i));
            els.clear();
            els.append(&(*it));
            name = it.key();
            i = 1;
        } else {
            els.append(&(*it));
            name = it.key();
            ++i;
        }
        ++it;
    }
    Path pName = newPath.key(name);
    foreach (T *el, els)
        el->updatePathFromOwner(pName.index(--i));
}

template <typename T>
void updatePathFromOwnerQList(QList<T> &list, Path newPath)
{
    auto it = list.begin();
    auto end = list.end();
    index_type i = 0;
    while (it != end)
        (it++)->updatePathFromOwner(newPath.index(i++));
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QMLDOMITEM_H
