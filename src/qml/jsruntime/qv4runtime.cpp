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
#include "private/qlocale_tools_p.h"
#include "qv4scopedvalue_p.h"
#include <private/qqmlcontextwrapper_p.h>
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
        foreach (const Line &line, lines)
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
            result->prepend(QString::fromLatin1("0.%1").arg(QString().fill(zero, -decpt)));
        } else if (decpt < result->length()) {
            result->insert(decpt, dot);
        } else {
            result->append(QString().fill(zero, decpt - result->length()));
        }

        if (sign)
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

ReturnedValue Runtime::closure(ExecutionEngine *engine, int functionId)
{
    QV4::Function *clos = engine->current->compilationUnit->runtimeFunctions[functionId];
    Q_ASSERT(clos);
    return FunctionObject::createScriptFunction(engine->currentContext, clos)->asReturnedValue();
}

ReturnedValue Runtime::deleteElement(ExecutionEngine *engine, const Value &base, const Value &index)
{
    Scope scope(engine);
    ScopedObject o(scope, base);
    if (o) {
        uint n = index.asArrayIndex();
        if (n < UINT_MAX) {
            return Encode((bool)o->deleteIndexedProperty(n));
        }
    }

    ScopedString name(scope, index.toString(engine));
    return Runtime::deleteMemberString(engine, base, name);
}

ReturnedValue Runtime::deleteMember(ExecutionEngine *engine, const Value &base, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);
    return deleteMemberString(engine, base, name);
}

ReturnedValue Runtime::deleteMemberString(ExecutionEngine *engine, const Value &base, String *name)
{
    Scope scope(engine);
    ScopedObject obj(scope, base.toObject(engine));
    if (scope.engine->hasException)
        return Encode::undefined();
    return Encode(obj->deleteProperty(name));
}

ReturnedValue Runtime::deleteName(ExecutionEngine *engine, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);
    return Encode(engine->currentContext->deleteProperty(name));
}

QV4::ReturnedValue Runtime::instanceof(ExecutionEngine *engine, const Value &left, const Value &right)
{
    Scope scope(engine);
    ScopedFunctionObject f(scope, right.as<FunctionObject>());
    if (!f)
        return engine->throwTypeError();

    if (f->isBoundFunction())
        f = static_cast<BoundFunction *>(f.getPointer())->target();

    ScopedObject v(scope, left.as<Object>());
    if (!v)
        return Encode(false);

    ScopedObject o(scope, f->protoProperty());
    if (!o)
        return engine->throwTypeError();

    while (v) {
        v = v->prototype();

        if (!v)
            break;
        else if (o->d() == v->d())
            return Encode(true);
    }

    return Encode(false);
}

QV4::ReturnedValue Runtime::in(ExecutionEngine *engine, const Value &left, const Value &right)
{
    if (!right.isObject())
        return engine->throwTypeError();
    Scope scope(engine);
    ScopedString s(scope, left.toString(engine));
    if (scope.hasException())
        return Encode::undefined();
    bool r = right.objectValue()->hasProperty(s);
    return Encode(r);
}

