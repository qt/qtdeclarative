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


#include "qv4ecmaobjects_p.h"
#include "qv4array_p.h"
#include <QtCore/qnumeric.h>
#include <QtCore/qmath.h>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QDebug>
#include <cmath>
#include <qmath.h>
#include <qnumeric.h>
#include <cassert>

#ifndef Q_WS_WIN
#  include <time.h>
#  ifndef Q_OS_VXWORKS
#    include <sys/time.h>
#  else
#    include "qplatformdefs.h"
#  endif
#else
#  include <windows.h>
#endif

using namespace QQmlJS::VM;

static const double qt_PI = 2.0 * ::asin(1.0);

static const double HoursPerDay = 24.0;
static const double MinutesPerHour = 60.0;
static const double SecondsPerMinute = 60.0;
static const double msPerSecond = 1000.0;
static const double msPerMinute = 60000.0;
static const double msPerHour = 3600000.0;
static const double msPerDay = 86400000.0;

static double LocalTZA = 0.0; // initialized at startup

static inline double TimeWithinDay(double t)
{
    double r = ::fmod(t, msPerDay);
    return (r >= 0) ? r : r + msPerDay;
}

static inline int HourFromTime(double t)
{
    int r = int(::fmod(::floor(t / msPerHour), HoursPerDay));
    return (r >= 0) ? r : r + int(HoursPerDay);
}

static inline int MinFromTime(double t)
{
    int r = int(::fmod(::floor(t / msPerMinute), MinutesPerHour));
    return (r >= 0) ? r : r + int(MinutesPerHour);
}

static inline int SecFromTime(double t)
{
    int r = int(::fmod(::floor(t / msPerSecond), SecondsPerMinute));
    return (r >= 0) ? r : r + int(SecondsPerMinute);
}

static inline int msFromTime(double t)
{
    int r = int(::fmod(t, msPerSecond));
    return (r >= 0) ? r : r + int(msPerSecond);
}

static inline double Day(double t)
{
    return ::floor(t / msPerDay);
}

static inline double DaysInYear(double y)
{
    if (::fmod(y, 4))
        return 365;

    else if (::fmod(y, 100))
        return 366;

    else if (::fmod(y, 400))
        return 365;

    return 366;
}

static inline double DayFromYear(double y)
{
    return 365 * (y - 1970)
        + ::floor((y - 1969) / 4)
        - ::floor((y - 1901) / 100)
        + ::floor((y - 1601) / 400);
}

static inline double TimeFromYear(double y)
{
    return msPerDay * DayFromYear(y);
}

static inline double YearFromTime(double t)
{
    int y = 1970;
    y += (int) ::floor(t / (msPerDay * 365.2425));

    double t2 = TimeFromYear(y);
    return (t2 > t) ? y - 1 : ((t2 + msPerDay * DaysInYear(y)) <= t) ? y + 1 : y;
}

static inline bool InLeapYear(double t)
{
    double x = DaysInYear(YearFromTime(t));
    if (x == 365)
        return 0;

    assert(x == 366);
    return 1;
}

static inline double DayWithinYear(double t)
{
    return Day(t) - DayFromYear(YearFromTime(t));
}

static inline double MonthFromTime(double t)
{
    double d = DayWithinYear(t);
    double l = InLeapYear(t);

    if (d < 31.0)
        return 0;

    else if (d < 59.0 + l)
        return 1;

    else if (d < 90.0 + l)
        return 2;

    else if (d < 120.0 + l)
        return 3;

    else if (d < 151.0 + l)
        return 4;

    else if (d < 181.0 + l)
        return 5;

    else if (d < 212.0 + l)
        return 6;

    else if (d < 243.0 + l)
        return 7;

    else if (d < 273.0 + l)
        return 8;

    else if (d < 304.0 + l)
        return 9;

    else if (d < 334.0 + l)
        return 10;

    else if (d < 365.0 + l)
        return 11;

    return qSNaN(); // ### assert?
}

static inline double DateFromTime(double t)
{
    int m = (int) Value::toInteger(MonthFromTime(t));
    double d = DayWithinYear(t);
    double l = InLeapYear(t);

    switch (m) {
    case 0: return d + 1.0;
    case 1: return d - 30.0;
    case 2: return d - 58.0 - l;
    case 3: return d - 89.0 - l;
    case 4: return d - 119.0 - l;
    case 5: return d - 150.0 - l;
    case 6: return d - 180.0 - l;
    case 7: return d - 211.0 - l;
    case 8: return d - 242.0 - l;
    case 9: return d - 272.0 - l;
    case 10: return d - 303.0 - l;
    case 11: return d - 333.0 - l;
    }

    return qSNaN(); // ### assert
}

static inline double WeekDay(double t)
{
    double r = ::fmod (Day(t) + 4.0, 7.0);
    return (r >= 0) ? r : r + 7.0;
}


static inline double MakeTime(double hour, double min, double sec, double ms)
{
    return ((hour * MinutesPerHour + min) * SecondsPerMinute + sec) * msPerSecond + ms;
}

static inline double DayFromMonth(double month, double leap)
{
    switch ((int) month) {
    case 0: return 0;
    case 1: return 31.0;
    case 2: return 59.0 + leap;
    case 3: return 90.0 + leap;
    case 4: return 120.0 + leap;
    case 5: return 151.0 + leap;
    case 6: return 181.0 + leap;
    case 7: return 212.0 + leap;
    case 8: return 243.0 + leap;
    case 9: return 273.0 + leap;
    case 10: return 304.0 + leap;
    case 11: return 334.0 + leap;
    }

    return qSNaN(); // ### assert?
}

static double MakeDay(double year, double month, double day)
{
    year += ::floor(month / 12.0);

    month = ::fmod(month, 12.0);
    if (month < 0)
        month += 12.0;

    double t = TimeFromYear(year);
    double leap = InLeapYear(t);

    day += ::floor(t / msPerDay);
    day += DayFromMonth(month, leap);

    return day - 1;
}

static inline double MakeDate(double day, double time)
{
    return day * msPerDay + time;
}

static inline double DaylightSavingTA(double t)
{
#ifndef Q_WS_WIN
    long int tt = (long int)(t / msPerSecond);
    struct tm *tmtm = localtime((const time_t*)&tt);
    if (! tmtm)
        return 0;
    return (tmtm->tm_isdst > 0) ? msPerHour : 0;
#else
    Q_UNUSED(t);
    /// ### implement me
    return 0;
#endif
}

static inline double LocalTime(double t)
{
    return t + LocalTZA + DaylightSavingTA(t);
}

static inline double UTC(double t)
{
    return t - LocalTZA - DaylightSavingTA(t - LocalTZA);
}

static inline double currentTime()
{
#ifndef Q_WS_WIN
    struct timeval tv;

    gettimeofday(&tv, 0);
    return ::floor(tv.tv_sec * msPerSecond + (tv.tv_usec / 1000.0));
#else
    SYSTEMTIME st;
    GetSystemTime(&st);
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    LARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    return double(li.QuadPart - Q_INT64_C(116444736000000000)) / 10000.0;
#endif
}

static inline double TimeClip(double t)
{
    if (! qIsFinite(t) || fabs(t) > 8.64e15)
        return qSNaN();
    return Value::toInteger(t);
}

static inline double FromDateTime(const QDateTime &dt)
{
    if (!dt.isValid())
        return qSNaN();
    QDate date = dt.date();
    QTime taim = dt.time();
    int year = date.year();
    int month = date.month() - 1;
    int day = date.day();
    int hours = taim.hour();
    int mins = taim.minute();
    int secs = taim.second();
    int ms = taim.msec();
    double t = MakeDate(MakeDay(year, month, day),
                        MakeTime(hours, mins, secs, ms));
    if (dt.timeSpec() == Qt::LocalTime)
        t = UTC(t);
    return TimeClip(t);
}

static inline double ParseString(const QString &s)
{
    QDateTime dt = QDateTime::fromString(s, Qt::TextDate);
    if (!dt.isValid())
        dt = QDateTime::fromString(s, Qt::ISODate);
    if (!dt.isValid()) {
        QStringList formats;
        formats << QStringLiteral("M/d/yyyy")
                << QStringLiteral("M/d/yyyy hh:mm")
                << QStringLiteral("M/d/yyyy hh:mm A")

                << QStringLiteral("M/d/yyyy, hh:mm")
                << QStringLiteral("M/d/yyyy, hh:mm A")

                << QStringLiteral("MMM d yyyy")
                << QStringLiteral("MMM d yyyy hh:mm")
                << QStringLiteral("MMM d yyyy hh:mm:ss")
                << QStringLiteral("MMM d yyyy, hh:mm")
                << QStringLiteral("MMM d yyyy, hh:mm:ss")

                << QStringLiteral("MMMM d yyyy")
                << QStringLiteral("MMMM d yyyy hh:mm")
                << QStringLiteral("MMMM d yyyy hh:mm:ss")
                << QStringLiteral("MMMM d yyyy, hh:mm")
                << QStringLiteral("MMMM d yyyy, hh:mm:ss")

                << QStringLiteral("MMM d, yyyy")
                << QStringLiteral("MMM d, yyyy hh:mm")
                << QStringLiteral("MMM d, yyyy hh:mm:ss")

                << QStringLiteral("MMMM d, yyyy")
                << QStringLiteral("MMMM d, yyyy hh:mm")
                << QStringLiteral("MMMM d, yyyy hh:mm:ss")

                << QStringLiteral("d MMM yyyy")
                << QStringLiteral("d MMM yyyy hh:mm")
                << QStringLiteral("d MMM yyyy hh:mm:ss")
                << QStringLiteral("d MMM yyyy, hh:mm")
                << QStringLiteral("d MMM yyyy, hh:mm:ss")

                << QStringLiteral("d MMMM yyyy")
                << QStringLiteral("d MMMM yyyy hh:mm")
                << QStringLiteral("d MMMM yyyy hh:mm:ss")
                << QStringLiteral("d MMMM yyyy, hh:mm")
                << QStringLiteral("d MMMM yyyy, hh:mm:ss")

                << QStringLiteral("d MMM, yyyy")
                << QStringLiteral("d MMM, yyyy hh:mm")
                << QStringLiteral("d MMM, yyyy hh:mm:ss")

                << QStringLiteral("d MMMM, yyyy")
                << QStringLiteral("d MMMM, yyyy hh:mm")
                << QStringLiteral("d MMMM, yyyy hh:mm:ss");

        for (int i = 0; i < formats.size(); ++i) {
            dt = QDateTime::fromString(s, formats.at(i));
            if (dt.isValid())
                break;
        }
    }
    return FromDateTime(dt);
}

