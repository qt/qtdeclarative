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
#include "qv4runtime_p.h"
#include "qv4engine_p.h"
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
#include <private/qqmljsast_p.h>
#include "qv4qobjectwrapper_p.h"
#include "qv4symbol_p.h"
#include "qv4generatorobject_p.h"

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

static QV4::Lookup *runtimeLookup(Function *f, uint i)
{
    return f->executableCompilationUnit()->runtimeLookups + i;
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
    num = Value::toInteger(num);

    do {
        char c = (char)::fmod(num, radix);
        c = (c < 10) ? (c + '0') : (c - 10 + 'a');
        result->prepend(QLatin1Char(c));
        num = ::floor(num / radix);
    } while (num != 0);

    if (frac != 0) {
        result->append(QLatin1Char('.'));
        double magnitude = 1;
        double next = frac;
        do {
            next *= radix;
            const int floored = ::floor(next);
            char c = char(floored);
            c = (c < 10) ? (c + '0') : (c - 10 + 'a');
            result->append(QLatin1Char(c));
            magnitude /= radix;
            frac -= double(floored) * magnitude;
            next -= double(floored);

            // The next digit still makes a difference
            // if a value of "radix" for it would change frac.
            // Otherwise we've reached the limit of numerical precision.
        } while (frac > 0 && frac - magnitude != frac);
    }

    if (negative)
        result->prepend(QLatin1Char('-'));
}

ReturnedValue Runtime::Closure::call(ExecutionEngine *engine, int functionId)
{
    QV4::Function *clos = engine->currentStackFrame->v4Function->executableCompilationUnit()
                                  ->runtimeFunctions[functionId];
    Q_ASSERT(clos);
    ExecutionContext *current = static_cast<ExecutionContext *>(&engine->currentStackFrame->jsFrame->context);
    if (clos->isGenerator())
        return GeneratorFunction::create(current, clos)->asReturnedValue();
    return FunctionObject::createScriptFunction(current, clos)->asReturnedValue();
}

Bool Runtime::DeleteProperty_NoThrow::call(ExecutionEngine *engine, const Value &base, const Value &index)
{
    Scope scope(engine);
    ScopedObject o(scope, base.toObject(engine));
    if (scope.engine->hasException)
        return Encode::undefined();
    Q_ASSERT(o);

    ScopedPropertyKey key(scope, index.toPropertyKey(engine));
    if (engine->hasException)
        return false;
    return o->deleteProperty(key);
}

ReturnedValue Runtime::DeleteProperty::call(ExecutionEngine *engine, QV4::Function *function, const QV4::Value &base, const QV4::Value &index)
{
    if (!Runtime::DeleteProperty_NoThrow::call(engine, base, index)) {
        if (function->isStrict())
            engine->throwTypeError();
        return Encode(false);
    } else {
        return Encode(true);
    }
}

Bool Runtime::DeleteName_NoThrow::call(ExecutionEngine *engine, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    return static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context).deleteProperty(name);
}

ReturnedValue Runtime::DeleteName::call(ExecutionEngine *engine, Function *function, int name)
{
    if (!Runtime::DeleteName_NoThrow::call(engine, name)) {
        if (function->isStrict())
            engine->throwTypeError();
        return Encode(false);
    } else {
        return Encode(true);
    }
}

QV4::ReturnedValue Runtime::Instanceof::call(ExecutionEngine *engine, const Value &lval, const Value &rval)
{
    // 11.8.6, 5: rval must be an Object
    const Object *rhs = rval.as<Object>();
    if (!rhs)
       return engine->throwTypeError();

    const FunctionObject *f = rhs->as<FunctionObject>();
    // shortcut hasInstance evaluation. In this case we know that we are calling the regular hasInstance()
    // method of the FunctionPrototype
    if (f && f->d()->prototype() == engine->functionPrototype()->d() && !f->hasHasInstanceProperty())
        return Object::checkedInstanceOf(engine, f, lval);

    Scope scope(engine);
    ScopedValue hasInstance(scope, rhs->get(engine->symbol_hasInstance()));
    if (hasInstance->isUndefined())
        return rhs->instanceOf(lval);
    FunctionObject *fHasInstance = hasInstance->as<FunctionObject>();
    if (!fHasInstance)
        return engine->throwTypeError();

    ScopedValue result(scope, fHasInstance->call(&rval, &lval, 1));
    return scope.hasException() ? Encode::undefined() : Encode(result->toBoolean());
}

QV4::ReturnedValue Runtime::In::call(ExecutionEngine *engine, const Value &left, const Value &right)
{
    Object *ro = right.objectValue();
    if (!ro)
        return engine->throwTypeError();
    Scope scope(engine);
    ScopedPropertyKey s(scope, left.toPropertyKey(engine));
    if (scope.hasException())
        return Encode::undefined();
    bool r = ro->hasProperty(s);
    return Encode(r);
}

double RuntimeHelpers::stringToNumber(const QString &string)
{
    // The actual maximum valid length is certainly shorter, but due to the sheer number of
    // different number formatting variants, we rather err on the side of caution here.
    // For example, you can have up to 772 valid decimal digits left of the dot, as stated in the
    // libdoubleconversion sources. The same maximum value would be represented by roughly 3.5 times
    // as many binary digits.
    const int excessiveLength = 16 * 1024;
    if (string.length() > excessiveLength)
        return qQNaN();

    const QStringRef s = QStringRef(&string).trimmed();
    if (s.startsWith(QLatin1Char('0'))) {
        int base = -1;
        if (s.startsWith(QLatin1String("0x")) || s.startsWith(QLatin1String("0X")))
            base = 16;
        else if (s.startsWith(QLatin1String("0o")) || s.startsWith(QLatin1String("0O")))
            base = 8;
        else if (s.startsWith(QLatin1String("0b")) || s.startsWith(QLatin1String("0B")))
            base = 2;
        if (base > 0) {
            bool ok = true;
            qlonglong num;
            num = s.mid(2).toLongLong(&ok, base);
            if (!ok)
                return qQNaN();
            return num;
        }
    }
    bool ok = false;
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
    ExecutionEngine *engine = object->internalClass()->engine;
    if (engine->hasException)
        return Encode::undefined();

    String *hint;
    switch (typeHint) {
    case STRING_HINT:
        hint = engine->id_string();
        break;
    case NUMBER_HINT:
        hint = engine->id_number();
        break;
    default:
        hint = engine->id_default();
        break;
    }

    Scope scope(engine);
    ScopedFunctionObject toPrimitive(scope, object->get(engine->symbol_toPrimitive()));
    if (engine->hasException)
        return Encode::undefined();
    if (toPrimitive) {
        ScopedValue result(scope, toPrimitive->call(object, hint, 1));
        if (engine->hasException)
            return Encode::undefined();
        if (!result->isPrimitive())
            return engine->throwTypeError();
        return result->asReturnedValue();
    }

    if (hint == engine->id_default())
        hint = engine->id_number();
    return ordinaryToPrimitive(engine, object, hint);
}


