/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include "qqmldomastcreator_p.h"
#include "qqmldomelements_p.h"
#include "qqmldomtop_p.h"
#include "qqmldomerrormessage_p.h"
#include "qqmldomastdumper_p.h"
#include "qqmldomattachedinfo_p.h"

#include <QtQml/private/qqmljsast_p.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtCore/QLoggingCategory>

#include <memory>
#include <variant>

static Q_LOGGING_CATEGORY(creatorLog, "qt.qmldom.astcreator", QtWarningMsg);

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

using namespace AST;

template<typename K, typename V>
V *valueFromMultimap(QMultiMap<K, V> &mmap, const K &key, index_type idx)
{
    if (idx < 0)
        return nullptr;
    auto it = mmap.find(key);
    auto end = mmap.end();
    if (it == end)
        return nullptr;
    auto it2 = it;
    index_type nEl = 0;
    while (it2 != end && it2.key() == key) {
        ++it2;
        ++nEl;
    }
    if (nEl <= idx)
        return nullptr;
    for (index_type i = idx + 1; i < nEl; ++i)
        ++it;
    return &(*it);
}

static ErrorGroups myParseErrors()
{
    static ErrorGroups errs = { { NewErrorGroup("Dom"), NewErrorGroup("QmlFile"),
                                  NewErrorGroup("Parsing") } };
    return errs;
}

static QString toString(const UiQualifiedId *qualifiedId, QChar delimiter = QLatin1Char('.'))
{
    QString result;

    for (const UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
        if (iter != qualifiedId)
            result += delimiter;

        result += iter->name;
    }

    return result;
}

SourceLocation combineLocations(SourceLocation s1, SourceLocation s2)
{
    return combine(s1, s2);
}

SourceLocation combineLocations(Node *n)
{
    return combineLocations(n->firstSourceLocation(), n->lastSourceLocation());
}

class DomValue
{
public:
    template<typename T>
    DomValue(const T &obj) : kind(T::kindValue), value(obj)
    {
    }
    DomType kind;
    std::variant<QmlObject, MethodInfo, QmlComponent, PropertyDefinition, Binding, EnumDecl,
                 EnumItem, ConstantData, Id>
            value;
};

class StackEl
{
public:
    Path path;
    DomValue item;
    FileLocations::Tree fileLocations;
};

class QmlDomAstCreator final : public AST::Visitor
{
    Q_DECLARE_TR_FUNCTIONS(QmlDomAstCreator)

    static constexpr const auto className = "QmlDomAstCreator";

    MutableDomItem qmlFile;
    std::shared_ptr<QmlFile> qmlFilePtr;
    QVector<StackEl> nodeStack;
    QVector<int> arrayBindingLevels;
    FileLocations::Tree rootMap;

    template<typename T>
    StackEl &currentEl(int idx = 0)
    {
        Q_ASSERT_X(idx < nodeStack.length() && idx >= 0, "currentQmlObjectOrComponentEl",
                   "Stack does not contain enough elements!");
        int i = nodeStack.length() - idx;
        while (i-- > 0) {
            DomType k = nodeStack.at(i).item.kind;
            if (k == T::kindValue)
                return nodeStack[i];
        }
        Q_ASSERT_X(false, "currentEl", "Stack does not contan object of type ");
        return nodeStack.last();
    }

    template<typename T>
    T &current(int idx = 0)
    {
        return std::get<T>(currentEl<T>(idx).item.value);
    }

    index_type currentIndex() { return currentNodeEl().path.last().headIndex(); }

    StackEl &currentQmlObjectOrComponentEl(int idx = 0)
    {
        Q_ASSERT_X(idx < nodeStack.length() && idx >= 0, "currentQmlObjectOrComponentEl",
                   "Stack does not contain enough elements!");
        int i = nodeStack.length() - idx;
        while (i-- > 0) {
            DomType k = nodeStack.at(i).item.kind;
            if (k == DomType::QmlObject || k == DomType::QmlComponent)
                return nodeStack[i];
        }
        Q_ASSERT_X(false, "currentQmlObjectEl", "No QmlObject or component in stack");
        return nodeStack.last();
    }

    StackEl &currentNodeEl(int i = 0)
    {
        Q_ASSERT_X(i < nodeStack.length() && i >= 0, "currentNode",
                   "Stack does not contain element!");
        return nodeStack[nodeStack.length() - i - 1];
    }

    DomValue &currentNode(int i = 0)
    {
        Q_ASSERT_X(i < nodeStack.length() && i >= 0, "currentNode",
                   "Stack does not contain element!");
        return nodeStack[nodeStack.length() - i - 1].item;
    }

