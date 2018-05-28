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

#include "qv4global_p.h"
#include "qv4engine_p.h"
#include "qv4runtime_p.h"
#ifndef V4_BOOTSTRAP
#include "qv4object_p.h"
#include "qv4objectproto_p.h"
#include "qv4globalobject_p.h"
#include "qv4stringobject_p.h"
#include "qv4argumentsobject_p.h"
#include "qv4objectiterator_p.h"
#include "qv4dateobject_p.h"
#include "qv4lookup_p.h"
#include "qv4function_p.h"
#include "qv4numberobject_p.h"
#include "qv4regexp_p.h"
#include "qv4regexpobject_p.h"
#include "private/qlocale_tools_p.h"
#include "qv4scopedvalue_p.h"
#include "qv4jscall_p.h"
#include <private/qv4qmlcontext_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmljavascriptexpression_p.h>
#include "qv4qobjectwrapper_p.h"
#include <private/qv8engine_p.h>
#endif

#include <QtCore/QDebug>
#include <cassert>
#include <cstdio>
#include <stdlib.h>

#include <wtf/MathExtras.h>

#ifdef QV4_COUNT_RUNTIME_FUNCTIONS
#  include <QtCore/QBuffer>
#  include <QtCore/QDebug>
#endif // QV4_COUNT_RUNTIME_FUNCTIONS

QT_BEGIN_NAMESPACE

namespace QV4 {

#ifdef QV4_COUNT_RUNTIME_FUNCTIONS
struct RuntimeCounters::Data {
    enum Type {
        None = 0,
        Undefined = 1,
        Null = 2,
        Boolean = 3,
        Integer = 4,
        Managed = 5,
        Double = 7
    };

    static const char *pretty(Type t) {
        switch (t) {
        case None: return "";
        case Undefined: return "Undefined";
        case Null: return "Null";
        case Boolean: return "Boolean";
        case Integer: return "Integer";
        case Managed: return "Managed";
        case Double: return "Double";
        default: return "Unknown";
        }
    }

    static unsigned mangle(unsigned tag) {
        switch (tag) {
        case Value::Undefined_Type: return Undefined;
        case Value::Null_Type: return Null;
        case Value::Boolean_Type: return Boolean;
        case Value::Integer_Type: return Integer;
        case Value::Managed_Type: return Managed;
        default: return Double;
        }
    }

    static unsigned mangle(unsigned tag1, unsigned tag2) {
        return (mangle(tag1) << 3) | mangle(tag2);
    }

    static void unmangle(unsigned signature, Type &tag1, Type &tag2) {
        tag1 = Type((signature >> 3) & 7);
        tag2 = Type(signature & 7);
    }

    typedef QVector<quint64> Counters;
    QHash<const char *, Counters> counters;

    inline void count(const char *func) {
        QVector<quint64> &cnt = counters[func];
        if (cnt.isEmpty())
            cnt.resize(64);
        cnt[0] += 1;
    }

    inline void count(const char *func, unsigned tag) {
        QVector<quint64> &cnt = counters[func];
        if (cnt.isEmpty())
            cnt.resize(64);
        cnt[mangle(tag)] += 1;
    }

    inline void count(const char *func, unsigned tag1, unsigned tag2) {
        QVector<quint64> &cnt = counters[func];
        if (cnt.isEmpty())
            cnt.resize(64);
        cnt[mangle(tag1, tag2)] += 1;
    }

    struct Line {
        const char *func;
        Type tag1, tag2;
        quint64 count;

        static bool less(const Line &line1, const Line &line2) {
            return line1.count > line2.count;
        }
    };

