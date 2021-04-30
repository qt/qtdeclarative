/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmljsimportvisitor_p.h"
#include "qqmljsresourcefilemapper_p.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qqueue.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace QQmlJS::AST;

/*!
  \internal
  Sets the name of \a scope to \a name based on \a type.
*/
inline void setScopeName(QQmlJSScope::Ptr &scope, QQmlJSScope::ScopeType type, const QString &name)
{
    Q_ASSERT(scope);
    if (type == QQmlJSScope::GroupedPropertyScope || type == QQmlJSScope::AttachedPropertyScope)
        scope->setInternalName(name);
    else
        scope->setBaseTypeName(name);
}

/*!
  \internal
  Returns the name of \a scope based on \a type.
*/
inline QString getScopeName(const QQmlJSScope::ConstPtr &scope, QQmlJSScope::ScopeType type)
{
    Q_ASSERT(scope);
    if (type == QQmlJSScope::GroupedPropertyScope || type == QQmlJSScope::AttachedPropertyScope)
        return scope->internalName();

    return scope->baseTypeName();
}

QQmlJSImportVisitor::QQmlJSImportVisitor(
        QQmlJSImporter *importer, const QString &implicitImportDirectory,
        const QStringList &qmltypesFiles, const QString &fileName, const QString &code, bool silent)
    : m_implicitImportDirectory(implicitImportDirectory)
    , m_qmltypesFiles(qmltypesFiles)
    , m_currentScope(QQmlJSScope::create(QQmlJSScope::JSFunctionScope))
    , m_importer(importer)
    , m_logger(fileName, code, silent)
{
    m_globalScope = m_currentScope;
    m_currentScope->setIsComposite(true);
}

void QQmlJSImportVisitor::enterEnvironment(QQmlJSScope::ScopeType type, const QString &name,
                                           const QQmlJS::SourceLocation &location)
{
    m_currentScope = QQmlJSScope::create(type, m_currentScope);
    setScopeName(m_currentScope, type, name);
    m_currentScope->setIsComposite(true);
    m_currentScope->setSourceLocation(location);
}

bool QQmlJSImportVisitor::enterEnvironmentNonUnique(QQmlJSScope::ScopeType type,
                                                    const QString &name,
                                                    const QQmlJS::SourceLocation &location)
{
    Q_ASSERT(type == QQmlJSScope::GroupedPropertyScope
             || type == QQmlJSScope::AttachedPropertyScope);

    const auto pred = [&](const QQmlJSScope::ConstPtr &s) {
        // it's either attached or group property, so use internalName()
        // directly. see setScopeName() for details
        return s->internalName() == name;
    };
    const auto scopes = m_currentScope->childScopes();
    // TODO: linear search. might want to make childScopes() a set/hash-set and
    // use faster algorithm here
    auto it = std::find_if(scopes.begin(), scopes.end(), pred);
    if (it == scopes.end()) {
        // create and enter new scope
        enterEnvironment(type, name, location);
        return false;
    }
    // enter found scope
    m_currentScope = *it;
    return true;
}

void QQmlJSImportVisitor::leaveEnvironment()
{
    m_currentScope = m_currentScope->parentScope();
}

