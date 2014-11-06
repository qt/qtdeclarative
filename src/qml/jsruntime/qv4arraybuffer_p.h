/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4ARRAYBUFFER_H
#define QV4ARRAYBUFFER_H

#include "qv4object_p.h"
#include "qv4functionobject_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

struct ArrayBufferCtor : FunctionObject {
    ArrayBufferCtor(QV4::ExecutionContext *scope);
};

struct ArrayBuffer : Object {
    ArrayBuffer(ExecutionEngine *e, int length);
    QTypedArrayData<char> *data;
};

}

struct ArrayBufferCtor: FunctionObject
{
    V4_OBJECT2(ArrayBufferCtor, FunctionObject)

    static ReturnedValue construct(Managed *m, CallData *callData);
    static ReturnedValue call(Managed *that, CallData *callData);

    static ReturnedValue method_isView(CallContext *ctx);

};

struct ArrayBuffer : Object
{
    V4_OBJECT2(ArrayBuffer, Object)

    QByteArray asByteArray() const;
    uint byteLength() const { return d()->data->size; }
    char *data() {
        // ### detach if refcount > 1
        return d()->data->data();
    }
    const char *constData() {
        // ### detach if refcount > 1
        return d()->data->data();
    }

    static void destroy(Managed *m);
};

struct ArrayBufferPrototype: Object
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_get_byteLength(CallContext *ctx);
    static ReturnedValue method_slice(CallContext *ctx);
};


} // namespace QV4

QT_END_NAMESPACE

#endif
