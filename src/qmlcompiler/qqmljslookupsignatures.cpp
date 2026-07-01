// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljslookupsignatures_p.h"

#include <private/qqmljstyperesolver_p.h>

using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE

QQmlJSLookupSignaturesRecorder::QQmlJSLookupSignaturesRecorder(
        const QString &currentFilePath, const QQmlJSTypeResolver *typeResolver)
    : m_currentFilePath(currentFilePath), m_typeResolver(typeResolver)
{
    Q_ASSERT(!m_currentFilePath.isEmpty());
    Q_ASSERT(m_typeResolver);
}

#define CHECKED_OPT_ASSIGN(var, expr)   \
    {                                   \
        const auto opt = expr;          \
        if (!opt.has_value())           \
            return false;               \
        var = opt.value();              \
    }

std::optional<QQmlPrivate::AOTLookupValidation::Type>
QQmlJSLookupSignaturesRecorder::type(const QQmlJSScope::ConstPtr &type)
{
    using namespace QQmlPrivate::AOTLookupValidation;

    QQmlPrivate::AOTLookupValidation::Type res;
    if (!type->isComposite()) {
        if (type->accessSemantics() == QQmlSA::AccessSemantics::Value && type->isSelfExtension()) {
            res.name = type->internalName();
            res.icNameOrExtensionTypeName = type->extensionTypeName();
        } else {
            res.name = type->internalName();
        }

        return res;
    }

    res.isComposite = IsComposite::Yes;
    if (isUnnamedCompositeType(type)) {
        // An unnamed type can only be used as the base of a lookup, not the type in a property or
        // method declaration. Assume the target property/method is declared on a base type, and
        // record the base type of \a type as the base of the lookup. Otherwise, we would have
        // discarded the lookup earlier. See recordPropertyLookup.
        if (!type->baseType()) {
            const QString msg = "unknown base type for composite object at %1:%2"_L1;
            m_rejectMessage = msg.arg(type->filePath(), QString::number(type->lineNumber()));
            return {};
        }
        return QQmlJSLookupSignaturesRecorder::type(type->baseType());
    }

    QQmlJSScope::ConstPtr root = type;
    while (!root->isFileRootComponent())
        root = root->parentScope();

    bool isFileBeingCompiled = root->filePath() == m_typeResolver->logger()->filePath();

    Q_ASSERT(!root->internalName().isEmpty());
    res.module = isFileBeingCompiled ? s_thisCuModule : root->moduleName();
    res.name = isFileBeingCompiled ? s_thisCuType : root->internalName();

    if (type->isInlineComponent()) {
        res.isInlineComponent = IsIC::Yes;
        res.icNameOrExtensionTypeName = *type->inlineComponentName();
        return res;
    }

    return res;
}

bool QQmlJSLookupSignaturesRecorder::cantDesync(const QQmlJSScope::ConstPtr &type) const
{
    Q_ASSERT(!m_typeResolver->logger()->filePath().isEmpty());
    // Same file -> If one changes the other also gets recompiled and adapts to the change
    return type->filePath() == m_typeResolver->logger()->filePath();
}

bool QQmlJSLookupSignaturesRecorder::safeBase(const QQmlJSScope::ConstPtr &base) const
{
    return base == m_typeResolver->varType() || base == m_typeResolver->jsValueType()
            || base->inherits(m_typeResolver->qmlPropertyMapType());
}

bool QQmlJSLookupSignaturesRecorder::isUnnamedCompositeType(const QQmlJSScope::ConstPtr &type) const
{
    return type->isComposite() && !type->isFileRootComponent() && !type->isInlineComponent();
}

static QQmlJSMetaMethod::RelativeFunctionIndex methodIndex(const QQmlJSMetaMethod &m)
{
    return m.otherMethodIndex() == QQmlJSMetaMethod::RelativeFunctionIndex::Invalid
            ? m.methodIndex() : m.otherMethodIndex();
}

// On the MetaObject, signals come before regular functions
static int ownRegularMethodCountBeforeIndex(const QQmlJSScope::ConstPtr &type, int index)
{
    const auto &ms = type->ownMethods();
    return std::count_if(ms.cbegin(), ms.cend(), [&](const QQmlJSMetaMethod &m) {
        Q_ASSERT(m.isConstructor() || methodIndex(m) != QQmlJSMetaMethod::RelativeFunctionIndex::Invalid);
        return m.methodType() != QQmlSA::MethodType::Signal && !m.isConstructor()
                && int(methodIndex(m)) < index;
    });
}

// On the MetaObject, signals come before regular functions
static int ownSignalCountAfterIndex(const QQmlJSScope::ConstPtr &type, int index)
{
    const auto &ms = type->ownMethods();
    return std::count_if(ms.cbegin(), ms.cend(), [&](const QQmlJSMetaMethod &m) {
        Q_ASSERT(m.isConstructor() || methodIndex(m) != QQmlJSMetaMethod::RelativeFunctionIndex::Invalid);
        return m.methodType() == QQmlSA::MethodType::Signal && int(methodIndex(m)) > index;
    });
}