ReturnedValue RuntimeHelpers::ordinaryToPrimitive(ExecutionEngine *engine, const Object *object, String *typeHint)
{
    Q_ASSERT(!engine->hasException);

    String *meth1 = engine->id_toString();
    String *meth2 = engine->id_valueOf();

    if (typeHint->propertyKey() == engine->id_number()->propertyKey()) {
        qSwap(meth1, meth2);
    } else {
        Q_ASSERT(typeHint->propertyKey() == engine->id_string()->propertyKey());
    }

    Scope scope(engine);
    ScopedValue result(scope);

    ScopedValue conv(scope, object->get(meth1));
    if (FunctionObject *o = conv->as<FunctionObject>()) {
        result = o->call(object, nullptr, 0);
        if (engine->hasException)
            return Encode::undefined();
        if (result->isPrimitive())
            return result->asReturnedValue();
    }

    if (engine->hasException)
        return Encode::undefined();

    conv = object->get(meth2);
    if (FunctionObject *o = conv->as<FunctionObject>()) {
        result = o->call(object, nullptr, 0);
        if (engine->hasException)
            return Encode::undefined();
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
        engine->throwTypeError(QLatin1String("Value is undefined and could not be converted to an object"));
        return nullptr;
    case Value::Null_Type:
        engine->throwTypeError(QLatin1String("Value is null and could not be converted to an object"));
        return nullptr;
    case Value::Boolean_Type:
        return engine->newBooleanObject(value.booleanValue());
    case Value::Managed_Type:
        Q_ASSERT(value.isStringOrSymbol());
        if (!value.isString())
            return engine->newSymbolObject(value.symbolValue());
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
        if (value.isSymbol()) {
            engine->throwTypeError(QLatin1String("Cannot convert a symbol to a string."));
            return  nullptr;
        }
        value = Value::fromReturnedValue(RuntimeHelpers::toPrimitive(value, hint));
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

ReturnedValue Runtime::GetTemplateObject::call(Function *function, int index)
{
    return function->executableCompilationUnit()->templateObjectAt(index)->asReturnedValue();
}

void Runtime::StoreProperty::call(ExecutionEngine *engine, const Value &object, int nameIndex, const Value &value)
{
    Scope scope(engine);
    QV4::Function *v4Function = engine->currentStackFrame->v4Function;
    ScopedString name(scope, v4Function->compilationUnit->runtimeStrings[nameIndex]);
    ScopedObject o(scope, object);
    if (!o) {
        if (v4Function->isStrict()) {
            engine->throwTypeError();
            return;
        }
        o = object.toObject(engine);
    }
    if ((!o || !o->put(name, value)) && v4Function->isStrict())
        engine->throwTypeError();
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

    return o->get(idx);
}

static Q_NEVER_INLINE ReturnedValue getElementFallback(ExecutionEngine *engine, const Value &object, const Value &index)
{
    Q_ASSERT(!index.isPositiveInt());

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

    ScopedPropertyKey name(scope, index.toPropertyKey(engine));
    if (scope.hasException())
        return Encode::undefined();
    return o->get(name);
}

ReturnedValue Runtime::LoadElement::call(ExecutionEngine *engine, const Value &object, const Value &index)
{
    if (index.isPositiveInt()) {
        uint idx = static_cast<uint>(index.int_32());
        if (Heap::Base *b = object.heapObject()) {
            if (b->internalClass->vtable->isObject) {
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
    ScopedObject o(scope, object);
    if (!o) {
        if (engine->currentStackFrame->v4Function->isStrict()) {
            engine->throwTypeError();
            return false;
        }

        o = object.toObject(engine);
    }
    if (engine->hasException)
        return false;

    if (index.isPositiveInt()) {
        uint idx = static_cast<uint>(index.int_32());
        if (o->d()->arrayData && o->d()->arrayData->type == Heap::ArrayData::Simple) {
            Heap::SimpleArrayData *s = o->d()->arrayData.cast<Heap::SimpleArrayData>();
            if (idx < s->values.size) {
                s->setData(engine, idx, value);
                return true;
            }
        }
        return o->put(idx, value);
    }

    ScopedPropertyKey name(scope, index.toPropertyKey(engine));
    if (engine->hasException)
        return false;
    return o->put(name, value);
}

void Runtime::StoreElement::call(ExecutionEngine *engine, const Value &object, const Value &index, const Value &value)
{
    if (index.isPositiveInt()) {
        uint idx = static_cast<uint>(index.int_32());
        if (Heap::Base *b = object.heapObject()) {
            if (b->internalClass->vtable->isObject) {
                Heap::Object *o = static_cast<Heap::Object *>(b);
                if (o->arrayData && o->arrayData->type == Heap::ArrayData::Simple) {
                    Heap::SimpleArrayData *s = o->arrayData.cast<Heap::SimpleArrayData>();
                    if (idx < s->values.size) {
                        s->setData(engine, idx, value);
                        return;
                    }
                }
            }
        }
    }

    if (!setElementFallback(engine, object, index, value) && engine->currentStackFrame->v4Function->isStrict())
        engine->throwTypeError();
}

ReturnedValue Runtime::GetIterator::call(ExecutionEngine *engine, const Value &in, int iterator)
{
    Scope scope(engine);
    ScopedObject o(scope, (Object *)nullptr);
    if (!in.isNullOrUndefined())
        o = in.toObject(engine);
    if (engine->hasException)
        return Encode::undefined();
    if (iterator == static_cast<int>(QQmlJS::AST::ForEachType::Of)) {
        if (!o)
            return engine->throwTypeError();
        ScopedFunctionObject f(scope, o->get(engine->symbol_iterator()));
        if (!f)
            return engine->throwTypeError();
        JSCallData cData(scope, 0, nullptr, o);
        ScopedObject it(scope, f->call(cData));
        if (engine->hasException)
            return Encode::undefined();
        if (!it)
            return engine->throwTypeError();
        return it->asReturnedValue();
    }
    return engine->newForInIteratorObject(o)->asReturnedValue();
}

ReturnedValue Runtime::IteratorNext::call(ExecutionEngine *engine, const Value &iterator, Value *value)
{
    // if we throw an exception from here, return true, not undefined. This is to ensure iteratorDone is set to true
    // and the stack unwinding won't close the iterator
    Q_ASSERT(iterator.isObject());

    Scope scope(engine);
    ScopedFunctionObject f(scope, static_cast<const Object &>(iterator).get(engine->id_next()));
    if (!f) {
        engine->throwTypeError();
        return Encode(true);
    }
    JSCallData cData(scope, 0, nullptr, &iterator);
    ScopedObject o(scope, f->call(cData));
    if (scope.hasException())
        return Encode(true);
    if (!o) {
        engine->throwTypeError();
        return Encode(true);
    }

    ScopedValue d(scope, o->get(engine->id_done()));
    if (scope.hasException())
        return Encode(true);
    bool done = d->toBoolean();
    if (done) {
        *value = Encode::undefined();
        return Encode(true);
    }

    *value = o->get(engine->id_value());
    if (scope.hasException())
        return Encode(true);
    return Encode(false);
}

ReturnedValue Runtime::IteratorNextForYieldStar::call(ExecutionEngine *engine, const Value &received, const Value &iterator, Value *object)
{
    // the return value encodes how to continue the yield* iteration.
    // true implies iteration is done, false for iteration to continue
    // a return value of undefines is a special marker, that the iterator has been invoked with return()

    Scope scope(engine);
    Q_ASSERT(iterator.isObject());

    const Value *arg = &received;
    bool returnCalled = false;
    FunctionObject *f = nullptr;
    if (engine->hasException) {
        if (engine->exceptionValue->isEmpty()) {
            // generator called with return()
            *engine->exceptionValue = Encode::undefined();
            engine->hasException = false;

            ScopedValue ret(scope, static_cast<const Object &>(iterator).get(engine->id_return()));
            if (ret->isUndefined()) {
                // propagate return()
                return Encode::undefined();
            }
            returnCalled = true;
            f = ret->as<FunctionObject>();
        } else {
            // generator called with throw
            ScopedValue exceptionValue(scope, *engine->exceptionValue);
            *engine->exceptionValue = Encode::undefined();
            engine->hasException = false;

            ScopedValue t(scope, static_cast<const Object &>(iterator).get(engine->id_throw()));
            if (engine->hasException)
                return Encode::undefined();
            if (t->isUndefined()) {
                // no throw method on the iterator
                ScopedValue done(scope, Encode(false));
                IteratorClose::call(engine, iterator, done);
                if (engine->hasException)
                    return Encode::undefined();
                return engine->throwTypeError();
            }
            f = t->as<FunctionObject>();
            arg = exceptionValue;
        }
    } else {
        // generator called with next()
        ScopedFunctionObject next(scope, static_cast<const Object &>(iterator).get(engine->id_next()));
        f = next->as<FunctionObject>();
    }

    if (!f)
        return engine->throwTypeError();

    ScopedObject o(scope, f->call(&iterator, arg, 1));
    if (scope.hasException())
        return Encode(true);
    if (!o)
        return engine->throwTypeError();

    ScopedValue d(scope, o->get(engine->id_done()));
    if (scope.hasException())
        return Encode(true);
    bool done = d->toBoolean();
    if (done) {
        *object = o->get(engine->id_value());
        return returnCalled ? Encode::undefined() : Encode(true);
    }
    *object = o;
    return Encode(false);
}

ReturnedValue Runtime::IteratorClose::call(ExecutionEngine *engine, const Value &iterator, const Value &done)
{
    Q_ASSERT(iterator.isObject());
    Q_ASSERT(done.isBoolean());
    if (done.booleanValue())
        return Encode::undefined();

    Scope scope(engine);
    ScopedValue e(scope);
    bool hadException = engine->hasException;
    if (hadException) {
        e = *engine->exceptionValue;
        engine->hasException = false;
    }

    auto originalCompletion = [=]() {
        if (hadException) {
            *engine->exceptionValue = e;
            engine->hasException = hadException;
        }
        return Encode::undefined();
    };

    ScopedValue ret(scope, static_cast<const Object &>(iterator).get(engine->id_return()));
    ScopedObject o(scope);
    if (!ret->isUndefined()) {
        FunctionObject *f = ret->as<FunctionObject>();
        o = f->call(&iterator, nullptr, 0);
        if (engine->hasException && !hadException)
            return Encode::undefined();
    }
    if (hadException || ret->isUndefined())
        return originalCompletion();

    if (!o)
        return engine->throwTypeError();

    return originalCompletion();
}

ReturnedValue Runtime::DestructureRestElement::call(ExecutionEngine *engine, const Value &iterator)
{
    Q_ASSERT(iterator.isObject());

    Scope scope(engine);
    ScopedArrayObject array(scope, engine->newArrayObject());
    array->arrayCreate();
    uint index = 0;
    while (1) {
        ScopedValue n(scope);
        ScopedValue done(scope, IteratorNext::call(engine, iterator, n));
        if (engine->hasException)
            return Encode::undefined();
        Q_ASSERT(done->isBoolean());
        if (done->booleanValue())
            break;
        array->arraySet(index, n);
        ++index;
    }
    return array->asReturnedValue();
}

void Runtime::StoreNameSloppy::call(ExecutionEngine *engine, int nameIndex, const Value &value)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    ExecutionContext::Error e = static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context).setProperty(name, value);

    if (e == ExecutionContext::RangeError)
        engine->globalObject->put(name, value);
}

void Runtime::StoreNameStrict::call(ExecutionEngine *engine, int nameIndex, const Value &value)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    ExecutionContext::Error e = static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context).setProperty(name, value);
    if (e == ExecutionContext::TypeError)
        engine->throwTypeError();
    else if (e == ExecutionContext::RangeError)
        engine->throwReferenceError(name);
}

ReturnedValue Runtime::LoadProperty::call(ExecutionEngine *engine, const Value &object, int nameIndex)
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

ReturnedValue Runtime::LoadName::call(ExecutionEngine *engine, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    return static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context).getProperty(name);
}