double RuntimeHelpers::stringToNumber(const QString &string)
{
    QString s = string.trimmed();
    if (s.startsWith(QLatin1String("0x")) || s.startsWith(QLatin1String("0X")))
        return s.toLong(0, 16);
    bool ok;
    QByteArray ba = s.toLatin1();
    const char *begin = ba.constData();
    const char *end = 0;
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
    ScopedCallData callData(scope, 0);
    callData->thisObject = *object;

    ScopedValue conv(scope, object->get(meth1));
    if (FunctionObject *o = conv->as<FunctionObject>()) {
        ScopedValue r(scope, o->call(callData));
        if (r->isPrimitive())
            return r->asReturnedValue();
    }

    if (engine->hasException)
        return Encode::undefined();

    conv = object->get(meth2);
    if (FunctionObject *o = conv->as<FunctionObject>()) {
        ScopedValue r(scope, o->call(callData));
        if (r->isPrimitive())
            return r->asReturnedValue();
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
        return 0;
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

Heap::String *RuntimeHelpers::convertToString(ExecutionEngine *engine, const Value &value)
{
    switch (value.type()) {
    case Value::Empty_Type:
        Q_ASSERT(!"empty Value encountered");
    case Value::Undefined_Type:
        return engine->id_undefined()->d();
    case Value::Null_Type:
        return engine->id_null()->d();
    case Value::Boolean_Type:
        if (value.booleanValue())
            return engine->id_true()->d();
        else
            return engine->id_false()->d();
    case Value::Managed_Type:
        if (value.isString())
            return value.stringValue()->d();
        {
            Scope scope(engine);
            ScopedValue prim(scope, RuntimeHelpers::toPrimitive(value, STRING_HINT));
            return RuntimeHelpers::convertToString(engine, prim);
        }
    case Value::Integer_Type:
        return RuntimeHelpers::stringFromNumber(engine, value.int_32());
    default: // double
        return RuntimeHelpers::stringFromNumber(engine, value.doubleValue());
    } // switch
}

// This is slightly different from the method above, as
// the + operator requires a slightly different conversion
static Heap::String *convert_to_string_add(ExecutionEngine *engine, const Value &value)
{
    switch (value.type()) {
    case Value::Empty_Type:
        Q_ASSERT(!"empty Value encountered");
    case Value::Undefined_Type:
        return engine->id_undefined()->d();
    case Value::Null_Type:
        return engine->id_null()->d();
    case Value::Boolean_Type:
        if (value.booleanValue())
            return engine->id_true()->d();
        else
            return engine->id_false()->d();
    case Value::Managed_Type:
        if (value.isString())
            return value.stringValue()->d();
        {
            Scope scope(engine);
            ScopedValue prim(scope, RuntimeHelpers::toPrimitive(value, PREFERREDTYPE_HINT));
            return RuntimeHelpers::convertToString(engine, prim);
        }
    case Value::Integer_Type:
        return RuntimeHelpers::stringFromNumber(engine, value.int_32());
    default: // double
        return RuntimeHelpers::stringFromNumber(engine, value.doubleValue());
    } // switch
}

QV4::ReturnedValue RuntimeHelpers::addHelper(ExecutionEngine *engine, const Value &left, const Value &right)
{
    Scope scope(engine);

    ScopedValue pleft(scope, RuntimeHelpers::toPrimitive(left, PREFERREDTYPE_HINT));
    ScopedValue pright(scope, RuntimeHelpers::toPrimitive(right, PREFERREDTYPE_HINT));
    if (pleft->isString() || pright->isString()) {
        if (!pleft->isString())
            pleft = convert_to_string_add(engine, pleft);
        if (!pright->isString())
            pright = convert_to_string_add(engine, pright);
        if (scope.engine->hasException)
            return Encode::undefined();
        if (!pleft->stringValue()->d()->length())
            return pright->asReturnedValue();
        if (!pright->stringValue()->d()->length())
            return pleft->asReturnedValue();
        MemoryManager *mm = engine->memoryManager;
        return (mm->alloc<String>(mm, pleft->stringValue()->d(), pright->stringValue()->d()))->asReturnedValue();
    }
    double x = RuntimeHelpers::toNumber(pleft);
    double y = RuntimeHelpers::toNumber(pright);
    return Encode(x + y);
}

QV4::ReturnedValue Runtime::addString(ExecutionEngine *engine, const Value &left, const Value &right)
{
    Q_ASSERT(left.isString() || right.isString());

    if (left.isString() && right.isString()) {
        if (!left.stringValue()->d()->length())
            return right.asReturnedValue();
        if (!right.stringValue()->d()->length())
            return left.asReturnedValue();
        MemoryManager *mm = engine->memoryManager;
        return (mm->alloc<String>(mm, left.stringValue()->d(), right.stringValue()->d()))->asReturnedValue();
    }

    Scope scope(engine);
    ScopedValue pleft(scope, left);
    ScopedValue pright(scope, right);

    if (!pleft->isString())
        pleft = convert_to_string_add(engine, left);
    if (!pright->isString())
        pright = convert_to_string_add(engine, right);
    if (scope.engine->hasException)
        return Encode::undefined();
    if (!pleft->stringValue()->d()->length())
        return pright->asReturnedValue();
    if (!pright->stringValue()->d()->length())
        return pleft->asReturnedValue();
    MemoryManager *mm = engine->memoryManager;
    return (mm->alloc<String>(mm, pleft->stringValue()->d(), pright->stringValue()->d()))->asReturnedValue();
}

void Runtime::setProperty(ExecutionEngine *engine, const Value &object, int nameIndex, const Value &value)
{
    Scope scope(engine);
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);
    ScopedObject o(scope, object.toObject(engine));
    if (!o)
        return;
    o->put(name, value);
}

ReturnedValue Runtime::getElement(ExecutionEngine *engine, const Value &object, const Value &index)
{
    Scope scope(engine);
    uint idx = index.asArrayIndex();

    ScopedObject o(scope, object);
    if (!o) {
        if (idx < UINT_MAX) {
            if (const String *str = object.as<String>()) {
                if (idx >= (uint)str->toQString().length()) {
                    return Encode::undefined();
                }
                const QString s = str->toQString().mid(idx, 1);
                return scope.engine->newString(s)->asReturnedValue();
            }
        }

        if (object.isNullOrUndefined()) {
            QString message = QStringLiteral("Cannot read property '%1' of %2").arg(index.toQStringNoThrow()).arg(object.toQStringNoThrow());
            return engine->throwTypeError(message);
        }

        o = RuntimeHelpers::convertToObject(scope.engine, object);
        if (!o) // type error
            return Encode::undefined();
    }

    if (idx < UINT_MAX) {
        if (o->arrayData() && !o->arrayData()->attrs) {
            ScopedValue v(scope, o->arrayData()->get(idx));
            if (!v->isEmpty())
                return v->asReturnedValue();
        }

        return o->getIndexed(idx);
    }

    ScopedString name(scope, index.toString(engine));
    if (scope.hasException())
        return Encode::undefined();
    return o->get(name);
}

void Runtime::setElement(ExecutionEngine *engine, const Value &object, const Value &index, const Value &value)
{
    Scope scope(engine);
    ScopedObject o(scope, object.toObject(engine));
    if (scope.engine->hasException)
        return;

    uint idx = index.asArrayIndex();
    if (idx < UINT_MAX) {
        if (o->arrayType() == Heap::ArrayData::Simple) {
            Heap::SimpleArrayData *s = static_cast<Heap::SimpleArrayData *>(o->arrayData());
            if (s && idx < s->len && !s->data(idx).isEmpty()) {
                s->data(idx) = value;
                return;
            }
        }
        o->putIndexed(idx, value);
        return;
    }

    ScopedString name(scope, index.toString(engine));
    o->put(name, value);
}

ReturnedValue Runtime::foreachIterator(ExecutionEngine *engine, const Value &in)
{
    Scope scope(engine);
    ScopedObject o(scope, (Object *)0);
    if (!in.isNullOrUndefined())
        o = in.toObject(engine);
    return engine->newForEachIteratorObject(o)->asReturnedValue();
}

ReturnedValue Runtime::foreachNextPropertyName(const Value &foreach_iterator)
{
    Q_ASSERT(foreach_iterator.isObject());

    ForEachIteratorObject *it = static_cast<ForEachIteratorObject *>(foreach_iterator.objectValue());
    Q_ASSERT(it->as<ForEachIteratorObject>());

    return it->nextPropertyName();
}


void Runtime::setActivationProperty(ExecutionEngine *engine, int nameIndex, const Value &value)
{
    Scope scope(engine);
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);
    engine->currentContext->setProperty(name, value);
}

