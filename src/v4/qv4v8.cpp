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
    Q_UNIMPLEMENTED();
}

Handle<Integer> ScriptOrigin::ResourceLineOffset() const
{
    Q_UNIMPLEMENTED();
}

Handle<Integer> ScriptOrigin::ResourceColumnOffset() const
{
    Q_UNIMPLEMENTED();
}


Local<Script> Script::New(Handle<String> source,
                         ScriptOrigin* origin,
                         ScriptData* pre_data,
                         Handle<String> script_data,
                         CompileFlags flags)
{
    Q_UNIMPLEMENTED();
}


Local<Script> Script::New(Handle<String> source,
                         Handle<Value> file_name,
                         CompileFlags flags)
{
    Q_UNIMPLEMENTED();
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
}

Local<Value> Script::Id()
{
    Q_UNIMPLEMENTED();
}

void Script::SetData(Handle<String> data)
{
    Q_UNIMPLEMENTED();
}


Local<String> Message::Get() const
{
    Q_UNIMPLEMENTED();
}

Handle<Value> Message::GetScriptResourceName() const
{
    Q_UNIMPLEMENTED();
}

int Message::GetLineNumber() const
{
    Q_UNIMPLEMENTED();
}


Local<StackFrame> StackTrace::GetFrame(uint32_t index) const
{
    Q_UNIMPLEMENTED();
}

int StackTrace::GetFrameCount() const
{
    Q_UNIMPLEMENTED();
}

Local<Array> StackTrace::AsArray()
{
    Q_UNIMPLEMENTED();
}

Local<StackTrace> StackTrace::CurrentStackTrace(int frame_limit, StackTrace::StackTraceOptions options)
{
    Q_UNIMPLEMENTED();
}


int StackFrame::GetLineNumber() const
{
    Q_UNIMPLEMENTED();
}

int StackFrame::GetColumn() const
{
    Q_UNIMPLEMENTED();
}

Local<String> StackFrame::GetScriptName() const
{
    Q_UNIMPLEMENTED();
}

Local<String> StackFrame::GetScriptNameOrSourceURL() const
{
    Q_UNIMPLEMENTED();
}

Local<String> StackFrame::GetFunctionName() const
{
    Q_UNIMPLEMENTED();
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
}

bool Value::IsInt32() const
{
    return ConstValuePtr(this)->isInteger();
}

bool Value::IsUint32() const
{
    Q_UNIMPLEMENTED();
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
    Q_UNIMPLEMENTED();
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
    Q_UNIMPLEMENTED();
}

uint32_t Value::Uint32Value() const
{
    Q_UNIMPLEMENTED();
}

int32_t Value::Int32Value() const
{
    Q_UNIMPLEMENTED();
}

bool Value::Equals(Handle<Value> that) const
{
    return __qmljs_equal(*ConstValuePtr(this), *ConstValuePtr(&that), currentEngine()->current);
}

bool Value::StrictEquals(Handle<Value> that) const
{
    return __qmljs_strict_equal(*ConstValuePtr(this), *ConstValuePtr(&that));
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
    Q_UNIMPLEMENTED();
}


int String::Length() const
{
    Q_UNIMPLEMENTED();
}

uint32_t String::Hash() const
{
    Q_UNIMPLEMENTED();
}


String::CompleteHashData String::CompleteHash() const
{
    CompleteHashData data;
    data.hash = asVMString()->hashValue();
    data.length = asQString().length();
    data.symbol_id = 0; // ###
    return data;
}

uint32_t String::ComputeHash(uint16_t *string, int length)
{
    Q_UNIMPLEMENTED();
}

uint32_t String::ComputeHash(char *string, int length)
{
    Q_UNIMPLEMENTED();
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
    Q_UNIMPLEMENTED();
}

v8::Local<String> String::Empty()
{
    return Local<String>::New(v8::Value::fromVmValue(VM::Value::fromString(currentEngine()->current, QString())));
}

bool String::IsExternal() const
{
    Q_UNIMPLEMENTED();
}

String::ExternalStringResource *String::GetExternalStringResource() const
{
    Q_UNIMPLEMENTED();
}

String *String::Cast(v8::Value *obj)
{
    Q_UNIMPLEMENTED();
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
    Q_UNIMPLEMENTED();
}

Local<String> String::NewExternal(String::ExternalStringResource *resource)
{
    Q_UNIMPLEMENTED();
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
    Q_UNIMPLEMENTED();
}

Number *Number::Cast(v8::Value *obj)
{
    Q_UNIMPLEMENTED();
}

Local<Integer> Integer::New(int32_t value)
{
    Q_UNIMPLEMENTED();
}

Local<Integer> Integer::NewFromUnsigned(uint32_t value)
{
    Q_UNIMPLEMENTED();
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
    Q_UNIMPLEMENTED();
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
}

bool Object::Set(uint32_t index, Handle<Value> value)
{
}

