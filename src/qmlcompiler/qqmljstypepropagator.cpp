// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsscope_p.h"
#include "qqmljstypepropagator_p.h"

#include "qqmljsutils_p.h"
#include "qqmlsa_p.h"

#include <private/qv4compilerscanfunctions_p.h>

#include <QtQmlCompiler/private/qqmlsasourcelocation_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
 * \internal
 * \class QQmlJSTypePropagator
 *
 * QQmlJSTypePropagator is the initial pass that performs the type inference and
 * annotates every register in use at any instruction with the possible types it
 * may hold. This includes information on how and in what scope the values are
 * retrieved. These annotations may be used by further compile passes for
 * refinement or code generation.
 */

QQmlJSTypePropagator::QQmlJSTypePropagator(const QV4::Compiler::JSUnitGenerator *unitGenerator,
                                           const QQmlJSTypeResolver *typeResolver,
                                           QQmlJSLogger *logger,  QQmlSA::PassManager *passManager)
    : QQmlJSCompilePass(unitGenerator, typeResolver, logger),
      m_passManager(passManager)
{
}

QQmlJSCompilePass::InstructionAnnotations QQmlJSTypePropagator::run(
        const Function *function, QQmlJS::DiagnosticMessage *error)
{
    m_function = function;
    m_error = error;
    m_returnType = m_typeResolver->globalType(m_function->returnType);

    do {
        // Reset the error if we need to do another pass
        if (m_state.needsMorePasses)
            *m_error = QQmlJS::DiagnosticMessage();

        m_prevStateAnnotations = m_state.annotations;
        m_state = PassState();
        m_state.State::operator=(initialState(m_function));

        reset();
        decode(m_function->code.constData(), static_cast<uint>(m_function->code.size()));

        // If we have found unresolved backwards jumps, we need to start over with a fresh state.
        // Mind that m_jumpOriginRegisterStateByTargetInstructionOffset is retained in that case.
        // This means that we won't start over for the same reason again.
    } while (m_state.needsMorePasses);

    return m_state.annotations;
}

#define INSTR_PROLOGUE_NOT_IMPLEMENTED()                                              \
  setError(u"Instruction \"%1\" not implemented"_s.arg(QString::fromUtf8(__func__))); \
  return;

#define INSTR_PROLOGUE_NOT_IMPLEMENTED_IGNORE()                                                    \
    m_logger->log(u"Instruction \"%1\" not implemented"_s.arg(QString::fromUtf8(__func__)),        \
                  qmlCompiler, QQmlJS::SourceLocation());                                          \
    return;

void QQmlJSTypePropagator::generate_Ret()
{
    if (m_passManager != nullptr && m_function->isProperty) {
        m_passManager->d_func()->analyzeBinding(
                QQmlJSScope::createQQmlSAElement(m_function->qmlScope),
                QQmlJSScope::createQQmlSAElement(
                        m_typeResolver->containedType(m_state.accumulatorIn())),
                QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                        getCurrentBindingSourceLocation()));
    }

    if (m_function->isSignalHandler) {
        // Signal handlers cannot return anything.
    } else if (m_typeResolver->registerContains(
                       m_state.accumulatorIn(), m_typeResolver->voidType())) {
        // You can always return undefined.
    } else if (!m_returnType.isValid() && m_state.accumulatorIn().isValid()) {
        setError(u"function without type annotation returns %1"_s
                         .arg(m_state.accumulatorIn().descriptiveName()));
        return;
    } else if (!canConvertFromTo(m_state.accumulatorIn(), m_returnType)) {
        setError(u"cannot convert from %1 to %2"_s
                         .arg(m_state.accumulatorIn().descriptiveName(),
                              m_returnType.descriptiveName()));

        m_logger->log(u"Cannot assign binding of type %1 to %2"_s.arg(
                              m_typeResolver->containedTypeName(m_state.accumulatorIn(), true),
                              m_typeResolver->containedTypeName(m_returnType, true)),
                      qmlIncompatibleType, getCurrentBindingSourceLocation());
        return;
    }

    if (m_returnType.isValid()) {
        // We need to preserve any possible undefined value as that resets the property.
        if (m_typeResolver->canHoldUndefined(m_state.accumulatorIn()))
            addReadAccumulator(m_state.accumulatorIn());
        else
            addReadAccumulator(m_returnType);
    }

    m_state.setHasSideEffects(true);
    m_state.skipInstructionsUntilNextJumpTarget = true;
}

void QQmlJSTypePropagator::generate_Debug()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_LoadConst(int index)
{
    auto encodedConst = m_jsUnitGenerator->constant(index);
    setAccumulator(m_typeResolver->globalType(m_typeResolver->typeForConst(encodedConst)));
}

void QQmlJSTypePropagator::generate_LoadZero()
{
    setAccumulator(m_typeResolver->globalType(m_typeResolver->int32Type()));
}

void QQmlJSTypePropagator::generate_LoadTrue()
{
    setAccumulator(m_typeResolver->globalType(m_typeResolver->boolType()));
}

void QQmlJSTypePropagator::generate_LoadFalse()
{
    setAccumulator(m_typeResolver->globalType(m_typeResolver->boolType()));
}

void QQmlJSTypePropagator::generate_LoadNull()
{
    setAccumulator(m_typeResolver->globalType(m_typeResolver->nullType()));
}

void QQmlJSTypePropagator::generate_LoadUndefined()
{
    setAccumulator(m_typeResolver->globalType(m_typeResolver->voidType()));
}

void QQmlJSTypePropagator::generate_LoadInt(int)
{
    setAccumulator(m_typeResolver->globalType(m_typeResolver->int32Type()));
}

void QQmlJSTypePropagator::generate_MoveConst(int constIndex, int destTemp)
{
    auto encodedConst = m_jsUnitGenerator->constant(constIndex);
    setRegister(destTemp, m_typeResolver->globalType(m_typeResolver->typeForConst(encodedConst)));
}

void QQmlJSTypePropagator::generate_LoadReg(int reg)
{
    // Do not re-track the register. We're not manipulating it.
    m_state.setIsRename(true);
    const QQmlJSRegisterContent content = checkedInputRegister(reg);
    m_state.addReadRegister(reg, content);
    m_state.setRegister(Accumulator, content);
}

void QQmlJSTypePropagator::generate_StoreReg(int reg)
{
    // Do not re-track the register. We're not manipulating it.
    m_state.setIsRename(true);
    m_state.addReadAccumulator(m_state.accumulatorIn());
    m_state.setRegister(reg, m_state.accumulatorIn());
}

void QQmlJSTypePropagator::generate_MoveReg(int srcReg, int destReg)
{
    Q_ASSERT(destReg != InvalidRegister);
    // Do not re-track the register. We're not manipulating it.
    m_state.setIsRename(true);
    const QQmlJSRegisterContent content = checkedInputRegister(srcReg);
    m_state.addReadRegister(srcReg, content);
    m_state.setRegister(destReg, content);
}

