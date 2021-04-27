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
#include "qqmldomitem_p.h"
#include "qqmldomtop_p.h"

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPair>
#include <QtCore/QScopeGuard>
#include <QtCore/QMutexLocker>
#include <QtCore/QCborMap>
#include <QtCore/QCborArray>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonDocument>

QT_BEGIN_NAMESPACE


namespace QQmlJS {
namespace Dom {

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
    Path pathFromOwner(const DomItem &self) const override; // returns the path from the owner to the current element
    Path canonicalPath(const DomItem &self) const override; // returns the path from
    virtual bool iterateDirectSubpaths(const DomItem &self, function_ref<bool(Path, DomItem)>) const = 0; // iterates the *direct* subpaths, returns false if a quick end was requested
\endcode
But you probably want to subclass either DomElement of OwningItem for your element.
DomElement stores its pathFromOwner, and computes the canonicalPath from it and its owner.
OwningItem is the unit for updates to the Dom model, exposed changes always change at least one OwningItem.
They have their lifetime handled with shared_ptr and own (i.e. are responsible of freeing) other items in them.

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
    return domTypeToStringMap().value(k, QString::number(int(k)));
}

bool domTypeIsObjWrap(DomType k)
{
    switch (k) {
    case DomType::ModuleAutoExport:
    case DomType::Import:
    case DomType::Export:
    case DomType::SimpleObjectWrap:
    case DomType::Version:
    case DomType::ErrorMessage:
    case DomType::PropertyDefinition:
    case DomType::MethodParameter:
    case DomType::MethodInfo:
    case DomType::Pragma:
    case DomType::Id:
    case DomType::EnumItem:
    case DomType::Binding:
    case DomType::RequiredProperty:
    case DomType::FileLocations:
        return true;
    default:
        return false;
    }
}

bool domTypeIsDomElement(DomType k)
{
    switch (k) {
    case DomType::ModuleScope:
    case DomType::QmlObject:
    case DomType::ConstantData:
    case DomType::SimpleObjectWrap:
    case DomType::ScriptExpression:
    case DomType::Reference:
    case DomType::Map:
    case DomType::List:
    case DomType::EnumDecl:
    case DomType::JsResource:
    case DomType::QmltypesComponent:
    case DomType::QmlComponent:
    case DomType::GlobalComponent:
    case DomType::GenericObject:
        return true;
    default:
        return false;
    }
}

bool domTypeIsOwningItem(DomType k)
{
    switch (k) {
    case DomType::ModuleIndex:

    case DomType::GenericOwner:

    case DomType::QmlDirectory:
    case DomType::JsFile:
    case DomType::QmlFile:
    case DomType::QmltypesFile:
    case DomType::GlobalScope:

    case DomType::LoadInfo:
    case DomType::AttachedInfo:

    case DomType::DomEnvironment:
    case DomType::DomUniverse:
        return true;
    default:
        return false;
    }
};

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
        return true;
    default:
        return false;
    }
}

bool domTypeCanBeInline(DomType k)
{
    switch (k) {
    case DomType::Empty:
    case DomType::Map:
    case DomType::List:
    case DomType::ConstantData:
    case DomType::SimpleObjectWrap:
    case DomType::Reference:
        return true;
    default:
        return false;
    }
};

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

DomKind DomBase::domKind() const
{
    return kind2domKind(kind());
}

bool DomBase::iterateDirectSubpathsConst(const DomItem &self, function_ref<bool (Path, const DomItem &)>visitor) const
{
    return const_cast<DomBase *>(this)->iterateDirectSubpaths(
                *const_cast<DomItem *>(&self),
                [visitor](Path p, DomItem &item) {
        return visitor(p, item);
    });
}

DomItem DomBase::containingObject(const DomItem &self) const
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

quintptr DomBase::id() const
{
    return quintptr(this);
}

QString DomBase::typeName() const
{
    return domTypeToStringMap()[kind()];
}

QList<QString> const DomBase::fields(const DomItem &self) const
{
    QList<QString> res;
    iterateDirectSubpathsConst(self, [&res](Path p, const DomItem &){
        if (p.headKind() == Path::Kind::Field)
            res.append(p.headName());
        return true;
    });
    return res;
}

DomItem DomBase::field(const DomItem &self, QStringView name) const
{
    DomItem res;
    iterateDirectSubpathsConst(self, [&res, name](Path p, const DomItem & i){
        if (p.headKind() == Path::Kind::Field && p.checkHeadName(name)) {
            res = i;
            return false;
        }
        return true;
    });
    return res;
}

index_type DomBase::indexes(const DomItem &self) const
{
    index_type res = 0;
    iterateDirectSubpathsConst(self, [&res](Path p, const DomItem &){
        if (p.headKind() == Path::Kind::Index) {
            index_type i = p.headIndex() + 1;
            if (res < i)
                res = i;
        }
        return true;
    });
    return res;
}

DomItem DomBase::index(const DomItem &self, qint64 index) const
{
    DomItem res;
    iterateDirectSubpathsConst(self, [&res, index](Path p, const DomItem &i){
        if (p.headKind() == Path::Kind::Index && p.headIndex() == index) {
            res = i;
            return false;
        }
        return true;
    });
    return res;
}