static Object *getSuperBase(Scope &scope)
{
    if (scope.engine->currentStackFrame->jsFrame->thisObject.isEmpty()) {
        scope.engine->throwReferenceError(QStringLiteral("Missing call to super()."), QString(), 0, 0);
        return nullptr;
    }

    ScopedFunctionObject f(
            scope, Value::fromStaticValue(scope.engine->currentStackFrame->jsFrame->function));
    ScopedObject homeObject(scope, f->getHomeObject());
    if (!homeObject) {
        ScopedContext ctx(scope, static_cast<ExecutionContext *>(&scope.engine->currentStackFrame->jsFrame->context));
        Q_ASSERT(ctx);
        while (ctx) {
            if (CallContext *c = ctx->asCallContext()) {
                f = c->d()->function;
                QV4::Function *fn = f->function();
                if (fn && !fn->isArrowFunction() && !fn->isEval)
                    break;
            }
            ctx = ctx->d()->outer;
        }
        homeObject = f->getHomeObject();
    }
    if (!homeObject) {
        scope.engine->throwTypeError();
        return nullptr;
    }
    Q_ASSERT(homeObject);
    ScopedObject proto(scope, homeObject->getPrototypeOf());
    if (!proto) {
        scope.engine->throwTypeError();
        return nullptr;
    }
    return proto;
}

ReturnedValue Runtime::LoadSuperProperty::call(ExecutionEngine *engine, const Value &property)
{
    Scope scope(engine);
    Object *base = getSuperBase(scope);
    if (!base)
        return Encode::undefined();
    ScopedPropertyKey key(scope, property.toPropertyKey(engine));
    if (engine->hasException)
        return Encode::undefined();
    return base->get(
            key, &(engine->currentStackFrame->jsFrame->thisObject.asValue<Value>()));
}

void Runtime::StoreSuperProperty::call(ExecutionEngine *engine, const Value &property, const Value &value)
{
    Scope scope(engine);
    Object *base = getSuperBase(scope);
    if (!base)
        return;
    ScopedPropertyKey key(scope, property.toPropertyKey(engine));
    if (engine->hasException)
        return;
    bool result = base->put(
            key, value, &(engine->currentStackFrame->jsFrame->thisObject.asValue<Value>()));
    if (!result && engine->currentStackFrame->v4Function->isStrict())
        engine->throwTypeError();
}

ReturnedValue Runtime::LoadGlobalLookup::call(ExecutionEngine *engine, Function *f, int index)
{
    Lookup *l = runtimeLookup(f, index);
    return l->globalGetter(l, engine);
}

ReturnedValue Runtime::LoadQmlContextPropertyLookup::call(ExecutionEngine *engine, uint index)
{
    Lookup *l = runtimeLookup(engine->currentStackFrame->v4Function, index);
    return l->qmlContextPropertyGetter(l, engine, nullptr);
}

ReturnedValue Runtime::GetLookup::call(ExecutionEngine *engine, Function *f, const Value &base, int index)
{
    Lookup *l = runtimeLookup(f, index);
    return l->getter(l, engine, base);
}

void Runtime::SetLookupSloppy::call(Function *f, const Value &base, int index, const Value &value)
{
    ExecutionEngine *engine = f->internalClass->engine;
    QV4::Lookup *l = runtimeLookup(f, index);
    l->setter(l, engine, const_cast<Value &>(base), value);
}

void Runtime::SetLookupStrict::call(Function *f, const Value &base, int index, const Value &value)
{
    ExecutionEngine *engine = f->internalClass->engine;
    QV4::Lookup *l = runtimeLookup(f, index);
    if (!l->setter(l, engine, const_cast<Value &>(base), value))
        engine->throwTypeError();
}

ReturnedValue Runtime::LoadSuperConstructor::call(ExecutionEngine *engine, const Value &t)
{
    if (engine->currentStackFrame->thisObject() != Value::emptyValue().asReturnedValue()) {
        return engine->throwReferenceError(QStringLiteral("super() already called."), QString(), 0, 0); // ### fix line number
    }
    const FunctionObject *f = t.as<FunctionObject>();
    if (!f)
        return engine->throwTypeError();
    Heap::Object *c = static_cast<const Object &>(t).getPrototypeOf();
    if (!c->vtable()->isFunctionObject || !static_cast<Heap::FunctionObject *>(c)->isConstructor())
        return engine->throwTypeError();
    return c->asReturnedValue();
}

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
        return Runtime::CompareEqual::call(Value::fromDouble((double) x.booleanValue()), y);
    } else if (y.isBoolean()) {
        return Runtime::CompareEqual::call(x, Value::fromDouble((double) y.booleanValue()));
    } else {
        Object *xo = x.objectValue();
        Object *yo = y.objectValue();
        if (yo && (x.isNumber() || x.isString())) {
            Scope scope(yo->engine());
            ScopedValue py(scope, RuntimeHelpers::objectDefaultValue(yo, PREFERREDTYPE_HINT));
            return Runtime::CompareEqual::call(x, py);
        } else if (xo && (y.isNumber() || y.isString())) {
            Scope scope(xo->engine());
            ScopedValue px(scope, RuntimeHelpers::objectDefaultValue(xo, PREFERREDTYPE_HINT));
            return Runtime::CompareEqual::call(px, y);
        }
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
    if (x.isManaged()) {
        return y.isManaged() && x.cast<Managed>()->isEqualTo(y.cast<Managed>());
    }
    return false;
}

QV4::Bool Runtime::CompareGreaterThan::call(const Value &l, const Value &r)
{
    TRACE2(l, r);
    if (l.isInteger() && r.isInteger())
        return l.integerValue() > r.integerValue();
    if (l.isNumber() && r.isNumber())
        return l.asDouble() > r.asDouble();
    String *sl = l.stringValue();
    String *sr = r.stringValue();
    if (sl && sr) {
        return sr->lessThan(sl);
    }

    Object *ro = r.objectValue();
    Object *lo = l.objectValue();
    if (ro || lo) {
        QV4::ExecutionEngine *e = (lo ? lo : ro)->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, lo ? RuntimeHelpers::objectDefaultValue(lo, QV4::NUMBER_HINT) : l.asReturnedValue());
        QV4::ScopedValue pr(scope, ro ? RuntimeHelpers::objectDefaultValue(ro, QV4::NUMBER_HINT) : r.asReturnedValue());
        return Runtime::CompareGreaterThan::call(pl, pr);
    }

    double dl = RuntimeHelpers::toNumber(l);
    double dr = RuntimeHelpers::toNumber(r);
    return dl > dr;
}

QV4::Bool Runtime::CompareLessThan::call(const Value &l, const Value &r)
{
    TRACE2(l, r);
    if (l.isInteger() && r.isInteger())
        return l.integerValue() < r.integerValue();
    if (l.isNumber() && r.isNumber())
        return l.asDouble() < r.asDouble();
    String *sl = l.stringValue();
    String *sr = r.stringValue();
    if (sl && sr) {
        return sl->lessThan(sr);
    }

    Object *ro = r.objectValue();
    Object *lo = l.objectValue();
    if (ro || lo) {
        QV4::ExecutionEngine *e = (lo ? lo : ro)->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, lo ? RuntimeHelpers::objectDefaultValue(lo, QV4::NUMBER_HINT) : l.asReturnedValue());
        QV4::ScopedValue pr(scope, ro ? RuntimeHelpers::objectDefaultValue(ro, QV4::NUMBER_HINT) : r.asReturnedValue());
        return Runtime::CompareLessThan::call(pl, pr);
    }

    double dl = RuntimeHelpers::toNumber(l);
    double dr = RuntimeHelpers::toNumber(r);
    return dl < dr;
}

