// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljslintertypepropagator_p.h"

#include <private/qqmljsutils_p.h>

#include <private/qqmljslintercodegen_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQmlJSLinterTypePropagator::QQmlJSLinterTypePropagator(
        const QV4::Compiler::JSUnitGenerator *unitGenerator, const QQmlJSTypeResolver *typeResolver,
        QQmlJSLogger *logger, const QQmlJS::LinterContext &context, const BasicBlocks &basicBlocks,
        const InstructionAnnotations &annotations, QQmlSA::PassManager *passManager)
    : QQmlJSTypePropagator(unitGenerator, typeResolver, logger, basicBlocks, annotations),
      m_passManager(passManager),
      m_context(context)
{
}

void QQmlJSLinterTypePropagator::generate_Ret()
{
    QQmlJSTypePropagator::generate_Ret();

    if (m_function->isSignalHandler) {
        // Signal handlers cannot return anything.
    } else if (m_state.accumulatorIn().contains(m_typeResolver->voidType())) {
        // You can always return undefined.
    } else if (!m_returnType.isValid() && m_state.accumulatorIn().isValid()) {
        if (m_function->isFullyTyped) {
            // Do not complain if the function didn't have a valid annotation in the first place.
            m_logger->log(u"Function without return type annotation returns %1"_s.arg(
                                  m_state.accumulatorIn().containedTypeName()),
                          qmlIncompatibleType, currentFunctionSourceLocation());
        }
    } else if (!canConvertFromTo(m_state.accumulatorIn(), m_returnType)) {
        m_logger->log(u"Cannot assign binding of type %1 to %2"_s.arg(
                              m_state.accumulatorIn().containedTypeName(),
                              m_returnType.containedTypeName()),
                      qmlIncompatibleType, currentFunctionSourceLocation());
    }

    const QQmlJS::SourceLocation location = m_function->isProperty
            ? currentFunctionSourceLocation()
            : currentNonEmptySourceLocation();
    QQmlSA::PassManagerPrivate::get(m_passManager)
            ->analyzeBinding(
                    QQmlJSScope::createQQmlSAElement(m_function->qmlScope.containedType()),
                    QQmlJSScope::createQQmlSAElement(m_state.accumulatorIn().containedType()),
                    QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(location));
}

void QQmlJSLinterTypePropagator::generate_LoadQmlContextPropertyLookup(int index)
{
    QQmlJSTypePropagator::generate_LoadQmlContextPropertyLookup(index);

    Q_ASSERT(m_idMemberShadows);

    const int nameIndex = m_jsUnitGenerator->lookupNameIndex(index);
    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);

    const auto qmlScope = m_function->qmlScope.containedType();
    QQmlSA::PassManagerPrivate::get(m_passManager)->analyzeRead(
            QQmlJSScope::createQQmlSAElement(qmlScope), name,
            QQmlJSScope::createQQmlSAElement(qmlScope),
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                    currentNonEmptySourceLocation()));

    const auto &accumulatorOut = m_state.accumulatorOut();
    if (!accumulatorOut.isValid())
        return;

    // complain about renamed types in enums like MyType.Enum.EnumValue
    if (accumulatorOut.variant() == QQmlJSRegisterContent::Attachment
        || accumulatorOut.variant() == QQmlJSRegisterContent::MetaType) {
        m_context.renamedComponents.handleRenamedType(accumulatorOut.scopeType(), name,
                                                      currentNonEmptySourceLocation(), m_logger);
    }

    const QQmlJSScope::ConstPtr scope = accumulatorOut.scopeType();
    const QQmlJSScope::ConstPtr idScope = m_context.scopesById.scope(name, scope);
    if (!idScope.isNull()) {
        const auto log = [&](const auto &memberType, const auto &memberOwnerScope) {
            IdMemberShadow idMemberShadow{ name, idScope, memberOwnerScope };

            // Only warn once per shadowing instance, even for multiple usages.
            if (m_idMemberShadows->contains(idMemberShadow))
                return;

            m_idMemberShadows->insert(std::move(idMemberShadow));
            const auto useLoc = currentSourceLocation();
            m_logger->log("Id for object %1 shadows %2 \"%3\". Rename one or the other."_L1
                                  .arg(idScope->baseTypeName(), memberType, name),
                          qmlIdShadowsMember, useLoc);
            m_logger->log("Note: Id defined here"_L1, qmlIdShadowsMember,
                          idScope->idSourceLocation(), true, true, {}, useLoc.startLine);
        };

        if (scope->hasProperty(name)) {
            log("property"_L1, scope->ownerOfProperty(scope, name).scope);
        } else if (scope->hasMethod(name)) {
            const auto methods = scope->methods(name);
            const auto &method = methods[0];
            if (method.methodType() == QQmlSA::MethodType::Method)
                log("method"_L1, scope->ownerOfMethod(scope, name).scope);
            else if (method.methodType() == QQmlSA::MethodType::Signal)
                log("signal"_L1, scope->ownerOfMethod(scope, name).scope);
        }
    }
}