    void dump() const {
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        QTextStream outs(&buf);
        QList<Line> lines;
        for (auto it = counters.cbegin(), end = counters.cend(); it != end; ++it) {
            const Counters &fCount = it.value();
            for (int i = 0, ei = fCount.size(); i != ei; ++i) {
                quint64 count = fCount[i];
                if (!count)
                    continue;
                Line line;
                line.func = it.key();
                unmangle(i, line.tag1, line.tag2);
                line.count = count;
                lines.append(line);
            }
        }
        std::sort(lines.begin(), lines.end(), Line::less);
        outs << lines.size() << " counters:" << endl;
        for (const Line &line : qAsConst(lines))
            outs << qSetFieldWidth(10) << line.count << qSetFieldWidth(0)
                 << " | " << line.func
                 << " | " << pretty(line.tag1)
                 << " | " << pretty(line.tag2)
                 << endl;
        qDebug("%s", buf.data().constData());
    }
};

RuntimeCounters *RuntimeCounters::instance = 0;
static RuntimeCounters runtimeCountersInstance;
RuntimeCounters::RuntimeCounters()
    : d(new Data)
{
    if (!instance)
        instance = this;
}

RuntimeCounters::~RuntimeCounters()
{
    d->dump();
    delete d;
}

void RuntimeCounters::count(const char *func)
{
    d->count(func);
}

void RuntimeCounters::count(const char *func, uint tag)
{
    d->count(func, tag);
}

void RuntimeCounters::count(const char *func, uint tag1, uint tag2)
{
    d->count(func, tag1, tag2);
}

#endif // QV4_COUNT_RUNTIME_FUNCTIONS

#ifndef V4_BOOTSTRAP

Runtime::Runtime()
{
#define INIT_METHOD(returnvalue, name, args) runtimeMethods[name] = reinterpret_cast<void*>(&method_##name);
FOR_EACH_RUNTIME_METHOD(INIT_METHOD)
#undef INIT_METHOD
}

void RuntimeHelpers::numberToString(QString *result, double num, int radix)
{
    Q_ASSERT(result);

    if (std::isnan(num)) {
        *result = QStringLiteral("NaN");
        return;
    } else if (qt_is_inf(num)) {
        *result = num < 0 ? QStringLiteral("-Infinity") : QStringLiteral("Infinity");
        return;
    }

    if (radix == 10) {
        // We cannot use our usual locale->toString(...) here, because EcmaScript has special rules
        // about the longest permissible number, depending on if it's <0 or >0.
        const int ecma_shortest_low = -6;
        const int ecma_shortest_high = 21;

        const QLatin1Char zero('0');
        const QLatin1Char dot('.');

        int decpt = 0;
        int sign = 0;
        *result = qdtoa(num, &decpt, &sign);

        if (decpt <= ecma_shortest_low || decpt > ecma_shortest_high) {
            if (result->length() > 1)
                result->insert(1, dot);
            result->append(QLatin1Char('e'));
            if (decpt > 0)
                result->append(QLatin1Char('+'));
            result->append(QString::number(decpt - 1));
        } else if (decpt <= 0) {
            result->prepend(QLatin1String("0.") + QString(-decpt, zero));
        } else if (decpt < result->length()) {
            result->insert(decpt, dot);
        } else {
            result->append(QString(decpt - result->length(), zero));
        }

        if (sign && num)
            result->prepend(QLatin1Char('-'));

        return;
    }

    result->clear();
    bool negative = false;

    if (num < 0) {
        negative = true;
        num = -num;
    }

    double frac = num - ::floor(num);
    num = Primitive::toInteger(num);

    do {
        char c = (char)::fmod(num, radix);
        c = (c < 10) ? (c + '0') : (c - 10 + 'a');
        result->prepend(QLatin1Char(c));
        num = ::floor(num / radix);
    } while (num != 0);

    if (frac != 0) {
        result->append(QLatin1Char('.'));
        do {
            frac = frac * radix;
            char c = (char)::floor(frac);
            c = (c < 10) ? (c + '0') : (c - 10 + 'a');
            result->append(QLatin1Char(c));
            frac = frac - ::floor(frac);
        } while (frac != 0);
    }

    if (negative)
        result->prepend(QLatin1Char('-'));
}

ReturnedValue Runtime::method_closure(ExecutionEngine *engine, int functionId)
{
    QV4::Function *clos = static_cast<CompiledData::CompilationUnit*>(engine->currentStackFrame->v4Function->compilationUnit)->runtimeFunctions[functionId];
    Q_ASSERT(clos);
    ExecutionContext *current = static_cast<ExecutionContext *>(&engine->currentStackFrame->jsFrame->context);
    return FunctionObject::createScriptFunction(current, clos)->asReturnedValue();
}

bool Runtime::method_deleteElement(ExecutionEngine *engine, const Value &base, const Value &index)
{
    Scope scope(engine);
    ScopedObject o(scope, base);
    if (o) {
        uint n = index.asArrayIndex();
        if (n < UINT_MAX)
            return o->deleteIndexedProperty(n);
    }

    ScopedString name(scope, index.toString(engine));
    return method_deleteMemberString(engine, base, name);
}

bool Runtime::method_deleteMember(ExecutionEngine *engine, const Value &base, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    return method_deleteMemberString(engine, base, name);
}

bool Runtime::method_deleteMemberString(ExecutionEngine *engine, const Value &base, String *name)
{
    Scope scope(engine);
    ScopedObject obj(scope, base.toObject(engine));
    if (scope.engine->hasException)
        return Encode::undefined();
    return obj->deleteProperty(name);
}

bool Runtime::method_deleteName(ExecutionEngine *engine, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    return static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context).deleteProperty(name);
}

QV4::ReturnedValue Runtime::method_instanceof(ExecutionEngine *engine, const Value &lval, const Value &rval)
{
    // 11.8.6, 5: rval must be an Object
    const Object *rhs = rval.as<Object>();
    if (!rhs)
       return engine->throwTypeError();

    // 11.8.6, 7: call "HasInstance", which we term instanceOf, and return the result.
    return rhs->instanceOf(lval);
}

QV4::ReturnedValue Runtime::method_in(ExecutionEngine *engine, const Value &left, const Value &right)
{
    Object *ro = right.objectValue();
    if (!ro)
        return engine->throwTypeError();
    Scope scope(engine);
    ScopedString s(scope, left.toString(engine));
    if (scope.hasException())
        return Encode::undefined();
    bool r = ro->hasProperty(s);
    return Encode(r);
}

double RuntimeHelpers::stringToNumber(const QString &string)
{
    const QStringRef s = QStringRef(&string).trimmed();
    if (s.startsWith(QLatin1String("0x")) || s.startsWith(QLatin1String("0X")))
        return s.toLong(nullptr, 16);
    bool ok;
    QByteArray ba = s.toLatin1();
    const char *begin = ba.constData();
    const char *end = nullptr;
    double d = qstrtod(begin, &end, &ok);
    if (end - begin != ba.size()) {
        if (ba == "Infinity" || ba == "+Infinity")
            d = Q_INFINITY;
        else if (ba == "-Infinity")
            d = -Q_INFINITY;
        else
            d = std::numeric_limits<double>::quiet_NaN();
    }
    return d;
}

Heap::String *RuntimeHelpers::stringFromNumber(ExecutionEngine *engine, double number)
{
    QString qstr;
    RuntimeHelpers::numberToString(&qstr, number, 10);
    return engine->newString(qstr);
}

ReturnedValue RuntimeHelpers::objectDefaultValue(const Object *object, int typeHint)
{
    if (typeHint == PREFERREDTYPE_HINT) {
        if (object->as<DateObject>())
            typeHint = STRING_HINT;
        else
            typeHint = NUMBER_HINT;
    }

    ExecutionEngine *engine = object->internalClass()->engine;
    if (engine->hasException)
        return Encode::undefined();

    String *meth1 = engine->id_toString();
    String *meth2 = engine->id_valueOf();

    if (typeHint == NUMBER_HINT)
        qSwap(meth1, meth2);

    Scope scope(engine);
    ScopedValue result(scope);
    ScopedValue conv(scope, object->get(meth1));

    if (FunctionObject *o = conv->as<FunctionObject>()) {
        result = o->call(object, nullptr, 0);
        if (result->isPrimitive())
            return result->asReturnedValue();
    }

    if (engine->hasException)
        return Encode::undefined();

    conv = object->get(meth2);
    if (FunctionObject *o = conv->as<FunctionObject>()) {
        result = o->call(object, nullptr, 0);
        if (result->isPrimitive())
            return result->asReturnedValue();
    }

    return engine->throwTypeError();
}