    void removeCurrentNode(std::optional<DomType> expectedType)
    {
        Q_ASSERT_X(!nodeStack.isEmpty(), className, "popCurrentNode() without any node");
        if (expectedType)
            Q_ASSERT(nodeStack.last().item.kind == *expectedType);
        nodeStack.removeLast();
    }

    void pushEl(Path p, DomValue it, AST::Node *n)
    {
        nodeStack.append({ p, it, createMap(it.kind, p, n) });
    }

    FileLocations::Tree createMap(FileLocations::Tree base, Path p, AST::Node *n)
    {
        FileLocations::Tree res = FileLocations::ensure(base, p, AttachedInfo::PathType::Relative);
        if (n)
            FileLocations::addRegion(res, QString(), combineLocations(n));
        return res;
    }

    FileLocations::Tree createMap(DomType k, Path p, AST::Node *n)
    {
        FileLocations::Tree base;
        switch (k) {
        case DomType::QmlObject:
            switch (currentNode().kind) {
            case DomType::QmlObject:
            case DomType::QmlComponent:
            case DomType::PropertyDefinition:
            case DomType::Binding:
            case DomType::Id:
            case DomType::MethodInfo:
                break;
            default:
                qDebug() << "unexpected type" << domTypeToString(currentNode().kind);
                Q_ASSERT(false);
            }
            base = currentNodeEl().fileLocations;
            if (p.length() > 2) {
                Path p2 = p[p.length() - 2];
                if (p2.headKind() == Path::Kind::Field
                    && (p2.checkHeadName(Fields::children) || p2.checkHeadName(Fields::objects)
                        || p2.checkHeadName(Fields::value) || p2.checkHeadName(Fields::annotations)
                        || p2.checkHeadName(Fields::children)))
                    p = p.mid(p.length() - 2, 2);
                else if (p.last().checkHeadName(Fields::value)
                         && p.last().headKind() == Path::Kind::Field)
                    p = p.last();
                else {
                    qCWarning(domLog) << "unexpected path to QmlObject in createMap" << p;
                    Q_ASSERT(false);
                }
            } else {
                qCWarning(domLog) << "unexpected path to QmlObject in createMap" << p;
                Q_ASSERT(false);
            }
            break;
        case DomType::EnumItem:
            base = currentNodeEl().fileLocations;
            break;
        case DomType::QmlComponent:
        case DomType::Pragma:
        case DomType::Import:
        case DomType::Id:
        case DomType::EnumDecl:
            base = rootMap;
            break;
        case DomType::Binding:
        case DomType::PropertyDefinition:
        case DomType::MethodInfo:
            base = currentEl<QmlObject>().fileLocations;
            if (p.length() > 3)
                p = p.mid(p.length() - 3, 3);
            break;
        default:
            qCWarning(domLog) << "Unexpected type in createMap:" << domTypeToString(k);
            Q_ASSERT(false);
            break;
        }
        return createMap(base, p, n);
    }

public:
    QmlDomAstCreator(MutableDomItem qmlFile)
        : qmlFile(qmlFile),
          qmlFilePtr(qmlFile.ownerAs<QmlFile>()),
          rootMap(qmlFilePtr->fileLocationsTree())
    {
    }

    bool visit(UiProgram *program) override
    {
        QFileInfo fInfo(qmlFile.canonicalFilePath());
        QString componentName = fInfo.baseName();
        QmlComponent *cPtr;
        Path p = qmlFilePtr->addComponent(QmlComponent(componentName), AddOption::KeepExisting,
                                          &cPtr);
        MutableDomItem newC(qmlFile.item(), p);
        Q_ASSERT_X(newC.item(), className, "could not recover component added with addComponent");
        // QmlFile region == Component region == program span
        // we hide the component span because the component s written after the imports
        FileLocations::addRegion(rootMap, QString(), combineLocations(program));
        pushEl(p, *cPtr, program);
        // implicit imports
        // add implicit directory import
        Import selfDirImport(QLatin1String("file://") + fInfo.canonicalPath());
        selfDirImport.implicit = true;
        qmlFilePtr->addImport(selfDirImport);
        for (Import i : qmlFile.environment().ownerAs<DomEnvironment>()->implicitImports()) {
            i.implicit = true;
            qmlFilePtr->addImport(i);
        }
        return true;
    }

    void endVisit(AST::UiProgram *) override
    {
        MutableDomItem newC = qmlFile.path(currentNodeEl().path);
        QmlComponent &comp = current<QmlComponent>();
        for (const Pragma &p : qmlFilePtr->pragmas()) {
            if (p.name.compare(u"singleton", Qt::CaseInsensitive) == 0) {
                comp.setIsSingleton(true);
                comp.setIsCreatable(false); // correct?
            }
        }
        *newC.mutableAs<QmlComponent>() = comp;
        removeCurrentNode(DomType::QmlComponent);
        Q_ASSERT_X(nodeStack.isEmpty(), className, "ui program did not finish node stack");
    }

