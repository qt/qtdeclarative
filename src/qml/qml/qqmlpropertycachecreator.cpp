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


QMetaType QQmlPropertyCacheCreatorBase::metaTypeForPropertyType(QV4::CompiledData::BuiltinType type)
{
    switch (type) {
    case QV4::CompiledData::BuiltinType::Var: return QMetaType::fromType<QVariant>();
    case QV4::CompiledData::BuiltinType::Int: return QMetaType::fromType<int>();
    case QV4::CompiledData::BuiltinType::Bool: return QMetaType::fromType<bool>();
    case QV4::CompiledData::BuiltinType::Real: return QMetaType::fromType<qreal>();
    case QV4::CompiledData::BuiltinType::String: return QMetaType::fromType<QString>();
    case QV4::CompiledData::BuiltinType::Url: return QMetaType::fromType<QUrl>();
    case QV4::CompiledData::BuiltinType::Time: return QMetaType::fromType<QTime>();
    case QV4::CompiledData::BuiltinType::Date: return QMetaType::fromType<QDate>();
    case QV4::CompiledData::BuiltinType::DateTime: return QMetaType::fromType<QDateTime>();
    case QV4::CompiledData::BuiltinType::Rect: return QMetaType::fromType<QRectF>();
    case QV4::CompiledData::BuiltinType::Point: return QMetaType::fromType<QPointF>();
    case QV4::CompiledData::BuiltinType::Size: return QMetaType::fromType<QSizeF>();
    case QV4::CompiledData::BuiltinType::InvalidBuiltin: break;
    };
    return QMetaType {};
}

QMetaType QQmlPropertyCacheCreatorBase::listTypeForPropertyType(QV4::CompiledData::BuiltinType type)
{
    switch (type) {
    case QV4::CompiledData::BuiltinType::Var:      return QMetaType::fromType<QList<QVariant>>();
    case QV4::CompiledData::BuiltinType::Int:      return QMetaType::fromType<QList<int>>();
    case QV4::CompiledData::BuiltinType::Bool:     return QMetaType::fromType<QList<bool>>();
    case QV4::CompiledData::BuiltinType::Real:     return QMetaType::fromType<QList<qreal>>();
    case QV4::CompiledData::BuiltinType::String:   return QMetaType::fromType<QList<QString>>();
    case QV4::CompiledData::BuiltinType::Url:      return QMetaType::fromType<QList<QUrl>>();
    case QV4::CompiledData::BuiltinType::Time:     return QMetaType::fromType<QList<QTime>>();
    case QV4::CompiledData::BuiltinType::Date:     return QMetaType::fromType<QList<QDate>>();
    case QV4::CompiledData::BuiltinType::DateTime: return QMetaType::fromType<QList<QDateTime>>();
    case QV4::CompiledData::BuiltinType::Rect:     return QMetaType::fromType<QList<QRectF>>();
    case QV4::CompiledData::BuiltinType::Point:    return QMetaType::fromType<QList<QPointF>>();
    case QV4::CompiledData::BuiltinType::Size:     return QMetaType::fromType<QList<QSizeF>>();
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

QQmlBindingInstantiationContext::QQmlBindingInstantiationContext(
        int referencingObjectIndex,
        const QV4::CompiledData::Binding *instantiatingBinding,
        const QString &instantiatingPropertyName,
        const QQmlRefPointer<QQmlPropertyCache> &referencingObjectPropertyCache)
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

QQmlRefPointer<QQmlPropertyCache>
QQmlBindingInstantiationContext::instantiatingPropertyCache() const
{
    if (instantiatingProperty) {
        if (instantiatingProperty->isQObject()) {
            // rawPropertyCacheForType assumes a given unspecified version means "any version".
            // There is another overload that takes no version, which we shall not use here.
            return QQmlMetaType::rawPropertyCacheForType(instantiatingProperty->propType(),
                                                         instantiatingProperty->typeVersion());
        } else if (const QMetaObject *vtmo = QQmlMetaType::metaObjectForValueType(instantiatingProperty->propType())) {
            return QQmlMetaType::propertyCache(vtmo, instantiatingProperty->typeVersion());
        }
    }
    return QQmlRefPointer<QQmlPropertyCache>();
}

void QQmlPendingGroupPropertyBindings::resolveMissingPropertyCaches(
        QQmlPropertyCacheVector *propertyCaches) const
{
    for (QQmlBindingInstantiationContext pendingBinding: *this) {
        const int groupPropertyObjectIndex = pendingBinding.instantiatingBinding->value.objectIndex;

        if (propertyCaches->at(groupPropertyObjectIndex))
            continue;

        if (pendingBinding.referencingObjectPropertyCache) {
            if (!pendingBinding.resolveInstantiatingProperty())
                continue;
            auto cache = pendingBinding.instantiatingPropertyCache();
            propertyCaches->set(groupPropertyObjectIndex, cache);
        } else {
            auto cache = propertyCaches->at(pendingBinding.referencingObjectIndex);
            propertyCaches->set(groupPropertyObjectIndex, cache);
        }
    }
}

QT_END_NAMESPACE
