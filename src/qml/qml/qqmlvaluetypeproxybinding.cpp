// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlvaluetypeproxybinding_p.h"

QT_BEGIN_NAMESPACE

QQmlValueTypeProxyBinding::QQmlValueTypeProxyBinding(QObject *o, QQmlPropertyIndex index)
    : QQmlAbstractBinding(),
      m_bindings(nullptr)
{
    m_target = o;
    m_targetIndex = index;
}

QQmlValueTypeProxyBinding::~QQmlValueTypeProxyBinding()
{
    QQmlAbstractBinding *binding = m_bindings.data();
    while (binding) {
        binding->setAddedToObject(false);
        binding = binding->nextBinding();
    }
}

void QQmlValueTypeProxyBinding::setEnabled(bool e, QQmlPropertyData::WriteFlags flags)
{
    QQmlAbstractBinding *b = m_bindings.data();
    while (b) {
        b->setEnabled(e, flags);
        b = b->nextBinding();
    }
}

QQmlAbstractBinding *QQmlValueTypeProxyBinding::subBindings() const
{
    return m_bindings.data();
}

QQmlAbstractBinding *QQmlValueTypeProxyBinding::binding(QQmlPropertyIndex propertyIndex) const
{
    QQmlAbstractBinding *binding = m_bindings.data();

    while (binding && binding->targetPropertyIndex() != propertyIndex)
        binding = binding->nextBinding();

    return binding;
}

/*!
Removes a collection of bindings, corresponding to the set bits in \a mask.
*/
void QQmlValueTypeProxyBinding::removeBindings(quint32 mask)
{
    QQmlAbstractBinding *binding = m_bindings.data();
    QQmlAbstractBinding *lastBinding = nullptr;

    while (binding) {
        const int valueTypeIndex = binding->targetPropertyIndex().valueTypeIndex();
        if (valueTypeIndex != -1 && (mask & (1 << valueTypeIndex))) {
            QQmlAbstractBinding *remove = binding;
            remove->setAddedToObject(false);
            binding = remove->nextBinding();

            if (lastBinding == nullptr)
                m_bindings = remove->nextBinding();
            else
                lastBinding->setNextBinding(remove->nextBinding());
        } else {
            lastBinding = binding;
            binding = binding->nextBinding();
        }
    }
}

QT_END_NAMESPACE
