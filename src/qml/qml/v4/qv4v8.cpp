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

#include "qv4v8_p.h"

#include "qv4engine_p.h"
#include "qv4runtime_p.h"
#include "qv4mm_p.h"
#include "qv4managed_p.h"
#include "qv4functionobject_p.h"
#include "qv4value_p.h"
#include "qv4isel_masm_p.h"
#include "qv4globalobject_p.h"
#include "qv4regexpobject_p.h"
#include "qv4dateobject_p.h"
#include "qv4numberobject_p.h"
#include "qv4booleanobject_p.h"
#include "qv4stringobject_p.h"
#include "qv4objectproto_p.h"
#include <QThreadStorage>

using namespace QQmlJS;
using namespace QV4;

namespace v8 {

#define currentEngine() Isolate::GetCurrent()->GetCurrentContext()->GetEngine()

#define Q_D(obj) QV4::Value *d = reinterpret_cast<QV4::Value*>(obj)

#define ValuePtr(obj) reinterpret_cast<QV4::Value*>(obj)
#define ConstValuePtr(obj) reinterpret_cast<const QV4::Value*>(obj)

void *gcProtect(void *handle)
{
    Q_D(handle);
    if (QV4::Managed *m = d->asManaged()) {
        currentEngine()->memoryManager->protect(m);
        return currentEngine()->memoryManager;
    }
}

void gcProtect(void *memoryManager, void *handle)
{
    Q_D(handle);
    if (QV4::Managed *m = d->asManaged())
        if (memoryManager)
            static_cast<QV4::MemoryManager *>(memoryManager)->protect(m);
}

void gcUnprotect(void *memoryManager, void *handle)
{
    Q_D(handle);
    if (QV4::Managed *m = d->asManaged())
        if (memoryManager)
            static_cast<QV4::MemoryManager *>(memoryManager)->unprotect(m);
}

quint64 qv8_get_value(const QV4::Value &v)
{
    return v.val;
}

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

