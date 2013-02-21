/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

#ifndef QJSVALUE_IMPL_P_H
#define QJSVALUE_IMPL_P_H

#include "qjsconverter_p.h"
#include "qjsvalue_p.h"
#include "qv8engine_p.h"
#include "qscriptisolate_p.h"

QT_BEGIN_NAMESPACE

QJSValuePrivate* QJSValuePrivate::get(const QJSValue& q) { Q_ASSERT(q.d_ptr.data()); return q.d_ptr.data(); }

QJSValue QJSValuePrivate::get(const QJSValuePrivate* d)
{
    Q_ASSERT(d);
    return QJSValue(const_cast<QJSValuePrivate*>(d));
}

QJSValue QJSValuePrivate::get(QScriptPassPointer<QJSValuePrivate> d)
{
    Q_ASSERT(d);
    return QJSValue(d);
}

QJSValue QJSValuePrivate::get(QJSValuePrivate* d)
{
    Q_ASSERT(d);
    return QJSValue(d);
}

QJSValuePrivate::QJSValuePrivate(bool value)
    : m_engine(0), m_state(CBool), u(value)
{
}

QJSValuePrivate::QJSValuePrivate(int value)
    : m_engine(0), m_state(CNumber), u(value)
{
}

QJSValuePrivate::QJSValuePrivate(uint value)
    : m_engine(0), m_state(CNumber), u(value)
{
}

QJSValuePrivate::QJSValuePrivate(double value)
    : m_engine(0), m_state(CNumber), u(value)
{
}

QJSValuePrivate::QJSValuePrivate(const QString& value)
    : m_engine(0), m_state(CString), u(new QString(value))
{
}

QJSValuePrivate::QJSValuePrivate(QJSValue::SpecialValue value)
    : m_engine(0), m_state(value == QJSValue::NullValue ? CNull : CUndefined)
{
}

QJSValuePrivate::QJSValuePrivate(QV8Engine* engine, bool value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    m_value = v8::Persistent<v8::Value>::New(m_engine->makeJSValue(value));
    m_engine->registerValue(this);
}

QJSValuePrivate::QJSValuePrivate(QV8Engine* engine, int value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    m_value = v8::Persistent<v8::Value>::New(m_engine->makeJSValue(value));
    m_engine->registerValue(this);
}

QJSValuePrivate::QJSValuePrivate(QV8Engine* engine, uint value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    m_value = v8::Persistent<v8::Value>::New(m_engine->makeJSValue(value));
    m_engine->registerValue(this);
}

QJSValuePrivate::QJSValuePrivate(QV8Engine* engine, double value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    m_value = v8::Persistent<v8::Value>::New(m_engine->makeJSValue(value));
    m_engine->registerValue(this);
}

QJSValuePrivate::QJSValuePrivate(QV8Engine* engine, const QString& value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    m_value = v8::Persistent<v8::Value>::New(m_engine->makeJSValue(value));
    m_engine->registerValue(this);
}

QJSValuePrivate::QJSValuePrivate(QV8Engine* engine, QJSValue::SpecialValue value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    m_value = v8::Persistent<v8::Value>::New(m_engine->makeJSValue(value));
    m_engine->registerValue(this);
}

QJSValuePrivate::QJSValuePrivate(QV8Engine *engine, v8::Handle<v8::Value> value)
    : m_engine(engine), m_state(JSValue), m_value(v8::Persistent<v8::Value>::New(value))
{
    Q_ASSERT(engine);
    // It shouldn't happen, v8 shows errors by returning an empty handler. This is important debug
    // information and it can't be simply ignored.
    Q_ASSERT(!value.IsEmpty());
    m_engine->registerValue(this);
}

QJSValuePrivate::~QJSValuePrivate()
{
    if (isJSBased()) {
        m_engine->unregisterValue(this);
        QScriptIsolate api(m_engine);
        m_value.Dispose();
    } else if (isStringBased()) {
        delete u.m_string;
    }
}

bool QJSValuePrivate::toBool() const
{
    switch (m_state) {
    case JSValue:
        {
            v8::HandleScope scope;
            return m_value->ToBoolean()->Value();
        }
    case CNumber:
        return !(qIsNaN(u.m_number) || !u.m_number);
    case CBool:
        return u.m_bool;
    case CNull:
    case CUndefined:
        return false;
    case CString:
        return u.m_string->length();
    }

    Q_ASSERT_X(false, "toBool()", "Not all states are included in the previous switch statement.");
    return false; // Avoid compiler warning.
}