ReturnedValue Runtime::getProperty(ExecutionEngine *engine, const Value &object, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);

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

ReturnedValue Runtime::getActivationProperty(ExecutionEngine *engine, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);
    return engine->currentContext->getProperty(name);
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
        return Runtime::compareEqual(Primitive::fromDouble((double) x.booleanValue()), y);
    } else if (y.isBoolean()) {
        return Runtime::compareEqual(x, Primitive::fromDouble((double) y.booleanValue()));
    } else {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
#else
        if ((x.isNumber() || x.isString()) && y.isObject()) {
            Scope scope(y.objectValue()->engine());
            ScopedValue py(scope, RuntimeHelpers::toPrimitive(y, PREFERREDTYPE_HINT));
            return Runtime::compareEqual(x, py);
        } else if (x.isObject() && (y.isNumber() || y.isString())) {
            Scope scope(x.objectValue()->engine());
            ScopedValue px(scope, RuntimeHelpers::toPrimitive(x, PREFERREDTYPE_HINT));
            return Runtime::compareEqual(px, y);
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

QV4::Bool Runtime::compareGreaterThan(const Value &l, const Value &r)
{
    TRACE2(l, r);
    if (l.isInteger() && r.isInteger())
        return l.integerValue() > r.integerValue();
    if (l.isNumber() && r.isNumber())
        return l.asDouble() > r.asDouble();
    if (l.isString() && r.isString()) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
        return false;
#else
        return r.stringValue()->compare(l.stringValue());
#endif
    }

    if (l.isObject() || r.isObject()) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
#else
        QV4::ExecutionEngine *e = (l.isObject() ? l.objectValue() : r.objectValue())->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, RuntimeHelpers::toPrimitive(l, QV4::NUMBER_HINT));
        QV4::ScopedValue pr(scope, RuntimeHelpers::toPrimitive(r, QV4::NUMBER_HINT));
        return Runtime::compareGreaterThan(pl, pr);
#endif
    }

    double dl = RuntimeHelpers::toNumber(l);
    double dr = RuntimeHelpers::toNumber(r);
    return dl > dr;
}

