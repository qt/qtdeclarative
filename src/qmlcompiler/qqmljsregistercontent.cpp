// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsregistercontent_p.h"

#include <variant>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

struct QQmlJSRegisterContentPrivate
{
public:

    using ContentVariant = QQmlJSRegisterContent::ContentVariant;

    enum class Kind : size_t {
        Type, Property, Enum, Method, ImportNamespace, Conversion, MethodCall
    };

    struct ConvertedTypes
    {
        QList<QQmlJSRegisterContent> origins;
        QQmlJSScope::ConstPtr result;
        QQmlJSRegisterContent resultScope;

        friend size_t qHash(const ConvertedTypes &types, size_t seed = 0)
        {
            return qHashMulti(seed, types.origins, types.result, types.resultScope);
        }

        friend bool operator==(const ConvertedTypes &a, const ConvertedTypes &b)
        {
            return a.origins == b.origins && a.result == b.result && a.resultScope == b.resultScope;
        }

        friend bool operator!=(const ConvertedTypes &a, const ConvertedTypes &b)
        {
            return !(a == b);
        }
    };

    struct PropertyLookup
    {
        QQmlJSMetaProperty property;
        int baseLookupIndex = QQmlJSRegisterContent::InvalidLookupIndex;
        int resultLookupIndex = QQmlJSRegisterContent::InvalidLookupIndex;

        friend size_t qHash(const PropertyLookup &property, size_t seed = 0)
        {
            return qHashMulti(
                    seed, property.property, property.baseLookupIndex, property.resultLookupIndex);
        }

        friend bool operator==(const PropertyLookup &a, const PropertyLookup &b)
        {
            return a.baseLookupIndex == b.baseLookupIndex
                    && a.resultLookupIndex == b.resultLookupIndex
                    && a.property == b.property;
        }

        friend bool operator!=(const PropertyLookup &a, const PropertyLookup &b)
        {
            return !(a == b);
        }
    };

    using Content = std::variant<
            std::pair<QQmlJSScope::ConstPtr, int>,
            PropertyLookup,
            std::pair<QQmlJSMetaEnum, QString>,
            std::pair<QList<QQmlJSMetaMethod>, QQmlJSScope::ConstPtr>,
            std::pair<uint, QQmlJSScope::ConstPtr>,
            ConvertedTypes,
            QQmlJSMetaMethod
    >;

    friend size_t qHash(const QQmlJSRegisterContentPrivate &registerContent, size_t seed = 0)
    {
        seed = qHashMulti(
                seed, registerContent.m_storage, registerContent.m_content.index(),
                registerContent.m_variant, registerContent.m_scope);

        switch (Kind(registerContent.m_content.index())) {
        case Kind::Type:
            return qHash(std::get<std::pair<QQmlJSScope::ConstPtr, int>>(registerContent.m_content),
                         seed);
        case Kind::Property:
            return qHash(std::get<PropertyLookup>(registerContent.m_content), seed);
        case Kind::Enum:
            return qHash(std::get<std::pair<QQmlJSMetaEnum, QString>>(registerContent.m_content),
                         seed);
        case Kind::Method:
            return qHash(std::get<std::pair<QList<QQmlJSMetaMethod>, QQmlJSScope::ConstPtr>>(
                                 registerContent.m_content), seed);
        case Kind::ImportNamespace:
            return qHash(std::get<std::pair<uint, QQmlJSScope::ConstPtr>>(
                                 registerContent.m_content), seed);
        case Kind::Conversion:
            return qHash(std::get<ConvertedTypes>(registerContent.m_content), seed);
        case Kind::MethodCall:
            return qHash(std::get<QQmlJSMetaMethod>(registerContent.m_content), seed);
        }

        Q_UNREACHABLE_RETURN(seed);
    }

    friend bool operator==(
            const QQmlJSRegisterContentPrivate &a, const QQmlJSRegisterContentPrivate &b)
    {
        return a.m_storage == b.m_storage && a.m_variant == b.m_variant
                && a.m_scope == b.m_scope && a.m_content == b.m_content;
    }

    friend bool operator!=(
            const QQmlJSRegisterContentPrivate &a, const QQmlJSRegisterContentPrivate &b)
    {
        return !(a == b);
    }

