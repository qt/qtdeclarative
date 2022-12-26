// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqmldomtop_p.h"
#include "qqmldomelements_p.h"
#include "qqmldomexternalitems_p.h"
#include "qqmldommock_p.h"
#include "qqmldomastdumper_p.h"
#include "qqmldomoutwriter_p.h"
#include "qqmldomfilewriter_p.h"
#include "qqmldomfieldfilter_p.h"
#include "qqmldomcompare_p.h"
#include "qqmldomastdumper_p.h"
#include "qqmldomlinewriter_p.h"

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>

#include <QtCore/QCborArray>
#include <QtCore/QCborMap>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonValue>
#include <QtCore/QMutexLocker>
#include <QtCore/QPair>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
namespace {
static constexpr auto UTC = Qt::UTC;
}
#else
#include <QtCore/QTimeZone>
namespace {
static constexpr auto UTC = QTimeZone::UTC;
}
#endif

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

Q_LOGGING_CATEGORY(writeOutLog, "qt.qmldom.writeOut", QtWarningMsg);
static Q_LOGGING_CATEGORY(refLog, "qt.qmldom.ref", QtWarningMsg);

using std::shared_ptr;
/*!
\internal
\class QQmljs::Dom::DomBase

\brief Abstract class common to all elements of the Qml code model

DomBase represents the base class common to all objects exposed by the Dom Model
through a DomItem.
Single inheritence is mandatory for the inline items (Empty, Map, List, ConstantData, Reference),
so that the base pointer of the object can be used a base pointer of the superclass directly.

The subclass *must* have a
\code
    constexpr static Kind kindValue
\endcode
entry with its kind to enable casting usng the DomItem::as DomItem::ownerAs templates.

The minimal overload set to be usable is:
\code
    Kind kind() const override {  return kindValue; } // returns the kind of the current element
    Path pathFromOwner(DomItem &self) const override; // returns the path from the owner to the
current element Path canonicalPath(DomItem &self) const override; // returns the path from virtual
bool iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem)>) const = 0; // iterates
the *direct* subpaths, returns false if a quick end was requested \endcode But you probably want to
subclass either DomElement of OwningItem for your element. DomElement stores its pathFromOwner, and
computes the canonicalPath from it and its owner. OwningItem is the unit for updates to the Dom
model, exposed changes always change at least one OwningItem. They have their lifetime handled with
shared_ptr and own (i.e. are responsible of freeing) other items in them.

\sa QQml::Dom::DomItem, QQml::Dom::DomElement, QQml::Dom::OwningItem
*/

QMap<DomType,QString> domTypeToStringMap()
{
    static QMap<DomType,QString> map = [](){
        QMetaEnum metaEnum = QMetaEnum::fromType<DomType>();
         QMap<DomType,QString> res;
         for (int i = 0; i < metaEnum.keyCount(); ++ i) {
            res[DomType(metaEnum.value(i))] = QString::fromUtf8(metaEnum.key(i));
         }
        return res;
    }();
    return map;
}

QString domTypeToString(DomType k)
{
    QString res = domTypeToStringMap().value(k);
    if (res.isEmpty())
        return QString::number(int(k));
    else
        return res;
}

QMap<DomKind, QString> domKindToStringMap()
{
    static QMap<DomKind, QString> map = []() {
        QMetaEnum metaEnum = QMetaEnum::fromType<DomKind>();
        QMap<DomKind, QString> res;
        for (int i = 0; i < metaEnum.keyCount(); ++i) {
            res[DomKind(metaEnum.value(i))] = QString::fromUtf8(metaEnum.key(i));
        }
        return res;
    }();
    return map;
}

QString domKindToString(DomKind k)
{
    return domKindToStringMap().value(k, QString::number(int(k)));
}

bool domTypeIsExternalItem(DomType k)
{
    switch (k) {
    case DomType::QmlDirectory:
    case DomType::JsFile:
    case DomType::QmlFile:
    case DomType::QmltypesFile:
    case DomType::GlobalScope:
        return true;
    default:
        return false;
    }
}

bool domTypeIsTopItem(DomType k)
{
    switch (k) {
    case DomType::DomEnvironment:
    case DomType::DomUniverse:
        return true;
    default:
        return false;
    }

}

bool domTypeIsContainer(DomType k)
{
    switch (k) {
    case DomType::Map:
    case DomType::List:
    case DomType::ListP:
        return true;
    default:
        return false;
    }
}

bool domTypeIsScope(DomType k)
{
    switch (k) {
    case DomType::QmlObject: // prop, methods,...
    case DomType::ScriptExpression: // Js lexical scope
    case DomType::QmlComponent: // (ids, enums -> qmlObj)
    case DomType::QmlFile: // (components ->importScope)
    case DomType::MethodInfo: // method arguments
    case DomType::ImportScope: // (types, qualifiedImports)
    case DomType::GlobalComponent: // global scope (enums -> qmlObj)
    case DomType::JsResource: // js resurce (enums -> qmlObj)
    case DomType::QmltypesComponent: // qmltypes component (enums -> qmlObj)
        return true;
    default:
        return false;
    }
}

QCborValue locationToData(SourceLocation loc, QStringView strValue)
{
    QCborMap res({
        {QStringLiteral(u"offset"), loc.offset},
        {QStringLiteral(u"length"), loc.length},
        {QStringLiteral(u"startLine"), loc.startLine},
        {QStringLiteral(u"startColumn"), loc.startColumn}
    });
    if (!strValue.isEmpty())
        res.insert(QStringLiteral(u"strValue"), QCborValue(strValue));
    return res;
}

QString DomBase::canonicalFilePath(DomItem &self) const
{
    auto parent = containingObject(self);
    if (parent)
        return parent.canonicalFilePath();
    return QString();
}

void DomBase::writeOut(DomItem &self, OutWriter &) const
{
    qCWarning(writeOutLog) << "Ignoring unsupported writeOut for " << domTypeToString(kind()) << ":"
                           << self.canonicalPath();
}

ConstantData::ConstantData(Path pathFromOwner, QCborValue value, Options options)
    : DomElement(pathFromOwner), m_value(value), m_options(options)
{}

bool ConstantData::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    static QHash<QString, QString> knownFields;
    static QBasicMutex m;
    auto toField = [](QString f) -> QStringView {
        QMutexLocker l(&m);
        if (!knownFields.contains(f))
            knownFields[f] = f;
        return knownFields[f];
    };
    if (m_value.isMap()) {
        QCborMap map = m_value.toMap();
        auto it = map.cbegin();
        auto end = map.cend();
        while (it != end) {
            QString key = it.key().toString();
            PathEls::PathComponent comp;
            switch (m_options) {
            case ConstantData::Options::MapIsMap:
                comp = PathEls::Key(key);
                break;
            case ConstantData::Options::FirstMapIsFields:
                comp = PathEls::Field(toField(key));
                break;
            }
            auto val = it.value();
            if (!self.dvValue(visitor, comp, val))
                return false;
            ++it;
        }
        return true;
    } else if (m_value.isArray()){
        QCborArray array = m_value.toArray();
        auto it = array.cbegin();
        auto end = array.cend();
        index_type i = 0;
        while (it != end) {
            if (!self.dvValue(visitor, PathEls::Index(i++), *it++))
                return false;
        }
        return true;
    } else {
        return true;
    }
}

quintptr ConstantData::id() const
{
    return quintptr(0);
}

DomKind ConstantData::domKind() const
{
    if (m_value.isMap()) {
        switch (m_options) {
        case ConstantData::Options::MapIsMap:
            return DomKind::Map;
        case ConstantData::Options::FirstMapIsFields:
            return DomKind::Object;
        }
    }
    if (m_value.isArray())
        return DomKind::List;
    return DomKind::Value;
}
/*!
\internal
\class QQmlJS::Dom::DomItem

\brief A value type that references any element of the Dom.

This class is the central element in the Dom, it is how any element can be identfied in a uniform
way, and provides the API to explore the Dom, and Path based operations.

The DomItem (unless it is Empty) keeps a pointer to the element, and a shared pointer to its owner
and to the DomEnvironment or DomUniverse that contains them. This means that:
\list
\li A DomItem always has some context: you can get the canonicalPath(), go up along it with
    containingObject() and container().
\li the indexing operator [], or the path(), field(), key() and index() methods (along with their
    fields(), keys(), and indexes() contreparts) let one visit the contents of the current element.
\li visitTree can be used to visit all subEments, if preferred on the top of it a visitor
    pattern can also be used.
\li If element specific attributes are wanted the two template casting as and ownerAs allow safe
casting of the DomItem to a specific concrete type (cast to superclasses is not supported). \li
Multithreading does not create issues, because even if an update replacing an OwningItem takes place
the DomItem keeps a shared_ptr to the current owner as long as you use it \li Some elements (Empty,
List, Map, ConstantData, Reference) might be inline, meaning that they are generated on the fly,
wrapping data of the original object. \endlist

One of the goals of the DomItem is to allow one to use real typed objects, as one is used to in C++,
and also let one use modern C++ patterns, meaning container that contain the actual object (without
pointer indirection).
Exposed OwningItems are basically immutable, but during construction, objects can be modified.
This will typically happen from a single thread, so there aren't locking issues, but pointers to
inner elements might become invalid.
In this case the use of the MutableDomItem is required.
It does not keep any pointers to internal elements, but rather the path to them, and it resolves
it every time it needs.
*/

ErrorGroup DomItem::domErrorGroup = NewErrorGroup("Dom");
DomItem DomItem::empty = DomItem();

ErrorGroups DomItem::myErrors()
{
    static ErrorGroups res = {{domErrorGroup}};
    return res;
}

ErrorGroups DomItem::myResolveErrors()
{
    static ErrorGroups res = {{domErrorGroup, NewErrorGroup("Resolve")}};
    return res;
}

Path DomItem::canonicalPath()
{
    Path res = visitEl([this](auto &&el) { return el->canonicalPath(*this); });
    if (!(!res || res.headKind() == Path::Kind::Root)) {
        qCWarning(domLog) << "non anchored canonical path:" << res.toString();
        Q_ASSERT(false);
    }
    return res;

}

DomItem DomItem::containingObject()
{
    return visitEl([this](auto &&el) { return el->containingObject(*this); });
}

DomItem DomItem::qmlObject(GoTo options, FilterUpOptions filterOptions)
{
    if (DomItem res = filterUp([](DomType k, DomItem &) { return k == DomType::QmlObject; },
                               filterOptions))
        return res;
    if (options == GoTo::MostLikely) {
        if (DomItem comp = component(options))
            return comp.field(Fields::objects).index(0);
    }
    return DomItem();
}

DomItem DomItem::fileObject(GoTo options)
{
    DomItem res = *this;
    DomType k = res.internalKind();
    if (k == DomType::List || k == DomType::Map) {
        res = res.containingObject();
        k = res.internalKind();
    }
    if (k == DomType::ExternalItemInfo || (options == GoTo::MostLikely && k == DomType::ExternalItemPair))
        return field(Fields::currentItem);
    res = owner();
    k = res.internalKind();
    while (k != DomType::Empty) {
        if (k == DomType::QmlFile || k == DomType::QmldirFile || k == DomType::QmltypesFile
            || k == DomType::JsFile)
            break;
        res = res.containingObject();
        res = res.owner();
        k = res.internalKind();
    }
    return res;
}

DomItem DomItem::rootQmlObject(GoTo options)
{
    if (DomItem res = filterUp([](DomType k, DomItem &) { return k == DomType::QmlObject; },
                               FilterUpOptions::ReturnInner))
        return res;
    if (options == GoTo::MostLikely) {
        if (DomItem comp = component(options))
            return comp.field(Fields::objects).index(0);
    }
    return DomItem();
}

DomItem DomItem::container()
{
    Path path = pathFromOwner();
    if (!path)
        path = canonicalPath();
    Source s = path.split();
    if (s.pathFromSource.length() > 1)
        return containingObject().path(s.pathFromSource.dropTail());
    return containingObject();
}

DomItem DomItem::globalScope()
{
    if (internalKind() == DomType::GlobalScope)
        return *this;
    DomItem env = environment();
    if (shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>()) {
        return env.copy(envPtr->ensureGlobalScopeWithName(env, envPtr->globalScopeName())->current,
                        Path());
    }
    return DomItem();
}

DomItem DomItem::owner()
{
    if (domTypeIsOwningItem(m_kind) || m_kind == DomType::Empty)
        return *this;
    return std::visit(
            [this](auto &&el) { return DomItem(this->m_top, el, this->m_ownerPath, el.get()); },
            *m_owner);
}

DomItem DomItem::top()
{
    if (domTypeIsTopItem(m_kind) || m_kind == DomType::Empty)
        return *this;
    return std::visit([](auto &&el) { return DomItem(el, el, Path(), el.get()); }, *m_top);
}

DomItem DomItem::environment()
{
    DomItem res = top();
    if (res.internalKind() == DomType::DomEnvironment)
        return res;
    return DomItem(); // we are in the universe, and cannot go back to the environment...
}

DomItem DomItem::universe()
{
    DomItem res = top();
    if (res.internalKind() == DomType::DomUniverse)
            return res;
    if (res.internalKind() == DomType::DomEnvironment)
        return res.field(Fields::universe);
    return DomItem(); // we should be in an empty DomItem already...
}

DomItem DomItem::filterUp(function_ref<bool(DomType k, DomItem &)> filter, FilterUpOptions options)
{
    DomItem it = *this;
    DomType k = it.internalKind();
    switch (options) {
    case FilterUpOptions::ReturnOuter:
    case FilterUpOptions::ReturnOuterNoSelf: {
        bool checkTop = (options == FilterUpOptions::ReturnOuter);
        while (k != DomType::Empty) {
            if (checkTop && filter(k, it))
                return it;
            checkTop = true;
            if (!domTypeIsOwningItem(k)) {
                DomItem el = it.owner();
                DomItem res;
                k = DomType::Empty;
                Path pp = it.pathFromOwner();
                DomType k2 = el.internalKind();
                if (filter(k2, el)) {
                    k = k2;
                    res = el;
                }
                for (Path p : pp.mid(0, pp.length() - 1)) {
                    el = el.path(p);
                    DomType k2 = el.internalKind();
                    if (filter(k2, el)) {
                        k = k2;
                        res = el;
                    }
                }
                if (k != DomType::Empty)
                    return res;
                it = it.owner();
            }
            it = it.containingObject();
            k = it.internalKind();
        }
    } break;
    case FilterUpOptions::ReturnInner:
        while (k != DomType::Empty) {
            if (!domTypeIsOwningItem(k)) {
                DomItem el = owner();
                Path pp = pathFromOwner();
                for (Path p : pp) {
                    DomItem child = el.path(p);
                    DomType k2 = child.internalKind();
                    if (filter(k2, child))
                        return child;
                    el = child;
                }
                it = it.owner();
            }
            it = it.containingObject();
            k = it.internalKind();
        }
        break;
    }
    return DomItem();
}

