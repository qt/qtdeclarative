
#include "qv4ecmaobjects_p.h"
#include <qmath.h>
#include <qnumeric.h>
#include <cassert>

using namespace QQmlJS::VM;

static const double qt_PI = 2.0 * ::asin(1.0);

//
// Object
//
Value ObjectCtor::create(ExecutionEngine *engine)
{
    Context *ctx = engine->rootContext;
    FunctionObject *ctor = ctx->engine->newObjectCtor(ctx);
    ctor->setProperty(ctx, QLatin1String("prototype"), Value::object(ctx, ctx->engine->newObjectPrototype(ctx, ctor)));
    return Value::object(ctx, ctor);
}

ObjectCtor::ObjectCtor(Context *scope)
    : FunctionObject(scope)
{
}

void ObjectCtor::construct(Context *ctx)
{
    __qmljs_init_object(ctx, &ctx->thisObject, ctx->engine->newObject());
}

void ObjectCtor::call(Context *)
{
    assert(!"not here");
}

ObjectPrototype::ObjectPrototype(Context *ctx, FunctionObject *ctor)
{
    setProperty(ctx, QLatin1String("constructor"), Value::object(ctx, ctor));
}

//
// String
//
Value StringCtor::create(ExecutionEngine *engine)
{
    Context *ctx = engine->rootContext;
    FunctionObject *ctor = ctx->engine->newStringCtor(ctx);
    ctor->setProperty(ctx, QLatin1String("prototype"), Value::object(ctx, ctx->engine->newStringPrototype(ctx, ctor)));
    return Value::object(ctx, ctor);
}

StringCtor::StringCtor(Context *scope)
    : FunctionObject(scope)
{
}

void StringCtor::construct(Context *ctx)
{
    Value value;
    if (ctx->argumentCount)
        value = Value::string(ctx, ctx->argument(0).toString(ctx));
    else
        value = Value::string(ctx, QString());
    __qmljs_init_object(ctx, &ctx->thisObject, ctx->engine->newStringObject(value));
}

void StringCtor::call(Context *ctx)
{
    const Value arg = ctx->argument(0);
    if (arg.is(UNDEFINED_TYPE))
        __qmljs_init_string(ctx, &ctx->result, String::get(ctx, QString()));
    else
        __qmljs_to_string(ctx, &ctx->result, &arg);
}

StringPrototype::StringPrototype(Context *ctx, FunctionObject *ctor)
{
    setProperty(ctx, QLatin1String("constructor"), Value::object(ctx, ctor));
    setProperty(ctx, QLatin1String("toString"), method_toString);
    setProperty(ctx, QLatin1String("valueOf"), method_valueOf);
    setProperty(ctx, QLatin1String("charAt"), method_charAt);
    setProperty(ctx, QLatin1String("charCodeAt"), method_charCodeAt);
    setProperty(ctx, QLatin1String("concat"), method_concat);
    setProperty(ctx, QLatin1String("indexOf"), method_indexOf);
    setProperty(ctx, QLatin1String("lastIndexOf"), method_lastIndexOf);
    setProperty(ctx, QLatin1String("localeCompare"), method_localeCompare);
    setProperty(ctx, QLatin1String("match"), method_match);
    setProperty(ctx, QLatin1String("replace"), method_replace);
    setProperty(ctx, QLatin1String("search"), method_search);
    setProperty(ctx, QLatin1String("slice"), method_slice);
    setProperty(ctx, QLatin1String("split"), method_split);
    setProperty(ctx, QLatin1String("substr"), method_substr);
    setProperty(ctx, QLatin1String("substring"), method_substring);
    setProperty(ctx, QLatin1String("toLowerCase"), method_toLowerCase);
    setProperty(ctx, QLatin1String("toLocaleLowerCase"), method_toLocaleLowerCase);
    setProperty(ctx, QLatin1String("toUpperCase"), method_toUpperCase);
    setProperty(ctx, QLatin1String("toLocaleUpperCase"), method_toLocaleUpperCase);
    setProperty(ctx, QLatin1String("fromCharCode"), method_fromCharCode);
}

QString StringPrototype::getThisString(Context *ctx)
{
    Value v;
    __qmljs_to_string(ctx, &v, &ctx->thisObject);
    assert(v.is(STRING_TYPE));
    return v.stringValue->toQString();
}

void StringPrototype::method_toString(Context *ctx)
{
    __qmljs_to_string(ctx, &ctx->result, &ctx->thisObject);
}

