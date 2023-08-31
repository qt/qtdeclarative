// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Suppress GCC 11 warning about maybe-uninitialized copy of
// another Data. We're not sure if the compiler is actually right,
// but in this type of warning, it often isn't.
//#if defined(Q_CC_GNU) && Q_CC_GNU >= 1100
//QT_WARNING_DISABLE_GCC("-Wmaybe-uninitialized")
#include "qqmldompath_p.h"
#if defined(__GNUC__) && __GNUC__ >= 11
#  pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include "qqmldomelements_p.h"
#include "qqmldomcomments_p.h"
#include "qqmldomastdumper_p.h"
#include "qqmldommock_p.h"
#include "qqmldomreformatter_p.h"
#include "qqmldomoutwriter_p.h"
#include "qqmldomlinewriter_p.h"
#include "qqmldomtop_p.h"
#include "qqmldomexternalitems_p.h"

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>

#include <QtCore/QScopeGuard>
#include <QtCore/QRegularExpression>
#include <QtCore/QDir>
#include <QtCore/QBasicMutex>
#include <QtCore/QUrl>

#include <optional>
#include <limits>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QQmlJS {
namespace Dom {

namespace Paths {

Path moduleIndexPath(QString uri, int majorVersion, ErrorHandler errorHandler)
{
    QString version = QString::number(majorVersion);
    if (majorVersion == Version::Latest)
        version = QLatin1String("Latest");
    else if (majorVersion == Version::Undefined)
        version = QString();
    QRegularExpression moduleRe(QLatin1String(R"(\A\w+(?:\.\w+)*\Z)"));
    auto m = moduleRe.match(uri);
    if (!m.isValid())
        Path::myErrors()
                .error(Path::tr("Invalid module name in import %1").arg(uri))
                .handle(errorHandler);
    return Path::Root(PathRoot::Env).field(Fields::moduleIndexWithUri).key(uri).key(version);
}

Path moduleScopePath(QString uri, Version version, ErrorHandler)
{
    return Path::Root(PathRoot::Env)
            .field(Fields::moduleIndexWithUri)
            .key(uri)
            .key(version.majorSymbolicString())
            .field(Fields::moduleScope)
            .key(version.minorString());
}

Path moduleScopePath(QString uri, QString version, ErrorHandler errorHandler)
{
    Version v = Version::fromString(version);
    if (!version.isEmpty() && !(v.isValid() || v.isLatest()))
        Path::myErrors().error(Path::tr("Invalid Version %1").arg(version)).handle(errorHandler);
    return moduleScopePath(uri, v, errorHandler);
}

} // end namespace Paths

static ErrorGroups domParsingErrors()
{
    static ErrorGroups res = { { DomItem::domErrorGroup, NewErrorGroup("Parsing") } };
    return res;
}

bool CommentableDomElement::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvWrapField(visitor, Fields::comments, m_comments);
    return cont;
}

void Component::updatePathFromOwner(Path newPath)
{
    DomElement::updatePathFromOwner(newPath);
    updatePathFromOwnerMultiMap(m_enumerations, newPath.field(Fields::enumerations));
    updatePathFromOwnerQList(m_objects, newPath.field(Fields::objects));
}

Component::Component(QString name) : CommentableDomElement(Path()), m_name(name) { }

Component::Component(Path pathFromOwner) : CommentableDomElement(pathFromOwner) { }

bool Component::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = CommentableDomElement::iterateDirectSubpaths(self, visitor);
    cont = cont && self.dvValueField(visitor, Fields::name, name());
    cont = cont && self.dvWrapField(visitor, Fields::enumerations, m_enumerations);
    cont = cont && self.dvWrapField(visitor, Fields::objects, m_objects);
    cont = cont && self.dvValueField(visitor, Fields::isSingleton, isSingleton());
    cont = cont && self.dvValueField(visitor, Fields::isCreatable, isCreatable());
    cont = cont && self.dvValueField(visitor, Fields::isComposite, isComposite());
    cont = cont && self.dvValueField(visitor, Fields::attachedTypeName, attachedTypeName());
    cont = cont && self.dvReferenceField(visitor, Fields::attachedType, attachedTypePath(self));
    return cont;
}

DomItem Component::field(DomItem &self, QStringView name)
{
    switch (name.size()) {
    case 4:
        if (name == Fields::name)
            return self.wrapField(Fields::name, m_name);
        break;
    case 7:
        if (name == Fields::objects)
            return self.wrapField(Fields::objects, m_objects);
        break;
    default:
        break;
    }
    return DomBase::field(self, name);
}

Path Component::addObject(const QmlObject &object, QmlObject **oPtr)
{
    return appendUpdatableElementInQList(pathFromOwner().field(Fields::objects), m_objects, object,
                                         oPtr);
}

bool QmlComponent::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = Component::iterateDirectSubpaths(self, visitor);
    cont = cont && self.dvWrapField(visitor, Fields::ids, m_ids);
    cont = cont && self.dvValueLazyField(visitor, Fields::subComponents, [this, &self]() {
        return this->subComponents(self);
    });
    return cont;
}

void QmlComponent::updatePathFromOwner(Path newPath)
{
    Component::updatePathFromOwner(newPath);
    updatePathFromOwnerMultiMap(m_ids, newPath.field(Fields::annotations));
}

void QmlComponent::writeOut(DomItem &self, OutWriter &lw) const
{
    if (name().contains(QLatin1Char('.'))) {
        // inline component
        lw.ensureNewline()
                .writeRegion(u"component")
                .space()
                .writeRegion(u"componentName", name().split(QLatin1Char('.')).last())
                .writeRegion(u"colon", u":")
                .space();
    }
    self.field(Fields::objects).index(0).writeOut(lw);
}

QList<QString> QmlComponent::subComponentsNames(DomItem &self) const
{
    DomItem components = self.owner().field(Fields::components);
    QSet<QString> cNames = components.keys();
    QString myNameDot = self.pathFromOwner()[1].headName();
    if (!myNameDot.isEmpty())
        myNameDot += QLatin1Char('.');
    QList<QString> subNames;
    for (QString cName : cNames)
        if (cName.startsWith(myNameDot)
            && !QStringView(cName).mid(myNameDot.size()).contains(QLatin1Char('.'))
            && !cName.isEmpty())
            subNames.append(cName);
    std::sort(subNames.begin(), subNames.end());
    return subNames;
}

QList<DomItem> QmlComponent::subComponents(DomItem &self) const
{
    DomItem components = self.owner().field(Fields::components);
    QList<DomItem> res;
    for (QString cName : subComponentsNames(self))
        for (DomItem comp : components.key(cName).values())
            res.append(comp);
    return res;
}

Version Version::fromString(QStringView v)
{
    if (v.isEmpty())
        return Version(Latest, Latest);
    QRegularExpression r(
            QRegularExpression::anchoredPattern(QStringLiteral(uR"(([0-9]*)(?:\.([0-9]*))?)")));
    auto m = r.matchView(v);
    if (m.hasMatch()) {
        bool ok;
        int majorV = m.capturedView(1).toInt(&ok);
        if (!ok)
            majorV = Version::Undefined;
        int minorV = m.capturedView(2).toInt(&ok);
        if (!ok)
            minorV = Version::Undefined;
        return Version(majorV, minorV);
    }
    return {};
}

Version::Version(qint32 majorV, qint32 minorV) : majorVersion(majorV), minorVersion(minorV) { }

bool Version::isLatest() const
{
    return majorVersion == Latest && minorVersion == Latest;
}

bool Version::isValid() const
{
    return majorVersion >= 0 && minorVersion >= 0;
}

QString Version::stringValue() const
{
    if (isLatest())
        return QString();
    if (minorVersion < 0) {
        if (majorVersion < 0)
            return QLatin1String(".");
        else
            return QString::number(majorVersion);
    }
    if (majorVersion < 0)
        return QLatin1String(".") + QString::number(minorVersion);
    return QString::number(majorVersion) + QChar::fromLatin1('.') + QString::number(minorVersion);
}

bool Version::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvWrapField(visitor, Fields::majorVersion, majorVersion);
    cont = cont && self.dvWrapField(visitor, Fields::minorVersion, minorVersion);
    cont = cont && self.dvValueField(visitor, Fields::isLatest, isLatest());
    cont = cont && self.dvValueField(visitor, Fields::isValid, isValid());
    cont = cont && self.dvValueLazyField(visitor, Fields::stringValue, [this]() {
        return this->stringValue();
    });
    return cont;
}

QRegularExpression Import::importRe()
{
    static QRegularExpression res(QRegularExpression::anchoredPattern(QStringLiteral(
            uR"((?<uri>\w+(?:\.\w+)*)(?:\W+(?<version>[0-9]+(?:\.[0-9]*)?))?(?:\W+as\W+(?<id>\w+))?$)")));
    return res;
}

Import Import::fromUriString(QString importStr, Version v, QString importId, ErrorHandler handler)
{
    auto m = importRe().match(importStr);
    if (m.hasMatch()) {
        if (v.majorVersion == Version::Undefined && v.minorVersion == Version::Undefined)
            v = Version::fromString(m.captured(2));
        else if (!m.captured(u"version").isEmpty())
            domParsingErrors()
                    .warning(tr("Version %1 in import string '%2' overridden by explicit "
                                "version %3")
                                     .arg(m.captured(2), importStr, v.stringValue()))
                    .handle(handler);
        if (importId.isEmpty())
            importId = m.captured(u"importId");
        else if (!m.captured(u"importId").isEmpty())
            domParsingErrors()
                    .warning(tr("namespace %1 in import string '%2' overridden by explicit "
                                "importId %3")
                                     .arg(m.captured(u"importId"), importStr, importId))
                    .handle(handler);
        return Import(QmlUri::fromUriString(m.captured(u"uri").trimmed()), v, importId);
    }
    domParsingErrors()
            .error(tr("Unexpected URI format in import '%1'").arg(importStr))
            .handle(handler);
    return Import();
}