Heap::Object *RuntimeHelpers::convertToObject(ExecutionEngine *engine, const Value &value)
{
    Q_ASSERT(!value.isObject());
    switch (value.type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        engine->throwTypeError();
        return nullptr;
    case Value::Boolean_Type:
        return engine->newBooleanObject(value.booleanValue());
    case Value::Managed_Type:
        Q_ASSERT(value.isString());
        return engine->newStringObject(value.stringValue());
    case Value::Integer_Type:
    default: // double
        return engine->newNumberObject(value.asDouble());
    }
}

Heap::String *RuntimeHelpers::convertToString(ExecutionEngine *engine, Value value, TypeHint hint)
{
  redo:
    switch (value.type()) {
    case Value::Empty_Type:
        Q_ASSERT(!"empty Value encountered");
        Q_UNREACHABLE();
    case Value::Undefined_Type:
        return engine->id_undefined()->d();
    case Value::Null_Type:
        return engine->id_null()->d();
    case Value::Boolean_Type:
        if (value.booleanValue())
            return engine->id_true()->d();
        else
            return engine->id_false()->d();
    case Value::Managed_Type: {
        if (value.isString())
            return static_cast<const String &>(value).d();
        value = Primitive::fromReturnedValue(RuntimeHelpers::toPrimitive(value, hint));
        Q_ASSERT(value.isPrimitive());
        if (value.isString())
            return static_cast<const String &>(value).d();
        goto redo;
    }
    case Value::Integer_Type:
        return RuntimeHelpers::stringFromNumber(engine, value.int_32());
    default: // double
        return RuntimeHelpers::stringFromNumber(engine, value.doubleValue());
    } // switch
}

// This is slightly different from the method above, as
// the + operator requires a slightly different conversion
static Heap::String *convert_to_string_add(ExecutionEngine *engine, Value value)
{
    return RuntimeHelpers::convertToString(engine, value, PREFERREDTYPE_HINT);
}

QV4::ReturnedValue RuntimeHelpers::addHelper(ExecutionEngine *engine, const Value &left, const Value &right)
{
    Scope scope(engine);

    ScopedValue pleft(scope, RuntimeHelpers::toPrimitive(left, PREFERREDTYPE_HINT));
    ScopedValue pright(scope, RuntimeHelpers::toPrimitive(right, PREFERREDTYPE_HINT));
    String *sleft = pleft->stringValue();
    String *sright = pright->stringValue();
    if (sleft || sright) {
        if (!sleft) {
            pleft = convert_to_string_add(engine, pleft);
            sleft = static_cast<String *>(pleft.ptr);
        }
        if (!sright) {
            pright = convert_to_string_add(engine, pright);
            sright = static_cast<String *>(pright.ptr);
        }
        if (engine->hasException)
            return Encode::undefined();
        if (!sleft->d()->length())
            return sright->asReturnedValue();
        if (!sright->d()->length())
            return sleft->asReturnedValue();
        MemoryManager *mm = engine->memoryManager;
        return (mm->alloc<ComplexString>(sleft->d(), sright->d()))->asReturnedValue();
    }
    double x = RuntimeHelpers::toNumber(pleft);
    double y = RuntimeHelpers::toNumber(pright);
    return Encode(x + y);
}

bool Runtime::method_storeProperty(ExecutionEngine *engine, const Value &object, int nameIndex, const Value &value)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    ScopedObject o(scope, object.toObject(engine));
    if (!o)
        return false;
    return o->put(name, value);
}

static Q_NEVER_INLINE ReturnedValue getElementIntFallback(ExecutionEngine *engine, const Value &object, uint idx)
{
    Q_ASSERT(idx < UINT_MAX);
    Scope scope(engine);

    ScopedObject o(scope, object);
    if (!o) {
        if (const String *str = object.as<String>()) {
            if (idx >= (uint)str->toQString().length()) {
                return Encode::undefined();
            }
            const QString s = str->toQString().mid(idx, 1);
            return scope.engine->newString(s)->asReturnedValue();
        }

        if (object.isNullOrUndefined()) {
            QString message = QStringLiteral("Cannot read property '%1' of %2").arg(idx).arg(object.toQStringNoThrow());
            return engine->throwTypeError(message);
        }

        o = RuntimeHelpers::convertToObject(scope.engine, object);
        Q_ASSERT(!!o); // can't fail as null/undefined is covered above
    }

    if (o->arrayData() && !o->arrayData()->attrs) {
        ScopedValue v(scope, o->arrayData()->get(idx));
        if (!v->isEmpty())
            return v->asReturnedValue();
    }

    return o->getIndexed(idx);
}

static Q_NEVER_INLINE ReturnedValue getElementFallback(ExecutionEngine *engine, const Value &object, const Value &index)
{
    Q_ASSERT(index.asArrayIndex() == UINT_MAX);
    Scope scope(engine);

    ScopedObject o(scope, object);
    if (!o) {
        if (object.isNullOrUndefined()) {
            QString message = QStringLiteral("Cannot read property '%1' of %2").arg(index.toQStringNoThrow()).arg(object.toQStringNoThrow());
            return engine->throwTypeError(message);
        }

        o = RuntimeHelpers::convertToObject(scope.engine, object);
        Q_ASSERT(!!o); // can't fail as null/undefined is covered above
    }

    ScopedString name(scope, index.toString(engine));
    if (scope.hasException())
        return Encode::undefined();
    return o->get(name);
}

/* load element:

  Managed *m = object.heapObject();
  if (m)
     return m->internalClass->getIndexed(m, index);
  return getIndexedFallback(object, index);
*/

ReturnedValue Runtime::method_loadElement(ExecutionEngine *engine, const Value &object, const Value &index)
{
    uint idx = 0;
    if (index.asArrayIndex(idx)) {
        if (Heap::Base *b = object.heapObject()) {
            if (b->vtable()->isObject) {
                Heap::Object *o = static_cast<Heap::Object *>(b);
                if (o->arrayData && o->arrayData->type == Heap::ArrayData::Simple) {
                    Heap::SimpleArrayData *s = o->arrayData.cast<Heap::SimpleArrayData>();
                    if (idx < s->values.size)
                        if (!s->data(idx).isEmpty())
                            return s->data(idx).asReturnedValue();
                }
            }
        }
        return getElementIntFallback(engine, object, idx);
    }

    return getElementFallback(engine, object, index);
}