    QQmlJSRegisterContent m_storage;
    QQmlJSRegisterContent m_scope;
    Content m_content;
    ContentVariant m_variant = ContentVariant::Unknown;

    QQmlJSRegisterContent m_original;
    QQmlJSRegisterContent m_shadowed;

    int resultLookupIndex() const
    {
        switch (Kind(m_content.index())) {
        case Kind::Type:
            return std::get<std::pair<QQmlJSScope::ConstPtr, int>>(m_content).second;
        case Kind::Property:
            return std::get<PropertyLookup>(m_content).resultLookupIndex;
        default:
            return QQmlJSRegisterContent::InvalidLookupIndex;
        }
    }

    void setType(const QQmlJSScope::ConstPtr &type)
    {
        switch (Kind(m_content.index())) {
        case Kind::Type:
            std::get<std::pair<QQmlJSScope::ConstPtr, int>>(m_content).first = type;
            return;
        case Kind::Property:
            std::get<PropertyLookup>(m_content).property.setType(type);
            return;
        case Kind::Enum:
            std::get<std::pair<QQmlJSMetaEnum, QString>>(m_content).first.setType(type);
            return;
        case Kind::Method:
            std::get<std::pair<QList<QQmlJSMetaMethod>, QQmlJSScope::ConstPtr>>(m_content)
                    .second = type;
            return;
        case Kind::ImportNamespace:
            std::get<std::pair<uint, QQmlJSScope::ConstPtr>>(m_content).second = type;
            return;
        case Kind::Conversion:
            std::get<ConvertedTypes>(m_content).result = type;
            return;
        case Kind::MethodCall:
            std::get<QQmlJSMetaMethod>(m_content).setReturnType({ type });
            return;
        }

        Q_UNREACHABLE_RETURN();
    }

private:
    friend class QQmlJSRegisterContentPool;

    QQmlJSRegisterContentPrivate() = default;
    ~QQmlJSRegisterContentPrivate() = default;
    QQmlJSRegisterContentPrivate(const QQmlJSRegisterContentPrivate &) = default;
    QQmlJSRegisterContentPrivate(QQmlJSRegisterContentPrivate &&) = default;
    QQmlJSRegisterContentPrivate &operator=(const QQmlJSRegisterContentPrivate &) = default;
    QQmlJSRegisterContentPrivate &operator=(QQmlJSRegisterContentPrivate &&) = default;
};

bool QQmlJSRegisterContent::isValid() const
{
    return !containedType().isNull();
};

QString QQmlJSRegisterContent::descriptiveName() const
{
    using Kind = QQmlJSRegisterContentPrivate::Kind;

    if (!d || (!d->m_storage.isValid() && containedType().isNull()))
        return u"(invalid type)"_s;

    const auto scope = [this]() -> QString {
        if (!d->m_scope.isValid())
            return u"(invalid type)::"_s;

        const QQmlJSScope::ConstPtr scopeContained = d->m_scope.containedType();
        if (scopeContained.isNull())
            return u"(invalid type)::"_s;

        return (scopeContained->internalName().isEmpty()
                        ? (scopeContained->filePath().isEmpty()
                                   ? u"??"_s
                                   : (u"(component in "_s + scopeContained->filePath() + u")"_s))
                        : scopeContained->internalName())
                + u"::"_s;
    };

    QString result;
    switch (Kind(d->m_content.index())) {
    case Kind::Type: {
        const QQmlJSScope::ConstPtr contained = type();
        result += contained->internalName();
        const QQmlJSScope::ConstPtr stored = d->m_storage.containedType();
        if (stored && stored->internalName() != contained->internalName())
            result += u" stored as "_s + stored->internalName();
        return result;
    }
    case Kind::Property: {
        const QQmlJSMetaProperty prop = property();
        result += scope() + prop.propertyName() + u" with type "_s + prop.typeName();

        QStringList details;
        if (original().isValid() && !prop.type()->internalName().isEmpty())
            details.append(u"adjusted to " + prop.type()->internalName());
        const QQmlJSScope::ConstPtr stored = d->m_storage.containedType();
        if (stored && stored->internalName() != prop.typeName())
            details.append(u"stored as "_s + stored->internalName());

        if (!details.isEmpty())
            result += u" (%1)"_s.arg(details.join(u", "));

        return result;
    }
    case Kind::Method: {
        const auto methods = method();
        if (methods.isEmpty())
            result = scope() + u"(unknown method)"_s;
        else
            result = scope() + methods[0].methodName() + u"(...)"_s;
        if (d->m_storage.isValid())
            return result + u" (stored as "_s + d->m_storage.containedType()->internalName() + u")";
        return result;
    }
    case Kind::Enum: {
        const QString enumName = enumeration().name();
        const QString memberName = enumMember();
        if (memberName.isEmpty())
            result = scope() + enumName;
        else
            result = scope() + enumName + u"::"_s + memberName;
        if (d->m_storage.isValid())
            return result + u" (stored as "_s + d->m_storage.containedType()->internalName() + u")";
        return result;
    }
    case Kind::ImportNamespace: {
        return u"import namespace %1"_s.arg(importNamespace());
    }
    case Kind::Conversion: {
        return u"conversion to %1"_s.arg(conversionResultType()->internalName());
    }
    case Kind::MethodCall: {
        const QQmlJSMetaMethod &method = std::get<QQmlJSMetaMethod>(d->m_content);
        return u"call to method %1, returning %2"_s.arg(
                method.methodName(), method.returnTypeName());
    }
    }

    Q_UNREACHABLE_RETURN(result + u"wat?"_s);
}