QV4::Bool Runtime::CompareGreaterEqual::call(const Value &l, const Value &r)
{
    TRACE2(l, r);
    if (l.isInteger() && r.isInteger())
        return l.integerValue() >= r.integerValue();
    if (l.isNumber() && r.isNumber())
        return l.asDouble() >= r.asDouble();
    String *sl = l.stringValue();
    String *sr = r.stringValue();
    if (sl && sr) {
        return !sl->lessThan(sr);
    }

    Object *ro = r.objectValue();
    Object *lo = l.objectValue();
    if (ro || lo) {
        QV4::ExecutionEngine *e = (lo ? lo : ro)->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, lo ? RuntimeHelpers::objectDefaultValue(lo, QV4::NUMBER_HINT) : l.asReturnedValue());
        QV4::ScopedValue pr(scope, ro ? RuntimeHelpers::objectDefaultValue(ro, QV4::NUMBER_HINT) : r.asReturnedValue());
        return Runtime::CompareGreaterEqual::call(pl, pr);
    }

    double dl = RuntimeHelpers::toNumber(l);
    double dr = RuntimeHelpers::toNumber(r);
    return dl >= dr;
}

QV4::Bool Runtime::CompareLessEqual::call(const Value &l, const Value &r)
{
    TRACE2(l, r);
    if (l.isInteger() && r.isInteger())
        return l.integerValue() <= r.integerValue();
    if (l.isNumber() && r.isNumber())
        return l.asDouble() <= r.asDouble();
    String *sl = l.stringValue();
    String *sr = r.stringValue();
    if (sl && sr) {
        return !sr->lessThan(sl);
    }

    Object *ro = r.objectValue();
    Object *lo = l.objectValue();
    if (ro || lo) {
        QV4::ExecutionEngine *e = (lo ? lo : ro)->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, lo ? RuntimeHelpers::objectDefaultValue(lo, QV4::NUMBER_HINT) : l.asReturnedValue());
        QV4::ScopedValue pr(scope, ro ? RuntimeHelpers::objectDefaultValue(ro, QV4::NUMBER_HINT) : r.asReturnedValue());
        return Runtime::CompareLessEqual::call(pl, pr);
    }

    double dl = RuntimeHelpers::toNumber(l);
    double dr = RuntimeHelpers::toNumber(r);
    return dl <= dr;
}

Bool Runtime::CompareInstanceof::call(ExecutionEngine *engine, const Value &left, const Value &right)
{
    TRACE2(left, right);

    Scope scope(engine);
    ScopedValue v(scope, Instanceof::call(engine, left, right));
    return v->booleanValue();
}

uint Runtime::CompareIn::call(ExecutionEngine *engine, const Value &left, const Value &right)
{
    TRACE2(left, right);

    Scope scope(engine);
    ScopedValue v(scope, In::call(engine, left, right));
    return v->booleanValue();
}

static ReturnedValue throwPropertyIsNotAFunctionTypeError(ExecutionEngine *engine, Value *thisObject, const QString &propertyName)
{
    QString objectAsString = QStringLiteral("[null]");
    if (!thisObject->isUndefined())
        objectAsString = thisObject->toQStringNoThrow();
    QString msg = QStringLiteral("Property '%1' of object %2 is not a function")
                  .arg(propertyName, objectAsString);
    return engine->throwTypeError(msg);
}

ReturnedValue Runtime::CallGlobalLookup::call(ExecutionEngine *engine, uint index, Value argv[], int argc)
{
    Scope scope(engine);
    Lookup *l = runtimeLookup(engine->currentStackFrame->v4Function, index);
    Value function = Value::fromReturnedValue(l->globalGetter(l, engine));
    Value thisObject = Value::undefinedValue();
    if (!function.isFunctionObject()) {
        return throwPropertyIsNotAFunctionTypeError(engine, &thisObject,
                                                    engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[l->nameIndex]->toQString());
    }

    return checkedResult(engine, static_cast<FunctionObject &>(function).call(
                             &thisObject, argv, argc));
}

ReturnedValue Runtime::CallQmlContextPropertyLookup::call(ExecutionEngine *engine, uint index,
                                                          Value *argv, int argc)
{
    Scope scope(engine);
    ScopedValue thisObject(scope);
    Lookup *l = runtimeLookup(engine->currentStackFrame->v4Function, index);
    Value function = Value::fromReturnedValue(l->qmlContextPropertyGetter(l, engine, thisObject));
    if (!function.isFunctionObject()) {
        return throwPropertyIsNotAFunctionTypeError(engine, thisObject,
                                                    engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[l->nameIndex]->toQString());
    }

    return checkedResult(engine, static_cast<FunctionObject &>(function).call(
                             thisObject, argv, argc));
}

ReturnedValue Runtime::CallPossiblyDirectEval::call(ExecutionEngine *engine, Value *argv, int argc)
{
    Scope scope(engine);
    ScopedValue thisObject(scope);

    ExecutionContext &ctx = static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context);
    ScopedFunctionObject function(scope, ctx.getPropertyAndBase(engine->id_eval(), thisObject));
    if (engine->hasException)
        return Encode::undefined();

    if (!function)
        return throwPropertyIsNotAFunctionTypeError(engine, thisObject, QLatin1String("eval"));

    if (function->d() == engine->evalFunction()->d())
        return static_cast<EvalFunction *>(function.getPointer())->evalCall(thisObject, argv, argc, true);

    return checkedResult(engine, function->call(thisObject, argv, argc));
}

ReturnedValue Runtime::CallName::call(ExecutionEngine *engine, int nameIndex, Value *argv, int argc)
{
    Scope scope(engine);
    ScopedValue thisObject(scope);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);

    ExecutionContext &ctx = static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context);
    ScopedFunctionObject f(scope, ctx.getPropertyAndBase(name, thisObject));
    if (engine->hasException)
        return Encode::undefined();

    if (!f) {
        return throwPropertyIsNotAFunctionTypeError(
                engine, thisObject, engine->currentStackFrame->v4Function->compilationUnit
                                            ->runtimeStrings[nameIndex]->toQString());
    }

    return checkedResult(engine, f->call(thisObject, argv, argc));
}