/*!
  \internal

  Converts the ECMA Date value \tt (in UTC form) to QDateTime
  according to \a spec.
*/
static inline QDateTime ToDateTime(double t, Qt::TimeSpec spec)
{
    if (qIsNaN(t))
        return QDateTime();
    if (spec == Qt::LocalTime)
        t = LocalTime(t);
    int year = int(YearFromTime(t));
    int month = int(MonthFromTime(t) + 1);
    int day = int(DateFromTime(t));
    int hours = HourFromTime(t);
    int mins = MinFromTime(t);
    int secs = SecFromTime(t);
    int ms = msFromTime(t);
    return QDateTime(QDate(year, month, day), QTime(hours, mins, secs, ms), spec);
}

static inline QString ToString(double t)
{
    if (qIsNaN(t))
        return QStringLiteral("Invalid Date");
    QString str = ToDateTime(t, Qt::LocalTime).toString() + QStringLiteral(" GMT");
    double tzoffset = LocalTZA + DaylightSavingTA(t);
    if (tzoffset) {
        int hours = static_cast<int>(::fabs(tzoffset) / 1000 / 60 / 60);
        int mins = int(::fabs(tzoffset) / 1000 / 60) % 60;
        str.append(QLatin1Char((tzoffset > 0) ?  '+' : '-'));
        if (hours < 10)
            str.append(QLatin1Char('0'));
        str.append(QString::number(hours));
        if (mins < 10)
            str.append(QLatin1Char('0'));
        str.append(QString::number(mins));
    }
    return str;
}

static inline QString ToUTCString(double t)
{
    if (qIsNaN(t))
        return QStringLiteral("Invalid Date");
    return ToDateTime(t, Qt::UTC).toString() + QStringLiteral(" GMT");
}

static inline QString ToDateString(double t)
{
    return ToDateTime(t, Qt::LocalTime).date().toString();
}

static inline QString ToTimeString(double t)
{
    return ToDateTime(t, Qt::LocalTime).time().toString();
}

static inline QString ToLocaleString(double t)
{
    return ToDateTime(t, Qt::LocalTime).toString(Qt::LocaleDate);
}

static inline QString ToLocaleDateString(double t)
{
    return ToDateTime(t, Qt::LocalTime).date().toString(Qt::LocaleDate);
}

static inline QString ToLocaleTimeString(double t)
{
    return ToDateTime(t, Qt::LocalTime).time().toString(Qt::LocaleDate);
}

static double getLocalTZA()
{
#ifndef Q_WS_WIN
    struct tm* t;
    time_t curr;
    time(&curr);
    t = localtime(&curr);
    time_t locl = mktime(t);
    t = gmtime(&curr);
    time_t globl = mktime(t);
    return double(locl - globl) * 1000.0;
#else
    TIME_ZONE_INFORMATION tzInfo;
    GetTimeZoneInformation(&tzInfo);
    return -tzInfo.Bias * 60.0 * 1000.0;
#endif
}

//
// Object
//
ObjectCtor::ObjectCtor(Context *scope)
    : FunctionObject(scope)
{
}

void ObjectCtor::construct(Context *ctx)
{
    ctx->thisObject = Value::fromObject(ctx->engine->newObject());
}

void ObjectCtor::call(Context *ctx)
{
    ctx->result = Value::fromObject(ctx->engine->newObject());
}

Value ObjectCtor::getProperty(Context *ctx, String *name, PropertyAttributes *attributes)
{
    if (name == ctx->engine->id_length)
        return Value::fromDouble(1);
    return Object::getProperty(ctx, name, attributes);
}

