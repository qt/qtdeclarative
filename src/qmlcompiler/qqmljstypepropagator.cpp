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
                                           QQmlJSLogger *logger,
                                           QList<QQmlJS::DiagnosticMessage> *errors,
                                           const BasicBlocks &basicBlocks,
                                           const InstructionAnnotations &annotations,
                                           QQmlSA::PassManager *passManager)
    : QQmlJSCompilePass(unitGenerator, typeResolver, logger, errors, basicBlocks, annotations),
      m_passManager(passManager)
{
}

QQmlJSCompilePass::BlocksAndAnnotations QQmlJSTypePropagator::run(const Function *function)
{
    m_function = function;
    m_returnType = m_function->returnType;

    QList<QQmlJS::DiagnosticMessage> oldErrors;
    std::swap(oldErrors, *m_errors);
    auto restoreErrors = qScopeGuard([&]() {
        oldErrors << *std::move(m_errors);
        *m_errors = std::move(oldErrors);
    });

    do {
        // Reset the error if we need to do another pass
        if (m_state.needsMorePasses) {
            m_errors->clear();
            m_logger->rollback();
        }

        m_logger->startTransaction();

        m_prevStateAnnotations = m_state.annotations;
        m_state = PassState();
        m_state.annotations = m_annotations;
        m_state.State::operator=(initialState(m_function));

        reset();
        decode(m_function->code.constData(), static_cast<uint>(m_function->code.size()));

        // If we have found unresolved backwards jumps, we need to start over with a fresh state.
        // Mind that m_jumpOriginRegisterStateByTargetInstructionOffset is retained in that case.
        // This means that we won't start over for the same reason again.
    } while (m_state.needsMorePasses);

    m_logger->commit();
    return { std::move(m_basicBlocks), std::move(m_state.annotations) };
}

#define INSTR_PROLOGUE_NOT_IMPLEMENTED()                                                \
    addError(u"Instruction \"%1\" not implemented"_s.arg(QString::fromUtf8(__func__))); \
    return;

#define INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC()                                  \
    addError(u"Instruction \"%1\" not implemented"_s.arg(QString::fromUtf8(__func__))); \
    setVarAccumulatorAndError(); /* Keep sane state after error */                      \
    return;

#define INSTR_PROLOGUE_NOT_IMPLEMENTED_IGNORE()                                                    \
    m_logger->log(u"Instruction \"%1\" not implemented"_s.arg(QString::fromUtf8(__func__)),        \
                  qmlCompiler, QQmlJS::SourceLocation());                                          \
    return;