    static QV4::Value call(Managed *that, ExecutionContext *context, const QV4::Value &thisObject, QV4::Value *args, int argc)
    {
        V8AccessorGetter *getter = static_cast<V8AccessorGetter*>(that);
        AccessorInfo info(thisObject, getter->data);
        QV4::Value result = QV4::Value::undefinedValue();
        try {
            result = getter->getter(Local<String>::New(getter->name), info)->v4Value();
        } catch (QV4::Exception &e) {
            Isolate::GetCurrent()->setException(e.value());
            e.accept(context);
        }
        return result;
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

    static QV4::Value call(Managed *that, ExecutionContext *context, const QV4::Value &thisObject, QV4::Value *args, int argc)
    {
        if (!argc)
            return QV4::Value::undefinedValue();
        V8AccessorSetter *setter = static_cast<V8AccessorSetter*>(that);
        AccessorInfo info(thisObject, setter->data);
        try {
            setter->setter(Local<String>::New(setter->name), Local<Value>::New(Value::fromV4Value(args[0])), info);
        } catch (QV4::Exception &e) {
            Isolate::GetCurrent()->setException(e.value());
            e.accept(context);
        }
        return QV4::Value::undefinedValue();
    }

protected:
    static const ManagedVTable static_vtbl;
};

DEFINE_MANAGED_VTABLE(V8AccessorSetter);

ScriptOrigin::ScriptOrigin(Handle<Value> resource_name, Handle<Integer> resource_line_offset, Handle<Integer> resource_column_offset)
{
    m_fileName = resource_name->ToString()->asQString();
    m_lineNumber = resource_line_offset->ToInt32()->Value();
    m_columnNumber = resource_column_offset->ToInt32()->Value();
}

Handle<Value> ScriptOrigin::ResourceName() const
{
    return Value::fromV4Value(QV4::Value::fromString(currentEngine()->current, m_fileName));
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
    if (context.IsEmpty())
        context = Context::GetCurrent();
    ASSERT(context.get());
    QV4::ExecutionEngine *engine = context->GetEngine();
    QV4::ExecutionContext *ctx = engine->current;

    QV4::Value result = QV4::Value::undefinedValue();
    try {
        QV4::Function *f = QV4::EvalFunction::parseSource(engine->rootContext, m_origin.m_fileName, m_script, QQmlJS::Codegen::EvalCode,
                                                                        /*strictMode =*/ false, /*inheritContext =*/ false);
        if (!f)
            __qmljs_throw(engine->current, QV4::Value::fromObject(engine->newSyntaxErrorObject(engine->current, 0)));

        result = context->GetEngine()->run(f);
    } catch (QV4::Exception &e) {
        Isolate::GetCurrent()->setException(e.value());
        e.accept(ctx);
        return Local<Value>();
    }

    return Local<Value>::New(Value::fromV4Value(result));
}

Local<Value> Script::Run(Handle<Object> qml)
{
    Handle<Context> context = m_context;
    if (context.IsEmpty())
        context = Context::GetCurrent();
    ASSERT(context.get());
    QV4::ExecutionEngine *engine = context->GetEngine();
    QV4::ExecutionContext *ctx = engine->current;

    QV4::Value result = QV4::Value::undefinedValue();

    try {

        QV4::EvalFunction *eval = new (engine->memoryManager) QV4::EvalFunction(engine->rootContext, qml->v4Value().asObject());

        QV4::Value arg = QV4::Value::fromString(engine->current, m_script);

        result = eval->evalCall(engine->current, QV4::Value::undefinedValue(), &arg, 1, /*directCall*/ false);
    } catch (QV4::Exception &e) {
        Isolate::GetCurrent()->setException(e.value());
        e.accept(ctx);
        return Local<Value>();
    }
    return Local<Value>::New(Value::fromV4Value(result));
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
    return Local<String>::New(Value::fromV4Value(QV4::Value::fromString(currentEngine()->current, m_message)));
}

Handle<Value> Message::GetScriptResourceName() const
{
    return Value::fromV4Value(QV4::Value::fromString(currentEngine()->current, m_resourceName));
}

int Message::GetLineNumber() const
{
    return m_lineNumber;
}


Local<StackFrame> StackTrace::GetFrame(uint32_t index) const
{
    if (index >= (uint)frames.size())
        return Local<StackFrame>();
    return frames.at(index);
}

int StackTrace::GetFrameCount() const
{
    return frames.size();
}

Local<Array> StackTrace::AsArray()
{
    Q_UNIMPLEMENTED();
    return Local<Array>();
}

Local<StackTrace> StackTrace::CurrentStackTrace(int frame_limit, StackTrace::StackTraceOptions options)
{
    StackTrace *trace = new StackTrace;
    QV4::ExecutionEngine *engine = currentEngine();
    QV4::ExecutionContext *current = engine->current;
    while (current && frame_limit) {
        if (CallContext *c = current->asCallContext()) {
            StackFrame *frame = new StackFrame(Value::fromV4Value(QV4::Value::fromString(engine->id_null)),
                                               Value::fromV4Value(QV4::Value::fromString(c->function->name)),
                                               0, 0);
            trace->frames.append(frame);
            --frame_limit;
        }
        current = current->parent;
    }

    return Local<StackTrace>::New(Handle<StackTrace>(trace));
}


int StackFrame::GetLineNumber() const
{
    return m_lineNumber;
}

int StackFrame::GetColumn() const
{
    return m_columnNumber;
}

Local<String> StackFrame::GetScriptName() const
{
    return Local<String>::New(m_scriptName);
}

Local<String> StackFrame::GetScriptNameOrSourceURL() const
{
    return Local<String>::New(m_scriptName);
}

Local<String> StackFrame::GetFunctionName() const
{
    return Local<String>::New(m_functionName);
}

StackFrame::StackFrame(Handle<String> script, Handle<String> function, int line, int column)
    : m_lineNumber(line)
    , m_columnNumber(column)
{
    m_scriptName = Persistent<String>::New(script);
    m_functionName = Persistent<String>::New(function);
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
    return Local<Boolean>::New(Value::fromV4Value(QV4::Value::fromBoolean(ConstValuePtr(this)->toBoolean())));
}

Local<Number> Value::ToNumber() const
{
    return Local<Number>::New(Value::fromV4Value(QV4::Value::fromDouble(ConstValuePtr(this)->toNumber())));
}

Local<String> Value::ToString() const
{
    return Local<String>::New(Value::fromV4Value(QV4::Value::fromString(ConstValuePtr(this)->toString(currentEngine()->current))));
}

Local<Object> Value::ToObject() const
{
    return Local<Object>::New(Value::fromV4Value(QV4::Value::fromObject(ConstValuePtr(this)->toObject(currentEngine()->current))));
}

Local<Integer> Value::ToInteger() const
{
    return Local<Integer>::New(Value::fromV4Value(QV4::Value::fromDouble(ConstValuePtr(this)->toInteger())));
}

Local<Uint32> Value::ToUint32() const
{
    return Local<Uint32>::New(Value::fromV4Value(QV4::Value::fromUInt32(ConstValuePtr(this)->toUInt32())));
}

Local<Int32> Value::ToInt32() const
{
    return Local<Int32>::New(Value::fromV4Value(QV4::Value::fromInt32(ConstValuePtr(this)->toInt32())));
}

Local<Uint32> Value::ToArrayIndex() const
{
    return Local<Uint32>::New(Value::fromV4Value(QV4::Value::fromUInt32(ConstValuePtr(this)->asArrayIndex())));
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
    return (int64_t)ConstValuePtr(this)->toInteger();
}

uint32_t Value::Uint32Value() const
{
    return ConstValuePtr(this)->toUInt32();
}

int32_t Value::Int32Value() const
{
    return ConstValuePtr(this)->toInt32();
}

bool Value::Equals(Handle<Value> that) const
{
    return __qmljs_equal(*ConstValuePtr(this), *ConstValuePtr(&that));
}

bool Value::StrictEquals(Handle<Value> that) const
{
    return __qmljs_strict_equal(*ConstValuePtr(this), *ConstValuePtr(&that));
}

QV4::Value Value::v4Value() const
{
    return *ConstValuePtr(this);
}

Handle<Value> Value::fromV4Value(const QV4::Value &v4Value)
{
    Handle<Value> res;
    res.val = v4Value.val;
    return res;
}


bool Boolean::Value() const
{
    return BooleanValue();
}

Handle<Boolean> Boolean::New(bool value)
{
    return Value::fromV4Value(QV4::Value::fromBoolean(value));
}


int String::Length() const
{
    return asV4String()->toQString().length();
}

uint32_t String::Hash() const
{
    return asV4String()->hashValue();
}


String::CompleteHashData String::CompleteHash() const
{
    QV4::String *s = asV4String();
    CompleteHashData data;
    data.hash = s->hashValue();
    data.length = s->toQString().length();
    data.symbol_id = s->identifier;
    return data;
}

uint32_t String::ComputeHash(uint16_t *string, int length)
{
    return QV4::String::createHashValue(reinterpret_cast<const QChar *>(string), length);
}

uint32_t String::ComputeHash(char *string, int length)
{
    // ### unefficient
    QString s = QString::fromLatin1((char *)string, length);
    return QV4::String::createHashValue(s.constData(), s.length());
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
    if (length < 0)
        length = asQString().length();
    if (length == 0)
        return 0;
    if (asQString().length() < length)
        length = asQString().length();
    // ### do we use options?
    memcpy(buffer + start, asQString().constData(), length*sizeof(QChar));
    return length;
}

v8::Local<String> String::Empty()
{
    return Local<String>::New(v8::Value::fromV4Value(QV4::Value::fromString(currentEngine()->current, QString())));
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
    QV4::Value v = QV4::Value::fromString(currentEngine()->current, QString::fromLatin1(data, length));
    return Local<String>::New(v8::Value::fromV4Value(v));
}

Local<String> String::New(const uint16_t *data, int length)
{
    QV4::Value v = QV4::Value::fromString(currentEngine()->current, QString((const QChar *)data, length));
    return Local<String>::New(v8::Value::fromV4Value(v));
}

Local<String> String::NewSymbol(const char *data, int length)
{
    QString str = QString::fromLatin1(data, length);
    QV4::String *vmString = currentEngine()->newIdentifier(str);
    return New(vmString);
}

Local<String> String::New(QV4::String *s)
{
    return Local<String>::New(v8::Value::fromV4Value(QV4::Value::fromString(s)));
}

Local<String> String::NewExternal(String::ExternalStringResource *resource)
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

QString String::asQString() const
{
    return asV4String()->toQString();
}

QV4::String *String::asV4String() const
{
    const QV4::Value *v = ConstValuePtr(this);
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
    const QV4::Value *v = ConstValuePtr(this);
    assert(v->isNumber());
    return v->asDouble();
}

Local<Number> Number::New(double value)
{
    return Local<Number>::New(Value::fromV4Value(QV4::Value::fromDouble(value)));
}

Number *Number::Cast(v8::Value *obj)
{
    return static_cast<Number *>(obj);
}

Local<Integer> Integer::New(int32_t value)
{
    return Local<Integer>::New(Value::fromV4Value(QV4::Value::fromInt32(value)));
}

Local<Integer> Integer::NewFromUnsigned(uint32_t value)
{
    return Local<Integer>::New(Value::fromV4Value(QV4::Value::fromUInt32(value)));
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
    const QV4::Value *v = ConstValuePtr(this);
    assert(v->isNumber());
    return (int64_t)v->asDouble();
}

Integer *Integer::Cast(v8::Value *obj)
{
    return static_cast<Integer *>(obj);
}

int32_t Int32::Value() const
{
    const QV4::Value *v = ConstValuePtr(this);
    assert(v->isInteger());
    return v->int_32;
}

uint32_t Uint32::Value() const
{
    const QV4::Value *v = ConstValuePtr(this);
    assert(v->isNumber());
    return v->toUInt32();
}


struct ExternalResourceWrapper : public QV4::Object::ExternalResource
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
    QV4::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    QV4::ExecutionContext *ctx = currentEngine()->current;
    bool result = true;
    try {
        o->put(ctx, ValuePtr(&key)->toString(ctx), *ValuePtr(&value));
        // ### attribs
    } catch (QV4::Exception &e) {
        Isolate::GetCurrent()->setException(e.value());
        e.accept(ctx);
        result = false;
    }
    return result;
}