Import Import::fromFileString(QString importStr, QString importId, ErrorHandler)
{
    return Import(QmlUri::fromDirectoryString(importStr), Version(), importId);
}

bool Import::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvValueField(visitor, Fields::uri, uri.toString());
    cont = cont && self.dvWrapField(visitor, Fields::version, version);
    if (!importId.isEmpty())
        cont = cont && self.dvValueField(visitor, Fields::importId, importId);
    if (implicit)
        cont = cont && self.dvValueField(visitor, Fields::implicit, implicit);
    cont = cont && self.dvWrapField(visitor, Fields::comments, comments);
    return cont;
}

void Import::writeOut(DomItem &, OutWriter &ow) const
{
    if (implicit)
        return;
    ow.ensureNewline();
    ow.writeRegion(u"import").space();
    ow.writeRegion(u"uri", uri.toString());
    if (uri.isModule()) {
        QString vString = version.stringValue();
        if (!vString.isEmpty())
            ow.space().write(vString);
    }
    if (!importId.isEmpty())
        ow.space().writeRegion(u"as").space().writeRegion(u"id", importId);
}

Id::Id(QString idName, Path referredObject) : name(idName), referredObjectPath(referredObject) { }

bool Id::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvValueField(visitor, Fields::name, name);
    cont = cont && self.dvReferenceField(visitor, Fields::referredObject, referredObjectPath);
    cont = cont && self.dvWrapField(visitor, Fields::comments, comments);
    cont = cont && self.dvWrapField(visitor, Fields::annotations, annotations);
    cont = cont && self.dvWrapField(visitor, Fields::value, value);
    return cont;
}

void Id::updatePathFromOwner(Path newPath)
{
    updatePathFromOwnerQList(annotations, newPath.field(Fields::annotations));
}

Path Id::addAnnotation(Path selfPathFromOwner, const QmlObject &annotation, QmlObject **aPtr)
{
    return appendUpdatableElementInQList(selfPathFromOwner.field(Fields::annotations), annotations,
                                         annotation, aPtr);
}

QmlObject::QmlObject(Path pathFromOwner) : CommentableDomElement(pathFromOwner) { }

bool QmlObject::iterateBaseDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = CommentableDomElement::iterateDirectSubpaths(self, visitor);
    if (!idStr().isEmpty())
        cont = cont && self.dvValueField(visitor, Fields::idStr, idStr());
    cont = cont && self.dvValueField(visitor, Fields::name, name());
    if (!prototypePaths().isEmpty())
        cont = cont && self.dvReferencesField(visitor, Fields::prototypes, m_prototypePaths);
    if (nextScopePath())
        cont = cont && self.dvReferenceField(visitor, Fields::nextScope, nextScopePath());
    cont = cont && self.dvWrapField(visitor, Fields::propertyDefs, m_propertyDefs);
    cont = cont && self.dvWrapField(visitor, Fields::bindings, m_bindings);
    cont = cont && self.dvWrapField(visitor, Fields::methods, m_methods);
    cont = cont && self.dvWrapField(visitor, Fields::children, m_children);
    cont = cont && self.dvWrapField(visitor, Fields::annotations, m_annotations);
    cont = cont && self.dvItemField(visitor, Fields::propertyInfos, [this, &self]() {
        return self.subMapItem(Map(
                pathFromOwner().field(Fields::propertyInfos),
                [&self](DomItem &map, QString k) {
                    auto pInfo = self.propertyInfoWithName(k);
                    return map.wrap(PathEls::Key(k), pInfo);
                },
                [&self](DomItem &) { return self.propertyInfoNames(); },
                QLatin1String("PropertyInfo")));
    });
    return cont;
}

QList<QString> QmlObject::fields() const
{
    static QList<QString> myFields(
            { QString::fromUtf16(Fields::comments), QString::fromUtf16(Fields::idStr),
              QString::fromUtf16(Fields::name), QString::fromUtf16(Fields::prototypes),
              QString::fromUtf16(Fields::nextScope), QString::fromUtf16(Fields::propertyDefs),
              QString::fromUtf16(Fields::bindings), QString::fromUtf16(Fields::methods),
              QString::fromUtf16(Fields::children), QString::fromUtf16(Fields::annotations),
              QString::fromUtf16(Fields::propertyInfos) });
    return myFields;
}

bool QmlObject::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = iterateBaseDirectSubpaths(self, visitor);
    cont = cont && self.dvValueLazyField(visitor, Fields::defaultPropertyName, [this, &self]() {
        return defaultPropertyName(self);
    });
    return cont;
}

DomItem QmlObject::field(DomItem &self, QStringView name)
{
    switch (name.size()) {
    case 4:
        if (name == Fields::name)
            return self.subDataItem(PathEls::Field(Fields::name), this->name());
        break;
    case 5:
        if (name == Fields::idStr) {
            if (idStr().isEmpty())
                return DomItem();
            return self.subDataItem(PathEls::Field(Fields::idStr), idStr());
        }
        break;
    case 7:
        if (name == Fields::methods)
            return self.wrapField(Fields::methods, m_methods);
        break;
    case 8:
        switch (name.at(1).unicode()) {
        case u'i':
            if (name == Fields::bindings)
                return self.wrapField(Fields::bindings, m_bindings);
            break;
        case u'o':
            if (name == Fields::comments)
                return CommentableDomElement::field(self, name);
            break;
        case u'h':
            if (name == Fields::children)
                return self.wrapField(Fields::children, m_children);
            break;
        default:
            break;
        }
        break;
    case 9:
        if (name == Fields::nextScope) {
            if (nextScopePath())
                return self.subReferenceItem(PathEls::Field(Fields::nextScope), nextScopePath());
            else
                return DomItem();
        }
        break;
    case 10:
        if (name == Fields::prototypes) {
            if (prototypePaths().isEmpty())
                return DomItem();
            return self.subReferencesItem(PathEls::Field(Fields::prototypes), m_prototypePaths);
        }
        break;
    case 11:
        if (name == Fields::annotations)
            return self.wrapField(Fields::annotations, m_annotations);
        break;
    case 12:
        return self.wrapField(Fields::propertyDefs, m_propertyDefs);
        break;
    case 13:
        if (name == Fields::propertyInfos)
            return self.subMapItem(Map(
                    pathFromOwner().field(Fields::propertyInfos),
                    [self](DomItem &map, QString k) mutable {
                        auto pInfo = self.propertyInfoWithName(k);
                        return map.wrap(PathEls::Key(k), pInfo);
                    },
                    [self](DomItem &) mutable { return self.propertyInfoNames(); },
                    QLatin1String("PropertyInfo")));
        break;
    case 19:
        if (name == Fields::defaultPropertyName)
            return self.subDataItem(PathEls::Field(Fields::defaultPropertyName),
                                    defaultPropertyName(self));
        break;
    default:
        break;
    }
    static QStringList knownLookups({ QString::fromUtf16(Fields::fileLocationsTree) });
    if (!knownLookups.contains(name))
        qCWarning(domLog()) << "Asked non existing field " << name << " in QmlObject "
                            << pathFromOwner();
    return DomItem();
}

void QmlObject::updatePathFromOwner(Path newPath)
{
    DomElement::updatePathFromOwner(newPath);
    updatePathFromOwnerMultiMap(m_propertyDefs, newPath.field(Fields::propertyDefs));
    updatePathFromOwnerMultiMap(m_bindings, newPath.field(Fields::bindings));
    updatePathFromOwnerMultiMap(m_methods, newPath.field(Fields::methods));
    updatePathFromOwnerQList(m_children, newPath.field(Fields::children));
    updatePathFromOwnerQList(m_annotations, newPath.field(Fields::annotations));
}

QString QmlObject::localDefaultPropertyName() const
{
    if (!m_defaultPropertyName.isEmpty())
        return m_defaultPropertyName;
    for (const PropertyDefinition &pDef : m_propertyDefs)
        if (pDef.isDefaultMember)
            return pDef.name;
    return QString();
}

QString QmlObject::defaultPropertyName(DomItem &self) const
{
    QString dProp = localDefaultPropertyName();
    if (!dProp.isEmpty())
        return dProp;
    QString res = QStringLiteral(u"data");
    self.visitPrototypeChain(
            [&res](DomItem &obj) {
                if (const QmlObject *objPtr = obj.as<QmlObject>()) {
                    QString dProp = objPtr->localDefaultPropertyName();
                    if (!dProp.isEmpty()) {
                        res = dProp;
                        return false;
                    }
                }
                return true;
            },
            VisitPrototypesOption::SkipFirst);
    return res;
}

bool QmlObject::iterateSubOwners(DomItem &self, function_ref<bool(DomItem &)> visitor) const
{
    bool cont = self.field(Fields::bindings).visitKeys([visitor](QString, DomItem &bs) {
        return bs.visitIndexes([visitor](DomItem &b) {
            DomItem v = b.field(Fields::value);
            if (std::shared_ptr<ScriptExpression> vPtr = v.ownerAs<ScriptExpression>()) {
                if (!visitor(v))
                    return false;
                return v.iterateSubOwners(visitor); // currently not needed, avoid?
            }
            return true;
        });
    });
    cont = cont && self.field(Fields::children).visitIndexes([visitor](DomItem &qmlObj) {
        if (const QmlObject *qmlObjPtr = qmlObj.as<QmlObject>()) {
            return qmlObjPtr->iterateSubOwners(qmlObj, visitor);
        }
        Q_ASSERT(false);
        return true;
    });
    return cont;
}