DomItem DomItem::scope(FilterUpOptions options)
{
    DomItem res = filterUp([](DomType, DomItem &el) { return el.isScope(); }, options);
    return res;
}

DomItem DomItem::get(ErrorHandler h, QList<Path> *visitedRefs)
{
    if (const Reference *refPtr = as<Reference>())
        return refPtr->get(*this, h, visitedRefs);
    return DomItem();
}

QList<DomItem> DomItem::getAll(ErrorHandler h, QList<Path> *visitedRefs)
{
    if (const Reference *refPtr = as<Reference>())
        return refPtr->getAll(*this, h, visitedRefs);
    return {};
}

PropertyInfo DomItem::propertyInfoWithName(QString name)
{
    PropertyInfo pInfo;
    visitPrototypeChain([&pInfo, name](DomItem &obj) {
        return obj.visitLocalSymbolsNamed(name, [&pInfo, name](DomItem &el) {
            switch (el.internalKind()) {
            case DomType::Binding:
                pInfo.bindings.append(el);
                break;
            case DomType::PropertyDefinition:
                pInfo.propertyDefs.append(el);
                break;
            default:
                break;
            }
            return true;
        });
    });
    return pInfo;
}

QSet<QString> DomItem::propertyInfoNames()
{
    QSet<QString> res;
    visitPrototypeChain([&res](DomItem &obj) {
        res += obj.propertyDefs().keys();
        res += obj.bindings().keys();
        return true;
    });
    return res;
}

DomItem DomItem::component(GoTo options)
{
    if (DomItem res = filterUp(
                [](DomType kind, DomItem &) {
                    return kind == DomType::QmlComponent || kind == DomType::QmltypesComponent
                            || kind == DomType::GlobalComponent;
                },
                FilterUpOptions::ReturnInner))
        return res;
    if (options == GoTo::MostLikely) {
        DomItem item = *this;
        DomType kind = item.internalKind();
        if (kind == DomType::List || kind == DomType::Map) {
            item = item.containingObject();
            kind = item.internalKind();
        }
        switch (kind) {
        case DomType::ExternalItemPair:
        case DomType::ExternalItemInfo:
            item = fileObject(options);
            Q_FALLTHROUGH();
        case DomType::QmlFile:
            return item.field(Fields::components).key(QString()).index(0);
        default:
            break;
        }
    }
    return DomItem();
}

struct ResolveToDo {
    DomItem item;
    int pathIndex;
};

static QMap<LookupType, QString> lookupTypeToStringMap()
{
    static QMap<LookupType, QString> map = []() {
        QMetaEnum metaEnum = QMetaEnum::fromType<LookupType>();
        QMap<LookupType, QString> res;
        for (int i = 0; i < metaEnum.keyCount(); ++i) {
            res[LookupType(metaEnum.value(i))] = QString::fromUtf8(metaEnum.key(i));
        }
        return res;
    }();
    return map;
}

bool DomItem::resolve(Path path, DomItem::Visitor visitor, ErrorHandler errorHandler,
                      ResolveOptions options, Path fullPath, QList<Path> *visitedRefs)
{
    QList<Path> vRefs;
    Path fPath = fullPath;
    if (fullPath.length() == 0)
        fPath = path;
    if (path.length()==0)
        return visitor(fPath, *this);
    QList<QSet<quintptr>> visited(path.length() + 1);
    Path myPath = path;
    QVector<ResolveToDo> toDos(1); // invariant: always increase pathIndex to guarantee end even with only partial visited match
    if (path.headKind() == Path::Kind::Root) {
        DomItem root = *this;
        PathRoot contextId = path.headRoot();
        switch (contextId) {
        case PathRoot::Modules:
            root = root.environment().field(Fields::moduleIndexWithUri);
            break;
        case PathRoot::Cpp:
                root = root.environment()[Fields::qmltypesFileWithPath];
            break;
        case PathRoot::Libs:
            root = root.environment()[Fields::plugins];
            break;
        case PathRoot::Top:
            root = root.top();
            break;
        case PathRoot::Env:
            root = root.environment();
            break;
        case PathRoot::Universe:
            root = root.environment()[u"universe"];
            break;
        case PathRoot::Other:
            myResolveErrors().error(tr("Root context %1 is not known").arg(path.headName())).handle(errorHandler);
            return false;
        }
        toDos[0] = {root, 1};
    } else {
        toDos[0] = {*this, 0};
    }
    while (!toDos.isEmpty()) {
        auto toDo = toDos.last();
        toDos.removeLast();
        {
            auto idNow = toDo.item.id();
            if (idNow == quintptr(0) && toDo.item == *this)
                idNow = quintptr(this);
            if (idNow != quintptr(0) && visited[0].contains(idNow))
                continue;
        }
        int iPath = toDo.pathIndex;
        DomItem it = toDo.item;
        bool branchExhausted = false;
        while (iPath < path.length() && it && !branchExhausted) {
            auto idNow = it.id();
            if (idNow == quintptr() && toDo.item == *this)
                idNow = quintptr(this);
            if (idNow != quintptr(0)) {
                auto vPair = qMakePair(idNow, iPath);
                if (visited[vPair.second].contains(vPair.first))
                    break;
                visited[vPair.second].insert(vPair.first);
            }
            if (options & ResolveOption::TraceVisit && !visitor(path.mid(0,iPath), it))
                return false;
            auto cNow = path[iPath++];
            switch (cNow.headKind()) {
            case Path::Kind::Key:
                it = it.key(cNow.headName());
                break;
            case Path::Kind::Field:
                if (cNow.checkHeadName(Fields::get) && it.internalKind() == DomType::Reference) {
                    Path toResolve = it.as<Reference>()->referredObjectPath;
                    Path refRef = it.canonicalPath();
                    if (visitedRefs == nullptr) {
                        visitedRefs = &vRefs;
                    }
                    if (visitedRefs->contains(refRef)) {
                        myResolveErrors()
                                .error([visitedRefs, refRef](Sink sink) {
                                    const QString msg = tr("Circular reference:") + QLatin1Char('\n');
                                    sink(QStringView{msg});
                                    for (const Path &vPath : *visitedRefs) {
                                        sink(u"  ");
                                        vPath.dump(sink);
                                        sink(u" >\n");
                                    }
                                    refRef.dump(sink);
                                })
                                .handle(errorHandler);
                        it = DomItem();
                    } else {
                        visitedRefs->append(refRef);
                        DomItem resolveRes;
                        it.resolve(
                                toResolve,
                                [&resolveRes](Path, DomItem &r) {
                                    resolveRes = r;
                                    return false;
                                },
                                errorHandler, ResolveOption::None, toResolve, visitedRefs);
                        it = resolveRes;
                    }
                } else {
                    it = it.field(cNow.headName()); // avoid instantiation of QString?
                }
                break;
            case Path::Kind::Index:
                it = it.index(cNow.headIndex());
                break;
            case Path::Kind::Empty:
            {
                // immediate expansion, might use extra memory, but simplifies code (no suspended evaluation)
                Path toFind;
                do {
                    if (iPath >= path.length()) {
                        myResolveErrors().warning(tr("Resolve with path ending with empty path, matches nothing."))
                                .handle(errorHandler);
                        branchExhausted = true; // allow and visit all?
                        break;
                    }
                    toFind = path[iPath++];
                } while (toFind.headKind() == Path::Kind::Empty);
                QVector<Path::Kind> validFind({Path::Kind::Key, Path::Kind::Field, Path::Kind::Field, Path::Kind::Index});
                if (!validFind.contains(toFind.headKind())) {
                    myResolveErrors().error(tr("After an empty path only key, field or indexes are supported, not %1.").arg(toFind.toString()))
                            .handle(errorHandler);
                    branchExhausted = true; // allow and visit all?
                    return false;
                }
                if (!branchExhausted)
                    visitTree(
                            Path(),
                            [toFind, &toDos, iPath](Path, DomItem &item, bool) {
                                // avoid non directly attached?
                                DomItem newItem = item[toFind];
                                if (newItem)
                                    toDos.append({ newItem, iPath });
                                return true;
                            },
                            VisitOption::VisitSelf | VisitOption::Recurse
                                    | VisitOption::VisitAdopted | VisitOption::NoPath);
                branchExhausted = true;
                break;
            }
            case Path::Kind::Root:
                myResolveErrors().error(tr("Root path is supported only at the beginning, and only once, found %1 at %2 in %3")
                                        .arg(cNow.toString()).arg(iPath -1).arg(path.toString())).handle(errorHandler);
                return false;
            case Path::Kind::Current:
            {
                PathCurrent current = cNow.headCurrent();
                switch (current) {
                case PathCurrent::Other:
                    // todo
                case PathCurrent::Obj:
                    if (domKind() != DomKind::Object)
                        it = it.containingObject();
                    break;
                case PathCurrent::ObjChain: {
                    bool cont = it.visitPrototypeChain(
                            [&toDos, iPath](DomItem &subEl) {
                                toDos.append({ subEl, iPath });
                                return true;
                            },
                            VisitPrototypesOption::Normal, errorHandler, nullptr,
                            visitedRefs); // avoid passing visitedRefs?
                    if (!cont)
                        return false;
                    branchExhausted = true;
                    break;
                }
                case PathCurrent::ScopeChain: {
                    bool cont = it.visitScopeChain(
                            [&toDos, iPath](DomItem &subEl) {
                                toDos.append({ subEl, iPath });
                                return true;
                            },
                            LookupOption::Normal, errorHandler);
                    if (!cont)
                        return false;
                    branchExhausted = true;
                    break;
                }
                case PathCurrent::Component:
                    it = it.component();
                    break;
                case PathCurrent::Module:
                case PathCurrent::Ids:
                    it = it.component().ids();
                    break;
                case PathCurrent::Types:
                    it = it.component()[Fields::exports];
                    break;
                case PathCurrent::LookupStrict:
                case PathCurrent::LookupDynamic:
                case PathCurrent::Lookup: {
                    LookupOptions opt = LookupOption::Normal;
                    if (current == PathCurrent::Lookup) {
                        DomItem comp = it.component();
                        DomItem strict = comp.field(u"~strictLookup~");
                        if (!strict) {
                            DomItem env = it.environment();
                            strict = env.field(u"defaultStrictLookup");
                        }
                        if (strict && strict.value().toBool())
                            opt = opt | LookupOption::Strict;
                    } else if (current == PathCurrent::LookupStrict) {
                        opt = opt | LookupOption::Strict;
                    }
                    if (it.internalKind() == DomType::ScriptExpression) {
                        myResolveErrors()
                                .error(tr("Javascript lookups not yet implemented"))
                                .handle(errorHandler);
                        return false;
                    }
                    // enter lookup
                    auto idNow = it.id();
                    if (idNow == quintptr(0) && toDo.item == *this)
                        idNow = quintptr(this);
                    if (idNow != quintptr(0)) {
                        auto vPair = qMakePair(idNow, iPath);
                        if (visited[vPair.second].contains(vPair.first))
                            break;
                        visited[vPair.second].insert(vPair.first);
                    }
                    if (options & ResolveOption::TraceVisit && !visitor(path.mid(0, iPath), it))
                        return false;
                    if (iPath + 1 >= path.length()) {
                        myResolveErrors()
                                .error(tr("Premature end of path, expected a field specifying the "
                                          "type, and a key specifying the name to search after a "
                                          "lookup directive in %2")
                                               .arg(path.toString()))
                                .handle(errorHandler);
                        return false;
                    }
                    Path cNow = path[iPath++];
                    if (cNow.headKind() != Path::Kind::Field) {
                        myResolveErrors()
                                .error(tr("Expected a key path specifying the type to search after "
                                          "a lookup directive, not %1 at component %2 of %3")
                                               .arg(cNow.toString())
                                               .arg(iPath)
                                               .arg(path.toString()))
                                .handle(errorHandler);
                        return false;
                    }
                    QString expectedType = cNow.headName();
                    LookupType lookupType = LookupType::Symbol;
                    {
                        bool found = false;
                        auto m = lookupTypeToStringMap();
                        auto it = m.begin();
                        auto end = m.end();
                        while (it != end) {
                            if (it.value().compare(expectedType, Qt::CaseInsensitive) == 0) {
                                lookupType = it.key();
                                found = true;
                            }
                            ++it;
                        }
                        if (!found) {
                            QString types;
                            it = lookupTypeToStringMap().begin();
                            while (it != end) {
                                if (!types.isEmpty())
                                    types += QLatin1String("', '");
                                types += it.value();
                                ++it;
                            }
                            myResolveErrors()
                                    .error(tr("Type for lookup was expected to be one of '%1', not "
                                              "%2")
                                                   .arg(types, expectedType))
                                    .handle(errorHandler);
                            return false;
                        }
                    }
                    cNow = path[iPath++];
                    if (cNow.headKind() != Path::Kind::Key) {
                        myResolveErrors()
                                .error(tr("Expected a key specifying the path to search after the "
                                          "@lookup directive and type, not %1 at component %2 of "
                                          "%3")
                                               .arg(cNow.toString())
                                               .arg(iPath)
                                               .arg(path.toString()))
                                .handle(errorHandler);
                        return false;
                    }
                    QString target = cNow.headName();
                    if (target.isEmpty()) {
                        myResolveErrors()
                                .warning(tr("Path with empty lookup at component %1 of %2 will "
                                            "match nothing in %3.")
                                                 .arg(iPath)
                                                 .arg(path.toString())
                                                 .arg(it.canonicalPath().toString()))
                                .handle(errorHandler);
                        return true;
                    }
                    it.visitLookup(
                            target,
                            [&toDos, iPath](DomItem &subEl) {
                                toDos.append({ subEl, iPath });
                                return true;
                            },
                            lookupType, opt, errorHandler, &(visited[iPath]), visitedRefs);
                    branchExhausted = true;
                    break;
                }
                }
                break;
            }
            case Path::Kind::Any:
                visitTree(
                        Path(),
                        [&toDos, iPath](Path, DomItem &item, bool) {
                            toDos.append({ item, iPath });
                            return true;
                        },
                        VisitOption::VisitSelf | VisitOption::Recurse | VisitOption::VisitAdopted);
                branchExhausted = true;
                break;
            case Path::Kind::Filter:
                if (cNow.headFilter() && !cNow.headFilter()(it))
                    branchExhausted = true;
                break;
            }
        }
        // visit the resolved path
        if (!branchExhausted && iPath == path.length() && !visitor(fPath, it))
            return false;
    }
    return true;
}

