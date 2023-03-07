// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqmldom_global.h"
#include "qqmldomastcreator_p.h"
#include "qqmldomelements_p.h"
#include "qqmldomitem_p.h"
#include "qqmldompath_p.h"
#include "qqmldomscriptelements_p.h"
#include "qqmldomtop_p.h"
#include "qqmldomerrormessage_p.h"
#include "qqmldomastdumper_p.h"
#include "qqmldomattachedinfo_p.h"
#include "qqmldomastcreator_p.h"

#include <QtQml/private/qqmljsast_p.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtCore/QLoggingCategory>

#include <memory>
#include <optional>
#include <type_traits>
#include <variant>

static Q_LOGGING_CATEGORY(creatorLog, "qt.qmldom.astcreator", QtWarningMsg);

/*
   Avoid crashing on files with JS-elements that are not implemented yet.
   Might be removed (definition + usages) once all script elements are implemented.
*/
#define Q_SCRIPTELEMENT_DISABLE()                                                      \
    do {                                                                               \
        qDebug() << "Could not construct the JS DOM at" << __FILE__ << ":" << __LINE__ \
                 << ", skipping JS elements...";                                       \
        disableScriptElements();                                                       \
    } while (false)

#define Q_SCRIPTELEMENT_EXIT_IF(check)            \
    do {                                          \
        if (m_enableScriptExpressions && check) { \
            Q_SCRIPTELEMENT_DISABLE();            \
            return;                               \
        }                                         \
    } while (false)

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

static ErrorGroups astParseErrors()
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

static QString typeToString(AST::Type *t)
{
    Q_ASSERT(t);
    QString res = toString(t->typeId);

    if (UiQualifiedId *arg = t->typeArgument)
        res += u'<' + toString(arg) + u'>';

    return res;
}

SourceLocation combineLocations(SourceLocation s1, SourceLocation s2)
{
    return combine(s1, s2);
}

SourceLocation combineLocations(Node *n)
{
    return combineLocations(n->firstSourceLocation(), n->lastSourceLocation());
}

QQmlDomAstCreator::QmlStackElement &QQmlDomAstCreator::currentQmlObjectOrComponentEl(int idx)
{
    Q_ASSERT_X(idx < nodeStack.size() && idx >= 0, "currentQmlObjectOrComponentEl",
               "Stack does not contain enough elements!");
    int i = nodeStack.size() - idx;
    while (i-- > 0) {
        DomType k = nodeStack.at(i).item.kind;
        if (k == DomType::QmlObject || k == DomType::QmlComponent)
            return nodeStack[i];
    }
    Q_ASSERT_X(false, "currentQmlObjectEl", "No QmlObject or component in stack");
    return nodeStack.last();
}

QQmlDomAstCreator::QmlStackElement &QQmlDomAstCreator::currentNodeEl(int i)
{
    Q_ASSERT_X(i < nodeStack.size() && i >= 0, "currentNode", "Stack does not contain element!");
    return nodeStack[nodeStack.size() - i - 1];
}

QQmlDomAstCreator::ScriptStackElement &QQmlDomAstCreator::currentScriptNodeEl(int i)
{
    Q_ASSERT_X(i < scriptNodeStack.size() && i >= 0, "currentNode",
               "Stack does not contain element!");
    return scriptNodeStack[scriptNodeStack.size() - i - 1];
}

QQmlDomAstCreator::DomValue &QQmlDomAstCreator::currentNode(int i)
{
    Q_ASSERT_X(i < nodeStack.size() && i >= 0, "currentNode",
               "Stack does not contain element!");
    return nodeStack[nodeStack.size() - i - 1].item;
}

void QQmlDomAstCreator::removeCurrentNode(std::optional<DomType> expectedType)
{
    Q_ASSERT_X(!nodeStack.isEmpty(), className, "popCurrentNode() without any node");
    if (expectedType)
        Q_ASSERT(nodeStack.last().item.kind == *expectedType);
    nodeStack.removeLast();
}

void QQmlDomAstCreator::removeCurrentScriptNode(std::optional<DomType> expectedType)
{
    Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
    Q_ASSERT_X(!scriptNodeStack.isEmpty(), className,
               "popCurrentScriptNode() without any node");
    if (expectedType)
        Q_ASSERT(scriptNodeStack.last().kind == *expectedType);
    scriptNodeStack.removeLast();
}

/*!
   \internal
   Prepares a script element DOM representation such that it can be used inside a QML DOM element.
 */
const ScriptElementVariant &
QQmlDomAstCreator::finalizeScriptExpression(const ScriptElementVariant &element,
                                            FileLocations::Tree base)
{
    auto e = element.base();
    Q_ASSERT(e);
    e->updatePathFromOwner(Path().field(Fields::scriptElement));
    if (!base)
        base = currentNodeEl().fileLocations;
    e->createFileLocations(base);
    return element;
}