void QQmlJSImportVisitor::resolveAliases()
{
    QQueue<QQmlJSScope::Ptr> objects;
    objects.enqueue(m_exportedRootScope);

    qsizetype lastRequeueLength = std::numeric_limits<qsizetype>::max();
    QQueue<QQmlJSScope::Ptr> requeue;

    while (!objects.isEmpty()) {
        const QQmlJSScope::Ptr object = objects.dequeue();
        const auto properties = object->ownProperties();

        bool doRequeue = false;
        for (auto property : properties) {
            if (!property.isAlias() || !property.type().isNull())
                continue;

            QStringList components = property.typeName().split(u'.');
            QQmlJSScope::ConstPtr type;
            QQmlJSMetaProperty targetProperty;

            // The first component has to be an ID. Find the object it refers to.
            const auto it = m_scopesById.find(components.takeFirst());
            if (it != m_scopesById.end()) {
                type = *it;

                // Any further components are nested properties of that object.
                // Technically we can only resolve a limited depth in the engine, but the rules
                // on that are fuzzy and subject to change. Let's ignore it for now.
                // If the target is itself an alias and has not been resolved, re-queue the object
                // and try again later.
                while (type && !components.isEmpty()) {
                    const auto target = type->property(components.takeFirst());
                    if (!target.type() && target.isAlias())
                        doRequeue = true;
                    type = target.type();
                    targetProperty = target;
                }
            }

            if (type.isNull()) {
                if (doRequeue)
                    continue;
                m_logger.log(QStringLiteral("Cannot deduce type of alias \"%1\"")
                                        .arg(property.propertyName()), Log_Alias, object->sourceLocation());
            } else {
                property.setType(type);
                // Copy additional property information from target
                property.setIsList(targetProperty.isList());
                property.setIsWritable(targetProperty.isWritable());
                property.setIsPointer(targetProperty.isPointer());

                if (const QString internalName = type->internalName(); !internalName.isEmpty())
                    property.setTypeName(internalName);
            }

            object->addOwnProperty(property);
        }

        const auto childScopes = object->childScopes();
        for (const auto &childScope : childScopes)
            objects.enqueue(childScope);

        if (doRequeue)
            requeue.enqueue(object);

        if (objects.isEmpty() && requeue.length() < lastRequeueLength) {
            lastRequeueLength = requeue.length();
            objects.swap(requeue);
        }
    }

    while (!requeue.isEmpty()) {
        const QQmlJSScope::Ptr object = requeue.dequeue();
        const auto properties = object->ownProperties();
        for (const auto &property : properties) {
            if (!property.isAlias() || property.type())
                continue;
           m_logger.log(QStringLiteral("Alias \"%1\" is part of an alias cycle")
                                    .arg(property.propertyName()),
                                Log_Alias,
                                object->sourceLocation());
        }
    }
}

QQmlJSScope::Ptr QQmlJSImportVisitor::result() const
{
    return m_exportedRootScope;
}

QString QQmlJSImportVisitor::implicitImportDirectory(
        const QString &localFile, QQmlJSResourceFileMapper *mapper)
{
    if (mapper) {
        const auto resource = mapper->entry(
                    QQmlJSResourceFileMapper::localFileFilter(localFile));
        if (resource.isValid()) {
            return resource.resourcePath.contains(u'/')
                    ? (u':' + resource.resourcePath.left(
                           resource.resourcePath.lastIndexOf(u'/') + 1))
                    : QStringLiteral(":/");
        }
    }

    return QFileInfo(localFile).canonicalPath() + u'/';
}

void QQmlJSImportVisitor::processImportWarnings(const QString &what, const QQmlJS::SourceLocation &srcLocation)
{
    const auto warnings = m_importer->takeWarnings();

    if (warnings.isEmpty())
        return;

    m_logger.log(QStringLiteral("Warnings occurred while importing %1:").arg(what), Log_Import, srcLocation);
    m_logger.processMessages(warnings, Log_Import);
}

void QQmlJSImportVisitor::importBaseModules()
{
    Q_ASSERT(m_rootScopeImports.isEmpty());
    m_rootScopeImports = m_importer->importBuiltins();

    const QQmlJS::SourceLocation invalidLoc;
    for (const QString &name : m_rootScopeImports.keys()) {
        addImportWithLocation(name, invalidLoc);
    }

    if (!m_qmltypesFiles.isEmpty())
        m_importer->importQmltypes(m_qmltypesFiles);

    m_rootScopeImports.insert(m_importer->importDirectory(m_implicitImportDirectory));
    processImportWarnings(QStringLiteral("base modules"));
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiProgram *)
{
    importBaseModules();
    return true;
}

