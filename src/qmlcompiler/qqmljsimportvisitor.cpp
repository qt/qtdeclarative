// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsimportvisitor_p.h"
#include "qqmljslogger_p.h"
#include "qqmljsmetatypes_p.h"
#include "qqmljsresourcefilemapper_p.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qqueue.h>
#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>

#include <QtQml/private/qv4codegen_p.h>
#include <QtQml/private/qqmlstringconverters_p.h>
#include <QtQml/private/qqmlirbuilder_p.h>
#include "qqmljsscope_p.h"
#include "qqmljsutils_p.h"
#include "qqmljsloggingutils.h"
#include "qqmlsaconstants.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <variant>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

using namespace QQmlJS::AST;

/*!
  \internal
  Sets the name of \a scope to \a name based on \a type.
*/
inline void setScopeName(QQmlJSScope::Ptr &scope, QQmlJSScope::ScopeType type, const QString &name)
{
    Q_ASSERT(scope);
    if (type == QQmlSA::ScopeType::GroupedPropertyScope
        || type == QQmlSA::ScopeType::AttachedPropertyScope)
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
    if (type == QQmlSA::ScopeType::GroupedPropertyScope
        || type == QQmlSA::ScopeType::AttachedPropertyScope)
        return scope->internalName();

    return scope->baseTypeName();
}

template<typename Node>
QString buildName(const Node *node)
{
    QString result;
    for (const Node *segment = node; segment; segment = segment->next) {
        if (!result.isEmpty())
            result += u'.';
        result += segment->name;
    }
    return result;
}

QQmlJSImportVisitor::QQmlJSImportVisitor(
        const QQmlJSScope::Ptr &target, QQmlJSImporter *importer, QQmlJSLogger *logger,
        const QString &implicitImportDirectory, const QStringList &qmldirFiles)
    : m_implicitImportDirectory(implicitImportDirectory),
      m_qmldirFiles(qmldirFiles),
      m_currentScope(QQmlJSScope::create()),
      m_exportedRootScope(target),
      m_importer(importer),
      m_logger(logger),
      m_rootScopeImports(
          QQmlJSImporter::ImportedTypes::QML, {},
          importer->builtinInternalNames().arrayType())
{
    m_currentScope->setScopeType(QQmlSA::ScopeType::JSFunctionScope);
    Q_ASSERT(logger); // must be valid

    m_globalScope = m_currentScope;
    m_currentScope->setIsComposite(true);

    m_currentScope->setInternalName(u"global"_s);

    QLatin1String jsGlobVars[] = { /* Not listed on the MDN page; browser and QML extensions: */
                                   // console/debug api
                                   QLatin1String("console"), QLatin1String("print"),
                                   // garbage collector
                                   QLatin1String("gc"),
                                   // i18n
                                   QLatin1String("qsTr"), QLatin1String("qsTrId"),
                                   QLatin1String("QT_TR_NOOP"), QLatin1String("QT_TRANSLATE_NOOP"),
                                   QLatin1String("QT_TRID_NOOP"),
                                   // XMLHttpRequest
                                   QLatin1String("XMLHttpRequest")
    };

    QQmlJSScope::JavaScriptIdentifier globalJavaScript = {
        QQmlJSScope::JavaScriptIdentifier::LexicalScoped, QQmlJS::SourceLocation(), std::nullopt,
        true
    };
    for (const char **globalName = QV4::Compiler::Codegen::s_globalNames; *globalName != nullptr;
         ++globalName) {
        m_currentScope->insertJSIdentifier(QString::fromLatin1(*globalName), globalJavaScript);
    }
    for (const auto &jsGlobVar : jsGlobVars)
        m_currentScope->insertJSIdentifier(jsGlobVar, globalJavaScript);
}

QQmlJSImportVisitor::~QQmlJSImportVisitor() = default;

void QQmlJSImportVisitor::populateCurrentScope(
        QQmlJSScope::ScopeType type, const QString &name, const QQmlJS::SourceLocation &location)
{
    m_currentScope->setScopeType(type);
    setScopeName(m_currentScope, type, name);
    m_currentScope->setIsComposite(true);
    m_currentScope->setFilePath(QFileInfo(m_logger->fileName()).absoluteFilePath());
    m_currentScope->setSourceLocation(location);
    m_scopesByIrLocation.insert({ location.startLine, location.startColumn }, m_currentScope);
}

void QQmlJSImportVisitor::enterRootScope(QQmlJSScope::ScopeType type, const QString &name, const QQmlJS::SourceLocation &location)
{
    QQmlJSScope::reparent(m_currentScope, m_exportedRootScope);
    m_currentScope = m_exportedRootScope;
    populateCurrentScope(type, name, location);
}

void QQmlJSImportVisitor::enterEnvironment(QQmlJSScope::ScopeType type, const QString &name,
                                           const QQmlJS::SourceLocation &location)
{
    QQmlJSScope::Ptr newScope = QQmlJSScope::create();
    QQmlJSScope::reparent(m_currentScope, newScope);
    m_currentScope = std::move(newScope);
    populateCurrentScope(type, name, location);
}

bool QQmlJSImportVisitor::enterEnvironmentNonUnique(QQmlJSScope::ScopeType type,
                                                    const QString &name,
                                                    const QQmlJS::SourceLocation &location)
{
    Q_ASSERT(type == QQmlSA::ScopeType::GroupedPropertyScope
             || type == QQmlSA::ScopeType::AttachedPropertyScope);

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
    m_scopesByIrLocation.insert({ location.startLine, location.startColumn }, *it);
    m_currentScope = *it;
    return true;
}

void QQmlJSImportVisitor::leaveEnvironment()
{
    m_currentScope = m_currentScope->parentScope();
}

bool QQmlJSImportVisitor::isTypeResolved(const QQmlJSScope::ConstPtr &type)
{
    const auto handleUnresolvedType = [this](const QQmlJSScope::ConstPtr &type) {
        m_logger->log(QStringLiteral("Type %1 is used but it is not resolved")
                              .arg(getScopeName(type, type->scopeType())),
                      qmlUnresolvedType, type->sourceLocation());
    };
    return isTypeResolved(type, handleUnresolvedType);
}

static bool mayBeUnresolvedGeneralizedGroupedProperty(const QQmlJSScope::ConstPtr &scope)
{
    return scope->scopeType() == QQmlSA::ScopeType::GroupedPropertyScope && !scope->baseType();
}