DomItem DomItem::path(Path p, ErrorHandler errorHandler)
{
    if (!p)
        return *this;
    DomItem res;
    resolve(p, [&res](Path, DomItem it) {
        res = it;
        return false;
    }, errorHandler);
    return res;
}

DomItem DomItem::path(QString p, ErrorHandler errorHandler)
{
    return path(Path::fromString(p, errorHandler));
}

DomItem DomItem::path(QStringView p, ErrorHandler errorHandler)
{
    return path(Path::fromString(p, errorHandler));
}

QList<QString> DomItem::fields()
{
    return visitEl([this](auto &&el) { return el->fields(*this); });
}

DomItem DomItem::field(QStringView name)
{
    return visitEl([this, name](auto &&el) { return el->field(*this, name); });
}

index_type DomItem::indexes()
{
    return visitEl([this](auto &&el) { return el->indexes(*this); });
}

DomItem DomItem::index(index_type i)
{
    return visitEl([this, i](auto &&el) { return el->index(*this, i); });
}

bool DomItem::visitIndexes(function_ref<bool(DomItem &)> visitor)
{
    // use iterateDirectSubpathsConst instead?
    int nIndexes = indexes();
    for (int i = 0; i < nIndexes; ++i) {
        DomItem v = index(i);
        if (!visitor(v))
            return false;
    }
    return true;
}

QSet<QString> DomItem::keys()
{
    return visitEl([this](auto &&el) { return el->keys(*this); });
}

QStringList DomItem::sortedKeys()
{
    QSet<QString> ks = keys();
    QStringList sortedKs(ks.begin(), ks.end());
    std::sort(sortedKs.begin(), sortedKs.end());
    return sortedKs;
}

DomItem DomItem::key(QString name)
{
    return visitEl([this, name](auto &&el) { return el->key(*this, name); });
}

bool DomItem::visitKeys(function_ref<bool(QString, DomItem &)> visitor)
{
    // use iterateDirectSubpathsConst instead?
    for (auto k : sortedKeys()) {
        DomItem v = key(k);
        if (!visitor(k, v))
            return false;
    }
    return true;
}

QList<DomItem> DomItem::values()
{
    QList<DomItem> res;
    visitEl([this, &res](auto &&el) {
        return el->iterateDirectSubpathsConst(
                *this, [&res](const PathEls::PathComponent &, function_ref<DomItem()> item) {
                    res.append(item());
                    return true;
                });
    });
    return res;
}

void DomItem::writeOutPre(OutWriter &ow)
{
    if (hasAnnotations()) {
        DomItem anns = field(Fields::annotations);
        for (auto ann : anns.values()) {
            if (ann.annotations().indexes() == 0) {
                ow.ensureNewline();
                ann.writeOut(ow);
                ow.ensureNewline();
            } else {
                DomItem annAnn = ann.annotations();
                Q_ASSERT_X(annAnn.indexes() == 1 && annAnn.index(0).name() == u"duplicate",
                           "DomItem::writeOutPre", "Unexpected annotation of annotation");
            }
        }
    }
    ow.itemStart(*this);
}

void DomItem::writeOut(OutWriter &ow)
{
    writeOutPre(ow);
    visitEl([this, &ow](auto &&el) { el->writeOut(*this, ow); });
    writeOutPost(ow);
}

void DomItem::writeOutPost(OutWriter &ow)
{
    ow.itemEnd(*this);
}

DomItem DomItem::writeOutForFile(OutWriter &ow, WriteOutChecks extraChecks)
{
    ow.indentNextlines = true;
    writeOut(ow);
    ow.eof();
    DomItem fObj = fileObject();
    DomItem copy = ow.updatedFile(fObj);
    if (extraChecks & WriteOutCheck::All) {
        QStringList dumped;
        auto maybeDump = [&ow, extraChecks, &dumped](DomItem &obj, QStringView objName) {
            QString objDumpPath;
            if (extraChecks & WriteOutCheck::DumpOnFailure) {
                objDumpPath = QDir(QDir::tempPath())
                                      .filePath(objName.toString()
                                                + QFileInfo(ow.lineWriter.fileName()).baseName()
                                                + QLatin1String(".dump.json"));
                obj.dump(objDumpPath);
                dumped.append(objDumpPath);
            }
            return objDumpPath;
        };
        auto dumpedDumper = [&dumped](Sink s) {
            if (dumped.isEmpty())
                return;
            s(u"\ndump: ");
            for (auto dumpPath : dumped) {
                s(u" ");
                sinkEscaped(s, dumpPath);
            }
        };
        auto compare = [&maybeDump, &dumpedDumper, this](DomItem &obj1, QStringView obj1Name,
                                                         DomItem &obj2, QStringView obj2Name,
                                                         const FieldFilter &filter) {
            if (!domCompareStrList(obj1, obj2, filter).isEmpty()) {
                maybeDump(obj1, obj1Name);
                maybeDump(obj2, obj2Name);
                qCWarning(writeOutLog).noquote().nospace()
                        << obj2Name << " writeOut of " << this->canonicalFilePath()
                        << " has changes:\n"
                        << domCompareStrList(obj1, obj2, filter, DomCompareStrList::AllDiffs)
                                   .join(QString())
                        << dumpedDumper;
                return false;
            }
            return true;
        };
        auto checkStability = [&maybeDump, &dumpedDumper, &dumped, &ow,
                               this](QString expected, DomItem &obj, QStringView objName) {
            LineWriter lw2([](QStringView) {}, ow.lineWriter.fileName(), ow.lineWriter.options());
            OutWriter ow2(lw2);
            ow2.indentNextlines = true;
            obj.writeOut(ow2);
            ow2.eof();
            if (ow2.writtenStr != expected) {
                DomItem fObj = this->fileObject();
                maybeDump(fObj, u"initial");
                maybeDump(obj, objName);
                qCWarning(writeOutLog).noquote().nospace()
                        << objName << " non stable writeOut of " << this->canonicalFilePath() << ":"
                        << lineDiff(ow2.writtenStr, expected, 2) << dumpedDumper;
                dumped.clear();
                return false;
            }
            return true;
        };
        if ((extraChecks & WriteOutCheck::UpdatedDomCompare)
            && !compare(fObj, u"initial", copy, u"reformatted", FieldFilter::noLocationFilter()))
            return DomItem();
        if (extraChecks & WriteOutCheck::UpdatedDomStable)
            checkStability(ow.writtenStr, copy, u"reformatted");
        if (extraChecks
            & (WriteOutCheck::Reparse | WriteOutCheck::ReparseCompare
               | WriteOutCheck::ReparseStable)) {
            DomItem newEnv = environment().makeCopy().item();
            if (std::shared_ptr<DomEnvironment> newEnvPtr = newEnv.ownerAs<DomEnvironment>()) {
                auto newFilePtr = std::make_shared<QmlFile>(
                        canonicalFilePath(), ow.writtenStr);
                newEnvPtr->addQmlFile(newFilePtr, AddOption::Overwrite);
                DomItem newFile = newEnv.copy(newFilePtr, Path());
                if (newFilePtr->isValid()) {
                    if (extraChecks
                        & (WriteOutCheck::ReparseCompare | WriteOutCheck::ReparseStable)) {
                        MutableDomItem newFileMutable(newFile);
                        createDom(newFileMutable);
                        if ((extraChecks & WriteOutCheck::ReparseCompare)
                            && !compare(copy, u"reformatted", newFile, u"reparsed",
                                        FieldFilter::compareNoCommentsFilter()))
                            return DomItem();
                        if ((extraChecks & WriteOutCheck::ReparseStable))
                            checkStability(ow.writtenStr, newFile, u"reparsed");
                    }
                } else {
                    qCWarning(writeOutLog).noquote().nospace()
                            << "writeOut of " << canonicalFilePath()
                            << " created invalid code:\n----------\n"
                            << ow.writtenStr << "\n----------" << [&newFile](Sink s) {
                                   newFile.iterateErrors(
                                           [s](DomItem, ErrorMessage msg) {
                                               s(u"\n  ");
                                               msg.dump(s);
                                               return true;
                                           },
                                           true);
                                   s(u"\n"); // extra empty line at the end...
                               };
                    return DomItem();
                }
            }
        }
    }
    return copy;
}

DomItem DomItem::writeOut(QString path, int nBackups, const LineWriterOptions &options,
                          FileWriter *fw, WriteOutChecks extraChecks)
{
    DomItem res = *this;
    DomItem copy;
    FileWriter localFw;
    if (!fw)
        fw = &localFw;
    switch (fw->write(
            path,
            [this, path, &copy, &options, extraChecks](QTextStream &ts) {
                LineWriter lw([&ts](QStringView s) { ts << s; }, path, options);
                OutWriter ow(lw);
                copy = writeOutForFile(ow, extraChecks);
                return bool(copy);
            },
            nBackups)) {
    case FileWriter::Status::ShouldWrite:
    case FileWriter::Status::SkippedDueToFailure:
        qCWarning(writeOutLog) << "failure reformatting " << path;
        break;
    case FileWriter::Status::DidWrite:
    case FileWriter::Status::SkippedEqual:
        res = copy;
        break;
    }
    return res;
}

bool DomItem::isCanonicalChild(DomItem &item)
{
    if (item.isOwningItem()) {
        return canonicalPath() == item.canonicalPath().dropTail();
    } else {
        DomItem itemOw = item.owner();
        DomItem selfOw = owner();
        return itemOw == selfOw && item.pathFromOwner().dropTail() == pathFromOwner();
    }
}

bool DomItem::hasAnnotations()
{
    bool hasAnnotations = false;
    DomType iKind = internalKind();
    switch (iKind) {
    case DomType::Id:
        if (const Id *myPtr = as<Id>())
            hasAnnotations = !myPtr->annotations.isEmpty();
        break;
    case DomType::PropertyDefinition:
        if (const PropertyDefinition *myPtr = as<PropertyDefinition>())
            hasAnnotations = !myPtr->annotations.isEmpty();
        break;
    case DomType::MethodInfo:
        if (const MethodInfo *myPtr = as<MethodInfo>())
            hasAnnotations = !myPtr->annotations.isEmpty();
        break;
    case DomType::QmlObject:
        if (const QmlObject *myPtr = as<QmlObject>())
            hasAnnotations = !myPtr->annotations().isEmpty();
        break;
    case DomType::Binding:
        if (const Binding *myPtr = as<Binding>())
            hasAnnotations = !myPtr->annotations().isEmpty();
        break;
    default:
        break;
    }
    return hasAnnotations;
}

bool DomItem::visitTree(Path basePath, DomItem::ChildrenVisitor visitor, VisitOptions options,
                        DomItem::ChildrenVisitor openingVisitor,
                        DomItem::ChildrenVisitor closingVisitor)
{
    if (!*this)
        return true;
    if (options & VisitOption::VisitSelf && !visitor(basePath, *this, true))
        return false;
    if (!openingVisitor(basePath, *this, true))
        return true;
    auto atEnd = qScopeGuard(
            [closingVisitor, basePath, this]() { closingVisitor(basePath, *this, true); });
    return visitEl([this, basePath, visitor, openingVisitor, closingVisitor, options](auto &&el) {
        return el->iterateDirectSubpathsConst(
                *this,
                [this, basePath, visitor, openingVisitor, closingVisitor,
                 options](const PathEls::PathComponent &c, function_ref<DomItem()> itemF) {
                    Path pNow;
                    if (!(options & VisitOption::NoPath)) {
                        pNow = basePath;
                        pNow = pNow.appendComponent(c);
                    }
                    DomItem item = itemF();
                    bool directChild = isCanonicalChild(item);
                    if (!directChild && !(options & VisitOption::VisitAdopted))
                        return true;
                    if (!directChild || !(options & VisitOption::Recurse)) {
                        if (!visitor(pNow, item, directChild))
                            return false;
                        // give an option to avoid calling open/close when not recursing?
                        // calling it always allows close to do the reverse looping (children before
                        // parent)
                        if (!openingVisitor(pNow, item, directChild))
                            return true;
                        closingVisitor(pNow, item, directChild);
                    } else {
                        return item.visitTree(pNow, visitor, options | VisitOption::VisitSelf,
                                              openingVisitor, closingVisitor);
                    }
                    return true;
                });
    });
}