void QQmlJSTypePropagator::generate_ret_SAcheck()
{
    const QQmlJS::SourceLocation location = m_function->isProperty
            ? getCurrentBindingSourceLocation()
            : getCurrentNonEmptySourceLocation();
    QQmlSA::PassManagerPrivate::get(m_passManager)
            ->analyzeBinding(
                    QQmlJSScope::createQQmlSAElement(m_function->qmlScope.containedType()),
                    QQmlJSScope::createQQmlSAElement(m_state.accumulatorIn().containedType()),
                    QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(location));
}
void QQmlJSTypePropagator::generate_Ret()
{
    if (m_passManager != nullptr)
        generate_ret_SAcheck();

    if (m_function->isSignalHandler) {
        // Signal handlers cannot return anything.
    } else if (m_state.accumulatorIn().contains(m_typeResolver->voidType())) {
        // You can always return undefined.
    } else if (!m_returnType.isValid() && m_state.accumulatorIn().isValid()) {
        addError(u"function without return type annotation returns %1. This may prevent proper "_s
                 u"compilation to Cpp."_s.arg(m_state.accumulatorIn().descriptiveName()));

        if (m_function->isFullyTyped) {
            // Do not complain if the function didn't have a valid annotation in the first place.
            m_logger->log(u"Function without return type annotation returns %1"_s.arg(
                                  m_state.accumulatorIn().containedTypeName()),
                          qmlIncompatibleType, getCurrentBindingSourceLocation());
        }
        return;
    } else if (!canConvertFromTo(m_state.accumulatorIn(), m_returnType)) {
        addError(u"cannot convert from %1 to %2"_s
                         .arg(m_state.accumulatorIn().descriptiveName(),
                              m_returnType.descriptiveName()));

        m_logger->log(u"Cannot assign binding of type %1 to %2"_s.arg(
                              m_state.accumulatorIn().containedTypeName(),
                              m_returnType.containedTypeName()),
                      qmlIncompatibleType, getCurrentBindingSourceLocation());
        return;
    }

    if (m_returnType.isValid()) {
        // We need to preserve any possible undefined value as that resets the property.
        if (m_typeResolver->canHoldUndefined(m_state.accumulatorIn()))
            addReadAccumulator();
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
    setAccumulator(m_typeResolver->literalType(m_typeResolver->typeForConst(encodedConst)));
}

void QQmlJSTypePropagator::generate_LoadZero()
{
    setAccumulator(m_typeResolver->literalType(m_typeResolver->int32Type()));
}

void QQmlJSTypePropagator::generate_LoadTrue()
{
    setAccumulator(m_typeResolver->literalType(m_typeResolver->boolType()));
}

void QQmlJSTypePropagator::generate_LoadFalse()
{
    setAccumulator(m_typeResolver->literalType(m_typeResolver->boolType()));
}

void QQmlJSTypePropagator::generate_LoadNull()
{
    setAccumulator(m_typeResolver->literalType(m_typeResolver->nullType()));
}

void QQmlJSTypePropagator::generate_LoadUndefined()
{
    setAccumulator(m_typeResolver->literalType(m_typeResolver->voidType()));
}

void QQmlJSTypePropagator::generate_LoadInt(int)
{
    setAccumulator(m_typeResolver->literalType(m_typeResolver->int32Type()));
}

void QQmlJSTypePropagator::generate_MoveConst(int constIndex, int destTemp)
{
    auto encodedConst = m_jsUnitGenerator->constant(constIndex);
    setRegister(destTemp, m_typeResolver->literalType(m_typeResolver->typeForConst(encodedConst)));
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
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_LoadLocal(int index)
{
    // TODO: In order to accurately track locals we'd need to track JavaScript contexts first.
    //       This could be done by populating the initial JS context and implementing the various
    //       Push and Pop operations. For now, this is pretty barren.

    QQmlJSMetaProperty local;
    local.setType(m_typeResolver->jsValueType());
    local.setIndex(index);

    setAccumulator(m_pool->createProperty(
            local, QQmlJSRegisterContent::InvalidLookupIndex,
            QQmlJSRegisterContent::InvalidLookupIndex,
            QQmlJSRegisterContent::Property, QQmlJSRegisterContent()));
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
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
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
    setAccumulator(m_typeResolver->literalType(m_typeResolver->stringType()));
}

void QQmlJSTypePropagator::generate_MoveRegExp(int regExpId, int destReg)
{
    Q_UNUSED(regExpId)
    m_state.setRegister(destReg, m_typeResolver->literalType(m_typeResolver->regexpType()));
}

void QQmlJSTypePropagator::generate_LoadClosure(int value)
{
    Q_UNUSED(value)
    // TODO: Check the function at index and see whether it's a generator to return another type
    // instead.
    setAccumulator(m_typeResolver->literalType(m_typeResolver->functionType()));
}

void QQmlJSTypePropagator::generate_LoadName(int nameIndex)
{
    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);
    setAccumulator(m_typeResolver->scopedType(m_function->qmlScope, name));
    if (!m_state.accumulatorOut().isValid()) {
        addError(u"Cannot find name "_s + name);
        setVarAccumulatorAndError();
    }
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

QQmlJS::SourceLocation QQmlJSTypePropagator::getCurrentNonEmptySourceLocation() const
{
    Q_ASSERT(m_function->sourceLocations);
    const auto &entries = m_function->sourceLocations->entries;

    auto item = std::lower_bound(entries.cbegin(), entries.cend(), currentInstructionOffset(),
                                 [](auto entry, uint offset) { return entry.offset < offset; });
    Q_ASSERT(item != entries.end());

    // filter out empty locations
    while (item->location.length == 0 && item != entries.begin()) {
        --item;
    }
    return item->location;
}

void QQmlJSTypePropagator::handleUnqualifiedAccess(const QString &name, bool isMethod) const
{
    auto location = getCurrentSourceLocation();

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
    const auto qmlScope = m_function->qmlScope.containedType();
    if (name == u"model" || name == u"index") {
        if (const QQmlJSScope::ConstPtr parent = qmlScope->parentScope(); !parent.isNull()) {
            const auto bindings = parent->ownPropertyBindings(u"delegate"_s);

            for (auto it = bindings.first; it != bindings.second; it++) {
                if (!it->hasObject())
                    continue;
                if (it->objectType() == qmlScope) {
                    suggestion = QQmlJSFixSuggestion {
                        name + " is implicitly injected into this delegate."
                               " Add a required property instead."_L1,
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
                const QString id = m_function->addressableScopes.id(scope, qmlScope);

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
                    QQmlJSUtils::didYouMean(
                            name, qmlScope->properties().keys() + qmlScope->methods().keys(),
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
    } else if (property.type() == m_typeResolver->varType()) {
        errorType =
                u"a var property. It may or may not be a method. Use a regular function instead."_s;
    } else if (property.type() == m_typeResolver->jsValueType()) {
        errorType =
                u"a QJSValue property. It may or may not be a method. Use a regular Q_INVOKABLE instead."_s;
    } else {
        errorType = u"not a method"_s;
    }

    m_logger->log(u"%1 \"%2\" is %3"_s.arg(propertyType, name, errorType), qmlUseProperFunction,
                  getCurrentSourceLocation(), true, true, {});

    return true;
}


void QQmlJSTypePropagator::generate_LoadQmlContextPropertyLookup_SAcheck(const QString &name)
{
    const auto qmlScope = m_function->qmlScope.containedType();
    QQmlSA::PassManagerPrivate::get(m_passManager)->analyzeRead(
            QQmlJSScope::createQQmlSAElement(qmlScope), name,
            QQmlJSScope::createQQmlSAElement(qmlScope),
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                    getCurrentNonEmptySourceLocation()));
}


void QQmlJSTypePropagator::generate_LoadQmlContextPropertyLookup(int index)
{
    // LoadQmlContextPropertyLookup does not use accumulatorIn. It always refers to the scope.
    // Any import namespaces etc. are handled via LoadProperty or GetLookup.

    const int nameIndex = m_jsUnitGenerator->lookupNameIndex(index);
    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);

    setAccumulator(m_typeResolver->scopedType(m_function->qmlScope, name, index));

    if (!m_state.accumulatorOut().isValid() && m_typeResolver->isPrefix(name)) {
        setAccumulator(m_pool->createImportNamespace(
                    nameIndex, m_typeResolver->voidType(), QQmlJSRegisterContent::ModulePrefix,
                    m_function->qmlScope));
        return;
    }

    checkDeprecated(m_function->qmlScope.containedType(), name, false);

    const QQmlJSRegisterContent accumulatorOut = m_state.accumulatorOut();

    if (!accumulatorOut.isValid()) {
        addError(u"Cannot access value for name "_s + name);
        handleUnqualifiedAccess(name, false);
        setVarAccumulatorAndError();
        return;
    }

    const QQmlJSScope::ConstPtr retrieved
            = m_typeResolver->genericType(accumulatorOut.containedType());

    if (retrieved.isNull()) {
        // It should really be valid.
        // We get the generic type from aotContext->loadQmlContextPropertyIdLookup().
        addError(u"Cannot determine generic type for "_s + name);
        return;
    }

    if (accumulatorOut.variant() == QQmlJSRegisterContent::ObjectById
            && !retrieved->isReferenceType()) {
        addError(u"Cannot retrieve a non-object type by ID: "_s + name);
        return;
    }

    if (m_passManager != nullptr)
        generate_LoadQmlContextPropertyLookup_SAcheck(name);
}

void QQmlJSTypePropagator::generate_StoreNameCommon_SAcheck(QQmlJSRegisterContent in, const QString &name)
{
    const auto qmlScope = m_function->qmlScope.containedType();
    QQmlSA::PassManagerPrivate::get(m_passManager)->analyzeWrite(
            QQmlJSScope::createQQmlSAElement(qmlScope), name,
            QQmlJSScope::createQQmlSAElement(in.containedType()),
            QQmlJSScope::createQQmlSAElement(qmlScope),
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                    getCurrentNonEmptySourceLocation()));
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
        addError(u"Cannot find name "_s + name);
        return;
    }

    if (!type.isProperty()) {
        QString message = type.isMethod() ? u"Cannot assign to method %1"_s
                                          : u"Cannot assign to non-property %1"_s;
        // The interpreter treats methods as read-only properties in its error messages
        // and we lack a better fitting category. We might want to revisit this later.
        m_logger->log(message.arg(name), qmlReadOnlyProperty,
                      getCurrentSourceLocation());
        addError(u"Cannot assign to non-property "_s + name);
        return;
    }

    if (!type.isWritable()) {
        addError(u"Can't assign to read-only property %1"_s.arg(name));

        m_logger->log(u"Cannot assign to read-only property %1"_s.arg(name), qmlReadOnlyProperty,
                      getCurrentSourceLocation());

        return;
    }

    if (!canConvertFromTo(in, type)) {
        addError(u"cannot convert from %1 to %2"_s
                 .arg(in.descriptiveName(), type.descriptiveName()));
    }

    if (m_passManager != nullptr)
        generate_StoreNameCommon_SAcheck(in, name);


    if (m_typeResolver->canHoldUndefined(in) && !m_typeResolver->canHoldUndefined(type)) {
        if (in.contains(m_typeResolver->voidType()))
            addReadAccumulator(m_typeResolver->varType());
        else
            addReadAccumulator();
    } else {
        addReadAccumulator(type);
    }

    m_state.setHasSideEffects(true);
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
        QQmlJSRegisterContent base, const QString &propertyName)
{
    if (base.isEnumeration()) {
        const auto metaEn = base.enumeration();
        if (!metaEn.hasKey(propertyName)) {
            auto fixSuggestion = QQmlJSUtils::didYouMean(propertyName, metaEn.keys(),
                                                         getCurrentSourceLocation());
            const QString error = u"\"%1\" is not an entry of enum \"%2\"."_s
                                          .arg(propertyName, metaEn.name());
            addError(error);
            m_logger->log(
                    error, qmlMissingEnumEntry, getCurrentSourceLocation(), true, true,
                    fixSuggestion);
            return true;
        }
    } else if (base.variant() == QQmlJSRegisterContent::MetaType) {
        const QQmlJSMetaEnum metaEn = base.scopeType()->enumeration(propertyName);
        if (metaEn.isValid() && !metaEn.isScoped() && !metaEn.isQml()) {
            const QString error
                    = u"You cannot access unscoped enum \"%1\" from here."_s.arg(propertyName);
            addError(error);
            m_logger->log(error, qmlRestrictedType, getCurrentSourceLocation());
            return true;
        }
    }

    return false;
}

void QQmlJSTypePropagator::generate_LoadElement(int base)
{
    const QQmlJSRegisterContent in = m_state.accumulatorIn();
    const QQmlJSRegisterContent baseRegister = m_state.registers[base].content;

    const auto fallback = [&]() {
        const QQmlJSScope::ConstPtr jsValue = m_typeResolver->jsValueType();

        addReadAccumulator(jsValue);
        addReadRegister(base, jsValue);

        QQmlJSMetaProperty property;
        property.setPropertyName(u"[]"_s);
        property.setTypeName(jsValue->internalName());
        property.setType(jsValue);

        setAccumulator(m_pool->createProperty(
                property, QQmlJSRegisterContent::InvalidLookupIndex,
                QQmlJSRegisterContent::InvalidLookupIndex, QQmlJSRegisterContent::ListValue,
                m_typeResolver->convert(m_typeResolver->valueType(baseRegister), jsValue)));
    };

    if (!baseRegister.isList() && !baseRegister.contains(m_typeResolver->stringType())) {
        fallback();
        return;
    }
    addReadRegister(base);

    if (m_typeResolver->isNumeric(in)) {
        const auto contained = in.containedType();
        if (m_typeResolver->isSignedInteger(contained))
            addReadAccumulator(m_typeResolver->sizeType());
        else if (m_typeResolver->isUnsignedInteger(contained))
            addReadAccumulator(m_typeResolver->uint32Type());
        else
            addReadAccumulator(m_typeResolver->realType());
    } else if (m_typeResolver->isNumeric(m_typeResolver->extractNonVoidFromOptionalType(in))) {
        addReadAccumulator();
    } else {
        fallback();
        return;
    }

    // We can end up with undefined.
    setAccumulator(m_typeResolver->merge(
            m_typeResolver->valueType(baseRegister),
            m_typeResolver->literalType(m_typeResolver->voidType())));
}

void QQmlJSTypePropagator::generate_StoreElement(int base, int index)
{
    const QQmlJSRegisterContent baseRegister = m_state.registers[base].content;
    const QQmlJSRegisterContent indexRegister = checkedInputRegister(index);

    if (!baseRegister.isList()
            || !m_typeResolver->isNumeric(indexRegister)) {
        const auto jsValue = m_typeResolver->jsValueType();
        addReadAccumulator(jsValue);
        addReadRegister(base, jsValue);
        addReadRegister(index, jsValue);

        // Writing to a JS array can have side effects all over the place since it's
        // passed by reference.
        m_state.setHasSideEffects(true);
        return;
    }

    const auto contained = indexRegister.containedType();
    if (m_typeResolver->isSignedInteger(contained))
        addReadRegister(index, m_typeResolver->int32Type());
    else if (m_typeResolver->isUnsignedInteger(contained))
        addReadRegister(index, m_typeResolver->uint32Type());
    else
        addReadRegister(index, m_typeResolver->realType());

    addReadRegister(base);
    addReadAccumulator(m_typeResolver->valueType(baseRegister));

    // If we're writing a QQmlListProperty backed by a container somewhere else,
    // that has side effects.
    // If we're writing to a list retrieved from a property, that _should_ have side effects,
    // but currently the QML engine doesn't implement them.
    // TODO: Figure out the above and accurately set the flag.
    m_state.setHasSideEffects(true);
}

void QQmlJSTypePropagator::propagatePropertyLookup_SAcheck(const QString &propertyName)
{
    const QQmlJSRegisterContent in = m_state.accumulatorIn();
    const bool isAttached = in.variant() == QQmlJSRegisterContent::Attachment;

    QQmlSA::PassManagerPrivate::get(m_passManager)->analyzeRead(
            QQmlJSScope::createQQmlSAElement(
                    m_state.accumulatorIn().containedType()),
            propertyName,
            QQmlJSScope::createQQmlSAElement(isAttached
                    ? in.attachee().containedType()
                    : m_function->qmlScope.containedType()),
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                    getCurrentNonEmptySourceLocation()));
}

void QQmlJSTypePropagator::propagatePropertyLookup(const QString &propertyName, int lookupIndex)
{
    setAccumulator(
            m_typeResolver->memberType(
                m_state.accumulatorIn(),
                m_state.accumulatorIn().isImportNamespace()
                    ? m_jsUnitGenerator->stringForIndex(m_state.accumulatorIn().importNamespace())
                      + u'.' + propertyName
                    : propertyName, lookupIndex));

    if (!m_state.accumulatorOut().isValid()) {
        if (m_typeResolver->isPrefix(propertyName)) {

            const QQmlJSRegisterContent accumulatorIn = m_state.accumulatorIn();
            Q_ASSERT(accumulatorIn.isValid());

            if (!accumulatorIn.containedType()->isReferenceType()) {
                m_logger->log(u"Cannot use non-QObject type %1 to access prefixed import"_s.arg(
                                      accumulatorIn.containedType()->internalName()),
                              qmlPrefixedImportType,
                              currentSourceLocation());
                setVarAccumulatorAndError();
                return;
            }

            addReadAccumulator();
            setAccumulator(m_pool->createImportNamespace(
                        m_jsUnitGenerator->getStringId(propertyName),
                        m_state.accumulatorIn().containedType(),
                        QQmlJSRegisterContent::ModulePrefix,
                        m_state.accumulatorIn()));
            return;
        }
        if (m_state.accumulatorIn().isImportNamespace())
            m_logger->log(u"Type not found in namespace"_s, qmlUnresolvedType,
                          getCurrentSourceLocation());
    }

    if (m_state.accumulatorOut().variant() == QQmlJSRegisterContent::Singleton
        && m_state.accumulatorIn().variant() == QQmlJSRegisterContent::ModulePrefix
        && !isQmlScopeObject(m_state.accumulatorIn().scope())) {
        m_logger->log(
                u"Cannot access singleton as a property of an object. Did you want to access an attached object?"_s,
                qmlAccessSingleton, getCurrentSourceLocation());
        setAccumulator(QQmlJSRegisterContent());
    } else if (m_state.accumulatorOut().isEnumeration()) {
        switch (m_state.accumulatorIn().variant()) {
        case QQmlJSRegisterContent::MetaType:
        case QQmlJSRegisterContent::Attachment:
        case QQmlJSRegisterContent::Enum:
        case QQmlJSRegisterContent::ModulePrefix:
        case QQmlJSRegisterContent::Singleton:
            break; // OK, can look up enums on that thing
        default:
            setAccumulator(QQmlJSRegisterContent());
        }
    }

    if (m_state.instructionHasError || !m_state.accumulatorOut().isValid()) {
        setVarAccumulatorAndError();
        if (checkForEnumProblems(m_state.accumulatorIn(), propertyName))
            return;

        addError(u"Cannot load property %1 from %2."_s
                         .arg(propertyName, m_state.accumulatorIn().descriptiveName()));

        const QString typeName = m_state.accumulatorIn().containedTypeName();

        if (typeName == u"QVariant")
            return;
        if (m_state.accumulatorIn().isList() && propertyName == u"length")
            return;

        auto baseType = m_state.accumulatorIn().containedType();
        // Warn separately when a property is only not found because of a missing type

        if (propertyResolution(baseType, propertyName) != PropertyMissing)
            return;

        if (baseType->isScript())
            return;

        std::optional<QQmlJSFixSuggestion> fixSuggestion;

        if (auto suggestion = QQmlJSUtils::didYouMean(propertyName, baseType->properties().keys(),
                                                      getCurrentSourceLocation());
            suggestion.has_value()) {
            fixSuggestion = suggestion;
        }

        if (!fixSuggestion.has_value()
                && m_state.accumulatorIn().variant() == QQmlJSRegisterContent::MetaType) {

            const QQmlJSScope::ConstPtr scopeType
                    = m_state.accumulatorIn().scopeType();
            const auto metaEnums = scopeType->enumerations();
            const bool enforcesScoped = scopeType->enforcesScopedEnums();

            QStringList enumKeys;
            for (const QQmlJSMetaEnum &metaEnum : metaEnums) {
                if (!enforcesScoped || !metaEnum.isScoped())
                    enumKeys << metaEnum.keys();
            }

            if (auto suggestion = QQmlJSUtils::didYouMean(
                        propertyName, enumKeys, getCurrentSourceLocation());
                suggestion.has_value()) {
                fixSuggestion = suggestion;
            }
        }

        m_logger->log(u"Member \"%1\" not found on type \"%2\""_s.arg(propertyName).arg(typeName),
                      qmlMissingProperty, getCurrentSourceLocation(), true, true, fixSuggestion);
        return;
    }

    if (m_state.accumulatorOut().isMethod() && m_state.accumulatorOut().method().size() != 1) {
        addError(u"Cannot determine overloaded method on loadProperty"_s);
        return;
    }

    if (m_state.accumulatorOut().isProperty()) {
        const QQmlJSScope::ConstPtr mathObject
                = m_typeResolver->jsGlobalObject()->property(u"Math"_s).type();
        if (m_state.accumulatorIn().contains(mathObject)) {
            QQmlJSMetaProperty prop;
            prop.setPropertyName(propertyName);
            prop.setTypeName(u"double"_s);
            prop.setType(m_typeResolver->realType());
            setAccumulator(
                m_pool->createProperty(
                    prop, m_state.accumulatorIn().resultLookupIndex(), lookupIndex,
                    // Use pre-determined scope type here to avoid adjusting it later.
                    QQmlJSRegisterContent::Property, m_state.accumulatorOut().scope())
            );

            return;
        }

        if (m_state.accumulatorOut().contains(m_typeResolver->voidType())) {
            addError(u"Type %1 does not have a property %2 for reading"_s
                             .arg(m_state.accumulatorIn().descriptiveName(), propertyName));
            return;
        }

        if (!m_state.accumulatorOut().property().type()) {
            m_logger->log(
                    QString::fromLatin1("Type of property \"%2\" not found").arg(propertyName),
                    qmlMissingType, getCurrentSourceLocation());
        }
    }

    if (m_passManager != nullptr)
        propagatePropertyLookup_SAcheck(propertyName);

    switch (m_state.accumulatorOut().variant()) {
    case QQmlJSRegisterContent::Enum:
    case QQmlJSRegisterContent::Singleton:
        // For reading enums or singletons, we don't need to access anything, unless it's an
        // import namespace. Then we need the name.
        if (m_state.accumulatorIn().isImportNamespace())
            addReadAccumulator();
        break;
    default:
        addReadAccumulator();
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
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_GetLookup(int index)
{
    propagatePropertyLookup(m_jsUnitGenerator->lookupName(index), index);
}

void QQmlJSTypePropagator::generate_GetOptionalLookup(int index, int offset)
{
    Q_UNUSED(offset);
    saveRegisterStateForJump(offset);
    propagatePropertyLookup(m_jsUnitGenerator->lookupName(index), index);
}

void QQmlJSTypePropagator::generate_StoreProperty_SAcheck(const QString &propertyName,
                                                          QQmlJSRegisterContent callBase)
{
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
                    getCurrentNonEmptySourceLocation()));
}

void QQmlJSTypePropagator::generate_StoreProperty(int nameIndex, int base)
{
    auto callBase = m_state.registers[base].content;
    const QString propertyName = m_jsUnitGenerator->stringForIndex(nameIndex);

    QQmlJSRegisterContent property = m_typeResolver->memberType(callBase, propertyName);
    if (!property.isProperty()) {
        addError(u"Type %1 does not have a property %2 for writing"_s
                         .arg(callBase.descriptiveName(), propertyName));
        return;
    }

    if (property.containedType().isNull()) {
        addError(u"Cannot determine type for property %1 of type %2"_s.arg(
                propertyName, callBase.descriptiveName()));
        return;
    }

    if (!property.isWritable() && !property.containedType()->isListProperty()) {
        addError(u"Can't assign to read-only property %1"_s.arg(propertyName));

        m_logger->log(u"Cannot assign to read-only property %1"_s.arg(propertyName),
                      qmlReadOnlyProperty, getCurrentSourceLocation());

        return;
    }

    if (!canConvertFromTo(m_state.accumulatorIn(), property)) {
        addError(u"cannot convert from %1 to %2"_s
                         .arg(m_state.accumulatorIn().descriptiveName(), property.descriptiveName()));
        return;
    }

    if (m_passManager != nullptr)
        generate_StoreProperty_SAcheck(propertyName, callBase);

    // If the input can hold undefined we must not coerce it to the property type
    // as that might eliminate an undefined value. For example, undefined -> string
    // becomes "undefined".
    // We need the undefined value for either resetting the property if that is supported
    // or generating an exception otherwise. Therefore we explicitly require the value to
    // be given as QVariant. This triggers the QVariant fallback path that's also used for
    // shadowable properties. QVariant can hold undefined and the lookup functions will
    // handle that appropriately.

    const QQmlJSScope::ConstPtr varType = m_typeResolver->varType();
    const QQmlJSRegisterContent readType = m_typeResolver->canHoldUndefined(m_state.accumulatorIn())
            ? m_typeResolver->convert(property, varType)
            : std::move(property);
    addReadAccumulator(readType);
    addReadRegister(base);
    m_state.setHasSideEffects(true);
}

void QQmlJSTypePropagator::generate_SetLookup(int index, int base)
{
    generate_StoreProperty(m_jsUnitGenerator->lookupNameIndex(index), base);
}

void QQmlJSTypePropagator::generate_LoadSuperProperty(int property)
{
    Q_UNUSED(property)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
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
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_CallWithReceiver(int name, int thisObject, int argc, int argv)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(name)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

static bool isLoggingMethod(const QString &consoleMethod)
{
    return consoleMethod == u"log" || consoleMethod == u"debug" || consoleMethod == u"info"
            || consoleMethod == u"warn" || consoleMethod == u"error";
}

void QQmlJSTypePropagator::generate_CallProperty_SCMath(
        const QString &name, int base, int argc, int argv)
{
    // If we call a method on the Math object we don't need the actual Math object. We do need
    // to transfer the type information to the code generator so that it knows that this is the
    // Math object. Read the base register as void. void isn't stored, and the place where it's
    // created will be optimized out if there are no other readers. The code generator can
    // retrieve the original type and determine that it was the Math object.

    addReadRegister(base, m_typeResolver->voidType());

    QQmlJSRegisterContent math = m_state.registers[base].content;
    const QList<QQmlJSMetaMethod> methods = math.containedType()->ownMethods(name);
    Q_ASSERT(methods.length() == 1);

    // Declare the Math object as base type of itself so that it gets cloned and won't be
    // adjusted later. This is what we do with all method calls.
    QQmlJSRegisterContent realType = m_typeResolver->returnType(
            methods[0], m_typeResolver->realType(),
            m_typeResolver->baseType(math.containedType(), math));
    for (int i = 0; i < argc; ++i)
        addReadRegister(argv + i, realType);
    setAccumulator(realType);
}

void QQmlJSTypePropagator::generate_CallProperty_SCconsole(
        const QString &name, int base, int argc, int argv)
{
    // If we call a method on the console object we don't need the console object.
    addReadRegister(base, m_typeResolver->voidType());

    if (argc > 0) {
        const QQmlJSRegisterContent firstContent = m_state.registers[argv].content;
        const QQmlJSScope::ConstPtr firstArg = firstContent.containedType();
        switch (firstArg->accessSemantics()) {
        case QQmlJSScope::AccessSemantics::Reference:
            // We cannot know whether this will be a logging category at run time.
            // Therefore we always pass any object types as special last argument.
            addReadRegister(argv, m_typeResolver->genericType(firstArg));
            break;
        case QQmlJSScope::AccessSemantics::Sequence:
            addReadRegister(argv);
            break;
        default:
            addReadRegister(argv, m_typeResolver->stringType());
            break;
        }
    }

    for (int i = 1; i < argc; ++i) {
        const QQmlJSRegisterContent argContent = m_state.registers[argv + i].content;
        const QQmlJSScope::ConstPtr arg = argContent.containedType();
        if (arg->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence)
            addReadRegister(argv + i);
        else
            addReadRegister(argv + i, m_typeResolver->stringType());
    }

    m_state.setHasSideEffects(true);

    QQmlJSRegisterContent console = m_state.registers[base].content;
    QList<QQmlJSMetaMethod> methods = console.containedType()->ownMethods(name);
    Q_ASSERT(methods.length() == 1);

    // Declare the console object as base type of itself so that it gets cloned and won't be
    // adjusted later. This is what we do with all method calls.
    setAccumulator(m_typeResolver->returnType(
            methods[0], m_typeResolver->voidType(),
            m_typeResolver->baseType(console.containedType(), console)));
}

void QQmlJSTypePropagator::propagateCall_SAcheck(const QQmlJSMetaMethod &method,
                                                 const QQmlJSScope::ConstPtr &baseType)
{
    Q_ASSERT(m_function);

    const QQmlSA::Element saBaseType = QQmlJSScope::createQQmlSAElement(baseType);
    const QQmlSA::SourceLocation saLocation{
        QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(getCurrentSourceLocation())
    };
    const QQmlSA::Element saContainedType{ QQmlJSScope::createQQmlSAElement(
            m_function->qmlScope.containedType()) };

    QQmlSA::PassManagerPrivate::get(m_passManager)
            ->analyzeCall(saBaseType, method.methodName(), saContainedType, saLocation);
}

void QQmlJSTypePropagator::generate_callProperty_SAcheck(const QString &propertyName,
                                                         const QQmlJSScope::ConstPtr &baseType)
{
    Q_ASSERT(m_function);

    const QQmlSA::Element saBaseType{ QQmlJSScope::createQQmlSAElement(baseType) };
    const QQmlSA::Element saContainedType{ QQmlJSScope::createQQmlSAElement(
            m_function->qmlScope.containedType()) };
    const QQmlSA::SourceLocation saLocation{
        QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(getCurrentSourceLocation())
    };

    QQmlSA::PassManagerPrivate::get(m_passManager)
            ->analyzeRead(saBaseType, propertyName, saContainedType, saLocation);
    QQmlSA::PassManagerPrivate::get(m_passManager)
            ->analyzeCall(saBaseType, propertyName, saContainedType, saLocation);
}

void QQmlJSTypePropagator::generate_CallProperty(int nameIndex, int base, int argc, int argv)
{
    Q_ASSERT(m_state.registers.contains(base));
    const auto callBase = m_state.registers[base].content;
    const QString propertyName = m_jsUnitGenerator->stringForIndex(nameIndex);

    if (callBase.contains(m_typeResolver->mathObject())) {
        generate_CallProperty_SCMath(propertyName, base, argc, argv);
        if (m_passManager != nullptr)
            generate_callProperty_SAcheck(propertyName, callBase.containedType());
        return;
    }

    if (callBase.contains(m_typeResolver->consoleObject()) && isLoggingMethod(propertyName)) {
        generate_CallProperty_SCconsole(propertyName, base, argc, argv);
        if (m_passManager != nullptr)
            generate_callProperty_SAcheck(propertyName, callBase.containedType());
        return;
    }

    const auto baseType = callBase.containedType();
    const auto member = m_typeResolver->memberType(callBase, propertyName);

    if (!member.isMethod()) {
        if (callBase.contains(m_typeResolver->jsValueType())
                || callBase.contains(m_typeResolver->varType())) {
            const auto jsValueType = m_typeResolver->jsValueType();
            addReadRegister(base, jsValueType);
            for (int i = 0; i < argc; ++i)
                addReadRegister(argv + i, jsValueType);
            m_state.setHasSideEffects(true);

            QQmlJSMetaMethod method;
            method.setIsJavaScriptFunction(true);
            method.setMethodName(propertyName);
            method.setMethodType(QQmlJSMetaMethod::MethodType::Method);

            setAccumulator(m_typeResolver->returnType(
                    method, m_typeResolver->jsValueType(), callBase));

            if (m_passManager != nullptr)
                generate_callProperty_SAcheck(propertyName, callBase.containedType());
            return;
        }

        setVarAccumulatorAndError();
        addError(u"Type %1 does not have a property %2 for calling"_s
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
                              propertyName, callBase.containedTypeName()),
                      qmlMissingProperty, getCurrentSourceLocation(), true, true, fixSuggestion);
        return;
    }

    checkDeprecated(baseType, propertyName, true);

    addReadRegister(base);

    if (callBase.contains(m_typeResolver->stringType())) {
        if (propertyName == u"arg"_s && argc == 1) {
            propagateStringArgCall(callBase, argv);
            return;
        }
    }

    if (baseType->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence
            && member.scope().contains(m_typeResolver->arrayPrototype())
            && propagateArrayMethod(propertyName, argc, argv, callBase)) {
        return;
    }

    propagateCall(member.method(), argc, argv, member.scope());
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
            if (content.contains(argumentType))
                continue;

            exactMatch = false;
            if (canConvertFromTo(content, argumentType))
                continue;

            // We can try to call a method that expects a derived type.
            if (argumentType->isReferenceType()
                    && m_typeResolver->inherits(
                        argumentType->baseType(), content.containedType())) {
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

void QQmlJSTypePropagator::setAccumulator(QQmlJSRegisterContent content)
{
    setRegister(Accumulator, content);
}

void QQmlJSTypePropagator::setRegister(int index, QQmlJSRegisterContent content)
{
    // If we've come to the same conclusion before, let's not track the type again.
    auto it = m_prevStateAnnotations.find(currentInstructionOffset());
    if (it != m_prevStateAnnotations.end()) {
        QQmlJSRegisterContent lastTry = it->second.changedRegister;
        if (lastTry.contains(content.containedType())) {
            m_state.setRegister(index, lastTry);
            return;
        }
    }

    m_state.setRegister(index, content);
}

void QQmlJSTypePropagator::mergeRegister(
            int index, QQmlJSRegisterContent a, QQmlJSRegisterContent b)
{
    const QQmlJSRegisterContent merged = (a == b) ? a : m_typeResolver->merge(a, b);
    Q_ASSERT(merged.isValid());

    if (!merged.isConversion()) {
        // The registers were the same. We're already tracking them.
        m_state.annotations[currentInstructionOffset()].typeConversions[index].content = merged;
        m_state.registers[index].content = merged;
        return;
    }

    auto tryPrevStateConversion = [this](int index, QQmlJSRegisterContent merged) -> bool {
        auto it = m_prevStateAnnotations.find(currentInstructionOffset());
        if (it == m_prevStateAnnotations.end())
            return false;

        auto conversion = it->second.typeConversions.find(index);
        if (conversion == it->second.typeConversions.end())
            return false;

        const VirtualRegister &lastTry = conversion.value();

        Q_ASSERT(lastTry.content.isValid());
        if (!lastTry.content.isConversion())
            return false;

        if (lastTry.content.conversionResultType() != merged.conversionResultType()
                || lastTry.content.conversionOrigins() != merged.conversionOrigins()) {
            return false;
        }

        // We don't need to track it again if we've come to the same conclusion before.
        m_state.annotations[currentInstructionOffset()].typeConversions[index] = lastTry;
        m_state.registers[index] = lastTry;
        return true;
    };

    if (!tryPrevStateConversion(index, merged)) {
        // if a != b, we have already re-tracked it.
        QQmlJSRegisterContent cloned = (a == b) ? m_pool->clone(merged) : merged;
        Q_ASSERT(cloned.isValid());
        m_state.annotations[currentInstructionOffset()].typeConversions[index].content = cloned;
        m_state.registers[index].content = cloned;
    }
}

void QQmlJSTypePropagator::addReadRegister(int index)
{
    // Explicitly pass the same type through without conversion
    m_state.addReadRegister(index, m_state.registers[index].content);
}

void QQmlJSTypePropagator::addReadRegister(int index, QQmlJSRegisterContent convertTo)
{
    if (m_state.registers[index].content == convertTo) {
        // Explicitly pass the same type through without conversion
        m_state.addReadRegister(index, convertTo);
    } else {
        m_state.addReadRegister(
                index, m_typeResolver->convert(m_state.registers[index].content, convertTo));
    }
}

void QQmlJSTypePropagator::addReadRegister(int index, const QQmlJSScope::ConstPtr &convertTo)
{
    m_state.addReadRegister(
            index, m_typeResolver->convert(m_state.registers[index].content, convertTo));
}

void QQmlJSTypePropagator::propagateCall(
        const QList<QQmlJSMetaMethod> &methods, int argc, int argv,
        QQmlJSRegisterContent scope)
{
    QStringList errors;
    const QQmlJSMetaMethod match = bestMatchForCall(methods, argc, argv, &errors);

    if (!match.isValid()) {
        setVarAccumulatorAndError();
        if (methods.size() == 1) {
            // Cannot have multiple fuzzy matches if there is only one method
            Q_ASSERT(errors.size() == 1);
            addError(errors.first());
        } else if (errors.size() < methods.size()) {
            addError(u"Multiple matching overrides found. Cannot determine the right one."_s);
        } else {
            addError(u"No matching override found. Candidates:\n"_s + errors.join(u'\n'));
        }
        return;
    }

    if (m_passManager)
        propagateCall_SAcheck(match, scope.containedType());

    QQmlJSScope::ConstPtr returnType;
    if (match.isJavaScriptFunction())
        returnType = m_typeResolver->jsValueType();
    else if (match.isConstructor())
        returnType = scope.containedType();
    else
        returnType = match.returnType();

    setAccumulator(m_typeResolver->returnType(match, returnType, scope));
    if (!m_state.accumulatorOut().isValid())
        addError(u"Cannot store return type of method %1()."_s.arg(match.methodName()));

    const auto types = match.parameters();
    for (int i = 0; i < argc; ++i) {
        if (i < types.size()) {
            const QQmlJSScope::ConstPtr type = match.isJavaScriptFunction()
                    ? m_typeResolver->jsValueType()
                    : QQmlJSScope::ConstPtr(types.at(i).type());
            if (!type.isNull()) {
                addReadRegister(argv + i, type);
                continue;
            }
        }
        addReadRegister(argv + i, m_typeResolver->jsValueType());
    }
    m_state.setHasSideEffects(true);
}

void QQmlJSTypePropagator::propagateTranslationMethod_SAcheck(const QString &methodName)
{
    QQmlSA::PassManagerPrivate::get(m_passManager)
    ->analyzeCall(QQmlJSScope::createQQmlSAElement(m_typeResolver->jsGlobalObject()),
                  methodName,
                  QQmlJSScope::createQQmlSAElement(m_function->qmlScope.containedType()),
                  QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                          getCurrentNonEmptySourceLocation()));
}

bool QQmlJSTypePropagator::propagateTranslationMethod(
        const QList<QQmlJSMetaMethod> &methods, int argc, int argv)
{
    if (methods.size() != 1)
        return false;

    const QQmlJSMetaMethod method = methods.front();
    const QQmlJSScope::ConstPtr intType = m_typeResolver->int32Type();
    const QQmlJSScope::ConstPtr stringType = m_typeResolver->stringType();

    const QQmlJSRegisterContent returnType = m_typeResolver->returnType(
            method, m_typeResolver->stringType(), m_typeResolver->jsGlobalObjectContent());

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

            if (m_passManager)
                propagateTranslationMethod_SAcheck(method.methodName());
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

            if (m_passManager)
                propagateTranslationMethod_SAcheck(method.methodName());
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

            if (m_passManager)
                propagateTranslationMethod_SAcheck(method.methodName());
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

            if (m_passManager)
                propagateTranslationMethod_SAcheck(method.methodName());
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

            if (m_passManager)
                propagateTranslationMethod_SAcheck(method.methodName());
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

            if (m_passManager)
                propagateTranslationMethod_SAcheck(method.methodName());
            return true;
        default:
            return false;
        }
    }

    return false;
}

void QQmlJSTypePropagator::propagateStringArgCall(QQmlJSRegisterContent base, int argv)
{
    QQmlJSMetaMethod method;
    method.setIsJavaScriptFunction(true);
    method.setMethodName(u"arg"_s);
    setAccumulator(m_typeResolver->returnType(method, m_typeResolver->stringType(), base));
    Q_ASSERT(m_state.accumulatorOut().isValid());

    const QQmlJSScope::ConstPtr input = m_state.registers[argv].content.containedType();

    if (input == m_typeResolver->uint32Type()
            || input == m_typeResolver->int64Type()
            || input == m_typeResolver->uint64Type()) {
        addReadRegister(argv, m_typeResolver->realType());
        return;
    }

    if (m_typeResolver->isIntegral(input)) {
        addReadRegister(argv, m_typeResolver->int32Type());
        return;
    }

    if (m_typeResolver->isNumeric(input)) {
        addReadRegister(argv, m_typeResolver->realType());
        return;
    }

    if (input == m_typeResolver->boolType()) {
        addReadRegister(argv, m_typeResolver->boolType());
        return;
    }

    addReadRegister(argv, m_typeResolver->stringType());
}

bool QQmlJSTypePropagator::propagateArrayMethod(
        const QString &name, int argc, int argv, QQmlJSRegisterContent baseType)
{
    // TODO:
    // * For concat() we need to decide what kind of array to return and what kinds of arguments to
    //   accept.
    // * For entries(), keys(), and values() we need iterators.
    // * For find(), findIndex(), sort(), every(), some(), forEach(), map(), filter(), reduce(),
    //   and reduceRight() we need typed function pointers.

    const auto intType = m_typeResolver->int32Type();
    const auto stringType = m_typeResolver->stringType();
    const auto baseContained = baseType.containedType();
    const auto valueContained = baseContained->valueType();

    const auto setReturnType = [&](const QQmlJSScope::ConstPtr type) {
        QQmlJSMetaMethod method;
        method.setIsJavaScriptFunction(true);
        method.setMethodName(name);
        setAccumulator(m_typeResolver->returnType(method, type, baseType));
    };

    if (name == u"copyWithin" && argc > 0 && argc < 4) {
        for (int i = 0; i < argc; ++i) {
            if (!canConvertFromTo(m_state.registers[argv + i].content, intType))
                return false;
        }

        for (int i = 0; i < argc; ++i)
            addReadRegister(argv + i, intType);

        m_state.setHasSideEffects(true);
        setReturnType(baseContained);
        return true;
    }

    if (name == u"fill" && argc > 0 && argc < 4) {
        if (!canConvertFromTo(m_state.registers[argv].content, valueContained))
            return false;

        for (int i = 1; i < argc; ++i) {
            if (!canConvertFromTo(m_state.registers[argv + i].content, intType))
                return false;
        }

        addReadRegister(argv, valueContained);

        for (int i = 1; i < argc; ++i)
            addReadRegister(argv + i, intType);

        m_state.setHasSideEffects(true);
        setReturnType(baseContained);
        return true;
    }

    if (name == u"includes" && argc > 0 && argc < 3) {
        if (!canConvertFromTo(m_state.registers[argv].content, valueContained))
            return false;

        if (argc == 2) {
            if (!canConvertFromTo(m_state.registers[argv + 1].content, intType))
                return false;
            addReadRegister(argv + 1, intType);
        }

        addReadRegister(argv, valueContained);
        setReturnType(m_typeResolver->boolType());
        return true;
    }

    if (name == u"toString" || (name == u"join" && argc < 2)) {
        if (argc == 1) {
            if (!canConvertFromTo(m_state.registers[argv].content, stringType))
                return false;
            addReadRegister(argv, stringType);
        }

        setReturnType(m_typeResolver->stringType());
        return true;
    }

    if ((name == u"pop" || name == u"shift") && argc == 0) {
        m_state.setHasSideEffects(true);
        setReturnType(valueContained);
        return true;
    }

    if (name == u"push" || name == u"unshift") {
        for (int i = 0; i < argc; ++i) {
            if (!canConvertFromTo(m_state.registers[argv + i].content, valueContained))
                return false;
        }

        for (int i = 0; i < argc; ++i)
            addReadRegister(argv + i, valueContained);

        m_state.setHasSideEffects(true);
        setReturnType(m_typeResolver->int32Type());
        return true;
    }

    if (name == u"reverse" && argc == 0) {
        m_state.setHasSideEffects(true);
        setReturnType(baseContained);
        return true;
    }

    if (name == u"slice" && argc < 3) {
        for (int i = 0; i < argc; ++i) {
            if (!canConvertFromTo(m_state.registers[argv + i].content, intType))
                return false;
        }

        for (int i = 0; i < argc; ++i)
            addReadRegister(argv + i, intType);

        setReturnType(baseType.containedType()->isListProperty()
                               ? m_typeResolver->qObjectListType()
                               : baseContained);
        return true;
    }

    if (name == u"splice" && argc > 0) {
        for (int i = 0; i < 2; ++i) {
            if (!canConvertFromTo(m_state.registers[argv + i].content, intType))
                return false;
        }

        for (int i = 2; i < argc; ++i) {
            if (!canConvertFromTo(m_state.registers[argv + i].content, valueContained))
                return false;
        }

        for (int i = 0; i < 2; ++i)
            addReadRegister(argv + i, intType);

        for (int i = 2; i < argc; ++i)
            addReadRegister(argv + i, valueContained);

        m_state.setHasSideEffects(true);
        setReturnType(baseContained);
        return true;
    }

    if ((name == u"indexOf" || name == u"lastIndexOf") && argc > 0 && argc < 3) {
        if (!canConvertFromTo(m_state.registers[argv].content, valueContained))
            return false;

        if (argc == 2) {
            if (!canConvertFromTo(m_state.registers[argv + 1].content, intType))
                return false;
            addReadRegister(argv + 1, intType);
        }

        addReadRegister(argv, valueContained);
        setReturnType(m_typeResolver->int32Type());
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
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::propagateScopeLookupCall(const QString &functionName, int argc, int argv)
{
    const QQmlJSRegisterContent resolvedContent
            = m_typeResolver->scopedType(m_function->qmlScope, functionName);
    if (resolvedContent.isMethod()) {
        const auto methods = resolvedContent.method();
        if (resolvedContent.scope().contains(m_typeResolver->jsGlobalObject())) {
            if (propagateTranslationMethod(methods, argc, argv))
                return;
        }

        if (!methods.isEmpty()) {
            propagateCall(methods, argc, argv, resolvedContent.scope());
            return;
        }
    }

    addError(u"method %1 cannot be resolved."_s.arg(functionName));
    const auto jsValue = m_typeResolver->jsValueType();
    QQmlJSMetaMethod method;
    method.setMethodName(functionName);
    method.setIsJavaScriptFunction(true);
    setAccumulator(m_typeResolver->returnType(method, jsValue, m_function->qmlScope));

    addError(u"Cannot find function '%1'"_s.arg(functionName));

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
    checkDeprecated(m_function->qmlScope.containedType(), name, true);
}

void QQmlJSTypePropagator::generate_CallWithSpread(int func, int thisObject, int argc, int argv)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(func)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
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

void QQmlJSTypePropagator::generate_Construct_SCDate(
        const QQmlJSMetaMethod &ctor, int argc, int argv)
{
    setAccumulator(m_typeResolver->returnType(ctor, m_typeResolver->dateTimeType(), {}));

    if (argc == 1) {
        const QQmlJSRegisterContent argType = m_state.registers[argv].content;
        if (m_typeResolver->isNumeric(argType)) {
            addReadRegister(argv, m_typeResolver->realType());
        } else if (argType.contains(m_typeResolver->stringType())) {
            addReadRegister(argv, m_typeResolver->stringType());
        } else if (argType.contains(m_typeResolver->dateTimeType())
                   || argType.contains(m_typeResolver->dateType())
                   || argType.contains(m_typeResolver->timeType())) {
            addReadRegister(argv, m_typeResolver->dateTimeType());
        } else {
            addReadRegister(argv, m_typeResolver->jsPrimitiveType());
        }
    } else {
        constexpr int maxArgc = 7; // year, month, day, hours, minutes, seconds, milliseconds
        for (int i = 0; i < std::min(argc, maxArgc); ++i)
            addReadRegister(argv + i, m_typeResolver->realType());
    }
}

void QQmlJSTypePropagator::generate_Construct_SCArray(
        const QQmlJSMetaMethod &ctor, int argc, int argv)
{
    if (argc == 1) {
        if (m_typeResolver->isNumeric(m_state.registers[argv].content)) {
            setAccumulator(m_typeResolver->returnType(ctor, m_typeResolver->variantListType(), {}));
            addReadRegister(argv, m_typeResolver->realType());
        } else {
            generate_DefineArray(argc, argv);
        }
    } else {
        generate_DefineArray(argc, argv);
    }
}
void QQmlJSTypePropagator::generate_Construct(int func, int argc, int argv)
{
    const QQmlJSRegisterContent type = m_state.registers[func].content;
    if (type.contains(m_typeResolver->metaObjectType())) {
        const QQmlJSRegisterContent valueType = type.scope();
        const QQmlJSScope::ConstPtr contained = type.scopeType();
        if (contained->isValueType() && contained->isCreatable()) {
            const auto extension = contained->extensionType();
            if (extension.extensionSpecifier == QQmlJSScope::ExtensionType) {
                propagateCall(
                        extension.scope->ownMethods(extension.scope->internalName()),
                        argc, argv, valueType);
            } else {
                propagateCall(
                        contained->ownMethods(contained->internalName()), argc, argv, valueType);
            }
            return;
        }
    }

    if (!type.isMethod()) {
        m_state.setHasSideEffects(true);
        QQmlJSMetaMethod method;
        method.setMethodName(type.containedTypeName());
        method.setIsJavaScriptFunction(true);
        method.setIsConstructor(true);
        setAccumulator(m_typeResolver->returnType(method, m_typeResolver->jsValueType(), {}));
        return;
    }

    if (const auto methods = type.method();
            methods == m_typeResolver->jsGlobalObject()->methods(u"Date"_s)) {
        Q_ASSERT(methods.length() == 1);
        generate_Construct_SCDate(methods[0], argc, argv);
        return;
    }

    if (const auto methods = type.method();
            methods == m_typeResolver->jsGlobalObject()->methods(u"Array"_s)) {
        Q_ASSERT(methods.length() == 1);
        generate_Construct_SCArray(methods[0], argc, argv);
        return;
    }

    m_state.setHasSideEffects(true);

    QStringList errors;
    QQmlJSMetaMethod match = bestMatchForCall(type.method(), argc, argv, &errors);
    if (!match.isValid())
        addError(u"Cannot determine matching constructor. Candidates:\n"_s + errors.join(u'\n'));
    setAccumulator(m_typeResolver->returnType(match, m_typeResolver->jsValueType(), {}));
}

void QQmlJSTypePropagator::generate_ConstructWithSpread(int func, int argc, int argv)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(func)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
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
        addError(u"Cannot statically assert the dead temporal zone check for %1"_s.arg(
                name ? m_jsUnitGenerator->stringForIndex(name) : u"the anonymous accumulator"_s));
    };

    const QQmlJSRegisterContent in = m_state.accumulatorIn();
    if (in.isConversion()) {
        for (QQmlJSRegisterContent origin : in.conversionOrigins()) {
            if (!origin.contains(m_typeResolver->emptyType()))
                continue;
            fail();
            break;
        }
    } else if (in.contains(m_typeResolver->emptyType())) {
        fail();
    }
}

void QQmlJSTypePropagator::generate_ThrowException()
{
    addReadAccumulator(m_typeResolver->jsValueType());
    m_state.setHasSideEffects(true);
    m_state.skipInstructionsUntilNextJumpTarget = true;
}

void QQmlJSTypePropagator::generate_GetException()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
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
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
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
    const QQmlJSRegisterContent listType = m_state.accumulatorIn();
    if (!listType.isList()) {
        const QQmlJSScope::ConstPtr jsValue = m_typeResolver->jsValueType();
        addReadAccumulator(jsValue);

        QQmlJSMetaProperty prop;
        prop.setPropertyName(u"<>"_s);
        prop.setTypeName(jsValue->internalName());
        prop.setType(jsValue);
        setAccumulator(m_pool->createProperty(
                prop, currentInstructionOffset(),
                QQmlJSRegisterContent::InvalidLookupIndex, QQmlJSRegisterContent::ListIterator,
                listType));
        return;
    }

    addReadAccumulator();
    setAccumulator(m_typeResolver->iteratorPointer(
            listType, QQmlJS::AST::ForEachType(iterator), currentInstructionOffset()));
}

