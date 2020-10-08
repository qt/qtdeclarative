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

class Empty: public Base {
public:
    Kind kind() const override { return Kind::Empty; }
    QString name() const override { return QString(); }
    bool checkName(QStringView s) const override { return s.isEmpty(); }
    const Empty * asEmpty() const override { return this; }
};

class Field: public Base {
public:
    Field(QStringView n): fieldName(n) {}
    Kind kind() const override { return Kind::Field; }
    QString name() const override { return fieldName.toString(); }
    bool checkName(QStringView s) const override { return s == fieldName; }
    QStringView stringView() const override { return fieldName; }
    const Field * asField() const override { return this; }
    void dump(Sink sink) const override { sink(fieldName); }

    QStringView fieldName;
};

class Index: public Base {
public:
    Index(index_type i): indexValue(i) {}
    Kind kind() const override { return Kind::Index; }
    QString name() const override { return QString::number(indexValue); }
    bool checkName(QStringView s) const override { return s == name(); }
    index_type index(index_type = -1) const override { return indexValue; }
    bool hasSquareBrackets() const override { return true; }
    const Index * asIndex() const override { return this; }

    index_type indexValue;
};

class Key: public Base {
public:
    Key(QStringView n): keyValue(n) {}
    Kind kind() const override { return Kind::Key; }
    QString name() const override { return keyValue.toString(); }
    bool checkName(QStringView s) const override { return s == keyValue; }
    QStringView stringView() const override { return keyValue; }
    void dump(Sink sink) const override {
        sink(u"[");
        sinkEscaped(sink, keyValue);
        sink(u"]");
    }
    bool hasSquareBrackets() const override { return true; }
    const Key * asKey() const override { return this; }

    QStringView keyValue;
};

class Root: public Base {
public:
    Root(): contextKind(PathRoot::Other), contextName() {}
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

    PathRoot contextKind;
    QStringView contextName;
};

class Current: public Base {
public:
    Current(): contextName() {}
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

    PathCurrent contextKind;
    QStringView contextName;
};

class Any: public Base {
public:
    Kind kind() const override { return Kind::Any; }
    QString name() const override { return QLatin1String("*"); }
    bool checkName(QStringView s) const override { return s == u"*"; }
    bool hasSquareBrackets() const override { return true; }
    const Any *asAny() const override { return this; }
};

class QMLDOM_EXPORT Filter: public Base {
public:
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
private:
    friend class QQmlJS::Dom::Path;
    friend class QQmlJS::Dom::PathEls::TestPaths;