bool DomItem::visitPrototypeChain(function_ref<bool(DomItem &)> visitor,
                                  VisitPrototypesOptions options, ErrorHandler h,
                                  QSet<quintptr> *visited, QList<Path> *visitedRefs)
{
    QSet<quintptr> visitedLocal;
    if (!visited)
        visited = &visitedLocal;
    QList<Path> refsLocal;
    if (!visitedRefs)
        visitedRefs = &refsLocal;
    bool shouldVisit = !(options & VisitPrototypesOption::SkipFirst);
    DomItem current = qmlObject();
    if (!current) {
        myErrors().warning(tr("Prototype chain called outside object")).withItem(*this).handle(h);
        return true;
    }
    QList<DomItem> toDo({ current });
    while (!toDo.isEmpty()) {
        current = toDo.takeLast();
        current = current.proceedToScope(h, visitedRefs);
        if (visited->contains(current.id())) {
            // to warn about circular dependencies a purely local visited trace is required
            // as common ancestors of unrelated objects are valid and should be skipped
            // so we do not to warn unless requested
            if (options & VisitPrototypesOption::RevisitWarn)
                myErrors()
                        .warning(tr("Detected multiple visit of %1 visiting prototypes of %2")
                                         .arg(current.canonicalPath().toString(),
                                              canonicalPath().toString()))
                        .withItem(*this)
                        .handle(h);
            continue;
        }
        visited->insert(current.id());
        if (shouldVisit && !visitor(current))
            return false;
        shouldVisit = true;
        current.field(Fields::prototypes)
                .visitIndexes([&toDo, &current, this, &h, visitedRefs, options](DomItem &el) {
                    Path elId = el.canonicalPath();
                    if (visitedRefs->contains(elId))
                        return true;
                    else
                        visitedRefs->append(elId);
                    QList<DomItem> protos = el.getAll(h, visitedRefs);
                    if (protos.isEmpty()) {
                        if (std::shared_ptr<DomEnvironment> envPtr =
                                    environment().ownerAs<DomEnvironment>())
                            if (!(envPtr->options() & DomEnvironment::Option::NoDependencies))
                                myErrors()
                                        .warning(tr("could not resolve prototype %1 (%2)")
                                                         .arg(current.canonicalPath().toString(),
                                                              el.field(Fields::referredObjectPath)
                                                                      .value()
                                                                      .toString()))
                                        .withItem(*this)
                                        .handle(h);
                    } else {
                        if (protos.size() > 1) {
                            QStringList protoPaths;
                            for (DomItem &p : protos)
                                protoPaths.append(p.canonicalPath().toString());
                            myErrors()
                                    .warning(tr("Multiple definitions found, using first only, "
                                                "resolving prototype %1 (%2): %3")
                                                     .arg(current.canonicalPath().toString(),
                                                          el.field(Fields::referredObjectPath)
                                                                  .value()
                                                                  .toString(),
                                                          protoPaths.join(QLatin1String(", "))))
                                    .withItem(*this)
                                    .handle(h);
                        }
                        int nProtos = 1; // change to protos.length() to us all prototypes found
                                         // (sloppier)
                        for (int i = nProtos; i != 0;) {
                            DomItem proto = protos.at(--i);
                            if (proto.internalKind() == DomType::Export) {
                                if (!(options & VisitPrototypesOption::ManualProceedToScope))
                                    proto = proto.proceedToScope(h, visitedRefs);
                                toDo.append(proto);
                            } else if (proto.internalKind() == DomType::QmlObject) {
                                toDo.append(proto);
                            } else {
                                myErrors()
                                        .warning(tr("Unexpected prototype type %1 (%2)")
                                                         .arg(current.canonicalPath().toString(),
                                                              el.field(Fields::referredObjectPath)
                                                                      .value()
                                                                      .toString()))
                                        .withItem(*this)
                                        .handle(h);
                            }
                        }
                    }
                    return true;
                });
    }
    return true;
}

bool DomItem::visitDirectAccessibleScopes(function_ref<bool(DomItem &)> visitor,
                                          VisitPrototypesOptions options, ErrorHandler h,
                                          QSet<quintptr> *visited, QList<Path> *visitedRefs)
{
    // these are the scopes one can access with the . operator from the current location
    // but currently not the attached types, which we should
    DomType k = internalKind();
    if (k == DomType::QmlObject)
        return visitPrototypeChain(visitor, options, h, visited, visitedRefs);
    if (visited && id() != 0) {
        if (visited->contains(id()))
            return true;
        visited->insert(id());
    }
    if (k == DomType::Id || k == DomType::Reference || k == DomType::Export) {
        // we go to the scope if it is clearly defined
        DomItem v = proceedToScope(h, visitedRefs);
        if (v.internalKind() == DomType::QmlObject)
            return v.visitPrototypeChain(visitor, options, h, visited, visitedRefs);
    }
    if (k == DomType::Binding) {
        // from a binding we can get to its value if it is a object
        DomItem v = field(Fields::value);
        if (v.internalKind() == DomType::QmlObject)
            return v.visitPrototypeChain(visitor, options, h, visited, visitedRefs);
    }
    if (k == DomType::PropertyDefinition) {
        // from a property definition we go to the type stored in it
        DomItem t = field(Fields::type).proceedToScope(h, visitedRefs);
        if (t.internalKind() == DomType::QmlObject)
            return t.visitPrototypeChain(visitor, options, h, visited, visitedRefs);
    }
    if (!(options & VisitPrototypesOption::SkipFirst) && isScope() && !visitor(*this))
        return false;
    return true;
}

/*!
 * \brief DomItem::visitStaticTypePrototypeChains
 * \param visitor
 * \param visitFirst
 * \param visited
 * \return
 *
 * visit the values JS reaches accessing a type directly: the values if it is a singleton or the
 * attached type
 */
bool DomItem::visitStaticTypePrototypeChains(function_ref<bool(DomItem &)> visitor,
                                             VisitPrototypesOptions options, ErrorHandler h,
                                             QSet<quintptr> *visited, QList<Path> *visitedRefs)
{
    QSet<quintptr> visitedLocal;
    if (!visited)
        visited = &visitedLocal;
    DomItem current = qmlObject();
    DomItem comp = current.component();
    if (comp.field(Fields::isSingleton).value().toBool(false)
        && !current.visitPrototypeChain(visitor, options, h, visited, visitedRefs))
        return false;
    if (DomItem attachedT = current.component().field(Fields::attachedType).field(Fields::get))
        if (!attachedT.visitPrototypeChain(
                    visitor, options & ~VisitPrototypesOptions(VisitPrototypesOption::SkipFirst), h,
                    visited, visitedRefs))
            return false;
    return true;
}

bool DomItem::visitScopeChain(function_ref<bool(DomItem &)> visitor, LookupOptions options,
                              ErrorHandler h, QSet<quintptr> *visited, QList<Path> *visitedRefs)
{
    QSet<quintptr> visitedLocal;
    if (!visited)
        visited = &visitedLocal;
    QList<Path> visitedRefsLocal;
    if (!visitedRefs)
        visitedRefs = &visitedRefsLocal;
    DomItem current = scope();
    if (!current) {
        myResolveErrors().warning(tr("Called visitScopeChain outside scopes")).handle(h);
        return true;
    }
    QList<DomItem> toDo { current };
    bool visitFirst = !(options & LookupOption::SkipFirstScope);
    bool visitCurrent = visitFirst;
    bool first = true;
    while (!toDo.isEmpty()) {
        DomItem current = toDo.takeLast();
        if (visited->contains(current.id()))
            continue;
        visited->insert(current.id());
        if (visitCurrent && !visitor(current))
            return false;
        visitCurrent = true;
        switch (current.internalKind()) {
        case DomType::QmlObject: {
            if (!current.visitPrototypeChain(visitor, VisitPrototypesOption::SkipFirst, h, visited,
                                             visitedRefs))
                return false;
            DomItem root = current.rootQmlObject();
            if (root && root != current) {
                first = false;
                toDo.append(root);
            } else if (DomItem next = current.scope(
                               FilterUpOptions::ReturnOuterNoSelf)) { //  should be the component
                toDo.append(next);
            }
        } break;
        case DomType::ScriptExpression: // Js lexical scope
            first = false;
            if (DomItem next = current.scope(FilterUpOptions::ReturnOuterNoSelf))
                toDo.append(next);
            break;
        case DomType::QmlComponent: // ids/attached type
            if ((options & LookupOption::Strict) == 0) {
                if (DomItem comp = current.field(Fields::nextComponent))
                    toDo.append(comp);
            }
            if (first && visitFirst && (options & LookupOption::VisitTopClassType)
                && *this == current) { // visit attached type if it is the top of the chain
                if (DomItem attachedT = current.field(Fields::attachedType).field(Fields::get))
                    toDo.append(attachedT);
            }
            if (DomItem next = current.scope(FilterUpOptions::ReturnOuterNoSelf))
                toDo.append(next);
            first = false;
            break;
        case DomType::QmlFile: // subComponents, imported types
            if (DomItem iScope =
                        current.field(Fields::importScope)) // treat file as a separate scope?
                toDo.append(iScope);
            first = false;
            break;
        case DomType::MethodInfo: // method arguments
            first = false;
            if (DomItem next = current.scope(FilterUpOptions::ReturnOuterNoSelf))
                toDo.append(next);
            break;
        case DomType::ImportScope: // types
            first = false;
            if (auto globalC = globalScope().field(Fields::rootComponent))
                toDo.append(globalC);
            break;
        case DomType::JsResource:
        case DomType::GlobalComponent:
            first = false;
            if (DomItem next = current.field(Fields::objects).index(0))
                toDo.append(next);
            break;
        case DomType::QmltypesComponent:
            first = false;
            break;
        default:
            first = false;
            myResolveErrors()
                    .error(tr("Unexpected non scope object %1 (%2) reached in visitScopeChain")
                                   .arg(domTypeToString(current.internalKind()),
                                        current.canonicalPath().toString()))
                    .handle(h);
            Q_ASSERT(false);
            break;
        }
    }
    return true;
}

QSet<QString> DomItem::localSymbolNames(LocalSymbolsTypes typeFilter)
{
    QSet<QString> res;
    if (typeFilter == LocalSymbolsType::None)
        return res;
    switch (internalKind()) {
    case DomType::QmlObject:
        if (typeFilter & LocalSymbolsType::Attributes) {
            res += propertyDefs().keys();
            res += bindings().keys();
        }
        if (typeFilter & LocalSymbolsType::Methods) {
            if ((typeFilter & LocalSymbolsType::Methods) == LocalSymbolsType::Methods) {
                res += methods().keys();
            } else {
                bool shouldAddSignals = bool(typeFilter & LocalSymbolsType::Signals);
                if (const QmlObject *objPtr = as<QmlObject>()) {
                    auto methods = objPtr->methods();
                    for (auto it = methods.cbegin(); it != methods.cend(); ++it) {
                        if (bool(it.value().methodType == MethodInfo::MethodType::Signal)
                            == shouldAddSignals)
                            res += it.key();
                    }
                }
            }
        }
        break;
    case DomType::ScriptExpression:
        // to do
        break;
    case DomType::QmlComponent:
        if (typeFilter & LocalSymbolsType::Ids)
            res += ids().keys();
        break;
    case DomType::QmlFile: // subComponents, imported types
        if (typeFilter & LocalSymbolsType::Components) {
            DomItem comps = field(Fields::components);
            for (auto k : comps.keys())
                if (!k.isEmpty())
                    res.insert(k);
        }
        break;
    case DomType::ImportScope: {
        const ImportScope *currentPtr = as<ImportScope>();
        if (typeFilter & LocalSymbolsType::Types) {
            if ((typeFilter & LocalSymbolsType::Types) == LocalSymbolsType::Types) {
                res += currentPtr->importedNames(*this);
            } else {
                bool qmlTypes = bool(typeFilter & LocalSymbolsType::QmlTypes);
                for (const QString &typeName : currentPtr->importedNames(*this)) {
                    if ((!typeName.isEmpty() && typeName.at(0).isUpper()) == qmlTypes)
                        res += typeName;
                }
            }
        }
        if (typeFilter & LocalSymbolsType::Namespaces) {
            for (const auto &k : currentPtr->subImports().keys())
                res.insert(k);
        }
        break;
    }
    case DomType::QmltypesComponent:
    case DomType::JsResource:
    case DomType::GlobalComponent:
        if (typeFilter & LocalSymbolsType::Globals)
            res += enumerations().keys();
        break;
    case DomType::MethodInfo: {
        if (typeFilter & LocalSymbolsType::MethodParameters) {
            DomItem params = field(Fields::parameters);
            params.visitIndexes([&res](DomItem &p) {
                const MethodParameter *pPtr = p.as<MethodParameter>();
                res.insert(pPtr->name);
                return true;
            });
        }
        break;
    }
    default:
        break;
    }
    return res;
}

bool DomItem::visitLookup1(QString symbolName, function_ref<bool(DomItem &)> visitor,
                           LookupOptions opts, ErrorHandler h, QSet<quintptr> *visited,
                           QList<Path> *visitedRefs)
{
    bool typeLookupInQmlFile = symbolName.size() > 1 && symbolName.at(0).isUpper()
            && fileObject().internalKind() == DomType::QmlFile;
    if (typeLookupInQmlFile) {
        // shortcut to lookup types (scope chain would find them too, but after looking
        // the prototype chain)
        DomItem importScope = fileObject().field(Fields::importScope);
        if (const ImportScope *importScopePtr = importScope.as<ImportScope>()) {
            if (importScopePtr->subImports().contains(symbolName)) {
                DomItem subItem = importScope.field(Fields::qualifiedImports).key(symbolName);
                if (!visitor(subItem))
                    return false;
            }
            QList<DomItem> types = importScopePtr->importedItemsWithName(importScope, symbolName);
            for (DomItem &t : types) {
                if (!visitor(t))
                    return false;
            }
        }
        return true;
    }
    return visitScopeChain(
            [symbolName, visitor](DomItem &obj) {
                return obj.visitLocalSymbolsNamed(symbolName,
                                                  [visitor](DomItem &el) { return visitor(el); });
            },
            opts, h, visited, visitedRefs);
}

class CppTypeInfo
{
    Q_DECLARE_TR_FUNCTIONS(CppTypeInfo)
public:
    CppTypeInfo() = default;

