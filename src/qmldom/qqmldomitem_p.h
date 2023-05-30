// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include "qqmldomstringdumper_p.h"
#include "qqmldompath_p.h"
#include "qqmldomerrormessage_p.h"
#include "qqmldomfunctionref_p.h"
#include "qqmldomfilewriter_p.h"
#include "qqmldomlinewriter_p.h"

#include <QtCore/QMap>
#include <QtCore/QMultiMap>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringView>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QMutex>
#include <QtCore/QCborValue>
#include <QtCore/QTimeZone>
#include <QtQml/private/qqmljssourcelocation_p.h>
#include <QtQmlCompiler/private/qqmljsscope_p.h>

#include <memory>
#include <typeinfo>
#include <utility>
#include <type_traits>
#include <variant>
#include <optional>
#include <cstddef>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
// we didn't have enough 'O's to properly name everything...
namespace Dom {

class Path;

Q_DECLARE_LOGGING_CATEGORY(writeOutLog);

constexpr bool domTypeIsObjWrap(DomType k);
constexpr bool domTypeIsValueWrap(DomType k);
constexpr bool domTypeIsDomElement(DomType);
constexpr bool domTypeIsOwningItem(DomType);
constexpr bool domTypeIsUnattachedOwningItem(DomType);
constexpr bool domTypeIsScriptElement(DomType);
QMLDOM_EXPORT bool domTypeIsExternalItem(DomType k);
QMLDOM_EXPORT bool domTypeIsTopItem(DomType k);
QMLDOM_EXPORT bool domTypeIsContainer(DomType k);
constexpr bool domTypeCanBeInline(DomType k)
{
    switch (k) {
    case DomType::Empty:
    case DomType::Map:
    case DomType::List:
    case DomType::ListP:
    case DomType::ConstantData:
    case DomType::SimpleObjectWrap:
    case DomType::ScriptElementWrap:
    case DomType::Reference:
        return true;
    default:
        return false;
    }
}
QMLDOM_EXPORT bool domTypeIsScope(DomType k);

QMLDOM_EXPORT QMap<DomType,QString> domTypeToStringMap();
QMLDOM_EXPORT QString domTypeToString(DomType k);
QMLDOM_EXPORT QMap<DomKind, QString> domKindToStringMap();
QMLDOM_EXPORT QString domKindToString(DomKind k);

QMLDOM_EXPORT QCborValue locationToData(SourceLocation loc, QStringView strValue=u"");

inline bool noFilter(DomItem &, const PathEls::PathComponent &, DomItem &)
{
    return true;
}

using DirectVisitor = function_ref<bool(const PathEls::PathComponent &, function_ref<DomItem()>)>;
// using DirectVisitor = function_ref<bool(Path, DomItem &)>;

namespace {
template<typename T>
struct IsMultiMap : std::false_type
{
};

template<typename Key, typename T>
struct IsMultiMap<QMultiMap<Key, T>> : std::true_type
{
};

template<typename T>
struct IsMap : std::false_type
{
};

template<typename Key, typename T>
struct IsMap<QMap<Key, T>> : std::true_type
{
};

template<typename... Ts>
using void_t = void;

template<typename T, typename = void>
struct IsDomObject : std::false_type
{
};

template<typename T>
struct IsDomObject<T, void_t<decltype(T::kindValue)>> : std::true_type
{
};

template<typename T, typename = void>
struct IsInlineDom : std::false_type
{
};

template<typename T>
struct IsInlineDom<T, void_t<decltype(T::kindValue)>>
    : std::integral_constant<bool, domTypeCanBeInline(T::kindValue)>
{
};

template<typename T>
struct IsInlineDom<T *, void_t<decltype(T::kindValue)>> : std::true_type
{
};

template<typename T>
struct IsInlineDom<std::shared_ptr<T>, void_t<decltype(T::kindValue)>> : std::true_type
{
};

template<typename T>
struct IsSharedPointerToDomObject : std::false_type
{
};

template<typename T>
struct IsSharedPointerToDomObject<std::shared_ptr<T>> : IsDomObject<T>
{
};

template<typename T, typename = void>
struct IsList : std::false_type
{
};

template<typename T>
struct IsList<T, void_t<typename T::value_type>> : std::true_type
{
};

}

template<typename T>
union SubclassStorage {
    int i;
    T lp;
    T *data() { return reinterpret_cast<T *>(this); }
    const T *data() const { return reinterpret_cast<const T *>(this); }
    SubclassStorage() { }
    SubclassStorage(T &&el) { el.moveTo(data()); }
    SubclassStorage(const T *el) { el->copyTo(data()); }
    SubclassStorage(const SubclassStorage &o) : SubclassStorage(o.data()) { }
    SubclassStorage(const SubclassStorage &&o) : SubclassStorage(o.data()) { }
    SubclassStorage &operator=(const SubclassStorage &o)
    {
        data()->~T();
        o.data()->copyTo(data());
        return *this;
    }
    ~SubclassStorage() { data()->~T(); }
};

class QMLDOM_EXPORT DomBase
{
public:
    using FilterT = function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)>;

    virtual ~DomBase() = default;

    DomBase *domBase() { return static_cast<DomBase *>(this); }

    // minimal overload set:
    virtual DomType kind() const = 0;
    virtual DomKind domKind() const;
    virtual Path pathFromOwner(DomItem &self) const = 0;
    virtual Path canonicalPath(DomItem &self) const = 0;
    virtual bool
    iterateDirectSubpaths(DomItem &self,
                          DirectVisitor visitor) = 0; // iterates the *direct* subpaths, returns
                                                      // false if a quick end was requested
    bool iterateDirectSubpathsConst(DomItem &self, DirectVisitor)
            const; // iterates the *direct* subpaths, returns false if a quick end was requested

    virtual DomItem containingObject(
            DomItem &self) const; // the DomItem corresponding to the canonicalSource source
    virtual void dump(DomItem &, Sink sink, int indent, FilterT filter) const;
    virtual quintptr id() const;
    QString typeName() const;

    virtual QList<QString> fields(DomItem &self) const;
    virtual DomItem field(DomItem &self, QStringView name) const;

    virtual index_type indexes(DomItem &self) const;
    virtual DomItem index(DomItem &self, index_type index) const;

    virtual QSet<QString> const keys(DomItem &self) const;
    virtual DomItem key(DomItem &self, QString name) const;

    virtual QString canonicalFilePath(DomItem &self) const;

    virtual void writeOut(DomItem &self, OutWriter &lw) const;

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
    case DomType::ListP:
        return DomKind::List;
    case DomType::Map:
        return DomKind::Map;
    case DomType::ConstantData:
        return DomKind::Value;
    default:
        return DomKind::Object;
    }
}

class QMLDOM_EXPORT Empty final : public DomBase
{
public:
    constexpr static DomType kindValue = DomType::Empty;
    DomType kind() const override {  return kindValue; }

    Empty *operator->() { return this; }
    const Empty *operator->() const { return this; }
    Empty &operator*() { return *this; }
    const Empty &operator*() const { return *this; }

    Empty();
    quintptr id() const override { return ~quintptr(0); }
    Path pathFromOwner(DomItem &self) const override;
    Path canonicalPath(DomItem &self) const override;
    DomItem containingObject(DomItem &self) const override;
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor) override;
    void dump(DomItem &, Sink s, int indent,
              function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)> filter)
            const override;
};

class QMLDOM_EXPORT DomElement: public DomBase {
protected:
    DomElement& operator=(const DomElement&) = default;
public:
    DomElement(Path pathFromOwner = Path());
    DomElement(const DomElement &o) = default;
    Path pathFromOwner(DomItem &self) const override;
    Path pathFromOwner() const { return m_pathFromOwner; }
    Path canonicalPath(DomItem &self) const override;
    DomItem containingObject(DomItem &self) const override;
    virtual void updatePathFromOwner(Path newPath);

private:
    Path m_pathFromOwner;
};

class QMLDOM_EXPORT Map final : public DomElement
{
public:
    constexpr static DomType kindValue = DomType::Map;
    DomType kind() const override {  return kindValue; }

    Map *operator->() { return this; }
    const Map *operator->() const { return this; }
    Map &operator*() { return *this; }
    const Map &operator*() const { return *this; }

    using LookupFunction = std::function<DomItem(DomItem &, QString)>;
    using Keys = std::function<QSet<QString>(DomItem &)>;
    Map(Path pathFromOwner, LookupFunction lookup, Keys keys, QString targetType);
    quintptr id() const override;
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor) override;
    QSet<QString> const keys(DomItem &self) const override;
    DomItem key(DomItem &self, QString name) const override;

    template<typename T>
    static Map fromMultiMapRef(Path pathFromOwner, QMultiMap<QString, T> &mmap);
    template<typename T>
    static Map
    fromMapRef(Path pathFromOwner, QMap<QString, T> &mmap,
               std::function<DomItem(DomItem &, const PathEls::PathComponent &, T &)> elWrapper);

private:
    LookupFunction m_lookup;
    Keys m_keys;
    QString m_targetType;
};

class QMLDOM_EXPORT List final : public DomElement
{
public:
    constexpr static DomType kindValue = DomType::List;
    DomType kind() const override {  return kindValue; }

    List *operator->() { return this; }
    const List *operator->() const { return this; }
    List &operator*() { return *this; }
    const List &operator*() const { return *this; }

    using LookupFunction = std::function<DomItem(DomItem &, index_type)>;
    using Length = std::function<index_type(DomItem &)>;
    using IteratorFunction =
            std::function<bool(DomItem &, function_ref<bool(index_type, function_ref<DomItem()>)>)>;

    List(Path pathFromOwner, LookupFunction lookup, Length length, IteratorFunction iterator, QString elType);
    quintptr id() const override;
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor) override;
    void
    dump(DomItem &, Sink s, int indent,
         function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)>) const override;
    index_type indexes(DomItem &self) const override;
    DomItem index(DomItem &self, index_type index) const override;

    template<typename T>
    static List
    fromQList(Path pathFromOwner, QList<T> list,
              std::function<DomItem(DomItem &, const PathEls::PathComponent &, T &)> elWrapper,
              ListOptions options = ListOptions::Normal);
    template<typename T>
    static List
    fromQListRef(Path pathFromOwner, QList<T> &list,
                 std::function<DomItem(DomItem &, const PathEls::PathComponent &, T &)> elWrapper,
                 ListOptions options = ListOptions::Normal);
    void writeOut(DomItem &self, OutWriter &ow, bool compact) const;
    void writeOut(DomItem &self, OutWriter &ow) const override { writeOut(self, ow, true); }

