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

Q_LOGGING_CATEGORY(lcTypeResolver, "qt.qml.compiler.typeresolver", QtInfoMsg);

QQmlJSTypeResolver::QQmlJSTypeResolver(QQmlJSImporter *importer)
    : m_imports(importer->builtinInternalNames()),
      m_trackedTypes(std::make_unique<QHash<QQmlJSScope::ConstPtr, TrackedType>>())
{
    const QQmlJSImporter::ImportedTypes &builtinTypes = m_imports;
    m_voidType = builtinTypes.type(u"void"_s).scope;
    Q_ASSERT(m_voidType);
    m_nullType = builtinTypes.type(u"std::nullptr_t"_s).scope;
    Q_ASSERT(m_nullType);
    m_realType = builtinTypes.type(u"double"_s).scope;
    Q_ASSERT(m_realType);
    m_floatType = builtinTypes.type(u"float"_s).scope;
    Q_ASSERT(m_floatType);
    m_int8Type = builtinTypes.type(u"qint8"_s).scope;
    Q_ASSERT(m_int8Type);
    m_uint8Type = builtinTypes.type(u"quint8"_s).scope;
    Q_ASSERT(m_uint8Type);
    m_int16Type = builtinTypes.type(u"short"_s).scope;
    Q_ASSERT(m_int16Type);
    m_uint16Type = builtinTypes.type(u"ushort"_s).scope;
    Q_ASSERT(m_uint16Type);
    m_int32Type = builtinTypes.type(u"int"_s).scope;
    Q_ASSERT(m_int32Type);
    m_uint32Type = builtinTypes.type(u"uint"_s).scope;
    Q_ASSERT(m_uint32Type);
    m_int64Type = builtinTypes.type(u"qlonglong"_s).scope;
    Q_ASSERT(m_int64Type);
    m_uint64Type = builtinTypes.type(u"qulonglong"_s).scope;
    Q_ASSERT(m_uint64Type);
    m_boolType = builtinTypes.type(u"bool"_s).scope;
    Q_ASSERT(m_boolType);
    m_stringType = builtinTypes.type(u"QString"_s).scope;
    Q_ASSERT(m_stringType);
    m_stringListType = builtinTypes.type(u"QStringList"_s).scope;
    Q_ASSERT(m_stringListType);
    m_byteArrayType = builtinTypes.type(u"QByteArray"_s).scope;
    Q_ASSERT(m_byteArrayType);
    m_urlType = builtinTypes.type(u"QUrl"_s).scope;
    Q_ASSERT(m_urlType);
    m_dateTimeType = builtinTypes.type(u"QDateTime"_s).scope;
    Q_ASSERT(m_dateTimeType);
    m_dateType = builtinTypes.type(u"QDate"_s).scope;
    Q_ASSERT(m_dateType);
    m_timeType = builtinTypes.type(u"QTime"_s).scope;
    Q_ASSERT(m_timeType);
    m_variantListType = builtinTypes.type(u"QVariantList"_s).scope;
    Q_ASSERT(m_variantListType);
    m_variantMapType = builtinTypes.type(u"QVariantMap"_s).scope;
    Q_ASSERT(m_variantMapType);
    m_varType = builtinTypes.type(u"QVariant"_s).scope;
    Q_ASSERT(m_varType);
    m_jsValueType = builtinTypes.type(u"QJSValue"_s).scope;
    Q_ASSERT(m_jsValueType);
    m_listPropertyType = builtinTypes.type(u"QQmlListProperty<QObject>"_s).scope;
    Q_ASSERT(m_listPropertyType);
    m_qObjectType = builtinTypes.type(u"QObject"_s).scope;
    Q_ASSERT(m_qObjectType);
    m_qObjectListType = builtinTypes.type(u"QObjectList"_s).scope;
    Q_ASSERT(m_qObjectListType);

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

    QQmlJSScope::Ptr functionType = QQmlJSScope::create();
    functionType->setInternalName(u"function"_s);
    functionType->setAccessSemantics(QQmlJSScope::AccessSemantics::Value);
    m_functionType = functionType;

    m_jsGlobalObject = importer->jsGlobalObject();
    auto numberMethods = m_jsGlobalObject->methods(u"Number"_s);
    Q_ASSERT(numberMethods.size() == 1);
    m_numberPrototype = numberMethods[0].returnType()->baseType();
    Q_ASSERT(m_numberPrototype);
    Q_ASSERT(m_numberPrototype->internalName() == u"NumberPrototype"_s);

    auto arrayMethods = m_jsGlobalObject->methods(u"Array"_s);
    Q_ASSERT(arrayMethods.size() == 1);
    m_arrayType = arrayMethods[0].returnType();
    Q_ASSERT(m_arrayType);
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
    m_imports.clearTypes();
    m_signalHandlers.clear();

    if (program)
        program->accept(visitor);

    m_objectsById = visitor->addressableScopes();
    m_objectsByLocation = visitor->scopesBylocation();
    m_signalHandlers = visitor->signalHandlers();
    m_imports = visitor->imports();
}

