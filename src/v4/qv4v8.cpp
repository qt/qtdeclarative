/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
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

#include "qv4v8.h"

#include "qmljs_engine.h"
#include "qmljs_runtime.h"
#include "qv4mm.h"
#include "qv4managed.h"
#include "qv4functionobject.h"
#include "qmljs_value.h"
#include "qv4isel_masm_p.h"
#include "qv4globalobject.h"
#include "qv4regexpobject.h"
#include "qv4dateobject.h"
#include "qv4numberobject.h"
#include "qv4booleanobject.h"
#include "qv4stringobject.h"
#include "qv4objectproto.h"
#include <QThreadStorage>

using namespace QQmlJS;
using namespace QQmlJS::VM;

namespace v8 {

#define currentEngine() Isolate::GetCurrent()->GetCurrentContext()->GetEngine()

#define Q_D(obj) QQmlJS::VM::Value *d = reinterpret_cast<QQmlJS::VM::Value*>(obj)

#define ValuePtr(obj) reinterpret_cast<QQmlJS::VM::Value*>(obj)
#define ConstValuePtr(obj) reinterpret_cast<const QQmlJS::VM::Value*>(obj)

void gcProtect(void *handle)
{
    Q_D(handle);
    if (VM::Managed *m = d->asManaged())
        currentEngine()->memoryManager->protect(m);
}

void gcUnprotect(void *handle)
{
    Q_D(handle);
    if (VM::Managed *m = d->asManaged())
        currentEngine()->memoryManager->unprotect(m);
}


ScriptOrigin::ScriptOrigin(Handle<Value> resource_name, Handle<Integer> resource_line_offset, Handle<Integer> resource_column_offset)
{
    m_fileName = resource_name->ToString()->asQString();
    m_lineNumber = resource_line_offset->ToInt32()->Value();
    m_columnNumber = resource_column_offset->ToInt32()->Value();
}

Handle<Value> ScriptOrigin::ResourceName() const
{
    return Value::fromVmValue(VM::Value::fromString(currentEngine()->current, m_fileName));
}

Handle<Integer> ScriptOrigin::ResourceLineOffset() const
{
    return Integer::New(m_lineNumber);
}

Handle<Integer> ScriptOrigin::ResourceColumnOffset() const
{
    return Integer::New(m_columnNumber);
}


Local<Script> Script::New(Handle<String> source,
                         ScriptOrigin* origin,
                         ScriptData* pre_data,
                         Handle<String> script_data,
                         CompileFlags flags)
{
    Script *s = new Script;
    s->m_script = source->ToString()->asQString();
    if (origin)
        s->m_origin = *origin;
    s->m_flags = flags;
    s->m_context = Handle<Context>();
    return Local<Script>::New(Handle<Script>(s));
}


Local<Script> Script::New(Handle<String> source,
                         Handle<Value> file_name,
                         CompileFlags flags)
{
    ScriptOrigin origin(file_name);
    return New(source, &origin, 0, Handle<String>(), flags);
}

Local<Script> Script::Compile(Handle<String> source, ScriptOrigin *origin, ScriptData *pre_data, Handle<String> script_data, Script::CompileFlags flags)
{
    Script *s = new Script;
    s->m_script = source->ToString()->asQString();
    if (origin)
        s->m_origin = *origin;
    s->m_flags = flags;
    s->m_context = Context::GetCurrent();
    return Local<Script>::New(Handle<Script>(s));
}

Local<Script> Script::Compile(Handle<String> source,
                             Handle<Value> file_name,
                             Handle<String> script_data,
                             CompileFlags flags)
{
    ScriptOrigin origin(file_name);
    return Compile(source, &origin, 0, script_data, flags);
}

Local<Value> Script::Run()
{
    Handle<Context> context = m_context;
    if (!context.IsEmpty())
        context = Context::GetCurrent();
    QQmlJS::VM::Function *f = QQmlJS::VM::EvalFunction::parseSource(context->GetEngine()->rootContext, m_origin.m_fileName, m_script, QQmlJS::Codegen::EvalCode,
                                                                    /*strictMode =*/ false, /*inheritContext =*/ false);
    if (!f)
        return Local<Value>();

    QQmlJS::VM::Value result = context->GetEngine()->run(f);
    return Local<Value>::New(Value::fromVmValue(result));
}

Local<Value> Script::Run(Handle<Object> qml)
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Value> Script::Id()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

void Script::SetData(Handle<String> data)
{
    Q_UNIMPLEMENTED();
}


Local<String> Message::Get() const
{
    return Local<String>::New(Value::fromVmValue(VM::Value::fromString(currentEngine()->current, m_message)));
}

Handle<Value> Message::GetScriptResourceName() const
{
    return Value::fromVmValue(VM::Value::fromString(currentEngine()->current, m_resourceName));
}

int Message::GetLineNumber() const
{
    return m_lineNumber;
}


Local<StackFrame> StackTrace::GetFrame(uint32_t index) const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

int StackTrace::GetFrameCount() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Array> StackTrace::AsArray()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<StackTrace> StackTrace::CurrentStackTrace(int frame_limit, StackTrace::StackTraceOptions options)
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}


int StackFrame::GetLineNumber() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

int StackFrame::GetColumn() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<String> StackFrame::GetScriptName() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<String> StackFrame::GetScriptNameOrSourceURL() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<String> StackFrame::GetFunctionName() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}