    static CppTypeInfo fromString(QStringView target, ErrorHandler h = nullptr)
    {
        CppTypeInfo res;
        QRegularExpression reTarget = QRegularExpression(QRegularExpression::anchoredPattern(
                uR"(QList<(?<list>[a-zA-Z_0-9:]+) *(?<listPtr>\*?)>|QMap< *(?<mapKey>[a-zA-Z_0-9:]+) *, *(?<mapValue>[a-zA-Z_0-9:]+) *(?<mapPtr>\*?)>|(?<baseType>[a-zA-Z_0-9:]+) *(?<ptr>\*?))"));

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
        QRegularExpressionMatch m = reTarget.match(target);
#else
        QRegularExpressionMatch m = reTarget.matchView(target);
#endif
        if (!m.hasMatch()) {
            DomItem::myResolveErrors()
                    .error(tr("Unexpected complex CppType %1").arg(target))
                    .handle(h);
        }
        res.baseType = m.captured(u"baseType");
        res.isPointer = !m.captured(u"ptr").isEmpty();
        if (!m.captured(u"list").isEmpty()) {
            res.isList = true;
            res.baseType = m.captured(u"list");
            res.isPointer = !m.captured(u"listPtr").isEmpty();
        }
        if (!m.captured(u"mapValue").isEmpty()) {
            res.isMap = true;
            if (m.captured(u"mapKey") != u"QString") {
                DomItem::myResolveErrors()
                        .error(tr("Unexpected complex CppType %1 (map with non QString key)")
                                       .arg(target))
                        .handle(h);
            }
            res.baseType = m.captured(u"mapValue");
            res.isPointer = !m.captured(u"mapPtr").isEmpty();
        }
        return res;
    }

    QString baseType;
    bool isPointer = false;
    bool isMap = false;
    bool isList = false;
};

bool DomItem::visitLookup(QString target, function_ref<bool(DomItem &)> visitor,
                          LookupType lookupType, LookupOptions opts, ErrorHandler errorHandler,
                          QSet<quintptr> *visited, QList<Path> *visitedRefs)
{
    if (target.isEmpty())
        return true;
    switch (lookupType) {
    case LookupType::Binding:
    case LookupType::Method:
    case LookupType::Property:
    case LookupType::PropertyDef:
    case LookupType::Symbol:
    case LookupType::Type: {
        QStringList subpath = target.split(QChar::fromLatin1('.'));
        if (subpath.size() == 1) {
            return visitLookup1(subpath.first(), visitor, opts, errorHandler, visited, visitedRefs);
        } else {
            return visitLookup1(
                    subpath.at(0),
                    [&subpath, visitor, lookupType, &errorHandler, visitedRefs](DomItem &newIt) {
                        QVector<ResolveToDo> lookupToDos({ ResolveToDo {
                                newIt, 1 } }); // invariant: always increase pathIndex to guarantee
                                               // end even with only partial visited match
                        QList<QSet<quintptr>> lookupVisited(subpath.size() + 1);
                        while (!lookupToDos.isEmpty()) {
                            ResolveToDo tNow = lookupToDos.takeFirst();
                            auto vNow = qMakePair(tNow.item.id(), tNow.pathIndex);
                            DomItem subNow = tNow.item;
                            int iSubPath = tNow.pathIndex;
                            Q_ASSERT(iSubPath < subpath.size());
                            QString subPathNow = subpath[iSubPath++];
                            DomItem scope = subNow.proceedToScope();
                            if (iSubPath < subpath.size()) {
                                if (vNow.first != 0) {
                                    if (lookupVisited[vNow.second].contains(vNow.first))
                                        continue;
                                    else
                                        lookupVisited[vNow.second].insert(vNow.first);
                                }
                                if (scope.internalKind() == DomType::QmlObject)
                                    scope.visitDirectAccessibleScopes(
                                            [&lookupToDos, subPathNow, iSubPath](DomItem &el) {
                                                return el.visitLocalSymbolsNamed(
                                                        subPathNow,
                                                        [&lookupToDos, iSubPath](DomItem &subEl) {
                                                            lookupToDos.append({ subEl, iSubPath });
                                                            return true;
                                                        });
                                            },
                                            VisitPrototypesOption::Normal, errorHandler,
                                            &(lookupVisited[vNow.second]), visitedRefs);
                            } else {
                                bool cont = scope.visitDirectAccessibleScopes(
                                        [&visitor, subPathNow, lookupType](DomItem &el) -> bool {
                                            if (lookupType == LookupType::Symbol)
                                                return el.visitLocalSymbolsNamed(subPathNow,
                                                                                 visitor);
                                            else
                                                return el.visitLocalSymbolsNamed(
                                                        subPathNow,
                                                        [lookupType,
                                                         &visitor](DomItem &el) -> bool {
                                                            bool correctType = false;
                                                            DomType iType = el.internalKind();
                                                            switch (lookupType) {
                                                            case LookupType::Binding:
                                                                correctType =
                                                                        (iType == DomType::Binding);
                                                                break;
                                                            case LookupType::Method:
                                                                correctType =
                                                                        (iType
                                                                         == DomType::MethodInfo);
                                                                break;
                                                            case LookupType::Property:
                                                                correctType =
                                                                        (iType
                                                                                 == DomType::
                                                                                         PropertyDefinition
                                                                         || iType
                                                                                 == DomType::
                                                                                         Binding);
                                                                break;
                                                            case LookupType::PropertyDef:
                                                                correctType =
                                                                        (iType
                                                                         == DomType::
                                                                                 PropertyDefinition);
                                                                break;
                                                            case LookupType::Type:
                                                                correctType =
                                                                        (iType
                                                                         == DomType::
                                                                                 Export); // accept
                                                                                          // direct
                                                                                          // QmlObject
                                                                                          // ref?
                                                                break;
                                                            default:
                                                                Q_ASSERT(false);
                                                                break;
                                                            }
                                                            if (correctType)
                                                                return visitor(el);
                                                            return true;
                                                        });
                                        },
                                        VisitPrototypesOption::Normal, errorHandler,
                                        &(lookupVisited[vNow.second]), visitedRefs);
                                if (!cont)
                                    return false;
                            }
                        }
                        return true;
                    },
                    opts, errorHandler, visited, visitedRefs);
        }
        break;
    }
    case LookupType::CppType: {
        QString baseTarget = CppTypeInfo::fromString(target, errorHandler).baseType;
        DomItem localQmltypes = owner();
        while (localQmltypes && localQmltypes.internalKind() != DomType::QmltypesFile) {
            localQmltypes = localQmltypes.containingObject();
            localQmltypes = localQmltypes.owner();
        }
        if (localQmltypes) {
            if (DomItem localTypes = localQmltypes.field(Fields::components).key(baseTarget)) {
                bool cont = localTypes.visitIndexes([&visitor](DomItem &els) {
                    return els.visitIndexes([&visitor](DomItem &el) {
                        if (DomItem obj = el.field(Fields::objects).index(0))
                            return visitor(obj);
                        return true;
                    });
                });
                if (!cont)
                    return false;
            }
        }
        DomItem qmltypes = environment().field(Fields::qmltypesFileWithPath);
        return qmltypes.visitKeys([baseTarget, &visitor](QString, DomItem &els) {
            DomItem comps =
                    els.field(Fields::currentItem).field(Fields::components).key(baseTarget);
            return comps.visitIndexes([&visitor](DomItem &el) {
                if (DomItem obj = el.field(Fields::objects).index(0))
                    return visitor(obj);
                return true;
            });
        });
        break;
    }
    }
    Q_ASSERT(false);
    return true;
}

DomItem DomItem::proceedToScope(ErrorHandler h, QList<Path> *visitedRefs)
{
    // follow references, resolve exports
    DomItem current = *this;
    while (current) {
        switch (current.internalKind()) {
        case DomType::Reference: {
            Path currentPath = current.canonicalPath();
            current = current.get(h, visitedRefs);
            break;
        }
        case DomType::Export:
            current = current.field(Fields::type);
            break;
        case DomType::Id:
            current = current.field(Fields::referredObject);
            break;
        default:
            return current.scope();
            break;
        }
    }
    return DomItem();
}

QList<DomItem> DomItem::lookup(QString symbolName, LookupType type, LookupOptions opts,
                               ErrorHandler errorHandler)
{
    QList<DomItem> res;
    visitLookup(
            symbolName,
            [&res](DomItem &el) {
                res.append(el);
                return true;
            },
            type, opts, errorHandler);
    return res;
}

DomItem DomItem::lookupFirst(QString symbolName, LookupType type, LookupOptions opts,
                             ErrorHandler errorHandler)
{
    DomItem res;
    visitLookup(
            symbolName,
            [&res](DomItem &el) {
                res = el;
                return false;
            },
            type, opts, errorHandler);
    return res;
}

quintptr DomItem::id()
{
    return visitEl([](auto &&b) { return b->id(); });
}

Path DomItem::pathFromOwner()
{
    return visitEl([this](auto &&e) { return e->pathFromOwner(*this); });
}

QString DomItem::canonicalFilePath()
{
    return visitEl([this](auto &&e) { return e->canonicalFilePath(*this); });
}

DomItem DomItem::fileLocationsTree()
{
    if (DomItem l = field(Fields::fileLocationsTree))
        return l;
    auto res = FileLocations::findAttachedInfo(*this, AttachedInfo::FindOption::SetFoundTreePath);
    if (res && res.foundTreePath.value()) {
        return copy(res.foundTree, res.foundTreePath.value());
    }
    return DomItem();
}

DomItem DomItem::fileLocations()
{
    return fileLocationsTree().field(Fields::infoItem);
}

MutableDomItem DomItem::makeCopy(DomItem::CopyOption option)
{
    if (m_kind == DomType::Empty)
        return MutableDomItem();
    DomItem o = owner();
    if (option == CopyOption::EnvDisconnected) {
        DomItem newItem = std::visit(
                [this, &o](auto &&el) {
                    auto copyPtr = el->makeCopy(o);
                    return DomItem(m_top, copyPtr, m_ownerPath, copyPtr.get());
                },
                *m_owner);
        return MutableDomItem(newItem.path(pathFromOwner()));
    }
    DomItem env = environment();
    std::shared_ptr<DomEnvironment> newEnvPtr;
    if (std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>()) {
        newEnvPtr = std::make_shared<DomEnvironment>(
                envPtr, envPtr->loadPaths(), envPtr->options());
        DomBase *eBase = envPtr.get();
        if (std::holds_alternative<DomEnvironment *>(m_element) && eBase
            && std::get<DomEnvironment *>(m_element) == eBase)
            return MutableDomItem(DomItem(newEnvPtr));
    } else if (std::shared_ptr<DomUniverse> univPtr = top().ownerAs<DomUniverse>()) {
        newEnvPtr = std::make_shared<DomEnvironment>(
                QStringList(),
                DomEnvironment::Option::SingleThreaded | DomEnvironment::Option::NoDependencies,
                univPtr);
    } else {
        Q_ASSERT(false);
        return {};
    }
    DomItem newItem = std::visit(
            [this, newEnvPtr, &o](auto &&el) {
                auto copyPtr = el->makeCopy(o);
                return DomItem(newEnvPtr, copyPtr, m_ownerPath, copyPtr.get());
            },
            *m_owner);

    switch (o.internalKind()) {
    case DomType::QmlDirectory:
        newEnvPtr->addQmlDirectory(newItem.ownerAs<QmlDirectory>(), AddOption::Overwrite);
        break;
    case DomType::JsFile:
        newEnvPtr->addJsFile(newItem.ownerAs<JsFile>(), AddOption::Overwrite);
        break;
    case DomType::QmlFile:
        newEnvPtr->addQmlFile(newItem.ownerAs<QmlFile>(), AddOption::Overwrite);
        break;
    case DomType::QmltypesFile:
        newEnvPtr->addQmltypesFile(newItem.ownerAs<QmltypesFile>(), AddOption::Overwrite);
        break;
    case DomType::GlobalScope: {
        newEnvPtr->addGlobalScope(newItem.ownerAs<GlobalScope>(), AddOption::Overwrite);
    } break;
    case DomType::ModuleIndex:
    case DomType::MockOwner:
    case DomType::ScriptExpression:
    case DomType::AstComments:
    case DomType::LoadInfo:
    case DomType::AttachedInfo:
    case DomType::DomEnvironment:
    case DomType::DomUniverse:
        qCWarning(domLog) << "DomItem::makeCopy " << internalKindStr()
                          << " does not support binding to environment";
        Q_ASSERT(false);
        return MutableDomItem();
    default:
        qCWarning(domLog) << "DomItem::makeCopy(" << internalKindStr()
                          << ") is not an known OwningItem";
        Q_ASSERT(o.isOwningItem());
        return MutableDomItem();
    }
    DomItem newEnv(newEnvPtr);
    Q_ASSERT(newEnv.path(o.canonicalPath()).m_owner == newItem.m_owner);
    return MutableDomItem(newItem.path(pathFromOwner()));
}

bool DomItem::commitToBase(std::shared_ptr<DomEnvironment> validEnvPtr)
{
    DomItem env = environment();
    if (std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>()) {
        return envPtr->commitToBase(env, validEnvPtr);
    }
    return false;
}

bool DomItem::visitLocalSymbolsNamed(QString name, function_ref<bool(DomItem &)> visitor)
{
    if (name.isEmpty()) // no empty symbol
        return true;
    // we could avoid discriminating by type and just access all the needed stuff in the correct
    // sequence, making sure it is empty if not provided, but for now I find it clearer to check the
    // type
    DomItem f;
    DomItem v;
    switch (internalKind()) {
    case DomType::QmlObject:
        f = field(Fields::propertyDefs);
        v = f.key(name);
        if (!v.visitIndexes(visitor))
            return false;
        f = field(Fields::bindings);
        v = f.key(name);
        if (!v.visitIndexes(visitor))
            return false;
        f = field(Fields::methods);
        v = f.key(name);
        if (!v.visitIndexes(visitor))
            return false;
        break;
    case DomType::ScriptExpression:
        // to do
        break;
    case DomType::QmlComponent:
        f = field(Fields::ids);
        v = f.key(name);
        if (!v.visitIndexes(visitor))
            return false;
        Q_FALLTHROUGH();
    case DomType::JsResource:
    case DomType::GlobalComponent:
    case DomType::QmltypesComponent:
        f = field(Fields::enumerations);
        v = f.key(name);
        if (!v.visitIndexes(visitor))
            return false;
        break;
    case DomType::MethodInfo: {
        DomItem params = field(Fields::parameters);
        if (!params.visitIndexes([name, visitor](DomItem &p) {
                const MethodParameter *pPtr = p.as<MethodParameter>();
                if (pPtr->name == name && !visitor(p))
                    return false;
                return true;
            }))
            return false;
        break;
    }
    case DomType::QmlFile: {
        f = field(Fields::components);
        v = f.key(name);
        if (!v.visitIndexes(visitor))
            return false;
        break;
    }
    case DomType::ImportScope: {
        f = field(Fields::imported);
        v = f.key(name);
        if (!v.visitIndexes(visitor))
            return false;
        f = field(Fields::qualifiedImports);
        v = f.key(name);
        if (v && !visitor(v))
            return false;
        break;
    default:
        Q_ASSERT(!isScope());
        break;
    }
    }
    return true;
}

