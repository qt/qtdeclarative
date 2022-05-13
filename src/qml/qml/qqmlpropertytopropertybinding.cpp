// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
