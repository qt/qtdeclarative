// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMLDOM_PATH_H
#define QMLDOM_PATH_H

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

#include "qqmldomconstants_p.h"
#include "qqmldomstringdumper_p.h"
#include "qqmldom_global.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QList>
#include <QtCore/QMetaEnum>
#include <QtCore/QString>
#include <QtCore/QStringView>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#include <functional>
#include <iterator>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class ErrorGroups;
class ErrorMessage;
class DomItem;
class Path;

using ErrorHandler = std::function<void(const ErrorMessage &)> ;

using index_type = qint64;

#define QMLDOM_FIELD_LIST(F) \
    F(access) \
    F(accessSemantics) \
    F(allSources) \
    F(alternative) \
    F(annotations) \
    F(arguments) \
    F(astComments) \
    F(astRelocatableDump) \
    F(attachedType) \
    F(attachedTypeName) \
    F(autoExports) \
    F(base) \
    F(binaryExpression) \
    F(bindable) \
    F(bindingElement) \
    F(bindingIdentifiers) \
    F(bindingType) \
    F(bindings) \
    F(block) \
    F(body) \
    F(callee) \
    F(canonicalFilePath) \
    F(canonicalPath) \
    F(caseBlock) \
    F(caseClause) \
    F(caseClauses) \
    F(catchBlock) \
    F(catchParameter) \
    F(children) \
    F(classNames) \
    F(code) \
    F(commentedElements) \
    F(comments) \
    F(components) \
    F(condition) \
    F(consequence) \
    F(contents) \
    F(contentsDate) \
    F(cppType) \
    F(currentExposedAt) \
    F(currentIsValid) \
    F(currentItem) \
    F(currentRevision) \
    F(declarations) \
    F(defaultClause) \
    F(defaultPropertyName) \
    F(defaultValue) \
    F(designerSupported) \
    F(elLocation) \
    F(elements) \
    F(elementCanonicalPath) \
    F(enumerations) \
    F(errors) \
    F(exportSource) \
    F(exports) \
    F(expr) \
    F(expression) \
    F(expressionType) \
    F(extensionTypeName) \
    F(fileLocationsTree) \
    F(fileName) \
    F(finallyBlock) \
    F(regExpFlags) \
    F(forStatement) \
    F(fullRegion) \
    F(get) \
    F(hasCallback) \
    F(hasCustomParser) \
    F(idStr) \
    F(identifier) \
    F(ids) \
    F(implicit) \
    F(import) \
    F(importId) \
    F(importScope) \
    F(importSources) \
    F(imported) \
    F(imports) \
    F(inProgress) \
    F(infoItem) \
    F(inheritVersion) \
    F(initializer) \
    F(interfaceNames) \
    F(isAlias) \
    F(isComposite) \
    F(isConstructor) \
    F(isCreatable) \
    F(isDefaultMember) \
    F(isFinal) \
    F(isInternal) \
    F(isLatest) \
    F(isList) \
    F(isPointer) \
    F(isReadonly) \
    F(isRequired) \
    F(isSignalHandler) \
    F(isSingleton) \
    F(isValid) \
    F(jsFileWithPath) \
    F(kind) \
    F(lastRevision) \
    F(label) \
    F(lastValidRevision) \
    F(left) \
    F(loadInfo) \
    F(loadOptions) \
    F(loadPaths) \
    F(loadsWithWork) \
    F(localOffset) \
    F(location) \
    F(logicalPath) \
    F(majorVersion) \
    F(metaRevisions) \
    F(methodType) \
    F(methods) \
    F(minorVersion) \
    F(moduleIndex) \
    F(moduleIndexWithUri) \
    F(moduleScope) \
    F(moreCaseClauses) \
    F(nAllLoadedCallbacks) \
    F(nCallbacks) \
    F(nLoaded) \
    F(nNotdone) \
    F(name) \
    F(nameIdentifiers) \
    F(newlinesBefore) \
    F(nextComponent) \
    F(nextScope) \
    F(notify) \
    F(objects) \
    F(onAttachedObject) \
    F(operation) \
    F(options) \
    F(parameters) \
    F(parent) \
    F(parentObject) \
    F(path) \
    F(regExpPattern) \
    F(plugins) \
    F(postCode) \
    F(postComments) \
    F(pragma) \
    F(pragmas) \
    F(preCode) \
    F(preComments) \
    F(properties) \
    F(propertyDef) \
    F(propertyDefRef) \
    F(propertyDefs) \
    F(propertyInfos) \
    F(propertyName) \
    F(prototypes) \
    F(qmlFileWithPath) \
    F(qmlFiles) \
    F(qualifiedImports) \
    F(rawComment) \
    F(read) \
    F(referredObject) \
    F(referredObjectPath) \
    F(regionComments) \
    F(regions) \
    F(requestedAt) \
    F(requestingUniverse) \
    F(returnType) \
    F(returnTypeName) \
    F(right) \
    F(rootComponent) \
    F(scopeType) \
    F(scriptElement) \
    F(sources) \
    F(statement) \
    F(statements) \
    F(status) \
    F(stringValue) \
    F(subComponents) \
    F(subImports) \
    F(subItems) \
    F(symbol) \
    F(symbols) \
    F(target) \
    F(targetPropertyName) \
    F(templateLiteral) \
    F(text) \
    F(type) \
    F(typeArgument) \
    F(typeArgumentName) \
    F(typeName) \
    F(types) \
    F(universe) \
    F(uri) \
    F(uris) \
    F(validExposedAt) \
    F(validItem) \
    F(value) \
    F(elementTypeName) \
    F(values) \
    F(version) \
    F(when) \
    F(write)

