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

#include <private/qqmljsimporter_p.h>
#include <private/qqmljsimportvisitor_p.h>
#include <private/qqmljslogger_p.h>
#include <private/qv4value_p.h>

#include <private/qduplicatetracker_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTypeResolver, "qt.qml.compiler.typeresolver", QtInfoMsg);

template<typename Action>
static bool searchBaseAndExtensionTypes(const QQmlJSScope::ConstPtr type, const Action &check)
{
    if (!type)
        return false;

    const bool isValueType = (type->accessSemantics() == QQmlJSScope::AccessSemantics::Value);

    QDuplicateTracker<QQmlJSScope::ConstPtr> seen;
    for (QQmlJSScope::ConstPtr scope = type; scope && !seen.hasSeen(scope);
         scope = scope->baseType()) {

        QDuplicateTracker<QQmlJSScope::ConstPtr> seenExtensions;
        // Extensions override the types they extend. However, usually base
        // types of extensions are ignored. The unusual cases are when we
        // have a value type or when we have the QObject type, in which case
        // we also study the extension's base type hierarchy.
        const bool isQObject = scope->internalName() == QLatin1String("QObject");
        QQmlJSScope::ConstPtr extension = scope->extensionType();
        do {
            if (!extension || seenExtensions.hasSeen(extension))
                break;
            if (check(extension, QQmlJSTypeResolver::Extension))
                return true;
            extension = extension->baseType();
        } while (isValueType || isQObject);

        if (check(scope, QQmlJSTypeResolver::Base))
            return true;
    }

    return false;
}

QQmlJSTypeResolver::QQmlJSTypeResolver(QQmlJSImporter *importer)
{
    const QQmlJSImporter::ImportedTypes builtinTypes = importer->builtinInternalNames();
    m_voidType = builtinTypes[u"void"_qs].scope;
    m_realType = builtinTypes[u"double"_qs].scope;
    m_floatType = builtinTypes[u"float"_qs].scope;
    m_intType = builtinTypes[u"int"_qs].scope;
    m_boolType = builtinTypes[u"bool"_qs].scope;
    m_stringType = builtinTypes[u"QString"_qs].scope;
    m_urlType = builtinTypes[u"QUrl"_qs].scope;
    m_dateTimeType = builtinTypes[u"QDateTime"_qs].scope;
    m_variantListType = builtinTypes[u"QVariantList"_qs].scope;
    m_varType = builtinTypes[u"QVariant"_qs].scope;
    m_jsValueType = builtinTypes[u"QJSValue"_qs].scope;

    QQmlJSScope::Ptr jsPrimitiveType = QQmlJSScope::create();
    jsPrimitiveType->setInternalName(u"QJSPrimitiveValue"_qs);
    jsPrimitiveType->setFileName(u"qjsprimitivevalue.h"_qs);
    jsPrimitiveType->setAccessSemantics(QQmlJSScope::AccessSemantics::Value);
    m_jsPrimitiveType = jsPrimitiveType;

    QQmlJSScope::Ptr listPropertyType = QQmlJSScope::create();
    listPropertyType->setInternalName(u"QQmlListProperty<QObject>"_qs);
    listPropertyType->setFileName(u"qqmllist.h"_qs);
    listPropertyType->setAccessSemantics(QQmlJSScope::AccessSemantics::Sequence);
    m_listPropertyType = listPropertyType;

    QQmlJSScope::Ptr metaObjectType = QQmlJSScope::create();
    metaObjectType->setInternalName(u"const QMetaObject"_qs);
    metaObjectType->setFileName(u"qmetaobject.h"_qs);
    metaObjectType->setAccessSemantics(QQmlJSScope::AccessSemantics::Reference);
    m_metaObjectType = metaObjectType;

    m_jsGlobalObject = importer->jsGlobalObject();
    auto numberMethods = m_jsGlobalObject->methods(u"Number"_qs);
    Q_ASSERT(numberMethods.length() == 1);
    m_numberPrototype = numberMethods[0].returnType()->baseType();
    Q_ASSERT(m_numberPrototype);
    Q_ASSERT(m_numberPrototype->internalName() == u"NumberPrototype"_qs);
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
            << "looking for object at " << location.line() << ':' << location.column();
    return m_objectsByLocation[location];
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::scopeForId(
        const QString &id, const QQmlJSScope::ConstPtr &referrer) const
{
    return m_objectsById.scope(id, referrer);
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
        if (leftContents == stringType() || rightContents == stringType())
            return QQmlJSRegisterContent::create(stringType(), stringType(),
                                                 QQmlJSRegisterContent::Builtin);

        const QQmlJSScope::ConstPtr result = merge(leftContents, rightContents);
        if (result == boolType())
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
        if (result == boolType())
            return QQmlJSRegisterContent::create(intType(), intType(),
                                                 QQmlJSRegisterContent::Builtin);
        return QQmlJSRegisterContent::create(realType(), realType(),
                                             QQmlJSRegisterContent::Builtin);
    }
    case QSOperator::Op::Mul:
    case QSOperator::Op::Div:
    case QSOperator::Op::Exp:
    case QSOperator::Op::Mod:
        return QQmlJSRegisterContent::create(
                realType(), realType(), QQmlJSRegisterContent::Builtin);
    case QSOperator::Op::As:
        return right;
    default:
        break;
    }

    return merge(left, right);
}