void StringPrototype::method_valueOf(Context *ctx)
{
    ctx->thisObject.objectValue->defaultValue(ctx, &ctx->result, STRING_HINT);
}

void StringPrototype::method_charAt(Context *ctx)
{
    const QString str = getThisString(ctx);

    int pos = 0;
    if (ctx->argumentCount > 0)
        pos = (int) ctx->argument(0).toInteger(ctx);

    QString result;
    if (pos >= 0 && pos < str.length())
        result += str.at(pos);

    ctx->result = Value::string(ctx, result);
}

void StringPrototype::method_charCodeAt(Context *ctx)
{
    const QString str = getThisString(ctx);

    int pos = 0;
    if (ctx->argumentCount > 0)
        pos = (int) ctx->argument(0).toInteger(ctx);

    double result = qSNaN();

    if (pos >= 0 && pos < str.length())
        result = str.at(pos).unicode();

    __qmljs_init_number(ctx, &ctx->result, result);
}

void StringPrototype::method_concat(Context *ctx)
{
    QString value = getThisString(ctx);

    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        Value v;
        __qmljs_to_string(ctx, &v, &ctx->arguments[i]);
        assert(v.is(STRING_TYPE));
        value += v.stringValue->toQString();
    }

    ctx->result = Value::string(ctx, value);
}

void StringPrototype::method_indexOf(Context *ctx)
{
    QString value = getThisString(ctx);

    QString searchString;
    if (ctx->argumentCount)
        searchString = ctx->argument(0).toString(ctx)->toQString();

    int pos = 0;
    if (ctx->argumentCount > 1)
        pos = (int) ctx->argument(1).toInteger(ctx);

    int index = -1;
    if (! value.isEmpty())
        index = value.indexOf(searchString, qMin(qMax(pos, 0), value.length()));

    __qmljs_init_number(ctx, &ctx->result, index);
}

void StringPrototype::method_lastIndexOf(Context *ctx)
{
    const QString value = getThisString(ctx);

    QString searchString;
    if (ctx->argumentCount) {
        Value v;
        __qmljs_to_string(ctx, &v, &ctx->arguments[0]);
        searchString = v.stringValue->toQString();
    }

    Value posArg = ctx->argument(1);
    double position = __qmljs_to_number(ctx, &posArg);
    if (qIsNaN(position))
        position = +qInf();
    else
        position = trunc(position);

    int pos = trunc(qMin(qMax(position, 0.0), double(value.length())));
    if (!searchString.isEmpty() && pos == value.length())
        --pos;
    int index = value.lastIndexOf(searchString, pos);
    __qmljs_init_number(ctx, &ctx->result, index);
}

void StringPrototype::method_localeCompare(Context *ctx)
{
    const QString value = getThisString(ctx);
    const QString that = ctx->argument(0).toString(ctx)->toQString();
    __qmljs_init_number(ctx, &ctx->result, QString::localeAwareCompare(value, that));
}

void StringPrototype::method_match(Context *)
{
    // requires Regexp
    Q_UNIMPLEMENTED();
}

void StringPrototype::method_replace(Context *)
{
    // requires Regexp
    Q_UNIMPLEMENTED();
}

void StringPrototype::method_search(Context *)
{
    // requires Regexp
    Q_UNIMPLEMENTED();
}

void StringPrototype::method_slice(Context *ctx)
{
    const QString text = getThisString(ctx);
    const int length = text.length();

    int start = int (ctx->argument(0).toInteger(ctx));
    int end = ctx->argument(1).isUndefined()
            ? length : int (ctx->argument(1).toInteger(ctx));

    if (start < 0)
        start = qMax(length + start, 0);
    else
        start = qMin(start, length);

    if (end < 0)
        end = qMax(length + end, 0);
    else
        end = qMin(end, length);

    int count = qMax(0, end - start);
    ctx->result = Value::string(ctx, text.mid(start, count));
}

void StringPrototype::method_split(Context *)
{
    // requires Array
    Q_UNIMPLEMENTED();
}

void StringPrototype::method_substr(Context *ctx)
{
    const QString value = getThisString(ctx);

    double start = 0;
    if (ctx->argumentCount > 0)
        start = ctx->argument(0).toInteger(ctx);

    double length = +qInf();
    if (ctx->argumentCount > 1)
        length = ctx->argument(1).toInteger(ctx);

    double count = value.length();
    if (start < 0)
        start = qMax(count + start, 0.0);

    length = qMin(qMax(length, 0.0), count - start);

    qint32 x = Value::toInt32(start);
    qint32 y = Value::toInt32(length);
    ctx->result = Value::string(ctx, value.mid(x, y));
}

