// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4mathobject_p.h"
#include "qv4symbol_p.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qmath.h>
#include <QtCore/qrandom.h>
#include <QtCore/private/qnumeric_p.h>
#include <QtCore/qthreadstorage.h>

#include <cmath>

using namespace QV4;

DEFINE_OBJECT_VTABLE(MathObject);

void Heap::MathObject::init()
{
    Object::init();
    Scope scope(internalClass->engine);
    ScopedObject m(scope, this);

    m->defineReadonlyProperty(QStringLiteral("E"), Value::fromDouble(M_E));
    m->defineReadonlyProperty(QStringLiteral("LN2"), Value::fromDouble(M_LN2));
    m->defineReadonlyProperty(QStringLiteral("LN10"), Value::fromDouble(M_LN10));
    m->defineReadonlyProperty(QStringLiteral("LOG2E"), Value::fromDouble(M_LOG2E));
    m->defineReadonlyProperty(QStringLiteral("LOG10E"), Value::fromDouble(M_LOG10E));
    m->defineReadonlyProperty(QStringLiteral("PI"), Value::fromDouble(M_PI));
    m->defineReadonlyProperty(QStringLiteral("SQRT1_2"), Value::fromDouble(M_SQRT1_2));
    m->defineReadonlyProperty(QStringLiteral("SQRT2"), Value::fromDouble(M_SQRT2));

    m->defineDefaultProperty(QStringLiteral("abs"), QV4::MathObject::method_abs, 1);
    m->defineDefaultProperty(QStringLiteral("acos"), QV4::MathObject::method_acos, 1);
    m->defineDefaultProperty(QStringLiteral("acosh"), QV4::MathObject::method_acosh, 1);
    m->defineDefaultProperty(QStringLiteral("asin"), QV4::MathObject::method_asin, 1);
    m->defineDefaultProperty(QStringLiteral("asinh"), QV4::MathObject::method_asinh, 1);
    m->defineDefaultProperty(QStringLiteral("atan"), QV4::MathObject::method_atan, 1);
    m->defineDefaultProperty(QStringLiteral("atanh"), QV4::MathObject::method_atanh, 1);
    m->defineDefaultProperty(QStringLiteral("atan2"), QV4::MathObject::method_atan2, 2);
    m->defineDefaultProperty(QStringLiteral("cbrt"), QV4::MathObject::method_cbrt, 1);
    m->defineDefaultProperty(QStringLiteral("ceil"), QV4::MathObject::method_ceil, 1);
    m->defineDefaultProperty(QStringLiteral("clz32"), QV4::MathObject::method_clz32, 1);
    m->defineDefaultProperty(QStringLiteral("cos"), QV4::MathObject::method_cos, 1);
    m->defineDefaultProperty(QStringLiteral("cosh"), QV4::MathObject::method_cosh, 1);
    m->defineDefaultProperty(QStringLiteral("exp"), QV4::MathObject::method_exp, 1);
    m->defineDefaultProperty(QStringLiteral("expm1"), QV4::MathObject::method_expm1, 1);
    m->defineDefaultProperty(QStringLiteral("floor"), QV4::MathObject::method_floor, 1);
    m->defineDefaultProperty(QStringLiteral("fround"), QV4::MathObject::method_fround, 1);
    m->defineDefaultProperty(QStringLiteral("hypot"), QV4::MathObject::method_hypot, 2);
    m->defineDefaultProperty(QStringLiteral("imul"), QV4::MathObject::method_imul, 2);
    m->defineDefaultProperty(QStringLiteral("log"), QV4::MathObject::method_log, 1);
    m->defineDefaultProperty(QStringLiteral("log10"), QV4::MathObject::method_log10, 1);
    m->defineDefaultProperty(QStringLiteral("log1p"), QV4::MathObject::method_log1p, 1);
    m->defineDefaultProperty(QStringLiteral("log2"), QV4::MathObject::method_log2, 1);
    m->defineDefaultProperty(QStringLiteral("max"), QV4::MathObject::method_max, 2);
    m->defineDefaultProperty(QStringLiteral("min"), QV4::MathObject::method_min, 2);
    m->defineDefaultProperty(QStringLiteral("pow"), QV4::MathObject::method_pow, 2);
    m->defineDefaultProperty(QStringLiteral("random"), QV4::MathObject::method_random, 0);
    m->defineDefaultProperty(QStringLiteral("round"), QV4::MathObject::method_round, 1);
    m->defineDefaultProperty(QStringLiteral("sign"), QV4::MathObject::method_sign, 1);
    m->defineDefaultProperty(QStringLiteral("sin"), QV4::MathObject::method_sin, 1);
    m->defineDefaultProperty(QStringLiteral("sinh"), QV4::MathObject::method_sinh, 1);
    m->defineDefaultProperty(QStringLiteral("sqrt"), QV4::MathObject::method_sqrt, 1);
    m->defineDefaultProperty(QStringLiteral("tan"), QV4::MathObject::method_tan, 1);
    m->defineDefaultProperty(QStringLiteral("tanh"), QV4::MathObject::method_tanh, 1);
    m->defineDefaultProperty(QStringLiteral("trunc"), QV4::MathObject::method_trunc, 1);

    ScopedString name(scope, scope.engine->newString(QStringLiteral("Math")));
    m->defineReadonlyConfigurableProperty(scope.engine->symbol_toStringTag(), name);
}