QQmlJSScope::ConstPtr
QQmlJSTypeResolver::scopeForLocation(const QV4::CompiledData::Location &location) const
{
    // #if required for standalone DOM compilation against Qt 6.2
    qCDebug(lcTypeResolver()).nospace()
            << "looking for object at " << location.line() << ':' << location.column();
    return m_objectsByLocation[location];
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::scopeForId(
        const QString &id, const QQmlJSScope::ConstPtr &referrer) const
{
    return m_objectsById.scope(id, referrer);
}

QString QQmlJSTypeResolver::idForScope(
        const QQmlJSScope::ConstPtr &scope, const QQmlJSScope::ConstPtr &referrer) const
{
    return m_objectsById.id(scope, referrer);
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
        return builtinType(int32Type());
    case QSOperator::Op::URShift:
        return builtinType(uint32Type());
    case QSOperator::Op::Add: {
        const auto leftContents = containedType(left);
        const auto rightContents = containedType(right);
        if (equals(leftContents, stringType()) || equals(rightContents, stringType()))
            return builtinType(stringType());

        const QQmlJSScope::ConstPtr result = merge(leftContents, rightContents);
        if (equals(result, boolType()))
            return builtinType(int32Type());
        if (isNumeric(result))
            return builtinType(realType());

        return builtinType(jsPrimitiveType());
    }
    case QSOperator::Op::Sub:
    case QSOperator::Op::Mul:
    case QSOperator::Op::Exp: {
        const QQmlJSScope::ConstPtr result = merge(containedType(left), containedType(right));
        return builtinType(equals(result, boolType()) ? int32Type() : realType());
    }
    case QSOperator::Op::Div:
    case QSOperator::Op::Mod:
        return builtinType(realType());
    case QSOperator::Op::As:
        return right;
    default:
        break;
    }

    return merge(left, right);
}

QQmlJSRegisterContent QQmlJSTypeResolver::typeForArithmeticUnaryOperation(
        UnaryOperator op, const QQmlJSRegisterContent &operand) const
{
    switch (op) {
    case UnaryOperator::Not:
        return builtinType(boolType());
    case UnaryOperator::Complement:
        return builtinType(int32Type());
    case UnaryOperator::Plus:
        if (isIntegral(operand))
            return operand;
        Q_FALLTHROUGH();
    default:
        if (equals(containedType(operand), boolType()))
            return builtinType(int32Type());
        break;
    }

    return builtinType(realType());
}

bool QQmlJSTypeResolver::isPrimitive(const QQmlJSRegisterContent &type) const
{
    return isPrimitive(containedType(type));
}

bool QQmlJSTypeResolver::isNumeric(const QQmlJSRegisterContent &type) const
{
    return isNumeric(containedType(type));
}

bool QQmlJSTypeResolver::isIntegral(const QQmlJSRegisterContent &type) const
{
    return isIntegral(containedType(type));
}

bool QQmlJSTypeResolver::isIntegral(const QQmlJSScope::ConstPtr &type) const
{
    // Only types of length <= 32bit count as integral
    return isSignedInteger(type) || isUnsignedInteger(type);
}

bool QQmlJSTypeResolver::isPrimitive(const QQmlJSScope::ConstPtr &type) const
{
    return isNumeric(type)
            || equals(type, m_boolType) || equals(type, m_voidType) || equals(type, m_nullType)
            || equals(type, m_stringType) || equals(type, m_jsPrimitiveType);
}

bool QQmlJSTypeResolver::isNumeric(const QQmlJSScope::ConstPtr &type) const
{
    return QQmlJSUtils::searchBaseAndExtensionTypes(
            type, [&](const QQmlJSScope::ConstPtr &scope, QQmlJSScope::ExtensionKind mode) {
                if (mode == QQmlJSScope::ExtensionNamespace)
                    return false;
                return equals(scope, m_numberPrototype);
    });
}

bool QQmlJSTypeResolver::isSignedInteger(const QQmlJSScope::ConstPtr &type) const
{
    // Only types of length <= 32bit count as integral
    return equals(type, m_int8Type)
            || equals(type, m_int16Type)
            || equals(type, m_int32Type);
}

