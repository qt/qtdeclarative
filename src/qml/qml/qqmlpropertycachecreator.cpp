/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlpropertycachecreator_p.h"

#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

QAtomicInt QQmlPropertyCacheCreatorBase::classIndexCounter(0);


int QQmlPropertyCacheCreatorBase::metaTypeForPropertyType(QV4::CompiledData::BuiltinType type)
{
    switch (type) {
    case QV4::CompiledData::BuiltinType::Var: return QMetaType::QVariant;
    case QV4::CompiledData::BuiltinType::Variant: return QMetaType::QVariant;
    case QV4::CompiledData::BuiltinType::Int: return QMetaType::Int;
    case QV4::CompiledData::BuiltinType::Bool: return QMetaType::Bool;
    case QV4::CompiledData::BuiltinType::Real: return QMetaType::Double;
    case QV4::CompiledData::BuiltinType::String: return QMetaType::QString;
    case QV4::CompiledData::BuiltinType::Url: return QMetaType::QUrl;
    case QV4::CompiledData::BuiltinType::Color: return QMetaType::QColor;
    case QV4::CompiledData::BuiltinType::Font: return QMetaType::QFont;
    case QV4::CompiledData::BuiltinType::Time: return QMetaType::QTime;
    case QV4::CompiledData::BuiltinType::Date: return QMetaType::QDate;
    case QV4::CompiledData::BuiltinType::DateTime: return QMetaType::QDateTime;
    case QV4::CompiledData::BuiltinType::Rect: return QMetaType::QRectF;
    case QV4::CompiledData::BuiltinType::Point: return QMetaType::QPointF;
    case QV4::CompiledData::BuiltinType::Size: return QMetaType::QSizeF;
    case QV4::CompiledData::BuiltinType::Vector2D: return QMetaType::QVector2D;
    case QV4::CompiledData::BuiltinType::Vector3D: return QMetaType::QVector3D;
    case QV4::CompiledData::BuiltinType::Vector4D: return QMetaType::QVector4D;
    case QV4::CompiledData::BuiltinType::Matrix4x4: return QMetaType::QMatrix4x4;
    case QV4::CompiledData::BuiltinType::Quaternion: return QMetaType::QQuaternion;
    case QV4::CompiledData::BuiltinType::InvalidBuiltin: break;
    };
    return QMetaType::UnknownType;
}

QByteArray QQmlPropertyCacheCreatorBase::createClassNameTypeByUrl(const QUrl &url)
{
    const QString path = url.path();
    int lastSlash = path.lastIndexOf(QLatin1Char('/'));
    // Not a reusable type if we don't have an absolute Url
    if (lastSlash <= -1)
        return QByteArray();
    // ### this might not be correct for .ui.qml files
    const QStringRef nameBase = path.midRef(lastSlash + 1, path.length() - lastSlash - 5);
    // Not a reusable type if it doesn't start with a upper case letter.
    if (nameBase.isEmpty() || !nameBase.at(0).isUpper())
        return QByteArray();
    return nameBase.toUtf8() + "_QMLTYPE_" +
            QByteArray::number(classIndexCounter.fetchAndAddRelaxed(1));
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
    if (!instantiatingBinding || instantiatingBinding->type != QV4::CompiledData::Binding::Type_GroupProperty)
        return true;

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
            return enginePrivate->rawPropertyCacheForType(instantiatingProperty->propType(), instantiatingProperty->typeMinorVersion());
        } else if (const QMetaObject *vtmo = QQmlValueTypeFactory::metaObjectForMetaType(instantiatingProperty->propType())) {
            return enginePrivate->cache(vtmo, instantiatingProperty->typeMinorVersion());
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