bool Value::IsUndefined() const
{
    return ConstValuePtr(this)->isUndefined();
}

bool Value::IsNull() const {
    return ConstValuePtr(this)->isNull();
}

bool Value::IsTrue() const
{
    return ConstValuePtr(this)->isBoolean() && ConstValuePtr(this)->booleanValue();
}

bool Value::IsFalse() const
{
    return !IsTrue();
}

bool Value::IsString() const
{
    return ConstValuePtr(this)->isString();
}

bool Value::IsFunction() const
{
    return ConstValuePtr(this)->asFunctionObject();
}

bool Value::IsArray() const
{
    return ConstValuePtr(this)->asArrayObject();
}

bool Value::IsObject() const
{
    return ConstValuePtr(this)->isObject();
}

bool Value::IsBoolean() const
{
    return ConstValuePtr(this)->isBoolean();
}

bool Value::IsNumber() const
{
    return ConstValuePtr(this)->isNumber();
}

bool Value::IsExternal() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

bool Value::IsInt32() const
{
    return ConstValuePtr(this)->isInteger();
}

bool Value::IsUint32() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

bool Value::IsDate() const
{
    return ConstValuePtr(this)->asDateObject();
}

bool Value::IsBooleanObject() const
{
    return ConstValuePtr(this)->asBooleanObject();
}

bool Value::IsNumberObject() const
{
    return ConstValuePtr(this)->asNumberObject();
}

bool Value::IsStringObject() const
{
    return ConstValuePtr(this)->asStringObject();
}

bool Value::IsRegExp() const
{
    return ConstValuePtr(this)->asRegExpObject();
}

bool Value::IsError() const
{
    return ConstValuePtr(this)->asErrorObject();
}

Local<Boolean> Value::ToBoolean() const
{
    return Local<Boolean>::New(Value::fromVmValue(VM::Value::fromBoolean(ConstValuePtr(this)->toBoolean())));
}

Local<Number> Value::ToNumber() const
{
    return Local<Number>::New(Value::fromVmValue(VM::Value::fromDouble(ConstValuePtr(this)->toNumber(currentEngine()->current))));
}

Local<String> Value::ToString() const
{
    return Local<String>::New(Value::fromVmValue(VM::Value::fromString(ConstValuePtr(this)->toString(currentEngine()->current))));
}

Local<Object> Value::ToObject() const
{
    return Local<Object>::New(Value::fromVmValue(QQmlJS::VM::Value::fromObject(ConstValuePtr(this)->toObject(currentEngine()->current))));
}

Local<Integer> Value::ToInteger() const
{
    return Local<Integer>::New(Value::fromVmValue(QQmlJS::VM::Value::fromDouble(ConstValuePtr(this)->toInteger(currentEngine()->current))));
}

Local<Uint32> Value::ToUint32() const
{
    return Local<Uint32>::New(Value::fromVmValue(QQmlJS::VM::Value::fromUInt32(ConstValuePtr(this)->toUInt32(currentEngine()->current))));
}

Local<Int32> Value::ToInt32() const
{
    return Local<Int32>::New(Value::fromVmValue(QQmlJS::VM::Value::fromInt32(ConstValuePtr(this)->toInt32(currentEngine()->current))));
}

Local<Uint32> Value::ToArrayIndex() const
{
    return Local<Uint32>::New(Value::fromVmValue(QQmlJS::VM::Value::fromUInt32(ConstValuePtr(this)->asArrayIndex())));
}

bool Value::BooleanValue() const
{
    return ConstValuePtr(this)->toBoolean();
}

double Value::NumberValue() const
{
    return ConstValuePtr(this)->asDouble();
}

int64_t Value::IntegerValue() const
{
    return (int64_t)ConstValuePtr(this)->toInteger(currentEngine()->current);
}

uint32_t Value::Uint32Value() const
{
    return ConstValuePtr(this)->toUInt32(currentEngine()->current);
}

int32_t Value::Int32Value() const
{
    return ConstValuePtr(this)->toInt32(currentEngine()->current);
}

bool Value::Equals(Handle<Value> that) const
{
    return __qmljs_equal(*ConstValuePtr(this), *ConstValuePtr(&that), currentEngine()->current);
}

bool Value::StrictEquals(Handle<Value> that) const
{
    return __qmljs_strict_equal(*ConstValuePtr(this), *ConstValuePtr(&that), currentEngine()->current);
}

VM::Value Value::vmValue() const
{
    return *ConstValuePtr(this);
}

Handle<Value> Value::fromVmValue(const VM::Value &vmValue)
{
    Handle<Value> res;
    res.val = vmValue.val;
    return res;
}


bool Boolean::Value() const
{
    return BooleanValue();
}

Handle<Boolean> Boolean::New(bool value)
{
    return Value::fromVmValue(VM::Value::fromBoolean(value));
}


int String::Length() const
{
    return asVMString()->toQString().length();
}

uint32_t String::Hash() const
{
    return asVMString()->hashValue();
}


String::CompleteHashData String::CompleteHash() const
{
    VM::String *s = asVMString();
    CompleteHashData data;
    data.hash = s->hashValue();
    data.length = s->toQString().length();
    data.symbol_id = s->identifier;
    return data;
}