QString QQmlJSRegisterContent::containedTypeName() const
{
    QQmlJSScope::ConstPtr type;

    switch (variant()) {
    case QQmlJSRegisterContent::MetaType:
        type = scopeType();
        break;
    default:
        type = containedType();
        break;
    }

    return QQmlJSScope::prettyName(
            type->internalName().isEmpty() ? type->baseTypeName() : type->internalName());
}

bool QQmlJSRegisterContent::isType() const
{
    return d && d->m_content.index() == size_t(QQmlJSRegisterContentPrivate::Kind::Type);
}

bool QQmlJSRegisterContent::isProperty() const
{
    return d && d->m_content.index() == size_t(QQmlJSRegisterContentPrivate::Kind::Property);
}

bool QQmlJSRegisterContent::isEnumeration() const
{
    return d && d->m_content.index() == size_t(QQmlJSRegisterContentPrivate::Kind::Enum);
}

bool QQmlJSRegisterContent::isMethod() const
{
    return d && d->m_content.index() == size_t(QQmlJSRegisterContentPrivate::Kind::Method);
}

bool QQmlJSRegisterContent::isImportNamespace() const
{
    return d && d->m_content.index() == size_t(QQmlJSRegisterContentPrivate::Kind::ImportNamespace);
}

bool QQmlJSRegisterContent::isConversion() const
{
    return d && d->m_content.index() == size_t(QQmlJSRegisterContentPrivate::Kind::Conversion);
}

bool QQmlJSRegisterContent::isMethodCall() const
{
    return d && d->m_content.index() == size_t(QQmlJSRegisterContentPrivate::Kind::MethodCall);
}

bool QQmlJSRegisterContent::isList() const
{
    using Kind = QQmlJSRegisterContentPrivate::Kind;

    if (!d)
        return false;

    switch (Kind(d->m_content.index())) {
    case Kind::Type:
        return std::get<std::pair<QQmlJSScope::ConstPtr, int>>(d->m_content).first
                       ->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence;
    case Kind::Property:
        return std::get<QQmlJSRegisterContentPrivate::PropertyLookup>(d->m_content).property.type()
                       ->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence;
    case Kind::Conversion:
        return std::get<QQmlJSRegisterContentPrivate::ConvertedTypes>(d->m_content).result
                       ->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence;
    case Kind::MethodCall:
        return std::get<QQmlJSMetaMethod>(d->m_content).returnType()
                       ->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence;
    default:
        return false;
    }
}

bool QQmlJSRegisterContent::isWritable() const
{
    using Kind = QQmlJSRegisterContentPrivate::Kind;

    if (!d)
        return false;

    switch (Kind(d->m_content.index())) {
    case Kind::Property:
        return std::get<QQmlJSRegisterContentPrivate::PropertyLookup>(d->m_content)
                .property.isWritable();

    // TODO: What can we actually write?
    default:
        break;
    }

    return true;
}