private:
    LookupFunction m_lookup;
    Length m_length;
    IteratorFunction m_iterator;
    QString m_elType;
};

class QMLDOM_EXPORT ListPBase : public DomElement
{
public:
    constexpr static DomType kindValue = DomType::ListP;
    DomType kind() const override { return kindValue; }

    ListPBase(Path pathFromOwner, const QList<void *> &pList, QString elType)
        : DomElement(pathFromOwner), m_pList(pList), m_elType(elType)
    {
    }
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor v) override;
    virtual void copyTo(ListPBase *) const { Q_ASSERT(false); };
    virtual void moveTo(ListPBase *) const { Q_ASSERT(false); };
    quintptr id() const override { return quintptr(0); }
    index_type indexes(DomItem &) const override { return index_type(m_pList.size()); }
    void writeOut(DomItem &self, OutWriter &ow, bool compact) const;
    void writeOut(DomItem &self, OutWriter &ow) const override { writeOut(self, ow, true); }

protected:
    QList<void *> m_pList;
    QString m_elType;
};

template<typename T>
class ListPT final : public ListPBase
{
public:
    constexpr static DomType kindValue = DomType::ListP;

    ListPT(Path pathFromOwner, QList<T *> pList, QString elType = QString(),
           ListOptions options = ListOptions::Normal)
        : ListPBase(pathFromOwner, {},
                    (elType.isEmpty() ? QLatin1String(typeid(T).name()) : elType))
    {
        static_assert(sizeof(ListPBase) == sizeof(ListPT),
                      "ListPT does not have the same size as ListPBase");
        static_assert(alignof(ListPBase) == alignof(ListPT),
                      "ListPT does not have the same size as ListPBase");
        m_pList.reserve(pList.size());
        if (options == ListOptions::Normal) {
            for (void *p : pList)
                m_pList.append(p);
        } else if (options == ListOptions::Reverse) {
            for (qsizetype i = pList.size(); i-- != 0;)
                // probably writing in reverse and reading sequentially would be better
                m_pList.append(pList.at(i));
        } else {
            Q_ASSERT(false);
        }
    }
    void copyTo(ListPBase *t) const override { new (t) ListPT(*this); }
    void moveTo(ListPBase *t) const override { new (t) ListPT(std::move(*this)); }
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor v) override;

    DomItem index(DomItem &self, index_type index) const override;
};

class QMLDOM_EXPORT ListP
{
public:
    constexpr static DomType kindValue = DomType::ListP;
    template<typename T>
    ListP(Path pathFromOwner, QList<T *> pList, QString elType = QString(),
          ListOptions options = ListOptions::Normal)
        : list(ListPT<T>(pathFromOwner, pList, elType, options))
    {
    }
    ListP() = delete;

    ListPBase *operator->() { return list.data(); }
    const ListPBase *operator->() const { return list.data(); }
    ListPBase &operator*() { return *list.data(); }
    const ListPBase &operator*() const { return *list.data(); }

private:
    SubclassStorage<ListPBase> list;
};

class QMLDOM_EXPORT ConstantData final : public DomElement
{
public:
    constexpr static DomType kindValue = DomType::ConstantData;
    DomType kind() const override { return kindValue; }

    enum class Options {
        MapIsMap,
        FirstMapIsFields
    };

    ConstantData *operator->() { return this; }
    const ConstantData *operator->() const { return this; }
    ConstantData &operator*() { return *this; }
    const ConstantData &operator*() const { return *this; }

    ConstantData(Path pathFromOwner, QCborValue value, Options options = Options::MapIsMap);
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor) override;
    quintptr id() const override;
    DomKind domKind() const override;
    QCborValue value() const override { return m_value; }
    Options options() const { return m_options; }
private:
    QCborValue m_value;
    Options m_options;
};

class QMLDOM_EXPORT SimpleObjectWrapBase : public DomElement
{
public:
    constexpr static DomType kindValue = DomType::SimpleObjectWrap;
    DomType kind() const final override { return m_kind; }

    quintptr id() const final override { return m_id; }
    DomKind domKind() const final override { return m_domKind; }

    template <typename T>
    T const *as() const
    {
        if (m_options & SimpleWrapOption::ValueType) {
            if (m_value.metaType() == QMetaType::fromType<T>())
                return reinterpret_cast<const T *>(m_value.constData());
            return nullptr;
        } else {
            return m_value.value<T *>();
        }
    }

    template <typename T>
    T *mutableAs()
    {
        if (m_options & SimpleWrapOption::ValueType) {
            if (m_value.metaType() == QMetaType::fromType<T>())
                return reinterpret_cast<T *>(m_value.data());
            return nullptr;
        } else {
            return m_value.value<T *>();
        }
    }

    SimpleObjectWrapBase() = delete;
    virtual void copyTo(SimpleObjectWrapBase *) const { Q_ASSERT(false); }
    virtual void moveTo(SimpleObjectWrapBase *) const { Q_ASSERT(false); }
    bool iterateDirectSubpaths(DomItem &, DirectVisitor) override
    {
        Q_ASSERT(false);
        return true;
    }

protected:
    friend class TestDomItem;
    SimpleObjectWrapBase(Path pathFromOwner, QVariant value, quintptr idValue,
                         DomType kind = kindValue,
                         SimpleWrapOptions options = SimpleWrapOption::None)
        : DomElement(pathFromOwner),
          m_kind(kind),
          m_domKind(kind2domKind(kind)),
          m_value(value),
          m_id(idValue),
          m_options(options)
    {
    }

    DomType m_kind;
    DomKind m_domKind;
    QVariant m_value;
    quintptr m_id;
    SimpleWrapOptions m_options;
};

template<typename T>
class SimpleObjectWrapT final : public SimpleObjectWrapBase
{
public:
    constexpr static DomType kindValue = DomType::SimpleObjectWrap;

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override
    {
        return mutableAsT()->iterateDirectSubpaths(self, visitor);
    }

    void writeOut(DomItem &self, OutWriter &lw) const override;

    T const *asT() const
    {
        if constexpr (domTypeIsValueWrap(T::kindValue)) {
            if (m_value.metaType() == QMetaType::fromType<T>())
                return reinterpret_cast<const T *>(m_value.constData());
            return nullptr;
        } else if constexpr (domTypeIsObjWrap(T::kindValue)) {
            return m_value.value<T *>();
        } else {
            // need dependent static assert to not unconditially trigger
            static_assert(!std::is_same_v<T, T>, "wrapping of unexpected type");
            return nullptr; // necessary to avoid warnings on INTEGRITY
        }
    }

    T *mutableAsT()
    {
        if (domTypeIsValueWrap(T::kindValue)) {
            if (m_value.metaType() == QMetaType::fromType<T>())
                return reinterpret_cast<T *>(m_value.data());
            return nullptr;
        } else if constexpr (domTypeIsObjWrap(T::kindValue)) {
            return m_value.value<T *>();
        } else {
            Q_ASSERT_X(false, "SimpleObjectWrap", "wrapping of unexpected type");
            return nullptr;
        }
    }

    void copyTo(SimpleObjectWrapBase *target) const override
    {
        static_assert(sizeof(SimpleObjectWrapBase) == sizeof(SimpleObjectWrapT),
                      "Size mismatch in SimpleObjectWrapT");
        static_assert(alignof(SimpleObjectWrapBase) == alignof(SimpleObjectWrapT),
                      "Size mismatch in SimpleObjectWrapT");
        new (target) SimpleObjectWrapT(*this);
    }

    void moveTo(SimpleObjectWrapBase *target) const override
    {
        static_assert(sizeof(SimpleObjectWrapBase) == sizeof(SimpleObjectWrapT),
                      "Size mismatch in SimpleObjectWrapT");
        static_assert(alignof(SimpleObjectWrapBase) == alignof(SimpleObjectWrapT),
                      "Size mismatch in SimpleObjectWrapT");
        new (target) SimpleObjectWrapT(std::move(*this));
    }

    SimpleObjectWrapT(Path pathFromOwner, QVariant v, quintptr idValue, SimpleWrapOptions o)
        : SimpleObjectWrapBase(pathFromOwner, v, idValue, T::kindValue, o)
    {
        Q_ASSERT(domTypeIsValueWrap(T::kindValue) == bool(o & SimpleWrapOption::ValueType));
    }
};

class QMLDOM_EXPORT SimpleObjectWrap
{
public:
    constexpr static DomType kindValue = DomType::SimpleObjectWrap;

    SimpleObjectWrapBase *operator->() { return wrap.data(); }
    const SimpleObjectWrapBase *operator->() const { return wrap.data(); }
    SimpleObjectWrapBase &operator*() { return *wrap.data(); }
    const SimpleObjectWrapBase &operator*() const { return *wrap.data(); }

    template<typename T>
    static SimpleObjectWrap fromObjectRef(Path pathFromOwner, T &value)
    {
        return SimpleObjectWrap(pathFromOwner, value);
    }
    SimpleObjectWrap() = delete;

private:
    template<typename T>
    SimpleObjectWrap(Path pathFromOwner, T &value)
    {
        using BaseT = std::decay_t<T>;
        if constexpr (domTypeIsObjWrap(BaseT::kindValue)) {
            new (wrap.data()) SimpleObjectWrapT<BaseT>(pathFromOwner, QVariant::fromValue(&value),
                                                       quintptr(&value), SimpleWrapOption::None);
        } else if constexpr (domTypeIsValueWrap(BaseT::kindValue)) {
            new (wrap.data()) SimpleObjectWrapT<BaseT>(pathFromOwner, QVariant::fromValue(value),
                                                       quintptr(0), SimpleWrapOption::ValueType);
        } else {
            qCWarning(domLog) << "Unexpected object to wrap in SimpleObjectWrap: "
                              << domTypeToString(BaseT::kindValue);
            Q_ASSERT_X(false, "SimpleObjectWrap",
                       "simple wrap of unexpected object"); // allow? (mocks for testing,...)
            new (wrap.data())
                    SimpleObjectWrapT<BaseT>(pathFromOwner, nullptr, 0, SimpleWrapOption::None);
        }
    }
    SubclassStorage<SimpleObjectWrapBase> wrap;
};