bool Object::Set(uint32_t index, Handle<Value> value)
{
    QV4::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    QV4::ExecutionContext *ctx = currentEngine()->current;
    bool result = true;
    try {
        o->putIndexed(ctx, index, *ValuePtr(&value));
        // ### attribs
    } catch (QV4::Exception &e) {
        Isolate::GetCurrent()->setException(e.value());
        e.accept(ctx);
        result = false;
    }
    return result;
}

Local<Value> Object::Get(Handle<Value> key)
{
    QV4::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    QV4::ExecutionContext *ctx = currentEngine()->current;
    QV4::Value prop = QV4::Value::undefinedValue();
    try {
        prop = o->get(ctx, ValuePtr(&key)->toString(ctx));
    } catch (QV4::Exception &e) {
        Isolate::GetCurrent()->setException(e.value());
        e.accept(ctx);
        return Local<Value>();
    }
    return Local<Value>::New(Value::fromV4Value(prop));
}

Local<Value> Object::Get(uint32_t key)
{
    QV4::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    QV4::ExecutionContext *ctx = currentEngine()->current;
    QV4::Value prop = QV4::Value::undefinedValue();
    try {
        prop = o->getIndexed(ctx, key);
    } catch (QV4::Exception &e) {
        Isolate::GetCurrent()->setException(e.value());
        e.accept(ctx);
        return Local<Value>();
    }
    return Local<Value>::New(Value::fromV4Value(prop));
}

bool Object::Has(Handle<String> key)
{
    QV4::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    return o->__hasProperty__(ValuePtr(&key)->asString());
}

bool Object::Delete(Handle<String> key)
{
    QV4::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    bool result = false;
    ExecutionContext *ctx = currentEngine()->current;
    try {
        result = o->deleteProperty(ctx, ValuePtr(&key)->asString());
    } catch (QV4::Exception &e) {
        Isolate::GetCurrent()->setException(e.value());
        e.accept(ctx);
    }
    return result;
}

bool Object::Has(uint32_t index)
{
    QV4::Object *o = ConstValuePtr(this)->asObject();
    if (!o)
        return false;
    return o->__hasProperty__(index);
}

bool Object::Delete(uint32_t index)
{
    QV4::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    ExecutionContext *ctx = currentEngine()->current;
    bool result = false;
    try {
        result = o->deleteIndexedProperty(ctx, index);
    } catch (QV4::Exception &e) {
        Isolate::GetCurrent()->setException(e.value());
        e.accept(ctx);
    }
    return result;
}

bool Object::SetAccessor(Handle<String> name, AccessorGetter getter, AccessorSetter setter, Handle<Value> data, AccessControl settings, PropertyAttribute attribute)
{
    QV4::ExecutionEngine *engine = currentEngine();

    QV4::FunctionObject *wrappedGetter = 0;
    if (getter) {
        wrappedGetter = new (engine->memoryManager) V8AccessorGetter(engine->rootContext, name, getter, data);
    }
    QV4::FunctionObject *wrappedSetter = 0;
    if (setter) {
        wrappedSetter = new (engine->memoryManager) V8AccessorSetter(engine->rootContext, name, setter, data);
    }

    QV4::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    PropertyAttributes attrs = Attr_Accessor;
    attrs.setConfigurable(!(attribute & DontDelete));
    attrs.setEnumerable(!(attribute & DontEnum));
    QV4::Property *pd = o->insertMember(name->asV4String(), attrs);
    pd->setGetter(wrappedGetter);
    pd->setSetter(wrappedSetter);
    return true;
}

Local<Array> Object::GetPropertyNames()
{
    QV4::Object *o = ConstValuePtr(this)->asObject();
    assert(o);

    QV4::ArrayObject *array = currentEngine()->newArrayObject();
    ObjectIterator it(o, ObjectIterator::WithProtoChain|ObjectIterator::EnumberableOnly);
    while (1) {
        QV4::Value v = it.nextPropertyNameAsString();
        if (v.isNull())
            break;
        array->push_back(v);
    }
    return Local<Array>::New(Value::fromV4Value(QV4::Value::fromObject(array)));
}

Local<Array> Object::GetOwnPropertyNames()
{
    QV4::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    QV4::Value arg = QV4::Value::fromObject(o);
    ArrayObject *array = currentEngine()->newArrayObject();
    ObjectIterator it(o, ObjectIterator::EnumberableOnly);
    while (1) {
        QV4::Value v = it.nextPropertyNameAsString();
        if (v.isNull())
            break;
        array->push_back(v);
    }
    return Local<Array>::New(Value::fromV4Value(QV4::Value::fromObject(array)));
}

Local<Value> Object::GetPrototype()
{
    Local<Value> result;
    QV4::Object *o = ConstValuePtr(this)->asObject();
    if (!o)
        return Local<Value>();
    return Local<Value>::New(Value::fromV4Value(QV4::Value::fromObject(o->prototype)));
}

