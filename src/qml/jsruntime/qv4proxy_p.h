/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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
#ifndef QV4PROXY_P_H
#define QV4PROXY_P_H

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

#include "qv4object_p.h"
#include "qv4functionobject_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

#define ProxyObjectMembers(class, Member) \
    Member(class, Pointer, Object *, target) \
    Member(class, Pointer, Object *, handler)

DECLARE_HEAP_OBJECT(ProxyObject, Object) {
    DECLARE_MARKOBJECTS(ProxyObject)

    void init(const QV4::Object *target, const QV4::Object *handler);
};

#define ProxyMembers(class, Member) \
    Member(class, Pointer, Symbol *, revokableProxySymbol) \

DECLARE_HEAP_OBJECT(Proxy, FunctionObject) {
    DECLARE_MARKOBJECTS(Proxy)

    void init(QV4::ExecutionContext *ctx);
};

}

struct ProxyObject: Object {
    V4_OBJECT2(ProxyObject, Object)
    Q_MANAGED_TYPE(ProxyObject)
    V4_INTERNALCLASS(ProxyObject)

    static ReturnedValue virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
    static bool virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver);
    static bool virtualDeleteProperty(Managed *m, PropertyKey id);
    static bool virtualHasProperty(const Managed *m, PropertyKey id);
    static PropertyAttributes virtualGetOwnProperty(Managed *m, PropertyKey id, Property *p);
    static bool virtualDefineOwnProperty(Managed *m, PropertyKey id, const Property *p, PropertyAttributes attrs);
    static bool virtualIsExtensible(const Managed *m);
    static bool virtualPreventExtensions(Managed *);
    static Heap::Object *virtualGetPrototypeOf(const Managed *);
    static bool virtualSetPrototypeOf(Managed *, const Object *);

    // those might require a second proxy object that derives from FunctionObject...
//    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
//    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct Proxy : FunctionObject
{
    V4_OBJECT2(Proxy, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue method_revocable(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue method_revoke(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

}

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