class QMLDOM_EXPORT Reference final : public DomElement
{
    Q_GADGET
public:
    constexpr static DomType kindValue = DomType::Reference;
    DomType kind() const override {  return kindValue; }

    Reference *operator->() { return this; }
    const Reference *operator->() const { return this; }
    Reference &operator*() { return *this; }
    const Reference &operator*() const { return *this; }

    bool shouldCache() const;
    Reference(Path referredObject = Path(), Path pathFromOwner = Path(), const SourceLocation & loc = SourceLocation());
    quintptr id() const override;
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor) override;
    DomItem field(DomItem &self, QStringView name) const override;
    QList<QString> fields(DomItem &self) const override;
    index_type indexes(DomItem &) const override { return 0; }
    DomItem index(DomItem &, index_type) const override;
    QSet<QString> const keys(DomItem &) const override { return {}; }
    DomItem key(DomItem &, QString) const override;

    DomItem get(DomItem &self, ErrorHandler h = nullptr, QList<Path> *visitedRefs = nullptr) const;
    QList<DomItem> getAll(DomItem &self, ErrorHandler h = nullptr,
                          QList<Path> *visitedRefs = nullptr) const;

    Path referredObjectPath;
};

template<typename Info>
class AttachedInfoT;
class FileLocations;

/*!
    \internal
    \brief A common base class for all the script elements.

    This marker class allows to use all the script elements as a ScriptElement*, using virtual
   dispatch. For now, it does not add any extra functionality, compared to a DomElement, but allows
   to forbid DomElement* at the places where only script elements are required.
 */
// TODO: do we need another marker struct like this one to differentiate expressions from
// statements? This would allow to avoid mismatchs between script expressions and script statements,
// using type-safety.
struct ScriptElement : public DomElement
{
    template<typename T>
    using PointerType = std::shared_ptr<T>;

    using DomElement::DomElement;
    virtual void createFileLocations(std::shared_ptr<AttachedInfoT<FileLocations>> fileLocationOfOwner) = 0;

    std::optional<QQmlJSScope::Ptr> semanticScope();
    void setSemanticScope(const QQmlJSScope::Ptr &scope);

private:
    std::optional<QQmlJSScope::Ptr> m_scope;
};

/*!
   \internal
   \brief Use this to contain any script element.
 */
class ScriptElementVariant
{
private:
    template<typename... T>
    using VariantOfPointer = std::variant<ScriptElement::PointerType<T>...>;

    template<typename T, typename Variant>
    struct TypeIsInVariant;

    template<typename T, typename... Ts>
    struct TypeIsInVariant<T, std::variant<Ts...>> : public std::disjunction<std::is_same<T, Ts>...>
    {
    };

public:
    using ScriptElementT =
            VariantOfPointer<ScriptElements::BlockStatement, ScriptElements::IdentifierExpression,
                             ScriptElements::ForStatement, ScriptElements::BinaryExpression,
                             ScriptElements::VariableDeclarationEntry, ScriptElements::Literal,
                             ScriptElements::IfStatement, ScriptElements::GenericScriptElement,
                             ScriptElements::VariableDeclaration, ScriptElements::ReturnStatement>;

    template<typename T>
    static ScriptElementVariant fromElement(T element)
    {
        static_assert(TypeIsInVariant<T, ScriptElementT>::value,
                      "Cannot construct ScriptElementVariant from T, as it is missing from the "
                      "ScriptElementT.");
        ScriptElementVariant p;
        p.m_data = element;
        return p;
    }

    /*!
    \internal
    \brief Returns a pointer to the virtual base for virtual method calls.

    A helper to call virtual methods without having to call std::visit(...).
    */
    ScriptElement::PointerType<ScriptElement> base() const
    {
        if (m_data)
            return std::visit(
                    [](auto &&e) {
                        // std::reinterpret_pointer_cast does not exist on qnx it seems...
                        return std::shared_ptr<ScriptElement>(
                                e, reinterpret_cast<ScriptElement *>(e.get()));
                    },
                    *m_data);
        return nullptr;
    }

    operator bool() const { return m_data.has_value(); }

    template<typename F>
    void visitConst(F &&visitor) const
    {
        if (m_data)
            std::visit(visitor, *m_data);
    }

    template<typename F>
    void visit(F &&visitor)
    {
        if (m_data)
            std::visit(visitor, *m_data);
    }
    std::optional<ScriptElementT> data() { return m_data; }
    void setData(ScriptElementT data) { m_data = data; }

private:
    std::optional<ScriptElementT> m_data;
};

/*!
    \internal

    To avoid cluttering the already unwieldy \l ElementT type below with all the types that the
   different script elements can have, wrap them in an extra class. It will behave like an internal
   Dom structure (e.g. like a List or a Map) and contain a pointer the the script element.
 */
class ScriptElementDomWrapper
{
public:
    ScriptElementDomWrapper(const ScriptElementVariant &element) : m_element(element) { }

    static constexpr DomType kindValue = DomType::ScriptElementWrap;

    DomBase *operator->() { return m_element.base().get(); }
    const DomBase *operator->() const { return m_element.base().get(); }
    DomBase &operator*() { return *m_element.base(); }
    const DomBase &operator*() const { return *m_element.base(); }

    ScriptElementVariant element() { return m_element; }

private:
    ScriptElementVariant m_element;
};

// TODO: create more "groups" to simplify this variant? Maybe into Internal, ScriptExpression, ???
using ElementT =
        std::variant<Empty, Map, List, ListP, ConstantData, SimpleObjectWrap, Reference,
                     ScriptElementDomWrapper, GlobalComponent *, JsResource *, QmlComponent *,
                     QmltypesComponent *, EnumDecl *, MockObject *, ModuleScope *, AstComments *,
                     AttachedInfo *, DomEnvironment *, DomUniverse *, ExternalItemInfoBase *,
                     ExternalItemPairBase *, GlobalScope *, JsFile *, QmlDirectory *, QmlFile *,
                     QmldirFile *, QmlObject *, QmltypesFile *, LoadInfo *, MockOwner *,
                     ModuleIndex *, ScriptExpression *>;

using TopT = std::variant<std::shared_ptr<DomEnvironment>, std::shared_ptr<DomUniverse>>;

using OwnerT =
        std::variant<std::shared_ptr<ModuleIndex>, std::shared_ptr<MockOwner>,
                     std::shared_ptr<ExternalItemInfoBase>, std::shared_ptr<ExternalItemPairBase>,
                     std::shared_ptr<QmlDirectory>, std::shared_ptr<QmldirFile>,
                     std::shared_ptr<JsFile>, std::shared_ptr<QmlFile>,
                     std::shared_ptr<QmltypesFile>, std::shared_ptr<GlobalScope>,
                     std::shared_ptr<ScriptExpression>, std::shared_ptr<AstComments>,
                     std::shared_ptr<LoadInfo>, std::shared_ptr<AttachedInfo>,
                     std::shared_ptr<DomEnvironment>, std::shared_ptr<DomUniverse>>;

inline bool emptyChildrenVisitor(Path, DomItem &, bool)
{
    return true;
}

class MutableDomItem;

enum DomCreationOption : char {
    None = 0,
    WithSemanticAnalysis = 1,
    WithScriptExpressions = 2,
};

Q_DECLARE_FLAGS(DomCreationOptions, DomCreationOption);

class FileToLoad
{
public:
    struct InMemoryContents
    {
        QString data;
        QDateTime date = QDateTime::currentDateTimeUtc();
    };

    FileToLoad(const std::weak_ptr<DomEnvironment> &environment, const QString &canonicalPath,
               const QString &logicalPath, std::optional<InMemoryContents> content,
               DomCreationOptions options);
    FileToLoad() = default;

    static FileToLoad fromMemory(const std::weak_ptr<DomEnvironment> &environment,
                                 const QString &path, const QString &data,
                                 DomCreationOptions options = None);
    static FileToLoad fromFileSystem(const std::weak_ptr<DomEnvironment> &environment,
                                     const QString &canonicalPath,
                                     DomCreationOptions options = None);

    std::weak_ptr<DomEnvironment> environment() const { return m_environment; }
    QString canonicalPath() const { return m_canonicalPath; }
    QString logicalPath() const { return m_logicalPath; }
    std::optional<InMemoryContents> content() const { return m_content; }
    DomCreationOptions options() const { return m_options; }

private:
    std::weak_ptr<DomEnvironment> m_environment;
    QString m_canonicalPath;
    QString m_logicalPath;
    std::optional<InMemoryContents> m_content;
    DomCreationOptions m_options;
};

class QMLDOM_EXPORT DomItem {
    Q_DECLARE_TR_FUNCTIONS(DomItem);
public:
    using Callback = function<void(Path, DomItem &, DomItem &)>;

    using InternalKind = DomType;
    using Visitor = function_ref<bool(Path, DomItem &)>;
    using ChildrenVisitor = function_ref<bool(Path, DomItem &, bool)>;

    static ErrorGroup domErrorGroup;
    static ErrorGroups myErrors();
    static ErrorGroups myResolveErrors();
    static DomItem empty;

    enum class CopyOption { EnvConnected, EnvDisconnected };

    template<typename F>
    auto visitMutableEl(F f)
    {
        return std::visit(f, this->m_element);
    }
    template<typename F>
    auto visitEl(F f)
    {
        return std::visit(f, this->m_element);
    }

    explicit operator bool() const { return m_kind != DomType::Empty; }
    InternalKind internalKind() const {
        return m_kind;
    }
    QString internalKindStr() const { return domTypeToString(internalKind()); }
    DomKind domKind() const
    {
        if (m_kind == DomType::ConstantData)
            return std::get<ConstantData>(m_element).domKind();
        else
            return kind2domKind(m_kind);
    }

    Path canonicalPath();

    DomItem filterUp(function_ref<bool(DomType k, DomItem &)> filter, FilterUpOptions options);
    DomItem containingObject();
    DomItem container();
    DomItem owner();
    DomItem top();
    DomItem environment();
    DomItem universe();
    DomItem containingFile();
    DomItem goToFile(const QString &filePath);
    DomItem goUp(int);
    DomItem directParent();