void ObjectPrototype::init(Context *ctx, const Value &ctor)
{
    ctor.objectValue()->setProperty(ctx, ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->setProperty(ctx, QStringLiteral("getPrototypeOf"), method_getPrototypeOf, 0);
    ctor.objectValue()->setProperty(ctx, QStringLiteral("getOwnPropertyDescriptor"), method_getOwnPropertyDescriptor, 0);
    ctor.objectValue()->setProperty(ctx, QStringLiteral("getOwnPropertyNames"), method_getOwnPropertyNames, 0);
    ctor.objectValue()->setProperty(ctx, QStringLiteral("create"), method_create, 0);
    ctor.objectValue()->setProperty(ctx, QStringLiteral("defineProperty"), method_defineProperty, 0);
    ctor.objectValue()->setProperty(ctx, QStringLiteral("defineProperties"), method_defineProperties, 0);
    ctor.objectValue()->setProperty(ctx, QStringLiteral("seal"), method_seal, 0);
    ctor.objectValue()->setProperty(ctx, QStringLiteral("freeze"), method_freeze, 0);
    ctor.objectValue()->setProperty(ctx, QStringLiteral("preventExtensions"), method_preventExtensions, 0);
    ctor.objectValue()->setProperty(ctx, QStringLiteral("isSealed"), method_isSealed, 0);
    ctor.objectValue()->setProperty(ctx, QStringLiteral("isFrozen"), method_isFrozen, 0);
    ctor.objectValue()->setProperty(ctx, QStringLiteral("isExtensible"), method_isExtensible, 0);
    ctor.objectValue()->setProperty(ctx, QStringLiteral("keys"), method_keys, 0);

    setProperty(ctx, QStringLiteral("constructor"), ctor);
    setProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    setProperty(ctx, QStringLiteral("toLocaleString"), method_toLocaleString, 0);
    setProperty(ctx, QStringLiteral("valueOf"), method_valueOf, 0);
    setProperty(ctx, QStringLiteral("hasOwnProperty"), method_hasOwnProperty, 0);
    setProperty(ctx, QStringLiteral("isPrototypeOf"), method_isPrototypeOf, 0);
    setProperty(ctx, QStringLiteral("propertyIsEnumerable"), method_propertyIsEnumerable, 0);
}

void ObjectPrototype::method_getPrototypeOf(Context *ctx)
{
    Value o = ctx->argument(0);
    if (! o.isObject()) {
        ctx->throwTypeError();
    } else {
        ctx->result = Value::fromObject(o.objectValue()->prototype);
    }
}

void ObjectPrototype::method_getOwnPropertyDescriptor(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Object.getOwnPropertyDescriptors"));
}

void ObjectPrototype::method_getOwnPropertyNames(Context *ctx)
{
    Value O = ctx->argument(0);
    if (! O.isObject())
        ctx->throwTypeError();
    else {
        ArrayObject *array = ctx->engine->newArrayObject()->asArrayObject();
        Array &a = array->value;
        if (Table *members = O.objectValue()->members) {
            for (Property **it = members->begin(), **end = members->end(); it != end; ++it) {
                if (Property *prop = *it) {
                    a.push(Value::fromString(prop->name));
                }
            }
        }
        ctx->result = Value::fromObject(array);
    }
}

void ObjectPrototype::method_create(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Object.create"));
}

void ObjectPrototype::method_defineProperty(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Object.defineProperty"));
}

void ObjectPrototype::method_defineProperties(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Object.defineProperties"));
}

void ObjectPrototype::method_seal(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Object.seal"));
}

void ObjectPrototype::method_freeze(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Object.freeze"));
}

void ObjectPrototype::method_preventExtensions(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Object.preventExtensions"));
}

void ObjectPrototype::method_isSealed(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Object.isSealed"));
}

void ObjectPrototype::method_isFrozen(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Object.isFrozen"));
}

void ObjectPrototype::method_isExtensible(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Object.isExtensible"));
}

void ObjectPrototype::method_keys(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Object.keys"));
}

void ObjectPrototype::method_toString(Context *ctx)
{
    if (! ctx->thisObject.isObject())
        ctx->throwTypeError();
    else
        ctx->result = Value::fromString(ctx, QString::fromUtf8("[object %1]").arg(ctx->thisObject.objectValue()->className()));
}

void ObjectPrototype::method_toLocaleString(Context *ctx)
{
    method_toString(ctx);
}

void ObjectPrototype::method_valueOf(Context *ctx)
{
    Value o = ctx->thisObject.toObject(ctx);
    ctx->result = o;
}

void ObjectPrototype::method_hasOwnProperty(Context *ctx)
{
    String *P = ctx->argument(0).toString(ctx);
    Value O = ctx->thisObject.toObject(ctx);
    bool r = O.objectValue()->getOwnProperty(ctx, P) != 0;
    ctx->result = Value::fromBoolean(r);
}

void ObjectPrototype::method_isPrototypeOf(Context *ctx)
{
    Value V = ctx->argument(0);
    if (! V.isObject())
        ctx->result = Value::fromBoolean(false);
    else {
        Value O = ctx->thisObject.toObject(ctx);
        Object *proto = V.objectValue()->prototype;
        ctx->result = Value::fromBoolean(proto && O.objectValue() == proto);
    }
}

void ObjectPrototype::method_propertyIsEnumerable(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Object.prototype.propertyIsEnumerable"));
}

//
// String
//
StringCtor::StringCtor(Context *scope)
    : FunctionObject(scope)
{
}

void StringCtor::construct(Context *ctx)
{
    Value value;
    if (ctx->argumentCount)
        value = Value::fromString(ctx->argument(0).toString(ctx));
    else
        value = Value::fromString(ctx, QString());
    ctx->thisObject = Value::fromObject(ctx->engine->newStringObject(value));
}

void StringCtor::call(Context *ctx)
{
    const Value arg = ctx->argument(0);
    if (arg.isUndefined())
        ctx->result = Value::fromString(ctx->engine->newString(QString()));
    else
        ctx->result = __qmljs_to_string(arg, ctx);
}

void StringPrototype::init(Context *ctx, const Value &ctor)
{
    ctor.objectValue()->setProperty(ctx, ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->setProperty(ctx, QStringLiteral("fromCharCode"), method_fromCharCode);

    setProperty(ctx, QStringLiteral("constructor"), ctor);
    setProperty(ctx, QStringLiteral("toString"), method_toString);
    setProperty(ctx, QStringLiteral("valueOf"), method_valueOf);
    setProperty(ctx, QStringLiteral("charAt"), method_charAt);
    setProperty(ctx, QStringLiteral("charCodeAt"), method_charCodeAt);
    setProperty(ctx, QStringLiteral("concat"), method_concat);
    setProperty(ctx, QStringLiteral("indexOf"), method_indexOf);
    setProperty(ctx, QStringLiteral("lastIndexOf"), method_lastIndexOf);
    setProperty(ctx, QStringLiteral("localeCompare"), method_localeCompare);
    setProperty(ctx, QStringLiteral("match"), method_match);
    setProperty(ctx, QStringLiteral("replace"), method_replace);
    setProperty(ctx, QStringLiteral("search"), method_search);
    setProperty(ctx, QStringLiteral("slice"), method_slice);
    setProperty(ctx, QStringLiteral("split"), method_split);
    setProperty(ctx, QStringLiteral("substr"), method_substr);
    setProperty(ctx, QStringLiteral("substring"), method_substring);
    setProperty(ctx, QStringLiteral("toLowerCase"), method_toLowerCase);
    setProperty(ctx, QStringLiteral("toLocaleLowerCase"), method_toLocaleLowerCase);
    setProperty(ctx, QStringLiteral("toUpperCase"), method_toUpperCase);
    setProperty(ctx, QStringLiteral("toLocaleUpperCase"), method_toLocaleUpperCase);
}

QString StringPrototype::getThisString(Context *ctx)
{
    if (StringObject *thisObject = ctx->thisObject.asStringObject()) {
        return thisObject->value.stringValue()->toQString();
    } else {
        ctx->throwTypeError();
        return QString();
    }
}

void StringPrototype::method_toString(Context *ctx)
{
    if (StringObject *o = ctx->thisObject.asStringObject()) {
        ctx->result = o->value;
    } else {
        ctx->throwTypeError();
    }
}

void StringPrototype::method_valueOf(Context *ctx)
{
    if (StringObject *o = ctx->thisObject.asStringObject()) {
        ctx->result = o->value;
    } else {
        ctx->throwTypeError();
    }
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

    ctx->result = Value::fromString(ctx, result);
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

    ctx->result = Value::fromDouble(result);
}

void StringPrototype::method_concat(Context *ctx)
{
    QString value = getThisString(ctx);

    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        Value v = __qmljs_to_string(ctx->arguments[i], ctx);
        assert(v.isString());
        value += v.stringValue()->toQString();
    }

    ctx->result = Value::fromString(ctx, value);
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

    ctx->result = Value::fromDouble(index);
}

void StringPrototype::method_lastIndexOf(Context *ctx)
{
    const QString value = getThisString(ctx);

    QString searchString;
    if (ctx->argumentCount) {
        Value v = __qmljs_to_string(ctx->arguments[0], ctx);
        searchString = v.stringValue()->toQString();
    }

    Value posArg = ctx->argument(1);
    double position = __qmljs_to_number(posArg, ctx);
    if (qIsNaN(position))
        position = +qInf();
    else
        position = trunc(position);

    int pos = trunc(qMin(qMax(position, 0.0), double(value.length())));
    if (!searchString.isEmpty() && pos == value.length())
        --pos;
    int index = value.lastIndexOf(searchString, pos);
    ctx->result = Value::fromDouble(index);
}

void StringPrototype::method_localeCompare(Context *ctx)
{
    const QString value = getThisString(ctx);
    const QString that = ctx->argument(0).toString(ctx)->toQString();
    ctx->result = Value::fromDouble(QString::localeAwareCompare(value, that));
}

void StringPrototype::method_match(Context *ctx)
{
    // requires Regexp
    ctx->throwUnimplemented(QStringLiteral("String.prototype.match"));
}

void StringPrototype::method_replace(Context *ctx)
{
    // requires Regexp
    ctx->throwUnimplemented(QStringLiteral("String.prototype.replace"));
}

void StringPrototype::method_search(Context *ctx)
{
    // requires Regexp
    ctx->throwUnimplemented(QStringLiteral("String.prototype.search"));
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
    ctx->result = Value::fromString(ctx, text.mid(start, count));
}

void StringPrototype::method_split(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("String.prototype.splt"));
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
    ctx->result = Value::fromString(ctx, value.mid(x, y));
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
    ctx->result = Value::fromString(ctx, value.mid(x, y));
}

void StringPrototype::method_toLowerCase(Context *ctx)
{
    QString value = getThisString(ctx);
    ctx->result = Value::fromString(ctx, value.toLower());
}

void StringPrototype::method_toLocaleLowerCase(Context *ctx)
{
    method_toLowerCase(ctx);
}

void StringPrototype::method_toUpperCase(Context *ctx)
{
    QString value = getThisString(ctx);
    ctx->result = Value::fromString(ctx, value.toUpper());
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
    ctx->result = Value::fromString(ctx, str);
}

//
// Number object
//
NumberCtor::NumberCtor(Context *scope)
    : FunctionObject(scope)
{
}

void NumberCtor::construct(Context *ctx)
{
    const double n = ctx->argument(0).toNumber(ctx);
    ctx->thisObject = Value::fromObject(ctx->engine->newNumberObject(Value::fromDouble(n)));
}

void NumberCtor::call(Context *ctx)
{
    double value = ctx->argumentCount ? ctx->argument(0).toNumber(ctx) : 0;
    ctx->result = Value::fromDouble(value);
}