QV4::Bool Runtime::compareLessThan(const Value &l, const Value &r)
{
    TRACE2(l, r);
    if (l.isInteger() && r.isInteger())
        return l.integerValue() < r.integerValue();
    if (l.isNumber() && r.isNumber())
        return l.asDouble() < r.asDouble();
    if (l.isString() && r.isString()) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
        return false;
#else
        return l.stringValue()->compare(r.stringValue());
#endif
    }

    if (l.isObject() || r.isObject()) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
#else
        QV4::ExecutionEngine *e = (l.isObject() ? l.objectValue() : r.objectValue())->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, RuntimeHelpers::toPrimitive(l, QV4::NUMBER_HINT));
        QV4::ScopedValue pr(scope, RuntimeHelpers::toPrimitive(r, QV4::NUMBER_HINT));
        return Runtime::compareLessThan(pl, pr);
#endif
    }

    double dl = RuntimeHelpers::toNumber(l);
    double dr = RuntimeHelpers::toNumber(r);
    return dl < dr;
}

QV4::Bool Runtime::compareGreaterEqual(const Value &l, const Value &r)
{
    TRACE2(l, r);
    if (l.isInteger() && r.isInteger())
        return l.integerValue() >= r.integerValue();
    if (l.isNumber() && r.isNumber())
        return l.asDouble() >= r.asDouble();
    if (l.isString() && r.isString()) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
        return false;
#else
        return !l.stringValue()->compare(r.stringValue());
#endif
    }

    if (l.isObject() || r.isObject()) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
#else
        QV4::ExecutionEngine *e = (l.isObject() ? l.objectValue() : r.objectValue())->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, RuntimeHelpers::toPrimitive(l, QV4::NUMBER_HINT));
        QV4::ScopedValue pr(scope, RuntimeHelpers::toPrimitive(r, QV4::NUMBER_HINT));
        return Runtime::compareGreaterEqual(pl, pr);
#endif
    }

    double dl = RuntimeHelpers::toNumber(l);
    double dr = RuntimeHelpers::toNumber(r);
    return dl >= dr;
}

QV4::Bool Runtime::compareLessEqual(const Value &l, const Value &r)
{
    TRACE2(l, r);
    if (l.isInteger() && r.isInteger())
        return l.integerValue() <= r.integerValue();
    if (l.isNumber() && r.isNumber())
        return l.asDouble() <= r.asDouble();
    if (l.isString() && r.isString()) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
        return false;
#else
        return !r.stringValue()->compare(l.stringValue());
#endif
    }

    if (l.isObject() || r.isObject()) {
#ifdef V4_BOOTSTRAP
        Q_UNIMPLEMENTED();
#else
        QV4::ExecutionEngine *e = (l.isObject() ? l.objectValue() : r.objectValue())->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, RuntimeHelpers::toPrimitive(l, QV4::NUMBER_HINT));
        QV4::ScopedValue pr(scope, RuntimeHelpers::toPrimitive(r, QV4::NUMBER_HINT));
        return Runtime::compareLessEqual(pl, pr);
#endif
    }

    double dl = RuntimeHelpers::toNumber(l);
    double dr = RuntimeHelpers::toNumber(r);
    return dl <= dr;
}

#ifndef V4_BOOTSTRAP
Bool Runtime::compareInstanceof(ExecutionEngine *engine, const Value &left, const Value &right)
{
    TRACE2(left, right);

    Scope scope(engine);
    ScopedValue v(scope, Runtime::instanceof(engine, left, right));
    return v->booleanValue();
}

uint Runtime::compareIn(ExecutionEngine *engine, const Value &left, const Value &right)
{
    TRACE2(left, right);

    Scope scope(engine);
    ScopedValue v(scope, Runtime::in(engine, left, right));
    return v->booleanValue();
}


ReturnedValue Runtime::callGlobalLookup(ExecutionEngine *engine, uint index, CallData *callData)
{
    Scope scope(engine);
    Q_ASSERT(callData->thisObject.isUndefined());

    Lookup *l = engine->current->lookups + index;
    ScopedFunctionObject o(scope, l->globalGetter(l, engine));
    if (!o)
        return engine->throwTypeError();

    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[l->nameIndex]);
    if (o->d() == scope.engine->evalFunction()->d() && name->equals(scope.engine->id_eval()))
        return static_cast<EvalFunction *>(o.getPointer())->evalCall(callData, true);

    return o->call(callData);
}