void StringPrototype::method_substring(Context *ctx)
{
    QString value = getThisString(ctx);
    int length = value.length();

    double start = 0;
    double end = length;

    if (ctx->argumentCount > 0)
        start = ctx->argument(0).toInteger(ctx);

    if (ctx->argumentCount > 1)
        end = ctx->argument(1).toInteger(ctx);

    if (qIsNaN(start) || start < 0)
        start = 0;

    if (qIsNaN(end) || end < 0)
        end = 0;

    if (start > length)
        start = length;

    if (end > length)
        end = length;

    if (start > end) {
        double was = start;
        start = end;
        end = was;
    }

    qint32 x = Value::toInt32(start);
    qint32 y = Value::toInt32(end - start);
    ctx->result = Value::string(ctx, value.mid(x, y));
}

void StringPrototype::method_toLowerCase(Context *ctx)
{
    QString value = getThisString(ctx);
    ctx->result = Value::string(ctx, value.toLower());
}

void StringPrototype::method_toLocaleLowerCase(Context *ctx)
{
    method_toLowerCase(ctx);
}

void StringPrototype::method_toUpperCase(Context *ctx)
{
    QString value = getThisString(ctx);
    ctx->result = Value::string(ctx, value.toUpper());
}

void StringPrototype::method_toLocaleUpperCase(Context *ctx)
{
    method_toUpperCase(ctx);
}

void StringPrototype::method_fromCharCode(Context *ctx)
{
    QString str;
    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        QChar c(ctx->argument(i).toUInt16(ctx));
        str += c;
    }
    ctx->result = Value::string(ctx, str);
}

//
// Number object
//
Value NumberCtor::create(ExecutionEngine *engine)
{
    Context *ctx = engine->rootContext;
    FunctionObject *ctor = ctx->engine->newNumberCtor(ctx);
    ctor->setProperty(ctx, QLatin1String("prototype"), Value::object(ctx, ctx->engine->newNumberPrototype(ctx, ctor)));
    return Value::object(ctx, ctor);
}

NumberCtor::NumberCtor(Context *scope)
    : FunctionObject(scope)
{
}

void NumberCtor::construct(Context *ctx)
{
    const double n = ctx->argument(0).toNumber(ctx);
    __qmljs_init_object(ctx, &ctx->thisObject, ctx->engine->newNumberObject(Value::number(ctx, n)));
}

void NumberCtor::call(Context *ctx)
{
    double value = ctx->argumentCount ? ctx->argument(0).toNumber(ctx) : 0;
    __qmljs_init_number(ctx, &ctx->result, value);
}

NumberPrototype::NumberPrototype(Context *ctx, FunctionObject *ctor)
{
    ctor->setProperty(ctx, QLatin1String("NaN"), Value::number(ctx, qSNaN()));
    ctor->setProperty(ctx, QLatin1String("NEGATIVE_INFINITY"), Value::number(ctx, -qInf()));
    ctor->setProperty(ctx, QLatin1String("POSITIVE_INFINITY"), Value::number(ctx, qInf()));
    ctor->setProperty(ctx, QLatin1String("MAX_VALUE"), Value::number(ctx, 1.7976931348623158e+308));
#ifdef __INTEL_COMPILER
# pragma warning( push )
# pragma warning(disable: 239)
#endif
    ctor->setProperty(ctx, QLatin1String("MIN_VALUE"), Value::number(ctx, 5e-324));
#ifdef __INTEL_COMPILER
# pragma warning( pop )
#endif

    setProperty(ctx, QLatin1String("constructor"), Value::object(ctx, ctor));
    setProperty(ctx, QLatin1String("toString"), method_toString);
    setProperty(ctx, QLatin1String("toLocalString"), method_toLocaleString);
    setProperty(ctx, QLatin1String("valueOf"), method_valueOf);
    setProperty(ctx, QLatin1String("toFixed"), method_toFixed);
    setProperty(ctx, QLatin1String("toExponential"), method_toExponential);
    setProperty(ctx, QLatin1String("toPrecision"), method_toPrecision);
}