double QJSValuePrivate::toNumber() const
{
    switch (m_state) {
    case JSValue:
    {
        v8::HandleScope scope;
        return m_value->ToNumber()->Value();
    }
    case CNumber:
        return u.m_number;
    case CBool:
        return u.m_bool ? 1 : 0;
    case CNull:
    case CUndefined:
        return qQNaN();
    case CString:
        bool ok;
        double result = u.m_string->toDouble(&ok);
        if (ok)
            return result;
        result = u.m_string->toInt(&ok, 0); // Try other bases.
        if (ok)
            return result;
        if (*u.m_string == QLatin1String("Infinity"))
            return qInf();
        if (*u.m_string == QLatin1String("-Infinity"))
            return -qInf();
        return u.m_string->length() ? qQNaN() : 0;
    }

    Q_ASSERT_X(false, "toNumber()", "Not all states are included in the previous switch statement.");
    return 0; // Avoid compiler warning.
}

QString QJSValuePrivate::toString() const
{
    switch (m_state) {
    case CBool:
        return u.m_bool ? QString::fromLatin1("true") : QString::fromLatin1("false");
    case CString:
        return *u.m_string;
    case CNumber:
        return QJSConverter::toString(u.m_number);
    case CNull:
        return QString::fromLatin1("null");
    case CUndefined:
        return QString::fromLatin1("undefined");
    case JSValue:
        Q_ASSERT(!m_value.IsEmpty());
        v8::HandleScope handleScope;
        v8::TryCatch tryCatch;
        v8::Local<v8::String> result = m_value->ToString();
        if (result.IsEmpty())
            result = tryCatch.Exception()->ToString();
        return QJSConverter::toString(result);
    }

    Q_ASSERT_X(false, "toString()", "Not all states are included in the previous switch statement.");
    return QString(); // Avoid compiler warning.
}

QVariant QJSValuePrivate::toVariant() const
{
    switch (m_state) {
        case CBool:
            return QVariant(u.m_bool);
        case CString:
            return QVariant(*u.m_string);
        case CNumber:
            return QVariant(u.m_number);
        case CNull:
            return QVariant(QMetaType::VoidStar, 0);
        case CUndefined:
            return QVariant();
        case JSValue:
            break;
    }

    Q_ASSERT(m_state == JSValue);
    Q_ASSERT(!m_value.IsEmpty());
    Q_ASSERT(m_engine);

    v8::HandleScope handleScope;
    return m_engine->variantFromJS(m_value);
}

inline QDateTime QJSValuePrivate::toDataTime() const
{
    if (!isDate())
        return QDateTime();

    v8::HandleScope handleScope;
    return QJSConverter::toDateTime(v8::Handle<v8::Date>::Cast(m_value));

}

QObject* QJSValuePrivate::toQObject() const
{
    if (!isJSBased())
        return 0;

    v8::HandleScope handleScope;
    return engine()->qtObjectFromJS(m_value);
}

double QJSValuePrivate::toInteger() const
{
    double result = toNumber();
    if (qIsNaN(result))
        return 0;
    if (qIsInf(result))
        return result;

    // Must use floor explicitly rather than qFloor here. On some
    // platforms qFloor will cast the value to a single precision float and use
    // floorf() which results in test failures.
    return (result > 0) ? floor(result) : -1 * floor(-result);
}

qint32 QJSValuePrivate::toInt32() const
{
    double result = toInteger();
    // Orginaly it should look like that (result == 0 || qIsInf(result) || qIsNaN(result)), but
    // some of these operation are invoked in toInteger subcall.
    if (qIsInf(result))
        return 0;
    return result;
}

quint32 QJSValuePrivate::toUInt32() const
{
    double result = toInteger();
    // Orginaly it should look like that (result == 0 || qIsInf(result) || qIsNaN(result)), but
    // some of these operation are invoked in toInteger subcall.
    if (qIsInf(result))
        return 0;

    // The explicit casts are required to avoid undefined behaviour. For example, casting
    // a negative double directly to an unsigned int on ARM NEON FPU results in the value
    // being set to zero. Casting to a signed int first ensures well defined behaviour.
    return (quint32) (qint32) result;
}

quint16 QJSValuePrivate::toUInt16() const
{
    return toInt32();
}

inline bool QJSValuePrivate::isArray() const
{
    return isJSBased() && m_value->IsArray();
}

