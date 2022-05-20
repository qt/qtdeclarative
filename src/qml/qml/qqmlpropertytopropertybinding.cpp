/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qqmlpropertytopropertybinding_p.h"
#include <private/qqmlvmemetaobject_p.h>

QT_BEGIN_NAMESPACE

/*!
 * \internal
 * \class QQmlPropertyToPropertyBinding
 *
 * This class can be used to create a direct binding from a source property to
 * a target property, without going through QQmlJavaScriptExpression and
 * QV4::Function. In particular you don't need a compilation unit or byte code
 * to set this up.
 */

QQmlPropertyToPropertyBinding::QQmlPropertyToPropertyBinding(
        QQmlEngine *engine, QObject *sourceObject, int sourcePropertyIndex,
        QObject *targetObject, int targetPropertyIndex)
    : QQmlNotifierEndpoint(QQmlPropertyGuard)
    , m_engine(engine)
    , m_sourceObject(sourceObject)
    , m_sourcePropertyIndex(sourcePropertyIndex)
{
    setTarget(targetObject, targetPropertyIndex, false, -1);
}

QQmlAbstractBinding::Kind QQmlPropertyToPropertyBinding::kind() const
{
    return PropertyToPropertyBinding;
}

void QQmlPropertyToPropertyBinding::setEnabled(bool e, QQmlPropertyData::WriteFlags flags)
{
    const bool wasEnabled = enabledFlag();
    setEnabledFlag(e);
    updateCanUseAccessor();
    if (e && !wasEnabled)
        update(flags);
}

void QQmlPropertyToPropertyBinding::captureProperty(
        const QMetaObject *sourceMetaObject, int notifyIndex,
        bool isSourceBindable, bool isTargetBindable)
{
    if (isSourceBindable) {
        // if the property is a QPropery, and we're binding to a QProperty
        // the automatic capturing process already takes care of everything
        if (isTargetBindable)
            return;

        // We have already captured.
        if (observer)
            return;

        observer = std::make_unique<Observer>(this);
        QUntypedBindable bindable;
        void *argv[] = { &bindable };
        sourceMetaObject->metacall(
                    m_sourceObject, QMetaObject::BindableProperty, m_sourcePropertyIndex, argv);
        bindable.observe(observer.get());
        return;
    }

    // We cannot capture non-bindable properties without signals
    if (notifyIndex == -1)
        return;

    if (isConnected(m_sourceObject, notifyIndex))
        cancelNotify();
    else
        connect(m_sourceObject, notifyIndex, m_engine, true);
}

void QQmlPropertyToPropertyBinding::update(QQmlPropertyData::WriteFlags flags)
{
    if (!enabledFlag())
        return;

    // Check that the target has not been deleted
    QObject *target = targetObject();
    if (QQmlData::wasDeleted(target))
        return;

    const QQmlPropertyData *d = nullptr;
    QQmlPropertyData vtd;
    getPropertyData(&d, &vtd);
    Q_ASSERT(d);

    // Check for a binding update loop
    if (Q_UNLIKELY(updatingFlag())) {
        QQmlAbstractBinding::printBindingLoopError(
                    QQmlPropertyPrivate::restore(target, *d, &vtd, nullptr));
        return;
    }

    setUpdatingFlag(true);

    if (canUseAccessor())
        flags.setFlag(QQmlPropertyData::BypassInterceptor);

    const QMetaObject *sourceMetaObject = m_sourceObject->metaObject();
    const QMetaProperty property = sourceMetaObject->property(m_sourcePropertyIndex);
    if (!property.isConstant()) {
        captureProperty(sourceMetaObject, QMetaObjectPrivate::signalIndex(property.notifySignal()),
                        property.isBindable(), !vtd.isValid() && d->isBindable());
    }

    QQmlPropertyPrivate::writeValueProperty(
            target, *d, vtd, property.read(m_sourceObject), {}, flags);

    setUpdatingFlag(false);
}

void QQmlPropertyGuard_callback(QQmlNotifierEndpoint *e, void **)
{
    static_cast<QQmlPropertyToPropertyBinding *>(e)->update();
}

void QQmlPropertyToPropertyBinding::Observer::trigger(
        QPropertyObserver *observer, QUntypedPropertyData *)
{
    static_cast<Observer *>(observer)->binding->update();
}

QT_END_NAMESPACE
