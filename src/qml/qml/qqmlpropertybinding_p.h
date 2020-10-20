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

class QQmlPropertyBinding : public QQmlJavaScriptExpression,
                            public QPropertyBindingPrivate

{
public:
    static constexpr auto propertyBindingOffset() {
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF
        return offsetof(QQmlPropertyBinding, ref);
        QT_WARNING_POP
    }

    static QUntypedPropertyBinding create(const QQmlPropertyData *pd, QV4::Function *function,
                                          QObject *obj, const QQmlRefPointer<QQmlContextData> &ctxt,
                                          QV4::ExecutionContext *scope, QObject *target,
                                          QQmlPropertyIndex targetIndex);

    void expressionChanged() override;


    static bool doEvaluate(QMetaType metaType, QUntypedPropertyData *dataPtr, void *f) {
        auto address = static_cast<std::byte*>(f);
        address -= sizeof (QPropertyBindingPrivate); // f now points to QPropertyBindingPrivate suboject
        address -= QQmlPropertyBinding::propertyBindingOffset(); //  f now points to QQmlPropertyBinding
        return reinterpret_cast<QQmlPropertyBinding *>(address)->evaluate(metaType, dataPtr);
    }

private:
    QQmlPropertyBinding(QMetaType metaType, QObject *target, QQmlPropertyIndex targetIndex);

    bool evaluate(QMetaType metaType, void *dataPtr);

    QString createBindingLoopErrorDescription(QJSEnginePrivate *ep);

    struct TargetData {
        QObject *target;
        QQmlPropertyIndex targetIndex;
    };

    QObject *target();
    QQmlPropertyIndex targetIndex();

    static void bindingErrorCallback(QPropertyBindingPrivate *);
};

template <auto I>
struct Print {};

namespace QtPrivate {
template<>
inline constexpr BindingFunctionVTable bindingFunctionVTable<QQmlPropertyBinding> = {
    &QQmlPropertyBinding::doEvaluate,
    [](void *qpropertyBinding){
        auto address = static_cast<std::byte*>(qpropertyBinding);
        address -= QQmlPropertyBinding::propertyBindingOffset(); //  f now points to QQmlPropertyBinding
        reinterpret_cast<QQmlPropertyBinding *>(address)->~QQmlPropertyBinding();
        delete[] address;
    },
    [](void *, void *){},
    0
};
}

class QQmlTranslationPropertyBinding
{
public:
    static QUntypedPropertyBinding create(const QQmlPropertyData *pd,
                                          const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
                                          const QV4::CompiledData::Binding *binding);
};

QT_END_NAMESPACE

#endif // QQMLPROPERTYBINDING_P_H