/*
   FieldName enumerates the symbolic names that may appear as a Field component
   of a Path.

   We use an X macro for the enum creation and the enum<->string conversion, as
   that has lower overhead than using QMetaEnum.

   The string spellings exist for two use cases: rendering paths as
   text (toString / dump / diagnostics) and parsing user-supplied paths back to
   their structural form.
 */
enum class FieldName : quint16 {
#define QMLDOM_DEFINE_FIELD_ENUMERATOR(name) name,
    QMLDOM_FIELD_LIST(QMLDOM_DEFINE_FIELD_ENUMERATOR)
#undef QMLDOM_DEFINE_FIELD_ENUMERATOR
    NumFieldNames
};

using FieldType = FieldName;

namespace FieldNameImpl {
#define QMLDOM_DEFINE_FIELD_STRING(name) QStringView(u ## #name),
inline constexpr std::array<QStringView, size_t(FieldName::NumFieldNames)> fieldNameStrings = {
    QMLDOM_FIELD_LIST(QMLDOM_DEFINE_FIELD_STRING)
};
#undef QMLDOM_DEFINE_FIELD_STRING
} // namespace FieldNameImpl

inline QStringView fieldNameToString(FieldName f)
{
    Q_ASSERT(size_t(f) < size_t(FieldName::NumFieldNames));
    return FieldNameImpl::fieldNameStrings[size_t(f)];
}

inline QString fieldNameToQString(FieldName f)
{
    return fieldNameToString(f).toString();
}

inline std::optional<FieldName> fieldNameFromString(QStringView s)
{
    for (size_t i = 0; i < std::size(FieldNameImpl::fieldNameStrings); ++i) {
        if (s == FieldNameImpl::fieldNameStrings[i])
            return static_cast<FieldName>(i);
    }
    return std::nullopt;
}

inline QDebug operator<<(QDebug d, FieldName f) { return d << fieldNameToString(f); }

// Fields:: namespace exposes each FieldName enumerator as a constant, so the
// canonical Fields::xxx usage compiles unchanged and resolves to a FieldName.
namespace Fields {
#define QMLDOM_DEFINE_FIELD_CONST(name) inline constexpr FieldName name = FieldName::name;
QMLDOM_FIELD_LIST(QMLDOM_DEFINE_FIELD_CONST)
#undef QMLDOM_DEFINE_FIELD_CONST
} // namespace Fields

namespace PathEls {

enum class Kind{
    Empty,
    Field,
    Index,
    Key,
    Root,
    Current,
};

class TestPaths;
class Empty;
class Field;
class Index;
class Key;
class Root;
class Current;

class Base {
public:
    QStringView stringView() const { return QStringView(); }
    index_type index(index_type defaultValue = -1) const { return defaultValue; }
    bool hasSquareBrackets() const { return false; }

protected:
    void dump(const Sink &sink, const QString &name, bool hasSquareBrackets) const;
};

class Empty final : public Base
{
public:
    Empty() = default;
    QString name() const { return QString(); }
    bool checkName(QStringView s) const { return s.isEmpty(); }
    void dump(const Sink &sink) const { Base::dump(sink, name(), hasSquareBrackets()); }
};

class Field final : public Base
{
public:
    Field() = default;
    Field(FieldName n) : fieldName(n) {}
    // No QString/QStringView constructors: Field is reserved for the symbolic
    // names listed in QMLDOM_FIELD_LIST. Runtime/free-form names are Keys, and
    // string input goes through the path parser (Path::fromString).
    QString name() const { return fieldNameToQString(fieldName); }
    bool checkName(QStringView s) const { return s == fieldNameToString(fieldName); }
    QStringView stringView() const { return fieldNameToString(fieldName); }
    void dump(const Sink &sink) const { sink(fieldNameToString(fieldName)); }

