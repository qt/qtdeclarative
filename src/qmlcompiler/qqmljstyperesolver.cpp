// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljstyperesolver_p.h"

#include "qqmljsimporter_p.h"
#include "qqmljsimportvisitor_p.h"
#include "qqmljslogger_p.h"
#include "qqmljsutils_p.h"
#include <private/qv4value_p.h>

#include <private/qduplicatetracker_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(lcTypeResolver, "qt.qml.compiler.typeresolver", QtInfoMsg);

static inline void assertExtension(const QQmlJSScope::ConstPtr &type, QLatin1String extension)
{
    Q_ASSERT(type);
    Q_ASSERT(type->extensionType().scope->internalName() == extension);
    Q_ASSERT(type->extensionIsJavaScript());
}

QQmlJSTypeResolver::QQmlJSTypeResolver(QQmlJSImporter *importer)
    : m_pool(std::make_unique<QQmlJSRegisterContentPool>())
    , m_imports(importer->builtinInternalNames())
{
    const QQmlJSImporter::ImportedTypes &builtinTypes = m_imports;

    m_voidType = builtinTypes.type(u"void"_s).scope;
    assertExtension(m_voidType, "undefined"_L1);

    m_nullType = builtinTypes.type(u"std::nullptr_t"_s).scope;
    Q_ASSERT(m_nullType);

    m_realType = builtinTypes.type(u"double"_s).scope;
    assertExtension(m_realType, "Number"_L1);

    m_floatType = builtinTypes.type(u"float"_s).scope;
    assertExtension(m_floatType, "Number"_L1);

    m_int8Type = builtinTypes.type(u"qint8"_s).scope;
    assertExtension(m_int8Type, "Number"_L1);

    m_uint8Type = builtinTypes.type(u"quint8"_s).scope;
    assertExtension(m_uint8Type, "Number"_L1);

    m_int16Type = builtinTypes.type(u"short"_s).scope;
    assertExtension(m_int16Type, "Number"_L1);

    m_uint16Type = builtinTypes.type(u"ushort"_s).scope;
    assertExtension(m_uint16Type, "Number"_L1);

    m_int32Type = builtinTypes.type(u"int"_s).scope;
    assertExtension(m_int32Type, "Number"_L1);

    m_uint32Type = builtinTypes.type(u"uint"_s).scope;
    assertExtension(m_uint32Type, "Number"_L1);

    m_int64Type = builtinTypes.type(u"qlonglong"_s).scope;
    Q_ASSERT(m_int64Type);

    m_uint64Type = builtinTypes.type(u"qulonglong"_s).scope;
    Q_ASSERT(m_uint64Type);

    m_sizeType = builtinTypes.type(u"qsizetype"_s).scope;
    assertExtension(m_sizeType, "Number"_L1);

    // qsizetype is either a 32bit or a 64bit signed integer. We don't want to special-case it.
    Q_ASSERT(m_sizeType == m_int32Type || m_sizeType == m_int64Type);

    m_boolType = builtinTypes.type(u"bool"_s).scope;
    assertExtension(m_boolType, "Boolean"_L1);

    m_stringType = builtinTypes.type(u"QString"_s).scope;
    assertExtension(m_stringType, "String"_L1);

    m_stringListType = builtinTypes.type(u"QStringList"_s).scope;
    assertExtension(m_stringListType, "Array"_L1);

    m_byteArrayType = builtinTypes.type(u"QByteArray"_s).scope;
    assertExtension(m_byteArrayType, "ArrayBuffer"_L1);

    m_urlType = builtinTypes.type(u"QUrl"_s).scope;
    assertExtension(m_urlType, "URL"_L1);

    m_dateTimeType = builtinTypes.type(u"QDateTime"_s).scope;
    assertExtension(m_dateTimeType, "Date"_L1);

    m_dateType = builtinTypes.type(u"QDate"_s).scope;
    Q_ASSERT(m_dateType);

    m_timeType = builtinTypes.type(u"QTime"_s).scope;
    Q_ASSERT(m_timeType);

    m_regexpType = builtinTypes.type(u"QRegularExpression"_s).scope;
    Q_ASSERT(m_regexpType);

    m_variantListType = builtinTypes.type(u"QVariantList"_s).scope;
    assertExtension(m_variantListType, "Array"_L1);

    m_variantMapType = builtinTypes.type(u"QVariantMap"_s).scope;
    Q_ASSERT(m_variantMapType);
    m_varType = builtinTypes.type(u"QVariant"_s).scope;
    Q_ASSERT(m_varType);

    m_jsValueType = builtinTypes.type(u"QJSValue"_s).scope;
    Q_ASSERT(m_jsValueType);

    m_qObjectType = builtinTypes.type(u"QObject"_s).scope;
    assertExtension(m_qObjectType, "Object"_L1);

    m_qObjectListType = builtinTypes.type(u"QObjectList"_s).scope;
    assertExtension(m_qObjectListType, "Array"_L1);

    m_qQmlScriptStringType = builtinTypes.type(u"QQmlScriptString"_s).scope;
    Q_ASSERT(m_qQmlScriptStringType);

    m_functionType = builtinTypes.type(u"function"_s).scope;
    Q_ASSERT(m_functionType);

    m_numberPrototype = builtinTypes.type(u"NumberPrototype"_s).scope;
    Q_ASSERT(m_numberPrototype);

    m_arrayPrototype = builtinTypes.type(u"ArrayPrototype"_s).scope;
    Q_ASSERT(m_arrayPrototype);

    m_listPropertyType = m_qObjectType->listType();
    Q_ASSERT(m_listPropertyType->internalName() == u"QQmlListProperty<QObject>"_s);
    Q_ASSERT(m_listPropertyType->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence);
    Q_ASSERT(m_listPropertyType->valueTypeName() == u"QObject"_s);
    assertExtension(m_listPropertyType, "Array"_L1);

    QQmlJSScope::Ptr emptyType = QQmlJSScope::create();
    emptyType->setAccessSemantics(QQmlJSScope::AccessSemantics::None);
    m_emptyType = emptyType;

    QQmlJSScope::Ptr jsPrimitiveType = QQmlJSScope::create();
    jsPrimitiveType->setInternalName(u"QJSPrimitiveValue"_s);
    jsPrimitiveType->setFilePath(u"qjsprimitivevalue.h"_s);
    jsPrimitiveType->setAccessSemantics(QQmlJSScope::AccessSemantics::Value);
    m_jsPrimitiveType = jsPrimitiveType;

    QQmlJSScope::Ptr metaObjectType = QQmlJSScope::create();
    metaObjectType->setInternalName(u"const QMetaObject"_s);
    metaObjectType->setFilePath(u"qmetaobject.h"_s);
    metaObjectType->setAccessSemantics(QQmlJSScope::AccessSemantics::Reference);
    m_metaObjectType = metaObjectType;

    m_jsGlobalObject = importer->jsGlobalObject();

    QQmlJSScope::Ptr forInIteratorPtr = QQmlJSScope::create();
    forInIteratorPtr->setAccessSemantics(QQmlJSScope::AccessSemantics::Value);
    forInIteratorPtr->setFilePath(u"qjslist.h"_s);
    forInIteratorPtr->setInternalName(u"QJSListForInIterator::Ptr"_s);
    m_forInIteratorPtr = forInIteratorPtr;

    QQmlJSScope::Ptr forOfIteratorPtr = QQmlJSScope::create();
    forOfIteratorPtr->setAccessSemantics(QQmlJSScope::AccessSemantics::Value);
    forOfIteratorPtr->setFilePath(u"qjslist.h"_s);
    forOfIteratorPtr->setInternalName(u"QJSListForOfIterator::Ptr"_s);
    m_forOfIteratorPtr = forOfIteratorPtr;

    // We use this as scope type quite often, and it should always be the same scope type.
    m_jsGlobalObjectContent = m_pool->createType(
            m_jsGlobalObject, QQmlJSRegisterContent::InvalidLookupIndex,
            QQmlJSRegisterContent::ScopeObject);
}