void NumberPrototype::method_toString(Context *ctx)
{
    Value self = ctx->thisObject;
    assert(self.isObject());
    //    if (self.classInfo() != classInfo) {
    //        return throwThisObjectTypeError(
    //            ctx, QLatin1String("Number.prototype.toString"));
    //    }

    Value arg = ctx->argument(0);
    if (!arg.isUndefined()) {
        int radix = arg.toInt32(ctx);
        //        if (radix < 2 || radix > 36)
        //            return ctx->throwError(QString::fromLatin1("Number.prototype.toString: %0 is not a valid radix")
        //                                       .arg(radix));
        if (radix != 10) {
            Value internalValue;
            self.objectValue->defaultValue(ctx, &internalValue, NUMBER_HINT);

            QString str;
            double num = internalValue.toNumber(ctx);
            if (qIsNaN(num)) {
                ctx->result = Value::string(ctx, QLatin1String("NaN"));
                return;
            } else if (qIsInf(num)) {
                ctx->result = Value::string(ctx, QLatin1String(num < 0 ? "-Infinity" : "Infinity"));
                return;
            }
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
                str.prepend(QLatin1Char(c));
                num = ::floor(num / radix);
            } while (num != 0);
            if (frac != 0) {
                str.append(QLatin1Char('.'));
                do {
                    frac = frac * radix;
                    char c = (char)::floor(frac);
                    c = (c < 10) ? (c + '0') : (c - 10 + 'a');
                    str.append(QLatin1Char(c));
                    frac = frac - ::floor(frac);
                } while (frac != 0);
            }
            if (negative)
                str.prepend(QLatin1Char('-'));
            ctx->result = Value::string(ctx, str);
            return;
        }
    }

    Value internalValue;
    self.objectValue->defaultValue(ctx, &internalValue, NUMBER_HINT);

    String *str = internalValue.toString(ctx);
    ctx->result = Value::string(ctx, str);
}

void NumberPrototype::method_toLocaleString(Context *ctx)
{
    Value self = ctx->thisObject;
    assert(self.isObject());
    //    if (self.classInfo() != classInfo) {
    //        return throwThisObjectTypeError(
    //            context, QLatin1String("Number.prototype.toLocaleString"));
    //    }
    Value internalValue;
    self.objectValue->defaultValue(ctx, &internalValue, STRING_HINT);
    String *str = internalValue.toString(ctx);
    ctx->result = Value::string(ctx, str);
}

void NumberPrototype::method_valueOf(Context *ctx)
{
    Value self = ctx->thisObject;
    assert(self.isObject());
    //    if (self.classInfo() != classInfo) {
    //        return throwThisObjectTypeError(
    //            context, QLatin1String("Number.prototype.toLocaleString"));
    //    }
    Value internalValue;
    self.objectValue->defaultValue(ctx, &internalValue, NUMBER_HINT);
    ctx->result = internalValue;
}

void NumberPrototype::method_toFixed(Context *ctx)
{
    Value self = ctx->thisObject;
    assert(self.isObject());
    //    if (self.classInfo() != classInfo) {
    //        return throwThisObjectTypeError(
    //            ctx, QLatin1String("Number.prototype.toFixed"));
    //    }
    double fdigits = 0;

    if (ctx->argumentCount > 0)
        fdigits = ctx->argument(0).toInteger(ctx);

    if (qIsNaN(fdigits))
        fdigits = 0;

    Value internalValue;
    self.objectValue->defaultValue(ctx, &internalValue, NUMBER_HINT);

    double v = internalValue.toNumber(ctx);
    QString str;
    if (qIsNaN(v))
        str = QString::fromLatin1("NaN");
    else if (qIsInf(v))
        str = QString::fromLatin1(v < 0 ? "-Infinity" : "Infinity");
    else
        str = QString::number(v, 'f', int (fdigits));
    ctx->result = Value::string(ctx, str);
}

void NumberPrototype::method_toExponential(Context *ctx)
{
    Value self = ctx->thisObject;
    assert(self.isObject());
    //    if (self.classInfo() != classInfo) {
    //        return throwThisObjectTypeError(
    //            ctx, QLatin1String("Number.prototype.toFixed"));
    //    }
    double fdigits = 0;

    if (ctx->argumentCount > 0)
        fdigits = ctx->argument(0).toInteger(ctx);

    Value internalValue;
    self.objectValue->defaultValue(ctx, &internalValue, NUMBER_HINT);

    double v = internalValue.toNumber(ctx);
    QString z = QString::number(v, 'e', int (fdigits));
    ctx->result = Value::string(ctx, z);
}

