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

#include "qv4mathobject.h"

#include <cmath>
#include <qmath.h>
#include <qnumeric.h>

using namespace QQmlJS::VM;

static const double qt_PI = 2.0 * ::asin(1.0);

MathObject::MathObject(ExecutionContext *ctx)
{
    defineReadonlyProperty(ctx->engine, QStringLiteral("E"), Value::fromDouble(::exp(1.0)));
    defineReadonlyProperty(ctx->engine, QStringLiteral("LN2"), Value::fromDouble(::log(2.0)));
    defineReadonlyProperty(ctx->engine, QStringLiteral("LN10"), Value::fromDouble(::log(10.0)));
    defineReadonlyProperty(ctx->engine, QStringLiteral("LOG2E"), Value::fromDouble(1.0/::log(2.0)));
    defineReadonlyProperty(ctx->engine, QStringLiteral("LOG10E"), Value::fromDouble(1.0/::log(10.0)));
    defineReadonlyProperty(ctx->engine, QStringLiteral("PI"), Value::fromDouble(qt_PI));
    defineReadonlyProperty(ctx->engine, QStringLiteral("SQRT1_2"), Value::fromDouble(::sqrt(0.5)));
    defineReadonlyProperty(ctx->engine, QStringLiteral("SQRT2"), Value::fromDouble(::sqrt(2.0)));

    defineDefaultProperty(ctx, QStringLiteral("abs"), method_abs, 1);
    defineDefaultProperty(ctx, QStringLiteral("acos"), method_acos, 1);
    defineDefaultProperty(ctx, QStringLiteral("asin"), method_asin, 0);
    defineDefaultProperty(ctx, QStringLiteral("atan"), method_atan, 1);
    defineDefaultProperty(ctx, QStringLiteral("atan2"), method_atan2, 2);
    defineDefaultProperty(ctx, QStringLiteral("ceil"), method_ceil, 1);
    defineDefaultProperty(ctx, QStringLiteral("cos"), method_cos, 1);
    defineDefaultProperty(ctx, QStringLiteral("exp"), method_exp, 1);
    defineDefaultProperty(ctx, QStringLiteral("floor"), method_floor, 1);
    defineDefaultProperty(ctx, QStringLiteral("log"), method_log, 1);
    defineDefaultProperty(ctx, QStringLiteral("max"), method_max, 2);
    defineDefaultProperty(ctx, QStringLiteral("min"), method_min, 2);
    defineDefaultProperty(ctx, QStringLiteral("pow"), method_pow, 2);
    defineDefaultProperty(ctx, QStringLiteral("random"), method_random, 0);
    defineDefaultProperty(ctx, QStringLiteral("round"), method_round, 1);
    defineDefaultProperty(ctx, QStringLiteral("sin"), method_sin, 1);
    defineDefaultProperty(ctx, QStringLiteral("sqrt"), method_sqrt, 1);
    defineDefaultProperty(ctx, QStringLiteral("tan"), method_tan, 1);
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

Value MathObject::method_abs(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v == 0) // 0 | -0
        return Value::fromDouble(0);

    return Value::fromDouble(v < 0 ? -v : v);
}

Value MathObject::method_acos(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v > 1)
        return Value::fromDouble(qSNaN());

    return Value::fromDouble(::acos(v));
}

Value MathObject::method_asin(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v > 1)
        return Value::fromDouble(qSNaN());
    else
        return Value::fromDouble(::asin(v));
}

Value MathObject::method_atan(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v == 0.0)
        return Value::fromDouble(v);
    else
        return Value::fromDouble(::atan(v));
}

Value MathObject::method_atan2(ExecutionContext *ctx)
{
    double v1 = ctx->argument(0).toNumber(ctx);
    double v2 = ctx->argument(1).toNumber(ctx);
    if ((v1 < 0) && qIsFinite(v1) && qIsInf(v2) && (copySign(1.0, v2) == 1.0)) {
        return Value::fromDouble(copySign(0, -1.0));
    }
    if ((v1 == 0.0) && (v2 == 0.0)) {
        if ((copySign(1.0, v1) == 1.0) && (copySign(1.0, v2) == -1.0)) {
            return Value::fromDouble(qt_PI);
        } else if ((copySign(1.0, v1) == -1.0) && (copySign(1.0, v2) == -1.0)) {
            return Value::fromDouble(-qt_PI);
        }
    }
    return Value::fromDouble(::atan2(v1, v2));
}