/*!
    \internal

    Initializes the type resolver. As part of that initialization, makes \a
    visitor traverse the program when given.
*/
void QQmlJSTypeResolver::init(QQmlJSImportVisitor *visitor, QQmlJS::AST::Node *program)
{
    m_logger = visitor->logger();

    m_objectsById.clear();
    m_objectsByLocation.clear();
    m_imports.clear();
    m_signalHandlers.clear();

    if (program)
        program->accept(visitor);

    m_objectsById = visitor->addressableScopes();
    m_objectsByLocation = visitor->scopesBylocation();
    m_signalHandlers = visitor->signalHandlers();
    m_imports = visitor->imports();
    m_seenModuleQualifiers = visitor->seenModuleQualifiers();
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::mathObject() const
{
    return jsGlobalObject()->property(u"Math"_s).type();
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::consoleObject() const
{
    return jsGlobalObject()->property(u"console"_s).type();
}

QQmlJSScope::ConstPtr
QQmlJSTypeResolver::scopeForLocation(const QV4::CompiledData::Location &location) const
{
    // #if required for standalone DOM compilation against Qt 6.2
    qCDebug(lcTypeResolver()).nospace()
            << "looking for object at " << location.line() << ':' << location.column();
    return m_objectsByLocation[location];
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::typeFromAST(QQmlJS::AST::Type *type) const
{
    const QString typeId = QmlIR::IRBuilder::asString(type->typeId);
    if (!type->typeArgument)
        return m_imports.type(typeId).scope;
    if (typeId == u"list"_s) {
        if (const QQmlJSScope::ConstPtr typeArgument = typeForName(type->typeArgument->toString()))
            return typeArgument->listType();
    }
    return QQmlJSScope::ConstPtr();
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::typeForConst(QV4::ReturnedValue rv) const
{
    QV4::Value value = QV4::Value::fromReturnedValue(rv);
    if (value.isUndefined())
        return voidType();

    if (value.isInt32())
        return int32Type();

    if (value.isBoolean())
        return boolType();

    if (value.isDouble())
        return realType();

    if (value.isNull())
        return nullType();

    if (value.isEmpty())
        return emptyType();

    return {};
}

QQmlJSRegisterContent
QQmlJSTypeResolver::typeForBinaryOperation(QSOperator::Op oper, QQmlJSRegisterContent left,
                                           QQmlJSRegisterContent right) const
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
        return operationType(boolType());
    case QSOperator::Op::BitAnd:
    case QSOperator::Op::BitOr:
    case QSOperator::Op::BitXor:
    case QSOperator::Op::LShift:
    case QSOperator::Op::RShift:
        return operationType(int32Type());
    case QSOperator::Op::URShift:
        return operationType(uint32Type());
    case QSOperator::Op::Add: {
        const auto leftContents = left.containedType();
        const auto rightContents = right.containedType();
        if (leftContents == stringType() || rightContents == stringType())
            return operationType(stringType());

        const QQmlJSScope::ConstPtr result = merge(leftContents, rightContents);
        if (result == boolType())
            return operationType(int32Type());
        if (isNumeric(result))
            return operationType(realType());

        return operationType(jsPrimitiveType());
    }
    case QSOperator::Op::Sub:
    case QSOperator::Op::Mul:
    case QSOperator::Op::Exp: {
        const QQmlJSScope::ConstPtr result = merge(left.containedType(), right.containedType());
        return operationType(result == boolType() ? int32Type() : realType());
    }
    case QSOperator::Op::Div:
    case QSOperator::Op::Mod:
        return operationType(realType());
    case QSOperator::Op::As:
        return operationType(right.containedType());
    default:
        break;
    }

    return operationType(merge(left.containedType(), right.containedType()));
}

QQmlJSRegisterContent QQmlJSTypeResolver::typeForArithmeticUnaryOperation(
        UnaryOperator op, QQmlJSRegisterContent operand) const
{
    switch (op) {
    case UnaryOperator::Not:
        return operationType(boolType());
    case UnaryOperator::Complement:
        return operationType(int32Type());
    case UnaryOperator::Plus:
        if (isIntegral(operand))
            return operationType(operand.containedType());
        Q_FALLTHROUGH();
    default:
        if (operand.containedType() == boolType())
            return operationType(int32Type());
        break;
    }

    return operationType(realType());
}

bool QQmlJSTypeResolver::isPrimitive(QQmlJSRegisterContent type) const
{
    return isPrimitive(type.containedType());
}

bool QQmlJSTypeResolver::isNumeric(QQmlJSRegisterContent type) const
{
    return isNumeric(type.containedType());
}

bool QQmlJSTypeResolver::isIntegral(QQmlJSRegisterContent type) const
{
    return isIntegral(type.containedType());
}

bool QQmlJSTypeResolver::isIntegral(const QQmlJSScope::ConstPtr &type) const
{
    return isSignedInteger(type) || isUnsignedInteger(type);
}

bool QQmlJSTypeResolver::isPrimitive(const QQmlJSScope::ConstPtr &type) const
{
    return (isNumeric(type) && type != m_int64Type && type != m_uint64Type)
            || type == m_boolType   || type == m_voidType || type == m_nullType
            || type == m_stringType || type == m_jsPrimitiveType;
}

bool QQmlJSTypeResolver::isNumeric(const QQmlJSScope::ConstPtr &type) const
{
    return QQmlJSUtils::searchBaseAndExtensionTypes(
            type, [&](const QQmlJSScope::ConstPtr &scope, QQmlJSScope::ExtensionKind mode) {
                if (mode == QQmlJSScope::ExtensionNamespace)
                    return false;
                return scope == m_numberPrototype;
    });
}

bool QQmlJSTypeResolver::isSignedInteger(const QQmlJSScope::ConstPtr &type) const
{
    return type == m_int8Type || type == m_int16Type
            || type == m_int32Type || type == m_int64Type;
}

bool QQmlJSTypeResolver::isUnsignedInteger(const QQmlJSScope::ConstPtr &type) const
{
    return type == m_uint8Type || type == m_uint16Type
            || type == m_uint32Type || type == m_uint64Type;
}

bool QQmlJSTypeResolver::isNativeArrayIndex(const QQmlJSScope::ConstPtr &type) const
{
    return type == m_uint8Type || type == m_int8Type || type == m_uint16Type || type == m_int16Type
            || type == m_uint32Type || type == m_int32Type;
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::containedTypeForName(const QString &name) const
{
    QQmlJSScope::ConstPtr type = typeForName(name);

    if (!type || type->isSingleton() || type->isScript())
        return type;

    switch (type->accessSemantics()) {
    case QQmlJSScope::AccessSemantics::Reference:
        if (const auto attached = type->attachedType())
            return genericType(attached) ? attached : QQmlJSScope::ConstPtr();
        return metaObjectType();
    case QQmlJSScope::AccessSemantics::None:
        return metaObjectType();
    case QQmlJSScope::AccessSemantics::Sequence:
    case QQmlJSScope::AccessSemantics::Value:
        return canAddressValueTypes() ?  metaObjectType() : QQmlJSScope::ConstPtr();
    }

    Q_UNREACHABLE_RETURN(QQmlJSScope::ConstPtr());
}

QQmlJSRegisterContent QQmlJSTypeResolver::registerContentForName(
        const QString &name, QQmlJSRegisterContent scopeType) const
{
    QQmlJSScope::ConstPtr type = typeForName(name);
    if (!type)
        return QQmlJSRegisterContent();

    if (type->isSingleton()) {
        return m_pool->createType(
                type, QQmlJSRegisterContent::InvalidLookupIndex,
                QQmlJSRegisterContent::Singleton, scopeType);
    }

    if (type->isScript()) {
        return m_pool->createType(
                type, QQmlJSRegisterContent::InvalidLookupIndex,
                QQmlJSRegisterContent::Script, scopeType);
    }

    const QQmlJSRegisterContent namedType = m_pool->createType(
            type, QQmlJSRegisterContent::InvalidLookupIndex, QQmlJSRegisterContent::TypeByName,
            scopeType);

    if (const auto attached = type->attachedType()) {
        if (!genericType(attached)) {
            m_logger->log(u"Cannot resolve generic base of attached %1"_s.arg(
                                  attached->internalName()),
                          qmlCompiler, attached->sourceLocation());
            return {};
        } else if (type->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
            m_logger->log(u"Cannot retrieve attached object for non-reference type %1"_s.arg(
                                  type->internalName()),
                          qmlCompiler, type->sourceLocation());
            return {};
        } else {
            // We don't know yet whether we need the attached or the plain object. In direct
            // mode, we will figure this out using the scope type and access any enums of the
            // plain type directly. In indirect mode, we can use enum lookups.
            return m_pool->createType(
                        attached, QQmlJSRegisterContent::InvalidLookupIndex,
                        QQmlJSRegisterContent::Attachment, namedType);
        }
    }

    switch (type->accessSemantics()) {
    case QQmlJSScope::AccessSemantics::None:
    case QQmlJSScope::AccessSemantics::Reference:
        // A plain reference to a non-singleton, non-attached type.
        // We may still need the plain type reference for enum lookups,
        // Store it as QMetaObject.
        // This only works with namespaces and object types.
        return m_pool->createType(
                metaObjectType(), QQmlJSRegisterContent::InvalidLookupIndex,
                QQmlJSRegisterContent::MetaType, namedType);
    case QQmlJSScope::AccessSemantics::Sequence:
    case QQmlJSScope::AccessSemantics::Value:
        if (scopeType.isImportNamespace() || canAddressValueTypes()) {
            return m_pool->createType(
                    metaObjectType(), QQmlJSRegisterContent::InvalidLookupIndex,
                    QQmlJSRegisterContent::MetaType, namedType);
        }
        // Else this is not actually a type reference. You cannot get the metaobject
        // of a value type in QML and sequences don't even have metaobjects.
        break;
    }

    return QQmlJSRegisterContent();
}

QQmlJSRegisterContent QQmlJSTypeResolver::original(QQmlJSRegisterContent type) const
{
    QQmlJSRegisterContent result = type.original();
    return result.isNull() ? type : result;
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::originalContainedType(
        QQmlJSRegisterContent container) const
{
    return original(container).containedType();
}

bool QQmlJSTypeResolver::adjustTrackedType(
        QQmlJSRegisterContent tracked, const QQmlJSScope::ConstPtr &conversion) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return true;

    QQmlJSScope::ConstPtr contained = tracked.containedType();
    QQmlJSScope::ConstPtr result = conversion;

    // Do not adjust to the JavaScript extension of the original type. Rather keep the original
    // type in that case.
    QQmlJSUtils::searchBaseAndExtensionTypes(
            contained, [&](const QQmlJSScope::ConstPtr &scope, QQmlJSScope::ExtensionKind kind) {
        if (kind != QQmlJSScope::ExtensionJavaScript || scope != result)
            return false;
        result = contained;
        return true;
    });


    // If we cannot convert to the new type without the help of e.g. lookupResultMetaType(),
    // we better not change the type.
    if (!canPrimitivelyConvertFromTo(contained, result)
        && !canPopulate(result, contained, nullptr)
        && !selectConstructor(result, contained, nullptr).isValid()) {
        return false;
    }

    m_pool->adjustType(tracked, result);
    return true;
}

bool QQmlJSTypeResolver::adjustTrackedType(
        QQmlJSRegisterContent tracked, QQmlJSRegisterContent conversion) const
{
    return adjustTrackedType(tracked, conversion.containedType());
}

bool QQmlJSTypeResolver::adjustTrackedType(
        QQmlJSRegisterContent tracked, const QList<QQmlJSRegisterContent> &conversions) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return true;

    QQmlJSScope::ConstPtr result;
    for (QQmlJSRegisterContent type : conversions)
        result = merge(type.containedType(), result);

    return adjustTrackedType(tracked, result);
}

void QQmlJSTypeResolver::adjustOriginalType(
        QQmlJSRegisterContent tracked, const QQmlJSScope::ConstPtr &conversion) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return;

    m_pool->generalizeType(tracked, conversion);
}

void QQmlJSTypeResolver::generalizeType(QQmlJSRegisterContent type) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return;

    for (QQmlJSRegisterContent orig = type; !orig.isNull(); orig = orig.original()) {
        if (!orig.shadowed().isValid())
            m_pool->generalizeType(orig, genericType(orig.containedType()));
    }
}