QSet<QString> const DomBase::keys(const DomItem &self) const
{
    QSet<QString> res;
    iterateDirectSubpathsConst(self, [&res](Path p, const DomItem &){
        if (p.headKind() == Path::Kind::Key)
            res.insert(p.headName());
        return true;
    });
    return res;
}

DomItem DomBase::key(const DomItem &self, QString name) const
{
    DomItem res;
    iterateDirectSubpathsConst(self, [&res, name](Path p, const DomItem &i){
        if (p.headKind() == Path::Kind::Key && p.checkHeadName(name)) {
            res = i;
            return false;
        }
        return true;
    });
    return res;
}

QString DomBase::canonicalFilePath(const DomItem & self) const
{
    auto parent = containingObject(self);
    if (parent)
        return parent.canonicalFilePath();
    return QString();
}

SourceLocation DomBase::location(const DomItem & self) const
{
    auto parent = containingObject(self);
    if (parent)
        return parent.location();
    return SourceLocation();
}

ConstantData::ConstantData(Path pathFromOwner, QCborValue value, Options options, const SourceLocation & loc):
    DomElement(pathFromOwner, loc), m_value(value), m_options(options)
{}

bool ConstantData::iterateDirectSubpaths(DomItem &self, function_ref<bool (Path, DomItem &)> visitor)
{
    if (m_value.isMap()) {
        QCborMap map = m_value.toMap();
        auto it = map.cbegin();
        auto end = map.cend();
        while (it != end) {
            QString key = it.key().toString();
            Path path;
            switch (m_options) {
            case ConstantData::Options::MapIsMap:
                path = Path::Key(key);
                break;
            case ConstantData::Options::FirstMapIsFields:
                path = Path::Field(key);
                break;
            }
            if (!self.subDataPath(path, it.value()).visit(visitor))
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
            if (!self.subDataPath(Path::Index(i++), *it++).visit(visitor))
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

SimpleObjectWrap::SimpleObjectWrap(Path pathFromOwner, QVariant value,
                                   std::function<bool(DomItem &, QVariant, function_ref<bool(Path, DomItem &)>)> directSubpathsIterate,
                                   DomType kind,
                                   DomKind domKind,
                                   QString typeName,
                                   const SourceLocation & loc):
    DomElement(pathFromOwner, loc), m_kind(kind), m_domKind(domKind), m_typeName(typeName), m_value(value),
    m_directSubpathsIterate(directSubpathsIterate)
{}

bool SimpleObjectWrap::iterateDirectSubpaths(DomItem &self, function_ref<bool (Path, DomItem &)> visitor)
{
    return m_directSubpathsIterate(self, m_value, visitor);
}

quintptr SimpleObjectWrap::id() const
{
    return quintptr(m_value.value<void *>());
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
\li visitChildren can be used to visit all subEments, if preferred on the top of it a visitor
    pattern can also be used.
\li If element specific attributes are wanted the two template casting as and ownerAs allow safe casting
of the DomItem to a specific concrete type (cast to superclasses is not supported).
\li Multithreading does not create issues, because even if an update replacing an OwningItem takes
    place the DomItem keeps a shared_ptr to the current owner as long as you use it
\li Some elements (Empty, List, Map, ConstantData, Reference) might be inline, meaning that they are
    generated on the fly, wrapping data of the original object.
\endlist

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

Path DomItem::canonicalPath() const
{
    Path res = base()->canonicalPath(*this);
    Q_ASSERT((!res || res.headKind() == Path::Kind::Root) && "non anchored canonical path");
    return res;

}

DomItem DomItem::containingObject() const
{
    return base()->containingObject(*this);
}

DomItem DomItem::fileObject(GoTo options) const
{
    DomItem res = *this;
    DomType k = res.internalKind();
    if (k == DomType::List || k == DomType::Map) {
        res = res.containingObject();
        k = res.internalKind();
    }
    if (k == DomType::ExternalItemInfo || (options == GoTo::MostLikely && k == DomType::ExternalItemPair))
        return field(Fields::currentItem);
    while (res) {
        if (k == DomType::QmlFile || k == DomType::QmldirFile || k == DomType::QmltypesFile
            || k == DomType::JsFile)
            break;
        res = res.containingObject();
        k = res.internalKind();
    }
    return res;
}

DomItem DomItem::container() const
{
    Path path = pathFromOwner();
    if (!path)
        path = canonicalPath();
    Source s = path.split();
    if (s.pathFromSource.length() > 1)
        return containingObject().path(s.pathFromSource.dropTail());
    return containingObject();
}

DomItem DomItem::owner() const {
    return DomItem(m_top, m_owner, m_ownerPath, m_owner.get());
}

DomItem DomItem::top() const
{
    return DomItem(m_top, m_top, m_ownerPath, m_top.get());
}

DomItem DomItem::environment() const
{
    DomItem res = top();
    if (res.internalKind() == DomType::DomEnvironment)
        return res;
    return DomItem(); // we are in the universe, and cannot go back to the environment...
}

DomItem DomItem::universe() const
{
    DomItem res = top();
    if (res.internalKind() == DomType::DomUniverse)
            return res;
    if (res.internalKind() == DomType::DomEnvironment)
        return res.field(Fields::universe);
    return DomItem(); // we should be in an empty DomItem already...
}

QString DomItem::name() const
{
    return field(Fields::name).value().toString();
}

DomItem DomItem::qmlChildren() const
{
    return field(Fields::children);
}

DomItem DomItem::annotations() const
{
    return field(Fields::annotations);
}

DomItem DomItem::component() const
{
    DomItem item = *this;
    while (item) {
        DomType kind = item.internalKind() ;
        if (kind == DomType::QmlComponent || kind == DomType::QmltypesComponent || kind == DomType::GlobalComponent)
            return item;
        item = item.containingObject();
    }
    return DomItem();
}

struct ResolveToDo {
    DomItem item;
    int pathIndex;
};

bool DomItem::resolve(Path path,
                      DomItem::Visitor visitor,
                      ErrorHandler errorHandler,
                      ResolveOptions options,
                      Path fullPath,
                      QList<Path> *visitedRefs) const
{
    QList<Path> vRefs;
    Path fPath = fullPath;
    if (fullPath.length() == 0)
        fPath = path;
    if (path.length()==0)
        return visitor(fPath, *this);
    QSet<QPair<quintptr,int> > visited;
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
            auto idNow = toDo.item.base()->id();
            if (idNow == quintptr(0) && toDo.item == *this)
                idNow = quintptr(base());
            if (idNow != quintptr(0) && visited.contains(qMakePair(idNow,0)))
                continue;
        }
        int iPath = toDo.pathIndex;
        DomItem it = toDo.item;
        bool branchExhausted = false;
        while (iPath < path.length() && it && !branchExhausted) {
            auto idNow = it.base()->id();
            if (idNow == quintptr() && toDo.item == *this)
                idNow = quintptr(base());
            if (idNow != quintptr(0)) {
                auto vPair = qMakePair(idNow, iPath);
                if (visited.contains(vPair))
                    break;
                visited.insert(vPair);
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
                    if (visitedRefs == nullptr) {
                        visitedRefs = &vRefs;
                        visitedRefs->append(fPath);
                    }
                    if (visitedRefs->contains(toResolve)) {
                        myResolveErrors().error([visitedRefs, toResolve](Sink sink) {
                            sink(tr("Circular reference:"));
                            sink(u"\n");
                            for (const Path &vPath : *visitedRefs) {
                                sink(u"  ");
                                vPath.dump(sink);
                                sink(u" >\n");
                            }
                            toResolve.dump(sink);
                        }).handle(errorHandler);
                        it = DomItem();
                    } else {
                        DomItem resolveRes;
                        it.resolve(toResolve, [&resolveRes](Path, const DomItem &r) {
                            resolveRes = r;
                            return false;
                        }, errorHandler, ResolveOption::None, toResolve, visitedRefs);
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
                    visitChildren(Path(),[toFind, &toDos, iPath](Path, const DomItem &item, bool) {
                        // avoid non directly attached?
                        DomItem newItem = item[toFind];
                        if (newItem)
                            toDos.append({newItem, iPath});
                        return true;
                    }, VisitOption::Recurse | VisitOption::VisitAdopted | VisitOption::NoPath);
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
                case PathCurrent::ObjChain:

                case PathCurrent::ScopeChain:
                case PathCurrent::Component:
                    it = it.component();
                    break;
                case PathCurrent::Module:
                case PathCurrent::Ids:
                    it = it.component().field(Fields::ids);
                    break;
                case PathCurrent::Types:
                    it = it.component()[Fields::exports];
                    break;
                case PathCurrent::LookupStrict:
                case PathCurrent::LookupDynamic:
                case PathCurrent::Lookup:
                    if (current == PathCurrent::Lookup) {
                        DomItem strict = it.component().field(u"~strictLookup~");
                        if (!strict)
                            strict = it.environment().field(u"defaultStrictLookup");
                        if (strict && strict.value().toBool())
                            current = PathCurrent::LookupStrict;
                        else
                            current = PathCurrent::LookupDynamic;
                    }
                    if (current == PathCurrent::LookupStrict) {
                        myResolveErrors().error(tr("@lookupStrict unimplemented"))
                                .handle(errorHandler);
                        return false;
                    } else if (current == PathCurrent::LookupDynamic) {
                        // expand, add self, prototype, components, prototype, global
                        DomItem proto = it.component()[u"~prototype~"];
                        if (proto) {
                            DomItem pVal = proto[u"get"];
                            if (!pVal) {
                                myResolveErrors().warning(tr("Could not find prototype %1").arg(proto.toString()))
                                        .handle(errorHandler);
                            } else {
                                toDos.append({proto, iPath - 1});
                            }
                        }
                        myResolveErrors().error(tr("@lookupDynamic unimplemented"))
                                .handle(errorHandler);
                        return false;
                    } else {
                        myResolveErrors().error(tr("Unexpected Current path component %1").arg(cNow.headName()))
                                .handle(errorHandler);
                        return false;
                    }
                    break;
                }
                break;
            }
            case Path::Kind::Any:
                visitChildren(Path(), [&toDos, iPath](Path, const DomItem &item, bool) {
                    toDos.append({item, iPath});
                    return true;
                }, VisitOption::VisitAdopted);
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

DomItem DomItem::path(Path p, ErrorHandler errorHandler) const
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

DomItem DomItem::path(QString p, ErrorHandler errorHandler) const
{
    return path(Path::fromString(p, errorHandler));
}

DomItem DomItem::path(QStringView p, ErrorHandler errorHandler) const
{
    return path(Path::fromString(p, errorHandler));
}

QList<QString> const DomItem::fields() const
{
    return base()->fields(*this);
}

DomItem DomItem::field(QStringView name) const
{
    return base()->field(*this, name);
}

index_type DomItem::indexes() const
{
    return base()->indexes(*this);
}

DomItem DomItem::index(index_type i) const
{
    return base()->index(*this, i);
}

QSet<QString> const DomItem::keys() const
{
    return base()->keys(*this);
}

DomItem DomItem::key(QString name) const
{
    return base()->key(*this, name);
}

bool DomItem::visitChildren(
        Path basePath,
        DomItem::ChildrenVisitor visitor,
        VisitOptions options,
        DomItem::ChildrenVisitor openingVisitor,
        DomItem::ChildrenVisitor closingVisitor) const
{
    if (!*this)
        return true;
    if (visitor && ! visitor(basePath, *this, true))
        return false;
    if (openingVisitor && !openingVisitor(basePath, *this, true))
        return true;
    auto atEnd = qScopeGuard([closingVisitor, basePath, this](){
        if (closingVisitor)
            closingVisitor(basePath, *this, true);
    });
    return base()->iterateDirectSubpathsConst(*this,
                                  [this, basePath, visitor, openingVisitor, closingVisitor, options]
                                  (Path p, const DomItem &item)
    {
        Path pNow;
        if (!(options & VisitOption::NoPath))
            pNow = basePath.path(p);
        if (item.containingObject() != *this) {
            if (!(options & VisitOption::VisitAdopted))
                return true;
            if (visitor && !visitor(pNow, item, false))
                return false;
            if (openingVisitor && !openingVisitor(pNow, item, false))
                return true;
            if (closingVisitor)
                closingVisitor(pNow, item, false);
        } else {
            return item.visitChildren(pNow, visitor, options, openingVisitor, closingVisitor);
        }
        return true;
    });
}

DomItem DomItem::operator[](const QString &cName) const
{
    if (internalKind() == DomType::Map)
        return key(cName);
    return field(cName);
}

DomItem DomItem::operator[](QStringView cName) const
{
    if (internalKind() == DomType::Map)
        return key(cName.toString());
    return field(cName);
}

DomItem DomItem::operator[](Path p) const
{
    return path(p);
}

QCborValue DomItem::value() const
{
    if (internalKind() == DomType::ConstantData)
        return static_cast<ConstantData const *>(base())->value();
    return QCborValue();
}

void DomItem::dumpPtr(Sink sink) const
{
    sink(u"DomItem{ topPtr:");
    sink(QString::number((quintptr)m_top.get(),16));
    sink(u", ownerPtr:");
    sink(QString::number((quintptr)m_owner.get(),16));
    sink(u", m_basePtr:");
    sink(QString::number((quintptr)m_base,16));
    sink(u", basePtr:");
    sink(QString::number((quintptr)base(),16));
    sink(u"}");
}

void DomItem::dump(Sink s, int indent) const
{
    base()->dump(*this, s, indent);
}

QString DomItem::toString() const
{
    return dumperToString([this](Sink s){ dump(s); });
}

int DomItem::derivedFrom() const
{
    if (m_owner)
        return m_owner->derivedFrom();
    return 0;
}

int DomItem::revision() const {
    if (m_owner)
        return m_owner->revision();
    else
        return -1;
}

QDateTime DomItem::createdAt() const
{
    if (m_owner)
        return m_owner->createdAt();
    else
        return QDateTime::fromMSecsSinceEpoch(0);
}

QDateTime DomItem::frozenAt() const
{
    if (m_owner)
        return m_owner->frozenAt();
    else
        return QDateTime::fromMSecsSinceEpoch(0);
}

QDateTime DomItem::lastDataUpdateAt() const
{
    if (m_owner)
        return m_owner->lastDataUpdateAt();
    else
        return QDateTime::fromMSecsSinceEpoch(0);
}

void DomItem::addError(ErrorMessage msg) const
{
    if (m_owner)
        m_owner->addError(this->copy(m_owner), msg.withItem(*this));
    else
        defaultErrorHandler(msg.withItem(*this));
}

ErrorHandler DomItem::errorHandler() const
{
    DomItem self = *this;
    return [self](ErrorMessage m){
               self.addError(m);
           };
}

void DomItem::clearErrors(ErrorGroups groups, bool iterate) const
{
    if (m_owner) {
        m_owner->clearErrors(groups);
        if (iterate)
            iterateSubOwners([groups](DomItem i){
                i.clearErrors(groups, true);
                return true;
            });
    }
}

bool DomItem::iterateErrors(function_ref<bool (DomItem, ErrorMessage)> visitor, bool iterate,
                            Path inPath) const
{
    if (m_owner) {
        if (!m_owner->iterateErrors(owner(), visitor, inPath))
            return false;
        if (iterate && !iterateSubOwners([inPath, visitor](DomItem i){
                return i.iterateErrors(visitor, true, inPath);
            }))
            return false;
    }
    return true;
}

bool DomItem::iterateSubOwners(function_ref<bool (DomItem)> visitor) const
{
    if (m_owner)
        return m_owner->iterateSubOwners(owner(), visitor);
    return true;
}

shared_ptr<DomTop> DomItem::topPtr() const
{
    return m_top;
}

shared_ptr<OwningItem> DomItem::owningItemPtr() const
{
    return m_owner;
}

DomItem DomItem::copy(shared_ptr<OwningItem> owner, Path ownerPath, DomBase *base) const
{
    return DomItem(m_top, owner, ownerPath, base);
}

DomItem DomItem::copy(shared_ptr<OwningItem> owner, Path ownerPath) const
{
    return DomItem(m_top, owner, ownerPath, owner.get());
}

DomItem DomItem::copy(DomBase *base) const
{
    return DomItem(m_top, m_owner, m_ownerPath, base);
}

Subpath DomItem::subDataField(QStringView fieldName, QCborValue value, ConstantData::Options options, const SourceLocation & loc) const
{
    return subDataPath(Path::Field(fieldName), value, options, loc);
}

Subpath DomItem::subDataField(QString fieldName, QCborValue value, ConstantData::Options options, const SourceLocation & loc) const
{
    return subDataPath(Path::Field(fieldName), value, options, loc);
}

Subpath DomItem::subDataIndex(index_type i, QCborValue value, ConstantData::Options options, const SourceLocation & loc) const
{
    return subDataPath(Path::Index(i), value, options, loc);
}

Subpath DomItem::subDataKey(QStringView keyName, QCborValue value, ConstantData::Options options, const SourceLocation & loc) const
{
    return subDataPath(Path::Key(keyName), value, options, loc);
}

Subpath DomItem::subDataKey(QString keyName, QCborValue value, ConstantData::Options options, const SourceLocation & loc) const
{
    return subDataPath(Path::Key(keyName), value, options, loc);
}

Subpath DomItem::subDataPath(Path path, QCborValue value, ConstantData::Options options, const SourceLocation & loc) const
{
    if (domTypeIsOwningItem(internalKind()))
        return Subpath{path, DomItem(m_top, m_owner,
                                     ConstantData(path, value, options, loc))};
    else
        return Subpath{path, DomItem(m_top, m_owner,
                                     ConstantData(pathFromOwner().path(path), value, options, loc))};
}

Subpath DomItem::subReferenceField(QStringView fieldName, Path referencedObject,
                                   const SourceLocation & loc) const
{
    return subReferencePath(Path::Field(fieldName), referencedObject, loc);
}

Subpath DomItem::subReferenceField(QString fieldName, Path referencedObject, const SourceLocation & loc) const
{
    return subReferencePath(Path::Field(fieldName), referencedObject, loc);
}

Subpath DomItem::subReferenceKey(QStringView keyName, Path referencedObject, const SourceLocation & loc) const
{
    return subReferencePath(Path::Key(keyName), referencedObject,loc);
}

Subpath DomItem::subReferenceKey(QString keyName, Path referencedObject, const SourceLocation & loc) const
{
    return subReferencePath(Path::Key(keyName), referencedObject, loc);
}

Subpath DomItem::subReferenceIndex(index_type i, Path referencedObject, const SourceLocation & loc) const
{
    return subReferencePath(Path::Index(i), referencedObject, loc);
}

Subpath DomItem::subReferencePath(Path path, Path referencedObject, const SourceLocation & loc) const
{
    if (domTypeIsOwningItem(internalKind()))
        return Subpath{path, DomItem(m_top, m_owner,
                                     Reference(referencedObject, path, loc))};
    else
        return Subpath{path, DomItem(m_top, m_owner,
                                     Reference(referencedObject, pathFromOwner().path(path), loc))};
}

Subpath DomItem::toSubField(QStringView fieldName) const
{
    return Subpath{Path::Field(fieldName), *this};
}

Subpath DomItem::toSubField(QString fieldName) const
{
    return Subpath{Path::Field(fieldName), *this};
}

Subpath DomItem::toSubKey(QStringView keyName) const
{
    return Subpath{Path::Key(keyName), *this};
}

Subpath DomItem::toSubKey(QString keyName) const
{
    return Subpath{Path::Key(keyName), *this};
}

Subpath DomItem::toSubIndex(index_type i) const
{
    return Subpath{Path::Index(i), *this};
}

Subpath DomItem::toSubPath(Path subPath) const
{
    return Subpath{subPath, *this};
}

Subpath DomItem::subList(const List &list) const
{
    return Subpath{list.pathFromOwner().last(), DomItem(m_top, m_owner, list)};
}

Subpath DomItem::subMap(const Map &map) const
{
    return Subpath{map.pathFromOwner().last(), DomItem(m_top, m_owner, map)};
}

Subpath DomItem::subObjectWrap(const SimpleObjectWrap &obj) const
{
    return Subpath{obj.pathFromOwner().last(), DomItem(m_top, m_owner, obj)};
}

DomItem::DomItem():
    DomItem(shared_ptr<DomTop>(), shared_ptr<OwningItem>(), Path(), nullptr)
{
}

DomItem::DomItem(std::shared_ptr<DomEnvironment> envPtr):
    DomItem(envPtr, envPtr, Path(), envPtr.get())
{}

DomItem::DomItem(std::shared_ptr<DomUniverse> universePtr):
    DomItem(universePtr, universePtr, Path(), universePtr.get())
{}

DomItem::DomItem(std::shared_ptr<DomTop> top, std::shared_ptr<OwningItem> owner, Path ownerPath, DomBase *base):
    m_top(top), m_owner(owner), m_ownerPath(ownerPath), m_base(base)
{
    if (m_base == nullptr || m_base->kind() == DomType::Empty) { // avoid null ptr, and allow only a single kind of Empty
        m_top.reset();
        m_owner.reset();
        m_base = nullptr;
    }
}

DomItem::DomItem(shared_ptr<DomTop> top, shared_ptr<OwningItem> owner, Map map):
    m_top(top), m_owner(owner), m_base(nullptr),
    inlineEl(map)
{}

DomItem::DomItem(shared_ptr<DomTop> top, shared_ptr<OwningItem> owner, List list):
    m_top(top), m_owner(owner), m_base(nullptr),
    inlineEl(list)
{}

DomItem::DomItem(shared_ptr<DomTop> top, shared_ptr<OwningItem> owner, ConstantData data):
    m_top(top), m_owner(owner), m_base(nullptr),
    inlineEl(data)
{}

DomItem::DomItem(shared_ptr<DomTop> top, shared_ptr<OwningItem> owner, Reference reference):
    m_top(top), m_owner(owner), m_base(nullptr), inlineEl(reference)
{}

DomItem::DomItem(std::shared_ptr<DomTop> top, std::shared_ptr<OwningItem> owner, SimpleObjectWrap wrapper):
    m_top(top), m_owner(owner), m_base(nullptr), inlineEl(wrapper)
{}

Empty::Empty()
{}

Path Empty::pathFromOwner(const DomItem &) const
{
    return Path();
}

Path Empty::canonicalPath(const DomItem &) const
{
    return Path();
}

bool Empty::iterateDirectSubpaths(DomItem &, function_ref<bool (Path, DomItem &)>)
{
    return true;
}

DomItem Empty::containingObject(const DomItem &self) const
{
    return self;
}

void Empty::dump(const DomItem &, Sink s, int) const
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

bool Map::iterateDirectSubpaths(DomItem &self, function_ref<bool (Path, DomItem &)>visitor)
{
    for (QString k:keys(self)) {
        DomItem el = key(self, k);
        if (!visitor(Path::Key(k), el))
            return false;
    }
    return true;
}

const QSet<QString> Map::keys(const DomItem &self) const
{
    return m_keys(self);
}

DomItem Map::key(const DomItem &self, QString name) const
{
    return m_lookup(self, name);
}

void DomBase::dump(const DomItem &self, Sink sink, int indent) const
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
    iterateDirectSubpathsConst(self, [&comma, &idx, dK, sink, indent, self](Path p, const DomItem &i) {
        if (comma)
            sink(u",");
        else
            comma = true;
        switch (p.headKind()) {
        case Path::Kind::Field:
            sinkNewline(sink, indent + 2);
            if (dK != DomKind::Object)
                sink(u"UNEXPECTED ENTRY ERROR:");
            sinkEscaped(sink, p.headName());
            sink(u":");
            break;
        case Path::Kind::Key:
            sinkNewline(sink, indent + 2);
            if (dK != DomKind::Map)
                sink(u"UNEXPECTED ENTRY ERROR:");
            sinkEscaped(sink, p.headName());
            sink(u":");
            break;
        case Path::Kind::Index:
            sinkNewline(sink, indent + 2);
            if (dK != DomKind::List)
                sink(u"UNEXPECTED ENTRY ERROR:");
            else if (idx++ != p.headIndex())
                sink(u"OUT OF ORDER ARRAY:");
            break;
        default:
            sinkNewline(sink, indent + 2);
            sink(u"UNEXPECTED PATH KIND ERROR (ignored)");
            break;
        }
        DomItem cObj=i.container();
        if (cObj == self) {
            i.dump(sink, indent + 2);
        } else {
            sink(uR"({ "~type~": "Reference", "immediate": true, "referredObjectPath":")");
            i.canonicalPath().dump([sink](QStringView s){ sinkEscaped(sink, s, EscapeOptions::NoOuterQuotes); });
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

void List::dump(const DomItem &self, Sink sink, int indent) const
{
    bool first = true;
    sink(u"[");
    iterateDirectSubpathsConst(self, [indent, &first, sink](Path, const DomItem &item) {
        if (first)
            first = false;
        else
            sink(u",");
        sinkNewline(sink, indent + 2);
        item.dump(sink, indent + 2);
        return true;
    });
    sink(u"]");
}

bool List::iterateDirectSubpaths(DomItem &self, function_ref<bool (Path, DomItem &)>visitor)
{
    if (m_iterator) {
        return m_iterator(self, [visitor](index_type i, DomItem &item){
            return visitor(Path::Index(i), item);
        });
    }
    index_type len = indexes(self);
    for (index_type i = 0; i < len; ++i) {
        DomItem idx = index(self, i);
        if (!visitor(Path::Index(i), idx))
            return false;
    }
    return true;
}

index_type List::indexes(const DomItem &self) const
{
    return m_length(self);
}

DomItem List::index(const DomItem &self, index_type index) const
{
    return m_lookup(self, index);
}

DomElement::DomElement(Path pathFromOwner, const SourceLocation & loc):
    loc(loc), m_pathFromOwner(pathFromOwner)
{
}

Path DomElement::pathFromOwner(const DomItem &) const
{
    Q_ASSERT(m_pathFromOwner && "uninitialized DomElement");
    return m_pathFromOwner;
}

Path DomElement::canonicalPath(const DomItem &self) const
{
    Q_ASSERT(m_pathFromOwner && "uninitialized DomElement");
    return self.owner().canonicalPath().path(m_pathFromOwner);
}

DomItem DomElement::containingObject(const DomItem &self) const
{
    Q_ASSERT(m_pathFromOwner && "uninitialized DomElement");
    return DomBase::containingObject(self);
}

void DomElement::updatePathFromOwner(Path newPath)
{
    //if (!domTypeCanBeInline(kind()))
    m_pathFromOwner = newPath;
}

SourceLocation DomElement::location(const DomItem &) const
{
    return loc;
}

Reference::Reference(Path referredObject, Path pathFromOwner, const SourceLocation & loc):
    DomElement(pathFromOwner, loc), referredObjectPath(referredObject)
{
}

quintptr Reference::id() const
{
    return quintptr(0);
}


bool Reference::iterateDirectSubpaths(DomItem &self, function_ref<bool (Path, DomItem &)> visitor)
{
    if (!self.subDataField(Fields::referredObjectPath, referredObjectPath.toString()).visit(visitor))
        return false;
    DomItem res = get(self);
    if (!visitor(Path::Field(Fields::get), res))
        return false;
    return true;
}

DomItem Reference::field(const DomItem &self, QStringView name) const
{
    if (Fields::referredObjectPath == name)
        return self.subDataField(Fields::referredObjectPath, referredObjectPath.toString()).item;
    if (Fields::get == name)
        return get(self);
    return DomItem();
}

const QList<QString> Reference::fields(const DomItem &) const
{
    return QList<QString>({QString::fromUtf16(Fields::referredObjectPath), QString::fromUtf16(Fields::get)});
}

DomItem Reference::index(const DomItem &, index_type) const {
    return DomItem();
}

DomItem Reference::key(const DomItem &, QString) const {
    return DomItem();
}

DomItem Reference::get(const DomItem &self) const
{
    if (!referredObjectPath)
        return DomItem();
    return self[referredObjectPath];
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

OwningItem::OwningItem(int derivedFrom):
    m_derivedFrom(derivedFrom), m_revision(nextRevision()),
    m_createdAt(QDateTime::currentDateTime()), m_lastDataUpdateAt(m_createdAt), m_frozenAt(QDateTime::fromMSecsSinceEpoch(0))
{}

OwningItem::OwningItem(int derivedFrom, QDateTime lastDataUpdateAt):
    m_derivedFrom(derivedFrom), m_revision(nextRevision()),
    m_createdAt(QDateTime::currentDateTime()), m_lastDataUpdateAt(lastDataUpdateAt), m_frozenAt(QDateTime::fromMSecsSinceEpoch(0))
{}

OwningItem::OwningItem(const OwningItem &o):
    m_derivedFrom(o.revision()), m_revision(nextRevision()),
    m_createdAt(QDateTime::currentDateTime()),   m_lastDataUpdateAt(o.lastDataUpdateAt()), m_frozenAt(QDateTime::fromMSecsSinceEpoch(0))
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

bool OwningItem::iterateDirectSubpaths(DomItem &self, function_ref<bool (Path, DomItem &)> visitor)
{
    bool cont = true;
    QMultiMap<Path, ErrorMessage> myErrors = localErrors();
    cont = cont && self.subMap(
                Map(
                    self.pathFromOwner().field(Fields::errors),
                    [myErrors](const DomItem &map, QString key) {
        auto it = myErrors.find(Path::fromString(key));
        if (it != myErrors.end())
            return map.subDataKey(
                        key, it->toCbor(),
                        ConstantData::Options::FirstMapIsFields, it->location).item;
        else
            return DomItem();
    }, [myErrors](const DomItem &) {
        QSet<QString> res;
        auto it = myErrors.keyBegin();
        auto end = myErrors.keyEnd();
        while (it != end)
            res.insert(it++->toString());
        return res;
    }, QLatin1String("ErrorMessages"))).visit(visitor);
    return cont;
}

Path OwningItem::pathFromOwner(const DomItem &) const
{
    return Path();
}

DomItem OwningItem::containingObject(const DomItem &self) const
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
        m_frozenAt = QDateTime::currentDateTime();
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

void OwningItem::addError(const DomItem &, ErrorMessage msg)
{
    addErrorLocal(msg);
}

void OwningItem::addErrorLocal(ErrorMessage msg)
{
    QMutexLocker l(mutex());
    auto it = m_errors.constFind(msg.path);
    while (it != m_errors.constEnd() && it->path == msg.path) {
        if (*it++ == msg)
            return;
    }
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

bool OwningItem::iterateErrors(const DomItem &self, function_ref<bool (DomItem, ErrorMessage)> visitor,
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
        if (!visitor(self, *it))
            return false;
    }
    return true;
}

bool OwningItem::iterateSubOwners(const DomItem &self, function_ref<bool (const DomItem &)>visitor)
{
    return iterateDirectSubpathsConst(self,[self, visitor](Path, const DomItem &i) {
        if (i.owningItemPtr() != self.owningItemPtr() && i.container().id() == self.id())
            return visitor(i);
        return true;
    });
}

GenericObject GenericObject::copy() const
{
    QMap<QString, GenericObject> newObjs;
    auto objs = subObjects;
    auto itO = objs.cbegin();
    auto endO = objs.cend();
    while (itO != endO) {
        newObjs.insert(itO.key(),itO->copy());
        ++itO;
    }
    return GenericObject(pathFromOwner(), loc, newObjs, subValues);
}

std::pair<QString, GenericObject> GenericObject::asStringPair() const
{
    return std::make_pair(pathFromOwner().last().headName(), *this);
}

bool GenericObject::iterateDirectSubpaths(DomItem &self, function_ref<bool (Path, DomItem &)> visitor)
{
    bool cont = true;
    auto itV = subValues.begin();
    auto endV = subValues.end();
    while (itV != endV) {
        cont = cont && self.subDataField(itV.key(), *itV).visit(visitor);
        ++itV;
    }
    auto itO = subObjects.begin();
    auto endO = subObjects.end();
    while (itO != endO) {
        cont = cont && self.copy(&(*itO)).toSubField(itO.key()).visit(visitor);
        ++itO;
    }
    return cont;
}

std::shared_ptr<OwningItem> GenericOwner::doCopy(const DomItem &) const
{
    return std::make_shared<GenericOwner>(*this);
}

GenericOwner::GenericOwner(const GenericOwner &o):
    OwningItem(o), pathFromTop(o.pathFromTop),
    subValues(o.subValues)
{
    auto objs = o.subObjects;
    auto itO = objs.cbegin();
    auto endO = objs.cend();
    while (itO != endO) {
        subObjects.insert(itO.key(),itO->copy());
        ++itO;
    }
}

std::shared_ptr<GenericOwner> GenericOwner::makeCopy(const DomItem &self)
{
    return std::static_pointer_cast<GenericOwner>(doCopy(self));
}

Path GenericOwner::canonicalPath(const DomItem &) const
{
    return pathFromTop;
}

bool GenericOwner::iterateDirectSubpaths(DomItem &self, function_ref<bool (Path, DomItem &)> visitor)
{
    bool cont = true;
    auto itV = subValues.begin();
    auto endV = subValues.end();
    while (itV != endV) {
        cont = cont && self.subDataField(itV.key(), *itV).visit(visitor);
        ++itV;
    }
    auto itO = subObjects.begin();
    auto endO = subObjects.end();
    while (itO != endO) {
        cont = cont && self.copy(&(*itO)).toSubField(itO.key()).visit(visitor);
        ++itO;
    }
    return cont;
}

bool operator ==(const DomItem &o1, const DomItem &o2) {
    if (o1.base() == o2.base())
        return true;
    quintptr i1 = o1.id();
    quintptr i2 = o2.id();
    if (i1 != i2)
        return false;
    if (i1 != quintptr(0))
        return true;
    Path p1 = o1.pathFromOwner();
    Path p2 = o2.pathFromOwner();
    if (p1 != p2)
        return false;
    return o1.owningItemPtr() == o2.owningItemPtr();
}

QString MutableDomItem::name() const
{
    return base().name();
}

ErrorHandler MutableDomItem::errorHandler() const
{
    MutableDomItem self;
    return [self](ErrorMessage m){
               self.addError(m);
    };
}

QDebug operator<<(QDebug debug, const DomItem &c)
{
    dumperToQDebug([&c](Sink s) {
        c.dump(s);
    }, debug);
    return debug;
}

QDebug operator<<(QDebug debug, const MutableDomItem &c)
{
    return debug.noquote().nospace() << "MutableDomItem(" << domTypeToString(c.internalKind())
             << ", " << c.canonicalPath().toString() << ")";
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