static QStringList dotExpressionToList(std::shared_ptr<ScriptExpression> expr)
{
    QStringList res;
    AST::Node *node = (expr ? expr->ast() : nullptr);
    while (node) {
        switch (node->kind) {
        case AST::Node::Kind_IdentifierExpression: {
            AST::IdentifierExpression *id = AST::cast<AST::IdentifierExpression *>(node);
            res.prepend(id->name.toString());
            return res;
        }
        case AST::Node::Kind_FieldMemberExpression: {
            AST::FieldMemberExpression *id = AST::cast<AST::FieldMemberExpression *>(node);
            res.prepend(id->name.toString());
            node = id->base;
            break;
        }
        default:
            qCDebug(writeOutLog).noquote() << "Could not convert dot expression to list for:\n"
                                           << expr->astRelocatableDump();
            return QStringList();
        }
    }
    return res;
}

LocallyResolvedAlias QmlObject::resolveAlias(DomItem &self,
                                             std::shared_ptr<ScriptExpression> accessSequence) const
{
    QStringList accessSequenceList = dotExpressionToList(accessSequence);
    return resolveAlias(self, accessSequenceList);
}

LocallyResolvedAlias QmlObject::resolveAlias(DomItem &self, const QStringList &accessSequence) const
{
    LocallyResolvedAlias res;
    QSet<QString> visitedAlias;
    if (accessSequence.isEmpty()) {
        return res;
    } else if (accessSequence.size() > 3) {
        res.status = LocallyResolvedAlias::Status::TooDeep;
        return res;
    }
    QString idName = accessSequence.first();
    DomItem idTarget = self.component()
                               .field(Fields::ids)
                               .key(idName)
                               .index(0)
                               .field(Fields::referredObject)
                               .get();
    if (!idTarget)
        return res;
    res.baseObject = idTarget;
    res.accessedPath = accessSequence.mid(1);
    res.typeName = idTarget.name();
    res.status = LocallyResolvedAlias::Status::ResolvedObject;
    // check if it refers to locally defined props/objs
    while (!res.accessedPath.isEmpty()) {
        QString pNow = res.accessedPath.first();
        DomItem defNow = res.baseObject.propertyDefs().key(pNow).index(0);
        if (const PropertyDefinition *defNowPtr = defNow.as<PropertyDefinition>()) {
            if (defNowPtr->isAlias()) {
                res.typeName = QString();
                ++res.nAliases;
                QString aliasPath = defNow.canonicalPath().toString();
                if (visitedAlias.contains(aliasPath)) {
                    res.status = LocallyResolvedAlias::Status::Loop;
                    return res;
                }
                visitedAlias.insert(aliasPath);
                DomItem valNow = res.baseObject.bindings().key(pNow).index(0);
                if (std::shared_ptr<ScriptExpression> exp =
                            valNow.field(Fields::value).ownerAs<ScriptExpression>()) {
                    QStringList expList = dotExpressionToList(exp);
                    if (expList.isEmpty()) {
                        res.status = LocallyResolvedAlias::Status::Invalid;
                        return res;
                    } else if (expList.size() > 3) {
                        res.status = LocallyResolvedAlias::Status::TooDeep;
                        return res;
                    }
                    idName = expList.first();
                    idTarget = self.component()
                                       .field(Fields::ids)
                                       .key(idName)
                                       .index(0)
                                       .field(Fields::referredObject)
                                       .get();
                    res.baseObject = idTarget;
                    res.accessedPath = expList.mid(1) + res.accessedPath.mid(1);
                    if (!idTarget) {
                        res.status = LocallyResolvedAlias::Status::Invalid;
                        return res;
                    }
                    res.status = LocallyResolvedAlias::Status::ResolvedObject;
                    res.typeName = idTarget.name();
                } else {
                    res.status = LocallyResolvedAlias::Status::Invalid;
                    return res;
                }
            } else {
                res.localPropertyDef = defNow;
                res.typeName = defNowPtr->typeName;
                res.accessedPath = res.accessedPath.mid(1);
                DomItem valNow = res.baseObject.bindings().key(pNow).index(0).field(Fields::value);
                if (valNow.internalKind() == DomType::QmlObject) {
                    res.baseObject = valNow;
                    res.typeName = valNow.name();
                    res.status = LocallyResolvedAlias::Status::ResolvedObject;
                } else {
                    res.status = LocallyResolvedAlias::Status::ResolvedProperty;
                    return res;
                }
            }
        } else {
            return res;
        }
    }
    return res;
}

MutableDomItem QmlObject::addPropertyDef(MutableDomItem &self, PropertyDefinition propertyDef,
                                         AddOption option)
{
    Path p = addPropertyDef(propertyDef, option);
    if (p.last().headIndex(0) > 1)
        self.owningItemPtr()->addErrorLocal(domParsingErrors().error(
                tr("Repeated PropertyDefinition with name %1").arg(propertyDef.name)));
    return self.owner().path(p);
}

MutableDomItem QmlObject::addBinding(MutableDomItem &self, Binding binding, AddOption option)
{
    Path p = addBinding(binding, option);
    if (p && p.last().headIndex(0) > 1)
        self.owningItemPtr()->addErrorLocal(
                domParsingErrors().error(tr("Repeated binding with name %1").arg(binding.name())));
    return self.owner().path(p);
}

MutableDomItem QmlObject::addMethod(MutableDomItem &self, MethodInfo functionDef, AddOption option)
{
    Path p = addMethod(functionDef, option);
    if (p.last().headIndex(0) > 1)
        self.owningItemPtr()->addErrorLocal(
                domParsingErrors().error(tr("Repeated Method with name %1").arg(functionDef.name)));
    return self.owner().path(p);
}