void QQmlJSTypePropagator::generate_IteratorNext(int value, int offset)
{
    const QQmlJSRegisterContent iteratorType = m_state.accumulatorIn();
    addReadAccumulator();
    setRegister(value, m_typeResolver->merge(
                                m_typeResolver->valueType(iteratorType),
                                m_typeResolver->literalType(m_typeResolver->voidType())));
    saveRegisterStateForJump(offset);
    m_state.setHasSideEffects(true);
}

void QQmlJSTypePropagator::generate_IteratorNextForYieldStar(int iterator, int object, int offset)
{
    Q_UNUSED(iterator)
    Q_UNUSED(object)
    Q_UNUSED(offset)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_IteratorClose()
{
    // Noop
}

void QQmlJSTypePropagator::generate_DestructureRestElement()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_DeleteProperty(int base, int index)
{
    Q_UNUSED(base)
    Q_UNUSED(index)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_DeleteName(int name)
{
    Q_UNUSED(name)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_TypeofName(int name)
{
    Q_UNUSED(name);
    setAccumulator(m_typeResolver->operationType(m_typeResolver->stringType()));
}

void QQmlJSTypePropagator::generate_TypeofValue()
{
    setAccumulator(m_typeResolver->operationType(m_typeResolver->stringType()));
}

void QQmlJSTypePropagator::generate_DeclareVar(int varName, int isDeletable)
{
    Q_UNUSED(varName)
    Q_UNUSED(isDeletable)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_DefineArray(int argc, int args)
{
    setAccumulator(m_typeResolver->operationType(m_typeResolver->variantListType()));

    // Track all arguments as the same type.
    const QQmlJSScope::ConstPtr elementType = m_typeResolver->varType();
    for (int i = 0; i < argc; ++i)
        addReadRegister(args + i, elementType);
}

void QQmlJSTypePropagator::generate_DefineObjectLiteral(int internalClassId, int argc, int args)
{
    const int classSize = m_jsUnitGenerator->jsClassSize(internalClassId);
    Q_ASSERT(argc >= classSize);

    // Track each element as separate type
    for (int i = 0; i < classSize; ++i)
        addReadRegister(args + i, m_typeResolver->varType());

    for (int i = classSize; i < argc; i += 3) {
        // layout for remaining members is:
        // 0: ObjectLiteralArgument - Value|Method|Getter|Setter
        // We cannot do anything useful with this. Any code that would call a getter/setter/method
        // could not be compiled to C++. Ignore it.

        // 1: name of argument
        addReadRegister(args + i + 1, m_typeResolver->stringType());

        // 2: value of argument
        addReadRegister(args + i + 2, m_typeResolver->varType());
    }

    setAccumulator(m_typeResolver->operationType(m_typeResolver->variantMapType()));
}

void QQmlJSTypePropagator::generate_CreateClass(int classIndex, int heritage, int computedNames)
{
    Q_UNUSED(classIndex)
    Q_UNUSED(heritage)
    Q_UNUSED(computedNames)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_CreateMappedArgumentsObject()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_CreateUnmappedArgumentsObject()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_CreateRestParameter(int argIndex)
{
    Q_UNUSED(argIndex)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_ConvertThisToObject()
{
    setRegister(This, m_pool->clone(m_function->qmlScope));
}

void QQmlJSTypePropagator::generate_LoadSuperConstructor()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_ToObject()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_Jump(int offset)
{
    saveRegisterStateForJump(offset);
    m_state.skipInstructionsUntilNextJumpTarget = true;
    m_state.setHasSideEffects(true);
}

void QQmlJSTypePropagator::generate_JumpTrue(int offset)
{
    if (!canConvertFromTo(m_state.accumulatorIn(), m_typeResolver->boolType())) {
        addError(u"cannot convert from %1 to boolean"_s
                         .arg(m_state.accumulatorIn().descriptiveName()));
        return;
    }
    saveRegisterStateForJump(offset);
    addReadAccumulator(m_typeResolver->boolType());
    m_state.setHasSideEffects(true);
}

void QQmlJSTypePropagator::generate_JumpFalse(int offset)
{
    if (!canConvertFromTo(m_state.accumulatorIn(), m_typeResolver->boolType())) {
        addError(u"cannot convert from %1 to boolean"_s
                         .arg(m_state.accumulatorIn().descriptiveName()));
        return;
    }
    saveRegisterStateForJump(offset);
    addReadAccumulator(m_typeResolver->boolType());
    m_state.setHasSideEffects(true);
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
    if (m_state.accumulatorIn().contains(m_typeResolver->nullType())
            || m_state.accumulatorIn().containedType()->isReferenceType()) {
        addReadAccumulator();
    } else {
        addReadAccumulator(m_typeResolver->jsPrimitiveType());
    }
}
void QQmlJSTypePropagator::recordEqualsIntType()
{
    // We have specializations for numeric types and bool.
    const QQmlJSScope::ConstPtr in = m_state.accumulatorIn().containedType();
    if (m_state.accumulatorIn().contains(m_typeResolver->boolType())
            || m_typeResolver->isNumeric(m_state.accumulatorIn())) {
        addReadAccumulator();
    } else {
        addReadAccumulator(m_typeResolver->jsPrimitiveType());
    }
}
void QQmlJSTypePropagator::recordEqualsType(int lhs)
{
    const auto isNumericOrEnum = [this](QQmlJSRegisterContent content) {
        return content.isEnumeration() || m_typeResolver->isNumeric(content);
    };

    const auto accumulatorIn = m_state.accumulatorIn();
    const auto lhsRegister = m_state.registers[lhs].content;

    // If the types are primitive, we compare directly ...
    if (m_typeResolver->isPrimitive(accumulatorIn) || accumulatorIn.isEnumeration()) {
        if (accumulatorIn.contains(lhsRegister.containedType())
                || (isNumericOrEnum(accumulatorIn) && isNumericOrEnum(lhsRegister))
                || m_typeResolver->isPrimitive(lhsRegister)) {
            addReadRegister(lhs);
            addReadAccumulator();
            return;
        }
    }

    const auto containedAccumulatorIn = m_typeResolver->isOptionalType(accumulatorIn)
            ? m_typeResolver->extractNonVoidFromOptionalType(accumulatorIn).containedType()
            : accumulatorIn.containedType();

    const auto containedLhs = m_typeResolver->isOptionalType(lhsRegister)
            ? m_typeResolver->extractNonVoidFromOptionalType(lhsRegister).containedType()
            : lhsRegister.containedType();

    // We don't modify types if the types are comparable with QObject, QUrl or var types
    if (canStrictlyCompareWithVar(m_typeResolver, containedLhs, containedAccumulatorIn)
        || canCompareWithQObject(m_typeResolver, containedLhs, containedAccumulatorIn)
        || canCompareWithQUrl(m_typeResolver, containedLhs, containedAccumulatorIn)) {
        addReadRegister(lhs);
        addReadAccumulator();
        return;
    }

    // Otherwise they're both casted to QJSValue.
    // TODO: We can add more specializations here: object/null etc

    const QQmlJSScope::ConstPtr jsval = m_typeResolver->jsValueType();
    addReadRegister(lhs, jsval);
    addReadAccumulator(jsval);
}

void QQmlJSTypePropagator::recordCompareType(int lhs)
{
    // TODO: Revisit this. Does it make any sense to do a comparison on something non-numeric?
    //       Does it pay off to record the exact number type to use?

    const QQmlJSRegisterContent lhsContent = m_state.registers[lhs].content;
    const QQmlJSRegisterContent rhsContent = m_state.accumulatorIn();
    if (lhsContent == rhsContent) {
        // Do not re-track in this case. We want any manipulations on the original types to persist.
        // TODO: Why? Can we just use double and be done with it?
        addReadRegister(lhs, lhsContent);
        addReadAccumulator(lhsContent);
    } else if (m_typeResolver->isNumeric(lhsContent) && m_typeResolver->isNumeric(rhsContent)) {
        // If they're both numeric, we can compare them directly.
        // They may be casted to double, though.
        const QQmlJSRegisterContent merged = m_typeResolver->merge(lhsContent, rhsContent);
        addReadRegister(lhs, merged);
        addReadAccumulator(merged);
    } else {
        const QQmlJSScope::ConstPtr primitive = m_typeResolver->jsPrimitiveType();
        addReadRegister(lhs, primitive);
        addReadAccumulator(primitive);
    }
}

void QQmlJSTypePropagator::generate_CmpEqNull()
{
    recordEqualsNullType();
    setAccumulator(m_typeResolver->operationType(m_typeResolver->boolType()));
}

void QQmlJSTypePropagator::generate_CmpNeNull()
{
    recordEqualsNullType();
    setAccumulator(m_typeResolver->operationType(m_typeResolver->boolType()));
}

void QQmlJSTypePropagator::generate_CmpEqInt(int lhsConst)
{
    recordEqualsIntType();
    Q_UNUSED(lhsConst)
    setAccumulator(m_typeResolver->typeForBinaryOperation(
            QSOperator::Op::Equal, m_typeResolver->literalType(m_typeResolver->int32Type()),
            m_state.accumulatorIn()));
}

void QQmlJSTypePropagator::generate_CmpNeInt(int lhsConst)
{
    recordEqualsIntType();
    Q_UNUSED(lhsConst)
    setAccumulator(m_typeResolver->typeForBinaryOperation(
            QSOperator::Op::NotEqual, m_typeResolver->literalType(m_typeResolver->int32Type()),
            m_state.accumulatorIn()));
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

    addReadRegister(lhs, m_typeResolver->jsValueType());
    addReadAccumulator(m_typeResolver->jsValueType());

    propagateBinaryOperation(QSOperator::Op::In, lhs);
}

void QQmlJSTypePropagator::generate_CmpInstanceOf(int lhs)
{
    Q_UNUSED(lhs)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

void QQmlJSTypePropagator::generate_As(int lhs)
{
    const QQmlJSRegisterContent input = checkedInputRegister(lhs);
    const QQmlJSScope::ConstPtr inContained = input.containedType();

    QQmlJSRegisterContent output;

    const QQmlJSRegisterContent accumulatorIn = m_state.accumulatorIn();
    switch (accumulatorIn.variant()) {
    case QQmlJSRegisterContent::Attachment:
        output = accumulatorIn.scope();
        break;
    case QQmlJSRegisterContent::MetaType:
        output = accumulatorIn.scope();
        if (output.containedType()->isComposite()) // Otherwise we don't need it
            addReadAccumulator(m_typeResolver->metaObjectType());
        break;
    default:
        output = accumulatorIn;
        break;
    }

    QQmlJSScope::ConstPtr outContained = output.containedType();

    if (outContained->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
        // A referece type cast can result in either the type or null.
        // Reference types can hold null. We don't need to special case that.

        if (m_typeResolver->inherits(inContained, outContained))
            output = m_pool->clone(input);
        else
            output = m_pool->castTo(input, outContained);
    } else if (m_typeResolver->inherits(inContained, outContained)) {
        // A "slicing" cannot result in void
        output = m_pool->castTo(input, outContained);
    } else {
        // A value type cast can result in either the type or undefined.
        // Using convert() retains the variant of the input type.
        output = m_typeResolver->merge(
                m_pool->castTo(input, outContained),
                m_pool->castTo(input, m_typeResolver->voidType()));
    }

    addReadRegister(lhs);
    setAccumulator(output);
}

void QQmlJSTypePropagator::checkConversion(
        QQmlJSRegisterContent from, QQmlJSRegisterContent to)
{
    if (!canConvertFromTo(from, to)) {
        addError(u"cannot convert from %1 to %2"_s
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
                m_typeResolver->literalType(m_typeResolver->int32Type()));

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
    setAccumulator(m_typeResolver->literalType(m_typeResolver->emptyType()));
    for (int reg = firstReg, end = firstReg + count; reg < end; ++reg)
        setRegister(reg, m_typeResolver->literalType(m_typeResolver->emptyType()));
}

void QQmlJSTypePropagator::generate_ThrowOnNullOrUndefined()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_GetTemplateObject(int index)
{
    Q_UNUSED(index)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_POPULATES_ACC();
}

QV4::Moth::ByteCodeHandler::Verdict
QQmlJSTypePropagator::startInstruction(QV4::Moth::Instr::Type type)
{
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
                addError(u"When reached from offset %1, %2 is undefined"_s
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

bool QQmlJSTypePropagator::populatesAccumulator(QV4::Moth::Instr::Type instr) const
{
    switch (instr) {
    case QV4::Moth::Instr::Type::CheckException:
    case QV4::Moth::Instr::Type::CloneBlockContext:
    case QV4::Moth::Instr::Type::ConvertThisToObject:
    case QV4::Moth::Instr::Type::CreateCallContext:
    case QV4::Moth::Instr::Type::DeadTemporalZoneCheck:
    case QV4::Moth::Instr::Type::Debug:
    case QV4::Moth::Instr::Type::DeclareVar:
    case QV4::Moth::Instr::Type::IteratorClose:
    case QV4::Moth::Instr::Type::IteratorNext:
    case QV4::Moth::Instr::Type::IteratorNextForYieldStar:
    case QV4::Moth::Instr::Type::Jump:
    case QV4::Moth::Instr::Type::JumpFalse:
    case QV4::Moth::Instr::Type::JumpNoException:
    case QV4::Moth::Instr::Type::JumpNotUndefined:
    case QV4::Moth::Instr::Type::JumpTrue:
    case QV4::Moth::Instr::Type::MoveConst:
    case QV4::Moth::Instr::Type::MoveReg:
    case QV4::Moth::Instr::Type::MoveRegExp:
    case QV4::Moth::Instr::Type::PopContext:
    case QV4::Moth::Instr::Type::PushBlockContext:
    case QV4::Moth::Instr::Type::PushCatchContext:
    case QV4::Moth::Instr::Type::PushScriptContext:
    case QV4::Moth::Instr::Type::Resume:
    case QV4::Moth::Instr::Type::Ret:
    case QV4::Moth::Instr::Type::SetException:
    case QV4::Moth::Instr::Type::SetLookup:
    case QV4::Moth::Instr::Type::SetUnwindHandler:
    case QV4::Moth::Instr::Type::StoreElement:
    case QV4::Moth::Instr::Type::StoreLocal:
    case QV4::Moth::Instr::Type::StoreNameSloppy:
    case QV4::Moth::Instr::Type::StoreNameStrict:
    case QV4::Moth::Instr::Type::StoreProperty:
    case QV4::Moth::Instr::Type::StoreReg:
    case QV4::Moth::Instr::Type::StoreScopedLocal:
    case QV4::Moth::Instr::Type::StoreSuperProperty:
    case QV4::Moth::Instr::Type::ThrowException:
    case QV4::Moth::Instr::Type::ThrowOnNullOrUndefined:
    case QV4::Moth::Instr::Type::UnwindDispatch:
    case QV4::Moth::Instr::Type::UnwindToLabel:
    case QV4::Moth::Instr::Type::Yield:
    case QV4::Moth::Instr::Type::YieldStar:
        return false;
    case QV4::Moth::Instr::Type::Add:
    case QV4::Moth::Instr::Type::As:
    case QV4::Moth::Instr::Type::BitAnd:
    case QV4::Moth::Instr::Type::BitAndConst:
    case QV4::Moth::Instr::Type::BitOr:
    case QV4::Moth::Instr::Type::BitOrConst:
    case QV4::Moth::Instr::Type::BitXor:
    case QV4::Moth::Instr::Type::BitXorConst:
    case QV4::Moth::Instr::Type::CallGlobalLookup:
    case QV4::Moth::Instr::Type::CallName:
    case QV4::Moth::Instr::Type::CallPossiblyDirectEval:
    case QV4::Moth::Instr::Type::CallProperty:
    case QV4::Moth::Instr::Type::CallPropertyLookup:
    case QV4::Moth::Instr::Type::CallQmlContextPropertyLookup:
    case QV4::Moth::Instr::Type::CallValue:
    case QV4::Moth::Instr::Type::CallWithReceiver:
    case QV4::Moth::Instr::Type::CallWithSpread:
    case QV4::Moth::Instr::Type::CmpEq:
    case QV4::Moth::Instr::Type::CmpEqInt:
    case QV4::Moth::Instr::Type::CmpEqNull:
    case QV4::Moth::Instr::Type::CmpGe:
    case QV4::Moth::Instr::Type::CmpGt:
    case QV4::Moth::Instr::Type::CmpIn:
    case QV4::Moth::Instr::Type::CmpInstanceOf:
    case QV4::Moth::Instr::Type::CmpLe:
    case QV4::Moth::Instr::Type::CmpLt:
    case QV4::Moth::Instr::Type::CmpNe:
    case QV4::Moth::Instr::Type::CmpNeInt:
    case QV4::Moth::Instr::Type::CmpNeNull:
    case QV4::Moth::Instr::Type::CmpStrictEqual:
    case QV4::Moth::Instr::Type::CmpStrictNotEqual:
    case QV4::Moth::Instr::Type::Construct:
    case QV4::Moth::Instr::Type::ConstructWithSpread:
    case QV4::Moth::Instr::Type::CreateClass:
    case QV4::Moth::Instr::Type::CreateMappedArgumentsObject:
    case QV4::Moth::Instr::Type::CreateRestParameter:
    case QV4::Moth::Instr::Type::CreateUnmappedArgumentsObject:
    case QV4::Moth::Instr::Type::Decrement:
    case QV4::Moth::Instr::Type::DefineArray:
    case QV4::Moth::Instr::Type::DefineObjectLiteral:
    case QV4::Moth::Instr::Type::DeleteName:
    case QV4::Moth::Instr::Type::DeleteProperty:
    case QV4::Moth::Instr::Type::DestructureRestElement:
    case QV4::Moth::Instr::Type::Div:
    case QV4::Moth::Instr::Type::Exp:
    case QV4::Moth::Instr::Type::GetException:
    case QV4::Moth::Instr::Type::GetIterator:
    case QV4::Moth::Instr::Type::GetLookup:
    case QV4::Moth::Instr::Type::GetOptionalLookup:
    case QV4::Moth::Instr::Type::GetTemplateObject:
    case QV4::Moth::Instr::Type::Increment:
    case QV4::Moth::Instr::Type::InitializeBlockDeadTemporalZone:
    case QV4::Moth::Instr::Type::LoadClosure:
    case QV4::Moth::Instr::Type::LoadConst:
    case QV4::Moth::Instr::Type::LoadElement:
    case QV4::Moth::Instr::Type::LoadFalse:
    case QV4::Moth::Instr::Type::LoadGlobalLookup:
    case QV4::Moth::Instr::Type::LoadImport:
    case QV4::Moth::Instr::Type::LoadInt:
    case QV4::Moth::Instr::Type::LoadLocal:
    case QV4::Moth::Instr::Type::LoadName:
    case QV4::Moth::Instr::Type::LoadNull:
    case QV4::Moth::Instr::Type::LoadOptionalProperty:
    case QV4::Moth::Instr::Type::LoadProperty:
    case QV4::Moth::Instr::Type::LoadQmlContextPropertyLookup:
    case QV4::Moth::Instr::Type::LoadReg:
    case QV4::Moth::Instr::Type::LoadRuntimeString:
    case QV4::Moth::Instr::Type::LoadScopedLocal:
    case QV4::Moth::Instr::Type::LoadSuperConstructor:
    case QV4::Moth::Instr::Type::LoadSuperProperty:
    case QV4::Moth::Instr::Type::LoadTrue:
    case QV4::Moth::Instr::Type::LoadUndefined:
    case QV4::Moth::Instr::Type::LoadZero:
    case QV4::Moth::Instr::Type::Mod:
    case QV4::Moth::Instr::Type::Mul:
    case QV4::Moth::Instr::Type::PushWithContext:
    case QV4::Moth::Instr::Type::Shl:
    case QV4::Moth::Instr::Type::ShlConst:
    case QV4::Moth::Instr::Type::Shr:
    case QV4::Moth::Instr::Type::ShrConst:
    case QV4::Moth::Instr::Type::Sub:
    case QV4::Moth::Instr::Type::TailCall:
    case QV4::Moth::Instr::Type::ToObject:
    case QV4::Moth::Instr::Type::TypeofName:
    case QV4::Moth::Instr::Type::TypeofValue:
    case QV4::Moth::Instr::Type::UCompl:
    case QV4::Moth::Instr::Type::UMinus:
    case QV4::Moth::Instr::Type::UNot:
    case QV4::Moth::Instr::Type::UPlus:
    case QV4::Moth::Instr::Type::UShr:
    case QV4::Moth::Instr::Type::UShrConst:
        return true;
    default:
        Q_UNREACHABLE_RETURN(false);
    }
}

bool QQmlJSTypePropagator::isNoop(QV4::Moth::Instr::Type instr) const
{
    switch (instr) {
    case QV4::Moth::Instr::Type::DeadTemporalZoneCheck:
    case QV4::Moth::Instr::Type::IteratorClose:
        return true;
    default:
        return false;
    }
}

void QQmlJSTypePropagator::endInstruction(QV4::Moth::Instr::Type instr)
{
    InstructionAnnotation &currentInstruction = m_state.annotations[currentInstructionOffset()];
    currentInstruction.changedRegister = m_state.changedRegister();
    currentInstruction.changedRegisterIndex = m_state.changedRegisterIndex();
    currentInstruction.readRegisters = m_state.takeReadRegisters();
    currentInstruction.hasSideEffects = m_state.hasSideEffects();
    currentInstruction.isRename = m_state.isRename();

    bool populates = populatesAccumulator(instr);
    int changedIndex = m_state.changedRegisterIndex();

    // TODO: Find a way to deal with instructions that change multiple registers
    if (instr != QV4::Moth::Instr::Type::InitializeBlockDeadTemporalZone) {
        Q_ASSERT((populates && changedIndex == Accumulator && m_state.accumulatorOut().isValid())
                 || (!populates && changedIndex != Accumulator));
    }

    const auto noError = std::none_of(m_errors->cbegin(), m_errors->cend(),
                                      [](const auto &e) { return e.isError(); });
    if (noError && !isNoop(instr)) {
        // An instruction needs to have side effects or write to another register or be a known
        // noop. Anything else is a problem.
        Q_ASSERT(m_state.hasSideEffects() || changedIndex != InvalidRegister);
    }

    if (changedIndex != InvalidRegister) {
        Q_ASSERT(!m_errors->isEmpty() || m_state.changedRegister().isValid());
        VirtualRegister &r = m_state.registers[changedIndex];
        r.content = m_state.changedRegister();
        r.canMove = false;
        r.affectedBySideEffects = m_state.isRename()
                && m_state.isRegisterAffectedBySideEffects(m_state.renameSourceRegisterIndex());
        m_state.clearChangedRegister();
    }

    m_state.setHasSideEffects(false);
    m_state.setIsRename(false);
    m_state.setReadRegisters(VirtualRegisters());
    m_state.instructionHasError = false;
}

QQmlJSRegisterContent QQmlJSTypePropagator::propagateBinaryOperation(QSOperator::Op op, int lhs)
{
    auto lhsRegister = checkedInputRegister(lhs);
    if (!lhsRegister.isValid())
        return QQmlJSRegisterContent();

    const QQmlJSRegisterContent type = m_typeResolver->typeForBinaryOperation(
                op, lhsRegister, m_state.accumulatorIn());

    setAccumulator(type);
    return type;
}

static bool deepCompare(const QQmlJSRegisterContent &a, const QQmlJSRegisterContent &b)
{
    if (!a.isValid() && !b.isValid())
        return true;

    return a.containedType() == b.containedType()
            && a.variant() == b.variant()
            && deepCompare(a.scope(), b.scope());
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
            if (it->registers.keys() != state.registers.keys())
                continue;

            const auto valuesIt = it->registers.values();
            const auto valuesState = state.registers.values();

            bool different = false;
            for (qsizetype i = 0, end = valuesIt.size(); i != end; ++i) {
                const auto &valueIt = valuesIt[i];
                const auto &valueState = valuesState[i];
                if (valueIt.affectedBySideEffects != valueState.affectedBySideEffects
                        || valueIt.canMove != valueState.canMove
                        || valueIt.isShadowable != valueState.isShadowable
                        || !deepCompare(valueIt.content, valueState.content)) {
                    different = true;
                    break;
                }
            }

            if (!different)
                return; // We've seen the same register state before. No need for merging.
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
        if (reg == This)
            return m_function->qmlScope;
        // over-approximation: needed in qmllint to not crash on `eval()`-calls
        if (reg == NewTarget)
            return m_typeResolver->syntheticType(m_typeResolver->varType());

        addError(u"Type error: could not infer the type of an expression"_s);
        return {};
    }
    return regIt.value().content;
}

bool QQmlJSTypePropagator::canConvertFromTo(
        QQmlJSRegisterContent from, QQmlJSRegisterContent to)
{
    return m_typeResolver->canConvertFromTo(from, to);
}

bool QQmlJSTypePropagator::canConvertFromTo(
        QQmlJSRegisterContent from, const QQmlJSScope::ConstPtr &to)
{
    return m_typeResolver->canConvertFromTo(from.containedType(), to);
}

QT_END_NAMESPACE