uint32_t String::ComputeHash(uint16_t *string, int length)
{
    // ### unefficient
    QString s = QString::fromUtf16((ushort *)string, length);
    VM::String *vmString = currentEngine()->newString(s);
    return vmString->hashValue();
}

uint32_t String::ComputeHash(char *string, int length)
{
    // ### unefficient
    QString s = QString::fromLatin1((char *)string, length);
    VM::String *vmString = currentEngine()->newString(s);
    return vmString->hashValue();
}

bool String::Equals(uint16_t *str, int length)
{
    return asQString() == QString(reinterpret_cast<QChar*>(str), length);
}

bool String::Equals(char *str, int length)
{
    return asQString() == QString::fromLatin1(str, length);
}

uint16_t String::GetCharacter(int index)
{
    return asQString().at(index).unicode();
}

int String::Write(uint16_t *buffer, int start, int length, int options) const
{
    // ### do we use options?
    memcpy(buffer + start, asQString().constData(), length*sizeof(QChar));
    Q_UNREACHABLE();
}

v8::Local<String> String::Empty()
{
    return Local<String>::New(v8::Value::fromVmValue(VM::Value::fromString(currentEngine()->current, QString())));
}

bool String::IsExternal() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

String::ExternalStringResource *String::GetExternalStringResource() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

String *String::Cast(v8::Value *obj)
{
    return static_cast<String *>(obj);
}


Local<String> String::New(const char *data, int length)
{
    QQmlJS::VM::Value v = QQmlJS::VM::Value::fromString(currentEngine()->current, QString::fromLatin1(data, length));
    return Local<String>::New(v8::Value::fromVmValue(v));
}

Local<String> String::New(const uint16_t *data, int length)
{
    QQmlJS::VM::Value v = QQmlJS::VM::Value::fromString(currentEngine()->current, QString((const QChar *)data, length));
    return Local<String>::New(v8::Value::fromVmValue(v));
}

Local<String> String::NewSymbol(const char *data, int length)
{
    QString str = QString::fromLatin1(data, length);
    VM::String *vmString = currentEngine()->newIdentifier(str);
    return Local<String>::New(v8::Value::fromVmValue(VM::Value::fromString(vmString)));
}

Local<String> String::NewExternal(String::ExternalStringResource *resource)
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

QString String::asQString() const
{
    return asVMString()->toQString();
}

VM::String *String::asVMString() const
{
    const VM::Value *v = ConstValuePtr(this);
    ASSERT(v->isString());
    return v->stringValue();
}

String::AsciiValue::AsciiValue(Handle<v8::Value> obj)
{
    str = obj->ToString()->asQString().toLatin1();
}

String::Value::Value(Handle<v8::Value> obj)
{
    str = obj->ToString()->asQString();
}


double Number::Value() const
{
    const VM::Value *v = ConstValuePtr(this);
    assert(v->isNumber());
    return v->asDouble();
}

Local<Number> Number::New(double value)
{
    return Local<Number>::New(Value::fromVmValue(VM::Value::fromDouble(value)));
}

Number *Number::Cast(v8::Value *obj)
{
    return static_cast<Number *>(obj);
}

Local<Integer> Integer::New(int32_t value)
{
    return Local<Integer>::New(Value::fromVmValue(VM::Value::fromInt32(value)));
}

Local<Integer> Integer::NewFromUnsigned(uint32_t value)
{
    return Local<Integer>::New(Value::fromVmValue(VM::Value::fromUInt32(value)));
}

Local<Integer> Integer::New(int32_t value, Isolate *)
{
    return New(value);
}

Local<Integer> Integer::NewFromUnsigned(uint32_t value, Isolate *)
{
    return NewFromUnsigned(value);
}

int64_t Integer::Value() const
{
    const VM::Value *v = ConstValuePtr(this);
    assert(v->isNumber());
    return (int64_t)v->asDouble();
}

Integer *Integer::Cast(v8::Value *obj)
{
    return static_cast<Integer *>(obj);
}

int32_t Int32::Value() const
{
    const VM::Value *v = ConstValuePtr(this);
    assert(v->isInteger());
    return v->int_32;
}

uint32_t Uint32::Value() const
{
    const VM::Value *v = ConstValuePtr(this);
    assert(v->isNumber());
    return v->toUInt32(currentEngine()->current);
}


struct ExternalResourceWrapper : public QQmlJS::VM::Object::ExternalResource
{
    ExternalResourceWrapper(v8::Object::ExternalResource *wrapped)
    {
        this->wrapped = wrapped;
    }

    virtual ~ExternalResourceWrapper()
    {
        wrapped->Dispose();
    }

    v8::Object::ExternalResource *wrapped;
};


bool Object::Set(Handle<Value> key, Handle<Value> value, PropertyAttribute attribs)
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    QQmlJS::VM::ExecutionContext *ctx = currentEngine()->current;
    o->put(ctx, ValuePtr(&key)->toString(ctx), *ValuePtr(&value));
    // ### attribs
    return true;
}

bool Object::Set(uint32_t index, Handle<Value> value)
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Value> Object::Get(Handle<Value> key)
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    QQmlJS::VM::ExecutionContext *ctx = currentEngine()->current;
    QQmlJS::VM::Value prop = o->get(ctx, ValuePtr(&key)->toString(ctx));
    return Local<Value>::New(Value::fromVmValue(prop));
}