    bool visit(UiPragma *el) override
    {
        createMap(DomType::Pragma, qmlFilePtr->addPragma(Pragma(el->name.toString())), el);
        return true;
    }

    bool visit(UiImport *el) override
    {
        Version v(Version::Latest, Version::Latest);
        if (el->version && el->version->version.hasMajorVersion())
            v.majorVersion = el->version->version.majorVersion();
        if (el->version && el->version->version.hasMinorVersion())
            v.minorVersion = el->version->version.minorVersion();
        if (el->importUri != nullptr)
            createMap(DomType::Import,
                      qmlFilePtr->addImport(Import::fromUriString(toString(el->importUri), v,
                                                                  el->importId.toString())),
                      el);
        else
            createMap(DomType::Import,
                      qmlFilePtr->addImport(Import::fromFileString(
                              el->fileName.toString(),
                              QFileInfo(qmlFilePtr->canonicalFilePath()).dir().absolutePath(),
                              el->importId.toString())),
                      el);
        return true;
    }

    bool visit(AST::UiPublicMember *el) override
    {
        switch (el->type) {
        case AST::UiPublicMember::Signal: {
            MethodInfo m;
            m.name = el->name.toString();
            m.typeName = toString(el->memberType);
            m.isReadonly = el->isReadonlyMember;
            m.access = MethodInfo::Public;
            m.methodType = MethodInfo::Signal;
            m.isList = el->typeModifier == QLatin1String("list");
            MethodInfo *mPtr;
            Path p = current<QmlObject>().addMethod(m, AddOption::KeepExisting, &mPtr);
            pushEl(p, *mPtr, el);
            FileLocations::addRegion(nodeStack.last().fileLocations, u"signal", el->propertyToken);
            MethodInfo &mInfo = std::get<MethodInfo>(currentNode().value);
            AST::UiParameterList *args = el->parameters;
            while (args) {
                MethodParameter param;
                param.name = args->name.toString();
                param.typeName = toString(args->type);
                index_type idx = index_type(mInfo.parameters.size());
                mInfo.parameters.append(param);
                auto argLocs = FileLocations::ensure(nodeStack.last().fileLocations,
                                                     Path::Field(Fields::parameters).index(idx),
                                                     AttachedInfo::PathType::Relative);
                FileLocations::addRegion(argLocs, QString(), combineLocations(args));
                args = args->next;
            }
            break;
        }
        case AST::UiPublicMember::Property: {
            PropertyDefinition p;
            p.name = el->name.toString();
            p.typeName = toString(el->memberType);
            p.isReadonly = el->isReadonlyMember;
            p.isDefaultMember = el->isDefaultMember;
            p.isList = el->typeModifier == QLatin1String("list");
            p.isRequired = el->isRequired;
            if (!el->typeModifier.isEmpty())
                p.typeName = el->typeModifier.toString() + QChar(u'<') + p.typeName + QChar(u'>');
            PropertyDefinition *pPtr;
            Path pPathFromOwner =
                    current<QmlObject>().addPropertyDef(p, AddOption::KeepExisting, &pPtr);
            pushEl(pPathFromOwner, *pPtr, el);
            FileLocations::addRegion(nodeStack.last().fileLocations, u"property",
                                     el->propertyToken);
            if (p.name == u"id")
                qmlFile.addError(
                        myParseErrors()
                                .warning(tr("id is a special attribute, that should not be "
                                            "used as property name"))
                                .withPath(currentNodeEl().path));
            if (p.isDefaultMember)
                FileLocations::addRegion(nodeStack.last().fileLocations, u"default",
                                         el->defaultToken);
            if (p.isRequired)
                FileLocations::addRegion(nodeStack.last().fileLocations, u"required",
                                         el->requiredToken);
            if (el->statement) {
                BindingType bType = BindingType::Normal;
                SourceLocation loc = combineLocations(el->statement);
                QStringView code = qmlFilePtr->code();

                std::shared_ptr<ScriptExpression> script(new ScriptExpression(
                        code.mid(loc.offset, loc.length), qmlFilePtr->engine(), el->statement,
                        qmlFilePtr->astComments(),
                        ScriptExpression::ExpressionType::BindingExpression, loc));
                Binding *bPtr;
                Path bPathFromOwner = current<QmlObject>().addBinding(
                        Binding(p.name, script, bType), AddOption::KeepExisting, &bPtr);
                FileLocations::Tree bLoc = createMap(DomType::Binding, bPathFromOwner, el);
                FileLocations::addRegion(bLoc, u"colon", el->colonToken);
                FileLocations::Tree valueLoc = FileLocations::ensure(
                        bLoc, Path::Field(Fields::value), AttachedInfo::PathType::Relative);
                FileLocations::addRegion(valueLoc, QString(), combineLocations(el->statement));
            }
            break;
        }
        }
        return true;
    }