void QQmlJSTypePropagator::generate_LoadImport(int index)
{
    Q_UNUSED(index)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_LoadLocal(int index)
{
    Q_UNUSED(index);
    setAccumulator(m_typeResolver->globalType(m_typeResolver->jsValueType()));
}

void QQmlJSTypePropagator::generate_StoreLocal(int index)
{
    Q_UNUSED(index)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_LoadScopedLocal(int scope, int index)
{
    Q_UNUSED(scope)
    Q_UNUSED(index)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_StoreScopedLocal(int scope, int index)
{
    Q_UNUSED(scope)
    Q_UNUSED(index)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_LoadRuntimeString(int stringId)
{
    Q_UNUSED(stringId)
    setAccumulator(m_typeResolver->globalType(m_typeResolver->stringType()));
    //    m_state.accumulatorOut.m_state.value = m_jsUnitGenerator->stringForIndex(stringId);
}

void QQmlJSTypePropagator::generate_MoveRegExp(int regExpId, int destReg)
{
    Q_UNUSED(regExpId)
    Q_UNUSED(destReg)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_LoadClosure(int value)
{
    Q_UNUSED(value)
    // TODO: Check the function at index and see whether it's a generator to return another type
    // instead.
    setAccumulator(m_typeResolver->globalType(m_typeResolver->functionType()));
}

void QQmlJSTypePropagator::generate_LoadName(int nameIndex)
{
    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);
    setAccumulator(m_typeResolver->scopedType(m_function->qmlScope, name));
    if (!m_state.accumulatorOut().isValid())
        setError(u"Cannot find name "_s + name);
}

void QQmlJSTypePropagator::generate_LoadGlobalLookup(int index)
{
    generate_LoadName(m_jsUnitGenerator->lookupNameIndex(index));
}

QQmlJS::SourceLocation QQmlJSTypePropagator::getCurrentSourceLocation() const
{
    Q_ASSERT(m_function->sourceLocations);
    const auto &entries = m_function->sourceLocations->entries;

    auto item = std::lower_bound(entries.begin(), entries.end(), currentInstructionOffset(),
                                 [](auto entry, uint offset) { return entry.offset < offset; });
    Q_ASSERT(item != entries.end());
    auto location = item->location;

    return location;
}

QQmlJS::SourceLocation QQmlJSTypePropagator::getCurrentBindingSourceLocation() const
{
    Q_ASSERT(m_function->sourceLocations);
    const auto &entries = m_function->sourceLocations->entries;

    Q_ASSERT(!entries.isEmpty());
    return combine(entries.constFirst().location, entries.constLast().location);
}

void QQmlJSTypePropagator::handleUnqualifiedAccess(const QString &name, bool isMethod) const
{
    auto location = getCurrentSourceLocation();

    if (m_function->qmlScope->isInCustomParserParent()) {
        // Only ignore custom parser based elements if it's not Connections.
        if (m_function->qmlScope->baseType().isNull()
            || m_function->qmlScope->baseType()->internalName() != u"QQmlConnections"_s)
            return;
    }

    if (isMethod) {
        if (isCallingProperty(m_function->qmlScope, name))
            return;
    } else if (propertyResolution(m_function->qmlScope, name) != PropertyMissing) {
        return;
    }

    std::optional<QQmlJSFixSuggestion> suggestion;

    auto childScopes = m_function->qmlScope->childScopes();
    for (qsizetype i = 0; i < m_function->qmlScope->childScopes().size(); i++) {
        auto &scope = childScopes[i];
        if (location.offset > scope->sourceLocation().offset) {
            if (i + 1 < childScopes.size()
                && childScopes.at(i + 1)->sourceLocation().offset < location.offset)
                continue;
            if (scope->childScopes().size() == 0)
                continue;

            const auto jsId = scope->childScopes().first()->findJSIdentifier(name);

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

                suggestion = QQmlJSFixSuggestion {
                    name + u" is accessible in this scope because you are handling a signal"
                           " at %1:%2. Use a function instead.\n"_s
                        .arg(id.location.startLine)
                        .arg(id.location.startColumn),
                    fixLocation,
                    fixString
                };
                suggestion->setAutoApplicable();
            }
            break;
        }
    }

    // Might be a delegate just missing a required property.
    // This heuristic does not recognize all instances of this occurring but should be sufficient
    // protection against wrongly suggesting to add an id to the view to access the model that way
    // which is very misleading
    if (name == u"model" || name == u"index") {
        if (QQmlJSScope::ConstPtr parent = m_function->qmlScope->parentScope(); !parent.isNull()) {
            const auto bindings = parent->ownPropertyBindings(u"delegate"_s);

            for (auto it = bindings.first; it != bindings.second; it++) {
                if (!it->hasObject())
                    continue;
                if (it->objectType() == m_function->qmlScope) {
                    suggestion = QQmlJSFixSuggestion {
                        name + " is implicitly injected into this delegate."
                               " Add a required property instead."_L1,
                        m_function->qmlScope->sourceLocation()
                    };
                };

                break;
            }
        }
    }

    if (!suggestion.has_value()) {
        for (QQmlJSScope::ConstPtr scope = m_function->qmlScope; !scope.isNull();
             scope = scope->parentScope()) {
            if (scope->hasProperty(name)) {
                const QString id = m_function->addressableScopes.id(scope, m_function->qmlScope);

                QQmlJS::SourceLocation fixLocation = location;
                fixLocation.length = 0;
                suggestion = QQmlJSFixSuggestion{
                    name
                            + " is a member of a parent element.\n      You can qualify the access "
                              "with its id to avoid this warning.\n"_L1,
                    fixLocation, (id.isEmpty() ? u"<id>."_s : (id + u'.'))
                };

                if (id.isEmpty())
                    suggestion->setHint("You first have to give the element an id"_L1);
                else
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
            QQmlJS::SourceLocation(0, 0, 1, 1),
            replacement + '\n'_L1
        };
        bindComponents.setAutoApplicable();
        suggestion = bindComponents;
    }

    if (!suggestion.has_value()) {
        if (auto didYouMean =
                    QQmlJSUtils::didYouMean(name,
                                            m_function->qmlScope->properties().keys()
                                                    + m_function->qmlScope->methods().keys(),
                                            location);
            didYouMean.has_value()) {
            suggestion = didYouMean;
        }
    }

    m_logger->log(QLatin1String("Unqualified access"), qmlUnqualified, location, true, true,
                  suggestion);
}

void QQmlJSTypePropagator::checkDeprecated(QQmlJSScope::ConstPtr scope, const QString &name,
                                           bool isMethod) const
{
    Q_ASSERT(!scope.isNull());
    auto qmlScope = QQmlJSScope::findCurrentQMLScope(scope);
    if (qmlScope.isNull())
        return;

    QList<QQmlJSAnnotation> annotations;

    QQmlJSMetaMethod method;

    if (isMethod) {
        const QVector<QQmlJSMetaMethod> methods = qmlScope->methods(name);
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

    QString message = QStringLiteral("%1 \"%2\" is deprecated")
                              .arg(isMethod ? u"Method"_s : u"Property"_s)
                              .arg(descriptor);

    if (!deprecation.reason.isEmpty())
        message.append(QStringLiteral(" (Reason: %1)").arg(deprecation.reason));

    m_logger->log(message, qmlDeprecated, getCurrentSourceLocation());
}

// Only to be called once a lookup has already failed
QQmlJSTypePropagator::PropertyResolution QQmlJSTypePropagator::propertyResolution(
        QQmlJSScope::ConstPtr scope, const QString &propertyName) const
{
    auto property = scope->property(propertyName);
    if (!property.isValid())
        return PropertyMissing;

    QString errorType;
    if (property.type().isNull())
        errorType = u"found"_s;
    else if (!property.type()->isFullyResolved())
        errorType = u"fully resolved"_s;
    else
        return PropertyFullyResolved;

    Q_ASSERT(!errorType.isEmpty());

    m_logger->log(
            u"Type \"%1\" of property \"%2\" not %3. This is likely due to a missing dependency entry or a type not being exposed declaratively."_s
                    .arg(property.typeName(), propertyName, errorType),
            qmlUnresolvedType, getCurrentSourceLocation());

    return PropertyTypeUnresolved;
}

bool QQmlJSTypePropagator::isCallingProperty(QQmlJSScope::ConstPtr scope, const QString &name) const
{
    auto property = scope->property(name);
    if (!property.isValid())
        return false;

    QString propertyType = u"Property"_s;

    auto methods = scope->methods(name);

    QString errorType;
    if (!methods.isEmpty()) {
        errorType = u"shadowed by a property."_s;
        switch (methods.first().methodType()) {
        case QQmlJSMetaMethodType::Signal:
            propertyType = u"Signal"_s;
            break;
        case QQmlJSMetaMethodType::Slot:
            propertyType = u"Slot"_s;
            break;
        case QQmlJSMetaMethodType::Method:
            propertyType = u"Method"_s;
            break;
        default:
            Q_UNREACHABLE();
        }
    } else if (m_typeResolver->equals(property.type(), m_typeResolver->varType())) {
        errorType =
                u"a variant property. It may or may not be a method. Use a regular function instead."_s;
    } else if (m_typeResolver->equals(property.type(), m_typeResolver->jsValueType())) {
        errorType =
                u"a QJSValue property. It may or may not be a method. Use a regular Q_INVOKABLE instead."_s;
    } else {
        errorType = u"not a method"_s;
    }

    m_logger->log(u"%1 \"%2\" is %3"_s.arg(propertyType, name, errorType), qmlUseProperFunction,
                  getCurrentSourceLocation(), true, true, {});

    return true;
}

void QQmlJSTypePropagator::generate_LoadQmlContextPropertyLookup(int index)
{
    // LoadQmlContextPropertyLookup does not use accumulatorIn. It always refers to the scope.
    // Any import namespaces etc. are handled via LoadProperty or GetLookup.

    const int nameIndex = m_jsUnitGenerator->lookupNameIndex(index);
    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);

    setAccumulator(m_typeResolver->scopedType(m_function->qmlScope, name));

    if (!m_state.accumulatorOut().isValid() && m_typeResolver->isPrefix(name)) {
        const QQmlJSRegisterContent inType = m_typeResolver->globalType(m_function->qmlScope);
        setAccumulator(QQmlJSRegisterContent::create(
                    m_typeResolver->voidType(), nameIndex, QQmlJSRegisterContent::ScopeModulePrefix,
                    m_typeResolver->containedType(inType)));
        return;
    }

    checkDeprecated(m_function->qmlScope, name, false);

    if (!m_state.accumulatorOut().isValid()) {
        setError(u"Cannot access value for name "_s + name);
        handleUnqualifiedAccess(name, false);
        return;
    }

    const QQmlJSScope::ConstPtr outStored
            = m_typeResolver->genericType(m_state.accumulatorOut().storedType());

    if (outStored.isNull()) {
        // It should really be valid.
        // We get the generic type from aotContext->loadQmlContextPropertyIdLookup().
        setError(u"Cannot determine generic type for "_s + name);
        return;
    }

    if (m_state.accumulatorOut().variant() == QQmlJSRegisterContent::ObjectById
            && !outStored->isReferenceType()) {
        setError(u"Cannot retrieve a non-object type by ID: "_s + name);
        return;
    }

    if (m_passManager != nullptr) {
        m_passManager->d_func()->analyzeRead(
                QQmlJSScope::createQQmlSAElement(m_function->qmlScope), name,
                QQmlJSScope::createQQmlSAElement(m_function->qmlScope),
                QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                        getCurrentBindingSourceLocation()));
    }

    if (m_state.accumulatorOut().variant() == QQmlJSRegisterContent::ScopeAttached)
        m_attachedContext = QQmlJSScope::ConstPtr();
}

/*!
    \internal
    As far as type propagation is involved, StoreNameSloppy and
    StoreNameStrict are completely the same
    StoreNameStrict is rejecting a few writes (where the variable was not
    defined before) that would work in a sloppy context in JS, but the
    compiler would always reject this. And for type propagation, this does
    not matter at all.
    \a nameIndex is the index in the string table corresponding to
    the name which we are storing
 */
void QQmlJSTypePropagator::generate_StoreNameCommon(int nameIndex)
{
    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);
    const QQmlJSRegisterContent type = m_typeResolver->scopedType(m_function->qmlScope, name);
    const QQmlJSRegisterContent in = m_state.accumulatorIn();

    if (!type.isValid()) {
        handleUnqualifiedAccess(name, false);
        setError(u"Cannot find name "_s + name);
        return;
    }

    if (!type.isProperty()) {
        QString message = type.isMethod() ? u"Cannot assign to method %1"_s
                                          : u"Cannot assign to non-property %1"_s;
        // The interpreter treats methods as read-only properties in its error messages
        // and we lack a better fitting category. We might want to revisit this later.
        m_logger->log(message.arg(name), qmlReadOnlyProperty,
                      getCurrentSourceLocation());
        setError(u"Cannot assign to non-property "_s + name);
        return;
    }

    if (!type.isWritable()) {
        setError(u"Can't assign to read-only property %1"_s.arg(name));

        m_logger->log(u"Cannot assign to read-only property %1"_s.arg(name), qmlReadOnlyProperty,
                      getCurrentSourceLocation());

        return;
    }

    if (!canConvertFromTo(in, type)) {
        setError(u"cannot convert from %1 to %2"_s
                 .arg(in.descriptiveName(), type.descriptiveName()));
    }

    if (m_passManager != nullptr) {
        m_passManager->d_func()->analyzeWrite(
                QQmlJSScope::createQQmlSAElement(m_function->qmlScope), name,
                QQmlJSScope::createQQmlSAElement(m_typeResolver->containedType(in)),
                QQmlJSScope::createQQmlSAElement(m_function->qmlScope),
                QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                        getCurrentBindingSourceLocation()));
    }

    m_state.setHasSideEffects(true);

    if (m_typeResolver->canHoldUndefined(in) && !m_typeResolver->canHoldUndefined(type)) {
        if (type.property().reset().isEmpty())
            setError(u"Cannot assign potential undefined to %1"_s.arg(type.descriptiveName()));
        else if (m_typeResolver->registerIsStoredIn(in, m_typeResolver->voidType()))
            addReadAccumulator(m_typeResolver->globalType(m_typeResolver->varType()));
        else
            addReadAccumulator(in);
    } else {
        addReadAccumulator(type);
    }
}

void QQmlJSTypePropagator::generate_StoreNameSloppy(int nameIndex)
{
    return generate_StoreNameCommon(nameIndex);
}

void QQmlJSTypePropagator::generate_StoreNameStrict(int name)
{
    return generate_StoreNameCommon(name);
}

bool QQmlJSTypePropagator::checkForEnumProblems(
        const QQmlJSRegisterContent &base, const QString &propertyName) const
{
    if (base.isEnumeration()) {
        const auto metaEn = base.enumeration();
        if (!metaEn.isScoped()) {
            m_logger->log(u"You cannot access unscoped enum \"%1\" from here."_s.arg(propertyName),
                          qmlRestrictedType, getCurrentSourceLocation());
            return true;
        } else if (!metaEn.hasKey(propertyName)) {
            auto fixSuggestion = QQmlJSUtils::didYouMean(propertyName, metaEn.keys(),
                                                         getCurrentSourceLocation());
            m_logger->log(u"\"%1\" is not an entry of enum \"%2\"."_s.arg(propertyName)
                                  .arg(metaEn.name()),
                          qmlMissingEnumEntry, getCurrentSourceLocation(), true, true,
                          fixSuggestion);
            return true;
        }
    }
    return false;
}

void QQmlJSTypePropagator::generate_LoadElement(int base)
{
    const QQmlJSRegisterContent baseRegister = m_state.registers[base].content;

    if ((!baseRegister.isList()
         && !m_typeResolver->registerContains(baseRegister, m_typeResolver->stringType()))
            || !m_typeResolver->isNumeric(m_state.accumulatorIn())) {
        const auto jsValue = m_typeResolver->globalType(m_typeResolver->jsValueType());
        addReadAccumulator(jsValue);
        addReadRegister(base, jsValue);
        setAccumulator(jsValue);
        return;
    }

    const auto contained = m_typeResolver->containedType(m_state.accumulatorIn());
    if (m_typeResolver->isSignedInteger(contained))
        addReadAccumulator(m_typeResolver->globalType(m_typeResolver->int32Type()));
    else if (m_typeResolver->isUnsignedInteger(contained))
        addReadAccumulator(m_typeResolver->globalType(m_typeResolver->uint32Type()));
    else
        addReadAccumulator(m_typeResolver->globalType(m_typeResolver->realType()));

    addReadRegister(base, baseRegister);
    // We can end up with undefined.
    setAccumulator(m_typeResolver->merge(
            m_typeResolver->valueType(baseRegister),
            m_typeResolver->globalType(m_typeResolver->voidType())));
}

void QQmlJSTypePropagator::generate_StoreElement(int base, int index)
{
    const QQmlJSRegisterContent baseRegister = m_state.registers[base].content;
    const QQmlJSRegisterContent indexRegister = checkedInputRegister(index);

    if (!baseRegister.isList()
            || !m_typeResolver->isNumeric(indexRegister)) {
        const auto jsValue = m_typeResolver->globalType(m_typeResolver->jsValueType());
        addReadAccumulator(jsValue);
        addReadRegister(base, jsValue);
        addReadRegister(index, jsValue);

        // Writing to a JS array can have side effects all over the place since it's
        // passed by reference.
        m_state.setHasSideEffects(true);
        return;
    }

    const auto contained = m_typeResolver->containedType(indexRegister);
    if (m_typeResolver->isSignedInteger(contained))
        addReadRegister(index, m_typeResolver->globalType(m_typeResolver->int32Type()));
    else if (m_typeResolver->isUnsignedInteger(contained))
        addReadRegister(index, m_typeResolver->globalType(m_typeResolver->uint32Type()));
    else
        addReadRegister(index, m_typeResolver->globalType(m_typeResolver->realType()));

    addReadRegister(base, baseRegister);
    addReadAccumulator(m_typeResolver->valueType(baseRegister));

    // If we're writing a QQmlListProperty backed by a container somewhere else,
    // that has side effects.
    // If we're writing to a list retrieved from a property, that _should_ have side effects,
    // but currently the QML engine doesn't implement them.
    // TODO: Figure out the above and accurately set the flag.
    m_state.setHasSideEffects(true);
}

void QQmlJSTypePropagator::propagatePropertyLookup(const QString &propertyName)
{
    setAccumulator(
            m_typeResolver->memberType(
                m_state.accumulatorIn(),
                m_state.accumulatorIn().isImportNamespace()
                    ? m_jsUnitGenerator->stringForIndex(m_state.accumulatorIn().importNamespace())
                      + u'.' + propertyName
                    : propertyName));

    if (!m_state.accumulatorOut().isValid()) {
        if (m_typeResolver->isPrefix(propertyName)) {
            Q_ASSERT(m_state.accumulatorIn().isValid());
            addReadAccumulator(m_state.accumulatorIn());
            setAccumulator(QQmlJSRegisterContent::create(
                        m_state.accumulatorIn().storedType(),
                        m_jsUnitGenerator->getStringId(propertyName),
                        QQmlJSRegisterContent::ObjectModulePrefix,
                        m_typeResolver->containedType(m_state.accumulatorIn())));
            return;
        }
        if (m_state.accumulatorIn().isImportNamespace())
            m_logger->log(u"Type not found in namespace"_s, qmlUnresolvedType,
                          getCurrentSourceLocation());
    } else if (m_state.accumulatorOut().variant() == QQmlJSRegisterContent::Singleton
               && m_state.accumulatorIn().variant() == QQmlJSRegisterContent::ObjectModulePrefix) {
        m_logger->log(
                u"Cannot access singleton as a property of an object. Did you want to access an attached object?"_s,
                qmlAccessSingleton, getCurrentSourceLocation());
        setAccumulator(QQmlJSRegisterContent());
    } else if (m_state.accumulatorOut().isEnumeration()) {
        switch (m_state.accumulatorIn().variant()) {
        case QQmlJSRegisterContent::ExtensionObjectEnum:
        case QQmlJSRegisterContent::MetaType:
        case QQmlJSRegisterContent::ObjectAttached:
        case QQmlJSRegisterContent::ObjectEnum:
        case QQmlJSRegisterContent::ObjectModulePrefix:
        case QQmlJSRegisterContent::ScopeAttached:
        case QQmlJSRegisterContent::ScopeModulePrefix:
        case QQmlJSRegisterContent::Singleton:
            break; // OK, can look up enums on that thing
        default:
            setAccumulator(QQmlJSRegisterContent());
        }
    }

    if (checkForEnumProblems(m_state.accumulatorIn(), propertyName))
        return;

    if (!m_state.accumulatorOut().isValid()) {
        setError(u"Cannot load property %1 from %2."_s
                         .arg(propertyName, m_state.accumulatorIn().descriptiveName()));

        const QString typeName = m_typeResolver->containedTypeName(m_state.accumulatorIn(), true);

        if (typeName == u"QVariant")
            return;
        if (m_state.accumulatorIn().isList() && propertyName == u"length")
            return;

        auto baseType = m_typeResolver->containedType(m_state.accumulatorIn());
        // Warn separately when a property is only not found because of a missing type

        if (propertyResolution(baseType, propertyName) != PropertyMissing)
            return;

        std::optional<QQmlJSFixSuggestion> fixSuggestion;

        if (auto suggestion = QQmlJSUtils::didYouMean(propertyName, baseType->properties().keys(),
                                                      getCurrentSourceLocation());
            suggestion.has_value()) {
            fixSuggestion = suggestion;
        }

        if (!fixSuggestion.has_value()
            && m_state.accumulatorIn().variant() == QQmlJSRegisterContent::MetaType) {
            QStringList enumKeys;
            for (const QQmlJSMetaEnum &metaEnum :
                 m_state.accumulatorIn().scopeType()->enumerations())
                enumKeys << metaEnum.keys();

            if (auto suggestion =
                        QQmlJSUtils::didYouMean(propertyName, enumKeys, getCurrentSourceLocation());
                suggestion.has_value()) {
                fixSuggestion = suggestion;
            }
        }

        m_logger->log(u"Member \"%1\" not found on type \"%2\""_s.arg(propertyName).arg(typeName),
                      qmlMissingProperty, getCurrentSourceLocation(), true, true, fixSuggestion);
        return;
    }

    if (m_state.accumulatorOut().isMethod() && m_state.accumulatorOut().method().size() != 1) {
        setError(u"Cannot determine overloaded method on loadProperty"_s);
        return;
    }

    if (m_state.accumulatorOut().isProperty()) {
        if (m_typeResolver->registerContains(
                    m_state.accumulatorOut(), m_typeResolver->voidType())) {
            setError(u"Type %1 does not have a property %2 for reading"_s
                             .arg(m_state.accumulatorIn().descriptiveName(), propertyName));
            return;
        }

        if (!m_state.accumulatorOut().property().type()) {
            m_logger->log(
                    QString::fromLatin1("Type of property \"%2\" not found").arg(propertyName),
                    qmlMissingType, getCurrentSourceLocation());
        }
    }

    if (m_passManager != nullptr) {
        const bool isAttached =
                m_state.accumulatorIn().variant() == QQmlJSRegisterContent::ObjectAttached;

        m_passManager->d_func()->analyzeRead(
                QQmlJSScope::createQQmlSAElement(
                        m_typeResolver->containedType(m_state.accumulatorIn())),
                propertyName,
                QQmlJSScope::createQQmlSAElement(isAttached ? m_attachedContext
                                                            : m_function->qmlScope),
                QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                        getCurrentBindingSourceLocation()));
    }

    if (m_state.accumulatorOut().variant() == QQmlJSRegisterContent::ObjectAttached)
        m_attachedContext = m_typeResolver->containedType(m_state.accumulatorIn());

    switch (m_state.accumulatorOut().variant()) {
    case QQmlJSRegisterContent::ObjectEnum:
    case QQmlJSRegisterContent::ExtensionObjectEnum:
    case QQmlJSRegisterContent::Singleton:
        // For reading enums or singletons, we don't need to access anything, unless it's an
        // import namespace. Then we need the name.
        if (m_state.accumulatorIn().isImportNamespace())
            addReadAccumulator(m_state.accumulatorIn());
        break;
    default:
        addReadAccumulator(m_state.accumulatorIn());
        break;
    }
}