Local<Value> Object::Get(uint32_t key)
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    QQmlJS::VM::ExecutionContext *ctx = currentEngine()->current;
    QQmlJS::VM::Value prop = o->getIndexed(ctx, key);
    return Local<Value>::New(Value::fromVmValue(prop));
}

bool Object::Has(Handle<String> key)
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    return o->__hasProperty__(currentEngine()->current, ValuePtr(&key)->asString());
}

bool Object::Delete(Handle<String> key)
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    return o->deleteProperty(currentEngine()->current, ValuePtr(&key)->asString());
}

bool Object::Has(uint32_t index)
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    if (!o)
        return false;
    return o->__hasProperty__(currentEngine()->current, index);
}

bool Object::Delete(uint32_t index)
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    return o->deleteIndexedProperty(currentEngine()->current, index);
}

bool Object::SetAccessor(Handle<String> name, AccessorGetter getter, AccessorSetter setter, Handle<Value> data, AccessControl settings, PropertyAttribute attribute)
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Array> Object::GetPropertyNames()
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    assert(o);

    VM::ArrayObject *array = currentEngine()->newArrayObject(currentEngine()->current)->asArrayObject();
    ObjectIterator it(currentEngine()->current, o, ObjectIterator::WithProtoChain);
    while (1) {
        VM::Value v = it.nextPropertyNameAsString();
        if (v.isNull())
            break;
        array->push_back(v);
    }
    return Local<Array>::New(Value::fromVmValue(VM::Value::fromObject(array)));
}

Local<Array> Object::GetOwnPropertyNames()
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    VM::Value arg = VM::Value::fromObject(o);
    VM::Value result = ObjectPrototype::method_getOwnPropertyNames(currentEngine()->current, VM::Value::undefinedValue(), &arg, 1);
    return Local<Array>::New((Value::fromVmValue(result)));
}

Local<Value> Object::GetPrototype()
{
    Local<Value> result;
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    return Local<Value>::New(Value::fromVmValue(QQmlJS::VM::Value::fromObject(o->prototype)));
}

bool Object::SetPrototype(Handle<Value> prototype)
{
    QQmlJS::VM::Object *p = ConstValuePtr(*prototype)->asObject();
    if (!p)
        return false;
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    o->prototype = p;
    return true;
}

Local<Value> Object::GetInternalField(int index)
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

void Object::SetInternalField(int index, Handle<Value> value)
{
    Q_UNIMPLEMENTED();
}

void Object::SetExternalResource(Object::ExternalResource *resource)
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    if (!o)
        return;
    o->externalResource = new ExternalResourceWrapper(resource);
}

Object::ExternalResource *Object::GetExternalResource()
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    if (!o || !o->externalResource)
        return 0;
    return static_cast<ExternalResourceWrapper*>(o->externalResource)->wrapped;
}

bool Object::HasOwnProperty(Handle<String> key)
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    QQmlJS::VM::ExecutionContext *ctx = currentEngine()->current;
    return o->__getOwnProperty__(ctx, ValuePtr(&key)->toString(ctx));
}

int Object::GetIdentityHash()
{
    return (quintptr)ConstValuePtr(this)->asObject() >> 2;
}

bool Object::SetHiddenValue(Handle<String> key, Handle<Value> value)
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Value> Object::GetHiddenValue(Handle<String> key)
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Object> Object::Clone()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

bool Object::IsCallable()
{
    return ConstValuePtr(this)->asFunctionObject();
}

Local<Value> Object::CallAsFunction(Handle<Object> recv, int argc, Handle<Value> argv[])
{
    VM::FunctionObject *f = ConstValuePtr(this)->asFunctionObject();
    if (!f)
        return Local<Value>();
    VM::Value retval = f->call(currentEngine()->current, recv->vmValue(),
                               reinterpret_cast<QQmlJS::VM::Value*>(argv),
                               argc);
    return Local<Value>::New(Value::fromVmValue(retval));
}

Local<Value> Object::CallAsConstructor(int argc, Handle<Value> argv[])
{
    VM::FunctionObject *f = ConstValuePtr(this)->asFunctionObject();
    if (!f)
        return Local<Value>();
    VM::Value retval = f->construct(currentEngine()->current,
                                    reinterpret_cast<QQmlJS::VM::Value*>(argv),
                                    argc);
    return Local<Value>::New(Value::fromVmValue(retval));
}

Local<Object> Object::New()
{
    VM::Object *o = currentEngine()->newObject();
    return Local<Object>::New(Value::fromVmValue(VM::Value::fromObject(o)));
}

Object *Object::Cast(Value *obj)
{
    return static_cast<Object *>(obj);
}


uint32_t Array::Length() const
{
    VM::ArrayObject *a = ConstValuePtr(this)->asArrayObject();
    assert(a);
    return a->arrayLength();
}

Local<Array> Array::New(int length)
{
    VM::ArrayObject *a = currentEngine()->newArrayObject(currentEngine()->current);
    if (length < 0x1000)
        a->arrayReserve(length);

    return Local<Array>::New(Value::fromVmValue(VM::Value::fromObject(a)));
}

Array *Array::Cast(Value *obj)
{
    return static_cast<Array *>(obj);
}