void QQmlJSLinterTypePropagator::generate_GetOptionalLookup(int index, int offset)
{
    QQmlJSTypePropagator::generate_GetOptionalLookup(index, offset);

    auto suggMsg = "Consider using non-optional chaining instead: '?.' -> '.'"_L1;
    auto suggestion = std::make_optional(QQmlJSFixSuggestion(suggMsg, currentSourceLocation()));
    if (m_state.accumulatorOut().variant() == QQmlJSRegisterContent::Enum) {
        m_logger->log("Redundant optional chaining for enum lookup"_L1, qmlRedundantOptionalChaining,
                      currentSourceLocation(), true, true, suggestion);
    } else if (!m_state.accumulatorIn().containedType()->isReferenceType()
               && !m_typeResolver->canHoldUndefined(m_state.accumulatorIn())) {
        auto baseType = m_state.accumulatorIn().containedTypeName();
        m_logger->log("Redundant optional chaining for lookup on non-voidable and non-nullable "_L1
                      "type %1"_L1.arg(baseType), qmlRedundantOptionalChaining,
                      currentSourceLocation(), true, true, suggestion);
    }
}

void QQmlJSLinterTypePropagator::generate_StoreProperty(int nameIndex, int base)
{
    QQmlJSTypePropagator::generate_StoreProperty(nameIndex, base);

    auto callBase = m_state.registers[base].content;
    const QString propertyName = m_jsUnitGenerator->stringForIndex(nameIndex);
    const bool isAttached = callBase.variant() == QQmlJSRegisterContent::Attachment;

    QQmlSA::PassManagerPrivate::get(m_passManager)->analyzeWrite(
            QQmlJSScope::createQQmlSAElement(callBase.containedType()),
            propertyName,
            QQmlJSScope::createQQmlSAElement(
                    m_state.accumulatorIn().containedType()),
            QQmlJSScope::createQQmlSAElement(isAttached
                                                     ? callBase.attachee().containedType()
                                                     : m_function->qmlScope.containedType()),
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                    currentNonEmptySourceLocation()));
}

void QQmlJSLinterTypePropagator::generate_CallProperty(int nameIndex, int base, int argc, int argv)
{
    QQmlJSTypePropagator::generate_CallProperty(nameIndex, base, argc, argv);

    const auto saCheck = [&](const QString &propertyName, const QQmlJSScope::ConstPtr &baseType) {
        const QQmlSA::Element saBaseType{ QQmlJSScope::createQQmlSAElement(baseType) };
        const QQmlSA::Element saContainedType{ QQmlJSScope::createQQmlSAElement(
                m_function->qmlScope.containedType()) };
        const QQmlSA::SourceLocation saLocation{
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(currentSourceLocation())
        };

        QQmlSA::PassManagerPrivate::get(m_passManager)
                ->analyzeRead(saBaseType, propertyName, saContainedType, saLocation);
        QQmlSA::PassManagerPrivate::get(m_passManager)
                ->analyzeCall(saBaseType, propertyName, saContainedType, saLocation);
    };

    const auto callBase = m_state.registers[base].content;
    const QString propertyName = m_jsUnitGenerator->stringForIndex(nameIndex);
    const auto member = m_typeResolver->memberType(callBase, propertyName);

    const bool isLoggingMethod = QQmlJSTypePropagator::isLoggingMethod(propertyName);
    if (callBase.contains(m_typeResolver->mathObject()))
        saCheck(propertyName, callBase.containedType());
    else if (callBase.contains(m_typeResolver->consoleObject()) && isLoggingMethod)
        saCheck(propertyName, callBase.containedType());
    else if (!member.isMethod()) {
        if (callBase.contains(m_typeResolver->jsValueType())
            || callBase.contains(m_typeResolver->varType())) {
            saCheck(propertyName, callBase.containedType());
        }
    }
}

