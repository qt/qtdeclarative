// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpropertyvalidator_p.h"

#include <private/qqmlcustomparser_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmlirbuilder_p.h>
#include <private/qqmlpropertycachecreator_p.h>
#include <private/qqmlpropertyresolver_p.h>
#include <private/qqmlstringconverters_p.h>
#include <private/qqmlsignalnames_p.h>

#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE

static bool isPrimitiveType(QMetaType metaType)
{
    switch (metaType.id()) {
#define HANDLE_PRIMITIVE(Type, id, T) \
    case QMetaType::Type:
QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(HANDLE_PRIMITIVE);
#undef HANDLE_PRIMITIVE
        return true;
    default:
        return false;
    }
}

QQmlPropertyValidator::QQmlPropertyValidator(
        QQmlTypeLoader *typeLoader, const QQmlImports *imports,
        const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit)
    : m_typeLoader(typeLoader)
    , compilationUnit(compilationUnit)
    , imports(imports)
    , qmlUnit(compilationUnit->unitData())
    , propertyCaches(compilationUnit->propertyCaches)
    , bindingPropertyDataPerObject(&compilationUnit->bindingPropertyDataPerObject)
{
    bindingPropertyDataPerObject->resize(compilationUnit->objectCount());
}

QVector<QQmlError> QQmlPropertyValidator::validate()
{
    return validateObject(/*root object*/0, /*instantiatingBinding*/nullptr);
}

typedef QVarLengthArray<const QV4::CompiledData::Binding *, 8> GroupPropertyVector;

struct BindingFinder
{
    bool operator()(quint32 name, const QV4::CompiledData::Binding *binding) const
    {
        return name < binding->propertyNameIndex;
    }
    bool operator()(const QV4::CompiledData::Binding *binding, quint32 name) const
    {
        return binding->propertyNameIndex < name;
    }
    bool operator()(const QV4::CompiledData::Binding *lhs, const QV4::CompiledData::Binding *rhs) const
    {
        return lhs->propertyNameIndex < rhs->propertyNameIndex;
    }
};