void QmlObject::writeOut(DomItem &self, OutWriter &ow, QString onTarget) const
{
    const quint32 posOfNewElements = std::numeric_limits<quint32>::max();
    bool isRootObject = pathFromOwner().length() == 5
            && pathFromOwner()[0] == Path::Field(Fields::components)
            && pathFromOwner()[3] == Path::Field(Fields::objects);
    QString code;
    DomItem owner = self.owner();
    if (std::shared_ptr<QmlFile> qmlFilePtr = self.ownerAs<QmlFile>())
        code = qmlFilePtr->code();
    ow.writeRegion(u"name", name());
    if (!onTarget.isEmpty())
        ow.space().writeRegion(u"on", u"on").space().writeRegion(u"onTarget", onTarget);
    ow.writeRegion(u"leftBrace", u" {").newline();
    int baseIndent = ow.increaseIndent();
    int spacerId = 0;
    if (!idStr().isEmpty()) { // *always* put id first
        DomItem myId = self.component().field(Fields::ids).key(idStr()).index(0);
        if (myId)
            myId.writeOutPre(ow);
        ow.ensureNewline()
                .writeRegion(u"idToken", u"id")
                .writeRegion(u"idColon", u":")
                .space()
                .writeRegion(u"id", idStr());
        if (ow.lineWriter.options().attributesSequence
            == LineWriterOptions::AttributesSequence::Normalize) {
            ow.ensureNewline(2);
        }
        if (myId)
            myId.writeOutPost(ow);
    }
    quint32 counter = ow.counter();
    DomItem component;
    if (isRootObject)
        component = self.containingObject();
    auto startLoc = [&](FileLocations::Tree l) {
        if (l)
            return l->info().fullRegion;
        return SourceLocation(posOfNewElements, 0, 0, 0);
    };
    if (ow.lineWriter.options().attributesSequence
        == LineWriterOptions::AttributesSequence::Preserve) {
        QList<QPair<SourceLocation, DomItem>> attribs;
        AttachedInfoLookupResult<FileLocations::Tree> objLoc =
                FileLocations::findAttachedInfo(self);
        FileLocations::Tree componentLoc;
        if (isRootObject && objLoc.foundTree)
            componentLoc = objLoc.foundTree->parent()->parent();
        auto addMMap = [&attribs, &startLoc](DomItem &base, FileLocations::Tree baseLoc) {
            if (!base)
                return;
            for (auto els : base.values()) {
                FileLocations::Tree elsLoc =
                        FileLocations::find(baseLoc, els.pathFromOwner().last());
                for (auto el : els.values()) {
                    FileLocations::Tree elLoc =
                            FileLocations::find(elsLoc, el.pathFromOwner().last());
                    attribs.append(std::make_pair(startLoc(elLoc), el));
                }
            }
        };
        auto addMyMMap = [this, &objLoc, &self, &addMMap](QStringView fieldName) {
            DomItem base = this->field(self, fieldName);
            addMMap(base, FileLocations::find(objLoc.foundTree, base.pathFromOwner().last()));
        };
        auto addSingleLevel = [&attribs, &startLoc](DomItem &base, FileLocations::Tree baseLoc) {
            if (!base)
                return;
            for (auto el : base.values()) {
                FileLocations::Tree elLoc = FileLocations::find(baseLoc, el.pathFromOwner().last());
                attribs.append(std::make_pair(startLoc(elLoc), el));
            }
        };
        if (isRootObject) {
            DomItem enums = component.field(Fields::enumerations);
            addMMap(enums, FileLocations::find(componentLoc, enums.pathFromOwner().last()));
        }
        addMyMMap(Fields::propertyDefs);
        addMyMMap(Fields::bindings);
        addMyMMap(Fields::methods);
        DomItem children = field(self, Fields::children);
        addSingleLevel(children,
                       FileLocations::find(objLoc.foundTree, children.pathFromOwner().last()));
        if (isRootObject) {
            DomItem subCs = component.field(Fields::subComponents);
            for (DomItem &c : subCs.values()) {
                AttachedInfoLookupResult<FileLocations::Tree> subLoc =
                        FileLocations::findAttachedInfo(c);
                Q_ASSERT(subLoc.foundTree);
                attribs.append(std::make_pair(startLoc(subLoc.foundTree), c));
            }
        }
        std::stable_sort(attribs.begin(), attribs.end(),
                         [](const std::pair<SourceLocation, DomItem> &el1,
                            const std::pair<SourceLocation, DomItem> &el2) {
                             if (el1.first.offset < el2.first.offset)
                                 return true;
                             if (el1.first.offset > el2.first.offset)
                                 return false;
                             int i = int(el1.second.internalKind())
                                     - int(el2.second.internalKind());
                             return i < 0;
                         });
        qsizetype iAttr = 0;
        while (iAttr != attribs.size()) {
            auto &el = attribs[iAttr++];
            // check for an empty line before the current element, and preserve it
            int preNewlines = 0;
            quint32 start = el.first.offset;
            if (start != posOfNewElements && size_t(code.size()) >= start) {
                while (start != 0) {
                    QChar c = code.at(--start);
                    if (c == u'\n') {
                        if (++preNewlines == 2)
                            break;
                    } else if (!c.isSpace())
                        break;
                }
            }
            if (preNewlines == 0)
                ++preNewlines;
            ow.ensureNewline(preNewlines);
            if (el.second.internalKind() == DomType::PropertyDefinition && iAttr != attribs.size()
                && el.first.offset != ~quint32(0)) {
                DomItem b;
                auto &bPair = attribs[iAttr];
                if (bPair.second.internalKind() == DomType::Binding
                    && bPair.first.begin() < el.first.end()
                    && bPair.second.name() == el.second.name()) {
                    b = bPair.second;
                    ++iAttr;
                    b.writeOutPre(ow);
                }
                el.second.writeOut(ow);
                if (b) {
                    ow.write(u": ");
                    if (const Binding *bPtr = b.as<Binding>())
                        bPtr->writeOutValue(b, ow);
                    else {
                        qWarning() << "Internal error casting binding to Binding in"
                                   << b.canonicalPath();
                        ow.writeRegion(u"leftBrace", u"{").writeRegion(u"rightBrace", u"}");
                    }
                    b.writeOutPost(ow);
                }
            } else {
                el.second.writeOut(ow);
            }
        }
        ow.decreaseIndent(1, baseIndent);
        ow.ensureNewline().write(u"}");

        return;
    }
    DomItem bindings = field(self, Fields::bindings);
    DomItem propertyDefs = field(self, Fields::propertyDefs);

    if (isRootObject) {
        for (auto enumDescs : component.field(Fields::enumerations).values()) {
            for (auto enumDesc : enumDescs.values()) {
                ow.ensureNewline(1);
                enumDesc.writeOut(ow);
                ow.ensureNewline(1);
            }
        }
    }
    if (counter != ow.counter() || !idStr().isEmpty())
        spacerId = ow.addNewlinesAutospacerCallback(2);
    QSet<QString> mergedDefBinding;
    for (const QString &defName : propertyDefs.sortedKeys()) {
        auto pDefs = propertyDefs.key(defName).values();
        for (auto pDef : pDefs) {
            const PropertyDefinition *pDefPtr = pDef.as<PropertyDefinition>();
            Q_ASSERT(pDefPtr);
            DomItem b;
            bool uniqueDeclarationWithThisName = pDefs.size() == 1;
            if (uniqueDeclarationWithThisName && !pDefPtr->isRequired)
                bindings.key(pDef.name()).visitIndexes([&b, pDefPtr](DomItem &el) {
                    const Binding *elPtr = el.as<Binding>();
                    if (elPtr && elPtr->bindingType() == BindingType::Normal) {
                        switch (elPtr->valueKind()) {
                        case BindingValueKind::ScriptExpression:
                            b = el;
                            break;
                        case BindingValueKind::Array:
                            if (!pDefPtr->isDefaultMember
                                && pDefPtr->isParametricType())
                                b = el;
                            break;
                        case BindingValueKind::Object:
                            if (!pDefPtr->isDefaultMember
                                && !pDefPtr->isParametricType())
                                b = el;
                            break;
                        case BindingValueKind::Empty:
                            break;
                        }
                        return false;
                    }
                    return true;
                });
            if (b) {
                mergedDefBinding.insert(defName);
                b.writeOutPre(ow);
            }
            pDef.writeOut(ow);
            if (b) {
                ow.write(u": ");
                if (const Binding *bPtr = b.as<Binding>())
                    bPtr->writeOutValue(b, ow);
                else {
                    qWarning() << "Internal error casting binding to Binding in"
                               << b.canonicalPath();
                    ow.writeRegion(u"leftBrace", u"{").writeRegion(u"rightBrace", u"}");
                }
                b.writeOutPost(ow);
            }
        }
    }
    ow.removeTextAddCallback(spacerId);
    QList<DomItem> signalList, methodList;
    for (auto ms : field(self, Fields::methods).values()) {
        for (auto m : ms.values()) {
            const MethodInfo *mPtr = m.as<MethodInfo>();
            if (mPtr && mPtr->methodType == MethodInfo::MethodType::Signal)
                signalList.append(m);
            else
                methodList.append(m);
        }
    }
    if (counter != ow.counter())
        spacerId = ow.addNewlinesAutospacerCallback(2);
    for (auto &sig : signalList) {
        ow.ensureNewline();
        sig.writeOut(ow);
        ow.ensureNewline();
    }
    ow.removeTextAddCallback(spacerId);
    if (counter != ow.counter())
        spacerId = ow.addNewlinesAutospacerCallback(2);
    bool first = true;
    for (auto &method : methodList) {
        if (!first && ow.lineWriter.options().functionsSpacing) {
            ow.newline();
        }
        ow.ensureNewline();
        first = false;
        method.writeOut(ow);
        ow.ensureNewline();
    }
    ow.removeTextAddCallback(spacerId);
    QList<DomItem> normalBindings, signalHandlers, delayedBindings;
    for (auto bName : bindings.sortedKeys()) {
        bool skipFirstNormal = mergedDefBinding.contains(bName);
        for (auto b : bindings.key(bName).values()) {
            const Binding *bPtr = b.as<Binding>();
            if (skipFirstNormal) {
                if (bPtr && bPtr->bindingType() == BindingType::Normal) {
                    skipFirstNormal = false;
                    continue;
                }
            }
            if (bPtr->valueKind() == BindingValueKind::Array
                || bPtr->valueKind() == BindingValueKind::Object)
                delayedBindings.append(b);
            else if (b.field(Fields::isSignalHandler).value().toBool(false))
                signalHandlers.append(b);
            else
                normalBindings.append(b);
        }
    }
    if (counter != ow.counter())
        spacerId = ow.addNewlinesAutospacerCallback(2);
    for (auto &b : normalBindings)
        b.writeOut(ow);
    ow.removeTextAddCallback(spacerId);
    if (counter != ow.counter())
        spacerId = ow.addNewlinesAutospacerCallback(2);
    for (auto &b : delayedBindings)
        b.writeOut(ow);
    ow.removeTextAddCallback(spacerId);
    if (counter != ow.counter())
        spacerId = ow.addNewlinesAutospacerCallback(2);
    for (auto &b : signalHandlers)
        b.writeOut(ow);
    ow.removeTextAddCallback(spacerId);
    if (counter != ow.counter())
        spacerId = ow.addNewlinesAutospacerCallback(2);
    first = true;
    for (auto c : field(self, Fields::children).values()) {
        if (!first && ow.lineWriter.options().objectsSpacing) {
            ow.newline().newline();
        }
        first = false;
        ow.ensureNewline();
        c.writeOut(ow);
    }
    ow.removeTextAddCallback(spacerId);
    if (isRootObject) {
        // we are a root object, possibly add components
        DomItem subComps = component.field(Fields::subComponents);
        if (counter != ow.counter())
            spacerId = ow.addNewlinesAutospacerCallback(2);
        for (auto subC : subComps.values()) {
            ow.ensureNewline();
            subC.writeOut(ow);
        }
        ow.removeTextAddCallback(spacerId);
    }
    ow.decreaseIndent(1, baseIndent);
    ow.ensureNewline().write(u"}");
}

Binding::Binding(QString name, std::unique_ptr<BindingValue> value, BindingType bindingType)
    : m_bindingType(bindingType), m_name(name), m_value(std::move(value))
{
}

Binding::Binding(QString name, std::shared_ptr<ScriptExpression> value, BindingType bindingType)
    : Binding(name, std::make_unique<BindingValue>(value), bindingType)
{
}

Binding::Binding(QString name, QString scriptCode, BindingType bindingType)
    : Binding(name,
              std::make_unique<BindingValue>(std::make_shared<ScriptExpression>(
                      scriptCode, ScriptExpression::ExpressionType::BindingExpression, 0,
                      Binding::preCodeForName(name), Binding::postCodeForName(name))),
              bindingType)
{
}