bool QQmlJSRegisterContent::isJavaScriptReturnValue() const
{
    return d && isMethodCall() && std::get<QQmlJSMetaMethod>(d->m_content).isJavaScriptFunction();
}

/*!
 * \internal
 * Precondition: This is an attachment.
 * Return the type that does the attaching.
 */
QQmlJSRegisterContent QQmlJSRegisterContent::attacher() const
{
    Q_ASSERT(d);
    Q_ASSERT(d->m_variant == Attachment);
    return scope();
}

/*!
 * \internal
 * Precondition: This is an attachment.
 * Return the type of the object the attachment is attached to.
 */
QQmlJSRegisterContent QQmlJSRegisterContent::attachee() const
{
    Q_ASSERT(d);
    Q_ASSERT(d->m_variant == Attachment);
    QQmlJSRegisterContent attachee = attacher().scope();
    while (attachee.variant() == ModulePrefix)
        attachee = attachee.scope();
    return attachee;
}

QQmlJSScope::ConstPtr QQmlJSRegisterContent::storedType() const
{
    return d ? d->m_storage.containedType() : QQmlJSScope::ConstPtr();
}

QQmlJSScope::ConstPtr QQmlJSRegisterContent::containedType() const
{
    if (!d)
        return QQmlJSScope::ConstPtr();
    if (isType())
        return type();
    if (isProperty())
        return std::get<QQmlJSRegisterContentPrivate::PropertyLookup>(d->m_content).property.type();
    if (isEnumeration())
        return std::get<std::pair<QQmlJSMetaEnum, QString>>(d->m_content).first.type();
    if (isMethod())
        return methodType();
    if (isImportNamespace())
        return importNamespaceType();
    if (isConversion())
        return conversionResultType();
    if (isMethodCall())
        return std::get<QQmlJSMetaMethod>(d->m_content).returnType();

    Q_UNREACHABLE_RETURN({});
}

QQmlJSScope::ConstPtr QQmlJSRegisterContent::scopeType() const
{
    return d ? d->m_scope.containedType() : QQmlJSScope::ConstPtr();
}

QQmlJSScope::ConstPtr QQmlJSRegisterContent::type() const
{
    Q_ASSERT(isType());
    return std::get<std::pair<QQmlJSScope::ConstPtr, int>>(d->m_content).first;
}

QQmlJSMetaProperty QQmlJSRegisterContent::property() const
{
    Q_ASSERT(isProperty());
    return std::get<QQmlJSRegisterContentPrivate::PropertyLookup>(d->m_content).property;
}

int QQmlJSRegisterContent::baseLookupIndex() const
{
    Q_ASSERT(isProperty());
    return std::get<QQmlJSRegisterContentPrivate::PropertyLookup>(d->m_content).baseLookupIndex;
}

int QQmlJSRegisterContent::resultLookupIndex() const
{
    return d ? d->resultLookupIndex() : InvalidLookupIndex;
}

QQmlJSMetaEnum QQmlJSRegisterContent::enumeration() const
{
    Q_ASSERT(isEnumeration());
    return std::get<std::pair<QQmlJSMetaEnum, QString>>(d->m_content).first;
}

QString QQmlJSRegisterContent::enumMember() const
{
    Q_ASSERT(isEnumeration());
    return std::get<std::pair<QQmlJSMetaEnum, QString>>(d->m_content).second;
}

QList<QQmlJSMetaMethod> QQmlJSRegisterContent::method() const
{
    Q_ASSERT(isMethod());
    return std::get<std::pair<QList<QQmlJSMetaMethod>, QQmlJSScope::ConstPtr>>(d->m_content).first;
}

QQmlJSScope::ConstPtr QQmlJSRegisterContent::methodType() const
{
    Q_ASSERT(isMethod());
    return std::get<std::pair<QList<QQmlJSMetaMethod>, QQmlJSScope::ConstPtr>>(d->m_content).second;
}