QVector<QQmlError> QQmlPropertyValidator::validateObject(
        int objectIndex, const QV4::CompiledData::Binding *instantiatingBinding,
        bool populatingValueTypeGroupProperty,
        QQmlPropertyResolver::RevisionCheck checkRevision) const
{
    const QV4::CompiledData::Object *obj = compilationUnit->objectAt(objectIndex);
    for (auto it = obj->inlineComponentsBegin(); it != obj->inlineComponentsEnd(); ++it) {
        const auto errors = validateObject(it->objectIndex, /* instantiatingBinding*/ nullptr);
        if (!errors.isEmpty())
            return errors;
    }

    if (obj->hasFlag(QV4::CompiledData::Object::IsComponent)
            && !obj->hasFlag(QV4::CompiledData::Object::IsInlineComponentRoot)) {
        Q_ASSERT(obj->nBindings == 1);
        const QV4::CompiledData::Binding *componentBinding = obj->bindingTable();
        Q_ASSERT(componentBinding->type() == QV4::CompiledData::Binding::Type_Object);
        return validateObject(componentBinding->value.objectIndex, componentBinding);
    }

    QQmlPropertyCache::ConstPtr propertyCache = propertyCaches.at(objectIndex);
    if (!propertyCache)
        return QVector<QQmlError>();

    QQmlCustomParser *customParser = nullptr;
    if (auto typeRef = resolvedType(obj->inheritedTypeNameIndex)) {

        // This binding instantiates a separate object. The separate object can have an ID and its
        // own group properties even if it's then assigned to a value type, for example a 'var', or
        // anything with an invokable ctor taking a QObject*.
        populatingValueTypeGroupProperty = false;

        const auto type = typeRef->type();
        if (type.isValid())
            customParser = type.customParser();
    }

    QList<const QV4::CompiledData::Binding*> customBindings;

    // Collect group properties first for sanity checking
    // vector values are sorted by property name string index.
    GroupPropertyVector groupProperties;
    const QV4::CompiledData::Binding *binding = obj->bindingTable();
    for (quint32 i = 0; i < obj->nBindings; ++i, ++binding) {
        if (!binding->isGroupProperty())
                continue;

        if (binding->hasFlag(QV4::CompiledData::Binding::IsOnAssignment))
            continue;

        if (populatingValueTypeGroupProperty) {
            return recordError(binding->location, tr("Property assignment expected"));
        }

        GroupPropertyVector::const_iterator pos = std::lower_bound(groupProperties.constBegin(), groupProperties.constEnd(), binding->propertyNameIndex, BindingFinder());
        groupProperties.insert(pos, binding);
    }

    QQmlPropertyResolver propertyResolver(propertyCache);

    QString defaultPropertyName;
    const QQmlPropertyData *defaultProperty = nullptr;
    if (obj->indexOfDefaultPropertyOrAlias != -1) {
        const QQmlPropertyCache *cache = propertyCache->parent().data();
        defaultPropertyName = cache->defaultPropertyName();
        defaultProperty = cache->defaultProperty();
    } else {
        defaultPropertyName = propertyCache->defaultPropertyName();
        defaultProperty = propertyCache->defaultProperty();
    }

    QV4::CompiledData::BindingPropertyData collectedBindingPropertyData(obj->nBindings);

    binding = obj->bindingTable();
    for (quint32 i = 0; i < obj->nBindings; ++i, ++binding) {
        QString name = stringAt(binding->propertyNameIndex);
        const QV4::CompiledData::Binding::Type bindingType = binding->type();
        const QV4::CompiledData::Binding::Flags bindingFlags = binding->flags();

        if (customParser) {
            if (bindingType == QV4::CompiledData::Binding::Type_AttachedProperty) {
                if (customParser->flags() & QQmlCustomParser::AcceptsAttachedProperties) {
                    customBindings << binding;
                    continue;
                }
            } else if (QQmlSignalNames::isHandlerName(name)
                       && !(customParser->flags() & QQmlCustomParser::AcceptsSignalHandlers)) {
                customBindings << binding;
                continue;
            }
        }

        bool bindingToDefaultProperty = false;
        bool isGroupProperty = instantiatingBinding
                && instantiatingBinding->type() == QV4::CompiledData::Binding::Type_GroupProperty;

        bool notInRevision = false;
        const QQmlPropertyData *pd = nullptr;
        if (!name.isEmpty()) {
            if (bindingFlags & QV4::CompiledData::Binding::IsSignalHandlerExpression
                    || bindingFlags & QV4::CompiledData::Binding::IsSignalHandlerObject) {
                pd = propertyResolver.signal(name, &notInRevision, checkRevision);
            } else {
                pd = propertyResolver.property(name, &notInRevision, checkRevision);
            }

            if (notInRevision) {
                QString typeName = stringAt(obj->inheritedTypeNameIndex);
                if (auto *objectType = resolvedType(obj->inheritedTypeNameIndex)) {
                    const auto type = objectType->type();
                    if (type.isValid()) {
                        const auto version = objectType->version();
                        return recordError(binding->location,
                                           tr("\"%1.%2\" is not available in %3 %4.%5.")
                                           .arg(typeName).arg(name).arg(type.module())
                                           .arg(version.majorVersion())
                                           .arg(version.minorVersion()));
                    }
                } else {
                    return recordError(binding->location, tr("\"%1.%2\" is not available due to component versioning.").arg(typeName).arg(name));
                }
            }
        } else {
           if (isGroupProperty)
               return recordError(binding->location, tr("Cannot assign a value directly to a grouped property"));

           pd = defaultProperty;
           name = defaultPropertyName;
           bindingToDefaultProperty = true;
        }

        if (pd)
            collectedBindingPropertyData[i] = pd;

        if (name.constData()->isUpper() && !binding->isAttachedProperty()) {
            QQmlType type;
            QQmlImportNamespace *typeNamespace = nullptr;
            imports->resolveType(
                    m_typeLoader, stringAt(binding->propertyNameIndex),
                    &type, nullptr, &typeNamespace);
            if (typeNamespace)
                return recordError(binding->location, tr("Invalid use of namespace"));
            return recordError(binding->location, tr("Invalid attached object assignment"));
        }

        if (bindingType >= QV4::CompiledData::Binding::Type_Object
                && (pd || binding->isAttachedProperty() || binding->isGroupProperty())) {
            const bool populatingValueTypeGroupProperty
                    = pd
                      && QQmlMetaType::metaObjectForValueType(pd->propType())
                      && !binding->hasFlag(QV4::CompiledData::Binding::IsOnAssignment);

            // As this is a sub-object, its properties are qualified. We can ignore revisions.
            const QVector<QQmlError> subObjectValidatorErrors = validateObject(
                    binding->value.objectIndex, binding, populatingValueTypeGroupProperty,
                    QQmlPropertyResolver::IgnoreRevision);

            if (!subObjectValidatorErrors.isEmpty())
                return subObjectValidatorErrors;
        }

        // Signal handlers were resolved and checked earlier in the signal handler conversion pass.
        if (binding->flags() & (QV4::CompiledData::Binding::IsSignalHandlerExpression
                              | QV4::CompiledData::Binding::IsSignalHandlerObject
                              | QV4::CompiledData::Binding::IsPropertyObserver)) {
            continue;
        }

        if ((pd && bindingType == QV4::CompiledData::Binding::Type_AttachedProperty)
                || (!pd && bindingType == QV4::CompiledData::Binding::Type_GroupProperty)) {
            if (instantiatingBinding && (instantiatingBinding->isAttachedProperty()
                                         || instantiatingBinding->isGroupProperty())) {
                return recordError(
                            binding->location, tr("%1 properties cannot be used here")
                            .arg(bindingType == QV4::CompiledData::Binding::Type_AttachedProperty
                                 ? QStringLiteral("Attached")
                                 : QStringLiteral("Group")));
            }
            continue;
        } else if (bindingType == QV4::CompiledData::Binding::Type_AttachedProperty) {
            continue;
        }

        if (pd) {
            GroupPropertyVector::const_iterator assignedGroupProperty = std::lower_bound(groupProperties.constBegin(), groupProperties.constEnd(), binding->propertyNameIndex, BindingFinder());
            const bool assigningToGroupProperty = assignedGroupProperty != groupProperties.constEnd() && !(binding->propertyNameIndex < (*assignedGroupProperty)->propertyNameIndex);

            if (!pd->isWritable()
                && !pd->isQList()
                && !binding->isGroupProperty()
                && !(bindingFlags & QV4::CompiledData::Binding::InitializerForReadOnlyDeclaration)
                ) {

                if (assigningToGroupProperty && bindingType < QV4::CompiledData::Binding::Type_Object)
                    return recordError(binding->valueLocation, tr("Cannot assign a value directly to a grouped property"));
                return recordError(binding->valueLocation, tr("Invalid property assignment: \"%1\" is a read-only property").arg(name));
            }

            if (!pd->isQList() && (bindingFlags & QV4::CompiledData::Binding::IsListItem)) {
                QString error;
                if (pd->propType() == QMetaType::fromType<QQmlScriptString>())
                    error = tr( "Cannot assign multiple values to a script property");
                else
                    error = tr( "Cannot assign multiple values to a singular property");
                return recordError(binding->valueLocation, error);
            }

            if (!bindingToDefaultProperty
                && !binding->isGroupProperty()
                && !(bindingFlags & QV4::CompiledData::Binding::IsOnAssignment)
                && assigningToGroupProperty) {
                QV4::CompiledData::Location loc = binding->valueLocation;
                if (loc < (*assignedGroupProperty)->valueLocation)
                    loc = (*assignedGroupProperty)->valueLocation;

                if (pd && QQmlMetaType::isValueType(pd->propType()))
                    return recordError(loc, tr("Property has already been assigned a value"));
                return recordError(loc, tr("Cannot assign a value directly to a grouped property"));
            }

            if (bindingType < QV4::CompiledData::Binding::Type_Script) {
                QQmlError bindingError = validateLiteralBinding(propertyCache, pd, binding);
                if (bindingError.isValid())
                    return recordError(bindingError);
            } else if (bindingType == QV4::CompiledData::Binding::Type_Object) {
                QQmlError bindingError = validateObjectBinding(pd, name, binding);
                if (bindingError.isValid())
                    return recordError(bindingError);
            } else if (binding->isGroupProperty()) {
                if (QQmlMetaType::isValueType(pd->propType())) {
                    if (QQmlMetaType::metaObjectForValueType(pd->propType())) {
                        if (!pd->isWritable()) {
                            return recordError(binding->location, tr("Invalid property assignment: \"%1\" is a read-only property").arg(name));
                        }
                    } else {
                        return recordError(binding->location, tr("Invalid grouped property access"));
                    }
                } else {
                    const QMetaType type = pd->propType();
                    if (isPrimitiveType(type)) {
                        return recordError(
                                    binding->location,
                                    tr("Invalid grouped property access: Property \"%1\" with primitive type \"%2\".")
                                        .arg(name, QString::fromUtf8(type.name()))
                                    );
                    }

                    if (!QQmlMetaType::propertyCacheForType(type)) {
                        auto mo = type.metaObject();
                        if (!mo) {
                            return recordError(binding->location,
                                               tr("Invalid grouped property access: Property \"%1\" with type \"%2\", which is neither a value nor an object type")
                                               .arg(name, QString::fromUtf8(type.name()))
                                              );
                        }
                        if (QMetaObjectPrivate::get(mo)->flags & DynamicMetaObject) {
                            return recordError(binding->location,
                                               tr("Unsupported grouped property access: Property \"%1\" with type \"%2\" has a dynamic meta-object.")
                                               .arg(name, QString::fromUtf8(type.name()))
                                               );
                        }
                        // fall through, this is okay
                    }
                }
            }
        } else {
            if (customParser) {
                customBindings << binding;
                continue;
            }
            if (bindingToDefaultProperty) {
                return recordError(binding->location, tr("Cannot assign to non-existent default property"));
            } else {
                return recordError(binding->location, tr("Cannot assign to non-existent property \"%1\"").arg(name));
            }
        }
    }

    if (obj->idNameIndex) {
        if (populatingValueTypeGroupProperty)
            return recordError(obj->locationOfIdProperty, tr("Invalid use of id property with a value type"));

        bool notInRevision = false;
        collectedBindingPropertyData
                << propertyResolver.property(QStringLiteral("id"), &notInRevision, checkRevision);
    }

    if (customParser && !customBindings.isEmpty()) {
        customParser->clearErrors();
        customParser->validator = this;
        customParser->imports = imports;
        customParser->verifyBindings(compilationUnit, customBindings);
        customParser->validator = nullptr;
        customParser->imports = (QQmlImports*)nullptr;
        QVector<QQmlError> parserErrors = customParser->errors();
        if (!parserErrors.isEmpty())
            return parserErrors;
    }

    (*bindingPropertyDataPerObject)[objectIndex] = collectedBindingPropertyData;

    QVector<QQmlError> noError;
    return noError;
}