    DomItem qmlObject(GoTo option = GoTo::Strict,
                      FilterUpOptions options = FilterUpOptions::ReturnOuter);
    DomItem fileObject(GoTo option = GoTo::Strict);
    DomItem rootQmlObject(GoTo option = GoTo::Strict);
    DomItem globalScope();
    DomItem component(GoTo option = GoTo::Strict);
    DomItem scope(FilterUpOptions options = FilterUpOptions::ReturnOuter);
    std::optional<QQmlJSScope::Ptr> nearestSemanticScope();
    std::optional<QQmlJSScope::Ptr> semanticScope();

    // convenience getters
    DomItem get(ErrorHandler h = nullptr, QList<Path> *visitedRefs = nullptr);
    QList<DomItem> getAll(ErrorHandler h = nullptr, QList<Path> *visitedRefs = nullptr);
    bool isOwningItem() { return domTypeIsOwningItem(internalKind()); }
    bool isExternalItem() { return domTypeIsExternalItem(internalKind()); }
    bool isTopItem() { return domTypeIsTopItem(internalKind()); }
    bool isContainer() { return domTypeIsContainer(internalKind()); }
    bool isScope() { return domTypeIsScope(internalKind()); }
    bool isCanonicalChild(DomItem &child);
    bool hasAnnotations();
    QString name() { return field(Fields::name).value().toString(); }
    DomItem pragmas() { return field(Fields::pragmas); }
    DomItem ids() { return field(Fields::ids); }
    QString idStr() { return field(Fields::idStr).value().toString(); }
    DomItem propertyInfos() { return field(Fields::propertyInfos); }
    PropertyInfo propertyInfoWithName(QString name);
    QSet<QString> propertyInfoNames();
    DomItem propertyDefs() { return field(Fields::propertyDefs); }
    DomItem bindings() { return field(Fields::bindings); }
    DomItem methods() { return field(Fields::methods); }
    DomItem enumerations() { return field(Fields::enumerations); }
    DomItem children() { return field(Fields::children); }
    DomItem child(index_type i) { return field(Fields::children).index(i); }
    DomItem annotations()
    {
        if (hasAnnotations())
            return field(Fields::annotations);
        else
            return DomItem();
    }

    bool resolve(Path path, Visitor visitor, ErrorHandler errorHandler,
                 ResolveOptions options = ResolveOption::None, Path fullPath = Path(),
                 QList<Path> *visitedRefs = nullptr);

    DomItem operator[](Path path);
    DomItem operator[](QStringView component);
    DomItem operator[](const QString &component);
    DomItem operator[](const char16_t *component)
    {
        return (*this)[QStringView(component)];
    } // to avoid clash with stupid builtin ptrdiff_t[DomItem&], coming from C
    DomItem operator[](index_type i) { return index(i); }
    DomItem operator[](int i) { return index(i); }
    index_type size() { return indexes() + keys().size(); }
    index_type length() { return size(); }

    DomItem path(Path p, ErrorHandler h = &defaultErrorHandler);
    DomItem path(QString p, ErrorHandler h = &defaultErrorHandler);
    DomItem path(QStringView p, ErrorHandler h = &defaultErrorHandler);

    QList<QString> fields();
    DomItem field(QStringView name);

    index_type indexes();
    DomItem index(index_type);
    bool visitIndexes(function_ref<bool(DomItem &)> visitor);

    QSet<QString> keys();
    QStringList sortedKeys();
    DomItem key(QString name);
    DomItem key(QStringView name) { return key(name.toString()); }
    bool visitKeys(function_ref<bool(QString, DomItem &)> visitor);

    QList<DomItem> values();
    void writeOutPre(OutWriter &lw);
    void writeOut(OutWriter &lw);
    void writeOutPost(OutWriter &lw);
    DomItem writeOutForFile(OutWriter &ow, WriteOutChecks extraChecks);
    DomItem writeOut(QString path, int nBackups = 2,
                     const LineWriterOptions &opt = LineWriterOptions(), FileWriter *fw = nullptr,
                     WriteOutChecks extraChecks = WriteOutCheck::Default);

    bool visitTree(Path basePath, ChildrenVisitor visitor,
                   VisitOptions options = VisitOption::Default,
                   ChildrenVisitor openingVisitor = emptyChildrenVisitor,
                   ChildrenVisitor closingVisitor = emptyChildrenVisitor);
    bool visitPrototypeChain(function_ref<bool(DomItem &)> visitor,
                             VisitPrototypesOptions options = VisitPrototypesOption::Normal,
                             ErrorHandler h = nullptr, QSet<quintptr> *visited = nullptr,
                             QList<Path> *visitedRefs = nullptr);
    bool visitDirectAccessibleScopes(function_ref<bool(DomItem &)> visitor,
                                     VisitPrototypesOptions options = VisitPrototypesOption::Normal,
                                     ErrorHandler h = nullptr, QSet<quintptr> *visited = nullptr,
                                     QList<Path> *visitedRefs = nullptr);
    bool
    visitStaticTypePrototypeChains(function_ref<bool(DomItem &)> visitor,
                                   VisitPrototypesOptions options = VisitPrototypesOption::Normal,
                                   ErrorHandler h = nullptr, QSet<quintptr> *visited = nullptr,
                                   QList<Path> *visitedRefs = nullptr);

    bool visitUp(function_ref<bool(DomItem &)> visitor);
    bool visitScopeChain(function_ref<bool(DomItem &)> visitor,
                         LookupOptions = LookupOption::Normal, ErrorHandler h = nullptr,
                         QSet<quintptr> *visited = nullptr, QList<Path> *visitedRefs = nullptr);
    bool visitLocalSymbolsNamed(QString name, function_ref<bool(DomItem &)> visitor);
    QSet<QString> localSymbolNames(LocalSymbolsTypes lTypes = LocalSymbolsType::All);
    bool visitLookup1(QString symbolName, function_ref<bool(DomItem &)> visitor,
                      LookupOptions = LookupOption::Normal, ErrorHandler h = nullptr,
                      QSet<quintptr> *visited = nullptr, QList<Path> *visitedRefs = nullptr);
    bool visitLookup(QString symbolName, function_ref<bool(DomItem &)> visitor,
                     LookupType type = LookupType::Symbol, LookupOptions = LookupOption::Normal,
                     ErrorHandler errorHandler = nullptr, QSet<quintptr> *visited = nullptr,
                     QList<Path> *visitedRefs = nullptr);
    bool visitSubSymbolsNamed(QString name, function_ref<bool(DomItem &)> visitor);
    DomItem proceedToScope(ErrorHandler h = nullptr, QList<Path> *visitedRefs = nullptr);
    QList<DomItem> lookup(QString symbolName, LookupType type = LookupType::Symbol,
                          LookupOptions = LookupOption::Normal,
                          ErrorHandler errorHandler = nullptr);
    DomItem lookupFirst(QString symbolName, LookupType type = LookupType::Symbol,
                        LookupOptions = LookupOption::Normal, ErrorHandler errorHandler = nullptr);

    quintptr id();
    Path pathFromOwner();
    QString canonicalFilePath();
    DomItem fileLocationsTree();
    DomItem fileLocations();
    MutableDomItem makeCopy(CopyOption option = CopyOption::EnvConnected);
    bool commitToBase(std::shared_ptr<DomEnvironment> validPtr = nullptr);
    DomItem refreshed() { return top().path(canonicalPath()); }
    QCborValue value();