DomItem DomItem::operator[](const QString &cName)
{
    if (internalKind() == DomType::Map)
        return key(cName);
    return field(cName);
}

DomItem DomItem::operator[](QStringView cName)
{
    if (internalKind() == DomType::Map)
        return key(cName.toString());
    return field(cName);
}

DomItem DomItem::operator[](Path p)
{
    return path(p);
}

QCborValue DomItem::value()
{
    if (internalKind() == DomType::ConstantData)
        return std::get<ConstantData>(m_element).value();
    return QCborValue();
}

void DomItem::dumpPtr(Sink sink)
{
    sink(u"DomItem{ topPtr:");
    sink(QString::number((quintptr)topPtr().get(), 16));
    sink(u", ownerPtr:");
    sink(QString::number((quintptr)owningItemPtr().get(), 16));
    sink(u", ownerPath:");
    m_ownerPath.dump(sink);
    sink(u", elPtr:");
    sink(QString::number((quintptr)base(),16));
    sink(u"}");
}

void DomItem::dump(Sink s, int indent,
                   function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)> filter)
{
    visitEl([this, s, indent, filter](auto &&e) { e->dump(*this, s, indent, filter); });
}

FileWriter::Status
DomItem::dump(QString path,
              function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)> filter,
              int nBackups, int indent, FileWriter *fw)
{
    FileWriter localFw;
    if (!fw)
        fw = &localFw;
    switch (fw->write(
            path,
            [this, indent, filter](QTextStream &ts) {
                this->dump([&ts](QStringView s) { ts << s; }, indent, filter);
                return true;
            },
            nBackups)) {
    case FileWriter::Status::ShouldWrite:
    case FileWriter::Status::SkippedDueToFailure:
        qWarning() << "Failure dumping " << canonicalPath() << " to " << path;
        break;
    case FileWriter::Status::DidWrite:
    case FileWriter::Status::SkippedEqual:
        break;
    }
    return fw->status;
}

QString DomItem::toString()
{
    return dumperToString([this](Sink s){ dump(s); });
}

int DomItem::derivedFrom()
{
    if (m_owner)
        return std::visit([](auto &&ow) { return ow->derivedFrom(); }, *m_owner);
    return 0;
}

int DomItem::revision()
{
    if (m_owner)
        return std::visit([](auto &&ow) { return ow->revision(); }, *m_owner);
    else
        return -1;
}

QDateTime DomItem::createdAt()
{
    if (m_owner)
        return std::visit([](auto &&ow) { return ow->createdAt(); }, *m_owner);
    else
        return QDateTime::fromMSecsSinceEpoch(0, UTC);
}

QDateTime DomItem::frozenAt()
{
    if (m_owner)
        return std::visit([](auto &&ow) { return ow->frozenAt(); }, *m_owner);
    else
        return QDateTime::fromMSecsSinceEpoch(0, UTC);
}

QDateTime DomItem::lastDataUpdateAt()
{
    if (m_owner)
        return std::visit([](auto &&ow) { return ow->lastDataUpdateAt(); }, *m_owner);
    else
        return QDateTime::fromMSecsSinceEpoch(0, UTC);
}

void DomItem::addError(ErrorMessage msg)
{
    if (m_owner) {
        DomItem myOwner = owner();
        std::visit(
                [this, &myOwner, &msg](auto &&ow) { ow->addError(myOwner, msg.withItem(*this)); },
                *m_owner);
    } else
        defaultErrorHandler(msg.withItem(*this));
}

ErrorHandler DomItem::errorHandler()
{
    DomItem self = *this;
    return [self](ErrorMessage m) mutable { self.addError(m); };
}

void DomItem::clearErrors(ErrorGroups groups, bool iterate)
{
    if (m_owner) {
        std::visit([&groups](auto &&ow) { ow->clearErrors(groups); }, *m_owner);
        if (iterate)
            iterateSubOwners([groups](DomItem i){
                i.clearErrors(groups, true);
                return true;
            });
    }
}

bool DomItem::iterateErrors(function_ref<bool(DomItem, ErrorMessage)> visitor, bool iterate,
                            Path inPath)
{
    if (m_owner) {
        DomItem ow = owner();
        if (!std::visit([&ow, visitor,
                         inPath](auto &&el) { return el->iterateErrors(ow, visitor, inPath); },
                        *m_owner))
            return false;
        if (iterate && !iterateSubOwners([inPath, visitor](DomItem &i) {
                return i.iterateErrors(visitor, true, inPath);
            }))
            return false;
    }
    return true;
}

bool DomItem::iterateSubOwners(function_ref<bool(DomItem &)> visitor)
{
    if (m_owner) {
        DomItem ow = owner();
        return std::visit([&ow, visitor](auto &&o) { return o->iterateSubOwners(ow, visitor); },
                          *m_owner);
    }
    return true;
}

bool DomItem::iterateDirectSubpaths(DirectVisitor v)
{
    return visitMutableEl(
            [this, v](auto &&el) mutable { return el->iterateDirectSubpaths(*this, v); });
}

DomItem DomItem::subReferencesItem(const PathEls::PathComponent &c, QList<Path> paths)
{
    return subListItem(
                List::fromQList<Path>(pathFromOwner().appendComponent(c), paths,
                                      [](DomItem &list, const PathEls::PathComponent &p, Path &el) {
                    return list.subReferenceItem(p, el);
                }));
}

DomItem DomItem::subReferenceItem(const PathEls::PathComponent &c, Path referencedObject)
{
    if (domTypeIsOwningItem(internalKind())) {
        return DomItem(m_top, m_owner, m_ownerPath, Reference(referencedObject, Path(c)));
    } else {
        return DomItem(m_top, m_owner, m_ownerPath,
                       Reference(referencedObject, pathFromOwner().appendComponent(c)));
    }
}

shared_ptr<DomTop> DomItem::topPtr()
{
    if (m_top)
        return std::visit([](auto &&el) -> shared_ptr<DomTop> { return el; }, *m_top);
    return {};
}

shared_ptr<OwningItem> DomItem::owningItemPtr()
{
    if (m_owner)
        return std::visit([](auto &&el) -> shared_ptr<OwningItem> { return el; }, *m_owner);
    return {};
}

const DomBase *DomItem::base()
{
    return visitEl([](auto &&el) { return static_cast<const DomBase *>(&(*el)); });
}

DomBase *DomItem::mutableBase()
{
    return visitMutableEl([](auto &&el) { return static_cast<DomBase *>(&(*el)); });
}

DomItem::DomItem(std::shared_ptr<DomEnvironment> envPtr):
    DomItem(envPtr, envPtr, Path(), envPtr.get())
{
}

DomItem::DomItem(std::shared_ptr<DomUniverse> universePtr):
    DomItem(universePtr, universePtr, Path(), universePtr.get())
{
}

void DomItem::loadFile(QString canonicalFilePath, QString logicalPath, QString code,
                       QDateTime codeDate, DomTop::Callback callback, LoadOptions loadOptions,
                       std::optional<DomType> fileType)
{
    DomItem topEl = top();
    if (topEl.internalKind() == DomType::DomEnvironment
        || topEl.internalKind() == DomType::DomUniverse) {
        if (auto univ = topEl.ownerAs<DomUniverse>())
            univ->loadFile(*this, canonicalFilePath, logicalPath, code, codeDate, callback,
                           loadOptions, fileType);
        else if (auto env = topEl.ownerAs<DomEnvironment>()) {
            if (env->options() & DomEnvironment::Option::NoDependencies)
                env->loadFile(topEl, canonicalFilePath, logicalPath, code, codeDate, callback,
                              DomTop::Callback(), DomTop::Callback(), loadOptions, fileType);
            else
                env->loadFile(topEl, canonicalFilePath, logicalPath, code, codeDate,
                              DomTop::Callback(), DomTop::Callback(), callback, loadOptions,
                              fileType);
        } else
            Q_ASSERT(false && "expected either DomUniverse or DomEnvironment cast to succeed");
    } else {
        addError(myErrors().warning(tr("loadFile called without DomEnvironment or DomUniverse.")));
        callback(Paths::qmlFileInfoPath(canonicalFilePath), DomItem::empty, DomItem::empty);
    }
}

void DomItem::loadFile(QString filePath, QString logicalPath, DomTop::Callback callback,
                       LoadOptions loadOptions, std::optional<DomType> fileType)
{
    DomItem topEl = top();
    if (topEl.internalKind() == DomType::DomEnvironment
        || topEl.internalKind() == DomType::DomUniverse) {
        if (auto univ = topEl.ownerAs<DomUniverse>())
            univ->loadFile(*this, filePath, logicalPath, callback, loadOptions);
        else if (auto env = topEl.ownerAs<DomEnvironment>()) {
            if (env->options() & DomEnvironment::Option::NoDependencies)
                env->loadFile(topEl, filePath, logicalPath, callback, DomTop::Callback(),
                              DomTop::Callback(), loadOptions, fileType);
            else
                env->loadFile(topEl, filePath, logicalPath, DomTop::Callback(), DomTop::Callback(),
                              callback, loadOptions, fileType);
        } else
            Q_ASSERT(false && "expected either DomUniverse or DomEnvironment cast to succeed");
    } else {
        addError(myErrors().warning(tr("loadFile called without DomEnvironment or DomUniverse.")));
        callback(Paths::qmlFileInfoPath(filePath), DomItem::empty, DomItem::empty);
    }
}

void DomItem::loadModuleDependency(QString uri, Version version,
                                   std::function<void(Path, DomItem &, DomItem &)> callback,
                                   ErrorHandler errorHandler)
{
    DomItem topEl = top();
    if (topEl.internalKind() == DomType::DomEnvironment) {
        if (auto envPtr = topEl.ownerAs<DomEnvironment>()) {
            if (envPtr->options() & DomEnvironment::Option::NoDependencies)
                envPtr->loadModuleDependency(topEl, uri, version, callback, nullptr, errorHandler);
            else
                envPtr->loadModuleDependency(topEl, uri, version, nullptr, callback, errorHandler);
        } else
            Q_ASSERT(false && "loadDependency expected the DomEnvironment cast to succeed");
    } else {
        addError(myErrors().warning(tr("loadModuleDependency called without DomEnvironment.")));
        callback(Paths::moduleScopePath(uri, version), DomItem::empty, DomItem::empty);
    }
}

void DomItem::loadBuiltins(std::function<void(Path, DomItem &, DomItem &)> callback, ErrorHandler h)
{
    DomItem env = environment();
    if (std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>())
        envPtr->loadBuiltins(env, callback, h);
    else
        myErrors().error(tr("Cannot load builtins without DomEnvironment")).handle(h);
}

void DomItem::loadPendingDependencies()
{
    DomItem env = environment();
    if (std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>())
        envPtr->loadPendingDependencies(env);
    else
        myErrors().error(tr("Called loadPendingDependencies without environment")).handle();
}

/*!
\brief Creates a new document with the given code

This is mostly useful for testing or loading a single code snippet without any dependency.
The fileType should normally be QmlFile, but you might want to load a qmltypes file for
example and interpret it as qmltypes file (not plain Qml), or as JsFile. In those case
set the file type accordingly.
*/
DomItem DomItem::fromCode(QString code, DomType fileType)
{
    if (code.isEmpty())
        return DomItem();
    DomItem env =
            DomEnvironment::create(QStringList(),
                                   QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                                           | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

    DomItem tFile;
    env.loadFile(
            QString(), QString(), code, QDateTime::currentDateTimeUtc(),
            [&tFile](Path, const DomItem &, const DomItem &newIt) { tFile = newIt; },
            LoadOption::DefaultLoad, fileType);
    env.loadPendingDependencies();
    return tFile.fileObject();
}

Empty::Empty()
{}

Path Empty::pathFromOwner(DomItem &) const
{
    return Path();
}

Path Empty::canonicalPath(DomItem &) const
{
    return Path();
}

bool Empty::iterateDirectSubpaths(DomItem &, DirectVisitor)
{
    return true;
}

DomItem Empty::containingObject(DomItem &self) const
{
    return self;
}

void Empty::dump(DomItem &, Sink s, int,
                 function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)>) const
{
    s(u"null");
}

Map::Map(Path pathFromOwner, Map::LookupFunction lookup, Keys keys, QString targetType)
    : DomElement(pathFromOwner), m_lookup(lookup), m_keys(keys), m_targetType(targetType)
{}

quintptr Map::id() const
{
    return quintptr(0);
}

bool Map::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    QSet<QString> ksSet = keys(self);
    QStringList ksList = QStringList(ksSet.begin(), ksSet.end());
    std::sort(ksList.begin(), ksList.end());
    for (QString k : ksList) {
        if (!visitor(PathEls::Key(k), [&self, this, k]() { return key(self, k); }))
            return false;
    }
    return true;
}

const QSet<QString> Map::keys(DomItem &self) const
{
    return m_keys(self);
}

DomItem Map::key(DomItem &self, QString name) const
{
    return m_lookup(self, name);
}