QQmlError QQmlPropertyValidator::validateLiteralBinding(
        const QQmlPropertyCache::ConstPtr &propertyCache, const QQmlPropertyData *property,
        const QV4::CompiledData::Binding *binding) const
{
    if (property->isQList()) {
        return qQmlCompileError(binding->valueLocation, tr("Cannot assign primitives to lists"));
    }

    QQmlError noError;

    if (property->isEnum()) {
        if (binding->hasFlag(QV4::CompiledData::Binding::IsResolvedEnum))
            return noError;

        // TODO: For historical reasons you can assign any number to an enum property alias
        //       This can be fixed with an opt-out mechanism, for example a pragma.
        if (property->isAlias() && binding->isNumberBinding())
            return noError;

        QString value = compilationUnit->bindingValueAsString(binding);
        QMetaProperty p = propertyCache->firstCppMetaObject()->property(property->coreIndex());
        bool ok;
        if (p.isFlagType()) {
            p.enumerator().keysToValue(value.toUtf8().constData(), &ok);
        } else
            p.enumerator().keyToValue(value.toUtf8().constData(), &ok);

        if (!ok) {
            return qQmlCompileError(binding->valueLocation, tr("Invalid property assignment: unknown enumeration"));
        }
        return noError;
    }

    auto warnOrError = [&](const QString &error) {
        return qQmlCompileError(binding->valueLocation, error);
    };

    const QV4::CompiledData::Binding::Type bindingType = binding->type();
    const auto isStringBinding = [&]() -> bool {
        // validateLiteralBinding is not supposed to be used on scripts
        Q_ASSERT(bindingType != QV4::CompiledData::Binding::Type_Script);
        return bindingType == QV4::CompiledData::Binding::Type_String;
    };

    switch (property->propType().id()) {
    case QMetaType::QVariant:
    break;
    case QMetaType::QString: {
        if (!binding->evaluatesToString()) {
            return warnOrError(tr("Invalid property assignment: string expected"));
        }
    }
    break;
    case QMetaType::QStringList: {
        if (!binding->evaluatesToString()) {
            return warnOrError(tr("Invalid property assignment: string or string list expected"));
        }
    }
    break;
    case QMetaType::QByteArray: {
        if (bindingType != QV4::CompiledData::Binding::Type_String)
            return warnOrError(tr("Invalid property assignment: byte array expected"));
    }
    break;
    case QMetaType::QUrl: {
        if (bindingType != QV4::CompiledData::Binding::Type_String)
            return warnOrError(tr("Invalid property assignment: url expected"));
    }
    break;
    case QMetaType::UInt: {
        if (bindingType == QV4::CompiledData::Binding::Type_Number) {
            double d = compilationUnit->bindingValueAsNumber(binding);
            if (double(uint(d)) == d)
                return noError;
        }
        return warnOrError(tr("Invalid property assignment: unsigned int expected"));
    }
    break;
    case QMetaType::Int: {
        if (bindingType == QV4::CompiledData::Binding::Type_Number) {
            double d = compilationUnit->bindingValueAsNumber(binding);
            if (double(int(d)) == d)
                return noError;
        }
        return warnOrError(tr("Invalid property assignment: int expected"));
    }
    break;
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::Float:
    case QMetaType::Double: {
        if (bindingType != QV4::CompiledData::Binding::Type_Number) {
            return warnOrError(tr("Invalid property assignment: number expected"));
        }
    }
    break;
    case QMetaType::QColor: {
        bool ok = false;
        if (isStringBinding())
            QQmlStringConverters::rgbaFromString(compilationUnit->bindingValueAsString(binding), &ok);
        if (!ok) {
            return warnOrError(tr("Invalid property assignment: color expected"));
        }
    }
    break;
#if QT_CONFIG(datestring)
    case QMetaType::QDate: {
        bool ok = false;
        if (isStringBinding())
            QQmlStringConverters::dateFromString(compilationUnit->bindingValueAsString(binding), &ok);
        if (!ok) {
            return warnOrError(tr("Invalid property assignment: date expected"));
        }
    }
    break;
    case QMetaType::QTime: {
        bool ok = false;
        if (isStringBinding())
            QQmlStringConverters::timeFromString(compilationUnit->bindingValueAsString(binding), &ok);
        if (!ok) {
            return warnOrError(tr("Invalid property assignment: time expected"));
        }
    }
    break;
    case QMetaType::QDateTime: {
        bool ok = false;
        if (isStringBinding())
            QQmlStringConverters::dateTimeFromString(compilationUnit->bindingValueAsString(binding), &ok);
        if (!ok) {
            return warnOrError(tr("Invalid property assignment: datetime expected"));
        }
    }
    break;
#endif // datestring
    case QMetaType::QPoint: {
        bool ok = false;
        if (isStringBinding())
            QQmlStringConverters::pointFFromString(compilationUnit->bindingValueAsString(binding), &ok);
        if (!ok) {
            return warnOrError(tr("Invalid property assignment: point expected"));
        }
    }
    break;
    case QMetaType::QPointF: {
        bool ok = false;
        if (isStringBinding())
            QQmlStringConverters::pointFFromString(compilationUnit->bindingValueAsString(binding), &ok);
        if (!ok) {
            return warnOrError(tr("Invalid property assignment: point expected"));
        }
    }
    break;
    case QMetaType::QSize: {
        bool ok = false;
        if (isStringBinding())
            QQmlStringConverters::sizeFFromString(compilationUnit->bindingValueAsString(binding), &ok);
        if (!ok) {
            return warnOrError(tr("Invalid property assignment: size expected"));
        }
    }
    break;
    case QMetaType::QSizeF: {
        bool ok = false;
        if (isStringBinding())
            QQmlStringConverters::sizeFFromString(compilationUnit->bindingValueAsString(binding), &ok);
        if (!ok) {
            return warnOrError(tr("Invalid property assignment: size expected"));
        }
    }
    break;
    case QMetaType::QRect: {
        bool ok = false;
        if (isStringBinding())
            QQmlStringConverters::rectFFromString(compilationUnit->bindingValueAsString(binding), &ok);
        if (!ok) {
            return warnOrError(tr("Invalid property assignment: rect expected"));
        }
    }
    break;
    case QMetaType::QRectF: {
        bool ok = false;
        if (isStringBinding())
            QQmlStringConverters::rectFFromString(compilationUnit->bindingValueAsString(binding), &ok);
        if (!ok) {
            return warnOrError(tr("Invalid property assignment: point expected"));
        }
    }
    break;
    case QMetaType::Bool: {
        if (bindingType != QV4::CompiledData::Binding::Type_Boolean) {
            return warnOrError(tr("Invalid property assignment: boolean expected"));
        }
    }
    break;
    case QMetaType::QVector2D:
    case QMetaType::QVector3D:
    case QMetaType::QVector4D:
    case QMetaType::QQuaternion: {
        auto typeName = [&]() {
            switch (property->propType().id()) {
            case QMetaType::QVector2D: return QStringLiteral("2D vector");
            case QMetaType::QVector3D: return QStringLiteral("3D vector");
            case QMetaType::QVector4D: return QStringLiteral("4D vector");
            case QMetaType::QQuaternion: return QStringLiteral("quaternion");
            default: return QString();
            }
        };
        const QVariant result = QQmlValueTypeProvider::createValueType(
                    compilationUnit->bindingValueAsString(binding),
                    property->propType());
        if (!result.isValid()) {
            return warnOrError(tr("Invalid property assignment: %1 expected")
                               .arg(typeName()));
        }
    }
    break;
    case QMetaType::QRegularExpression:
        return warnOrError(tr("Invalid property assignment: regular expression expected; use /pattern/ syntax"));
    default: {
        // generate single literal value assignment to a list property if required
        if (property->propType() == QMetaType::fromType<QList<qreal> >()) {
            if (bindingType != QV4::CompiledData::Binding::Type_Number) {
                return warnOrError(tr("Invalid property assignment: number or array of numbers expected"));
            }
            break;
        } else if (property->propType() == QMetaType::fromType<QList<int> >()) {
            bool ok = (bindingType == QV4::CompiledData::Binding::Type_Number);
            if (ok) {
                double n = compilationUnit->bindingValueAsNumber(binding);
                if (double(int(n)) != n)
                    ok = false;
            }
            if (!ok)
                return warnOrError(tr("Invalid property assignment: int or array of ints expected"));
            break;
        } else if (property->propType() == QMetaType::fromType<QList<bool> >()) {
            if (bindingType != QV4::CompiledData::Binding::Type_Boolean) {
                return warnOrError(tr("Invalid property assignment: bool or array of bools expected"));
            }
            break;
        } else if (property->propType() == QMetaType::fromType<QList<QUrl> >()) {
            if (bindingType != QV4::CompiledData::Binding::Type_String) {
                return warnOrError(tr("Invalid property assignment: url or array of urls expected"));
            }
            break;
        } else if (property->propType() == QMetaType::fromType<QList<QString> >()) {
            if (!binding->evaluatesToString()) {
                return warnOrError(tr("Invalid property assignment: string or array of strings expected"));
            }
            break;
        } else if (property->propType() == QMetaType::fromType<QJSValue>()) {
            break;
        } else if (property->propType() == QMetaType::fromType<QQmlScriptString>()) {
            break;
        } else if (property->isQObject()
                   && bindingType == QV4::CompiledData::Binding::Type_Null) {
            break;
        } else if (QQmlMetaType::qmlType(property->propType()).canConstructValueType()) {
            break;
        }

        return warnOrError(tr("Invalid property assignment: unsupported type \"%1\"").arg(QLatin1StringView(property->propType().name())));
    }
    break;
    }
    return noError;
}