void NumberPrototype::init(Context *ctx, const Value &ctor)
{
    ctor.objectValue()->setProperty(ctx, ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->setProperty(ctx, QStringLiteral("NaN"), Value::fromDouble(qSNaN()));
    ctor.objectValue()->setProperty(ctx, QStringLiteral("NEGATIVE_INFINITY"), Value::fromDouble(-qInf()));
    ctor.objectValue()->setProperty(ctx, QStringLiteral("POSITIVE_INFINITY"), Value::fromDouble(qInf()));
    ctor.objectValue()->setProperty(ctx, QStringLiteral("MAX_VALUE"), Value::fromDouble(1.7976931348623158e+308));
#ifdef __INTEL_COMPILER
# pragma warning( push )
# pragma warning(disable: 239)
#endif
    ctor.objectValue()->setProperty(ctx, QStringLiteral("MIN_VALUE"), Value::fromDouble(5e-324));
#ifdef __INTEL_COMPILER
# pragma warning( pop )
#endif

    setProperty(ctx, QStringLiteral("constructor"), ctor);
    setProperty(ctx, QStringLiteral("toString"), method_toString);
    setProperty(ctx, QStringLiteral("toLocalString"), method_toLocaleString);
    setProperty(ctx, QStringLiteral("valueOf"), method_valueOf);
    setProperty(ctx, QStringLiteral("toFixed"), method_toFixed);
    setProperty(ctx, QStringLiteral("toExponential"), method_toExponential);
    setProperty(ctx, QStringLiteral("toPrecision"), method_toPrecision);
}

void NumberPrototype::method_toString(Context *ctx)
{
    if (NumberObject *thisObject = ctx->thisObject.asNumberObject()) {
        Value arg = ctx->argument(0);
        if (!arg.isUndefined()) {
            int radix = arg.toInt32(ctx);
            if (radix < 2 || radix > 36) {
                ctx->throwError(QString::fromLatin1("Number.prototype.toString: %0 is not a valid radix")
                                .arg(radix));
                return;
            }

            double num = thisObject->value.asDouble();
            if (qIsNaN(num)) {
                ctx->result = Value::fromString(ctx, QStringLiteral("NaN"));
                return;
            } else if (qIsInf(num)) {
                ctx->result = Value::fromString(ctx, QLatin1String(num < 0 ? "-Infinity" : "Infinity"));
                return;
            }

            if (radix != 10) {
                QString str;
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
                ctx->result = Value::fromString(ctx, str);
                return;
            }
        }

        Value internalValue = thisObject->value;
        String *str = internalValue.toString(ctx);
        ctx->result = Value::fromString(str);
    } else {
        ctx->throwTypeError();
    }
}

void NumberPrototype::method_toLocaleString(Context *ctx)
{
    if (NumberObject *thisObject = ctx->thisObject.asNumberObject()) {
        String *str = thisObject->value.toString(ctx);
        ctx->result = Value::fromString(str);
    } else {
        ctx->throwTypeError();
    }
}

void NumberPrototype::method_valueOf(Context *ctx)
{
    if (NumberObject *thisObject = ctx->thisObject.asNumberObject()) {
        ctx->result = thisObject->value;
    } else {
        ctx->throwTypeError();
    }
}

void NumberPrototype::method_toFixed(Context *ctx)
{
    if (NumberObject *thisObject = ctx->thisObject.asNumberObject()) {
        double fdigits = 0;

        if (ctx->argumentCount > 0)
            fdigits = ctx->argument(0).toInteger(ctx);

        if (qIsNaN(fdigits))
            fdigits = 0;

        double v = thisObject->value.asDouble();
        QString str;
        if (qIsNaN(v))
            str = QString::fromLatin1("NaN");
        else if (qIsInf(v))
            str = QString::fromLatin1(v < 0 ? "-Infinity" : "Infinity");
        else
            str = QString::number(v, 'f', int (fdigits));
        ctx->result = Value::fromString(ctx, str);
    } else {
        ctx->throwTypeError();
    }
}

void NumberPrototype::method_toExponential(Context *ctx)
{
    if (NumberObject *thisObject = ctx->thisObject.asNumberObject()) {
        double fdigits = 0;

        if (ctx->argumentCount > 0)
            fdigits = ctx->argument(0).toInteger(ctx);

        QString z = QString::number(thisObject->value.asDouble(), 'e', int (fdigits));
        ctx->result = Value::fromString(ctx, z);
    } else {
        ctx->throwTypeError();
    }
}

void NumberPrototype::method_toPrecision(Context *ctx)
{
    if (NumberObject *thisObject = ctx->thisObject.asNumberObject()) {
        double fdigits = 0;

        if (ctx->argumentCount > 0)
            fdigits = ctx->argument(0).toInteger(ctx);

        ctx->result = Value::fromString(ctx, QString::number(thisObject->value.asDouble(), 'g', int (fdigits)));
    } else {
        ctx->throwTypeError();
    }
}

//
// Boolean object
//
BooleanCtor::BooleanCtor(Context *scope)
    : FunctionObject(scope)
{
}

void BooleanCtor::construct(Context *ctx)
{
    const double n = ctx->argument(0).toBoolean(ctx);
    ctx->thisObject = Value::fromObject(ctx->engine->newBooleanObject(Value::fromBoolean(n)));
}

void BooleanCtor::call(Context *ctx)
{
    bool value = ctx->argumentCount ? ctx->argument(0).toBoolean(ctx) : 0;
    ctx->result = Value::fromBoolean(value);
}

void BooleanPrototype::init(Context *ctx, const Value &ctor)
{
    ctor.objectValue()->setProperty(ctx, ctx->engine->id_prototype, Value::fromObject(this));
    setProperty(ctx, QStringLiteral("constructor"), ctor);
    setProperty(ctx, QStringLiteral("toString"), method_toString);
    setProperty(ctx, QStringLiteral("valueOf"), method_valueOf);
}

void BooleanPrototype::method_toString(Context *ctx)
{
    if (BooleanObject *thisObject = ctx->thisObject.asBooleanObject()) {
        ctx->result = Value::fromString(ctx, QLatin1String(thisObject->value.booleanValue() ? "true" : "false"));
    } else {
        ctx->throwTypeError();
    }
}

void BooleanPrototype::method_valueOf(Context *ctx)
{
    if (BooleanObject *thisObject = ctx->thisObject.asBooleanObject()) {
        ctx->result = thisObject->value;
    } else {
        ctx->throwTypeError();
    }
}

//
// Array object
//
ArrayCtor::ArrayCtor(Context *scope)
    : FunctionObject(scope)
{
}

void ArrayCtor::construct(Context *ctx)
{
    call(ctx);
    ctx->thisObject = ctx->result;
}

void ArrayCtor::call(Context *ctx)
{
    Array value;
    if (ctx->argumentCount == 1 && ctx->argument(0).isNumber()) {
        double size = ctx->argument(0).asDouble();
        quint32 isize = Value::toUInt32(size);

        if (size != double(isize)) {
            ctx->throwError(QStringLiteral("Invalid array length"));
            return;
        }

        value.resize(isize);
    } else {
        for (unsigned int i = 0; i < ctx->argumentCount; ++i) {
            value.assign(i, ctx->argument(i));
        }
    }

    ctx->result = Value::fromObject(ctx->engine->newArrayObject(value));
}

void ArrayPrototype::init(Context *ctx, const Value &ctor)
{
    ctor.objectValue()->setProperty(ctx, ctx->engine->id_prototype, Value::fromObject(this));
    setProperty(ctx, QStringLiteral("constructor"), ctor);
    setProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    setProperty(ctx, QStringLiteral("toLocalString"), method_toLocaleString, 0);
    setProperty(ctx, QStringLiteral("concat"), method_concat, 1);
    setProperty(ctx, QStringLiteral("join"), method_join, 1);
    setProperty(ctx, QStringLiteral("pop"), method_pop, 0);
    setProperty(ctx, QStringLiteral("push"), method_push, 1);
    setProperty(ctx, QStringLiteral("reverse"), method_reverse, 0);
    setProperty(ctx, QStringLiteral("shift"), method_shift, 0);
    setProperty(ctx, QStringLiteral("slice"), method_slice, 2);
    setProperty(ctx, QStringLiteral("sort"), method_sort, 1);
    setProperty(ctx, QStringLiteral("splice"), method_splice, 2);
    setProperty(ctx, QStringLiteral("unshift"), method_unshift, 1);
    setProperty(ctx, QStringLiteral("indexOf"), method_indexOf, 0);
    setProperty(ctx, QStringLiteral("lastIndexOf"), method_lastIndexOf, 0);
    setProperty(ctx, QStringLiteral("every"), method_every, 0);
    setProperty(ctx, QStringLiteral("some"), method_some, 0);
    setProperty(ctx, QStringLiteral("forEach"), method_forEach, 0);
    setProperty(ctx, QStringLiteral("map"), method_map, 0);
    setProperty(ctx, QStringLiteral("filter"), method_filter, 0);
    setProperty(ctx, QStringLiteral("reduce"), method_reduce, 0);
    setProperty(ctx, QStringLiteral("reduceRight"), method_reduceRight, 0);
}

void ArrayPrototype::method_toString(Context *ctx)
{
    method_join(ctx);
}

void ArrayPrototype::method_toLocaleString(Context *ctx)
{
    method_toString(ctx);
}

void ArrayPrototype::method_concat(Context *ctx)
{
    Array result;

    if (ArrayObject *instance = ctx->thisObject.asArrayObject())
        result = instance->value;
    else {
        QString v = ctx->thisObject.toString(ctx)->toQString();
        result.assign(0, Value::fromString(ctx, v));
    }

    for (uint i = 0; i < ctx->argumentCount; ++i) {
        quint32 k = result.size();
        Value arg = ctx->argument(i);

        if (ArrayObject *elt = arg.asArrayObject())
            result.concat(elt->value);

        else
            result.assign(k, arg);
    }

    ctx->result = Value::fromObject(ctx->engine->newArrayObject(result));
}

void ArrayPrototype::method_join(Context *ctx)
{
    Value arg = ctx->argument(0);

    QString r4;
    if (arg.isUndefined())
        r4 = QStringLiteral(",");
    else
        r4 = arg.toString(ctx)->toQString();

    Value self = ctx->thisObject;
    const Value length = self.property(ctx, ctx->engine->id_length);
    const quint32 r2 = Value::toUInt32(length.isUndefined() ? 0 : length.toNumber(ctx));

    static QSet<Object *> visitedArrayElements;

    if (! r2 || visitedArrayElements.contains(self.objectValue())) {
        ctx->result = Value::fromString(ctx, QString());
        return;
    }

    // avoid infinite recursion
    visitedArrayElements.insert(self.objectValue());

    QString R;

    if (ArrayObject *a = self.asArrayObject()) {
        for (uint i = 0; i < a->value.size(); ++i) {
            if (i)
                R += r4;

            Value e = a->value.at(i);
            if (! (e.isUndefined() || e.isNull()))
                R += e.toString(ctx)->toQString();
        }
    } else {
        //
        // crazy!
        //
        Value r6 = self.property(ctx, ctx->engine->identifier(QStringLiteral("0")));
        if (!(r6.isUndefined() || r6.isNull()))
            R = r6.toString(ctx)->toQString();

        for (quint32 k = 1; k < r2; ++k) {
            R += r4;

            String *name = Value::fromDouble(k).toString(ctx);
            Value r12 = self.property(ctx, name);

            if (! (r12.isUndefined() || r12.isNull()))
                R += r12.toString(ctx)->toQString();
        }
    }

    visitedArrayElements.remove(self.objectValue());
    ctx->result = Value::fromString(ctx, R);
}

void ArrayPrototype::method_pop(Context *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        Value elt = instance->value.pop();
        ctx->result = elt;
    } else {
        Value r1 = self.property(ctx, ctx->engine->id_length);
        quint32 r2 = !r1.isUndefined() ? r1.toUInt32(ctx) : 0;
        if (! r2) {
            self.objectValue()->setProperty(ctx, ctx->engine->id_length, Value::fromDouble(0));
        } else {
            String *r6 = Value::fromDouble(r2 - 1).toString(ctx);
            Value r7 = self.property(ctx, r6);
            self.objectValue()->deleteProperty(ctx, r6, 0);
            self.objectValue()->setProperty(ctx, ctx->engine->id_length, Value::fromDouble(2 - 1));
            ctx->result = r7;
        }
    }
}

