/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qqmljstypepropagator_p.h"
#include <private/qv4compilerscanfunctions_p.h>

QT_BEGIN_NAMESPACE

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
                                           const QQmlJSTypeResolver *typeResolver, QQmlJSLogger *logger,
                                           QQmlJSTypeInfo *typeInfo)
    : QQmlJSCompilePass(unitGenerator, typeResolver, logger), m_typeInfo(typeInfo)
{
}

QQmlJSCompilePass::InstructionAnnotations QQmlJSTypePropagator::run(
        const Function *function, QQmlJS::DiagnosticMessage *error)
{
    m_function = function;
    m_error = error;
    m_returnType = m_typeResolver->globalType(m_function->returnType);

    do {
        m_state = PassState();
        m_state.State::operator=(initialState(m_function, m_typeResolver));

        reset();
        decode(m_function->code.constData(), static_cast<uint>(m_function->code.length()));

        // If we have found unresolved backwards jumps, we need to start over with a fresh state.
        // Mind that m_jumpOriginRegisterStateByTargetInstructionOffset is retained in that case.
        // This means that we won't start over for the same reason again.
    } while (!m_error->isValid() && m_state.needsMorePasses);

    return m_state.annotations;
}

#define INSTR_PROLOGUE_NOT_IMPLEMENTED()                                                           \
    setError(u"Instruction \"%1\" not implemented"_qs                                              \
                     .arg(QString::fromUtf8(__func__)));                                           \
    return;

#define INSTR_PROLOGUE_NOT_IMPLEMENTED_IGNORE()                                                    \
    m_logger->logWarning(                                                                          \
            u"Instruction \"%1\" not implemented"_qs.arg(QString::fromUtf8(__func__)),             \
            Log_Compiler);                                                                         \
    return;

void QQmlJSTypePropagator::generate_Ret()
{
    if (m_function->isSignalHandler) {
        // Signal handlers cannot return anything.
    } else if (!m_returnType.isValid() && m_state.accumulatorIn.isValid()
               && m_typeResolver->containedType(m_state.accumulatorIn)
                       != m_typeResolver->voidType()) {
        setError(u"function without type annotation returns %1"_qs
                         .arg(m_state.accumulatorIn.descriptiveName()));
        return;
    } else if (m_state.accumulatorIn != m_returnType
               && !canConvertFromTo(m_state.accumulatorIn, m_returnType)) {
        setError(u"cannot convert from %1 to %2"_qs
                         .arg(m_state.accumulatorIn.descriptiveName(),
                              m_returnType.descriptiveName()));

        m_logger->logWarning(u"Cannot assign binding of type %1 to %2"_qs.arg(
                                     m_typeResolver->containedTypeName(m_state.accumulatorIn),
                                     m_typeResolver->containedTypeName(m_returnType)),
                             Log_Type, getCurrentBindingSourceLocation());
        return;
    }

    m_state.skipInstructionsUntilNextJumpTarget = true;
}

void QQmlJSTypePropagator::generate_Debug()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_LoadConst(int index)
{
    auto encodedConst = m_jsUnitGenerator->constant(index);
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->typeForConst(encodedConst));
}

void QQmlJSTypePropagator::generate_LoadZero()
{
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->intType());
}

void QQmlJSTypePropagator::generate_LoadTrue()
{
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->boolType());
}

void QQmlJSTypePropagator::generate_LoadFalse()
{
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->boolType());
}

void QQmlJSTypePropagator::generate_LoadNull()
{
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->jsPrimitiveType());
}

void QQmlJSTypePropagator::generate_LoadUndefined()
{
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->voidType());
}

void QQmlJSTypePropagator::generate_LoadInt(int)
{
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->intType());
}

void QQmlJSTypePropagator::generate_MoveConst(int constIndex, int destTemp)
{
    auto encodedConst = m_jsUnitGenerator->constant(constIndex);
    setRegister(destTemp, m_typeResolver->globalType(m_typeResolver->typeForConst(encodedConst)));
}

void QQmlJSTypePropagator::generate_LoadReg(int reg)
{
    m_state.accumulatorOut = checkedInputRegister(reg);
}

void QQmlJSTypePropagator::generate_StoreReg(int reg)
{
    setRegister(reg, m_state.accumulatorIn);
}

void QQmlJSTypePropagator::generate_MoveReg(int srcReg, int destReg)
{
    setRegister(destReg, m_state.registers[srcReg]);
}