/*!
    Returns true if from can be assigned to a (QObject) property of type
    to.
*/
bool QQmlPropertyValidator::canCoerce(QMetaType to, QQmlPropertyCache::ConstPtr fromMo) const
{
    QQmlPropertyCache::ConstPtr toMo = QQmlMetaType::rawPropertyCacheForType(to);

    if (toMo.isNull()) {
        // if we have an inline component from the current file,
        // it is not properly registered at this point, as registration
        // only occurs after the whole file has been validated
        // Therefore we need to check the ICs here
        for (const auto& icDatum : compilationUnit->inlineComponentData) {
            if (icDatum.qmlType.typeId() == to) {
                toMo = compilationUnit->propertyCaches.at(icDatum.objectIndex);
                break;
            }
        }
    }

    while (fromMo) {
        if (fromMo == toMo)
            return true;
        fromMo = fromMo->parent();
    }
    return false;
}

QVector<QQmlError> QQmlPropertyValidator::recordError(const QV4::CompiledData::Location &location, const QString &description) const
{
    QVector<QQmlError> errors;
    errors.append(qQmlCompileError(location, description));
    return errors;
}

QVector<QQmlError> QQmlPropertyValidator::recordError(const QQmlError &error) const
{
    QVector<QQmlError> errors;
    errors.append(error);
    return errors;
}