bool Object::SetPrototype(Handle<Value> prototype)
{
    QV4::Object *p = ConstValuePtr(&prototype)->asObject();
    if (!p)
        return false;
    QV4::Object *o = ConstValuePtr(this)->asObject();
    if (!o)
        return false;

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
    QV4::Object *o = ConstValuePtr(this)->asObject();
    if (!o)
        return;
    o->externalResource = new ExternalResourceWrapper(resource);
}

Object::ExternalResource *Object::GetExternalResource()
{
    QV4::Object *o = ConstValuePtr(this)->asObject();
    if (!o || !o->externalResource)
        return 0;
    return static_cast<ExternalResourceWrapper*>(o->externalResource)->wrapped;
}

bool Object::HasOwnProperty(Handle<String> key)
{
    QV4::Object *o = ConstValuePtr(this)->asObject();
    assert(o);
    QV4::ExecutionContext *ctx = currentEngine()->current;
    return o->__getOwnProperty__(ValuePtr(&key)->toString(ctx));
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
    QV4::FunctionObject *f = ConstValuePtr(this)->asFunctionObject();
    if (!f)
        return Local<Value>();
    ExecutionContext *context = currentEngine()->current;
    try {
        QV4::Value retval = f->call(context, recv->v4Value(),
                                   reinterpret_cast<QV4::Value*>(argv),
                                   argc);
        return Local<Value>::New(Value::fromV4Value(retval));
    } catch (QV4::Exception &e) {
        Isolate::GetCurrent()->setException(e.value());
        e.accept(context);
    }
    return Local<Object>();
}

Local<Value> Object::CallAsConstructor(int argc, Handle<Value> argv[])
{
    QV4::FunctionObject *f = ConstValuePtr(this)->asFunctionObject();
    if (!f)
        return Local<Value>();
    ExecutionContext *context = currentEngine()->current;
    try {
        QV4::Value retval = f->construct(context,
                                        reinterpret_cast<QV4::Value*>(argv),
                                        argc);
        return Local<Value>::New(Value::fromV4Value(retval));
    } catch (QV4::Exception &e) {
        Isolate::GetCurrent()->setException(e.value());
        e.accept(context);
    }
    return Local<Object>();
}

Local<Object> Object::New()
{
    QV4::Object *o = currentEngine()->newObject();
    return Local<Object>::New(Value::fromV4Value(QV4::Value::fromObject(o)));
}

Object *Object::Cast(Value *obj)
{
    return static_cast<Object *>(obj);
}


uint32_t Array::Length() const
{
    QV4::ArrayObject *a = ConstValuePtr(this)->asArrayObject();
    if (!a)
        return 0;
    return a->arrayLength();
}

Local<Array> Array::New(int length)
{
    if (length < 0)
        length = 0;
    QV4::ArrayObject *a = currentEngine()->newArrayObject();
    if (length < 0x1000)
        a->arrayReserve(length);

    return Local<Array>::New(Value::fromV4Value(QV4::Value::fromObject(a)));
}

Array *Array::Cast(Value *obj)
{
    return static_cast<Array *>(obj);
}


Local<Object> Function::NewInstance() const
{
    QV4::FunctionObject *f = ConstValuePtr(this)->asFunctionObject();
    assert(f);
    QV4::ExecutionContext *context = currentEngine()->current;
    QV4::Value result = QV4::Value::undefinedValue();
    try {
        result = f->construct(context, 0, 0);
    } catch (QV4::Exception &e) {
        Isolate::GetCurrent()->setException(e.value());
        e.accept(context);
        return Local<Object>();
    }
    return Local<Object>::New(Value::fromV4Value(result));
}

Local<Object> Function::NewInstance(int argc, Handle<Value> argv[]) const
{
    QV4::FunctionObject *f = ConstValuePtr(this)->asFunctionObject();
    assert(f);
    QV4::ExecutionContext *context = currentEngine()->current;
    QV4::Value result = QV4::Value::undefinedValue();
    try {
        result = f->construct(context, reinterpret_cast<QV4::Value*>(argv), argc);
    } catch (QV4::Exception &e) {
        Isolate::GetCurrent()->setException(e.value());
        e.accept(context);
        return Local<Object>();
    }
    return Local<Object>::New(Value::fromV4Value(result));
}

Local<Value> Function::Call(Handle<Object> thisObj, int argc, Handle<Value> argv[])
{
    QV4::FunctionObject *f = ConstValuePtr(this)->asFunctionObject();
    if (!f)
        return Local<Value>();
    QV4::ExecutionContext *context = currentEngine()->current;
    QV4::Value result = QV4::Value::undefinedValue();
    try {
        result = f->call(context, *ConstValuePtr(&thisObj),
                         reinterpret_cast<QV4::Value*>(argv), argc);
    } catch (QV4::Exception &e) {
        Isolate::GetCurrent()->setException(e.value());
        e.accept(context);
        return Local<Value>();
    }
    return Local<Value>::New(Value::fromV4Value(result));
}

Handle<Value> Function::GetName() const
{
    QV4::FunctionObject *f = ConstValuePtr(this)->asFunctionObject();
    if (!f)
        return Handle<Value>();
    return Value::fromV4Value(QV4::Value::fromString(f->name));
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
    QV4::Object *o = currentEngine()->newDateObject(QV4::Value::fromDouble(time));
    return Local<Value>::New(Value::fromV4Value(QV4::Value::fromObject(o)));
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
    QV4::Object *o = currentEngine()->newNumberObject(QV4::Value::fromDouble(value));
    return Local<Value>::New(Value::fromV4Value(QV4::Value::fromObject(o)));
}

double NumberObject::NumberValue() const
{
    QV4::NumberObject *n = ConstValuePtr(this)->asNumberObject();
    assert(n);
    return n->value.doubleValue();
}

NumberObject *NumberObject::Cast(Value *obj)
{
    return static_cast<NumberObject *>(obj);
}

Local<Value> BooleanObject::New(bool value)
{
    QV4::Object *o = currentEngine()->newBooleanObject(QV4::Value::fromBoolean(value));
    return Local<Value>::New(Value::fromV4Value(QV4::Value::fromObject(o)));
}

bool BooleanObject::BooleanValue() const
{
    QV4::BooleanObject *b = ConstValuePtr(this)->asBooleanObject();
    assert(b);
    return b->value.booleanValue();
}

BooleanObject *BooleanObject::Cast(Value *obj)
{
    return static_cast<BooleanObject *>(obj);
}

Local<Value> StringObject::New(Handle<String> value)
{
    QV4::Object *o = currentEngine()->newStringObject(QV4::Value::fromString(value->v4Value().asString()));
    return Local<Value>::New(Value::fromV4Value(QV4::Value::fromObject(o)));
}