void QQmlJSTypePropagator::generate_LoadImport(int index)
{
    Q_UNUSED(index)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_LoadLocal(int index)
{
    Q_UNUSED(index);
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->jsValueType());
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
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->stringType());
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
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_LoadName(int nameIndex)
{
    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);
    m_state.accumulatorOut = m_typeResolver->scopedType(m_function->qmlScope, name);
    if (!m_state.accumulatorOut.isValid())
        setError(u"Cannot find name "_qs + name);
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
            || m_function->qmlScope->baseType()->internalName() != u"QQmlConnections"_qs)
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

                QString fixString = handler.isMultiline ? u" function("_qs : u" ("_qs;
                const auto parameters = handler.signalParameters;
                for (int numParams = parameters.size(); numParams > 0; --numParams) {
                    fixString += parameters.at(parameters.size() - numParams);
                    if (numParams > 1)
                        fixString += u", "_qs;
                }

                fixString += handler.isMultiline ? u") "_qs : u") => "_qs;

                suggestion->fixes << FixSuggestion::Fix {
                    name
                            + QString::fromLatin1(" is accessible in this scope because "
                                                  "you are handling a signal at %1:%2. Use a "
                                                  "function instead.\n")
                                      .arg(id.location.startLine)
                                      .arg(id.location.startColumn),
                    fixLocation, fixString
                };
            }
            break;
        }
    }

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
                fixLocation, (id.isEmpty() ? u"<id>."_qs : (id + u'.'))
            };

            if (id.isEmpty()) {
                suggestion->fixes << FixSuggestion::Fix {
                    u"You first have to give the element an id"_qs, QQmlJS::SourceLocation {}, {}
                };
            }
        }
    }

    m_logger->logWarning(QLatin1String("Unqualified access"), Log_UnqualifiedAccess, location, true,
                         true, suggestion);
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
        descriptor += u'(' + method.parameterNames().join(u", "_qs) + u')';

    QString message = QStringLiteral("%1 \"%2\" is deprecated")
                              .arg(isMethod ? u"Method"_qs : u"Property"_qs)
                              .arg(descriptor);

    if (!deprecation.reason.isEmpty())
        message.append(QStringLiteral(" (Reason: %1)").arg(deprecation.reason));

    m_logger->logWarning(message, Log_Deprecation, getCurrentSourceLocation());
}

bool QQmlJSTypePropagator::checkRestricted(const QString &propertyName) const
{
    QString restrictedKind;

    if (!m_state.accumulatorIn.isValid())
        return false;

    if (m_state.accumulatorIn.isList() && propertyName != u"length") {
        restrictedKind = u"a list"_qs;
    } else if (m_state.accumulatorIn.isEnumeration()
               && !m_state.accumulatorIn.enumeration().hasKey(propertyName)) {
        restrictedKind = u"an enum"_qs;
    } else if (m_state.accumulatorIn.isMethod()) {
        restrictedKind = u"a method"_qs;
    }

    if (!restrictedKind.isEmpty())
        m_logger->logWarning(u"Type is %1. You cannot access \"%2\" from here."_qs.arg(
                                     restrictedKind, propertyName),
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
        errorType = u"found"_qs;
    else if (!property.type()->isFullyResolved())
        errorType = u"fully resolved"_qs;

    Q_ASSERT(!errorType.isEmpty());

    m_logger->logWarning(
            u"Type \"%1\" of property \"%2\" not %3. This is likely due to a missing dependency entry or a type not being exposed declaratively."_qs
                    .arg(property.typeName(), propertyName, errorType),
            Log_Type, getCurrentSourceLocation());

    return true;
}

bool QQmlJSTypePropagator::isCallingProperty(QQmlJSScope::ConstPtr scope, const QString &name) const
{
    auto property = scope->property(name);
    if (!property.isValid())
        return false;

    QString propertyType = u"Property"_qs;

    auto methods = scope->methods(name);

    QString errorType;
    if (!methods.isEmpty()) {
        errorType = u"shadowed by a property."_qs;
        switch (methods.first().methodType()) {
        case QQmlJSMetaMethod::Signal:
            propertyType = u"Signal"_qs;
            break;
        case QQmlJSMetaMethod::Slot:
            propertyType = u"Slot"_qs;
            break;
        case QQmlJSMetaMethod::Method:
            propertyType = u"Method"_qs;
            break;
        }
    } else if (property.type() == m_typeResolver->varType()) {
        errorType =
                u"a variant property. It may or may not be a method. Use a regular function instead."_qs;
    } else if (property.type() == m_typeResolver->jsValueType()) {
        errorType =
                u"a QJSValue property. It may or may not be a method. Use a regular Q_INVOKABLE instead."_qs;
    } else {
        errorType = u"not a method"_qs;
    }

    m_logger->logWarning(u"%1 \"%2\" is %3"_qs.arg(propertyType, name, errorType), Log_Type,
                         getCurrentSourceLocation(), true, true, {});

    return true;
}

void QQmlJSTypePropagator::generate_LoadQmlContextPropertyLookup(int index)
{
    const int nameIndex = m_jsUnitGenerator->lookupNameIndex(index);
    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);

    m_state.accumulatorOut = m_typeResolver->scopedType(
                m_function->qmlScope,
                m_state.accumulatorIn.isImportNamespace()
                    ? m_jsUnitGenerator->stringForIndex(m_state.accumulatorIn.importNamespace())
                      + u'.' + name
                    : name);

    if (!m_state.accumulatorOut.isValid() && m_typeResolver->isPrefix(name)) {
        const QQmlJSRegisterContent inType = m_state.accumulatorIn.isValid()
                ? m_state.accumulatorIn
                : m_typeResolver->globalType(m_function->qmlScope);
        m_state.accumulatorOut = QQmlJSRegisterContent::create(
                    inType.storedType(), nameIndex, QQmlJSRegisterContent::ScopeModulePrefix,
                    m_typeResolver->containedType(inType));
        return;
    }

    checkDeprecated(m_function->qmlScope, name, false);

    bool isRestricted = checkRestricted(name);

    if (!m_state.accumulatorOut.isValid()) {
        setError(u"Cannot access value for name "_qs + name);

        if (!isRestricted)
            handleUnqualifiedAccess(name, false);
    } else if (m_typeResolver->genericType(m_state.accumulatorOut.storedType()).isNull()) {
        // It should really be valid.
        // We get the generic type from aotContext->loadQmlContextPropertyIdLookup().
        setError(u"Cannot determine generic type for "_qs + name);
    }
}