QQmlJSRegisterContent
QQmlJSTypeResolver::typeForUnaryOperation(UnaryOperator oper,
                                          const QQmlJSRegisterContent &operand) const
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

bool QQmlJSTypeResolver::isIntegral(const QQmlJSRegisterContent &type) const
{
    return containedType(type) == m_intType;
}

bool QQmlJSTypeResolver::canHoldUndefined(const QQmlJSRegisterContent &content) const
{
    const auto canBeUndefined = [this](const QQmlJSScope::ConstPtr &type) {
        return type == m_voidType || type == m_varType
                || type == m_jsValueType || type == m_jsPrimitiveType;
    };

    return canBeUndefined(content.storedType()) && canBeUndefined(containedType(content));
}

bool QQmlJSTypeResolver::isPrimitive(const QQmlJSScope::ConstPtr &type) const
{
    return type == m_intType || type == m_realType || type == m_floatType || type == m_boolType
            || type == m_voidType || type == m_stringType || type == m_jsPrimitiveType;
}

bool QQmlJSTypeResolver::isNumeric(const QQmlJSScope::ConstPtr &type) const
{
    return searchBaseAndExtensionTypes(
                type, [&](const QQmlJSScope::ConstPtr &scope, BaseOrExtension) {
        return scope == m_numberPrototype;
    });
}

QQmlJSScope::ConstPtr
QQmlJSTypeResolver::containedType(const QQmlJSRegisterContent &container) const
{
    if (container.isType())
        return container.type();
    if (container.isProperty()) {
        const QQmlJSMetaProperty prop = container.property();
        return prop.isList() ? listPropertyType() : QQmlJSScope::ConstPtr(prop.type());
    }
    if (container.isEnumeration())
        return container.enumeration().type();
    if (container.isMethod())
        return jsValueType();
    if (container.isImportNamespace())
        return container.scopeType();

    Q_UNREACHABLE();
    return {};
}

QString QQmlJSTypeResolver::containedTypeName(const QQmlJSRegisterContent &container) const
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

    return type->internalName().isEmpty() ? type->baseTypeName() : type->internalName();
}