Local<String> StringObject::StringValue() const
{
    QV4::StringObject *s = ConstValuePtr(this)->asStringObject();
    assert(s);
    return Local<String>::New(Value::fromV4Value(s->value));
}

StringObject *StringObject::Cast(Value *obj)
{
    return static_cast<StringObject *>(obj);
}

Local<RegExp> RegExp::New(Handle<String> pattern, RegExp::Flags flags)
{
    int f = 0;
    if (flags & kGlobal)
        f |= V4IR::RegExp::RegExp_Global;
    if (flags & kIgnoreCase)
        f |= V4IR::RegExp::RegExp_IgnoreCase;
    if (flags & kMultiline)
        f |= V4IR::RegExp::RegExp_Multiline;
    QV4::Object *o = currentEngine()->newRegExpObject(pattern->asQString(), f);
    return Local<RegExp>::New(Value::fromV4Value(QV4::Value::fromObject(o)));
}

Local<String> RegExp::GetSource() const
{
    RegExpObject *re = ConstValuePtr(this)->asRegExpObject();
    assert(re);
    return Local<String>::New(Value::fromV4Value(QV4::Value::fromString(currentEngine()->current, re->value->pattern())));
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

struct VoidStarWrapper : public QV4::Object::ExternalResource
{
    void *data;
};

Local<Value> External::Wrap(void *data)
{
    return New(data);
}

void *External::Unwrap(Handle<v8::Value> obj)
{
    return obj.As<External>()->Value();
}

Local<External> External::New(void *value)
{
    QV4::Object *o = currentEngine()->newObject();
    VoidStarWrapper *wrapper = new VoidStarWrapper;
    wrapper->data = value;
    o->externalResource = wrapper;
    return Local<v8::External>::New(v8::Value::fromV4Value(QV4::Value::fromObject(o)));
}

External *External::Cast(v8::Value *obj)
{
    return static_cast<External *>(obj);
}

void *External::Value() const
{
    QV4::Object *o = ConstValuePtr(this)->asObject();
    if (!o || !o->externalResource)
        return 0;
    return static_cast<VoidStarWrapper*>(o->externalResource)->data;
}


void Template::Set(Handle<String> name, Handle<Value> value, PropertyAttribute attributes)
{
    Property p;
    p.name = Persistent<String>::New(name);
    p.value = Persistent<Value>::New(value);
    p.attributes = attributes;
    m_properties << p;
}

void Template::Set(const char *name, Handle<Value> value)
{
    Set(String::New(name), value);
}


Arguments::Arguments(const QV4::Value *args, int argc, const QV4::Value &thisObject, bool isConstructor, const Persistent<Value> &data)
{
    for (int i = 0; i < argc; ++i)
        m_args << Persistent<Value>::New(Value::fromV4Value(args[i]));
    m_thisObject = Persistent<Object>::New(Value::fromV4Value(thisObject));
    m_isConstructor = isConstructor;
    m_data = Persistent<Value>::New(data);
}

int Arguments::Length() const
{
    return m_args.size();
}

Local<Value> Arguments::operator [](int i) const
{
    return Local<Value>::New(m_args.at(i));
}

Local<Object> Arguments::This() const
{
    return Local<Object>::New(m_thisObject);
}

Local<Object> Arguments::Holder() const
{
    // ### FIXME.
    return Local<Object>::New(m_thisObject);
}

bool Arguments::IsConstructCall() const
{
    return m_isConstructor;
}

Local<Value> Arguments::Data() const
{
    return Local<Value>::New(m_data);
}

Isolate *Arguments::GetIsolate() const
{
    return Isolate::GetCurrent();
}


AccessorInfo::AccessorInfo(const QV4::Value &thisObject, const Persistent<Value> &data)
{
    m_this = Persistent<Object>::New(Value::fromV4Value(thisObject));
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

template <typename BaseClass>
class V4V8Object : public BaseClass
{
public:
    V4V8Object(QV4::ExecutionEngine *engine, ObjectTemplate *tmpl)
        : BaseClass(engine->rootContext)
    {
        this->vtbl = &static_vtbl;
        m_template = Persistent<ObjectTemplate>(tmpl);
        if (m_template.IsEmpty())
            m_template = Persistent<ObjectTemplate>::New(ObjectTemplate::New());

        foreach (const ObjectTemplate::Accessor &acc, m_template->m_accessors) {
            PropertyAttributes attrs = Attr_Accessor;
            attrs.setConfigurable(!(acc.attribute & DontDelete));
            attrs.setEnumerable(!(acc.attribute & DontEnum));
            QV4::Property *pd = this->insertMember(acc.name->asV4String(), attrs);
            *pd = QV4::Property::fromAccessor(acc.getter->v4Value().asFunctionObject(),
                                             acc.setter->v4Value().asFunctionObject());
        }

        initProperties(m_template.get());
    }

    void initProperties(Template *tmpl)
    {
        foreach (const Template::Property &p, tmpl->m_properties) {
            PropertyAttributes attrs = Attr_Data;
            attrs.setConfigurable(!(p.attributes & DontDelete));
            attrs.setEnumerable(!(p.attributes & DontEnum));
            attrs.setWritable(!(p.attributes & ReadOnly));
            QV4::Property *pd = this->insertMember(p.name->asV4String(), attrs);
            *pd = QV4::Property::fromValue(p.value->v4Value());
        }
    }

    Persistent<ObjectTemplate> m_template;

protected:
    AccessorInfo namedAccessorInfo()
    {
        // ### thisObject?
        return AccessorInfo(QV4::Value::fromObject(this), m_template->m_namedPropertyData);
    }
    AccessorInfo fallbackAccessorInfo()
    {
        // ### thisObject?
        return AccessorInfo(QV4::Value::fromObject(this), m_template->m_fallbackPropertyData);
    }
    AccessorInfo indexedAccessorInfo()
    {
        // ### thisObject?
        return AccessorInfo(QV4::Value::fromObject(this), m_template->m_namedPropertyData);
    }

    static const ManagedVTable static_vtbl;

    static QV4::Value get(QV4::Managed *m, ExecutionContext *ctx, QV4::String *name, bool *hasProperty)
    {
        V4V8Object *that = static_cast<V4V8Object*>(m);
        if (that->m_template->m_namedPropertyGetter) {
            Handle<Value> result = that->m_template->m_namedPropertyGetter(String::New(name), that->namedAccessorInfo());
            if (!result.IsEmpty()) {
                if (hasProperty)
                    *hasProperty = true;
                return result->v4Value();
            }
        }

        bool hasProp = false;
        QV4::Value result = BaseClass::get(m, ctx, name, &hasProp);

        if (!hasProp && that->m_template->m_fallbackPropertyGetter) {
            Handle<Value> fallbackResult = that->m_template->m_fallbackPropertyGetter(String::New(name), that->fallbackAccessorInfo());
            if (!fallbackResult.IsEmpty()) {
                if (hasProperty)
                    *hasProperty = true;
                return fallbackResult->v4Value();
            }
        }

        if (hasProperty)
            *hasProperty = hasProp;
        return result;
    }

    static QV4::Value getIndexed(QV4::Managed *m, ExecutionContext *ctx, uint index, bool *hasProperty)
    {
        V4V8Object *that = static_cast<V4V8Object*>(m);
        if (that->m_template->m_indexedPropertyGetter) {
            Handle<Value> result = that->m_template->m_indexedPropertyGetter(index, that->indexedAccessorInfo());
            if (!result.IsEmpty()) {
                if (hasProperty)
                    *hasProperty = true;
                return result->v4Value();
            }
        }
        return BaseClass::getIndexed(m, ctx, index, hasProperty);
    }

    static void put(QV4::Managed *m, ExecutionContext *ctx, QV4::String *name, const QV4::Value &value)
    {
        Local<Value> v8Value = Local<Value>::New(Value::fromV4Value(value));
        V4V8Object *that = static_cast<V4V8Object*>(m);
        if (that->m_template->m_namedPropertySetter) {
            Handle<Value> result = that->m_template->m_namedPropertySetter(String::New(name), v8Value, that->namedAccessorInfo());
            if (!result.IsEmpty())
                return;
        }
        PropertyAttributes attrs;
        Property *pd  = that->__getOwnProperty__(name, &attrs);
        if (pd)
            that->putValue(ctx, pd, attrs, value);
        else if (that->m_template->m_fallbackPropertySetter)
            that->m_template->m_fallbackPropertySetter(String::New(name), v8Value, that->fallbackAccessorInfo());
        else
            BaseClass::put(m, ctx, name, value);
    }

    static void putIndexed(QV4::Managed *m, ExecutionContext *ctx, uint index, const QV4::Value &value)
    {
        V4V8Object *that = static_cast<V4V8Object*>(m);
        if (that->m_template->m_indexedPropertySetter) {
            Handle<Value> result = that->m_template->m_indexedPropertySetter(index, Local<Value>::New(Value::fromV4Value(value)), that->indexedAccessorInfo());
            if (!result.IsEmpty())
                return;
        }
        BaseClass::putIndexed(m, ctx, index, value);
    }

    static void getLookup(QV4::Managed *m, ExecutionContext *ctx, Lookup *l, QV4::Value *result)
    {
        *result = get(m, ctx, l->name, 0);
    }

    static void setLookup(QV4::Managed *m, ExecutionContext *ctx, Lookup *l, const QV4::Value &value)
    {
        put(m, ctx, l->name, value);
    }

    static PropertyAttributes propertyAttributesToFlags(const Handle<Value> &attr)
    {
        PropertyAttributes flags;
        int intAttr = attr->ToInt32()->Value();
        flags.setWritable(!(intAttr & ReadOnly));
        flags.setEnumerable(!(intAttr & DontEnum));
        flags.setConfigurable(!(intAttr & DontDelete));
        return flags;
    }

    static PropertyAttributes query(QV4::Managed *m, ExecutionContext *ctx, QV4::String *name)
    {
        V4V8Object *that = static_cast<V4V8Object*>(m);
        if (that->m_template->m_namedPropertyQuery) {
            Handle<Value> result = that->m_template->m_namedPropertyQuery(String::New(name), that->namedAccessorInfo());
            if (!result.IsEmpty())
                return propertyAttributesToFlags(result);
        }
        PropertyAttributes flags = BaseClass::query(m, ctx, name);
        if (flags.type() == PropertyAttributes::Generic && that->m_template->m_fallbackPropertySetter) {
            Handle<Value> result = that->m_template->m_fallbackPropertyQuery(String::New(name), that->fallbackAccessorInfo());
            if (!result.IsEmpty())
                return propertyAttributesToFlags(result);
        }

        return flags;
    }

    static PropertyAttributes queryIndexed(QV4::Managed *m, ExecutionContext *ctx, uint index)
    {
        V4V8Object *that = static_cast<V4V8Object*>(m);
        if (that->m_template->m_indexedPropertyQuery) {
            Handle<Value> result = that->m_template->m_indexedPropertyQuery(index, that->indexedAccessorInfo());
            if (!result.IsEmpty())
                return propertyAttributesToFlags(result);
        }

        return BaseClass::queryIndexed(m, ctx, index);
    }

    static bool deleteProperty(QV4::Managed *m, ExecutionContext *ctx, QV4::String *name)
    {
        V4V8Object *that = static_cast<V4V8Object*>(m);
        if (that->m_template->m_namedPropertyDeleter) {
            Handle<Boolean> result = that->m_template->m_namedPropertyDeleter(String::New(name), that->namedAccessorInfo());
            if (!result.IsEmpty())
                return result->Value();
        }

        bool result = BaseClass::deleteProperty(m, ctx, name);

        if (that->m_template->m_fallbackPropertyDeleter) {
            Handle<Boolean> interceptResult = that->m_template->m_fallbackPropertyDeleter(String::New(name), that->fallbackAccessorInfo());
            if (!interceptResult.IsEmpty())
                result = interceptResult->Value();
        }

        return result;
    }

    static bool deleteIndexedProperty(QV4::Managed *m, ExecutionContext *ctx, uint index)
    {
        V4V8Object *that = static_cast<V4V8Object*>(m);
        if (that->m_template->m_indexedPropertyDeleter) {
            Handle<Boolean> result = that->m_template->m_indexedPropertyDeleter(index, that->indexedAccessorInfo());
            if (!result.IsEmpty())
                return result->Value();
        }
        return BaseClass::deleteIndexedProperty(m, ctx, index);
    }
};

template<>
DEFINE_MANAGED_VTABLE(V4V8Object<QV4::Object>);
template<>
DEFINE_MANAGED_VTABLE(V4V8Object<QV4::FunctionObject>);
template<>
DEFINE_MANAGED_VTABLE(V4V8Object<QV4::FunctionPrototype>);

struct V4V8Function : public V4V8Object<QV4::FunctionObject>
{
    V4V8Function(QV4::ExecutionEngine *engine, FunctionTemplate *functionTemplate)
        : V4V8Object<QV4::FunctionObject>(engine, 0)
    {
        vtbl = &static_vtbl;
        m_functionTemplate = Persistent<FunctionTemplate>(functionTemplate);
        initProperties(m_functionTemplate.get());
    }

protected:
    static const ManagedVTable static_vtbl;

    static QV4::Value call(QV4::Managed *m, ExecutionContext *context, const QV4::Value &thisObject, QV4::Value *args, int argc)
    {
        V4V8Function *that = static_cast<V4V8Function*>(m);
        Arguments arguments(args, argc, thisObject, false, that->m_functionTemplate->m_data);
        QV4::Value result = QV4::Value::undefinedValue();
        if (that->m_functionTemplate->m_callback)
            result = that->m_functionTemplate->m_callback(arguments)->v4Value();
        return result;
    }

    static QV4::Value construct(QV4::Managed *m, ExecutionContext *context, QV4::Value *args, int argc)
    {
        V4V8Function *that = static_cast<V4V8Function*>(m);
        Arguments arguments(args, argc, QV4::Value::undefinedValue(), true, that->m_functionTemplate->m_data);

        QV4::Object *obj = that->m_functionTemplate->m_instanceTemplate->NewInstance()->v4Value().asObject();
        QV4::Value proto = that->Managed::get(context, context->engine->id_prototype);
        if (proto.isObject())
            obj->prototype = proto.objectValue();

        QV4::Value result = QV4::Value::undefinedValue();
        if (that->m_functionTemplate->m_callback)
            result = that->m_functionTemplate->m_callback(arguments)->v4Value();
        if (result.isObject())
            return result;
        return QV4::Value::fromObject(obj);

    }

    Persistent<FunctionTemplate> m_functionTemplate;
};

DEFINE_MANAGED_VTABLE(V4V8Function);

FunctionTemplate::FunctionTemplate(InvocationCallback callback, Handle<Value> data)
    : m_callback(callback)
{
    m_instanceTemplate = Local<ObjectTemplate>();
    m_prototypeTemplate = Local<ObjectTemplate>();
    m_data = Persistent<Value>::New(data);
}

Local<FunctionTemplate> FunctionTemplate::New(InvocationCallback callback, Handle<Value> data)
{
    FunctionTemplate *ft = new FunctionTemplate(callback, data);
    return Local<FunctionTemplate>::New(Handle<FunctionTemplate>(ft));
}

Local<Function> FunctionTemplate::GetFunction()
{
    QV4::ExecutionEngine *engine = currentEngine();
    QV4::Object *o = new (engine->memoryManager) V4V8Function(engine, this);
    QV4::Object *proto = new (engine->memoryManager) V4V8Object<QV4::FunctionPrototype>(engine, m_prototypeTemplate.get());
    o->put(engine->current, engine->id_prototype, QV4::Value::fromObject(proto));
    return Local<Function>::New(Value::fromV4Value(QV4::Value::fromObject(o)));
}

Local<ObjectTemplate> FunctionTemplate::InstanceTemplate()
{
    if (m_instanceTemplate.IsEmpty())
        m_instanceTemplate = ObjectTemplate::New();
    return m_instanceTemplate;
}

Local<ObjectTemplate> FunctionTemplate::PrototypeTemplate()
{
    if (m_prototypeTemplate.IsEmpty())
        m_prototypeTemplate = ObjectTemplate::New();
    return m_prototypeTemplate;
}


Local<ObjectTemplate> ObjectTemplate::New()
{
    ObjectTemplate *ot = new ObjectTemplate;
    return Local<ObjectTemplate>::New(Handle<ObjectTemplate>(ot));
}

Local<Object> ObjectTemplate::NewInstance()
{
    QV4::ExecutionEngine *engine = currentEngine();
    QV4::Object *o = new (engine->memoryManager) V4V8Object<QV4::Object>(engine, this);
    o->prototype = engine->objectPrototype;
    o->externalComparison = m_useUserComparison;

    return Local<Object>::New(Value::fromV4Value(QV4::Value::fromObject(o)));
}

void ObjectTemplate::SetAccessor(Handle<String> name, AccessorGetter getter, AccessorSetter setter, Handle<Value> data, AccessControl settings, PropertyAttribute attribute)
{
    QV4::ExecutionEngine *engine = currentEngine();

    Accessor a;
    if (getter) {
        QV4::FunctionObject *wrappedGetter = new (engine->memoryManager) V8AccessorGetter(engine->rootContext, name, getter, data);
        a.getter = Persistent<Value>::New(Value::fromV4Value(QV4::Value::fromObject(wrappedGetter)));
    }
    if (setter) {
        QV4::FunctionObject *wrappedSetter = new (engine->memoryManager) V8AccessorSetter(engine->rootContext, name, setter, data);
        a.setter = Persistent<Value>::New(Value::fromV4Value(QV4::Value::fromObject(wrappedSetter)));
    }
    a.attribute = attribute;
    a.name = Persistent<String>::New(name);
    m_accessors << a;
}

void ObjectTemplate::SetNamedPropertyHandler(NamedPropertyGetter getter, NamedPropertySetter setter, NamedPropertyQuery query, NamedPropertyDeleter deleter, NamedPropertyEnumerator enumerator, Handle<Value> data)
{
    m_namedPropertyGetter = getter;
    m_namedPropertySetter = setter;
    m_namedPropertyQuery = query;
    m_namedPropertyDeleter = deleter;
    m_namedPropertyEnumerator = enumerator;
    m_namedPropertyData = Persistent<Value>::New(data);
}

void ObjectTemplate::SetFallbackPropertyHandler(NamedPropertyGetter getter, NamedPropertySetter setter, NamedPropertyQuery query, NamedPropertyDeleter deleter, NamedPropertyEnumerator enumerator, Handle<Value> data)
{
    m_fallbackPropertyGetter = getter;
    m_fallbackPropertySetter = setter;
    m_fallbackPropertyQuery = query;
    m_fallbackPropertyDeleter = deleter;
    m_fallbackPropertyEnumerator = enumerator;
    m_fallbackPropertyData = Persistent<Value>::New(data);
}

void ObjectTemplate::SetIndexedPropertyHandler(IndexedPropertyGetter getter, IndexedPropertySetter setter, IndexedPropertyQuery query, IndexedPropertyDeleter deleter, IndexedPropertyEnumerator enumerator, Handle<Value> data)
{
    m_indexedPropertyGetter = getter;
    m_indexedPropertySetter = setter;
    m_indexedPropertyQuery = query;
    m_indexedPropertyDeleter = deleter;
    m_indexedPropertyEnumerator = enumerator;
    m_indexedPropertyData = Persistent<Value>::New(data);
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
    // we always reserve the space for the external resource
    return true;
}

void ObjectTemplate::SetHasExternalResource(bool value)
{
    // no need for this, we always reserve the space for the external resource
    Q_UNUSED(value);
}

void ObjectTemplate::MarkAsUseUserObjectComparison()
{
    m_useUserComparison = true;
}

ObjectTemplate::ObjectTemplate()
{
    m_namedPropertyGetter = 0;
    m_namedPropertySetter = 0;
    m_namedPropertyQuery = 0;
    m_namedPropertyDeleter = 0;
    m_namedPropertyEnumerator = 0;

    m_fallbackPropertyGetter = 0;
    m_fallbackPropertySetter = 0;
    m_fallbackPropertyQuery = 0;
    m_fallbackPropertyDeleter = 0;
    m_fallbackPropertyEnumerator = 0;

    m_indexedPropertyGetter = 0;
    m_indexedPropertySetter = 0;
    m_indexedPropertyQuery = 0;
    m_indexedPropertyDeleter = 0;
    m_indexedPropertyEnumerator = 0;

    m_useUserComparison = false;
}

Handle<Primitive> Undefined()
{
    Handle<Primitive> val;
    val.val = QV4::Value::undefinedValue().val;
    return val;
}

Handle<Primitive> Null()
{
    Handle<Primitive> val;
    val.val = QV4::Value::nullValue().val;
    return val;
}

Handle<Boolean> True()
{
    Handle<Primitive> val;
    val.val = QV4::Value::fromBoolean(true).val;
    return val;
}

Handle<Boolean> False()
{
    Handle<Primitive> val;
    val.val = QV4::Value::fromBoolean(false).val;
    return val;
}


Handle<Value> ThrowException(Handle<Value> exception)
{
    __qmljs_throw(currentEngine()->current, exception->v4Value());
    return Handle<Value>();
}


Local<Value> Exception::ReferenceError(Handle<String> message)
{
    Q_UNUSED(message);
    QV4::Object *o = currentEngine()->newReferenceErrorObject(message->ToString()->asQString());
    return Local<Value>::New(Value::fromV4Value(QV4::Value::fromObject(o)));
}

Local<Value> Exception::SyntaxError(Handle<String> message)
{
    Q_UNUSED(message);
    QV4::Object *o = currentEngine()->newSyntaxErrorObject(currentEngine()->current, 0);
    return Local<Value>::New(Value::fromV4Value(QV4::Value::fromObject(o)));
}

Local<Value> Exception::TypeError(Handle<String> message)
{
    Q_UNUSED(message);
    QV4::Object *o = currentEngine()->newTypeErrorObject(message->ToString()->asQString());
    return Local<Value>::New(Value::fromV4Value(QV4::Value::fromObject(o)));
}

Local<Value> Exception::Error(Handle<String> message)
{
    Q_UNUSED(message);
    QV4::Object *o = currentEngine()->newErrorObject(QV4::Value::fromString(currentEngine()->current, message->ToString()->asQString()));
    return Local<Value>::New(Value::fromV4Value(QV4::Value::fromObject(o)));
}


static QThreadStorage<Isolate*> currentIsolate;

Isolate::Isolate()
    : tryCatch(0)
{
}

Isolate::~Isolate()
{
}

void Isolate::setException(const QV4::Value &ex)
{
    if (tryCatch) {
        tryCatch->hasCaughtException = true;
        tryCatch->exception = Local<Value>::New(Value::fromV4Value(ex));
    }
}

ExecutionEngine *Isolate::GetEngine()
{
    return Isolate::GetCurrent()->GetCurrentContext()->GetEngine();
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

static bool v8ExternalResourceComparison(const QV4::Value &a, const QV4::Value &b)
{
    if (!userObjectComparisonCallback)
        return false;
    Local<Object> la = Local<Object>::New(Value::fromV4Value(a));
    Local<Object> lb = Local<Object>::New(Value::fromV4Value(b));
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
    return true;
}

void V8::LowMemoryNotification()
{
    currentEngine()->memoryManager->runGC();
}


TryCatch::TryCatch()
{
    Isolate *i = Isolate::GetCurrent();

    hasCaughtException = false;
    parent = i->tryCatch;
    i->tryCatch = this;
}

TryCatch::~TryCatch()
{
    Isolate *i = Isolate::GetCurrent();
    i->tryCatch = parent;
}

bool TryCatch::HasCaught() const
{
    return hasCaughtException;
}

Handle<Value> TryCatch::ReThrow()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Value> TryCatch::Exception() const
{
    return exception;
}

Local<Message> TryCatch::Message() const
{
    Q_UNIMPLEMENTED();
    return Local<v8::Message>::New(Handle<v8::Message>(new v8::Message(QString(), QString(), 0)));
}

void TryCatch::Reset()
{
    hasCaughtException = false;
}



struct Context::Private
{
    Private()
    {
        engine.reset(new QV4::ExecutionEngine);
    }

    QScopedPointer<QV4::ExecutionEngine> engine;
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
    return Local<Object>::New(Value::fromV4Value(QV4::Value::fromObject(d->engine->globalObject)));
}

Local<Context> Context::GetCurrent()
{
    return Context::Adopt(Isolate::GetCurrent()->m_contextStack.top());
}

Local<Context> Context::GetCalling()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

Local<Object> Context::GetCallingQmlGlobal()
{
    QV4::ExecutionEngine *engine = GetCurrent()->GetEngine();
    QV4::ExecutionContext *ctx = engine->current;
    while (ctx && ctx->outer->type != ExecutionContext::Type_GlobalContext)
        ctx = ctx->outer;

    assert(ctx);
    if (!ctx->type == ExecutionContext::Type_QmlContext)
        return Local<Object>();

    return Local<Object>::New(Value::fromV4Value(QV4::Value::fromObject(static_cast<CallContext *>(ctx)->activation)));
}

Local<Value> Context::GetCallingScriptData()
{
    Q_UNIMPLEMENTED();
    Q_UNREACHABLE();
}

void Context::Enter()
{
    Isolate* iso = Isolate::GetCurrent();
    iso->m_contextStack.push(this);
}

void Context::Exit()
{
    Isolate::GetCurrent()->m_contextStack.pop();
}


QV4::ExecutionEngine *Context::GetEngine()
{
    return d->engine.data();
}


}
