// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlpropertytopropertybinding_p.h"

#include <private/qqmlanybinding_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qv4alloca_p.h>

#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*!
 * \internal
 * \class QQmlPropertyToPropertyBinding
 *
 * This class can be used to create a direct binding from a source property to
 * a target property, without going through QQmlJavaScriptExpression and
 * QV4::Function. In particular you don't need a compilation unit or byte code
 * to set this up.
 *
 * \note The target cannot be a group property, but the source can.
 */

QQmlAnyBinding QQmlPropertyToPropertyBinding::create(
        QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target)
{
    QQmlAnyBinding result;
    if (target.isBindable() && !QQmlPropertyPrivate::get(target)->isValueType()) {
        QPropertyBindingPrivate *binding = nullptr;
        if (source.isBindable())
            binding = new QQmlBindableToBindablePropertyBinding(engine, source, target);
        else
            binding = new QQmlUnbindableToBindablePropertyBinding(engine, source, target);
        result = QPropertyBindingPrivate::makeUntyped(binding);
        return result;
    }

    if (source.isBindable()) {
        result = new QQmlBindableToUnbindablePropertyBinding(engine, source, target);
        return result;
    }

    result = new QQmlUnbindableToUnbindablePropertyBinding(engine, source, target);
    return result;
}

QQmlPropertyToPropertyBinding::QQmlPropertyToPropertyBinding(
        QQmlEngine *engine, const QQmlProperty &source)
    : engine(engine)
    , sourceObject(source.object())
    , sourcePropertyIndex(QQmlPropertyPrivate::get(source)->encodedIndex())
{
}

QQmlUnbindableToUnbindablePropertyBinding::QQmlUnbindableToUnbindablePropertyBinding(
        QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target)
    : QQmlNotifierEndpoint(QQmlUnbindableToUnbindableGuard)
    , QQmlPropertyToUnbindablePropertyBinding(engine, source, target)
{
}

QQmlAbstractBinding::Kind QQmlPropertyToUnbindablePropertyBinding::kind() const
{
    return PropertyToPropertyBinding;
}

void QQmlPropertyToUnbindablePropertyBinding::setEnabled(
        bool e, QQmlPropertyData::WriteFlags flags)
{
    const bool wasEnabled = enabledFlag();
    setEnabledFlag(e);
    updateCanUseAccessor();
    if (e && !wasEnabled)
        update(flags);
}

void QQmlPropertyToUnbindablePropertyBinding::update(QQmlPropertyData::WriteFlags flags)
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
    if (Q_UNLIKELY(updatingFlag()))
        return;

    setUpdatingFlag(true);

    if (canUseAccessor())
        flags.setFlag(QQmlPropertyData::BypassInterceptor);

    QVariant value = m_binding.readSourceValue(
            [&](const QMetaObject *sourceMetaObject, const QMetaProperty &property) {
        captureProperty(sourceMetaObject, property);
    });

    QV4::ExecutionEngine *v4 = m_binding.engine->handle();
    Q_ASSERT(v4);

    const QMetaType targetMetaType = vtd.isValid() ? vtd.propType() : d->propType();
    const QMetaType valueMetaType = value.metaType();

    if (valueMetaType != targetMetaType && targetMetaType != QMetaType::fromType<QVariant>()) {
        // Coerce the value to the target type using JavaScript semantics.
        // Don't use QQmlProperty's internal coercion. It's not the same.
        QVariant coerced(targetMetaType);
        QV4::Scope scope(v4);
        QV4::ScopedValue jsValue(
                scope, value.isValid()
                        ? v4->metaTypeToJS(valueMetaType, value.constData())
                        : QV4::Encode::undefined());

        // If the coercion fails, let writeValueProperty() do its thing after all.
        if (QV4::ExecutionEngine::metaTypeFromJS(jsValue, targetMetaType, coerced.data()))
            value = std::move(coerced);
    }

    QQmlPropertyPrivate::writeValueProperty(target, *d, vtd, value, {}, flags);
    setUpdatingFlag(false);
}

QQmlPropertyToUnbindablePropertyBinding::QQmlPropertyToUnbindablePropertyBinding(
        QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target)
    : m_binding(engine, source)
{
    setTarget(target);
}

