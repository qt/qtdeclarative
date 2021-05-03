/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QQMLPROPERTYBINDING_P_H
#define QQMLPROPERTYBINDING_P_H

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

#include <QtCore/qproperty.h>
#include <QtCore/private/qproperty_p.h>

#include "qqmlpropertydata_p.h"
#include "qqmljavascriptexpression_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

namespace QV4 {
    struct BoundFunction;
}

class QQmlPropertyBinding;

class Q_QML_PRIVATE_EXPORT QQmlPropertyBindingJS : public QQmlJavaScriptExpression
{
    friend class QQmlPropertyBinding;
    void expressionChanged() override;
    QQmlPropertyBinding *asBinding()
    {
        return const_cast<QQmlPropertyBinding *>(static_cast<const QQmlPropertyBindingJS *>(this)->asBinding());
    }
    QQmlPropertyBinding const *asBinding() const;;
};

class Q_QML_PRIVATE_EXPORT QQmlPropertyBindingJSForBoundFunction : public QQmlPropertyBindingJS
{
public:
    QV4::ReturnedValue evaluate(bool *isUndefined);
    QV4::PersistentValue m_boundFunction;
};

class Q_QML_PRIVATE_EXPORT QQmlPropertyBinding : public QPropertyBindingPrivate

{
    friend class QQmlPropertyBindingJS;
public:

    QQmlPropertyBindingJS *jsExpression()
    {
        return const_cast<QQmlPropertyBindingJS *>(static_cast<const QQmlPropertyBinding *>(this)->jsExpression());
    }
    QQmlPropertyBindingJS const *jsExpression() const;

    static QUntypedPropertyBinding create(const QQmlPropertyData *pd, QV4::Function *function,
                                          QObject *obj, const QQmlRefPointer<QQmlContextData> &ctxt,
                                          QV4::ExecutionContext *scope, QObject *target,
                                          QQmlPropertyIndex targetIndex);
    static QUntypedPropertyBinding createFromCodeString(const QQmlPropertyData *property,
                                                        const QString &str, QObject *obj,
                                                        const QQmlRefPointer<QQmlContextData> &ctxt,
                                                        const QString &url, quint16 lineNumber,
                                                        QObject *target, QQmlPropertyIndex targetIndex);

    static QUntypedPropertyBinding createFromBoundFunction(const QQmlPropertyData *pd, QV4::BoundFunction *function,
                                          QObject *obj, const QQmlRefPointer<QQmlContextData> &ctxt,
                                          QV4::ExecutionContext *scope, QObject *target,
                                          QQmlPropertyIndex targetIndex);

    static bool isUndefined(const QUntypedPropertyBinding &binding)
    {
        return isUndefined(QPropertyBindingPrivate::get(binding));
    }

    static bool isUndefined(const QPropertyBindingPrivate *binding)
    {
        if (!(binding && binding->hasCustomVTable()))
            return false;
        return static_cast<const QQmlPropertyBinding *>(binding)->isUndefined();
    }

    static bool doEvaluate(QMetaType metaType, QUntypedPropertyData *dataPtr, void *f) {
        auto address = static_cast<std::byte*>(f);
        address -= sizeof (QPropertyBindingPrivate); // f now points to QPropertyBindingPrivate suboject
        // and that has the same address as QQmlPropertyBinding
        return reinterpret_cast<QQmlPropertyBinding *>(address)->evaluate(metaType, dataPtr);
    }

    bool hasDependencies();

private:
    bool evaluate(QMetaType metaType, void *dataPtr);

    Q_NEVER_INLINE void handleUndefinedAssignment(QQmlEnginePrivate *ep, void *dataPtr);

    QString createBindingLoopErrorDescription(QJSEnginePrivate *ep);

    struct TargetData {
        enum BoundFunction : bool {
            WithoutBoundFunction = false,
            HasBoundFunction = true,
        };
        TargetData(QObject *target, QQmlPropertyIndex index, BoundFunction state)
            : target(target), targetIndex(index), hasBoundFunction(state)
        {}
        QObject *target;
        QQmlPropertyIndex targetIndex;
        bool hasBoundFunction;
        bool isUndefined = false;
    };
    QQmlPropertyBinding(QMetaType metaType, QObject *target, QQmlPropertyIndex targetIndex, TargetData::BoundFunction hasBoundFunction);

    QObject *target();
    QQmlPropertyIndex targetIndex();
    bool hasBoundFunction();
    bool isUndefined() const;
    void setIsUndefined(bool isUndefined);

    static void bindingErrorCallback(QPropertyBindingPrivate *);
};

template <auto I>
struct Print {};

namespace QtPrivate {
template<>
inline constexpr BindingFunctionVTable bindingFunctionVTable<QQmlPropertyBinding> = {
    &QQmlPropertyBinding::doEvaluate,
    [](void *qpropertyBinding){
        QQmlPropertyBinding *binding = reinterpret_cast<QQmlPropertyBinding *>(qpropertyBinding);
        binding->jsExpression()->~QQmlPropertyBindingJS();
        binding->~QQmlPropertyBinding();
        auto address = static_cast<std::byte*>(qpropertyBinding);
        delete[] address;
    },
    [](void *, void *){},
    0
};
}

class QQmlTranslationPropertyBinding
{
public:
    static QUntypedPropertyBinding Q_QML_PRIVATE_EXPORT create(const QQmlPropertyData *pd,
                                          const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
                                          const QV4::CompiledData::Binding *binding);
};

QT_END_NAMESPACE

#endif // QQMLPROPERTYBINDING_P_H