ReturnedValue Runtime::callActivationProperty(ExecutionEngine *engine, int nameIndex, CallData *callData)
{
    Q_ASSERT(callData->thisObject.isUndefined());
    Scope scope(engine);
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);

    ScopedObject base(scope);
    ScopedValue func(scope, engine->currentContext->getPropertyAndBase(name, base.getRef()));
    if (scope.engine->hasException)
        return Encode::undefined();

    if (base)
        callData->thisObject = base;

    FunctionObject *o = func->as<FunctionObject>();
    if (!o) {
        QString objectAsString = QStringLiteral("[null]");
        if (base)
            objectAsString = ScopedValue(scope, base.asReturnedValue())->toQStringNoThrow();
        QString msg = QStringLiteral("Property '%1' of object %2 is not a function").arg(name->toQString()).arg(objectAsString);
        return engine->throwTypeError(msg);
    }

    if (o->d() == scope.engine->evalFunction()->d() && name->equals(scope.engine->id_eval())) {
        return static_cast<EvalFunction *>(o)->evalCall(callData, true);
    }

    return o->call(callData);
}

ReturnedValue Runtime::callQmlScopeObjectProperty(ExecutionEngine *engine, int propertyIndex, CallData *callData)
{
    Scope scope(engine);
    ScopedFunctionObject o(scope, getQmlScopeObjectProperty(engine, callData->thisObject, propertyIndex));
    if (!o) {
        QString error = QStringLiteral("Property '%1' of scope object is not a function").arg(propertyIndex);
        return engine->throwTypeError(error);
    }
    return o->call(callData);
}

ReturnedValue Runtime::callQmlContextObjectProperty(ExecutionEngine *engine, int propertyIndex, CallData *callData)
{
    Scope scope(engine);
    ScopedFunctionObject o(scope, getQmlContextObjectProperty(engine, callData->thisObject, propertyIndex));
    if (!o) {
        QString error = QStringLiteral("Property '%1' of context object is not a function").arg(propertyIndex);
        return engine->throwTypeError(error);
    }

    return o->call(callData);
}

ReturnedValue Runtime::callProperty(ExecutionEngine *engine, int nameIndex, CallData *callData)
{
    Scope scope(engine);
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);
    ScopedObject baseObject(scope, callData->thisObject);
    if (!baseObject) {
        Q_ASSERT(!callData->thisObject.isEmpty());
        if (callData->thisObject.isNullOrUndefined()) {
            QString message = QStringLiteral("Cannot call method '%1' of %2").arg(name->toQString()).arg(callData->thisObject.toQStringNoThrow());
            return engine->throwTypeError(message);
        }

        baseObject = RuntimeHelpers::convertToObject(scope.engine, callData->thisObject);
        if (!baseObject) // type error
            return Encode::undefined();
        callData->thisObject = baseObject.asReturnedValue();
    }

    ScopedFunctionObject o(scope, baseObject->get(name));
    if (!o) {
        QString error = QStringLiteral("Property '%1' of object %2 is not a function").arg(name->toQString(), callData->thisObject.toQStringNoThrow());
        return engine->throwTypeError(error);
    }

    return o->call(callData);
}

ReturnedValue Runtime::callPropertyLookup(ExecutionEngine *engine, uint index, CallData *callData)
{
    Lookup *l = engine->current->lookups + index;
    Value v;
    v = l->getter(l, engine, callData->thisObject);
    if (!v.isObject())
        return engine->throwTypeError();

    return v.objectValue()->call(callData);
}

ReturnedValue Runtime::callElement(ExecutionEngine *engine, const Value &index, CallData *callData)
{
    Scope scope(engine);
    ScopedObject baseObject(scope, callData->thisObject.toObject(engine));
    ScopedString s(scope, index.toString(engine));

    if (scope.engine->hasException)
        return Encode::undefined();
    callData->thisObject = baseObject;

    ScopedObject o(scope, baseObject->get(s));
    if (!o)
        return engine->throwTypeError();

    return o->call(callData);
}

ReturnedValue Runtime::callValue(ExecutionEngine *engine, const Value &func, CallData *callData)
{
    if (!func.isObject())
        return engine->throwTypeError(QStringLiteral("%1 is not a function").arg(func.toQStringNoThrow()));

    return func.objectValue()->call(callData);
}