Binding::Binding(QString name, QmlObject value, BindingType bindingType)
    : Binding(name, std::make_unique<BindingValue>(value), bindingType)
{
}

Binding::Binding(QString name, QList<QmlObject> value, BindingType bindingType)
    : Binding(name, std::make_unique<BindingValue>(value), bindingType)
{
}

Binding::Binding(const Binding &o)
    : m_bindingType(o.m_bindingType),
      m_name(o.m_name),
      m_annotations(o.m_annotations),
      m_comments(o.m_comments)
{
    if (o.m_value) {
        m_value = std::make_unique<BindingValue>(*o.m_value);
    }
}

Binding::~Binding() { }

Binding &Binding::operator=(const Binding &o)
{
    m_name = o.m_name;
    m_bindingType = o.m_bindingType;
    m_annotations = o.m_annotations;
    m_comments = o.m_comments;
    if (o.m_value) {
        if (!m_value)
            m_value = std::make_unique<BindingValue>(*o.m_value);
        else
            *m_value = *o.m_value;
    } else {
        m_value.reset();
    }
    return *this;
}

bool Binding::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvValueField(visitor, Fields::name, m_name);
    cont = cont && self.dvValueField(visitor, Fields::isSignalHandler, isSignalHandler());
    if (!m_value)
        cont = cont && visitor(PathEls::Field(Fields::value), []() { return DomItem(); });
    else
        cont = cont && self.dvItemField(visitor, Fields::value, [this, &self]() {
            return m_value->value(self);
        });
    cont = cont && self.dvValueField(visitor, Fields::bindingType, int(m_bindingType));
    cont = cont && self.dvWrapField(visitor, Fields::comments, m_comments);
    cont = cont && self.dvValueLazyField(visitor, Fields::preCode, [this]() {
        return this->preCode();
    });
    cont = cont && self.dvValueLazyField(visitor, Fields::postCode, [this]() {
        return this->postCode();
    });
    cont = cont && self.dvWrapField(visitor, Fields::annotations, m_annotations);
    return cont;
}

DomItem Binding::valueItem(DomItem &self) const
{
    if (!m_value)
        return DomItem();
    return m_value->value(self);
}

BindingValueKind Binding::valueKind() const
{
    if (!m_value)
        return BindingValueKind::Empty;
    return m_value->kind;
}

QmlObject const *Binding::objectValue() const
{
    if (valueKind() == BindingValueKind::Object)
        return &(m_value->object);
    return nullptr;
}

QmlObject *Binding::objectValue()
{
    if (valueKind() == BindingValueKind::Object)
        return &(m_value->object);
    return nullptr;
}

QList<QmlObject> const *Binding::arrayValue() const
{
    if (valueKind() == BindingValueKind::Array)
        return &(m_value->array);
    return nullptr;
}

QList<QmlObject> *Binding::arrayValue()
{
    if (valueKind() == BindingValueKind::Array)
        return &(m_value->array);
    return nullptr;
}

std::shared_ptr<ScriptExpression> Binding::scriptExpressionValue() const
{
    if (valueKind() == BindingValueKind::ScriptExpression)
        return m_value->scriptExpression;
    return nullptr;
}

std::shared_ptr<ScriptExpression> Binding::scriptExpressionValue()
{
    if (valueKind() == BindingValueKind::ScriptExpression)
        return m_value->scriptExpression;
    return nullptr;
}

Path Binding::addAnnotation(Path selfPathFromOwner, const QmlObject &annotation, QmlObject **aPtr)
{
    return appendUpdatableElementInQList(selfPathFromOwner.field(Fields::annotations),
                                         m_annotations, annotation, aPtr);
}

void Binding::updatePathFromOwner(Path newPath)
{
    Path base = newPath.field(Fields::annotations);
    if (m_value)
        m_value->updatePathFromOwner(newPath.field(Fields::value));
    updatePathFromOwnerQList(m_annotations, newPath.field(Fields::annotations));
}

void Binding::writeOut(DomItem &self, OutWriter &lw) const
{
    lw.ensureNewline();
    if (m_bindingType == BindingType::Normal) {
        lw.writeRegion(u"name", name());
        lw.writeRegion(u"colon", u":").space();
        writeOutValue(self, lw);
    } else {
        DomItem v = valueItem(self);
        if (const QmlObject *vPtr = v.as<QmlObject>()) {
            v.writeOutPre(lw);
            vPtr->writeOut(v, lw, name());
            v.writeOutPost(lw);
        } else {
            qCWarning(writeOutLog()) << "On Binding requires an QmlObject Value, not "
                                     << v.internalKindStr() << " at " << self.canonicalPath();
        }
    }
}

void Binding::writeOutValue(DomItem &self, OutWriter &lw) const
{
    DomItem v = valueItem(self);
    switch (valueKind()) {
    case BindingValueKind::Empty:
        qCWarning(writeOutLog()) << "Writing of empty binding " << name();
        lw.write(u"{}");
        break;
    case BindingValueKind::Array:
        if (const List *vPtr = v.as<List>()) {
            v.writeOutPre(lw);
            vPtr->writeOut(v, lw, false);
            v.writeOutPost(lw);
        }
        break;
    case BindingValueKind::Object:
    case BindingValueKind::ScriptExpression:
        v.writeOut(lw);
        break;
    }
}

bool QmltypesComponent::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = Component::iterateDirectSubpaths(self, visitor);
    cont = cont && self.dvWrapField(visitor, Fields::exports, m_exports);
    cont = cont && self.dvValueField(visitor, Fields::metaRevisions, m_metaRevisions);
    if (!fileName().isEmpty())
        cont = cont && self.dvValueField(visitor, Fields::fileName, fileName()); // remove?
    cont = cont && self.dvValueField(visitor, Fields::interfaceNames, m_interfaceNames);
    cont = cont && self.dvValueField(visitor, Fields::hasCustomParser, m_hasCustomParser);
    cont = cont && self.dvValueField(visitor, Fields::valueTypeName, m_valueTypeName);
    cont = cont && self.dvValueField(visitor, Fields::extensionTypeName, m_extensionTypeName);
    cont = cont && self.dvValueField(visitor, Fields::accessSemantics, int(m_accessSemantics));
    return cont;
}

Export Export::fromString(Path source, QStringView exp, Path typePath, ErrorHandler h)
{
    Export res;
    res.exportSourcePath = source;
    res.typePath = typePath;
    int slashIdx = exp.indexOf(QLatin1Char('/'));
    int spaceIdx = exp.indexOf(QLatin1Char(' '));
    if (spaceIdx == -1)
        spaceIdx = exp.size();
    else
        res.version = Version::fromString(exp.mid(spaceIdx + 1));
    if (!res.version.isValid())
        domParsingErrors()
                .error(tr("Expected string literal to contain 'Package/Name major.minor' "
                          "or 'Name major.minor' not '%1'.")
                               .arg(exp))
                .handle(h);
    QString package;
    if (slashIdx != -1)
        res.uri = exp.left(slashIdx).toString();
    res.typeName = exp.mid(slashIdx + 1, spaceIdx - (slashIdx + 1)).toString();
    return res;
}

bool AttributeInfo::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvValueField(visitor, Fields::name, name);
    cont = cont && self.dvValueField(visitor, Fields::access, int(access));
    cont = cont && self.dvValueField(visitor, Fields::typeName, typeName);
    cont = cont && self.dvValueField(visitor, Fields::isReadonly, isReadonly);
    cont = cont && self.dvValueField(visitor, Fields::isList, isList);
    cont = cont && self.dvWrapField(visitor, Fields::comments, comments);
    cont = cont && self.dvWrapField(visitor, Fields::annotations, annotations);
    return cont;
}

Path AttributeInfo::addAnnotation(Path selfPathFromOwner, const QmlObject &annotation,
                                  QmlObject **aPtr)
{
    return appendUpdatableElementInQList(selfPathFromOwner.field(Fields::annotations), annotations,
                                         annotation, aPtr);
}

void AttributeInfo::updatePathFromOwner(Path newPath)
{
    Path base = newPath.field(Fields::annotations);
    updatePathFromOwnerQList(annotations, newPath.field(Fields::annotations));
}

bool EnumDecl::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = CommentableDomElement::iterateDirectSubpaths(self, visitor);
    cont = cont && self.dvValueField(visitor, Fields::name, name());
    cont = cont && self.dvWrapField(visitor, Fields::values, m_values);
    cont = cont && self.dvWrapField(visitor, Fields::annotations, m_annotations);
    return cont;
}

void EnumDecl::updatePathFromOwner(Path newPath)
{
    DomElement::updatePathFromOwner(newPath);
    updatePathFromOwnerQList(m_annotations, newPath.field(Fields::annotations));
}

void EnumDecl::setAnnotations(QList<QmlObject> annotations)
{
    m_annotations = annotations;
}

Path EnumDecl::addAnnotation(const QmlObject &annotation, QmlObject **aPtr)
{
    return appendUpdatableElementInQList(pathFromOwner().field(Fields::annotations), m_annotations,
                                         annotation, aPtr);
}

void EnumDecl::writeOut(DomItem &self, OutWriter &ow) const
{
    ow.writeRegion(u"enum", u"enum")
            .space()
            .writeRegion(u"name", name())
            .space()
            .writeRegion(u"lbrace", u"{");
    int iLevel = ow.increaseIndent(1);
    for (auto value : self.field(Fields::values).values()) {
        ow.ensureNewline();
        value.writeOut(ow);
    }
    ow.decreaseIndent(1, iLevel);
    ow.ensureNewline().writeRegion(u"rbrace", u"}");
}