void ArrayPrototype::method_push(Context *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        uint pos = instance->value.size();
        for (unsigned int i = 0; i < ctx->argumentCount; ++i) {
            Value val = ctx->argument(i);
            instance->value.assign(pos++, val);
        }
        ctx->result = Value::fromDouble(pos);
    } else {
        Value r1 = self.property(ctx, ctx->engine->id_length);
        quint32 n = !r1.isUndefined() ? r1.toUInt32(ctx) : 0;
        for (unsigned int index = 0; index < ctx->argumentCount; ++index, ++n) {
            Value r3 = ctx->argument(index);
            String *name = Value::fromDouble(n).toString(ctx);
            self.objectValue()->setProperty(ctx, name, r3);
        }
        Value r = Value::fromDouble(n);
        self.objectValue()->setProperty(ctx, ctx->engine->id_length, r);
        ctx->result = r;
    }
}

void ArrayPrototype::method_reverse(Context *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        int lo = 0, hi = instance->value.count() - 1;

        for (; lo < hi; ++lo, --hi) {
            Value tmp = instance->value.at(lo);
            instance->value.assign(lo, instance->value.at(hi));
            instance->value.assign(hi, tmp);
        }
    } else {
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.reverse"));
    }
}

void ArrayPrototype::method_shift(Context *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        ctx->result = instance->value.takeFirst();
    } else {
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.reverse"));
    }
}

void ArrayPrototype::method_slice(Context *ctx)
{
    // ### TODO implement the fast non-generic version of slice.

    Array result;
    Value start = ctx->argument(0);
    Value end = ctx->argument(1);
    Value self = ctx->thisObject;
    Value l = self.property(ctx, ctx->engine->id_length);
    double r2 = !l.isUndefined() ? l.toNumber(ctx) : 0;
    quint32 r3 = Value::toUInt32(r2);
    qint32 r4 = qint32(start.toInteger(ctx));
    quint32 r5 = r4 < 0 ? qMax(quint32(r3 + r4), quint32(0)) : qMin(quint32(r4), r3);
    quint32 k = r5;
    qint32 r7 = end.isUndefined() ? r3 : qint32 (end.toInteger(ctx));
    quint32 r8 = r7 < 0 ? qMax(quint32(r3 + r7), quint32(0)) : qMin(quint32(r7), r3);
    quint32 n = 0;
    for (; k < r8; ++k) {
        String *r11 = Value::fromDouble(k).toString(ctx);
        Value v = self.property(ctx, r11);
        if (! v.isUndefined())
            result.assign(n++, v);
    }
    ctx->result = Value::fromObject(ctx->engine->newArrayObject(result));
}

void ArrayPrototype::method_sort(Context *ctx)
{
    Value self = ctx->thisObject;
    Value comparefn = ctx->argument(0);
    if (ArrayObject *instance = self.asArrayObject()) {
        instance->value.sort(ctx, comparefn);
        ctx->result = ctx->thisObject;
    } else {
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.sort"));
    }
}

void ArrayPrototype::method_splice(Context *ctx)
{
    if (ctx->argumentCount < 2)
        return;

    double start = ctx->argument(0).toInteger(ctx);
    double deleteCount = ctx->argument(1).toInteger(ctx);
    Value a = Value::fromObject(ctx->engine->newArrayObject());
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        QVector<Value> items;
        for (unsigned int i = 2; i < ctx->argumentCount; ++i)
            items << ctx->argument(i);
        ArrayObject *otherInstance = a.asArrayObject();
        assert(otherInstance);
        instance->value.splice(start, deleteCount, items, otherInstance->value);
        ctx->result = a;
    } else {
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.splice"));
    }
}

void ArrayPrototype::method_unshift(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Array.prototype.indexOf"));
}

void ArrayPrototype::method_indexOf(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Array.prototype.indexOf"));
}

void ArrayPrototype::method_lastIndexOf(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Array.prototype.indexOf"));
}

void ArrayPrototype::method_every(Context *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        Value callback = ctx->argument(0);
        if (FunctionObject *f = callback.asFunctionObject()) {
            Value thisArg = ctx->argument(1);
            bool ok = true;
            for (uint k = 0; ok && k < instance->value.size(); ++k) {
                Value v = instance->value.at(k);
                if (v.isUndefined())
                    continue;

                Value args[3];
                args[0] = v;
                args[1] = Value::fromDouble(k);
                args[2] = ctx->thisObject;
                Value r = __qmljs_call_function(ctx, thisArg, f, args, 3);
                ok = __qmljs_to_boolean(r, ctx);
            }
            ctx->result = Value::fromBoolean(ok);
        } else {
            ctx->throwTypeError();
        }
    } else {
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.every"));
    }
}

void ArrayPrototype::method_some(Context *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        Value callback = ctx->argument(0);
        if (FunctionObject *f = callback.asFunctionObject()) {
            Value thisArg = ctx->argument(1);
            bool ok = false;
            for (uint k = 0; !ok && k < instance->value.size(); ++k) {
                Value v = instance->value.at(k);
                if (v.isUndefined())
                    continue;

                Value args[3];
                args[0] = v;
                args[1] = Value::fromDouble(k);
                args[2] = ctx->thisObject;
                Value r = __qmljs_call_function(ctx, thisArg, f, args, 3);
                ok = __qmljs_to_boolean(r, ctx);
            }
            ctx->result = Value::fromBoolean(ok);
        } else {
            ctx->throwTypeError();
        }
    } else {
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.some"));
    }
}

void ArrayPrototype::method_forEach(Context *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        Value callback = ctx->argument(0);
        if (FunctionObject *f = callback.asFunctionObject()) {
            Value thisArg = ctx->argument(1);
            for (quint32 k = 0; k < instance->value.size(); ++k) {
                Value v = instance->value.at(k);
                if (v.isUndefined())
                    continue;
                Value args[3];
                args[0] = v;
                args[1] = Value::fromDouble(k);
                args[2] = ctx->thisObject;
                /*Value r =*/ __qmljs_call_function(ctx, thisArg, f, args, 3);
            }
        } else {
            ctx->throwTypeError();
        }
    } else {
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.forEach"));
    }
}

void ArrayPrototype::method_map(Context *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        Value callback = ctx->argument(0);
        if (FunctionObject *f = callback.asFunctionObject()) {
            Value thisArg = ctx->argument(1);
            ArrayObject *a = ctx->engine->newArrayObject()->asArrayObject();
            a->value.resize(instance->value.size());
            for (quint32 k = 0; k < instance->value.size(); ++k) {
                Value v = instance->value.at(k);
                if (v.isUndefined())
                    continue;
                Value args[3];
                args[0] = v;
                args[1] = Value::fromDouble(k);
                args[2] = ctx->thisObject;
                Value r = __qmljs_call_function(ctx, thisArg, f, args, 3);
                a->value.assign(k, r);
            }
            ctx->result = Value::fromObject(a);
        } else {
            ctx->throwTypeError();
        }
    } else {
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.map"));
    }
}

void ArrayPrototype::method_filter(Context *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        Value callback = ctx->argument(0);
        if (FunctionObject *f = callback.asFunctionObject()) {
            Value thisArg = ctx->argument(1);
            ArrayObject *a = ctx->engine->newArrayObject()->asArrayObject();
            for (quint32 k = 0; k < instance->value.size(); ++k) {
                Value v = instance->value.at(k);
                if (v.isUndefined())
                    continue;
                Value args[3];
                args[0] = v;
                args[1] = Value::fromDouble(k);
                args[2] = ctx->thisObject;
                Value r = __qmljs_call_function(ctx, thisArg, f, args, 3);
                if (__qmljs_to_boolean(r, ctx)) {
                    const uint index = a->value.size();
                    a->value.resize(index + 1);
                    a->value.assign(index, v);
                }
            }
            ctx->result = Value::fromObject(a);
        } else {
            ctx->throwTypeError();
        }
    } else {
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.filter"));
    }
}

void ArrayPrototype::method_reduce(Context *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        Value callback = ctx->argument(0);
        if (FunctionObject *f = callback.asFunctionObject()) {
            Value initialValue = ctx->argument(1);
            Value acc = initialValue;
            for (quint32 k = 0; k < instance->value.size(); ++k) {
                Value v = instance->value.at(k);
                if (v.isUndefined())
                    continue;

                if (acc.isUndefined()) {
                    acc = v;
                    continue;
                }

                Value args[4];
                args[0] = acc;
                args[1] = v;
                args[2] = Value::fromDouble(k);
                args[3] = ctx->thisObject;
                Value r = __qmljs_call_function(ctx, Value::undefinedValue(), f, args, 4);
                acc = r;
            }
            ctx->result = acc;
        } else {
            ctx->throwTypeError();
        }
    } else {
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.reduce"));
    }
}