ReturnedValue Runtime::CallProperty::call(ExecutionEngine *engine, const Value &baseRef, int nameIndex, Value *argv, int argc)
{
    const Value *base = &baseRef;
    Scope scope(engine);
    ScopedString name(
            scope,
            engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    ScopedObject lookupObject(scope, base);

    if (!lookupObject) {
        Q_ASSERT(!base->isEmpty());
        if (base->isNullOrUndefined()) {
            QString message = QStringLiteral("Cannot call method '%1' of %2")
                    .arg(name->toQString(), base->toQStringNoThrow());
            return engine->throwTypeError(message);
        }

        if (base->isManaged()) {
            const Managed *m = static_cast<const Managed *>(base);
            lookupObject = m->internalClass()->prototype;
            Q_ASSERT(m->internalClass()->prototype);
        } else {
            lookupObject = RuntimeHelpers::convertToObject(engine, *base);
            if (engine->hasException) // type error
                return Encode::undefined();
            if (!engine->currentStackFrame->v4Function->isStrict())
                base = lookupObject;
        }
    }

    ScopedFunctionObject f(scope, static_cast<Object *>(lookupObject)->get(name));

    if (!f) {
        QString error = QStringLiteral("Property '%1' of object %2 is not a function")
                .arg(name->toQString(),
                     base->toQStringNoThrow());
        return engine->throwTypeError(error);
    }

    return checkedResult(engine, f->call(base, argv, argc));
}

ReturnedValue Runtime::CallPropertyLookup::call(ExecutionEngine *engine, const Value &base, uint index, Value *argv, int argc)
{
    Lookup *l = runtimeLookup(engine->currentStackFrame->v4Function, index);
    // ok to have the value on the stack here
    Value f = Value::fromReturnedValue(l->getter(l, engine, base));

    if (!f.isFunctionObject())
        return engine->throwTypeError();

    return checkedResult(engine, static_cast<FunctionObject &>(f).call(&base, argv, argc));
}

ReturnedValue Runtime::CallElement::call(ExecutionEngine *engine, const Value &baseRef, const Value &index, Value *argv, int argc)
{
    const Value *base = &baseRef;
    Scope scope(engine);
    ScopedValue thisObject(scope, base->toObject(engine));
    base = thisObject;

    ScopedPropertyKey str(scope, index.toPropertyKey(engine));
    if (engine->hasException)
        return Encode::undefined();

    ScopedFunctionObject f(scope, static_cast<const Object *>(base)->get(str));
    if (!f)
        return engine->throwTypeError();

    return checkedResult(engine, f->call(base, argv, argc));
}

ReturnedValue Runtime::CallValue::call(ExecutionEngine *engine, const Value &func, Value *argv, int argc)
{
    if (!func.isFunctionObject())
        return engine->throwTypeError(QStringLiteral("%1 is not a function").arg(func.toQStringNoThrow()));
    Value undef = Value::undefinedValue();
    return checkedResult(engine, static_cast<const FunctionObject &>(func).call(
                             &undef, argv, argc));
}

ReturnedValue Runtime::CallWithReceiver::call(ExecutionEngine *engine, const Value &func,
                                               const Value &thisObject, Value argv[], int argc)
{
    if (!func.isFunctionObject())
        return engine->throwTypeError(QStringLiteral("%1 is not a function").arg(func.toQStringNoThrow()));
    return checkedResult(engine, static_cast<const FunctionObject &>(func).call(
                             &thisObject, argv, argc));
}

struct CallArgs {
    Value *argv;
    int argc;
};

static CallArgs createSpreadArguments(Scope &scope, Value *argv, int argc)
{
    ScopedValue it(scope);
    ScopedValue done(scope);

    int argCount = 0;

    Value *v = scope.alloc<Scope::Uninitialized>();
    Value *arguments = v;
    for (int i = 0; i < argc; ++i) {
        if (!argv[i].isEmpty()) {
            *v = argv[i];
            ++argCount;
            v = scope.alloc<Scope::Uninitialized>();
            continue;
        }
        // spread element
        ++i;
        it = Runtime::GetIterator::call(scope.engine, argv[i], /* ForInIterator */ 1);
        if (scope.engine->hasException)
            return { nullptr, 0 };
        while (1) {
            done = Runtime::IteratorNext::call(scope.engine, it, v);
            if (scope.engine->hasException)
                return { nullptr, 0 };
            Q_ASSERT(done->isBoolean());
            if (done->booleanValue())
                break;
            ++argCount;
            v = scope.alloc<Scope::Uninitialized>();
        }
    }
    return { arguments, argCount };
}

ReturnedValue Runtime::CallWithSpread::call(ExecutionEngine *engine, const Value &function, const Value &thisObject, Value *argv, int argc)
{
    Q_ASSERT(argc >= 1);
    if (!function.isFunctionObject())
        return engine->throwTypeError();

    Scope scope(engine);
    CallArgs arguments = createSpreadArguments(scope, argv, argc);
    if (engine->hasException)
        return Encode::undefined();

    return checkedResult(engine, static_cast<const FunctionObject &>(function).call(
                             &thisObject, arguments.argv, arguments.argc));
}

ReturnedValue Runtime::Construct::call(ExecutionEngine *engine, const Value &function, const Value &newTarget, Value *argv, int argc)
{
    if (!function.isFunctionObject())
        return engine->throwTypeError();

    return static_cast<const FunctionObject &>(function).callAsConstructor(argv, argc, &newTarget);
}

ReturnedValue Runtime::ConstructWithSpread::call(ExecutionEngine *engine, const Value &function, const Value &newTarget, Value *argv, int argc)
{
    if (!function.isFunctionObject())
        return engine->throwTypeError();

    Scope scope(engine);
    CallArgs arguments = createSpreadArguments(scope, argv, argc);
    if (engine->hasException)
        return Encode::undefined();

    return static_cast<const FunctionObject &>(function).callAsConstructor(arguments.argv, arguments.argc, &newTarget);
}

ReturnedValue Runtime::TailCall::call(CppStackFrame *frame, ExecutionEngine *engine)
{
    // IMPORTANT! The JIT assumes that this method has the same amount (or less) arguments than
    // the jitted function, so it can safely do a tail call.

    Value *tos = engine->jsStackTop;
    const Value &function = tos[StackOffsets::tailCall_function];
    const Value &thisObject = tos[StackOffsets::tailCall_thisObject];
    Value *argv = reinterpret_cast<Value *>(frame->jsFrame) + tos[StackOffsets::tailCall_argv].int_32();
    int argc = tos[StackOffsets::tailCall_argc].int_32();
    Q_ASSERT(argc >= 0);

    if (!function.isFunctionObject())
        return engine->throwTypeError();

    const FunctionObject &fo = static_cast<const FunctionObject &>(function);
    if (!frame->callerCanHandleTailCall || !fo.canBeTailCalled() || engine->debugger()
            || unsigned(argc) > fo.formalParameterCount()) {
        // Cannot tailcall, do a normal call:
        return checkedResult(engine, fo.call(&thisObject, argv, argc));
    }

    memcpy(frame->jsFrame->args, argv, argc * sizeof(Value));
    frame->init(engine, fo.function(), frame->jsFrame->argValues<Value>(), argc,
                frame->callerCanHandleTailCall);
    frame->setupJSFrame(frame->savedStackTop, fo, fo.scope(), thisObject, Primitive::undefinedValue());
    engine->jsStackTop = frame->savedStackTop + frame->requiredJSStackFrameSize();
    frame->pendingTailCall = true;
    return Encode::undefined();
}

void Runtime::ThrowException::call(ExecutionEngine *engine, const Value &value)
{
    if (!value.isEmpty())
        engine->throwError(value);
}

ReturnedValue Runtime::TypeofValue::call(ExecutionEngine *engine, const Value &value)
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
        else if (value.isSymbol())
            res = engine->id_symbol();
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

QV4::ReturnedValue Runtime::TypeofName::call(ExecutionEngine *engine, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    ScopedValue prop(scope, static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context).getProperty(name));
    // typeof doesn't throw. clear any possible exception
    scope.engine->hasException = false;
    return TypeofValue::call(engine, prop);
}

void Runtime::PushCallContext::call(CppStackFrame *frame)
{
    frame->jsFrame->context = ExecutionContext::newCallContext(frame)->asReturnedValue();
}

ReturnedValue Runtime::PushWithContext::call(ExecutionEngine *engine, const Value &acc)
{
    CallData *jsFrame = engine->currentStackFrame->jsFrame;
    Value &newAcc = jsFrame->accumulator.asValue<Value>();
    newAcc = Value::fromHeapObject(acc.toObject(engine));
    if (!engine->hasException) {
        Q_ASSERT(newAcc.isObject());
        const Object &obj = static_cast<const Object &>(newAcc);
        Value &context = jsFrame->context.asValue<Value>();
        auto ec = static_cast<const ExecutionContext *>(&context);
        context = ec->newWithContext(obj.d())->asReturnedValue();
    }
    return newAcc.asReturnedValue();
}

void Runtime::PushCatchContext::call(ExecutionEngine *engine, int blockIndex, int exceptionVarNameIndex)
{
    auto name = engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[exceptionVarNameIndex];
    engine->currentStackFrame->jsFrame->context = ExecutionContext::newCatchContext(engine->currentStackFrame, blockIndex, name)->asReturnedValue();
}

void Runtime::PushBlockContext::call(ExecutionEngine *engine, int index)
{
    engine->currentStackFrame->jsFrame->context = ExecutionContext::newBlockContext(engine->currentStackFrame, index)->asReturnedValue();
}

void Runtime::CloneBlockContext::call(ExecutionEngine *engine)
{
    auto frame = engine->currentStackFrame;
    auto context = static_cast<Heap::CallContext *>(
            Value::fromStaticValue(frame->jsFrame->context).m());
    frame->jsFrame->context =
            ExecutionContext::cloneBlockContext(engine, context)->asReturnedValue();
}

void Runtime::PushScriptContext::call(ExecutionEngine *engine, int index)
{
    Q_ASSERT(engine->currentStackFrame->context()->d()->type == Heap::ExecutionContext::Type_GlobalContext ||
             engine->currentStackFrame->context()->d()->type == Heap::ExecutionContext::Type_QmlContext);
    ReturnedValue c = ExecutionContext::newBlockContext(engine->currentStackFrame, index)->asReturnedValue();
    engine->setScriptContext(c);
    engine->currentStackFrame->jsFrame->context = c;
}

void Runtime::PopScriptContext::call(ExecutionEngine *engine)
{
    ReturnedValue root = engine->rootContext()->asReturnedValue();
    engine->setScriptContext(root);
    engine->currentStackFrame->jsFrame->context = root;
}

void Runtime::ThrowReferenceError::call(ExecutionEngine *engine, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    engine->throwReferenceError(name);
}

void Runtime::ThrowOnNullOrUndefined::call(ExecutionEngine *engine, const Value &v)
{
    if (v.isNullOrUndefined())
        engine->throwTypeError();
}

ReturnedValue Runtime::ConvertThisToObject::call(ExecutionEngine *engine, const Value &t)
{
    if (!t.isObject()) {
        if (t.isNullOrUndefined()) {
            return engine->globalObject->asReturnedValue();
        } else {
            return t.toObject(engine)->asReturnedValue();
        }
    }
    return t.asReturnedValue();
}

void Runtime::DeclareVar::call(ExecutionEngine *engine, Bool deletable, int nameIndex)
{
    Scope scope(engine);
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context).createMutableBinding(name, deletable);
}