// On the MetaObject, regular properties come before aliases
static int ownAliasCountBeforeIndex(const QQmlJSScope::ConstPtr &type, int index)
{
    const auto &ps = type->ownProperties();
    return std::count_if(ps.cbegin(), ps.cend(), [&](const auto &p) {
        return p.isAlias() && p.index() < index;
    });
}

// On the MetaObject, regular properties come before aliases
static int ownRegularPropertyCountAfterIndex(const QQmlJSScope::ConstPtr &type, int index)
{
    const auto &ps = type->ownProperties();
    return std::count_if(ps.cbegin(), ps.cend(), [&](const auto &p) {
        return !p.isAlias() && p.index() > index;
    });
}

bool QQmlJSLookupSignaturesRecorder::recordPropertyLookup(const QQmlJSScope::ConstPtr &base,
                                                          const QQmlJSMetaProperty &property)
{
    using namespace QQmlPrivate::AOTLookupValidation;

    const QString &name = property.propertyName();
    const auto [owner, extensionSpecifier] = QQmlJSScope::ownerOfProperty(base, name);
    if (base->isScript() || safeBase(base) || cantDesync(owner))
        return true;

    // When performing a lookup on an inner object of unnamed type, one of two things is true:
    //   1) The target property (or method) is declared within the object. Then, because the object
    //      is unnamed, it must be defined in the same file as the lookup and they cannot desync and
    //      we don't have to record it.
    //   2) Or the target property is declared on a base type of the inner object. Then we can
    //      simply record the lookup as a being performed on the base type instead. The addition of
    //      a property on the inner object that would shadow the base type's property can only be
    //      introduced by recompiling the lookup as well.
    if (isUnnamedCompositeType(base) && owner == base)
        return true;

    PropertySignature propertySignature;
    CHECKED_OPT_ASSIGN(propertySignature.type, type(property.type()))

    // Compiler indexes follow document order. On the MO, aliases come after regular properties.
    propertySignature.relativeIndex = property.isAlias()
            ? property.index() + ownRegularPropertyCountAfterIndex(owner, property.index())
            : property.index() - ownAliasCountBeforeIndex(owner, property.index());

    Lookup lookup;
    CHECKED_OPT_ASSIGN(lookup.base, type(base))
    lookup.member = name;

    m_signatures.insert(lookup, propertySignature);
    return true;
}

bool QQmlJSLookupSignaturesRecorder::recordMethodLookup(const QQmlJSScope::ConstPtr &base,
                                                        const QQmlJSMetaMethod &method)
{
    using namespace QQmlPrivate::AOTLookupValidation;

    const QString &name = method.methodName();
    const auto [owner, extensionSpecifier] = QQmlJSScope::ownerOfMethod(base, name);
    if (base->isScript() || safeBase(base) || cantDesync(owner))
        return true;

    // See recordPropertyLookup
    if (isUnnamedCompositeType(base) && owner == base)
        return true;

    // destroy and toString are special?
    if (name == QStringLiteral("destroy") || name == QStringLiteral("toString"))
        return true;

    MethodSignature methodSignature;

    if (method.methodType() == QQmlSA::MethodType::Signal) {
        methodSignature.isSignal = IsSignal::Yes;
        int index = int(methodIndex(method));
        methodSignature.relativeIndex = index - ownRegularMethodCountBeforeIndex(owner, index);
    } else {
        methodSignature.isSignal = IsSignal::No;
        int index = int(methodIndex(method));
        methodSignature.relativeIndex = index + ownSignalCountAfterIndex(owner, index);
    }

    methodSignature.types.push_back({});
    CHECKED_OPT_ASSIGN(methodSignature.types.back(), type(method.returnType()))
    for (const auto &param : method.parameters()) {
        methodSignature.paramNames.push_back(param.name());
        methodSignature.types.push_back({});
        CHECKED_OPT_ASSIGN(methodSignature.types.back(), type(param.type()))
    }

    Lookup lookup;
    CHECKED_OPT_ASSIGN(lookup.base, type(base))
    lookup.member = name;

    m_signatures.insert(lookup, methodSignature);
    return true;
}

bool QQmlJSLookupSignaturesRecorder::recordEnumKeyLookup(const QQmlJSScope::ConstPtr &base,
                                                         const QQmlJSMetaEnum &metaEnum,
                                                         const QString &keyName)
{
    using namespace QQmlPrivate::AOTLookupValidation;

    const auto [owner, extensionSpecifier] = QQmlJSScope::ownerOfEnum(base, metaEnum.name());
    if (base->isScript() || safeBase(base) || cantDesync(owner))
        return true;

    EnumKeySignature enumSignature;
    // QTBUG-145053: enums can only hold ints in the Compiler
    enumSignature.value = quint64(metaEnum.value(keyName));
    if (metaEnum.isFlag())
        enumSignature.isFlag = IsFlag::Yes;

    Lookup lookup;
    CHECKED_OPT_ASSIGN(lookup.base, type(base))
    lookup.member = keyName;
    lookup.enumName = metaEnum.name();
    m_signatures.insert(lookup, enumSignature);
    return true;
}

QT_END_NAMESPACE