Local<Value> Object::Get(Handle<Value> key)
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    QQmlJS::VM::ExecutionContext *ctx = currentEngine()->current;
    QQmlJS::VM::Value prop = o->__get__(ctx, ValuePtr(&key)->toString(ctx));
    return Local<Value>::New(Value::fromVmValue(prop));
}

Local<Value> Object::Get(uint32_t key)
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    QQmlJS::VM::ExecutionContext *ctx = currentEngine()->current;
    QQmlJS::VM::Value prop = o->__get__(ctx, key);
    return Local<Value>::New(Value::fromVmValue(prop));
}

bool Object::Has(Handle<String> key)
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    if (!o)
        return false;
    return o->__hasProperty__(currentEngine()->current, ValuePtr(&key)->asString());
}

bool Object::Delete(Handle<String> key)
{
    Q_UNIMPLEMENTED();
}

bool Object::Has(uint32_t index)
{
    Q_UNIMPLEMENTED();
}

bool Object::Delete(uint32_t index)
{
    Q_UNIMPLEMENTED();
}

bool Object::SetAccessor(Handle<String> name, AccessorGetter getter, AccessorSetter setter, Handle<Value> data, AccessControl settings, PropertyAttribute attribute)
{
    Q_UNIMPLEMENTED();
}

Local<Array> Object::GetPropertyNames()
{
    Q_UNIMPLEMENTED();
}

Local<Array> Object::GetOwnPropertyNames()
{
    Q_UNIMPLEMENTED();
}

Local<Value> Object::GetPrototype()
{
    Local<Value> result;
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    if (!o)
        return result;
    return Local<Value>::New(Value::fromVmValue(QQmlJS::VM::Value::fromObject(o->prototype)));
}

bool Object::SetPrototype(Handle<Value> prototype)
{
    Q_UNIMPLEMENTED();
}

Local<String> Object::GetConstructorName()
{
    Q_UNIMPLEMENTED();
}

int Object::InternalFieldCount()
{
    Q_UNIMPLEMENTED();
}

Local<Value> Object::GetInternalField(int index)
{
    Q_UNIMPLEMENTED();
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
    Q_UNIMPLEMENTED();
}

int Object::GetIdentityHash()
{
    Q_UNIMPLEMENTED();
}

bool Object::SetHiddenValue(Handle<String> key, Handle<Value> value)
{
    Q_UNIMPLEMENTED();
}

Local<Value> Object::GetHiddenValue(Handle<String> key)
{
    Q_UNIMPLEMENTED();
}

Local<Object> Object::Clone()
{
    Q_UNIMPLEMENTED();
}

bool Object::IsCallable()
{
    Q_UNIMPLEMENTED();
}

Local<Value> Object::CallAsFunction(Handle<Object> recv, int argc, Handle<Value> argv[])
{
    Q_UNIMPLEMENTED();
}

Local<Value> Object::CallAsConstructor(int argc, Handle<Value> argv[])
{
    Q_UNIMPLEMENTED();
}

Local<Object> Object::New()
{
    Q_UNIMPLEMENTED();
}

Object *Object::Cast(Value *obj)
{
    Q_UNIMPLEMENTED();
}


uint32_t Array::Length() const
{
    Q_UNIMPLEMENTED();
}

Local<Array> Array::New(int length)
{
    Q_UNIMPLEMENTED();
}

Array *Array::Cast(Value *obj)
{
    Q_UNIMPLEMENTED();
}


Local<Object> Function::NewInstance() const
{
    Q_UNIMPLEMENTED();
}

Local<Object> Function::NewInstance(int argc, Handle<Value> argv[]) const
{
    Q_UNIMPLEMENTED();
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
    Q_UNIMPLEMENTED();
}

ScriptOrigin Function::GetScriptOrigin() const
{
    Q_UNIMPLEMENTED();
}

Function *Function::Cast(Value *obj)
{
    Q_UNIMPLEMENTED();
}


Local<Value> Date::New(double time)
{
    Q_UNIMPLEMENTED();
}

double Date::NumberValue() const
{
    Q_UNIMPLEMENTED();
}

Date *Date::Cast(Value *obj)
{
    Q_UNIMPLEMENTED();
}

void Date::DateTimeConfigurationChangeNotification()
{
    Q_UNIMPLEMENTED();
}


Local<Value> NumberObject::New(double value)
{
    Q_UNIMPLEMENTED();
}

double NumberObject::NumberValue() const
{
    Q_UNIMPLEMENTED();
}

NumberObject *NumberObject::Cast(Value *obj)
{
    Q_UNIMPLEMENTED();
}

Local<Value> BooleanObject::New(bool value)
{
    Q_UNIMPLEMENTED();
}

bool BooleanObject::BooleanValue() const
{
    Q_UNIMPLEMENTED();
}

BooleanObject *BooleanObject::Cast(Value *obj)
{
    Q_UNIMPLEMENTED();
}

Local<Value> StringObject::New(Handle<String> value)
{
    Q_UNIMPLEMENTED();
}

Local<String> StringObject::StringValue() const
{
    Q_UNIMPLEMENTED();
}