void ArrayPrototype::method_reduceRight(Context *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        Value callback = ctx->argument(0);
        if (FunctionObject *f = callback.asFunctionObject()) {
            Value initialValue = ctx->argument(1);
            Value acc = initialValue;
            for (int k = instance->value.size() - 1; k != -1; --k) {
                Value v = instance->value.at(k);
                if (v.isUndefined())
                    continue;

                if (acc.isUndefined()) {
                    acc = v;
                    continue;
                }

                Value args[4];
                args[0] = acc;
                args[1] = v;
                args[2] = Value::fromDouble(k);
                args[3] = ctx->thisObject;
                Value r = __qmljs_call_function(ctx, Value::undefinedValue(), f, args, 4);
                acc = r;
            }
            ctx->result = acc;
        } else {
            ctx->throwTypeError();
        }
    } else {
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.reduceRight"));
    }
}

//
// Function object
//
FunctionCtor::FunctionCtor(Context *scope)
    : FunctionObject(scope)
{
}

void FunctionCtor::construct(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Function.prototype.constructor"));
}

void FunctionCtor::call(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Function"));
}

void FunctionPrototype::init(Context *ctx, const Value &ctor)
{
    ctor.objectValue()->setProperty(ctx, ctx->engine->id_prototype, Value::fromObject(this));
    setProperty(ctx, QStringLiteral("constructor"), ctor);
    setProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    setProperty(ctx, QStringLiteral("apply"), method_apply, 0);
    setProperty(ctx, QStringLiteral("call"), method_call, 0);
    setProperty(ctx, QStringLiteral("bind"), method_bind, 0);
}

void FunctionPrototype::method_toString(Context *ctx)
{
    if (FunctionObject *fun = ctx->thisObject.asFunctionObject()) {
        Q_UNUSED(fun);
        ctx->result = Value::fromString(ctx, QStringLiteral("function() { [code] }"));
    } else {
        ctx->throwTypeError();
    }
}

void FunctionPrototype::method_apply(Context *ctx)
{
    if (FunctionObject *f = ctx->thisObject.asFunctionObject()) {

        Value thisObject = ctx->argument(0).toObject(ctx);
        if (thisObject.isNull() || thisObject.isUndefined())
            thisObject = ctx->engine->globalObject;

        Value arg = ctx->argument(1);
        QVector<Value> args;

        if (ArrayObject *arr = arg.asArrayObject()) {
            const Array &actuals = arr->value;

            for (quint32 i = 0; i < actuals.count(); ++i) {
                Value a = actuals.at(i);
                args.append(a);
            }
        } else if (!(arg.isUndefined() || arg.isNull())) {
            ctx->throwError(QLatin1String("Function.prototype.apply: second argument is not an array"));
            return;
        }

        ctx->result = __qmljs_call_function(ctx, thisObject, f, args.data(), args.size());
    } else {
        ctx->throwTypeError();
    }
}

void FunctionPrototype::method_call(Context *ctx)
{
    if (FunctionObject *f = ctx->thisObject.asFunctionObject()) {
        Value thisArg = ctx->argument(0);
        QVector<Value> args(ctx->argumentCount ? ctx->argumentCount - 1 : 0);
        if (ctx->argumentCount)
            qCopy(ctx->arguments + 1, ctx->arguments + ctx->argumentCount, args.begin());
        ctx->result = __qmljs_call_function(ctx, thisArg, f, args.data(), args.size());
    } else {
        ctx->throwTypeError();
    }
}

void FunctionPrototype::method_bind(Context *ctx)
{
    if (FunctionObject *fun = ctx->thisObject.asFunctionObject()) {
        Q_UNUSED(fun);
        ctx->throwUnimplemented(QStringLiteral("Function.prototype.bind"));
    } else {
        ctx->throwTypeError();
    }
}

//
// Date object
//
DateCtor::DateCtor(Context *scope)
    : FunctionObject(scope)
{
}

void DateCtor::construct(Context *ctx)
{
    double t = 0;

    if (ctx->argumentCount == 0)
        t = currentTime();

    else if (ctx->argumentCount == 1) {
        Value arg = ctx->argument(0);
        if (DateObject *d = arg.asDateObject())
            arg = d->value;
        else
            arg = __qmljs_to_primitive(arg, ctx, PREFERREDTYPE_HINT);

        if (arg.isString())
            t = ParseString(arg.toString(ctx)->toQString());
        else
            t = TimeClip(arg.toNumber(ctx));
    }

    else { // ctx->argumentCount() > 1
        double year  = ctx->argument(0).toNumber(ctx);
        double month = ctx->argument(1).toNumber(ctx);
        double day  = ctx->argumentCount >= 3 ? ctx->argument(2).toNumber(ctx) : 1;
        double hours = ctx->argumentCount >= 4 ? ctx->argument(3).toNumber(ctx) : 0;
        double mins = ctx->argumentCount >= 5 ? ctx->argument(4).toNumber(ctx) : 0;
        double secs = ctx->argumentCount >= 6 ? ctx->argument(5).toNumber(ctx) : 0;
        double ms    = ctx->argumentCount >= 7 ? ctx->argument(6).toNumber(ctx) : 0;
        if (year >= 0 && year <= 99)
            year += 1900;
        t = MakeDate(MakeDay(year, month, day), MakeTime(hours, mins, secs, ms));
        t = TimeClip(UTC(t));
    }

    Object *d = ctx->engine->newDateObject(Value::fromDouble(t));
    ctx->thisObject = Value::fromObject(d);
}

void DateCtor::call(Context *ctx)
{
    double t = currentTime();
    ctx->result = Value::fromString(ctx, ToString(t));
}

void DatePrototype::init(Context *ctx, const Value &ctor)
{
    ctor.objectValue()->setProperty(ctx, ctx->engine->id_prototype, Value::fromObject(this));
    LocalTZA = getLocalTZA();

    ctor.objectValue()->setProperty(ctx, QStringLiteral("parse"), method_parse, 1);
    ctor.objectValue()->setProperty(ctx, QStringLiteral("UTC"), method_UTC, 7);

    setProperty(ctx, QStringLiteral("constructor"), ctor);
    setProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    setProperty(ctx, QStringLiteral("toDateString"), method_toDateString, 0);
    setProperty(ctx, QStringLiteral("toTimeString"), method_toTimeString, 0);
    setProperty(ctx, QStringLiteral("toLocaleString"), method_toLocaleString, 0);
    setProperty(ctx, QStringLiteral("toLocaleDateString"), method_toLocaleDateString, 0);
    setProperty(ctx, QStringLiteral("toLocaleTimeString"), method_toLocaleTimeString, 0);
    setProperty(ctx, QStringLiteral("valueOf"), method_valueOf, 0);
    setProperty(ctx, QStringLiteral("getTime"), method_getTime, 0);
    setProperty(ctx, QStringLiteral("getYear"), method_getYear, 0);
    setProperty(ctx, QStringLiteral("getFullYear"), method_getFullYear, 0);
    setProperty(ctx, QStringLiteral("getUTCFullYear"), method_getUTCFullYear, 0);
    setProperty(ctx, QStringLiteral("getMonth"), method_getMonth, 0);
    setProperty(ctx, QStringLiteral("getUTCMonth"), method_getUTCMonth, 0);
    setProperty(ctx, QStringLiteral("getDate"), method_getDate, 0);
    setProperty(ctx, QStringLiteral("getUTCDate"), method_getUTCDate, 0);
    setProperty(ctx, QStringLiteral("getDay"), method_getDay, 0);
    setProperty(ctx, QStringLiteral("getUTCDay"), method_getUTCDay, 0);
    setProperty(ctx, QStringLiteral("getHours"), method_getHours, 0);
    setProperty(ctx, QStringLiteral("getUTCHours"), method_getUTCHours, 0);
    setProperty(ctx, QStringLiteral("getMinutes"), method_getMinutes, 0);
    setProperty(ctx, QStringLiteral("getUTCMinutes"), method_getUTCMinutes, 0);
    setProperty(ctx, QStringLiteral("getSeconds"), method_getSeconds, 0);
    setProperty(ctx, QStringLiteral("getUTCSeconds"), method_getUTCSeconds, 0);
    setProperty(ctx, QStringLiteral("getMilliseconds"), method_getMilliseconds, 0);
    setProperty(ctx, QStringLiteral("getUTCMilliseconds"), method_getUTCMilliseconds, 0);
    setProperty(ctx, QStringLiteral("getTimezoneOffset"), method_getTimezoneOffset, 0);
    setProperty(ctx, QStringLiteral("setTime"), method_setTime, 1);
    setProperty(ctx, QStringLiteral("setMilliseconds"), method_setMilliseconds, 1);
    setProperty(ctx, QStringLiteral("setUTCMilliseconds"), method_setUTCMilliseconds, 1);
    setProperty(ctx, QStringLiteral("setSeconds"), method_setSeconds, 2);
    setProperty(ctx, QStringLiteral("setUTCSeconds"), method_setUTCSeconds, 2);
    setProperty(ctx, QStringLiteral("setMinutes"), method_setMinutes, 3);
    setProperty(ctx, QStringLiteral("setUTCMinutes"), method_setUTCMinutes, 3);
    setProperty(ctx, QStringLiteral("setHours"), method_setHours, 4);
    setProperty(ctx, QStringLiteral("setUTCHours"), method_setUTCHours, 4);
    setProperty(ctx, QStringLiteral("setDate"), method_setDate, 1);
    setProperty(ctx, QStringLiteral("setUTCDate"), method_setUTCDate, 1);
    setProperty(ctx, QStringLiteral("setMonth"), method_setMonth, 2);
    setProperty(ctx, QStringLiteral("setUTCMonth"), method_setUTCMonth, 2);
    setProperty(ctx, QStringLiteral("setYear"), method_setYear, 1);
    setProperty(ctx, QStringLiteral("setFullYear"), method_setFullYear, 3);
    setProperty(ctx, QStringLiteral("setUTCFullYear"), method_setUTCFullYear, 3);
    setProperty(ctx, QStringLiteral("toUTCString"), method_toUTCString, 0);
    setProperty(ctx, QStringLiteral("toGMTString"), method_toUTCString, 0);
}