void QQmlJSLinterTypePropagator::generate_CallPossiblyDirectEval(int argc, int argv)
{
    QQmlJSTypePropagator::generate_CallPossiblyDirectEval(argc, argv);

    const QQmlSA::SourceLocation saLocation{
        QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(currentSourceLocation())
    };
    const QQmlSA::Element saBaseType{ QQmlJSScope::createQQmlSAElement(
            m_typeResolver->jsGlobalObject()) };
    const QQmlSA::Element saContainedType{ QQmlJSScope::createQQmlSAElement(
            m_function->qmlScope.containedType()) };

    QQmlSA::PassManagerPrivate::get(m_passManager)
            ->analyzeCall(saBaseType, "eval"_L1, saContainedType, saLocation);
}

void QQmlJSLinterTypePropagator::handleUnqualifiedAccess(const QString &name, bool isMethod) const
{
    QQmlJSTypePropagator::handleUnqualifiedAccess(name, isMethod);

    auto location = currentSourceLocation();

    const auto qmlScopeContained = m_function->qmlScope.containedType();
    if (qmlScopeContained->isInCustomParserParent()) {
        // Only ignore custom parser based elements if it's not Connections.
        if (qmlScopeContained->baseType().isNull()
            || qmlScopeContained->baseType()->internalName() != u"QQmlConnections"_s)
            return;
    }

    if (isMethod) {
        if (isCallingProperty(qmlScopeContained, name))
            return;
    } else if (propertyResolution(qmlScopeContained, name) != PropertyMissing) {
        return;
    }

    std::optional<QQmlJSFixSuggestion> suggestion;

    const auto childScopes = m_function->qmlScope.containedType()->childScopes();
    for (qsizetype i = 0, end = childScopes.size(); i < end; i++) {
        auto &scope = childScopes[i];
        if (location.offset > scope->sourceLocation().offset) {
            if (i + 1 < end
                && childScopes.at(i + 1)->sourceLocation().offset < location.offset)
                continue;
            if (scope->childScopes().size() == 0)
                continue;

            const auto jsId = scope->childScopes().first()->jsIdentifier(name);

            if (jsId.has_value() && jsId->kind == QQmlJSScope::JavaScriptIdentifier::Injected) {
                const QQmlJSScope::JavaScriptIdentifier id = jsId.value();

                QQmlJS::SourceLocation fixLocation = id.location;
                Q_UNUSED(fixLocation)
                fixLocation.length = 0;

                const auto handler = m_typeResolver->signalHandlers()[id.location];

                QString fixString = handler.isMultiline ? u"function("_s : u"("_s;
                const auto parameters = handler.signalParameters;
                for (int numParams = parameters.size(); numParams > 0; --numParams) {
                    fixString += parameters.at(parameters.size() - numParams);
                    if (numParams > 1)
                        fixString += u", "_s;
                }

                fixString += handler.isMultiline ? u") "_s : u") => "_s;
                const auto msg = u"\"%1\" is ambiguous. Use a function instead: %2%3"_s.arg(
                        name, fixString, handler.isMultiline ? "{ ... }"_L1 : "..."_L1);
                QQmlJSDocumentEdit documentEdit{ m_logger->filePath(), fixLocation, fixString };
                suggestion = {{ msg, fixLocation, documentEdit }};
                suggestion->setAutoApplicable();
            }
            break;
        }
    }

    // Might be a delegate just missing a required property.
    // This heuristic does not recognize all instances of this occurring but should be sufficient
    // protection against wrongly suggesting to add an id to the view to access the model that way
    // which is very misleading
    const auto qmlScope = m_function->qmlScope.containedType();
    if (name == u"model" || name == u"index") {
        if (const QQmlJSScope::ConstPtr parent = qmlScope->parentScope(); !parent.isNull()) {
            const auto bindings = parent->ownPropertyBindings(u"delegate"_s);

            for (auto it = bindings.first; it != bindings.second; it++) {
                if (!it->hasObject())
                    continue;
                if (it->objectType() == qmlScope) {
                    suggestion = QQmlJSFixSuggestion {
                        "'%1' is implicitly injected into this delegate. "
                        "Add a required property '%1' to the delegate instead."_L1
                                .arg(name),
                        qmlScope->sourceLocation()
                    };
                };

                break;
            }
        }
    }

    if (!suggestion.has_value()) {
        for (QQmlJSScope::ConstPtr scope = qmlScope; !scope.isNull(); scope = scope->parentScope()) {
            if (scope->hasProperty(name)) {
                QQmlJSScopesById::MostLikelyCallback<QString> id;
                m_function->addressableScopes.possibleIds(scope, qmlScope,
                                                          QQmlJSScopesByIdOption::Default, id);

                QQmlJS::SourceLocation fixLocation = location;
                fixLocation.length = 0;
                QString m = "%1 is a member of a parent element.\n      You can qualify the "
                            "access with its id to avoid this warning%2.\n"_L1.arg(name);
                m = m.arg(id.result.isEmpty() ? " (You first have to give the element an id)"_L1 : ""_L1);

                suggestion = QQmlJSFixSuggestion{
                    m, fixLocation, { m_logger->filePath(), fixLocation,
                                      (id.result.isEmpty() ? u"<id>."_s : (id.result + u'.')) }
                };

                if (!id.result.isEmpty())
                    suggestion->setAutoApplicable();
            }
        }
    }

    if (!suggestion.has_value() && !m_function->addressableScopes.componentsAreBound()
        && m_function->addressableScopes.existsAnywhereInDocument(name)) {
        const QLatin1String replacement = "pragma ComponentBehavior: Bound"_L1;
        QQmlJSFixSuggestion bindComponents {
            "Set \"%1\" in order to use IDs from outer components in nested components."_L1
                    .arg(replacement),
            QQmlJS::s_documentOrigin,
            QQmlJSDocumentEdit{ m_logger->filePath(), QQmlJS::s_documentOrigin, replacement + u'\n' }
        };
        bindComponents.setAutoApplicable();
        suggestion = std::move(bindComponents);
    }

    if (!suggestion.has_value()) {
        if (auto didYouMean = QQmlJSUtils::didYouMean(
                    name, qmlScope->properties().keys() + qmlScope->methods().keys(),
                    m_logger->filePath(), location);
                didYouMean.has_value()) {
            suggestion = std::move(didYouMean);
        }
    }

    m_logger->log(QLatin1String("Unqualified access"), qmlUnqualified, location, true, true,
                  suggestion);
}