void QQmlJSTypePropagator::generate_LoadProperty(int nameIndex)
{
    propagatePropertyLookup(m_jsUnitGenerator->stringForIndex(nameIndex));
}

void QQmlJSTypePropagator::generate_LoadOptionalProperty(int name, int offset)
{
    Q_UNUSED(name);
    Q_UNUSED(offset);
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_GetLookup(int index)
{
    propagatePropertyLookup(m_jsUnitGenerator->lookupName(index));
}

void QQmlJSTypePropagator::generate_GetOptionalLookup(int index, int offset)
{
    Q_UNUSED(index);
    Q_UNUSED(offset);
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_StoreProperty(int nameIndex, int base)
{
    auto callBase = m_state.registers[base].content;
    const QString propertyName = m_jsUnitGenerator->stringForIndex(nameIndex);

    QQmlJSRegisterContent property = m_typeResolver->memberType(callBase, propertyName);
    if (!property.isProperty()) {
        setError(u"Type %1 does not have a property %2 for writing"_s
                         .arg(callBase.descriptiveName(), propertyName));
        return;
    }

    if (property.storedType().isNull()) {
        setError(u"Cannot determine type for property %1 of type %2"_s.arg(
                propertyName, callBase.descriptiveName()));
        return;
    }

    if (!property.isWritable() && !property.storedType()->isListProperty()) {
        setError(u"Can't assign to read-only property %1"_s.arg(propertyName));

        m_logger->log(u"Cannot assign to read-only property %1"_s.arg(propertyName),
                      qmlReadOnlyProperty, getCurrentSourceLocation());

        return;
    }

    if (!canConvertFromTo(m_state.accumulatorIn(), property)) {
        setError(u"cannot convert from %1 to %2"_s
                         .arg(m_state.accumulatorIn().descriptiveName(), property.descriptiveName()));
        return;
    }

    if (m_passManager != nullptr) {
        const bool isAttached = callBase.variant() == QQmlJSRegisterContent::ObjectAttached;

        m_passManager->d_func()->analyzeWrite(
                QQmlJSScope::createQQmlSAElement(m_typeResolver->containedType(callBase)),
                propertyName,
                QQmlJSScope::createQQmlSAElement(
                        m_typeResolver->containedType(m_state.accumulatorIn())),
                QQmlJSScope::createQQmlSAElement(isAttached ? m_attachedContext
                                                            : m_function->qmlScope),
                QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                        getCurrentBindingSourceLocation()));
    }

    m_state.setHasSideEffects(true);
    addReadAccumulator(property);
    addReadRegister(base, callBase);
}

void QQmlJSTypePropagator::generate_SetLookup(int index, int base)
{
    generate_StoreProperty(m_jsUnitGenerator->lookupNameIndex(index), base);
}

void QQmlJSTypePropagator::generate_LoadSuperProperty(int property)
{
    Q_UNUSED(property)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_StoreSuperProperty(int property)
{
    Q_UNUSED(property)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_Yield()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_YieldStar()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_Resume(int)
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_CallValue(int name, int argc, int argv)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(name)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_CallWithReceiver(int name, int thisObject, int argc, int argv)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(name)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

static bool isLoggingMethod(const QString &consoleMethod)
{
    return consoleMethod == u"log" || consoleMethod == u"debug" || consoleMethod == u"info"
            || consoleMethod == u"warn" || consoleMethod == u"error";
}

void QQmlJSTypePropagator::generate_CallProperty(int nameIndex, int base, int argc, int argv)
{
    Q_ASSERT(m_state.registers.contains(base));
    const auto callBase = m_state.registers[base].content;
    const QString propertyName = m_jsUnitGenerator->stringForIndex(nameIndex);

    if (m_typeResolver->registerContains(
                callBase, m_typeResolver->jsGlobalObject()->property(u"Math"_s).type())) {

        // If we call a method on the Math object we don't need the actual Math object. We do need
        // to transfer the type information to the code generator so that it knows that this is the
        // Math object. Read the base register as void. void isn't stored, and the place where it's
        // created will be optimized out if there are no other readers. The code generator can
        // retrieve the original type and determine that it was the Math object.
        addReadRegister(base, m_typeResolver->globalType(m_typeResolver->voidType()));

        QQmlJSRegisterContent realType = m_typeResolver->globalType(m_typeResolver->realType());
        for (int i = 0; i < argc; ++i)
            addReadRegister(argv + i, realType);
        setAccumulator(realType);
        return;
    }

    if (m_typeResolver->registerContains(
                callBase, m_typeResolver->jsGlobalObject()->property(u"console"_s).type())
            && isLoggingMethod(propertyName)) {

        const QQmlJSRegisterContent voidType
                = m_typeResolver->globalType(m_typeResolver->voidType());

        // If we call a method on the console object we don't need the console object.
        addReadRegister(base, voidType);

        const QQmlJSRegisterContent stringType
                = m_typeResolver->globalType(m_typeResolver->stringType());

        if (argc > 0) {
            const QQmlJSScope::ConstPtr firstArg
                    = m_typeResolver->containedType(m_state.registers[argv].content);
            if (firstArg->isReferenceType()) {
                // We cannot know whether this will be a logging category at run time.
                // Therefore we always pass any object types as special last argument.
                addReadRegister(argv, m_typeResolver->globalType(
                                    m_typeResolver->genericType(firstArg)));
            } else {
                addReadRegister(argv, stringType);
            }
        }

        for (int i = 1; i < argc; ++i)
            addReadRegister(argv + i, stringType);

        m_state.setHasSideEffects(true);
        setAccumulator(voidType);
        return;
    }

    if (m_typeResolver->registerContains(callBase, m_typeResolver->jsValueType())
            || m_typeResolver->registerContains(callBase, m_typeResolver->varType())) {
        const auto jsValueType = m_typeResolver->globalType(m_typeResolver->jsValueType());
        addReadRegister(base, jsValueType);
        for (int i = 0; i < argc; ++i)
            addReadRegister(argv + i, jsValueType);
        setAccumulator(jsValueType);
        m_state.setHasSideEffects(true);
        return;
    }

    const auto baseType = m_typeResolver->containedType(callBase);
    const auto member = m_typeResolver->memberType(callBase, propertyName);
    if (!member.isMethod()) {
        setError(u"Type %1 does not have a property %2 for calling"_s
                         .arg(callBase.descriptiveName(), propertyName));

        if (callBase.isType() && isCallingProperty(callBase.type(), propertyName))
            return;

        if (checkForEnumProblems(callBase, propertyName))
            return;

        std::optional<QQmlJSFixSuggestion> fixSuggestion;

        if (auto suggestion = QQmlJSUtils::didYouMean(propertyName, baseType->methods().keys(),
                                                      getCurrentSourceLocation());
            suggestion.has_value()) {
            fixSuggestion = suggestion;
        }

        m_logger->log(u"Member \"%1\" not found on type \"%2\""_s.arg(
                              propertyName, m_typeResolver->containedTypeName(callBase, true)),
                      qmlMissingProperty, getCurrentSourceLocation(), true, true, fixSuggestion);
        return;
    }

    checkDeprecated(baseType, propertyName, true);

    if (m_passManager != nullptr) {
        // TODO: Should there be an analyzeCall() in the future? (w. corresponding onCall in Pass)
        m_passManager->d_func()->analyzeRead(
                QQmlJSScope::createQQmlSAElement(baseType), propertyName,
                QQmlJSScope::createQQmlSAElement(m_function->qmlScope),
                QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                        getCurrentBindingSourceLocation()));
    }

    addReadRegister(base, callBase);

    if (m_typeResolver->registerContains(callBase, m_typeResolver->stringType())) {
        if (propertyName == u"arg"_s && argc == 1) {
            propagateStringArgCall(argv);
            return;
        }
    }

    if (baseType->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence
            && m_typeResolver->equals(
                member.scopeType(),
                m_typeResolver->arrayType()->baseType())
            && propagateArrayMethod(propertyName, argc, argv, callBase)) {
        return;
    }

    propagateCall(member.method(), argc, argv, member.scopeType());
}

QQmlJSMetaMethod QQmlJSTypePropagator::bestMatchForCall(const QList<QQmlJSMetaMethod> &methods,
                                                        int argc, int argv, QStringList *errors)
{
    QQmlJSMetaMethod javascriptFunction;
    QQmlJSMetaMethod candidate;
    bool hasMultipleCandidates = false;

    for (const auto &method : methods) {

        // If we encounter a JavaScript function, use this as a fallback if no other method matches
        if (method.isJavaScriptFunction() && !javascriptFunction.isValid())
            javascriptFunction = method;

        if (method.returnType().isNull() && !method.returnTypeName().isEmpty()) {
            errors->append(u"return type %1 cannot be resolved"_s
                                   .arg(method.returnTypeName()));
            continue;
        }

        const auto arguments = method.parameters();
        if (argc != arguments.size()) {
            errors->append(
                    u"Function expects %1 arguments, but %2 were provided"_s.arg(arguments.size())
                            .arg(argc));
            continue;
        }

        bool fuzzyMatch = true;
        bool exactMatch = true;
        for (int i = 0; i < argc; ++i) {
            const auto argumentType = arguments[i].type();
            if (argumentType.isNull()) {
                errors->append(
                        u"type %1 for argument %2 cannot be resolved"_s.arg(arguments[i].typeName())
                                .arg(i));
                exactMatch = false;
                fuzzyMatch = false;
                break;
            }

            const auto content = m_state.registers[argv + i].content;
            if (m_typeResolver->registerContains(content, argumentType))
                continue;

            exactMatch = false;
            if (canConvertFromTo(content, m_typeResolver->globalType(argumentType)))
                continue;

            // We can try to call a method that expects a derived type.
            if (argumentType->isReferenceType()
                    && m_typeResolver->inherits(
                        argumentType->baseType(), m_typeResolver->containedType(content))) {
                continue;
            }

            errors->append(
                    u"argument %1 contains %2 but is expected to contain the type %3"_s.arg(i).arg(
                            content.descriptiveName(), arguments[i].typeName()));
            fuzzyMatch = false;
            break;
        }

        if (exactMatch) {
            return method;
        } else if (fuzzyMatch) {
            if (!candidate.isValid())
                candidate = method;
            else
                hasMultipleCandidates = true;
        }
    }

    if (hasMultipleCandidates)
        return QQmlJSMetaMethod();

    return candidate.isValid() ? candidate : javascriptFunction;
}

void QQmlJSTypePropagator::setAccumulator(const QQmlJSRegisterContent &content)
{
    setRegister(Accumulator, content);
}

void QQmlJSTypePropagator::setRegister(int index, const QQmlJSRegisterContent &content)
{
    // If we've come to the same conclusion before, let's not track the type again.
    auto it = m_prevStateAnnotations.find(currentInstructionOffset());
    if (it != m_prevStateAnnotations.end()) {
        const QQmlJSRegisterContent &lastTry = it->second.changedRegister;
        if (m_typeResolver->registerContains(lastTry, m_typeResolver->containedType(content))) {
            m_state.setRegister(index, lastTry);
            return;
        }
    }

    m_state.setRegister(index, m_typeResolver->tracked(content));
}

void QQmlJSTypePropagator::mergeRegister(
            int index, const QQmlJSRegisterContent &a, const QQmlJSRegisterContent &b)
{
    auto merged = m_typeResolver->merge(a, b);

    Q_ASSERT(merged.isValid());
    Q_ASSERT(merged.isConversion());

    auto tryPrevStateConversion = [this](int index, const QQmlJSRegisterContent &merged) -> bool {
        auto it = m_prevStateAnnotations.find(currentInstructionOffset());
        if (it == m_prevStateAnnotations.end())
            return false;

        auto conversion = it->second.typeConversions.find(index);
        if (conversion == it->second.typeConversions.end())
            return false;

        const VirtualRegister &lastTry = conversion.value();

        Q_ASSERT(lastTry.content.isValid());
        Q_ASSERT(lastTry.content.isConversion());

        if (!m_typeResolver->equals(lastTry.content.conversionResult(), merged.conversionResult())
                || lastTry.content.conversionOrigins() != merged.conversionOrigins()) {
            return false;
        }

        // We don't need to track it again if we've come to the same conclusion before.
        m_state.annotations[currentInstructionOffset()].typeConversions[index] = lastTry;
        m_state.registers[index] = lastTry;
        return true;
    };

    if (!tryPrevStateConversion(index, merged)) {
        merged = m_typeResolver->tracked(merged);
        Q_ASSERT(merged.isValid());
        m_state.annotations[currentInstructionOffset()].typeConversions[index].content = merged;
        m_state.registers[index].content = merged;
    }
}

void QQmlJSTypePropagator::addReadRegister(int index, const QQmlJSRegisterContent &convertTo)
{
    m_state.addReadRegister(index, m_typeResolver->convert
                            (m_state.registers[index].content, convertTo));
}

void QQmlJSTypePropagator::propagateCall(
        const QList<QQmlJSMetaMethod> &methods, int argc, int argv,
        const QQmlJSScope::ConstPtr &scope)
{
    QStringList errors;
    const QQmlJSMetaMethod match = bestMatchForCall(methods, argc, argv, &errors);

    if (!match.isValid()) {
        if (methods.size() == 1) {
            // Cannot have multiple fuzzy matches if there is only one method
            Q_ASSERT(errors.size() == 1);
            setError(errors.first());
        } else if (errors.size() < methods.size()) {
            setError(u"Multiple matching overrides found. Cannot determine the right one."_s);
        } else {
            setError(u"No matching override found. Candidates:\n"_s + errors.join(u'\n'));
        }
        return;
    }

    const auto returnType = match.isJavaScriptFunction()
            ? m_typeResolver->jsValueType()
            : QQmlJSScope::ConstPtr(match.returnType());
    setAccumulator(m_typeResolver->returnType(
            returnType ? QQmlJSScope::ConstPtr(returnType) : m_typeResolver->voidType(),
            match.isJavaScriptFunction()
                    ? QQmlJSRegisterContent::JavaScriptReturnValue
                    : QQmlJSRegisterContent::MethodReturnValue,
            scope));
    if (!m_state.accumulatorOut().isValid())
        setError(u"Cannot store return type of method %1()."_s.arg(match.methodName()));

    m_state.setHasSideEffects(true);
    const auto types = match.parameters();
    for (int i = 0; i < argc; ++i) {
        if (i < types.size()) {
            const QQmlJSScope::ConstPtr type = match.isJavaScriptFunction()
                    ? m_typeResolver->jsValueType()
                    : QQmlJSScope::ConstPtr(types.at(i).type());
            if (!type.isNull()) {
                addReadRegister(argv + i, m_typeResolver->globalType(type));
                continue;
            }
        }
        addReadRegister(argv + i, m_typeResolver->globalType(m_typeResolver->jsValueType()));
    }
}

bool QQmlJSTypePropagator::propagateTranslationMethod(
        const QList<QQmlJSMetaMethod> &methods, int argc, int argv)
{
    if (methods.size() != 1)
        return false;

    const QQmlJSMetaMethod method = methods.front();
    const QQmlJSRegisterContent intType
            = m_typeResolver->globalType(m_typeResolver->int32Type());
    const QQmlJSRegisterContent stringType
            = m_typeResolver->globalType(m_typeResolver->stringType());
    const QQmlJSRegisterContent returnType
            = m_typeResolver->returnType(
                    m_typeResolver->stringType(), QQmlJSRegisterContent::MethodReturnValue,
                    m_typeResolver->jsGlobalObject());

    if (method.methodName() == u"qsTranslate"_s) {
        switch (argc) {
        case 4:
            addReadRegister(argv + 3, intType);    // n
            Q_FALLTHROUGH();
        case 3:
            addReadRegister(argv + 2, stringType); // disambiguation
            Q_FALLTHROUGH();
        case 2:
            addReadRegister(argv + 1, stringType); // sourceText
            addReadRegister(argv, stringType);     // context
            setAccumulator(returnType);
            return true;
        default:
            return false;
        }
    }

    if (method.methodName() == u"QT_TRANSLATE_NOOP"_s) {
        switch (argc) {
        case 3:
            addReadRegister(argv + 2, stringType); // disambiguation
            Q_FALLTHROUGH();
        case 2:
            addReadRegister(argv + 1, stringType); // sourceText
            addReadRegister(argv, stringType);     // context
            setAccumulator(returnType);
            return true;
        default:
            return false;
        }
    }

    if (method.methodName() == u"qsTr"_s) {
        switch (argc) {
        case 3:
            addReadRegister(argv + 2, intType);    // n
            Q_FALLTHROUGH();
        case 2:
            addReadRegister(argv + 1, stringType); // disambiguation
            Q_FALLTHROUGH();
        case 1:
            addReadRegister(argv, stringType);     // sourceText
            setAccumulator(returnType);
            return true;
        default:
            return false;
        }
    }

    if (method.methodName() == u"QT_TR_NOOP"_s) {
        switch (argc) {
        case 2:
            addReadRegister(argv + 1, stringType); // disambiguation
            Q_FALLTHROUGH();
        case 1:
            addReadRegister(argv, stringType);     // sourceText
            setAccumulator(returnType);
            return true;
        default:
            return false;
        }
    }

    if (method.methodName() == u"qsTrId"_s) {
        switch (argc) {
        case 2:
            addReadRegister(argv + 1, intType);    // n
            Q_FALLTHROUGH();
        case 1:
            addReadRegister(argv, stringType);     // id
            setAccumulator(returnType);
            return true;
        default:
            return false;
        }
    }

    if (method.methodName() == u"QT_TRID_NOOP"_s) {
        switch (argc) {
        case 1:
            addReadRegister(argv, stringType);     // id
            setAccumulator(returnType);
            return true;
        default:
            return false;
        }
    }

    return false;
}

void QQmlJSTypePropagator::propagateStringArgCall(int argv)
{
    setAccumulator(m_typeResolver->returnType(
                       m_typeResolver->stringType(), QQmlJSRegisterContent::MethodReturnValue,
                       m_typeResolver->stringType()));
    Q_ASSERT(m_state.accumulatorOut().isValid());

    const QQmlJSScope::ConstPtr input = m_typeResolver->containedType(
                m_state.registers[argv].content);

    if (m_typeResolver->equals(input, m_typeResolver->uint32Type())) {
        addReadRegister(argv, m_typeResolver->globalType(m_typeResolver->realType()));
        return;
    }

    if (m_typeResolver->isIntegral(input)) {
        addReadRegister(argv, m_typeResolver->globalType(m_typeResolver->int32Type()));
        return;
    }

    if (m_typeResolver->isNumeric(input)) {
        addReadRegister(argv, m_typeResolver->globalType(m_typeResolver->realType()));
        return;
    }

    if (m_typeResolver->equals(input, m_typeResolver->boolType())) {
        addReadRegister(argv, m_typeResolver->globalType(m_typeResolver->boolType()));
        return;
    }

    addReadRegister(argv, m_typeResolver->globalType(m_typeResolver->stringType()));
}

bool QQmlJSTypePropagator::propagateArrayMethod(
        const QString &name, int argc, int argv, const QQmlJSRegisterContent &baseType)
{
    // TODO:
    // * For concat() we need to decide what kind of array to return and what kinds of arguments to
    //   accept.
    // * For entries(), keys(), and values() we need iterators.
    // * For find(), findIndex(), sort(), every(), some(), forEach(), map(), filter(), reduce(),
    //   and reduceRight() we need typed function pointers.

    const auto intType = m_typeResolver->globalType(m_typeResolver->int32Type());
    const auto boolType = m_typeResolver->globalType(m_typeResolver->boolType());
    const auto stringType = m_typeResolver->globalType(m_typeResolver->stringType());
    const auto valueType = m_typeResolver->globalType(
            m_typeResolver->containedType(baseType)->valueType());

    // TODO: We should remember whether a register content can be written back when
    //       converting and merging. Also, we need a way to detect the "only in same statement"
    //       write back case. To do this, we should store the statementNumber(s) in
    //       Property and Conversion RegisterContents.
    const bool canHaveSideEffects = (baseType.isProperty() && baseType.isWritable())
            || baseType.isConversion();

    if (name == u"copyWithin" && argc > 0 && argc < 4) {
        for (int i = 0; i < argc; ++i) {
            if (!canConvertFromTo(m_state.registers[argv + i].content, intType))
                return false;
        }

        for (int i = 0; i < argc; ++i)
            addReadRegister(argv + i, intType);

        setAccumulator(baseType);
        m_state.setHasSideEffects(canHaveSideEffects);
        return true;
    }

    if (name == u"fill" && argc > 0 && argc < 4) {
        if (!canConvertFromTo(m_state.registers[argv].content, valueType))
            return false;

        for (int i = 1; i < argc; ++i) {
            if (!canConvertFromTo(m_state.registers[argv + i].content, intType))
                return false;
        }

        addReadRegister(argv, valueType);

        for (int i = 1; i < argc; ++i)
            addReadRegister(argv + i, intType);

        setAccumulator(baseType);
        m_state.setHasSideEffects(canHaveSideEffects);
        return true;
    }

    if (name == u"includes" && argc > 0 && argc < 3) {
        if (!canConvertFromTo(m_state.registers[argv].content, valueType))
            return false;

        if (argc == 2) {
            if (!canConvertFromTo(m_state.registers[argv + 1].content, intType))
                return false;
            addReadRegister(argv + 1, intType);
        }

        addReadRegister(argv, valueType);
        setAccumulator(boolType);
        return true;
    }

    if (name == u"toString" || (name == u"join" && argc < 2)) {
        if (argc == 1) {
            if (!canConvertFromTo(m_state.registers[argv].content, stringType))
                return false;
            addReadRegister(argv, stringType);
        }

        setAccumulator(stringType);
        return true;
    }

    if ((name == u"pop" || name == u"shift") && argc == 0) {
        setAccumulator(valueType);
        m_state.setHasSideEffects(canHaveSideEffects);
        return true;
    }

    if (name == u"push" || name == u"unshift") {
        for (int i = 0; i < argc; ++i) {
            if (!canConvertFromTo(m_state.registers[argv + i].content, valueType))
                return false;
        }

        for (int i = 0; i < argc; ++i)
            addReadRegister(argv + i, valueType);

        setAccumulator(intType);
        m_state.setHasSideEffects(canHaveSideEffects);
        return true;
    }

    if (name == u"reverse" && argc == 0) {
        setAccumulator(baseType);
        m_state.setHasSideEffects(canHaveSideEffects);
        return true;
    }

    if (name == u"slice" && argc < 3) {
        for (int i = 0; i < argc; ++i) {
            if (!canConvertFromTo(m_state.registers[argv + i].content, intType))
                return false;
        }

        for (int i = 0; i < argc; ++i)
            addReadRegister(argv + i, intType);

        setAccumulator(baseType.storedType()->isListProperty()
                               ? m_typeResolver->globalType(m_typeResolver->qObjectListType())
                               : baseType);
        return true;
    }

    if (name == u"splice" && argc > 0) {
        for (int i = 0; i < 2; ++i) {
            if (!canConvertFromTo(m_state.registers[argv + i].content, intType))
                return false;
        }

        for (int i = 2; i < argc; ++i) {
            if (!canConvertFromTo(m_state.registers[argv + i].content, valueType))
                return false;
        }

        for (int i = 0; i < 2; ++i)
            addReadRegister(argv + i, intType);

        for (int i = 2; i < argc; ++i)
            addReadRegister(argv + i, valueType);

        setAccumulator(baseType);
        m_state.setHasSideEffects(canHaveSideEffects);
        return true;
    }

    if ((name == u"indexOf" || name == u"lastIndexOf") && argc > 0 && argc < 3) {
        if (!canConvertFromTo(m_state.registers[argv].content, valueType))
            return false;

        if (argc == 2) {
            if (!canConvertFromTo(m_state.registers[argv + 1].content, intType))
                return false;
            addReadRegister(argv + 1, intType);
        }

        addReadRegister(argv, valueType);
        setAccumulator(intType);
        return true;
    }

    return false;
}

void QQmlJSTypePropagator::generate_CallPropertyLookup(int lookupIndex, int base, int argc,
                                                       int argv)
{
    generate_CallProperty(m_jsUnitGenerator->lookupNameIndex(lookupIndex), base, argc, argv);
}

void QQmlJSTypePropagator::generate_CallName(int name, int argc, int argv)
{
    propagateScopeLookupCall(m_jsUnitGenerator->stringForIndex(name), argc, argv);
}

void QQmlJSTypePropagator::generate_CallPossiblyDirectEval(int argc, int argv)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::propagateScopeLookupCall(const QString &functionName, int argc, int argv)
{
    const QQmlJSRegisterContent resolvedContent
            = m_typeResolver->scopedType(m_function->qmlScope, functionName);
    if (resolvedContent.isMethod()) {
        const auto methods = resolvedContent.method();
        if (resolvedContent.variant() == QQmlJSRegisterContent::JavaScriptGlobal) {
            if (propagateTranslationMethod(methods, argc, argv))
                return;
        }

        if (!methods.isEmpty()) {
            propagateCall(methods, argc, argv, resolvedContent.scopeType());
            return;
        }
    }

    setError(u"method %1 cannot be resolved."_s.arg(functionName));
    setAccumulator(m_typeResolver->globalType(m_typeResolver->jsValueType()));

    setError(u"Cannot find function '%1'"_s.arg(functionName));

    handleUnqualifiedAccess(functionName, true);
}

void QQmlJSTypePropagator::generate_CallGlobalLookup(int index, int argc, int argv)
{
    propagateScopeLookupCall(m_jsUnitGenerator->lookupName(index), argc, argv);
}

void QQmlJSTypePropagator::generate_CallQmlContextPropertyLookup(int index, int argc, int argv)
{
    const QString name = m_jsUnitGenerator->lookupName(index);
    propagateScopeLookupCall(name, argc, argv);
    checkDeprecated(m_function->qmlScope, name, true);
}

void QQmlJSTypePropagator::generate_CallWithSpread(int func, int thisObject, int argc, int argv)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(func)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_TailCall(int func, int thisObject, int argc, int argv)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(func)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_Construct(int func, int argc, int argv)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(func)
    Q_UNUSED(argv)

    Q_UNUSED(argc)

    setAccumulator(m_typeResolver->globalType(m_typeResolver->jsValueType()));
}

void QQmlJSTypePropagator::generate_ConstructWithSpread(int func, int argc, int argv)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(func)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_SetUnwindHandler(int offset)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(offset)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_IGNORE();
}