double DatePrototype::getThisDate(Context *ctx)
{
    if (DateObject *thisObject = ctx->thisObject.asDateObject())
        return thisObject->value.asDouble();
    else {
        ctx->throwTypeError();
        return 0;
    }
}

void DatePrototype::method_MakeTime(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Data.MakeTime"));
}

void DatePrototype::method_MakeDate(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Data.MakeDate"));
}

void DatePrototype::method_TimeClip(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("Data.TimeClip"));
}

void DatePrototype::method_parse(Context *ctx)
{
    ctx->result = Value::fromDouble(ParseString(ctx->argument(0).toString(ctx)->toQString()));
}

void DatePrototype::method_UTC(Context *ctx)
{
    const int numArgs = ctx->argumentCount;
    if (numArgs >= 2) {
        double year  = ctx->argument(0).toNumber(ctx);
        double month = ctx->argument(1).toNumber(ctx);
        double day   = numArgs >= 3 ? ctx->argument(2).toNumber(ctx) : 1;
        double hours = numArgs >= 4 ? ctx->argument(3).toNumber(ctx) : 0;
        double mins  = numArgs >= 5 ? ctx->argument(4).toNumber(ctx) : 0;
        double secs  = numArgs >= 6 ? ctx->argument(5).toNumber(ctx) : 0;
        double ms    = numArgs >= 7 ? ctx->argument(6).toNumber(ctx) : 0;
        if (year >= 0 && year <= 99)
            year += 1900;
        double t = MakeDate(MakeDay(year, month, day),
                            MakeTime(hours, mins, secs, ms));
        ctx->result = Value::fromDouble(TimeClip(t));
    }
}

void DatePrototype::method_toString(Context *ctx)
{
    double t = getThisDate(ctx);
    ctx->result = Value::fromString(ctx, ToString(t));
}

void DatePrototype::method_toDateString(Context *ctx)
{
    double t = getThisDate(ctx);
    ctx->result = Value::fromString(ctx, ToDateString(t));
}

void DatePrototype::method_toTimeString(Context *ctx)
{
    double t = getThisDate(ctx);
    ctx->result = Value::fromString(ctx, ToTimeString(t));
}

void DatePrototype::method_toLocaleString(Context *ctx)
{
    double t = getThisDate(ctx);
    ctx->result = Value::fromString(ctx, ToLocaleString(t));
}

void DatePrototype::method_toLocaleDateString(Context *ctx)
{
    double t = getThisDate(ctx);
    ctx->result = Value::fromString(ctx, ToLocaleDateString(t));
}

void DatePrototype::method_toLocaleTimeString(Context *ctx)
{
    double t = getThisDate(ctx);
    ctx->result = Value::fromString(ctx, ToLocaleTimeString(t));
}

void DatePrototype::method_valueOf(Context *ctx)
{
    double t = getThisDate(ctx);
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getTime(Context *ctx)
{
    double t = getThisDate(ctx);
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getYear(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = YearFromTime(LocalTime(t)) - 1900;
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getFullYear(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = YearFromTime(LocalTime(t));
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getUTCFullYear(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = YearFromTime(t);
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getMonth(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = MonthFromTime(LocalTime(t));
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getUTCMonth(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = MonthFromTime(t);
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getDate(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = DateFromTime(LocalTime(t));
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getUTCDate(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = DateFromTime(t);
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getDay(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = WeekDay(LocalTime(t));
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getUTCDay(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = WeekDay(t);
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getHours(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = HourFromTime(LocalTime(t));
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getUTCHours(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = HourFromTime(t);
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getMinutes(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = MinFromTime(LocalTime(t));
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getUTCMinutes(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = MinFromTime(t);
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getSeconds(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = SecFromTime(LocalTime(t));
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getUTCSeconds(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = SecFromTime(t);
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getMilliseconds(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = msFromTime(LocalTime(t));
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getUTCMilliseconds(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = msFromTime(t);
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_getTimezoneOffset(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = (t - LocalTime(t)) / msPerMinute;
    ctx->result = Value::fromDouble(t);
}

void DatePrototype::method_setTime(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        self->value.setDouble(TimeClip(ctx->argument(0).toNumber(ctx)));
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setMilliseconds(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.asDouble());
        double ms = ctx->argument(0).toNumber(ctx);
        self->value.setDouble(TimeClip(UTC(MakeDate(Day(t), MakeTime(HourFromTime(t), MinFromTime(t), SecFromTime(t), ms)))));
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setUTCMilliseconds(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.asDouble();
        double ms = ctx->argument(0).toNumber(ctx);
        self->value.setDouble(TimeClip(UTC(MakeDate(Day(t), MakeTime(HourFromTime(t), MinFromTime(t), SecFromTime(t), ms)))));
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setSeconds(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.asDouble());
        double sec = ctx->argument(0).toNumber(ctx);
        double ms = (ctx->argumentCount < 2) ? msFromTime(t) : ctx->argument(1).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(Day(t), MakeTime(HourFromTime(t), MinFromTime(t), sec, ms))));
        self->value.setDouble(t);
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setUTCSeconds(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.asDouble();
        double sec = ctx->argument(0).toNumber(ctx);
        double ms = (ctx->argumentCount < 2) ? msFromTime(t) : ctx->argument(1).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(Day(t), MakeTime(HourFromTime(t), MinFromTime(t), sec, ms))));
        self->value.setDouble(t);
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setMinutes(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.asDouble());
        double min = ctx->argument(0).toNumber(ctx);
        double sec = (ctx->argumentCount < 2) ? SecFromTime(t) : ctx->argument(1).toNumber(ctx);
        double ms = (ctx->argumentCount < 3) ? msFromTime(t) : ctx->argument(2).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(Day(t), MakeTime(HourFromTime(t), min, sec, ms))));
        self->value.setDouble(t);
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setUTCMinutes(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.asDouble();
        double min = ctx->argument(0).toNumber(ctx);
        double sec = (ctx->argumentCount < 2) ? SecFromTime(t) : ctx->argument(1).toNumber(ctx);
        double ms = (ctx->argumentCount < 3) ? msFromTime(t) : ctx->argument(2).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(Day(t), MakeTime(HourFromTime(t), min, sec, ms))));
        self->value.setDouble(t);
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setHours(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.asDouble());
        double hour = ctx->argument(0).toNumber(ctx);
        double min = (ctx->argumentCount < 2) ? MinFromTime(t) : ctx->argument(1).toNumber(ctx);
        double sec = (ctx->argumentCount < 3) ? SecFromTime(t) : ctx->argument(2).toNumber(ctx);
        double ms = (ctx->argumentCount < 4) ? msFromTime(t) : ctx->argument(3).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(Day(t), MakeTime(hour, min, sec, ms))));
        self->value.setDouble(t);
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setUTCHours(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.asDouble();
        double hour = ctx->argument(0).toNumber(ctx);
        double min = (ctx->argumentCount < 2) ? MinFromTime(t) : ctx->argument(1).toNumber(ctx);
        double sec = (ctx->argumentCount < 3) ? SecFromTime(t) : ctx->argument(2).toNumber(ctx);
        double ms = (ctx->argumentCount < 4) ? msFromTime(t) : ctx->argument(3).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(Day(t), MakeTime(hour, min, sec, ms))));
        self->value.setDouble(t);
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setDate(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.asDouble());
        double date = ctx->argument(0).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(MakeDay(YearFromTime(t), MonthFromTime(t), date), TimeWithinDay(t))));
        self->value.setDouble(t);
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setUTCDate(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.asDouble();
        double date = ctx->argument(0).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(MakeDay(YearFromTime(t), MonthFromTime(t), date), TimeWithinDay(t))));
        self->value.setDouble(t);
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setMonth(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.asDouble());
        double month = ctx->argument(0).toNumber(ctx);
        double date = (ctx->argumentCount < 2) ? DateFromTime(t) : ctx->argument(1).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(MakeDay(YearFromTime(t), month, date), TimeWithinDay(t))));
        self->value.setDouble(t);
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setUTCMonth(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.asDouble();
        double month = ctx->argument(0).toNumber(ctx);
        double date = (ctx->argumentCount < 2) ? DateFromTime(t) : ctx->argument(1).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(MakeDay(YearFromTime(t), month, date), TimeWithinDay(t))));
        self->value.setDouble(t);
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setYear(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.asDouble();
        if (qIsNaN(t))
            t = 0;
        else
            t = LocalTime(t);
        double year = ctx->argument(0).toNumber(ctx);
        double r;
        if (qIsNaN(year)) {
            r = qSNaN();
        } else {
            if ((Value::toInteger(year) >= 0) && (Value::toInteger(year) <= 99))
                year += 1900;
            r = MakeDay(year, MonthFromTime(t), DateFromTime(t));
            r = UTC(MakeDate(r, TimeWithinDay(t)));
            r = TimeClip(r);
        }
        self->value.setDouble(r);
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setUTCFullYear(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.asDouble();
        double year = ctx->argument(0).toNumber(ctx);
        double month = (ctx->argumentCount < 2) ? MonthFromTime(t) : ctx->argument(1).toNumber(ctx);
        double date = (ctx->argumentCount < 3) ? DateFromTime(t) : ctx->argument(2).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(MakeDay(year, month, date), TimeWithinDay(t))));
        self->value.setDouble(t);
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_setFullYear(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.asDouble());
        double year = ctx->argument(0).toNumber(ctx);
        double month = (ctx->argumentCount < 2) ? MonthFromTime(t) : ctx->argument(1).toNumber(ctx);
        double date = (ctx->argumentCount < 3) ? DateFromTime(t) : ctx->argument(2).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(MakeDay(year, month, date), TimeWithinDay(t))));
        self->value.setDouble(t);
        ctx->result = self->value;
    } else {
        ctx->throwTypeError();
    }
}

void DatePrototype::method_toUTCString(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.asDouble();
        ctx->result = Value::fromString(ctx, ToUTCString(t));
    }
}

//
// RegExp object
//
RegExpCtor::RegExpCtor(Context *scope)
    : FunctionObject(scope)
{
}

void RegExpCtor::construct(Context *ctx)
{
    ctx->thisObject = Value::fromObject(ctx->engine->newStringObject(Value::undefinedValue()));
}

void RegExpCtor::call(Context *ctx)
{
    ctx->result = Value::fromObject(ctx->engine->newRegExpObject(Value::undefinedValue()));
}

void RegExpPrototype::init(Context *ctx, const Value &ctor)
{
    ctor.objectValue()->setProperty(ctx, ctx->engine->id_prototype, Value::fromObject(this));
    setProperty(ctx, QStringLiteral("constructor"), ctor);
    setProperty(ctx, QStringLiteral("exec"), method_exec, 0);
    setProperty(ctx, QStringLiteral("test"), method_test, 0);
    setProperty(ctx, QStringLiteral("toString"), method_toString, 0);
}

void RegExpPrototype::method_exec(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("RegExp.prototype.exec"));
}