bool QQmlJSTypeResolver::isUnsignedInteger(const QQmlJSScope::ConstPtr &type) const
{
    // Only types of length <= 32bit count as integral
    return equals(type, m_uint8Type)
            || equals(type, m_uint16Type)
            || equals(type, m_uint32Type);
}

QQmlJSScope::ConstPtr
QQmlJSTypeResolver::containedType(const QQmlJSRegisterContent &container) const
{
    if (container.isType())
        return container.type();
    if (container.isProperty())
        return container.property().type();
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

    Q_UNREACHABLE_RETURN({});
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::trackedType(const QQmlJSScope::ConstPtr &type) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return type;

    // If origin is in fact an already tracked type, track the original of that one instead.
    const auto it = m_trackedTypes->find(type);
    QQmlJSScope::ConstPtr orig = (it == m_trackedTypes->end()) ? type : it->original;

    QQmlJSScope::Ptr clone = QQmlJSScope::clone(orig);
    m_trackedTypes->insert(clone, { std::move(orig), QQmlJSScope::ConstPtr(), clone });
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
                    (this->*op)(origin.conversionResultScope()),
                    origin.variant(), (this->*op)(origin.scopeType()));
    }

    Q_UNREACHABLE_RETURN({});
}

QQmlJSRegisterContent QQmlJSTypeResolver::registerContentForName(
        const QString &name, const QQmlJSScope::ConstPtr &scopeType,
        bool hasObjectModulePrefix) const
{
    QQmlJSScope::ConstPtr type = typeForName(name);
    if (!type)
        return QQmlJSRegisterContent();

    if (type->isSingleton())
        return QQmlJSRegisterContent::create(storedType(type), type,
                                             QQmlJSRegisterContent::Singleton, scopeType);

    if (type->isScript())
        return QQmlJSRegisterContent::create(storedType(type), type,
                                             QQmlJSRegisterContent::Script, scopeType);

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
            return QQmlJSRegisterContent::create(
                        storedType(attached), attached,
                        hasObjectModulePrefix
                            ? QQmlJSRegisterContent::ObjectAttached
                            : QQmlJSRegisterContent::ScopeAttached, type);
        }
    }

    switch (type->accessSemantics()) {
    case QQmlJSScope::AccessSemantics::None:
    case QQmlJSScope::AccessSemantics::Reference:
        // A plain reference to a non-singleton, non-attached type.
        // We may still need the plain type reference for enum lookups,
        // Store it as QMetaObject.
        // This only works with namespaces and object types.
        return QQmlJSRegisterContent::create(metaObjectType(), metaObjectType(),
                                             QQmlJSRegisterContent::MetaType, type);
    case QQmlJSScope::AccessSemantics::Sequence:
    case QQmlJSScope::AccessSemantics::Value:
        if (canAddressValueTypes()) {
            return QQmlJSRegisterContent::create(metaObjectType(), metaObjectType(),
                                                 QQmlJSRegisterContent::MetaType, type);
        }
        // Else this is not actually a type reference. You cannot get the metaobject
        // of a value type in QML and sequences don't even have metaobjects.
        break;
    }

    return QQmlJSRegisterContent();
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
    return m_trackedTypes->contains(type) ? type : QQmlJSScope::ConstPtr();
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::originalContainedType(
        const QQmlJSRegisterContent &container) const
{
    return originalType(containedType(container));
}

bool QQmlJSTypeResolver::adjustTrackedType(
        const QQmlJSScope::ConstPtr &tracked, const QQmlJSScope::ConstPtr &conversion) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return true;

    const auto it = m_trackedTypes->find(tracked);
    Q_ASSERT(it != m_trackedTypes->end());

    // If we cannot convert to the new type without the help of e.g. lookupResultMetaType(),
    // we better not change the type.
    if (!canPrimitivelyConvertFromTo(tracked, conversion)
           && !selectConstructor(conversion, tracked, nullptr).isValid()) {
        return false;
    }

    it->replacement = comparableType(conversion);
    *it->clone = std::move(*QQmlJSScope::clone(conversion));
    return true;
}

bool QQmlJSTypeResolver::adjustTrackedType(
        const QQmlJSScope::ConstPtr &tracked, const QList<QQmlJSScope::ConstPtr> &conversions) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return true;

    const auto it = m_trackedTypes->find(tracked);
    Q_ASSERT(it != m_trackedTypes->end());
    QQmlJSScope::Ptr mutableTracked = it->clone;
    QQmlJSScope::ConstPtr result;
    for (const QQmlJSScope::ConstPtr &type : conversions)
        result = merge(type, result);

    // If we cannot convert to the new type without the help of e.g. lookupResultMetaType(),
    // we better not change the type.
    if (!canPrimitivelyConvertFromTo(tracked, result)
            && !selectConstructor(result, tracked, nullptr).isValid()) {
        return false;
    }

    it->replacement = comparableType(result);
    *mutableTracked = std::move(*QQmlJSScope::clone(result));
    return true;
}