ReturnedValue Runtime::ArrayLiteral::call(ExecutionEngine *engine, Value *values, uint length)
{
    return engine->newArrayObject(values, length)->asReturnedValue();
}

ReturnedValue Runtime::ObjectLiteral::call(ExecutionEngine *engine, int classId, QV4::Value args[], int argc)
{
    Scope scope(engine);
    Scoped<InternalClass> klass(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeClasses[classId]);
    ScopedObject o(scope, engine->newObject(klass->d()));

    Q_ASSERT(uint(argc) >= klass->d()->size);

    for (uint i = 0; i < klass->d()->size; ++i)
        o->setProperty(i, *args++);

    Q_ASSERT((argc - klass->d()->size) % 3 == 0);
    int additionalArgs = (argc - int(klass->d()->size))/3;

    if (!additionalArgs)
        return o->asReturnedValue();

    ScopedPropertyKey name(scope);
    ScopedProperty pd(scope);
    ScopedFunctionObject fn(scope);
    ScopedString fnName(scope);
    ScopedValue value(scope);
    for (int i = 0; i < additionalArgs; ++i) {
        Q_ASSERT(args->isInteger());
        ObjectLiteralArgument arg = ObjectLiteralArgument(args->integerValue());
        name = args[1].toPropertyKey(engine);
        value = args[2];
        if (engine->hasException)
            return Encode::undefined();
        if (arg != ObjectLiteralArgument::Value) {
            Q_ASSERT(args[2].isInteger());
            int functionId = args[2].integerValue();
            QV4::Function *clos = engine->currentStackFrame->v4Function->executableCompilationUnit()
                                          ->runtimeFunctions[functionId];
            Q_ASSERT(clos);

            PropertyKey::FunctionNamePrefix prefix = PropertyKey::None;
            if (arg == ObjectLiteralArgument::Getter)
                prefix = PropertyKey::Getter;
            else if (arg == ObjectLiteralArgument::Setter)
                prefix = PropertyKey::Setter;
            else
                arg = ObjectLiteralArgument::Value;
            fnName = name->asFunctionName(engine, prefix);

            ExecutionContext *current = static_cast<ExecutionContext *>(&engine->currentStackFrame->jsFrame->context);
            if (clos->isGenerator())
                value = MemberGeneratorFunction::create(current, clos, o, fnName)->asReturnedValue();
            else
                value = FunctionObject::createMemberFunction(current, clos, o, fnName)->asReturnedValue();
        } else if (args[2].isFunctionObject()) {
            fn = static_cast<const FunctionObject &>(args[2]);

            fnName = name->asFunctionName(engine, PropertyKey::None);
            fn->setName(fnName);
        }
        Q_ASSERT(arg != ObjectLiteralArgument::Method);
        Q_ASSERT(arg == ObjectLiteralArgument::Value || value->isFunctionObject());
        if (arg == ObjectLiteralArgument::Value || arg == ObjectLiteralArgument::Getter) {
            pd->value = value;
            pd->set = Value::emptyValue();
        } else {
            pd->value = Value::emptyValue();
            pd->set = value;
        }
        bool ok = o->defineOwnProperty(name, pd, (arg == ObjectLiteralArgument::Value ? Attr_Data : Attr_Accessor));
        if (!ok)
            return engine->throwTypeError();

        args += 3;
    }
    return o.asReturnedValue();
}

ReturnedValue Runtime::CreateClass::call(ExecutionEngine *engine, int classIndex,
                                          const Value &superClass, Value computedNames[])
{
    const QV4::ExecutableCompilationUnit *unit
            = engine->currentStackFrame->v4Function->executableCompilationUnit();
    const QV4::CompiledData::Class *cls = unit->unitData()->classAt(classIndex);

    Scope scope(engine);
    ScopedObject protoParent(scope, engine->objectPrototype());
    ScopedObject constructorParent(scope, engine->functionPrototype());
    if (!superClass.isEmpty()) {
        if (superClass.isNull()) {
            protoParent = Encode::null();
        } else {
            const FunctionObject *superFunction = superClass.as<FunctionObject>();
            // ### check that the heritage object is a constructor
            if (!superFunction || !superFunction->isConstructor())
                return engine->throwTypeError(QStringLiteral("The superclass is not a function object."));
            const FunctionObject *s = static_cast<const FunctionObject *>(&superClass);
            ScopedValue result(scope, s->get(scope.engine->id_prototype()));
            if (!result->isObject() && !result->isNull())
                return engine->throwTypeError(QStringLiteral("The value of the superclass's prototype property is not an object."));
            protoParent = *result;
            constructorParent = superClass;
        }
    }

    ScopedObject proto(scope, engine->newObject());
    proto->setPrototypeUnchecked(protoParent);
    ExecutionContext *current = static_cast<ExecutionContext *>(&engine->currentStackFrame->jsFrame->context);

    ScopedFunctionObject constructor(scope);
    QV4::Function *f = cls->constructorFunction != UINT_MAX ? unit->runtimeFunctions[cls->constructorFunction] : nullptr;
    constructor = FunctionObject::createConstructorFunction(current, f, proto, !superClass.isEmpty())->asReturnedValue();
    constructor->setPrototypeUnchecked(constructorParent);
    Value argCount = Value::fromInt32(f ? f->nFormals : 0);
    constructor->defineReadonlyConfigurableProperty(scope.engine->id_length(), argCount);
    constructor->defineReadonlyConfigurableProperty(engine->id_prototype(), proto);
    proto->defineDefaultProperty(engine->id_constructor(), constructor);

    ScopedString name(scope);
    if (cls->nameIndex != UINT_MAX) {
        name = unit->runtimeStrings[cls->nameIndex];
        constructor->defineReadonlyConfigurableProperty(engine->id_name(), name);
    }

    ScopedObject receiver(scope, *constructor);
    ScopedPropertyKey propertyName(scope);
    ScopedFunctionObject function(scope);
    ScopedProperty property(scope);
    const CompiledData::Method *methods = cls->methodTable();
    for (uint i = 0; i < cls->nStaticMethods + cls->nMethods; ++i) {
        if (i == cls->nStaticMethods)
            receiver = proto;
        if (methods[i].name == UINT_MAX) {
            propertyName = computedNames->toPropertyKey(engine);
            if (propertyName == scope.engine->id_prototype()->propertyKey() && receiver->d() == constructor->d())
                return engine->throwTypeError(QStringLiteral("Cannot declare a static method named 'prototype'."));
            if (engine->hasException)
                return Encode::undefined();
            ++computedNames;
        } else {
            name = unit->runtimeStrings[methods[i].name];
            propertyName = name->toPropertyKey();
        }
        QV4::Function *f = unit->runtimeFunctions[methods[i].function];
        Q_ASSERT(f);
        PropertyKey::FunctionNamePrefix prefix = PropertyKey::None;
        if (methods[i].type == CompiledData::Method::Getter)
            prefix = PropertyKey::Getter;
        else if (methods[i].type == CompiledData::Method::Setter)
            prefix = PropertyKey::Setter;

        name = propertyName->asFunctionName(engine, prefix);

        if (f->isGenerator())
            function = MemberGeneratorFunction::create(current, f, receiver, name);
        else
            function = FunctionObject::createMemberFunction(current, f, receiver, name);
        Q_ASSERT(function);
        PropertyAttributes attributes;
        switch (methods[i].type) {
        case CompiledData::Method::Getter:
            property->setGetter(function);
            property->set = Value::emptyValue();
            attributes = Attr_Accessor|Attr_NotEnumerable;
            break;
        case CompiledData::Method::Setter:
            property->value = Value::emptyValue();
            property->setSetter(function);
            attributes = Attr_Accessor|Attr_NotEnumerable;
            break;
        default: // Regular
            property->value = function;
            property->set = Value::emptyValue();
            attributes = Attr_Data|Attr_NotEnumerable;
            break;
        }
        receiver->defineOwnProperty(propertyName, property, attributes);
    }

    return constructor->asReturnedValue();
}

QV4::ReturnedValue Runtime::CreateMappedArgumentsObject::call(ExecutionEngine *engine)
{
    Q_ASSERT(engine->currentContext()->d()->type == Heap::ExecutionContext::Type_CallContext);
    Heap::InternalClass *ic = engine->internalClasses(EngineBase::Class_ArgumentsObject);
    return engine->memoryManager->allocObject<ArgumentsObject>(ic, engine->currentStackFrame)->asReturnedValue();
}

QV4::ReturnedValue Runtime::CreateUnmappedArgumentsObject::call(ExecutionEngine *engine)
{
    Heap::InternalClass *ic = engine->internalClasses(EngineBase::Class_StrictArgumentsObject);
    return engine->memoryManager->allocObject<StrictArgumentsObject>(ic, engine->currentStackFrame)->asReturnedValue();
}

