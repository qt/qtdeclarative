/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlvaluetypeproxybinding_p.h"

QT_BEGIN_NAMESPACE

// Used in qqmlabstractbinding.cpp
QQmlAbstractBinding::VTable QQmlValueTypeProxyBinding_vtable = {
    QQmlAbstractBinding::default_destroy<QQmlValueTypeProxyBinding>,
    QQmlAbstractBinding::default_expression,
    QQmlValueTypeProxyBinding::propertyIndex,
    QQmlValueTypeProxyBinding::object,
    QQmlValueTypeProxyBinding::setEnabled,
    QQmlValueTypeProxyBinding::update,
    QQmlAbstractBinding::default_retargetBinding
};

QQmlValueTypeProxyBinding::QQmlValueTypeProxyBinding(QObject *o, int index)
: QQmlAbstractBinding(ValueTypeProxy), m_object(o), m_index(index), m_bindings(0)
{
}

QQmlValueTypeProxyBinding::~QQmlValueTypeProxyBinding()
{
    QQmlAbstractBinding *binding = m_bindings;
    // This must be identical to the logic in QQmlData::destroyed()
    while (binding) {
        QQmlAbstractBinding *next = binding->nextBinding();
        binding->setAddedToObject(false);
        binding->setNextBinding(0);
        binding->destroy();
        binding = next;
    }
}

void QQmlValueTypeProxyBinding::setEnabled(QQmlAbstractBinding *_This,
                                           bool e, QQmlPropertyPrivate::WriteFlags flags)
{
    QQmlValueTypeProxyBinding *This = static_cast<QQmlValueTypeProxyBinding *>(_This);

    if (e) {
        QQmlAbstractBinding *bindings = This->m_bindings;
        This->recursiveEnable(bindings, flags);
    } else {
        QQmlAbstractBinding *bindings = This->m_bindings;
        This->recursiveDisable(bindings);
    }
}

void QQmlValueTypeProxyBinding::recursiveEnable(QQmlAbstractBinding *b, QQmlPropertyPrivate::WriteFlags flags)
{
    if (!b)
        return;

    recursiveEnable(b->nextBinding(), flags);

    if (b)
        b->setEnabled(true, flags);
}

void QQmlValueTypeProxyBinding::recursiveDisable(QQmlAbstractBinding *b)
{
    if (!b)
        return;

    recursiveDisable(b->nextBinding());

    if (b)
        b->setEnabled(false, 0);
}

void QQmlValueTypeProxyBinding::update(QQmlAbstractBinding *, QQmlPropertyPrivate::WriteFlags)
{
}

QQmlAbstractBinding *QQmlValueTypeProxyBinding::binding(int propertyIndex)
{
    QQmlAbstractBinding *binding = m_bindings;

    while (binding && binding->propertyIndex() != propertyIndex)
        binding = binding->nextBinding();

    return binding;
}

/*!
Removes a collection of bindings, corresponding to the set bits in \a mask.
*/
void QQmlValueTypeProxyBinding::removeBindings(quint32 mask)
{
    QQmlAbstractBinding *binding = m_bindings;
    QQmlAbstractBinding *lastBinding = 0;

    while (binding) {
        if (mask & (1 << (binding->propertyIndex() >> 24))) {
            QQmlAbstractBinding *remove = binding;
            binding = remove->nextBinding();

            if (lastBinding == 0)
                m_bindings = remove->nextBinding();
            else
                lastBinding->setNextBinding(remove->nextBinding());

            remove->setAddedToObject(false);
            remove->setNextBinding(0);
            remove->destroy();
        } else {
            lastBinding = binding;
            binding = binding->nextBinding();
        }
    }
}

int QQmlValueTypeProxyBinding::propertyIndex(const QQmlAbstractBinding *This)
{
    return static_cast<const QQmlValueTypeProxyBinding *>(This)->m_index;
}

QObject *QQmlValueTypeProxyBinding::object(const QQmlAbstractBinding *This)
{
    return static_cast<const QQmlValueTypeProxyBinding *>(This)->m_object;
}

int QQmlValueTypeProxyBinding::propertyIndex() const
{
    return m_index;
}

QObject *QQmlValueTypeProxyBinding::object() const
{
    return m_object;
}

QT_END_NAMESPACE