bool QQmlJSTypeResolver::canConvertFromTo(const QQmlJSScope::ConstPtr &from,
                                          const QQmlJSScope::ConstPtr &to) const
{
    if (canPrimitivelyConvertFromTo(from, to)
            || canPopulate(to, from, nullptr)
            || selectConstructor(to, from, nullptr).isValid()) {
        return true;
    }

    // ### need a generic solution for custom cpp types:
    // if (from->m_hasBoolOverload && equals(to, boolType))
    //    return true;

    // All of these types have QString conversions that require a certain format
    // TODO: Actually verify these strings or deprecate them.
    //       Some of those type are builtins or should be builtins. We should add code for them
    //       in QQmlJSCodeGenerator::conversion().
    if (from == m_stringType && !to.isNull()) {
        const QString toTypeName = to->internalName();
        if (toTypeName == u"QPoint"_s || toTypeName == u"QPointF"_s
                || toTypeName == u"QSize"_s || toTypeName == u"QSizeF"_s
                || toTypeName == u"QRect"_s || toTypeName == u"QRectF"_s) {
            return true;
        }
    }

    return false;
}

bool QQmlJSTypeResolver::canConvertFromTo(QQmlJSRegisterContent from,
                                          QQmlJSRegisterContent to) const
{
    return canConvertFromTo(from.containedType(), to.containedType());
}

static QQmlJSRegisterContent::ContentVariant mergeVariants(QQmlJSRegisterContent::ContentVariant a,
                                                           QQmlJSRegisterContent::ContentVariant b)
{
    return (a == b) ? a : QQmlJSRegisterContent::Unknown;
}