void QQmlJSTypePropagator::generate_UnwindDispatch()
{
    m_state.setHasSideEffects(true);
    INSTR_PROLOGUE_NOT_IMPLEMENTED_IGNORE();
}

void QQmlJSTypePropagator::generate_UnwindToLabel(int level, int offset)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(level)
    Q_UNUSED(offset)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_DeadTemporalZoneCheck(int name)
{
    const auto fail = [this, name]() {
        setError(u"Cannot statically assert the dead temporal zone check for %1"_s.arg(
                name ? m_jsUnitGenerator->stringForIndex(name) : u"the anonymous accumulator"_s));
    };

    const QQmlJSRegisterContent in = m_state.accumulatorIn();
    if (in.isConversion()) {
        for (const QQmlJSScope::ConstPtr &origin : in.conversionOrigins()) {
            if (!m_typeResolver->equals(origin, m_typeResolver->emptyType()))
                continue;
            fail();
            break;
        }
    } else if (m_typeResolver->registerContains(in, m_typeResolver->emptyType())) {
        fail();
    }
}

void QQmlJSTypePropagator::generate_ThrowException()
{
    addReadAccumulator(m_typeResolver->globalType(m_typeResolver->jsValueType()));
    m_state.setHasSideEffects(true);
    m_state.skipInstructionsUntilNextJumpTarget = true;
}