StringObject *StringObject::Cast(Value *obj)
{
    Q_UNIMPLEMENTED();
}

Local<RegExp> RegExp::New(Handle<String> pattern, RegExp::Flags flags)
{
    Q_UNIMPLEMENTED();
}

Local<String> RegExp::GetSource() const
{
    Q_UNIMPLEMENTED();
}

RegExp::Flags RegExp::GetFlags() const
{
    Q_UNIMPLEMENTED();
}

RegExp *RegExp::Cast(Value *obj)
{
    Q_UNIMPLEMENTED();
}

Local<Value> External::Wrap(void *data)
{
    Q_UNIMPLEMENTED();
}

void *External::Unwrap(Handle<v8::Value> obj)
{
    Q_UNIMPLEMENTED();
}

Local<External> External::New(void *value)
{
    Q_UNIMPLEMENTED();
}

External *External::Cast(v8::Value *obj)
{
    Q_UNIMPLEMENTED();
}

void *External::Value() const
{
    Q_UNIMPLEMENTED();
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
}

Local<Value> Arguments::operator [](int i) const
{
    Q_UNIMPLEMENTED();
}

Local<Function> Arguments::Callee() const
{
    Q_UNIMPLEMENTED();
}

Local<Object> Arguments::This() const
{
    Q_UNIMPLEMENTED();
}

Local<Object> Arguments::Holder() const
{
    Q_UNIMPLEMENTED();
}

bool Arguments::IsConstructCall() const
{
    Q_UNIMPLEMENTED();
}

Local<Value> Arguments::Data() const
{
    Q_UNIMPLEMENTED();
}

Isolate *Arguments::GetIsolate() const
{
    Q_UNIMPLEMENTED();
}


AccessorInfo::AccessorInfo(internal::Object **args)
{
    Q_UNIMPLEMENTED();
}

Isolate *AccessorInfo::GetIsolate() const
{
    Q_UNIMPLEMENTED();
}

Local<Value> AccessorInfo::Data() const
{
    Q_UNIMPLEMENTED();
}

Local<Object> AccessorInfo::This() const
{
    Q_UNIMPLEMENTED();
}

Local<Object> AccessorInfo::Holder() const
{
    Q_UNIMPLEMENTED();
}


Local<FunctionTemplate> FunctionTemplate::New(InvocationCallback callback, Handle<Value> data)
{
    Q_UNIMPLEMENTED();
}

Local<Function> FunctionTemplate::GetFunction()
{
    Q_UNIMPLEMENTED();
}

Local<ObjectTemplate> FunctionTemplate::InstanceTemplate()
{
    Q_UNIMPLEMENTED();
}

Local<ObjectTemplate> FunctionTemplate::PrototypeTemplate()
{
    Q_UNIMPLEMENTED();
}


Local<ObjectTemplate> ObjectTemplate::New()
{
    Q_UNIMPLEMENTED();
}

Local<Object> ObjectTemplate::NewInstance()
{
    Q_UNIMPLEMENTED();
}

void ObjectTemplate::SetAccessor(Handle<String> name, AccessorGetter getter, AccessorSetter setter, Handle<Value> data, AccessControl settings, PropertyAttribute attribute)
{
    Q_UNIMPLEMENTED();
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
}

void ObjectTemplate::SetInternalFieldCount(int value)
{
    Q_UNIMPLEMENTED();
}

bool ObjectTemplate::HasExternalResource()
{
    Q_UNIMPLEMENTED();
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
    Q_UNIMPLEMENTED();
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

void V8::SetUserObjectComparisonCallbackFunction(UserObjectComparisonCallback)
{
    Q_UNIMPLEMENTED();
}

void V8::AddGCPrologueCallback(GCPrologueCallback callback, GCType gc_type_filter)
{
    Q_UNIMPLEMENTED();
}

void V8::RemoveGCPrologueCallback(GCPrologueCallback callback)
{
    Q_UNIMPLEMENTED();
}

void V8::AddImplicitReferences(Persistent<Object> parent, Persistent<Value> *children, size_t length)
{
    Q_UNIMPLEMENTED();
}

bool V8::Initialize()
{
    Q_UNIMPLEMENTED();
}

bool V8::Dispose()
{
    Q_UNIMPLEMENTED();
}

bool V8::IdleNotification(int hint)
{
    Q_UNIMPLEMENTED();
}

void V8::LowMemoryNotification()
{
    Q_UNIMPLEMENTED();
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
}

Handle<Value> TryCatch::ReThrow()
{
    Q_UNIMPLEMENTED();
}

Local<Value> TryCatch::Exception() const
{
    Q_UNIMPLEMENTED();
}

Local<Message> TryCatch::Message() const
{
    Q_UNIMPLEMENTED();
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
}

Local<Object> Context::GetCallingQmlGlobal()
{
    Q_UNIMPLEMENTED();
}

Local<Value> Context::GetCallingScriptData()
{
    Q_UNIMPLEMENTED();
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