static Q_ALWAYS_INLINE double copySign(double x, double y)
{
    return ::copysign(x, y);
}

ReturnedValue MathObject::method_abs(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    if (!argc)
        RETURN_RESULT(Encode(qt_qnan()));

    if (argv[0].isInteger()) {
        int i = argv[0].integerValue();
        RETURN_RESULT(Encode(i < 0 ? - i : i));
    }

    double v = argv[0].toNumber();
    if (v == 0) // 0 | -0
        RETURN_RESULT(Encode(0));

    RETURN_RESULT(Encode(v < 0 ? -v : v));
}

ReturnedValue MathObject::method_acos(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : 2;
    if (v > 1)
        RETURN_RESULT(Encode(qt_qnan()));

    RETURN_RESULT(Encode(std::acos(v)));
}

ReturnedValue MathObject::method_acosh(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : 2;
    if (v < 1)
        RETURN_RESULT(Encode(qt_qnan()));

#ifdef Q_CC_MINGW
    // Mingw has a broken std::acosh(). It returns NaN when passed Infinity.
    if (std::isinf(v))
        RETURN_RESULT(Encode(v));
#endif
    RETURN_RESULT(Encode(std::acosh(v)));
}

ReturnedValue MathObject::method_asin(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : 2;
    if (v > 1)
        RETURN_RESULT(Encode(qt_qnan()));
    else
        RETURN_RESULT(Encode(std::asin(v)));
}

ReturnedValue MathObject::method_asinh(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : 2;
    if (v == 0.0)
        RETURN_RESULT(Encode(v));
    RETURN_RESULT(Encode(std::asinh(v)));
}

ReturnedValue MathObject::method_atan(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (v == 0.0)
        RETURN_RESULT(Encode(v));
    else
        RETURN_RESULT(Encode(std::atan(v)));
}

ReturnedValue MathObject::method_atanh(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (v == 0.0)
        RETURN_RESULT(Encode(v));

    RETURN_RESULT(Encode(std::atanh(v)));
}

ReturnedValue MathObject::method_atan2(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v1 = argc ? argv[0].toNumber() : qt_qnan();
    double v2 = argc > 1 ? argv[1].toNumber() : qt_qnan();

    if ((v1 < 0) && qt_is_finite(v1) && qt_is_inf(v2) && (copySign(1.0, v2) == 1.0))
        RETURN_RESULT(Encode(copySign(0, -1.0)));

    if ((v1 == 0.0) && (v2 == 0.0)) {
        if ((copySign(1.0, v1) == 1.0) && (copySign(1.0, v2) == -1.0)) {
            RETURN_RESULT(Encode(M_PI));
        } else if ((copySign(1.0, v1) == -1.0) && (copySign(1.0, v2) == -1.0)) {
            RETURN_RESULT(Encode(-M_PI));
        }
    }
    RETURN_RESULT(Encode(std::atan2(v1, v2)));
}

ReturnedValue MathObject::method_cbrt(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    RETURN_RESULT(Encode(std::cbrt(v))); // cube root
}

ReturnedValue MathObject::method_ceil(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (v < 0.0 && v > -1.0)
        RETURN_RESULT(Encode(copySign(0, -1.0)));
    else
        RETURN_RESULT(Encode(std::ceil(v)));
}

ReturnedValue MathObject::method_clz32(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    quint32 v = argc ? argv[0].toUInt32() : 0;
    RETURN_RESULT(Encode(qint32(qCountLeadingZeroBits(v))));
}

ReturnedValue MathObject::method_cos(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    RETURN_RESULT(Encode(std::cos(v)));
}

ReturnedValue MathObject::method_cosh(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    RETURN_RESULT(Encode(std::cosh(v)));
}

ReturnedValue MathObject::method_exp(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (qt_is_inf(v)) {
        if (copySign(1.0, v) == -1.0)
            RETURN_RESULT(Encode(0));
        else
            RETURN_RESULT(Encode(qt_inf()));
    } else {
        RETURN_RESULT(Encode(std::exp(v)));
    }
}

ReturnedValue MathObject::method_expm1(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (std::isnan(v) || qIsNull(v)) {
        RETURN_RESULT(Encode(v));
    } else if (qt_is_inf(v)) {
        if (copySign(1.0, v) == -1.0)
            RETURN_RESULT(Encode(-1.0));
        else
            RETURN_RESULT(Encode(qt_inf()));
    } else {
        RETURN_RESULT(Encode(std::expm1(v)));
    }
}

ReturnedValue MathObject::method_floor(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    Value result = Value::fromDouble(std::floor(v));
    result.isInt32();
    RETURN_RESULT(result);
}

