// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljstypepropagator_p.h"

#include "qqmljsutils_p.h"
#include "qqmlsa_p.h"

#include <private/qv4compilerscanfunctions_p.h>

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
                                           QQmlJSLogger *logger, QQmlJSTypeInfo *typeInfo,
                                           QQmlSA::PassManager *passManager)
    : QQmlJSCompilePass(unitGenerator, typeResolver, logger),
      m_typeInfo(typeInfo),
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
        decode(m_function->code.constData(), static_cast<uint>(m_function->code.length()));

        // If we have found unresolved backwards jumps, we need to start over with a fresh state.
        // Mind that m_jumpOriginRegisterStateByTargetInstructionOffset is retained in that case.
        // This means that we won't start over for the same reason again.
    } while (m_state.needsMorePasses);

    return m_state.annotations;
}

#define INSTR_PROLOGUE_NOT_IMPLEMENTED()                                                           \
    setError(u"Instruction \"%1\" not implemented"_s                                              \
                     .arg(QString::fromUtf8(__func__)));                                           \
    return;

#define INSTR_PROLOGUE_NOT_IMPLEMENTED_IGNORE()                                                    \
    m_logger->log(u"Instruction \"%1\" not implemented"_s.arg(QString::fromUtf8(__func__)),       \
                  Log_Compiler, QQmlJS::SourceLocation());                                         \
    return;