    PathComponent(const Empty &o): data(o) {}
    PathComponent(const Field &o): data(o) {}
    PathComponent(const Index &o): data(o) {}
    PathComponent(const Key &o): data(o) {}
    PathComponent(const Root &o): data(o) {}
    PathComponent(const Current &o): data(o) {}
    PathComponent(const Any &o): data(o) {}
    PathComponent(const Filter &o): data(o) {}

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

#define QMLDOM_USTRING(name) constexpr const auto name = u#name
// namespace, so it cam be reopened to add more entries
namespace Fields{
constexpr const auto access = u"access";
constexpr const auto annotations = u"annotations";
constexpr const auto attachedType = u"attachedType";
constexpr const auto attachedTypeName = u"attachedTypeName";
constexpr const auto autoExport = u"autoExport";
constexpr const auto base = u"base";
constexpr const auto bindingType = u"bindingType";
constexpr const auto bindings = u"bindings";
constexpr const auto body = u"body";
constexpr const auto canonicalFilePath = u"canonicalFilePath";
constexpr const auto canonicalPath = u"canonicalPath";
constexpr const auto children = u"children";
constexpr const auto classNames = u"classNames";
constexpr const auto code = u"code";
constexpr const auto components = u"components";
constexpr const auto contents = u"contents";
constexpr const auto contentsDate = u"contentsDate";
constexpr const auto currentExposedAt = u"currentExposedAt";
constexpr const auto currentIsValid = u"currentIsValid";
constexpr const auto currentItem = u"currentItem";
constexpr const auto currentRevision = u"currentRevision";
constexpr const auto defaultPropertyName = u"defaultPropertyName";
constexpr const auto designerSupported = u"designerSupported";
constexpr const auto elementCanonicalPath = u"elementCanonicalPath";
constexpr const auto enumerations = u"enumerations";
constexpr const auto errors = u"errors";
constexpr const auto exportSource = u"exportSource";
constexpr const auto exports = u"exports";
constexpr const auto extraRequired = u"extraRequired";
constexpr const auto fileName = u"fileName";
constexpr const auto get = u"get";
constexpr const auto globalScopeName = u"globalScopeName";
constexpr const auto globalScopeWithName = u"globalScopeWithName";
constexpr const auto hasCallback = u"hasCallback";
constexpr const auto idStr = u"idStr";
constexpr const auto ids = u"ids";
constexpr const auto import = u"import";
constexpr const auto importId = u"importId";
constexpr const auto imports = u"imports";
constexpr const auto inheritVersion = u"inheritVersion";
constexpr const auto inProgress = u"inProgress";
constexpr const auto isAlias = u"isAlias";
constexpr const auto isComposite = u"isComposite";
constexpr const auto isCreatable = u"isCreatable";
constexpr const auto isDefaultMember = u"isDefaultMember";
constexpr const auto isInternal = u"isInternal";
constexpr const auto isLatest = u"isLatest";
constexpr const auto isList = u"isList";
constexpr const auto isPointer = u"isPointer";
constexpr const auto isRequired = u"isRequired";
constexpr const auto isSingleton = u"isSingleton";
constexpr const auto isValid = u"isValid";
constexpr const auto isWritable = u"isWritable";
constexpr const auto jsFileWithPath = u"jsFileWithPath";
constexpr const auto kind = u"kind";
constexpr const auto lastRevision = u"lastRevision";
constexpr const auto lastValidRevision = u"lastValidRevision";
constexpr const auto loadInfo = u"loadInfo";
constexpr const auto loadOptions = u"loadOptions";
constexpr const auto loadPaths = u"loadPaths";
constexpr const auto loadsWithWork = u"loadsWithWork";
constexpr const auto location = u"location";
constexpr const auto logicalPath = u"logicalPath";
constexpr const auto majorVersion = u"majorVersion";
constexpr const auto metaRevisions = u"metaRevisions";
constexpr const auto methodType = u"methodType";
constexpr const auto methods = u"methods";
constexpr const auto minorVersion = u"minorVersion";
constexpr const auto moduleIndex = u"moduleIndex";
constexpr const auto moduleIndexWithUri = u"moduleIndexWithUri";
constexpr const auto moduleScope = u"moduleScope";
constexpr const auto nAllLoadedCallbacks = u"nAllLoadedCallbacks";
constexpr const auto nCallbacks = u"nCallbacks";
constexpr const auto nLoaded = u"nLoaded";
constexpr const auto nNotdone = u"nNotdone";
constexpr const auto name = u"name";
constexpr const auto nextScope = u"nextScope";
constexpr const auto objects = u"objects";
constexpr const auto onAttachedObject = u"onAttachedObject";
constexpr const auto options = u"options";
constexpr const auto parameters = u"parameters";
constexpr const auto parentObject = u"parentObject";
constexpr const auto path = u"path";
constexpr const auto plugins = u"plugins";
constexpr const auto pragma = u"pragma";
constexpr const auto pragmas = u"pragmas";
constexpr const auto propertyDef = u"propertyDef";
constexpr const auto propertyDefRef = u"propertyDefRef";
constexpr const auto propertyDefs = u"propertyDefs";
constexpr const auto propertyName = u"propertyName";
constexpr const auto prototype = u"prototype";
constexpr const auto qmlDirectoryWithPath = u"qmlDirectoryWithPath";
constexpr const auto qmlFileWithPath = u"qmlFileWithPath";
constexpr const auto qmldirFileWithPath = u"qmldirFileWithPath";
constexpr const auto qmltypesFileWithPath = u"qmltypesFileWithPath";
constexpr const auto qmltypesFiles = u"qmltypesFiles";
constexpr const auto queue = u"queue";
constexpr const auto referredObject = u"referredObject";
constexpr const auto referredObjectPath = u"referredObjectPath";
constexpr const auto requestedAt = u"requestedAt";
constexpr const auto requestingUniverse = u"requestingUniverse";
constexpr const auto returnType = u"returnType";
constexpr const auto returnTypeName = u"returnTypeName";
constexpr const auto rootComponent = u"rootComponent";
constexpr const auto sources = u"sources";
constexpr const auto status = u"status";
constexpr const auto stringValue = u"stringValue";
constexpr const auto symbols = u"symbols";
constexpr const auto target = u"target";
constexpr const auto targetPropertyName = u"targetPropertyName";
constexpr const auto type = u"type";
constexpr const auto typeName = u"typeName";
constexpr const auto types = u"types";
constexpr const auto universe = u"universe";
constexpr const auto uri = u"uri";
constexpr const auto uris = u"uris";
constexpr const auto validExposedAt = u"validExposedAt";
constexpr const auto validItem = u"validItem";
constexpr const auto value = u"value";
constexpr const auto values = u"values";
constexpr const auto version = u"version";
constexpr const auto when = u"when";
}

class Source;
size_t qHash(const Path &, size_t);

// Define a iterator for it?
// begin() can basically be itself, end() the empty path (zero length), iteration though dropFront()
class QMLDOM_EXPORT Path{
    Q_DECLARE_TR_FUNCTIONS(ErrorGroup);
public:
    using Kind = PathEls::Kind;
    using Component = PathEls::PathComponent;
    static ErrorGroups myErrors(); // use static consts and central registration instead?