void NumberPrototype::method_toPrecision(Context *ctx)
{
    Value self = ctx->thisObject;
    assert(self.isObject());
    //    if (self.classInfo() != classInfo) {
    //        return throwThisObjectTypeError(
    //            ctx, QLatin1String("Number.prototype.toFixed"));
    //    }
    double fdigits = 0;

    if (ctx->argumentCount > 0)
        fdigits = ctx->argument(0).toInteger(ctx);

    Value internalValue;
    self.objectValue->defaultValue(ctx, &internalValue, NUMBER_HINT);

    double v = internalValue.toNumber(ctx);
    ctx->result = Value::string(ctx, QString::number(v, 'g', int (fdigits)));
}

//
// Math object
//
MathObject::MathObject(Context *ctx)
{
    setProperty(ctx, QLatin1String("E"), Value::number(ctx, ::exp(1.0)));
    setProperty(ctx, QLatin1String("LN2"), Value::number(ctx, ::log(2.0)));
    setProperty(ctx, QLatin1String("LN10"), Value::number(ctx, ::log(10.0)));
    setProperty(ctx, QLatin1String("LOG2E"), Value::number(ctx, 1.0/::log(2.0)));
    setProperty(ctx, QLatin1String("LOG10E"), Value::number(ctx, 1.0/::log(10.0)));
    setProperty(ctx, QLatin1String("PI"), Value::number(ctx, qt_PI));
    setProperty(ctx, QLatin1String("SQRT1_2"), Value::number(ctx, ::sqrt(0.5)));
    setProperty(ctx, QLatin1String("SQRT2"), Value::number(ctx, ::sqrt(2.0)));

    setProperty(ctx, QLatin1String("abs"), method_abs, 1);
    setProperty(ctx, QLatin1String("acos"), method_acos, 1);
    setProperty(ctx, QLatin1String("asin"), method_asin, 0);
    setProperty(ctx, QLatin1String("atan"), method_atan, 1);
    setProperty(ctx, QLatin1String("atan2"), method_atan2, 2);
    setProperty(ctx, QLatin1String("ceil"), method_ceil, 1);
    setProperty(ctx, QLatin1String("cos"), method_cos, 1);
    setProperty(ctx, QLatin1String("exp"), method_exp, 1);
    setProperty(ctx, QLatin1String("floor"), method_floor, 1);
    setProperty(ctx, QLatin1String("log"), method_log, 1);
    setProperty(ctx, QLatin1String("max"), method_max, 2);
    setProperty(ctx, QLatin1String("min"), method_min, 2);
    setProperty(ctx, QLatin1String("pow"), method_pow, 2);
    setProperty(ctx, QLatin1String("random"), method_random, 0);
    setProperty(ctx, QLatin1String("round"), method_round, 1);
    setProperty(ctx, QLatin1String("sin"), method_sin, 1);
    setProperty(ctx, QLatin1String("sqrt"), method_sqrt, 1);
    setProperty(ctx, QLatin1String("tan"), method_tan, 1);
}

/* copies the sign from y to x and returns the result */
static double copySign(double x, double y)
{
    uchar *xch = (uchar *)&x;
    uchar *ych = (uchar *)&y;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
        xch[0] = (xch[0] & 0x7f) | (ych[0] & 0x80);
    else
        xch[7] = (xch[7] & 0x7f) | (ych[7] & 0x80);
    return x;
}

void MathObject::method_abs(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v == 0) // 0 | -0
        ctx->result = Value::number(ctx, 0);
    else
        ctx->result = Value::number(ctx, v < 0 ? -v : v);
}

void MathObject::method_acos(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v > 1)
        ctx->result = Value::number(ctx, qSNaN());
    else
        ctx->result = Value::number(ctx, ::acos(v));
}

void MathObject::method_asin(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v > 1)
        ctx->result = Value::number(ctx, qSNaN());
    else
        ctx->result = Value::number(ctx, ::asin(v));
}

void MathObject::method_atan(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v == 0.0)
        ctx->result = Value::number(ctx, v);
    else
        ctx->result = Value::number(ctx, ::atan(v));
}