    void dumpPtr(Sink sink);
    void dump(Sink, int indent = 0,
              function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)> filter =
                      noFilter);
    FileWriter::Status
    dump(QString path,
         function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)> filter = noFilter,
         int nBackups = 2, int indent = 0, FileWriter *fw = nullptr);
    QString toString();
    QString toString() const
    {
        DomItem self = *this;
        return self.toString();
    }

    // OwnigItem elements
    int derivedFrom();
    int revision();
    QDateTime createdAt();
    QDateTime frozenAt();
    QDateTime lastDataUpdateAt();

    void addError(ErrorMessage msg);
    ErrorHandler errorHandler();
    void clearErrors(ErrorGroups groups = ErrorGroups({}), bool iterate = true);
    // return false if a quick exit was requested
    bool iterateErrors(function_ref<bool(DomItem source, ErrorMessage msg)> visitor, bool iterate,
                       Path inPath = Path());

    bool iterateSubOwners(function_ref<bool(DomItem &owner)> visitor);
    bool iterateDirectSubpaths(DirectVisitor v);

    template<typename T>
    DomItem subDataItem(const PathEls::PathComponent &c, T value,
                        ConstantData::Options options = ConstantData::Options::MapIsMap);
    template<typename T>
    DomItem subDataItemField(QStringView f, T value,
                             ConstantData::Options options = ConstantData::Options::MapIsMap)
    {
        return subDataItem(PathEls::Field(f), value, options);
    }
    template<typename T>
    DomItem subValueItem(const PathEls::PathComponent &c, T value,
                         ConstantData::Options options = ConstantData::Options::MapIsMap);
    template<typename T>
    bool dvValue(DirectVisitor visitor, const PathEls::PathComponent &c, T value,
                 ConstantData::Options options = ConstantData::Options::MapIsMap);
    template<typename T>
    bool dvValueField(DirectVisitor visitor, QStringView f, T value,
                      ConstantData::Options options = ConstantData::Options::MapIsMap)
    {
        return this->dvValue<T>(visitor, PathEls::Field(f), value, options);
    }
    template<typename F>
    bool dvValueLazy(DirectVisitor visitor, const PathEls::PathComponent &c, F valueF,
                     ConstantData::Options options = ConstantData::Options::MapIsMap);
    template<typename F>
    bool dvValueLazyField(DirectVisitor visitor, QStringView f, F valueF,
                          ConstantData::Options options = ConstantData::Options::MapIsMap)
    {
        return this->dvValueLazy(visitor, PathEls::Field(f), valueF, options);
    }
    DomItem subLocationItem(const PathEls::PathComponent &c, SourceLocation loc,
                            QStringView code = QStringView())
    {
        return this->subDataItem(c, locationToData(loc, code));
    }
    // bool dvSubReference(DirectVisitor visitor, const PathEls::PathComponent &c, Path
    // referencedObject);
    DomItem subReferencesItem(const PathEls::PathComponent &c, QList<Path> paths);
    DomItem subReferenceItem(const PathEls::PathComponent &c, Path referencedObject);
    bool dvReference(DirectVisitor visitor, const PathEls::PathComponent &c, Path referencedObject)
    {
        return dvItem(visitor, c, [c, this, referencedObject]() {
            return this->subReferenceItem(c, referencedObject);
        });
    }
    bool dvReferences(DirectVisitor visitor, const PathEls::PathComponent &c, QList<Path> paths)
    {
        return dvItem(visitor, c, [c, this, paths]() { return this->subReferencesItem(c, paths); });
    }
    bool dvReferenceField(DirectVisitor visitor, QStringView f, Path referencedObject)
    {
        return dvReference(visitor, PathEls::Field(f), referencedObject);
    }
    bool dvReferencesField(DirectVisitor visitor, QStringView f, QList<Path> paths)
    {
        return dvReferences(visitor, PathEls::Field(f), paths);
    }
    bool dvItem(DirectVisitor visitor, const PathEls::PathComponent &c, function_ref<DomItem()> it)
    {
        return visitor(c, it);
    }
    bool dvItemField(DirectVisitor visitor, QStringView f, function_ref<DomItem()> it)
    {
        return dvItem(visitor, PathEls::Field(f), it);
    }
    DomItem subListItem(const List &list);
    DomItem subMapItem(const Map &map);
    DomItem subObjectWrapItem(SimpleObjectWrap obj)
    {
        return DomItem(m_top, m_owner, m_ownerPath, obj);
    }

    DomItem subScriptElementWrapperItem(const ScriptElementVariant &obj)
    {
        Q_ASSERT(obj);
        return DomItem(m_top, m_owner, m_ownerPath, ScriptElementDomWrapper(obj));
    }

    template<typename Owner>
    DomItem subOwnerItem(const PathEls::PathComponent &c, Owner o)
    {
        if constexpr (domTypeIsUnattachedOwningItem(Owner::element_type::kindValue))
            return DomItem(m_top, o, canonicalPath().appendComponent(c), o.get());
        else
            return DomItem(m_top, o, Path(), o.get());
    }
    template<typename T>
    DomItem wrap(const PathEls::PathComponent &c, T &obj);
    template<typename T>
    DomItem wrapField(QStringView f, T &obj)
    {
        return wrap<T>(PathEls::Field(f), obj);
    }
    template<typename T>
    bool dvWrap(DirectVisitor visitor, const PathEls::PathComponent &c, T &obj);
    template<typename T>
    bool dvWrapField(DirectVisitor visitor, QStringView f, T &obj)
    {
        return dvWrap<T>(visitor, PathEls::Field(f), obj);
    }

    DomItem() = default;
    DomItem(std::shared_ptr<DomEnvironment>);
    DomItem(std::shared_ptr<DomUniverse>);

    static DomItem fromCode(QString code, DomType fileType = DomType::QmlFile);
    void loadFile(const FileToLoad &file, std::function<void(Path, DomItem &, DomItem &)> callback,
                  LoadOptions loadOptions,
                  std::optional<DomType> fileType = std::optional<DomType>());
    void loadModuleDependency(QString uri, Version v,
                              std::function<void(Path, DomItem &, DomItem &)> callback = nullptr,
                              ErrorHandler = nullptr);
    void loadBuiltins(std::function<void(Path, DomItem &, DomItem &)> callback = nullptr,
                      ErrorHandler = nullptr);
    void loadPendingDependencies();

    // --- start of potentially dangerous stuff, make private? ---

    std::shared_ptr<DomTop> topPtr();
    std::shared_ptr<OwningItem> owningItemPtr();

    // keep the DomItem around to ensure that it doesn't get deleted
    template<typename T, typename std::enable_if<std::is_base_of_v<DomBase, T>, bool>::type = true>
    T const *as()
    {
        if (m_kind == T::kindValue) {
            if constexpr (domTypeIsObjWrap(T::kindValue) || domTypeIsValueWrap(T::kindValue))
                return std::get<SimpleObjectWrap>(m_element)->as<T>();
            else
                return static_cast<T const *>(base());
        }
        return nullptr;
    }

    template<typename T, typename std::enable_if<!std::is_base_of_v<DomBase, T>, bool>::type = true>
    T const *as()
    {
        if (m_kind == T::kindValue) {
            Q_ASSERT(domTypeIsObjWrap(m_kind) || domTypeIsValueWrap(m_kind));
            return std::get<SimpleObjectWrap>(m_element)->as<T>();
        }
        return nullptr;
    }

    template<typename T>
    std::shared_ptr<T> ownerAs();

    template<typename Owner, typename T>
    DomItem copy(Owner owner, Path ownerPath, T base)
    {
        Q_ASSERT(m_top);
        static_assert(IsInlineDom<std::decay_t<T>>::value, "Expected an inline item or pointer");
        return DomItem(m_top, owner, ownerPath, base);
    }

    template<typename Owner>
    DomItem copy(Owner owner, Path ownerPath)
    {
        Q_ASSERT(m_top);
        return DomItem(m_top, owner, ownerPath, owner.get());
    }

    template<typename T>
    DomItem copy(T base)
    {
        Q_ASSERT(m_top);
        using BaseT = std::decay_t<T>;
        static_assert(!std::is_same_v<BaseT, ElementT>,
                      "variant not supported, pass in the stored types");
        static_assert(IsInlineDom<BaseT>::value, "expected either a pointer or an inline item");
        if constexpr (IsSharedPointerToDomObject<BaseT>::value) {
            return DomItem(m_top, base, Path(), base.get());
        } else {
            return DomItem(m_top, m_owner, m_ownerPath, base);
        }
    }

private:
    DomBase const *base();
    template <typename T, typename std::enable_if<std::is_base_of<DomBase, T>::value, bool>::type = true>
    T *mutableAs() {
        if (m_kind == T::kindValue) {
            if constexpr (domTypeIsObjWrap(T::kindValue) || domTypeIsValueWrap(T::kindValue))
                return static_cast<SimpleObjectWrapBase *>(mutableBase())->mutableAs<T>();
            else
                return static_cast<T *>(mutableBase());
        }
        return nullptr;
    }

    template <typename T, typename std::enable_if<!std::is_base_of<DomBase, T>::value, bool>::type = true>
    T *mutableAs() {
        if (m_kind == T::kindValue) {
            Q_ASSERT(domTypeIsObjWrap(m_kind) || domTypeIsValueWrap(m_kind));
            return static_cast<SimpleObjectWrapBase *>(mutableBase())->mutableAs<T>();
        }
        return nullptr;
    }
    DomBase *mutableBase();

    template<typename Env, typename Owner>
    DomItem(Env, Owner, Path, std::nullptr_t) : DomItem()
    {
    }

    template<typename Env, typename Owner, typename T,
             typename = std::enable_if_t<IsInlineDom<std::decay_t<T>>::value>>
    DomItem(Env env, Owner owner, Path ownerPath, T el)
        : m_top(env), m_owner(owner), m_ownerPath(ownerPath), m_element(el)
    {
        using BaseT = std::decay_t<T>;
        if constexpr (std::is_pointer_v<BaseT>) {
            if (!el || el->kind() == DomType::Empty) { // avoid null ptr, and allow only a
                // single kind of Empty
                m_kind = DomType::Empty;
                m_top.reset();
                m_owner.reset();
                m_ownerPath = Path();
                m_element = Empty();
            } else {
                using DomT = std::remove_pointer_t<BaseT>;
                m_element = el;
                m_kind = DomT::kindValue;
            }
        } else {
            static_assert(!std::is_same_v<BaseT, ElementT>,
                          "variant not supported, pass in the internal type");
            m_kind = el->kind();
        }
    }
    friend class DomBase;
    friend class DomElement;
    friend class Map;
    friend class List;
    friend class QmlObject;
    friend class DomUniverse;
    friend class DomEnvironment;
    friend class ExternalItemInfoBase;
    friend class ConstantData;
    friend class MutableDomItem;
    friend class ScriptExpression;
    friend class AstComments;
    friend class AttachedInfo;
    friend class TestDomItem;
    friend QMLDOM_EXPORT bool operator==(const DomItem &, const DomItem &);
    DomType m_kind = DomType::Empty;
    std::optional<TopT> m_top;
    std::optional<OwnerT> m_owner;
    Path m_ownerPath;
    ElementT m_element = Empty();
};

QMLDOM_EXPORT bool operator==(const DomItem &o1, const DomItem &o2);

inline bool operator!=(const DomItem &o1, const DomItem &o2)
{
    return !(o1 == o2);
}

template<typename T>
Map Map::fromMultiMapRef(Path pathFromOwner, QMultiMap<QString, T> &mmap)
{
    return Map(
            pathFromOwner,
            [&mmap](DomItem &self, QString key) {
                auto it = mmap.find(key);
                auto end = mmap.cend();
                if (it == end)
                    return DomItem();
                else {
                    // special case single element (++it == end || it.key() != key)?
                    QList<T *> values;
                    while (it != end && it.key() == key)
                        values.append(&(*it++));
                    ListP ll(self.pathFromOwner().appendComponent(PathEls::Key(key)), values,
                             QString(), ListOptions::Reverse);
                    return self.copy(ll);
                }
            },
            [&mmap](DomItem &) { return QSet<QString>(mmap.keyBegin(), mmap.keyEnd()); },
            QLatin1String(typeid(T).name()));
}

template<typename T>
Map Map::fromMapRef(
        Path pathFromOwner, QMap<QString, T> &map,
        std::function<DomItem(DomItem &, const PathEls::PathComponent &, T &)> elWrapper)
{
    return Map(
            pathFromOwner,
            [&map, elWrapper](DomItem &self, QString key) {
                if (!map.contains(key))
                    return DomItem();
                else {
                    return elWrapper(self, PathEls::Key(key), map[key]);
                }
            },
            [&map](DomItem &) { return QSet<QString>(map.keyBegin(), map.keyEnd()); },
            QLatin1String(typeid(T).name()));
}