    Path(){}
    int length() const { return m_length; }
    Path operator[](int i) const;
    operator bool() const;

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
    Path dropFront() const;
    Path dropTail() const;
    Path mid(int offset, int length) const;
    Path mid(int offset) const;

    // # Path construction
    static Path fromString(QString s, ErrorHandler errorHandler=nullptr);
    static Path fromString(QStringView s, ErrorHandler errorHandler=nullptr);
    static Path root(PathRoot r);
    static Path root(QStringView s=u"");
    static Path root(QString s);
    static Path index(index_type i);
    static Path field(QStringView s=u"");
    static Path field(QString s);
    static Path key(QStringView s=u"");
    static Path key(QString s);
    static Path current(PathCurrent c);
    static Path current(QStringView s=u"");
    static Path current(QString s);
    static Path empty();
    // add
    Path subEmpty() const;
    Path subField(QString name) const;
    Path subField(QStringView name) const;
    Path subKey(QString name) const;
    Path subKey(QStringView name) const;
    Path subIndex(index_type i) const;
    Path subAny() const;
    Path subFilter(std::function<bool(DomItem)>, QString) const;
    Path subFilter(std::function<bool(DomItem)>, QStringView desc=u"<native code filter>") const;
    Path subCurrent(PathCurrent s) const;
    Path subCurrent(QString s) const;
    Path subCurrent(QStringView s=u"") const;
    Path subPath(Path toAdd, bool avoidToAddAsBase = false) const;

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
    explicit Path(quint16 endOffset, quint16 length, std::shared_ptr<PathEls::PathData> data);
    friend class QQmlJS::Dom::PathEls::TestPaths;
    friend size_t qHash(const Path &, size_t);

    Component component(int i) const;
    Path noEndOffset() const;

    quint16 m_endOffset = 0;
    quint16 m_length = 0;
    std::shared_ptr<PathEls::PathData> m_data;
};

inline bool operator==(const Path& lhs, const Path& rhs){ return lhs.length() == rhs.length() && Path::cmp(lhs,rhs) == 0; }
inline bool operator!=(const Path& lhs, const Path& rhs){ return lhs.length() != rhs.length() || Path::cmp(lhs,rhs) != 0; }
inline bool operator< (const Path& lhs, const Path& rhs){ return Path::cmp(lhs,rhs) <  0; }
inline bool operator> (const Path& lhs, const Path& rhs){ return Path::cmp(lhs,rhs) >  0; }
inline bool operator<=(const Path& lhs, const Path& rhs){ return Path::cmp(lhs,rhs) <= 0; }
inline bool operator>=(const Path& lhs, const Path& rhs){ return Path::cmp(lhs,rhs) >= 0; }

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