static bool shouldMentionRequiredProperties(const QQmlJSScope::ConstPtr &qmlScope)
{
    if (!qmlScope->isWrappedInImplicitComponent() && !qmlScope->isFileRootComponent()
        && !qmlScope->isInlineComponent()) {
        return false;
    }

    const auto properties = qmlScope->properties();
    return std::none_of(properties.constBegin(), properties.constEnd(),
                        [&qmlScope](const QQmlJSMetaProperty &property) {
                            return qmlScope->isPropertyRequired(property.propertyName());
                        });
}

void QQmlJSLinterTypePropagator::handleUnqualifiedAccessAndContextProperties(
        const QString &name, bool isMethod) const
{
    QQmlJSTypePropagator::handleUnqualifiedAccessAndContextProperties(name, isMethod);

    if (m_context.userContextProperties.isUnqualifiedAccessDisabled(name))
        return;

    const auto warningMessage = [&name, this]() {
        QString result =
                "Potential context property access detected."
                " Context properties are discouraged in QML: use normal, required, or singleton properties instead."_L1;

        if (shouldMentionRequiredProperties(m_function->qmlScope.containedType())) {
            result.append(
                    "\nNote: '%1' assumed to be a potential context property because it is not declared as required property."_L1
                            .arg(name));
        }
        return result;
    };

    if (m_context.userContextProperties.isOnUsageWarned(name)) {
        m_logger->log(warningMessage(), qmlContextProperties, currentSourceLocation());
        return;
    }

    // name is not the name of a user context property, so emit the unqualified warning.
    handleUnqualifiedAccess(name, isMethod);

    const QList<QQmlJS::HeuristicContextProperty> definitions =
            m_context.heuristicContextProperties.definitionsForName(name);
    if (definitions.isEmpty())
        return;
    QString warning = warningMessage();
    for (const auto &candidate : definitions) {
        warning.append("\nNote: candidate context property declaration '%1' at %2:%3:%4"_L1.arg(
                name, QDir::cleanPath(candidate.filename),
                QString::number(candidate.location.startLine),
                QString::number(candidate.location.startColumn)));
    }
    m_logger->log(warning, qmlContextProperties, currentSourceLocation());
}

