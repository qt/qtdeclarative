/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QQMLANYBINDINGPTR_P_H
#define QQMLANYBINDINGPTR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qqmlproperty.h>
#include <private/qqmlpropertybinding_p.h>
#include <private/qqmlbinding_p.h>

// Fully inline so that subsequent prop.isBindable check might get ellided.

/*!
    \internal
    \brief QQmlAnyBinding is an abstraction over the various bindings in QML

    QQmlAnyBinding can store both classical bindings (derived from QQmlAbstractBinding)
    as well as new-style bindings (derived from QPropertyBindingPrivate). For both, it keeps
    a strong reference to them, and knows how to delete them in case the reference count
    becomes zero. In that sense it can be thought of as a union of QUntypedPropertyBinding
    and QQmlAbstractBinding::Ptr.

    It also offers methods to create bindings (from QV4::Function, from translation bindings
    and from code strings). Moreover, it allows the retrieval, the removal and the
    installation of bindings on a QQmlProperty.

    Note that the class intentionally does not allow construction from QUntypedProperty and
    QQmlAbstractBinding::Ptr. This is meant to catch code which doesn't handle bindable properties
    yet when porting existing code.
 */
class QQmlAnyBinding {
public:

    QQmlAnyBinding() = default;
    QQmlAnyBinding(std::nullptr_t) : d(static_cast<QQmlAbstractBinding *>(nullptr)) {}


    /*!
        \internal
        Returns the binding of the property \a prop as a QQmlAnyBinding.
        The binding continues to be active and set on the property.
        If there was no binding set, the returned QQmlAnyBinding is null.
     */
    static QQmlAnyBinding ofProperty(const QQmlProperty &prop) {
        QQmlAnyBinding binding;
        if (prop.isBindable()) {
            QUntypedBindable bindable = prop.property().bindable(prop.object());
            binding = bindable.binding();
        } else {
            binding = QQmlPropertyPrivate::binding(prop);
        }
        return binding;
    }

    /*!
        Removes the binding from the property \a prop, and returns it as a
        QQmlAnyBinding if there was any. Otherwise returns a null
        QQmlAnyBinding.
     */
    static QQmlAnyBinding takeFrom(const QQmlProperty &prop)
    {
        QQmlAnyBinding binding;
        if (prop.isBindable()) {
            QUntypedBindable bindable = prop.property().bindable(prop.object());
            binding = bindable.takeBinding();
        } else {
            auto qmlBinding = QQmlPropertyPrivate::binding(prop);
            if (qmlBinding) {
                binding = qmlBinding; // this needs to run before removeFromObject, else the refcount might reach zero
                qmlBinding->setEnabled(false, QQmlPropertyData::DontRemoveBinding | QQmlPropertyData::BypassInterceptor);
                qmlBinding->removeFromObject();
            }
        }
        return binding;
    }

    /*!
        \internal
        Creates a binding for property \a prop from \a function.
        \a obj is the scope object which shall be used for the function and \a scope its QML scope.
        The binding is not installed on the property (but if a QQmlBinding is created, it has its
        target set to \a prop).
     */
    static QQmlAnyBinding createFromFunction(const QQmlProperty &prop, QV4::Function *function,
                                                  QObject *obj, const QQmlRefPointer<QQmlContextData> &ctxt,
                                                  QV4::ExecutionContext *scope)
    {
        QQmlAnyBinding binding;
        auto propPriv = QQmlPropertyPrivate::get(prop);
        if (prop.isBindable()) {
            auto index = QQmlPropertyIndex(propPriv->core.coreIndex(), -1);
            binding = QQmlPropertyBinding::create(&propPriv->core,
                                                  function, obj, ctxt,
                                                  scope, prop.object(), index);
        } else {
            auto qmlBinding = QQmlBinding::create(&propPriv->core, function, obj, ctxt, scope);
            qmlBinding->setTarget(prop);
            binding = qmlBinding;
        }
        return binding;
    }

    /*!
        \internal
        Creates a binding for property \a prop from \a script.
        \a obj is the scope object which shall be used for the function and \a ctxt its QML scope.
        The binding is not installed on the property (but if a QQmlBinding is created, it has its
        target set to \a prop).
     */
    static QQmlAnyBinding createFromScriptString(const QQmlProperty &prop, const QQmlScriptString &script,
                                                  QObject *obj,  QQmlContext *ctxt)
    {
        QQmlAnyBinding binding;
        auto propPriv = QQmlPropertyPrivate::get(prop);
        if (prop.isBindable()) {
            auto index = QQmlPropertyIndex(propPriv->core.coreIndex(), -1);
            binding = QQmlPropertyBinding::createFromScriptString(&propPriv->core, script, obj, ctxt, prop.object(), index);
        } else {
            auto qmlBinding = QQmlBinding::create(&propPriv->core, script, obj, ctxt);
            qmlBinding->setTarget(prop);
            binding = qmlBinding;
        }
        return binding;
    }