void RegExpPrototype::method_test(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("RegExp.prototype.test"));
}

void RegExpPrototype::method_toString(Context *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("RegExp.prototype.toString"));
}

//
// Math object
//
MathObject::MathObject(Context *ctx)
{
    setProperty(ctx, QStringLiteral("E"), Value::fromDouble(::exp(1.0)));
    setProperty(ctx, QStringLiteral("LN2"), Value::fromDouble(::log(2.0)));
    setProperty(ctx, QStringLiteral("LN10"), Value::fromDouble(::log(10.0)));
    setProperty(ctx, QStringLiteral("LOG2E"), Value::fromDouble(1.0/::log(2.0)));
    setProperty(ctx, QStringLiteral("LOG10E"), Value::fromDouble(1.0/::log(10.0)));
    setProperty(ctx, QStringLiteral("PI"), Value::fromDouble(qt_PI));
    setProperty(ctx, QStringLiteral("SQRT1_2"), Value::fromDouble(::sqrt(0.5)));
    setProperty(ctx, QStringLiteral("SQRT2"), Value::fromDouble(::sqrt(2.0)));

    setProperty(ctx, QStringLiteral("abs"), method_abs, 1);
    setProperty(ctx, QStringLiteral("acos"), method_acos, 1);
    setProperty(ctx, QStringLiteral("asin"), method_asin, 0);
    setProperty(ctx, QStringLiteral("atan"), method_atan, 1);
    setProperty(ctx, QStringLiteral("atan2"), method_atan2, 2);
    setProperty(ctx, QStringLiteral("ceil"), method_ceil, 1);
    setProperty(ctx, QStringLiteral("cos"), method_cos, 1);
    setProperty(ctx, QStringLiteral("exp"), method_exp, 1);
    setProperty(ctx, QStringLiteral("floor"), method_floor, 1);
    setProperty(ctx, QStringLiteral("log"), method_log, 1);
    setProperty(ctx, QStringLiteral("max"), method_max, 2);
    setProperty(ctx, QStringLiteral("min"), method_min, 2);
    setProperty(ctx, QStringLiteral("pow"), method_pow, 2);
    setProperty(ctx, QStringLiteral("random"), method_random, 0);
    setProperty(ctx, QStringLiteral("round"), method_round, 1);
    setProperty(ctx, QStringLiteral("sin"), method_sin, 1);
    setProperty(ctx, QStringLiteral("sqrt"), method_sqrt, 1);
    setProperty(ctx, QStringLiteral("tan"), method_tan, 1);
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
        ctx->result = Value::fromDouble(0);
    else
        ctx->result = Value::fromDouble(v < 0 ? -v : v);
}

void MathObject::method_acos(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v > 1)
        ctx->result = Value::fromDouble(qSNaN());
    else
        ctx->result = Value::fromDouble(::acos(v));
}

void MathObject::method_asin(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v > 1)
        ctx->result = Value::fromDouble(qSNaN());
    else
        ctx->result = Value::fromDouble(::asin(v));
}

void MathObject::method_atan(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v == 0.0)
        ctx->result = Value::fromDouble(v);
    else
        ctx->result = Value::fromDouble(::atan(v));
}

void MathObject::method_atan2(Context *ctx)
{
    double v1 = ctx->argument(0).toNumber(ctx);
    double v2 = ctx->argument(1).toNumber(ctx);
    if ((v1 < 0) && qIsFinite(v1) && qIsInf(v2) && (copySign(1.0, v2) == 1.0)) {
        ctx->result = Value::fromDouble(copySign(0, -1.0));
        return;
    }
    if ((v1 == 0.0) && (v2 == 0.0)) {
        if ((copySign(1.0, v1) == 1.0) && (copySign(1.0, v2) == -1.0)) {
            ctx->result = Value::fromDouble(qt_PI);
            return;
        } else if ((copySign(1.0, v1) == -1.0) && (copySign(1.0, v2) == -1.0)) {
            ctx->result = Value::fromDouble(-qt_PI);
            return;
        }
    }
    ctx->result = Value::fromDouble(::atan2(v1, v2));
}

void MathObject::method_ceil(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v < 0.0 && v > -1.0)
        ctx->result = Value::fromDouble(copySign(0, -1.0));
    else
        ctx->result = Value::fromDouble(::ceil(v));
}

void MathObject::method_cos(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    ctx->result = Value::fromDouble(::cos(v));
}

void MathObject::method_exp(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (qIsInf(v)) {
        if (copySign(1.0, v) == -1.0)
            ctx->result = Value::fromDouble(0);
        else
            ctx->result = Value::fromDouble(qInf());
    } else {
        ctx->result = Value::fromDouble(::exp(v));
    }
}

void MathObject::method_floor(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    ctx->result = Value::fromDouble(::floor(v));
}

void MathObject::method_log(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v < 0)
        ctx->result = Value::fromDouble(qSNaN());
    else
        ctx->result = Value::fromDouble(::log(v));
}

void MathObject::method_max(Context *ctx)
{
    double mx = -qInf();
    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        double x = ctx->argument(i).toNumber(ctx);
        if (x > mx || qIsNaN(x))
            mx = x;
    }
    ctx->result = Value::fromDouble(mx);
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
    ctx->result = Value::fromDouble(mx);
}

void MathObject::method_pow(Context *ctx)
{
    double x = ctx->argument(0).toNumber(ctx);
    double y = ctx->argument(1).toNumber(ctx);

    if (qIsNaN(y)) {
        ctx->result = Value::fromDouble(qSNaN());
        return;
    }

    if (y == 0) {
        ctx->result = Value::fromDouble(1);
    } else if (((x == 1) || (x == -1)) && qIsInf(y)) {
        ctx->result = Value::fromDouble(qSNaN());
    } else if (((x == 0) && copySign(1.0, x) == 1.0) && (y < 0)) {
        ctx->result = Value::fromDouble(qInf());
    } else if ((x == 0) && copySign(1.0, x) == -1.0) {
        if (y < 0) {
            if (::fmod(-y, 2.0) == 1.0)
                ctx->result = Value::fromDouble(-qInf());
            else
                ctx->result = Value::fromDouble(qInf());
        } else if (y > 0) {
            if (::fmod(y, 2.0) == 1.0)
                ctx->result = Value::fromDouble(copySign(0, -1.0));
            else
                ctx->result = Value::fromDouble(0);
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
        ctx->result = Value::fromDouble(::pow(x, y));
    }
}

void MathObject::method_random(Context *ctx)
{
    ctx->result = Value::fromDouble(qrand() / (double) RAND_MAX);
}

void MathObject::method_round(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    v = copySign(::floor(v + 0.5), v);
    ctx->result = Value::fromDouble(v);
}

void MathObject::method_sin(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    ctx->result = Value::fromDouble(::sin(v));
}

void MathObject::method_sqrt(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    ctx->result = Value::fromDouble(::sqrt(v));
}

void MathObject::method_tan(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v == 0.0)
        ctx->result = Value::fromDouble(v);
    else
        ctx->result = Value::fromDouble(::tan(v));
}