void QQmlJSImportVisitor::endVisit(UiProgram *)
{
    resolveAliases();
}

static QQmlJSAnnotation::Value bindingToVariant(QQmlJS::AST::Statement *statement)
{
    ExpressionStatement *expr = cast<ExpressionStatement *>(statement);

    if (!statement || !expr->expression)
        return {};

    switch (expr->expression->kind) {
    case Node::Kind_StringLiteral:
        return cast<StringLiteral *>(expr->expression)->value.toString();
    case Node::Kind_NumericLiteral:
        return cast<NumericLiteral *>(expr->expression)->value;
    default:
        return {};
    }
}

QVector<QQmlJSAnnotation> QQmlJSImportVisitor::parseAnnotations(QQmlJS::AST::UiAnnotationList *list)
{

    QVector<QQmlJSAnnotation> annotationList;

    for (UiAnnotationList *item = list; item != nullptr; item = item->next) {
        UiAnnotation *annotation = item->annotation;

        QString name;
        for (auto id = annotation->qualifiedTypeNameId; id; id = id->next)
            name += id->name.toString() + QLatin1Char('.');

        name.chop(1);



        QQmlJSAnnotation qqmljsAnnotation;

        qqmljsAnnotation.name = name;

        for (UiObjectMemberList *memberItem = annotation->initializer->members; memberItem != nullptr; memberItem = memberItem->next) {
            switch (memberItem->member->kind) {
            case Node::Kind_UiScriptBinding: {
                auto *scriptBinding = QQmlJS::AST::cast<UiScriptBinding*>(memberItem->member);
                QString bindingName;
                for (auto id = scriptBinding->qualifiedId; id; id = id->next)
                    bindingName += id->name.toString() + QLatin1Char('.');

                bindingName.chop(1);

                qqmljsAnnotation.bindings[bindingName] = bindingToVariant(scriptBinding->statement);
                break;
            }
            default:
                // We ignore all the other information contained in the annotation
                break;
            }
        }

        annotationList.append(qqmljsAnnotation);
    }

    return annotationList;
}


bool QQmlJSImportVisitor::visit(UiObjectDefinition *definition)
{
    QString superType;
    for (auto segment = definition->qualifiedTypeNameId; segment; segment = segment->next) {
        if (!superType.isEmpty())
            superType.append(u'.');
        superType.append(segment->name.toString());
    }
    enterEnvironment(QQmlJSScope::QMLScope, superType, definition->firstSourceLocation());
    if (!m_exportedRootScope)
        m_exportedRootScope = m_currentScope;

    m_currentScope->setAnnotations(parseAnnotations(definition->annotations));

    QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports, &m_usedTypes);
    return true;
}

