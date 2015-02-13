/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4mathobject_p.h"
#include "qv4objectproto_p.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qmath.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qthreadstorage.h>

#include <cmath>

using namespace QV4;

DEFINE_OBJECT_VTABLE(MathObject);

static const double qt_PI = 2.0 * ::asin(1.0);

Heap::MathObject::MathObject(ExecutionEngine *e)
    : Heap::Object(e->emptyClass, e->objectPrototype.asObject())
{
    Scope scope(e);
    ScopedObject m(scope, this);

    m->defineReadonlyProperty(QStringLiteral("E"), Primitive::fromDouble(M_E));
    m->defineReadonlyProperty(QStringLiteral("LN2"), Primitive::fromDouble(M_LN2));
    m->defineReadonlyProperty(QStringLiteral("LN10"), Primitive::fromDouble(M_LN10));
    m->defineReadonlyProperty(QStringLiteral("LOG2E"), Primitive::fromDouble(M_LOG2E));
    m->defineReadonlyProperty(QStringLiteral("LOG10E"), Primitive::fromDouble(M_LOG10E));
    m->defineReadonlyProperty(QStringLiteral("PI"), Primitive::fromDouble(M_PI));
    m->defineReadonlyProperty(QStringLiteral("SQRT1_2"), Primitive::fromDouble(M_SQRT1_2));
    m->defineReadonlyProperty(QStringLiteral("SQRT2"), Primitive::fromDouble(M_SQRT2));

    m->defineDefaultProperty(QStringLiteral("abs"), QV4::MathObject::method_abs, 1);
    m->defineDefaultProperty(QStringLiteral("acos"), QV4::MathObject::method_acos, 1);
    m->defineDefaultProperty(QStringLiteral("asin"), QV4::MathObject::method_asin, 0);
    m->defineDefaultProperty(QStringLiteral("atan"), QV4::MathObject::method_atan, 1);
    m->defineDefaultProperty(QStringLiteral("atan2"), QV4::MathObject::method_atan2, 2);
    m->defineDefaultProperty(QStringLiteral("ceil"), QV4::MathObject::method_ceil, 1);
    m->defineDefaultProperty(QStringLiteral("cos"), QV4::MathObject::method_cos, 1);
    m->defineDefaultProperty(QStringLiteral("exp"), QV4::MathObject::method_exp, 1);
    m->defineDefaultProperty(QStringLiteral("floor"), QV4::MathObject::method_floor, 1);
    m->defineDefaultProperty(QStringLiteral("log"), QV4::MathObject::method_log, 1);
    m->defineDefaultProperty(QStringLiteral("max"), QV4::MathObject::method_max, 2);
    m->defineDefaultProperty(QStringLiteral("min"), QV4::MathObject::method_min, 2);
    m->defineDefaultProperty(QStringLiteral("pow"), QV4::MathObject::method_pow, 2);
    m->defineDefaultProperty(QStringLiteral("random"), QV4::MathObject::method_random, 0);
    m->defineDefaultProperty(QStringLiteral("round"), QV4::MathObject::method_round, 1);
    m->defineDefaultProperty(QStringLiteral("sin"), QV4::MathObject::method_sin, 1);
    m->defineDefaultProperty(QStringLiteral("sqrt"), QV4::MathObject::method_sqrt, 1);
    m->defineDefaultProperty(QStringLiteral("tan"), QV4::MathObject::method_tan, 1);
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

ReturnedValue MathObject::method_abs(CallContext *context)
{
    if (!context->argc())
        return Encode(qSNaN());

    if (context->args()[0].isInteger()) {
        int i = context->args()[0].integerValue();
        return Encode(i < 0 ? - i : i);
    }

    double v = context->args()[0].toNumber();
    if (v == 0) // 0 | -0
        return Encode(0);

    return Encode(v < 0 ? -v : v);
}

ReturnedValue MathObject::method_acos(CallContext *context)
{
    double v = context->argc() ? context->args()[0].toNumber() : 2;
    if (v > 1)
        return Encode(qSNaN());

    return Encode(std::acos(v));
}

ReturnedValue MathObject::method_asin(CallContext *context)
{
    double v = context->argc() ? context->args()[0].toNumber() : 2;
    if (v > 1)
        return Encode(qSNaN());
    else
        return Encode(std::asin(v));
}

ReturnedValue MathObject::method_atan(CallContext *context)
{
    double v = context->argc() ? context->args()[0].toNumber() : qSNaN();
    if (v == 0.0)
        return Encode(v);
    else
        return Encode(std::atan(v));
}

ReturnedValue MathObject::method_atan2(CallContext *context)
{
    double v1 = context->argc() ? context->args()[0].toNumber() : qSNaN();
    double v2 = context->argc() > 1 ? context->args()[1].toNumber() : qSNaN();

    if ((v1 < 0) && qIsFinite(v1) && qIsInf(v2) && (copySign(1.0, v2) == 1.0))
        return Encode(copySign(0, -1.0));

    if ((v1 == 0.0) && (v2 == 0.0)) {
        if ((copySign(1.0, v1) == 1.0) && (copySign(1.0, v2) == -1.0)) {
            return Encode(M_PI);
        } else if ((copySign(1.0, v1) == -1.0) && (copySign(1.0, v2) == -1.0)) {
            return Encode(-M_PI);
        }
    }
    return Encode(std::atan2(v1, v2));
}

ReturnedValue MathObject::method_ceil(CallContext *context)
{
    double v = context->argc() ? context->args()[0].toNumber() : qSNaN();
    if (v < 0.0 && v > -1.0)
        return Encode(copySign(0, -1.0));
    else
        return Encode(std::ceil(v));
}

ReturnedValue MathObject::method_cos(CallContext *context)
{
    double v = context->argc() ? context->args()[0].toNumber() : qSNaN();
    return Encode(std::cos(v));
}

ReturnedValue MathObject::method_exp(CallContext *context)
{
    double v = context->argc() ? context->args()[0].toNumber() : qSNaN();
    if (qIsInf(v)) {
        if (copySign(1.0, v) == -1.0)
            return Encode(0);
        else
            return Encode(qInf());
    } else {
        return Encode(std::exp(v));
    }
}

ReturnedValue MathObject::method_floor(CallContext *context)
{
    double v = context->argc() ? context->args()[0].toNumber() : qSNaN();
    return Encode(std::floor(v));
}

ReturnedValue MathObject::method_log(CallContext *context)
{
    double v = context->argc() ? context->args()[0].toNumber() : qSNaN();
    if (v < 0)
        return Encode(qSNaN());
    else
        return Encode(std::log(v));
}

ReturnedValue MathObject::method_max(CallContext *context)
{
    double mx = -qInf();
    for (int i = 0; i < context->argc(); ++i) {
        double x = context->args()[i].toNumber();
        if (x > mx || std::isnan(x))
            mx = x;
    }
    return Encode(mx);
}

ReturnedValue MathObject::method_min(CallContext *context)
{
    double mx = qInf();
    for (int i = 0; i < context->argc(); ++i) {
        double x = context->args()[i].toNumber();
        if ((x == 0 && mx == x && copySign(1.0, x) == -1.0)
                || (x < mx) || std::isnan(x)) {
            mx = x;
        }
    }
    return Encode(mx);
}

ReturnedValue MathObject::method_pow(CallContext *context)
{
    double x = context->argc() > 0 ? context->args()[0].toNumber() : qSNaN();
    double y = context->argc() > 1 ? context->args()[1].toNumber() : qSNaN();

    if (std::isnan(y))
        return Encode(qSNaN());

    if (y == 0) {
        return Encode(1);
    } else if (((x == 1) || (x == -1)) && std::isinf(y)) {
        return Encode(qSNaN());
    } else if (((x == 0) && copySign(1.0, x) == 1.0) && (y < 0)) {
        return Encode(qInf());
    } else if ((x == 0) && copySign(1.0, x) == -1.0) {
        if (y < 0) {
            if (std::fmod(-y, 2.0) == 1.0)
                return Encode(-qInf());
            else
                return Encode(qInf());
        } else if (y > 0) {
            if (std::fmod(y, 2.0) == 1.0)
                return Encode(copySign(0, -1.0));
            else
                return Encode(0);
        }
    }

#ifdef Q_OS_AIX
    else if (qIsInf(x) && copySign(1.0, x) == -1.0) {
        if (y > 0) {
            if (std::fmod(y, 2.0) == 1.0)
                return Encode(-qInf());
            else
                return Encode(qInf());
        } else if (y < 0) {
            if (std::fmod(-y, 2.0) == 1.0)
                return Encode(copySign(0, -1.0));
            else
                return Encode(0);
        }
    }
#endif
    else {
        return Encode(std::pow(x, y));
    }
    // ###
    return Encode(qSNaN());
}

Q_GLOBAL_STATIC(QThreadStorage<bool *>, seedCreatedStorage);

ReturnedValue MathObject::method_random(CallContext *context)
{
    if (!seedCreatedStorage()->hasLocalData()) {
        qsrand(QTime(0,0,0).msecsTo(QTime::currentTime()) ^ reinterpret_cast<quintptr>(context));
        seedCreatedStorage()->setLocalData(new bool(true));
    }
    return Encode(qrand() / (double) RAND_MAX);
}

ReturnedValue MathObject::method_round(CallContext *context)
{
    double v = context->argc() ? context->args()[0].toNumber() : qSNaN();
    v = copySign(std::floor(v + 0.5), v);
    return Encode(v);
}

ReturnedValue MathObject::method_sin(CallContext *context)
{
    double v = context->argc() ? context->args()[0].toNumber() : qSNaN();
    return Encode(std::sin(v));
}

ReturnedValue MathObject::method_sqrt(CallContext *context)
{
    double v = context->argc() ? context->args()[0].toNumber() : qSNaN();
    return Encode(std::sqrt(v));
}

ReturnedValue MathObject::method_tan(CallContext *context)
{
    double v = context->argc() ? context->args()[0].toNumber() : qSNaN();
    if (v == 0.0)
        return Encode(v);
    else
        return Encode(std::tan(v));
}