ReturnedValue Runtime::constructGlobalLookup(ExecutionEngine *engine, uint index, CallData *callData)
{
    Scope scope(engine);
    Q_ASSERT(callData->thisObject.isUndefined());

    Lookup *l = engine->current->lookups + index;
    ScopedObject f(scope, l->globalGetter(l, engine));
    if (!f)
        return engine->throwTypeError();

    return f->construct(callData);
}


ReturnedValue Runtime::constructActivationProperty(ExecutionEngine *engine, int nameIndex, CallData *callData)
{
    Scope scope(engine);
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);
    ScopedValue func(scope, engine->currentContext->getProperty(name));
    if (scope.engine->hasException)
        return Encode::undefined();

    Object *f = func->as<Object>();
    if (!f)
        return engine->throwTypeError();

    return f->construct(callData);
}

ReturnedValue Runtime::constructValue(ExecutionEngine *engine, const Value &func, CallData *callData)
{
    const Object *f = func.as<Object>();
    if (!f)
        return engine->throwTypeError();

    return f->construct(callData);
}

ReturnedValue Runtime::constructProperty(ExecutionEngine *engine, int nameIndex, CallData *callData)
{
    Scope scope(engine);
    ScopedObject thisObject(scope, callData->thisObject.toObject(engine));
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);
    if (scope.engine->hasException)
        return Encode::undefined();

    ScopedObject f(scope, thisObject->get(name));
    if (!f)
        return engine->throwTypeError();

    return f->construct(callData);
}

ReturnedValue Runtime::constructPropertyLookup(ExecutionEngine *engine, uint index, CallData *callData)
{
    Lookup *l = engine->current->lookups + index;
    Value v;
    v = l->getter(l, engine, callData->thisObject);
    if (!v.isObject())
        return engine->throwTypeError();

    return v.objectValue()->construct(callData);
}


void Runtime::throwException(ExecutionEngine *engine, const Value &value)
{
    if (!value.isEmpty())
        engine->throwError(value);
}

ReturnedValue Runtime::typeofValue(ExecutionEngine *engine, const Value &value)
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

QV4::ReturnedValue Runtime::typeofName(ExecutionEngine *engine, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);
    ScopedValue prop(scope, engine->currentContext->getProperty(name));
    // typeof doesn't throw. clear any possible exception
    scope.engine->hasException = false;
    return Runtime::typeofValue(engine, prop);
}

#ifndef V4_BOOTSTRAP
ReturnedValue Runtime::typeofScopeObjectProperty(ExecutionEngine *engine, const Value &context,
                                                 int propertyIndex)
{
    Scope scope(engine);
    ScopedValue prop(scope, getQmlScopeObjectProperty(engine, context, propertyIndex));
    if (scope.engine->hasException)
        return Encode::undefined();
    return Runtime::typeofValue(engine, prop);
}

ReturnedValue Runtime::typeofContextObjectProperty(ExecutionEngine *engine, const Value &context,
                                                   int propertyIndex)
{
    Scope scope(engine);
    ScopedValue prop(scope, getQmlContextObjectProperty(engine, context, propertyIndex));
    if (scope.engine->hasException)
        return Encode::undefined();
    return Runtime::typeofValue(engine, prop);
}
#endif // V4_BOOTSTRAP

QV4::ReturnedValue Runtime::typeofMember(ExecutionEngine *engine, const Value &base, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);
    ScopedObject obj(scope, base.toObject(engine));
    if (scope.engine->hasException)
        return Encode::undefined();
    ScopedValue prop(scope, obj->get(name));
    return Runtime::typeofValue(engine, prop);
}

QV4::ReturnedValue Runtime::typeofElement(ExecutionEngine *engine, const Value &base, const Value &index)
{
    Scope scope(engine);
    ScopedString name(scope, index.toString(engine));
    ScopedObject obj(scope, base.toObject(engine));
    if (scope.engine->hasException)
        return Encode::undefined();
    ScopedValue prop(scope, obj->get(name));
    return Runtime::typeofValue(engine, prop);
}

ReturnedValue Runtime::unwindException(ExecutionEngine *engine)
{
    if (!engine->hasException)
        return Primitive::emptyValue().asReturnedValue();
    return engine->catchException(0);
}

/* The next three methods are a bit tricky. They can't open up a Scope, as that
 * would mess up the pushing of the context.
 *
 * Instead the push/pop pair acts as a non local scope.
 */
void Runtime::pushWithScope(const Value &o, ExecutionEngine *engine)
{
    engine->pushContext(engine->currentContext->newWithContext(o.toObject(engine)));
    Q_ASSERT(engine->jsStackTop = engine->currentContext + 2);
}