    void endVisit(AST::UiPublicMember *el) override
    {
        Node::accept(el->parameters, this);
        loadAnnotations(el);
        if ((el->binding || el->statement)
            && nodeStack.last().item.kind == DomType::PropertyDefinition) {
            PropertyDefinition &pDef = std::get<PropertyDefinition>(nodeStack.last().item.value);
            if (!pDef.annotations.isEmpty()) {
                QmlObject duplicate;
                duplicate.setName(QLatin1String("duplicate"));
                QmlObject &obj = current<QmlObject>();
                auto it = obj.m_bindings.find(pDef.name);
                if (it != obj.m_bindings.end()) {
                    for (QmlObject ann : pDef.annotations) {
                        ann.addAnnotation(duplicate);
                        it->addAnnotation(
                                currentEl<QmlObject>()
                                        .path.field(Fields::bindings)
                                        .key(pDef.name)
                                        .index(obj.m_bindings.values(pDef.name).length() - 1),
                                ann);
                    }
                }
            }
        }
        QmlObject &obj = current<QmlObject>();
        StackEl &sEl = nodeStack.last();
        switch (sEl.item.kind) {
        case DomType::PropertyDefinition: {
            PropertyDefinition pDef = std::get<PropertyDefinition>(sEl.item.value);
            PropertyDefinition *pDefPtr =
                    valueFromMultimap(obj.m_propertyDefs, pDef.name, sEl.path.last().headIndex());
            Q_ASSERT(pDefPtr);
            *pDefPtr = pDef;
        } break;
        case DomType::MethodInfo: {
            MethodInfo m = std::get<MethodInfo>(sEl.item.value);
            MethodInfo *mPtr =
                    valueFromMultimap(obj.m_methods, m.name, sEl.path.last().headIndex());
            Q_ASSERT(mPtr);
            *mPtr = m;
        } break;
        default:
            Q_ASSERT(false);
        }
        removeCurrentNode({});
    }

    bool visit(AST::UiSourceElement *el) override
    {
        QStringView code(qmlFilePtr->code());
        if (FunctionDeclaration *fDef = cast<FunctionDeclaration *>(el->sourceElement)) {
            MethodInfo m;
            m.name = fDef->name.toString();
            if (AST::TypeAnnotation *tAnn = fDef->typeAnnotation) {
                if (AST::Type *t = tAnn->type) {
                    m.typeName = toString(t->typeId);
                    if (t->typeArguments) {
                        Q_ASSERT_X(false, className,
                                   "todo: type argument should be added to the typeName");
                    }
                }
            }
            m.access = MethodInfo::Public;
            m.methodType = MethodInfo::Method;
            if (fDef->body) {
                SourceLocation bodyLoc = combineLocations(fDef->body);
                SourceLocation methodLoc = combineLocations(el);
                QStringView preCode =
                        code.mid(methodLoc.begin(), bodyLoc.begin() - methodLoc.begin());
                QStringView postCode = code.mid(bodyLoc.end(), methodLoc.end() - bodyLoc.end());
                m.body = std::shared_ptr<ScriptExpression>(new ScriptExpression(
                        code.mid(bodyLoc.offset, bodyLoc.length), qmlFilePtr->engine(), fDef->body,
                        qmlFilePtr->astComments(), ScriptExpression::ExpressionType::FunctionBody,
                        bodyLoc, 0, preCode, postCode));
            }
            MethodInfo *mPtr;
            Path mPathFromOwner = current<QmlObject>().addMethod(m, AddOption::KeepExisting, &mPtr);
            pushEl(mPathFromOwner, *mPtr,
                   fDef); // add at the start and use the normal recursive visit?
            loadAnnotations(el);
            MethodInfo &mInfo = std::get<MethodInfo>(currentNode().value);
            AST::FormalParameterList *args = fDef->formals;
            while (args) {
                MethodParameter param;
                param.name = args->element->bindingIdentifier.toString();
                if (AST::TypeAnnotation *tAnn = args->element->typeAnnotation) {
                    if (AST::Type *t = tAnn->type) {
                        param.typeName = toString(t->typeId);
                        if (t->typeArguments) {
                            Q_ASSERT_X(false, className,
                                       "todo: type argument should be added to the typeName");
                        }
                    }
                }
                if (args->element->initializer) {
                    SourceLocation loc = combineLocations(args->element->initializer);
                    std::shared_ptr<ScriptExpression> script =
                            std::shared_ptr<ScriptExpression>(new ScriptExpression(
                                    code.mid(loc.offset, loc.length), qmlFilePtr->engine(),
                                    args->element->initializer, qmlFilePtr->astComments(),
                                    ScriptExpression::ExpressionType::ArgInitializer, loc));
                    param.defaultValue = script;
                }
                index_type idx = index_type(mInfo.parameters.size());
                mInfo.parameters.append(param);
                auto argLocs = FileLocations::ensure(nodeStack.last().fileLocations,
                                                     Path::Field(Fields::parameters).index(idx),
                                                     AttachedInfo::PathType::Relative);
                FileLocations::addRegion(argLocs, QString(), combineLocations(args));
                args = args->next;
            }
            return false;
        } else {
            qCWarning(creatorLog) << "source el:" << static_cast<AST::Node *>(el);
            Q_ASSERT(false);
        }
        return true;
    }