static Q_NEVER_INLINE bool setElementFallback(ExecutionEngine *engine, const Value &object, const Value &index, const Value &value)
{
    Scope scope(engine);
    ScopedObject o(scope, object.toObject(engine));
    if (engine->hasException)
        return false;

    uint idx = 0;
    if (index.asArrayIndex(idx)) {
        if (o->d()->arrayData && o->d()->arrayData->type == Heap::ArrayData::Simple) {
            Heap::SimpleArrayData *s = o->d()->arrayData.cast<Heap::SimpleArrayData>();
            if (idx < s->values.size) {
                s->setData(engine, idx, value);
                return true;
            }
        }
        return o->putIndexed(idx, value);
    }

    ScopedString name(scope, index.toString(engine));
    return o->put(name, value);
}

bool Runtime::method_storeElement(ExecutionEngine *engine, const Value &object, const Value &index, const Value &value)
{
    uint idx = 0;
    if (index.asArrayIndex(idx)) {
        if (Heap::Base *b = object.heapObject()) {
            if (b->vtable()->isObject) {
                Heap::Object *o = static_cast<Heap::Object *>(b);
                if (o->arrayData && o->arrayData->type == Heap::ArrayData::Simple) {
                    Heap::SimpleArrayData *s = o->arrayData.cast<Heap::SimpleArrayData>();
                    if (idx < s->values.size) {
                        s->setData(engine, idx, value);
                        return true;
                    }
                }
            }
        }
    }

    return setElementFallback(engine, object, index, value);
}

ReturnedValue Runtime::method_foreachIterator(ExecutionEngine *engine, const Value &in)
{
    Scope scope(engine);
    ScopedObject o(scope, (Object *)nullptr);
    if (!in.isNullOrUndefined())
        o = in.toObject(engine);
    return engine->newForEachIteratorObject(o)->asReturnedValue();
}

ReturnedValue Runtime::method_foreachNextPropertyName(const Value &foreach_iterator)
{
    Q_ASSERT(foreach_iterator.isObject());

    ForEachIteratorObject *it = static_cast<ForEachIteratorObject *>(foreach_iterator.objectValue());
    Q_ASSERT(it->as<ForEachIteratorObject>());

    return it->nextPropertyName();
}


void Runtime::method_storeNameSloppy(ExecutionEngine *engine, int nameIndex, const Value &value)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    ExecutionContext::Error e = static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context).setProperty(name, value);

    if (e == ExecutionContext::RangeError)
        engine->globalObject->put(name, value);
}

void Runtime::method_storeNameStrict(ExecutionEngine *engine, int nameIndex, const Value &value)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    ExecutionContext::Error e = static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context).setProperty(name, value);
    if (e == ExecutionContext::TypeError)
        engine->throwTypeError();
    else if (e == ExecutionContext::RangeError)
        engine->throwReferenceError(name);
}

ReturnedValue Runtime::method_loadProperty(ExecutionEngine *engine, const Value &object, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);

    ScopedObject o(scope, object);
    if (o)
        return o->get(name);

    if (object.isNullOrUndefined()) {
        QString message = QStringLiteral("Cannot read property '%1' of %2").arg(name->toQString()).arg(object.toQStringNoThrow());
        return engine->throwTypeError(message);
    }

    o = RuntimeHelpers::convertToObject(scope.engine, object);
    if (!o) // type error
        return Encode::undefined();
    return o->get(name);
}

ReturnedValue Runtime::method_loadName(ExecutionEngine *engine, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    return static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context).getProperty(name);
}

#endif // V4_BOOTSTRAP

uint RuntimeHelpers::equalHelper(const Value &x, const Value &y)
{
    Q_ASSERT(x.type() != y.type() || (x.isManaged() && (x.isString() != y.isString())));

    if (x.isNumber() && y.isNumber())
        return x.asDouble() == y.asDouble();
    if (x.isNull() && y.isUndefined()) {
        return true;
    } else if (x.isUndefined() && y.isNull()) {
        return true;
    } else if (x.isNumber() && y.isString()) {
        double dy = RuntimeHelpers::toNumber(y);
        return x.asDouble() == dy;
    } else if (x.isString() && y.isNumber()) {
        double dx = RuntimeHelpers::toNumber(x);
        return dx == y.asDouble();
    } else if (x.isBoolean()) {
        return Runtime::method_compareEqual(Primitive::fromDouble((double) x.booleanValue()), y);
    } else if (y.isBoolean()) {
        return Runtime::method_compareEqual(x, Primitive::fromDouble((double) y.booleanValue()));
    } else {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
#else
        Object *xo = x.objectValue();
        Object *yo = y.objectValue();
        if (yo && (x.isNumber() || x.isString())) {
            Scope scope(yo->engine());
            ScopedValue py(scope, RuntimeHelpers::objectDefaultValue(yo, PREFERREDTYPE_HINT));
            return Runtime::method_compareEqual(x, py);
        } else if (xo && (y.isNumber() || y.isString())) {
            Scope scope(xo->engine());
            ScopedValue px(scope, RuntimeHelpers::objectDefaultValue(xo, PREFERREDTYPE_HINT));
            return Runtime::method_compareEqual(px, y);
        }
#endif
    }

    return false;
}

Bool RuntimeHelpers::strictEqual(const Value &x, const Value &y)
{
    TRACE2(x, y);

    if (x.rawValue() == y.rawValue())
        // NaN != NaN
        return !x.isNaN();

    if (x.isNumber())
        return y.isNumber() && x.asDouble() == y.asDouble();
    if (x.isManaged())
        return y.isManaged() && x.cast<Managed>()->isEqualTo(y.cast<Managed>());
    return false;
}