    FieldName fieldName = FieldName(0);
};

class Index final : public Base
{
public:
    Index() = default;
    Index(index_type i): indexValue(i) {}
    QString name() const { return QString::number(indexValue); }
    bool checkName(QStringView s) const { return s == name(); }
    index_type index(index_type = -1) const { return indexValue; }
    void dump(const Sink &sink) const { Base::dump(sink, name(), hasSquareBrackets()); }
    bool hasSquareBrackets() const { return true; }

    index_type indexValue = -1;
};

class Key final : public Base
{
public:
    Key() = default;
    Key(const QString &n) : keyValue(n) { }
    QString name() const { return keyValue; }
    bool checkName(QStringView s) const { return s == keyValue; }
    QStringView stringView() const { return keyValue; }
    void dump(const Sink &sink) const {
        sink(u"[");
        sinkEscaped(sink, keyValue);
        sink(u"]");
    }
    bool hasSquareBrackets() const { return true; }

    QString keyValue;
};

class Root final : public Base
{
public:
    Root() = default;
    Root(PathRoot r): contextKind(r) {}
    QString name() const {
        switch (contextKind) {
        case PathRoot::Modules:
            return QStringLiteral(u"$modules");
        case PathRoot::Cpp:
            return QStringLiteral(u"$cpp");
        case PathRoot::Libs:
            return QStringLiteral(u"$libs");
        case PathRoot::Top:
            return QStringLiteral(u"$top");
        case PathRoot::Env:
            return QStringLiteral(u"$env");
        case PathRoot::Universe:
            return QStringLiteral(u"$universe");
        }
        Q_ASSERT(false && "Unexpected contextKind in name");
        return QString();
    }
    bool checkName(QStringView s) const {
        return s.compare(name(), Qt::CaseInsensitive) == 0;
    }
    void dump(const Sink &sink) const { sink(name()); }

    PathRoot contextKind = PathRoot::Modules;
};

class Current final : public Base
{
public:
    Current() = default;
    Current(PathCurrent c): contextKind(c) {}
    QString name() const {
        switch (contextKind) {
        case PathCurrent::Obj:
            return QStringLiteral(u"@obj");
        case PathCurrent::Component:
            return QStringLiteral(u"@component");
        case PathCurrent::Module:
            return QStringLiteral(u"@module");
        case PathCurrent::Ids:
            return QStringLiteral(u"@ids");
        case PathCurrent::Types:
            return QStringLiteral(u"@types");
        case PathCurrent::LookupStrict:
            return QStringLiteral(u"@lookupStrict");
        case PathCurrent::LookupDynamic:
            return QStringLiteral(u"@lookupDynamic");
        case PathCurrent::Lookup:
            return QStringLiteral(u"@lookup");
        }
        Q_ASSERT(false && "Unexpected contextKind in Current::name");
        return QString();
    }
    bool checkName(QStringView s) const {
        return s.compare(name(), Qt::CaseInsensitive) == 0;
    }
    void dump(const Sink &sink) const { Base::dump(sink, name(), hasSquareBrackets()); }

    PathCurrent contextKind = PathCurrent::Obj;
};

class QMLDOM_EXPORT PathComponent {
public:
    PathComponent() = default;
    PathComponent(const PathComponent &) = default;
    PathComponent(PathComponent &&) = default;
    PathComponent &operator=(const PathComponent &) = default;
    PathComponent &operator=(PathComponent &&) = default;
    ~PathComponent() = default;

    Kind kind() const { return Kind(m_data.index()); }

    QString name() const
    {
        return std::visit([](auto &&d) { return d.name(); }, m_data);
    }

    bool checkName(QStringView s) const
    {
        return std::visit([s](auto &&d) { return d.checkName(s); }, m_data);
    }