void QQmlJSTypePropagator::generate_Ret()
{
    if (m_passManager != nullptr && m_function->isProperty) {
        m_passManager->analyzeBinding(m_function->qmlScope,
                                      m_typeResolver->containedType(m_state.accumulatorIn()),
                                      getCurrentBindingSourceLocation());
    }

    if (m_function->isSignalHandler) {
        // Signal handlers cannot return anything.
    } else if (!m_returnType.isValid() && m_state.accumulatorIn().isValid()
               && !m_typeResolver->registerContains(
                   m_state.accumulatorIn(), m_typeResolver->voidType())) {
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
                      Log_Type, getCurrentBindingSourceLocation());
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
    setAccumulator(m_typeResolver->globalType(m_typeResolver->intType()));
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
    setAccumulator(m_typeResolver->globalType(m_typeResolver->intType()));
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
    } else if (isMissingPropertyType(m_function->qmlScope, name)) {
        return;
    }

    std::optional<FixSuggestion> suggestion;

    auto childScopes = m_function->qmlScope->childScopes();
    for (qsizetype i = 0; i < m_function->qmlScope->childScopes().length(); i++) {
        auto &scope = childScopes[i];
        if (location.offset > scope->sourceLocation().offset) {
            if (i + 1 < childScopes.length()
                && childScopes.at(i + 1)->sourceLocation().offset < location.offset)
                continue;
            if (scope->childScopes().length() == 0)
                continue;

            const auto jsId = scope->childScopes().first()->findJSIdentifier(name);

            if (jsId.has_value() && jsId->kind == QQmlJSScope::JavaScriptIdentifier::Injected) {

                suggestion = FixSuggestion {};

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

                suggestion->fixes << FixSuggestion::Fix {
                    name
                            + QString::fromLatin1(" is accessible in this scope because "
                                                  "you are handling a signal at %1:%2. Use a "
                                                  "function instead.\n")
                                      .arg(id.location.startLine)
                                      .arg(id.location.startColumn),
                    fixLocation, fixString, QString(), false
                };
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
                    suggestion = FixSuggestion {};

                    suggestion->fixes << FixSuggestion::Fix {
                        name + u" is implicitly injected into this delegate. Add a required property instead."_s,
                        m_function->qmlScope->sourceLocation(), QString(), QString(), true
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
                const QString id = m_function->addressableScopes.id(scope);

                suggestion = FixSuggestion {};

                QQmlJS::SourceLocation fixLocation = location;
                fixLocation.length = 0;
                suggestion->fixes << FixSuggestion::Fix {
                    name + QLatin1String(" is a member of a parent element\n")
                            + QLatin1String("      You can qualify the access with its id "
                                            "to avoid this warning:\n"),
                    fixLocation, (id.isEmpty() ? u"<id>."_s : (id + u'.')), QString(), id.isEmpty()
                };

                if (id.isEmpty()) {
                    suggestion->fixes << FixSuggestion::Fix {
                        u"You first have to give the element an id"_s, QQmlJS::SourceLocation {}, {}
                    };
                }
            }
        }
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

    m_logger->log(QLatin1String("Unqualified access"), Log_UnqualifiedAccess, location, true, true,
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

    m_logger->log(message, Log_Deprecation, getCurrentSourceLocation());
}

bool QQmlJSTypePropagator::isRestricted(const QString &propertyName) const
{
    QString restrictedKind;

    const auto accumulatorIn = m_state.registers.find(Accumulator);
    if (accumulatorIn == m_state.registers.end())
        return false;

    if (accumulatorIn.value().isList() && propertyName != u"length") {
        restrictedKind = u"a list"_s;
    } else if (accumulatorIn.value().isEnumeration()
               && !accumulatorIn.value().enumeration().hasKey(propertyName)) {
        restrictedKind = u"an enum"_s;
    } else if (accumulatorIn.value().isMethod()) {
        restrictedKind = u"a method"_s;
    }

    if (!restrictedKind.isEmpty())
        m_logger->log(u"Type is %1. You cannot access \"%2\" from here."_s.arg(restrictedKind,
                                                                                propertyName),
                      Log_Type, getCurrentSourceLocation());

    return !restrictedKind.isEmpty();
}

// Only to be called once a lookup has already failed
bool QQmlJSTypePropagator::isMissingPropertyType(QQmlJSScope::ConstPtr scope,
                                                 const QString &propertyName) const
{
    auto property = scope->property(propertyName);
    if (!property.isValid())
        return false;

    QString errorType;
    if (property.type().isNull())
        errorType = u"found"_s;
    else if (!property.type()->isFullyResolved())
        errorType = u"fully resolved"_s;

    Q_ASSERT(!errorType.isEmpty());

    m_logger->log(
            u"Type \"%1\" of property \"%2\" not %3. This is likely due to a missing dependency entry or a type not being exposed declaratively."_s
                    .arg(property.typeName(), propertyName, errorType),
            Log_Type, getCurrentSourceLocation());

    return true;
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
        case QQmlJSMetaMethod::Signal:
            propertyType = u"Signal"_s;
            break;
        case QQmlJSMetaMethod::Slot:
            propertyType = u"Slot"_s;
            break;
        case QQmlJSMetaMethod::Method:
            propertyType = u"Method"_s;
            break;
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

    m_logger->log(u"%1 \"%2\" is %3"_s.arg(propertyType, name, errorType), Log_Type,
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
    } else if (m_typeResolver->genericType(m_state.accumulatorOut().storedType()).isNull()) {
        // It should really be valid.
        // We get the generic type from aotContext->loadQmlContextPropertyIdLookup().
        setError(u"Cannot determine generic type for "_s + name);
    } else if (m_passManager != nullptr) {
        m_passManager->analyzeRead(m_function->qmlScope, name, m_function->qmlScope,
                                   getCurrentSourceLocation());
    }
}

void QQmlJSTypePropagator::generate_StoreNameSloppy(int nameIndex)
{
    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);
    const QQmlJSRegisterContent type = m_typeResolver->scopedType(m_function->qmlScope, name);
    const QQmlJSRegisterContent in = m_state.accumulatorIn();

    if (!type.isValid()) {
        setError(u"Cannot find name "_s + name);
        return;
    }

    if (!type.isProperty()) {
        setError(u"Cannot assign to non-property "_s + name);
        return;
    }

    if (!type.isWritable() && !m_function->qmlScope->hasOwnProperty(name)) {
        setError(u"Can't assign to read-only property %1"_s.arg(name));

        m_logger->log(u"Cannot assign to read-only property %1"_s.arg(name), Log_Property,
                      getCurrentSourceLocation());

        return;
    }

    if (!canConvertFromTo(in, type)) {
        setError(u"cannot convert from %1 to %2"_s
                         .arg(in.descriptiveName(), type.descriptiveName()));
    }

    if (m_passManager != nullptr) {
        m_passManager->analyzeWrite(m_function->qmlScope, name,
                                    m_typeResolver->containedType(in),
                                    m_function->qmlScope, getCurrentSourceLocation());
    }

    m_state.setHasSideEffects(true);
    if (m_typeResolver->canHoldUndefined(in) && !m_typeResolver->canHoldUndefined(type))
        setError(u"Cannot assign potential undefined to %1"_s.arg(type.descriptiveName()));
    else
        addReadAccumulator(type);
}

void QQmlJSTypePropagator::generate_StoreNameStrict(int name)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(name)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_LoadElement(int base)
{
    const QQmlJSRegisterContent baseRegister = m_state.registers[base];

    if ((baseRegister.storedType()->accessSemantics() != QQmlJSScope::AccessSemantics::Sequence
         && !m_typeResolver->registerIsStoredIn(baseRegister, m_typeResolver->stringType()))
            || !m_typeResolver->isNumeric(m_state.accumulatorIn())) {
        const auto jsValue = m_typeResolver->globalType(m_typeResolver->jsValueType());
        addReadAccumulator(jsValue);
        addReadRegister(base, jsValue);
        setAccumulator(jsValue);
        return;
    }

    if (m_typeResolver->registerContains(m_state.accumulatorIn(), m_typeResolver->intType()))
        addReadAccumulator(m_state.accumulatorIn());
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
    const QQmlJSRegisterContent baseRegister = m_state.registers[base];
    const QQmlJSRegisterContent indexRegister = checkedInputRegister(index);

    if (baseRegister.storedType()->accessSemantics() != QQmlJSScope::AccessSemantics::Sequence
            || !m_typeResolver->isNumeric(indexRegister)) {
        const auto jsValue = m_typeResolver->globalType(m_typeResolver->jsValueType());
        addReadAccumulator(jsValue);
        addReadRegister(base, jsValue);
        addReadRegister(index, jsValue);
        return;
    }

    if (m_typeResolver->registerContains(indexRegister, m_typeResolver->intType()))
        addReadRegister(index, indexRegister);
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

    if (m_typeInfo != nullptr
        && m_state.accumulatorIn().variant() == QQmlJSRegisterContent::ScopeAttached) {
        QQmlJSScope::ConstPtr attachedType = m_typeResolver->originalType(
                    m_state.accumulatorIn().scopeType());

        for (QQmlJSScope::ConstPtr scope = m_function->qmlScope->parentScope(); !scope.isNull();
             scope = scope->parentScope()) {
            if (m_typeInfo->usedAttachedTypes.values(scope).contains(attachedType)) {

                // Ignore enum accesses, as these will not cause the attached object to be created
                if (m_state.accumulatorOut().isValid() && m_state.accumulatorOut().isEnumeration())
                    continue;

                const QString id = m_function->addressableScopes.id(scope);

                FixSuggestion suggestion;

                QQmlJS::SourceLocation fixLocation = getCurrentSourceLocation();
                fixLocation.length = 0;

                suggestion.fixes << FixSuggestion::Fix { u"Reference it by id instead:"_s,
                                                         fixLocation,
                                                         id.isEmpty() ? u"<id>."_s : (id + u'.'),
                                                         QString(), id.isEmpty() };

                fixLocation = scope->sourceLocation();
                fixLocation.length = 0;

                if (id.isEmpty()) {
                    suggestion.fixes
                            << FixSuggestion::Fix { u"You first have to give the element an id"_s,
                                                    QQmlJS::SourceLocation {},
                                                    {} };
                }

                m_logger->log(
                        u"Using attached type %1 already initialized in a parent scope."_s.arg(
                                m_state.accumulatorIn().scopeType()->internalName()),
                        Log_AttachedPropertyReuse, getCurrentSourceLocation(), true, true,
                        suggestion);
            }
        }
        m_typeInfo->usedAttachedTypes.insert(m_function->qmlScope, attachedType);
    }

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
            m_logger->log(u"Type not found in namespace"_s, Log_Type, getCurrentSourceLocation());
    } else if (m_state.accumulatorOut().variant() == QQmlJSRegisterContent::Singleton
               && m_state.accumulatorIn().variant() == QQmlJSRegisterContent::ObjectModulePrefix) {
        m_logger->log(u"Cannot load singleton as property of object"_s, Log_Type,
                      getCurrentSourceLocation());
        setAccumulator(QQmlJSRegisterContent());
    }

    const bool isRestrictedProperty = isRestricted(propertyName);

    if (!m_state.accumulatorOut().isValid()) {
        setError(u"Cannot load property %1 from %2."_s
                         .arg(propertyName, m_state.accumulatorIn().descriptiveName()));

        if (isRestrictedProperty)
            return;

        const QString typeName = m_typeResolver->containedTypeName(m_state.accumulatorIn(), true);

        if (typeName == u"QVariant")
            return;
        if (m_state.accumulatorIn().isList() && propertyName == u"length")
            return;

        auto baseType = m_typeResolver->containedType(m_state.accumulatorIn());
        // Warn separately when a property is only not found because of a missing type

        if (isMissingPropertyType(baseType, propertyName))
            return;

        std::optional<FixSuggestion> fixSuggestion;

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

        m_logger->log(
                u"Property \"%1\" not found on type \"%2\""_s.arg(propertyName).arg(typeName),
                Log_Type, getCurrentSourceLocation(), true, true, fixSuggestion);
        return;
    }

    if (m_state.accumulatorOut().isMethod() && m_state.accumulatorOut().method().length() != 1) {
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
                        Log_Type, getCurrentSourceLocation());
        }
    }

    if (m_passManager != nullptr) {
        m_passManager->analyzeRead(m_typeResolver->containedType(m_state.accumulatorIn()),
                                   propertyName, m_function->qmlScope, getCurrentSourceLocation());
    }

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
    auto callBase = m_state.registers[base];
    const QString propertyName = m_jsUnitGenerator->stringForIndex(nameIndex);

    QQmlJSRegisterContent property = m_typeResolver->memberType(callBase, propertyName);
    if (!property.isProperty()) {
        setError(u"Type %1 does not have a property %2 for writing"_s
                         .arg(callBase.descriptiveName(), propertyName));
        return;
    }

    if (!property.isWritable()) {
        setError(u"Can't assign to read-only property %1"_s.arg(propertyName));

        m_logger->log(u"Cannot assign to read-only property %1"_s.arg(propertyName), Log_Property,
                      getCurrentSourceLocation());

        return;
    }

    if (!canConvertFromTo(m_state.accumulatorIn(), property)) {
        setError(u"cannot convert from %1 to %2"_s
                         .arg(m_state.accumulatorIn().descriptiveName(), property.descriptiveName()));
        return;
    }

    if (m_passManager != nullptr) {
        m_passManager->analyzeWrite(m_typeResolver->containedType(callBase), propertyName,
                                    m_typeResolver->containedType(m_state.accumulatorIn()),
                                    m_function->qmlScope, getCurrentSourceLocation());
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

void QQmlJSTypePropagator::generate_CallProperty(int nameIndex, int base, int argc, int argv)
{
    Q_ASSERT(m_state.registers.contains(base));
    const auto callBase = m_state.registers[base];
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

    const auto member = m_typeResolver->memberType(callBase, propertyName);
    if (!member.isMethod()) {
        setError(u"Type %1 does not have a property %2 for calling"_s
                         .arg(callBase.descriptiveName(), propertyName));

        if (callBase.isType() && isCallingProperty(callBase.type(), propertyName))
            return;

        if (isRestricted(propertyName))
            return;

        std::optional<FixSuggestion> fixSuggestion;

        const auto baseType = m_typeResolver->containedType(callBase);

        if (auto suggestion = QQmlJSUtils::didYouMean(propertyName, baseType->methods().keys(),
                                                      getCurrentSourceLocation());
            suggestion.has_value()) {
            fixSuggestion = suggestion;
        }

        m_logger->log(u"Property \"%1\" not found on type \"%2\""_s.arg(
                              propertyName, m_typeResolver->containedTypeName(callBase, true)),
                      Log_Type, getCurrentSourceLocation(), true, true, fixSuggestion);
        return;
    }

    checkDeprecated(m_typeResolver->containedType(callBase), propertyName, true);

    if (m_passManager != nullptr) {
        // TODO: Should there be an analyzeCall() in the future? (w. corresponding onCall in Pass)
        m_passManager->analyzeRead(m_typeResolver->containedType(m_state.accumulatorIn()),
                                   propertyName, m_function->qmlScope, getCurrentSourceLocation());
    }

    addReadRegister(base, callBase);
    propagateCall(member.method(), argc, argv);
}

QQmlJSMetaMethod QQmlJSTypePropagator::bestMatchForCall(const QList<QQmlJSMetaMethod> &methods,
                                                        int argc, int argv, QStringList *errors)
{
    QQmlJSMetaMethod javascriptFunction;
    for (const auto &method : methods) {

        // If we encounter a JavaScript function, use this as a fallback if no other method matches
        if (method.isJavaScriptFunction())
            javascriptFunction = method;

        if (method.returnType().isNull() && !method.returnTypeName().isEmpty()) {
            errors->append(u"return type %1 cannot be resolved"_s
                                   .arg(method.returnTypeName()));
            continue;
        }

        const auto argumentTypes = method.parameterTypes();
        if (argc != argumentTypes.size()) {
            errors->append(u"Function expects %1 arguments, but %2 were provided"_s
                                   .arg(argumentTypes.size())
                                   .arg(argc));
            continue;
        }

        bool matches = true;
        for (int i = 0; i < argc; ++i) {
            const auto argumentType = argumentTypes[i];
            if (argumentType.isNull()) {
                errors->append(u"type %1 for argument %2 cannot be resolved"_s
                                       .arg(method.parameterTypeNames().at(i))
                                       .arg(i));
                matches = false;
                break;
            }

            if (canConvertFromTo(m_state.registers[argv + i],
                                 m_typeResolver->globalType(argumentType))) {
                continue;
            }

            errors->append(
                    u"argument %1 contains %2 but is expected to contain the type %3"_s.arg(i).arg(
                            m_state.registers[argv + i].descriptiveName(),
                            method.parameterTypeNames().at(i)));
            matches = false;
            break;
        }
        if (matches)
            return method;
    }
    return javascriptFunction;
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

        const QQmlJSRegisterContent &lastTry = conversion.value();

        Q_ASSERT(lastTry.isValid());
        Q_ASSERT(lastTry.isConversion());

        if (!m_typeResolver->equals(lastTry.conversionResult(), merged.conversionResult())
                || lastTry.conversionOrigins() != merged.conversionOrigins()) {
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
        m_state.annotations[currentInstructionOffset()].typeConversions[index] = merged;
        m_state.registers[index] = merged;
    }
}

void QQmlJSTypePropagator::addReadRegister(int index, const QQmlJSRegisterContent &convertTo)
{
    m_state.addReadRegister(index, m_typeResolver->convert(m_state.registers[index], convertTo));
}

void QQmlJSTypePropagator::propagateCall(const QList<QQmlJSMetaMethod> &methods, int argc, int argv)
{
    QStringList errors;
    const QQmlJSMetaMethod match = bestMatchForCall(methods, argc, argv, &errors);

    if (!match.isValid()) {
        Q_ASSERT(errors.length() == methods.length());
        if (methods.length() == 1)
            setError(errors.first());
        else
            setError(u"No matching override found. Candidates:\n"_s + errors.join(u'\n'));
        return;
    }

    const auto returnType = match.isJavaScriptFunction()
            ? m_typeResolver->jsValueType()
            : QQmlJSScope::ConstPtr(match.returnType());
    setAccumulator(m_typeResolver->returnType(
                       returnType ? QQmlJSScope::ConstPtr(returnType) : m_typeResolver->voidType(),
                       match.isJavaScriptFunction() ? QQmlJSRegisterContent::JavaScriptReturnValue
                                                    : QQmlJSRegisterContent::MethodReturnValue));
    if (!m_state.accumulatorOut().isValid())
        setError(u"Cannot store return type of method %1()."_s.arg(match.methodName()));

    m_state.setHasSideEffects(true);
    const auto types = match.parameterTypes();
    for (int i = 0; i < argc; ++i) {
        if (i < types.length()) {
            const QQmlJSScope::ConstPtr type = match.isJavaScriptFunction()
                    ? m_typeResolver->jsValueType()
                    : QQmlJSScope::ConstPtr(types.at(i));
            if (!type.isNull()) {
                addReadRegister(argv + i, m_typeResolver->globalType(type));
                continue;
            }
        }
        addReadRegister(argv + i, m_typeResolver->globalType(m_typeResolver->jsValueType()));
    }
}

void QQmlJSTypePropagator::generate_CallPropertyLookup(int lookupIndex, int base, int argc,
                                                       int argv)
{
    generate_CallProperty(m_jsUnitGenerator->lookupNameIndex(lookupIndex), base, argc, argv);
}

void QQmlJSTypePropagator::generate_CallElement(int base, int index, int argc, int argv)
{
    m_state.setHasSideEffects(true);
    Q_UNUSED(base)
    Q_UNUSED(index)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
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
        if (!methods.isEmpty()) {
            propagateCall(methods, argc, argv);
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
    Q_UNUSED(name)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
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
    setAccumulator(m_typeResolver->globalType(argc == 0
                                                      ? m_typeResolver->emptyListType()
                                                      : m_typeResolver->variantListType()));

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
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
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

    const auto isIntCompatible = [this](const QQmlJSRegisterContent &content) {
        const QQmlJSScope::ConstPtr contained = m_typeResolver->containedType(content);
        return contained->scopeType() == QQmlJSScope::EnumScope
                || m_typeResolver->equals(contained, m_typeResolver->intType());
    };

    const auto accumulatorIn = m_state.accumulatorIn();
    const auto lhsRegister = m_state.registers[lhs];

    // If the types are primitive, we compare directly ...
    if (m_typeResolver->isPrimitive(accumulatorIn) || accumulatorIn.isEnumeration()) {
        if (m_typeResolver->registerContains(
                    accumulatorIn, m_typeResolver->containedType(lhsRegister))) {
            addReadRegister(lhs, accumulatorIn);
            addReadAccumulator(accumulatorIn);
            return;
        } else if (isNumericOrEnum(accumulatorIn) && isNumericOrEnum(lhsRegister)) {
            const auto targetType = isIntCompatible(accumulatorIn) && isIntCompatible(lhsRegister)
                    ? m_typeResolver->globalType(m_typeResolver->intType())
                    : m_typeResolver->globalType(m_typeResolver->realType());
            addReadRegister(lhs, targetType);
            addReadAccumulator(targetType);
            return;
        } else if (m_typeResolver->isPrimitive(lhsRegister)) {
            const QQmlJSRegisterContent primitive = m_typeResolver->globalType(
                        m_typeResolver->jsPrimitiveType());
            addReadRegister(lhs, primitive);
            addReadAccumulator(primitive);
        }
    }

    // Otherwise they're both casted to QJSValue.
    // TODO: We can add more specializations here: void/void null/null object/null etc

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
               && m_typeResolver->isNumeric(m_state.registers[lhs]))
                    ? m_typeResolver->merge(m_state.accumulatorIn(), m_state.registers[lhs])
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
            QSOperator::Op::Equal, m_typeResolver->globalType(m_typeResolver->intType()),
            m_state.accumulatorIn())));
}