QV4::Bool Runtime::method_compareGreaterThan(const Value &l, const Value &r)
{
    TRACE2(l, r);
    if (l.isInteger() && r.isInteger())
        return l.integerValue() > r.integerValue();
    if (l.isNumber() && r.isNumber())
        return l.asDouble() > r.asDouble();
    String *sl = l.stringValue();
    String *sr = r.stringValue();
    if (sl && sr) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
        return false;
#else
        return sr->compare(sl);
#endif
    }

    Object *ro = r.objectValue();
    Object *lo = l.objectValue();
    if (ro || lo) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
#else
        QV4::ExecutionEngine *e = (lo ? lo : ro)->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, lo ? RuntimeHelpers::objectDefaultValue(lo, QV4::NUMBER_HINT) : l.asReturnedValue());
        QV4::ScopedValue pr(scope, ro ? RuntimeHelpers::objectDefaultValue(ro, QV4::NUMBER_HINT) : r.asReturnedValue());
        return Runtime::method_compareGreaterThan(pl, pr);
#endif
    }

    double dl = RuntimeHelpers::toNumber(l);
    double dr = RuntimeHelpers::toNumber(r);
    return dl > dr;
}

QV4::Bool Runtime::method_compareLessThan(const Value &l, const Value &r)
{
    TRACE2(l, r);
    if (l.isInteger() && r.isInteger())
        return l.integerValue() < r.integerValue();
    if (l.isNumber() && r.isNumber())
        return l.asDouble() < r.asDouble();
    String *sl = l.stringValue();
    String *sr = r.stringValue();
    if (sl && sr) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
        return false;
#else
        return sl->compare(sr);
#endif
    }

    Object *ro = r.objectValue();
    Object *lo = l.objectValue();
    if (ro || lo) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
#else
        QV4::ExecutionEngine *e = (lo ? lo : ro)->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, lo ? RuntimeHelpers::objectDefaultValue(lo, QV4::NUMBER_HINT) : l.asReturnedValue());
        QV4::ScopedValue pr(scope, ro ? RuntimeHelpers::objectDefaultValue(ro, QV4::NUMBER_HINT) : r.asReturnedValue());
        return Runtime::method_compareLessThan(pl, pr);
#endif
    }

    double dl = RuntimeHelpers::toNumber(l);
    double dr = RuntimeHelpers::toNumber(r);
    return dl < dr;
}

QV4::Bool Runtime::method_compareGreaterEqual(const Value &l, const Value &r)
{
    TRACE2(l, r);
    if (l.isInteger() && r.isInteger())
        return l.integerValue() >= r.integerValue();
    if (l.isNumber() && r.isNumber())
        return l.asDouble() >= r.asDouble();
    String *sl = l.stringValue();
    String *sr = r.stringValue();
    if (sl && sr) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
        return false;
#else
        return !sl->compare(sr);
#endif
    }

    Object *ro = r.objectValue();
    Object *lo = l.objectValue();
    if (ro || lo) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
#else
        QV4::ExecutionEngine *e = (lo ? lo : ro)->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, lo ? RuntimeHelpers::objectDefaultValue(lo, QV4::NUMBER_HINT) : l.asReturnedValue());
        QV4::ScopedValue pr(scope, ro ? RuntimeHelpers::objectDefaultValue(ro, QV4::NUMBER_HINT) : r.asReturnedValue());
        return Runtime::method_compareGreaterEqual(pl, pr);
#endif
    }

    double dl = RuntimeHelpers::toNumber(l);
    double dr = RuntimeHelpers::toNumber(r);
    return dl >= dr;
}

QV4::Bool Runtime::method_compareLessEqual(const Value &l, const Value &r)
{
    TRACE2(l, r);
    if (l.isInteger() && r.isInteger())
        return l.integerValue() <= r.integerValue();
    if (l.isNumber() && r.isNumber())
        return l.asDouble() <= r.asDouble();
    String *sl = l.stringValue();
    String *sr = r.stringValue();
    if (sl && sr) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
        return false;
#else
        return !sr->compare(sl);
#endif
    }

    Object *ro = r.objectValue();
    Object *lo = l.objectValue();
    if (ro || lo) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
#else
        QV4::ExecutionEngine *e = (lo ? lo : ro)->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, lo ? RuntimeHelpers::objectDefaultValue(lo, QV4::NUMBER_HINT) : l.asReturnedValue());
        QV4::ScopedValue pr(scope, ro ? RuntimeHelpers::objectDefaultValue(ro, QV4::NUMBER_HINT) : r.asReturnedValue());
        return Runtime::method_compareLessEqual(pl, pr);
#endif
    }

    double dl = RuntimeHelpers::toNumber(l);
    double dr = RuntimeHelpers::toNumber(r);
    return dl <= dr;
}

#ifndef V4_BOOTSTRAP
Bool Runtime::method_compareInstanceof(ExecutionEngine *engine, const Value &left, const Value &right)
{
    TRACE2(left, right);

    Scope scope(engine);
    ScopedValue v(scope, method_instanceof(engine, left, right));
    return v->booleanValue();
}

uint Runtime::method_compareIn(ExecutionEngine *engine, const Value &left, const Value &right)
{
    TRACE2(left, right);

    Scope scope(engine);
    ScopedValue v(scope, method_in(engine, left, right));
    return v->booleanValue();
}


ReturnedValue Runtime::method_callGlobalLookup(ExecutionEngine *engine, uint index, Value *argv, int argc)
{
    Lookup *l = engine->currentStackFrame->v4Function->compilationUnit->runtimeLookups + index;
    Value function = Value::fromReturnedValue(l->globalGetter(l, engine));
    if (!function.isFunctionObject())
        return engine->throwTypeError();

    Value thisObject = Primitive::undefinedValue();
    return static_cast<FunctionObject &>(function).call(&thisObject, argv, argc);
}

ReturnedValue Runtime::method_callPossiblyDirectEval(ExecutionEngine *engine, Value *argv, int argc)
{
    Scope scope(engine);
    ScopedValue thisObject(scope);

    ExecutionContext &ctx = static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context);
    ScopedFunctionObject function(scope, ctx.getPropertyAndBase(engine->id_eval(), thisObject));
    if (engine->hasException)
        return Encode::undefined();

    if (!function) {
        QString objectAsString = QStringLiteral("[null]");
        if (!thisObject->isUndefined())
            objectAsString = thisObject->toQStringNoThrow();
        QString msg = QStringLiteral("Property 'eval' of object %2 is not a function").arg(objectAsString);
        return engine->throwTypeError(msg);
    }

    if (function->d() == engine->evalFunction()->d())
        return static_cast<EvalFunction *>(function.getPointer())->evalCall(thisObject, argv, argc, true);

    return function->call(thisObject, argv, argc);
}