bool QQmlJSTypeResolver::canConvertFromTo(const QQmlJSScope::ConstPtr &from,
                                          const QQmlJSScope::ConstPtr &to) const
{
    // ### need a generic solution for custom cpp types:
    // if (from->m_hasBoolOverload && to == boolType)
    //    return true;

    if (from == to)
        return true;
    if (from == m_varType || to == m_varType)
        return true;
    if (from == m_jsValueType || to == m_jsValueType)
        return true;
    if (isNumeric(from) && isNumeric(to))
        return true;
    if (from == m_intType && to == m_boolType)
        return true;
    if (from->accessSemantics() == QQmlJSScope::AccessSemantics::Reference && to == m_boolType)
        return true;

    // Yes, our String has number constructors.
    if (isNumeric(from) && to == m_stringType)
        return true;

    // We can always convert between strings and urls.
    if ((from == m_stringType && to == m_urlType) || (from == m_urlType && to == m_stringType))
        return true;

    // All of these types have QString conversions that require a certain format
    // TODO: Actually verify these strings or deprecate them
    if (from == m_stringType && !to.isNull()) {
        const QString toTypeName = to->internalName();
        if (to == m_dateTimeType || toTypeName == u"QTime"_qs || toTypeName == u"QDate"_qs
            || toTypeName == u"QPoint"_qs || toTypeName == u"QPointF"_qs
            || toTypeName == u"QSize"_qs || toTypeName == u"QSizeF"_qs || toTypeName == u"QRect"_qs
            || toTypeName == u"QRectF"_qs || toTypeName == u"QColor"_qs) {
            return true;
        }
    }

    if (from == m_voidType)
        return true;

    if (to.isNull())
        return false;

    if (from == m_jsPrimitiveType) {
        // You can cast any primitive (in particular null) to a nullptr
        return isPrimitive(to) || to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference;
    }

    if (to == m_jsPrimitiveType)
        return isPrimitive(from);

    const bool matchByName = !to->isComposite();
    Q_ASSERT(!matchByName || !to->internalName().isEmpty());
    for (auto baseType = from; baseType; baseType = baseType->baseType()) {
        if (baseType == to)
            return true;
        if (matchByName && baseType->internalName() == to->internalName())
            return true;
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
    return QQmlJSRegisterContent::create(
            merge(a.storedType(), b.storedType()),
            merge(containedType(a), containedType(b)), mergeVariants(a.variant(), b.variant()),
            merge(a.scopeType(), b.scopeType()));
}

static QQmlJSScope::ConstPtr commonBaseType(const QQmlJSScope::ConstPtr &a,
                                            const QQmlJSScope::ConstPtr &b)
{
    for (QQmlJSScope::ConstPtr aBase = a; aBase; aBase = aBase->baseType()) {
        for (QQmlJSScope::ConstPtr bBase = b; bBase; bBase = bBase->baseType()) {
            if (aBase == bBase)
                return aBase;
        }
    }

    return {};
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::merge(const QQmlJSScope::ConstPtr &a,
                                                const QQmlJSScope::ConstPtr &b) const
{
    if (a == b)
        return a;

    if (a == jsValueType() || a == varType())
        return a;
    if (b == jsValueType() || b == varType())
        return b;

    auto canConvert = [&](const QQmlJSScope::ConstPtr &from, const QQmlJSScope::ConstPtr &to) {
        return (a == from && b == to) || (b == from && a == to);
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

    return varType();
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::genericType(const QQmlJSScope::ConstPtr &type,
                                                      ComponentIsGeneric allowComponent) const
{
    if (type->isScript())
        return m_jsValueType;

    if (type == m_metaObjectType)
        return m_metaObjectType;

    if (type->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
        for (auto base = type; base; base = base->baseType()) {
            // QObject and QQmlComponent are the two required base types.
            // Any QML type system has to define those, or use the ones from builtins.
            // As QQmlComponent is derived from QObject, we can restrict ourselves to the latter.
            // This results in less if'ery when retrieving a QObject* from somewhere and deciding
            // what it is.
            if (base->internalName() == u"QObject"_qs) {
                return base;
            } else if (allowComponent == ComponentIsGeneric::Yes
                       && base->internalName() == u"QQmlComponent"_qs) {
                return base;
            }
        }

        m_logger->logWarning(u"Object type %1 is not derived from QObject or QQmlComponent"_qs.arg(
                                     type->internalName()),
                             Log_Compiler);

        // Reference types that are not QObject or QQmlComponent are likely JavaScript objects.
        // We don't want to deal with those, but m_jsValueType is the best generic option.
        return m_jsValueType;
    }

    if (type == voidType())
        return jsPrimitiveType();

    if (isPrimitive(type) || type == m_jsValueType || type == m_listPropertyType
        || type == m_urlType || type == m_dateTimeType || type == m_variantListType
        || type == m_varType) {
        return type;
    }

    if (isNumeric(type))
        return m_realType;

    if (type->scopeType() == QQmlJSScope::EnumScope)
        return m_intType;

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

static bool isAssignedToDefaultProperty(const QQmlJSScope::ConstPtr &parent,
                                        const QQmlJSScope::ConstPtr &child)
{
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
            && binding.objectType() == child) {
            return true;
        }
    }
    return false;
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
                m_logger->logWarning(u"Cannot resolve generic base of attached %1"_qs.arg(
                                             attached->internalName()),
                                     Log_Compiler);
                return {};
            } else if (type->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
                m_logger->logWarning(
                        u"Cannot retrieve attached object for non-reference type %1"_qs.arg(
                                type->internalName()),
                        Log_Compiler);
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

QQmlJSRegisterContent QQmlJSTypeResolver::lengthProperty(
        bool isWritable, const QQmlJSScope::ConstPtr &scope) const
{
    QQmlJSMetaProperty prop;
    prop.setPropertyName(u"length"_qs);
    prop.setTypeName(u"int"_qs);
    prop.setType(intType());
    prop.setIsWritable(isWritable);
    return QQmlJSRegisterContent::create(intType(), prop, QQmlJSRegisterContent::Builtin, scope);
}

QQmlJSRegisterContent QQmlJSTypeResolver::memberType(const QQmlJSScope::ConstPtr &type,
                                                     const QString &name) const
{
    QQmlJSRegisterContent result;

    if (type == jsValueType()) {
        QQmlJSMetaProperty prop;
        prop.setPropertyName(name);
        prop.setTypeName(u"QJSValue"_qs);
        prop.setType(jsValueType());
        prop.setIsWritable(true);
        return QQmlJSRegisterContent::create(jsValueType(), prop,
                                             QQmlJSRegisterContent::JavaScriptObjectProperty, type);
    }

    if ((type == stringType() || type->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence)
            && name == u"length"_qs) {
        return lengthProperty(type != stringType(), type);
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

        return checkEnums(scope, name, &result, mode);
    };

    if (searchBaseAndExtensionTypes(type, check))
        return result;

    if (QQmlJSScope::ConstPtr attachedBase = typeForName(name)) {
        if (QQmlJSScope::ConstPtr attached = attachedBase->attachedType()) {
            if (!genericType(attached)) {
                m_logger->logWarning(u"Cannot resolve generic base of attached %1"_qs.arg(
                                             attached->internalName()),
                                     Log_Compiler);
                return {};
            } else if (type->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
                m_logger->logWarning(
                        u"Cannot retrieve attached object for non-reference type %1"_qs.arg(
                                type->internalName()),
                        Log_Compiler);
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
        if (prop.isList() && name == u"length"_qs)
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
        prop.setTypeName(u"QJSValue"_qs);
        prop.setPropertyName(name);
        prop.setType(jsValueType());
        prop.setIsWritable(true);
        return QQmlJSRegisterContent::create(jsValueType(), prop,
                                             QQmlJSRegisterContent::JavaScriptObjectProperty,
                                             jsValueType());
    }
    if (type.isImportNamespace()) {
        if (type.scopeType()->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
            m_logger->logWarning(
                    u"Cannot use non-reference type %1 as base of namespaced attached type"_qs.arg(
                            type.scopeType()->internalName()),
                    Log_Type);
            return {};
        }

        if (QQmlJSScope::ConstPtr result = typeForName(name)) {
            QQmlJSScope::ConstPtr attached = result->attachedType();
            if (attached && genericType(attached)) {
                return QQmlJSRegisterContent::create(storedType(attached), attached,
                                                     QQmlJSRegisterContent::ObjectAttached,
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
        else if (scope == m_jsValueType || scope == m_varType)
            return m_jsValueType;
        return QQmlJSScope::ConstPtr();
    };

    if (listType.isType()) {
        scope = listType.type();
        value = valueType(scope);
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
    property.setPropertyName(u"[]"_qs);
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

bool QQmlJSTypeResolver::registerContains(const QQmlJSRegisterContent &reg,
                                          const QQmlJSScope::ConstPtr &type) const
{
    if (reg.isType())
        return reg.type() == type;
    if (reg.isProperty()) {
        const auto prop = reg.property();
        return prop.isList() ? type == listPropertyType() : prop.type() == type;
    }
    if (reg.isEnumeration())
        return type == intType();
    if (reg.isMethod())
        return type == jsValueType();
    return false;
}

QQmlJSScope::ConstPtr QQmlJSTypeResolver::storedType(const QQmlJSScope::ConstPtr &type) const
{
    if (type.isNull())
        return {};
    if (type == voidType())
        return jsPrimitiveType();
    if (type->isScript())
        return jsValueType();
    if (type->isComposite()) {
        if (const QQmlJSScope::ConstPtr nonComposite = QQmlJSScope::nonCompositeBaseType(type))
            return nonComposite;

        // If we can't find the non-composite base, we really don't know what it is.
        return genericType(type);
    }
    if (type->fileName().isEmpty())
        return genericType(type);
    return type;
}

QT_END_NAMESPACE