    void endVisit(AST::UiSourceElement *) override
    {
        MethodInfo &m = std::get<MethodInfo>(currentNode().value);
        QmlObject &obj = current<QmlObject>();
        MethodInfo *mPtr =
                valueFromMultimap(obj.m_methods, m.name, nodeStack.last().path.last().headIndex());
        Q_ASSERT(mPtr);
        *mPtr = m;
        removeCurrentNode(DomType::MethodInfo);
    }

    void loadAnnotations(UiObjectMember *el) { Node::accept(el->annotations, this); }

    bool visit(AST::UiObjectDefinition *el) override
    {
        QmlObject scope;
        scope.setName(toString(el->qualifiedTypeNameId));
        scope.addPrototypePath(Paths::lookupTypePath(scope.name()));
        QmlObject *sPtr = nullptr;
        Path sPathFromOwner;
        if (!arrayBindingLevels.isEmpty() && nodeStack.length() == arrayBindingLevels.last()) {
            if (currentNode().kind == DomType::Binding) {
                QList<QmlObject> *vals = std::get<Binding>(currentNode().value).arrayValue();
                if (vals) {
                    int idx = vals->length();
                    vals->append(scope);
                    sPathFromOwner = currentNodeEl().path.field(Fields::value).index(idx);
                    sPtr = &((*vals)[idx]);
                    sPtr->updatePathFromOwner(sPathFromOwner);
                } else {
                    Q_ASSERT_X(false, className,
                               "expected an array binding with a valid QList<QmlScope> as value");
                }
            } else {
                Q_ASSERT_X(false, className, "expected an array binding as last node on the stack");
            }
        } else {
            DomValue &containingObject = currentQmlObjectOrComponentEl().item;
            switch (containingObject.kind) {
            case DomType::QmlComponent:
                sPathFromOwner =
                        std::get<QmlComponent>(containingObject.value).addObject(scope, &sPtr);
                break;
            case DomType::QmlObject:
                sPathFromOwner = std::get<QmlObject>(containingObject.value).addChild(scope, &sPtr);
                break;
            default:
                Q_ASSERT(false);
            }
        }
        Q_ASSERT_X(sPtr, className, "could not recover new scope");
        pushEl(sPathFromOwner, *sPtr, el);
        loadAnnotations(el);
        return true;
    }

    void endVisit(AST::UiObjectDefinition *) override
    {
        QmlObject &obj = current<QmlObject>();
        int idx = currentIndex();
        if (!arrayBindingLevels.isEmpty() && nodeStack.length() == arrayBindingLevels.last() + 1) {
            if (currentNode(1).kind == DomType::Binding) {
                Binding &b = std::get<Binding>(currentNode(1).value);
                QList<QmlObject> *vals = b.arrayValue();
                Q_ASSERT_X(vals, className,
                           "expected an array binding with a valid QList<QmlScope> as value");
                (*vals)[idx] = obj;
            } else {
                Q_ASSERT_X(false, className, "expected an array binding as last node on the stack");
            }
        } else {
            DomValue &containingObject = currentNodeEl(1).item;
            Path p = currentNodeEl().path;
            switch (containingObject.kind) {
            case DomType::QmlComponent:
                if (p[p.length() - 2] == Path::Field(Fields::objects))
                    std::get<QmlComponent>(containingObject.value).m_objects[idx] = obj;
                else
                    Q_ASSERT(false);
                break;
            case DomType::QmlObject:
                if (p[p.length() - 2] == Path::Field(Fields::children))
                    std::get<QmlObject>(containingObject.value).m_children[idx] = obj;
                else
                    Q_ASSERT(false);
                break;
            default:
                Q_ASSERT(false);
            }
        }
        removeCurrentNode(DomType::QmlObject);
    }

