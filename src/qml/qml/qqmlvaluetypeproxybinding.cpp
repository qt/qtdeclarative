/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlvaluetypeproxybinding_p.h"

QT_BEGIN_NAMESPACE

QQmlValueTypeProxyBinding::QQmlValueTypeProxyBinding(QObject *o, int index)
    : QQmlAbstractBinding(),
      m_bindings(0)
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

void QQmlValueTypeProxyBinding::setEnabled(bool e, QQmlPropertyPrivate::WriteFlags flags)
{
    QQmlAbstractBinding *b = m_bindings.data();
    while (b) {
        b->setEnabled(e, flags);
        b = b->nextBinding();
    }
}

bool QQmlValueTypeProxyBinding::isValueTypeProxy() const
{
    return true;
}

QQmlAbstractBinding *QQmlValueTypeProxyBinding::binding(int propertyIndex)
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
    QQmlAbstractBinding *lastBinding = 0;

    while (binding) {
        int valueTypeIndex = QQmlPropertyData::decodeValueTypePropertyIndex(binding->targetPropertyIndex());
        if (valueTypeIndex != -1 && (mask & (1 << valueTypeIndex))) {
            QQmlAbstractBinding *remove = binding;
            remove->setAddedToObject(false);
            binding = remove->nextBinding();

            if (lastBinding == 0)
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