Value MathObject::method_ceil(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v < 0.0 && v > -1.0)
        return Value::fromDouble(copySign(0, -1.0));
    else
        return Value::fromDouble(::ceil(v));
}

Value MathObject::method_cos(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    return Value::fromDouble(::cos(v));
}

Value MathObject::method_exp(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (qIsInf(v)) {
        if (copySign(1.0, v) == -1.0)
            return Value::fromDouble(0);
        else
            return Value::fromDouble(qInf());
    } else {
        return Value::fromDouble(::exp(v));
    }
}

Value MathObject::method_floor(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    return Value::fromDouble(::floor(v));
}

Value MathObject::method_log(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v < 0)
        return Value::fromDouble(qSNaN());
    else
        return Value::fromDouble(::log(v));
}

Value MathObject::method_max(ExecutionContext *ctx)
{
    double mx = -qInf();
    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        double x = ctx->argument(i).toNumber(ctx);
        if (x > mx || std::isnan(x))
            mx = x;
    }
    return Value::fromDouble(mx);
}

Value MathObject::method_min(ExecutionContext *ctx)
{
    double mx = qInf();
    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        double x = ctx->argument(i).toNumber(ctx);
        if ((x == 0 && mx == x && copySign(1.0, x) == -1.0)
                || (x < mx) || std::isnan(x)) {
            mx = x;
        }
    }
    return Value::fromDouble(mx);
}

Value MathObject::method_pow(ExecutionContext *ctx)
{
    double x = ctx->argument(0).toNumber(ctx);
    double y = ctx->argument(1).toNumber(ctx);

    if (std::isnan(y))
        return Value::fromDouble(qSNaN());

    if (y == 0) {
        return Value::fromDouble(1);
    } else if (((x == 1) || (x == -1)) && std::isinf(y)) {
        return Value::fromDouble(qSNaN());
    } else if (((x == 0) && copySign(1.0, x) == 1.0) && (y < 0)) {
        return Value::fromDouble(qInf());
    } else if ((x == 0) && copySign(1.0, x) == -1.0) {
        if (y < 0) {
            if (::fmod(-y, 2.0) == 1.0)
                return Value::fromDouble(-qInf());
            else
                return Value::fromDouble(qInf());
        } else if (y > 0) {
            if (::fmod(y, 2.0) == 1.0)
                return Value::fromDouble(copySign(0, -1.0));
            else
                return Value::fromDouble(0);
        }
    }

#ifdef Q_OS_AIX
    else if (qIsInf(x) && copySign(1.0, x) == -1.0) {
        if (y > 0) {
            if (::fmod(y, 2.0) == 1.0)
                return Value::number(ctx, -qInf());
            else
                return Value::number(ctx, qInf());
        } else if (y < 0) {
            if (::fmod(-y, 2.0) == 1.0)
                return Value::number(ctx, copySign(0, -1.0));
            else
                return Value::number(ctx, 0);
        }
    }
#endif
    else {
        return Value::fromDouble(::pow(x, y));
    }
    // ###
    return Value::fromDouble(qSNaN());
}

Value MathObject::method_random(ExecutionContext */*ctx*/)
{
    return Value::fromDouble(qrand() / (double) RAND_MAX);
}

Value MathObject::method_round(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    v = copySign(::floor(v + 0.5), v);
    return Value::fromDouble(v);
}

Value MathObject::method_sin(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    return Value::fromDouble(::sin(v));
}

Value MathObject::method_sqrt(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    return Value::fromDouble(::sqrt(v));
}

Value MathObject::method_tan(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v == 0.0)
        return Value::fromDouble(v);
    else
        return Value::fromDouble(::tan(v));
}