Local<Object> Function::NewInstance() const
{
    VM::FunctionObject *f = ConstValuePtr(this)->asFunctionObject();
    assert(f);
    VM::Value retval = f->construct(currentEngine()->current, 0, 0);
    return Local<Object>::New(Value::fromVmValue(retval));
}

Local<Object> Function::NewInstance(int argc, Handle<Value> argv[]) const
{
    VM::FunctionObject *f = ConstValuePtr(this)->asFunctionObject();
    assert(f);
    VM::Value retval = f->construct(currentEngine()->current,
                                    reinterpret_cast<QQmlJS::VM::Value*>(argv),
                                    argc);
    return Local<Object>::New(Value::fromVmValue(retval));
}

Local<Value> Function::Call(Handle<Object> thisObj, int argc, Handle<Value> argv[])
{
    QQmlJS::VM::FunctionObject *f = ConstValuePtr(this)->asFunctionObject();
    if (!f)
        return Local<Value>();
    QQmlJS::VM::Value result = f->call(currentEngine()->current,
                                       *ConstValuePtr(&thisObj),
                                       reinterpret_cast<QQmlJS::VM::Value*>(argv),
                                       argc);
    return Local<Value>::New(Value::fromVmValue(result));
}

Handle<Value> Function::GetName() const
{
    QQmlJS::VM::FunctionObject *f = ConstValuePtr(this)->asFunctionObject();
    if (!f)
        return Handle<Value>();
    return Value::fromVmValue(VM::Value::fromString(f->name));
}

ScriptOrigin Function::GetScriptOrigin() const
{
    Q_UNIMPLEMENTED();
    return ScriptOrigin();
}

Function *Function::Cast(Value *obj)
{
    return static_cast<Function *>(obj);
}


Local<Value> Date::New(double time)
{
    VM::Object *o = currentEngine()->newDateObject(VM::Value::fromDouble(time));
    return Local<Value>::New(Value::fromVmValue(VM::Value::fromObject(o)));
}

double Date::NumberValue() const
{
    DateObject *d = ConstValuePtr(this)->asDateObject();
    assert(d);
    return d->value.doubleValue();
}

Date *Date::Cast(Value *obj)
{
    return static_cast<Date *>(obj);
}

void Date::DateTimeConfigurationChangeNotification()
{
    Q_UNIMPLEMENTED();
}


Local<Value> NumberObject::New(double value)
{
    VM::Object *o = currentEngine()->newNumberObject(VM::Value::fromDouble(value));
    return Local<Value>::New(Value::fromVmValue(VM::Value::fromObject(o)));
}

double NumberObject::NumberValue() const
{
    VM::NumberObject *n = ConstValuePtr(this)->asNumberObject();
    assert(n);
    return n->value.doubleValue();
}

NumberObject *NumberObject::Cast(Value *obj)
{
    return static_cast<NumberObject *>(obj);
}

Local<Value> BooleanObject::New(bool value)
{
    VM::Object *o = currentEngine()->newBooleanObject(VM::Value::fromBoolean(value));
    return Local<Value>::New(Value::fromVmValue(VM::Value::fromObject(o)));
}

bool BooleanObject::BooleanValue() const
{
    VM::BooleanObject *b = ConstValuePtr(this)->asBooleanObject();
    assert(b);
    return b->value.booleanValue();
}

BooleanObject *BooleanObject::Cast(Value *obj)
{
    return static_cast<BooleanObject *>(obj);
}

Local<Value> StringObject::New(Handle<String> value)
{
    VM::Object *o = currentEngine()->newStringObject(currentEngine()->current, VM::Value::fromString(value->vmValue().asString()));
    return Local<Value>::New(Value::fromVmValue(VM::Value::fromObject(o)));
}

Local<String> StringObject::StringValue() const
{
    VM::StringObject *s = ConstValuePtr(this)->asStringObject();
    assert(s);
    return Local<String>::New(Value::fromVmValue(s->value));
}

StringObject *StringObject::Cast(Value *obj)
{
    return static_cast<StringObject *>(obj);
}

Local<RegExp> RegExp::New(Handle<String> pattern, RegExp::Flags flags)
{
    int f = 0;
    if (flags & kGlobal)
        f |= IR::RegExp::RegExp_Global;
    if (flags & kIgnoreCase)
        f |= IR::RegExp::RegExp_IgnoreCase;
    if (flags & kMultiline)
        f |= IR::RegExp::RegExp_Multiline;
    VM::Object *o = currentEngine()->newRegExpObject(pattern->asQString(), f);
    return Local<RegExp>::New(Value::fromVmValue(VM::Value::fromObject(o)));
}

Local<String> RegExp::GetSource() const
{
    RegExpObject *re = ConstValuePtr(this)->asRegExpObject();
    assert(re);
    return Local<String>::New(Value::fromVmValue(VM::Value::fromString(currentEngine()->current, re->value->pattern())));
}

RegExp::Flags RegExp::GetFlags() const
{
    RegExpObject *re = ConstValuePtr(this)->asRegExpObject();
    assert(re);

    int f = 0;
    if (re->global)
        f |= kGlobal;
    if (re->value->ignoreCase())
        f |= kIgnoreCase;
    if (re->value->multiLine())
        f |= kMultiline;

    return (RegExp::Flags)f;
}

RegExp *RegExp::Cast(Value *obj)
{
    return static_cast<RegExp *>(obj);
}

Local<Value> External::Wrap(void *data)
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