QV4::ReturnedValue Runtime::CreateRestParameter::call(ExecutionEngine *engine, int argIndex)
{
    const Value *values = engine->currentStackFrame->originalArguments + argIndex;
    int nValues = engine->currentStackFrame->originalArgumentsCount - argIndex;
    if (nValues <= 0)
        return engine->newArrayObject(0)->asReturnedValue();
    return engine->newArrayObject(values, nValues)->asReturnedValue();
}

ReturnedValue Runtime::RegexpLiteral::call(ExecutionEngine *engine, int id)
{
    const auto val
            = engine->currentStackFrame->v4Function->compilationUnit->runtimeRegularExpressions[id];
    Heap::RegExpObject *ro = engine->newRegExpObject(Value::fromStaticValue(val).as<RegExp>());
    return ro->asReturnedValue();
}

ReturnedValue Runtime::ToObject::call(ExecutionEngine *engine, const Value &obj)
{
    if (obj.isObject())
        return obj.asReturnedValue();

    return obj.toObject(engine)->asReturnedValue();
}

Bool Runtime::ToBoolean::call(const Value &obj)
{
    return obj.toBoolean();
}

ReturnedValue Runtime::ToNumber::call(ExecutionEngine *, const Value &v)
{
    return Encode(v.toNumber());
}

ReturnedValue Runtime::UMinus::call(const Value &value)
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

ReturnedValue Runtime::Add::call(ExecutionEngine *engine, const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Q_LIKELY(left.integerCompatible() && right.integerCompatible()))
        return add_int32(left.integerValue(), right.integerValue());
    if (left.isNumber() && right.isNumber())
        return Value::fromDouble(left.asDouble() + right.asDouble()).asReturnedValue();

    return RuntimeHelpers::addHelper(engine, left, right);
}

ReturnedValue Runtime::Sub::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Q_LIKELY(left.integerCompatible() && right.integerCompatible()))
        return sub_int32(left.integerValue(), right.integerValue());

    double lval = left.isNumber() ? left.asDouble() : left.toNumberImpl();
    double rval = right.isNumber() ? right.asDouble() : right.toNumberImpl();

    return Value::fromDouble(lval - rval).asReturnedValue();
}

ReturnedValue Runtime::Mul::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Q_LIKELY(left.integerCompatible() && right.integerCompatible()))
        return mul_int32(left.integerValue(), right.integerValue());

    double lval = left.isNumber() ? left.asDouble() : left.toNumberImpl();
    double rval = right.isNumber() ? right.asDouble() : right.toNumberImpl();

    return Value::fromDouble(lval * rval).asReturnedValue();
}

ReturnedValue Runtime::Div::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right)) {
        int lval = left.integerValue();
        int rval = right.integerValue();
        if (rval != 0 // division by zero should result in a NaN
                && !(lval == std::numeric_limits<int>::min() && rval == -1) // doesn't fit in int
                && (lval % rval == 0)  // fractions can't be stored in an int
                && !(lval == 0 && rval < 0)) // 0 / -something results in -0.0
            return Encode(int(lval / rval));
        else
            return Encode(double(lval) / rval);
    }

    double lval = left.toNumber();
    double rval = right.toNumber();
    return Value::fromDouble(lval / rval).asReturnedValue();
}

ReturnedValue Runtime::Mod::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right) && left.integerValue() >= 0 && right.integerValue() > 0) {
        // special cases are handled by fmod, among them:
        //  - arithmic execeptions for ints in c++, eg: INT_MIN % -1
        //  - undefined behavior in c++, e.g.: anything % 0
        //  - uncommon cases which would complicate the condition, e.g.: negative integers
        //    (this makes sure that -1 % 1 == -0 by passing it to fmod)
        return Encode(left.integerValue() % right.integerValue());
    }

    double lval = RuntimeHelpers::toNumber(left);
    double rval = RuntimeHelpers::toNumber(right);
#ifdef fmod
#  undef fmod
#endif
    return Value::fromDouble(std::fmod(lval, rval)).asReturnedValue();
}

ReturnedValue Runtime::Exp::call(const Value &base, const Value &exp)
{
    double b = base.toNumber();
    double e = exp.toNumber();
    if (qt_is_inf(e) && (b == 1 || b == -1))
        return Encode(qt_qnan());
    return Encode(pow(b,e));
}

ReturnedValue Runtime::BitAnd::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    int lval = left.toInt32();
    int rval = right.toInt32();
    return Encode((int)(lval & rval));
}

ReturnedValue Runtime::BitOr::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    int lval = left.toInt32();
    int rval = right.toInt32();
    return Encode((int)(lval | rval));
}

ReturnedValue Runtime::BitXor::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    int lval = left.toInt32();
    int rval = right.toInt32();
    return Encode((int)(lval ^ rval));
}

ReturnedValue Runtime::Shl::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    int lval = left.toInt32();
    int rval = right.toInt32() & 0x1f;
    return Encode((int)(lval << rval));
}

ReturnedValue Runtime::Shr::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    int lval = left.toInt32();
    unsigned rval = right.toUInt32() & 0x1f;
    return Encode((int)(lval >> rval));
}

ReturnedValue Runtime::UShr::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    unsigned lval = left.toUInt32();
    unsigned rval = right.toUInt32() & 0x1f;
    uint res = lval >> rval;

    return Encode(res);
}

ReturnedValue Runtime::GreaterThan::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = CompareGreaterThan::call(left, right);
    return Encode(r);
}

ReturnedValue Runtime::LessThan::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = CompareLessThan::call(left, right);
    return Encode(r);
}

ReturnedValue Runtime::GreaterEqual::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = CompareGreaterEqual::call(left, right);
    return Encode(r);
}

ReturnedValue Runtime::LessEqual::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = CompareLessEqual::call(left, right);
    return Encode(r);
}

struct LazyScope
{
    ExecutionEngine *engine = nullptr;
    Value *stackMark = nullptr;
    ~LazyScope() {
        if (engine)
            engine->jsStackTop = stackMark;
    }
    template <typename T>
    void set(Value **scopedValue, T value, ExecutionEngine *e) {
        if (!engine) {
            engine = e;
            stackMark = engine->jsStackTop;
        }
        if (!*scopedValue)
            *scopedValue = e->jsAlloca(1);
        **scopedValue = value;
    }
};

Bool Runtime::CompareEqual::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    Value lhs = left;
    Value rhs = right;

    LazyScope scope;
    Value *lhsGuard = nullptr;
    Value *rhsGuard = nullptr;

  redo:
    if (lhs.asReturnedValue() == rhs.asReturnedValue())
        return !lhs.isNaN();

    int lt = lhs.quickType();
    int rt = rhs.quickType();
    if (rt < lt) {
        qSwap(lhs, rhs);
        qSwap(lt, rt);
    }

    switch (lt) {
    case QV4::Value::QT_ManagedOrUndefined:
        if (lhs.isUndefined())
            return rhs.isNullOrUndefined();
        Q_FALLTHROUGH();
    case QV4::Value::QT_ManagedOrUndefined1:
    case QV4::Value::QT_ManagedOrUndefined2:
    case QV4::Value::QT_ManagedOrUndefined3:
        // LHS: Managed
        switch (rt) {
        case QV4::Value::QT_ManagedOrUndefined:
            if (rhs.isUndefined())
                return false;
            Q_FALLTHROUGH();
        case QV4::Value::QT_ManagedOrUndefined1:
        case QV4::Value::QT_ManagedOrUndefined2:
        case QV4::Value::QT_ManagedOrUndefined3: {
            // RHS: Managed
            Heap::Base *l = lhs.m();
            Heap::Base *r = rhs.m();
            Q_ASSERT(l);
            Q_ASSERT(r);
            if (l->internalClass->vtable->isStringOrSymbol == r->internalClass->vtable->isStringOrSymbol)
                return static_cast<QV4::Managed &>(lhs).isEqualTo(&static_cast<QV4::Managed &>(rhs));
            if (l->internalClass->vtable->isStringOrSymbol) {
                scope.set(&rhsGuard, RuntimeHelpers::objectDefaultValue(&static_cast<QV4::Object &>(rhs), PREFERREDTYPE_HINT), r->internalClass->engine);
                rhs = rhsGuard->asReturnedValue();
                break;
            } else {
                Q_ASSERT(r->internalClass->vtable->isStringOrSymbol);
                scope.set(&lhsGuard, RuntimeHelpers::objectDefaultValue(&static_cast<QV4::Object &>(lhs), PREFERREDTYPE_HINT), l->internalClass->engine);
                lhs = lhsGuard->asReturnedValue();
                break;
            }
            return false;
        }
        case QV4::Value::QT_Empty:
            Q_UNREACHABLE();
        case QV4::Value::QT_Null:
            return false;
        case QV4::Value::QT_Bool:
        case QV4::Value::QT_Int:
            rhs = Value::fromDouble(rhs.int_32());
            // fall through
        default: // double
            if (lhs.m()->internalClass->vtable->isStringOrSymbol) {
                return lhs.m()->internalClass->vtable->isString ? (RuntimeHelpers::toNumber(lhs) == rhs.doubleValue()) : false;
            } else {
                scope.set(&lhsGuard, RuntimeHelpers::objectDefaultValue(&static_cast<QV4::Object &>(lhs), PREFERREDTYPE_HINT), lhs.m()->internalClass->engine);
                lhs = lhsGuard->asReturnedValue();
            }
        }
        goto redo;
    case QV4::Value::QT_Empty:
        Q_UNREACHABLE();
    case QV4::Value::QT_Null:
        return rhs.isNull();
    case QV4::Value::QT_Bool:
    case QV4::Value::QT_Int:
        switch (rt) {
        case QV4::Value::QT_ManagedOrUndefined:
        case QV4::Value::QT_ManagedOrUndefined1:
        case QV4::Value::QT_ManagedOrUndefined2:
        case QV4::Value::QT_ManagedOrUndefined3:
        case QV4::Value::QT_Empty:
        case QV4::Value::QT_Null:
            Q_UNREACHABLE();
        case QV4::Value::QT_Bool:
        case QV4::Value::QT_Int:
            return lhs.int_32() == rhs.int_32();
        default: // double
            return lhs.int_32() == rhs.doubleValue();
        }
    default: // double
        Q_ASSERT(rhs.isDouble());
        return lhs.doubleValue() == rhs.doubleValue();
    }
}