void QQmlJSTypePropagator::generate_StoreNameSloppy(int nameIndex)
{
    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);
    const QQmlJSRegisterContent type = m_typeResolver->scopedType(m_function->qmlScope, name);
    const QQmlJSRegisterContent in = m_state.accumulatorIn;

    if (!type.isValid()) {
        setError(u"Cannot find name "_qs + name);
        return;
    }

    if (!type.isProperty()) {
        setError(u"Cannot assign to non-property "_qs + name);
        return;
    }

    if (!type.isWritable() && !m_function->qmlScope->hasOwnProperty(name)) {
        setError(u"Can't assign to read-only property %1"_qs.arg(name));

        m_logger->logWarning(u"Cannot assign to read-only property %1"_qs.arg(name), Log_Property,
                             getCurrentSourceLocation());

        return;
    }

    if (!canConvertFromTo(in, type)) {
        setError(u"cannot convert from %1 to %2"_qs
                         .arg(in.descriptiveName(), type.descriptiveName()));
    }

    if (m_typeResolver->canHoldUndefined(in) && !m_typeResolver->canHoldUndefined(type))
        setError(u"Cannot assign potential undefined to %1"_qs.arg(type.descriptiveName()));
}

void QQmlJSTypePropagator::generate_StoreNameStrict(int name)
{
    Q_UNUSED(name)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_LoadElement(int base)
{
    const QQmlJSRegisterContent baseRegister = m_state.registers[base];
    if (!m_typeResolver->registerContains(m_state.accumulatorIn, m_typeResolver->intType())
        || baseRegister.storedType()->accessSemantics() != QQmlJSScope::AccessSemantics::Sequence) {
        m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->jsValueType());
        return;
    }

    m_state.accumulatorOut = m_typeResolver->valueType(baseRegister);
}

void QQmlJSTypePropagator::generate_StoreElement(int base, int index)
{
    Q_UNUSED(base)
    Q_UNUSED(index)
}