ReturnedValue Runtime::method_callName(ExecutionEngine *engine, int nameIndex, Value *argv, int argc)
{
    Scope scope(engine);
    ScopedValue thisObject(scope);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);

    ExecutionContext &ctx = static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context);
    ScopedFunctionObject f(scope, ctx.getPropertyAndBase(name, thisObject));
    if (engine->hasException)
        return Encode::undefined();

    if (!f) {
        QString objectAsString = QStringLiteral("[null]");
        if (!thisObject->isUndefined())
            objectAsString = thisObject->toQStringNoThrow();
        QString msg = QStringLiteral("Property '%1' of object %2 is not a function")
                .arg(engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]->toQString(),
                     objectAsString);
        return engine->throwTypeError(msg);
    }

    return f->call(thisObject, argv, argc);
}

ReturnedValue Runtime::method_callProperty(ExecutionEngine *engine, Value *base, int nameIndex, Value *argv, int argc)
{
    Scope scope(engine);

    if (!base->isObject()) {
        Q_ASSERT(!base->isEmpty());
        if (base->isNullOrUndefined()) {
            QString message = QStringLiteral("Cannot call method '%1' of %2")
                    .arg(engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]->toQString(),
                         base->toQStringNoThrow());
            return engine->throwTypeError(message);
        }

        ScopedValue thisObject(scope, RuntimeHelpers::convertToObject(engine, *base));
        if (engine->hasException) // type error
            return Encode::undefined();
        base = thisObject;
    }

    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    ScopedFunctionObject f(scope, static_cast<Object *>(base)->get(name));

    if (!f) {
        QString error = QStringLiteral("Property '%1' of object %2 is not a function")
                .arg(engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]->toQString(),
                     base->toQStringNoThrow());
        return engine->throwTypeError(error);
    }

    return f->call(base, argv, argc);
}

ReturnedValue Runtime::method_callPropertyLookup(ExecutionEngine *engine, Value *base, uint index, Value *argv, int argc)
{
    Lookup *l = engine->currentStackFrame->v4Function->compilationUnit->runtimeLookups + index;
    // ok to have the value on the stack here
    Value f = Value::fromReturnedValue(l->getter(l, engine, *base));

    if (!f.isFunctionObject())
        return engine->throwTypeError();

    return static_cast<FunctionObject &>(f).call(base, argv, argc);
}

ReturnedValue Runtime::method_callElement(ExecutionEngine *engine, Value *base, const Value &index, Value *argv, int argc)
{
    Scope scope(engine);
    ScopedValue thisObject(scope, base->toObject(engine));
    base = thisObject;

    ScopedString str(scope, index.toString(engine));
    if (engine->hasException)
        return Encode::undefined();

    ScopedFunctionObject f(scope, static_cast<Object *>(base)->get(str));
    if (!f)
        return engine->throwTypeError();

    return f->call(base, argv, argc);
}

ReturnedValue Runtime::method_callValue(ExecutionEngine *engine, const Value &func, Value *argv, int argc)
{
    if (!func.isFunctionObject())
        return engine->throwTypeError(QStringLiteral("%1 is not a function").arg(func.toQStringNoThrow()));
    return static_cast<const FunctionObject &>(func).call(nullptr, argv, argc);
}

ReturnedValue Runtime::method_callQmlScopeObjectProperty(ExecutionEngine *engine, Value *base,
                                                         int propertyIndex, Value *argv, int argc)
{
    Scope scope(engine);
    ScopedFunctionObject fo(scope, method_loadQmlScopeObjectProperty(engine, *base, propertyIndex,
                                                                     /*captureRequired*/true));
    if (!fo) {
        QString error = QStringLiteral("Property '%1' of scope object is not a function").arg(propertyIndex);
        return engine->throwTypeError(error);
    }

    QObject *qmlScopeObj = static_cast<QmlContext *>(base)->d()->qml()->scopeObject;
    ScopedValue qmlScopeValue(scope, QObjectWrapper::wrap(engine, qmlScopeObj));
    return fo->call(qmlScopeValue, argv, argc);
}

ReturnedValue Runtime::method_callQmlContextObjectProperty(ExecutionEngine *engine, Value *base,
                                                           int propertyIndex, Value *argv, int argc)
{
    Scope scope(engine);
    ScopedFunctionObject fo(scope, method_loadQmlContextObjectProperty(engine, *base, propertyIndex,
                                                                       /*captureRequired*/true));
    if (!fo) {
        QString error = QStringLiteral("Property '%1' of context object is not a function").arg(propertyIndex);
        return engine->throwTypeError(error);
    }

    QObject *qmlContextObj = static_cast<QmlContext *>(base)->d()->qml()->context->contextData()->contextObject;
    ScopedValue qmlContextValue(scope, QObjectWrapper::wrap(engine, qmlContextObj));
    return fo->call(qmlContextValue, argv, argc);
}

ReturnedValue Runtime::method_construct(ExecutionEngine *engine, const Value &function, Value *argv, int argc)
{
    if (!function.isFunctionObject())
        return engine->throwTypeError();

    return static_cast<const FunctionObject &>(function).callAsConstructor(argv, argc);
}

void Runtime::method_throwException(ExecutionEngine *engine, const Value &value)
{
    if (!value.isEmpty())
        engine->throwError(value);
}

ReturnedValue Runtime::method_typeofValue(ExecutionEngine *engine, const Value &value)
{
    Scope scope(engine);
    ScopedString res(scope);
    switch (value.type()) {
    case Value::Undefined_Type:
        res = engine->id_undefined();
        break;
    case Value::Null_Type:
        res = engine->id_object();
        break;
    case Value::Boolean_Type:
        res = engine->id_boolean();
        break;
    case Value::Managed_Type:
        if (value.isString())
            res = engine->id_string();
        else if (value.objectValue()->as<FunctionObject>())
            res = engine->id_function();
        else
            res = engine->id_object(); // ### implementation-defined
        break;
    default:
        res = engine->id_number();
        break;
    }
    return res.asReturnedValue();
}