void QQmlJSLinterTypePropagator::checkDeprecated(QQmlJSScope::ConstPtr scope, const QString &name,
                                           bool isMethod) const
{
    QQmlJSTypePropagator::checkDeprecated(scope, name, isMethod);

    Q_ASSERT(!scope.isNull());
    auto qmlScope = QQmlJSScope::findCurrentQMLScope(scope);
    if (qmlScope.isNull())
        return;

    QList<QQmlJSAnnotation> annotations;

    QQmlJSMetaMethod method;

    if (isMethod) {
        const QList<QQmlJSMetaMethod> methods = qmlScope->methods(name);
        if (methods.isEmpty())
            return;
        method = methods.constFirst();
        annotations = method.annotations();
    } else {
        QQmlJSMetaProperty property = qmlScope->property(name);
        if (!property.isValid())
            return;
        annotations = property.annotations();
    }

    auto deprecationAnn = std::find_if(
            annotations.constBegin(), annotations.constEnd(),
            [](const QQmlJSAnnotation &annotation) { return annotation.isDeprecation(); });

    if (deprecationAnn == annotations.constEnd())
        return;

    QQQmlJSDeprecation deprecation = deprecationAnn->deprecation();

    QString descriptor = name;
    if (isMethod)
        descriptor += u'(' + method.parameterNames().join(u", "_s) + u')';

    QString message = "%1 \"%2\" is deprecated"_L1
                              .arg(isMethod ? u"Method"_s : u"Property"_s, descriptor);

    if (!deprecation.reason.isEmpty())
        message.append(QStringLiteral(" (Reason: %1)").arg(deprecation.reason));

    m_logger->log(message, qmlDeprecated, currentSourceLocation());
}

bool QQmlJSLinterTypePropagator::isCallingProperty(QQmlJSScope::ConstPtr scope,
                                                   const QString &name) const
{
    const bool res = QQmlJSTypePropagator::isCallingProperty(scope, name);

    if (const auto property = scope->property(name); property.isValid()) {
        QString errorType;
        if (property.type() == m_typeResolver->varType()) {
            errorType = u"a var property. It may or may not be a method. "_s
                        u"Use a regular function instead."_s;
        } else if (property.type() == m_typeResolver->jsValueType()) {
            errorType = u"a QJSValue property. It may or may not be a method. "_s
                        u"Use a regular Q_INVOKABLE instead."_s;
        } else {
            errorType = u"not a method"_s;
        }

        m_logger->log(u"Property \"%1\" is %2"_s.arg(name, errorType),
                      qmlUseProperFunction, currentSourceLocation(), true, true, {});
    }

    return res;
}

bool QQmlJSLinterTypePropagator::handleImportNamespaceLookup(const QString &propertyName)
{
    const auto res = QQmlJSTypePropagator::handleImportNamespaceLookup(propertyName);

    const QQmlJSRegisterContent accumulatorIn = m_state.accumulatorIn();
    if (m_typeResolver->isPrefix(propertyName)) {
        if (!accumulatorIn.containedType()->isReferenceType()) {
            m_logger->log(u"Cannot use non-QObject type %1 to access prefixed import"_s.arg(
                                  accumulatorIn.containedType()->internalName()),
                          qmlPrefixedImportType,
                          currentSourceLocation());
        }
    } else if (accumulatorIn.isImportNamespace()) {
        m_logger->log(u"Type not found in namespace"_s, qmlUnresolvedType,
                      currentSourceLocation());
    }

    return res;
}

bool QQmlJSLinterTypePropagator::checkTypeResolved(const QQmlJSScope::ConstPtr &type)
{
    if (type->isFullyResolved() || type->isScript())
        return true;

    if (!m_context.knownUnresolvedTypes.hasSeen(type)) {

        m_logger->log(QStringLiteral("Type %1 is used but it is not resolved")
                              .arg(QQmlJSUtils::getScopeName(type, type->scopeType())),
                      qmlUnresolvedType, currentSourceLocation());
    }

    return false;
}