FileLocations::Tree QQmlDomAstCreator::createMap(FileLocations::Tree base, Path p, AST::Node *n)
{
    FileLocations::Tree res = FileLocations::ensure(base, p, AttachedInfo::PathType::Relative);
    if (n)
        FileLocations::addRegion(res, QString(), combineLocations(n));
    return res;
}

FileLocations::Tree QQmlDomAstCreator::createMap(DomType k, Path p, AST::Node *n)
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
            qCWarning(domLog) << "unexpected type" << domTypeToString(currentNode().kind);
            Q_UNREACHABLE();
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
                Q_UNREACHABLE();
            }
        } else {
            qCWarning(domLog) << "unexpected path to QmlObject in createMap" << p;
            Q_UNREACHABLE();
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
        Q_UNREACHABLE();
        break;
    }
    return createMap(base, p, n);
}

QQmlDomAstCreator::QQmlDomAstCreator(MutableDomItem qmlFile)
    : qmlFile(qmlFile),
      qmlFilePtr(qmlFile.ownerAs<QmlFile>()),
      rootMap(qmlFilePtr->fileLocationsTree())
{
}

bool QQmlDomAstCreator::visit(UiProgram *program)
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
    if (!fInfo.canonicalPath().isEmpty()) {
        Import selfDirImport(QmlUri::fromDirectoryString(fInfo.canonicalPath()));
        selfDirImport.implicit = true;
        qmlFilePtr->addImport(selfDirImport);
    }
    for (Import i : qmlFile.environment().ownerAs<DomEnvironment>()->implicitImports()) {
        i.implicit = true;
        qmlFilePtr->addImport(i);
    }
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiProgram *)
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

bool QQmlDomAstCreator::visit(UiPragma *el)
{
    createMap(DomType::Pragma, qmlFilePtr->addPragma(Pragma(el->name.toString())), el);
    return true;
}

bool QQmlDomAstCreator::visit(UiImport *el)
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
                  qmlFilePtr->addImport(
                          Import::fromFileString(el->fileName.toString(), el->importId.toString())),
                  el);
    return true;
}

bool QQmlDomAstCreator::visit(AST::UiPublicMember *el)
{
    switch (el->type) {
    case AST::UiPublicMember::Signal: {
        MethodInfo m;
        m.name = el->name.toString();
        m.typeName = toString(el->memberType);
        m.isReadonly = el->isReadonly();
        m.access = MethodInfo::Public;
        m.methodType = MethodInfo::Signal;
        m.isList = el->typeModifier == QLatin1String("list");
        MethodInfo *mPtr;
        Path p = current<QmlObject>().addMethod(m, AddOption::KeepExisting, &mPtr);
        pushEl(p, *mPtr, el);
        FileLocations::addRegion(nodeStack.last().fileLocations, u"signal", el->propertyToken());
        MethodInfo &mInfo = std::get<MethodInfo>(currentNode().value);
        AST::UiParameterList *args = el->parameters;
        while (args) {
            MethodParameter param;
            param.name = args->name.toString();
            param.typeName = args->type ? args->type->toString() : QString();
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
        p.isReadonly = el->isReadonly();
        p.isDefaultMember = el->isDefaultMember();
        p.isRequired = el->isRequired();
        p.isList = el->typeModifier == QLatin1String("list");
        if (!el->typeModifier.isEmpty())
            p.typeName = el->typeModifier.toString() + QChar(u'<') + p.typeName + QChar(u'>');
        PropertyDefinition *pPtr;
        Path pPathFromOwner =
                current<QmlObject>().addPropertyDef(p, AddOption::KeepExisting, &pPtr);
        pushEl(pPathFromOwner, *pPtr, el);
        FileLocations::addRegion(nodeStack.last().fileLocations, u"property",
                                 el->propertyToken());
        if (p.name == u"id")
            qmlFile.addError(astParseErrors()
                                     .warning(tr("id is a special attribute, that should not be "
                                                 "used as property name"))
                                     .withPath(currentNodeEl().path));
        if (p.isDefaultMember)
            FileLocations::addRegion(nodeStack.last().fileLocations, u"default",
                                     el->defaultToken());
        if (p.isRequired)
            FileLocations::addRegion(nodeStack.last().fileLocations, u"required",
                                     el->requiredToken());
        if (el->statement) {
            BindingType bType = BindingType::Normal;
            SourceLocation loc = combineLocations(el->statement);
            QStringView code = qmlFilePtr->code();

            auto script = std::make_shared<ScriptExpression>(
                    code.mid(loc.offset, loc.length), qmlFilePtr->engine(), el->statement,
                    qmlFilePtr->astComments(), ScriptExpression::ExpressionType::BindingExpression,
                    loc);
            Binding *bPtr;
            Path bPathFromOwner = current<QmlObject>().addBinding(Binding(p.name, script, bType),
                                                                  AddOption::KeepExisting, &bPtr);
            FileLocations::Tree bLoc = createMap(DomType::Binding, bPathFromOwner, el);
            FileLocations::addRegion(bLoc, u"colon", el->colonToken);
            FileLocations::Tree valueLoc = FileLocations::ensure(bLoc, Path::Field(Fields::value),
                                                                 AttachedInfo::PathType::Relative);
            FileLocations::addRegion(valueLoc, QString(), combineLocations(el->statement));
            // push it also: its needed in endVisit to add the scriptNode to it
            // do not use pushEl to avoid recreating the already created "bLoc" Map
            nodeStack.append({ bPathFromOwner, *bPtr, bLoc });
        }
        break;
    }
    }
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiPublicMember *el)
{
    if (auto &lastEl = currentNode(); lastEl.kind == DomType::Binding) {
        Binding &b = std::get<Binding>(lastEl.value);
        if (m_enableScriptExpressions && scriptNodeStack.size() != 1)
            Q_SCRIPTELEMENT_DISABLE();
        if (m_enableScriptExpressions) {
            b.scriptExpressionValue()->setScriptElement(
                    finalizeScriptExpression(currentScriptNodeEl().takeVariant()));
            removeCurrentScriptNode({});
        }

        QmlObject &containingObject = current<QmlObject>();
        Binding *bPtr =
                valueFromMultimap(containingObject.m_bindings, b.name(), currentIndex());
        Q_ASSERT(bPtr);
        removeCurrentNode({});
    }
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
                    it->addAnnotation(currentEl<QmlObject>()
                                              .path.field(Fields::bindings)
                                              .key(pDef.name)
                                              .index(obj.m_bindings.values(pDef.name).size() - 1),
                                      ann);
                }
            }
        }
    }
    QmlObject &obj = current<QmlObject>();
    QmlStackElement &sEl = nodeStack.last();
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
        MethodInfo *mPtr = valueFromMultimap(obj.m_methods, m.name, sEl.path.last().headIndex());
        Q_ASSERT(mPtr);
        *mPtr = m;
    } break;
    default:
        Q_UNREACHABLE();
    }
    removeCurrentNode({});
}