void QQmlJSImportVisitor::endVisit(UiObjectDefinition *)
{
    QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports, &m_usedTypes);
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(UiPublicMember *publicMember)
{
    switch (publicMember->type) {
    case UiPublicMember::Signal: {
        UiParameterList *param = publicMember->parameters;
        QQmlJSMetaMethod method;
        method.setMethodType(QQmlJSMetaMethod::Signal);
        method.setMethodName(publicMember->name.toString());
        while (param) {
            method.addParameter(param->name.toString(), param->type->name.toString());
            param = param->next;
        }
        m_currentScope->addOwnMethod(method);
        break;
    }
    case UiPublicMember::Property: {
        QString typeName = publicMember->memberType
                ? publicMember->memberType->name.toString()
                : QString();
        const bool isAlias = (typeName == QLatin1String("alias"));
        if (isAlias) {
            typeName.clear();
            const auto expression = cast<ExpressionStatement *>(publicMember->statement);
            auto node = expression->expression;
            auto fex = cast<FieldMemberExpression *>(node);
            while (fex) {
                node = fex->base;
                typeName.prepend(u'.' + fex->name);
                fex = cast<FieldMemberExpression *>(node);
            }

            if (const auto idExpression = cast<IdentifierExpression *>(node)) {
                typeName.prepend(idExpression->name.toString());
            } else {
                m_logger.log(QStringLiteral("Invalid alias expression. Only IDs and field "
                                                   "member expressions can be aliased."),
                                Log_Alias,
                                expression->firstSourceLocation());
            }
        }
        QQmlJSMetaProperty prop;
        prop.setPropertyName(publicMember->name.toString());
        prop.setIsList(publicMember->typeModifier == QLatin1String("list"));
        prop.setIsWritable(!publicMember->isReadonlyMember);
        prop.setIsAlias(isAlias);
        if (const auto type = m_rootScopeImports.value(typeName)) {
            prop.setType(type);
            const QString internalName = type->internalName();
            prop.setTypeName(internalName.isEmpty() ? typeName : internalName);
        } else {
            prop.setTypeName(typeName);
        }
        prop.setAnnotations(parseAnnotations(publicMember->annotations));
        if (publicMember->isDefaultMember)
            m_currentScope->setDefaultPropertyName(prop.propertyName());
        m_currentScope->insertPropertyIdentifier(prop);
        if (publicMember->isRequired)
            m_currentScope->setPropertyLocallyRequired(prop.propertyName(), true);
        break;
    }
    }
    return true;
}

bool QQmlJSImportVisitor::visit(UiRequired *required)
{
    const QString name = required->name.toString();

    // The required property must be defined in some scope
    if (!m_currentScope->hasProperty(name)) {
        m_logger.log(QStringLiteral("Property \"%1\" was marked as required but does not exist.").arg(name),
                     Log_Required,
                     required->firstSourceLocation());
        return true;
    }

    m_currentScope->setPropertyLocallyRequired(name, true);
    return true;
}