void QQmlJSLinterTypePropagator::handleLookupError(const QString &propertyName)
{
    QQmlJSTypePropagator::handleLookupError(propertyName);

    const QQmlJSRegisterContent accumulatorIn = m_state.accumulatorIn();
    const QString typeName = accumulatorIn.containedTypeName();

    if (typeName == u"QVariant")
        return;
    if (accumulatorIn.isList() && propertyName == u"length")
        return;

    auto baseType = accumulatorIn.containedType();
    // Warn separately when a property is only not found because of a missing type

    if (propertyResolution(baseType, propertyName) != PropertyMissing)
        return;

    if (baseType->isScript())
        return;

    std::optional<QQmlJSFixSuggestion> fixSuggestion;

    if (auto suggestion = QQmlJSUtils::didYouMean(propertyName, baseType->properties().keys(),
                                                  m_logger->filePath(), currentSourceLocation());
            suggestion.has_value()) {
        fixSuggestion = std::move(suggestion);
    }

    if (!fixSuggestion.has_value()
        && accumulatorIn.variant() == QQmlJSRegisterContent::MetaType) {

        const QQmlJSScope::ConstPtr scopeType = accumulatorIn.scopeType();
        const auto metaEnums = scopeType->enumerations();
        const bool enforcesScoped = scopeType->enforcesScopedEnums();

        QStringList enumKeys;
        for (const QQmlJSMetaEnum &metaEnum : metaEnums) {
            if (!enforcesScoped || !metaEnum.isScoped())
                enumKeys << metaEnum.keys();
        }

        if (auto suggestion = QQmlJSUtils::didYouMean(
                    propertyName, enumKeys, m_logger->filePath(), currentSourceLocation());
                suggestion.has_value()) {
            fixSuggestion = std::move(suggestion);
        }
    }

    if (checkTypeResolved(baseType)) {
        m_logger->log(u"Member \"%1\" not found on type \"%2\""_s.arg(propertyName, typeName),
                      qmlMissingProperty, currentSourceLocation(), true, true, fixSuggestion);
    }
}

bool QQmlJSLinterTypePropagator::checkForEnumProblems(QQmlJSRegisterContent base,
                                                      const QString &propertyName)
{
    const bool res = QQmlJSTypePropagator::checkForEnumProblems(base, propertyName);

    if (base.isEnumeration()) {
        const auto metaEnum = base.enumeration();
        if (!metaEnum.hasKey(propertyName)) {
            const auto fixSuggestion = QQmlJSUtils::didYouMean(
                    propertyName, metaEnum.keys(), m_logger->filePath(), currentSourceLocation());
            const QString error = u"\"%1\" is not an entry of enum \"%2\"."_s
                                          .arg(propertyName, metaEnum.name());
            m_logger->log(error, qmlMissingEnumEntry, currentSourceLocation(), true, true,
                          fixSuggestion);
        }
    }

    return res;
}

void QQmlJSLinterTypePropagator::generate_StoreNameCommon(int nameIndex)
{
    QQmlJSTypePropagator::generate_StoreNameCommon(nameIndex);

    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);
    const QQmlJSRegisterContent in = m_state.accumulatorIn();
    const bool isAttached = in.variant() == QQmlJSRegisterContent::Attachment;

    QQmlSA::PassManagerPrivate::get(m_passManager)->analyzeRead(
            QQmlJSScope::createQQmlSAElement(
                    m_state.accumulatorIn().containedType()),
            name,
            QQmlJSScope::createQQmlSAElement(isAttached
                    ? in.attachee().containedType()
                    : m_function->qmlScope.containedType()),
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                    currentNonEmptySourceLocation()));

    const auto qmlScope = m_function->qmlScope.containedType();
    QQmlSA::PassManagerPrivate::get(m_passManager)->analyzeWrite(
            QQmlJSScope::createQQmlSAElement(qmlScope), name,
            QQmlJSScope::createQQmlSAElement(in.containedType()),
            QQmlJSScope::createQQmlSAElement(qmlScope),
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                    currentNonEmptySourceLocation()));
}