QV4::ReturnedValue Runtime::method_typeofName(ExecutionEngine *engine, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    ScopedValue prop(scope, static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context).getProperty(name));
    // typeof doesn't throw. clear any possible exception
    scope.engine->hasException = false;
    return method_typeofValue(engine, prop);
}

/* The next three methods are a bit tricky. They can't open up a Scope, as that
 * would mess up the pushing of the context.
 *
 * Instead the push/pop pair acts as a non local scope.
 */
ReturnedValue Runtime::method_createWithContext(ExecutionContext *parent, const Value &o)
{
    Q_ASSERT(o.isObject());
    const Object &obj = static_cast<const Object &>(o);
    return parent->newWithContext(obj.d())->asReturnedValue();
}

ReturnedValue Runtime::method_createCatchContext(ExecutionContext *parent, int exceptionVarNameIndex)
{
    ExecutionEngine *e = parent->engine();
    return parent->newCatchContext(e->currentStackFrame->v4Function->compilationUnit->runtimeStrings[exceptionVarNameIndex],
                                   e->catchException(nullptr))->asReturnedValue();
}

void Runtime::method_declareVar(ExecutionEngine *engine, bool deletable, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context).createMutableBinding(name, deletable);
}

ReturnedValue Runtime::method_arrayLiteral(ExecutionEngine *engine, Value *values, uint length)
{
    return engine->newArrayObject(values, length)->asReturnedValue();
}

ReturnedValue Runtime::method_objectLiteral(ExecutionEngine *engine, const QV4::Value *args, int classId, int arrayValueCount, int arrayGetterSetterCountAndFlags)
{
    Scope scope(engine);
    QV4::InternalClass *klass = static_cast<CompiledData::CompilationUnit*>(engine->currentStackFrame->v4Function->compilationUnit)->runtimeClasses[classId];
    ScopedObject o(scope, engine->newObject(klass, engine->objectPrototype()));

    {
        bool needSparseArray = arrayGetterSetterCountAndFlags >> 30;
        if (needSparseArray)
            o->initSparseArray();
    }

    for (uint i = 0; i < klass->size; ++i)
        o->setProperty(i, *args++);

    if (arrayValueCount > 0) {
        ScopedValue entry(scope);
        for (int i = 0; i < arrayValueCount; ++i) {
            uint idx = args->toUInt32();
            ++args;
            entry = *args++;
            o->arraySet(idx, entry);
        }
    }

    uint arrayGetterSetterCount = arrayGetterSetterCountAndFlags & ((1 << 30) - 1);
    if (arrayGetterSetterCount > 0) {
        ScopedProperty pd(scope);
        for (uint i = 0; i < arrayGetterSetterCount; ++i) {
            uint idx = args->toUInt32();
            ++args;
            pd->value = *args;
            ++args;
            pd->set = *args;
            ++args;
            o->arraySet(idx, pd, Attr_Accessor);
        }
    }

    return o.asReturnedValue();
}

QV4::ReturnedValue Runtime::method_createMappedArgumentsObject(ExecutionEngine *engine)
{
    Q_ASSERT(engine->currentContext()->d()->type == Heap::ExecutionContext::Type_CallContext);
    QV4::InternalClass *ic = engine->internalClasses[EngineBase::Class_ArgumentsObject];
    return engine->memoryManager->allocObject<ArgumentsObject>(ic, engine->objectPrototype(), engine->currentStackFrame)->asReturnedValue();
}

QV4::ReturnedValue Runtime::method_createUnmappedArgumentsObject(ExecutionEngine *engine)
{
    QV4::InternalClass *ic = engine->internalClasses[EngineBase::Class_StrictArgumentsObject];
    return engine->memoryManager->allocObject<StrictArgumentsObject>(ic, engine->objectPrototype(), engine->currentStackFrame)->asReturnedValue();
}

ReturnedValue Runtime::method_loadQmlContext(NoThrowEngine *engine)
{
    Heap::QmlContext *ctx = engine->qmlContext();
    Q_ASSERT(ctx);
    return ctx->asReturnedValue();
}

ReturnedValue Runtime::method_regexpLiteral(ExecutionEngine *engine, int id)
{
    Heap::RegExpObject *ro = engine->newRegExpObject(engine->currentStackFrame->v4Function->compilationUnit->runtimeRegularExpressions[id].as<RegExp>());
    return ro->asReturnedValue();
}

ReturnedValue Runtime::method_loadQmlScopeObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex, bool captureRequired)
{
    const QmlContext &c = static_cast<const QmlContext &>(context);
    return QV4::QObjectWrapper::getProperty(engine, c.d()->qml()->scopeObject, propertyIndex, captureRequired);
}

ReturnedValue Runtime::method_loadQmlContextObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex, bool captureRequired)
{
    const QmlContext &c = static_cast<const QmlContext &>(context);
    return QV4::QObjectWrapper::getProperty(engine, (*c.d()->qml()->context)->contextObject, propertyIndex, captureRequired);
}

ReturnedValue Runtime::method_loadQmlIdObject(ExecutionEngine *engine, const Value &c, uint index)
{
    const QmlContext &qmlContext = static_cast<const QmlContext &>(c);
    QQmlContextData *context = *qmlContext.d()->qml()->context;
    if (!context || index >= (uint)context->idValueCount)
        return Encode::undefined();

    QQmlEnginePrivate *ep = engine->qmlEngine() ? QQmlEnginePrivate::get(engine->qmlEngine()) : nullptr;
    if (ep && ep->propertyCapture)
        ep->propertyCapture->captureProperty(&context->idValues[index].bindings);

    return QObjectWrapper::wrap(engine, context->idValues[index].data());
}

void Runtime::method_storeQmlScopeObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex, const Value &value)
{
    const QmlContext &c = static_cast<const QmlContext &>(context);
    return QV4::QObjectWrapper::setProperty(engine, c.d()->qml()->scopeObject, propertyIndex, value);
}