QQmlBindableToUnbindablePropertyBinding::QQmlBindableToUnbindablePropertyBinding(
        QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target)
    : QPropertyObserver(QQmlBindableToUnbindablePropertyBinding::update)
    , QQmlPropertyToUnbindablePropertyBinding(engine, source, target)
{
}

void QQmlBindableToUnbindablePropertyBinding::update(
        QPropertyObserver *observer, QUntypedPropertyData *)
{
    static_cast<QQmlBindableToUnbindablePropertyBinding *>(observer)
            ->QQmlPropertyToUnbindablePropertyBinding::update();
}

void QQmlUnbindableToUnbindablePropertyBinding::captureProperty(
        const QMetaObject *sourceMetaObject, const QMetaProperty &sourceProperty)
{
    Q_UNUSED(sourceMetaObject);
    m_binding.doConnectNotify(this, sourceProperty);
}

void QQmlBindableToUnbindablePropertyBinding::captureProperty(
        const QMetaObject *sourceMetaObject, const QMetaProperty &sourceProperty)
{
    Q_UNUSED(sourceProperty);

    // We have already captured.
    if (m_isObserving)
        return;

    QUntypedBindable bindable;
    void *argv[] = { &bindable };
    sourceMetaObject->metacall(
            m_binding.sourceObject, QMetaObject::BindableProperty,
            m_binding.sourcePropertyIndex.coreIndex(), argv);
    bindable.observe(this);
    m_isObserving = true;
}

namespace QtPrivate {
template<typename Binding>
inline constexpr BindingFunctionVTable
    bindingFunctionVTableForQQmlPropertyToBindablePropertyBinding = {
        &QQmlPropertyToBindablePropertyBinding::update<Binding>,
        [](void *qpropertyBinding) { delete reinterpret_cast<Binding *>(qpropertyBinding); },
        [](void *, void *){},
        0
    };
}

QQmlPropertyToBindablePropertyBinding::QQmlPropertyToBindablePropertyBinding(
        QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target,
        const QtPrivate::BindingFunctionVTable *vtable)
    : QQmlPropertyBindingBase(target.object(), QQmlPropertyPrivate::propertyIndex(target),
                              target.propertyMetaType(), vtable, BindingKind::PropertyToProperty),
      m_binding(engine, source)
{
    errorCallBack = [](QPropertyBindingPrivate *that) {
        auto self = static_cast<QQmlPropertyToBindablePropertyBinding *>(that);
        if (self->m_binding.engine) {
            const auto error = self->bindingError();
            if (error.type() == QPropertyBindingError::BindingLoop) {
                qmlWarning(self->m_binding.sourceObject)
                        << QStringLiteral("Binding loop detected for property bound to ")
                        << self->propertyDataPtr;
            }
        }
    };
}

QQmlUnbindableToBindablePropertyBinding::QQmlUnbindableToBindablePropertyBinding(
        QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target)
    : QQmlPropertyToBindablePropertyBinding(
              engine, source, target,
              &QtPrivate::bindingFunctionVTableForQQmlPropertyToBindablePropertyBinding<
                      QQmlUnbindableToBindablePropertyBinding>)
    , QQmlNotifierEndpoint(QQmlUnbindableToBindableGuard)
{
}

void QQmlUnbindableToBindablePropertyBinding::update()
{
    PendingBindingObserverList bindingObservers;
    evaluateRecursive(bindingObservers);

    if (const QPropertyBindingError error = bindingError();
            Q_UNLIKELY(error.type() == QPropertyBindingError::BindingLoop)) {
        return;
    }

    notifyNonRecursive(bindingObservers);
}

QQmlBindableToBindablePropertyBinding::QQmlBindableToBindablePropertyBinding(
        QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target)
    : QQmlPropertyToBindablePropertyBinding(
              engine, source, target,
              &QtPrivate::bindingFunctionVTableForQQmlPropertyToBindablePropertyBinding<
                      QQmlBindableToBindablePropertyBinding>)
{
}

void QQmlUnbindableToUnbindableGuard_callback(QQmlNotifierEndpoint *e, void **)
{
    static_cast<QQmlUnbindableToUnbindablePropertyBinding *>(e)->update();
}

void QQmlUnbindableToBindableGuard_callback(QQmlNotifierEndpoint *e, void **)
{
    static_cast<QQmlUnbindableToBindablePropertyBinding *>(e)->update();
}

QT_END_NAMESPACE