void QQmlJSLinterTypePropagator::propagatePropertyLookup(const QString &name, int lookupIndex)
{
    QQmlJSTypePropagator::propagatePropertyLookup(name, lookupIndex);

    const QQmlJSRegisterContent in = m_state.accumulatorIn();
    const bool isAttached = in.variant() == QQmlJSRegisterContent::Attachment;

    QQmlSA::PassManagerPrivate::get(m_passManager)->analyzeRead(
            QQmlJSScope::createQQmlSAElement(
                    m_state.accumulatorIn().containedType()),
            name,
            QQmlJSScope::createQQmlSAElement(isAttached
                    ? in.attachee().containedType()
                    : m_function->qmlScope.containedType()),
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                    currentNonEmptySourceLocation()));
}

void QQmlJSLinterTypePropagator::propagateCall(const QList<QQmlJSMetaMethod> &methods, int argc, int argv,
                                               QQmlJSRegisterContent scope)
{
    QQmlJSTypePropagator::propagateCall(methods, argc, argv, scope);

    QStringList errors;
    const QQmlJSMetaMethod match = bestMatchForCall(methods, argc, argv, &errors);
    if (!match.isValid())
        return;

    const QQmlSA::Element saBaseType = QQmlJSScope::createQQmlSAElement(scope.containedType());
    const QQmlSA::SourceLocation saLocation{
        QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(currentSourceLocation())
    };
    const QQmlSA::Element saContainedType{ QQmlJSScope::createQQmlSAElement(
            m_function->qmlScope.containedType()) };

    QQmlSA::PassManagerPrivate::get(m_passManager)
            ->analyzeCall(saBaseType, match.methodName(), saContainedType, saLocation);
}

void QQmlJSLinterTypePropagator::propagateTranslationMethod_SAcheck(const QString &methodName)
{
    QQmlJSTypePropagator::propagateTranslationMethod_SAcheck(methodName);

    QQmlSA::PassManagerPrivate::get(m_passManager)
            ->analyzeCall(QQmlJSScope::createQQmlSAElement(m_typeResolver->jsGlobalObject()),
                          methodName,
                          QQmlJSScope::createQQmlSAElement(m_function->qmlScope.containedType()),
                          QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                                  currentNonEmptySourceLocation()));
}

static bool mightContainStringOrNumberOrBoolean(const QQmlJSScope::ConstPtr &scope,
                                                const QQmlJSTypeResolver *resolver)
{
    return scope == resolver->varType() || scope == resolver->jsValueType()
    || scope == resolver->jsPrimitiveType();
}

static bool isStringOrNumberOrBoolean(const QQmlJSScope::ConstPtr &scope,
                                      const QQmlJSTypeResolver *resolver)
{
    return scope == resolver->boolType() || scope == resolver->stringType()
    || resolver->isNumeric(scope);
}

static bool isVoidOrUndefined(const QQmlJSScope::ConstPtr &scope,
                              const QQmlJSTypeResolver *resolver)
{
    return scope == resolver->nullType() || scope == resolver->voidType();
}

static bool requiresStrictEquality(const QQmlJSScope::ConstPtr &lhs,
                                   const QQmlJSScope::ConstPtr &rhs,
                                   const QQmlJSTypeResolver *resolver)
{
    if (lhs == rhs)
        return false;

    if (resolver->isNumeric(lhs) && resolver->isNumeric(rhs))
        return false;

    if (isVoidOrUndefined(lhs, resolver) || isVoidOrUndefined(rhs, resolver))
        return false;

    if (isStringOrNumberOrBoolean(lhs, resolver)
        && !mightContainStringOrNumberOrBoolean(rhs, resolver)) {
        return true;
    }

    if (isStringOrNumberOrBoolean(rhs, resolver)
        && !mightContainStringOrNumberOrBoolean(lhs, resolver)) {
        return true;
    }

    return false;
}

void QQmlJSLinterTypePropagator::warnAboutTypeCoercion(int lhs)
{
    const QQmlJSScope::ConstPtr lhsType = checkedInputRegister(lhs).containedType();
    const QQmlJSScope::ConstPtr rhsType = m_state.accumulatorIn().containedType();

    if (!requiresStrictEquality(lhsType, rhsType, m_typeResolver))
        return;

    m_logger->log("== and != may perform type coercion, use === or !== to avoid it."_L1,
                  qmlEqualityTypeCoercion, currentNonEmptySourceLocation());
}

QT_END_NAMESPACE
