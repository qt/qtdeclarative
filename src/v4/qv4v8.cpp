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

#define ValuePtr(obj) reinterpret_cast<QQmlJS::VM::Value*>(obj)
#define ConstValuePtr(obj) reinterpret_cast<const QQmlJS::VM::Value*>(obj)

bool Value::IsUndefined() const { return ConstValuePtr(this)->isUndefined(); }
bool Value::IsNull() const { return ConstValuePtr(this)->isNull(); }

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

bool Value::StrictEquals(Handle<Value> that) const
{
    return __qmljs_strict_equal(*ConstValuePtr(this), *ConstValuePtr(&that));
}

Local<Boolean> Value::ToBoolean() const
{
    return Local<Boolean>::New(Value::NewFromInternalValue(QQmlJS::VM::Value::fromBoolean(ConstValuePtr(this)->toBoolean()).val));
}

Local<Number> Value::ToNumber() const
{
    return Local<Number>::New(Value::NewFromInternalValue(QQmlJS::VM::Value::fromDouble(ConstValuePtr(this)->toNumber(currentEngine()->current)).val));
}

Local<String> Value::ToString() const
{
    return Local<String>::New(Value::NewFromInternalValue(QQmlJS::VM::Value::fromString(ConstValuePtr(this)->toString(currentEngine()->current)).val));
}

Local<Integer> Value::ToInteger() const
{
    return Local<Integer>::New(Value::NewFromInternalValue(QQmlJS::VM::Value::fromDouble(ConstValuePtr(this)->toInteger(currentEngine()->current)).val));
}

Local<Uint32> Value::ToUint32() const
{
    return Local<Uint32>::New(Value::NewFromInternalValue(QQmlJS::VM::Value::fromUInt32(ConstValuePtr(this)->toUInt32(currentEngine()->current)).val));
}

Local<Int32> Value::ToInt32() const
{
    return Local<Int32>::New(Value::NewFromInternalValue(QQmlJS::VM::Value::fromInt32(ConstValuePtr(this)->toInt32(currentEngine()->current)).val));
}

Local<Object> Value::ToObject() const
{
    return Local<Object>::New(Value::NewFromInternalValue(QQmlJS::VM::Value::fromObject(ConstValuePtr(this)->toObject(currentEngine()->current)).val));
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

bool Value::BooleanValue() const
{
    return ConstValuePtr(this)->toBoolean();
}

String::CompleteHashData String::CompleteHash() const
{
    CompleteHashData data;
    data.hash = asVMString()->hashValue();
    data.length = asQString().length();
    data.symbol_id = 0; // ###
    return data;
}

bool String::Equals(uint16_t *str, int length)
{
    return asQString() == QString(reinterpret_cast<QChar*>(str), length);
}

bool String::Equals(char *str, int length)
{
    return asQString() == QString::fromLatin1(str, length);
}

Local<String> String::New(const char *data, int length)
{
    QQmlJS::VM::Value v = QQmlJS::VM::Value::fromString(currentEngine()->current, QString::fromLatin1(data, length));
    return Local<String>::New(v8::Value::NewFromInternalValue(v.val));
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

double Number::Value() const
{
    const VM::Value *v = ConstValuePtr(this);
    assert(v->isNumber());
    return v->asDouble();
}

int64_t Integer::Value() const
{
    const VM::Value *v = ConstValuePtr(this);
    assert(v->isNumber());
    return (int64_t)v->asDouble();
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
    return Local<Object>::New(Value::NewFromInternalValue(d->engine->globalObject.val));
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

Local<Context> Context::GetCurrent()
{
    return Context::Adopt(Isolate::GetCurrent()->m_currentContext);
}

QQmlJS::VM::ExecutionEngine *Context::GetEngine()
{
    return d->engine.data();
}

static QThreadStorage<Isolate*> currentIsolate;

Isolate::Isolate()
    : m_lastIsolate(0)
{
}

Isolate::~Isolate()
{
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

Isolate *Isolate::GetCurrent()
{
    if (!currentIsolate.hasLocalData())
        currentIsolate.setLocalData(new Isolate);
    return currentIsolate.localData();
}

Local<Value> Object::Get(Handle<Value> key)
{
    Local<Value> result;
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    if (!o)
        return result;
    QQmlJS::VM::ExecutionContext *ctx = currentEngine()->current;
    QQmlJS::VM::Value prop = o->__get__(ctx, ValuePtr(&key)->toString(ctx));
    return Local<Value>::New(Value::NewFromInternalValue(prop.val));
}

Local<Value> Object::Get(uint32_t key)
{
    Local<Value> result;
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    if (!o)
        return result;
    QQmlJS::VM::ExecutionContext *ctx = currentEngine()->current;
    QQmlJS::VM::Value prop = o->__get__(ctx, key);
    return Local<Value>::New(Value::NewFromInternalValue(prop.val));
}

Local<Value> Object::GetPrototype()
{
    Local<Value> result;
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    if (!o)
        return result;
    return Local<Value>::New(Value::NewFromInternalValue(QQmlJS::VM::Value::fromObject(o->prototype).val));
}

bool Object::Has(Handle<String> key)
{
    QQmlJS::VM::Object *o = ConstValuePtr(this)->asObject();
    if (!o)
        return false;
    return o->__hasProperty__(currentEngine()->current, ValuePtr(&key)->asString());
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
    return Local<Value>::New(Value::NewFromInternalValue(result.val));
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
    return Local<Value>::New(Value::NewFromInternalValue(result.val));
}

ScriptOrigin::ScriptOrigin(Handle<Value> resource_name, Handle<Integer> resource_line_offset, Handle<Integer> resource_column_offset)
{
    m_fileName = resource_name->ToString()->asQString();
    m_lineNumber = resource_line_offset->ToInt32()->Value();
    m_columnNumber = resource_column_offset->ToInt32()->Value();
}

String::AsciiValue::AsciiValue(Handle<v8::Value> obj)
{
    str = obj->ToString()->asQString().toLatin1();
}

String::Value::Value(Handle<v8::Value> obj)
{
    str = obj->ToString()->asQString();
}


}