bool QQmlDomAstCreator::visit(AST::UiSourceElement *el)
{
    QStringView code(qmlFilePtr->code());
    if (FunctionDeclaration *fDef = cast<FunctionDeclaration *>(el->sourceElement)) {
        MethodInfo m;
        m.name = fDef->name.toString();
        if (AST::TypeAnnotation *tAnn = fDef->typeAnnotation) {
            if (AST::Type *t = tAnn->type)
                m.typeName = typeToString(t);
        }
        m.access = MethodInfo::Public;
        m.methodType = MethodInfo::Method;
        if (fDef->body) {
            SourceLocation bodyLoc = combineLocations(fDef->body);
            SourceLocation methodLoc = combineLocations(el);
            QStringView preCode = code.mid(methodLoc.begin(), bodyLoc.begin() - methodLoc.begin());
            QStringView postCode = code.mid(bodyLoc.end(), methodLoc.end() - bodyLoc.end());
            m.body = std::make_shared<ScriptExpression>(
                    code.mid(bodyLoc.offset, bodyLoc.length), qmlFilePtr->engine(), fDef->body,
                    qmlFilePtr->astComments(), ScriptExpression::ExpressionType::FunctionBody,
                    bodyLoc, 0, preCode, postCode);
        }
        MethodInfo *mPtr;
        Path mPathFromOwner = current<QmlObject>().addMethod(m, AddOption::KeepExisting, &mPtr);
        pushEl(mPathFromOwner, *mPtr,
               fDef); // add at the start and use the normal recursive visit?
        FileLocations::Tree &fLoc = nodeStack.last().fileLocations;
        if (fDef->body) {
            auto bodyLoc = FileLocations::ensure(fLoc, Path::Field(Fields::body),
                                                 AttachedInfo::PathType::Relative);
            FileLocations::addRegion(bodyLoc, QString(), combineLocations(fDef->body));
        }
        if (fDef->lparenToken.length != 0)
            FileLocations::addRegion(fLoc, u"leftParen", fDef->lparenToken);
        if (fDef->rparenToken.length != 0)
            FileLocations::addRegion(fLoc, u"rightParen", fDef->rparenToken);
        if (fDef->lbraceToken.length != 0)
            FileLocations::addRegion(fLoc, u"leftBrace", fDef->lbraceToken);
        if (fDef->rbraceToken.length != 0)
            FileLocations::addRegion(fLoc, u"rightBrace", fDef->rbraceToken);
        loadAnnotations(el);
        MethodInfo &mInfo = std::get<MethodInfo>(currentNode().value);
        AST::FormalParameterList *args = fDef->formals;
        while (args) {
            MethodParameter param;
            param.name = args->element->bindingIdentifier.toString();
            if (AST::TypeAnnotation *tAnn = args->element->typeAnnotation) {
                if (AST::Type *t = tAnn->type)
                    param.typeName = typeToString(t);
            }
            if (args->element->initializer) {
                SourceLocation loc = combineLocations(args->element->initializer);
                // TODO: fill statements in endVisit for this scriptexpression
                auto script = std::make_shared<ScriptExpression>(
                        code.mid(loc.offset, loc.length), qmlFilePtr->engine(),
                        args->element->initializer, qmlFilePtr->astComments(),
                        ScriptExpression::ExpressionType::ArgInitializer, loc);
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
        return true;
    } else {
        qCWarning(creatorLog) << "unhandled source el:" << static_cast<AST::Node *>(el);
        Q_UNREACHABLE();
    }
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiSourceElement *el)
{
    MethodInfo &m = std::get<MethodInfo>(currentNode().value);
    if (FunctionDeclaration *fDef = cast<FunctionDeclaration *>(el->sourceElement);
        fDef && fDef->body) {
        FileLocations::Tree bodyLoc =
                FileLocations::find(currentNodeEl().fileLocations, Path().field(Fields::body));
        Q_ASSERT(bodyLoc);
        if (m_enableScriptExpressions && scriptNodeStack.size() != 1)
            Q_SCRIPTELEMENT_DISABLE();
        if (m_enableScriptExpressions) {
            if (currentScriptNodeEl().isList()) {
                // It is more intuitive to have functions with a block as a body instead of a list.
                auto body = makeScriptElement<ScriptElements::BlockStatement>(fDef->body);
                body->setStatements(currentScriptNodeEl().takeList());
                if (auto semanticScope = body->statements().semanticScope())
                    body->setSemanticScope(*semanticScope);
                m.body->setScriptElement(
                        finalizeScriptExpression(ScriptElementVariant::fromElement(body), bodyLoc));
            } else {
                m.body->setScriptElement(
                        finalizeScriptExpression(currentScriptNodeEl().takeVariant(), bodyLoc));
            }
            removeCurrentScriptNode({});
        }
    }
    QmlObject &obj = current<QmlObject>();
    MethodInfo *mPtr =
            valueFromMultimap(obj.m_methods, m.name, nodeStack.last().path.last().headIndex());
    Q_ASSERT(mPtr);
    *mPtr = m;
    removeCurrentNode(DomType::MethodInfo);
}

bool QQmlDomAstCreator::visit(AST::UiObjectDefinition *el)
{
    QmlObject scope;
    scope.setName(toString(el->qualifiedTypeNameId));
    scope.addPrototypePath(Paths::lookupTypePath(scope.name()));
    QmlObject *sPtr = nullptr;
    Path sPathFromOwner;
    if (!arrayBindingLevels.isEmpty() && nodeStack.size() == arrayBindingLevels.last()) {
        if (currentNode().kind == DomType::Binding) {
            QList<QmlObject> *vals = std::get<Binding>(currentNode().value).arrayValue();
            if (vals) {
                int idx = vals->size();
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
            sPathFromOwner = std::get<QmlComponent>(containingObject.value).addObject(scope, &sPtr);
            break;
        case DomType::QmlObject:
            sPathFromOwner = std::get<QmlObject>(containingObject.value).addChild(scope, &sPtr);
            break;
        default:
            Q_UNREACHABLE();
        }
    }
    Q_ASSERT_X(sPtr, className, "could not recover new scope");
    pushEl(sPathFromOwner, *sPtr, el);
    loadAnnotations(el);
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiObjectDefinition *)
{
    QmlObject &obj = current<QmlObject>();
    int idx = currentIndex();
    if (!arrayBindingLevels.isEmpty() && nodeStack.size() == arrayBindingLevels.last() + 1) {
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
                Q_UNREACHABLE();
            break;
        case DomType::QmlObject:
            if (p[p.length() - 2] == Path::Field(Fields::children))
                std::get<QmlObject>(containingObject.value).m_children[idx] = obj;
            else
                Q_UNREACHABLE();
            break;
        default:
            Q_UNREACHABLE();
        }
    }
    removeCurrentNode(DomType::QmlObject);
}

bool QQmlDomAstCreator::visit(AST::UiObjectBinding *el)
{
    BindingType bType = (el->hasOnToken ? BindingType::OnBinding : BindingType::Normal);
    QmlObject value;
    value.setName(toString(el->qualifiedTypeNameId));
    Binding *bPtr;
    Path bPathFromOwner = current<QmlObject>().addBinding(
            Binding(toString(el->qualifiedId), value, bType), AddOption::KeepExisting, &bPtr);
    if (bPtr->name() == u"id")
        qmlFile.addError(astParseErrors()
                                 .warning(tr("id attributes should only be a lower case letter "
                                             "followed by letters, numbers or underscore, "
                                             "assuming they refer to an id property"))
                                 .withPath(bPathFromOwner));
    pushEl(bPathFromOwner, *bPtr, el);
    FileLocations::addRegion(nodeStack.last().fileLocations, u"colon", el->colonToken);
    loadAnnotations(el);
    QmlObject *objValue = bPtr->objectValue();
    Q_ASSERT_X(objValue, className, "could not recover objectValue");
    objValue->setName(toString(el->qualifiedTypeNameId));
    objValue->addPrototypePath(Paths::lookupTypePath(objValue->name()));
    pushEl(bPathFromOwner.field(Fields::value), *objValue, el->initializer);
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiObjectBinding *)
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

bool QQmlDomAstCreator::visit(AST::UiScriptBinding *el)
{
    QStringView code = qmlFilePtr->code();
    SourceLocation loc = combineLocations(el->statement);
    auto script = std::make_shared<ScriptExpression>(
            code.mid(loc.offset, loc.length), qmlFilePtr->engine(), el->statement,
            qmlFilePtr->astComments(), ScriptExpression::ExpressionType::BindingExpression, loc);
    Binding bindingV(toString(el->qualifiedId), script, BindingType::Normal);
    Binding *bindingPtr = nullptr;
    Id *idPtr = nullptr;
    Path pathFromOwner;
    if (bindingV.name() == u"id") {
        Node *exp = script->ast();
        if (ExpressionStatement *eStat = cast<ExpressionStatement *>(script->ast()))
            exp = eStat->expression;
        if (IdentifierExpression *iExp = cast<IdentifierExpression *>(exp)) {
            QmlStackElement &containingObjectEl = currentEl<QmlObject>();
            QmlObject &containingObject = std::get<QmlObject>(containingObjectEl.item.value);
            QString idName = iExp->name.toString();
            Id idVal(idName, qmlFile.canonicalPath().path(containingObject.pathFromOwner()));
            containingObject.setIdStr(idName);
            FileLocations::addRegion(containingObjectEl.fileLocations, u"idToken",
                                     combineLocations(el->qualifiedId));
            FileLocations::addRegion(containingObjectEl.fileLocations, u"idColon", el->colonToken);
            FileLocations::addRegion(containingObjectEl.fileLocations, u"id",
                                     combineLocations(el->statement));
            QmlComponent &comp = current<QmlComponent>();
            pathFromOwner = comp.addId(idVal, AddOption::KeepExisting, &idPtr);
            QRegularExpression idRe(QRegularExpression::anchoredPattern(
                    QStringLiteral(uR"([[:lower:]][[:lower:][:upper:]0-9_]*)")));
            auto m = idRe.matchView(iExp->name);
            if (!m.hasMatch()) {
                qmlFile.addError(
                        astParseErrors()
                                .warning(tr("id attributes should only be a lower case letter "
                                            "followed by letters, numbers or underscore, not %1")
                                                 .arg(iExp->name))
                                .withPath(pathFromOwner));
            }
        } else {
            pathFromOwner =
                    current<QmlObject>().addBinding(bindingV, AddOption::KeepExisting, &bindingPtr);
            Q_ASSERT_X(bindingPtr, className, "binding could not be retrieved");
            qmlFile.addError(
                    astParseErrors()
                            .warning(tr("id attributes should only be a lower case letter "
                                        "followed by letters, numbers or underscore, not %1 "
                                        "%2, assuming they refer to a property")
                                             .arg(script->code(), script->astRelocatableDump()))
                            .withPath(pathFromOwner));
        }
    } else {
        pathFromOwner =
                current<QmlObject>().addBinding(bindingV, AddOption::KeepExisting, &bindingPtr);
        Q_ASSERT_X(bindingPtr, className, "binding could not be retrieved");
    }
    if (bindingPtr)
        pushEl(pathFromOwner, *bindingPtr, el);
    else if (idPtr)
        pushEl(pathFromOwner, *idPtr, el);
    else
        Q_UNREACHABLE();
    loadAnnotations(el);
    // avoid duplicate colon location for id?
    FileLocations::addRegion(nodeStack.last().fileLocations, u"colon", el->colonToken);
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiScriptBinding *)
{
    DomValue &lastEl = currentNode();
    index_type idx = currentIndex();
    if (lastEl.kind == DomType::Binding) {
        Binding &b = std::get<Binding>(lastEl.value);
        if (m_enableScriptExpressions
            && (scriptNodeStack.size() != 1 || currentScriptNodeEl().isList()))
            Q_SCRIPTELEMENT_DISABLE();
        if (m_enableScriptExpressions) {
            b.scriptExpressionValue()->setScriptElement(
                    finalizeScriptExpression(currentScriptNodeEl().takeVariant()));
            removeCurrentScriptNode({});
        }

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
        Q_UNREACHABLE();
    }
    removeCurrentNode({});
}

bool QQmlDomAstCreator::visit(AST::UiArrayBinding *el)
{
    QList<QmlObject> value;
    Binding bindingV(toString(el->qualifiedId), value, BindingType::Normal);
    Binding *bindingPtr;
    Path bindingPathFromOwner =
            current<QmlObject>().addBinding(bindingV, AddOption::KeepExisting, &bindingPtr);
    if (bindingV.name() == u"id")
        qmlFile.addError(
                astParseErrors()
                        .error(tr("id attributes should have only simple strings as values"))
                        .withPath(bindingPathFromOwner));
    pushEl(bindingPathFromOwner, *bindingPtr, el);
    FileLocations::addRegion(currentNodeEl().fileLocations, u"colon", el->colonToken);
    loadAnnotations(el);
    FileLocations::Tree arrayList =
            createMap(currentNodeEl().fileLocations, Path::Field(Fields::value), nullptr);
    FileLocations::addRegion(arrayList, u"leftSquareBrace", el->lbracketToken);
    FileLocations::addRegion(arrayList, u"rightSquareBrace", el->lbracketToken);
    arrayBindingLevels.append(nodeStack.size());
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiArrayBinding *)
{
    index_type idx = currentIndex();
    Binding &b = std::get<Binding>(currentNode().value);
    Binding *bPtr = valueFromMultimap(current<QmlObject>().m_bindings, b.name(), idx);
    *bPtr = b;
    arrayBindingLevels.removeLast();
    removeCurrentNode(DomType::Binding);
}

bool QQmlDomAstCreator::visit(AST::UiParameterList *el)
{ // currently not used...
    MethodParameter p{ el->name.toString(),
                       el->type ? el->type->toString() : QString(),
                       false,
                       false,
                       false,
                       {},
                       {},
                       {} };
    return true;
}
void QQmlDomAstCreator::endVisit(AST::UiParameterList *el)
{
    Node::accept(el->next, this); // put other args at the same level as this one...
}

bool QQmlDomAstCreator::visit(AST::UiQualifiedId *)
{
    return false;
}

bool QQmlDomAstCreator::visit(AST::UiEnumDeclaration *el)
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

void QQmlDomAstCreator::endVisit(AST::UiEnumDeclaration *)
{
    EnumDecl &e = std::get<EnumDecl>(currentNode().value);
    EnumDecl *ePtr =
            valueFromMultimap(current<QmlComponent>().m_enumerations, e.name(), currentIndex());
    Q_ASSERT(ePtr);
    *ePtr = e;
    removeCurrentNode(DomType::EnumDecl);
}

bool QQmlDomAstCreator::visit(AST::UiEnumMemberList *el)
{
    EnumItem it(el->member.toString(), el->value);
    EnumDecl &eDecl = std::get<EnumDecl>(currentNode().value);
    Path itPathFromDecl = eDecl.addValue(it);
    FileLocations::addRegion(createMap(DomType::EnumItem, itPathFromDecl, nullptr), QString(),
                             combine(el->memberToken, el->valueToken));
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiEnumMemberList *el)
{
    Node::accept(el->next, this); // put other enum members at the same level as this one...
}

bool QQmlDomAstCreator::visit(AST::UiInlineComponent *el)
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

void QQmlDomAstCreator::endVisit(AST::UiInlineComponent *)
{
    QmlComponent &component = std::get<QmlComponent>(currentNode().value);
    QStringList nameEls = component.name().split(QChar::fromLatin1('.'));
    QString key = nameEls.mid(1).join(QChar::fromLatin1('.'));
    QmlComponent *cPtr = valueFromMultimap(qmlFilePtr->m_components, key, currentIndex());
    Q_ASSERT(cPtr);
    *cPtr = component;
    removeCurrentNode(DomType::QmlComponent);
}

bool QQmlDomAstCreator::visit(UiRequired *el)
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

bool QQmlDomAstCreator::visit(AST::UiAnnotation *el)
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
        pathFromOwner =
                std::get<Id>(containingElement.value).addAnnotation(currentNodeEl().path, a, &aPtr);
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
        Q_UNREACHABLE();
    }
    pushEl(pathFromOwner, *aPtr, el);
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiAnnotation *)
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
        Q_UNREACHABLE();
    }
    removeCurrentNode(DomType::QmlObject);
}