void *External::Unwrap(Handle<v8::Value> obj)
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<External> External::New(void *value)
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

External *External::Cast(v8::Value *obj)
{
    return static_cast<External *>(obj);
}

void *External::Value() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}


void Template::Set(Handle<String> name, Handle<Data> value, PropertyAttribute attributes)
{
    Q_UNIMPLEMENTED();
}

void Template::Set(const char *name, Handle<Data> value)
{
    Q_UNIMPLEMENTED();
}


int Arguments::Length() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Value> Arguments::operator [](int i) const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Function> Arguments::Callee() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Object> Arguments::This() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Object> Arguments::Holder() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

bool Arguments::IsConstructCall() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Value> Arguments::Data() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Isolate *Arguments::GetIsolate() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}


AccessorInfo::AccessorInfo(const VM::Value &thisObject, const Persistent<Value> &data)
{
    m_this = Persistent<Object>::New(Value::fromVmValue(thisObject));
    m_data = data;
}

Isolate *AccessorInfo::GetIsolate() const
{
    return Isolate::GetCurrent();
}

Local<Value> AccessorInfo::Data() const
{
    return Local<Value>::New(m_data);
}

Local<Object> AccessorInfo::This() const
{
    return Local<Object>::New(m_this);
}

Local<Object> AccessorInfo::Holder() const
{
    // ### FIXME
    return Local<Object>::New(m_this);
}


Local<FunctionTemplate> FunctionTemplate::New(InvocationCallback callback, Handle<Value> data)
{
    FunctionTemplate *ft = new FunctionTemplate;
    ft->m_callback = callback;
    ft->m_data = Persistent<Value>::New(data);
    return Local<FunctionTemplate>::New(Handle<FunctionTemplate>(ft));
}

Local<Function> FunctionTemplate::GetFunction()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<ObjectTemplate> FunctionTemplate::InstanceTemplate()
{
    if (!*m_instanceTemplate)
        m_instanceTemplate = ObjectTemplate::New();
    return m_instanceTemplate;
}

Local<ObjectTemplate> FunctionTemplate::PrototypeTemplate()
{
    if (!*m_prototypeTemplate)
        m_prototypeTemplate = ObjectTemplate::New();
    return m_prototypeTemplate;
}

class V4V8Object : public VM::Object
{
public:
    V4V8Object(VM::ExecutionEngine *engine)
        : VM::Object(engine)
    {
        vtbl = &static_vtbl;
    }

protected:
    static const ManagedVTable static_vtbl;

    static VM::Value get(Managed *m, ExecutionContext *ctx, VM::String *name, bool *hasProperty)
    {
        return VM::Object::get(m, ctx, name, hasProperty);
    }

    static VM::Value getIndexed(Managed *m, ExecutionContext *ctx, uint index, bool *hasProperty)
    {
        return VM::Object::getIndexed(m, ctx, index, hasProperty);
    }

    static void put(Managed *m, ExecutionContext *ctx, VM::String *name, const VM::Value &value)
    {
        VM::Object::put(m, ctx, name, value);
    }

    static void putIndexed(Managed *m, ExecutionContext *ctx, uint index, const VM::Value &value)
    {
        VM::Object::putIndexed(m, ctx, index, value);
    }

    static PropertyFlags query(Managed *m, ExecutionContext *ctx, VM::String *name)
    {
        return VM::Object::query(m, ctx, name);
    }

    static PropertyFlags queryIndexed(Managed *m, ExecutionContext *ctx, uint index)
    {
        return VM::Object::queryIndexed(m, ctx, index);
    }

    static bool deleteProperty(Managed *m, ExecutionContext *ctx, VM::String *name)
    {
        return VM::Object::deleteProperty(m, ctx, name);
    }

    static bool deleteIndexedProperty(Managed *m, ExecutionContext *ctx, uint index)
    {
        return VM::Object::deleteIndexedProperty(m, ctx, index);
    }
};

DEFINE_MANAGED_VTABLE(V4V8Object);

struct V8AccessorGetter: FunctionObject {
    AccessorGetter getter;
    Persistent<Value> data;
    Persistent<String> name;

    V8AccessorGetter(ExecutionContext *scope, const Handle<String> &name, const AccessorGetter &getter, Handle<Value> data)
        : FunctionObject(scope)
    {
        vtbl = &static_vtbl;
        this->getter = getter;
        this->data = Persistent<Value>::New(data);
        this->name = Persistent<String>::New(name);
    }

    using Object::construct;

    static VM::Value call(Managed *that, ExecutionContext *context, const VM::Value &thisObject, VM::Value *args, int argc)
    {
        V8AccessorGetter *getter = static_cast<V8AccessorGetter*>(that);
        AccessorInfo info(thisObject, getter->data);
        return getter->getter(Local<String>::New(getter->name), info)->vmValue();
    }

protected:
    static const ManagedVTable static_vtbl;
};

DEFINE_MANAGED_VTABLE(V8AccessorGetter);

struct V8AccessorSetter: FunctionObject {
    AccessorSetter setter;
    Persistent<Value> data;
    Persistent<String> name;

    V8AccessorSetter(ExecutionContext *scope, const Handle<String> &name, const AccessorSetter &setter, Handle<Value> data)
        : FunctionObject(scope)
    {
        vtbl = &static_vtbl;
        this->setter = setter;
        this->data = Persistent<Value>::New(data);
        this->name = Persistent<String>::New(name);
    }