void Runtime::method_storeQmlContextObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex, const Value &value)
{
    const QmlContext &c = static_cast<const QmlContext &>(context);
    return QV4::QObjectWrapper::setProperty(engine, (*c.d()->qml()->context)->contextObject, propertyIndex, value);
}

ReturnedValue Runtime::method_loadQmlImportedScripts(NoThrowEngine *engine)
{
    QQmlContextData *context = engine->callingQmlContext();
    if (!context)
        return Encode::undefined();
    return context->importedScripts.value();
}

ReturnedValue Runtime::method_uMinus(const Value &value)
{
    TRACE1(value);

    // +0 != -0, so we need to convert to double when negating 0
    if (value.isInteger() && value.integerValue() &&
            value.integerValue() != std::numeric_limits<int>::min())
        return Encode(-value.integerValue());
    else {
        double n = RuntimeHelpers::toNumber(value);
        return Encode(-n);
    }
}

// binary operators

#ifndef V4_BOOTSTRAP
ReturnedValue Runtime::method_add(ExecutionEngine *engine, const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Q_LIKELY(left.integerCompatible() && right.integerCompatible()))
        return add_int32(left.integerValue(), right.integerValue());
    if (left.isNumber() && right.isNumber())
        return Primitive::fromDouble(left.asDouble() + right.asDouble()).asReturnedValue();

    return RuntimeHelpers::addHelper(engine, left, right);
}
#endif // V4_BOOTSTRAP

ReturnedValue Runtime::method_sub(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Q_LIKELY(left.integerCompatible() && right.integerCompatible()))
        return sub_int32(left.integerValue(), right.integerValue());

    double lval = left.isNumber() ? left.asDouble() : left.toNumberImpl();
    double rval = right.isNumber() ? right.asDouble() : right.toNumberImpl();

    return Primitive::fromDouble(lval - rval).asReturnedValue();
}

ReturnedValue Runtime::method_mul(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Q_LIKELY(left.integerCompatible() && right.integerCompatible()))
        return mul_int32(left.integerValue(), right.integerValue());

    double lval = left.isNumber() ? left.asDouble() : left.toNumberImpl();
    double rval = right.isNumber() ? right.asDouble() : right.toNumberImpl();

    return Primitive::fromDouble(lval * rval).asReturnedValue();
}

ReturnedValue Runtime::method_div(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right)) {
        int lval = left.integerValue();
        int rval = right.integerValue();
        if (rval != 0 // division by zero should result in a NaN
                && (lval % rval == 0)  // fractions can't be stored in an int
                && !(lval == 0 && rval < 0)) // 0 / -something results in -0.0
            return Encode(int(lval / rval));
        else
            return Encode(double(lval) / rval);
    }

    double lval = left.toNumber();
    double rval = right.toNumber();
    return Primitive::fromDouble(lval / rval).asReturnedValue();
}

ReturnedValue Runtime::method_mod(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right) && left.integerValue() > 0 && right.integerValue() > 0) {
        int intRes = left.integerValue() % right.integerValue();
        if (intRes != 0 || left.integerValue() >= 0)
            return Encode(intRes);
    }

    double lval = RuntimeHelpers::toNumber(left);
    double rval = RuntimeHelpers::toNumber(right);
#ifdef fmod
#  undef fmod
#endif
    return Primitive::fromDouble(std::fmod(lval, rval)).asReturnedValue();
}

ReturnedValue Runtime::method_shl(const Value &left, const Value &right)
{
    TRACE2(left, right);

    int lval = left.toInt32();
    int rval = right.toInt32() & 0x1f;
    return Encode((int)(lval << rval));
}

ReturnedValue Runtime::method_shr(const Value &left, const Value &right)
{
    TRACE2(left, right);

    int lval = left.toInt32();
    unsigned rval = right.toUInt32() & 0x1f;
    return Encode((int)(lval >> rval));
}

ReturnedValue Runtime::method_ushr(const Value &left, const Value &right)
{
    TRACE2(left, right);

    unsigned lval = left.toUInt32();
    unsigned rval = right.toUInt32() & 0x1f;
    uint res = lval >> rval;

    return Encode(res);
}

#endif // V4_BOOTSTRAP

ReturnedValue Runtime::method_greaterThan(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = method_compareGreaterThan(left, right);
    return Encode(r);
}

ReturnedValue Runtime::method_lessThan(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = method_compareLessThan(left, right);
    return Encode(r);
}

ReturnedValue Runtime::method_greaterEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = method_compareGreaterEqual(left, right);
    return Encode(r);
}

ReturnedValue Runtime::method_lessEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = method_compareLessEqual(left, right);
    return Encode(r);
}

Bool Runtime::method_compareEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (left.rawValue() == right.rawValue())
        // NaN != NaN
        return !left.isNaN();

    if (left.type() == right.type()) {
        if (left.isDouble() && left.doubleValue() == 0 && right.doubleValue() == 0)
            return true; // this takes care of -0 == +0 (which obviously have different raw values)
        if (!left.isManaged())
            return false;
        if (left.isString() == right.isString())
            return left.cast<Managed>()->isEqualTo(right.cast<Managed>());
    }

    return RuntimeHelpers::equalHelper(left, right);
}

ReturnedValue Runtime::method_equal(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = method_compareEqual(left, right);
    return Encode(r);
}

ReturnedValue Runtime::method_notEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = !method_compareEqual(left, right);
    return Encode(r);
}

ReturnedValue Runtime::method_strictEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = RuntimeHelpers::strictEqual(left, right);
    return Encode(r);
}

ReturnedValue Runtime::method_strictNotEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = ! RuntimeHelpers::strictEqual(left, right);
    return Encode(r);
}

Bool Runtime::method_compareNotEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    return !Runtime::method_compareEqual(left, right);
}

Bool Runtime::method_compareStrictEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    return RuntimeHelpers::strictEqual(left, right);
}

Bool Runtime::method_compareStrictNotEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    return ! RuntimeHelpers::strictEqual(left, right);
}

} // namespace QV4

QT_END_NAMESPACE
