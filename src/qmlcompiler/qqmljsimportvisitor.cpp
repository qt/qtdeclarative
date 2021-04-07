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

QT_BEGIN_NAMESPACE

using namespace QQmlJS::AST;

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
    if (type == QQmlJSScope::GroupedPropertyScope || type == QQmlJSScope::AttachedPropertyScope)
        m_currentScope->setInternalName(name);
    else
        m_currentScope->setBaseTypeName(name);
    m_currentScope->setIsComposite(true);
    m_currentScope->setSourceLocation(location);
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
                }
            }

            if (type.isNull()) {
                if (doRequeue)
                    continue;
                m_logger.log(QStringLiteral("Cannot deduce type of alias \"%1\"")
                                        .arg(property.propertyName()), Log_Alias, object->sourceLocation());
            } else {
                property.setType(type);
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

static QVariant bindingToVariant(QQmlJS::AST::Statement *statement)
{
    ExpressionStatement *expr = cast<ExpressionStatement *>(statement);

    if (!statement || !expr->expression)
        return QVariant();

    switch (expr->expression->kind) {
    case Node::Kind_StringLiteral:
        return cast<StringLiteral *>(expr->expression)->value.toString();
    case Node::Kind_NumericLiteral:
        return cast<NumericLiteral *>(expr->expression)->value;
    default:
        return QVariant();
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
        const auto *idExprension = cast<IdentifierExpression *>(statement->expression);
        m_scopesById.insert(idExprension->name.toString(), m_currentScope);
    } else {
        for (auto group = id; group->next; group = group->next) {
            const QString name = group->name.toString();

            if (name.isEmpty())
                break;

            enterEnvironment(name.front().isUpper() ? QQmlJSScope::AttachedPropertyScope
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

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiImport *import)
{
    auto addImportLocation = [this, import](const QString &name) {
        if (m_importTypeLocationMap.contains(name)
                && m_importTypeLocationMap.values(name).contains(import->firstSourceLocation()))
            return;

        m_importTypeLocationMap.insert(name, import->firstSourceLocation());
        m_importLocations.insert(import->firstSourceLocation());
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
    // property QtObject __styleData: QtObject {...}

    Q_ASSERT(uiob->qualifiedTypeNameId);
    QString name;
    for (auto id = uiob->qualifiedTypeNameId; id; id = id->next)
        name += id->name.toString() + QLatin1Char('.');

    name.chop(1);

    if (!uiob->hasOnToken) {
        QQmlJSMetaProperty prop;
        prop.setPropertyName(uiob->qualifiedId->name.toString());
        prop.setTypeName(name);
        prop.setIsWritable(true);
        prop.setIsPointer(true);
        prop.setIsAlias(name == QLatin1String("alias"));
        prop.setType(m_rootScopeImports.value(uiob->qualifiedTypeNameId->name.toString()));
        m_currentScope->addOwnProperty(prop);
    }

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

    if (!uiob->hasOnToken) {
        QQmlJSMetaProperty property = m_currentScope->property(uiob->qualifiedId->name.toString());
        property.setType(childScope);
        m_currentScope->addOwnProperty(property);
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
