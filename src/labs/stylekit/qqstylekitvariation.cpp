// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitvariation_p.h"

QT_BEGIN_NAMESPACE

int QQStyleKitVariation::s_typeVariationCount = 0;
int QQStyleKitVariationAttached::s_instanceVariationCount = 0;

QQStyleKitVariation::QQStyleKitVariation(QObject *parent)
    : QQStyleKitControls(parent)
{
    /* As an optimization, keep track of how many type variations that are defined
     * inside the style. That way we can skip looking for them later if s_typeVariationCount == 0. */
    ++s_typeVariationCount;
}

QQStyleKitVariationAttached *QQStyleKitVariation::qmlAttachedProperties(QObject *object)
{
    return new QQStyleKitVariationAttached(object);
}

QString QQStyleKitVariation::name() const
{
    return m_name;
}

void QQStyleKitVariation::setName(const QString &name)
{
    if (m_name == name)
        return;

    m_name = name;
    emit nameChanged();
}

QQStyleKitVariationAttached::QQStyleKitVariationAttached(QObject *parent)
    : QObject(parent)
{
}

QStringList QQStyleKitVariationAttached::variations() const
{
    return m_variations;
}

void QQStyleKitVariationAttached::setVariations(const QStringList &variations)
{
    if (m_variations == variations)
        return;

    /* As an optimization, we count the number of instance variations set from the application.
     * That way, if s_instanceVariationCount == 1, for example, and we found a variation while
     * resolving the effective variations for a specific QQStyleReader, we can stop the search. */
    s_instanceVariationCount++;

    m_variations = variations;
    emit variationsChanged();
}

QQStyleKitExtendableControlType QQStyleKitVariationAttached::controlType()
{
    return m_controlType;
}

void QQStyleKitVariationAttached::setControlType(QQStyleKitExtendableControlType type)
{
    if (m_controlType == type)
        return;

    m_controlType = type;
    emit controlTypeChanged();
}

QT_END_NAMESPACE

#include "moc_qqstylekitvariation_p.cpp"