    /*!
        \internal
        Removes the binding from \a prop if there is any.
     */
    static void removeBindingFrom(QQmlProperty &prop)
    {
        if (prop.isBindable())
            prop.property().bindable(prop.object()).takeBinding();
        else
            QQmlPropertyPrivate::removeBinding(prop);
    }

    /*!
        \internal
        Creates a binding for property \a prop from \a function.
        \a obj is the scope object which shall be used for the function and \a scope its QML scope.
        The binding is not installed on the property (but if a QQmlBinding is created, it has its
        target set to \a prop).
     */
    static QQmlAnyBinding createFromCodeString(const QQmlProperty &prop, const QString& code, QObject *obj, const QQmlRefPointer<QQmlContextData> &ctxt, const QString &url, quint16 lineNumber) {
        QQmlAnyBinding binding;
        auto propPriv = QQmlPropertyPrivate::get(prop);
        if (prop.isBindable()) {
            auto index = QQmlPropertyIndex(propPriv->core.coreIndex(), -1);
            binding = QQmlPropertyBinding::createFromCodeString(&propPriv->core,
                                                                code, obj, ctxt,
                                                                url, lineNumber,
                                                                prop.object(), index);
        } else {
            auto qmlBinding = QQmlBinding::create(&propPriv->core, code, obj, ctxt, url, lineNumber);
            qmlBinding->setTarget(prop);
            binding = qmlBinding;
        }
        return binding;
    }

    /*!
        \internal
        Creates a translattion binding for \a prop from \a compilationUnit and \a transationBinding.
        \a obj is the context object, \a context the qml context.
    */
    static QQmlAnyBinding createTranslationBinding(const QQmlProperty &prop, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QV4::CompiledData::Binding *translationBinding, QObject *scopeObject=nullptr, QQmlRefPointer<QQmlContextData> context={})
    {
        QQmlAnyBinding binding;
        auto propPriv = QQmlPropertyPrivate::get(prop);
        if (prop.isBindable()) {
            binding = QQmlTranslationPropertyBinding::create(&propPriv->core, compilationUnit, translationBinding);
        } else {
            auto qmlBinding = QQmlBinding::createTranslationBinding(compilationUnit, translationBinding, scopeObject, context);
            binding = qmlBinding;
            qmlBinding->setTarget(prop);
        }
        return binding;
    }

    /*!
        \internal
        Installs the binding referenced by this QQmlAnyBinding on the target.
        If \a mode is set to RespectInterceptors, interceptors are honored, otherwise
        writes and binding installation bypass them (the default).
        Preconditions:
        - The binding is non-null.
        - If the binding is QQmlAbstractBinding derived, the target is non-bindable.
        - If the binding is a QUntypedPropertyBinding, then the target is bindable.
     */
    enum InterceptorMode : bool {
        IgnoreInterceptors,
        RespectInterceptors
    };

    void installOn(const QQmlProperty &target, InterceptorMode mode = IgnoreInterceptors)
    {
        Q_ASSERT(!d.isNull());
        if (isAbstractPropertyBinding()) {
            auto abstractBinding = asAbstractBinding();
            Q_ASSERT(abstractBinding->targetObject() == target.object() || QQmlPropertyPrivate::get(target)->core.isAlias());
            Q_ASSERT(!target.isBindable());
            if (mode == IgnoreInterceptors)
                QQmlPropertyPrivate::setBinding(abstractBinding, QQmlPropertyPrivate::None, QQmlPropertyData::DontRemoveBinding | QQmlPropertyData::BypassInterceptor);
            else
                QQmlPropertyPrivate::setBinding(abstractBinding);
        } else {
            Q_ASSERT(target.isBindable());
            QUntypedBindable bindable;
            void *argv[] = {&bindable};
            if (mode == IgnoreInterceptors) {
                target.object()->qt_metacall(QMetaObject::BindableProperty, target.index(), argv);
            } else {
                QMetaObject::metacall(target.object(), QMetaObject::BindableProperty, target.index(), argv);
            }
            bindable.setBinding(asUntypedPropertyBinding());
        }
    }

    /*!
        \internal
        Returns true if the binding is in an error state (e.g. binding loop), false otherwise.

        \note For ValueTypeProxyBindings, this methods will always return false
    */
    bool hasError() {
        if (isAbstractPropertyBinding()) {
            auto abstractBinding = asAbstractBinding();
            if (abstractBinding->isValueTypeProxy())
                return false;
            return static_cast<QQmlBinding *>(abstractBinding)->hasError();
        } else {
            return asUntypedPropertyBinding().error().hasError();
        }
    }