void QQmlJSImportVisitor::resolveAliasesAndIds()
{
    QQueue<QQmlJSScope::Ptr> objects;
    objects.enqueue(m_exportedRootScope);

    qsizetype lastRequeueLength = std::numeric_limits<qsizetype>::max();
    QQueue<QQmlJSScope::Ptr> requeue;

    while (!objects.isEmpty()) {
        const QQmlJSScope::Ptr object = objects.dequeue();
        const auto properties = object->ownProperties();

        bool doRequeue = false;
        for (const auto &property : properties) {
            if (!property.isAlias() || !property.type().isNull())
                continue;

            QStringList components = property.aliasExpression().split(u'.');
            QQmlJSMetaProperty targetProperty;

            bool foundProperty = false;

            // The first component has to be an ID. Find the object it refers to.
            QQmlJSScope::ConstPtr type = m_scopesById.scope(components.takeFirst(), object);
            QQmlJSScope::ConstPtr typeScope;
            if (!type.isNull()) {
                foundProperty = true;

                // Any further components are nested properties of that object.
                // Technically we can only resolve a limited depth in the engine, but the rules
                // on that are fuzzy and subject to change. Let's ignore it for now.
                // If the target is itself an alias and has not been resolved, re-queue the object
                // and try again later.
                while (type && !components.isEmpty()) {
                    const QString name = components.takeFirst();

                    if (!type->hasProperty(name)) {
                        foundProperty = false;
                        type = {};
                        break;
                    }

                    const auto target = type->property(name);
                    if (!target.type() && target.isAlias())
                        doRequeue = true;
                    typeScope = type;
                    type = target.type();
                    targetProperty = target;
                }
            }

            if (type.isNull()) {
                if (doRequeue)
                    continue;
                if (foundProperty) {
                    m_logger->log(QStringLiteral("Cannot deduce type of alias \"%1\"")
                                          .arg(property.propertyName()),
                                  qmlMissingType, object->sourceLocation());
                } else {
                    m_logger->log(QStringLiteral("Cannot resolve alias \"%1\"")
                                          .arg(property.propertyName()),
                                  qmlUnresolvedAlias, object->sourceLocation());
                }

                Q_ASSERT(property.index() >= 0); // this property is already in object
                object->addOwnProperty(property);

            } else {
                QQmlJSMetaProperty newProperty = property;
                newProperty.setType(type);
                // Copy additional property information from target
                newProperty.setIsList(targetProperty.isList());
                newProperty.setIsWritable(targetProperty.isWritable());
                newProperty.setIsPointer(targetProperty.isPointer());

                if (!typeScope.isNull()) {
                    object->setPropertyLocallyRequired(
                            newProperty.propertyName(),
                            typeScope->isPropertyRequired(targetProperty.propertyName()));
                }

                if (const QString internalName = type->internalName(); !internalName.isEmpty())
                    newProperty.setTypeName(internalName);

                Q_ASSERT(newProperty.index() >= 0); // this property is already in object
                object->addOwnProperty(newProperty);
            }
        }

        const auto childScopes = object->childScopes();
        for (const auto &childScope : childScopes) {
            if (mayBeUnresolvedGeneralizedGroupedProperty(childScope)) {
                const QString name = childScope->internalName();
                if (object->isNameDeferred(name)) {
                    const QQmlJSScope::ConstPtr deferred = m_scopesById.scope(name, childScope);
                    if (!deferred.isNull()) {
                        QQmlJSScope::resolveGeneralizedGroup(
                                    childScope, deferred, m_rootScopeImports, &m_usedTypes);
                    }
                }
            }
            objects.enqueue(childScope);
        }

        if (doRequeue)
            requeue.enqueue(object);

        if (objects.isEmpty() && requeue.size() < lastRequeueLength) {
            lastRequeueLength = requeue.size();
            objects.swap(requeue);
        }
    }

    while (!requeue.isEmpty()) {
        const QQmlJSScope::Ptr object = requeue.dequeue();
        const auto properties = object->ownProperties();
        for (const auto &property : properties) {
            if (!property.isAlias() || property.type())
                continue;
            m_logger->log(QStringLiteral("Alias \"%1\" is part of an alias cycle")
                                  .arg(property.propertyName()),
                          qmlAliasCycle, object->sourceLocation());
        }
    }
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

void QQmlJSImportVisitor::processImportWarnings(
        const QString &what, const QQmlJS::SourceLocation &srcLocation)
{
    const auto warnings = m_importer->takeWarnings();
    if (warnings.isEmpty())
        return;

    m_logger->log(QStringLiteral("Warnings occurred while importing %1:").arg(what), qmlImport,
                  srcLocation);
    m_logger->processMessages(warnings, qmlImport);
}

void QQmlJSImportVisitor::importBaseModules()
{
    Q_ASSERT(m_rootScopeImports.types().isEmpty());
    m_rootScopeImports = m_importer->importBuiltins();

    const QQmlJS::SourceLocation invalidLoc;
    for (auto it = m_rootScopeImports.types().keyBegin(), end = m_rootScopeImports.types().keyEnd();
         it != end; it++) {
        addImportWithLocation(*it, invalidLoc);
    }

    if (!m_qmldirFiles.isEmpty())
        m_importer->importQmldirs(m_qmldirFiles);

    // Pulling in the modules and neighboring qml files of the qmltypes we're trying to lint is not
    // something we need to do.
    if (!m_logger->fileName().endsWith(u".qmltypes"_s)) {
        QQmlJSScope::ContextualTypes fromDirectory =
                m_importer->importDirectory(m_implicitImportDirectory);
        m_rootScopeImports.addTypes(std::move(fromDirectory));

        // Import all possible resource directories the file may belong to.
        // This is somewhat fuzzy, but if you're mapping the same file to multiple resource
        // locations, you're on your own anyway.
        if (QQmlJSResourceFileMapper *mapper = m_importer->resourceFileMapper()) {
            const QStringList resourcePaths = mapper->resourcePaths(QQmlJSResourceFileMapper::Filter {
                    m_logger->fileName(), QStringList(), QQmlJSResourceFileMapper::Resource });
            for (const QString &path : resourcePaths) {
                const qsizetype lastSlash = path.lastIndexOf(QLatin1Char('/'));
                if (lastSlash == -1)
                    continue;
                m_rootScopeImports.addTypes(m_importer->importDirectory(path.first(lastSlash)));
            }
        }
    }

    processImportWarnings(QStringLiteral("base modules"));
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiProgram *)
{
    importBaseModules();
    return true;
}

void QQmlJSImportVisitor::endVisit(UiProgram *)
{
    for (const auto &scope : m_objectBindingScopes) {
        breakInheritanceCycles(scope);
        checkDeprecation(scope);
    }

    for (const auto &scope : m_objectDefinitionScopes) {
        if (m_pendingDefaultProperties.contains(scope))
            continue; // We're going to check this one below.
        breakInheritanceCycles(scope);
        checkDeprecation(scope);
    }

    for (const auto &scope : m_pendingDefaultProperties.keys()) {
        breakInheritanceCycles(scope);
        checkDeprecation(scope);
    }

    resolveAliasesAndIds();

    for (const auto &scope : m_objectDefinitionScopes)
        checkGroupedAndAttachedScopes(scope);

    setAllBindings();
    processDefaultProperties();
    processPropertyTypes();
    processPropertyBindings();
    processPropertyBindingObjects();
    checkRequiredProperties();

    auto unusedImports = m_importLocations;
    for (const QString &type : m_usedTypes) {
        for (const auto &importLocation : m_importTypeLocationMap.values(type))
            unusedImports.remove(importLocation);

        // If there are no more unused imports left we can abort early
        if (unusedImports.isEmpty())
            break;
    }

    for (const QQmlJS::SourceLocation &import : m_importStaticModuleLocationMap.values())
        unusedImports.remove(import);

    for (const auto &import : unusedImports) {
        m_logger->log(QString::fromLatin1("Unused import"), qmlUnusedImports, import);
    }

    populateRuntimeFunctionIndicesForDocument();
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

        QQmlJSAnnotation qqmljsAnnotation;
        qqmljsAnnotation.name = buildName(annotation->qualifiedTypeNameId);

        for (UiObjectMemberList *memberItem = annotation->initializer->members; memberItem != nullptr; memberItem = memberItem->next) {
            switch (memberItem->member->kind) {
            case Node::Kind_UiScriptBinding: {
                auto *scriptBinding = QQmlJS::AST::cast<UiScriptBinding*>(memberItem->member);
                qqmljsAnnotation.bindings[buildName(scriptBinding->qualifiedId)]
                        = bindingToVariant(scriptBinding->statement);
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

void QQmlJSImportVisitor::setAllBindings()
{
    for (auto it = m_bindings.cbegin(); it != m_bindings.cend(); ++it) {
        // ensure the scope is resolved, if not - it is an error
        auto type = it->owner;
        if (!type->isFullyResolved()) {
            if (!type->isInCustomParserParent()) { // special otherwise
                m_logger->log(QStringLiteral("'%1' is used but it is not resolved")
                                      .arg(getScopeName(type, type->scopeType())),
                              qmlUnresolvedType, type->sourceLocation());
            }
            continue;
        }
        auto binding = it->create();
        if (binding.isValid())
            type->addOwnPropertyBinding(binding, it->specifier);
    }
}

void QQmlJSImportVisitor::processDefaultProperties()
{
    for (auto it = m_pendingDefaultProperties.constBegin();
         it != m_pendingDefaultProperties.constEnd(); ++it) {
        QQmlJSScope::ConstPtr parentScope = it.key();

        // We can't expect custom parser default properties to be sensible, discard them for now.
        if (parentScope->isInCustomParserParent())
            continue;

        /* consider:
         *
         *      QtObject { // <- parentScope
         *          default property var p // (1)
         *          QtObject {} // (2)
         *      }
         *
         * `p` (1) is a property of a subtype of QtObject, it couldn't be used
         * in a property binding (2)
         */
        // thus, use a base type of parent scope to detect a default property
        parentScope = parentScope->baseType();

        const QString defaultPropertyName =
                parentScope ? parentScope->defaultPropertyName() : QString();

        if (defaultPropertyName.isEmpty()) {
            // If the parent scope is based on Component it can have any child element
            // TODO: We should also store these somewhere
            bool isComponent = false;
            for (QQmlJSScope::ConstPtr s = parentScope; s; s = s->baseType()) {
                if (s->internalName() == QStringLiteral("QQmlComponent")) {
                    isComponent = true;
                    break;
                }
            }

            if (!isComponent) {
                m_logger->log(QStringLiteral("Cannot assign to non-existent default property"),
                              qmlMissingProperty, it.value().constFirst()->sourceLocation());
            }

            continue;
        }

        const QQmlJSMetaProperty defaultProp = parentScope->property(defaultPropertyName);
        auto propType = defaultProp.type();
        const auto handleUnresolvedDefaultProperty = [&](const QQmlJSScope::ConstPtr &) {
            // Property type is not fully resolved we cannot tell any more than this
            m_logger->log(QStringLiteral("Property \"%1\" has incomplete type \"%2\". You may be "
                                         "missing an import.")
                                  .arg(defaultPropertyName)
                                  .arg(defaultProp.typeName()),
                          qmlMissingProperty, it.value().constFirst()->sourceLocation());
        };

        if (propType.isNull()) {
            handleUnresolvedDefaultProperty(propType);
            continue;
        }

        if (it.value().size() > 1
                && !defaultProp.isList()
                && !propType->isListProperty()) {
            m_logger->log(
                    QStringLiteral("Cannot assign multiple objects to a default non-list property"),
                    qmlNonListProperty, it.value().constFirst()->sourceLocation());
        }

        if (!isTypeResolved(propType, handleUnresolvedDefaultProperty))
            continue;

        const auto scopes = *it;
        for (const auto &scope : scopes) {
            if (!isTypeResolved(scope))
                continue;

            // Assigning any element to a QQmlComponent property implicitly wraps it into a Component
            // Check whether the property can be assigned the scope
            if (propType->canAssign(scope)) {
                scope->setIsWrappedInImplicitComponent(
                        QQmlJSScope::causesImplicitComponentWrapping(defaultProp, scope));
                continue;
            }

            m_logger->log(QStringLiteral("Cannot assign to default property of incompatible type"),
                          qmlIncompatibleType, scope->sourceLocation());
        }
    }
}

void QQmlJSImportVisitor::processPropertyTypes()
{
    for (const PendingPropertyType &type : m_pendingPropertyTypes) {
        Q_ASSERT(type.scope->hasOwnProperty(type.name));

        auto property = type.scope->ownProperty(type.name);

        if (const auto propertyType =
                    QQmlJSScope::findType(property.typeName(), m_rootScopeImports).scope) {
            property.setType(propertyType);
            type.scope->addOwnProperty(property);
        } else {
            m_logger->log(property.typeName()
                                  + QStringLiteral(" was not found. Did you add all import paths?"),
                          qmlImport, type.location);
        }
    }
}

void QQmlJSImportVisitor::processPropertyBindingObjects()
{
    QSet<QPair<QQmlJSScope::Ptr, QString>> foundLiterals;
    {
        // Note: populating literals here is special, because we do not store
        // them in m_pendingPropertyObjectBindings, so we have to lookup all
        // bindings on a property for each scope and see if there are any
        // literal bindings there. this is safe to do once at the beginning
        // because this function doesn't add new literal bindings and all
        // literal bindings must already be added at this point.
        QSet<QPair<QQmlJSScope::Ptr, QString>> visited;
        for (const PendingPropertyObjectBinding &objectBinding :
             std::as_const(m_pendingPropertyObjectBindings)) {
            // unique because it's per-scope and per-property
            const auto uniqueBindingId = qMakePair(objectBinding.scope, objectBinding.name);
            if (visited.contains(uniqueBindingId))
                continue;
            visited.insert(uniqueBindingId);

            auto [existingBindingsBegin, existingBindingsEnd] =
                    uniqueBindingId.first->ownPropertyBindings(uniqueBindingId.second);
            const bool hasLiteralBindings =
                    std::any_of(existingBindingsBegin, existingBindingsEnd,
                                [](const QQmlJSMetaPropertyBinding &x) { return x.hasLiteral(); });
            if (hasLiteralBindings)
                foundLiterals.insert(uniqueBindingId);
        }
    }

    QSet<QPair<QQmlJSScope::Ptr, QString>> foundObjects;
    QSet<QPair<QQmlJSScope::Ptr, QString>> foundInterceptors;
    QSet<QPair<QQmlJSScope::Ptr, QString>> foundValueSources;

    for (const PendingPropertyObjectBinding &objectBinding :
         std::as_const(m_pendingPropertyObjectBindings)) {
        const QString propertyName = objectBinding.name;
        QQmlJSScope::ConstPtr childScope = objectBinding.childScope;

        if (!isTypeResolved(objectBinding.scope)) // guarantees property lookup
            continue;

        QQmlJSMetaProperty property = objectBinding.scope->property(propertyName);

        if (!property.isValid()) {
            m_logger->log(QStringLiteral("Property \"%1\" does not exist").arg(propertyName),
                          qmlMissingProperty, objectBinding.location);
            continue;
        }
        const auto handleUnresolvedProperty = [&](const QQmlJSScope::ConstPtr &) {
            // Property type is not fully resolved we cannot tell any more than this
            m_logger->log(QStringLiteral("Property \"%1\" has incomplete type \"%2\". You may be "
                                         "missing an import.")
                                  .arg(propertyName)
                                  .arg(property.typeName()),
                          qmlUnresolvedType, objectBinding.location);
        };
        if (property.type().isNull()) {
            handleUnresolvedProperty(property.type());
            continue;
        }

        // guarantee that canAssign() can be called
        if (!isTypeResolved(property.type(), handleUnresolvedProperty)
            || !isTypeResolved(childScope)) {
            continue;
        }

        if (!objectBinding.onToken && !property.type()->canAssign(childScope)) {
            // the type is incompatible
            m_logger->log(QStringLiteral("Property \"%1\" of type \"%2\" is assigned an "
                                         "incompatible type \"%3\"")
                                  .arg(propertyName)
                                  .arg(property.typeName())
                                  .arg(getScopeName(childScope, QQmlSA::ScopeType::QMLScope)),
                          qmlIncompatibleType, objectBinding.location);
            continue;
        }

        objectBinding.childScope->setIsWrappedInImplicitComponent(
                QQmlJSScope::causesImplicitComponentWrapping(property, childScope));

        // unique because it's per-scope and per-property
        const auto uniqueBindingId = qMakePair(objectBinding.scope, objectBinding.name);
        const QString typeName = getScopeName(childScope, QQmlSA::ScopeType::QMLScope);

        if (objectBinding.onToken) {
            if (childScope->hasInterface(QStringLiteral("QQmlPropertyValueInterceptor"))) {
                if (foundInterceptors.contains(uniqueBindingId)) {
                    m_logger->log(QStringLiteral("Duplicate interceptor on property \"%1\"")
                                          .arg(propertyName),
                                  qmlDuplicatePropertyBinding, objectBinding.location);
                } else {
                    foundInterceptors.insert(uniqueBindingId);
                }
            } else if (childScope->hasInterface(QStringLiteral("QQmlPropertyValueSource"))) {
                if (foundValueSources.contains(uniqueBindingId)) {
                    m_logger->log(QStringLiteral("Duplicate value source on property \"%1\"")
                                          .arg(propertyName),
                                  qmlDuplicatePropertyBinding, objectBinding.location);
                } else if (foundObjects.contains(uniqueBindingId)
                           || foundLiterals.contains(uniqueBindingId)) {
                    m_logger->log(QStringLiteral("Cannot combine value source and binding on "
                                                 "property \"%1\"")
                                          .arg(propertyName),
                                  qmlDuplicatePropertyBinding, objectBinding.location);
                } else {
                    foundValueSources.insert(uniqueBindingId);
                }
            } else {
                m_logger->log(QStringLiteral("On-binding for property \"%1\" has wrong type \"%2\"")
                                      .arg(propertyName)
                                      .arg(typeName),
                              qmlIncompatibleType, objectBinding.location);
            }
        } else {
            // TODO: Warn here if binding.hasValue() is true
            if (foundValueSources.contains(uniqueBindingId)) {
                m_logger->log(
                        QStringLiteral("Cannot combine value source and binding on property \"%1\"")
                                .arg(propertyName),
                        qmlDuplicatePropertyBinding, objectBinding.location);
            } else {
                foundObjects.insert(uniqueBindingId);
            }
        }
    }
}

void QQmlJSImportVisitor::checkRequiredProperties()
{
    for (const auto &required : m_requiredProperties) {
        if (!required.scope->hasProperty(required.name)) {
            m_logger->log(
                    QStringLiteral("Property \"%1\" was marked as required but does not exist.")
                            .arg(required.name),
                    qmlRequired, required.location);
        }
    }

    for (const auto &defScope : m_objectDefinitionScopes) {
        if (defScope->parentScope() == m_globalScope || defScope->isInlineComponent() || defScope->isComponentRootElement())
            continue;

        QVector<QQmlJSScope::ConstPtr> scopesToSearch;
        for (QQmlJSScope::ConstPtr scope = defScope; scope; scope = scope->baseType()) {
            scopesToSearch << scope;
            const auto ownProperties = scope->ownProperties();
            for (auto propertyIt = ownProperties.constBegin();
                 propertyIt != ownProperties.constEnd(); ++propertyIt) {
                const QString propName = propertyIt.key();

                QQmlJSScope::ConstPtr prevRequiredScope;
                for (QQmlJSScope::ConstPtr requiredScope : scopesToSearch) {
                    if (requiredScope->isPropertyLocallyRequired(propName)) {
                        bool found =
                                std::find_if(scopesToSearch.constBegin(), scopesToSearch.constEnd(),
                                             [&](QQmlJSScope::ConstPtr scope) {
                                                 return scope->hasPropertyBindings(propName);
                                             })
                                != scopesToSearch.constEnd();

                        if (!found) {
                            const QString scopeId = m_scopesById.id(defScope, scope);
                            bool propertyUsedInRootAlias = false;
                            if (!scopeId.isEmpty()) {
                                for (const QQmlJSMetaProperty &property :
                                     m_exportedRootScope->ownProperties()) {
                                    if (!property.isAlias())
                                        continue;

                                    QStringList aliasExpression =
                                            property.aliasExpression().split(u'.');

                                    if (aliasExpression.size() != 2)
                                        continue;
                                    if (aliasExpression[0] == scopeId
                                        && aliasExpression[1] == propName) {
                                        propertyUsedInRootAlias = true;
                                        break;
                                    }
                                }
                            }

                            if (propertyUsedInRootAlias)
                                continue;

                            const QQmlJSScope::ConstPtr propertyScope = scopesToSearch.size() > 1
                                    ? scopesToSearch.at(scopesToSearch.size() - 2)
                                    : QQmlJSScope::ConstPtr();

                            const QString propertyScopeName = !propertyScope.isNull()
                                    ? getScopeName(propertyScope, QQmlSA::ScopeType::QMLScope)
                                    : u"here"_s;

                            const QString requiredScopeName = prevRequiredScope
                                    ? getScopeName(prevRequiredScope, QQmlSA::ScopeType::QMLScope)
                                    : u"here"_s;

                            std::optional<QQmlJSFixSuggestion> suggestion;

                            QString message =
                                    QStringLiteral(
                                            "Component is missing required property %1 from %2")
                                            .arg(propName)
                                            .arg(propertyScopeName);
                            if (requiredScope != scope) {
                                if (!prevRequiredScope.isNull()) {
                                    auto sourceScope = prevRequiredScope->baseType();
                                    suggestion = QQmlJSFixSuggestion{
                                        "%1:%2:%3: Property marked as required in %4."_L1
                                                .arg(sourceScope->filePath())
                                                .arg(sourceScope->sourceLocation().startLine)
                                                .arg(sourceScope->sourceLocation().startColumn)
                                                .arg(requiredScopeName),
                                        sourceScope->sourceLocation()
                                    };
                                    suggestion->setFilename(sourceScope->filePath());
                                } else {
                                    message += QStringLiteral(" (marked as required by %1)")
                                                       .arg(requiredScopeName);
                                }
                            }

                            m_logger->log(message, qmlRequired, defScope->sourceLocation(), true,
                                          true, suggestion);
                        }
                    }
                    prevRequiredScope = requiredScope;
                }
            }
        }
    }
}

void QQmlJSImportVisitor::processPropertyBindings()
{
    for (auto it = m_propertyBindings.constBegin(); it != m_propertyBindings.constEnd(); ++it) {
        QQmlJSScope::Ptr scope = it.key();
        for (auto &[visibilityScope, location, name] : it.value()) {
            if (!scope->hasProperty(name)) {
                // These warnings do not apply for custom parsers and their children and need to be
                // handled on a case by case basis

                if (scope->isInCustomParserParent())
                    continue;

                // TODO: Can this be in a better suited category?
                std::optional<QQmlJSFixSuggestion> fixSuggestion;

                for (QQmlJSScope::ConstPtr baseScope = scope; !baseScope.isNull();
                     baseScope = baseScope->baseType()) {
                    if (auto suggestion = QQmlJSUtils::didYouMean(
                                name, baseScope->ownProperties().keys(), location);
                        suggestion.has_value()) {
                        fixSuggestion = suggestion;
                        break;
                    }
                }

                m_logger->log(QStringLiteral("Binding assigned to \"%1\", but no property \"%1\" "
                                             "exists in the current element.")
                                      .arg(name),
                              qmlMissingProperty, location, true, true, fixSuggestion);
                continue;
            }

            const auto property = scope->property(name);
            if (!property.type()) {
                m_logger->log(QStringLiteral("No type found for property \"%1\". This may be due "
                                             "to a missing import statement or incomplete "
                                             "qmltypes files.")
                                      .arg(name),
                              qmlMissingType, location);
            }

            const auto &annotations = property.annotations();

            const auto deprecationAnn =
                    std::find_if(annotations.cbegin(), annotations.cend(),
                                 [](const QQmlJSAnnotation &ann) { return ann.isDeprecation(); });

            if (deprecationAnn != annotations.cend()) {
                const auto deprecation = deprecationAnn->deprecation();

                QString message = QStringLiteral("Binding on deprecated property \"%1\"")
                                          .arg(property.propertyName());

                if (!deprecation.reason.isEmpty())
                    message.append(QStringLiteral(" (Reason: %1)").arg(deprecation.reason));

                m_logger->log(message, qmlDeprecated, location);
            }
        }
    }
}

void QQmlJSImportVisitor::checkSignal(
        const QQmlJSScope::ConstPtr &signalScope, const QQmlJS::SourceLocation &location,
        const QString &handlerName, const QStringList &handlerParameters)
{
    const auto signal = QQmlJSUtils::signalName(handlerName);

    std::optional<QQmlJSMetaMethod> signalMethod;
    const auto setSignalMethod = [&](const QQmlJSScope::ConstPtr &scope, const QString &name) {
        const auto methods = scope->methods(name, QQmlJSMetaMethodType::Signal);
        if (!methods.isEmpty())
            signalMethod = methods[0];
    };

    if (signal.has_value()) {
        if (signalScope->hasMethod(*signal)) {
            setSignalMethod(signalScope, *signal);
        } else if (auto p = QQmlJSUtils::changeHandlerProperty(signalScope, *signal);
                   p.has_value()) {
            // we have a change handler of the form "onXChanged" where 'X'
            // is a property name

            // NB: qqmltypecompiler prefers signal to bindable
            if (auto notify = p->notify(); !notify.isEmpty()) {
                setSignalMethod(signalScope, notify);
            } else {
                Q_ASSERT(!p->bindable().isEmpty());
                signalMethod = QQmlJSMetaMethod {}; // use dummy in this case
            }
        }
    }

    if (!signalMethod.has_value()) { // haven't found anything
        std::optional<QQmlJSFixSuggestion> fix;

        // There is a small chance of suggesting this fix for things that are not actually
        // QtQml/Connections elements, but rather some other thing that is also called
        // "Connections". However, I guess we can live with this.
        if (signalScope->baseTypeName() == QStringLiteral("Connections")) {

            // Cut to the end of the line to avoid hairy issues with pre-existing function()
            // and the colon.
            const qsizetype newLength = m_logger->code().indexOf(u'\n', location.end())
                    - location.offset;

            fix = QQmlJSFixSuggestion{
                "Implicitly defining %1 as signal handler in Connections is deprecated. "
                "Create a function instead."_L1.arg(handlerName),
                QQmlJS::SourceLocation(location.offset, newLength, location.startLine,
                                       location.startColumn),
                "function %1(%2) { ... }"_L1.arg(handlerName, handlerParameters.join(u", "))
            };
        }

        m_logger->log(QStringLiteral("no matching signal found for handler \"%1\"")
                              .arg(handlerName),
                      qmlUnqualified, location, true, true, fix);
        return;
    }

    const auto signalParameters = signalMethod->parameters();
    QHash<QString, qsizetype> parameterNameIndexes;
    // check parameter positions and also if signal is suitable for onSignal handler
    for (int i = 0, end = signalParameters.size(); i < end; i++) {
        auto &p = signalParameters[i];
        parameterNameIndexes[p.name()] = i;

        auto signalName = [&]() {
            if (signal)
                return u" called %1"_s.arg(*signal);
            return QString();
        };
        auto type = p.type();
        if (!type) {
            m_logger->log(
                    QStringLiteral(
                            "Type %1 of parameter %2 in signal%3 was not found, but is "
                            "required to compile %4. Did you add all import paths?")
                            .arg(p.typeName(), p.name(), signalName(), handlerName),
                    qmlSignalParameters, location);
            continue;
        }

        if (type->isComposite())
            continue;

        // only accept following parameters for non-composite types:
        // * QObjects by pointer (nonconst*, const*, const*const,*const)
        // * Value types by value (QFont, int)
        // * Value types by const ref (const QFont&, const int&)

        auto parameterName = [&]() {
            if (p.name().isEmpty())
                return QString();
            return u" called %1"_s.arg(p.name());
        };
        switch (type->accessSemantics()) {
        case QQmlJSScope::AccessSemantics::Reference:
            if (!p.isPointer())
                m_logger->log(QStringLiteral("Type %1 of parameter%2 in signal%3 should be "
                                             "passed by pointer to be able to compile %4. ")
                                      .arg(p.typeName(), parameterName(), signalName(),
                                           handlerName),
                              qmlSignalParameters, location);
            break;
        case QQmlJSScope::AccessSemantics::Value:
        case QQmlJSScope::AccessSemantics::Sequence:
            if (p.isPointer())
                m_logger->log(
                        QStringLiteral(
                                "Type %1 of parameter%2 in signal%3 should be passed by "
                                "value or const reference to be able to compile %4. ")
                                .arg(p.typeName(), parameterName(), signalName(),
                                     handlerName),
                        qmlSignalParameters, location);
            break;
        case QQmlJSScope::AccessSemantics::None:
            m_logger->log(
                    QStringLiteral("Type %1 of parameter%2 in signal%3 required by the "
                                   "compilation of %4 cannot be used. ")
                            .arg(p.typeName(), parameterName(), signalName(), handlerName),
                    qmlSignalParameters, location);
            break;
        }
    }

    if (handlerParameters.size() > signalParameters.size()) {
        m_logger->log(QStringLiteral("Signal handler for \"%2\" has more formal"
                                     " parameters than the signal it handles.")
                              .arg(handlerName),
                      qmlSignalParameters, location);
        return;
    }

    for (qsizetype i = 0, end = handlerParameters.size(); i < end; i++) {
        const QStringView handlerParameter = handlerParameters.at(i);
        auto it = parameterNameIndexes.constFind(handlerParameter.toString());
        if (it == parameterNameIndexes.constEnd())
            continue;
        const qsizetype j = *it;

        if (j == i)
            continue;

        m_logger->log(QStringLiteral("Parameter %1 to signal handler for \"%2\""
                                     " is called \"%3\". The signal has a parameter"
                                     " of the same name in position %4.")
                              .arg(i + 1)
                              .arg(handlerName, handlerParameter)
                              .arg(j + 1),
                      qmlSignalParameters, location);
    }
}

void QQmlJSImportVisitor::addDefaultProperties()
{
    QQmlJSScope::ConstPtr parentScope = m_currentScope->parentScope();
    if (m_currentScope == m_exportedRootScope || parentScope->isArrayScope()
        || m_currentScope->isInlineComponent()) // inapplicable
        return;

    m_pendingDefaultProperties[m_currentScope->parentScope()] << m_currentScope;

    if (parentScope->isInCustomParserParent())
        return;

    /* consider:
     *
     *      QtObject { // <- parentScope
     *          default property var p // (1)
     *          QtObject {} // (2)
     *      }
     *
     * `p` (1) is a property of a subtype of QtObject, it couldn't be used
     * in a property binding (2)
     */
    // thus, use a base type of parent scope to detect a default property
    parentScope = parentScope->baseType();

    const QString defaultPropertyName =
            parentScope ? parentScope->defaultPropertyName() : QString();

    if (defaultPropertyName.isEmpty()) // an error somewhere else
        return;

    // Note: in this specific code path, binding on default property
    // means an object binding (we work with pending objects here)
    QQmlJSMetaPropertyBinding binding(m_currentScope->sourceLocation(), defaultPropertyName);
    binding.setObject(getScopeName(m_currentScope, QQmlSA::ScopeType::QMLScope),
                      QQmlJSScope::ConstPtr(m_currentScope));
    m_bindings.append(UnfinishedBinding { m_currentScope->parentScope(), [=]() { return binding; },
                                          QQmlJSScope::UnnamedPropertyTarget });
}

void QQmlJSImportVisitor::breakInheritanceCycles(const QQmlJSScope::Ptr &originalScope)
{
    QList<QQmlJSScope::ConstPtr> scopes;
    for (QQmlJSScope::ConstPtr scope = originalScope; scope;) {
        if (scopes.contains(scope)) {
            QString inheritenceCycle;
            for (const auto &seen : std::as_const(scopes)) {
                inheritenceCycle.append(seen->baseTypeName());
                inheritenceCycle.append(QLatin1String(" -> "));
            }
            inheritenceCycle.append(scopes.first()->baseTypeName());

            const QString message = QStringLiteral("%1 is part of an inheritance cycle: %2")
                                            .arg(scope->internalName(), inheritenceCycle);
            m_logger->log(message, qmlInheritanceCycle, scope->sourceLocation());
            originalScope->clearBaseType();
            originalScope->setBaseTypeError(message);
            break;
        }

        scopes.append(scope);

        const auto newScope = scope->baseType();
        if (newScope.isNull()) {
            const QString error = scope->baseTypeError();
            const QString name = scope->baseTypeName();
            if (!error.isEmpty()) {
                m_logger->log(error, qmlImport, scope->sourceLocation(), true, true);
            } else if (!name.isEmpty()) {
                m_logger->log(
                        name + QStringLiteral(" was not found. Did you add all import paths?"),
                        qmlImport, scope->sourceLocation(), true, true,
                        QQmlJSUtils::didYouMean(scope->baseTypeName(),
                                                m_rootScopeImports.types().keys(),
                                                scope->sourceLocation()));
            }
        }

        scope = newScope;
    }
}

void QQmlJSImportVisitor::checkDeprecation(const QQmlJSScope::ConstPtr &originalScope)
{
    for (QQmlJSScope::ConstPtr scope = originalScope; scope; scope = scope->baseType()) {
        for (const QQmlJSAnnotation &annotation : scope->annotations()) {
            if (annotation.isDeprecation()) {
                QQQmlJSDeprecation deprecation = annotation.deprecation();

                QString message =
                        QStringLiteral("Type \"%1\" is deprecated").arg(scope->internalName());

                if (!deprecation.reason.isEmpty())
                    message.append(QStringLiteral(" (Reason: %1)").arg(deprecation.reason));

                m_logger->log(message, qmlDeprecated, originalScope->sourceLocation());
            }
        }
    }
}

void QQmlJSImportVisitor::checkGroupedAndAttachedScopes(QQmlJSScope::ConstPtr scope)
{
    // These warnings do not apply for custom parsers and their children and need to be handled on a
    // case by case basis
    if (scope->isInCustomParserParent())
        return;

    auto children = scope->childScopes();
    while (!children.isEmpty()) {
        auto childScope = children.takeFirst();
        const auto type = childScope->scopeType();
        switch (type) {
        case QQmlSA::ScopeType::GroupedPropertyScope:
        case QQmlSA::ScopeType::AttachedPropertyScope:
            if (!childScope->baseType()) {
                m_logger->log(QStringLiteral("unknown %1 property scope %2.")
                                      .arg(type == QQmlSA::ScopeType::GroupedPropertyScope
                                                   ? QStringLiteral("grouped")
                                                   : QStringLiteral("attached"),
                                           childScope->internalName()),
                              qmlUnqualified, childScope->sourceLocation());
            }
            children.append(childScope->childScopes());
        default:
            break;
        }
    }
}

void QQmlJSImportVisitor::flushPendingSignalParameters()
{
    const QQmlJSMetaSignalHandler handler = m_signalHandlers[m_pendingSignalHandler];
    for (const QString &parameter : handler.signalParameters) {
        m_currentScope->insertJSIdentifier(parameter,
                                           { QQmlJSScope::JavaScriptIdentifier::Injected,
                                             m_pendingSignalHandler, std::nullopt, false });
    }
    m_pendingSignalHandler = QQmlJS::SourceLocation();
}

/*! \internal

    Records a JS function or a Script binding for a given \a scope. Returns an
    index of a just recorded function-or-expression.

    \sa synthesizeCompilationUnitRuntimeFunctionIndices
*/
QQmlJSMetaMethod::RelativeFunctionIndex
QQmlJSImportVisitor::addFunctionOrExpression(const QQmlJSScope::ConstPtr &scope,
                                             const QString &name)
{
    auto &array = m_functionsAndExpressions[scope];
    array.emplaceBack(name);

    // add current function to all preceding functions in the stack. we don't
    // know which one is going to be the "publicly visible" one, so just blindly
    // add it to every level and let further logic take care of that. this
    // matches what m_innerFunctions represents as function at each level just
    // got a new inner function
    for (const auto &function : m_functionStack)
        m_innerFunctions[function]++;
    m_functionStack.push({ scope, name }); // create new function

    return QQmlJSMetaMethod::RelativeFunctionIndex { int(array.size() - 1) };
}

/*! \internal

    Removes last FunctionOrExpressionIdentifier from m_functionStack, performing
    some checks on \a name.

    \note \a name must match the name added via addFunctionOrExpression().

    \sa addFunctionOrExpression, synthesizeCompilationUnitRuntimeFunctionIndices
*/
void QQmlJSImportVisitor::forgetFunctionExpression(const QString &name)
{
    auto nameToVerify = name.isEmpty() ? u"<anon>"_s : name;
    Q_UNUSED(nameToVerify);
    Q_ASSERT(!m_functionStack.isEmpty());
    Q_ASSERT(m_functionStack.top().name == nameToVerify);
    m_functionStack.pop();
}

/*! \internal

    Sets absolute runtime function indices for \a scope based on \a count
    (document-level variable). Returns count incremented by the number of
    runtime functions that the current \a scope has.

    \note Not all scopes are considered as the function is compatible with the
    compilation unit output. The runtime functions are only recorded for
    QmlIR::Object (even if they don't strictly belong to it). Thus, in
    QQmlJSScope terms, we are only interested in QML scopes, group and attached
    property scopes.
*/
int QQmlJSImportVisitor::synthesizeCompilationUnitRuntimeFunctionIndices(
        const QQmlJSScope::Ptr &scope, int count) const
{
    const auto suitableScope = [](const QQmlJSScope::Ptr &scope) {
        const auto type = scope->scopeType();
        return type == QQmlSA::ScopeType::QMLScope
                || type == QQmlSA::ScopeType::GroupedPropertyScope
                || type == QQmlSA::ScopeType::AttachedPropertyScope;
    };

    if (!suitableScope(scope))
        return count;

    QList<QQmlJSMetaMethod::AbsoluteFunctionIndex> indices;
    auto it = m_functionsAndExpressions.constFind(scope);
    if (it == m_functionsAndExpressions.cend()) // scope has no runtime functions
        return count;

    const auto &functionsAndExpressions = *it;
    for (const QString &functionOrExpression : functionsAndExpressions) {
        scope->addOwnRuntimeFunctionIndex(
                static_cast<QQmlJSMetaMethod::AbsoluteFunctionIndex>(count));
        ++count;

        // there are special cases: onSignal: function() { doSomethingUsefull }
        // in which we would register 2 functions in the runtime functions table
        // for the same expression. even more, we can have named and unnamed
        // closures inside a function or a script binding e.g.:
        // ```
        // function foo() {
        //  var closure = () => { return 42; }; // this is an inner function
        //  /* or:
        //      property = Qt.binding(function() { return anotherProperty; });
        //   */
        //  return closure();
        // }
        // ```
        // see Codegen::defineFunction() in qv4codegen.cpp for more details
        count += m_innerFunctions.value({ scope, functionOrExpression }, 0);
    }

    return count;
}

void QQmlJSImportVisitor::populateRuntimeFunctionIndicesForDocument() const
{
    int count = 0;
    const auto synthesize = [&](const QQmlJSScope::Ptr &current) {
        count = synthesizeCompilationUnitRuntimeFunctionIndices(current, count);
    };
    QQmlJSUtils::traverseFollowingQmlIrObjectStructure(m_exportedRootScope, synthesize);
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::ExpressionStatement *ast)
{
    if (m_pendingSignalHandler.isValid()) {
        enterEnvironment(QQmlSA::ScopeType::JSFunctionScope, u"signalhandler"_s,
                         ast->firstSourceLocation());
        flushPendingSignalParameters();
    }
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::ExpressionStatement *)
{
    if (m_currentScope->scopeType() == QQmlSA::ScopeType::JSFunctionScope
        && m_currentScope->baseTypeName() == u"signalhandler"_s) {
        leaveEnvironment();
    }
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::StringLiteral *sl)
{
    const QString s = m_logger->code().mid(sl->literalToken.begin(), sl->literalToken.length);

    if (s.contains(QLatin1Char('\r')) || s.contains(QLatin1Char('\n')) || s.contains(QChar(0x2028u))
        || s.contains(QChar(0x2029u))) {
        QString templateString;

        bool escaped = false;
        const QChar stringQuote = s[0];
        for (qsizetype i = 1; i < s.size() - 1; i++) {
            const QChar c = s[i];

            if (c == u'\\') {
                escaped = !escaped;
            } else if (escaped) {
                // If we encounter an escaped quote, unescape it since we use backticks here
                if (c == stringQuote)
                    templateString.chop(1);

                escaped = false;
            } else {
                if (c == u'`')
                    templateString += u'\\';
                if (c == u'$' && i + 1 < s.size() - 1 && s[i + 1] == u'{')
                    templateString += u'\\';
            }

            templateString += c;
        }

        QQmlJSFixSuggestion suggestion = { "Use a template literal instead."_L1, sl->literalToken,
                                           u"`" % templateString % u"`" };
        suggestion.setAutoApplicable();
        m_logger->log(QStringLiteral("String contains unescaped line terminator which is "
                                     "deprecated."),
                      qmlMultilineStrings, sl->literalToken, true, true, suggestion);
    }

    return true;
}

inline QQmlJSImportVisitor::UnfinishedBinding
createNonUniqueScopeBinding(QQmlJSScope::Ptr &scope, const QString &name,
                            const QQmlJS::SourceLocation &srcLocation);

static void logLowerCaseImport(QStringView superType, QQmlJS::SourceLocation location,
                               QQmlJSLogger *logger)
{
    QStringView namespaceName{ superType };
    namespaceName = namespaceName.first(namespaceName.indexOf(u'.'));
    logger->log(u"Namespace '%1' of '%2' must start with an upper case letter."_s.arg(namespaceName)
                        .arg(superType),
                qmlUncreatableType, location, true, true);
}

bool QQmlJSImportVisitor::visit(UiObjectDefinition *definition)
{
    const QString superType = buildName(definition->qualifiedTypeNameId);

    const bool isRoot = !rootScopeIsValid();
    Q_ASSERT(!superType.isEmpty());

    // we need to assume that it is a type based on its capitalization. Types defined in inline
    // components, for example, can have their type definition after their type usages:
    // Item { property IC myIC; component IC: Item{}; }
    const qsizetype indexOfTypeName = superType.lastIndexOf(u'.');
    const bool looksLikeGroupedProperty = superType.front().isLower();

    if (indexOfTypeName != -1 && looksLikeGroupedProperty) {
        logLowerCaseImport(superType, definition->qualifiedTypeNameId->identifierToken,
                           m_logger);
    }

    if (!looksLikeGroupedProperty) {
        if (!isRoot) {
            enterEnvironment(QQmlSA::ScopeType::QMLScope, superType,
                             definition->firstSourceLocation());
        } else {
            enterRootScope(QQmlSA::ScopeType::QMLScope, superType,
                           definition->firstSourceLocation());
            m_currentScope->setIsSingleton(m_rootIsSingleton);
        }

        const QTypeRevision revision = QQmlJSScope::resolveTypes(
                    m_currentScope, m_rootScopeImports, &m_usedTypes);
        if (auto base = m_currentScope->baseType(); base) {
            if (isRoot && base->internalName() == u"QQmlComponent") {
                m_logger->log(u"Qml top level type cannot be 'Component'."_s, qmlTopLevelComponent,
                              definition->qualifiedTypeNameId->identifierToken, true, true);
            }
            if (base->isSingleton() && m_currentScope->isComposite()) {
                m_logger->log(u"Singleton Type %1 is not creatable."_s.arg(
                                      m_currentScope->baseTypeName()),
                              qmlUncreatableType, definition->qualifiedTypeNameId->identifierToken,
                              true, true);

            } else if (!base->isCreatable()) {
                // composite type m_currentScope is allowed to be uncreatable, but it cannot be the base of anything else
                m_logger->log(u"Type %1 is not creatable."_s.arg(m_currentScope->baseTypeName()),
                              qmlUncreatableType, definition->qualifiedTypeNameId->identifierToken,
                              true, true);
            }
        }
        if (m_nextIsInlineComponent) {
            Q_ASSERT(std::holds_alternative<InlineComponentNameType>(m_currentRootName));
            const QString &name = std::get<InlineComponentNameType>(m_currentRootName);
            m_currentScope->setIsInlineComponent(true);
            m_currentScope->setInlineComponentName(name);
            m_rootScopeImports.setType(name, { m_currentScope, revision });
            m_nextIsInlineComponent = false;
        }

        addDefaultProperties();
        Q_ASSERT(m_currentScope->scopeType() == QQmlSA::ScopeType::QMLScope);
        m_qmlTypes.append(m_currentScope);

        m_objectDefinitionScopes << m_currentScope;
    } else {
        enterEnvironmentNonUnique(QQmlSA::ScopeType::GroupedPropertyScope, superType,
                                  definition->firstSourceLocation());
        m_bindings.append(createNonUniqueScopeBinding(m_currentScope, superType,
                                                      definition->firstSourceLocation()));
        QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports, &m_usedTypes);
    }

    m_currentScope->setAnnotations(parseAnnotations(definition->annotations));

    return true;
}

void QQmlJSImportVisitor::endVisit(UiObjectDefinition *)
{
    QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports, &m_usedTypes);
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(UiInlineComponent *component)
{
    if (!std::holds_alternative<RootDocumentNameType>(m_currentRootName)) {
        m_logger->log(u"Nested inline components are not supported"_s, qmlSyntax,
                      component->firstSourceLocation());
        return true;
    }

    m_nextIsInlineComponent = true;
    m_currentRootName = component->name.toString();
    return true;
}

void QQmlJSImportVisitor::endVisit(UiInlineComponent *component)
{
    m_currentRootName = RootDocumentNameType();
    if (m_nextIsInlineComponent) {
        m_logger->log(u"Inline component declaration must be followed by a typename"_s,
                      qmlSyntax, component->firstSourceLocation());
    }
    m_nextIsInlineComponent = false; // might have missed an inline component if file contains invalid QML
}

bool QQmlJSImportVisitor::visit(UiPublicMember *publicMember)
{
    switch (publicMember->type) {
    case UiPublicMember::Signal: {
        if (m_currentScope->ownMethods().contains(publicMember->name.toString())) {
            m_logger->log(QStringLiteral("Duplicated signal name \"%1\".").arg(
                publicMember->name.toString()), qmlDuplicatedName,
                publicMember->firstSourceLocation());
        }
        UiParameterList *param = publicMember->parameters;
        QQmlJSMetaMethod method;
        method.setMethodType(QQmlJSMetaMethodType::Signal);
        method.setMethodName(publicMember->name.toString());
        while (param) {
            method.addParameter(
                    QQmlJSMetaParameter(
                            param->name.toString(),
                            param->type ? param->type->toString() : QString()
                        ));
            param = param->next;
        }
        m_currentScope->addOwnMethod(method);
        break;
    }
    case UiPublicMember::Property: {
        if (m_currentScope->ownProperties().contains(publicMember->name.toString())) {
            m_logger->log(QStringLiteral("Duplicated property name \"%1\".").arg(
                publicMember->name.toString()), qmlDuplicatedName,
                publicMember->firstSourceLocation());
        }
        QString typeName = buildName(publicMember->memberType);
        if (typeName.contains(u'.') && typeName.front().isLower()) {
            logLowerCaseImport(typeName, publicMember->typeToken, m_logger);
        }

        QString aliasExpr;
        const bool isAlias = (typeName == u"alias"_s);
        if (isAlias) {
            auto tryParseAlias = [&]() {
            typeName.clear(); // type name is useless for alias here, so keep it empty
            if (!publicMember->statement) {
                m_logger->log(QStringLiteral("Invalid alias expression – an initalizer is needed."),
                              qmlSyntax, publicMember->memberType->firstSourceLocation()); // TODO: extend warning to cover until endSourceLocation
                return;
            }
            const auto expression = cast<ExpressionStatement *>(publicMember->statement);
            auto node = expression ? expression->expression : nullptr;
            auto fex = cast<FieldMemberExpression *>(node);
            while (fex) {
                node = fex->base;
                aliasExpr.prepend(u'.' + fex->name.toString());
                fex = cast<FieldMemberExpression *>(node);
            }

            if (const auto idExpression = cast<IdentifierExpression *>(node)) {
                aliasExpr.prepend(idExpression->name.toString());
            } else {
                // cast to expression might have failed above, so use publicMember->statement
                // to obtain the source location
                m_logger->log(QStringLiteral("Invalid alias expression. Only IDs and field "
                                             "member expressions can be aliased."),
                              qmlSyntax, publicMember->statement->firstSourceLocation());
            }
            };
            tryParseAlias();
        } else {
            if (m_rootScopeImports.hasType(typeName)
                && !m_rootScopeImports.type(typeName).scope.isNull()) {
                if (m_importTypeLocationMap.contains(typeName))
                    m_usedTypes.insert(typeName);
            }
        }
        QQmlJSMetaProperty prop;
        prop.setPropertyName(publicMember->name.toString());
        prop.setIsList(publicMember->typeModifier == QLatin1String("list"));
        prop.setIsWritable(!publicMember->isReadonly());
        prop.setAliasExpression(aliasExpr);
        const auto type =
                isAlias ? QQmlJSScope::ConstPtr() : m_rootScopeImports.type(typeName).scope;
        if (type) {
            prop.setType(prop.isList() ? type->listType() : type);
            const QString internalName = type->internalName();
            prop.setTypeName(internalName.isEmpty() ? typeName : internalName);
        } else if (!isAlias) {
            m_pendingPropertyTypes << PendingPropertyType { m_currentScope, prop.propertyName(),
                                                            publicMember->firstSourceLocation() };
            prop.setTypeName(typeName);
        }
        prop.setAnnotations(parseAnnotations(publicMember->annotations));
        if (publicMember->isDefaultMember())
            m_currentScope->setOwnDefaultPropertyName(prop.propertyName());
        prop.setIndex(m_currentScope->ownProperties().size());
        m_currentScope->insertPropertyIdentifier(prop);
        if (publicMember->isRequired())
            m_currentScope->setPropertyLocallyRequired(prop.propertyName(), true);

        BindingExpressionParseResult parseResult = BindingExpressionParseResult::Invalid;
        // if property is an alias, initialization expression is not a binding
        if (!isAlias) {
            parseResult =
                    parseBindingExpression(publicMember->name.toString(), publicMember->statement);
        }

        // however, if we have a property with a script binding assigned to it,
        // we have to create a new scope
        if (parseResult == BindingExpressionParseResult::Script) {
            Q_ASSERT(!m_savedBindingOuterScope); // automatically true due to grammar
            m_savedBindingOuterScope = m_currentScope;
            enterEnvironment(QQmlSA::ScopeType::JSFunctionScope, QStringLiteral("binding"),
                             publicMember->statement->firstSourceLocation());
        }

        break;
    }
    }

    return true;
}

void QQmlJSImportVisitor::endVisit(UiPublicMember *publicMember)
{
    if (m_savedBindingOuterScope) {
        m_currentScope = m_savedBindingOuterScope;
        m_savedBindingOuterScope = {};
        // m_savedBindingOuterScope is only set if we encounter a script binding
        forgetFunctionExpression(publicMember->name.toString());
    }
}

bool QQmlJSImportVisitor::visit(UiRequired *required)
{
    const QString name = required->name.toString();

    m_requiredProperties << RequiredProperty { m_currentScope, name,
                                               required->firstSourceLocation() };

    m_currentScope->setPropertyLocallyRequired(name, true);
    return true;
}

void QQmlJSImportVisitor::visitFunctionExpressionHelper(QQmlJS::AST::FunctionExpression *fexpr)
{
    using namespace QQmlJS::AST;
    auto name = fexpr->name.toString();
    if (!name.isEmpty()) {
        QQmlJSMetaMethod method(name);
        method.setMethodType(QQmlJSMetaMethodType::Method);

        if (!m_pendingMethodAnnotations.isEmpty()) {
            method.setAnnotations(m_pendingMethodAnnotations);
            m_pendingMethodAnnotations.clear();
        }

        // If signatures are explicitly ignored, we don't parse the types
        const bool parseTypes = m_scopesById.signaturesAreEnforced();

        bool formalsFullyTyped = parseTypes;
        bool anyFormalTyped = false;
        if (const auto *formals = parseTypes ? fexpr->formals : nullptr) {
            const auto parameters = formals->formals();
            for (const auto &parameter : parameters) {
                const QString type = parameter.typeAnnotation
                        ? parameter.typeAnnotation->type->toString()
                        : QString();
                if (type.isEmpty()) {
                    formalsFullyTyped = false;
                    method.addParameter(QQmlJSMetaParameter(parameter.id, QStringLiteral("var")));
                }  else {
                    anyFormalTyped = true;
                    method.addParameter(QQmlJSMetaParameter(parameter.id, type));
                }
            }
        }

        // If a function is fully typed, we can call it like a C++ function.
        method.setIsJavaScriptFunction(!formalsFullyTyped);

        // Methods with explicit return type return that.
        // Methods with only untyped arguments return an untyped value.
        // Methods with at least one typed argument but no explicit return type return void.
        // In order to make a function without arguments return void, you have to specify that.
        if (parseTypes && fexpr->typeAnnotation)
            method.setReturnTypeName(fexpr->typeAnnotation->type->toString());
        else if (anyFormalTyped)
            method.setReturnTypeName(QStringLiteral("void"));
        else
            method.setReturnTypeName(QStringLiteral("var"));

        method.setJsFunctionIndex(addFunctionOrExpression(m_currentScope, method.methodName()));
        m_currentScope->addOwnMethod(method);

        if (m_currentScope->scopeType() != QQmlSA::ScopeType::QMLScope) {
            m_currentScope->insertJSIdentifier(name,
                                               { QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
                                                 fexpr->firstSourceLocation(),
                                                 method.returnTypeName(), false });
        }
        enterEnvironment(QQmlSA::ScopeType::JSFunctionScope, name, fexpr->firstSourceLocation());
    } else {
        addFunctionOrExpression(m_currentScope, QStringLiteral("<anon>"));
        enterEnvironment(QQmlSA::ScopeType::JSFunctionScope, QStringLiteral("<anon>"),
                         fexpr->firstSourceLocation());
    }
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::FunctionExpression *fexpr)
{
    visitFunctionExpressionHelper(fexpr);
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::FunctionExpression *fexpr)
{
    forgetFunctionExpression(fexpr->name.toString());
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

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::FunctionDeclaration *fdecl)
{
    forgetFunctionExpression(fdecl->name.toString());
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::ClassExpression *ast)
{
    QQmlJSMetaProperty prop;
    prop.setPropertyName(ast->name.toString());
    m_currentScope->addOwnProperty(prop);
    enterEnvironment(QQmlSA::ScopeType::JSFunctionScope, ast->name.toString(),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::ClassExpression *)
{
    leaveEnvironment();
}

void handleTranslationBinding(QQmlJSMetaPropertyBinding &binding, QStringView base,
                              QQmlJS::AST::ArgumentList *args)
{
    QStringView contextString;
    QStringView mainString;
    QStringView commentString;
    auto registerContextString = [&](QStringView string) {
        contextString = string;
        return 0;
    };
    auto registerMainString = [&](QStringView string) {
        mainString = string;
        return 0;
    };
    auto registerCommentString = [&](QStringView string) {
        commentString = string;
        return 0;
    };
    auto finalizeBinding = [&](QV4::CompiledData::Binding::Type type,
                               QV4::CompiledData::TranslationData data) {
        if (type == QV4::CompiledData::Binding::Type_Translation) {
            binding.setTranslation(mainString, commentString, contextString, data.number);
        } else if (type == QV4::CompiledData::Binding::Type_TranslationById) {
            binding.setTranslationId(mainString, data.number);
        } else {
            binding.setStringLiteral(mainString);
        }
    };
    QmlIR::tryGeneratingTranslationBindingBase(
                base, args,
                registerMainString, registerCommentString, registerContextString, finalizeBinding);
}

QQmlJSImportVisitor::BindingExpressionParseResult
QQmlJSImportVisitor::parseBindingExpression(const QString &name,
                                            const QQmlJS::AST::Statement *statement)
{
    if (statement == nullptr)
        return BindingExpressionParseResult::Invalid;

    const auto *exprStatement = cast<const ExpressionStatement *>(statement);

    if (exprStatement == nullptr) {
        QQmlJS::SourceLocation location = statement->firstSourceLocation();

        if (const auto *block = cast<const Block *>(statement); block && block->statements) {
            location = block->statements->firstSourceLocation();
        }

        QQmlJSMetaPropertyBinding binding(location, name);
        binding.setScriptBinding(addFunctionOrExpression(m_currentScope, name),
                                 QQmlSA::ScriptBindingKind::PropertyBinding);
        m_bindings.append(UnfinishedBinding { m_currentScope, [=]() { return binding; } });
        return BindingExpressionParseResult::Script;
    }

    auto expr = exprStatement->expression;
    QQmlJSMetaPropertyBinding binding(
                combine(expr->firstSourceLocation(), expr->lastSourceLocation()),
                name);

    bool isUndefinedBinding = false;

    switch (expr->kind) {
    case Node::Kind_TrueLiteral:
        binding.setBoolLiteral(true);
        break;
    case Node::Kind_FalseLiteral:
        binding.setBoolLiteral(false);
        break;
    case Node::Kind_NullExpression:
        binding.setNullLiteral();
        break;
    case Node::Kind_IdentifierExpression: {
        auto idExpr = QQmlJS::AST::cast<QQmlJS::AST::IdentifierExpression *>(expr);
        Q_ASSERT(idExpr);
        isUndefinedBinding = (idExpr->name == u"undefined");
        break;
    }
    case Node::Kind_NumericLiteral:
        binding.setNumberLiteral(cast<NumericLiteral *>(expr)->value);
        break;
    case Node::Kind_StringLiteral:
        binding.setStringLiteral(cast<StringLiteral *>(expr)->value);
        break;
    case Node::Kind_RegExpLiteral:
        binding.setRegexpLiteral(cast<RegExpLiteral *>(expr)->pattern);
        break;
    case Node::Kind_TemplateLiteral: {
        auto templateLit = QQmlJS::AST::cast<QQmlJS::AST::TemplateLiteral *>(expr);
        Q_ASSERT(templateLit);
        if (templateLit->hasNoSubstitution) {
            binding.setStringLiteral(templateLit->value);
        } else {
            binding.setScriptBinding(addFunctionOrExpression(m_currentScope, name),
                                     QQmlSA::ScriptBindingKind::PropertyBinding);
            for (QQmlJS::AST::TemplateLiteral *l = templateLit; l; l = l->next) {
                if (QQmlJS::AST::ExpressionNode *expression = l->expression)
                    expression->accept(this);
            }
        }
        break;
    }
    default:
        if (QQmlJS::AST::UnaryMinusExpression *unaryMinus = QQmlJS::AST::cast<QQmlJS::AST::UnaryMinusExpression *>(expr)) {
            if (QQmlJS::AST::NumericLiteral *lit = QQmlJS::AST::cast<QQmlJS::AST::NumericLiteral *>(unaryMinus->expression))
                binding.setNumberLiteral(-lit->value);
        } else if (QQmlJS::AST::CallExpression *call = QQmlJS::AST::cast<QQmlJS::AST::CallExpression *>(expr)) {
            if (QQmlJS::AST::IdentifierExpression *base = QQmlJS::AST::cast<QQmlJS::AST::IdentifierExpression *>(call->base))
                handleTranslationBinding(binding, base->name, call->arguments);
        }
        break;
    }

    if (!binding.isValid()) {
        // consider this to be a script binding (see IRBuilder::setBindingValue)
        binding.setScriptBinding(addFunctionOrExpression(m_currentScope, name),
                                 QQmlSA::ScriptBindingKind::PropertyBinding,
                                 isUndefinedBinding ? ScriptBindingValueType::ScriptValue_Undefined
                                                    : ScriptBindingValueType::ScriptValue_Unknown);
    }
    m_bindings.append(UnfinishedBinding { m_currentScope, [=]() { return binding; } });

    // translations are neither literal bindings nor script bindings
    if (binding.bindingType() == QQmlSA::BindingType::Translation
        || binding.bindingType() == QQmlSA::BindingType::TranslationById) {
        return BindingExpressionParseResult::Translation;
    }
    if (!QQmlJSMetaPropertyBinding::isLiteralBinding(binding.bindingType()))
        return BindingExpressionParseResult::Script;
    m_literalScopesToCheck << m_currentScope;
    return BindingExpressionParseResult::Literal;
}

bool QQmlJSImportVisitor::isImportPrefix(QString prefix) const
{
    if (prefix.isEmpty() || !prefix.front().isUpper())
        return false;

    return m_rootScopeImports.isNullType(prefix);
}

void QQmlJSImportVisitor::handleIdDeclaration(QQmlJS::AST::UiScriptBinding *scriptBinding)
{
    const auto *statement = cast<ExpressionStatement *>(scriptBinding->statement);
    if (!statement) {
        m_logger->log(u"id must be followed by an identifier"_s, qmlSyntax,
                      scriptBinding->statement->firstSourceLocation());
        return;
    }
    const QString name = [&]() {
        if (const auto *idExpression = cast<IdentifierExpression *>(statement->expression))
            return idExpression->name.toString();
        else if (const auto *idString = cast<StringLiteral *>(statement->expression)) {
            m_logger->log(u"ids do not need quotation marks"_s, qmlSyntaxIdQuotation,
                          idString->firstSourceLocation());
            return idString->value.toString();
        }
        m_logger->log(u"Failed to parse id"_s, qmlSyntax,
                      statement->expression->firstSourceLocation());
        return QString();

    }();
    if (m_scopesById.existsAnywhereInDocument(name)) {
        // ### TODO: find an alternative to breakInhertianceCycles here
        // we shouldn't need to search for the current root component in any case here
        breakInheritanceCycles(m_currentScope);
        if (auto otherScopeWithID = m_scopesById.scope(name, m_currentScope)) {
            auto otherLocation = otherScopeWithID->sourceLocation();
            // critical because subsequent analysis cannot cope with messed up ids
            // and the file is invalid
            m_logger->log(u"Found a duplicated id. id %1 was first declared at %2:%3"_s.arg(
                                  name, QString::number(otherLocation.startLine),
                                  QString::number(otherLocation.startColumn)),
                          qmlSyntaxDuplicateIds, // ??
                          scriptBinding->firstSourceLocation());
        }
    }
    if (!name.isEmpty())
        m_scopesById.insert(name, m_currentScope);
}

/*! \internal

    Creates a new binding of either a GroupProperty or an AttachedProperty type.
    The binding is added to the parentScope() of \a scope, under property name
    \a name and location \a srcLocation.
*/
inline QQmlJSImportVisitor::UnfinishedBinding
createNonUniqueScopeBinding(QQmlJSScope::Ptr &scope, const QString &name,
                            const QQmlJS::SourceLocation &srcLocation)
{
    const auto createBinding = [=]() {
        const QQmlJSScope::ScopeType type = scope->scopeType();
        Q_ASSERT(type == QQmlSA::ScopeType::GroupedPropertyScope
                 || type == QQmlSA::ScopeType::AttachedPropertyScope);
        const QQmlSA::BindingType bindingType = (type == QQmlSA::ScopeType::GroupedPropertyScope)
                ? QQmlSA::BindingType::GroupProperty
                : QQmlSA::BindingType::AttachedProperty;

        const auto propertyBindings = scope->parentScope()->ownPropertyBindings(name);
        const bool alreadyHasBinding = std::any_of(propertyBindings.first, propertyBindings.second,
                                                   [&](const QQmlJSMetaPropertyBinding &binding) {
                                                       return binding.bindingType() == bindingType;
                                                   });
        if (alreadyHasBinding) // no need to create any more
            return QQmlJSMetaPropertyBinding(QQmlJS::SourceLocation {});

        QQmlJSMetaPropertyBinding binding(srcLocation, name);
        if (type == QQmlSA::ScopeType::GroupedPropertyScope)
            binding.setGroupBinding(static_cast<QSharedPointer<QQmlJSScope>>(scope));
        else
            binding.setAttachedBinding(static_cast<QSharedPointer<QQmlJSScope>>(scope));
        return binding;
    };
    return { scope->parentScope(), createBinding };
}

bool QQmlJSImportVisitor::visit(UiScriptBinding *scriptBinding)
{
    Q_ASSERT(!m_savedBindingOuterScope); // automatically true due to grammar
    Q_ASSERT(!m_thisScriptBindingIsJavaScript); // automatically true due to grammar
    m_savedBindingOuterScope = m_currentScope;
    const auto id = scriptBinding->qualifiedId;
    if (!id->next && id->name == QLatin1String("id")) {
        handleIdDeclaration(scriptBinding);
        return true;
    }

    auto group = id;

    QString prefix;
    for (; group->next; group = group->next) {
        const QString name = group->name.toString();
        if (name.isEmpty())
            break;

        if (group == id && isImportPrefix(name)) {
            prefix = name + u'.';
            continue;
        }

        const bool isAttachedProperty = name.front().isUpper();
        if (isAttachedProperty) {
            // attached property
            enterEnvironmentNonUnique(QQmlSA::ScopeType::AttachedPropertyScope, prefix + name,
                                      group->firstSourceLocation());
        } else {
            // grouped property
            enterEnvironmentNonUnique(QQmlSA::ScopeType::GroupedPropertyScope, prefix + name,
                                      group->firstSourceLocation());
        }
        m_bindings.append(createNonUniqueScopeBinding(m_currentScope, prefix + name,
                                                      group->firstSourceLocation()));

        prefix.clear();
    }

    auto name = group->name.toString();

    // This is a preliminary check.
    // Even if the name starts with "on", it might later turn out not to be a signal.
    const auto signal = QQmlJSUtils::signalName(name);

    if (!signal.has_value() || m_currentScope->hasProperty(name)) {
        m_propertyBindings[m_currentScope].append(
                { m_savedBindingOuterScope, group->firstSourceLocation(), name });
        // ### TODO: report Invalid parse status as a warning/error
        auto result = parseBindingExpression(name, scriptBinding->statement);
        m_thisScriptBindingIsJavaScript = (result == BindingExpressionParseResult::Script);
    } else {
        const auto statement = scriptBinding->statement;
        QStringList signalParameters;

        if (ExpressionStatement *expr = cast<ExpressionStatement *>(statement)) {
            if (FunctionExpression *func = expr->expression->asFunctionDefinition()) {
                for (FormalParameterList *formal = func->formals; formal; formal = formal->next)
                    signalParameters << formal->element->bindingIdentifier.toString();
            }
        }

        QQmlJSMetaMethod scopeSignal;
        const auto methods = m_currentScope->methods(*signal, QQmlJSMetaMethodType::Signal);
        if (!methods.isEmpty())
            scopeSignal = methods[0];

        const auto firstSourceLocation = statement->firstSourceLocation();
        bool hasMultilineStatementBody =
                statement->lastSourceLocation().startLine > firstSourceLocation.startLine;
        m_pendingSignalHandler = firstSourceLocation;
        m_signalHandlers.insert(firstSourceLocation,
                                { scopeSignal.parameterNames(), hasMultilineStatementBody });

        // NB: calculate runtime index right away to avoid miscalculation due to
        // losing real AST traversal order
        const auto index = addFunctionOrExpression(m_currentScope, name);
        const auto createBinding = [
                this,
                scope = m_currentScope,
                signalName = *signal,
                index,
                name,
                firstSourceLocation,
                groupLocation = group->firstSourceLocation(),
                signalParameters]() {
            // when encountering a signal handler, add it as a script binding
            Q_ASSERT(scope->isFullyResolved());
            QQmlSA::ScriptBindingKind kind = QQmlSA::ScriptBindingKind::Invalid;
            const auto methods = scope->methods(signalName, QQmlJSMetaMethodType::Signal);
            if (!methods.isEmpty()) {
                kind = QQmlSA::ScriptBindingKind::SignalHandler;
                checkSignal(scope, groupLocation, name, signalParameters);
            } else if (QQmlJSUtils::changeHandlerProperty(scope, signalName).has_value()) {
                kind = QQmlSA::ScriptBindingKind::ChangeHandler;
                checkSignal(scope, groupLocation, name, signalParameters);
            } else if (scope->hasProperty(name)) {
                // Not a signal handler after all.
                // We can see this now because the type is fully resolved.
                kind = QQmlSA::ScriptBindingKind::PropertyBinding;
                m_signalHandlers.remove(firstSourceLocation);
            } else {
                // We already know it's bad, but let's allow checkSignal() to do its thing.
                checkSignal(scope, groupLocation, name, signalParameters);
            }

            QQmlJSMetaPropertyBinding binding(firstSourceLocation, name);
            binding.setScriptBinding(index, kind);
            return binding;
        };
        m_bindings.append(UnfinishedBinding { m_currentScope, createBinding });
        m_thisScriptBindingIsJavaScript = true;
    }

    // TODO: before leaving the scopes, we must create the binding.

    // Leave any group/attached scopes so that the binding scope doesn't see its properties.
    while (m_currentScope->scopeType() == QQmlSA::ScopeType::GroupedPropertyScope
           || m_currentScope->scopeType() == QQmlSA::ScopeType::AttachedPropertyScope) {
        leaveEnvironment();
    }

    enterEnvironment(QQmlSA::ScopeType::JSFunctionScope, QStringLiteral("binding"),
                     scriptBinding->statement->firstSourceLocation());

    return true;
}

void QQmlJSImportVisitor::endVisit(UiScriptBinding *)
{
    if (m_savedBindingOuterScope) {
        m_currentScope = m_savedBindingOuterScope;
        m_savedBindingOuterScope = {};
    }

    // forgetFunctionExpression() but without the name check since script
    // bindings are special (script bindings only sometimes result in java
    // script bindings. e.g. a literal binding is also a UiScriptBinding)
    if (m_thisScriptBindingIsJavaScript) {
        m_thisScriptBindingIsJavaScript = false;
        Q_ASSERT(!m_functionStack.isEmpty());
        m_functionStack.pop();
    }
}

bool QQmlJSImportVisitor::visit(UiArrayBinding *arrayBinding)
{
    enterEnvironment(QQmlSA::ScopeType::QMLScope, buildName(arrayBinding->qualifiedId),
                     arrayBinding->firstSourceLocation());
    m_currentScope->setIsArrayScope(true);

    // TODO: support group/attached properties

    return true;
}

void QQmlJSImportVisitor::endVisit(UiArrayBinding *arrayBinding)
{
    // immediate children (QML scopes) of m_currentScope are the objects inside
    // the array binding. note that we always work with object bindings here as
    // this is the only kind of bindings that UiArrayBinding is created for. any
    // other expressions involving lists (e.g. `var p: [1,2,3]`) are considered
    // to be script bindings
    const auto children = m_currentScope->childScopes();
    const auto propertyName = getScopeName(m_currentScope, QQmlSA::ScopeType::QMLScope);
    leaveEnvironment();

    qsizetype i = 0;
    for (auto element = arrayBinding->members; element; element = element->next, ++i) {
        const auto &type = children[i];
        if ((type->scopeType() != QQmlSA::ScopeType::QMLScope)) {
            m_logger->log(u"Declaring an object which is not an Qml object"
                          " as a list member."_s, qmlSyntax, element->firstSourceLocation());
            return;
        }
        m_pendingPropertyObjectBindings
                << PendingPropertyObjectBinding { m_currentScope, type, propertyName,
                                                  element->firstSourceLocation(), false };
        QQmlJSMetaPropertyBinding binding(element->firstSourceLocation(), propertyName);
        binding.setObject(getScopeName(type, QQmlSA::ScopeType::QMLScope),
                          QQmlJSScope::ConstPtr(type));
        m_bindings.append(UnfinishedBinding { m_currentScope, [=]() { return binding; },
                                              QQmlJSScope::ListPropertyTarget });
    }
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiEnumDeclaration *uied)
{
    if (m_currentScope->inlineComponentName()) {
        m_logger->log(u"Enums declared inside of inline component are ignored."_s, qmlSyntax,
                      uied->firstSourceLocation());
    }
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

    // If it's not valid it's a builtin. We don't need to complain about it being unused.
    if (loc.isValid())
        m_importLocations.insert(loc);
}

void QQmlJSImportVisitor::importFromHost(const QString &path, const QString &prefix,
                                         const QQmlJS::SourceLocation &location)
{
    QFileInfo fileInfo(path);
    if (fileInfo.isFile()) {
        const auto scope = m_importer->importFile(path);
        const QString actualPrefix = prefix.isEmpty() ? scope->internalName() : prefix;
        m_rootScopeImports.setType(actualPrefix, { scope, QTypeRevision() });
        addImportWithLocation(actualPrefix, location);
    } else if (fileInfo.isDir()) {
        const auto scopes = m_importer->importDirectory(path, prefix);
        m_rootScopeImports.addTypes(scopes);
        for (auto it = scopes.types().keyBegin(), end = scopes.types().keyEnd(); it != end; it++)
            addImportWithLocation(*it, location);
    }
}

void QQmlJSImportVisitor::importFromQrc(const QString &path, const QString &prefix,
                                        const QQmlJS::SourceLocation &location)
{
    if (const auto &mapper = m_importer->resourceFileMapper()) {
        if (mapper->isFile(path)) {
            const auto entry = m_importer->resourceFileMapper()->entry(
                    QQmlJSResourceFileMapper::resourceFileFilter(path));
            const auto scope = m_importer->importFile(entry.filePath);
            const QString actualPrefix =
                    prefix.isEmpty() ? QFileInfo(entry.resourcePath).baseName() : prefix;
            m_rootScopeImports.setType(actualPrefix, { scope, QTypeRevision() });
            addImportWithLocation(actualPrefix, location);
        } else {
            const auto scopes = m_importer->importDirectory(path, prefix);
            m_rootScopeImports.addTypes(scopes);
            for (auto it = scopes.types().keyBegin(), end = scopes.types().keyEnd(); it != end; it++)
                addImportWithLocation(*it, location);
        }
    }
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiImport *import)
{
    // construct path
    QString prefix = QLatin1String("");
    if (import->asToken.isValid()) {
        prefix += import->importId;
        if (!import->importId.isEmpty() && !import->importId.front().isUpper()) {
            m_logger->log(u"Import qualifier '%1' must start with a capital letter."_s.arg(
                                  import->importId),
                          qmlImport, import->importIdToken, true, true);
        }
    }

    auto filename = import->fileName.toString();
    if (!filename.isEmpty()) {
        const QUrl url(filename);
        const QString scheme = url.scheme();
        const QQmlJS::SourceLocation importLocation = import->firstSourceLocation();
        if (scheme == ""_L1) {
            QFileInfo fileInfo(url.path());
            QString absolute = fileInfo.isRelative()
                    ? QDir::cleanPath(QDir(m_implicitImportDirectory).filePath(filename))
                    : filename;
            if (absolute.startsWith(u':')) {
                importFromQrc(absolute, prefix, importLocation);
            } else {
                importFromHost(absolute, prefix, importLocation);
            }
            processImportWarnings("path \"%1\""_L1.arg(url.path()), importLocation);
            return true;
        } else if (scheme == "file"_L1) {
            importFromHost(url.path(), prefix, importLocation);
            processImportWarnings("URL \"%1\""_L1.arg(url.path()), importLocation);
            return true;
        } else if (scheme == "qrc"_L1) {
            importFromQrc(":"_L1 + url.path(), prefix, importLocation);
            processImportWarnings("URL \"%1\""_L1.arg(url.path()), importLocation);
            return true;
        } else {
            m_logger->log("Unknown import syntax. Imports can be paths, qrc urls or file urls"_L1,
                          qmlImport, import->firstSourceLocation());
        }
    }

    const QString path = buildName(import->importUri);

    QStringList staticModulesProvided;

    const auto imported = m_importer->importModule(
            path, prefix, import->version ? import->version->version : QTypeRevision(),
            &staticModulesProvided);
    m_rootScopeImports.addTypes(imported);
    for (auto it = imported.types().keyBegin(), end = imported.types().keyEnd(); it != end; it++)
        addImportWithLocation(*it, import->firstSourceLocation());

    if (prefix.isEmpty()) {
        for (const QString &staticModule : staticModulesProvided) {
            // Always prefer a direct import of static module to it being imported as a dependency
            if (path != staticModule && m_importStaticModuleLocationMap.contains(staticModule))
                continue;

            m_importStaticModuleLocationMap[staticModule] = import->firstSourceLocation();
        }
    }

    processImportWarnings(QStringLiteral("module \"%1\"").arg(path), import->firstSourceLocation());
    return true;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
template<typename F>
void handlePragmaValues(QQmlJS::AST::UiPragma *pragma, F &&assign)
{
    for (const QQmlJS::AST::UiPragmaValueList *v = pragma->values; v; v = v->next)
        assign(v->value);
}
#else
template<typename F>
void handlePragmaValues(QQmlJS::AST::UiPragma *pragma, F &&assign)
{
    assign(pragma->value);
}
#endif

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiPragma *pragma)
{
    if (pragma->name == u"Strict"_s) {
        // If a file uses pragma Strict, it expects to be compiled, so automatically
        // enable compiler warnings unless the level is set explicitly already (e.g.
        // by the user).

        if (!m_logger->wasCategoryChanged(qmlCompiler)) {
            // TODO: the logic here is rather complicated and may be buggy
            m_logger->setCategoryLevel(qmlCompiler, QtWarningMsg);
            m_logger->setCategoryIgnored(qmlCompiler, false);
        }
    } else if (pragma->name == u"Singleton") {
        m_rootIsSingleton = true;
    } else if (pragma->name == u"ComponentBehavior") {
        handlePragmaValues(pragma, [this, pragma](QStringView value) {
            if (value == u"Bound") {
                m_scopesById.setComponentsAreBound(true);
            } else if (value == u"Unbound") {
                m_scopesById.setComponentsAreBound(false);
            } else {
                m_logger->log(
                        u"Unkonwn argument \"%s\" to pragma ComponentBehavior"_s.arg(value),
                        qmlSyntax, pragma->firstSourceLocation());
            }
        });
    } else if (pragma->name == u"FunctionSignatureBehavior") {
        handlePragmaValues(pragma, [this, pragma](QStringView value) {
            if (value == u"Enforced") {
                m_scopesById.setSignaturesAreEnforced(true);
            } else if (value == u"Ignored") {
                m_scopesById.setSignaturesAreEnforced(false);
            } else {
                m_logger->log(
                        u"Unkonwn argument \"%s\" to pragma FunctionSignatureBehavior"_s.arg(value),
                        qmlSyntax, pragma->firstSourceLocation());
            }
        });
    } else if (pragma->name == u"ValueTypeBehavior") {
        handlePragmaValues(pragma, [this, pragma](QStringView value) {
            if (value == u"Copy") {
                // Ignore
            } else if (value == u"Reference") {
                // Ignore
            } else if (value == u"Addressable") {
                m_scopesById.setValueTypesAreAddressable(true);
            } else if (value == u"Inaddressable") {
                m_scopesById.setValueTypesAreAddressable(false);
            } else {
                m_logger->log(
                        u"Unkonwn argument \"%s\" to pragma ValueTypeBehavior"_s.arg(value),
                        qmlSyntax, pragma->firstSourceLocation());
            }
        });
    }

    return true;
}

void QQmlJSImportVisitor::throwRecursionDepthError()
{
    m_logger->log(QStringLiteral("Maximum statement or expression depth exceeded"),
                  qmlRecursionDepthErrors, QQmlJS::SourceLocation());
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::ClassDeclaration *ast)
{
    enterEnvironment(QQmlSA::ScopeType::JSFunctionScope, ast->name.toString(),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::ClassDeclaration *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::ForStatement *ast)
{
    enterEnvironment(QQmlSA::ScopeType::JSLexicalScope, QStringLiteral("forloop"),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::ForStatement *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::ForEachStatement *ast)
{
    enterEnvironment(QQmlSA::ScopeType::JSLexicalScope, QStringLiteral("foreachloop"),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::ForEachStatement *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::Block *ast)
{
    enterEnvironment(QQmlSA::ScopeType::JSLexicalScope, QStringLiteral("block"),
                     ast->firstSourceLocation());

    if (m_pendingSignalHandler.isValid())
        flushPendingSignalParameters();

    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::Block *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::CaseBlock *ast)
{
    enterEnvironment(QQmlSA::ScopeType::JSLexicalScope, QStringLiteral("case"),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::CaseBlock *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::Catch *catchStatement)
{
    enterEnvironment(QQmlSA::ScopeType::JSLexicalScope, QStringLiteral("catch"),
                     catchStatement->firstSourceLocation());
    m_currentScope->insertJSIdentifier(
            catchStatement->patternElement->bindingIdentifier.toString(),
            { QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
              catchStatement->patternElement->firstSourceLocation(), std::nullopt,
              catchStatement->patternElement->scope == QQmlJS::AST::VariableScope::Const });
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::Catch *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::WithStatement *ast)
{
    enterEnvironment(QQmlSA::ScopeType::JSLexicalScope, QStringLiteral("with"),
                     ast->firstSourceLocation());

    m_logger->log(QStringLiteral("with statements are strongly discouraged in QML "
                                 "and might cause false positives when analysing unqualified "
                                 "identifiers"),
                  qmlWith, ast->firstSourceLocation());

    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::WithStatement *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::VariableDeclarationList *vdl)
{
    while (vdl) {
        std::optional<QString> typeName;
        if (TypeAnnotation *annotation = vdl->declaration->typeAnnotation)
            if (Type *type = annotation->type)
                typeName = type->toString();

        m_currentScope->insertJSIdentifier(
                vdl->declaration->bindingIdentifier.toString(),
                { (vdl->declaration->scope == QQmlJS::AST::VariableScope::Var)
                          ? QQmlJSScope::JavaScriptIdentifier::FunctionScoped
                          : QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
                  vdl->declaration->firstSourceLocation(), typeName,
                  vdl->declaration->scope == QQmlJS::AST::VariableScope::Const });
        vdl = vdl->next;
    }
    return true;
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::FormalParameterList *fpl)
{
    for (auto const &boundName : fpl->boundNames()) {

        std::optional<QString> typeName;
        if (TypeAnnotation *annotation = boundName.typeAnnotation.data())
            if (Type *type = annotation->type)
                typeName = type->toString();
        m_currentScope->insertJSIdentifier(boundName.id,
                                           { QQmlJSScope::JavaScriptIdentifier::Parameter,
                                             fpl->firstSourceLocation(), typeName, false });
    }
    return true;
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiObjectBinding *uiob)
{
    // ... __styleData: QtObject {...}

    Q_ASSERT(uiob->qualifiedTypeNameId);

    bool needsResolution = false;
    int scopesEnteredCounter = 0;

    const QString typeName = buildName(uiob->qualifiedTypeNameId);
    if (typeName.front().isLower() && typeName.contains(u'.')) {
        logLowerCaseImport(typeName, uiob->qualifiedTypeNameId->identifierToken, m_logger);
    }

    QString prefix;
    for (auto group = uiob->qualifiedId; group->next; group = group->next) {
        const QString idName = group->name.toString();

        if (idName.isEmpty())
            break;

        if (group == uiob->qualifiedId && isImportPrefix(idName)) {
            prefix = idName + u'.';
            continue;
        }

        const auto scopeKind = idName.front().isUpper() ? QQmlSA::ScopeType::AttachedPropertyScope
                                                        : QQmlSA::ScopeType::GroupedPropertyScope;

        bool exists =
                enterEnvironmentNonUnique(scopeKind, prefix + idName, group->firstSourceLocation());

        m_bindings.append(createNonUniqueScopeBinding(m_currentScope, prefix + idName,
                                                      group->firstSourceLocation()));

        ++scopesEnteredCounter;
        needsResolution = needsResolution || !exists;

        prefix.clear();
    }

    for (int i=0; i < scopesEnteredCounter; ++i) { // leave the scopes we entered again
        leaveEnvironment();
    }

    // recursively resolve types for current scope if new scopes are found
    if (needsResolution)
        QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports, &m_usedTypes);

    enterEnvironment(QQmlSA::ScopeType::QMLScope, typeName,
                     uiob->qualifiedTypeNameId->identifierToken);
    QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports, &m_usedTypes);

    m_qmlTypes.append(m_currentScope); // new QMLScope is created here, so add it
    m_objectBindingScopes << m_currentScope;
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::UiObjectBinding *uiob)
{
    QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports, &m_usedTypes);
    // must be mutable, as we might mark it as implicitly wrapped in a component
    const QQmlJSScope::Ptr childScope = m_currentScope;
    leaveEnvironment();

    auto group = uiob->qualifiedId;
    int scopesEnteredCounter = 0;

    QString prefix;
    for (; group->next; group = group->next) {
        const QString idName = group->name.toString();

        if (idName.isEmpty())
            break;

        if (group == uiob->qualifiedId && isImportPrefix(idName)) {
            prefix = idName + u'.';
            continue;
        }

        const auto scopeKind = idName.front().isUpper() ? QQmlSA::ScopeType::AttachedPropertyScope
                                                        : QQmlSA::ScopeType::GroupedPropertyScope;
        // definitely exists
        [[maybe_unused]] bool exists =
                enterEnvironmentNonUnique(scopeKind, prefix + idName, group->firstSourceLocation());
        Q_ASSERT(exists);
        scopesEnteredCounter++;

        prefix.clear();
    }

    // on ending the visit to UiObjectBinding, set the property type to the
    // just-visited one if the property exists and this type is valid

    const QString propertyName = group->name.toString();

    if (m_currentScope->isNameDeferred(propertyName)) {
        bool foundIds = false;
        QList<QQmlJSScope::ConstPtr> childScopes { childScope };

        while (!childScopes.isEmpty()) {
            const QQmlJSScope::ConstPtr scope = childScopes.takeFirst();
            if (!m_scopesById.id(scope, scope).isEmpty()) {
                foundIds = true;
                break;
            }

            childScopes << scope->childScopes();
        }

        if (foundIds) {
            m_logger->log(
                    u"Cannot defer property assignment to \"%1\". Assigning an id to an object or one of its sub-objects bound to a deferred property will make the assignment immediate."_s
                            .arg(propertyName),
                    qmlDeferredPropertyId, uiob->firstSourceLocation());
        }
    }

    if (m_currentScope->isInCustomParserParent()) {
        // These warnings do not apply for custom parsers and their children and need to be handled
        // on a case by case basis
    } else {
        m_pendingPropertyObjectBindings
                << PendingPropertyObjectBinding { m_currentScope, childScope, propertyName,
                                                  uiob->firstSourceLocation(), uiob->hasOnToken };

        QQmlJSMetaPropertyBinding binding(uiob->firstSourceLocation(), propertyName);
        if (uiob->hasOnToken) {
            if (childScope->hasInterface(u"QQmlPropertyValueInterceptor"_s)) {
                binding.setInterceptor(getScopeName(childScope, QQmlSA::ScopeType::QMLScope),
                                       QQmlJSScope::ConstPtr(childScope));
            } else { // if (childScope->hasInterface(u"QQmlPropertyValueSource"_s))
                binding.setValueSource(getScopeName(childScope, QQmlSA::ScopeType::QMLScope),
                                       QQmlJSScope::ConstPtr(childScope));
            }
        } else {
            binding.setObject(getScopeName(childScope, QQmlSA::ScopeType::QMLScope),
                              QQmlJSScope::ConstPtr(childScope));
        }
        m_bindings.append(UnfinishedBinding { m_currentScope, [=]() { return binding; } });
    }

    for (int i = 0; i < scopesEnteredCounter; ++i)
        leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(ExportDeclaration *)
{
    Q_ASSERT(rootScopeIsValid());
    Q_ASSERT(m_exportedRootScope != m_globalScope);
    Q_ASSERT(m_currentScope == m_globalScope);
    m_currentScope = m_exportedRootScope;
    return true;
}

void QQmlJSImportVisitor::endVisit(ExportDeclaration *)
{
    Q_ASSERT(rootScopeIsValid());
    m_currentScope = m_exportedRootScope->parentScope();
    Q_ASSERT(m_currentScope == m_globalScope);
}

bool QQmlJSImportVisitor::visit(ESModule *module)
{
    Q_ASSERT(!rootScopeIsValid());
    enterRootScope(QQmlSA::ScopeType::JSLexicalScope, QStringLiteral("module"),
                   module->firstSourceLocation());
    m_currentScope->setIsScript(true);
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
    Q_ASSERT(!rootScopeIsValid());
    *m_exportedRootScope = std::move(*QQmlJSScope::clone(m_currentScope));
    m_exportedRootScope->setIsScript(true);
    m_currentScope = m_exportedRootScope;
    importBaseModules();
    return true;
}

void QQmlJSImportVisitor::endVisit(Program *)
{
    QQmlJSScope::resolveTypes(m_exportedRootScope, m_rootScopeImports, &m_usedTypes);
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::FieldMemberExpression *fieldMember)
{
    // This is a rather rough approximation of "used type" but the "unused import"
    // info message doesn't have to be 100% accurate.
    const QString name = fieldMember->name.toString();
    if (m_importTypeLocationMap.contains(name)) {
        const QQmlJSImportedScope type = m_rootScopeImports.type(name);
        if (type.scope.isNull()) {
            if (m_rootScopeImports.hasType(name))
                m_usedTypes.insert(name);
        } else if (!type.scope->ownAttachedTypeName().isEmpty()) {
            m_usedTypes.insert(name);
        }
    }
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::IdentifierExpression *idexp)
{
    const QString name = idexp->name.toString();
    if (m_importTypeLocationMap.contains(name)) {
        m_usedTypes.insert(name);
    }

    return true;
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::PatternElement *element)
{
    // Handles variable declarations such as var x = [1,2,3].
    if (element->isVariableDeclaration()) {
        QQmlJS::AST::BoundNames names;
        element->boundNames(&names);
        for (const auto &name : names) {
            std::optional<QString> typeName;
            if (TypeAnnotation *annotation = name.typeAnnotation.data())
                if (Type *type = annotation->type)
                    typeName = type->toString();
            m_currentScope->insertJSIdentifier(
                    name.id,
                    { (element->scope == QQmlJS::AST::VariableScope::Var)
                              ? QQmlJSScope::JavaScriptIdentifier::FunctionScoped
                              : QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
                      element->firstSourceLocation(), typeName,
                      element->scope == QQmlJS::AST::VariableScope::Const });
        }
    }

    return true;
}

QT_END_NAMESPACE