    bool visit(AST::UiObjectBinding *el) override
    {
        BindingType bType = (el->hasOnToken ? BindingType::OnBinding : BindingType::Normal);
        QmlObject value;
        value.setName(toString(el->qualifiedTypeNameId));
        Binding *bPtr;
        Path bPathFromOwner = current<QmlObject>().addBinding(
                Binding(toString(el->qualifiedId), value, bType), AddOption::KeepExisting, &bPtr);
        if (bPtr->name() == u"id")
            qmlFile.addError(myParseErrors()
                                     .error(tr("id attributes should only be a lower case letter "
                                               "followed by letters, numbers or underscore"))
                                     .withPath(bPathFromOwner));
        pushEl(bPathFromOwner, *bPtr, el);
        FileLocations::addRegion(nodeStack.last().fileLocations, u"colon", el->colonToken);
        loadAnnotations(el);
        QmlObject *objValue = bPtr->objectValue();
        Q_ASSERT_X(objValue, className, "could not recover objectValue");
        pushEl(bPathFromOwner.field(Fields::value), *objValue, el->initializer);
        return true;
    }

    void endVisit(AST::UiObjectBinding *) override
    {
        QmlObject &objValue = current<QmlObject>();
        QmlObject &containingObj = current<QmlObject>(1);
        Binding &b = std::get<Binding>(currentNode(1).value);
        QmlObject *objPtr = b.objectValue();
        Q_ASSERT(objPtr);
        *objPtr = objValue;
        index_type idx = currentNodeEl(1).path.last().headIndex();
        Binding *bPtr = valueFromMultimap(containingObj.m_bindings, b.name(), idx);
        Q_ASSERT(bPtr);
        *bPtr = b;
        removeCurrentNode(DomType::QmlObject);
        removeCurrentNode(DomType::Binding);
    }

    bool visit(AST::UiScriptBinding *el) override
    {
        QStringView code = qmlFilePtr->code();
        SourceLocation loc = combineLocations(el->statement);
        std::shared_ptr<ScriptExpression> script(
                new ScriptExpression(code.mid(loc.offset, loc.length), qmlFilePtr->engine(),
                                     el->statement, qmlFilePtr->astComments(),
                                     ScriptExpression::ExpressionType::BindingExpression, loc));
        Binding bindingV(toString(el->qualifiedId), script, BindingType::Normal);
        Binding *bindingPtr = nullptr;
        Id *idPtr = nullptr;
        Path pathFromOwner;
        if (bindingV.name() == u"id") {
            Node *exp = script->ast();
            if (ExpressionStatement *eStat = cast<ExpressionStatement *>(script->ast()))
                exp = eStat->expression;
            if (IdentifierExpression *iExp = cast<IdentifierExpression *>(exp)) {
                StackEl &containingObjectEl = currentEl<QmlObject>();
                QmlObject &containingObject = std::get<QmlObject>(containingObjectEl.item.value);
                QString idName = iExp->name.toString();
                Id idVal(idName, qmlFile.canonicalPath().path(containingObject.pathFromOwner()));
                containingObject.setIdStr(idName);
                FileLocations::addRegion(containingObjectEl.fileLocations, u"idToken",
                                         combineLocations(el->qualifiedId));
                FileLocations::addRegion(containingObjectEl.fileLocations, u"idColon",
                                         el->colonToken);
                FileLocations::addRegion(containingObjectEl.fileLocations, u"id",
                                         combineLocations(el->statement));
                QmlComponent &comp = current<QmlComponent>();
                pathFromOwner = comp.addId(idVal, AddOption::KeepExisting, &idPtr);
                QRegularExpression idRe(QRegularExpression::anchoredPattern(
                        QStringLiteral(uR"([[:lower:]][[:lower:][:upper:]0-9_]*)")));
                auto m = idRe.match(iExp->name);
                if (!m.hasMatch()) {
                    qmlFile.addError(
                            myParseErrors()
                                    .error(tr("id attributes should only be a lower case letter "
                                              "followed by letters, numbers or underscore, not %1")
                                                   .arg(iExp->name))
                                    .withPath(pathFromOwner));
                }
            } else {
                pathFromOwner = current<QmlObject>().addBinding(bindingV, AddOption::KeepExisting,
                                                                &bindingPtr);
                Q_ASSERT_X(bindingPtr, className, "binding could not be retrived");
                qmlFile.addError(
                        myParseErrors()
                                .error(tr("id attributes should only be a lower case letter "
                                          "followed by letters, numbers or underscore, not %1 %2")
                                               .arg(script->code(), script->astRelocatableDump()))
                                .withPath(pathFromOwner)
                                .handle());
            }
        } else {
            pathFromOwner =
                    current<QmlObject>().addBinding(bindingV, AddOption::KeepExisting, &bindingPtr);
            Q_ASSERT_X(bindingPtr, className, "binding could not be retrived");
        }
        if (bindingPtr)
            pushEl(pathFromOwner, *bindingPtr, el);
        else if (idPtr)
            pushEl(pathFromOwner, *idPtr, el);
        else
            Q_ASSERT(false);
        loadAnnotations(el);
        // avoid duplicate colon location for id?
        FileLocations::addRegion(nodeStack.last().fileLocations, u"colon", el->colonToken);
        return false;
    }