QQmlError QQmlPropertyValidator::validateObjectBinding(const QQmlPropertyData *property, const QString &propertyName, const QV4::CompiledData::Binding *binding) const
{
    QQmlError noError;

    if (binding->hasFlag(QV4::CompiledData::Binding::IsOnAssignment)) {
        Q_ASSERT(binding->type() == QV4::CompiledData::Binding::Type_Object);

        bool isValueSource = false;
        bool isPropertyInterceptor = false;

        const QV4::CompiledData::Object *targetObject = compilationUnit->objectAt(binding->value.objectIndex);
        if (auto *typeRef = resolvedType(targetObject->inheritedTypeNameIndex)) {
            QQmlPropertyCache::ConstPtr cache = typeRef->createPropertyCache();
            const QMetaObject *mo = cache ? cache->firstCppMetaObject() : nullptr;
            QQmlType qmlType;
            while (mo && !qmlType.isValid()) {
                qmlType = QQmlMetaType::qmlType(mo);
                mo = mo->superClass();
            }

            isValueSource = qmlType.propertyValueSourceCast() != -1;
            isPropertyInterceptor = qmlType.propertyValueInterceptorCast() != -1;
        }

        if (!isValueSource && !isPropertyInterceptor) {
            return qQmlCompileError(binding->valueLocation, tr("\"%1\" cannot operate on \"%2\"").arg(stringAt(targetObject->inheritedTypeNameIndex)).arg(propertyName));
        }

        return noError;
    }

    const QMetaType propType = property->propType();
    const auto rhsType = [&]() {
        return stringAt(compilationUnit->objectAt(binding->value.objectIndex)
                        ->inheritedTypeNameIndex);
    };

    if (QQmlMetaType::isInterface(propType)) {
        // Can only check at instantiation time if the created sub-object successfully casts to the
        // target interface.
        return noError;
    } else if (propType == QMetaType::fromType<QVariant>()
               || propType == QMetaType::fromType<QJSValue>()) {
        // We can convert everything to QVariant :)
        return noError;
    } else if (property->isQList()) {
        const QMetaType listType = QQmlMetaType::listValueType(property->propType());
        if (!QQmlMetaType::isInterface(listType)) {
            QQmlPropertyCache::ConstPtr source = propertyCaches.at(binding->value.objectIndex);
            if (!canCoerce(listType, source)) {
                const QString expectedTypeName = QString::fromUtf8(listType.name()).remove(QLatin1Char('*'));
                return qQmlCompileError(binding->valueLocation,
                    tr("Cannot assign object of type \"%1\" to list property \"%2\"; expected \"%3\"")
                        .arg(source->className(), propertyName, expectedTypeName));
            }
        }
        return noError;
    } else if (binding->hasFlag(QV4::CompiledData::Binding::IsSignalHandlerObject)
               && property->isFunction()) {
        return noError;
    } else if (isPrimitiveType(propType)) {
        auto typeName = QString::fromUtf8(QMetaType(propType).name());
        return qQmlCompileError(binding->location, tr("Cannot assign value of type \"%1\" to property \"%2\", expecting \"%3\"")
                                                      .arg(rhsType())
                                                      .arg(propertyName)
                                                      .arg(typeName));
    } else if (propType == QMetaType::fromType<QQmlScriptString>()) {
        return qQmlCompileError(binding->valueLocation, tr("Invalid property assignment: script expected"));
    } else if (!QQmlMetaType::isValueType(property->propType())) {
        QQmlPropertyCache::ConstPtr propertyMetaObject;

        // if we have an inline component from the current file,
        // it is not properly registered at this point, as registration
        // only occurs after the whole file has been validated
        // Therefore we need to check the ICs here
        for (const auto &icDatum: std::as_const(compilationUnit->inlineComponentData)) {
            if (icDatum.qmlType.typeId() == property->propType()) {
                propertyMetaObject = compilationUnit->propertyCaches.at(icDatum.objectIndex);
                break;
            }
        }

        if (!propertyMetaObject) {
            // We want to use the raw metaObject here as the raw metaobject is the
            // actual property type before we applied any extensions that might
            // effect the properties on the type, but don't effect assignability
            // Not passing a version ensures that we get the raw metaObject.
            propertyMetaObject = QQmlMetaType::rawPropertyCacheForType(propType);
        }

        if (propertyMetaObject) {
            // Will be true if the assigned type inherits propertyMetaObject
            // Determine isAssignable value
            bool isAssignable = false;
            QQmlPropertyCache::ConstPtr c = propertyCaches.at(binding->value.objectIndex);
            while (c && !isAssignable) {
                isAssignable |= c == propertyMetaObject;
                c = c->parent();
            }

            if (!isAssignable) {
                return qQmlCompileError(binding->valueLocation, tr("Cannot assign object of type \"%1\" to property of type \"%2\" as the former is neither the same as the latter nor a sub-class of it.")
                        .arg(rhsType()).arg(QLatin1String(property->propType().name())));
            }
        } else {
            return qQmlCompileError(binding->valueLocation, tr("Cannot assign to property of unknown type \"%1\".")
                        .arg(QLatin1String(property->propType().name())));
        }

    }
    return noError;
}

QT_END_NAMESPACE