void QQmlDomAstCreator::throwRecursionDepthError()
{
    qmlFile.addError(astParseErrors().error(
            tr("Maximum statement or expression depth exceeded in QmlDomAstCreator")));
}

bool QQmlDomAstCreator::visit(AST::StatementList *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::StatementList *list)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptList(list);

    for (auto it = list; it; it = it->next) {
        Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
        current.append(scriptNodeStack.takeLast().takeVariant());
    }

    current.reverse();
    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::BinaryExpression *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::BinaryExpression *exp)
{
    Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.size() < 2);

    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptElement<ScriptElements::BinaryExpression>(exp);
    current->setRight(currentScriptNodeEl().takeVariant());
    removeCurrentScriptNode({});
    current->setLeft(currentScriptNodeEl().takeVariant());
    removeCurrentScriptNode({});

    current->setOp(exp->op);
    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::Block *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::Block *block)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptElement<ScriptElements::BlockStatement>(block);

    if (block->statements) {
        Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
        current->setStatements(currentScriptNodeEl().takeList());
        removeCurrentScriptNode(DomType::List);
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::ForStatement *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::ForStatement *forStatement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptElement<ScriptElements::ForStatement>(forStatement);

    if (forStatement->statement) {
        Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
        current->setBody(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode(std::nullopt);
    }

    if (forStatement->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
        current->setExpression(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode(std::nullopt);
    }

    if (forStatement->condition) {
        Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
        current->setCondition(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode(std::nullopt);
    }

    if (forStatement->declarations) {
        Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
        current->setDeclarations(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode(DomType::ScriptVariableDeclaration);
    }

    if (forStatement->initialiser) {
        Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
        current->setInitializer(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode(std::nullopt);
    }
    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::IdentifierExpression *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    auto current = makeScriptElement<ScriptElements::IdentifierExpression>(expression);
    current->setName(expression->name);
    pushScriptElement(current);
    return true;
}

bool QQmlDomAstCreator::visit(AST::NumericLiteral *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    auto current = makeScriptElement<ScriptElements::Literal>(expression);
    current->setLiteralValue(expression->value);
    pushScriptElement(current);
    return true;
}

bool QQmlDomAstCreator::visit(AST::StringLiteral *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    auto current = makeScriptElement<ScriptElements::Literal>(expression);
    current->setLiteralValue(expression->value.toString());
    pushScriptElement(current);
    return true;
}

bool QQmlDomAstCreator::visit(AST::VariableDeclarationList *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::VariableDeclarationList *list)
{
    if (!m_enableScriptExpressions)
        return;

    auto declarationList = makeScriptList(list);

    for (auto it = list; it; it = it->next) {
        Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
        declarationList.append(scriptNodeStack.takeLast().takeVariant());
    }

    declarationList.reverse();

    auto current = makeScriptElement<ScriptElements::VariableDeclaration>(list);
    current->setDeclarations(declarationList);
    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::PatternElement *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::PatternElement *pe)
{
    if (!m_enableScriptExpressions)
        return;

    if (pe->isVariableDeclaration()) {
        auto variableDeclaration = makeScriptElement<ScriptElements::VariableDeclarationEntry>(pe);
        ScriptElements::VariableDeclarationEntry::ScopeType scopeType;
        switch (pe->scope) {
        case QQmlJS::AST::VariableScope::Const:
            scopeType = ScriptElements::VariableDeclarationEntry::Const;
            break;
        case QQmlJS::AST::VariableScope::Let:
            scopeType = ScriptElements::VariableDeclarationEntry::Let;
            break;
        case QQmlJS::AST::VariableScope::Var:
            scopeType = ScriptElements::VariableDeclarationEntry::Var;
            break;
        default:
            Q_UNREACHABLE();
        }
        variableDeclaration->setScopeType(scopeType);
        variableDeclaration->setIdentifier(pe->bindingIdentifier.toString());
        if (pe->initializer) {
            Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
            variableDeclaration->setInitializer(scriptNodeStack.takeLast().takeVariant());
        }
        pushScriptElement(variableDeclaration);
    }
}

bool QQmlDomAstCreator::visit(AST::IfStatement *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::IfStatement *ifStatement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptElement<ScriptElements::IfStatement>(ifStatement);

    if (ifStatement->ko) {
        Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
        current->setAlternative(scriptNodeStack.last().takeVariant());
        scriptNodeStack.removeLast();
    }

    if (ifStatement->ok) {
        Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
        current->setConsequence(scriptNodeStack.last().takeVariant());
        scriptNodeStack.removeLast();
    }
    if (ifStatement->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
        current->setCondition(scriptNodeStack.last().takeVariant());
        scriptNodeStack.removeLast();
    }

    pushScriptElement(current);
}

static const DomEnvironment *environmentFrom(MutableDomItem &qmlFile)
{
    auto top = qmlFile.top();
    if (!top) {
        return {};
    }
    auto domEnvironment = top.as<DomEnvironment>();
    if (!domEnvironment) {
        return {};
    }
    return domEnvironment;
}

static QStringList importPathsFrom(MutableDomItem &qmlFile)
{
    if (auto env = environmentFrom(qmlFile))
        return env->loadPaths();

    return {};
}

static QStringList qmldirFilesFrom(MutableDomItem &qmlFile)
{
    if (auto env = environmentFrom(qmlFile))
        return env->qmldirFiles();

    return {};
}

QQmlDomAstCreatorWithQQmlJSScope::QQmlDomAstCreatorWithQQmlJSScope(MutableDomItem &qmlFile,
                                                                   QQmlJSLogger *logger)
    : m_root(QQmlJSScope::create()),
      m_logger(logger),
      m_importer(importPathsFrom(qmlFile), nullptr, true),
      m_implicitImportDirectory(QQmlJSImportVisitor::implicitImportDirectory(
              m_logger->fileName(), m_importer.resourceFileMapper())),
      m_scopeCreator(m_root, &m_importer, m_logger, m_implicitImportDirectory,
                     qmldirFilesFrom(qmlFile)),
      m_domCreator(qmlFile)
{
}

#define X(name)                                                 \
    bool QQmlDomAstCreatorWithQQmlJSScope::visit(name *node)    \
    {                                                           \
        return visitT(node);                                    \
    }                                                           \
    void QQmlDomAstCreatorWithQQmlJSScope::endVisit(name *node) \
    {                                                           \
        endVisitT(node);                                        \
    }
QQmlJSASTClassListToVisit
#undef X

        void
        QQmlDomAstCreatorWithQQmlJSScope::setScopeInDom()
{
    QQmlJSScope::Ptr scope = m_scopeCreator.m_currentScope;
    if (!m_domCreator.scriptNodeStack.isEmpty()) {
        auto topOfStack = m_domCreator.currentScriptNodeEl();
        switch (topOfStack.kind) {
        case DomType::ScriptBlockStatement:
        case DomType::ScriptForStatement:
        case DomType::List:
            m_domCreator.currentScriptNodeEl().setSemanticScope(scope);
            break;
        // TODO: find which script elements also have a scope and implement them here
        default:
            break;
        };
    } else if (!m_domCreator.nodeStack.isEmpty()) {
        std::visit(
                [&scope](auto &&e) {
                    using U = std::remove_cv_t<std::remove_reference_t<decltype(e)>>;
                    // TODO: find which dom elements also have a scope and implement them here
                    if constexpr (std::is_same_v<U, QmlObject>) {
                        e.setSemanticScope(scope);
                    } else if constexpr (std::is_same_v<U, MethodInfo>) {
                        if (e.body) {
                            if (auto scriptElement = e.body->scriptElement())
                                scriptElement.base()->setSemanticScope(scope);
                        }
                        e.setSemanticScope(scope);
                    }
                },
                m_domCreator.currentNodeEl().item.value);
    }
}

void QQmlDomAstCreatorWithQQmlJSScope::throwRecursionDepthError()
{
}

void createDom(MutableDomItem qmlFile, DomCreationOptions options)
{
    if (std::shared_ptr<QmlFile> qmlFilePtr = qmlFile.ownerAs<QmlFile>()) {
        QQmlJSLogger logger; // TODO
        std::unique_ptr<BaseVisitor> visitor;
        if (options.testFlag(DomCreationOption::WithSemanticAnalysis)) {
            auto v = std::make_unique<QQmlDomAstCreatorWithQQmlJSScope>(qmlFile, &logger);
            v->enableScriptExpressions(options.testFlag(DomCreationOption::WithScriptExpressions));
            visitor = std::move(v);
        } else {
            auto v = std::make_unique<QQmlDomAstCreator>(qmlFile);
            v->enableScriptExpressions(options.testFlag(DomCreationOption::WithScriptExpressions));
            visitor = std::move(v);
        }
        AST::Node::accept(qmlFilePtr->ast(), visitor.get());
        AstComments::collectComments(qmlFile);
    } else {
        qCWarning(creatorLog) << "createDom called on non qmlFile";
    }
}

} // end namespace Dom
} // end namespace QQmlJS

#undef Q_SCRIPTELEMENT_DISABLE
#undef Q_SCRIPTELEMENT_EXIT_IF

QT_END_NAMESPACE