    using Object::construct;

    static VM::Value call(Managed *that, ExecutionContext *context, const VM::Value &thisObject, VM::Value *args, int argc)
    {
        if (!argc)
            return VM::Value::undefinedValue();
        V8AccessorSetter *setter = static_cast<V8AccessorSetter*>(that);
        AccessorInfo info(thisObject, setter->data);
        setter->setter(Local<String>::New(setter->name), Local<Value>::New(Value::fromVmValue(args[0])), info);
        return VM::Value::undefinedValue();
    }

protected:
    static const ManagedVTable static_vtbl;
};

DEFINE_MANAGED_VTABLE(V8AccessorSetter);

Local<ObjectTemplate> ObjectTemplate::New()
{
    ObjectTemplate *ot = new ObjectTemplate;
    return Local<ObjectTemplate>::New(Handle<ObjectTemplate>(ot));
}

Local<Object> ObjectTemplate::NewInstance()
{
    VM::ExecutionEngine *engine = currentEngine();
    VM::Object *o = new (engine->memoryManager) V4V8Object(engine);
    o->prototype = engine->objectPrototype;

    foreach (const Accessor &acc, m_accessors) {
        VM::PropertyDescriptor *pd = o->insertMember(acc.name->asVMString());
        *pd = VM::PropertyDescriptor::fromAccessor(acc.getter->vmValue().asFunctionObject(),
                                                   acc.setter->vmValue().asFunctionObject());
        pd->writable = VM::PropertyDescriptor::Undefined;
        pd->configurable = acc.attribute & DontDelete ? VM::PropertyDescriptor::Disabled : VM::PropertyDescriptor::Enabled;
        pd->enumerable = acc.attribute & DontEnum ? VM::PropertyDescriptor::Disabled : VM::PropertyDescriptor::Enabled;
    }

    return Local<Object>::New(Value::fromVmValue(VM::Value::fromObject(o)));
}

void ObjectTemplate::SetAccessor(Handle<String> name, AccessorGetter getter, AccessorSetter setter, Handle<Value> data, AccessControl settings, PropertyAttribute attribute)
{
    VM::ExecutionEngine *engine = currentEngine();

    Accessor a;
    VM::FunctionObject *wrappedGetter = getter ? new (engine->memoryManager) V8AccessorGetter(engine->rootContext, name, getter, data) : 0;
    a.getter = Persistent<Value>::New(Value::fromVmValue(VM::Value::fromObject(wrappedGetter)));
    VM::FunctionObject *wrappedSetter = setter ? new (engine->memoryManager) V8AccessorSetter(engine->rootContext, name, setter, data) : 0;
    a.setter = Persistent<Value>::New(Value::fromVmValue(VM::Value::fromObject(wrappedSetter)));
    a.attribute = attribute;
    a.name = Persistent<String>::New(name);
    m_accessors << a;
}

void ObjectTemplate::SetNamedPropertyHandler(NamedPropertyGetter getter, NamedPropertySetter setter, NamedPropertyQuery query, NamedPropertyDeleter deleter, NamedPropertyEnumerator enumerator, Handle<Value> data)
{
    Q_UNIMPLEMENTED();
}

void ObjectTemplate::SetFallbackPropertyHandler(NamedPropertyGetter getter, NamedPropertySetter setter, NamedPropertyQuery query, NamedPropertyDeleter deleter, NamedPropertyEnumerator enumerator, Handle<Value> data)
{
    Q_UNIMPLEMENTED();
}

void ObjectTemplate::SetIndexedPropertyHandler(IndexedPropertyGetter getter, IndexedPropertySetter setter, IndexedPropertyQuery query, IndexedPropertyDeleter deleter, IndexedPropertyEnumerator enumerator, Handle<Value> data)
{
    Q_UNIMPLEMENTED();
}

int ObjectTemplate::InternalFieldCount()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

void ObjectTemplate::SetInternalFieldCount(int value)
{
    Q_UNIMPLEMENTED();
}

bool ObjectTemplate::HasExternalResource()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

void ObjectTemplate::SetHasExternalResource(bool value)
{
    Q_UNIMPLEMENTED();
}

void ObjectTemplate::MarkAsUseUserObjectComparison()
{
    Q_UNIMPLEMENTED();
}


Handle<Primitive> Undefined()
{
    Handle<Primitive> val;
    val.val = VM::Value::undefinedValue().val;
    return val;
}

Handle<Primitive> Null()
{
    Handle<Primitive> val;
    val.val = VM::Value::nullValue().val;
    return val;
}

Handle<Boolean> True()
{
    Handle<Primitive> val;
    val.val = VM::Value::fromBoolean(true).val;
    return val;
}

Handle<Boolean> False()
{
    Handle<Primitive> val;
    val.val = VM::Value::fromBoolean(false).val;
    return val;
}


Handle<Value> ThrowException(Handle<Value> exception)
{
    __qmljs_throw(currentEngine()->current, exception->vmValue());
    return Handle<Value>();
}


Local<Value> Exception::ReferenceError(Handle<String> message)
{
    Q_UNUSED(message);
    VM::Object *o = currentEngine()->newReferenceErrorObject(currentEngine()->current, message->ToString()->asQString());
    return Local<Value>::New(Value::fromVmValue(VM::Value::fromObject(o)));
}

