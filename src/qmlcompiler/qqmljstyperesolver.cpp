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

#include "qqmljstyperesolver_p.h"

#include "qqmljsimporter_p.h"
#include "qqmljsimportvisitor_p.h"
#include "qqmljslogger_p.h"
#include <private/qv4value_p.h>

#include <private/qduplicatetracker_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcTypeResolver, "qt.qml.compiler.typeresolver", QtInfoMsg);

template<typename Action>
static bool searchBaseAndExtensionTypes(const QQmlJSScope::ConstPtr type, const Action &check)
{
    QDuplicateTracker<QQmlJSScope::ConstPtr> seen;
    for (QQmlJSScope::ConstPtr scope = type; scope && !seen.hasSeen(scope);
         scope = scope->baseType()) {
        // Extensions override their base types
        QDuplicateTracker<QQmlJSScope::ConstPtr> seenExtensions;
        for (QQmlJSScope::ConstPtr extension = scope->extensionType();
             extension && !seenExtensions.hasSeen(extension);
             extension = extension->baseType()) {
            if (check(extension, QQmlJSTypeResolver::Extension))
                return true;
        }

        if (check(scope, QQmlJSTypeResolver::Base))
            return true;
    }

    return false;
}

QQmlJSTypeResolver::QQmlJSTypeResolver(QQmlJSImporter *importer)
    : m_typeTracker(std::make_unique<TypeTracker>())
{
    const QQmlJSImporter::ImportedTypes builtinTypes = importer->builtinInternalNames();
    m_voidType = builtinTypes[u"void"_s].scope;
    m_nullType = builtinTypes[u"std::nullptr_t"_s].scope;
    m_realType = builtinTypes[u"double"_s].scope;
    m_floatType = builtinTypes[u"float"_s].scope;
    m_intType = builtinTypes[u"int"_s].scope;
    m_boolType = builtinTypes[u"bool"_s].scope;
    m_stringType = builtinTypes[u"QString"_s].scope;
    m_stringListType = builtinTypes[u"QStringList"_s].scope;
    m_urlType = builtinTypes[u"QUrl"_s].scope;
    m_dateTimeType = builtinTypes[u"QDateTime"_s].scope;
    m_variantListType = builtinTypes[u"QVariantList"_s].scope;
    m_varType = builtinTypes[u"QVariant"_s].scope;
    m_jsValueType = builtinTypes[u"QJSValue"_s].scope;

    QQmlJSScope::Ptr emptyListType = QQmlJSScope::create();
    emptyListType->setInternalName(u"void*"_s);
    emptyListType->setAccessSemantics(QQmlJSScope::AccessSemantics::Sequence);
    m_emptyListType = emptyListType;

    QQmlJSScope::Ptr jsPrimitiveType = QQmlJSScope::create();
    jsPrimitiveType->setInternalName(u"QJSPrimitiveValue"_s);
    jsPrimitiveType->setFilePath(u"qjsprimitivevalue.h"_s);
    jsPrimitiveType->setAccessSemantics(QQmlJSScope::AccessSemantics::Value);
    m_jsPrimitiveType = jsPrimitiveType;

    QQmlJSScope::Ptr listPropertyType = QQmlJSScope::create();
    listPropertyType->setInternalName(u"QQmlListProperty<QObject>"_s);
    listPropertyType->setFilePath(u"qqmllist.h"_s);
    listPropertyType->setAccessSemantics(QQmlJSScope::AccessSemantics::Sequence);
    listPropertyType->setValueTypeName(u"QObject"_s);
    QQmlJSScope::resolveTypes(listPropertyType, builtinTypes);
    m_listPropertyType = listPropertyType;

    QQmlJSScope::Ptr metaObjectType = QQmlJSScope::create();
    metaObjectType->setInternalName(u"const QMetaObject"_s);
    metaObjectType->setFilePath(u"qmetaobject.h"_s);
    metaObjectType->setAccessSemantics(QQmlJSScope::AccessSemantics::Reference);
    m_metaObjectType = metaObjectType;

    m_jsGlobalObject = importer->jsGlobalObject();
    auto numberMethods = m_jsGlobalObject->methods(u"Number"_s);
    Q_ASSERT(numberMethods.length() == 1);
    m_numberPrototype = numberMethods[0].returnType()->baseType();
    Q_ASSERT(m_numberPrototype);
    Q_ASSERT(m_numberPrototype->internalName() == u"NumberPrototype"_s);
}

/*!
    \internal

    Initializes the type resolver. As part of that initialization, makes \a
    visitor traverse the program.
*/
void QQmlJSTypeResolver::init(QQmlJSImportVisitor *visitor, QQmlJS::AST::Node *program)
{
    m_logger = visitor->logger();

    m_objectsById.clear();
    m_objectsByLocation.clear();
    m_imports.clear();
    m_signalHandlers.clear();

    program->accept(visitor);

    m_objectsById = visitor->addressableScopes();
    m_objectsByLocation = visitor->scopesBylocation();
    m_signalHandlers = visitor->signalHandlers();
    m_imports = visitor->imports();
}