void QQmlJSImportVisitor::visitFunctionExpressionHelper(QQmlJS::AST::FunctionExpression *fexpr)
{
    using namespace QQmlJS::AST;
    auto name = fexpr->name.toString();
    if (!name.isEmpty()) {
        QQmlJSMetaMethod method(name);
        method.setMethodType(QQmlJSMetaMethod::Method);

        if (!m_pendingMethodAnnotations.isEmpty()) {
            method.setAnnotations(m_pendingMethodAnnotations);
            m_pendingMethodAnnotations.clear();
        }

        if (const auto *formals = fexpr->formals) {
            const auto parameters = formals->formals();
            for (const auto &parameter : parameters) {
                const QString type = parameter.typeName();
                method.addParameter(parameter.id,
                                    type.isEmpty() ? QStringLiteral("var") : type);
            }
        }
        method.setReturnTypeName(fexpr->typeAnnotation
                                 ? fexpr->typeAnnotation->type->toString()
                                 : QStringLiteral("var"));
        m_currentScope->addOwnMethod(method);

        if (m_currentScope->scopeType() != QQmlJSScope::QMLScope) {
            m_currentScope->insertJSIdentifier(
                        name, {
                            QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
                            fexpr->firstSourceLocation()
                        });
        }
        enterEnvironment(QQmlJSScope::JSFunctionScope, name, fexpr->firstSourceLocation());
    } else {
        enterEnvironment(QQmlJSScope::JSFunctionScope, QStringLiteral("<anon>"),
                         fexpr->firstSourceLocation());
    }
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::FunctionExpression *fexpr)
{
    visitFunctionExpressionHelper(fexpr);
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::FunctionExpression *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiSourceElement *srcElement)
{
    m_pendingMethodAnnotations = parseAnnotations(srcElement->annotations);
    return true;
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::FunctionDeclaration *fdecl)
{
    visitFunctionExpressionHelper(fdecl);
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::FunctionDeclaration *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::ClassExpression *ast)
{
    QQmlJSMetaProperty prop;
    prop.setPropertyName(ast->name.toString());
    m_currentScope->addOwnProperty(prop);
    enterEnvironment(QQmlJSScope::JSFunctionScope, ast->name.toString(),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::ClassExpression *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(UiScriptBinding *scriptBinding)
{
    const auto id = scriptBinding->qualifiedId;
    const auto *statement = cast<ExpressionStatement *>(scriptBinding->statement);
    if (!id->next && id->name == QLatin1String("id")) {
        const auto *idExpression = cast<IdentifierExpression *>(statement->expression);
        m_scopesById.insert(idExpression->name.toString(), m_currentScope);
    } else {
        for (auto group = id; group->next; group = group->next) {
            const QString name = group->name.toString();

            if (name.isEmpty())
                break;

            enterEnvironmentNonUnique(name.front().isUpper() ? QQmlJSScope::AttachedPropertyScope
                                                             : QQmlJSScope::GroupedPropertyScope,
                                      name, group->firstSourceLocation());
        }

        // TODO: remember the actual binding, once we can process it.

        while (m_currentScope->scopeType() == QQmlJSScope::GroupedPropertyScope
               || m_currentScope->scopeType() == QQmlJSScope::AttachedPropertyScope) {
            leaveEnvironment();
        }

        if (!statement || !statement->expression->asFunctionDefinition()) {
            enterEnvironment(QQmlJSScope::JSFunctionScope, QStringLiteral("binding"),
                             scriptBinding->statement->firstSourceLocation());
        }
    }

    return true;
}

void QQmlJSImportVisitor::endVisit(UiScriptBinding *scriptBinding)
{
    const auto id = scriptBinding->qualifiedId;
    if (id->next || id->name != QLatin1String("id")) {
        const auto *statement = cast<ExpressionStatement *>(scriptBinding->statement);
        if (!statement || !statement->expression->asFunctionDefinition())
            leaveEnvironment();
    }
}

bool QQmlJSImportVisitor::visit(UiArrayBinding *arrayBinding)
{
    QString name;
    for (auto id = arrayBinding->qualifiedId; id; id = id->next)
        name += id->name.toString() + QLatin1Char('.');

    name.chop(1);

    enterEnvironment(QQmlJSScope::QMLScope, name, arrayBinding->firstSourceLocation());
    m_currentScope->setIsArrayScope(true);

    return true;
}

void QQmlJSImportVisitor::endVisit(UiArrayBinding *)
{
    leaveEnvironment();

    // TODO: Actually generate a binding from the scope
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiEnumDeclaration *uied)
{
    QQmlJSMetaEnum qmlEnum(uied->name.toString());
    for (const auto *member = uied->members; member; member = member->next) {
        qmlEnum.addKey(member->member.toString());
        qmlEnum.addValue(int(member->value));
    }
    m_currentScope->addOwnEnumeration(qmlEnum);
    return true;
}

void QQmlJSImportVisitor::addImportWithLocation(const QString &name,
                                                const QQmlJS::SourceLocation &loc)
{
    if (m_importTypeLocationMap.contains(name)
        && m_importTypeLocationMap.values(name).contains(loc))
        return;

    m_importTypeLocationMap.insert(name, loc);
    m_importLocations.insert(loc);
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiImport *import)
{
    auto addImportLocation = [this, import](const QString &name) {
        addImportWithLocation(name, import->firstSourceLocation());
    };
    // construct path
    QString prefix = QLatin1String("");
    if (import->asToken.isValid()) {
        prefix += import->importId;
    }
    auto filename = import->fileName.toString();
    if (!filename.isEmpty()) {
        const QFileInfo file(filename);
        const QString absolute = file.isRelative()
                ? QDir(m_implicitImportDirectory).filePath(filename)
                : filename;

        if (absolute.startsWith(u':')) {
            if (m_importer->resourceFileMapper()) {
                if (m_importer->resourceFileMapper()->isFile(absolute.mid(1))) {
                    const auto entry = m_importer->resourceFileMapper()->entry(
                                QQmlJSResourceFileMapper::resourceFileFilter(absolute.mid(1)));
                    const auto scope = m_importer->importFile(entry.filePath);
                    const QString actualPrefix = prefix.isEmpty()
                            ? QFileInfo(entry.resourcePath).baseName()
                            : prefix;
                    m_rootScopeImports.insert(actualPrefix, scope);

                    addImportLocation(actualPrefix);
                } else {
                    const auto scopes = m_importer->importDirectory(absolute, prefix);
                    m_rootScopeImports.insert(scopes);
                    for (const QString &key : scopes.keys())
                        addImportLocation(key);
                }
            }

            processImportWarnings(QStringLiteral("URL \"%1\"").arg(absolute), import->firstSourceLocation());
            return true;
        }

        QFileInfo path(absolute);
        if (path.isDir()) {
            const auto scopes = m_importer->importDirectory(path.canonicalFilePath(), prefix);
            m_rootScopeImports.insert(scopes);
            for (const QString &key : scopes.keys())
                addImportLocation(key);
        } else if (path.isFile()) {
            const auto scope = m_importer->importFile(path.canonicalFilePath());
            const QString actualPrefix = prefix.isEmpty() ? scope->internalName() : prefix;
            m_rootScopeImports.insert(actualPrefix, scope);
            addImportLocation(actualPrefix);
        }

        processImportWarnings(QStringLiteral("path \"%1\"").arg(path.canonicalFilePath()), import->firstSourceLocation());
        return true;
    }

    QString path {};
    auto uri = import->importUri;
    while (uri) {
        path.append(uri->name);
        path.append(u'/');
        uri = uri->next;
    }
    path.chop(1);

    const auto imported = m_importer->importModule(
                path, prefix, import->version ? import->version->version : QTypeRevision());

    m_rootScopeImports.insert(imported);
    for (const QString &key : imported.keys())
        addImportLocation(key);

    processImportWarnings(QStringLiteral("module \"%1\"").arg(path), import->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::throwRecursionDepthError()
{
    m_logger.log(QStringLiteral("Maximum statement or expression depth exceeded"),
                        Log_RecursionDepthError);
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::ClassDeclaration *ast)
{
    enterEnvironment(QQmlJSScope::JSFunctionScope, ast->name.toString(),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::ClassDeclaration *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::ForStatement *ast)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("forloop"),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::ForStatement *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::ForEachStatement *ast)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("foreachloop"),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::ForEachStatement *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::Block *ast)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("block"),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::Block *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::CaseBlock *ast)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("case"),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::CaseBlock *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::Catch *catchStatement)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("catch"),
                     catchStatement->firstSourceLocation());
    m_currentScope->insertJSIdentifier(
                catchStatement->patternElement->bindingIdentifier.toString(), {
                    QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
                    catchStatement->patternElement->firstSourceLocation()
                });
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::Catch *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::WithStatement *ast)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("with"),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::WithStatement *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::VariableDeclarationList *vdl)
{
    while (vdl) {
        m_currentScope->insertJSIdentifier(
                    vdl->declaration->bindingIdentifier.toString(),
                    {
                        (vdl->declaration->scope == QQmlJS::AST::VariableScope::Var)
                            ? QQmlJSScope::JavaScriptIdentifier::FunctionScoped
                            : QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
                        vdl->declaration->firstSourceLocation()
                    });
        vdl = vdl->next;
    }
    return true;
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::FormalParameterList *fpl)
{
    for (auto const &boundName : fpl->boundNames()) {
        m_currentScope->insertJSIdentifier(
                    boundName.id, {
                        QQmlJSScope::JavaScriptIdentifier::Parameter,
                        fpl->firstSourceLocation()
                    });
    }
    return true;
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiObjectBinding *uiob)
{
    // ... __styleData: QtObject {...}

    Q_ASSERT(uiob->qualifiedTypeNameId);
    QString name;
    for (auto id = uiob->qualifiedTypeNameId; id; id = id->next)
        name += id->name.toString() + QLatin1Char('.');

    name.chop(1);

    bool needsResolution = false;
    for (auto group = uiob->qualifiedId; group->next; group = group->next) {
        const QString idName = group->name.toString();

        if (idName.isEmpty())
            break;

        const auto scopeKind = idName.front().isUpper() ? QQmlJSScope::AttachedPropertyScope
                                                        : QQmlJSScope::GroupedPropertyScope;
        bool exists = enterEnvironmentNonUnique(scopeKind, idName, group->firstSourceLocation());
        needsResolution = needsResolution || !exists;
    }

    while (m_currentScope->scopeType() == QQmlJSScope::GroupedPropertyScope
           || m_currentScope->scopeType() == QQmlJSScope::AttachedPropertyScope) {
        leaveEnvironment();
    }

    // recursively resolve types for current scope if new scopes are found
    if (needsResolution)
        QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports, &m_usedTypes);

    enterEnvironment(QQmlJSScope::QMLScope, name,
                     uiob->qualifiedTypeNameId->identifierToken);
    QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports, &m_usedTypes);
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::UiObjectBinding *uiob)
{
    QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports, &m_usedTypes);
    const QQmlJSScope::ConstPtr childScope = m_currentScope;
    leaveEnvironment();

    auto group = uiob->qualifiedId;
    for (; group->next; group = group->next) {
        const QString idName = group->name.toString();

        if (idName.isEmpty())
            break;

        const auto scopeKind = idName.front().isUpper() ? QQmlJSScope::AttachedPropertyScope
                                                        : QQmlJSScope::GroupedPropertyScope;
        // definitely exists
        enterEnvironmentNonUnique(scopeKind, idName, group->firstSourceLocation());
    }

    // on ending the visit to UiObjectBinding, set the property type to the
    // just-visited one if the property exists and this type is valid

    const QString propertyName = group->name.toString();

    QQmlJSMetaProperty property = m_currentScope->property(propertyName);

    if (m_currentScope->isInCustomParserParent()) {
        // These warnings do not apply for custom parsers and their children and need to be handled
        // on a case by case basis
    } else if (property.isValid() && !property.type().isNull()
               && (uiob->hasOnToken || property.type()->canAssign(childScope))) {

        QQmlJSMetaPropertyBinding binding = m_currentScope->hasOwnPropertyBinding(propertyName)
                ? m_currentScope->ownPropertyBinding(propertyName)
                : QQmlJSMetaPropertyBinding(property);

        const QString typeName = getScopeName(childScope, QQmlJSScope::QMLScope);

        if (uiob->hasOnToken) {
            if (childScope->hasInterface(QStringLiteral("QQmlPropertyValueInterceptor"))) {
                if (binding.hasInterceptor()) {
                    m_logger.log(QStringLiteral("Duplicate interceptor on property \"%1\"")
                                         .arg(propertyName),
                                 Log_Property, uiob->firstSourceLocation());
                } else {
                    binding.setInterceptor(childScope);
                    binding.setInterceptorTypeName(typeName);

                    m_currentScope->addOwnPropertyBinding(binding);
                }
            } else if (childScope->hasInterface(QStringLiteral("QQmlPropertyValueSource"))) {
                if (binding.hasValueSource()) {
                    m_logger.log(QStringLiteral("Duplicate value source on property \"%1\"")
                                         .arg(propertyName),
                                 Log_Property, uiob->firstSourceLocation());
                } else if (binding.hasValue()) {
                    m_logger.log(
                            QStringLiteral(
                                    "Cannot combine value source and binding on property \"%1\"")
                                    .arg(propertyName),
                            Log_Property, uiob->firstSourceLocation());
                } else {
                    binding.setValueSource(childScope);
                    binding.setValueSourceTypeName(typeName);
                    m_currentScope->addOwnPropertyBinding(binding);
                }
            } else {
                m_logger.log(QStringLiteral("On-binding for property \"%1\" has wrong type \"%2\"")
                                     .arg(propertyName)
                                     .arg(typeName),
                             Log_Property, uiob->firstSourceLocation());
            }
        } else {
            // TODO: Warn here if binding.hasValue() is true
            if (binding.hasValueSource()) {
                m_logger.log(
                        QStringLiteral("Cannot combine value source and binding on property \"%1\"")
                                .arg(propertyName),
                        Log_Property, uiob->firstSourceLocation());
            } else {
                binding.setValue(childScope);
                binding.setValueTypeName(typeName);
                m_currentScope->addOwnPropertyBinding(binding);
            }
        }
    } else if (!m_currentScope->isFullyResolved()) {
        // If the current scope is not fully resolved we cannot tell whether the property exists or
        // not (we already warn elsewhere)
    } else if (!property.isValid()) {
        m_logger.log(
                QStringLiteral("Property \"%1\" is invalid or does not exist").arg(propertyName),
                Log_Property, group->firstSourceLocation());
    } else if (property.type().isNull() || !property.type()->isFullyResolved()) {
        // Property type is not fully resolved we cannot tell any more than this
        m_logger.log(
                QStringLiteral(
                        "Property \"%1\" has incomplete type \"%2\". You may be missing an import.")
                        .arg(propertyName)
                        .arg(property.typeName()),
                Log_Property, group->firstSourceLocation());
    } else if (!childScope->isFullyResolved()) {
        // If the childScope type is not fully resolved we cannot tell whether the type is
        // incompatible (we already warn elsewhere)
    } else {
        // the type is incompatible
        m_logger.log(
                QStringLiteral(
                        "Property \"%1\" of type \"%2\" is assigned an incompatible type \"%3\"")
                        .arg(propertyName)
                        .arg(property.typeName())
                        .arg(getScopeName(childScope, QQmlJSScope::QMLScope)),
                Log_Property, group->firstSourceLocation());
    }

    while (m_currentScope->scopeType() == QQmlJSScope::GroupedPropertyScope
           || m_currentScope->scopeType() == QQmlJSScope::AttachedPropertyScope) {
        leaveEnvironment();
    }
}

