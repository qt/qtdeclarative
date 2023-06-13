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
#include <QtCore/QMetaEnum>
#include <QtCore/QString>
#include <QtCore/QStringView>
#include <QtCore/QStringList>
#include <QtCore/QVector>
#include <QtCore/QDebug>

#include <functional>
#include <iterator>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class ErrorGroups;
class ErrorMessage;
class DomItem;
class Path;

using ErrorHandler = std::function<void(const ErrorMessage &)> ;

using index_type = qint64;

namespace PathEls {

enum class Kind{
    Empty,
    Field,
    Index,
    Key,
    Root,
    Current,
    Any,
    Filter
};

class TestPaths;
class Empty;
class Field;
class Index;
class Key;
class Root;
class Current;
class Any;
class Filter;

class Base {
public:
    virtual ~Base() = default;
    virtual Kind kind() const = 0;
    virtual QString name() const = 0;
    virtual bool checkName(QStringView s) const = 0;
    virtual QStringView stringView() const { return QStringView(); }
    virtual index_type index(index_type defaultValue=-1) const { return defaultValue; }

    virtual void dump(Sink sink) const;
    virtual bool hasSquareBrackets() const { return false; }

    // casting, could use optional, but that is c++17...
    virtual const Empty *asEmpty() const { return nullptr; }
    virtual const Field *asField() const { return nullptr; }
    virtual const Index *asIndex() const { return nullptr; }
    virtual const Key *asKey() const { return nullptr; }
    virtual const Root *asRoot() const { return nullptr; }
    virtual const Current *asCurrent() const { return nullptr; }
    virtual const Any *asAny() const { return nullptr; }
    virtual const Filter *asFilter() const { return nullptr; }
};

class Empty final : public Base
{
public:
    Empty() = default;
    Kind kind() const override { return Kind::Empty; }
    QString name() const override { return QString(); }
    bool checkName(QStringView s) const override { return s.isEmpty(); }
    const Empty * asEmpty() const override { return this; }
};

class Field final : public Base
{
public:
    Field() = default;
    Field(QStringView n): fieldName(n) {}
    Kind kind() const override { return Kind::Field; }
    QString name() const override { return fieldName.toString(); }
    bool checkName(QStringView s) const override { return s == fieldName; }
    QStringView stringView() const override { return fieldName; }
    const Field * asField() const override { return this; }
    void dump(Sink sink) const override { sink(fieldName); }

    QStringView fieldName;
};

class Index final : public Base
{
public:
    Index() = default;
    Index(index_type i): indexValue(i) {}
    Kind kind() const override { return Kind::Index; }
    QString name() const override { return QString::number(indexValue); }
    bool checkName(QStringView s) const override { return s == name(); }
    index_type index(index_type = -1) const override { return indexValue; }
    bool hasSquareBrackets() const override { return true; }
    const Index * asIndex() const override { return this; }

    index_type indexValue = -1;
};

class Key final : public Base
{
public:
    Key() = default;
    Key(QString n) : keyValue(n) { }
    Kind kind() const override { return Kind::Key; }
    QString name() const override { return keyValue; }
    bool checkName(QStringView s) const override { return s == keyValue; }
    QStringView stringView() const override { return keyValue; }
    void dump(Sink sink) const override {
        sink(u"[");
        sinkEscaped(sink, keyValue);
        sink(u"]");
    }
    bool hasSquareBrackets() const override { return true; }
    const Key * asKey() const override { return this; }