void QQmlJSTypePropagator::propagatePropertyLookup(const QString &propertyName)
{
    m_state.accumulatorOut =
            m_typeResolver->memberType(
                m_state.accumulatorIn,
                m_state.accumulatorIn.isImportNamespace()
                    ? m_jsUnitGenerator->stringForIndex(m_state.accumulatorIn.importNamespace())
                      + u'.' + propertyName
                    : propertyName);

    if (m_typeInfo != nullptr
        && m_state.accumulatorIn.variant() == QQmlJSRegisterContent::ScopeAttached) {
        QQmlJSScope::ConstPtr attachedType = m_state.accumulatorIn.scopeType();

        for (QQmlJSScope::ConstPtr scope = m_function->qmlScope->parentScope(); !scope.isNull();
             scope = scope->parentScope()) {
            if (m_typeInfo->usedAttachedTypes.values(scope).contains(attachedType)) {

                // Ignore enum accesses, as these will not cause the attached object to be created
                if (m_state.accumulatorOut.isValid() && m_state.accumulatorOut.isEnumeration())
                    continue;

                const QString id = m_function->addressableScopes.id(scope);

                FixSuggestion suggestion;

                QQmlJS::SourceLocation fixLocation = getCurrentSourceLocation();
                fixLocation.length = 0;

                suggestion.fixes << FixSuggestion::Fix {
                    u"Reference it by id instead:"_qs,
                    fixLocation,
                    id.isEmpty() ? u"<id>."_qs : (id + u'.')
                };

                fixLocation = scope->sourceLocation();
                fixLocation.length = 0;

                if (id.isEmpty()) {
                    suggestion.fixes
                            << FixSuggestion::Fix { u"You first have to give the element an id"_qs,
                                                    QQmlJS::SourceLocation {},
                                                    {} };
                }

                m_logger->logWarning(
                        u"Using attached type %1 already initialized in a parent scope."_qs.arg(
                                m_state.accumulatorIn.scopeType()->internalName()),
                        Log_AttachedPropertyReuse, getCurrentSourceLocation(), true, true,
                        suggestion);
            }
        }
        m_typeInfo->usedAttachedTypes.insert(m_function->qmlScope, attachedType);
    }

    if (!m_state.accumulatorOut.isValid()) {
        if (m_typeResolver->isPrefix(propertyName)) {
            Q_ASSERT(m_state.accumulatorIn.isValid());
            m_state.accumulatorOut = QQmlJSRegisterContent::create(
                        m_state.accumulatorIn.storedType(),
                        m_jsUnitGenerator->getStringId(propertyName),
                        QQmlJSRegisterContent::ObjectModulePrefix,
                        m_typeResolver->containedType(m_state.accumulatorIn));
            return;
        }
        if (m_state.accumulatorIn.isImportNamespace())
            m_logger->logWarning(u"Type not found in namespace"_qs, Log_Type,
                                 getCurrentSourceLocation());
    } else if (m_state.accumulatorOut.variant() == QQmlJSRegisterContent::Singleton
               && m_state.accumulatorIn.variant() == QQmlJSRegisterContent::ObjectModulePrefix) {
        m_logger->logWarning(u"Cannot load singleton as property of object"_qs, Log_Type,
                             getCurrentSourceLocation());
        m_state.accumulatorOut = QQmlJSRegisterContent();
    }

    bool isRestricted = checkRestricted(propertyName);

    if (!m_state.accumulatorOut.isValid()) {
        setError(u"Cannot load property %1 from %2."_qs
                         .arg(propertyName, m_state.accumulatorIn.descriptiveName()));

        if (isRestricted)
            return;

        const QString typeName = m_typeResolver->containedTypeName(m_state.accumulatorIn);

        if (typeName == u"QVariant")
            return;
        if (m_state.accumulatorIn.isList() && propertyName == u"length")
            return;

        auto baseType = m_typeResolver->containedType(m_state.accumulatorIn);
        // Warn separately when a property is only not found because of a missing type
        if (isMissingPropertyType(baseType, propertyName))
            return;

        m_logger->logWarning(
                u"Property \"%1\" not found on type \"%2\""_qs.arg(propertyName).arg(typeName),
                Log_Type, getCurrentSourceLocation());
        return;
    }

    if (m_state.accumulatorOut.isMethod() && m_state.accumulatorOut.method().length() != 1) {
        setError(u"Cannot determine overloaded method on loadProperty"_qs);
        return;
    }

    if (m_state.accumulatorOut.isProperty()) {
        if (m_typeResolver->registerContains(m_state.accumulatorOut, m_typeResolver->voidType())) {
            setError(u"Type %1 does not have a property %2 for reading"_qs
                             .arg(m_state.accumulatorIn.descriptiveName(), propertyName));
            return;
        }

        if (!m_state.accumulatorOut.property().type()) {
            m_logger->logWarning(
                    QString::fromLatin1("Type of property \"%2\" not found").arg(propertyName),
                    Log_Type, getCurrentSourceLocation());
        }
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
        setError(u"Type %1 does not have a property %2 for writing"_qs
                         .arg(callBase.descriptiveName(), propertyName));
        return;
    }

    if (!property.isWritable()) {
        setError(u"Can't assign to read-only property %1"_qs.arg(propertyName));

        m_logger->logWarning(u"Cannot assign to read-only property %1"_qs.arg(propertyName),
                             Log_Property, getCurrentSourceLocation());

        return;
    }

    if (!canConvertFromTo(m_state.accumulatorIn, property)) {
        setError(u"cannot convert from %1 to %2"_qs
                         .arg(m_state.accumulatorIn.descriptiveName(), property.descriptiveName()));
        return;
    }
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
    Q_UNUSED(name)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_CallWithReceiver(int name, int thisObject, int argc, int argv)
{
    Q_UNUSED(name)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_CallProperty(int nameIndex, int base, int argc, int argv)
{
    auto callBase = m_state.registers[base];
    const QString propertyName = m_jsUnitGenerator->stringForIndex(nameIndex);

    const auto member = m_typeResolver->memberType(callBase, propertyName);
    const auto containedType = m_typeResolver->containedType(callBase);
    if (containedType == m_typeResolver->jsValueType()
        || containedType == m_typeResolver->varType()) {
        m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->jsValueType());
        return;
    }

    if (!member.isMethod()) {
        setError(u"Type %1 does not have a property %2 for calling"_qs
                         .arg(callBase.descriptiveName(), propertyName));

        if (callBase.isType() && isCallingProperty(callBase.type(), propertyName))
            return;

        if (checkRestricted(propertyName))
            return;

        m_logger->logWarning(
                u"Property \"%1\" not found on type \"%2\""_qs.arg(
                        propertyName, m_typeResolver->containedTypeName(callBase)),
                Log_Type, getCurrentSourceLocation());
        return;
    }

    checkDeprecated(containedType, propertyName, true);

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
            errors->append(u"return type %1 cannot be resolved"_qs
                                   .arg(method.returnTypeName()));
            continue;
        }

        const auto argumentTypes = method.parameterTypes();
        if (argc != argumentTypes.size()) {
            errors->append(u"Function expects %1 arguments, but %2 were provided"_qs
                                   .arg(argumentTypes.size())
                                   .arg(argc));
            continue;
        }

        bool matches = true;
        for (int i = 0; i < argc; ++i) {
            const auto argumentType = argumentTypes[i];
            if (argumentType.isNull()) {
                errors->append(u"type %1 for argument %2 cannot be resolved"_qs
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
                    u"argument %1 contains %2 but is expected to contain the type %3"_qs.arg(i).arg(
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

void QQmlJSTypePropagator::propagateCall(const QList<QQmlJSMetaMethod> &methods, int argc, int argv)
{
    QStringList errors;
    const QQmlJSMetaMethod match = bestMatchForCall(methods, argc, argv, &errors);

    if (!match.isValid()) {
        Q_ASSERT(errors.length() == methods.length());
        if (methods.length() == 1)
            setError(errors.first());
        else
            setError(u"No matching override found. Candidates:\n"_qs + errors.join(u'\n'));
        return;
    }

    const auto returnType = match.isJavaScriptFunction()
            ? m_typeResolver->jsValueType()
            : QQmlJSScope::ConstPtr(match.returnType());
    m_state.accumulatorOut = m_typeResolver->returnType(
                returnType ? QQmlJSScope::ConstPtr(returnType) : m_typeResolver->voidType(),
                match.isJavaScriptFunction() ? QQmlJSRegisterContent::JavaScriptReturnValue
                                             : QQmlJSRegisterContent::MethodReturnValue);
    if (!m_state.accumulatorOut.isValid())
        setError(u"Cannot store return type of method %1()."_qs.arg(match.methodName()));
}

void QQmlJSTypePropagator::generate_CallPropertyLookup(int lookupIndex, int base, int argc,
                                                       int argv)
{
    generate_CallProperty(m_jsUnitGenerator->lookupNameIndex(lookupIndex), base, argc, argv);
}

void QQmlJSTypePropagator::generate_CallElement(int base, int index, int argc, int argv)
{
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

    setError(u"method %1 cannot be resolved."_qs.arg(functionName));
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->jsValueType());

    setError(u"Cannot find function '%1'"_qs.arg(functionName));

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
    Q_UNUSED(func)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_TailCall(int func, int thisObject, int argc, int argv)
{
    Q_UNUSED(func)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_Construct(int func, int argc, int argv)
{
    Q_UNUSED(func)
    Q_UNUSED(argv)

    Q_UNUSED(argc)

    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->jsValueType());
}

void QQmlJSTypePropagator::generate_ConstructWithSpread(int func, int argc, int argv)
{
    Q_UNUSED(func)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_SetUnwindHandler(int offset)
{
    Q_UNUSED(offset)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_IGNORE();
}

void QQmlJSTypePropagator::generate_UnwindDispatch()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED_IGNORE();
}

void QQmlJSTypePropagator::generate_UnwindToLabel(int level, int offset)
{
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
    m_state.skipInstructionsUntilNextJumpTarget = true;
}

void QQmlJSTypePropagator::generate_GetException()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_SetException()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_CreateCallContext()
{
    m_state.accumulatorOut = m_state.accumulatorIn;
}

void QQmlJSTypePropagator::generate_PushCatchContext(int index, int name)
{
    Q_UNUSED(index)
    Q_UNUSED(name)
    INSTR_PROLOGUE_NOT_IMPLEMENTED_IGNORE();
}

void QQmlJSTypePropagator::generate_PushWithContext()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_PushBlockContext(int index)
{
    Q_UNUSED(index)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_CloneBlockContext()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_PushScriptContext(int index)
{
    Q_UNUSED(index)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_PopScriptContext()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_PopContext()
{
    m_state.accumulatorOut = m_state.accumulatorIn;
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
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->stringType());
}

void QQmlJSTypePropagator::generate_TypeofValue()
{
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->stringType());
}

void QQmlJSTypePropagator::generate_DeclareVar(int varName, int isDeletable)
{
    Q_UNUSED(varName)
    Q_UNUSED(isDeletable)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_DefineArray(int argc, int args)
{
    Q_UNUSED(argc);
    Q_UNUSED(args);
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->jsValueType());
}

void QQmlJSTypePropagator::generate_DefineObjectLiteral(int internalClassId, int argc, int args)
{
    // TODO: computed property names, getters, and setters are unsupported. How do we catch them?

    Q_UNUSED(internalClassId)
    Q_UNUSED(argc)
    Q_UNUSED(args)
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->jsValueType());
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
    m_state.accumulatorIn = QQmlJSRegisterContent();
    m_state.accumulatorOut = QQmlJSRegisterContent();
    m_state.skipInstructionsUntilNextJumpTarget = true;
}

void QQmlJSTypePropagator::generate_JumpTrue(int offset)
{
    if (!canConvertFromTo(m_state.accumulatorIn,
                          m_typeResolver->globalType(m_typeResolver->boolType()))) {
        setError(u"cannot convert from %1 to boolean"_qs
                         .arg(m_state.accumulatorIn.descriptiveName()));
        return;
    }
    saveRegisterStateForJump(offset);
}

void QQmlJSTypePropagator::generate_JumpFalse(int offset)
{
    if (!canConvertFromTo(m_state.accumulatorIn,
                          m_typeResolver->globalType(m_typeResolver->boolType()))) {
        setError(u"cannot convert from %1 to boolean"_qs
                         .arg(m_state.accumulatorIn.descriptiveName()));
        return;
    }
    saveRegisterStateForJump(offset);
}

void QQmlJSTypePropagator::generate_JumpNoException(int offset)
{
    saveRegisterStateForJump(offset);
}

void QQmlJSTypePropagator::generate_JumpNotUndefined(int offset)
{
    Q_UNUSED(offset)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_CheckException()
{
    m_state.accumulatorOut = m_state.accumulatorIn;
}

void QQmlJSTypePropagator::generate_CmpEqNull()
{
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->boolType());
}

void QQmlJSTypePropagator::generate_CmpNeNull()
{
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->boolType());
}

void QQmlJSTypePropagator::generate_CmpEqInt(int lhsConst)
{
    Q_UNUSED(lhsConst)
    m_state.accumulatorOut = QQmlJSRegisterContent(m_typeResolver->typeForBinaryOperation(
            QSOperator::Op::Equal, m_typeResolver->globalType(m_typeResolver->intType()),
            m_state.accumulatorIn));
}

void QQmlJSTypePropagator::generate_CmpNeInt(int lhsConst)
{
    Q_UNUSED(lhsConst)
    m_state.accumulatorOut = QQmlJSRegisterContent(m_typeResolver->typeForBinaryOperation(
            QSOperator::Op::NotEqual, m_typeResolver->globalType(m_typeResolver->intType()),
            m_state.accumulatorIn));
}

void QQmlJSTypePropagator::generate_CmpEq(int lhs)
{
    propagateBinaryOperation(QSOperator::Op::Equal, lhs);
}

void QQmlJSTypePropagator::generate_CmpNe(int lhs)
{
    propagateBinaryOperation(QSOperator::Op::NotEqual, lhs);
}

void QQmlJSTypePropagator::generate_CmpGt(int lhs)
{
    propagateBinaryOperation(QSOperator::Op::Gt, lhs);
}

void QQmlJSTypePropagator::generate_CmpGe(int lhs)
{
    propagateBinaryOperation(QSOperator::Op::Ge, lhs);
}

void QQmlJSTypePropagator::generate_CmpLt(int lhs)
{
    propagateBinaryOperation(QSOperator::Op::Lt, lhs);
}

void QQmlJSTypePropagator::generate_CmpLe(int lhs)
{
    propagateBinaryOperation(QSOperator::Op::Le, lhs);
}

void QQmlJSTypePropagator::generate_CmpStrictEqual(int lhs)
{
    propagateBinaryOperation(QSOperator::Op::StrictEqual, lhs);
}

void QQmlJSTypePropagator::generate_CmpStrictNotEqual(int lhs)
{
    propagateBinaryOperation(QSOperator::Op::StrictNotEqual, lhs);
}

void QQmlJSTypePropagator::generate_CmpIn(int lhs)
{
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

    switch (m_state.accumulatorIn.variant()) {
    case QQmlJSRegisterContent::ScopeAttached:
    case QQmlJSRegisterContent::MetaType:
        contained = m_state.accumulatorIn.scopeType();
        break;
    default:
        contained = m_typeResolver->containedType(m_state.accumulatorIn);
        break;
    }

    if (m_typeResolver->containedType(input)->accessSemantics()
                != QQmlJSScope::AccessSemantics::Reference
        || contained->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
        setError(u"invalid cast from %1 to %2. You can only cast object types."_qs
                         .arg(input.descriptiveName(), m_state.accumulatorIn.descriptiveName()));
    } else {
        m_state.accumulatorOut = m_typeResolver->globalType(contained);
    }
}

void QQmlJSTypePropagator::generate_UNot()
{
    if (!canConvertFromTo(m_state.accumulatorIn,
                          m_typeResolver->globalType(m_typeResolver->boolType()))) {
        setError(u"cannot convert from %1 to boolean"_qs
                         .arg(m_state.accumulatorIn.descriptiveName()));
        return;
    }
    m_state.accumulatorOut = m_typeResolver->globalType(m_typeResolver->boolType());
}

void QQmlJSTypePropagator::generate_UPlus()
{
    m_state.accumulatorOut = m_typeResolver->typeForUnaryOperation(
            QQmlJSTypeResolver::UnaryOperator::Plus, m_state.accumulatorIn);
}

void QQmlJSTypePropagator::generate_UMinus()
{
    m_state.accumulatorOut = m_typeResolver->typeForUnaryOperation(
            QQmlJSTypeResolver::UnaryOperator::Minus, m_state.accumulatorIn);
}

void QQmlJSTypePropagator::generate_UCompl()
{
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_Increment()
{
    m_state.accumulatorOut = m_typeResolver->typeForUnaryOperation(
            QQmlJSTypeResolver::UnaryOperator::Increment, m_state.accumulatorIn);
}

void QQmlJSTypePropagator::generate_Decrement()
{
    m_state.accumulatorOut = m_typeResolver->typeForUnaryOperation(
            QQmlJSTypeResolver::UnaryOperator::Decrement, m_state.accumulatorIn);
}

void QQmlJSTypePropagator::generate_Add(int lhs)
{
    propagateBinaryOperation(QSOperator::Op::Add, lhs);
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
    m_state.accumulatorOut = m_typeResolver->typeForBinaryOperation(
            QSOperator::Op::RShift, lhsRegister, m_state.accumulatorIn);
}

void QQmlJSTypePropagator::generate_Shl(int lhs)
{
    auto lhsRegister = checkedInputRegister(lhs);
    m_state.accumulatorOut = m_typeResolver->typeForBinaryOperation(
            QSOperator::Op::LShift, lhsRegister, m_state.accumulatorIn);
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

    m_state.accumulatorOut = m_typeResolver->typeForBinaryOperation(
            QSOperator::Op::RShift, m_state.accumulatorIn,
            m_typeResolver->globalType(m_typeResolver->intType()));
}

void QQmlJSTypePropagator::generate_ShlConst(int rhsConst)
{
    Q_UNUSED(rhsConst)

    m_state.accumulatorOut = m_typeResolver->typeForBinaryOperation(
            QSOperator::Op::LShift, m_state.accumulatorIn,
            m_typeResolver->globalType(m_typeResolver->intType()));
}

void QQmlJSTypePropagator::generate_Exp(int lhs)
{
    Q_UNUSED(lhs)
    INSTR_PROLOGUE_NOT_IMPLEMENTED();
}

void QQmlJSTypePropagator::generate_Mul(int lhs)
{
    propagateBinaryOperation(QSOperator::Op::Mul, lhs);
}

void QQmlJSTypePropagator::generate_Div(int lhs)
{
    propagateBinaryOperation(QSOperator::Op::Div, lhs);
}

void QQmlJSTypePropagator::generate_Mod(int lhs)
{
    propagateBinaryOperation(QSOperator::Op::Mod, lhs);
}

void QQmlJSTypePropagator::generate_Sub(int lhs)
{
    propagateBinaryOperation(QSOperator::Op::Sub, lhs);
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

QV4::Moth::ByteCodeHandler::Verdict
QQmlJSTypePropagator::startInstruction(QV4::Moth::Instr::Type instr)
{
    if (m_error->isValid())
        return SkipInstruction;

    if (m_state.jumpTargets.contains(currentInstructionOffset()))
        m_state.skipInstructionsUntilNextJumpTarget = false;
    else if (m_state.skipInstructionsUntilNextJumpTarget)
        return SkipInstruction;

    bool instructionWritesAccumulatorWithoutReading = false;
    switch (instr) {
    case QV4::Moth::Instr::Type::LoadReg:
    case QV4::Moth::Instr::Type::LoadZero:
    case QV4::Moth::Instr::Type::LoadTrue:
    case QV4::Moth::Instr::Type::LoadFalse:
    case QV4::Moth::Instr::Type::LoadConst:
    case QV4::Moth::Instr::Type::LoadInt:
    case QV4::Moth::Instr::Type::LoadUndefined:
    case QV4::Moth::Instr::Type::LoadName:
    case QV4::Moth::Instr::Type::LoadRuntimeString:
    case QV4::Moth::Instr::Type::LoadLocal:
    case QV4::Moth::Instr::Type::LoadQmlContextPropertyLookup:
    case QV4::Moth::Instr::Type::LoadGlobalLookup:
    case QV4::Moth::Instr::Type::CallQmlContextPropertyLookup:
    case QV4::Moth::Instr::Type::CallGlobalLookup:
    case QV4::Moth::Instr::Type::CallPropertyLookup:
        instructionWritesAccumulatorWithoutReading = true;
        break;
    default:
        break;
    }

    const int currentOffset = currentInstructionOffset();

    // If we reach an instruction that is a target of a jump earlier, then we must check that the
    // register state at the origin matches the current state. If not, then we may have to inject
    // conversion code (communicated to code gen via m_state.expectedTargetTypesBeforeJump). For
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

            // AccumulatorOut will be the "in" for this (upcoming) instruction, so if that one
            // merely writes to it, we don't care about it's value at the origin of the jump.
            if (registerIndex == Accumulator && instructionWritesAccumulatorWithoutReading)
                continue;

            auto newType = registerIt.value();
            if (!newType.isValid()) {
                setError(u"When reached from offset %1, %2 is undefined"_qs
                                 .arg(stateToMerge.originatingOffset)
                                 .arg(registerName(registerIndex)));
                return SkipInstruction;
            }

            auto currentRegister = m_state.registers.find(registerIndex);
            if (currentRegister != m_state.registers.end()) {
                // Careful with accessing the hash iterator later inside this block,
                // some operations may invalidate it.
                auto currentRegisterType = currentRegister;

                if (*currentRegisterType != newType) {
                    auto merged = m_typeResolver->merge(newType, *currentRegisterType);
                    Q_ASSERT(merged.isValid());
                    m_state.annotations[currentInstructionOffset()]
                            .expectedTargetTypesBeforeJump[registerIndex] = merged;
                    setRegister(registerIndex, merged);
                } else {
                    // Clear the constant value as this from a jump that might be merging two
                    // different value
                    //                    currentRegister->m_state.value = {};
                }
            }
        }
    }

    // Most instructions require a valid accumulator as input
    switch (instr) {
    case QV4::Moth::Instr::Type::Jump:
    case QV4::Moth::Instr::Type::LoadReg:
    case QV4::Moth::Instr::Type::LoadName:
    case QV4::Moth::Instr::Type::LoadRuntimeString:
    case QV4::Moth::Instr::Type::LoadInt:
    case QV4::Moth::Instr::Type::LoadNull:
    case QV4::Moth::Instr::Type::TypeofName:
    case QV4::Moth::Instr::Type::CallProperty:
    case QV4::Moth::Instr::Type::CallName:
    case QV4::Moth::Instr::Type::MoveReg:
    case QV4::Moth::Instr::Type::MoveConst:
    case QV4::Moth::Instr::Type::DefineArray:
    case QV4::Moth::Instr::Type::DefineObjectLiteral:
    case QV4::Moth::Instr::Type::CheckException:
    case QV4::Moth::Instr::Type::CreateCallContext:
    case QV4::Moth::Instr::Type::PopContext:
    case QV4::Moth::Instr::Type::JumpNoException:
    case QV4::Moth::Instr::Type::SetUnwindHandler:
    case QV4::Moth::Instr::Type::PushCatchContext:
    case QV4::Moth::Instr::Type::UnwindDispatch:
        break;
    default:
        if (instructionWritesAccumulatorWithoutReading)
            m_state.accumulatorIn = QQmlJSRegisterContent();
        else
            m_state.accumulatorIn = checkedInputRegister(Accumulator);
    }

    return ProcessInstruction;
}

void QQmlJSTypePropagator::endInstruction(QV4::Moth::Instr::Type instr)
{
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
        break;
    default:
        // If the instruction is expected to produce output, save it in the register set
        // for the next instruction.
        if (m_state.accumulatorOut.isValid()) {
            setRegister(Accumulator, m_state.accumulatorOut);
            m_state.accumulatorOut = QQmlJSRegisterContent();
        } else if (!m_error->isValid()) {
            setError(u"Instruction is expected to populate the accumulator"_qs);
            return;
        }
    }

    m_state.annotations[currentInstructionOffset()].registers = m_state.registers;
}

void QQmlJSTypePropagator::propagateBinaryOperation(QSOperator::Op op, int lhs)
{
    auto lhsRegister = checkedInputRegister(lhs);
    if (!lhsRegister.isValid())
        return;

    m_state.accumulatorOut =
            m_typeResolver->typeForBinaryOperation(op, lhsRegister, m_state.accumulatorIn);
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
            if (it->registers == state.registers)
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
        return u"accumulator"_qs;
    if (registerIndex >= FirstArgument
            && registerIndex < FirstArgument + m_function->argumentTypes.count()) {
        return u"argument %1"_qs.arg(registerIndex - FirstArgument);
    }

    return u"temporary register %1"_qs.arg(
            registerIndex - FirstArgument - m_function->argumentTypes.count());
}

// As the source register content may also be a register, we expect a copy here,
// rather than a reference. Otherwise you might pass a reference to another entry
// of m_state.registers, which then becomes invalid when making space for the new
// entry in the hash.
void QQmlJSTypePropagator::setRegister(int index, QQmlJSRegisterContent content)
{
    m_state.registers[index] = std::move(content);
}

void QQmlJSTypePropagator::setRegister(int index, const QQmlJSScope::ConstPtr &content)
{
    m_state.registers[index] = m_typeResolver->globalType(content);
}

QQmlJSRegisterContent QQmlJSTypePropagator::checkedInputRegister(int reg)
{
    VirtualRegisters::ConstIterator regIt = m_state.registers.find(reg);
    if (regIt == m_state.registers.constEnd()) {
        setError(u"Type error: could not infer the type of an expression"_qs);
        return {};
    }
    return *regIt;
}

bool QQmlJSTypePropagator::canConvertFromTo(const QQmlJSRegisterContent &from,
                                            const QQmlJSRegisterContent &to)
{
    return m_typeResolver->canConvertFromTo(from, to);
}

QT_END_NAMESPACE