uint QQmlJSRegisterContent::importNamespace() const
{
    Q_ASSERT(isImportNamespace());
    return std::get<std::pair<uint, QQmlJSScope::ConstPtr>>(d->m_content).first;
}

QQmlJSScope::ConstPtr QQmlJSRegisterContent::importNamespaceType() const
{
    Q_ASSERT(isImportNamespace());
    return std::get<std::pair<uint, QQmlJSScope::ConstPtr>>(d->m_content).second;
}

QQmlJSScope::ConstPtr QQmlJSRegisterContent::conversionResultType() const
{
    Q_ASSERT(isConversion());
    return std::get<QQmlJSRegisterContentPrivate::ConvertedTypes>(d->m_content).result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::conversionResultScope() const
{
    Q_ASSERT(isConversion());
    return std::get<QQmlJSRegisterContentPrivate::ConvertedTypes>(d->m_content).resultScope;
}

QList<QQmlJSRegisterContent> QQmlJSRegisterContent::conversionOrigins() const
{
    Q_ASSERT(isConversion());
    return std::get<QQmlJSRegisterContentPrivate::ConvertedTypes>(d->m_content).origins;
}

QQmlJSMetaMethod QQmlJSRegisterContent::methodCall() const
{
    Q_ASSERT(isMethodCall());
    return std::get<QQmlJSMetaMethod>(d->m_content);
}

QQmlJSRegisterContent::ContentVariant QQmlJSRegisterContent::variant() const
{
    return d ? d->m_variant : Unknown;
}

QQmlJSRegisterContent QQmlJSRegisterContent::scope() const
{
    return d ? d->m_scope : QQmlJSRegisterContent();
}

QQmlJSRegisterContent QQmlJSRegisterContent::storage() const
{
    return d ? d->m_storage : QQmlJSRegisterContent();
}

QQmlJSRegisterContent QQmlJSRegisterContent::original() const
{
    return d ? d->m_original : QQmlJSRegisterContent();
}

QQmlJSRegisterContent QQmlJSRegisterContent::shadowed() const
{
    return d ? d->m_shadowed : QQmlJSRegisterContent();
}

QQmlJSRegisterContentPool::QQmlJSRegisterContentPool() = default;
QQmlJSRegisterContentPool::~QQmlJSRegisterContentPool() = default;

QQmlJSRegisterContent QQmlJSRegisterContentPool::createType(
        const QQmlJSScope::ConstPtr &type, int resultLookupIndex,
        QQmlJSRegisterContent::ContentVariant variant, QQmlJSRegisterContent scope)
{
    QQmlJSRegisterContentPrivate *result = create(scope, variant);
    result->m_content = std::make_pair(type, resultLookupIndex);
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContentPool::createProperty(
        const QQmlJSMetaProperty &property, int baseLookupIndex, int resultLookupIndex,
        QQmlJSRegisterContent::ContentVariant variant, QQmlJSRegisterContent scope)
{
    QQmlJSRegisterContentPrivate *result = create(scope, variant);
    result->m_content = QQmlJSRegisterContentPrivate::PropertyLookup {
        property,
        baseLookupIndex,
        resultLookupIndex
    };
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContentPool::createEnumeration(
        const QQmlJSMetaEnum &enumeration, const QString &enumMember,
        QQmlJSRegisterContent::ContentVariant variant, QQmlJSRegisterContent scope)
{
    QQmlJSRegisterContentPrivate *result = create(scope, variant);
    result->m_content = std::make_pair(enumeration, enumMember);
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContentPool::createMethod(
        const QList<QQmlJSMetaMethod> &methods, const QQmlJSScope::ConstPtr &methodType,
        QQmlJSRegisterContent::ContentVariant variant, QQmlJSRegisterContent scope)
{
    // Methods can only be stored in QJSValue.
    Q_ASSERT(methodType->internalName() == u"QJSValue"_s);
    QQmlJSRegisterContentPrivate *result = create(scope, variant);
    result->m_content = std::make_pair(methods, methodType);
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContentPool::createMethodCall(
        const QQmlJSMetaMethod &method, const QQmlJSScope::ConstPtr &returnType,
        QQmlJSRegisterContent scope)
{
    QQmlJSRegisterContentPrivate *result = create(scope, ContentVariant::MethodCall);

    QQmlJSMetaMethod resultMethod = method;
    resultMethod.setReturnType({ returnType });
    resultMethod.setReturnTypeName(returnType->internalName());
    result->m_content = std::move(resultMethod);

    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContentPool::createImportNamespace(
        uint importNamespaceStringId, const QQmlJSScope::ConstPtr &importNamespaceType,
        QQmlJSRegisterContent::ContentVariant variant, QQmlJSRegisterContent scope)
{
    QQmlJSRegisterContentPrivate *result = create(scope, variant);
    result->m_content = std::make_pair(importNamespaceStringId, importNamespaceType);
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContentPool::createConversion(
        const QList<QQmlJSRegisterContent> &origins, const QQmlJSScope::ConstPtr &conversion,
        QQmlJSRegisterContent conversionScope, ContentVariant variant,
        QQmlJSRegisterContent scope)
{
    QQmlJSRegisterContentPrivate *result = create(scope, variant);

    result->m_content = QQmlJSRegisterContentPrivate::ConvertedTypes {
        origins,
        conversion,
        conversionScope
    };

    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContentPool::storedIn(
        QQmlJSRegisterContent content, const QQmlJSScope::ConstPtr &newStoredType)
{
    Q_ASSERT(content.d);
    QQmlJSRegisterContentPrivate *result = clone(content.d);
    result->m_storage = createType(
            newStoredType, QQmlJSRegisterContent::InvalidLookupIndex, ContentVariant::Storage);
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContentPool::castTo(
        QQmlJSRegisterContent content, const QQmlJSScope::ConstPtr &newContainedType)
{
    // This is not a conversion but a run time cast. It may result in null or undefined.
    QQmlJSRegisterContentPrivate *result = create(content, ContentVariant::Cast);
    result->m_content = std::make_pair(newContainedType, result->resultLookupIndex());
    return result;
}

void QQmlJSRegisterContentPool::storeType(
        QQmlJSRegisterContent content, const QQmlJSScope::ConstPtr &stored)
{
    QQmlJSRegisterContentPrivate *d = content.d;

    Q_ASSERT(d);
    Q_ASSERT(d->m_storage.isNull());
    d->m_storage = createType(
            stored, QQmlJSRegisterContent::InvalidLookupIndex, ContentVariant::Storage);
}

void QQmlJSRegisterContentPool::adjustType(
        QQmlJSRegisterContent content, const QQmlJSScope::ConstPtr &adjusted)
{
    QQmlJSRegisterContentPrivate *d = content.d;

    Q_ASSERT(d);
    Q_ASSERT(d->m_original.isNull());
    d->m_original = clone(d);
    d->setType(adjusted);
}

void QQmlJSRegisterContentPool::generalizeType(
        QQmlJSRegisterContent content, const QQmlJSScope::ConstPtr &generalized)
{
    QQmlJSRegisterContentPrivate *d = content.d;

    Q_ASSERT(d);
    Q_ASSERT(d->m_shadowed.isNull());
    d->m_shadowed = clone(d);
    d->setType(generalized);
}

void QQmlJSRegisterContentPool::setAllocationMode(AllocationMode mode)
{
    m_checkpoint = (mode == Temporary) ? m_pool.size() : -1;
}

void QQmlJSRegisterContentPool::clearTemporaries()
{
    if (m_checkpoint != -1)
        m_pool.resize(m_checkpoint);
}

QQmlJSRegisterContentPrivate *QQmlJSRegisterContentPool::clone(
        const QQmlJSRegisterContentPrivate *from)
{
    m_pool.push_back(std::unique_ptr<QQmlJSRegisterContentPrivate, Deleter>(from
        ? new QQmlJSRegisterContentPrivate(*from)
        : new QQmlJSRegisterContentPrivate));
    return m_pool.back().get();
}

QQmlJSRegisterContentPrivate *QQmlJSRegisterContentPool::create(
        QQmlJSRegisterContent scope, ContentVariant variant)
{
    QQmlJSRegisterContentPrivate *result = create();
    result->m_scope = scope;
    result->m_variant = variant;
    return result;
}

QT_END_NAMESPACE
