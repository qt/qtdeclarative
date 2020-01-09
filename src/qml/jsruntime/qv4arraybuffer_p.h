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
#ifndef QV4ARRAYBUFFER_H
#define QV4ARRAYBUFFER_H

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
#include <QtCore/qarraydatapointer.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

struct SharedArrayBufferCtor : FunctionObject {
    void init(QV4::ExecutionContext *scope);
};

struct ArrayBufferCtor : SharedArrayBufferCtor {
    void init(QV4::ExecutionContext *scope);
};

struct Q_QML_PRIVATE_EXPORT SharedArrayBuffer : Object {
    void init(size_t length);
    void init(const QByteArray& array);
    void destroy();

    void setSharedArrayBuffer(bool shared) noexcept { isShared = shared; }
    bool isSharedArrayBuffer() const noexcept { return isShared; }

    char *arrayData() noexcept { return arrayDataPointer()->data(); }
    const char *constArrayData() const noexcept { return constArrayDataPointer()->data(); }
    uint arrayDataLength() const noexcept { return constArrayDataPointer().size; }

    bool hasSharedArrayData() const noexcept { return constArrayDataPointer().isShared(); }
    bool hasDetachedArrayData() const noexcept { return constArrayDataPointer().isNull(); }
    void detachArrayData() noexcept { arrayDataPointer().clear(); }

    bool arrayDataNeedsDetach() const noexcept { return constArrayDataPointer().needsDetach(); }

private:
    const QArrayDataPointer<const char> &constArrayDataPointer() const noexcept
    {
        return *reinterpret_cast<const QArrayDataPointer<const char> *>(&arrayDataPointerStorage);
    }
    QArrayDataPointer<char> &arrayDataPointer() noexcept
    {
        return *reinterpret_cast<QArrayDataPointer<char> *>(&arrayDataPointerStorage);
    }

    std::aligned_storage_t<sizeof(QArrayDataPointer<char>), alignof(QArrayDataPointer<char>)>
    arrayDataPointerStorage;
    bool isShared;
};

struct Q_QML_PRIVATE_EXPORT ArrayBuffer : SharedArrayBuffer {
    void init(size_t length) {
        SharedArrayBuffer::init(length);
        setSharedArrayBuffer(false);
    }
    void init(const QByteArray& array) {
        SharedArrayBuffer::init(array);
        setSharedArrayBuffer(false);
    }
};

}

struct SharedArrayBufferCtor : FunctionObject
{
    V4_OBJECT2(SharedArrayBufferCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct ArrayBufferCtor : SharedArrayBufferCtor
{
    V4_OBJECT2(ArrayBufferCtor, SharedArrayBufferCtor)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);

    static ReturnedValue method_isView(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

struct Q_QML_PRIVATE_EXPORT SharedArrayBuffer : Object
{
    V4_OBJECT2(SharedArrayBuffer, Object)
    V4_NEEDS_DESTROY
    V4_PROTOTYPE(sharedArrayBufferPrototype)

    QByteArray asByteArray() const;

    uint arrayDataLength() const { return d()->arrayDataLength(); }
    char *arrayData() { return d()->arrayData(); }
    const char *constArrayData() const { return d()->constArrayData(); }

    bool hasSharedArrayData() { return d()->hasSharedArrayData(); }
    bool hasDetachedArrayData() const { return d()->hasDetachedArrayData(); }
    bool isSharedArrayBuffer() const { return d()->isSharedArrayBuffer(); }
};

struct Q_QML_PRIVATE_EXPORT ArrayBuffer : SharedArrayBuffer
{
    V4_OBJECT2(ArrayBuffer, SharedArrayBuffer)
    V4_NEEDS_DESTROY
    V4_PROTOTYPE(arrayBufferPrototype)

    QByteArray asByteArray() const;
    uint arrayDataLength() const { return d()->arrayDataLength(); }
    char *dataData() { if (d()->arrayDataNeedsDetach()) detach(); return d()->arrayData(); }
    // ### is that detach needed?
    const char *constArrayData() const { return d()->constArrayData(); }
    bool hasSharedArrayData() { return d()->hasSharedArrayData(); }
    void detachArrayData() { d()->detachArrayData(); }

    void detach();
};

struct SharedArrayBufferPrototype : Object
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_get_byteLength(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_slice(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue slice(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc, bool shared);
};

struct ArrayBufferPrototype : SharedArrayBufferPrototype
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_get_byteLength(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_slice(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


} // namespace QV4

QT_END_NAMESPACE

#endif