inline bool QJSValuePrivate::isBool() const
{
    return m_state == CBool || (isJSBased() && m_value->IsBoolean());
}

inline bool QJSValuePrivate::isCallable() const
{
    if (isFunction())
        return true;
    if (isObject()) {
        // Our C++ wrappers register function handlers but not always act as callables.
        return v8::Object::Cast(*m_value)->IsCallable();
    }
    return false;
}

inline bool QJSValuePrivate::isError() const
{
    if (!isJSBased())
        return false;
    v8::HandleScope handleScope;
    return m_value->IsError();
}

inline bool QJSValuePrivate::isFunction() const
{
    return isJSBased() && m_value->IsFunction();
}

inline bool QJSValuePrivate::isNull() const
{
    return m_state == CNull || (isJSBased() && m_value->IsNull());
}

inline bool QJSValuePrivate::isNumber() const
{
    return m_state == CNumber || (isJSBased() && m_value->IsNumber());
}

inline bool QJSValuePrivate::isObject() const
{
    return isJSBased() && m_value->IsObject();
}

inline bool QJSValuePrivate::isString() const
{
    return m_state == CString || (isJSBased() && m_value->IsString());
}

inline bool QJSValuePrivate::isUndefined() const
{
    return m_state == CUndefined || (isJSBased() && m_value->IsUndefined());
}

inline bool QJSValuePrivate::isVariant() const
{
    return isJSBased() && m_engine->isVariant(m_value);
}

bool QJSValuePrivate::isDate() const
{
    return (isJSBased() && m_value->IsDate());
}

bool QJSValuePrivate::isRegExp() const
{
    return (isJSBased() && m_value->IsRegExp());
}

bool QJSValuePrivate::isQObject() const
{
    return isJSBased() && engine()->isQObject(m_value);
}

inline bool QJSValuePrivate::equals(QJSValuePrivate* other)
{
    if (!isJSBased() && !other->isJSBased()) {
        switch (m_state) {
        case CNull:
        case CUndefined:
            return other->isUndefined() || other->isNull();
        case CNumber:
            switch (other->m_state) {
            case CBool:
            case CString:
                return u.m_number == other->toNumber();
            case CNumber:
                return u.m_number == other->u.m_number;
            default:
                return false;
            }
        case CBool:
            switch (other->m_state) {
            case CBool:
                return u.m_bool == other->u.m_bool;
            case CNumber:
                return toNumber() == other->u.m_number;
            case CString:
                return toNumber() == other->toNumber();
            default:
                return false;
            }
        case CString:
            switch (other->m_state) {
            case CBool:
                return toNumber() == other->toNumber();
            case CNumber:
                return toNumber() == other->u.m_number;
            case CString:
                return *u.m_string == *other->u.m_string;
            default:
                return false;
            }
        default:
            Q_ASSERT_X(false, "QJSValue::equals", "Not all states are included in the previous switch statement.");
        }
    }

    v8::HandleScope handleScope;
    if (isJSBased() && !other->isJSBased()) {
        if (!other->assignEngine(engine())) {
            qWarning("QJSValue::equals: cannot compare to a value created in a different engine");
            return false;
        }
    } else if (!isJSBased() && other->isJSBased()) {
        if (!assignEngine(other->engine())) {
            qWarning("QJSValue::equals: cannot compare to a value created in a different engine");
            return false;
        }
    }

    Q_ASSERT(this->engine() && other->engine());
    if (this->engine() != other->engine()) {
        qWarning("QJSValue::equals: cannot compare to a value created in a different engine");
        return false;
    }
    return m_value->Equals(other->m_value);
}