QList<Path> ImportScope::allSources(DomItem &self) const
{
    DomItem top = self.top();
    DomItem env = top.environment();
    Path selfPath = self.canonicalPath().field(Fields::allSources);
    RefCacheEntry cached = (env ? RefCacheEntry::forPath(env, selfPath) : RefCacheEntry());
    if (cached.cached == RefCacheEntry::Cached::All)
        return cached.canonicalPaths;
    QList<Path> res;
    QSet<Path> knownPaths;
    QList<Path> toDo(m_importSourcePaths.rbegin(), m_importSourcePaths.rend());
    while (!toDo.isEmpty()) {
        Path pNow = toDo.takeLast();
        if (knownPaths.contains(pNow))
            continue;
        knownPaths.insert(pNow);
        res.append(pNow);
        DomItem sourceBase = top.path(pNow);
        for (DomItem autoExp : sourceBase.field(Fields::autoExports).values()) {
            if (const ModuleAutoExport *autoExpPtr = autoExp.as<ModuleAutoExport>()) {
                Path newSource;
                if (autoExpPtr->inheritVersion) {
                    Version v = autoExpPtr->import.version;
                    DomItem sourceVersion = sourceBase.field(Fields::version);
                    if (const Version *sourceVersionPtr = sourceVersion.as<Version>()) {
                        if (v.majorVersion < 0)
                            v.majorVersion = sourceVersionPtr->majorVersion;
                        if (v.minorVersion < 0)
                            v.minorVersion = sourceVersionPtr->minorVersion;
                    } else {
                        qWarning() << "autoExport with inherited version " << autoExp
                                   << " but missing version in source" << pNow;
                    }
                    Import toImport(autoExpPtr->import.uri, v);
                    newSource = toImport.importedPath();
                } else {
                    newSource = autoExpPtr->import.importedPath();
                }
                if (newSource && !knownPaths.contains(newSource))
                    toDo.append(newSource);
            } else {
                qWarning() << "expected ModuleAutoExport not " << autoExp.internalKindStr()
                           << "looking up autoExports of" << sourceBase;
                Q_ASSERT(false);
            }
        }
    }
    if (env)
        RefCacheEntry::addForPath(env, selfPath, RefCacheEntry { RefCacheEntry::Cached::All, res });
    return res;
}

bool ImportScope::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvReferencesField(visitor, Fields::importSources, m_importSourcePaths);
    cont = cont && self.dvItemField(visitor, Fields::allSources, [this, &self]() -> DomItem {
        return self.subListItem(List::fromQList<Path>(
                self.pathFromOwner().field(Fields::allSources), allSources(self),
                [](DomItem &list, const PathEls::PathComponent &p, const Path &el) {
                    return list.subDataItem(p, el.toString());
                }));
    });
    cont = cont && self.dvWrapField(visitor, Fields::qualifiedImports, m_subImports);
    cont = cont && self.dvItemField(visitor, Fields::imported, [this, &self]() -> DomItem {
        return self.subMapItem(Map(
                self.pathFromOwner().field(Fields::imported),
                [this, &self](DomItem &map, QString key) {
                    return map.subListItem(List::fromQList<DomItem>(
                            map.pathFromOwner().key(key), importedItemsWithName(self, key),
                            [](DomItem &, const PathEls::PathComponent &, DomItem &el) {
                                return el;
                            }));
                },
                [this, &self](DomItem &) { return this->importedNames(self); },
                QLatin1String("List<Export>")));
    });
    return cont;
}

bool PropertyInfo::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvValueField(visitor, Fields::propertyDefs, propertyDefs);
    cont = cont && self.dvValueField(visitor, Fields::bindings, bindings);
    return cont;
}

BindingValue::BindingValue() : kind(BindingValueKind::Empty) { }

BindingValue::BindingValue(const QmlObject &o) : kind(BindingValueKind::Object)
{
    new (&object) QmlObject(o);
}

BindingValue::BindingValue(std::shared_ptr<ScriptExpression> o)
    : kind(BindingValueKind::ScriptExpression)
{
    new (&scriptExpression) std::shared_ptr<ScriptExpression>(o);
}

BindingValue::BindingValue(const QList<QmlObject> &l) : kind(BindingValueKind::Array)
{
    new (&array) QList<QmlObject>(l);
}

BindingValue::~BindingValue()
{
    clearValue();
}

BindingValue::BindingValue(const BindingValue &o) : kind(o.kind)
{
    switch (o.kind) {
    case BindingValueKind::Empty:
        break;
    case BindingValueKind::Object:
        new (&object) QmlObject(o.object);
        break;
    case BindingValueKind::ScriptExpression:
        new (&scriptExpression) std::shared_ptr<ScriptExpression>(o.scriptExpression);
        break;
    case BindingValueKind::Array:
        new (&array) QList<QmlObject>(o.array);
    }
}

BindingValue &BindingValue::operator=(const BindingValue &o)
{
    clearValue();
    kind = o.kind;
    switch (o.kind) {
    case BindingValueKind::Empty:
        break;
    case BindingValueKind::Object:
        new (&object) QmlObject(o.object);
        break;
    case BindingValueKind::ScriptExpression:
        new (&scriptExpression) std::shared_ptr<ScriptExpression>(o.scriptExpression);
        break;
    case BindingValueKind::Array:
        new (&array) QList<QmlObject>(o.array);
    }
    return *this;
}

DomItem BindingValue::value(DomItem &binding)
{
    switch (kind) {
    case BindingValueKind::Empty:
        break;
    case BindingValueKind::Object:
        return binding.copy(&object);
    case BindingValueKind::ScriptExpression:
        return binding.subOwnerItem(PathEls::Field(Fields::value), scriptExpression);
    case BindingValueKind::Array:
        return binding.subListItem(List::fromQListRef<QmlObject>(
                binding.pathFromOwner().field(u"value"), array,
                [binding](DomItem &self, const PathEls::PathComponent &, QmlObject &obj) {
                    return self.copy(&obj);
                }));
    }
    return DomItem();
}

void BindingValue::updatePathFromOwner(Path newPath)
{
    switch (kind) {
    case BindingValueKind::Empty:
        break;
    case BindingValueKind::Object:
        object.updatePathFromOwner(newPath);
        break;
    case BindingValueKind::ScriptExpression:
        break;
    case BindingValueKind::Array:
        updatePathFromOwnerQList(array, newPath);
        break;
    }
}

void BindingValue::clearValue()
{
    switch (kind) {
    case BindingValueKind::Empty:
        break;
    case BindingValueKind::Object:
        object.~QmlObject();
        break;
    case BindingValueKind::ScriptExpression:
        scriptExpression.~shared_ptr();
        break;
    case BindingValueKind::Array:
        array.~QList<QmlObject>();
        break;
    }
    kind = BindingValueKind::Empty;
}

ScriptExpression::ScriptExpression(QStringView code, std::shared_ptr<QQmlJS::Engine> engine,
                                   AST::Node *ast, std::shared_ptr<AstComments> comments,
                                   ExpressionType expressionType, SourceLocation localOffset,
                                   int derivedFrom, QStringView preCode, QStringView postCode)
    : OwningItem(derivedFrom),
      m_expressionType(expressionType),
      m_code(code),
      m_preCode(preCode),
      m_postCode(postCode),
      m_engine(engine),
      m_ast(ast),
      m_astComments(comments),
      m_localOffset(localOffset)
{
    if (m_expressionType == ExpressionType::BindingExpression)
        if (AST::ExpressionStatement *exp = AST::cast<AST::ExpressionStatement *>(m_ast))
            m_ast = exp->expression;
    Q_ASSERT(m_astComments);
}

ScriptExpression::ScriptExpression(const ScriptExpression &e) : OwningItem(e)
{
    QMutexLocker l(mutex());
    m_expressionType = e.m_expressionType;
    m_engine = e.m_engine;
    m_ast = e.m_ast;
    if (m_codeStr.isEmpty()) {
        m_code = e.m_code;
    } else {
        m_codeStr = e.m_codeStr;
        m_code = m_codeStr;
    }
    m_localOffset = e.m_localOffset;
    m_astComments = e.m_astComments;
}

std::shared_ptr<ScriptExpression> ScriptExpression::copyWithUpdatedCode(DomItem &self,
                                                                        QString code) const
{
    std::shared_ptr<ScriptExpression> copy = makeCopy(self);
    DomItem container = self.containingObject();
    QString preCodeStr = container.field(Fields::preCode).value().toString(m_preCode.toString());
    QString postCodeStr = container.field(Fields::postCode).value().toString(m_postCode.toString());
    copy->setCode(code, preCodeStr, postCodeStr);
    return copy;
}

bool ScriptExpression::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = OwningItem::iterateDirectSubpaths(self, visitor);
    cont = cont && self.dvValueField(visitor, Fields::code, code());
    if (!preCode().isEmpty())
        cont = cont
                && self.dvValueField(visitor, Fields::preCode, preCode(),
                                     ConstantData::Options::MapIsMap);
    if (!postCode().isEmpty())
        cont = cont
                && self.dvValueField(visitor, Fields::postCode, postCode(),
                                     ConstantData::Options::MapIsMap);
    cont = cont
            && self.dvValueLazyField(
                    visitor, Fields::localOffset,
                    [this]() { return locationToData(localOffset()); },
                    ConstantData::Options::MapIsMap);
    cont = cont && self.dvValueLazyField(visitor, Fields::astRelocatableDump, [this]() {
        return astRelocatableDump();
    });
    cont = cont && self.dvValueField(visitor, Fields::expressionType, int(expressionType()));
    if (m_element) {
        cont = cont && self.dvItemField(visitor, Fields::scriptElement, [this, &self]() {
            return self.subScriptElementWrapperItem(m_element);
        });
    }
    return cont;
}