    void endVisit(AST::UiScriptBinding *) override
    {
        DomValue &lastEl = currentNode();
        index_type idx = currentIndex();
        if (lastEl.kind == DomType::Binding) {
            Binding &b = std::get<Binding>(lastEl.value);
            QmlObject &containingObject = current<QmlObject>();
            Binding *bPtr = valueFromMultimap(containingObject.m_bindings, b.name(), idx);
            Q_ASSERT(bPtr);
            *bPtr = b;
        } else if (lastEl.kind == DomType::Id) {
            Id &id = std::get<Id>(lastEl.value);
            QmlComponent &comp = current<QmlComponent>();
            Id *idPtr = valueFromMultimap(comp.m_ids, id.name, idx);
            *idPtr = id;
        } else {
            Q_ASSERT(false);
        }
        removeCurrentNode({});
    }

    bool visit(AST::UiArrayBinding *el) override
    {
        QList<QmlObject> value;
        Binding bindingV(toString(el->qualifiedId), value, BindingType::Normal);
        Binding *bindingPtr;
        Path bindingPathFromOwner =
                current<QmlObject>().addBinding(bindingV, AddOption::KeepExisting, &bindingPtr);
        if (bindingV.name() == u"id")
            qmlFile.addError(
                    myParseErrors()
                            .error(tr("id attributes should have only simple strings as values"))
                            .withPath(bindingPathFromOwner));
        pushEl(bindingPathFromOwner, *bindingPtr, el);
        FileLocations::addRegion(currentNodeEl().fileLocations, u"colon", el->colonToken);
        loadAnnotations(el);
        FileLocations::Tree arrayList =
                createMap(currentNodeEl().fileLocations, Path::Field(Fields::value), nullptr);
        FileLocations::addRegion(arrayList, u"leftSquareBrace", el->lbracketToken);
        FileLocations::addRegion(arrayList, u"rightSquareBrace", el->lbracketToken);
        arrayBindingLevels.append(nodeStack.length());
        return true;
    }

    void endVisit(AST::UiArrayBinding *) override
    {
        index_type idx = currentIndex();
        Binding &b = std::get<Binding>(currentNode().value);
        Binding *bPtr = valueFromMultimap(current<QmlObject>().m_bindings, b.name(), idx);
        *bPtr = b;
        arrayBindingLevels.removeLast();
        removeCurrentNode(DomType::Binding);
    }

    bool visit(AST::UiParameterList *el) override
    { // currently not used...
        MethodParameter p {
            el->name.toString(), toString(el->type), false, false, false, {}, {}, {}
        };
        return true;
    }
    void endVisit(AST::UiParameterList *el) override
    {
        Node::accept(el->next, this); // put other args at the same level as this one...
    }

    bool visit(AST::UiQualifiedId *) override { return false; }

    bool visit(AST::UiEnumDeclaration *el) override
    {
        EnumDecl eDecl;
        eDecl.setName(el->name.toString());
        EnumDecl *ePtr;
        Path enumPathFromOwner =
                current<QmlComponent>().addEnumeration(eDecl, AddOption::KeepExisting, &ePtr);
        pushEl(enumPathFromOwner, *ePtr, el);
        loadAnnotations(el);
        return true;
    }

    void endVisit(AST::UiEnumDeclaration *) override
    {
        EnumDecl &e = std::get<EnumDecl>(currentNode().value);
        EnumDecl *ePtr =
                valueFromMultimap(current<QmlComponent>().m_enumerations, e.name(), currentIndex());
        Q_ASSERT(ePtr);
        *ePtr = e;
        removeCurrentNode(DomType::EnumDecl);
    }

    bool visit(AST::UiEnumMemberList *el) override
    {
        EnumItem it(el->member.toString(), el->value);
        EnumDecl &eDecl = std::get<EnumDecl>(currentNode().value);
        Path itPathFromDecl = eDecl.addValue(it);
        FileLocations::addRegion(createMap(DomType::EnumItem, itPathFromDecl, nullptr), QString(),
                                 combine(el->memberToken, el->valueToken));
        return true;
    }

    void endVisit(AST::UiEnumMemberList *el) override
    {
        Node::accept(el->next, this); // put other enum members at the same level as this one...
    }

