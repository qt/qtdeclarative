/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QJSVALUE_P_H
#define QJSVALUE_P_H

#include <qjsvalue.h>
#include <private/qtqmlglobal_p.h>
#include <private/qv4value_p.h>
#include <private/qv4string_p.h>
#include <private/qv4engine_p.h>
#include <private/qflagpointer_p.h>
#include <private/qv4mm_p.h>
#include <private/qv4persistent_p.h>

#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

// QJSValue::d is a QV4::ReturnedValue, but we don't want to expose that in the public header.
// We use the lower bits of the managed pointer to hide a QString* or a QV4::Value* in there.
Q_STATIC_ASSERT(sizeof(QV4::ReturnedValue) == sizeof(quint64));
Q_STATIC_ASSERT(alignof(QV4::Value) >= 4);
Q_STATIC_ASSERT(alignof(QString) >= 4);

enum PointerMask: quintptr {
    IsV4Value = 0x0,
    IsString = 0x1
};

class Q_AUTOTEST_EXPORT QJSValuePrivate
{
    static const QV4::Value *managedValue(const QV4::Value &v)
    {
        const quintptr m = quintptr(v.m());
        return (m & IsString) ? nullptr : reinterpret_cast<QV4::Value *>(m);
    }

    static QV4::Value *managedValue(QV4::Value &v)
    {
        quintptr m = quintptr(v.m());
        return (m & IsString) ? nullptr : reinterpret_cast<QV4::Value *>(m);
    }

    static const QString *qstring(const QV4::Value &v)
    {
        const quintptr m = quintptr(v.m());
        return (m & IsString) ? reinterpret_cast<QString *>(m & ~IsString) : nullptr;
    }

    static QV4::ReturnedValue encode(QString string)
    {
        const quintptr m = quintptr(new QString(std::move(string))) | IsString;
        return encodeRawValue(m);
    }

    static QV4::ReturnedValue encode(const QV4::Value &managedValue)
    {
        QV4::Value *m = managedValue.as<QV4::Managed>()->engine()
                ->memoryManager->m_persistentValues->allocate();
        *m = managedValue;
        return encodeRawValue(quintptr(m));
    }

    static QV4::ReturnedValue encodeRawValue(quintptr m)
    {
        return QV4::Value::fromHeapObject(reinterpret_cast<QV4::Heap::Base *>(m)).asReturnedValue();
    }

public:

    static void setRawValue(QJSValue *jsval, QV4::Value *m)
    {
        jsval->d = encodeRawValue(quintptr(m));
    }

    static QJSValue fromReturnedValue(QV4::ReturnedValue d)
    {
        QJSValue result;
        setValue(&result, d);
        return result;
    }

    template<typename T>
    static const T *asManagedType(const QJSValue *jsval)
    {
        const QV4::Value v = QV4::Value::fromReturnedValue(jsval->d);
        if (!v.isManaged())
            return nullptr;
        if (const QV4::Value *value = managedValue(v))
            return value->as<T>();
        return nullptr;
    }

    // This is a move operation and transfers ownership.
    static QV4::Value *takeManagedValue(QJSValue *jsval)
    {
        QV4::Value v = QV4::Value::fromReturnedValue(jsval->d);
        if (!v.isManaged())
            return nullptr;
        if (QV4::Value *value = managedValue(v)) {
            setValue(jsval, QV4::Encode::undefined());
            return value;
        }
        return nullptr;
    }

    static QV4::ReturnedValue asPrimitiveType(const QJSValue *jsval)
    {
        const QV4::Value v = QV4::Value::fromReturnedValue(jsval->d);
        return v.isManaged() ? QV4::Encode::undefined() : v.asReturnedValue();
    }

    // Beware: This only returns a non-null string if the QJSValue actually holds one.
    //         QV4::Strings are kept as managed values. Retrieve those with getValue().
    static const QString *asQString(const QJSValue *jsval)
    {
        const QV4::Value v = QV4::Value::fromReturnedValue(jsval->d);
        return v.isManaged() ? qstring(v) : nullptr;
    }

    static QV4::ReturnedValue asReturnedValue(const QJSValue *jsval)
    {
        const QV4::Value v = QV4::Value::fromReturnedValue(jsval->d);
        if (!v.isManaged())
            return v.asReturnedValue();

        if (const QV4::Value *value = managedValue(v))
            return value->asReturnedValue();

        return QV4::Encode::undefined();
    }

    static void setString(QJSValue *jsval, QString s)
    {
        jsval->d = encode(std::move(s));
    }

    static void setValue(QJSValue *jsval, const QV4::Value &v)
    {
        jsval->d = v.isManaged() ? encode(v) : v.asReturnedValue();
    }

    static void adoptValue(QJSValue *jsval, QV4::Value *v)
    {
        jsval->d = v->isManaged() ? encodeRawValue(quintptr(v)) : v->asReturnedValue();
    }

    // Moves any QString onto the V4 heap, changing the value to reflect that.
    static void manageStringOnV4Heap(QV4::ExecutionEngine *e, QJSValue *jsval)
    {
        if (const QString *string = asQString(jsval)) {
            jsval->d = encode(e->newString(*string)->asReturnedValue());
            delete string;
        }
    }

    // Converts any QString on the fly, involving an allocation.
    // Does not change the value.
    static QV4::ReturnedValue convertToReturnedValue(QV4::ExecutionEngine *e,
                                                     const QJSValue &jsval)
    {
        if (const QString *string = asQString(&jsval))
            return e->newString(*string)->asReturnedValue();
        if (const QV4::Value *val = asManagedType<QV4::Managed>(&jsval)) {
            if (QV4::PersistentValueStorage::getEngine(val) == e)
                return val->asReturnedValue();

            qWarning("JSValue can't be reassigned to another engine.");
            return QV4::Encode::undefined();
        }
        return jsval.d;
    }

    static QV4::ExecutionEngine *engine(const QJSValue *jsval)
    {
        const QV4::Value v = QV4::Value::fromReturnedValue(jsval->d);
        if (!v.isManaged())
            return nullptr;

        if (const QV4::Value *m = managedValue(v))
            return QV4::PersistentValueStorage::getEngine(m);

        return nullptr;
    }

    static bool checkEngine(QV4::ExecutionEngine *e, const QJSValue &jsval)
    {
        QV4::ExecutionEngine *v4 = engine(&jsval);
        return !v4 || v4 == e;
    }

    static void free(QJSValue *jsval)
    {
        QV4::Value v = QV4::Value::fromReturnedValue(jsval->d);
        if (!v.isManaged())
            return;

        if (const QString *m = qstring(v)) {
            delete m;
            return;
        }

        // We need a mutable value for free(). It needs to write to the actual memory.
        Q_ASSERT(!(quintptr(v.m()) & IsString));
        QV4::Value *m = reinterpret_cast<QV4::Value *>(v.m());
        Q_ASSERT(m); // Otherwise it would have been undefined, that is !v.isManaged() above.
        if (QV4::ExecutionEngine *e = QV4::PersistentValueStorage::getEngine(m)) {
            if (QJSEngine *jsEngine = e->jsEngine()) {
                if (jsEngine->thread() != QThread::currentThread()) {
                    QMetaObject::invokeMethod(
                            jsEngine, [m](){ QV4::PersistentValueStorage::free(m); });
                    return;
                }
            }
        }
        QV4::PersistentValueStorage::free(m);
    }
};

QT_END_NAMESPACE

#endif