class FirstNodeVisitor : public VisitAll
{
public:
    quint32 minStart = 0;
    quint32 maxEnd = std::numeric_limits<quint32>::max();
    AST::Node *firstNodeInRange = nullptr;

    FirstNodeVisitor(quint32 minStart = 0, quint32 maxEnd = std::numeric_limits<quint32>::max())
        : minStart(minStart), maxEnd(maxEnd)
    {
    }

    bool preVisit(AST::Node *n) override
    {
        if (!VisitAll::uiKinds().contains(n->kind)) {
            quint32 start = n->firstSourceLocation().begin();
            quint32 end = n->lastSourceLocation().end();
            if (!firstNodeInRange && minStart <= start && end <= maxEnd && start < end)
                firstNodeInRange = n;
        }
        return !firstNodeInRange;
    }
};

AST::Node *firstNodeInRange(AST::Node *n, quint32 minStart = 0, quint32 maxEnd = ~quint32(0))
{
    FirstNodeVisitor visitor(minStart, maxEnd);
    AST::Node::accept(n, &visitor);
    return visitor.firstNodeInRange;
}

void ScriptExpression::setCode(QString code, QString preCode, QString postCode)
{
    m_codeStr = code;
    const bool qmlMode = (m_expressionType == ExpressionType::BindingExpression);
    if (qmlMode && preCode.isEmpty()) {
        preCode = Binding::preCodeForName(u"binding");
        postCode = Binding::postCodeForName(u"binding");
    }
    if (!preCode.isEmpty() || !postCode.isEmpty())
        m_codeStr = preCode + code + postCode;
    m_code = QStringView(m_codeStr).mid(preCode.size(), code.size());
    m_preCode = QStringView(m_codeStr).mid(0, preCode.size());
    m_postCode = QStringView(m_codeStr).mid(preCode.size() + code.size(), postCode.size());
    m_engine = nullptr;
    m_ast = nullptr;
    m_localOffset = SourceLocation();
    if (!m_code.isEmpty()) {
        IndentInfo preChange(m_preCode, 4);
        m_localOffset.offset = m_preCode.size();
        m_localOffset.length = m_code.size();
        m_localOffset.startColumn = preChange.trailingString.size();
        m_localOffset.startLine = preChange.nNewlines;
        m_engine = std::make_shared<QQmlJS::Engine>();
        m_astComments = std::make_shared<AstComments>(m_engine);
        QQmlJS::Lexer lexer(m_engine.get());
        lexer.setCode(m_codeStr, /*lineno = */ 1, /*qmlMode=*/true);
        QQmlJS::Parser parser(m_engine.get());
        if ((qmlMode && !parser.parse()) || (!qmlMode && !parser.parseScript()))
            addErrorLocal(domParsingErrors().error(tr("Parsing of code failed")));
        for (DiagnosticMessage msg : parser.diagnosticMessages()) {
            ErrorMessage err = domParsingErrors().errorMessage(msg);
            err.location.offset -= m_localOffset.offset;
            err.location.startLine -= m_localOffset.startLine;
            if (err.location.startLine == 1)
                err.location.startColumn -= m_localOffset.startColumn;
            addErrorLocal(err);
        }
        m_ast = parser.rootNode();
        if (AST::Program *programPtr = AST::cast<AST::Program *>(m_ast)) {
            m_ast = programPtr->statements;
        }
        if (!m_preCode.isEmpty())
            m_ast = firstNodeInRange(m_ast, m_preCode.size(),
                                     m_preCode.size() + m_code.size());
        if (auto *sList = AST::cast<AST::FormalParameterList *>(m_ast)) {
            m_ast = sList->element;
        }
        if (m_expressionType != ExpressionType::FunctionBody) {
            if (AST::StatementList *sList = AST::cast<AST::StatementList *>(m_ast)) {
                if (!sList->next)
                    m_ast = sList->statement;
            }
        }
        if (m_expressionType == ExpressionType::BindingExpression)
            if (AST::ExpressionStatement *exp = AST::cast<AST::ExpressionStatement *>(m_ast))
                m_ast = exp->expression;
        AstComments::collectComments(m_engine, m_ast, m_astComments, MutableDomItem(), nullptr);
    }
}

void ScriptExpression::astDumper(Sink s, AstDumperOptions options) const
{
    astNodeDumper(s, ast(), options, 1, 0, [this](SourceLocation astL) {
        SourceLocation l = this->locationToLocal(astL);
        return this->code().mid(l.offset, l.length);
    });
}

QString ScriptExpression::astRelocatableDump() const
{
    return dumperToString([this](Sink s) {
        this->astDumper(s, AstDumperOption::NoLocations | AstDumperOption::SloppyCompare);
    });
}

void ScriptExpression::writeOut(DomItem &self, OutWriter &lw) const
{
    OutWriter *ow = &lw;

    std::optional<PendingSourceLocationId> codeLoc;
    if (lw.lineWriter.options().updateOptions & LineWriterOptions::Update::Expressions)
        codeLoc = lw.lineWriter.startSourceLocation([this, self, ow](SourceLocation myLoc) mutable {
            QStringView reformattedCode =
                    QStringView(ow->writtenStr).mid(myLoc.offset, myLoc.length);
            if (reformattedCode != code()) {
                std::shared_ptr<ScriptExpression> copy =
                        copyWithUpdatedCode(self, reformattedCode.toString());
                ow->addReformattedScriptExpression(self.canonicalPath(), copy);
            }
        });
    reformatAst(
            lw, m_astComments,
            [this](SourceLocation astL) {
                SourceLocation l = this->locationToLocal(astL); // use engine->code() instead?
                return this->code().mid(l.offset, l.length);
            },
            ast());
    if (codeLoc)
        lw.lineWriter.endSourceLocation(*codeLoc);
}

SourceLocation ScriptExpression::globalLocation(DomItem &self) const
{
    if (const FileLocations *fLocPtr = FileLocations::fileLocationsOf(self)) {
        return fLocPtr->regions.value(QString(), fLocPtr->fullRegion);
    }
    return SourceLocation();
}

bool PropertyDefinition::isParametricType() const
{
    return typeName.contains(QChar(u'<'));
}

void PropertyDefinition::writeOut(DomItem &, OutWriter &lw) const
{
    lw.ensureNewline();
    if (isDefaultMember)
        lw.writeRegion(u"default").space();
    if (isRequired)
        lw.writeRegion(u"required").space();
    if (isReadonly)
        lw.writeRegion(u"readonly").space();
    if (!typeName.isEmpty()) {
        lw.writeRegion(u"property").space();
        lw.writeRegion(u"type", typeName).space();
    }
    lw.writeRegion(u"name", name);
}

bool MethodInfo::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = AttributeInfo::iterateDirectSubpaths(self, visitor);
    cont = cont && self.dvWrapField(visitor, Fields::parameters, parameters);
    cont = cont && self.dvValueField(visitor, Fields::methodType, int(methodType));
    if (!typeName.isEmpty())
        cont = cont && self.dvReferenceField(visitor, Fields::type, typePath(self));
    if (methodType == MethodType::Method) {
        cont = cont && self.dvValueField(visitor, Fields::preCode, preCode(self));
        cont = cont && self.dvValueField(visitor, Fields::postCode, postCode(self));
        cont = cont && self.dvValueField(visitor, Fields::isConstructor, isConstructor);
    }
    if (returnType)
        cont = cont && self.dvItemField(visitor, Fields::returnType, [this, &self]() {
            return self.subOwnerItem(PathEls::Field(Fields::returnType), returnType);
        });
    if (body)
        cont = cont && self.dvItemField(visitor, Fields::body, [this, &self]() {
            return self.subOwnerItem(PathEls::Field(Fields::body), body);
        });
    return cont;
}

QString MethodInfo::preCode(DomItem &self) const
{
    QString res;
    LineWriter lw([&res](QStringView s) { res.append(s); }, QLatin1String("*preCode*"));
    OutWriter ow(lw);
    ow.indentNextlines = true;
    ow.skipComments = true;
    MockObject standinObj(self.pathFromOwner());
    DomItem standin = self.copy(&standinObj);
    ow.itemStart(standin);
    ow.writeRegion(u"function").space().writeRegion(u"name", name);
    bool first = true;
    ow.writeRegion(u"leftParen", u"(");
    for (const MethodParameter &mp : parameters) {
        if (first)
            first = false;
        else
            ow.write(u", ");
        ow.write(mp.value->code());
    }
    ow.writeRegion(u"rightParen", u")");
    ow.ensureSpace().writeRegion(u"leftBrace", u"{");
    ow.itemEnd(standin);
    ow.eof();
    return res;
}

QString MethodInfo::postCode(DomItem &) const
{
    return QLatin1String("\n}\n");
}