inline bool QJSValuePrivate::strictlyEquals(QJSValuePrivate* other)
{
    if (isJSBased()) {
        // We can't compare these two values without binding to the same engine.
        if (!other->isJSBased()) {
            if (other->assignEngine(engine()))
                return m_value->StrictEquals(other->m_value);
            return false;
        }
        if (other->engine() != engine()) {
            qWarning("QJSValue::strictlyEquals: cannot compare to a value created in a different engine");
            return false;
        }
        return m_value->StrictEquals(other->m_value);
    }
    if (isStringBased()) {
        if (other->isStringBased())
            return *u.m_string == *(other->u.m_string);
        if (other->isJSBased()) {
            assignEngine(other->engine());
            return m_value->StrictEquals(other->m_value);
        }
    }
    if (isNumberBased()) {
        if (other->isJSBased()) {
            assignEngine(other->engine());
            return m_value->StrictEquals(other->m_value);
        }
        if (m_state != other->m_state)
            return false;
        if (m_state == CNumber)
            return u.m_number == other->u.m_number;
        Q_ASSERT(m_state == CBool);
        return u.m_bool == other->u.m_bool;
    }

    return (isUndefined() && other->isUndefined())
            || (isNull() && other->isNull());
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::prototype() const
{
    if (isObject()) {
        v8::HandleScope handleScope;
        return new QJSValuePrivate(engine(), v8::Handle<v8::Object>::Cast(m_value)->GetPrototype());
    }
    return new QJSValuePrivate();
}

inline void QJSValuePrivate::setPrototype(QJSValuePrivate* prototype)
{
    if (isObject() && (prototype->isObject() || prototype->isNull())) {
        if (engine() != prototype->engine()) {
            if (prototype->engine()) {
                qWarning("QJSValue::setPrototype() failed: cannot set a prototype created in a different engine");
                return;
            }
            prototype->assignEngine(engine());
        }
        v8::HandleScope handleScope;
        if (!v8::Handle<v8::Object>::Cast(m_value)->SetPrototype(*prototype))
            qWarning("QJSValue::setPrototype() failed: cyclic prototype value");
    }
}

inline void QJSValuePrivate::setProperty(const QString& name, QJSValuePrivate* value, uint attribs)
{
    if (!isObject())
        return;
    v8::HandleScope handleScope;
    setProperty(QJSConverter::toString(name), value, attribs);
}

inline void QJSValuePrivate::setProperty(v8::Handle<v8::String> name, QJSValuePrivate* value, uint attribs)
{
    if (!isObject())
        return;

    if (!value->isJSBased())
        value->assignEngine(engine());

    if (engine() != value->engine()) {
        qWarning("QJSValue::setProperty(%s) failed: "
                 "cannot set value created in a different engine",
                 qPrintable(QJSConverter::toString(name)));
        return;
    }

    v8::TryCatch tryCatch;
//    if (attribs & (QJSValue::PropertyGetter | QJSValue::PropertySetter)) {
//        engine()->originalGlobalObject()->defineGetterOrSetter(*this, name, value->m_value, attribs);
//    } else {
        v8::Object::Cast(*m_value)->Set(name, value->m_value, v8::PropertyAttribute(attribs & QJSConverter::PropertyAttributeMask));
//    }
}

inline void QJSValuePrivate::setProperty(quint32 index, QJSValuePrivate* value, uint attribs)
{
    // FIXME this method should by integrated with other overloads to use the same code patch.
    // for now it is not possible as v8 doesn't allow to set property attributes using index based api.

    if (!isObject())
        return;

    if (attribs) {
        // FIXME we don't need to convert index to a string.
        //Object::Set(int,value) do not take attributes.
        setProperty(QString::number(index), value, attribs);
        return;
    }

    if (!value->isJSBased())
        value->assignEngine(engine());

    if (engine() != value->engine()) {
        qWarning("QJSValue::setProperty() failed: cannot set value created in a different engine");
        return;
    }

    v8::HandleScope handleScope;
    v8::Object::Cast(*m_value)->Set(index, value->m_value);
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::property(const QString& name) const
{
    if (!isObject())
        return new QJSValuePrivate();
    if (!name.length())
        return new QJSValuePrivate(engine());

    v8::HandleScope handleScope;
    return property(QJSConverter::toString(name));
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::property(v8::Handle<v8::String> name) const
{
    Q_ASSERT(!name.IsEmpty());
    if (!isObject())
        return new QJSValuePrivate();
    return property<>(name);
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::property(quint32 index) const
{
    if (!isObject())
        return new QJSValuePrivate();
    return property<>(index);
}

template<typename T>
inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::property(T name) const
{
    Q_ASSERT(isObject());
    v8::HandleScope handleScope;
    v8::Handle<v8::Object> self(v8::Object::Cast(*m_value));

    v8::TryCatch tryCatch;
    v8::Handle<v8::Value> result = self->Get(name);
    if (tryCatch.HasCaught())
        result = tryCatch.Exception();
    if (result.IsEmpty())
        return new QJSValuePrivate(engine());
    return new QJSValuePrivate(engine(), result);
}

inline bool QJSValuePrivate::deleteProperty(const QString& name)
{
    if (!isObject())
        return false;

    v8::HandleScope handleScope;
    v8::Handle<v8::Object> self(v8::Handle<v8::Object>::Cast(m_value));
    return self->Delete(QJSConverter::toString(name));
}

inline bool QJSValuePrivate::hasProperty(const QString &name) const
{
    if (!isObject())
        return false;

    v8::HandleScope handleScope;
    v8::Handle<v8::Object> self(v8::Handle<v8::Object>::Cast(m_value));
    return self->Has(QJSConverter::toString(name));
}

inline bool QJSValuePrivate::hasOwnProperty(const QString &name) const
{
    if (!isObject())
        return false;

    v8::HandleScope handleScope;
    v8::Handle<v8::Object> self(v8::Handle<v8::Object>::Cast(m_value));
    return self->HasOwnProperty(QJSConverter::toString(name));
}

inline QJSValuePrivate::PropertyFlags QJSValuePrivate::propertyFlags(const QString& name) const
{
    if (!isObject())
        return QJSValuePrivate::PropertyFlags(0);

    v8::HandleScope handleScope;
    return engine()->getPropertyFlags(v8::Handle<v8::Object>::Cast(m_value), QJSConverter::toString(name));
}

inline QJSValuePrivate::PropertyFlags QJSValuePrivate::propertyFlags(v8::Handle<v8::String> name) const
{
    if (!isObject())
        return QJSValuePrivate::PropertyFlags(0);

    v8::HandleScope handleScope;
    return engine()->getPropertyFlags(v8::Handle<v8::Object>::Cast(m_value), name);
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::call(QJSValuePrivate* thisObject, const QJSValueList& args)
{
    if (!isCallable())
        return new QJSValuePrivate();

    v8::HandleScope handleScope;

    // Convert all arguments and bind to the engine.
    int argc = args.size();
    QVarLengthArray<v8::Handle<v8::Value>, 8> argv(argc);
    if (!prepareArgumentsForCall(argv.data(), args)) {
        qWarning("QJSValue::call() failed: cannot call function with argument created in a different engine");
        return new QJSValuePrivate(engine());
    }

    return call(thisObject, argc, argv.data());
}

QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::call(QJSValuePrivate* thisObject, int argc, v8::Handle<v8::Value> *argv)
{
    QV8Engine *e = engine();

    v8::Handle<v8::Object> recv;

    if (!thisObject || !thisObject->isObject()) {
        recv = v8::Handle<v8::Object>(v8::Object::Cast(*e->global()));
    } else {
        if (!thisObject->assignEngine(e)) {
            qWarning("QJSValue::call() failed: cannot call function with thisObject created in a different engine");
            return new QJSValuePrivate(engine());
        }

        recv = v8::Handle<v8::Object>(v8::Object::Cast(*thisObject->m_value));
    }

    if (argc < 0) {
        v8::Local<v8::Value> exeption = v8::Exception::TypeError(v8::String::New("Arguments must be an array"));
        return new QJSValuePrivate(e, exeption);
    }

    v8::TryCatch tryCatch;
    v8::Handle<v8::Value> result = v8::Object::Cast(*m_value)->CallAsFunction(recv, argc, argv);

    if (result.IsEmpty()) {
        result = tryCatch.Exception();
        // TODO: figure out why v8 doesn't always produce an exception value.
        //Q_ASSERT(!result.IsEmpty());
        if (result.IsEmpty())
            result = v8::Exception::Error(v8::String::New("missing exception value"));
    }

    return new QJSValuePrivate(e, result);
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::callAsConstructor(int argc, v8::Handle<v8::Value> *argv)
{
    QV8Engine *e = engine();

    if (argc < 0) {
        v8::Local<v8::Value> exeption = v8::Exception::TypeError(v8::String::New("Arguments must be an array"));
        return new QJSValuePrivate(e, exeption);
    }

    v8::TryCatch tryCatch;
    v8::Handle<v8::Value> result = v8::Object::Cast(*m_value)->CallAsConstructor(argc, argv);

    if (result.IsEmpty())
        result = tryCatch.Exception();

    return new QJSValuePrivate(e, result);
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::callAsConstructor(const QJSValueList& args)
{
    if (!isCallable())
        return new QJSValuePrivate();

    v8::HandleScope handleScope;

    // Convert all arguments and bind to the engine.
    int argc = args.size();
    QVarLengthArray<v8::Handle<v8::Value>, 8> argv(argc);
    if (!prepareArgumentsForCall(argv.data(), args)) {
        qWarning("QJSValue::callAsConstructor() failed: cannot construct function with argument created in a different engine");
        return new QJSValuePrivate(engine());
    }

    return callAsConstructor(argc, argv.data());
}

/*! \internal
 * Make sure this value is associated with a v8 value belonging to this engine.
 * If the value belongs to another engine, returns false.
 */
bool QJSValuePrivate::assignEngine(QV8Engine* engine)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    switch (m_state) {
    case CBool:
        m_value = v8::Persistent<v8::Value>::New(engine->makeJSValue(u.m_bool));
        break;
    case CString:
        m_value = v8::Persistent<v8::Value>::New(engine->makeJSValue(*u.m_string));
        delete u.m_string;
        break;
    case CNumber:
        m_value = v8::Persistent<v8::Value>::New(engine->makeJSValue(u.m_number));
        break;
    case CNull:
        m_value = v8::Persistent<v8::Value>::New(engine->makeJSValue(QJSValue::NullValue));
        break;
    case CUndefined:
        m_value = v8::Persistent<v8::Value>::New(engine->makeJSValue(QJSValue::UndefinedValue));
        break;
    default:
        if (this->engine() == engine)
            return true;
        else if (!isJSBased())
            Q_ASSERT_X(!isJSBased(), "assignEngine()", "Not all states are included in the previous switch statement.");
        else
            qWarning("JSValue can't be rassigned to an another engine.");
        return false;
    }
    m_engine = engine;
    m_state = JSValue;

    m_engine->registerValue(this);
    return true;
}

/*!
  \internal
  Invalidates this value (makes it undefined).

  Does not remove the value from the engine's list of
  registered values; that's the responsibility of the caller.
*/
void QJSValuePrivate::invalidate()
{
    if (isJSBased()) {
        m_value.Dispose();
        m_value.Clear();
    } else if (isStringBased()) {
        delete u.m_string;
    }
    m_engine = 0;
    m_state = CUndefined;
}

QV8Engine* QJSValuePrivate::engine() const
{
    return m_engine;
}

inline QJSValuePrivate::operator v8::Handle<v8::Value>() const
{
    Q_ASSERT(isJSBased());
    return m_value;
}

inline QJSValuePrivate::operator v8::Handle<v8::Object>() const
{
    Q_ASSERT(isObject());
    return v8::Handle<v8::Object>::Cast(m_value);
}

inline v8::Handle<v8::Value> QJSValuePrivate::handle() const
{
    return m_value;
}

/*!
 * Return a v8::Handle, assign to the engine if needed.
 */
v8::Handle<v8::Value> QJSValuePrivate::asV8Value(QV8Engine* engine)
{
    if (!m_engine) {
        if (!assignEngine(engine))
            return v8::Handle<v8::Value>();
    }
    Q_ASSERT(isJSBased());
    return m_value;
}

/*!
  \internal
  Returns true if QSV have an engine associated.
*/
bool QJSValuePrivate::isJSBased() const
{
#ifndef QT_NO_DEBUG
    // internals check.
    if (m_state >= JSValue)
        Q_ASSERT(!m_value.IsEmpty());
    else
        Q_ASSERT(m_value.IsEmpty());
#endif
    return m_state >= JSValue;
}

/*!
  \internal
  Returns true if current value of QSV is placed in m_number.
*/
bool QJSValuePrivate::isNumberBased() const { return m_state == CNumber || m_state == CBool; }

/*!
  \internal
  Returns true if current value of QSV is placed in m_string.
*/
bool QJSValuePrivate::isStringBased() const { return m_state == CString; }

/*!
  \internal
  Converts arguments and bind them to the engine.
  \attention argv should be big enough
*/
inline bool QJSValuePrivate::prepareArgumentsForCall(v8::Handle<v8::Value> argv[], const QJSValueList& args) const
{
    QJSValueList::const_iterator i = args.constBegin();
    for (int j = 0; i != args.constEnd(); j++, i++) {
        QJSValuePrivate* value = QJSValuePrivate::get(*i);
        if ((value->isJSBased() && engine() != value->engine())
                || (!value->isJSBased() && !value->assignEngine(engine())))
            // Different engines are not allowed!
            return false;
        argv[j] = *value;
    }
    return true;
}

QT_END_NAMESPACE

#endif