template<typename T>
List List::fromQList(
        Path pathFromOwner, QList<T> list,
        std::function<DomItem(DomItem &, const PathEls::PathComponent &, T &)> elWrapper,
        ListOptions options)
{
    index_type len = list.size();
    if (options == ListOptions::Reverse) {
        return List(
                pathFromOwner,
                [list, elWrapper](DomItem &self, index_type i) mutable {
                    if (i < 0 || i >= list.size())
                        return DomItem();
                    return elWrapper(self, PathEls::Index(i), list[list.size() - i - 1]);
                },
                [len](DomItem &) { return len; }, nullptr, QLatin1String(typeid(T).name()));
    } else {
        return List(
                pathFromOwner,
                [list, elWrapper](DomItem &self, index_type i) mutable {
                    if (i < 0 || i >= list.size())
                        return DomItem();
                    return elWrapper(self, PathEls::Index(i), list[i]);
                },
                [len](DomItem &) { return len; }, nullptr, QLatin1String(typeid(T).name()));
    }
}

template<typename T>
List List::fromQListRef(
        Path pathFromOwner, QList<T> &list,
        std::function<DomItem(DomItem &, const PathEls::PathComponent &, T &)> elWrapper,
        ListOptions options)
{
    if (options == ListOptions::Reverse) {
        return List(
                pathFromOwner,
                [&list, elWrapper](DomItem &self, index_type i) {
                    if (i < 0 || i >= list.size())
                        return DomItem();
                    return elWrapper(self, PathEls::Index(i), list[list.size() - i - 1]);
                },
                [&list](DomItem &) { return list.size(); }, nullptr,
                QLatin1String(typeid(T).name()));
    } else {
        return List(
                pathFromOwner,
                [&list, elWrapper](DomItem &self, index_type i) {
                    if (i < 0 || i >= list.size())
                        return DomItem();
                    return elWrapper(self, PathEls::Index(i), list[i]);
                },
                [&list](DomItem &) { return list.size(); }, nullptr,
                QLatin1String(typeid(T).name()));
    }
}

class QMLDOM_EXPORT OwningItem: public DomBase {
protected:
    virtual std::shared_ptr<OwningItem> doCopy(DomItem &self) const = 0;

public:
    OwningItem(const OwningItem &o);
    OwningItem(int derivedFrom=0);
    OwningItem(int derivedFrom, QDateTime lastDataUpdateAt);
    OwningItem(const OwningItem &&) = delete;
    OwningItem &operator=(const OwningItem &&) = delete;
    static int nextRevision();

    Path canonicalPath(DomItem &self) const override = 0;

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor) override;
    std::shared_ptr<OwningItem> makeCopy(DomItem &self) const { return doCopy(self); }
    Path pathFromOwner() const { return Path(); }
    Path pathFromOwner(DomItem &) const override final { return Path(); }
    DomItem containingObject(DomItem &self) const override;
    int derivedFrom() const;
    virtual int revision() const;

    QDateTime createdAt() const;
    virtual QDateTime lastDataUpdateAt() const;
    virtual void refreshedDataAt(QDateTime tNew);

    // explicit freeze handling needed?
    virtual bool frozen() const;
    virtual bool freeze();
    QDateTime frozenAt() const;

    virtual void addError(DomItem &self, ErrorMessage msg);
    void addErrorLocal(ErrorMessage msg);
    void clearErrors(ErrorGroups groups = ErrorGroups({}));
    // return false if a quick exit was requested
    bool iterateErrors(DomItem &self, function_ref<bool(DomItem source, ErrorMessage msg)> visitor,
                       Path inPath = Path());
    QMultiMap<Path, ErrorMessage> localErrors() const {
        QMutexLocker l(mutex());
        return m_errors;
    }

    virtual bool iterateSubOwners(DomItem &self, function_ref<bool(DomItem &owner)> visitor);

    QBasicMutex *mutex() const { return &m_mutex; }
private:
    mutable QBasicMutex m_mutex;
    int m_derivedFrom;
    int m_revision;
    QDateTime m_createdAt;
    QDateTime m_lastDataUpdateAt;
    QDateTime m_frozenAt;
    QMultiMap<Path, ErrorMessage> m_errors;
    QMap<ErrorMessage, quint32> m_errorsCounts;
};

template<typename T>
std::shared_ptr<T> DomItem::ownerAs()
{
    if constexpr (domTypeIsOwningItem(T::kindValue)) {
        if (m_owner) {
            if constexpr (T::kindValue == DomType::AttachedInfo) {
                if (std::holds_alternative<std::shared_ptr<AttachedInfo>>(*m_owner))
                    return std::static_pointer_cast<T>(
                            std::get<std::shared_ptr<AttachedInfo>>(*m_owner));
            } else if constexpr (T::kindValue == DomType::ExternalItemInfo) {
                if (std::holds_alternative<std::shared_ptr<ExternalItemInfoBase>>(*m_owner))
                    return std::static_pointer_cast<T>(
                            std::get<std::shared_ptr<ExternalItemInfoBase>>(*m_owner));
            } else if constexpr (T::kindValue == DomType::ExternalItemPair) {
                if (std::holds_alternative<std::shared_ptr<ExternalItemPairBase>>(*m_owner))
                    return std::static_pointer_cast<T>(
                            std::get<std::shared_ptr<ExternalItemPairBase>>(*m_owner));
            } else {
                if (std::holds_alternative<std::shared_ptr<T>>(*m_owner)) {
                    return std::get<std::shared_ptr<T>>(*m_owner);
                }
            }
        }
    } else {
        Q_ASSERT_X(false, "DomItem::ownerAs", "unexpected non owning value in ownerAs");
    }
    return std::shared_ptr<T> {};
}

template<int I>
struct rank : rank<I - 1>
{
    static_assert(I > 0, "");
};
template<>
struct rank<0>
{
};

template<typename T>
auto writeOutWrap(const T &t, DomItem &self, OutWriter &lw, rank<1>)
        -> decltype(t.writeOut(self, lw))
{
    t.writeOut(self, lw);
}

template<typename T>
auto writeOutWrap(const T &, DomItem &, OutWriter &, rank<0>) -> void
{
    qCWarning(writeOutLog) << "Ignoring writeout to wrapped object not supporting it ("
                           << typeid(T).name();
}
template<typename T>
auto writeOutWrap(const T &t, DomItem &self, OutWriter &lw) -> void
{
    writeOutWrap(t, self, lw, rank<1>());
}

template<typename T>
void SimpleObjectWrapT<T>::writeOut(DomItem &self, OutWriter &lw) const
{
    writeOutWrap<T>(*asT(), self, lw);
}

QMLDOM_EXPORT QDebug operator<<(QDebug debug, const DomItem &c);

class QMLDOM_EXPORT MutableDomItem {
public:
    using CopyOption = DomItem::CopyOption;

    explicit operator bool() const
    {
        return bool(m_owner);
    } // this is weaker than item(), but normally correct
    DomType internalKind() { return item().internalKind(); }
    QString internalKindStr() { return domTypeToString(internalKind()); }
    DomKind domKind() { return kind2domKind(internalKind()); }

    Path canonicalPath() { return m_owner.canonicalPath().path(m_pathFromOwner); }
    MutableDomItem containingObject()
    {
        if (m_pathFromOwner)
            return MutableDomItem(m_owner, m_pathFromOwner.split().pathToSource);
        else {
            DomItem cObj = m_owner.containingObject();
            return MutableDomItem(cObj.owner(), (domTypeIsOwningItem(cObj.internalKind()) ? Path() :cObj.pathFromOwner()));
        }
    }

    MutableDomItem container()
    {
        if (m_pathFromOwner)
            return MutableDomItem(m_owner, m_pathFromOwner.dropTail());
        else {
            return MutableDomItem(item().container());
        }
    }

    MutableDomItem qmlObject(GoTo option = GoTo::Strict,
                             FilterUpOptions fOptions = FilterUpOptions::ReturnOuter)
    {
        return MutableDomItem(item().qmlObject(option, fOptions));
    }
    MutableDomItem fileObject(GoTo option = GoTo::Strict)
    {
        return MutableDomItem(item().fileObject(option));
    }
    MutableDomItem rootQmlObject(GoTo option = GoTo::Strict)
    {
        return MutableDomItem(item().rootQmlObject(option));
    }
    MutableDomItem globalScope() { return MutableDomItem(item().globalScope()); }
    MutableDomItem scope() { return MutableDomItem(item().scope()); }

    MutableDomItem component(GoTo option = GoTo::Strict)
    {
        return MutableDomItem { item().component(option) };
    }
    MutableDomItem owner() { return MutableDomItem(m_owner); }
    MutableDomItem top() { return MutableDomItem(item().top()); }
    MutableDomItem environment() { return MutableDomItem(item().environment()); }
    MutableDomItem universe() { return MutableDomItem(item().universe()); }
    Path pathFromOwner() { return m_pathFromOwner; }
    MutableDomItem operator[](const Path &path) { return MutableDomItem(item()[path]); }
    MutableDomItem operator[](QStringView component) { return MutableDomItem(item()[component]); }
    MutableDomItem operator[](const QString &component)
    {
        return MutableDomItem(item()[component]);
    }
    MutableDomItem operator[](const char16_t *component)
    {
        // to avoid clash with stupid builtin ptrdiff_t[MutableDomItem&], coming from C
        return MutableDomItem(item()[QStringView(component)]);
    }
    MutableDomItem operator[](index_type i) { return MutableDomItem(item().index(i)); }

    MutableDomItem path(const Path &p) { return MutableDomItem(item().path(p)); }
    MutableDomItem path(const QString &p) { return path(Path::fromString(p)); }
    MutableDomItem path(QStringView p) { return path(Path::fromString(p)); }

    QList<QString> const fields() { return item().fields(); }
    MutableDomItem field(QStringView name) { return MutableDomItem(item().field(name)); }
    index_type indexes() { return item().indexes(); }
    MutableDomItem index(index_type i) { return MutableDomItem(item().index(i)); }

    QSet<QString> const keys() { return item().keys(); }
    MutableDomItem key(QString name) { return MutableDomItem(item().key(name)); }
    MutableDomItem key(QStringView name) { return key(name.toString()); }