void MethodInfo::writeOut(DomItem &self, OutWriter &ow) const
{
    switch (methodType) {
    case MethodType::Signal: {
        if (body)
            qCWarning(domLog) << "signal should not have a body in" << self.canonicalPath();
        ow.writeRegion(u"signal").space().writeRegion(u"name", name);
        if (parameters.isEmpty())
            return;
        bool first = true;
        ow.writeRegion(u"leftParen", u"(");
        int baseIndent = ow.increaseIndent();
        for (DomItem arg : self.field(Fields::parameters).values()) {
            if (first)
                first = false;
            else
                ow.write(u", ");
            if (const MethodParameter *argPtr = arg.as<MethodParameter>())
                argPtr->writeOutSignal(arg, ow);
            else
                qCWarning(domLog) << "failed to cast to MethodParameter";
        }
        ow.writeRegion(u"rightParen", u")");
        ow.decreaseIndent(1, baseIndent);
        return;
    } break;
    case MethodType::Method: {
        ow.writeRegion(u"function").space().writeRegion(u"name", name);
        bool first = true;
        ow.writeRegion(u"leftParen", u"(");
        for (DomItem arg : self.field(Fields::parameters).values()) {
            if (first)
                first = false;
            else
                ow.write(u", ");
            arg.writeOut(ow);
        }
        ow.writeRegion(u"rightParen", u")");
        if (!typeName.isEmpty()) {
            ow.writeRegion(u"colon", u":");
            ow.space();
            ow.writeRegion(u"returnType", typeName);
        }
        ow.ensureSpace().writeRegion(u"leftBrace", u"{");
        int baseIndent = ow.increaseIndent();
        if (DomItem b = self.field(Fields::body)) {
            ow.ensureNewline();
            b.writeOut(ow);
        }
        ow.decreaseIndent(1, baseIndent);
        ow.ensureNewline().writeRegion(u"rightBrace", u"}");
    } break;
    }
}

bool MethodParameter::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvValueField(visitor, Fields::name, name);
    if (!typeName.isEmpty()) {
        cont = cont
                && self.dvReferenceField(visitor, Fields::type, Paths::lookupTypePath(typeName));
        cont = cont && self.dvValueField(visitor, Fields::typeName, typeName);
    }
    cont = cont && self.dvValueField(visitor, Fields::isPointer, isPointer);
    cont = cont && self.dvValueField(visitor, Fields::isReadonly, isReadonly);
    cont = cont && self.dvValueField(visitor, Fields::isList, isList);
    cont = cont && self.dvWrapField(visitor, Fields::defaultValue, defaultValue);
    cont = cont && self.dvWrapField(visitor, Fields::value, value);

    cont = cont && self.dvValueField(visitor, Fields::preCode, u"function f("_s);
    cont = cont && self.dvValueField(visitor, Fields::postCode, u") {}"_s);

    if (!annotations.isEmpty())
        cont = cont && self.dvWrapField(visitor, Fields::annotations, annotations);
    cont = cont && self.dvWrapField(visitor, Fields::comments, comments);
    return cont;
}

void MethodParameter::writeOut(DomItem &self, OutWriter &ow) const
{
    if (!name.isEmpty()) {
        if (isRestElement)
            ow.writeRegion(u"ellipsis", u"...");
        ow.writeRegion(u"name", name);
        if (!typeName.isEmpty())
            ow.writeRegion(u"colon", u":").space().writeRegion(u"type", typeName);
        if (defaultValue) {
            ow.space().writeRegion(u"equal", u"=").space();
            self.subOwnerItem(PathEls::Field(Fields::defaultValue), defaultValue).writeOut(ow);
        }
    } else {
        if (value) {
            self.subOwnerItem(PathEls::Field(Fields::value), value).writeOut(ow);
        }
    }
}

void MethodParameter::writeOutSignal(DomItem &self, OutWriter &ow) const
{
    self.writeOutPre(ow);
    if (!typeName.isEmpty())
        ow.writeRegion(u"type", typeName).space();
    ow.writeRegion(u"name", name);
    self.writeOutPost(ow);
}

void Pragma::writeOut(DomItem &, OutWriter &ow) const
{
    ow.ensureNewline();
    ow.writeRegion(u"pragma").space().writeRegion(u"name", name);

    bool isFirst = true;
    for (const auto &value : values) {
        if (isFirst) {
            isFirst = false;
            ow.writeRegion(u"colon", u": ");
            ow.writeRegion(u"values", value);
            continue;
        }

        ow.writeRegion(u"comma", u", ");
        ow.writeRegion(u"values", value);
    }
    ow.ensureNewline();
}

bool EnumItem::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && self.dvValueField(visitor, Fields::name, name());
    cont = cont && self.dvValueField(visitor, Fields::value, value());
    cont = cont && self.dvWrapField(visitor, Fields::comments, m_comments);
    return cont;
}

void EnumItem::writeOut(DomItem &self, OutWriter &ow) const
{
    ow.ensureNewline();
    ow.writeRegion(u"name", name());
    bool hasDefaultValue = false;
    index_type myIndex = self.pathFromOwner().last().headIndex();
    if (myIndex == 0)
        hasDefaultValue = value() == 0;
    else if (myIndex > 0)
        hasDefaultValue = value()
                == self.container()
                                .index(myIndex - 1)
                                .field(Fields::value)
                                .value()
                                .toDouble(value())
                        + 1;
    if (!hasDefaultValue) {
        QString v = QString::number(value(), 'f', 0);
        if (abs(value() - v.toDouble()) > 1.e-10)
            v = QString::number(value());
        ow.space().writeRegion(u"equal", u"=").space().writeRegion(u"value", v);
    }
    if (myIndex >= 0 && self.container().indexes() != myIndex + 1)
        ow.writeRegion(u"comma", u",");
}

QmlUri QmlUri::fromString(const QString &str)
{
    if (str.startsWith(u'"'))
        return fromDirectoryString(str.mid(1, str.size() - 2)
                                           .replace(u"\\\""_s, u"\""_s)
                                           .replace(u"\\\\"_s, u"\\"_s));
    else
        return fromUriString(str);
}

QmlUri QmlUri::fromUriString(const QString &str)
{
    QRegularExpression moduleUriRe(QLatin1String(R"(\A\w+(?:\.\w+)*\Z)"));
    return QmlUri((moduleUriRe.match(str).hasMatch() ? Kind::ModuleUri : Kind::Invalid), str);
}

QmlUri QmlUri::fromDirectoryString(const QString &str)
{
    QUrl url(str);
    if (url.isValid() && url.scheme().size() > 1)
        return QmlUri(url);
    if (!str.isEmpty()) {
        QFileInfo path(str);
        return QmlUri((path.isRelative() ? Kind::RelativePath : Kind::AbsolutePath), str);
    }
    return {};
}

bool QmlUri::isValid() const
{
    return m_kind != Kind::Invalid;
}

bool QmlUri::isDirectory() const
{
    switch (m_kind) {
    case Kind::Invalid:
    case Kind::ModuleUri:
        break;
    case Kind::DirectoryUrl:
    case Kind::RelativePath:
    case Kind::AbsolutePath:
        return true;
    }
    return false;
}

bool QmlUri::isModule() const
{
    return m_kind == Kind::ModuleUri;
}

QString QmlUri::moduleUri() const
{
    if (m_kind == Kind::ModuleUri)
        return std::get<QString>(m_value);
    return QString();
}

QString QmlUri::localPath() const
{
    switch (m_kind) {
    case Kind::Invalid:
    case Kind::ModuleUri:
        break;
    case Kind::DirectoryUrl: {
        const QUrl &url = std::get<QUrl>(m_value);
        if (url.scheme().compare(u"file", Qt::CaseInsensitive) == 0)
            return url.path();
        break;
    }
    case Kind::RelativePath:
    case Kind::AbsolutePath:
        return std::get<QString>(m_value);
    }
    return QString();
}

QString QmlUri::absoluteLocalPath(const QString &basePath) const
{
    switch (m_kind) {
    case Kind::Invalid:
    case Kind::ModuleUri:
        break;
    case Kind::DirectoryUrl: {
        const QUrl &url = std::get<QUrl>(m_value);
        if (url.scheme().compare(u"file", Qt::CaseInsensitive) == 0)
            return url.path();
        break;
    }
    case Kind::RelativePath: {
        if (!basePath.isEmpty())
            return QDir(basePath).filePath(std::get<QString>(m_value));
        break;
    }
    case Kind::AbsolutePath:
        return std::get<QString>(m_value);
    }
    return QString();
}

QUrl QmlUri::directoryUrl() const
{
    if (m_kind == Kind::DirectoryUrl)
        return std::get<QUrl>(m_value);
    return QUrl {};
}

QString QmlUri::directoryString() const
{
    switch (m_kind) {
    case Kind::Invalid:
    case Kind::ModuleUri:
        break;
    case Kind::DirectoryUrl:
        return std::get<QUrl>(m_value).toString(); // set formatting? options?
    case Kind::RelativePath:
    case Kind::AbsolutePath:
        return std::get<QString>(m_value);
    }
    return QString();
}

QString QmlUri::toString() const
{
    switch (m_kind) {
    case Kind::Invalid:
        break;
    case Kind::ModuleUri:
        return std::get<QString>(m_value);
    case Kind::DirectoryUrl:
    case Kind::RelativePath:
    case Kind::AbsolutePath:
        return u"\""_s + directoryString().replace(u'\\', u"\\\\"_s).replace(u'"', u"\\\""_s)
                + u"\""_s;
    }
    return QString();
}

QmlUri::Kind QmlUri::kind() const
{
    return m_kind;
}

void ScriptExpression::setScriptElement(const ScriptElementVariant &p)
{
    m_element = p;
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE

#include "moc_qqmldomelements_p.cpp"