void MathObject::method_atan2(Context *ctx)
{
    double v1 = ctx->argument(0).toNumber(ctx);
    double v2 = ctx->argument(1).toNumber(ctx);
    if ((v1 < 0) && qIsFinite(v1) && qIsInf(v2) && (copySign(1.0, v2) == 1.0)) {
        ctx->result = Value::number(ctx, copySign(0, -1.0));
        return;
    }
    if ((v1 == 0.0) && (v2 == 0.0)) {
        if ((copySign(1.0, v1) == 1.0) && (copySign(1.0, v2) == -1.0)) {
            ctx->result = Value::number(ctx, qt_PI);
            return;
        } else if ((copySign(1.0, v1) == -1.0) && (copySign(1.0, v2) == -1.0)) {
            ctx->result = Value::number(ctx, -qt_PI);
            return;
        }
    }
    ctx->result = Value::number(ctx, ::atan2(v1, v2));
}

void MathObject::method_ceil(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v < 0.0 && v > -1.0)
        ctx->result = Value::number(ctx, copySign(0, -1.0));
    else
        ctx->result = Value::number(ctx, ::ceil(v));
}

void MathObject::method_cos(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    ctx->result = Value::number(ctx, ::cos(v));
}

void MathObject::method_exp(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (qIsInf(v)) {
        if (copySign(1.0, v) == -1.0)
            ctx->result = Value::number(ctx, 0);
        else
            ctx->result = Value::number(ctx, qInf());
    } else {
        ctx->result = Value::number(ctx, ::exp(v));
    }
}

void MathObject::method_floor(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    ctx->result = Value::number(ctx, ::floor(v));
}

void MathObject::method_log(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v < 0)
        ctx->result = Value::number(ctx, qSNaN());
    else
        ctx->result = Value::number(ctx, ::log(v));
}

void MathObject::method_max(Context *ctx)
{
    double mx = -qInf();
    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        double x = ctx->argument(i).toNumber(ctx);
        if (x > mx || qIsNaN(x))
            mx = x;
    }
    ctx->result = Value::number(ctx, mx);
}

void MathObject::method_min(Context *ctx)
{
    double mx = qInf();
    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        double x = ctx->argument(i).toNumber(ctx);
        if ((x == 0 && mx == x && copySign(1.0, x) == -1.0)
                || (x < mx) || qIsNaN(x)) {
            mx = x;
        }
    }
    ctx->result = Value::number(ctx, mx);
}

void MathObject::method_pow(Context *ctx)
{
    double x = ctx->argument(0).toNumber(ctx);
    double y = ctx->argument(1).toNumber(ctx);

    if (qIsNaN(y)) {
        ctx->result = Value::number(ctx, qSNaN());
        return;
    }

    if (y == 0) {
        ctx->result = Value::number(ctx, 1);
    } else if (((x == 1) || (x == -1)) && qIsInf(y)) {
        ctx->result = Value::number(ctx, qSNaN());
    } else if (((x == 0) && copySign(1.0, x) == 1.0) && (y < 0)) {
        ctx->result = Value::number(ctx, qInf());
    } else if ((x == 0) && copySign(1.0, x) == -1.0) {
        if (y < 0) {
            if (::fmod(-y, 2.0) == 1.0)
                ctx->result = Value::number(ctx, -qInf());
            else
                ctx->result = Value::number(ctx, qInf());
        } else if (y > 0) {
            if (::fmod(y, 2.0) == 1.0)
                ctx->result = Value::number(ctx, copySign(0, -1.0));
            else
                ctx->result = Value::number(ctx, 0);
        }
    }

#ifdef Q_OS_AIX
    else if (qIsInf(x) && copySign(1.0, x) == -1.0) {
        if (y > 0) {
            if (::fmod(y, 2.0) == 1.0)
                ctx->result = Value::number(ctx, -qInf());
            else
                ctx->result = Value::number(ctx, qInf());
        } else if (y < 0) {
            if (::fmod(-y, 2.0) == 1.0)
                ctx->result = Value::number(ctx, copySign(0, -1.0));
            else
                ctx->result = Value::number(ctx, 0);
        }
    }
#endif
    else {
        ctx->result = Value::number(ctx, ::pow(x, y));
    }
}

void MathObject::method_random(Context *ctx)
{
    ctx->result = Value::number(ctx, qrand() / (double) RAND_MAX);
}

void MathObject::method_round(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    v = copySign(::floor(v + 0.5), v);
    ctx->result = Value::number(ctx, v);
}

void MathObject::method_sin(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    ctx->result = Value::number(ctx, ::sin(v));
}

void MathObject::method_sqrt(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    ctx->result = Value::number(ctx, ::sqrt(v));
}

void MathObject::method_tan(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v == 0.0)
        ctx->result = Value::number(ctx, v);
    else
        ctx->result = Value::number(ctx, ::tan(v));
}