    QString keyValue;
};

class Root final : public Base
{
public:
    Root() = default;
    Root(PathRoot r): contextKind(r), contextName() {}
    Root(QStringView n) {
        QMetaEnum metaEnum = QMetaEnum::fromType<PathRoot>();
        contextKind = PathRoot::Other;
        for (int i = 0; i < metaEnum.keyCount(); ++ i)
            if (n.compare(QString::fromUtf8(metaEnum.key(i)), Qt::CaseInsensitive) == 0)
                contextKind = PathRoot(metaEnum.value(i));
        if (contextKind == PathRoot::Other)
            contextName = n;
    }
    Kind kind() const override { return Kind::Root; }
    QString name() const override {
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
        case PathRoot::Other:
            return QString::fromUtf8("$").append(contextName.toString());
        }
        Q_ASSERT(false && "Unexpected contextKind in name");
        return QString();
    }
    bool checkName(QStringView s) const override {
        if (contextKind != PathRoot::Other)
            return s.compare(name(), Qt::CaseInsensitive) == 0;
        return s.startsWith(QChar::fromLatin1('$')) && s.mid(1) == contextName;
    }
    QStringView stringView() const override { return contextName; }
    void dump(Sink sink) const override {
        sink(name());
    }
    const Root *asRoot() const override { return this; }

    PathRoot contextKind = PathRoot::Other;
    QStringView contextName;
};