void QQmlJSTypePropagator::generate_CmpNeInt(int lhsConst)
{
    recordEqualsIntType();
    Q_UNUSED(lhsConst)
    setAccumulator(QQmlJSRegisterContent(m_typeResolver->typeForBinaryOperation(
            QSOperator::Op::NotEqual, m_typeResolver->globalType(m_typeResolver->intType()),
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

    addReadRegister(lhs, m_typeResolver->globalType(contained));

    if (m_typeResolver->containedType(input)->accessSemantics()
                != QQmlJSScope::AccessSemantics::Reference
        || contained->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
        setError(u"invalid cast from %1 to %2. You can only cast object types."_s
                         .arg(input.descriptiveName(), m_state.accumulatorIn().descriptiveName()));
    } else {
        setAccumulator(m_typeResolver->globalType(contained));
    }
}

void QQmlJSTypePropagator::generate_UNot()
{
    if (!canConvertFromTo(m_state.accumulatorIn(),
                          m_typeResolver->globalType(m_typeResolver->boolType()))) {
        setError(u"cannot convert from %1 to boolean"_s
                         .arg(m_state.accumulatorIn().descriptiveName()));
        return;
    }
    const QQmlJSRegisterContent boolType = m_typeResolver->globalType(m_typeResolver->boolType());
    addReadAccumulator(boolType);
    setAccumulator(boolType);
}

void QQmlJSTypePropagator::generate_UPlus()
{
    const QQmlJSRegisterContent type = m_typeResolver->typeForArithmeticUnaryOperation(
                QQmlJSTypeResolver::UnaryOperator::Plus, m_state.accumulatorIn());
    addReadAccumulator(type);
    setAccumulator(type);
}

void QQmlJSTypePropagator::generate_UMinus()
{
    const QQmlJSRegisterContent type = m_typeResolver->typeForArithmeticUnaryOperation(
                QQmlJSTypeResolver::UnaryOperator::Minus, m_state.accumulatorIn());
    addReadAccumulator(type);
    setAccumulator(type);
}

void QQmlJSTypePropagator::generate_UCompl()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_Increment()
{
    const QQmlJSRegisterContent type = m_typeResolver->typeForArithmeticUnaryOperation(
                QQmlJSTypeResolver::UnaryOperator::Increment, m_state.accumulatorIn());
    addReadAccumulator(type);
    setAccumulator(type);
}

void QQmlJSTypePropagator::generate_Decrement()
{
    const QQmlJSRegisterContent type = m_typeResolver->typeForArithmeticUnaryOperation(
                QQmlJSTypeResolver::UnaryOperator::Decrement, m_state.accumulatorIn());
    addReadAccumulator(type);
    setAccumulator(type);
}

void QQmlJSTypePropagator::generate_Add(int lhs)
{
    const auto type = propagateBinaryOperation(QSOperator::Op::Add, lhs);
    addReadRegister(lhs, type);
    addReadAccumulator(type);
}

void QQmlJSTypePropagator::generate_BitAnd(int lhs)
{
    Q_UNUSED(lhs)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_BitOr(int lhs)
{
    Q_UNUSED(lhs)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_BitXor(int lhs)
{
    Q_UNUSED(lhs)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_UShr(int lhs)
{
    Q_UNUSED(lhs)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_Shr(int lhs)
{
    auto lhsRegister = checkedInputRegister(lhs);
    const QQmlJSRegisterContent type = m_typeResolver->typeForBinaryOperation(
                QSOperator::Op::RShift, lhsRegister, m_state.accumulatorIn());
    addReadRegister(lhs, type);
    addReadAccumulator(type);
    setAccumulator(type);
}

void QQmlJSTypePropagator::generate_Shl(int lhs)
{
    auto lhsRegister = checkedInputRegister(lhs);
    const QQmlJSRegisterContent type = m_typeResolver->typeForBinaryOperation(
                QSOperator::Op::LShift, lhsRegister, m_state.accumulatorIn());
    addReadRegister(lhs, type);
    addReadAccumulator(type);
    setAccumulator(type);
}

void QQmlJSTypePropagator::generate_BitAndConst(int rhs)
{
    Q_UNUSED(rhs)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_BitOrConst(int rhs)
{
    Q_UNUSED(rhs)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_BitXorConst(int rhs)
{
    Q_UNUSED(rhs)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_UShrConst(int rhs)
{
    Q_UNUSED(rhs)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_ShrConst(int rhsConst)
{
    Q_UNUSED(rhsConst)

    const QQmlJSRegisterContent type = m_typeResolver->typeForBinaryOperation(
                QSOperator::Op::RShift, m_state.accumulatorIn(),
                m_typeResolver->globalType(m_typeResolver->intType()));
    addReadAccumulator(type);
    setAccumulator(type);
}

void QQmlJSTypePropagator::generate_ShlConst(int rhsConst)
{
    Q_UNUSED(rhsConst)

    const QQmlJSRegisterContent type = m_typeResolver->typeForBinaryOperation(
                QSOperator::Op::LShift, m_state.accumulatorIn(),
                m_typeResolver->globalType(m_typeResolver->intType()));
    addReadAccumulator(type);
    setAccumulator(type);
}

void QQmlJSTypePropagator::generate_Exp(int lhs)
{
    Q_UNUSED(lhs)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_Mul(int lhs)
{
    const auto type = propagateBinaryOperation(QSOperator::Op::Mul, lhs);
    addReadRegister(lhs, type);
    addReadAccumulator(type);
}

void QQmlJSTypePropagator::generate_Div(int lhs)
{
    const auto type = propagateBinaryOperation(QSOperator::Op::Div, lhs);
    addReadRegister(lhs, type);
    addReadAccumulator(type);
}

void QQmlJSTypePropagator::generate_Mod(int lhs)
{
    const auto type = propagateBinaryOperation(QSOperator::Op::Mod, lhs);
    addReadRegister(lhs, type);
    addReadAccumulator(type);
}

void QQmlJSTypePropagator::generate_Sub(int lhs)
{
    const auto type = propagateBinaryOperation(QSOperator::Op::Sub, lhs);
    addReadRegister(lhs, type);
    addReadAccumulator(type);
}

void QQmlJSTypePropagator::generate_InitializeBlockDeadTemporalZone(int firstReg, int count)
{
    Q_UNUSED(firstReg)
    Q_UNUSED(count)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
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

static bool instructionManipulatesContext(QV4::Moth::Instr::Type type)
{
    using Type = QV4::Moth::Instr::Type;
    switch (type) {
    case Type::PopContext:
    case Type::PopScriptContext:
    case Type::CreateCallContext:
    case Type::CreateCallContext_Wide:
    case Type::PushCatchContext:
    case Type::PushCatchContext_Wide:
    case Type::PushWithContext:
    case Type::PushWithContext_Wide:
    case Type::PushBlockContext:
    case Type::PushBlockContext_Wide:
    case Type::CloneBlockContext:
    case Type::CloneBlockContext_Wide:
    case Type::PushScriptContext:
    case Type::PushScriptContext_Wide:
        return true;
    default:
        break;
    }
    return false;
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

            auto newType = registerIt.value();
            if (!newType.isValid()) {
                setError(u"When reached from offset %1, %2 is undefined"_s
                                 .arg(stateToMerge.originatingOffset)
                                 .arg(registerName(registerIndex)));
                return SkipInstruction;
            }

            auto currentRegister = m_state.registers.find(registerIndex);
            if (currentRegister != m_state.registers.end())
                mergeRegister(registerIndex, newType, currentRegister.value());
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
    m_state.setHasSideEffects(false);
    m_state.setIsRename(false);
    m_state.setReadRegisters(VirtualRegisters());

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
        m_state.registers[m_state.changedRegisterIndex()] = m_state.changedRegister();
        m_state.clearChangedRegister();
    }
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
            && registerIndex < FirstArgument + m_function->argumentTypes.count()) {
        return u"argument %1"_s.arg(registerIndex - FirstArgument);
    }

    return u"temporary register %1"_s.arg(
            registerIndex - FirstArgument - m_function->argumentTypes.count());
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
    return regIt.value();
}

bool QQmlJSTypePropagator::canConvertFromTo(const QQmlJSRegisterContent &from,
                                            const QQmlJSRegisterContent &to)
{
    return m_typeResolver->canConvertFromTo(from, to);
}

QT_END_NAMESPACE