    QStringView stringView() const
    {
        return std::visit([](auto &&d) { return d.stringView(); }, m_data);
    }

    index_type index(index_type defaultValue=-1) const
    {
        return std::visit([defaultValue](auto &&d) { return d.index(defaultValue); }, m_data);
    }

    void dump(const Sink &sink) const
    {
        return std::visit([sink](auto &&d) { return d.dump(sink); }, m_data);
    }

    bool hasSquareBrackets() const
    {
        return std::visit([](auto &&d) { return d.hasSquareBrackets(); }, m_data);
    }

    const Empty   *asEmpty()   const { return std::get_if<Empty>(&m_data); }
    const Field   *asField()   const { return std::get_if<Field>(&m_data); }
    const Index   *asIndex()   const { return std::get_if<Index>(&m_data); }
    const Key     *asKey()     const { return std::get_if<Key>(&m_data); }
    const Root    *asRoot()    const { return std::get_if<Root>(&m_data); }
    const Current *asCurrent() const { return std::get_if<Current>(&m_data); }

    static int cmp(const PathComponent &p1, const PathComponent &p2);

    PathComponent(Empty &&o): m_data(std::move(o)) {}
    PathComponent(Field &&o): m_data(std::move(o)) {}
    PathComponent(Index &&o): m_data(std::move(o)) {}
    PathComponent(Key &&o): m_data(std::move(o)) {}
    PathComponent(Root &&o): m_data(std::move(o)) {}
    PathComponent(Current &&o): m_data(std::move(o)) {}

    // Convenience constructors
    Q_IMPLICIT PathComponent(FieldName f): m_data(Field(f)) {}
    Q_IMPLICIT PathComponent(PathRoot r): m_data(Root(r)) {}
    Q_IMPLICIT PathComponent(PathCurrent c): m_data(Current(c)) {}

private:
    friend class QQmlJS::Dom::Path;
    friend class QQmlJS::Dom::PathEls::TestPaths;

    using Variant = std::variant<Empty, Field, Index, Key, Root, Current>;

    template<typename T, Kind K>
    static constexpr bool variantTypeMatches
            = std::is_same_v<std::variant_alternative_t<size_t(K), Variant>, T>;

    static_assert(size_t(Kind::Empty) == 0);
    static_assert(variantTypeMatches<Empty,   Kind::Empty>);
    static_assert(variantTypeMatches<Field,   Kind::Field>);
    static_assert(variantTypeMatches<Key,     Kind::Key>);
    static_assert(variantTypeMatches<Root,    Kind::Root>);
    static_assert(variantTypeMatches<Current, Kind::Current>);
    static_assert(std::variant_size_v<Variant> == size_t(Kind::Current) + 1);

    Variant m_data;
};

inline bool operator==(const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) == 0; }
inline bool operator!=(const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) != 0; }
inline bool operator< (const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) <  0; }
inline bool operator> (const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) >  0; }
inline bool operator<=(const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) <= 0; }
inline bool operator>=(const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) >= 0; }

class PathData {
public:
    PathData(const QList<PathComponent> &components)
        : components(components)
    {}
    PathData(const QList<PathComponent> &components,
             const std::shared_ptr<PathData> &parent)
        : components(components), parent(parent)
    {}

    QList<PathComponent> components;
    std::shared_ptr<PathData> parent;
};

} // namespace PathEls


class Source;
size_t qHash(const Path &, size_t);
class PathIterator;
// Define a iterator for it?
// begin() can basically be itself, end() the empty path (zero length), iteration though dropFront()
class QMLDOM_EXPORT Path{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(ErrorGroup);
public:
    using Kind = PathEls::Kind;
    using Component = PathEls::PathComponent;
    static ErrorGroups myErrors(); // use static consts and central registration instead?

    Path() = default;
    explicit Path(const PathEls::PathComponent &c) : m_endOffset(0), m_length(0)
    {
        *this = withComponent(c);
    }

    int length() const { return m_length; }
    Path operator[](int i) const;
    explicit operator bool() const;

    PathIterator begin() const;
    PathIterator end() const;

    PathRoot headRoot() const;
    PathCurrent headCurrent() const;
    Kind headKind() const;
    QString headName() const;
    bool checkHeadName(QStringView name) const;
    bool checkHeadName(FieldName name) const
    {
        const auto f = headField();
        return f && *f == name;
    }
    std::optional<FieldName> headField() const;
    index_type headIndex(index_type defaultValue=-1) const;
    Path head() const;
    Path last() const;
    Source split() const;