    bool visit(AST::UiInlineComponent *el) override
    {
        QStringList els = current<QmlComponent>().name().split(QLatin1Char('.'));
        els.append(el->name.toString());
        QString cName = els.join(QLatin1Char('.'));
        QmlComponent *compPtr;
        Path p = qmlFilePtr->addComponent(QmlComponent(cName), AddOption::KeepExisting, &compPtr);
        pushEl(p, *compPtr, el);
        FileLocations::addRegion(nodeStack.last().fileLocations, u"component", el->componentToken);
        loadAnnotations(el);
        return true;
    }

    void endVisit(AST::UiInlineComponent *) override
    {
        QmlComponent &component = std::get<QmlComponent>(currentNode().value);
        QStringList nameEls = component.name().split(QChar::fromLatin1('.'));
        QString key = nameEls.mid(1).join(QChar::fromLatin1('.'));
        QmlComponent *cPtr = valueFromMultimap(qmlFilePtr->m_components, key, currentIndex());
        Q_ASSERT(cPtr);
        *cPtr = component;
        removeCurrentNode(DomType::QmlComponent);
    }

    bool visit(UiRequired *el) override
    {
        PropertyDefinition pDef;
        pDef.name = el->name.toString();
        pDef.isRequired = true;
        PropertyDefinition *pDefPtr;
        Path pathFromOwner =
                current<QmlObject>().addPropertyDef(pDef, AddOption::KeepExisting, &pDefPtr);
        createMap(DomType::PropertyDefinition, pathFromOwner, el);
        return false;
    }

    bool visit(AST::UiAnnotation *el) override
    {
        QmlObject a;
        a.setName(QStringLiteral(u"@") + toString(el->qualifiedTypeNameId));
        // add annotation prototype?
        DomValue &containingElement = currentNode();
        Path pathFromOwner;
        QmlObject *aPtr = nullptr;
        switch (containingElement.kind) {
        case DomType::QmlObject:
            pathFromOwner = std::get<QmlObject>(containingElement.value).addAnnotation(a, &aPtr);
            break;
        case DomType::Binding:
            pathFromOwner = std::get<Binding>(containingElement.value)
                                    .addAnnotation(currentNodeEl().path, a, &aPtr);
            break;
        case DomType::Id:
            pathFromOwner = std::get<Id>(containingElement.value)
                                    .addAnnotation(currentNodeEl().path, a, &aPtr);
            break;
        case DomType::PropertyDefinition:
            pathFromOwner = std::get<PropertyDefinition>(containingElement.value)
                                    .addAnnotation(currentNodeEl().path, a, &aPtr);
            break;
        case DomType::MethodInfo:
            pathFromOwner = std::get<MethodInfo>(containingElement.value)
                                    .addAnnotation(currentNodeEl().path, a, &aPtr);
            break;
        default:
            qCWarning(domLog) << "Unexpected container object for annotation:"
                              << domTypeToString(containingElement.kind);
            Q_ASSERT(false);
        }
        pushEl(pathFromOwner, *aPtr, el);
        return true;
    }

    void endVisit(AST::UiAnnotation *) override
    {
        DomValue &containingElement = currentNode(1);
        Path pathFromOwner;
        QmlObject &a = std::get<QmlObject>(currentNode().value);
        switch (containingElement.kind) {
        case DomType::QmlObject:
            std::get<QmlObject>(containingElement.value).m_annotations[currentIndex()] = a;
            break;
        case DomType::Binding:
            std::get<Binding>(containingElement.value).m_annotations[currentIndex()] = a;
            break;
        case DomType::Id:
            std::get<Id>(containingElement.value).annotations[currentIndex()] = a;
            break;
        case DomType::PropertyDefinition:
            std::get<PropertyDefinition>(containingElement.value).annotations[currentIndex()] = a;
            break;
        case DomType::MethodInfo:
            std::get<MethodInfo>(containingElement.value).annotations[currentIndex()] = a;
            break;
        default:
            Q_ASSERT(false);
        }
        removeCurrentNode(DomType::QmlObject);
    }

    void throwRecursionDepthError() override
    {
        qmlFile.addError(myParseErrors().error(
                tr("Maximum statement or expression depth exceeded in QmlDomAstCreator")));
    }
};

void createDom(MutableDomItem qmlFile)
{
    if (std::shared_ptr<QmlFile> qmlFilePtr = qmlFile.ownerAs<QmlFile>()) {
        QmlDomAstCreator componentCreator(qmlFile);
        AST::Node::accept(qmlFilePtr->ast(), &componentCreator);
        AstComments::collectComments(qmlFile);
    } else {
        qCWarning(creatorLog) << "createDom called on non qmlFile";
    }
}

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