    void
    dump(Sink s, int indent = 0,
         function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)> filter = noFilter)
    {
        item().dump(s, indent, filter);
    }
    FileWriter::Status
    dump(QString path,
         function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)> filter = noFilter,
         int nBackups = 2, int indent = 0, FileWriter *fw = nullptr)
    {
        return item().dump(path, filter, nBackups, indent, fw);
    }
    void writeOut(OutWriter &lw) { return item().writeOut(lw); }
    MutableDomItem writeOut(QString path, int nBackups = 2,
                            const LineWriterOptions &opt = LineWriterOptions(),
                            FileWriter *fw = nullptr)
    {
        return MutableDomItem(item().writeOut(path, nBackups, opt, fw));
    }

    MutableDomItem fileLocations() { return MutableDomItem(item().fileLocations()); }
    MutableDomItem makeCopy(CopyOption option = CopyOption::EnvConnected)
    {
        return item().makeCopy(option);
    }
    bool commitToBase(std::shared_ptr<DomEnvironment> validEnvPtr = nullptr)
    {
        return item().commitToBase(validEnvPtr);
    }
    QString canonicalFilePath() { return item().canonicalFilePath(); }

    MutableDomItem refreshed() { return MutableDomItem(item().refreshed()); }

    QCborValue value() { return item().value(); }

    QString toString() { return item().toString(); }

    // convenience getters
    QString name() { return item().name(); }
    MutableDomItem pragmas() { return item().pragmas(); }
    MutableDomItem ids() { return MutableDomItem::item().ids(); }
    QString idStr() { return item().idStr(); }
    MutableDomItem propertyDefs() { return MutableDomItem(item().propertyDefs()); }
    MutableDomItem bindings() { return MutableDomItem(item().bindings()); }
    MutableDomItem methods() { return MutableDomItem(item().methods()); }
    MutableDomItem children() { return MutableDomItem(item().children()); }
    MutableDomItem child(index_type i) { return MutableDomItem(item().child(i)); }
    MutableDomItem annotations() { return MutableDomItem(item().annotations()); }

    //    // OwnigItem elements
    int derivedFrom() { return m_owner.derivedFrom(); }
    int revision() { return m_owner.revision(); }
    QDateTime createdAt() { return m_owner.createdAt(); }
    QDateTime frozenAt() { return m_owner.frozenAt(); }
    QDateTime lastDataUpdateAt() { return m_owner.lastDataUpdateAt(); }

    void addError(ErrorMessage msg) { item().addError(msg); }
    ErrorHandler errorHandler();

    // convenience setters
    MutableDomItem addPrototypePath(Path prototypePath);
    MutableDomItem setNextScopePath(Path nextScopePath);
    MutableDomItem setPropertyDefs(QMultiMap<QString, PropertyDefinition> propertyDefs);
    MutableDomItem setBindings(QMultiMap<QString, Binding> bindings);
    MutableDomItem setMethods(QMultiMap<QString, MethodInfo> functionDefs);
    MutableDomItem setChildren(QList<QmlObject> children);
    MutableDomItem setAnnotations(QList<QmlObject> annotations);
    MutableDomItem setScript(std::shared_ptr<ScriptExpression> exp);
    MutableDomItem setCode(QString code);
    MutableDomItem addPropertyDef(PropertyDefinition propertyDef,
                                  AddOption option = AddOption::Overwrite);
    MutableDomItem addBinding(Binding binding, AddOption option = AddOption::Overwrite);
    MutableDomItem addMethod(MethodInfo functionDef, AddOption option = AddOption::Overwrite);
    MutableDomItem addChild(QmlObject child);
    MutableDomItem addAnnotation(QmlObject child);
    MutableDomItem addPreComment(const Comment &comment, QString regionName = QString());
    MutableDomItem addPreComment(const Comment &comment, QStringView regionName)
    {
        return addPreComment(comment, regionName.toString());
    }
    MutableDomItem addPostComment(const Comment &comment, QString regionName = QString());
    MutableDomItem addPostComment(const Comment &comment, QStringView regionName)
    {
        return addPostComment(comment, regionName.toString());
    }
    QQmlJSScope::Ptr semanticScope();
    void setSemanticScope(const QQmlJSScope::Ptr &scope);

    MutableDomItem() = default;
    MutableDomItem(DomItem owner, Path pathFromOwner):
        m_owner(owner), m_pathFromOwner(pathFromOwner)
    {}
    MutableDomItem(DomItem item):
        m_owner(item.owner()), m_pathFromOwner(item.pathFromOwner())
    {}

    std::shared_ptr<DomTop> topPtr() { return m_owner.topPtr(); }
    std::shared_ptr<OwningItem> owningItemPtr() { return m_owner.owningItemPtr(); }

    template<typename T>
    T const *as()
    {
        return item().as<T>();
    }

    template <typename T>
    T *mutableAs() {
        Q_ASSERT(!m_owner || !m_owner.owningItemPtr()->frozen());
        return item().mutableAs<T>();
    }

    template<typename T>
    std::shared_ptr<T> ownerAs()
    {
        return m_owner.ownerAs<T>();
    }
    // it is dangerous to assume it stays valid when updates are preformed...
    DomItem item() { return m_owner.path(m_pathFromOwner); }

    friend bool operator==(const MutableDomItem o1, const MutableDomItem &o2)
    {
        return o1.m_owner == o2.m_owner && o1.m_pathFromOwner == o2.m_pathFromOwner;
    }
    friend bool operator!=(const MutableDomItem &o1, const MutableDomItem &o2)
    {
        return !(o1 == o2);
    }

private:
    DomItem m_owner;
    Path m_pathFromOwner;
};

QMLDOM_EXPORT QDebug operator<<(QDebug debug, const MutableDomItem &c);

template<typename K, typename T>
Path insertUpdatableElementInMultiMap(Path mapPathFromOwner, QMultiMap<K, T> &mmap, K key,
                                      const T &value, AddOption option = AddOption::KeepExisting,
                                      T **valuePtr = nullptr)
{
    if (option == AddOption::Overwrite) {
        auto it = mmap.find(key);
        if (it != mmap.end()) {
            T &v = *it;
            v = value;
            if (++it != mmap.end() && it.key() == key) {
                qWarning() << " requested overwrite of " << key
                           << " that contains aleready multiple entries in" << mapPathFromOwner;
            }
            Path newPath = mapPathFromOwner.key(key).index(0);
            v.updatePathFromOwner(newPath);
            if (valuePtr)
                *valuePtr = &v;
            return newPath;
        }
    }
    mmap.insert(key, value);
    auto it = mmap.find(key);
    auto it2 = it;
    int nVal = 0;
    while (it2 != mmap.end() && it2.key() == key) {
       ++nVal;
        ++it2;
    }
    Path newPath = mapPathFromOwner.key(key).index(nVal-1);
    T &v = *it;
    v.updatePathFromOwner(newPath);
    if (valuePtr)
        *valuePtr = &v;
    return newPath;
}