    /*!
        Stores a null binding. For purpose of classification, the null bindings is
        treated as a QQmlAbstractPropertyBindings.
     */
    QQmlAnyBinding& operator=(std::nullptr_t )
    {
        clear();
        return *this;
    }

    operator bool() const{
        return !d.isNull();
    }

    /*!
        \internal
        Returns true if a binding derived from QQmlAbstractPropertyBinding is stored.
        The binding migh still be null.
     */
    bool isAbstractPropertyBinding() const
    { return d.isT1(); }

    /*!
        \internal
        Returns true if a binding derived from QPropertyBindingPrivate is stored.
        The binding might still be null.
     */
    bool isUntypedPropertyBinding() const
    { return d.isT2(); }

    /*!
        \internal
        Returns the stored QPropertyBindingPrivate as a QUntypedPropertyBinding.
        If no such binding is currently stored, a null QUntypedPropertyBinding is returned.
     */
    QUntypedPropertyBinding asUntypedPropertyBinding() const
    {
        if (d.isT1()  || d.isNull())
            return {};
        auto priv = d.asT2();
        return QUntypedPropertyBinding {priv};
    }

    /*!
        \internal
        Returns the stored QQmlAbstractBinding.
        If no such binding is currently stored, a null pointer is returned.
     */
    QQmlAbstractBinding *asAbstractBinding() const
    {
        if (d.isT2() || d.isNull())
            return nullptr;
        return d.asT1();
    }

    /*!
        \internal
        Stores \a binding and keeps a reference to it.
     */
    QQmlAnyBinding& operator=(QQmlAbstractBinding *binding)
    {
        clear();
        if (binding) {
            d = binding;
            binding->ref.ref();
        }
        return *this;
    }

    /*!
        \internal
        Stores the binding stored in \a binding and keeps a reference to it.
     */
    QQmlAnyBinding& operator=(const QQmlAbstractBinding::Ptr &binding)
    {
        clear();
        if (binding) {
            d = binding.data();
            binding->ref.ref();
        }
        return *this;
    }

    /*!
        \internal
        Stores \a binding's binding, taking ownership from \a binding.
     */
    QQmlAnyBinding& operator=(QQmlAbstractBinding::Ptr &&binding)
    {
        clear();
        if (binding) {
            d = binding.take();
        }
        return *this;
    }

    /*!
        \internal
        Stores the binding stored in \a untypedBinding and keeps a reference to it.
     */
    QQmlAnyBinding& operator=(const QUntypedPropertyBinding &untypedBinding)
    {
        clear();
        auto binding = QPropertyBindingPrivate::get(untypedBinding);
        if (binding) {
            d = binding;
            binding->addRef();
        }
        return *this;
    }

    /*!
        \internal
        \overload
        Stores the binding stored in \a untypedBinding, taking ownership from it.
     */
    QQmlAnyBinding& operator=(const QUntypedPropertyBinding &&untypedBinding)
    {
        clear();
        auto binding = QPropertyBindingPrivate::get(untypedBinding);
        QPropertyBindingPrivatePtr ptr(binding);
        if (binding) {
            d = static_cast<QPropertyBindingPrivate *>(ptr.take());
        }
        return *this;
    }

    QQmlAnyBinding(const QQmlAnyBinding &other)
    {
        *this = other;
    }

    friend void swap(const QQmlAnyBinding &a, const QQmlAnyBinding &b)
    {
        qSwap(a.d, b.d);
    }

    QQmlAnyBinding& operator=(const QQmlAnyBinding &other)
    {
        clear();
        if (auto abstractBinding = other.asAbstractBinding())
            *this = abstractBinding;
        else if (auto untypedBinding = other.asUntypedPropertyBinding(); !untypedBinding.isNull())
            *this = untypedBinding;
        return *this;
    }

    friend inline bool operator==(const QQmlAnyBinding &p1, const QQmlAnyBinding &p2)
    {
        return p1.d == p2.d;
    }

    friend inline bool operator!=(const QQmlAnyBinding &p1, const QQmlAnyBinding &p2)
    {
        return p1.d != p2.d;
    }

    ~QQmlAnyBinding() {
        clear();
    }
private:
    void clear() {
        if (d.isNull())
            return;
        if (d.isT1()) {
            QQmlAbstractBinding *qqmlptr = d.asT1();
            if (!qqmlptr->ref.deref())
                delete  qqmlptr;
        } else if (d.isT2()) {
            QPropertyBindingPrivate *priv = d.asT2();
            priv->ref--;
            if (!priv->ref)
                QPropertyBindingPrivate::destroyAndFreeMemory(priv);
        }
        d = static_cast<QQmlAbstractBinding *>(nullptr);
    }
    QBiPointer<QQmlAbstractBinding, QPropertyBindingPrivate> d = static_cast<QQmlAbstractBinding *>(nullptr);
};


#endif // QQMLANYBINDINGPTR_P_H