    void dump(const Sink &sink) const;
    QString toString() const;
    Path dropFront(int n = 1) const;
    Path dropTail(int n = 1) const;
    Path mid(int offset, int length) const;
    Path mid(int offset) const;
    Path withComponent(const PathEls::PathComponent &c);

    // # Path construction
    static Path fromString(QStringView s, const ErrorHandler &errorHandler = nullptr);
    static Path fromRoot(PathRoot r);
    static Path fromIndex(index_type i);
    static Path fromField(FieldName name);
    static Path fromKey(QStringView s=u"");
    static Path fromKey(const QString &s);
    static Path fromCurrent(PathCurrent c);
    // add
    Path withEmpty() const;
    Path withField(FieldName name) const;
    Path withKey(const QString &name) const;
    Path withKey(QStringView name) const;
    Path withIndex(index_type i) const;
    Path withCurrent(PathCurrent s) const;
    Path withPath(const Path &toAdd, bool avoidToAddAsBase = false) const;

    Path expandFront() const;
    Path expandBack() const;

    Path &operator++();
    Path operator ++(int);

    // iterator traits
    using difference_type = long;
    using value_type = Path;
    using pointer = const Component*;
    using reference = const Path&;
    using iterator_category = std::forward_iterator_tag;

    static int cmp(const Path &p1, const Path &p2);

private:
    const Component &component(int i) const;
    explicit Path(quint16 endOffset, quint16 length,
                  const std::shared_ptr<PathEls::PathData> &data);
    friend class QQmlJS::Dom::PathEls::TestPaths;
    friend class FieldFilter;
    friend size_t qHash(const Path &, size_t);

    Path noEndOffset() const;

    quint16 m_endOffset = 0;
    quint16 m_length = 0;
    std::shared_ptr<PathEls::PathData> m_data = {};
};

inline bool operator==(const Path &lhs, const Path &rhs)
{
    return lhs.length() == rhs.length() && Path::cmp(lhs, rhs) == 0;
}
inline bool operator!=(const Path &lhs, const Path &rhs)
{
    return lhs.length() != rhs.length() || Path::cmp(lhs, rhs) != 0;
}
inline bool operator<(const Path &lhs, const Path &rhs)
{
    return Path::cmp(lhs, rhs) < 0;
}
inline bool operator>(const Path &lhs, const Path &rhs)
{
    return Path::cmp(lhs, rhs) > 0;
}
inline bool operator<=(const Path &lhs, const Path &rhs)
{
    return Path::cmp(lhs, rhs) <= 0;
}
inline bool operator>=(const Path &lhs, const Path &rhs)
{
    return Path::cmp(lhs, rhs) >= 0;
}

class PathIterator {
public:
    Path currentEl;
    Path operator *() const { return currentEl.head(); }
    PathIterator operator ++() { currentEl = currentEl.dropFront(); return *this; }
    PathIterator operator ++(int) { PathIterator res{currentEl}; currentEl = currentEl.dropFront(); return res; }
    bool operator ==(const PathIterator &o) const { return currentEl == o.currentEl; }
    bool operator !=(const PathIterator &o) const { return currentEl != o.currentEl; }
};

class Source {
public:
    Path pathToSource;
    Path pathFromSource;
};

inline size_t qHash(const Path &path, size_t seed)
{
    const size_t bufSize = 256;
    size_t buf[bufSize];
    size_t *it = &buf[0];
    *it++ = path.length();
    if (path.length()>0) {
        int iPath = path.length();
        size_t maxPath = bufSize / 2 - 1;
        size_t endPath = (size_t(iPath) > maxPath) ? maxPath - iPath : 0;
        while (size_t(iPath) > endPath) {
            Path p = path[--iPath];
            Path::Kind k = p.headKind();
            *it++ = size_t(k);
            *it++ = qHash(p.component(0).stringView(), seed)^size_t(p.headRoot())^size_t(p.headCurrent());
        }
    }

    // TODO: Get rid of the reinterpret_cast.
    // Rather hash the path components in a more structured way.
    return qHash(QByteArray::fromRawData(reinterpret_cast<char *>(&buf[0]), (it - &buf[0])*sizeof(size_t)), seed);
}

inline QDebug operator<<(QDebug debug, const Path &p)
{
    debug << p.toString();
    return debug;
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE

#endif // QMLDOM_PATH_H