void DomBase::dump(
        DomItem &self, Sink sink, int indent,
        function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)> filter) const
{
    bool comma = false;
    DomKind dK = self.domKind();
    switch (dK) {
    case DomKind::Object:
        sink(u"{ \"~type~\":");
        sinkEscaped(sink, typeName());
        comma = true;
        break;
    case DomKind::Value:
    {
        QJsonValue v = value().toJsonValue();
        if (v.isString())
            sinkEscaped(sink, v.toString());
        else if (v.isBool())
            if (v.toBool())
                sink(u"true");
            else
                sink(u"false");
        else if (v.isDouble())
            if (value().isInteger())
                sink(QString::number(value().toInteger()));
            else
                sink(QString::number(v.toDouble()));
        else {
            sink(QString::fromUtf8(QJsonDocument::fromVariant(v.toVariant()).toJson()));
        }
        break;
    }
    case DomKind::Empty:
        sink(u"null");
        break;
    case DomKind::List:
        sink(u"[");
        break;
    case DomKind::Map:
        sink(u"{");
        break;
    }
    auto closeParens = qScopeGuard(
                [dK, sink, indent]{
        switch (dK) {
        case DomKind::Object:
            sinkNewline(sink, indent);
            sink(u"}");
            break;
        case DomKind::Value:
            break;
        case DomKind::Empty:
            break;
        case DomKind::List:
            sinkNewline(sink, indent);
            sink(u"]");
            break;
        case DomKind::Map:
            sinkNewline(sink, indent);
            sink(u"}");
            break;
        }
    });
    index_type idx = 0;
    self.iterateDirectSubpaths(
            [&comma, &idx, dK, sink, indent, &self, filter](const PathEls::PathComponent &c,
                                                            function_ref<DomItem()> itemF) {
                DomItem i = itemF();
                if (!filter(self, c, i))
                    return true;
                if (comma)
                    sink(u",");
                else
                    comma = true;
                switch (c.kind()) {
                case Path::Kind::Field:
                    sinkNewline(sink, indent + 2);
                    if (dK != DomKind::Object)
                        sink(u"UNEXPECTED ENTRY ERROR:");
                    sinkEscaped(sink, c.name());
                    sink(u":");
                    break;
                case Path::Kind::Key:
                    sinkNewline(sink, indent + 2);
                    if (dK != DomKind::Map)
                        sink(u"UNEXPECTED ENTRY ERROR:");
                    sinkEscaped(sink, c.name());
                    sink(u":");
                    break;
                case Path::Kind::Index:
                    sinkNewline(sink, indent + 2);
                    if (dK != DomKind::List)
                        sink(u"UNEXPECTED ENTRY ERROR:");
                    else if (idx++ != c.index())
                        sink(u"OUT OF ORDER ARRAY:");
                    break;
                default:
                    sinkNewline(sink, indent + 2);
                    sink(u"UNEXPECTED PATH KIND ERROR (ignored)");
                    break;
                }
                if (self.isCanonicalChild(i)) {
                    i.dump(sink, indent + 2, filter);
                } else {
                    sink(uR"({ "~type~": "Reference", "immediate": true, "referredObjectPath":")");
                    i.canonicalPath().dump([sink](QStringView s) {
                        sinkEscaped(sink, s, EscapeOptions::NoOuterQuotes);
                    });
                    sink(u"\"}");
                }
                return true;
            });
}

List::List(Path pathFromOwner, List::LookupFunction lookup, List::Length length,
           List::IteratorFunction iterator, QString elType):
    DomElement(pathFromOwner), m_lookup(lookup), m_length(length), m_iterator(iterator),
    m_elType(elType)
{}

quintptr List::id() const
{
    return quintptr(0);
}

void List::dump(
        DomItem &self, Sink sink, int indent,
        function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)> filter) const
{
    bool first = true;
    sink(u"[");
    const_cast<List *>(this)->iterateDirectSubpaths(
            self,
            [&self, indent, &first, sink, filter](const PathEls::PathComponent &c,
                                                  function_ref<DomItem()> itemF) {
                DomItem item = itemF();
                if (!filter(self, c, item))
                    return true;
                if (first)
                    first = false;
                else
                    sink(u",");
                sinkNewline(sink, indent + 2);
                item.dump(sink, indent + 2, filter);
                return true;
            });
    sink(u"]");
}

bool List::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    if (m_iterator) {
        return m_iterator(self, [visitor](index_type i, function_ref<DomItem()> itemF) {
            return visitor(PathEls::Index(i), itemF);
        });
    }
    index_type len = indexes(self);
    for (index_type i = 0; i < len; ++i) {
        if (!visitor(PathEls::Index(i), [this, &self, i]() { return index(self, i); }))
            return false;
    }
    return true;
}

index_type List::indexes(DomItem &self) const
{
    return m_length(self);
}

DomItem List::index(DomItem &self, index_type index) const
{
    return m_lookup(self, index);
}

void List::writeOut(DomItem &self, OutWriter &ow, bool compact) const
{
    ow.writeRegion(u"leftSquareBrace", u"[");
    int baseIndent = ow.increaseIndent(1);
    bool first = true;
    const_cast<List *>(this)->iterateDirectSubpaths(
            self,
            [&ow, &first, compact](const PathEls::PathComponent &, function_ref<DomItem()> elF) {
                if (first)
                    first = false;
                else
                    ow.write(u", ", LineWriter::TextAddType::Extra);
                if (!compact)
                    ow.ensureNewline(1);
                DomItem el = elF();
                el.writeOut(ow);
                return true;
            });
    if (!compact && !first)
        ow.newline();
    ow.decreaseIndent(1, baseIndent);
    ow.writeRegion(u"rightSquareBrace", u"]");
}

DomElement::DomElement(Path pathFromOwner) : m_pathFromOwner(pathFromOwner) { }

Path DomElement::pathFromOwner(DomItem &) const
{
    Q_ASSERT(m_pathFromOwner && "uninitialized DomElement");
    return m_pathFromOwner;
}

Path DomElement::canonicalPath(DomItem &self) const
{
    Q_ASSERT(m_pathFromOwner && "uninitialized DomElement");
    return self.owner().canonicalPath().path(m_pathFromOwner);
}

DomItem DomElement::containingObject(DomItem &self) const
{
    Q_ASSERT(m_pathFromOwner && "uninitialized DomElement");
    return DomBase::containingObject(self);
}

void DomElement::updatePathFromOwner(Path newPath)
{
    //if (!domTypeCanBeInline(kind()))
    m_pathFromOwner = newPath;
}

bool Reference::shouldCache() const
{
    for (Path p : referredObjectPath) {
        switch (p.headKind()) {
        case Path::Kind::Current:
            switch (p.headCurrent()) {
            case PathCurrent::Lookup:
            case PathCurrent::LookupDynamic:
            case PathCurrent::LookupStrict:
            case PathCurrent::ObjChain:
            case PathCurrent::ScopeChain:
                return true;
            default:
                break;
            }
            break;
        case Path::Kind::Empty:
        case Path::Kind::Any:
        case Path::Kind::Filter:
            return true;
        default:
            break;
        }
    }
    return false;
}

Reference::Reference(Path referredObject, Path pathFromOwner, const SourceLocation &)
    : DomElement(pathFromOwner), referredObjectPath(referredObject)
{
}

quintptr Reference::id() const
{
    return quintptr(0);
}

bool Reference::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvValueLazyField(visitor, Fields::referredObjectPath, [this]() {
        return referredObjectPath.toString();
    });
    cont = cont
            && self.dvItemField(visitor, Fields::get, [this, &self]() { return this->get(self); });
    return cont;
}

DomItem Reference::field(DomItem &self, QStringView name) const
{
    if (Fields::referredObjectPath == name)
        return self.subDataItemField(Fields::referredObjectPath, referredObjectPath.toString());
    if (Fields::get == name)
        return get(self);
    return DomItem();
}

QList<QString> Reference::fields(DomItem &) const
{
    return QList<QString>({QString::fromUtf16(Fields::referredObjectPath), QString::fromUtf16(Fields::get)});
}

DomItem Reference::index(DomItem &, index_type) const
{
    return DomItem();
}

DomItem Reference::key(DomItem &, QString) const
{
    return DomItem();
}

DomItem Reference::get(DomItem &self, ErrorHandler h, QList<Path> *visitedRefs) const
{
    DomItem res;
    if (referredObjectPath) {
        DomItem env;
        Path selfPath;
        Path cachedPath;
        if (shouldCache()) {
            env = self.environment();
            if (env) {
                selfPath = self.canonicalPath();
                RefCacheEntry cached = RefCacheEntry::forPath(self, selfPath);
                switch (cached.cached) {
                case RefCacheEntry::Cached::None:
                    break;
                case RefCacheEntry::Cached::First:
                case RefCacheEntry::Cached::All:
                    if (!cached.canonicalPaths.isEmpty())
                        cachedPath = cached.canonicalPaths.first();
                    else
                        return res;
                    break;
                }
                if (cachedPath) {
                    res = env.path(cachedPath);
                    if (!res)
                        qCWarning(refLog) << "referenceCache outdated, reference at " << selfPath
                                          << " leads to invalid path " << cachedPath;
                    else
                        return res;
                }
            }
        }
        QList<Path> visitedRefsLocal;
        self.resolve(
                referredObjectPath,
                [&res](Path, DomItem &el) {
                    res = el;
                    return false;
                },
                h, ResolveOption::None, referredObjectPath,
                (visitedRefs ? visitedRefs : &visitedRefsLocal));
        if (env)
            RefCacheEntry::addForPath(
                    env, selfPath, RefCacheEntry { RefCacheEntry::Cached::First, { cachedPath } });
    }
    return res;
}

QList<DomItem> Reference::getAll(DomItem &self, ErrorHandler h, QList<Path> *visitedRefs) const
{
    QList<DomItem> res;
    if (referredObjectPath) {
        DomItem env;
        Path selfPath;
        QList<Path> cachedPaths;
        if (shouldCache()) {
            selfPath = canonicalPath(self);
            env = self.environment();
            RefCacheEntry cached = RefCacheEntry::forPath(env, selfPath);
            switch (cached.cached) {
            case RefCacheEntry::Cached::None:
            case RefCacheEntry::Cached::First:
                break;
            case RefCacheEntry::Cached::All:
                cachedPaths += cached.canonicalPaths;
                if (cachedPaths.isEmpty())
                    return res;
            }
        }
        if (!cachedPaths.isEmpty()) {
            bool outdated = false;
            for (Path p : cachedPaths) {
                DomItem newEl = env.path(p);
                if (!newEl) {
                    outdated = true;
                    qCWarning(refLog) << "referenceCache outdated, reference at " << selfPath
                                      << " leads to invalid path " << p;
                    break;
                } else {
                    res.append(newEl);
                }
            }
            if (outdated) {
                res.clear();
            } else {
                return res;
            }
        }
        self.resolve(
                referredObjectPath,
                [&res](Path, DomItem &el) {
                    res.append(el);
                    return true;
                },
                h, ResolveOption::None, referredObjectPath, visitedRefs);
        if (env) {
            QList<Path> canonicalPaths;
            for (DomItem i : res) {
                if (i)
                    canonicalPaths.append(i.canonicalPath());
                else
                    qCWarning(refLog)
                            << "getAll of reference at " << selfPath << " visits empty items.";
            }
            RefCacheEntry::addForPath(env, selfPath,
                                      RefCacheEntry { RefCacheEntry::Cached::All, canonicalPaths });
        }
    }
    return res;
}

/*!
\internal
\class QQmlJS::Dom::OwningItem

\brief A DomItem that owns other DomItems and is managed through a shared pointer

This is the unit of update of the Dom model, only OwningItem can be updated after
exposed, to update a single element one has to copy its owner, change it, and expose an new one.
The non owning contents of an exposed OwiningItem can safely be considered immutable, and pointers
to them must remain valid for the whole lifetime of the OwningItem (that is managed with
shared_ptr), only the sub OwningItems *might* be change.
The OwningItem has a mutex that controls it access (mainly for the errors, observers, and sub
OwningItems), Access to the rest is *not* controlled, it should either be by a single thread
(during construction), or immutable (after pubblication).

*/

OwningItem::OwningItem(int derivedFrom)
    : m_derivedFrom(derivedFrom),
      m_revision(nextRevision()),
      m_createdAt(QDateTime::currentDateTimeUtc()),
      m_lastDataUpdateAt(m_createdAt),
      m_frozenAt(QDateTime::fromMSecsSinceEpoch(0, UTC))
{}

OwningItem::OwningItem(int derivedFrom, QDateTime lastDataUpdateAt)
    : m_derivedFrom(derivedFrom),
      m_revision(nextRevision()),
      m_createdAt(QDateTime::currentDateTimeUtc()),
      m_lastDataUpdateAt(lastDataUpdateAt),
      m_frozenAt(QDateTime::fromMSecsSinceEpoch(0, UTC))
{}

OwningItem::OwningItem(const OwningItem &o)
    : m_derivedFrom(o.revision()),
      m_revision(nextRevision()),
      m_createdAt(QDateTime::currentDateTimeUtc()),
      m_lastDataUpdateAt(o.lastDataUpdateAt()),
      m_frozenAt(QDateTime::fromMSecsSinceEpoch(0, UTC))
{
    QMultiMap<Path, ErrorMessage> my_errors;
    {
        QMutexLocker l1(o.mutex());
        my_errors = o.m_errors;

    }
    {
        QMutexLocker l2(mutex());
        m_errors = my_errors;
    }
}


int OwningItem::nextRevision()
{
    static QAtomicInt nextRev(0);
    return ++nextRev;
}

bool OwningItem::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvItemField(visitor, Fields::errors, [&self, this]() {
        QMultiMap<Path, ErrorMessage> myErrors = localErrors();
        return self.subMapItem(Map(
                self.pathFromOwner().field(Fields::errors),
                [myErrors](DomItem &map, QString key) {
                    auto it = myErrors.find(Path::fromString(key));
                    if (it != myErrors.end())
                        return map.subDataItem(PathEls::Key(key), it->toCbor(),
                                               ConstantData::Options::FirstMapIsFields);
                    else
                        return DomItem();
                },
                [myErrors](DomItem &) {
                    QSet<QString> res;
                    auto it = myErrors.keyBegin();
                    auto end = myErrors.keyEnd();
                    while (it != end)
                        res.insert(it++->toString());
                    return res;
                },
                QLatin1String("ErrorMessages")));
    });
    return cont;
}

DomItem OwningItem::containingObject(DomItem &self) const
{
    Source s = self.canonicalPath().split();
    if (s.pathFromSource) {
        if (!s.pathToSource)
            return DomItem();
        return self.path(s.pathToSource);
    }
    return DomItem();
}

int OwningItem::derivedFrom() const
{
    return m_derivedFrom;
}

int OwningItem::revision() const
{
    return m_revision;
}

bool OwningItem::frozen() const
{
    return m_frozenAt > m_createdAt;
}