ReturnedValue Runtime::Equal::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = CompareEqual::call(left, right);
    return Encode(r);
}

ReturnedValue Runtime::NotEqual::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = !CompareEqual::call(left, right);
    return Encode(r);
}

ReturnedValue Runtime::StrictEqual::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = RuntimeHelpers::strictEqual(left, right);
    return Encode(r);
}

ReturnedValue Runtime::StrictNotEqual::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = ! RuntimeHelpers::strictEqual(left, right);
    return Encode(r);
}

Bool Runtime::CompareNotEqual::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    return !Runtime::CompareEqual::call(left, right);
}

Bool Runtime::CompareStrictEqual::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    return RuntimeHelpers::strictEqual(left, right);
}

Bool Runtime::CompareStrictNotEqual::call(const Value &left, const Value &right)
{
    TRACE2(left, right);

    return ! RuntimeHelpers::strictEqual(left, right);
}

template<typename Operation>
static inline const void *symbol()
{
    return reinterpret_cast<void *>(&Operation::call);
}

QHash<const void *, const char *> Runtime::symbolTable()
{
    static const QHash<const void *, const char *> symbols({
#ifndef V4_BOOTSTRAP
            {symbol<CallGlobalLookup>(), "CallGlobalLookup" },
            {symbol<CallQmlContextPropertyLookup>(), "CallQmlContextPropertyLookup" },
            {symbol<CallName>(), "CallName" },
            {symbol<CallProperty>(), "CallProperty" },
            {symbol<CallPropertyLookup>(), "CallPropertyLookup" },
            {symbol<CallElement>(), "CallElement" },
            {symbol<CallValue>(), "CallValue" },
            {symbol<CallWithReceiver>(), "CallWithReceiver" },
            {symbol<CallPossiblyDirectEval>(), "CallPossiblyDirectEval" },
            {symbol<CallWithSpread>(), "CallWithSpread" },
            {symbol<TailCall>(), "TailCall" },

            {symbol<Construct>(), "Construct" },
            {symbol<ConstructWithSpread>(), "ConstructWithSpread" },

            {symbol<StoreNameStrict>(), "StoreNameStrict" },
            {symbol<StoreNameSloppy>(), "StoreNameSloppy" },
            {symbol<StoreProperty>(), "StoreProperty" },
            {symbol<StoreElement>(), "StoreElement" },
            {symbol<LoadProperty>(), "LoadProperty" },
            {symbol<LoadName>(), "LoadName" },
            {symbol<LoadElement>(), "LoadElement" },
            {symbol<LoadSuperProperty>(), "LoadSuperProperty" },
            {symbol<StoreSuperProperty>(), "StoreSuperProperty" },
            {symbol<LoadSuperConstructor>(), "LoadSuperConstructor" },
            {symbol<LoadGlobalLookup>(), "LoadGlobalLookup" },
            {symbol<LoadQmlContextPropertyLookup>(), "LoadQmlContextPropertyLookup" },
            {symbol<GetLookup>(), "GetLookup" },
            {symbol<SetLookupStrict>(), "SetLookupStrict" },
            {symbol<SetLookupSloppy>(), "SetLookupSloppy" },

            {symbol<TypeofValue>(), "TypeofValue" },
            {symbol<TypeofName>(), "TypeofName" },

            {symbol<DeleteProperty_NoThrow>(), "DeleteProperty_NoThrow" },
            {symbol<DeleteProperty>(), "DeleteProperty" },
            {symbol<DeleteName_NoThrow>(), "DeleteName_NoThrow" },
            {symbol<DeleteName>(), "DeleteName" },

            {symbol<ThrowException>(), "ThrowException" },
            {symbol<PushCallContext>(), "PushCallContext" },
            {symbol<PushWithContext>(), "PushWithContext" },
            {symbol<PushCatchContext>(), "PushCatchContext" },
            {symbol<PushBlockContext>(), "PushBlockContext" },
            {symbol<CloneBlockContext>(), "CloneBlockContext" },
            {symbol<PushScriptContext>(), "PushScriptContext" },
            {symbol<PopScriptContext>(), "PopScriptContext" },
            {symbol<ThrowReferenceError>(), "ThrowReferenceError" },
            {symbol<ThrowOnNullOrUndefined>(), "ThrowOnNullOrUndefined" },

            {symbol<Closure>(), "Closure" },

            {symbol<ConvertThisToObject>(), "ConvertThisToObject" },
            {symbol<DeclareVar>(), "DeclareVar" },
            {symbol<CreateMappedArgumentsObject>(), "CreateMappedArgumentsObject" },
            {symbol<CreateUnmappedArgumentsObject>(), "CreateUnmappedArgumentsObject" },
            {symbol<CreateRestParameter>(), "CreateRestParameter" },

            {symbol<ArrayLiteral>(), "ArrayLiteral" },
            {symbol<ObjectLiteral>(), "ObjectLiteral" },
            {symbol<CreateClass>(), "CreateClass" },

            {symbol<GetIterator>(), "GetIterator" },
            {symbol<IteratorNext>(), "IteratorNext" },
            {symbol<IteratorNextForYieldStar>(), "IteratorNextForYieldStar" },
            {symbol<IteratorClose>(), "IteratorClose" },
            {symbol<DestructureRestElement>(), "DestructureRestElement" },

            {symbol<ToObject>(), "ToObject" },
            {symbol<ToBoolean>(), "ToBoolean" },
            {symbol<ToNumber>(), "ToNumber" },

            {symbol<UMinus>(), "UMinus" },

            {symbol<Instanceof>(), "Instanceof" },
            {symbol<In>(), "In" },
            {symbol<Add>(), "Add" },
            {symbol<Sub>(), "Sub" },
            {symbol<Mul>(), "Mul" },
            {symbol<Div>(), "Div" },
            {symbol<Mod>(), "Mod" },
            {symbol<Exp>(), "Exp" },
            {symbol<BitAnd>(), "BitAnd" },
            {symbol<BitOr>(), "BitOr" },
            {symbol<BitXor>(), "BitXor" },
            {symbol<Shl>(), "Shl" },
            {symbol<Shr>(), "Shr" },
            {symbol<UShr>(), "UShr" },
            {symbol<GreaterThan>(), "GreaterThan" },
            {symbol<LessThan>(), "LessThan" },
            {symbol<GreaterEqual>(), "GreaterEqual" },
            {symbol<LessEqual>(), "LessEqual" },
            {symbol<Equal>(), "Equal" },
            {symbol<NotEqual>(), "NotEqual" },
            {symbol<StrictEqual>(), "StrictEqual" },
            {symbol<StrictNotEqual>(), "StrictNotEqual" },

            {symbol<CompareGreaterThan>(), "CompareGreaterThan" },
            {symbol<CompareLessThan>(), "CompareLessThan" },
            {symbol<CompareGreaterEqual>(), "CompareGreaterEqual" },
            {symbol<CompareLessEqual>(), "CompareLessEqual" },
            {symbol<CompareEqual>(), "CompareEqual" },
            {symbol<CompareNotEqual>(), "CompareNotEqual" },
            {symbol<CompareStrictEqual>(), "CompareStrictEqual" },
            {symbol<CompareStrictNotEqual>(), "CompareStrictNotEqual" },

            {symbol<CompareInstanceof>(), "CompareInstanceOf" },
            {symbol<CompareIn>(), "CompareIn" },

            {symbol<RegexpLiteral>(), "RegexpLiteral" },
            {symbol<GetTemplateObject>(), "GetTemplateObject" }
#endif
    });

    return symbols;
}

} // namespace QV4

QT_END_NAMESPACE