void Runtime::pushCatchScope(NoThrowEngine *engine, int exceptionVarNameIndex)
{
    ExecutionContext *c = engine->currentContext;
    engine->pushContext(c->newCatchContext(c->d()->compilationUnit->runtimeStrings[exceptionVarNameIndex], engine->catchException(0)));
    Q_ASSERT(engine->jsStackTop = engine->currentContext + 2);
}

void Runtime::popScope(ExecutionEngine *engine)
{
    Q_ASSERT(engine->jsStackTop = engine->currentContext + 2);
    engine->popContext();
    engine->jsStackTop -= 2;
}

void Runtime::declareVar(ExecutionEngine *engine, bool deletable, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);
    engine->currentContext->createMutableBinding(name, deletable);
}

ReturnedValue Runtime::arrayLiteral(ExecutionEngine *engine, Value *values, uint length)
{
    return engine->newArrayObject(values, length)->asReturnedValue();
}

ReturnedValue Runtime::objectLiteral(ExecutionEngine *engine, const QV4::Value *args, int classId, int arrayValueCount, int arrayGetterSetterCountAndFlags)
{
    Scope scope(engine);
    QV4::InternalClass *klass = engine->current->compilationUnit->runtimeClasses[classId];
    ScopedObject o(scope, engine->newObject(klass, engine->objectPrototype()));

    {
        bool needSparseArray = arrayGetterSetterCountAndFlags >> 30;
        if (needSparseArray)
            o->initSparseArray();
    }

    for (uint i = 0; i < klass->size; ++i)
        *o->propertyData(i) = *args++;

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

QV4::ReturnedValue Runtime::setupArgumentsObject(ExecutionEngine *engine)
{
    Q_ASSERT(engine->current->type == Heap::ExecutionContext::Type_CallContext);
    QV4::CallContext *c = static_cast<QV4::CallContext *>(engine->currentContext);
    QV4::InternalClass *ic = c->d()->strictMode ? engine->strictArgumentsObjectClass : engine->argumentsObjectClass;
    return engine->memoryManager->allocObject<ArgumentsObject>(ic, engine->objectPrototype(), c)->asReturnedValue();
}

#endif // V4_BOOTSTRAP

QV4::ReturnedValue Runtime::increment(const Value &value)
{
    TRACE1(value);

    if (value.isInteger() && value.integerValue() < INT_MAX)
        return Encode(value.integerValue() + 1);
    else {
        double d = value.toNumber();
        return Encode(d + 1.);
    }
}

QV4::ReturnedValue Runtime::decrement(const Value &value)
{
    TRACE1(value);

    if (value.isInteger() && value.integerValue() > INT_MIN)
        return Encode(value.integerValue() - 1);
    else {
        double d = value.toNumber();
        return Encode(d - 1.);
    }
}

#ifndef V4_BOOTSTRAP

QV4::ReturnedValue RuntimeHelpers::toString(ExecutionEngine *engine, const Value &value)
{
    if (value.isString())
        return value.asReturnedValue();
    return RuntimeHelpers::convertToString(engine, value)->asReturnedValue();
}

QV4::ReturnedValue RuntimeHelpers::toObject(ExecutionEngine *engine, const Value &value)
{
    if (value.isObject())
        return value.asReturnedValue();

    Heap::Object *o = RuntimeHelpers::convertToObject(engine, value);
    if (!o) // type error
        return Encode::undefined();

    return Encode(o);
}

#endif // V4_BOOTSTRAP

ReturnedValue Runtime::toDouble(const Value &value)
{
    TRACE1(value);
    return Encode(value.toNumber());
}

int Runtime::toInt(const Value &value)
{
    TRACE1(value);
    return value.toInt32();
}

int Runtime::doubleToInt(const double &d)
{
    TRACE0();
    return Primitive::toInt32(d);
}

unsigned Runtime::toUInt(const Value &value)
{
    TRACE1(value);
    return value.toUInt32();
}

unsigned Runtime::doubleToUInt(const double &d)
{
    TRACE0();
    return Primitive::toUInt32(d);
}

#ifndef V4_BOOTSTRAP

ReturnedValue Runtime::getQmlContext(NoThrowEngine *engine)
{
    return engine->qmlContext()->asReturnedValue();
}

ReturnedValue Runtime::regexpLiteral(ExecutionEngine *engine, int id)
{
    return engine->current->compilationUnit->runtimeRegularExpressions[id].asReturnedValue();
}

ReturnedValue Runtime::getQmlQObjectProperty(ExecutionEngine *engine, const Value &object, int propertyIndex, bool captureRequired)
{
    Scope scope(engine);
    QV4::Scoped<QObjectWrapper> wrapper(scope, object);
    if (!wrapper) {
        engine->throwTypeError(QStringLiteral("Cannot read property of null"));
        return Encode::undefined();
    }
    return QV4::QObjectWrapper::getProperty(scope.engine, wrapper->object(), propertyIndex, captureRequired);
}

QV4::ReturnedValue Runtime::getQmlAttachedProperty(ExecutionEngine *engine, int attachedPropertiesId, int propertyIndex)
{
    QObject *scopeObject = engine->qmlScopeObject();
    QObject *attachedObject = qmlAttachedPropertiesObjectById(attachedPropertiesId, scopeObject);

    QJSEngine *jsEngine = engine->jsEngine();
    QQmlData::ensurePropertyCache(jsEngine, attachedObject);
    return QV4::QObjectWrapper::getProperty(engine, attachedObject, propertyIndex, /*captureRequired*/true);
}

ReturnedValue Runtime::getQmlScopeObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex)
{
    const QmlContext &c = static_cast<const QmlContext &>(context);
    return QV4::QObjectWrapper::getProperty(engine, c.d()->qml->scopeObject, propertyIndex, false);
}