void QQmlJSTypeResolver::adjustOriginalType(
        const QQmlJSScope::ConstPtr &tracked, const QQmlJSScope::ConstPtr &conversion) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return;

    const auto it = m_trackedTypes->find(tracked);
    Q_ASSERT(it != m_trackedTypes->end());

    it->original = conversion;
    *it->clone = std::move(*QQmlJSScope::clone(conversion));
}

void QQmlJSTypeResolver::generalizeType(const QQmlJSScope::ConstPtr &type) const
{
    if (m_cloneMode == QQmlJSTypeResolver::DoNotCloneTypes)
        return;

    const auto it = m_trackedTypes->find(type);
    Q_ASSERT(it != m_trackedTypes->end());
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
    if (canPrimitivelyConvertFromTo(from, to) || selectConstructor(to, from, nullptr).isValid())
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
        if (toTypeName == u"QPoint"_s || toTypeName == u"QPointF"_s
                || toTypeName == u"QSize"_s || toTypeName == u"QSizeF"_s
                || toTypeName == u"QRect"_s || toTypeName == u"QRectF"_s) {
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

    QQmlJSScope::ConstPtr aResultScope;
    if (a.isConversion()) {
        origins.append(a.conversionOrigins());
        aResultScope = a.conversionResultScope();
    } else {
        origins.append(containedType(a));
        aResultScope = a.scopeType();
    }

    QQmlJSScope::ConstPtr bResultScope;
    if (b.isConversion()) {
        origins.append(b.conversionOrigins());
        bResultScope = b.conversionResultScope();
    } else {
        origins.append(containedType(b));
        bResultScope = b.scopeType();
    }

    std::sort(origins.begin(), origins.end());
    const auto erase = std::unique(origins.begin(), origins.end());
    origins.erase(erase, origins.end());

    return QQmlJSRegisterContent::create(
                merge(a.storedType(), b.storedType()),
                origins,
                merge(containedType(a), containedType(b)),
                merge(aResultScope, bResultScope),
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

    const auto isInt32Compatible = [&](const QQmlJSScope::ConstPtr &type) {
        return (isIntegral(type) && !equals(type, uint32Type())) || equals(type, boolType());
    };

    if (isInt32Compatible(a) && isInt32Compatible(b))
        return int32Type();

    const auto isUInt32Compatible = [&](const QQmlJSScope::ConstPtr &type) {
        return isUnsignedInteger(type) || equals(type, boolType());
    };

    if (isUInt32Compatible(a) && isUInt32Compatible(b))
        return uint32Type();

    if (isNumeric(a) && isNumeric(b))
        return realType();

    const auto isStringCompatible = [&](const QQmlJSScope::ConstPtr &type) {
        // TODO: We can losslessly coerce more types to string. Should we?
        return isIntegral(type) || equals(type, stringType());
    };

    if (isStringCompatible(a) && isStringCompatible(b))
        return stringType();

    if (isPrimitive(a) && isPrimitive(b))
        return jsPrimitiveType();

    if (auto commonBase = commonBaseType(a, b))
        return commonBase;

    if ((equals(a, nullType()) || equals(a, boolType())) && b->isReferenceType())
        return b;

    if ((equals(b, nullType()) || equals(b, boolType())) && a->isReferenceType())
        return a;

    return varType();
}

bool QQmlJSTypeResolver::canHold(
        const QQmlJSScope::ConstPtr &container, const QQmlJSScope::ConstPtr &contained) const
{
    if (equals(container, contained)
            || equals(container, m_varType)
            || equals(container, m_jsValueType)) {
        return true;
    }

    if (equals(container, m_jsPrimitiveType))
        return isPrimitive(contained);

    if (equals(container, m_variantListType))
        return contained->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence;

    if (equals(container, m_qObjectListType) || equals(container, m_listPropertyType)) {
        if (contained->accessSemantics() != QQmlJSScope::AccessSemantics::Sequence)
            return false;
        if (QQmlJSScope::ConstPtr value = contained->valueType())
            return value->isReferenceType();
        return false;
    }

    if (QQmlJSUtils::searchBaseAndExtensionTypes(
                container, [&](const QQmlJSScope::ConstPtr &base) {
        return equals(base, contained);
    })) {
        return true;
    }

    if (container->isReferenceType()) {
        if (QQmlJSUtils::searchBaseAndExtensionTypes(
                    contained, [&](const QQmlJSScope::ConstPtr &base) {
            return equals(base, container);
        })) {
            return true;
        }
    }

    return false;
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

QQmlJSScope::ConstPtr QQmlJSTypeResolver::genericType(
        const QQmlJSScope::ConstPtr &type,
        ComponentIsGeneric allowComponent) const
{
    if (type->isScript())
        return m_jsValueType;

    if (equals(type, m_metaObjectType))
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

        m_logger->log(u"Object type %1 is not derived from QObject or QQmlComponent. "
                      "You may need to fully qualify all names in C++ so that moc can see them. "
                      "You may also need to add qt_extract_metatypes(<target containing %2>)."_s
                      .arg(type->internalName(), unresolvedBaseTypeName),
                      qmlCompiler, type->sourceLocation());

        // Reference types that are not QObject or QQmlComponent are likely JavaScript objects.
        // We don't want to deal with those, but m_jsValueType is the best generic option.
        return m_jsValueType;
    }

    if (type->isListProperty())
        return m_listPropertyType;

    if (type->scopeType() == QQmlSA::ScopeType::EnumScope)
        return type->baseType();

    if (isPrimitive(type))
        return type;

    for (const QQmlJSScope::ConstPtr &builtin : {
                 m_realType, m_floatType, m_int8Type, m_uint8Type, m_int16Type, m_uint16Type,
                 m_int32Type, m_uint32Type, m_int64Type, m_uint64Type, m_boolType, m_stringType,
                 m_stringListType, m_byteArrayType, m_urlType, m_dateTimeType, m_dateType,
                 m_timeType, m_variantListType, m_variantMapType, m_varType, m_jsValueType,
                 m_jsPrimitiveType, m_listPropertyType, m_qObjectType, m_qObjectListType,
                 m_metaObjectType }) {
        if (equals(type, builtin) || equals(type, builtin->listType()))
            return type;
    }

    return m_varType;
}

QQmlJSRegisterContent QQmlJSTypeResolver::builtinType(const QQmlJSScope::ConstPtr &type) const
{
    Q_ASSERT(storedType(type) == type);
    return QQmlJSRegisterContent::create(type, type, QQmlJSRegisterContent::Builtin);
}

QQmlJSRegisterContent QQmlJSTypeResolver::globalType(const QQmlJSScope::ConstPtr &type) const
{
    return QQmlJSRegisterContent::create(storedType(type), type, QQmlJSRegisterContent::Unknown);
}

static QQmlJSRegisterContent::ContentVariant scopeContentVariant(QQmlJSScope::ExtensionKind mode,
                                                                 bool isMethod)
{
    switch (mode) {
    case QQmlJSScope::NotExtension:
        return isMethod ? QQmlJSRegisterContent::ScopeMethod : QQmlJSRegisterContent::ScopeProperty;
    case QQmlJSScope::ExtensionType:
        return isMethod ? QQmlJSRegisterContent::ExtensionScopeMethod
                        : QQmlJSRegisterContent::ExtensionScopeProperty;
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

QQmlJSRegisterContent QQmlJSTypeResolver::scopedType(const QQmlJSScope::ConstPtr &scope,
                                                     const QString &name) const
{
    const auto isAssignedToDefaultProperty = [this](const QQmlJSScope::ConstPtr &parent,
                                                    const QQmlJSScope::ConstPtr &child) {
        const QString defaultPropertyName = parent->defaultPropertyName();
        if (defaultPropertyName.isEmpty()) // no reason to search for bindings
            return false;

        const QList<QQmlJSMetaPropertyBinding> defaultPropBindings =
                parent->propertyBindings(defaultPropertyName);
        for (const QQmlJSMetaPropertyBinding &binding : defaultPropBindings) {
            if (binding.bindingType() == QQmlSA::BindingType::Object
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
        if (QQmlJSUtils::searchBaseAndExtensionTypes(
                    base, [&](const QQmlJSScope::ConstPtr &found, QQmlJSScope::ExtensionKind mode) {
                        if (mode == QQmlJSScope::ExtensionNamespace) // no use for it here
                            return false;
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
                                    storedType(prop.type()),
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

    QQmlJSRegisterContent result = registerContentForName(name);

    if (result.isValid())
        return result;

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
                                    QQmlJSRegisterContent *result,
                                    QQmlJSScope::ExtensionKind mode) const
{
    // You can't have lower case enum names in QML, even if we know the enums here.
    if (name.isEmpty() || !name.at(0).isUpper())
        return false;

    const bool inExtension =
            (mode == QQmlJSScope::ExtensionType) || (mode == QQmlJSScope::ExtensionNamespace);

    const auto enums = scope->ownEnumerations();
    for (const auto &enumeration : enums) {
        if (enumeration.name() == name) {
            *result = QQmlJSRegisterContent::create(
                    storedType(enumeration.type()), enumeration, QString(),
                    inExtension ? QQmlJSRegisterContent::ExtensionObjectEnum
                                : QQmlJSRegisterContent::ObjectEnum,
                    scope);
            return true;
        }

        if (enumeration.hasKey(name)) {
            *result = QQmlJSRegisterContent::create(
                    storedType(enumeration.type()), enumeration, name,
                    inExtension ? QQmlJSRegisterContent::ExtensionObjectEnum
                                : QQmlJSRegisterContent::ObjectEnum,
                    scope);
            return true;
        }
    }

    return false;
}

QQmlJSMetaMethod QQmlJSTypeResolver::selectConstructor(
        const QQmlJSScope::ConstPtr &type, const QQmlJSScope::ConstPtr &passedArgumentType, bool *isExtension) const
{
    // If the "from" type can hold the target type, we should not try to coerce
    // it to any constructor argument.
    if (type.isNull() || canHold(passedArgumentType, type))
        return QQmlJSMetaMethod();

    if (QQmlJSScope::ConstPtr extension = type->extensionType().scope) {
        const QQmlJSMetaMethod ctor = selectConstructor(extension, passedArgumentType, nullptr);
        if (ctor.isValid()) {
            if (isExtension)
                *isExtension = true;
            return ctor;
        }
    }

    if (isExtension)
        *isExtension = false;

    QQmlJSMetaMethod candidate;
    if (!type->isCreatable() || type->accessSemantics() != QQmlJSScope::AccessSemantics::Value)
        return candidate;

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

        if (equals(passedArgumentType, methodArgumentType))
            return method;

        // Do not select further ctors here. We don't want to do multi-step construction as that
        // is confusing and easily leads to infinite recursion.
        if (!candidate.isValid()
                && canPrimitivelyConvertFromTo(passedArgumentType, methodArgumentType)) {
            candidate = method;
        }
    }

    return candidate;
}

bool QQmlJSTypeResolver::areEquivalentLists(
        const QQmlJSScope::ConstPtr &a, const QQmlJSScope::ConstPtr &b) const
{
    const QQmlJSScope::ConstPtr equivalentLists[2][2] = {
        { m_stringListType, m_stringType->listType() },
        { m_variantListType, m_varType->listType() }
    };

    for (const auto eq : equivalentLists) {
        if ((equals(a, eq[0]) && equals(b, eq[1])) || (equals(a, eq[1]) && equals(b, eq[0])))
            return true;
    }

    return false;
}

bool QQmlJSTypeResolver::inherits(const QQmlJSScope::ConstPtr &derived, const QQmlJSScope::ConstPtr &base) const
{
    const bool matchByName = !base->isComposite();
    for (QQmlJSScope::ConstPtr derivedBase = derived; derivedBase;
            derivedBase = derivedBase->baseType()) {
        if (equals(derivedBase, base))
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
            && (equals(to, m_boolType) || equals(to, m_stringType))) {
        return true;
    }

    // Yes, our String has number constructors.
    if (isNumeric(from) && equals(to, m_stringType))
        return true;

    // We can convert strings to numbers, but not to enums
    if (equals(from, m_stringType) && isNumeric(to))
        return to->scopeType() != QQmlJSScope::ScopeType::EnumScope;

    // We can always convert between strings and urls.
    if ((equals(from, m_stringType) && equals(to, m_urlType))
            || (equals(from, m_urlType) && equals(to, m_stringType))) {
        return true;
    }

    // We can always convert between strings and byte arrays.
    if ((equals(from, m_stringType) && equals(to, m_byteArrayType))
            || (equals(from, m_byteArrayType) && equals(to, m_stringType))) {
        return true;
    }

    if (equals(to, m_voidType))
        return true;

    if (to.isNull())
        return equals(from, m_voidType);

    const auto types = { m_dateTimeType, m_dateType, m_timeType, m_stringType };
    for (const auto &originType : types) {
        if (!equals(from, originType))
            continue;

        for (const auto &targetType : types) {
            if (equals(to, targetType))
                return true;
        }

        break;;
    }

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

    if (equals(from, m_variantListType))
        return to->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence;

    const bool matchByName = !to->isComposite();
    Q_ASSERT(!matchByName || !to->internalName().isEmpty());
    for (auto baseType = from; baseType; baseType = baseType->baseType()) {
        if (equals(baseType, to))
            return true;
        if (matchByName && baseType->internalName() == to->internalName())
            return true;
    }

    // We can convert anything that fits into QJSPrimitiveValue
    if (canConvertFromTo(from, m_jsPrimitiveType) && canConvertFromTo(m_jsPrimitiveType, to))
        return true;

    // We can convert everything to bool.
    if (equals(to, m_boolType))
        return true;

    if (areEquivalentLists(from, to))
        return true;

    if (from->isListProperty()
            && to->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence
            && canConvertFromTo(from->valueType(), to->valueType())) {
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
    prop.setType(int32Type());
    prop.setIsWritable(isWritable);
    return QQmlJSRegisterContent::create(int32Type(), prop, QQmlJSRegisterContent::Builtin, scope);
}

QQmlJSRegisterContent QQmlJSTypeResolver::memberType(const QQmlJSScope::ConstPtr &type,
                                                     const QString &name) const
{
    QQmlJSRegisterContent result;

    // If we got a plain type reference we have to check the enums of the _scope_.
    if (equals(type, metaObjectType()))
        return {};

    if (equals(type, variantMapType())) {
        QQmlJSMetaProperty prop;
        prop.setPropertyName(name);
        prop.setTypeName(u"QVariant"_s);
        prop.setType(varType());
        prop.setIsWritable(true);
        return QQmlJSRegisterContent::create(varType(), prop,
                                             QQmlJSRegisterContent::GenericObjectProperty, type);
    }

    if (equals(type, jsValueType())) {
        QQmlJSMetaProperty prop;
        prop.setPropertyName(name);
        prop.setTypeName(u"QJSValue"_s);
        prop.setType(jsValueType());
        prop.setIsWritable(true);
        return QQmlJSRegisterContent::create(jsValueType(), prop,
                                             QQmlJSRegisterContent::GenericObjectProperty, type);
    }

    if ((equals(type, stringType())
         || type->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence)
            && name == u"length"_s) {
        return lengthProperty(!equals(type, stringType()), type);
    }

    const auto check = [&](const QQmlJSScope::ConstPtr &scope, QQmlJSScope::ExtensionKind mode) {
        if (mode != QQmlJSScope::ExtensionNamespace) {
            if (scope->hasOwnProperty(name)) {
                const auto prop = scope->ownProperty(name);
                result = QQmlJSRegisterContent::create(
                        storedType(prop.type()),
                        prop,
                        mode == QQmlJSScope::NotExtension
                                ? QQmlJSRegisterContent::ObjectProperty
                                : QQmlJSRegisterContent::ExtensionObjectProperty,
                        scope);
                return true;
            }

            if (scope->hasOwnMethod(name)) {
                const auto methods = scope->ownMethods(name);
                result = QQmlJSRegisterContent::create(
                        jsValueType(), methods,
                        mode == QQmlJSScope::NotExtension
                                ? QQmlJSRegisterContent::ObjectMethod
                                : QQmlJSRegisterContent::ExtensionObjectMethod,
                        scope);
                return true;
            }
        }

        return checkEnums(scope, name, &result, mode);
    };

    if (QQmlJSUtils::searchBaseAndExtensionTypes(type, check))
        return result;

    for (auto scope = type;
         scope && (scope->scopeType() == QQmlSA::ScopeType::JSFunctionScope
                   || scope->scopeType() == QQmlSA::ScopeType::JSLexicalScope);
         scope = scope->parentScope()) {
        if (auto ownIdentifier = scope->JSIdentifier(name)) {
            QQmlJSMetaProperty prop;
            prop.setPropertyName(name);
            prop.setTypeName(u"QJSValue"_s);
            prop.setType(jsValueType());
            prop.setIsWritable(!(ownIdentifier.value().isConst));

            return QQmlJSRegisterContent::create(jsValueType(), prop,
                                                 QQmlJSRegisterContent::JavaScriptObject, scope);
        }
    }

    if (QQmlJSScope::ConstPtr attachedBase = typeForName(name)) {
        if (QQmlJSScope::ConstPtr attached = attachedBase->attachedType()) {
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

    if (QQmlJSUtils::searchBaseAndExtensionTypes(
                type, [&](const QQmlJSScope::ConstPtr &scope, QQmlJSScope::ExtensionKind mode) {
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
    if (type.isProperty())
        return memberType(type.property().type(), name);
    if (type.isEnumeration()) {
        const auto enumeration = type.enumeration();
        if (!type.enumMember().isEmpty() || !enumeration.hasKey(name))
            return {};
        return QQmlJSRegisterContent::create(storedType(enumeration.type()), enumeration, name,
                                             QQmlJSRegisterContent::ObjectEnum, type.scopeType());
    }
    if (type.isMethod()) {
        QQmlJSMetaProperty prop;
        prop.setTypeName(u"QJSValue"_s);
        prop.setPropertyName(name);
        prop.setType(jsValueType());
        prop.setIsWritable(true);
        return QQmlJSRegisterContent::create(
                jsValueType(), prop, QQmlJSRegisterContent::GenericObjectProperty, jsValueType());
    }
    if (type.isImportNamespace()) {
        if (type.scopeType()->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
            m_logger->log(u"Cannot use a non-QObject type %1 to access prefixed import"_s.arg(
                                  type.scopeType()->internalName()),
                          qmlPrefixedImportType, type.scopeType()->sourceLocation());
            return {};
        }

        return registerContentForName(
                    name, type.scopeType(),
                    type.variant() == QQmlJSRegisterContent::ObjectModulePrefix);
    }
    if (type.isConversion()) {
        if (const auto result = memberType(type.conversionResult(), name); result.isValid())
            return result;
        if (const auto result = memberEnumType(type.scopeType(), name); result.isValid())
            return result;

        // If the conversion consists of only undefined and one actual type,
        // we can produce the members of that one type.
        // If the value is then actually undefined, the result is an exception.

        auto origins = type.conversionOrigins();
        const auto begin = origins.begin();
        const auto end = std::remove_if(begin, origins.end(),
                       [this](const QQmlJSScope::ConstPtr &origin) {
            return equals(origin, m_voidType);
        });

        // If the conversion cannot hold the original type, it loses information.
        return (end - begin == 1 && canHold(type.conversionResult(), *begin))
                ? memberType(*begin, name)
                : QQmlJSRegisterContent();
    }

    Q_UNREACHABLE_RETURN({});
}

QQmlJSRegisterContent QQmlJSTypeResolver::valueType(const QQmlJSRegisterContent &list) const
{
    QQmlJSScope::ConstPtr scope;
    QQmlJSScope::ConstPtr value;

    auto valueType = [this](const QQmlJSScope::ConstPtr &scope) {
        if (scope->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence)
            return scope->valueType();
        else if (equals(scope, m_jsValueType) || equals(scope, m_varType))
            return m_jsValueType;
        else if (equals(scope, m_stringType))
            return m_stringType;
        return QQmlJSScope::ConstPtr();
    };

    if (list.isType()) {
        scope = list.type();
        value = valueType(scope);
    } else if (list.isConversion()) {
        value = valueType(list.conversionResult());
    } else if (list.isProperty()) {
        const auto prop = list.property();
        scope = prop.type();
        value = valueType(scope);
    }

    if (value.isNull())
        return {};

    QQmlJSMetaProperty property;
    property.setPropertyName(u"[]"_s);
    property.setTypeName(value->internalName());
    property.setType(value);

    return QQmlJSRegisterContent::create(
            storedType(value), property, QQmlJSRegisterContent::ListValue, scope);
}

QQmlJSRegisterContent QQmlJSTypeResolver::returnType(
        const QQmlJSScope::ConstPtr &type, QQmlJSRegisterContent::ContentVariant variant,
        const QQmlJSScope::ConstPtr &scope) const
{
    Q_ASSERT(variant == QQmlJSRegisterContent::MethodReturnValue
             || variant == QQmlJSRegisterContent::JavaScriptReturnValue);
    return QQmlJSRegisterContent::create(storedType(type), type, variant, scope);
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
    if (reg.isProperty())
        return equals(type, reg.property().type());
    if (reg.isEnumeration())
        return equals(type, reg.enumeration().type());
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
    const auto it = m_trackedTypes->find(type);
    return it == m_trackedTypes->end() ? type : it->original;
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
                to.storedType(), from.conversionOrigins(), containedType(to),
                to.scopeType() ? to.scopeType() : from.conversionResultScope(),
                from.variant(), from.scopeType());
    }

    return QQmlJSRegisterContent::create(
            to.storedType(), QList<QQmlJSScope::ConstPtr>{containedType(from)},
            containedType(to), to.scopeType(), from.variant(), from.scopeType());
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::comparableType(const QQmlJSScope::ConstPtr &type) const
{
    const auto it = m_trackedTypes->constFind(type);
    if (it == m_trackedTypes->constEnd())
        return type;
    return it->replacement ? it->replacement : it->original;
}

QT_END_NAMESPACE