QQmlJSRegisterContent QQmlJSTypeResolver::merge(
        QQmlJSRegisterContent a, QQmlJSRegisterContent b) const
{
    Q_ASSERT(a != b);

    // We cannot easily provide an operator< for QQmlJSRegisterContent.
    // Therefore we use qHash and operator== here to deduplicate. That's somewhat inefficient.
    QSet<QQmlJSRegisterContent> origins;

    QQmlJSRegisterContent aResultScope;
    if (a.isConversion()) {
        const auto aOrigins = a.conversionOrigins();
        for (const auto &aOrigin : aOrigins)
            origins.insert(aOrigin);
        aResultScope = a.conversionResultScope();
    } else {
        origins.insert(a);
        aResultScope = a.scope();
    }

    QQmlJSRegisterContent bResultScope;
    if (b.isConversion()) {
        const auto bOrigins = b.conversionOrigins();
        for (const auto &bOrigin : bOrigins)
            origins.insert(bOrigin);
        bResultScope = b.conversionResultScope();
    } else {
        origins.insert(b);
        bResultScope = b.scope();
    }

    const auto mergeScopes = [&](QQmlJSRegisterContent a, QQmlJSRegisterContent b) {
        // If they are the same, we don't want to re-track them.
        // We want the repetition on type mismatches to converge.
        return (a == b) ? a : merge(a, b);
    };

    return m_pool->createConversion(
            origins.values(),
            merge(a.containedType(), b.containedType()),
            mergeScopes(aResultScope, bResultScope),
            mergeVariants(a.variant(), b.variant()),
            mergeScopes(a.scope(), b.scope()));
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::merge(const QQmlJSScope::ConstPtr &a,
                                                const QQmlJSScope::ConstPtr &b) const
{
    if (a.isNull())
        return b;

    if (b.isNull())
        return a;

    const auto baseOrExtension
            = [](const QQmlJSScope::ConstPtr &a, const QQmlJSScope::ConstPtr &b) {
        QQmlJSScope::ConstPtr found;
        QQmlJSUtils::searchBaseAndExtensionTypes(
                a, [&](const QQmlJSScope::ConstPtr &scope, QQmlJSScope::ExtensionKind kind) {
            switch (kind) {
            case QQmlJSScope::NotExtension:
                // If b inherits scope, then scope is a common base type of a and b
                if (b->inherits(scope)) {
                    found = scope;
                    return true;
                }
                break;
            case QQmlJSScope::ExtensionJavaScript:
                // Merging a type with its JavaScript extension produces the type.
                // Giving the JavaScript extension as type to be read means we expect any type
                // that fulfills the given JavaScript interface
                if (scope == b) {
                    found = a;
                    return true;
                }
                break;
            case QQmlJSScope::ExtensionType:
            case QQmlJSScope::ExtensionNamespace:
                break;
            }
            return false;
        });
        return found;
    };

    if (a == b)
        return a;

    if (a == jsValueType() || a == varType())
        return a;
    if (b == jsValueType() || b == varType())
        return b;

    const auto isInt32Compatible = [&](const QQmlJSScope::ConstPtr &type) {
        return (isIntegral(type)
                    && type != uint32Type()
                    && type != int64Type()
                    && type != uint64Type())
                || type == boolType();
    };

    if (isInt32Compatible(a) && isInt32Compatible(b))
        return int32Type();

    const auto isUInt32Compatible = [&](const QQmlJSScope::ConstPtr &type) {
        return (isUnsignedInteger(type) && type != uint64Type()) || type == boolType();
    };

    if (isUInt32Compatible(a) && isUInt32Compatible(b))
        return uint32Type();

    if (isNumeric(a) && isNumeric(b))
        return realType();

    if (isPrimitive(a) && isPrimitive(b))
        return jsPrimitiveType();

    if (const auto base = baseOrExtension(a, b))
        return base;

    if (const auto base = baseOrExtension(b, a))
        return base;

    if ((a == nullType() || a == boolType()) && b->isReferenceType())
        return b;

    if ((b == nullType() || b == boolType()) && a->isReferenceType())
        return a;

    return varType();
}

bool QQmlJSTypeResolver::canHold(
        const QQmlJSScope::ConstPtr &container, const QQmlJSScope::ConstPtr &contained) const
{
    if (container == contained || container == m_varType || container == m_jsValueType)
        return true;

    if (container == m_jsPrimitiveType)
        return isPrimitive(contained);

    if (container == m_variantListType)
        return contained->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence;

    if (container == m_qObjectListType || container == m_listPropertyType) {
        if (contained->accessSemantics() != QQmlJSScope::AccessSemantics::Sequence)
            return false;
        if (QQmlJSScope::ConstPtr value = contained->valueType())
            return value->isReferenceType();
        return false;
    }

    if (QQmlJSUtils::searchBaseAndExtensionTypes(
                container, [&](const QQmlJSScope::ConstPtr &base) {
        return base == contained;
    })) {
        return true;
    }

    if (container->isReferenceType()) {
        if (QQmlJSUtils::searchBaseAndExtensionTypes(
                    contained, [&](const QQmlJSScope::ConstPtr &base) {
            return base == container;
        })) {
            return true;
        }
    }

    return false;
}


bool QQmlJSTypeResolver::canHoldUndefined(QQmlJSRegisterContent content) const
{
    const auto canBeUndefined = [this](const QQmlJSScope::ConstPtr &type) {
        return type == m_voidType || type == m_varType
                || type == m_jsValueType || type == m_jsPrimitiveType;
    };

    if (!canBeUndefined(content.containedType()))
        return false;

    if (!content.isConversion())
        return true;

    const auto origins = content.conversionOrigins();
    for (const auto &origin : origins) {
        if (canBeUndefined(originalContainedType(origin)))
            return true;
    }

    return false;
}

bool QQmlJSTypeResolver::isOptionalType(QQmlJSRegisterContent content) const
{
    if (!content.isConversion())
        return false;

    const auto origins = content.conversionOrigins();
    if (origins.length() != 2)
        return false;

    // Conversion origins are always adjusted to the conversion result. None of them will be void.
    // Therefore, retrieve the originals first.

    return original(origins[0]).contains(m_voidType) || original(origins[1]).contains(m_voidType);
}

QQmlJSRegisterContent QQmlJSTypeResolver::extractNonVoidFromOptionalType(
        QQmlJSRegisterContent content) const
{
    if (!isOptionalType(content))
        return QQmlJSRegisterContent();

    // Conversion origins are always adjusted to the conversion result. None of them will be void.
    // Therefore, retrieve the originals first.

    auto origins = content.conversionOrigins();
    std::transform(origins.cbegin(), origins.cend(), origins.begin(),
                   [this](QQmlJSRegisterContent content) {return original(content);});
    const QQmlJSRegisterContent result = origins[0].contains(m_voidType)
            ? origins[1]
            : origins[0];

    // The result may still be undefined. You can write "undefined ?? undefined ?? 1"
    return result;
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::genericType(
        const QQmlJSScope::ConstPtr &type,
        ComponentIsGeneric allowComponent) const
{
    if (type->isScript())
        return m_jsValueType;

    if (type == m_metaObjectType)
        return m_metaObjectType;

    if (type->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
        QString unresolvedBaseTypeName;
        for (auto base = type; base;) {
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

            if (auto baseBase = base->baseType()) {
                base = baseBase;
            } else {
                unresolvedBaseTypeName = base->baseTypeName();
                break;
            }
        }

        // Reference types that are not QObject or QQmlComponent are likely JavaScript objects.
        // We don't want to deal with those, but m_jsValueType is the best generic option.
        if (type->filePath().isEmpty())
            return m_jsValueType;

        m_logger->log(u"Object type %1 is not derived from QObject or QQmlComponent. "
                      "You may need to fully qualify all names in C++ so that moc can see them. "
                      "You may also need to add qt_extract_metatypes(<target containing %2>)."_s
                      .arg(type->internalName(), unresolvedBaseTypeName),
                      qmlCompiler, type->sourceLocation());

        // If it does have a filePath, it's some C++ type which we haven't fully resolved.
        return m_jsValueType;
    }

    if (type->isListProperty())
        return m_listPropertyType;

    if (type->scopeType() == QQmlSA::ScopeType::EnumScope)
        return type->baseType();

    if (isPrimitive(type)) {
        // If the filePath is set, the type is storable and we can just return it.
        if (!type->filePath().isEmpty())
            return type;

        // If the type is JavaScript's 'number' type, it's not directly storable, but still
        // primitive. We use C++ 'double' then.
        if (isNumeric(type))
            return m_realType;

        // Otherwise we use QJSPrimitiveValue.
        // TODO: JavaScript's 'string' and 'boolean' could be special-cased here.
        return m_jsPrimitiveType;
    }

    for (const QQmlJSScope::ConstPtr &builtin : {
                 m_realType, m_floatType, m_int8Type, m_uint8Type, m_int16Type, m_uint16Type,
                 m_int32Type, m_uint32Type, m_int64Type, m_uint64Type, m_boolType, m_stringType,
                 m_stringListType, m_byteArrayType, m_urlType, m_dateTimeType, m_dateType,
                 m_timeType, m_variantListType, m_variantMapType, m_varType, m_jsValueType,
                 m_jsPrimitiveType, m_listPropertyType, m_qObjectType, m_qObjectListType,
                 m_metaObjectType, m_forInIteratorPtr, m_forOfIteratorPtr }) {
        if (type == builtin || type == builtin->listType())
            return type;
    }

    return m_varType;
}

/*!
 * \internal
 * The type of a JavaScript literal value appearing in script code
 */
QQmlJSRegisterContent QQmlJSTypeResolver::literalType(const QQmlJSScope::ConstPtr &type) const
{
    return m_pool->createType(
            type, QQmlJSRegisterContent::InvalidLookupIndex, QQmlJSRegisterContent::Literal);
}

/*!
 * \internal
 * The type of the result of a JavaScript operation
 */
QQmlJSRegisterContent QQmlJSTypeResolver::operationType(const QQmlJSScope::ConstPtr &type) const
{
    return m_pool->createType(
            type, QQmlJSRegisterContent::InvalidLookupIndex, QQmlJSRegisterContent::Operation);
}

/*!
 * \internal
 * A type named explicitly, for example in "as"-casts or as function annotation.
 */
QQmlJSRegisterContent QQmlJSTypeResolver::namedType(const QQmlJSScope::ConstPtr &type) const
{
    return m_pool->createType(
            type, QQmlJSRegisterContent::InvalidLookupIndex, QQmlJSRegisterContent::TypeByName);
}

QQmlJSRegisterContent QQmlJSTypeResolver::syntheticType(const QQmlJSScope::ConstPtr &type) const
{
    return m_pool->createType(
            type, QQmlJSRegisterContent::InvalidLookupIndex, QQmlJSRegisterContent::Unknown);
}

static QQmlJSRegisterContent::ContentVariant scopeContentVariant(QQmlJSScope::ExtensionKind mode,
                                                                 bool isMethod)
{
    switch (mode) {
    case QQmlJSScope::NotExtension:
    case QQmlJSScope::ExtensionType:
    case QQmlJSScope::ExtensionJavaScript:
        return isMethod
                ? QQmlJSRegisterContent::Method
                : QQmlJSRegisterContent::Property;
    case QQmlJSScope::ExtensionNamespace:
        break;
    }
    Q_UNREACHABLE_RETURN(QQmlJSRegisterContent::Unknown);
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

QQmlJSScope::ConstPtr QQmlJSTypeResolver::resolveParentProperty(
        const QString &name, const QQmlJSScope::ConstPtr &base,
        const QQmlJSScope::ConstPtr &propType) const
{
    if (m_parentMode != UseDocumentParent || name != base->parentPropertyName())
        return propType;

    const QQmlJSScope::ConstPtr baseParent = base->parentScope();
    if (!baseParent || !baseParent->inherits(propType))
        return propType;

    const QString defaultPropertyName = baseParent->defaultPropertyName();
    if (defaultPropertyName.isEmpty()) // no reason to search for bindings
        return propType;

    const QList<QQmlJSMetaPropertyBinding> defaultPropBindings
            = baseParent->propertyBindings(defaultPropertyName);
    for (const QQmlJSMetaPropertyBinding &binding : defaultPropBindings) {
        if (binding.bindingType() == QQmlSA::BindingType::Object && binding.objectType() == base)
            return baseParent;
    }

    return propType;
}

/*!
 * \internal
 * We can generally determine the relevant component boundaries for each scope. However,
 * if the scope or any of its parents is assigned to a property of which we cannot see the
 * type, we don't know whether the type of that property happens to be Component. In that
 * case, we can't say.
 */
bool QQmlJSTypeResolver::canFindComponentBoundaries(const QQmlJSScope::ConstPtr &scope) const
{
    for (QQmlJSScope::ConstPtr parent = scope; parent; parent = parent->parentScope()) {
        if (parent->isAssignedToUnknownProperty())
            return false;
    }
    return true;
}

/*!
 * \internal
 *
 * Retrieves the type of whatever \a name signifies in the given \a scope.
 * \a name can be an ID, a property of the scope, a singleton, an attachment,
 * a plain type reference or a JavaScript global.
 *
 * TODO: The lookup is actually wrong. We cannot really retrieve JavaScript
 *       globals here because any runtime-inserted context property would
 *       override them. We still do because we don't have a better solution for
 *       identifying e.g. the console object, yet.
 *
 * \a options tells us whether to consider components as bound. If components
 * are bound we can retrieve objects identified by ID in outer contexts.
 *
 * TODO: This is also wrong because we should alternate scopes and contexts when
 *       traveling the scope/context hierarchy. Currently we have IDs from any
 *       context override all scope properties if components are considered
 *       bound. This is mostly because we don't care about outer scopes at all;
 *       so we cannot determine with certainty whether an ID from a far outer
 *       context is overridden by a property of a near outer scope. To
 *       complicate this further, user context properties can also be inserted
 *       in outer contexts at run time, shadowing names in further removed outer
 *       scopes and contexts. What we need to do is determine where exactly what
 *       kind of property can show up and defend against that with additional
 *       pragmas.
 *
 * Note: It probably takes at least 3 nested bound components in one document to
 *       trigger the misbehavior.
 */
QQmlJSScope::ConstPtr QQmlJSTypeResolver::scopedType(
        const QQmlJSScope::ConstPtr &scope, const QString &name,
        QQmlJSScopesByIdOptions options) const
{
    if (!canFindComponentBoundaries(scope))
        return {};

    if (QQmlJSScope::ConstPtr identified = m_objectsById.scope(name, scope, options))
        return identified;

    if (QQmlJSScope::ConstPtr base = QQmlJSScope::findCurrentQMLScope(scope)) {
        QQmlJSScope::ConstPtr result;
        if (QQmlJSUtils::searchBaseAndExtensionTypes(
                    base, [&](const QQmlJSScope::ConstPtr &found, QQmlJSScope::ExtensionKind mode) {
            if (mode == QQmlJSScope::ExtensionNamespace) // no use for it here
                return false;

            if (found->hasOwnProperty(name)) {
                const QQmlJSMetaProperty prop = found->ownProperty(name);
                if (!isRevisionAllowed(prop.revision(), scope))
                    return false;

                result = resolveParentProperty(name, base, prop.type());
                return true;
            }

            if (found->hasOwnMethod(name)) {
                const auto methods = found->ownMethods(name);
                for (const auto &method : methods) {
                    if (isRevisionAllowed(method.revision(), scope)) {
                        result = jsValueType();
                        return true;
                    }
                }
            }

            return false;
        })) {
            return result;
        }
    }

    if (QQmlJSScope::ConstPtr result = containedTypeForName(name))
        return result;

    if (m_jsGlobalObject->hasProperty(name))
        return m_jsGlobalObject->property(name).type();

    if (m_jsGlobalObject->hasMethod(name))
        return jsValueType();

    return {};
}

/*!
 * \internal
 *
 * Same as the other scopedType method, but accepts a QQmlJSRegisterContent and
 * also returns one. This way you not only get the type, but also the content
 * variant and various meta info.
 */
QQmlJSRegisterContent QQmlJSTypeResolver::scopedType(QQmlJSRegisterContent scope,
                                                     const QString &name, int lookupIndex,
                                                     QQmlJSScopesByIdOptions options) const
{
    const QQmlJSScope::ConstPtr contained = scope.containedType();
    if (!canFindComponentBoundaries(contained))
        return {};

    if (QQmlJSScope::ConstPtr identified = m_objectsById.scope(name, contained, options)) {
        return m_pool->createType(
                identified, lookupIndex, QQmlJSRegisterContent::ObjectById, scope);
    }

    if (QQmlJSScope::ConstPtr base = QQmlJSScope::findCurrentQMLScope(contained)) {
        QQmlJSRegisterContent result;
        if (QQmlJSUtils::searchBaseAndExtensionTypes(
                    base, [&](const QQmlJSScope::ConstPtr &found, QQmlJSScope::ExtensionKind mode) {
            if (mode == QQmlJSScope::ExtensionNamespace) // no use for it here
                return false;

            const QQmlJSRegisterContent resultScope = mode == QQmlJSScope::NotExtension
                    ? scope
                    : extensionType(found, scope);

            if (found->hasOwnProperty(name)) {
                QQmlJSMetaProperty prop = found->ownProperty(name);
                if (!isRevisionAllowed(prop.revision(), contained))
                    return false;

                prop.setType(resolveParentProperty(name, base, prop.type()));
                result = m_pool->createProperty(
                        prop, QQmlJSRegisterContent::InvalidLookupIndex, lookupIndex,
                        scopeContentVariant(mode, false), resultScope);
                return true;
            }

            if (found->hasOwnMethod(name)) {
                auto methods = found->ownMethods(name);
                for (auto it = methods.begin(); it != methods.end();) {
                    if (!isRevisionAllowed(it->revision(), contained))
                        it = methods.erase(it);
                    else
                        ++it;
                }
                if (methods.isEmpty())
                    return false;
                result = m_pool->createMethod(
                        methods, jsValueType(), scopeContentVariant(mode, true), resultScope);
                return true;
            }

            // Unqualified enums are not allowed
            return false;
        })) {
            return result;
        }
    }

    QQmlJSRegisterContent result = registerContentForName(name, scope);

    if (result.isValid())
        return result;

    if (m_jsGlobalObject->hasProperty(name)) {
        return m_pool->createProperty(
                m_jsGlobalObject->property(name), QQmlJSRegisterContent::InvalidLookupIndex,
                lookupIndex, QQmlJSRegisterContent::Property, m_jsGlobalObjectContent);
    } else if (m_jsGlobalObject->hasMethod(name)) {
        return m_pool->createMethod(
                m_jsGlobalObject->methods(name), jsValueType(),
                QQmlJSRegisterContent::Property, m_jsGlobalObjectContent);
    }

    return {};
}

/*!
 * \fn QQmlJSScope::ConstPtr typeForId(const QQmlJSScope::ConstPtr &scope, const QString &name, QQmlJSScopesByIdOptions options) const
 *
 * \internal
 *
 * Same as scopedType(), but assumes that the \a name is an ID and only searches
 * the context.
 *
 * TODO: This is just as wrong as scopedType() in that it disregards both scope
 *       properties overriding context properties and run time context
 *       properties.
 */

bool QQmlJSTypeResolver::checkEnums(
        QQmlJSRegisterContent scope, const QString &name,
        QQmlJSRegisterContent *result) const
{
    // You can't have lower case enum names in QML, even if we know the enums here.
    if (name.isEmpty() || !name.at(0).isUpper())
        return false;

    const auto enums = scope.containedType()->ownEnumerations();
    for (const auto &enumeration : enums) {
        if ((enumeration.isScoped() || enumeration.isQml()) && enumeration.name() == name) {
            *result = m_pool->createEnumeration(
                    enumeration, QString(),
                    QQmlJSRegisterContent::Enum,
                    scope);
            return true;
        }

        if ((!enumeration.isScoped() || enumeration.isQml()
             || !scope.containedType()->enforcesScopedEnums()) && enumeration.hasKey(name)) {
            *result = m_pool->createEnumeration(
                    enumeration, name,
                    QQmlJSRegisterContent::Enum,
                    scope);
            return true;
        }
    }

    return false;
}

bool QQmlJSTypeResolver::canPopulate(
        const QQmlJSScope::ConstPtr &type, const QQmlJSScope::ConstPtr &passedArgumentType,
        bool *isExtension) const
{
    // TODO: We could allow QVariantMap and QVariantHash to be populated, but that needs extra
    //       code in the code generator.

    if (type.isNull()
            || canHold(passedArgumentType, type)
            || isPrimitive(passedArgumentType)
            || type->accessSemantics() != QQmlJSScope::AccessSemantics::Value
            || !type->isStructured()) {
        return false;
    }

    if (isExtension)
        *isExtension = !type->extensionType().scope.isNull();

    return true;
}

QQmlJSMetaMethod QQmlJSTypeResolver::selectConstructor(
        const QQmlJSScope::ConstPtr &type, const QQmlJSScope::ConstPtr &passedArgumentType,
        bool *isExtension) const
{
    // If the "from" type can hold the target type, we should not try to coerce
    // it to any constructor argument.
    if (type.isNull()
            || canHold(passedArgumentType, type)
            || type->accessSemantics() != QQmlJSScope::AccessSemantics::Value
            || !type->isCreatable()) {
        return QQmlJSMetaMethod();
    }

    auto doSelectConstructor = [&](const QQmlJSScope::ConstPtr &type) {
        QQmlJSMetaMethod candidate;

        const auto ownMethods = type->ownMethods();
        for (const QQmlJSMetaMethod &method : ownMethods) {
            if (!method.isConstructor())
                continue;

            const auto index = method.constructorIndex();
            Q_ASSERT(index != QQmlJSMetaMethod::RelativeFunctionIndex::Invalid);

            const auto methodArguments = method.parameters();
            if (methodArguments.size() != 1)
                continue;

            const QQmlJSScope::ConstPtr methodArgumentType = methodArguments[0].type();

            if (passedArgumentType == methodArgumentType)
                return method;

            // Do not select further ctors here. We don't want to do multi-step construction as that
            // is confusing and easily leads to infinite recursion.
            if (!candidate.isValid()
                && canPrimitivelyConvertFromTo(passedArgumentType, methodArgumentType)) {
                candidate = method;
            }
        }

        return candidate;
    };

    if (QQmlJSScope::ConstPtr extension = type->extensionType().scope) {
        const QQmlJSMetaMethod ctor = doSelectConstructor(extension);
        if (ctor.isValid()) {
            if (isExtension)
                *isExtension = true;
            return ctor;
        }
    }

    if (isExtension)
        *isExtension = false;

    return doSelectConstructor(type);
}

bool QQmlJSTypeResolver::areEquivalentLists(
        const QQmlJSScope::ConstPtr &a, const QQmlJSScope::ConstPtr &b) const
{
    const QQmlJSScope::ConstPtr equivalentLists[2][2] = {
        { m_stringListType, m_stringType->listType() },
        { m_variantListType, m_varType->listType() }
    };

    for (const auto eq : equivalentLists) {
        if ((a == eq[0] && b == eq[1]) || (a == eq[1] && b == eq[0]))
            return true;
    }

    return false;
}

bool QQmlJSTypeResolver::isTriviallyCopyable(const QQmlJSScope::ConstPtr &type) const
{
    // pointers are trivially copyable
    if (type->isReferenceType())
        return true;

    // Enum values are trivially copyable
    if (type->scopeType() == QQmlSA::ScopeType::EnumScope)
        return true;

    for (const QQmlJSScope::ConstPtr &trivial : {
            m_nullType, m_voidType,
            m_boolType, m_metaObjectType,
            m_realType, m_floatType,
            m_int8Type, m_uint8Type,
            m_int16Type, m_uint16Type,
            m_int32Type, m_uint32Type,
            m_int64Type, m_uint64Type }) {
        if (type == trivial)
            return true;
    }

    return false;
}

bool QQmlJSTypeResolver::inherits(const QQmlJSScope::ConstPtr &derived, const QQmlJSScope::ConstPtr &base) const
{
    const bool matchByName = !base->isComposite();
    for (QQmlJSScope::ConstPtr derivedBase = derived; derivedBase;
            derivedBase = derivedBase->baseType()) {
        if (derivedBase == base)
            return true;
        if (matchByName
                && !derivedBase->isComposite()
                && derivedBase->internalName() == base->internalName()) {
            return true;
        }
    }
    return false;
}

bool QQmlJSTypeResolver::canPrimitivelyConvertFromTo(
        const QQmlJSScope::ConstPtr &from, const QQmlJSScope::ConstPtr &to) const
{
    if (from == to)
        return true;
    if (from == m_varType || to == m_varType)
        return true;
    if (from == m_jsValueType || to == m_jsValueType)
        return true;
    if (to == m_qQmlScriptStringType)
        return true;
    if (isNumeric(from) && isNumeric(to))
        return true;
    if (isNumeric(from) && to == m_boolType)
        return true;
    if (from->accessSemantics() == QQmlJSScope::AccessSemantics::Reference
            && (to == m_boolType || to == m_stringType)) {
        return true;
    }

    // Yes, our String has number constructors.
    if (isNumeric(from) && to == m_stringType)
        return true;

    // We can convert strings to numbers, but not to enums
    if (from == m_stringType && isNumeric(to))
        return to->scopeType() != QQmlJSScope::ScopeType::EnumScope;

    // We can always convert between strings and urls.
    if ((from == m_stringType && to == m_urlType)
            || (from == m_urlType && to == m_stringType)) {
        return true;
    }

    // We can always convert between strings and byte arrays.
    if ((from == m_stringType && to == m_byteArrayType)
            || (from == m_byteArrayType && to == m_stringType)) {
        return true;
    }

    if (to == m_voidType)
        return true;

    if (to.isNull())
        return from == m_voidType;

    const auto types = { m_dateTimeType, m_dateType, m_timeType, m_stringType };
    for (const auto &originType : types) {
        if (from != originType)
            continue;

        for (const auto &targetType : types) {
            if (to == targetType)
                return true;
        }

        if (to == m_realType)
            return true;

        break;
    }

    if (from == m_nullType && to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
        return true;

    if (from == m_jsPrimitiveType) {
        // You can cast any primitive to a nullptr
        return isPrimitive(to) || to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference;
    }

    if (to == m_jsPrimitiveType)
        return isPrimitive(from);

    const bool matchByName = !to->isComposite();
    Q_ASSERT(!matchByName || !to->internalName().isEmpty());
    if (QQmlJSUtils::searchBaseAndExtensionTypes(
            from, [&](const QQmlJSScope::ConstPtr &scope, QQmlJSScope::ExtensionKind kind) {
        switch (kind) {
        case QQmlJSScope::NotExtension:
        case QQmlJSScope::ExtensionJavaScript:
            // Converting to a base type is trivially supported.
            // Converting to the JavaScript extension of a type just produces the type itself.
            // Giving the JavaScript extension as type to be converted to means we expect any
            // result that fulfills the given JavaScript interface.
            return scope == to
                    || (matchByName && scope->internalName() == to->internalName());
        case QQmlJSScope::ExtensionType:
        case QQmlJSScope::ExtensionNamespace:
            break;
        }
        return false;
    })) {
        return true;
    }

    if (from == m_variantListType)
        return to->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence;

    // We can convert anything that fits into QJSPrimitiveValue
    if (canConvertFromTo(from, m_jsPrimitiveType) && canConvertFromTo(m_jsPrimitiveType, to))
        return true;

    // We can convert everything to bool.
    if (to == m_boolType)
        return true;

    if (areEquivalentLists(from, to))
        return true;

    if (from->isListProperty()
            && to->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence
            && canConvertFromTo(from->valueType(), to->valueType())) {
        return true;
    }

    if (to == m_stringType && from->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence)
        return canConvertFromTo(from->valueType(), m_stringType);

    return false;
}

QQmlJSRegisterContent QQmlJSTypeResolver::lengthProperty(
        bool isWritable, QQmlJSRegisterContent scope) const
{
    QQmlJSMetaProperty prop;
    prop.setPropertyName(u"length"_s);
    prop.setTypeName(u"qsizetype"_s);
    prop.setType(sizeType());
    prop.setIsWritable(isWritable);
    return m_pool->createProperty(
            prop, QQmlJSRegisterContent::InvalidLookupIndex,
            QQmlJSRegisterContent::InvalidLookupIndex, QQmlJSRegisterContent::Property, scope);
}

QQmlJSRegisterContent QQmlJSTypeResolver::memberType(
        QQmlJSRegisterContent type, const QString &name, int baseLookupIndex,
        int resultLookupIndex) const
{
    QQmlJSRegisterContent result;
    const QQmlJSScope::ConstPtr contained = type.containedType();

    // If we got a plain type reference we have to check the enums of the _scope_.
    if (contained == metaObjectType())
        return {};

    if (contained == variantMapType()) {
        QQmlJSMetaProperty prop;
        prop.setPropertyName(name);
        prop.setTypeName(u"QVariant"_s);
        prop.setType(varType());
        prop.setIsWritable(true);
        return m_pool->createProperty(
                prop, baseLookupIndex, resultLookupIndex,
                QQmlJSRegisterContent::Property, type);
    }

    if (contained == jsValueType()) {
        QQmlJSMetaProperty prop;
        prop.setPropertyName(name);
        prop.setTypeName(u"QJSValue"_s);
        prop.setType(jsValueType());
        prop.setIsWritable(true);
        return m_pool->createProperty(
                prop, baseLookupIndex, resultLookupIndex,
                QQmlJSRegisterContent::Property, type);
    }

    if ((contained == stringType()
            || contained->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence)
            && name == u"length"_s) {
        return lengthProperty(contained != stringType(), type);
    }

    const auto check = [&](const QQmlJSScope::ConstPtr &scope, QQmlJSScope::ExtensionKind mode) {
        const QQmlJSRegisterContent resultScope = mode == QQmlJSScope::NotExtension
                ? baseType(scope, type)
                : extensionType(scope, type);

        if (mode != QQmlJSScope::ExtensionNamespace) {
            if (scope->hasOwnProperty(name)) {
                const auto prop = scope->ownProperty(name);
                result = m_pool->createProperty(
                        prop, baseLookupIndex, resultLookupIndex,
                        QQmlJSRegisterContent::Property, resultScope);
                return true;
            }

            if (scope->hasOwnMethod(name)) {
                const auto methods = scope->ownMethods(name);
                result = m_pool->createMethod(
                        methods, jsValueType(), QQmlJSRegisterContent::Method, resultScope);
                return true;
            }
        }

        return checkEnums(resultScope, name, &result);
    };

    if (QQmlJSUtils::searchBaseAndExtensionTypes(type.containedType(), check))
        return result;

    for (auto scope = contained;
         scope && (scope->scopeType() == QQmlSA::ScopeType::JSFunctionScope
                   || scope->scopeType() == QQmlSA::ScopeType::JSLexicalScope);
         scope = scope->parentScope()) {
        if (auto ownIdentifier = scope->ownJSIdentifier(name)) {
            QQmlJSMetaProperty prop;
            prop.setPropertyName(name);
            prop.setTypeName(u"QJSValue"_s);
            prop.setType(jsValueType());
            prop.setIsWritable(!(ownIdentifier.value().isConst));

            return m_pool->createProperty(
                    prop, baseLookupIndex, resultLookupIndex,
                    QQmlJSRegisterContent::Property,
                    parentScope(scope, type));
        }
    }

    if (QQmlJSScope::ConstPtr attachedBase = typeForName(name)) {
        if (QQmlJSScope::ConstPtr attached = attachedBase->attachedType()) {
            if (!genericType(attached)) {
                m_logger->log(u"Cannot resolve generic base of attached %1"_s.arg(
                                      attached->internalName()),
                              qmlCompiler, attached->sourceLocation());
                return {};
            } else if (contained->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
                m_logger->log(u"Cannot retrieve attached object for non-reference type %1"_s.arg(
                                      contained->internalName()),
                              qmlCompiler, contained->sourceLocation());
                return {};
            } else {
                const QQmlJSRegisterContent namedType = m_pool->createType(
                        attachedBase, QQmlJSRegisterContent::InvalidLookupIndex,
                        QQmlJSRegisterContent::TypeByName, type);

                return m_pool->createType(
                        attached, resultLookupIndex, QQmlJSRegisterContent::Attachment,
                        namedType);
            }
        }
    }

    return {};
}

QQmlJSRegisterContent QQmlJSTypeResolver::memberEnumType(
        QQmlJSRegisterContent type, const QString &name) const
{
    QQmlJSRegisterContent result;

    if (QQmlJSUtils::searchBaseAndExtensionTypes(
                type.containedType(),
                [&](const QQmlJSScope::ConstPtr &scope, QQmlJSScope::ExtensionKind mode) {
                    return checkEnums(mode == QQmlJSScope::NotExtension
                                              ? baseType(scope, type)
                                              : extensionType(scope, type),
                                      name, &result);
                })) {
        return result;
    }

    return {};
}

QQmlJSRegisterContent QQmlJSTypeResolver::memberType(
        QQmlJSRegisterContent type, const QString &name, int lookupIndex) const
{
    if (type.isType()) {
        const auto result = memberType(type, name, type.resultLookupIndex(), lookupIndex);
        if (result.isValid())
            return result;

        // If we didn't find anything and it's an attached type,
        // we might have an enum of the attaching type.
        return memberEnumType(type.scope(), name);
    }
    if (type.isProperty() || type.isMethodCall())
        return memberType(type, name, type.resultLookupIndex(), lookupIndex);
    if (type.isEnumeration()) {
        const auto enumeration = type.enumeration();
        if (!type.enumMember().isEmpty() || !enumeration.hasKey(name))
            return {};
        return m_pool->createEnumeration(
                enumeration, name, QQmlJSRegisterContent::Enum, type.scope());
    }
    if (type.isMethod()) {
        QQmlJSMetaProperty prop;
        prop.setTypeName(u"QJSValue"_s);
        prop.setPropertyName(name);
        prop.setType(jsValueType());
        prop.setIsWritable(true);
        return m_pool->createProperty(
                prop, QQmlJSRegisterContent::InvalidLookupIndex, lookupIndex,
                QQmlJSRegisterContent::Property, type);
    }
    if (type.isImportNamespace()) {
        Q_ASSERT(type.scopeType()->isReferenceType());
        return registerContentForName(name, type);
    }
    if (type.isConversion()) {
        if (const auto result = memberType(
                    type, name, type.resultLookupIndex(), lookupIndex);
            result.isValid()) {
            return result;
        }

        if (const auto result = memberEnumType(type.scope(), name); result.isValid())
            return result;

        // If the conversion consists of only undefined and one actual type,
        // we can produce the members of that one type.
        // If the value is then actually undefined, the result is an exception.

        const auto nonVoid = extractNonVoidFromOptionalType(type);

        // If the conversion cannot hold the original type, it loses information.
        return (!nonVoid.isNull() && canHold(type.conversionResultType(), nonVoid.containedType()))
                ? memberType(nonVoid, name, type.resultLookupIndex(), lookupIndex)
                : QQmlJSRegisterContent();
    }

    Q_UNREACHABLE_RETURN({});
}

QQmlJSRegisterContent QQmlJSTypeResolver::valueType(QQmlJSRegisterContent list) const
{
    QQmlJSScope::ConstPtr value;

    auto valueType = [&](const QQmlJSScope::ConstPtr &scope) -> QQmlJSScope::ConstPtr {
        if (scope->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence)
            return scope->valueType();

        if (scope == m_forInIteratorPtr)
            return m_sizeType;

        if (scope == m_forOfIteratorPtr)
            return list.scopeType()->valueType();

        if (scope == m_jsValueType || scope == m_varType)
            return m_jsValueType;

        if (scope == m_stringType)
            return m_stringType;

        return QQmlJSScope::ConstPtr();
    };

    value = valueType(list.containedType());

    if (value.isNull())
        return {};

    QQmlJSMetaProperty property;
    property.setPropertyName(u"[]"_s);
    property.setTypeName(value->internalName());
    property.setType(value);

    return m_pool->createProperty(
            property, QQmlJSRegisterContent::InvalidLookupIndex,
            QQmlJSRegisterContent::InvalidLookupIndex, QQmlJSRegisterContent::ListValue,
            list);
}

QQmlJSRegisterContent QQmlJSTypeResolver::returnType(
        const QQmlJSMetaMethod &method, const QQmlJSScope::ConstPtr &returnType,
        QQmlJSRegisterContent scope) const
{
    return m_pool->createMethodCall(method, returnType, scope);
}

QQmlJSRegisterContent QQmlJSTypeResolver::extensionType(
        const QQmlJSScope::ConstPtr &extension, QQmlJSRegisterContent base) const
{
    return m_pool->createType(
            extension, base.resultLookupIndex(), QQmlJSRegisterContent::Extension, base);
}

/*!
 * \internal
 * Encodes \a base as a base type of \a derived and returns a QQmlJSRegisterContent.
 * "Base type" here is understood the same way as std::is_base_of would understand it.
 * That means, if you pass the contained type of \a derived as \a base, then \a derived
 * itself is returned.
 */
QQmlJSRegisterContent QQmlJSTypeResolver::baseType(
        const QQmlJSScope::ConstPtr &base, QQmlJSRegisterContent derived) const
{
    return m_pool->createType(
            base, derived.resultLookupIndex(), QQmlJSRegisterContent::BaseType, derived);
}

/*!
 * \internal
 * Encodes \a parent as a parent scope of \a child and returns a QQmlJSRegisterContent.
 * "Parent scope" here means any scope above, but also _including_ \a child.
 * That means, if you pass the contained type of \a child as \a parent, then \a child
 * itself is returned.
 */
QQmlJSRegisterContent QQmlJSTypeResolver::parentScope(
        const QQmlJSScope::ConstPtr &parent, QQmlJSRegisterContent child) const
{
    return m_pool->createType(
            parent, child.resultLookupIndex(), QQmlJSRegisterContent::ParentScope, child);
}

QQmlJSRegisterContent QQmlJSTypeResolver::iteratorPointer(
        QQmlJSRegisterContent listType, QQmlJS::AST::ForEachType type,
        int lookupIndex) const
{
    const QQmlJSScope::ConstPtr value = (type == QQmlJS::AST::ForEachType::In)
            ? m_int32Type
            : valueType(listType).containedType();

    QQmlJSScope::ConstPtr iteratorPointer = type == QQmlJS::AST::ForEachType::In
            ? m_forInIteratorPtr
            : m_forOfIteratorPtr;

    QQmlJSMetaProperty prop;
    prop.setPropertyName(u"<>"_s);
    prop.setTypeName(iteratorPointer->internalName());
    prop.setType(iteratorPointer);
    return m_pool->createProperty(
            prop, lookupIndex,
            QQmlJSRegisterContent::InvalidLookupIndex, QQmlJSRegisterContent::ListIterator,
            listType);
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::storedType(const QQmlJSScope::ConstPtr &type) const
{
    if (type.isNull())
        return {};
    if (type == voidType())
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

static QQmlJSRegisterContent doConvert(
        QQmlJSRegisterContent from, const QQmlJSScope::ConstPtr &to,
        QQmlJSRegisterContent scope, QQmlJSRegisterContentPool *pool)
{
    if (from.isConversion()) {
        return pool->createConversion(
                from.conversionOrigins(), to,
                scope.isValid() ? scope : from.conversionResultScope(),
                from.variant(), from.scope());
    }

    return pool->createConversion(
            QList<QQmlJSRegisterContent>{from},
            to, scope, from.variant(),
            from.scope());
}

QQmlJSRegisterContent QQmlJSTypeResolver::convert(
        QQmlJSRegisterContent from, QQmlJSRegisterContent to) const
{
    return doConvert(from, to.containedType(), to.scope(), m_pool.get());
}

QQmlJSRegisterContent QQmlJSTypeResolver::convert(
        QQmlJSRegisterContent from, const QQmlJSScope::ConstPtr &to) const
{
    return doConvert(from, to, QQmlJSRegisterContent(), m_pool.get());
}

QT_END_NAMESPACE