ReturnedValue MathObject::method_fround(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (std::isnan(v) || qt_is_inf(v) || qIsNull(v))
        RETURN_RESULT(Encode(v));
    else // convert to 32-bit float using roundTiesToEven, then convert back to 64-bit double
        RETURN_RESULT(Encode(double(float(v))));
}

ReturnedValue MathObject::method_hypot(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    // ES6 Math.hypot(v1, ..., vn) -> sqrt(sum(vi**2)) but "should take care to
    // avoid the loss of precision from overflows and underflows" (as std::hypot does).
    double v = 0;
    // Spec mandates +0 on no args; and says nothing about what to do if toNumber() signals ...
    if (argc > 0) {
        QtPrivate::QHypotHelper<double> h(argv[0].toNumber());
        for (int i = 1; i < argc; i++)
            h = h.add(argv[i].toNumber());
        v = h.result();
    }
    RETURN_RESULT(Value::fromDouble(v));
}

ReturnedValue MathObject::method_imul(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    quint32 a = argc ? argv[0].toUInt32() : 0;
    quint32 b = argc > 0 ? argv[1].toUInt32() : 0;
    qint32 product = a * b;
    RETURN_RESULT(Encode(product));
}

ReturnedValue MathObject::method_log(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (v < 0)
        RETURN_RESULT(Encode(qt_qnan()));
    else
        RETURN_RESULT(Encode(std::log(v)));
}

ReturnedValue MathObject::method_log10(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (v < 0)
        RETURN_RESULT(Encode(qt_qnan()));
    else
        RETURN_RESULT(Encode(std::log10(v)));
}

ReturnedValue MathObject::method_log1p(const FunctionObject *, const Value *, const Value *argv, int argc)
{
#if !defined(__ANDROID__)
    using std::log1p;
#endif
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (v < -1)
        RETURN_RESULT(Encode(qt_qnan()));
    else
        RETURN_RESULT(Encode(log1p(v)));
}

ReturnedValue MathObject::method_log2(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (v < 0) {
        RETURN_RESULT(Encode(qt_qnan()));
    } else {
        RETURN_RESULT(Encode(std::log2(v)));
    }
}

ReturnedValue MathObject::method_max(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double mx = -qt_inf();
    for (int i = 0, ei = argc; i < ei; ++i) {
        double x = argv[i].toNumber();
        if ((x == 0 && mx == x && copySign(1.0, x) == 1.0)
                || (x > mx) || std::isnan(x)) {
            mx = x;
        }
    }
    RETURN_RESULT(Encode::smallestNumber(mx));
}

ReturnedValue MathObject::method_min(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double mx = qt_inf();
    for (int i = 0, ei = argc; i < ei; ++i) {
        double x = argv[i].toNumber();
        if ((x == 0 && mx == x && copySign(1.0, x) == -1.0)
                || (x < mx) || std::isnan(x)) {
            mx = x;
        }
    }
    RETURN_RESULT(Encode::smallestNumber(mx));
}

ReturnedValue MathObject::method_pow(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double x = argc > 0 ? argv[0].toNumber() : qt_qnan();
    double y = argc > 1 ? argv[1].toNumber() : qt_qnan();

    RETURN_RESULT(Encode(QQmlPrivate::jsExponentiate(x, y)));
}

ReturnedValue MathObject::method_random(const FunctionObject *, const Value *, const Value *, int)
{
    RETURN_RESULT(Encode(QRandomGenerator::global()->generateDouble()));
}

ReturnedValue MathObject::method_round(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (!std::isfinite(v))
        RETURN_RESULT(Encode(v));

    if (v < 0.5 && v >= -0.5)
        v = std::copysign(0.0, v);
    else
        v = std::floor(v + 0.5);
    RETURN_RESULT(Encode(v));
}

ReturnedValue MathObject::method_sign(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();

    if (std::isnan(v))
        RETURN_RESULT(Encode(qt_qnan()));

    if (qIsNull(v))
        RETURN_RESULT(Encode(v));

    RETURN_RESULT(Encode(std::signbit(v) ? -1 : 1));
}

ReturnedValue MathObject::method_sin(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (v == 0.0)
        RETURN_RESULT(Encode(v));
    else
        RETURN_RESULT(Encode(std::sin(v)));
}

ReturnedValue MathObject::method_sinh(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (v == 0.0)
        RETURN_RESULT(Encode(v));
    else
        RETURN_RESULT(Encode(std::sinh(v)));
}

ReturnedValue MathObject::method_sqrt(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    RETURN_RESULT(Encode(std::sqrt(v)));
}

ReturnedValue MathObject::method_tan(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (v == 0.0)
        RETURN_RESULT(Encode(v));
    else
        RETURN_RESULT(Encode(std::tan(v)));
}

ReturnedValue MathObject::method_tanh(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    if (v == 0.0)
        RETURN_RESULT(Encode(v));
    else
        RETURN_RESULT(Encode(std::tanh(v)));
}

ReturnedValue MathObject::method_trunc(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    double v = argc ? argv[0].toNumber() : qt_qnan();
    RETURN_RESULT(Encode(std::trunc(v)));
}