class Current final : public Base
{
public:
    Current() = default;
    Current(PathCurrent c): contextKind(c) {}
    Current(QStringView n) {
        QMetaEnum metaEnum = QMetaEnum::fromType<PathCurrent>();
        contextKind = PathCurrent::Other;
        for (int i = 0; i < metaEnum.keyCount(); ++ i)
            if (n.compare(QString::fromUtf8(metaEnum.key(i)), Qt::CaseInsensitive) == 0)
                contextKind = PathCurrent(metaEnum.value(i));
        if (contextKind == PathCurrent::Other)
            contextName = n;
    }
    Kind kind() const override { return Kind::Current; }
    QString name() const override {
        switch (contextKind) {
        case PathCurrent::Other:
            return QString::fromUtf8("@").append(contextName.toString());
        case PathCurrent::Obj:
            return QStringLiteral(u"@obj");
        case PathCurrent::ObjChain:
            return QStringLiteral(u"@objChain");
        case PathCurrent::ScopeChain:
            return QStringLiteral(u"@scopeChain");
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
    bool checkName(QStringView s) const override {
        if (contextKind != PathCurrent::Other)
            return s.compare(name(), Qt::CaseInsensitive) == 0;
        return s.startsWith(QChar::fromLatin1('@')) && s.mid(1) == contextName;
    }
    QStringView stringView() const override { return contextName; }
    const Current *asCurrent() const override { return this; }

    PathCurrent contextKind = PathCurrent::Other;
    QStringView contextName;
};

class Any final : public Base
{
public:
    Any() = default;
    Kind kind() const override { return Kind::Any; }
    QString name() const override { return QLatin1String("*"); }
    bool checkName(QStringView s) const override { return s == u"*"; }
    bool hasSquareBrackets() const override { return true; }
    const Any *asAny() const override { return this; }
};

class QMLDOM_EXPORT Filter final : public Base
{
public:
    Filter() = default;
    Filter(std::function<bool(DomItem)> f, QStringView filterDescription = u"<native code filter>");
    Kind kind() const override { return Kind::Filter; }
    QString name() const override;
    bool checkName(QStringView s) const override;
    QStringView stringView() const override { return filterDescription; }
    bool hasSquareBrackets() const override { return true; }
    const Filter *asFilter() const override { return this; }

    std::function<bool(DomItem)> filterFunction;
    QStringView filterDescription;
};

class QMLDOM_EXPORT PathComponent {
public:
    PathComponent(): data() {}
    ~PathComponent();

    Kind kind() const { return base()->kind(); }
    QString name() const { return base()->name(); };
    bool checkName(QStringView s) const { return base()->checkName(s); }
    QStringView stringView() const { return base()->stringView(); };
    index_type index(index_type defaultValue=-1) const { return base()->index(defaultValue); }
    void dump(Sink sink) const { base()->dump(sink); }
    bool hasSquareBrackets() const { return base()->hasSquareBrackets(); }

    const Empty *asEmpty() const { return base()->asEmpty(); }
    const Field *asField() const { return base()->asField(); }
    const Index *asIndex() const { return base()->asIndex(); }
    const Key *asKey() const { return base()->asKey(); }
    const Root *asRoot() const { return base()->asRoot(); }
    const Current *asCurrent() const { return base()->asCurrent(); }
    const Any *asAny() const { return base()->asAny(); }
    static int cmp(const PathComponent &p1, const PathComponent &p2);

    PathComponent(const Empty &o): data(o) {}
    PathComponent(const Field &o): data(o) {}
    PathComponent(const Index &o): data(o) {}
    PathComponent(const Key &o): data(o) {}
    PathComponent(const Root &o): data(o) {}
    PathComponent(const Current &o): data(o) {}
    PathComponent(const Any &o): data(o) {}
    PathComponent(const Filter &o): data(o) {}
private:
    friend class QQmlJS::Dom::Path;
    friend class QQmlJS::Dom::PathEls::TestPaths;

    Base *base() {
        return reinterpret_cast<Base*>(&data);
    }
    const Base *base() const {
        return reinterpret_cast<const Base*>(&data);
    }
    union Data {
        Data(): empty() { }
        Data(const Data &d) {
            switch (d.kind()){
            case Kind::Empty:
                Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&empty) && "non C++11 compliant compiler");
                new (&empty) Empty(d.empty);
                break;
            case Kind::Field:
                Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&field) && "non C++11 compliant compiler");
                new (&field) Field(d.field);
                break;
            case Kind::Index:
                Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&index) && "non C++11 compliant compiler");
                new (&index) Index(d.index);
                break;
            case Kind::Key:
                Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&key) && "non C++11 compliant compiler");
                new (&key) Key(d.key);
                break;
            case Kind::Root:
                Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&root) && "non C++11 compliant compiler");
                new (&root) Root(d.root);
                break;
            case Kind::Current:
                Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&current) && "non C++11 compliant compiler");
                new (&current) Current(d.current);
                break;
            case Kind::Any:
                Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&any) && "non C++11 compliant compiler");
                new (&any) Any(d.any);
                break;
            case Kind::Filter:
                Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&filter) && "non C++11 compliant compiler");
                new (&filter) Filter(d.filter);
                break;
            }
        }
        Data(const Empty &o) {
            Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&empty) && "non C++11 compliant compiler");
            new (&empty) Empty(o);
        }
        Data(const Field &o) {
            Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&field) && "non C++11 compliant compiler");
            new (&field) Field(o);
        }
        Data(const Index &o){
            Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&index) && "non C++11 compliant compiler");
            new (&index) Index(o);
        }
        Data(const Key &o) {
            Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&key) && "non C++11 compliant compiler");
            new (&key) Key(o);
        }
        Data(const Root &o) {
            Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&root) && "non C++11 compliant compiler");
            new (&root) Root(o);
        }
        Data(const Current &o) {
            Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&current) && "non C++11 compliant compiler");
            new (&current) Current(o);
        }
        Data(const Any &o) {
            Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&any) && "non C++11 compliant compiler");
            new (&any) Any(o);
        }
        Data(const Filter &o) {
            Q_ASSERT(static_cast<void*>(this)==static_cast<void*>(&filter) && "non C++11 compliant compiler");
            new (&filter) Filter(o);
        }
        Data &operator=(const Data &d) {
            Q_ASSERT(this != &d);
            this->~Data(); // destruct & construct new...
            new (this)Data(d);
            return *this;
        }
        Kind kind() const {
            return reinterpret_cast<const Base*>(this)->kind();
        }
        ~Data() {
            reinterpret_cast<const Base*>(this)->~Base();
        }
        Empty empty;
        Field field;
        Index index;
        Key key;
        Root root;
        Current current;
        Any any;
        Filter filter;
    } data;
};