bool OwningItem::freeze()
{
    if (!frozen()) {
        m_frozenAt = QDateTime::currentDateTimeUtc();
        if (m_frozenAt <= m_createdAt)
            m_frozenAt = m_createdAt.addSecs(1);
        return true;
    }
    return false;
}

QDateTime OwningItem::createdAt() const
{
    return m_createdAt;
}

QDateTime OwningItem::lastDataUpdateAt() const
{
    return m_lastDataUpdateAt;
}

QDateTime OwningItem::frozenAt() const
{
    return m_frozenAt;
}

void OwningItem::refreshedDataAt(QDateTime tNew)
{
    if (m_lastDataUpdateAt < tNew) // remove check?
        m_lastDataUpdateAt = tNew;
}

void OwningItem::addError(DomItem &, ErrorMessage msg)
{
    addErrorLocal(msg);
}

void OwningItem::addErrorLocal(ErrorMessage msg)
{
    QMutexLocker l(mutex());
    quint32 &c = m_errorsCounts[msg];
    c += 1;
    if (c == 1)
        m_errors.insert(msg.path, msg);
}

void OwningItem::clearErrors(ErrorGroups groups)
{
    QMutexLocker l(mutex());
    auto it = m_errors.begin();
    while (it != m_errors.end()) {
      if (it->errorGroups == groups)
        it = m_errors.erase(it);
      else
        ++it;
    }
}

bool OwningItem::iterateErrors(DomItem &self, function_ref<bool(DomItem, ErrorMessage)> visitor,
                               Path inPath)
{
    QMultiMap<Path, ErrorMessage> myErrors;
    {
        QMutexLocker l(mutex());
        myErrors = m_errors;
    }
    auto it = myErrors.lowerBound(inPath);
    auto end = myErrors.end();
    while (it != end && it.key().mid(0, inPath.length()) == inPath) {
        if (!visitor(self, *it++))
            return false;
    }
    return true;
}

bool OwningItem::iterateSubOwners(DomItem &self, function_ref<bool(DomItem &owner)> visitor)
{
    return self.iterateDirectSubpaths(
            [&self, visitor](const PathEls::PathComponent &, function_ref<DomItem()> iF) {
                DomItem i = iF();
                if (i.owningItemPtr() != self.owningItemPtr()) {
                    DomItem container = i.container();
                    if (container.id() == self.id())
                        return visitor(i);
                }
                return true;
            });
}

bool operator==(const DomItem &o1c, const DomItem &o2c)
{
    DomItem &o1 = *const_cast<DomItem *>(&o1c);
    DomItem &o2 = *const_cast<DomItem *>(&o2c);
    if (o1.m_kind != o2.m_kind)
        return false;
    return o1.visitMutableEl([&o1, &o2](auto &&el1) {
        auto &&el2 = std::get<std::decay_t<decltype(el1)>>(o2.m_element);
        auto id1 = el1->id();
        auto id2 = el2->id();
        if (id1 != id2)
            return false;
        if (id1 != quintptr(0))
            return true;
        if (o1.m_owner != o2.m_owner)
            return false;
        Path p1 = el1->pathFromOwner(o1);
        Path p2 = el2->pathFromOwner(o2);
        if (p1 != p2)
            return false;
        return true;
    });
}

ErrorHandler MutableDomItem::errorHandler()
{
    MutableDomItem self;
    return [&self](ErrorMessage m) { self.addError(m); };
}

MutableDomItem MutableDomItem::addPrototypePath(Path prototypePath)
{
    if (QmlObject *el = mutableAs<QmlObject>()) {
        return path(el->addPrototypePath(prototypePath));
    } else {
        Q_ASSERT(false && "setPrototypePath on non qml scope");
        return {};
    }
}

MutableDomItem MutableDomItem::setNextScopePath(Path nextScopePath)
{
    if (QmlObject *el = mutableAs<QmlObject>()) {
        el->setNextScopePath(nextScopePath);
        return field(Fields::nextScope);
    } else {
        Q_ASSERT(false && "setNextScopePath on non qml scope");
        return {};
    }
}

MutableDomItem MutableDomItem::setPropertyDefs(QMultiMap<QString, PropertyDefinition> propertyDefs)
{
    if (QmlObject *el = mutableAs<QmlObject>()) {
        el->setPropertyDefs(propertyDefs);
        return field(Fields::propertyDefs);
    } else {
        Q_ASSERT(false && "setPropertyDefs on non qml scope");
        return {};
    }
}

MutableDomItem MutableDomItem::setBindings(QMultiMap<QString, Binding> bindings)
{
    if (QmlObject *el = mutableAs<QmlObject>()) {
        el->setBindings(bindings);
        return field(Fields::bindings);
    } else {
        Q_ASSERT(false && "setBindings on non qml scope");
        return {};
    }
}

MutableDomItem MutableDomItem::setMethods(QMultiMap<QString, MethodInfo> functionDefs)
{
    if (QmlObject *el = mutableAs<QmlObject>())
        el->setMethods(functionDefs);
    else
        Q_ASSERT(false && "setMethods on non qml scope");
    return {};
}

MutableDomItem MutableDomItem::setChildren(QList<QmlObject> children)
{
    if (QmlObject *el = mutableAs<QmlObject>()) {
        el->setChildren(children);
        return field(Fields::children);
    } else
        Q_ASSERT(false && "setChildren on non qml scope");
    return {};
}

MutableDomItem MutableDomItem::setAnnotations(QList<QmlObject> annotations)
{
    if (QmlObject *el = mutableAs<QmlObject>())
        el->setAnnotations(annotations);
    else if (Binding *el = mutableAs<Binding>()) {
        el->setAnnotations(annotations);
        el->updatePathFromOwner(pathFromOwner());
    } else if (PropertyDefinition *el = mutableAs<PropertyDefinition>()) {
        el->annotations = annotations;
        el->updatePathFromOwner(pathFromOwner());
    } else if (MethodInfo *el = mutableAs<MethodInfo>()) {
        el->annotations = annotations;
        el->updatePathFromOwner(pathFromOwner());
    } else if (EnumDecl *el = mutableAs<EnumDecl>()) {
        el->setAnnotations(annotations);
        el->updatePathFromOwner(pathFromOwner());
    } else if (!annotations.isEmpty()) {
        Q_ASSERT(false && "setAnnotations on element not supporting them");
    }
    return field(Fields::annotations);
}
MutableDomItem MutableDomItem::setScript(std::shared_ptr<ScriptExpression> exp)
{
    switch (internalKind()) {
    case DomType::Binding:
        if (Binding *b = mutableAs<Binding>()) {
            b->setValue(std::make_unique<BindingValue>(exp));
            return field(Fields::value);
        }
        break;
    case DomType::MethodInfo:
        if (MethodInfo *m = mutableAs<MethodInfo>()) {
            m->body = exp;
            return field(Fields::body);
        }
        break;
    case DomType::MethodParameter:
        if (MethodParameter *p = mutableAs<MethodParameter>()) {
            p->defaultValue = exp;
            return field(Fields::body);
        }
        break;
    case DomType::ScriptExpression:
        return container().setScript(exp);
    default:
        qCWarning(domLog) << "setScript called on" << internalKindStr();
        Q_ASSERT_X(false, "setScript",
                   "setScript supported only on Binding, MethodInfo, MethodParameter, and "
                   "ScriptExpression contained in them");
    }
    return MutableDomItem();
}

MutableDomItem MutableDomItem::setCode(QString code)
{
    DomItem it = item();
    switch (it.internalKind()) {
    case DomType::Binding:
        if (Binding *b = mutableAs<Binding>()) {
            auto exp = std::make_shared<ScriptExpression>(
                    code, ScriptExpression::ExpressionType::BindingExpression);
            b->setValue(std::make_unique<BindingValue>(exp));
            return field(Fields::value);
        }
        break;
    case DomType::MethodInfo:
        if (MethodInfo *m = mutableAs<MethodInfo>()) {
            QString pre = m->preCode(it);
            QString post = m->preCode(it);
            m->body = std::make_shared<ScriptExpression>(
                    code, ScriptExpression::ExpressionType::FunctionBody, 0, pre, post);
            return field(Fields::body);
        }
        break;
    case DomType::MethodParameter:
        if (MethodParameter *p = mutableAs<MethodParameter>()) {
            p->defaultValue = std::make_shared<ScriptExpression>(
                    code, ScriptExpression::ExpressionType::ArgInitializer);
            return field(Fields::defaultValue);
        }
        break;
    case DomType::ScriptExpression:
        if (std::shared_ptr<ScriptExpression> exp = ownerAs<ScriptExpression>()) {
            std::shared_ptr<ScriptExpression> newExp = exp->copyWithUpdatedCode(it, code);
            return container().setScript(newExp);
        }
        break;
    default:
        qCWarning(domLog) << "setCode called on" << internalKindStr();
        Q_ASSERT_X(
                false, "setCode",
                "setCode supported only on Binding, MethodInfo, MethodParameter, ScriptExpression");
    }
    return MutableDomItem();
}

MutableDomItem MutableDomItem::addPropertyDef(PropertyDefinition propertyDef, AddOption option)
{
    if (QmlObject *el = mutableAs<QmlObject>())
        return el->addPropertyDef(*this, propertyDef, option);
    else
        Q_ASSERT(false && "addPropertyDef on non qml scope");
    return MutableDomItem();
}

MutableDomItem MutableDomItem::addBinding(Binding binding, AddOption option)
{
    if (QmlObject *el = mutableAs<QmlObject>())
        return el->addBinding(*this, binding, option);
    else
        Q_ASSERT(false && "addBinding on non qml scope");
    return MutableDomItem();
}

MutableDomItem MutableDomItem::addMethod(MethodInfo functionDef, AddOption option)
{
    if (QmlObject *el = mutableAs<QmlObject>())
        return el->addMethod(*this, functionDef, option);
    else
        Q_ASSERT(false && "addMethod on non qml scope");
    return MutableDomItem();
}

MutableDomItem MutableDomItem::addChild(QmlObject child)
{
    if (QmlObject *el = mutableAs<QmlObject>()) {
        return el->addChild(*this, child);
    } else if (QmlComponent *el = mutableAs<QmlComponent>()) {
        Path p = el->addObject(child);
        return owner().path(p); // convenience: treat component objects as children
    } else {
        Q_ASSERT(false && "addChild on non qml scope");
    }
    return MutableDomItem();
}

MutableDomItem MutableDomItem::addAnnotation(QmlObject annotation)
{
    Path res;
    switch (internalKind()) {
    case DomType::QmlObject: {
        QmlObject *el = mutableAs<QmlObject>();
        res = el->addAnnotation(annotation);
    } break;
    case DomType::Binding: {
        Binding *el = mutableAs<Binding>();

        res = el->addAnnotation(m_pathFromOwner, annotation);
    } break;
    case DomType::PropertyDefinition: {
        PropertyDefinition *el = mutableAs<PropertyDefinition>();
        res = el->addAnnotation(m_pathFromOwner, annotation);
    } break;
    case DomType::MethodInfo: {
        MethodInfo *el = mutableAs<MethodInfo>();
        res = el->addAnnotation(m_pathFromOwner, annotation);
    } break;
    case DomType::Id: {
        Id *el = mutableAs<Id>();
        res = el->addAnnotation(m_pathFromOwner, annotation);
    } break;
    default:
        Q_ASSERT(false && "addAnnotation on element not supporting them");
    }
    return MutableDomItem(owner().item(), res);
}

MutableDomItem MutableDomItem::addPreComment(const Comment &comment, QString regionName)
{
    index_type idx;
    MutableDomItem rC = field(Fields::comments);
    if (auto rcPtr = rC.mutableAs<RegionComments>()) {
        auto &preList = rcPtr->regionComments[regionName].preComments;
        idx = preList.size();
        preList.append(comment);
        MutableDomItem res = path(Path::Field(Fields::comments)
                                          .field(Fields::regionComments)
                                          .key(regionName)
                                          .field(Fields::preComments)
                                          .index(idx));
        Q_ASSERT(res);
        return res;
    }
    return MutableDomItem();
}

MutableDomItem MutableDomItem::addPostComment(const Comment &comment, QString regionName)
{
    index_type idx;
    MutableDomItem rC = field(Fields::comments);
    if (auto rcPtr = rC.mutableAs<RegionComments>()) {
        auto &postList = rcPtr->regionComments[regionName].postComments;
        idx = postList.size();
        postList.append(comment);
        MutableDomItem res = path(Path::Field(Fields::comments)
                                          .field(Fields::regionComments)
                                          .key(regionName)
                                          .field(Fields::postComments)
                                          .index(idx));
        Q_ASSERT(res);
        return res;
    }
    return MutableDomItem();
}

QDebug operator<<(QDebug debug, const DomItem &c)
{
    dumperToQDebug([&c](Sink s) { const_cast<DomItem *>(&c)->dump(s); }, debug);
    return debug;
}

QDebug operator<<(QDebug debug, const MutableDomItem &c)
{
    MutableDomItem cc(c);
    return debug.noquote().nospace() << "MutableDomItem(" << domTypeToString(cc.internalKind())
                                     << ", " << cc.canonicalPath().toString() << ")";
}

bool ListPBase::iterateDirectSubpaths(DomItem &self, DirectVisitor v)
{
    index_type len = index_type(m_pList.size());
    for (index_type i = 0; i < len; ++i) {
        if (!v(PathEls::Index(i), [this, &self, i] { return this->index(self, i); }))
            return false;
    }
    return true;
}

void ListPBase::writeOut(DomItem &self, OutWriter &ow, bool compact) const
{
    ow.writeRegion(u"leftSquareBrace", u"[");
    int baseIndent = ow.increaseIndent(1);
    bool first = true;
    index_type len = index_type(m_pList.size());
    for (index_type i = 0; i < len; ++i) {
        if (first)
            first = false;
        else
            ow.write(u", ", LineWriter::TextAddType::Extra);
        if (!compact)
            ow.ensureNewline(1);
        DomItem el = index(self, i);
        el.writeOut(ow);
    }
    if (!compact && !first)
        ow.newline();
    ow.decreaseIndent(1, baseIndent);
    ow.writeRegion(u"rightSquareBrace", u"]");
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE

#include "moc_qqmldomitem_p.cpp"