void QQmlJSTypePropagator::generate_GetException()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_SetException()
{
    m_state.setHasSideEffects(true);
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_CreateCallContext()
{
    m_state.setHasSideEffects(true);
}

void QQmlJSTypePropagator::generate_PushCatchContext(int index, int name)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(index)
    Q_UNUSED(name)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_IGNORE();
}

void QQmlJSTypePropagator::generate_PushWithContext()
{
    m_state.setHasSideEffects(true);
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_PushBlockContext(int index)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(index)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_CloneBlockContext()
{
    m_state.setHasSideEffects(true);
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_PushScriptContext(int index)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(index)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_PopScriptContext()
{
    m_state.setHasSideEffects(true);
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_PopContext()
{
    m_state.setHasSideEffects(true);
}

void QQmlJSTypePropagator::generate_GetIterator(int iterator)
{
    Q_UNUSED(iterator)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_IteratorNext(int value, int done)
{
    Q_UNUSED(value)
    Q_UNUSED(done)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_IteratorNextForYieldStar(int iterator, int object)
{
    Q_UNUSED(iterator)
    Q_UNUSED(object)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_IteratorClose(int done)
{
    Q_UNUSED(done)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_DestructureRestElement()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_DeleteProperty(int base, int index)
{
    Q_UNUSED(base)
    Q_UNUSED(index)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_DeleteName(int name)
{
    Q_UNUSED(name)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_TypeofName(int name)
{
    Q_UNUSED(name);
    setAccumulator(m_typeResolver->globalType(m_typeResolver->stringType()));
}

void QQmlJSTypePropagator::generate_TypeofValue()
{
    setAccumulator(m_typeResolver->globalType(m_typeResolver->stringType()));
}

void QQmlJSTypePropagator::generate_DeclareVar(int varName, int isDeletable)
{
    Q_UNUSED(varName)
    Q_UNUSED(isDeletable)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_DefineArray(int argc, int args)
{
    setAccumulator(m_typeResolver->globalType(m_typeResolver->variantListType()));

    // Track all arguments as the same type.
    const QQmlJSRegisterContent elementType
            = m_typeResolver->tracked(m_typeResolver->globalType(m_typeResolver->varType()));
    for (int i = 0; i < argc; ++i)
        addReadRegister(args + i, elementType);
}

void QQmlJSTypePropagator::generate_DefineObjectLiteral(int internalClassId, int argc, int args)
{
    // TODO: computed property names, getters, and setters are unsupported. How do we catch them?

    Q_UNUSED(internalClassId)
    Q_UNUSED(argc)
    Q_UNUSED(args)
    setAccumulator(m_typeResolver->globalType(m_typeResolver->jsValueType()));
}

void QQmlJSTypePropagator::generate_CreateClass(int classIndex, int heritage, int computedNames)
{
    Q_UNUSED(classIndex)
    Q_UNUSED(heritage)
    Q_UNUSED(computedNames)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_CreateMappedArgumentsObject()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_CreateUnmappedArgumentsObject()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_CreateRestParameter(int argIndex)
{
    Q_UNUSED(argIndex)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_ConvertThisToObject()
{
    setRegister(This, m_typeResolver->globalType(m_typeResolver->qObjectType()));
}

void QQmlJSTypePropagator::generate_LoadSuperConstructor()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_ToObject()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_Jump(int offset)
{
    saveRegisterStateForJump(offset);
    m_state.skipInstructionsUntilNextJumpTarget = true;
    m_state.setHasSideEffects(true);
}

void QQmlJSTypePropagator::generate_JumpTrue(int offset)
{
    if (!canConvertFromTo(m_state.accumulatorIn(),
                          m_typeResolver->globalType(m_typeResolver->boolType()))) {
        setError(u"cannot convert from %1 to boolean"_s
                         .arg(m_state.accumulatorIn().descriptiveName()));
        return;
    }
    saveRegisterStateForJump(offset);
    m_state.setHasSideEffects(true);
    addReadAccumulator(m_typeResolver->globalType(m_typeResolver->boolType()));
}

void QQmlJSTypePropagator::generate_JumpFalse(int offset)
{
    if (!canConvertFromTo(m_state.accumulatorIn(),
                          m_typeResolver->globalType(m_typeResolver->boolType()))) {
        setError(u"cannot convert from %1 to boolean"_s
                         .arg(m_state.accumulatorIn().descriptiveName()));
        return;
    }
    saveRegisterStateForJump(offset);
    m_state.setHasSideEffects(true);
    addReadAccumulator(m_typeResolver->globalType(m_typeResolver->boolType()));
}

void QQmlJSTypePropagator::generate_JumpNoException(int offset)
{
    saveRegisterStateForJump(offset);
    m_state.setHasSideEffects(true);
}

void QQmlJSTypePropagator::generate_JumpNotUndefined(int offset)
{
    Q_UNUSED(offset)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_CheckException()
{
    m_state.setHasSideEffects(true);
}

void QQmlJSTypePropagator::recordEqualsNullType()
{
    // TODO: We can specialize this further, for QVariant, QJSValue, int, bool, whatever.
    if (m_typeResolver->registerContains(m_state.accumulatorIn(), m_typeResolver->nullType())
            || m_typeResolver->containedType(m_state.accumulatorIn())->isReferenceType()) {
        addReadAccumulator(m_state.accumulatorIn());
    } else {
        addReadAccumulator(m_typeResolver->globalType(m_typeResolver->jsPrimitiveType()));
    }
}
void QQmlJSTypePropagator::recordEqualsIntType()
{
    // We have specializations for numeric types and bool.
    const QQmlJSScope::ConstPtr in = m_typeResolver->containedType(m_state.accumulatorIn());
    if (m_typeResolver->registerContains(m_state.accumulatorIn(), m_typeResolver->boolType())
            || m_typeResolver->isNumeric(m_state.accumulatorIn())) {
        addReadAccumulator(m_state.accumulatorIn());
    } else {
        addReadAccumulator(m_typeResolver->globalType(m_typeResolver->jsPrimitiveType()));
    }
}
void QQmlJSTypePropagator::recordEqualsType(int lhs)
{
    const auto isNumericOrEnum = [this](const QQmlJSRegisterContent &content) {
        return content.isEnumeration() || m_typeResolver->isNumeric(content);
    };

    const auto accumulatorIn = m_state.accumulatorIn();
    const auto lhsRegister = m_state.registers[lhs].content;

    // If the types are primitive, we compare directly ...
    if (m_typeResolver->isPrimitive(accumulatorIn) || accumulatorIn.isEnumeration()) {
        if (m_typeResolver->registerContains(
                    accumulatorIn, m_typeResolver->containedType(lhsRegister))
                || (isNumericOrEnum(accumulatorIn) && isNumericOrEnum(lhsRegister))
                || m_typeResolver->isPrimitive(lhsRegister)) {
            addReadRegister(lhs, lhsRegister);
            addReadAccumulator(accumulatorIn);
            return;
        }
    }

    // We don't modify types if the types are comparable with QObject, QUrl or var types
    if (canStrictlyCompareWithVar(m_typeResolver, lhsRegister, accumulatorIn)
        || canCompareWithQObject(m_typeResolver, lhsRegister, accumulatorIn)
        || canCompareWithQUrl(m_typeResolver, lhsRegister, accumulatorIn)) {
        addReadRegister(lhs, lhsRegister);
        addReadAccumulator(accumulatorIn);
        return;
    }

    // Otherwise they're both casted to QJSValue.
    // TODO: We can add more specializations here: object/null etc

    const QQmlJSRegisterContent jsval = m_typeResolver->globalType(m_typeResolver->jsValueType());
    addReadRegister(lhs, jsval);
    addReadAccumulator(jsval);
}

void QQmlJSTypePropagator::recordCompareType(int lhs)
{
    // If they're both numeric, we can compare them directly.
    // They may be casted to double, though.
    const QQmlJSRegisterContent read
            = (m_typeResolver->isNumeric(m_state.accumulatorIn())
               && m_typeResolver->isNumeric(m_state.registers[lhs].content))
                    ? m_typeResolver->merge(m_state.accumulatorIn(), m_state.registers[lhs].content)
                    : m_typeResolver->globalType(m_typeResolver->jsPrimitiveType());
    addReadRegister(lhs, read);
    addReadAccumulator(read);
}

void QQmlJSTypePropagator::generate_CmpEqNull()
{
    recordEqualsNullType();
    setAccumulator(m_typeResolver->globalType(m_typeResolver->boolType()));
}

void QQmlJSTypePropagator::generate_CmpNeNull()
{
    recordEqualsNullType();
    setAccumulator(m_typeResolver->globalType(m_typeResolver->boolType()));
}

void QQmlJSTypePropagator::generate_CmpEqInt(int lhsConst)
{
    recordEqualsIntType();
    Q_UNUSED(lhsConst)
    setAccumulator(QQmlJSRegisterContent(m_typeResolver->typeForBinaryOperation(
            QSOperator::Op::Equal, m_typeResolver->globalType(m_typeResolver->int32Type()),
            m_state.accumulatorIn())));
}

void QQmlJSTypePropagator::generate_CmpNeInt(int lhsConst)
{
    recordEqualsIntType();
    Q_UNUSED(lhsConst)
    setAccumulator(QQmlJSRegisterContent(m_typeResolver->typeForBinaryOperation(
            QSOperator::Op::NotEqual, m_typeResolver->globalType(m_typeResolver->int32Type()),
            m_state.accumulatorIn())));
}

void QQmlJSTypePropagator::generate_CmpEq(int lhs)
{
    recordEqualsType(lhs);
    propagateBinaryOperation(QSOperator::Op::Equal, lhs);
}

void QQmlJSTypePropagator::generate_CmpNe(int lhs)
{
    recordEqualsType(lhs);
    propagateBinaryOperation(QSOperator::Op::NotEqual, lhs);
}

void QQmlJSTypePropagator::generate_CmpGt(int lhs)
{
    recordCompareType(lhs);
    propagateBinaryOperation(QSOperator::Op::Gt, lhs);
}

void QQmlJSTypePropagator::generate_CmpGe(int lhs)
{
    recordCompareType(lhs);
    propagateBinaryOperation(QSOperator::Op::Ge, lhs);
}

void QQmlJSTypePropagator::generate_CmpLt(int lhs)
{
    recordCompareType(lhs);
    propagateBinaryOperation(QSOperator::Op::Lt, lhs);
}

void QQmlJSTypePropagator::generate_CmpLe(int lhs)
{
    recordCompareType(lhs);
    propagateBinaryOperation(QSOperator::Op::Le, lhs);
}

void QQmlJSTypePropagator::generate_CmpStrictEqual(int lhs)
{
    recordEqualsType(lhs);
    propagateBinaryOperation(QSOperator::Op::StrictEqual, lhs);
}

void QQmlJSTypePropagator::generate_CmpStrictNotEqual(int lhs)
{
    recordEqualsType(lhs);
    propagateBinaryOperation(QSOperator::Op::StrictNotEqual, lhs);
}

void QQmlJSTypePropagator::generate_CmpIn(int lhs)
{
    // TODO: Most of the time we don't need the object at all, but only its metatype.
    //       Fix this when we add support for the "in" instruction to the code generator.
    //       Also, specialize on lhs to avoid conversion to QJSPrimitiveValue.

    addReadRegister(lhs, m_typeResolver->globalType(m_typeResolver->jsValueType()));
    addReadAccumulator(m_typeResolver->globalType(m_typeResolver->jsValueType()));

    propagateBinaryOperation(QSOperator::Op::In, lhs);
}

void QQmlJSTypePropagator::generate_CmpInstanceOf(int lhs)
{
    Q_UNUSED(lhs)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_As(int lhs)
{
    const QQmlJSRegisterContent input = checkedInputRegister(lhs);
    QQmlJSScope::ConstPtr contained;

    switch (m_state.accumulatorIn().variant()) {
    case QQmlJSRegisterContent::ScopeAttached:
        contained = m_state.accumulatorIn().scopeType();
        break;
    case QQmlJSRegisterContent::MetaType:
        contained = m_state.accumulatorIn().scopeType();
        if (contained->isComposite()) // Otherwise we don't need it
            addReadAccumulator(m_typeResolver->globalType(m_typeResolver->metaObjectType()));
        break;
    default:
        contained = m_typeResolver->containedType(m_state.accumulatorIn());
        break;
    }

    QQmlJSRegisterContent output;

    if (contained->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
        // A referece type cast can result in either the type or null.
        // Reference tpyes can hold null. We don't need to special case that.
        output = m_typeResolver->globalType(contained);
    } else if (!m_typeResolver->canAddressValueTypes()) {
        setError(u"invalid cast from %1 to %2. You can only cast object types."_s
                 .arg(input.descriptiveName(), m_state.accumulatorIn().descriptiveName()));
        return;
    } else {
        // A value type cast can result in either the type or undefined.
        output = m_typeResolver->merge(
                    m_typeResolver->globalType(contained),
                    m_typeResolver->globalType(m_typeResolver->voidType()));
    }

    addReadRegister(lhs, input);
    setAccumulator(output);
}

void QQmlJSTypePropagator::checkConversion(
        const QQmlJSRegisterContent &from, const QQmlJSRegisterContent &to)
{
    if (!canConvertFromTo(from, to)) {
        setError(u"cannot convert from %1 to %2"_s
                 .arg(from.descriptiveName(), to.descriptiveName()));
    }
}

void QQmlJSTypePropagator::generateUnaryArithmeticOperation(QQmlJSTypeResolver::UnaryOperator op)
{
    const QQmlJSRegisterContent type = m_typeResolver->typeForArithmeticUnaryOperation(
                op, m_state.accumulatorIn());
    checkConversion(m_state.accumulatorIn(), type);
    addReadAccumulator(type);
    setAccumulator(type);
}

void QQmlJSTypePropagator::generate_UNot()
{
    generateUnaryArithmeticOperation(QQmlJSTypeResolver::UnaryOperator::Not);
}

void QQmlJSTypePropagator::generate_UPlus()
{
    generateUnaryArithmeticOperation(QQmlJSTypeResolver::UnaryOperator::Plus);
}

void QQmlJSTypePropagator::generate_UMinus()
{
    generateUnaryArithmeticOperation(QQmlJSTypeResolver::UnaryOperator::Minus);
}

void QQmlJSTypePropagator::generate_UCompl()
{
    generateUnaryArithmeticOperation(QQmlJSTypeResolver::UnaryOperator::Complement);
}

void QQmlJSTypePropagator::generate_Increment()
{
    generateUnaryArithmeticOperation(QQmlJSTypeResolver::UnaryOperator::Increment);
}

void QQmlJSTypePropagator::generate_Decrement()
{
    generateUnaryArithmeticOperation(QQmlJSTypeResolver::UnaryOperator::Decrement);
}

void QQmlJSTypePropagator::generateBinaryArithmeticOperation(QSOperator::Op op, int lhs)
{
    const QQmlJSRegisterContent type = propagateBinaryOperation(op, lhs);

    checkConversion(checkedInputRegister(lhs), type);
    addReadRegister(lhs, type);

    checkConversion(m_state.accumulatorIn(), type);
    addReadAccumulator(type);
}

void QQmlJSTypePropagator::generateBinaryConstArithmeticOperation(QSOperator::Op op)
{
    const QQmlJSRegisterContent type = m_typeResolver->typeForBinaryOperation(
                op, m_state.accumulatorIn(),
                m_typeResolver->builtinType(m_typeResolver->int32Type()));

    checkConversion(m_state.accumulatorIn(), type);
    addReadAccumulator(type);
    setAccumulator(type);
}

void QQmlJSTypePropagator::generate_Add(int lhs)
{
    generateBinaryArithmeticOperation(QSOperator::Op::Add, lhs);
}

void QQmlJSTypePropagator::generate_BitAnd(int lhs)
{
    generateBinaryArithmeticOperation(QSOperator::Op::BitAnd, lhs);
}

void QQmlJSTypePropagator::generate_BitOr(int lhs)
{
    generateBinaryArithmeticOperation(QSOperator::Op::BitOr, lhs);
}

void QQmlJSTypePropagator::generate_BitXor(int lhs)
{
    generateBinaryArithmeticOperation(QSOperator::Op::BitXor, lhs);
}

void QQmlJSTypePropagator::generate_UShr(int lhs)
{
    generateBinaryArithmeticOperation(QSOperator::Op::URShift, lhs);
}

void QQmlJSTypePropagator::generate_Shr(int lhs)
{
    generateBinaryArithmeticOperation(QSOperator::Op::RShift, lhs);
}

void QQmlJSTypePropagator::generate_Shl(int lhs)
{
    generateBinaryArithmeticOperation(QSOperator::Op::LShift, lhs);
}

void QQmlJSTypePropagator::generate_BitAndConst(int rhsConst)
{
    Q_UNUSED(rhsConst)
    generateBinaryConstArithmeticOperation(QSOperator::Op::BitAnd);
}

void QQmlJSTypePropagator::generate_BitOrConst(int rhsConst)
{
    Q_UNUSED(rhsConst)
    generateBinaryConstArithmeticOperation(QSOperator::Op::BitOr);
}

void QQmlJSTypePropagator::generate_BitXorConst(int rhsConst)
{
    Q_UNUSED(rhsConst)
    generateBinaryConstArithmeticOperation(QSOperator::Op::BitXor);
}

void QQmlJSTypePropagator::generate_UShrConst(int rhsConst)
{
    Q_UNUSED(rhsConst)
    generateBinaryConstArithmeticOperation(QSOperator::Op::URShift);
}

void QQmlJSTypePropagator::generate_ShrConst(int rhsConst)
{
    Q_UNUSED(rhsConst)
    generateBinaryConstArithmeticOperation(QSOperator::Op::RShift);
}

void QQmlJSTypePropagator::generate_ShlConst(int rhsConst)
{
    Q_UNUSED(rhsConst)
    generateBinaryConstArithmeticOperation(QSOperator::Op::LShift);
}

void QQmlJSTypePropagator::generate_Exp(int lhs)
{
    generateBinaryArithmeticOperation(QSOperator::Op::Exp, lhs);
}

void QQmlJSTypePropagator::generate_Mul(int lhs)
{
    generateBinaryArithmeticOperation(QSOperator::Op::Mul, lhs);
}

void QQmlJSTypePropagator::generate_Div(int lhs)
{
    generateBinaryArithmeticOperation(QSOperator::Op::Div, lhs);
}

void QQmlJSTypePropagator::generate_Mod(int lhs)
{
    generateBinaryArithmeticOperation(QSOperator::Op::Mod, lhs);
}

void QQmlJSTypePropagator::generate_Sub(int lhs)
{
    generateBinaryArithmeticOperation(QSOperator::Op::Sub, lhs);
}

void QQmlJSTypePropagator::generate_InitializeBlockDeadTemporalZone(int firstReg, int count)
{
    for (int reg = firstReg, end = firstReg + count; reg < end; ++reg)
        setRegister(reg, m_typeResolver->globalType(m_typeResolver->emptyType()));
}

void QQmlJSTypePropagator::generate_ThrowOnNullOrUndefined()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_GetTemplateObject(int index)
{
    Q_UNUSED(index)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

QV4::Moth::ByteCodeHandler::Verdict
QQmlJSTypePropagator::startInstruction(QV4::Moth::Instr::Type type)
{
    if (m_error->isValid())
        return SkipInstruction;

    if (m_state.jumpTargets.contains(currentInstructionOffset())) {
        if (m_state.skipInstructionsUntilNextJumpTarget) {
            // When re-surfacing from dead code, all registers are invalid.
            m_state.registers.clear();
            m_state.skipInstructionsUntilNextJumpTarget = false;
        }
    } else if (m_state.skipInstructionsUntilNextJumpTarget
               && !instructionManipulatesContext(type)) {
        return SkipInstruction;
    }

    const int currentOffset = currentInstructionOffset();

    // If we reach an instruction that is a target of a jump earlier, then we must check that the
    // register state at the origin matches the current state. If not, then we may have to inject
    // conversion code (communicated to code gen via m_state.typeConversions). For
    // example:
    //
    //     function blah(x: number) { return x > 10 ? 10 : x}
    //
    // translates to a situation where in the "true" case, we load an integer into the accumulator
    // and in the else case a number (x). When the control flow is joined, the types don't match and
    // we need to make sure that the int is converted to a double just before the jump.
    for (auto originRegisterStateIt =
                 m_jumpOriginRegisterStateByTargetInstructionOffset.constFind(currentOffset);
         originRegisterStateIt != m_jumpOriginRegisterStateByTargetInstructionOffset.constEnd()
         && originRegisterStateIt.key() == currentOffset;
         ++originRegisterStateIt) {
        auto stateToMerge = *originRegisterStateIt;
        for (auto registerIt = stateToMerge.registers.constBegin(),
                  end = stateToMerge.registers.constEnd();
             registerIt != end; ++registerIt) {
            const int registerIndex = registerIt.key();

            auto newType = registerIt.value().content;
            if (!newType.isValid()) {
                setError(u"When reached from offset %1, %2 is undefined"_s
                                 .arg(stateToMerge.originatingOffset)
                                 .arg(registerName(registerIndex)));
                return SkipInstruction;
            }

            auto currentRegister = m_state.registers.find(registerIndex);
            if (currentRegister != m_state.registers.end())
                mergeRegister(registerIndex, newType, currentRegister.value().content);
            else
                mergeRegister(registerIndex, newType, newType);
        }
    }

    return ProcessInstruction;
}

void QQmlJSTypePropagator::endInstruction(QV4::Moth::Instr::Type instr)
{
    InstructionAnnotation &currentInstruction = m_state.annotations[currentInstructionOffset()];
    currentInstruction.changedRegister = m_state.changedRegister();
    currentInstruction.changedRegisterIndex = m_state.changedRegisterIndex();
    currentInstruction.readRegisters = m_state.takeReadRegisters();
    currentInstruction.hasSideEffects = m_state.hasSideEffects();
    currentInstruction.isRename = m_state.isRename();

    switch (instr) {
    // the following instructions are not expected to produce output in the accumulator
    case QV4::Moth::Instr::Type::Ret:
    case QV4::Moth::Instr::Type::Jump:
    case QV4::Moth::Instr::Type::JumpFalse:
    case QV4::Moth::Instr::Type::JumpTrue:
    case QV4::Moth::Instr::Type::StoreReg:
    case QV4::Moth::Instr::Type::StoreElement:
    case QV4::Moth::Instr::Type::StoreNameSloppy:
    case QV4::Moth::Instr::Type::StoreProperty:
    case QV4::Moth::Instr::Type::SetLookup:
    case QV4::Moth::Instr::Type::MoveConst:
    case QV4::Moth::Instr::Type::MoveReg:
    case QV4::Moth::Instr::Type::CheckException:
    case QV4::Moth::Instr::Type::CreateCallContext:
    case QV4::Moth::Instr::Type::PopContext:
    case QV4::Moth::Instr::Type::JumpNoException:
    case QV4::Moth::Instr::Type::ThrowException:
    case QV4::Moth::Instr::Type::SetUnwindHandler:
    case QV4::Moth::Instr::Type::PushCatchContext:
    case QV4::Moth::Instr::Type::UnwindDispatch:
    case QV4::Moth::Instr::Type::InitializeBlockDeadTemporalZone:
    case QV4::Moth::Instr::Type::ConvertThisToObject:
    case QV4::Moth::Instr::Type::DeadTemporalZoneCheck:
        if (m_state.changedRegisterIndex() == Accumulator && !m_error->isValid()) {
            setError(u"Instruction is not expected to populate the accumulator"_s);
            return;
        }
        break;
    default:
        // If the instruction is expected to produce output, save it in the register set
        // for the next instruction.
        if ((!m_state.changedRegister().isValid() || m_state.changedRegisterIndex() != Accumulator)
                && !m_error->isValid()) {
            setError(u"Instruction is expected to populate the accumulator"_s);
            return;
        }
    }

    if (m_state.changedRegisterIndex() != InvalidRegister) {
        Q_ASSERT(m_error->isValid() || m_state.changedRegister().isValid());
        VirtualRegister &r = m_state.registers[m_state.changedRegisterIndex()];
        r.content = m_state.changedRegister();
        r.canMove = false;
        r.affectedBySideEffects = m_state.isRename()
                && m_state.isRegisterAffectedBySideEffects(m_state.renameSourceRegisterIndex());
        m_state.clearChangedRegister();
    }

    m_state.setHasSideEffects(false);
    m_state.setIsRename(false);
    m_state.setReadRegisters(VirtualRegisters());
}

QQmlJSRegisterContent QQmlJSTypePropagator::propagateBinaryOperation(QSOperator::Op op, int lhs)
{
    auto lhsRegister = checkedInputRegister(lhs);
    if (!lhsRegister.isValid())
        return QQmlJSRegisterContent();

    const QQmlJSRegisterContent type = m_typeResolver->typeForBinaryOperation(
                op, lhsRegister, m_state.accumulatorIn());

    setAccumulator(type);

    // If we're dealing with QJSPrimitiveType, do not force premature conversion of the arguemnts
    // to the target type. Such an operation can lose information.
    if (type.storedType() == m_typeResolver->jsPrimitiveType())
        return m_typeResolver->globalType(m_typeResolver->jsPrimitiveType());

    return type;
}

void QQmlJSTypePropagator::saveRegisterStateForJump(int offset)
{
    auto jumpToOffset = offset + nextInstructionOffset();
    ExpectedRegisterState state;
    state.registers = m_state.registers;
    state.originatingOffset = currentInstructionOffset();
    m_state.jumpTargets.insert(jumpToOffset);
    if (offset < 0) {
        // We're jumping backwards. We won't get to merge the register states in this pass anymore.

        const auto registerStates =
                m_jumpOriginRegisterStateByTargetInstructionOffset.equal_range(jumpToOffset);
        for (auto it = registerStates.first; it != registerStates.second; ++it) {
            if (it->registers.keys() == state.registers.keys()
                    && it->registers.values() == state.registers.values()) {
                return; // We've seen the same register state before. No need for merging.
            }
        }

        // The register state at the target offset needs to be resolved in a further pass.
        m_state.needsMorePasses = true;
    }
    m_jumpOriginRegisterStateByTargetInstructionOffset.insert(jumpToOffset, state);
}

QString QQmlJSTypePropagator::registerName(int registerIndex) const
{
    if (registerIndex == Accumulator)
        return u"accumulator"_s;
    if (registerIndex >= FirstArgument
            && registerIndex < FirstArgument + m_function->argumentTypes.size()) {
        return u"argument %1"_s.arg(registerIndex - FirstArgument);
    }

    return u"temporary register %1"_s.arg(
            registerIndex - FirstArgument - m_function->argumentTypes.size());
}

QQmlJSRegisterContent QQmlJSTypePropagator::checkedInputRegister(int reg)
{
    const auto regIt = m_state.registers.find(reg);
    if (regIt == m_state.registers.end()) {
        if (isArgument(reg))
            return argumentType(reg);

        setError(u"Type error: could not infer the type of an expression"_s);
        return {};
    }
    return regIt.value().content;
}

bool QQmlJSTypePropagator::canConvertFromTo(const QQmlJSRegisterContent &from,
                                            const QQmlJSRegisterContent &to)
{
    return m_typeResolver->canConvertFromTo(from, to);
}

QT_END_NAMESPACE