QQmlJSScope::ConstPtr
QQmlJSTypeResolver::scopeForLocation(const QV4::CompiledData::Location &location) const
{
    qCDebug(lcTypeResolver()).nospace()
            << "looking for object at " << location.line << ':' << location.column;
    return m_objectsByLocation[location];
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::scopeForId(
        const QString &id, const QQmlJSScope::ConstPtr &referrer) const
{
    return m_objectsById.scope(id, referrer);
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::listType(
        const QQmlJSScope::ConstPtr &elementType, ListMode mode) const
{
    auto it = m_typeTracker->listTypes.find(elementType);
    if (it != m_typeTracker->listTypes.end())
        return *it;

    switch (elementType->accessSemantics()) {
    case QQmlJSScope::AccessSemantics::Reference:
        if (mode == UseListReference)
            return m_listPropertyType;
        if (elementType->internalName() != u"QObject"_s)
            return listType(genericType(elementType), mode);
        Q_FALLTHROUGH();
    case QQmlJSScope::AccessSemantics::Value: {
        QQmlJSScope::Ptr listType = QQmlJSScope::create();
        listType->setAccessSemantics(QQmlJSScope::AccessSemantics::Sequence);
        listType->setValueTypeName(elementType->internalName());
        listType->setInternalName(u"QList<%1>"_s.arg(elementType->augmentedInternalName()));
        listType->setFilePath(elementType->filePath());
        const QQmlJSImportedScope element = {elementType, QTypeRevision()};
        QQmlJSScope::resolveTypes(listType, {{elementType->internalName(), element}});
        Q_ASSERT(equals(listType->valueType(), elementType));
        m_typeTracker->listTypes[elementType] = listType;
        return listType;
    }
    default:
        break;
    }
    return QQmlJSScope::ConstPtr();
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::typeFromAST(QQmlJS::AST::Type *type) const
{
    return m_imports[QmlIR::IRBuilder::asString(type->typeId)].scope;
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::typeForConst(QV4::ReturnedValue rv) const
{
    QV4::Value value = QV4::Value::fromReturnedValue(rv);
    if (value.isUndefined())
        return voidType();

    if (value.isInt32())
        return intType();

    if (value.isBoolean())
        return boolType();

    if (value.isDouble())
        return realType();

    if (value.isNull())
        return jsPrimitiveType();

    return {};
}

QQmlJSRegisterContent
QQmlJSTypeResolver::typeForBinaryOperation(QSOperator::Op oper, const QQmlJSRegisterContent &left,
                                           const QQmlJSRegisterContent &right) const
{
    Q_ASSERT(left.isValid());
    Q_ASSERT(right.isValid());

    switch (oper) {
    case QSOperator::Op::Equal:
    case QSOperator::Op::NotEqual:
    case QSOperator::Op::StrictEqual:
    case QSOperator::Op::StrictNotEqual:
    case QSOperator::Op::Lt:
    case QSOperator::Op::Gt:
    case QSOperator::Op::Ge:
    case QSOperator::Op::In:
    case QSOperator::Op::Le:
        return globalType(boolType());
    case QSOperator::Op::BitAnd:
    case QSOperator::Op::BitOr:
    case QSOperator::Op::BitXor:
    case QSOperator::Op::LShift:
    case QSOperator::Op::RShift:
    case QSOperator::Op::URShift:
        return globalType(intType());
    case QSOperator::Op::Add: {
        const auto leftContents = containedType(left);
        const auto rightContents = containedType(right);
        if (equals(leftContents, stringType()) || equals(rightContents, stringType()))
            return QQmlJSRegisterContent::create(stringType(), stringType(),
                                                 QQmlJSRegisterContent::Builtin);

        const QQmlJSScope::ConstPtr result = merge(leftContents, rightContents);
        if (equals(result, boolType()))
            return QQmlJSRegisterContent::create(intType(), intType(),
                                                 QQmlJSRegisterContent::Builtin);
        if (isNumeric(result))
            return QQmlJSRegisterContent::create(realType(), realType(),
                                                 QQmlJSRegisterContent::Builtin);

        return QQmlJSRegisterContent::create(jsPrimitiveType(), jsPrimitiveType(),
                                             QQmlJSRegisterContent::Builtin);
    }
    case QSOperator::Op::Sub: {
        const QQmlJSScope::ConstPtr result = merge(containedType(left), containedType(right));
        if (equals(result, boolType()))
            return QQmlJSRegisterContent::create(intType(), intType(),
                                                 QQmlJSRegisterContent::Builtin);
        if (isNumeric(result))
            return QQmlJSRegisterContent::create(realType(), realType(),
                                                 QQmlJSRegisterContent::Builtin);
        return QQmlJSRegisterContent::create(jsPrimitiveType(), realType(),
                                             QQmlJSRegisterContent::Builtin);
    }
    case QSOperator::Op::Mul:
    case QSOperator::Op::Div:
    case QSOperator::Op::Exp:
    case QSOperator::Op::Mod:
        return QQmlJSRegisterContent::create(
                isNumeric(merge(containedType(left), containedType(right))) ? realType()
                                                                            : jsPrimitiveType(),
                realType(), QQmlJSRegisterContent::Builtin);
    case QSOperator::Op::As:
        return right;
    default:
        break;
    }

    return merge(left, right);
}

QQmlJSRegisterContent QQmlJSTypeResolver::typeForArithmeticUnaryOperation(
        UnaryOperator oper, const QQmlJSRegisterContent &operand) const
{
    // For now, we are only concerned with the unary arithmetic operators.
    // The boolean and bitwise ones are special cased elsewhere.
    Q_UNUSED(oper);

    return QQmlJSRegisterContent::create(isNumeric(operand) ? realType() : jsPrimitiveType(),
                                         realType(), QQmlJSRegisterContent::Builtin);
}

bool QQmlJSTypeResolver::isPrimitive(const QQmlJSRegisterContent &type) const
{
    return isPrimitive(containedType(type));
}

bool QQmlJSTypeResolver::isNumeric(const QQmlJSRegisterContent &type) const
{
    return isNumeric(containedType(type));
}

bool QQmlJSTypeResolver::isPrimitive(const QQmlJSScope::ConstPtr &type) const
{
    return equals(type, m_intType) || equals(type, m_realType) || equals(type, m_floatType)
            || equals(type, m_boolType) || equals(type, m_voidType) || equals(type, m_nullType)
            || equals(type, m_stringType) || equals(type, m_jsPrimitiveType);
}

bool QQmlJSTypeResolver::isNumeric(const QQmlJSScope::ConstPtr &type) const
{
    return searchBaseAndExtensionTypes(
                type, [&](const QQmlJSScope::ConstPtr &scope, BaseOrExtension) {
        return equals(scope, m_numberPrototype);
    });
}

QQmlJSScope::ConstPtr
QQmlJSTypeResolver::containedType(const QQmlJSRegisterContent &container) const
{
    if (container.isType())
        return container.type();
    if (container.isProperty()) {
        const QQmlJSMetaProperty prop = container.property();
        return prop.isList()
                ? listType(prop.type(), UseListReference)
                : QQmlJSScope::ConstPtr(prop.type());
    }
    if (container.isEnumeration())
        return container.enumeration().type();
    if (container.isMethod())
        return container.storedType(); // Methods can only be stored in QJSValue.
    if (container.isImportNamespace()) {
        switch (container.variant()) {
        case QQmlJSRegisterContent::ScopeModulePrefix:
            return container.storedType(); // We don't store scope module prefixes
        case QQmlJSRegisterContent::ObjectModulePrefix:
            return container.scopeType();  // We need to pass the original object through.
        default:
            Q_UNREACHABLE();
        }
    }
    if (container.isConversion())
        return container.conversionResult();

    Q_UNREACHABLE();
    return {};
}

void QQmlJSTypeResolver::trackListPropertyType(
        const QQmlJSScope::ConstPtr &trackedListElementType) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return;

    if (m_typeTracker->trackedTypes.contains(trackedListElementType)
            && !m_typeTracker->listTypes.contains(trackedListElementType)) {
        QQmlJSScope::Ptr clone = QQmlJSScope::clone(m_listPropertyType);
        m_typeTracker->listTypes[trackedListElementType] = clone;
        m_typeTracker->trackedTypes[clone] = { m_listPropertyType, QQmlJSScope::ConstPtr(), clone };
    }
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::trackedType(const QQmlJSScope::ConstPtr &type) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return type;

    // If origin is in fact an already tracked type, track the original of that one instead.
    const auto it = m_typeTracker->trackedTypes.find(type);
    QQmlJSScope::ConstPtr orig = (it == m_typeTracker->trackedTypes.end()) ? type : it->original;

    QQmlJSScope::Ptr clone = QQmlJSScope::clone(orig);
    m_typeTracker->trackedTypes[clone] = { std::move(orig), QQmlJSScope::ConstPtr(), clone };
    return clone;
}

QQmlJSRegisterContent QQmlJSTypeResolver::transformed(
        const QQmlJSRegisterContent &origin,
        QQmlJSScope::ConstPtr (QQmlJSTypeResolver::*op)(const QQmlJSScope::ConstPtr &) const) const
{
    if (origin.isType()) {
        return QQmlJSRegisterContent::create(
                    (this->*op)(origin.storedType()), (this->*op)(origin.type()),
                    origin.variant(), (this->*op)(origin.scopeType()));
    }

    if (origin.isProperty()) {
        QQmlJSMetaProperty prop = origin.property();
        prop.setType((this->*op)(prop.type()));
        if (prop.isList())
            trackListPropertyType(prop.type());
        return QQmlJSRegisterContent::create(
                    (this->*op)(origin.storedType()), prop,
                    origin.variant(), (this->*op)(origin.scopeType()));
    }

    if (origin.isEnumeration()) {
        QQmlJSMetaEnum enumeration = origin.enumeration();
        enumeration.setType((this->*op)(enumeration.type()));
        return QQmlJSRegisterContent::create(
                    (this->*op)(origin.storedType()), enumeration, origin.enumMember(),
                    origin.variant(), (this->*op)(origin.scopeType()));
    }

    if (origin.isMethod()) {
        return QQmlJSRegisterContent::create(
                    (this->*op)(origin.storedType()),  origin.method(), origin.variant(),
                    (this->*op)(origin.scopeType()));
    }

    if (origin.isImportNamespace()) {
        return QQmlJSRegisterContent::create(
                    (this->*op)(origin.storedType()), origin.importNamespace(),
                    origin.variant(), (this->*op)(origin.scopeType()));
    }

    if (origin.isConversion()) {
        return QQmlJSRegisterContent::create(
                    (this->*op)(origin.storedType()), origin.conversionOrigins(),
                    (this->*op)(origin.conversionResult()),
                    origin.variant(), (this->*op)(origin.scopeType()));
    }

    Q_UNREACHABLE();
    return {};
}

QQmlJSRegisterContent QQmlJSTypeResolver::original(const QQmlJSRegisterContent &type) const
{
    return transformed(type, &QQmlJSTypeResolver::originalType);
}

QQmlJSRegisterContent QQmlJSTypeResolver::tracked(const QQmlJSRegisterContent &type) const
{
    return transformed(type, &QQmlJSTypeResolver::trackedType);
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::trackedContainedType(
        const QQmlJSRegisterContent &container) const
{
    const QQmlJSScope::ConstPtr type = containedType(container);
    return m_typeTracker->trackedTypes.contains(type) ? type : QQmlJSScope::ConstPtr();
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::originalContainedType(
        const QQmlJSRegisterContent &container) const
{
    return originalType(containedType(container));
}

void QQmlJSTypeResolver::adjustTrackedType(
        const QQmlJSScope::ConstPtr &tracked, const QQmlJSScope::ConstPtr &conversion) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return;

    const auto it = m_typeTracker->trackedTypes.find(tracked);
    Q_ASSERT(it != m_typeTracker->trackedTypes.end());
    it->replacement = comparableType(conversion);
    *it->clone = std::move(*QQmlJSScope::clone(conversion));
}

void QQmlJSTypeResolver::adjustTrackedType(
        const QQmlJSScope::ConstPtr &tracked, const QList<QQmlJSScope::ConstPtr> &conversions) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return;

    const auto it = m_typeTracker->trackedTypes.find(tracked);
    Q_ASSERT(it != m_typeTracker->trackedTypes.end());
    QQmlJSScope::Ptr mutableTracked = it->clone;
    QQmlJSScope::ConstPtr result;
    for (const QQmlJSScope::ConstPtr &type : conversions)
        result = merge(type, result);

    // If we cannot convert to the new type without the help of e.g. lookupResultMetaType(),
    // we better not change the type.
    if (canPrimitivelyConvertFromTo(tracked, result)) {
        it->replacement = comparableType(result);
        *mutableTracked = std::move(*QQmlJSScope::clone(result));
    }
}

void QQmlJSTypeResolver::generalizeType(const QQmlJSScope::ConstPtr &type) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return;

    const auto it = m_typeTracker->trackedTypes.find(type);
    Q_ASSERT(it != m_typeTracker->trackedTypes.end());
    *it->clone = std::move(*QQmlJSScope::clone(genericType(type)));
    if (it->replacement)
        it->replacement = genericType(it->replacement);
    it->original = genericType(it->original);
}

QString QQmlJSTypeResolver::containedTypeName(const QQmlJSRegisterContent &container,
                                              bool useFancyName) const
{
    QQmlJSScope::ConstPtr type;

    // Use the type proper instead of the attached type
    switch (container.variant()) {
    case QQmlJSRegisterContent::ScopeAttached:
    case QQmlJSRegisterContent::MetaType:
        type = container.scopeType();
        break;
    default:
        type = containedType(container);
        break;
    }

    QString typeName = type->internalName().isEmpty() ? type->baseTypeName() : type->internalName();

    if (useFancyName)
        return QQmlJSScope::prettyName(typeName);

    return typeName;
}

bool QQmlJSTypeResolver::canConvertFromTo(const QQmlJSScope::ConstPtr &from,
                                          const QQmlJSScope::ConstPtr &to) const
{
    if (canPrimitivelyConvertFromTo(from, to))
        return true;

    // ### need a generic solution for custom cpp types:
    // if (from->m_hasBoolOverload && equals(to, boolType))
    //    return true;

    // All of these types have QString conversions that require a certain format
    // TODO: Actually verify these strings or deprecate them.
    //       Some of those type are builtins or should be builtins. We should add code for them
    //       in QQmlJSCodeGenerator::conversion().
    if (equals(from, m_stringType) && !to.isNull()) {
        const QString toTypeName = to->internalName();
        if (toTypeName == u"QTime"_s || toTypeName == u"QDate"_s
                || toTypeName == u"QPoint"_s || toTypeName == u"QPointF"_s
                || toTypeName == u"QSize"_s || toTypeName == u"QSizeF"_s
                || toTypeName == u"QRect"_s || toTypeName == u"QRectF"_s
                || toTypeName == u"QColor"_s) {
            return true;
        }
    }

    return false;
}

bool QQmlJSTypeResolver::canConvertFromTo(const QQmlJSRegisterContent &from,
                                          const QQmlJSRegisterContent &to) const
{
    return canConvertFromTo(containedType(from), containedType(to));
}

static QQmlJSRegisterContent::ContentVariant mergeVariants(QQmlJSRegisterContent::ContentVariant a,
                                                           QQmlJSRegisterContent::ContentVariant b)
{
    return (a == b) ? a : QQmlJSRegisterContent::Unknown;
}

QQmlJSRegisterContent QQmlJSTypeResolver::merge(const QQmlJSRegisterContent &a,
                                                const QQmlJSRegisterContent &b) const
{
    QList<QQmlJSScope::ConstPtr> origins;
    if (a.isConversion())
        origins.append(a.conversionOrigins());
    else
        origins.append(containedType(a));

    if (b.isConversion())
        origins.append(b.conversionOrigins());
    else
        origins.append(containedType(b));

    std::sort(origins.begin(), origins.end());
    const auto erase = std::unique(origins.begin(), origins.end());
    origins.erase(erase, origins.end());

    return QQmlJSRegisterContent::create(
                merge(a.storedType(), b.storedType()),
                origins,
                merge(containedType(a), containedType(b)),
                mergeVariants(a.variant(), b.variant()),
                merge(a.scopeType(), b.scopeType()));
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::merge(const QQmlJSScope::ConstPtr &a,
                                                const QQmlJSScope::ConstPtr &b) const
{
    if (a.isNull())
        return b;

    if (b.isNull())
        return a;

    const auto commonBaseType = [this](
            const QQmlJSScope::ConstPtr &a, const QQmlJSScope::ConstPtr &b) {
        for (QQmlJSScope::ConstPtr aBase = a; aBase; aBase = aBase->baseType()) {
            for (QQmlJSScope::ConstPtr bBase = b; bBase; bBase = bBase->baseType()) {
                if (equals(aBase, bBase))
                    return aBase;
            }
        }

        return QQmlJSScope::ConstPtr();
    };


    if (equals(a, b))
        return a;

    if (equals(a, jsValueType()) || equals(a, varType()))
        return a;
    if (equals(b, jsValueType()) || equals(b, varType()))
        return b;

    auto canConvert = [&](const QQmlJSScope::ConstPtr &from, const QQmlJSScope::ConstPtr &to) {
        return (equals(a, from) && equals(b, to)) || (equals(b, from) && equals(a, to));
    };

    if (isNumeric(a) && isNumeric(b))
        return realType();

    if (canConvert(boolType(), intType()))
        return intType();
    if (canConvert(intType(), stringType()))
        return stringType();
    if (isPrimitive(a) && isPrimitive(b))
        return jsPrimitiveType();

    if (auto commonBase = commonBaseType(a, b))
        return commonBase;

    if (equals(a, nullType()) && b->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
        return b;

    if (equals(b, nullType()) && a->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
        return a;

    return varType();
}

bool QQmlJSTypeResolver::canHoldUndefined(const QQmlJSRegisterContent &content) const
{
    const auto canBeUndefined = [this](const QQmlJSScope::ConstPtr &type) {
        return equals(type, m_voidType) || equals(type, m_varType)
                || equals(type, m_jsValueType) || equals(type, m_jsPrimitiveType);
    };

    if (!canBeUndefined(content.storedType()))
        return false;

    if (!content.isConversion())
        return canBeUndefined(containedType(content));

    const auto origins = content.conversionOrigins();
    for (const auto &origin : origins) {
        if (canBeUndefined(origin))
            return true;
    }

    return false;
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::genericType(const QQmlJSScope::ConstPtr &type,
                                                      ComponentIsGeneric allowComponent) const
{
    if (type->isScript())
        return m_jsValueType;

    if (equals(type, m_metaObjectType))
        return m_metaObjectType;

    if (type->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
        for (auto base = type; base; base = base->baseType()) {
            // QObject and QQmlComponent are the two required base types.
            // Any QML type system has to define those, or use the ones from builtins.
            // As QQmlComponent is derived from QObject, we can restrict ourselves to the latter.
            // This results in less if'ery when retrieving a QObject* from somewhere and deciding
            // what it is.
            if (base->internalName() == u"QObject"_s) {
                return base;
            } else if (allowComponent == ComponentIsGeneric::Yes
                       && base->internalName() == u"QQmlComponent"_s) {
                return base;
            }
        }

        m_logger->log(u"Object type %1 is not derived from QObject or QQmlComponent"_s.arg(
                              type->internalName()),
                      Log_Compiler, type->sourceLocation());

        // Reference types that are not QObject or QQmlComponent are likely JavaScript objects.
        // We don't want to deal with those, but m_jsValueType is the best generic option.
        return m_jsValueType;
    }

    if (isPrimitive(type) || equals(type, m_jsValueType) || equals(type, m_listPropertyType)
            || equals(type, m_urlType) || equals(type, m_dateTimeType)
            || equals(type, m_variantListType) || equals(type, m_varType)
            || equals(type, m_stringListType) || equals(type, m_emptyListType)) {
        return type;
    }

    if (isNumeric(type))
        return m_realType;

    if (type->scopeType() == QQmlJSScope::EnumScope)
        return m_intType;

    if (type->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence) {
        return equals(type, m_listPropertyType)
                ? type
                : listType(genericType(type->valueType()), UseQObjectList);
    }

    return m_varType;
}

QQmlJSRegisterContent QQmlJSTypeResolver::globalType(const QQmlJSScope::ConstPtr &type) const
{
    return QQmlJSRegisterContent::create(storedType(type), type, QQmlJSRegisterContent::Unknown);
}

static QQmlJSRegisterContent::ContentVariant
scopeContentVariant(QQmlJSTypeResolver::BaseOrExtension mode, bool isMethod)
{
    switch (mode) {
    case QQmlJSTypeResolver::Base:
        return isMethod ? QQmlJSRegisterContent::ScopeMethod : QQmlJSRegisterContent::ScopeProperty;
    case QQmlJSTypeResolver::Extension:
        return isMethod ? QQmlJSRegisterContent::ExtensionScopeMethod
                        : QQmlJSRegisterContent::ExtensionScopeProperty;
    }
    Q_UNREACHABLE();
    return QQmlJSRegisterContent::Unknown;
}

static bool isRevisionAllowed(int memberRevision, const QQmlJSScope::ConstPtr &scope)
{
    Q_ASSERT(scope->isComposite());
    const QTypeRevision revision = QTypeRevision::fromEncodedVersion(memberRevision);

    // If the memberRevision is either invalid or 0.0, then everything is allowed.
    if (!revision.isValid() || revision == QTypeRevision::zero())
        return true;

    const QTypeRevision typeRevision = QQmlJSScope::nonCompositeBaseRevision(
                {scope->baseType(), scope->baseTypeRevision()});

    // If the revision is not valid, we haven't found a non-composite base type.
    // There is nothing we can say about the property then.
    return typeRevision.isValid() && typeRevision >= revision;
}

QQmlJSRegisterContent QQmlJSTypeResolver::scopedType(const QQmlJSScope::ConstPtr &scope,
                                                     const QString &name) const
{
    const auto isAssignedToDefaultProperty = [this](
            const QQmlJSScope::ConstPtr &parent, const QQmlJSScope::ConstPtr &child) {
        QString defaultPropertyName;
        QQmlJSMetaProperty defaultProperty;
        if (!searchBaseAndExtensionTypes(
                    parent, [&](const QQmlJSScope::ConstPtr &scope,
                                QQmlJSTypeResolver::BaseOrExtension mode) {
            Q_UNUSED(mode);
            defaultPropertyName = scope->defaultPropertyName();
            defaultProperty = scope->property(defaultPropertyName);
            return !defaultPropertyName.isEmpty();
        })) {
            return false;
        }

        QQmlJSScope::ConstPtr bindingHolder = parent;
        while (bindingHolder->property(defaultPropertyName) != defaultProperty) {
            // Only traverse the base type hierarchy here, not the extended types.
            // Extensions cannot hold bindings.
            bindingHolder = bindingHolder->baseType();

            // Consequently, the default property may be inaccessibly
            // hidden in some extension via shadowing.
            // Nothing can be assigned to it then.
            if (!bindingHolder)
                return false;
        }

        const QList<QQmlJSMetaPropertyBinding> defaultPropBindings
                = bindingHolder->propertyBindings(defaultPropertyName);
        for (const QQmlJSMetaPropertyBinding &binding : defaultPropBindings) {
            if (binding.bindingType() == QQmlJSMetaPropertyBinding::Object
                && equals(binding.objectType(), child)) {
                return true;
            }
        }
        return false;
    };


    if (QQmlJSScope::ConstPtr identified = scopeForId(name, scope)) {
        return QQmlJSRegisterContent::create(storedType(identified), identified,
                                             QQmlJSRegisterContent::ObjectById, scope);
    }

    if (QQmlJSScope::ConstPtr base = QQmlJSScope::findCurrentQMLScope(scope)) {
        QQmlJSRegisterContent result;
        if (searchBaseAndExtensionTypes(
                    base, [&](const QQmlJSScope::ConstPtr &found, BaseOrExtension mode) {
                        if (found->hasOwnProperty(name)) {
                            QQmlJSMetaProperty prop = found->ownProperty(name);
                            if (!isRevisionAllowed(prop.revision(), scope))
                                return false;
                            if (m_parentMode == UseDocumentParent
                                    && name == base->parentPropertyName()) {
                                QQmlJSScope::ConstPtr baseParent = base->parentScope();
                                if (baseParent && baseParent->inherits(prop.type())
                                        && isAssignedToDefaultProperty(baseParent, base)) {
                                    prop.setType(baseParent);
                                }
                            }
                            result = QQmlJSRegisterContent::create(
                                    prop.isList() ? listPropertyType() : storedType(prop.type()),
                                    prop, scopeContentVariant(mode, false), scope);
                            return true;
                        }

                        if (found->hasOwnMethod(name)) {
                            auto methods = found->ownMethods(name);
                            for (auto it = methods.begin(); it != methods.end();) {
                                if (!isRevisionAllowed(it->revision(), scope))
                                    it = methods.erase(it);
                                else
                                    ++it;
                            }
                            if (methods.isEmpty())
                                return false;
                            result = QQmlJSRegisterContent::create(
                                    jsValueType(), methods, scopeContentVariant(mode, true), scope);
                            return true;
                        }

                        // Unqualified enums are not allowed

                        return false;
                    })) {
            return result;
        }
    }

    if (QQmlJSScope::ConstPtr type = typeForName(name)) {
        if (type->isSingleton())
            return QQmlJSRegisterContent::create(storedType(type), type,
                                                 QQmlJSRegisterContent::Singleton);

        if (type->isScript())
            return QQmlJSRegisterContent::create(storedType(type), type,
                                                 QQmlJSRegisterContent::Script);

        if (const auto attached = type->attachedType()) {
            if (!genericType(attached)) {
                m_logger->log(u"Cannot resolve generic base of attached %1"_s.arg(
                                      attached->internalName()),
                              Log_Compiler, attached->sourceLocation());
                return {};
            } else if (type->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
                m_logger->log(u"Cannot retrieve attached object for non-reference type %1"_s.arg(
                                      type->internalName()),
                              Log_Compiler, type->sourceLocation());
                return {};
            } else {
                // We don't know yet whether we need the attached or the plain object. In direct
                // mode, we will figure this out using the scope type and access any enums of the
                // plain type directly. In indirect mode, we can use enum lookups.
                return QQmlJSRegisterContent::create(storedType(attached), attached,
                                                     QQmlJSRegisterContent::ScopeAttached, type);
            }
        }

        // A plain reference to a non-singleton, non-attached type.
        // If it's undefined, we can actually get an "instance" of it.
        // Therefore, use a primitive value to store it.
        // Otherwise this is a plain type reference without instance.
        // We may still need the plain type reference for enum lookups,
        // so store it in QJSValue for now.
        return QQmlJSRegisterContent::create(metaObjectType(), metaObjectType(),
                                             QQmlJSRegisterContent::MetaType, type);
    }

    if (m_jsGlobalObject->hasProperty(name)) {
        return QQmlJSRegisterContent::create(jsValueType(), m_jsGlobalObject->property(name),
                                             QQmlJSRegisterContent::JavaScriptGlobal,
                                             m_jsGlobalObject);
    } else if (m_jsGlobalObject->hasMethod(name)) {
        return QQmlJSRegisterContent::create(jsValueType(), m_jsGlobalObject->methods(name),
                                             QQmlJSRegisterContent::JavaScriptGlobal,
                                             m_jsGlobalObject);
    }

    return {};
}

bool QQmlJSTypeResolver::checkEnums(const QQmlJSScope::ConstPtr &scope, const QString &name,
                                    QQmlJSRegisterContent *result, BaseOrExtension mode) const
{
    // You can't have lower case enum names in QML, even if we know the enums here.
    if (name.isEmpty() || !name.at(0).isUpper())
        return false;

    const auto enums = scope->ownEnumerations();
    for (const auto &enumeration : enums) {
        if (enumeration.name() == name) {
            *result = QQmlJSRegisterContent::create(
                    storedType(intType()), enumeration, QString(),
                    mode == Extension ? QQmlJSRegisterContent::ExtensionObjectEnum
                                      : QQmlJSRegisterContent::ObjectEnum,
                    scope);
            return true;
        }

        if (enumeration.hasKey(name)) {
            *result = QQmlJSRegisterContent::create(
                    storedType(intType()), enumeration, name,
                    mode == Extension ? QQmlJSRegisterContent::ExtensionObjectEnum
                                      : QQmlJSRegisterContent::ObjectEnum,
                    scope);
            return true;
        }
    }

    return false;
}

bool QQmlJSTypeResolver::canPrimitivelyConvertFromTo(
        const QQmlJSScope::ConstPtr &from, const QQmlJSScope::ConstPtr &to) const
{
    if (equals(from, to))
        return true;
    if (equals(from, m_varType) || equals(to, m_varType))
        return true;
    if (equals(from, m_jsValueType) || equals(to, m_jsValueType))
        return true;
    if (isNumeric(from) && isNumeric(to))
        return true;
    if (isNumeric(from) && equals(to, m_boolType))
        return true;
    if (from->accessSemantics() == QQmlJSScope::AccessSemantics::Reference
            && equals(to, m_boolType)) {
        return true;
    }

    // Yes, our String has number constructors.
    if (isNumeric(from) && equals(to, m_stringType))
        return true;

    // We can always convert between strings and urls.
    if ((equals(from, m_stringType) && equals(to, m_urlType))
            || (equals(from, m_urlType) && equals(to, m_stringType))) {
        return true;
    }

    if (equals(from, m_voidType) || equals(to, m_voidType))
        return true;

    if (to.isNull())
        return false;

    if (equals(from, m_stringType) && equals(to, m_dateTimeType))
        return true;

    if (equals(from, m_nullType)
            && to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
        return true;
    }

    if (equals(from, m_jsPrimitiveType)) {
        // You can cast any primitive to a nullptr
        return isPrimitive(to) || to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference;
    }

    if (equals(to, m_jsPrimitiveType))
        return isPrimitive(from);

    if (equals(from, m_emptyListType) || equals(from, m_variantListType))
        return to->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence;

    const bool matchByName = !to->isComposite();
    Q_ASSERT(!matchByName || !to->internalName().isEmpty());
    for (auto baseType = from; baseType; baseType = baseType->baseType()) {
        if (equals(baseType, to))
            return true;
        if (matchByName && baseType->internalName() == to->internalName())
            return true;
    }

    return false;
}

QQmlJSRegisterContent QQmlJSTypeResolver::lengthProperty(
        bool isWritable, const QQmlJSScope::ConstPtr &scope) const
{
    QQmlJSMetaProperty prop;
    prop.setPropertyName(u"length"_s);
    prop.setTypeName(u"int"_s);
    prop.setType(intType());
    prop.setIsWritable(isWritable);
    return QQmlJSRegisterContent::create(intType(), prop, QQmlJSRegisterContent::Builtin, scope);
}

QQmlJSRegisterContent QQmlJSTypeResolver::memberType(const QQmlJSScope::ConstPtr &type,
                                                     const QString &name) const
{
    QQmlJSRegisterContent result;

    if (equals(type, jsValueType())) {
        QQmlJSMetaProperty prop;
        prop.setPropertyName(name);
        prop.setTypeName(u"QJSValue"_s);
        prop.setType(jsValueType());
        prop.setIsWritable(true);
        return QQmlJSRegisterContent::create(jsValueType(), prop,
                                             QQmlJSRegisterContent::JavaScriptObjectProperty, type);
    }

    if ((equals(type, stringType())
         || type->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence)
            && name == u"length"_s) {
        return lengthProperty(!equals(type, stringType()), type);
    }

    const auto check = [&](const QQmlJSScope::ConstPtr &scope, BaseOrExtension mode) {
        if (scope->hasOwnProperty(name)) {
            const auto prop = scope->ownProperty(name);
            result = QQmlJSRegisterContent::create(
                    prop.isList() ? listPropertyType() : storedType(prop.type()), prop,
                    mode == Base ? QQmlJSRegisterContent::ObjectProperty
                                 : QQmlJSRegisterContent::ExtensionObjectProperty,
                    scope);
            return true;
        }

        if (scope->hasOwnMethod(name)) {
            const auto methods = scope->ownMethods(name);
            result = QQmlJSRegisterContent::create(
                    jsValueType(), methods,
                    mode == Base ? QQmlJSRegisterContent::ObjectMethod
                                 : QQmlJSRegisterContent::ExtensionObjectMethod,
                    scope);
            return true;
        }

        if (std::optional<QQmlJSScope::JavaScriptIdentifier> identifier =
                    scope->findJSIdentifier(name);
            identifier.has_value()) {
            QQmlJSMetaProperty prop;
            prop.setPropertyName(name);
            prop.setTypeName(u"QJSValue"_s);
            prop.setType(jsValueType());
            prop.setIsWritable(!identifier->isConst);

            result = QQmlJSRegisterContent::create(jsValueType(), prop,
                                                   QQmlJSRegisterContent::JavaScriptObject, type);
            return true;
        }

        return checkEnums(scope, name, &result, mode);
    };

    if (searchBaseAndExtensionTypes(type, check))
        return result;

    if (QQmlJSScope::ConstPtr attachedBase = typeForName(name)) {
        if (QQmlJSScope::ConstPtr attached = attachedBase->attachedType()) {
            if (!genericType(attached)) {
                m_logger->log(u"Cannot resolve generic base of attached %1"_s.arg(
                                      attached->internalName()),
                              Log_Compiler, attached->sourceLocation());
                return {};
            } else if (type->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
                m_logger->log(u"Cannot retrieve attached object for non-reference type %1"_s.arg(
                                      type->internalName()),
                              Log_Compiler, type->sourceLocation());
                return {};
            } else {
                return QQmlJSRegisterContent::create(storedType(attached), attached,
                                                     QQmlJSRegisterContent::ObjectAttached,
                                                     attachedBase);
            }
        }
    }

    return {};
}

QQmlJSRegisterContent QQmlJSTypeResolver::memberEnumType(const QQmlJSScope::ConstPtr &type,
                                                         const QString &name) const
{
    QQmlJSRegisterContent result;

    if (searchBaseAndExtensionTypes(type,
                                    [&](const QQmlJSScope::ConstPtr &scope, BaseOrExtension mode) {
                                        return checkEnums(scope, name, &result, mode);
                                    })) {
        return result;
    }

    return {};
}

QQmlJSRegisterContent QQmlJSTypeResolver::memberType(const QQmlJSRegisterContent &type,
                                                     const QString &name) const
{
    if (type.isType()) {
        const auto content = type.type();
        const auto result = memberType(content, name);
        if (result.isValid())
            return result;

        // If we didn't find anything and it's an attached type,
        // we might have an enum of the attaching type.
        return memberEnumType(type.scopeType(), name);
    }
    if (type.isProperty()) {
        const auto prop = type.property();
        if (prop.isList() && name == u"length"_s)
            return lengthProperty(true, listPropertyType());
        return memberType(prop.type(), name);
    }
    if (type.isEnumeration()) {
        const auto enumeration = type.enumeration();
        if (!type.enumMember().isEmpty() || !enumeration.hasKey(name))
            return {};
        return QQmlJSRegisterContent::create(storedType(intType()), enumeration, name,
                                             QQmlJSRegisterContent::ObjectEnum, type.scopeType());
    }
    if (type.isMethod()) {
        QQmlJSMetaProperty prop;
        prop.setTypeName(u"QJSValue"_s);
        prop.setPropertyName(name);
        prop.setType(jsValueType());
        prop.setIsWritable(true);
        return QQmlJSRegisterContent::create(jsValueType(), prop,
                                             QQmlJSRegisterContent::JavaScriptObjectProperty,
                                             jsValueType());
    }
    if (type.isImportNamespace()) {
        if (type.scopeType()->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
            m_logger->log(
                    u"Cannot use non-reference type %1 as base of namespaced attached type"_s.arg(
                            type.scopeType()->internalName()),
                    Log_Type, type.scopeType()->sourceLocation());
            return {};
        }

        if (QQmlJSScope::ConstPtr result = typeForName(name)) {
            QQmlJSScope::ConstPtr attached = result->attachedType();
            if (attached && genericType(attached)) {
                return QQmlJSRegisterContent::create(
                            storedType(attached), attached,
                            type.variant() == QQmlJSRegisterContent::ObjectModulePrefix
                                ? QQmlJSRegisterContent::ObjectAttached
                                : QQmlJSRegisterContent::ScopeAttached,
                            result);
            }

            if (result->isSingleton()) {
                return QQmlJSRegisterContent::create(
                            storedType(result), result,
                            QQmlJSRegisterContent::Singleton, type.scopeType());
            }

            return QQmlJSRegisterContent::create(metaObjectType(), metaObjectType(),
                                                 QQmlJSRegisterContent::MetaType, result);
        }

        return {};
    }
    if (type.isConversion()) {
        const auto result = memberType(type.conversionResult(), name);
        return result.isValid() ? result : memberEnumType(type.scopeType(), name);
    }

    Q_UNREACHABLE();
    return {};
}

QQmlJSRegisterContent QQmlJSTypeResolver::valueType(const QQmlJSRegisterContent &listType) const
{
    QQmlJSScope::ConstPtr scope;
    QQmlJSScope::ConstPtr value;

    auto valueType = [this](const QQmlJSScope::ConstPtr &scope) {
        if (scope->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence)
            return scope->valueType();
        else if (equals(scope, m_jsValueType) || equals(scope, m_varType))
            return m_jsValueType;
        return QQmlJSScope::ConstPtr();
    };

    if (listType.isType()) {
        scope = listType.type();
        value = valueType(scope);
    } else if (listType.isConversion()) {
        value = valueType(listType.conversionResult());
    } else if (listType.isProperty()) {
        const auto prop = listType.property();
        if (prop.isList()) {
            scope = m_listPropertyType;
            value = prop.type();
        } else {
            scope = prop.type();
            value = valueType(scope);
        }
    }

    if (value.isNull())
        return {};

    QQmlJSMetaProperty property;
    property.setPropertyName(u"[]"_s);
    property.setTypeName(value->internalName());
    property.setType(value);

    QQmlJSScope::ConstPtr stored;

    // Special handling of stored type here: List lookup can always produce undefined
    if (isPrimitive(value))
        stored = jsPrimitiveType();
    else
        stored = jsValueType();

    return QQmlJSRegisterContent::create(stored, property, QQmlJSRegisterContent::ListValue, scope);
}

QQmlJSRegisterContent QQmlJSTypeResolver::returnType(
        const QQmlJSScope::ConstPtr &type, QQmlJSRegisterContent::ContentVariant variant) const
{
    Q_ASSERT(variant == QQmlJSRegisterContent::MethodReturnValue
             || variant == QQmlJSRegisterContent::JavaScriptReturnValue);
    return QQmlJSRegisterContent::create(storedType(type), type, variant);
}

bool QQmlJSTypeResolver::registerIsStoredIn(
        const QQmlJSRegisterContent &reg, const QQmlJSScope::ConstPtr &type) const
{
    return equals(reg.storedType(), type);
}

bool QQmlJSTypeResolver::registerContains(const QQmlJSRegisterContent &reg,
                                          const QQmlJSScope::ConstPtr &type) const
{
    if (reg.isType())
        return equals(reg.type(), type);
    if (reg.isConversion())
        return equals(reg.conversionResult(), type);
    if (reg.isProperty()) {
        const auto prop = reg.property();
        return prop.isList() ? equals(type, listPropertyType()) : equals(prop.type(), type);
    }
    if (reg.isEnumeration())
        return equals(type, intType());
    if (reg.isMethod())
        return equals(type, jsValueType());
    return false;
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::storedType(const QQmlJSScope::ConstPtr &type) const
{
    if (type.isNull())
        return {};
    if (equals(type, voidType()))
        return type;
    if (type->isScript())
        return jsValueType();
    if (type->isComposite()) {
        if (const QQmlJSScope::ConstPtr nonComposite = QQmlJSScope::nonCompositeBaseType(type))
            return nonComposite;

        // If we can't find the non-composite base, we really don't know what it is.
        return genericType(type);
    }
    if (type->filePath().isEmpty())
        return genericType(type);
    return type;
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::originalType(const QQmlJSScope::ConstPtr &type) const
{
    const auto it = m_typeTracker->trackedTypes.find(type);
    return it == m_typeTracker->trackedTypes.end() ? type : it->original;
}

/*!
 * \internal
 *
 * Compares the origin types of \a a and \a b. A straight a == b would compare the identity
 * of the pointers. However, since we clone types to keep track of them, we need a separate
 * way to compare the clones. Usually you'd do *a == *b for that, but as QQmlJSScope is rather
 * large, we offer an optimization here that uses the type tracking we already have in place.
 */
bool QQmlJSTypeResolver::equals(const QQmlJSScope::ConstPtr &a, const QQmlJSScope::ConstPtr &b) const
{
    return comparableType(a) == comparableType(b);
}

QQmlJSRegisterContent QQmlJSTypeResolver::convert(
        const QQmlJSRegisterContent &from, const QQmlJSRegisterContent &to) const
{
    if (from.isConversion()) {
        return QQmlJSRegisterContent::create(
                    to.storedType(), from.conversionOrigins(), containedType(to), from.variant(),
                    from.scopeType());
    }

    return QQmlJSRegisterContent::create(
                to.storedType(), QList<QQmlJSScope::ConstPtr>{containedType(from)},
                containedType(to), from.variant(), from.scopeType());
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::comparableType(const QQmlJSScope::ConstPtr &type) const
{
    const auto it = m_typeTracker->trackedTypes.constFind(type);
    if (it == m_typeTracker->trackedTypes.constEnd())
        return type;
    return it->replacement ? it->replacement : it->original;
}

QT_END_NAMESPACE
