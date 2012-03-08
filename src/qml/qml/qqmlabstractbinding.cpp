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

#include "qqmlabstractbinding_p.h"

#include <private/qqmlbinding_p.h>
#include <private/qqmlvaluetypeproxybinding_p.h>

QT_BEGIN_NAMESPACE

QQmlAbstractBinding::QQmlAbstractBinding()
: m_prevBinding(0), m_nextBinding(0)
{
}

QQmlAbstractBinding::~QQmlAbstractBinding()
{
    Q_ASSERT(m_prevBinding == 0);
    Q_ASSERT(*m_mePtr == 0);
}

/*!
Destroy the binding.  Use this instead of calling delete.

Bindings are free to implement their own memory management, so the delete operator is not
necessarily safe.  The default implementation clears the binding, removes it from the object
and calls delete.
*/
void QQmlAbstractBinding::destroy()
{
    removeFromObject();
    clear();

    delete this;
}

/*!
Add this binding to \a object.

This transfers ownership of the binding to the object, marks the object's property as
being bound.

However, it does not enable the binding itself or call update() on it.
*/
void QQmlAbstractBinding::addToObject()
{
    Q_ASSERT(!m_prevBinding);

    QObject *obj = object();
    Q_ASSERT(obj);

    int index = propertyIndex();

    QQmlData *data = QQmlData::get(obj, true);

    if (index & 0xFF000000) {
        // Value type

        int coreIndex = index & 0xFFFFFF;

        // Find the value type proxy (if there is one)
        QQmlValueTypeProxyBinding *proxy = 0;
        if (data->hasBindingBit(coreIndex)) {
            QQmlAbstractBinding *b = data->bindings;
            while (b && b->propertyIndex() != coreIndex)
                b = b->m_nextBinding;
            Q_ASSERT(b && b->bindingType() == QQmlAbstractBinding::ValueTypeProxy);
            proxy = static_cast<QQmlValueTypeProxyBinding *>(b);
        }

        if (!proxy) {
            proxy = new QQmlValueTypeProxyBinding(obj, coreIndex);

            Q_ASSERT(proxy->propertyIndex() == coreIndex);
            Q_ASSERT(proxy->object() == obj);

            proxy->addToObject();
        }

        m_nextBinding = proxy->m_bindings;
        if (m_nextBinding) m_nextBinding->m_prevBinding = &m_nextBinding;
        m_prevBinding = &proxy->m_bindings;
        proxy->m_bindings = this;

    } else {
        m_nextBinding = data->bindings;
        if (m_nextBinding) m_nextBinding->m_prevBinding = &m_nextBinding;
        m_prevBinding = &data->bindings;
        data->bindings = this;

        data->setBindingBit(obj, index);
    }
}

/*!
Remove the binding from the object.
*/
void QQmlAbstractBinding::removeFromObject()
{
    if (m_prevBinding) {
        int index = propertyIndex();

        *m_prevBinding = m_nextBinding;
        if (m_nextBinding) m_nextBinding->m_prevBinding = m_prevBinding;
        m_prevBinding = 0;
        m_nextBinding = 0;

        if (index & 0xFF000000) {
            // Value type - we don't remove the proxy from the object.  It will sit their happily
            // doing nothing until it is removed by a write, a binding change or it is reused
            // to hold more sub-bindings.
        } else if (QObject *obj = object()) {
            QQmlData *data = QQmlData::get(obj, false);
            if (data) data->clearBindingBit(index);
        }
    }
}

static void bindingDummyDeleter(QQmlAbstractBinding *)
{
}

QQmlAbstractBinding::Pointer QQmlAbstractBinding::weakPointer()
{
    if (m_mePtr.value().isNull())
        m_mePtr.value() = QSharedPointer<QQmlAbstractBinding>(this, bindingDummyDeleter);

    return m_mePtr.value().toWeakRef();
}

void QQmlAbstractBinding::clear()
{
    if (!m_mePtr.isNull()) {
        **m_mePtr = 0;
        m_mePtr = 0;
    }
}

void QQmlAbstractBinding::retargetBinding(QObject *, int)
{
    qFatal("QQmlAbstractBinding::retargetBinding() called on illegal binding.");
}

QString QQmlAbstractBinding::expression() const
{
    return QLatin1String("<Unknown>");
}

void QQmlAbstractBinding::setEnabled(bool enabled, QQmlPropertyPrivate::WriteFlags flags)
{
    if (enabled) update(flags);
}


QT_END_NAMESPACE