bool QQmlJSImportVisitor::visit(ExportDeclaration *)
{
    Q_ASSERT(!m_exportedRootScope.isNull());
    Q_ASSERT(m_exportedRootScope != m_globalScope);
    Q_ASSERT(m_currentScope == m_globalScope);
    m_currentScope = m_exportedRootScope;
    return true;
}

void QQmlJSImportVisitor::endVisit(ExportDeclaration *)
{
    Q_ASSERT(!m_exportedRootScope.isNull());
    m_currentScope = m_exportedRootScope->parentScope();
    Q_ASSERT(m_currentScope == m_globalScope);
}

bool QQmlJSImportVisitor::visit(ESModule *module)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("module"),
                     module->firstSourceLocation());
    Q_ASSERT(m_exportedRootScope.isNull());
    m_exportedRootScope = m_currentScope;
    m_exportedRootScope->setIsScript(true);
    importBaseModules();
    leaveEnvironment();
    return true;
}

void QQmlJSImportVisitor::endVisit(ESModule *)
{
    QQmlJSScope::resolveTypes(m_exportedRootScope, m_rootScopeImports, &m_usedTypes);
}

bool QQmlJSImportVisitor::visit(Program *)
{
    Q_ASSERT(m_globalScope == m_currentScope);
    Q_ASSERT(m_exportedRootScope.isNull());
    m_exportedRootScope = m_currentScope;
    m_exportedRootScope->setIsScript(true);
    importBaseModules();
    return true;
}

void QQmlJSImportVisitor::endVisit(Program *)
{
    QQmlJSScope::resolveTypes(m_exportedRootScope, m_rootScopeImports, &m_usedTypes);
}

QT_END_NAMESPACE