Local<Value> Exception::SyntaxError(Handle<String> message)
{
    Q_UNUSED(message);
    VM::Object *o = currentEngine()->newSyntaxErrorObject(currentEngine()->current, 0);
    return Local<Value>::New(Value::fromVmValue(VM::Value::fromObject(o)));
}

Local<Value> Exception::TypeError(Handle<String> message)
{
    Q_UNUSED(message);
    VM::Object *o = currentEngine()->newTypeErrorObject(currentEngine()->current, message->ToString()->asQString());
    return Local<Value>::New(Value::fromVmValue(VM::Value::fromObject(o)));
}

Local<Value> Exception::Error(Handle<String> message)
{
    Q_UNUSED(message);
    VM::Object *o = currentEngine()->newErrorObject(VM::Value::fromString(currentEngine()->current, message->ToString()->asQString()));
    return Local<Value>::New(Value::fromVmValue(VM::Value::fromObject(o)));
}


static QThreadStorage<Isolate*> currentIsolate;

Isolate::Isolate()
    : m_lastIsolate(0)
{
}

Isolate::~Isolate()
{
}

Isolate *Isolate::New()
{
    assert(!"Isolate::New()");
    Q_UNREACHABLE();
}

void Isolate::Enter()
{
    m_lastIsolate = currentIsolate.localData();
    currentIsolate.localData() = this;
}

void Isolate::Exit()
{
    currentIsolate.localData() = m_lastIsolate;
    m_lastIsolate = 0;
}

void Isolate::Dispose()
{
    Q_UNIMPLEMENTED();
}

void Isolate::SetData(void *data)
{
    Q_UNIMPLEMENTED();
}

void *Isolate::GetData()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Isolate *Isolate::GetCurrent()
{
    if (!currentIsolate.hasLocalData())
        currentIsolate.setLocalData(new Isolate);
    return currentIsolate.localData();
}


void V8::SetFlagsFromString(const char *, int)
{
    // we can safely ignore these
}

static UserObjectComparisonCallback userObjectComparisonCallback = 0;

static bool v8ExternalResourceComparison(const VM::Value &a, const VM::Value &b)
{
    if (!userObjectComparisonCallback)
        return false;
    Local<Object> la = Local<Object>::New(Value::fromVmValue(a));
    Local<Object> lb = Local<Object>::New(Value::fromVmValue(b));
    return userObjectComparisonCallback(la, lb);
}

void V8::SetUserObjectComparisonCallbackFunction(UserObjectComparisonCallback callback)
{
    userObjectComparisonCallback = callback;
    currentEngine()->externalResourceComparison = v8ExternalResourceComparison;
}

void V8::AddGCPrologueCallback(GCPrologueCallback, GCType)
{
    // not required currently as we don't have weak Persistent references.
    // not having them will lead to some leaks in QQmlVMEMetaObejct, but shouldn't matter otherwise
}

void V8::RemoveGCPrologueCallback(GCPrologueCallback)
{
    assert(!"RemoveGCPrologueCallback();");
}

void V8::AddImplicitReferences(Persistent<Object> parent, Persistent<Value> *children, size_t length)
{
    // not required currently as we don't have weak Persistent references.
    // not having them will lead to some leaks in QQmlVMEMetaObejct, but shouldn't matter otherwise
    assert(!"AddImplicitReferences();");
}

bool V8::Initialize()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

bool V8::Dispose()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

bool V8::IdleNotification(int hint)
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

void V8::LowMemoryNotification()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

TryCatch::TryCatch()
{
    Q_UNIMPLEMENTED();
}

TryCatch::~TryCatch()
{
    Q_UNIMPLEMENTED();
}

bool TryCatch::HasCaught() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Handle<Value> TryCatch::ReThrow()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Value> TryCatch::Exception() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Message> TryCatch::Message() const
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

void TryCatch::Reset()
{
    Q_UNIMPLEMENTED();
}



struct Context::Private
{
    Private()
    {
        engine.reset(new QQmlJS::VM::ExecutionEngine);
    }

    QScopedPointer<QQmlJS::VM::ExecutionEngine> engine;
};

Context::Context()
    : m_lastContext(0)
    , d(new Private)
{
}

Context::~Context()
{
    delete d;
}

Persistent<Context> Context::New(ExtensionConfiguration *extensions, Handle<ObjectTemplate> global_template, Handle<Value> global_object)
{
    Context *result = new Context;
    return Persistent<Context>::New(Handle<Context>(result));
}

Local<Object> Context::Global()
{
    return Local<Object>::New(Value::fromVmValue(d->engine->globalObject));
}

Local<Context> Context::GetCurrent()
{
    return Context::Adopt(Isolate::GetCurrent()->m_currentContext);
}

Local<Context> Context::GetCalling()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Object> Context::GetCallingQmlGlobal()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Value> Context::GetCallingScriptData()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

void Context::Enter()
{
    Isolate* iso = Isolate::GetCurrent();
    m_lastContext = iso->m_currentContext;
    iso->m_currentContext = this;
}

void Context::Exit()
{
    Isolate::GetCurrent()->m_currentContext = m_lastContext;
}


QQmlJS::VM::ExecutionEngine *Context::GetEngine()
{
    return d->engine.data();
}


}