ReturnedValue Runtime::getQmlContextObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex)
{
    const QmlContext &c = static_cast<const QmlContext &>(context);
    return QV4::QObjectWrapper::getProperty(engine, c.d()->qml->context->contextObject, propertyIndex, false);
}

ReturnedValue Runtime::getQmlSingletonQObjectProperty(ExecutionEngine *engine, const Value &object, int propertyIndex, bool captureRequired)
{
    Scope scope(engine);
    QV4::Scoped<QmlTypeWrapper> wrapper(scope, object);
    if (!wrapper) {
        scope.engine->throwTypeError(QStringLiteral("Cannot read property of null"));
        return Encode::undefined();
    }
    return QV4::QObjectWrapper::getProperty(scope.engine, wrapper->singletonObject(), propertyIndex, captureRequired);
}

ReturnedValue Runtime::getQmlIdObject(ExecutionEngine *engine, const Value &c, uint index)
{
    Scope scope(engine);
    const QmlContext &qmlContext = static_cast<const QmlContext &>(c);
    QQmlContextData *context = qmlContext.d()->qml->context;
    if (!context || index >= (uint)context->idValueCount)
        return Encode::undefined();

    QQmlEnginePrivate *ep = engine->qmlEngine() ? QQmlEnginePrivate::get(engine->qmlEngine()) : 0;
    if (ep && ep->propertyCapture)
        ep->propertyCapture->captureProperty(&context->idValues[index].bindings);

    return QObjectWrapper::wrap(engine, context->idValues[index].data());
}

void Runtime::setQmlScopeObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex, const Value &value)
{
    const QmlContext &c = static_cast<const QmlContext &>(context);
    return QV4::QObjectWrapper::setProperty(engine, c.d()->qml->scopeObject, propertyIndex, value);
}

void Runtime::setQmlContextObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex, const Value &value)
{
    const QmlContext &c = static_cast<const QmlContext &>(context);
    return QV4::QObjectWrapper::setProperty(engine, c.d()->qml->context->contextObject, propertyIndex, value);
}

void Runtime::setQmlQObjectProperty(ExecutionEngine *engine, const Value &object, int propertyIndex, const Value &value)
{
    Scope scope(engine);
    QV4::Scoped<QObjectWrapper> wrapper(scope, object);
    if (!wrapper) {
        engine->throwTypeError(QStringLiteral("Cannot write property of null"));
        return;
    }
    wrapper->setProperty(engine, propertyIndex, value);
}

ReturnedValue Runtime::getQmlImportedScripts(NoThrowEngine *engine)
{
    QQmlContextData *context = engine->callingQmlContext();
    if (!context)
        return Encode::undefined();
    return context->importedScripts.value();
}

QV4::ReturnedValue Runtime::getQmlSingleton(QV4::NoThrowEngine *engine, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->current->compilationUnit->runtimeStrings[nameIndex]);
    return engine->qmlSingletonWrapper(name);
}

void Runtime::convertThisToObject(ExecutionEngine *engine)
{
    Value *t = &engine->current->callData->thisObject;
    if (t->isObject())
        return;
    if (t->isNullOrUndefined()) {
        *t = engine->globalObject->asReturnedValue();
    } else {
        *t = t->toObject(engine)->asReturnedValue();
    }
}

#endif // V4_BOOTSTRAP

} // namespace QV4

QT_END_NAMESPACE
