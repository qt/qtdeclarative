/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qqmlpropertycachecreator_p.h"

#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

QAtomicInt QQmlPropertyCacheCreatorBase::classIndexCounter(0);


QMetaType QQmlPropertyCacheCreatorBase::metaTypeForPropertyType(QV4::CompiledData::BuiltinType type)
{
    // TODO: we cannot depend on GUI types in declarative, so we'll have to do the id lookup
    switch (type) {
    case QV4::CompiledData::BuiltinType::Var: return QMetaType::fromType<QVariant>();
    case QV4::CompiledData::BuiltinType::Int: return QMetaType::fromType<int>();
    case QV4::CompiledData::BuiltinType::Bool: return QMetaType::fromType<bool>();
    case QV4::CompiledData::BuiltinType::Real: return QMetaType::fromType<qreal>();
    case QV4::CompiledData::BuiltinType::String: return QMetaType::fromType<QString>();
    case QV4::CompiledData::BuiltinType::Url: return QMetaType::fromType<QUrl>();
    case QV4::CompiledData::BuiltinType::Color: return QMetaType(QMetaType::QColor);
    case QV4::CompiledData::BuiltinType::Font: return QMetaType(QMetaType::QFont);
    case QV4::CompiledData::BuiltinType::Time: return QMetaType::fromType<QTime>();
    case QV4::CompiledData::BuiltinType::Date: return QMetaType::fromType<QDate>();
    case QV4::CompiledData::BuiltinType::DateTime: return QMetaType::fromType<QDateTime>();
    case QV4::CompiledData::BuiltinType::Rect: return QMetaType::fromType<QRectF>();
    case QV4::CompiledData::BuiltinType::Point: return QMetaType::fromType<QPointF>();
    case QV4::CompiledData::BuiltinType::Size: return QMetaType::fromType<QSizeF>();
    case QV4::CompiledData::BuiltinType::Vector2D: return QMetaType(QMetaType::QVector2D);
    case QV4::CompiledData::BuiltinType::Vector3D: return QMetaType(QMetaType::QVector3D);
    case QV4::CompiledData::BuiltinType::Vector4D: return QMetaType(QMetaType::QVector4D);
    case QV4::CompiledData::BuiltinType::Matrix4x4: return QMetaType(QMetaType::QMatrix4x4);
    case QV4::CompiledData::BuiltinType::Quaternion: return QMetaType(QMetaType::QQuaternion);
    case QV4::CompiledData::BuiltinType::InvalidBuiltin: break;
    };
    return QMetaType {};
}

QByteArray QQmlPropertyCacheCreatorBase::createClassNameTypeByUrl(const QUrl &url)
{
    const QString path = url.path();
    int lastSlash = path.lastIndexOf(QLatin1Char('/'));
    // Not a reusable type if we don't have an absolute Url
    if (lastSlash <= -1)
        return QByteArray();
    // ### this might not be correct for .ui.qml files
    const QStringView nameBase = QStringView{path}.mid(lastSlash + 1, path.length() - lastSlash - 5);
    // Not a reusable type if it doesn't start with a upper case letter.
    if (nameBase.isEmpty() || !nameBase.at(0).isUpper())
        return QByteArray();
    return nameBase.toUtf8() + "_QMLTYPE_" +
            QByteArray::number(classIndexCounter.fetchAndAddRelaxed(1));
}

QByteArray QQmlPropertyCacheCreatorBase::createClassNameForInlineComponent(const QUrl &baseUrl, int icId)
{
    QByteArray baseName = createClassNameTypeByUrl(baseUrl);
    if (baseName.isEmpty())
        baseName = QByteArray("ANON_QML_IC_") + QByteArray::number(classIndexCounter.fetchAndAddRelaxed(1));
    baseName += "_" + QByteArray::number(icId);
    return baseName;
}

QQmlBindingInstantiationContext::QQmlBindingInstantiationContext(int referencingObjectIndex, const QV4::CompiledData::Binding *instantiatingBinding,
                                                                 const QString &instantiatingPropertyName, QQmlPropertyCache *referencingObjectPropertyCache)
    : referencingObjectIndex(referencingObjectIndex)
    , instantiatingBinding(instantiatingBinding)
    , instantiatingPropertyName(instantiatingPropertyName)
    , referencingObjectPropertyCache(referencingObjectPropertyCache)
{
}

bool QQmlBindingInstantiationContext::resolveInstantiatingProperty()
{
    if (!instantiatingBinding
           || instantiatingBinding->type() != QV4::CompiledData::Binding::Type_GroupProperty) {
        return true;
    }

    Q_ASSERT(referencingObjectIndex >= 0);
    Q_ASSERT(referencingObjectPropertyCache);
    Q_ASSERT(instantiatingBinding->propertyNameIndex != 0);

    bool notInRevision = false;
    instantiatingProperty = QQmlPropertyResolver(referencingObjectPropertyCache)
                                    .property(instantiatingPropertyName, &notInRevision,
                                              QQmlPropertyResolver::IgnoreRevision);
    return instantiatingProperty != nullptr;
}

QQmlRefPointer<QQmlPropertyCache> QQmlBindingInstantiationContext::instantiatingPropertyCache(QQmlEnginePrivate *enginePrivate) const
{
    if (instantiatingProperty) {
        if (instantiatingProperty->isQObject()) {
            // rawPropertyCacheForType assumes a given unspecified version means "any version".
            // There is another overload that takes no version, which we shall not use here.
            return enginePrivate->rawPropertyCacheForType(instantiatingProperty->propType().id(),
                                                          instantiatingProperty->typeVersion());
        } else if (const QMetaObject *vtmo = QQmlMetaType::metaObjectForValueType(instantiatingProperty->propType())) {
            return enginePrivate->cache(vtmo, instantiatingProperty->typeVersion());
        }
    }
    return QQmlRefPointer<QQmlPropertyCache>();
}

void QQmlPendingGroupPropertyBindings::resolveMissingPropertyCaches(QQmlEnginePrivate *enginePrivate, QQmlPropertyCacheVector *propertyCaches) const
{
    for (QQmlBindingInstantiationContext pendingBinding: *this) {
        const int groupPropertyObjectIndex = pendingBinding.instantiatingBinding->value.objectIndex;

        if (propertyCaches->at(groupPropertyObjectIndex))
            continue;

        if (!pendingBinding.resolveInstantiatingProperty())
            continue;

        auto cache = pendingBinding.instantiatingPropertyCache(enginePrivate);
        propertyCaches->set(groupPropertyObjectIndex, cache);
    }
}

QT_END_NAMESPACE