inline bool operator==(const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) == 0; }
inline bool operator!=(const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) != 0; }
inline bool operator< (const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) <  0; }
inline bool operator> (const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) >  0; }
inline bool operator<=(const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) <= 0; }
inline bool operator>=(const PathComponent& lhs, const PathComponent& rhs){ return PathComponent::cmp(lhs,rhs) >= 0; }

class PathData {
public:
    PathData(QStringList strData, QVector<PathComponent> components): strData(strData), components(components) {}
    PathData(QStringList strData, QVector<PathComponent> components, std::shared_ptr<PathData> parent):
        strData(strData), components(components), parent(parent) {}

    QStringList strData;
    QVector<PathComponent> components;
    std::shared_ptr<PathData> parent;
};

} // namespace PathEls

#define QMLDOM_USTRING(s) u##s
#define QMLDOM_FIELD(name) inline constexpr const auto name = QMLDOM_USTRING(#name)
/*!
   \internal
   In an ideal world, the Fields namespace would be an enum, not strings.
   Use FieldType whenever you expect a static String from the Fields namespace instead of an
   arbitrary QStringView.
 */
using FieldType = QStringView;
// namespace, so it cam be reopened to add more entries
namespace Fields{
QMLDOM_FIELD(access);
QMLDOM_FIELD(accessSemantics);
QMLDOM_FIELD(allSources);
QMLDOM_FIELD(alternative);
QMLDOM_FIELD(annotations);
QMLDOM_FIELD(arguments);
QMLDOM_FIELD(astComments);
QMLDOM_FIELD(astRelocatableDump);
QMLDOM_FIELD(attachedType);
QMLDOM_FIELD(attachedTypeName);
QMLDOM_FIELD(autoExports);
QMLDOM_FIELD(base);
QMLDOM_FIELD(binaryExpression);
QMLDOM_FIELD(bindable);
QMLDOM_FIELD(bindingElement);
QMLDOM_FIELD(bindingType);
QMLDOM_FIELD(bindings);
QMLDOM_FIELD(block);
QMLDOM_FIELD(body);
QMLDOM_FIELD(callee);
QMLDOM_FIELD(canonicalFilePath);
QMLDOM_FIELD(canonicalPath);
QMLDOM_FIELD(children);
QMLDOM_FIELD(classNames);
QMLDOM_FIELD(code);
QMLDOM_FIELD(commentedElements);
QMLDOM_FIELD(comments);
QMLDOM_FIELD(components);
QMLDOM_FIELD(condition);
QMLDOM_FIELD(consequence);
QMLDOM_FIELD(contents);
QMLDOM_FIELD(contentsDate);
QMLDOM_FIELD(cppType);
QMLDOM_FIELD(currentExposedAt);
QMLDOM_FIELD(currentIsValid);
QMLDOM_FIELD(currentItem);
QMLDOM_FIELD(currentRevision);
QMLDOM_FIELD(declarations);
QMLDOM_FIELD(defaultPropertyName);
QMLDOM_FIELD(defaultValue);
QMLDOM_FIELD(designerSupported);
QMLDOM_FIELD(elLocation);
QMLDOM_FIELD(elements);
QMLDOM_FIELD(elementCanonicalPath);
QMLDOM_FIELD(enumerations);
QMLDOM_FIELD(errors);
QMLDOM_FIELD(exportSource);
QMLDOM_FIELD(exports);
QMLDOM_FIELD(expr);
QMLDOM_FIELD(expression);
QMLDOM_FIELD(expressionType);
QMLDOM_FIELD(extensionTypeName);
QMLDOM_FIELD(fileLocationsTree);
QMLDOM_FIELD(fileName);
QMLDOM_FIELD(forStatement);
QMLDOM_FIELD(fullRegion);
QMLDOM_FIELD(get);
QMLDOM_FIELD(globalScopeName);
QMLDOM_FIELD(globalScopeWithName);
QMLDOM_FIELD(hasCallback);
QMLDOM_FIELD(hasCustomParser);
QMLDOM_FIELD(idStr);
QMLDOM_FIELD(identifier);
QMLDOM_FIELD(ids);
QMLDOM_FIELD(implicit);
QMLDOM_FIELD(import);
QMLDOM_FIELD(importId);
QMLDOM_FIELD(importScope);
QMLDOM_FIELD(importSources);
QMLDOM_FIELD(imported);
QMLDOM_FIELD(imports);
QMLDOM_FIELD(inProgress);
QMLDOM_FIELD(infoItem);
QMLDOM_FIELD(inheritVersion);
QMLDOM_FIELD(initializer);
QMLDOM_FIELD(interfaceNames);
QMLDOM_FIELD(isAlias);
QMLDOM_FIELD(isComposite);
QMLDOM_FIELD(isConstructor);
QMLDOM_FIELD(isCreatable);
QMLDOM_FIELD(isDefaultMember);
QMLDOM_FIELD(isFinal);
QMLDOM_FIELD(isInternal);
QMLDOM_FIELD(isLatest);
QMLDOM_FIELD(isList);
QMLDOM_FIELD(isPointer);
QMLDOM_FIELD(isReadonly);
QMLDOM_FIELD(isRequired);
QMLDOM_FIELD(isSignalHandler);
QMLDOM_FIELD(isSingleton);
QMLDOM_FIELD(isValid);
QMLDOM_FIELD(jsFileWithPath);
QMLDOM_FIELD(kind);
QMLDOM_FIELD(lastRevision);
QMLDOM_FIELD(lastValidRevision);
QMLDOM_FIELD(left);
QMLDOM_FIELD(loadInfo);
QMLDOM_FIELD(loadOptions);
QMLDOM_FIELD(loadPaths);
QMLDOM_FIELD(loadsWithWork);
QMLDOM_FIELD(localOffset);
QMLDOM_FIELD(location);
QMLDOM_FIELD(logicalPath);
QMLDOM_FIELD(majorVersion);
QMLDOM_FIELD(metaRevisions);
QMLDOM_FIELD(methodType);
QMLDOM_FIELD(methods);
QMLDOM_FIELD(minorVersion);
QMLDOM_FIELD(moduleIndex);
QMLDOM_FIELD(moduleIndexWithUri);
QMLDOM_FIELD(moduleScope);
QMLDOM_FIELD(nAllLoadedCallbacks);
QMLDOM_FIELD(nCallbacks);
QMLDOM_FIELD(nLoaded);
QMLDOM_FIELD(nNotdone);
QMLDOM_FIELD(name);
QMLDOM_FIELD(newlinesBefore);
QMLDOM_FIELD(nextComponent);
QMLDOM_FIELD(nextScope);
QMLDOM_FIELD(notify);
QMLDOM_FIELD(objects);
QMLDOM_FIELD(onAttachedObject);
QMLDOM_FIELD(operation);
QMLDOM_FIELD(options);
QMLDOM_FIELD(parameters);
QMLDOM_FIELD(parent);
QMLDOM_FIELD(parentObject);
QMLDOM_FIELD(path);
QMLDOM_FIELD(plugins);
QMLDOM_FIELD(postCode);
QMLDOM_FIELD(postCommentLocations);
QMLDOM_FIELD(postComments);
QMLDOM_FIELD(pragma);
QMLDOM_FIELD(pragmas);
QMLDOM_FIELD(preCode);
QMLDOM_FIELD(preCommentLocations);
QMLDOM_FIELD(preComments);
QMLDOM_FIELD(properties);
QMLDOM_FIELD(propertyDef);
QMLDOM_FIELD(propertyDefRef);
QMLDOM_FIELD(propertyDefs);
QMLDOM_FIELD(propertyInfos);
QMLDOM_FIELD(propertyName);
QMLDOM_FIELD(prototypes);
QMLDOM_FIELD(qmlDirectoryWithPath);
QMLDOM_FIELD(qmlFileWithPath);
QMLDOM_FIELD(qmlFiles);
QMLDOM_FIELD(qmldirFileWithPath);
QMLDOM_FIELD(qmldirWithPath);
QMLDOM_FIELD(qmltypesFileWithPath);
QMLDOM_FIELD(qmltypesFiles);
QMLDOM_FIELD(qualifiedImports);
QMLDOM_FIELD(queue);
QMLDOM_FIELD(rawComment);
QMLDOM_FIELD(read);
QMLDOM_FIELD(referredObject);
QMLDOM_FIELD(referredObjectPath);
QMLDOM_FIELD(regionComments);
QMLDOM_FIELD(regions);
QMLDOM_FIELD(requestedAt);
QMLDOM_FIELD(requestingUniverse);
QMLDOM_FIELD(returnType);
QMLDOM_FIELD(returnTypeName);
QMLDOM_FIELD(right);
QMLDOM_FIELD(rootComponent);
QMLDOM_FIELD(scopeType);
QMLDOM_FIELD(scriptElement);
QMLDOM_FIELD(sources);
QMLDOM_FIELD(statements);
QMLDOM_FIELD(status);
QMLDOM_FIELD(stringValue);
QMLDOM_FIELD(subComponents);
QMLDOM_FIELD(subImports);
QMLDOM_FIELD(subItems);
QMLDOM_FIELD(symbol);
QMLDOM_FIELD(symbols);
QMLDOM_FIELD(target);
QMLDOM_FIELD(targetPropertyName);
QMLDOM_FIELD(text);
QMLDOM_FIELD(type);
QMLDOM_FIELD(typeArgument);
QMLDOM_FIELD(typeName);
QMLDOM_FIELD(types);
QMLDOM_FIELD(universe);
QMLDOM_FIELD(updatedScriptExpressions);
QMLDOM_FIELD(uri);
QMLDOM_FIELD(uris);
QMLDOM_FIELD(validExposedAt);
QMLDOM_FIELD(validItem);
QMLDOM_FIELD(value);
QMLDOM_FIELD(valueTypeName);
QMLDOM_FIELD(values);
QMLDOM_FIELD(version);
QMLDOM_FIELD(when);
QMLDOM_FIELD(write);
} // namespace Fields

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
        *this = appendComponent(c);
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
    index_type headIndex(index_type defaultValue=-1) const;
    std::function<bool(DomItem)> headFilter() const;
    Path head() const;
    Path last() const;
    Source split() const;

    void dump(Sink sink) const;
    QString toString() const;
    Path dropFront(int n = 1) const;
    Path dropTail(int n = 1) const;
    Path mid(int offset, int length) const;
    Path mid(int offset) const;
    Path appendComponent(const PathEls::PathComponent &c);

    // # Path construction
    static Path fromString(QString s, ErrorHandler errorHandler=nullptr);
    static Path fromString(QStringView s, ErrorHandler errorHandler=nullptr);
    static Path Root(PathRoot r);
    static Path Root(QStringView s=u"");
    static Path Root(QString s);
    static Path Index(index_type i);
    static Path Field(QStringView s=u"");
    static Path Field(QString s);
    static Path Key(QStringView s=u"");
    static Path Key(QString s);
    static Path Current(PathCurrent c);
    static Path Current(QStringView s=u"");
    static Path Current(QString s);
    static Path Empty();
    // add
    Path empty() const;
    Path field(QString name) const;
    Path field(QStringView name) const;
    Path key(QString name) const;
    Path key(QStringView name) const;
    Path index(index_type i) const;
    Path any() const;
    Path filter(std::function<bool(DomItem)>, QString) const;
    Path filter(std::function<bool(DomItem)>, QStringView desc=u"<native code filter>") const;
    Path current(PathCurrent s) const;
    Path current(QString s) const;
    Path current(QStringView s=u"") const;
    Path path(Path toAdd, bool avoidToAddAsBase = false) const;

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
    explicit Path(quint16 endOffset, quint16 length, std::shared_ptr<PathEls::PathData> data);
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