template<typename T>
Path appendUpdatableElementInQList(Path listPathFromOwner, QList<T> &list, const T &value,
                                   T **vPtr = nullptr)
{
    int idx = list.size();
    list.append(value);
    Path newPath = listPathFromOwner.index(idx);
    T &targetV = list[idx];
    targetV.updatePathFromOwner(newPath);
    if (vPtr)
        *vPtr = &targetV;
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
            for (T *el : els)
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
    for (T *el : els)
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

constexpr bool domTypeIsObjWrap(DomType k)
{
    switch (k) {
    case DomType::Binding:
    case DomType::EnumItem:
    case DomType::ErrorMessage:
    case DomType::Export:
    case DomType::Id:
    case DomType::Import:
    case DomType::ImportScope:
    case DomType::MethodInfo:
    case DomType::MethodParameter:
    case DomType::ModuleAutoExport:
    case DomType::Pragma:
    case DomType::PropertyDefinition:
    case DomType::Version:
    case DomType::Comment:
    case DomType::CommentedElement:
    case DomType::RegionComments:
    case DomType::FileLocations:
    case DomType::UpdatedScriptExpression:
        return true;
    default:
        return false;
    }
}

constexpr bool domTypeIsValueWrap(DomType k)
{
    switch (k) {
    case DomType::PropertyInfo:
        return true;
    default:
        return false;
    }
}

constexpr bool domTypeIsDomElement(DomType k)
{
    switch (k) {
    case DomType::ModuleScope:
    case DomType::QmlObject:
    case DomType::ConstantData:
    case DomType::SimpleObjectWrap:
    case DomType::Reference:
    case DomType::Map:
    case DomType::List:
    case DomType::ListP:
    case DomType::EnumDecl:
    case DomType::JsResource:
    case DomType::QmltypesComponent:
    case DomType::QmlComponent:
    case DomType::GlobalComponent:
    case DomType::MockObject:
        return true;
    default:
        return false;
    }
}

constexpr bool domTypeIsOwningItem(DomType k)
{
    switch (k) {
    case DomType::ModuleIndex:

    case DomType::MockOwner:

    case DomType::ExternalItemInfo:
    case DomType::ExternalItemPair:

    case DomType::QmlDirectory:
    case DomType::QmldirFile:
    case DomType::JsFile:
    case DomType::QmlFile:
    case DomType::QmltypesFile:
    case DomType::GlobalScope:

    case DomType::ScriptExpression:
    case DomType::AstComments:

    case DomType::LoadInfo:
    case DomType::AttachedInfo:

    case DomType::DomEnvironment:
    case DomType::DomUniverse:
        return true;
    default:
        return false;
    }
}

constexpr bool domTypeIsUnattachedOwningItem(DomType k)
{
    switch (k) {
    case DomType::ScriptExpression:
    case DomType::AstComments:
    case DomType::AttachedInfo:
        return true;
    default:
        return false;
    }
}

constexpr bool domTypeIsScriptElement(DomType k)
{
    return DomType::ScriptElementStart <= k && k <= DomType::ScriptElementStop;
}

template<typename T>
DomItem DomItem::subValueItem(const PathEls::PathComponent &c, T value,
                              ConstantData::Options options)
{
    using BaseT = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr (
            std::is_base_of_v<
                    QCborValue,
                    BaseT> || std::is_base_of_v<QCborArray, BaseT> || std::is_base_of_v<QCborMap, BaseT>) {
        return DomItem(m_top, m_owner, m_ownerPath,
                       ConstantData(pathFromOwner().appendComponent(c), value, options));
    } else if constexpr (std::is_same_v<DomItem, BaseT>) {
        Q_UNUSED(options);
        return value;
    } else if constexpr (IsList<T>::value && !std::is_convertible_v<BaseT, QStringView>) {
        return subListItem(List::fromQList<typename BaseT::value_type>(
                pathFromOwner().appendComponent(c), value,
                [options](DomItem &list, const PathEls::PathComponent &p,
                          typename T::value_type &v) { return list.subValueItem(p, v, options); }));
    } else if constexpr (IsSharedPointerToDomObject<BaseT>::value) {
        Q_UNUSED(options);
        return subOwnerItem(c, value);
    } else {
        return subDataItem(c, value, options);
    }
}

template<typename T>
DomItem DomItem::subDataItem(const PathEls::PathComponent &c, T value,
                             ConstantData::Options options)
{
    using BaseT = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr (std::is_same_v<BaseT, ConstantData>) {
        return this->copy(value);
    } else if constexpr (std::is_base_of_v<QCborValue, BaseT>) {
        return DomItem(m_top, m_owner, m_ownerPath,
                       ConstantData(pathFromOwner().appendComponent(c), value, options));
    } else {
        return DomItem(
                m_top, m_owner, m_ownerPath,
                ConstantData(pathFromOwner().appendComponent(c), QCborValue(value), options));
    }
}

template<typename T>
bool DomItem::dvValue(DirectVisitor visitor, const PathEls::PathComponent &c, T value,
                      ConstantData::Options options)
{
    auto lazyWrap = [this, &c, &value, options]() {
        return this->subValueItem<T>(c, value, options);
    };
    return visitor(c, lazyWrap);
}

template<typename F>
bool DomItem::dvValueLazy(DirectVisitor visitor, const PathEls::PathComponent &c, F valueF,
                          ConstantData::Options options)
{
    auto lazyWrap = [this, &c, &valueF, options]() {
        return this->subValueItem<decltype(valueF())>(c, valueF(), options);
    };
    return visitor(c, lazyWrap);
}

template<typename T>
DomItem DomItem::wrap(const PathEls::PathComponent &c, T &obj)
{
    using BaseT = std::decay_t<T>;
    if constexpr (std::is_same_v<QString, BaseT> || std::is_arithmetic_v<BaseT>) {
        return this->subDataItem(c, QCborValue(obj));
    } else if constexpr (std::is_same_v<SourceLocation, BaseT>) {
        return this->subLocationItem(c, obj);
    } else if constexpr (std::is_same_v<BaseT, Reference>) {
        Q_ASSERT_X(false, "DomItem::wrap",
                   "wrapping a reference object, probably an error (wrap the target path instead)");
        return this->copy(obj);
    } else if constexpr (std::is_same_v<BaseT, ConstantData>) {
        return this->subDataItem(c, obj);
    } else if constexpr (std::is_same_v<BaseT, Map>) {
        return this->subMapItem(obj);
    } else if constexpr (std::is_same_v<BaseT, List>) {
        return this->subListItem(obj);
    } else if constexpr (std::is_base_of_v<ListPBase, BaseT>) {
        return this->subListItem(obj);
    } else if constexpr (std::is_same_v<BaseT, SimpleObjectWrap>) {
        return this->subObjectWrapItem(obj);
    } else if constexpr (IsDomObject<BaseT>::value) {
        if constexpr (domTypeIsObjWrap(BaseT::kindValue) || domTypeIsValueWrap(BaseT::kindValue)) {
            return this->subObjectWrapItem(
                    SimpleObjectWrap::fromObjectRef(this->pathFromOwner().appendComponent(c), obj));
        } else if constexpr (domTypeIsDomElement(BaseT::kindValue)) {
            return this->copy(&obj);
        } else {
            qCWarning(domLog) << "Unhandled object of type " << domTypeToString(BaseT::kindValue)
                              << " in DomItem::wrap, not using a shared_ptr for an "
                              << "OwningItem, or unexpected wrapped object?";
            return DomItem();
        }
    } else if constexpr (IsSharedPointerToDomObject<BaseT>::value) {
        if constexpr (domTypeIsOwningItem(BaseT::element_type::kindValue)) {
            return this->subOwnerItem(c, obj);
        } else {
            Q_ASSERT_X(false, "DomItem::wrap", "shared_ptr with non owning item");
            return DomItem();
        }
    } else if constexpr (IsMultiMap<BaseT>::value) {
        if constexpr (std::is_same_v<typename BaseT::key_type, QString>) {
            return subMapItem(Map::fromMultiMapRef<typename BaseT::mapped_type>(
                    pathFromOwner().appendComponent(c), obj));
        } else {
            Q_ASSERT_X(false, "DomItem::wrap", "non string keys not supported (try .toString()?)");
        }
    } else if constexpr (IsMap<BaseT>::value) {
        if constexpr (std::is_same_v<typename BaseT::key_type, QString>) {
            return subMapItem(Map::fromMapRef<typename BaseT::mapped_type>(
                    pathFromOwner().appendComponent(c), obj,
                    [](DomItem &map, const PathEls::PathComponent &p,
                       typename BaseT::mapped_type &el) { return map.wrap(p, el); }));
        } else {
            Q_ASSERT_X(false, "DomItem::wrap", "non string keys not supported (try .toString()?)");
        }
    } else if constexpr (IsList<BaseT>::value) {
        if constexpr (IsDomObject<typename BaseT::value_type>::value) {
            return subListItem(List::fromQListRef<typename BaseT::value_type>(
                    pathFromOwner().appendComponent(c), obj,
                    [](DomItem &list, const PathEls::PathComponent &p,
                       typename BaseT::value_type &el) { return list.wrap(p, el); }));
        } else {
            Q_ASSERT_X(false, "DomItem::wrap", "Unsupported list type T");
            return DomItem();
        }
    } else {
        qCWarning(domLog) << "Cannot wrap " << typeid(BaseT).name();
        Q_ASSERT_X(false, "DomItem::wrap", "Do not know how to wrap type T");
        return DomItem();
    }
}

template<typename T>
bool DomItem::dvWrap(DirectVisitor visitor, const PathEls::PathComponent &c, T &obj)
{
    auto lazyWrap = [this, &c, &obj]() { return this->wrap<T>(c, obj); };
    return visitor(c, lazyWrap);
}

template<typename T>
bool ListPT<T>::iterateDirectSubpaths(DomItem &self, DirectVisitor v)
{
    index_type len = index_type(m_pList.size());
    for (index_type i = 0; i < len; ++i) {
        if (!v(PathEls::Index(i), [this, &self, i] { return this->index(self, i); }))
            return false;
    }
    return true;
}

template<typename T>
DomItem ListPT<T>::index(DomItem &self, index_type index) const
{
    if (index >= 0 && index < m_pList.size())
        return self.wrap(PathEls::Index(index), *reinterpret_cast<T *>(m_pList.value(index)));
    return DomItem();
}

// allow inlining of DomBase
inline DomKind DomBase::domKind() const
{
    return kind2domKind(kind());
}

inline bool DomBase::iterateDirectSubpathsConst(DomItem &self, DirectVisitor visitor) const
{
    Q_ASSERT(self.base() == this);
    return self.iterateDirectSubpaths(visitor);
}

inline DomItem DomBase::containingObject(DomItem &self) const
{
    Path path = pathFromOwner(self);
    DomItem base = self.owner();
    if (!path) {
        path = canonicalPath(self);
        base = self;
    }
    Source source = path.split();
    return base.path(source.pathToSource);
}

inline quintptr DomBase::id() const
{
    return quintptr(this);
}

inline QString DomBase::typeName() const
{
    return domTypeToString(kind());
}

inline QList<QString> DomBase::fields(DomItem &self) const
{
    QList<QString> res;
    self.iterateDirectSubpaths([&res](const PathEls::PathComponent &c, function_ref<DomItem()>) {
        if (c.kind() == Path::Kind::Field)
            res.append(c.name());
        return true;
    });
    return res;
}

inline DomItem DomBase::field(DomItem &self, QStringView name) const
{
    DomItem res;
    self.iterateDirectSubpaths(
            [&res, name](const PathEls::PathComponent &c, function_ref<DomItem()> obj) {
                if (c.kind() == Path::Kind::Field && c.checkName(name)) {
                    res = obj();
                    return false;
                }
                return true;
            });
    return res;
}

inline index_type DomBase::indexes(DomItem &self) const
{
    index_type res = 0;
    self.iterateDirectSubpaths([&res](const PathEls::PathComponent &c, function_ref<DomItem()>) {
        if (c.kind() == Path::Kind::Index) {
            index_type i = c.index() + 1;
            if (res < i)
                res = i;
        }
        return true;
    });
    return res;
}

inline DomItem DomBase::index(DomItem &self, qint64 index) const
{
    DomItem res;
    self.iterateDirectSubpaths(
            [&res, index](const PathEls::PathComponent &c, function_ref<DomItem()> obj) {
                if (c.kind() == Path::Kind::Index && c.index() == index) {
                    res = obj();
                    return false;
                }
                return true;
            });
    return res;
}

inline QSet<QString> const DomBase::keys(DomItem &self) const
{
    QSet<QString> res;
    self.iterateDirectSubpaths([&res](const PathEls::PathComponent &c, function_ref<DomItem()>) {
        if (c.kind() == Path::Kind::Key)
            res.insert(c.name());
        return true;
    });
    return res;
}

inline DomItem DomBase::key(DomItem &self, QString name) const
{
    DomItem res;
    self.iterateDirectSubpaths(
            [&res, name](const PathEls::PathComponent &c, function_ref<DomItem()> obj) {
                if (c.kind() == Path::Kind::Key && c.checkName(name)) {
                    res = obj();
                    return false;
                }
                return true;
            });
    return res;
}

inline DomItem DomItem::subListItem(const List &list)
{
    return DomItem(m_top, m_owner, m_ownerPath, list);
}

inline DomItem DomItem::subMapItem(const Map &map)
{
    return DomItem(m_top, m_owner, m_ownerPath, map);
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QMLDOMITEM_H
