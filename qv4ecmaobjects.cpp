
#include "qv4ecmaobjects_p.h"
#include "qv4array_p.h"
#include <QtCore/qnumeric.h>
#include <QtCore/qmath.h>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QDebug>
#include <math.h>
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

    Q_ASSERT (x == 366);
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
        formats << QLatin1String("M/d/yyyy")
                << QLatin1String("M/d/yyyy hh:mm")
                << QLatin1String("M/d/yyyy hh:mm A")

                << QLatin1String("M/d/yyyy, hh:mm")
                << QLatin1String("M/d/yyyy, hh:mm A")

                << QLatin1String("MMM d yyyy")
                << QLatin1String("MMM d yyyy hh:mm")
                << QLatin1String("MMM d yyyy hh:mm:ss")
                << QLatin1String("MMM d yyyy, hh:mm")
                << QLatin1String("MMM d yyyy, hh:mm:ss")

                << QLatin1String("MMMM d yyyy")
                << QLatin1String("MMMM d yyyy hh:mm")
                << QLatin1String("MMMM d yyyy hh:mm:ss")
                << QLatin1String("MMMM d yyyy, hh:mm")
                << QLatin1String("MMMM d yyyy, hh:mm:ss")

                << QLatin1String("MMM d, yyyy")
                << QLatin1String("MMM d, yyyy hh:mm")
                << QLatin1String("MMM d, yyyy hh:mm:ss")

                << QLatin1String("MMMM d, yyyy")
                << QLatin1String("MMMM d, yyyy hh:mm")
                << QLatin1String("MMMM d, yyyy hh:mm:ss")

                << QLatin1String("d MMM yyyy")
                << QLatin1String("d MMM yyyy hh:mm")
                << QLatin1String("d MMM yyyy hh:mm:ss")
                << QLatin1String("d MMM yyyy, hh:mm")
                << QLatin1String("d MMM yyyy, hh:mm:ss")

                << QLatin1String("d MMMM yyyy")
                << QLatin1String("d MMMM yyyy hh:mm")
                << QLatin1String("d MMMM yyyy hh:mm:ss")
                << QLatin1String("d MMMM yyyy, hh:mm")
                << QLatin1String("d MMMM yyyy, hh:mm:ss")

                << QLatin1String("d MMM, yyyy")
                << QLatin1String("d MMM, yyyy hh:mm")
                << QLatin1String("d MMM, yyyy hh:mm:ss")

                << QLatin1String("d MMMM, yyyy")
                << QLatin1String("d MMMM, yyyy hh:mm")
                << QLatin1String("d MMMM, yyyy hh:mm:ss");

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
        return QLatin1String("Invalid Date");
    QString str = ToDateTime(t, Qt::LocalTime).toString() + QLatin1String(" GMT");
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
        return QLatin1String("Invalid Date");
    return ToDateTime(t, Qt::UTC).toString() + QLatin1String(" GMT");
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
Value ObjectCtor::create(ExecutionEngine *engine)
{
    Context *ctx = engine->rootContext;
    FunctionObject *ctor = ctx->engine->newObjectCtor(ctx);
    ctor->setProperty(ctx, QLatin1String("prototype"), Value::fromObject(ctx->engine->newObjectPrototype(ctx, ctor)));
    return Value::fromObject(ctor);
}

ObjectCtor::ObjectCtor(Context *scope)
    : FunctionObject(scope)
{
}

void ObjectCtor::construct(Context *ctx)
{
    __qmljs_init_object(&ctx->thisObject, ctx->engine->newObject());
}

void ObjectCtor::call(Context *)
{
    assert(!"not here");
}

ObjectPrototype::ObjectPrototype(Context *ctx, FunctionObject *ctor)
{
    setProperty(ctx, QLatin1String("constructor"), Value::fromObject(ctor));
    setProperty(ctx, QLatin1String("toString"), method_toString, 0);
}

void ObjectPrototype::method_toString(Context *ctx)
{
    ctx->result = Value::fromString(ctx, "object");
}

//
// String
//
Value StringCtor::create(ExecutionEngine *engine)
{
    Context *ctx = engine->rootContext;
    FunctionObject *ctor = ctx->engine->newStringCtor(ctx);
    ctor->setProperty(ctx, QLatin1String("prototype"), Value::fromObject(ctx->engine->newStringPrototype(ctx, ctor)));
    return Value::fromObject(ctor);
}

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
    __qmljs_init_object(&ctx->thisObject, ctx->engine->newStringObject(value));
}

void StringCtor::call(Context *ctx)
{
    const Value arg = ctx->argument(0);
    if (arg.is(UNDEFINED_TYPE))
        __qmljs_init_string(&ctx->result, ctx->engine->newString(QString()));
    else
        __qmljs_to_string(ctx, &ctx->result, &arg);
}

StringPrototype::StringPrototype(Context *ctx, FunctionObject *ctor)
{
    setProperty(ctx, QLatin1String("constructor"), Value::fromObject(ctor));
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
    if (StringObject *thisObject = ctx->thisObject.asStringObject()) {
        return thisObject->value.stringValue->toQString();
    } else {
        assert(!"type error");
        return QString();
    }
}

void StringPrototype::method_toString(Context *ctx)
{
    if (StringObject *o = ctx->thisObject.asStringObject()) {
        ctx->result = o->value;
    } else {
        assert(!"type error");
    }
}

void StringPrototype::method_valueOf(Context *ctx)
{
    if (StringObject *o = ctx->thisObject.asStringObject()) {
        ctx->result = o->value;
    } else {
        assert(!"type error");
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

    __qmljs_init_number(&ctx->result, result);
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

    __qmljs_init_number(&ctx->result, index);
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
    __qmljs_init_number(&ctx->result, index);
}

void StringPrototype::method_localeCompare(Context *ctx)
{
    const QString value = getThisString(ctx);
    const QString that = ctx->argument(0).toString(ctx)->toQString();
    __qmljs_init_number(&ctx->result, QString::localeAwareCompare(value, that));
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
    ctx->result = Value::fromString(ctx, text.mid(start, count));
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
Value NumberCtor::create(ExecutionEngine *engine)
{
    Context *ctx = engine->rootContext;
    FunctionObject *ctor = ctx->engine->newNumberCtor(ctx);
    ctor->setProperty(ctx, QLatin1String("prototype"), Value::fromObject(ctx->engine->newNumberPrototype(ctx, ctor)));
    return Value::fromObject(ctor);
}

NumberCtor::NumberCtor(Context *scope)
    : FunctionObject(scope)
{
}

void NumberCtor::construct(Context *ctx)
{
    const double n = ctx->argument(0).toNumber(ctx);
    __qmljs_init_object(&ctx->thisObject, ctx->engine->newNumberObject(Value::fromNumber(n)));
}

void NumberCtor::call(Context *ctx)
{
    double value = ctx->argumentCount ? ctx->argument(0).toNumber(ctx) : 0;
    __qmljs_init_number(&ctx->result, value);
}

NumberPrototype::NumberPrototype(Context *ctx, FunctionObject *ctor)
    : NumberObject(Value::fromNumber(0))
{
    ctor->setProperty(ctx, QLatin1String("NaN"), Value::fromNumber(qSNaN()));
    ctor->setProperty(ctx, QLatin1String("NEGATIVE_INFINITY"), Value::fromNumber(-qInf()));
    ctor->setProperty(ctx, QLatin1String("POSITIVE_INFINITY"), Value::fromNumber(qInf()));
    ctor->setProperty(ctx, QLatin1String("MAX_VALUE"), Value::fromNumber(1.7976931348623158e+308));
#ifdef __INTEL_COMPILER
# pragma warning( push )
# pragma warning(disable: 239)
#endif
    ctor->setProperty(ctx, QLatin1String("MIN_VALUE"), Value::fromNumber(5e-324));
#ifdef __INTEL_COMPILER
# pragma warning( pop )
#endif

    setProperty(ctx, QLatin1String("constructor"), Value::fromObject(ctor));
    setProperty(ctx, QLatin1String("toString"), method_toString);
    setProperty(ctx, QLatin1String("toLocalString"), method_toLocaleString);
    setProperty(ctx, QLatin1String("valueOf"), method_valueOf);
    setProperty(ctx, QLatin1String("toFixed"), method_toFixed);
    setProperty(ctx, QLatin1String("toExponential"), method_toExponential);
    setProperty(ctx, QLatin1String("toPrecision"), method_toPrecision);
}

void NumberPrototype::method_toString(Context *ctx)
{
    assert(!"here");
    if (NumberObject *thisObject = ctx->thisObject.asNumberObject()) {
        Value arg = ctx->argument(0);
        if (!arg.isUndefined()) {
            int radix = arg.toInt32(ctx);
            if (radix < 2 || radix > 36) {
//                return ctx->throwError(QString::fromLatin1("Number.prototype.toString: %0 is not a valid radix")
//                                       .arg(radix));
                assert(!"not a valid redix");
                return;
            }

            double num = thisObject->value.numberValue;
            if (qIsNaN(num)) {
                ctx->result = Value::fromString(ctx, QLatin1String("NaN"));
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
        assert(!"type error");
    }
}

void NumberPrototype::method_toLocaleString(Context *ctx)
{
    if (NumberObject *thisObject = ctx->thisObject.asNumberObject()) {
        String *str = thisObject->value.toString(ctx);
        ctx->result = Value::fromString(str);
    } else {
        assert(!"type error");
    }
}

void NumberPrototype::method_valueOf(Context *ctx)
{
    if (NumberObject *thisObject = ctx->thisObject.asNumberObject()) {
        ctx->result = thisObject->value;
    } else {
        assert(!"type error");
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

        double v = thisObject->value.numberValue;
        QString str;
        if (qIsNaN(v))
            str = QString::fromLatin1("NaN");
        else if (qIsInf(v))
            str = QString::fromLatin1(v < 0 ? "-Infinity" : "Infinity");
        else
            str = QString::number(v, 'f', int (fdigits));
        ctx->result = Value::fromString(ctx, str);
    } else {
        assert(!"type error");
    }
}

void NumberPrototype::method_toExponential(Context *ctx)
{
    if (NumberObject *thisObject = ctx->thisObject.asNumberObject()) {
        double fdigits = 0;

        if (ctx->argumentCount > 0)
            fdigits = ctx->argument(0).toInteger(ctx);

        QString z = QString::number(thisObject->value.numberValue, 'e', int (fdigits));
        ctx->result = Value::fromString(ctx, z);
    } else {
        assert(!"type error");
    }
}

void NumberPrototype::method_toPrecision(Context *ctx)
{
    if (NumberObject *thisObject = ctx->thisObject.asNumberObject()) {
        double fdigits = 0;

        if (ctx->argumentCount > 0)
            fdigits = ctx->argument(0).toInteger(ctx);

        ctx->result = Value::fromString(ctx, QString::number(thisObject->value.numberValue, 'g', int (fdigits)));
    } else {
        assert(!"type error");
    }
}

//
// Boolean object
//
Value BooleanCtor::create(ExecutionEngine *engine)
{
    Context *ctx = engine->rootContext;
    FunctionObject *ctor = ctx->engine->newBooleanCtor(ctx);
    ctor->setProperty(ctx, QLatin1String("prototype"), Value::fromObject(ctx->engine->newBooleanPrototype(ctx, ctor)));
    return Value::fromObject(ctor);
}

BooleanCtor::BooleanCtor(Context *scope)
    : FunctionObject(scope)
{
}

void BooleanCtor::construct(Context *ctx)
{
    const double n = ctx->argument(0).toBoolean(ctx);
    __qmljs_init_object(&ctx->thisObject, ctx->engine->newBooleanObject(Value::fromBoolean(n)));
}

void BooleanCtor::call(Context *ctx)
{
    double value = ctx->argumentCount ? ctx->argument(0).toBoolean(ctx) : 0;
    __qmljs_init_boolean(&ctx->result, value);
}

BooleanPrototype::BooleanPrototype(Context *ctx, FunctionObject *ctor)
    : BooleanObject(Value::fromBoolean(false))
{
    ctor->setProperty(ctx, QLatin1String("constructor"), Value::fromObject(ctor));
    ctor->setProperty(ctx, QLatin1String("toString"), method_toString);
    ctor->setProperty(ctx, QLatin1String("valueOf"), method_valueOf);
}

void BooleanPrototype::method_toString(Context *ctx)
{
    if (BooleanObject *thisObject = ctx->thisObject.asBooleanObject()) {
        ctx->result = Value::fromString(ctx, QLatin1String(thisObject->value.booleanValue ? "true" : "false"));
    } else {
        assert(!"type error");
    }
}

void BooleanPrototype::method_valueOf(Context *ctx)
{
    if (BooleanObject *thisObject = ctx->thisObject.asBooleanObject()) {
        ctx->result = thisObject->value;
    } else {
        assert(!"type error");
    }
}

//
// Array object
//
//
// Number object
//
Value ArrayCtor::create(ExecutionEngine *engine)
{
    Context *ctx = engine->rootContext;
    FunctionObject *ctor = ctx->engine->newArrayCtor(ctx);
    ctor->setProperty(ctx, QLatin1String("prototype"), Value::fromObject(ctx->engine->newArrayPrototype(ctx, ctor)));
    return Value::fromObject(ctor);
}

ArrayCtor::ArrayCtor(Context *scope)
    : FunctionObject(scope)
{
}

void ArrayCtor::construct(Context *ctx)
{
    ctx->thisObject = Value::fromObject(ctx->engine->newArrayObject());
}

void ArrayCtor::call(Context *ctx)
{
    ctx->result = Value::fromObject(ctx->engine->newArrayObject());
}

ArrayPrototype::ArrayPrototype(Context *ctx, FunctionObject *ctor)
{
    setProperty(ctx, QLatin1String("constructor"), Value::fromObject(ctor));
    setProperty(ctx, QLatin1String("toString"), method_toString, 0);
    setProperty(ctx, QLatin1String("toLocalString"), method_toLocaleString, 0);
    setProperty(ctx, QLatin1String("concat"), method_concat, 1);
    setProperty(ctx, QLatin1String("join"), method_join, 1);
    setProperty(ctx, QLatin1String("pop"), method_pop, 0);
    setProperty(ctx, QLatin1String("push"), method_push, 1);
    setProperty(ctx, QLatin1String("reverse"), method_reverse, 0);
    setProperty(ctx, QLatin1String("shift"), method_shift, 0);
    setProperty(ctx, QLatin1String("slice"), method_slice, 2);
    setProperty(ctx, QLatin1String("sort"), method_sort, 1);
    setProperty(ctx, QLatin1String("splice"), method_splice, 2);
    setProperty(ctx, QLatin1String("unshift"), method_unshift, 1);
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
            result.assign(k, Value::fromString(arg.toString(ctx)));
    }

    ctx->result = Value::fromObject(ctx->engine->newArrayObject(result));
}

void ArrayPrototype::method_join(Context *ctx)
{
    Value arg = ctx->argument(0);

    QString r4;
    if (arg.isUndefined())
        r4 = QLatin1String(",");
    else
        r4 = arg.toString(ctx)->toQString();

    Value self = ctx->thisObject;

    Value *length = self.objectValue->getProperty(ctx->engine->identifier("length"));
    double r1 = length ? length->toNumber(ctx) : 0;
    quint32 r2 = Value::toUInt32(r1);

    static QSet<Object *> visitedArrayElements;

    if (! r2 || visitedArrayElements.contains(self.objectValue)) {
        ctx->result = Value::fromString(ctx, QString());
        return;
    }

    // avoid infinite recursion
    visitedArrayElements.insert(self.objectValue);

    QString R;

    if (ArrayObject *a = self.objectValue->asArrayObject()) {
        for (uint i = 0; i < a->value.size(); ++i) {
            if (! R.isEmpty())
                R += r4;

            Value e = a->value.at(i);
            if (! (e.isUndefined() || e.isNull()))
                R += e.toString(ctx)->toQString();
        }
    } else {
        //
        // crazy!
        //
        Value *r6 = self.objectValue->getProperty(ctx->engine->identifier(QLatin1String("0")));
        if (r6 && !(r6->isUndefined() || r6->isNull()))
            R = r6->toString(ctx)->toQString();

        for (quint32 k = 1; k < r2; ++k) {
            R += r4;

            String *name = Value::fromNumber(k).toString(ctx);
            Value *r12 = self.objectValue->getProperty(name);

            if (r12 && ! (r12->isUndefined() || r12->isNull()))
                R += r12->toString(ctx)->toQString();
        }
    }

    visitedArrayElements.remove(self.objectValue);
    ctx->result = Value::fromString(ctx, R);
}

void ArrayPrototype::method_pop(Context *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        Value elt = instance->value.pop();
        ctx->result = elt;
    } else {
        String *id_length = ctx->engine->identifier(QLatin1String("length"));
        Value *r1 = self.objectValue->getProperty(id_length);
        quint32 r2 = r1 ? r1->toUInt32(ctx) : 0;
        if (! r2) {
            self.objectValue->put(id_length, Value::fromNumber(0));
        } else {
            String *r6 = Value::fromNumber(r2 - 1).toString(ctx);
            Value *r7 = self.objectValue->getProperty(r6);
            self.objectValue->deleteProperty(r6, 0);
            self.objectValue->put(id_length, Value::fromNumber(2 - 1));
            if (r7)
                ctx->result = *r7;
        }
    }
}

void ArrayPrototype::method_push(Context *ctx)
{
}

void ArrayPrototype::method_reverse(Context *ctx)
{
}

void ArrayPrototype::method_shift(Context *ctx)
{
}

void ArrayPrototype::method_slice(Context *ctx)
{
}

void ArrayPrototype::method_sort(Context *ctx)
{
}

void ArrayPrototype::method_splice(Context *ctx)
{
}

void ArrayPrototype::method_unshift(Context *ctx)
{
}

//
// Date object
//
Value DateCtor::create(ExecutionEngine *engine)
{
    Context *ctx = engine->rootContext;
    FunctionObject *ctor = ctx->engine->newDateCtor(ctx);
    ctor->setProperty(ctx, QLatin1String("prototype"), Value::fromObject(ctx->engine->newDatePrototype(ctx, ctor)));
    return Value::fromObject(ctor);
}

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
            __qmljs_to_primitive(ctx, &arg, &arg, PREFERREDTYPE_HINT);

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

    Object *d = ctx->engine->newDateObject(Value::fromNumber(t));
    ctx->thisObject = Value::fromObject(d);
}

void DateCtor::call(Context *ctx)
{
    double t = currentTime();
    ctx->result = Value::fromString(ctx, ToString(t));
}

DatePrototype::DatePrototype(Context *ctx, FunctionObject *ctor)
    : DateObject(Value::fromNumber(qSNaN()))
{
    LocalTZA = getLocalTZA();

    ctor->setProperty(ctx, QLatin1String("parse"), method_parse, 1);
    ctor->setProperty(ctx, QLatin1String("UTC"), method_UTC, 7);

    setProperty(ctx, QLatin1String("constructor"), Value::fromObject(ctor));
    setProperty(ctx, QLatin1String("toString"), method_toString, 0);
    setProperty(ctx, QLatin1String("toDateString"), method_toDateString, 0);
    setProperty(ctx, QLatin1String("toTimeString"), method_toTimeString, 0);
    setProperty(ctx, QLatin1String("toLocaleString"), method_toLocaleString, 0);
    setProperty(ctx, QLatin1String("toLocaleDateString"), method_toLocaleDateString, 0);
    setProperty(ctx, QLatin1String("toLocaleTimeString"), method_toLocaleTimeString, 0);
    setProperty(ctx, QLatin1String("valueOf"), method_valueOf, 0);
    setProperty(ctx, QLatin1String("getTime"), method_getTime, 0);
    setProperty(ctx, QLatin1String("getYear"), method_getYear, 0);
    setProperty(ctx, QLatin1String("getFullYear"), method_getFullYear, 0);
    setProperty(ctx, QLatin1String("getUTCFullYear"), method_getUTCFullYear, 0);
    setProperty(ctx, QLatin1String("getMonth"), method_getMonth, 0);
    setProperty(ctx, QLatin1String("getUTCMonth"), method_getUTCMonth, 0);
    setProperty(ctx, QLatin1String("getDate"), method_getDate, 0);
    setProperty(ctx, QLatin1String("getUTCDate"), method_getUTCDate, 0);
    setProperty(ctx, QLatin1String("getDay"), method_getDay, 0);
    setProperty(ctx, QLatin1String("getUTCDay"), method_getUTCDay, 0);
    setProperty(ctx, QLatin1String("getHours"), method_getHours, 0);
    setProperty(ctx, QLatin1String("getUTCHours"), method_getUTCHours, 0);
    setProperty(ctx, QLatin1String("getMinutes"), method_getMinutes, 0);
    setProperty(ctx, QLatin1String("getUTCMinutes"), method_getUTCMinutes, 0);
    setProperty(ctx, QLatin1String("getSeconds"), method_getSeconds, 0);
    setProperty(ctx, QLatin1String("getUTCSeconds"), method_getUTCSeconds, 0);
    setProperty(ctx, QLatin1String("getMilliseconds"), method_getMilliseconds, 0);
    setProperty(ctx, QLatin1String("getUTCMilliseconds"), method_getUTCMilliseconds, 0);
    setProperty(ctx, QLatin1String("getTimezoneOffset"), method_getTimezoneOffset, 0);
    setProperty(ctx, QLatin1String("setTime"), method_setTime, 1);
    setProperty(ctx, QLatin1String("setMilliseconds"), method_setMilliseconds, 1);
    setProperty(ctx, QLatin1String("setUTCMilliseconds"), method_setUTCMilliseconds, 1);
    setProperty(ctx, QLatin1String("setSeconds"), method_setSeconds, 2);
    setProperty(ctx, QLatin1String("setUTCSeconds"), method_setUTCSeconds, 2);
    setProperty(ctx, QLatin1String("setMinutes"), method_setMinutes, 3);
    setProperty(ctx, QLatin1String("setUTCMinutes"), method_setUTCMinutes, 3);
    setProperty(ctx, QLatin1String("setHours"), method_setHours, 4);
    setProperty(ctx, QLatin1String("setUTCHours"), method_setUTCHours, 4);
    setProperty(ctx, QLatin1String("setDate"), method_setDate, 1);
    setProperty(ctx, QLatin1String("setUTCDate"), method_setUTCDate, 1);
    setProperty(ctx, QLatin1String("setMonth"), method_setMonth, 2);
    setProperty(ctx, QLatin1String("setUTCMonth"), method_setUTCMonth, 2);
    setProperty(ctx, QLatin1String("setYear"), method_setYear, 1);
    setProperty(ctx, QLatin1String("setFullYear"), method_setFullYear, 3);
    setProperty(ctx, QLatin1String("setUTCFullYear"), method_setUTCFullYear, 3);
    setProperty(ctx, QLatin1String("toUTCString"), method_toUTCString, 0);
    setProperty(ctx, QLatin1String("toGMTString"), method_toUTCString, 0);
}

double DatePrototype::getThisDate(Context *ctx)
{
    if (DateObject *thisObject = ctx->thisObject.asDateObject())
        return thisObject->value.numberValue;
    else {
        assert(!"type error");
        return 0;
    }
}

void DatePrototype::method_MakeTime(Context *ctx)
{
}

void DatePrototype::method_MakeDate(Context *ctx)
{
}

void DatePrototype::method_TimeClip(Context *ctx)
{
}

void DatePrototype::method_parse(Context *ctx)
{
    ctx->result = Value::fromNumber(ParseString(ctx->argument(0).toString(ctx)->toQString()));
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
        ctx->result = Value::fromNumber(TimeClip(t));
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
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getTime(Context *ctx)
{
    double t = getThisDate(ctx);
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getYear(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = YearFromTime(LocalTime(t)) - 1900;
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getFullYear(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = YearFromTime(LocalTime(t));
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getUTCFullYear(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = YearFromTime(t);
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getMonth(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = MonthFromTime(LocalTime(t));
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getUTCMonth(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = MonthFromTime(t);
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getDate(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = DateFromTime(LocalTime(t));
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getUTCDate(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = DateFromTime(t);
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getDay(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = WeekDay(LocalTime(t));
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getUTCDay(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = WeekDay(t);
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getHours(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = HourFromTime(LocalTime(t));
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getUTCHours(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = HourFromTime(t);
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getMinutes(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = MinFromTime(LocalTime(t));
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getUTCMinutes(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = MinFromTime(t);
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getSeconds(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = SecFromTime(LocalTime(t));
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getUTCSeconds(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = SecFromTime(t);
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getMilliseconds(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = msFromTime(LocalTime(t));
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getUTCMilliseconds(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = msFromTime(t);
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_getTimezoneOffset(Context *ctx)
{
    double t = getThisDate(ctx);
    if (! qIsNaN(t))
        t = (t - LocalTime(t)) / msPerMinute;
    ctx->result = Value::fromNumber(t);
}

void DatePrototype::method_setTime(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        self->value.numberValue = TimeClip(ctx->argument(0).toNumber(ctx));
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setMilliseconds(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.numberValue);
        double ms = ctx->argument(0).toNumber(ctx);
        self->value.numberValue = TimeClip(UTC(MakeDate(Day(t), MakeTime(HourFromTime(t), MinFromTime(t), SecFromTime(t), ms))));
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setUTCMilliseconds(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.numberValue;
        double ms = ctx->argument(0).toNumber(ctx);
        self->value.numberValue = TimeClip(UTC(MakeDate(Day(t), MakeTime(HourFromTime(t), MinFromTime(t), SecFromTime(t), ms))));
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setSeconds(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.numberValue);
        double sec = ctx->argument(0).toNumber(ctx);
        double ms = (ctx->argumentCount < 2) ? msFromTime(t) : ctx->argument(1).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(Day(t), MakeTime(HourFromTime(t), MinFromTime(t), sec, ms))));
        self->value.numberValue = t;
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setUTCSeconds(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.numberValue;
        double sec = ctx->argument(0).toNumber(ctx);
        double ms = (ctx->argumentCount < 2) ? msFromTime(t) : ctx->argument(1).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(Day(t), MakeTime(HourFromTime(t), MinFromTime(t), sec, ms))));
        self->value.numberValue = t;
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setMinutes(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.numberValue);
        double min = ctx->argument(0).toNumber(ctx);
        double sec = (ctx->argumentCount < 2) ? SecFromTime(t) : ctx->argument(1).toNumber(ctx);
        double ms = (ctx->argumentCount < 3) ? msFromTime(t) : ctx->argument(2).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(Day(t), MakeTime(HourFromTime(t), min, sec, ms))));
        self->value.numberValue = t;
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setUTCMinutes(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.numberValue;
        double min = ctx->argument(0).toNumber(ctx);
        double sec = (ctx->argumentCount < 2) ? SecFromTime(t) : ctx->argument(1).toNumber(ctx);
        double ms = (ctx->argumentCount < 3) ? msFromTime(t) : ctx->argument(2).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(Day(t), MakeTime(HourFromTime(t), min, sec, ms))));
        self->value.numberValue = t;
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setHours(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.numberValue);
        double hour = ctx->argument(0).toNumber(ctx);
        double min = (ctx->argumentCount < 2) ? MinFromTime(t) : ctx->argument(1).toNumber(ctx);
        double sec = (ctx->argumentCount < 3) ? SecFromTime(t) : ctx->argument(2).toNumber(ctx);
        double ms = (ctx->argumentCount < 4) ? msFromTime(t) : ctx->argument(3).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(Day(t), MakeTime(hour, min, sec, ms))));
        self->value.numberValue = t;
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setUTCHours(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.numberValue;
        double hour = ctx->argument(0).toNumber(ctx);
        double min = (ctx->argumentCount < 2) ? MinFromTime(t) : ctx->argument(1).toNumber(ctx);
        double sec = (ctx->argumentCount < 3) ? SecFromTime(t) : ctx->argument(2).toNumber(ctx);
        double ms = (ctx->argumentCount < 4) ? msFromTime(t) : ctx->argument(3).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(Day(t), MakeTime(hour, min, sec, ms))));
        self->value.numberValue = t;
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setDate(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.numberValue);
        double date = ctx->argument(0).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(MakeDay(YearFromTime(t), MonthFromTime(t), date), TimeWithinDay(t))));
        self->value.numberValue = t;
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setUTCDate(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.numberValue;
        double date = ctx->argument(0).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(MakeDay(YearFromTime(t), MonthFromTime(t), date), TimeWithinDay(t))));
        self->value.numberValue = t;
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setMonth(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.numberValue);
        double month = ctx->argument(0).toNumber(ctx);
        double date = (ctx->argumentCount < 2) ? DateFromTime(t) : ctx->argument(1).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(MakeDay(YearFromTime(t), month, date), TimeWithinDay(t))));
        self->value.numberValue = t;
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setUTCMonth(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.numberValue;
        double month = ctx->argument(0).toNumber(ctx);
        double date = (ctx->argumentCount < 2) ? DateFromTime(t) : ctx->argument(1).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(MakeDay(YearFromTime(t), month, date), TimeWithinDay(t))));
        self->value.numberValue = t;
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setYear(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.numberValue;
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
        self->value.numberValue = r;
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setUTCFullYear(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.numberValue;
        double year = ctx->argument(0).toNumber(ctx);
        double month = (ctx->argumentCount < 2) ? MonthFromTime(t) : ctx->argument(1).toNumber(ctx);
        double date = (ctx->argumentCount < 3) ? DateFromTime(t) : ctx->argument(2).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(MakeDay(year, month, date), TimeWithinDay(t))));
        self->value.numberValue = t;
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_setFullYear(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = LocalTime(self->value.numberValue);
        double year = ctx->argument(0).toNumber(ctx);
        double month = (ctx->argumentCount < 2) ? MonthFromTime(t) : ctx->argument(1).toNumber(ctx);
        double date = (ctx->argumentCount < 3) ? DateFromTime(t) : ctx->argument(2).toNumber(ctx);
        t = TimeClip(UTC(MakeDate(MakeDay(year, month, date), TimeWithinDay(t))));
        self->value.numberValue = t;
        ctx->result = self->value;
    } else {
        assert(!"type error");
    }
}

void DatePrototype::method_toUTCString(Context *ctx)
{
    if (DateObject *self = ctx->thisObject.asDateObject()) {
        double t = self->value.numberValue;
        ctx->result = Value::fromString(ctx, ToUTCString(t));
    }
}

//
// Math object
//
MathObject::MathObject(Context *ctx)
{
    setProperty(ctx, QLatin1String("E"), Value::fromNumber(::exp(1.0)));
    setProperty(ctx, QLatin1String("LN2"), Value::fromNumber(::log(2.0)));
    setProperty(ctx, QLatin1String("LN10"), Value::fromNumber(::log(10.0)));
    setProperty(ctx, QLatin1String("LOG2E"), Value::fromNumber(1.0/::log(2.0)));
    setProperty(ctx, QLatin1String("LOG10E"), Value::fromNumber(1.0/::log(10.0)));
    setProperty(ctx, QLatin1String("PI"), Value::fromNumber(qt_PI));
    setProperty(ctx, QLatin1String("SQRT1_2"), Value::fromNumber(::sqrt(0.5)));
    setProperty(ctx, QLatin1String("SQRT2"), Value::fromNumber(::sqrt(2.0)));

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
        ctx->result = Value::fromNumber(0);
    else
        ctx->result = Value::fromNumber(v < 0 ? -v : v);
}

void MathObject::method_acos(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v > 1)
        ctx->result = Value::fromNumber(qSNaN());
    else
        ctx->result = Value::fromNumber(::acos(v));
}

void MathObject::method_asin(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v > 1)
        ctx->result = Value::fromNumber(qSNaN());
    else
        ctx->result = Value::fromNumber(::asin(v));
}

void MathObject::method_atan(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v == 0.0)
        ctx->result = Value::fromNumber(v);
    else
        ctx->result = Value::fromNumber(::atan(v));
}

void MathObject::method_atan2(Context *ctx)
{
    double v1 = ctx->argument(0).toNumber(ctx);
    double v2 = ctx->argument(1).toNumber(ctx);
    if ((v1 < 0) && qIsFinite(v1) && qIsInf(v2) && (copySign(1.0, v2) == 1.0)) {
        ctx->result = Value::fromNumber(copySign(0, -1.0));
        return;
    }
    if ((v1 == 0.0) && (v2 == 0.0)) {
        if ((copySign(1.0, v1) == 1.0) && (copySign(1.0, v2) == -1.0)) {
            ctx->result = Value::fromNumber(qt_PI);
            return;
        } else if ((copySign(1.0, v1) == -1.0) && (copySign(1.0, v2) == -1.0)) {
            ctx->result = Value::fromNumber(-qt_PI);
            return;
        }
    }
    ctx->result = Value::fromNumber(::atan2(v1, v2));
}

void MathObject::method_ceil(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v < 0.0 && v > -1.0)
        ctx->result = Value::fromNumber(copySign(0, -1.0));
    else
        ctx->result = Value::fromNumber(::ceil(v));
}

void MathObject::method_cos(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    ctx->result = Value::fromNumber(::cos(v));
}

void MathObject::method_exp(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (qIsInf(v)) {
        if (copySign(1.0, v) == -1.0)
            ctx->result = Value::fromNumber(0);
        else
            ctx->result = Value::fromNumber(qInf());
    } else {
        ctx->result = Value::fromNumber(::exp(v));
    }
}

void MathObject::method_floor(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    ctx->result = Value::fromNumber(::floor(v));
}

void MathObject::method_log(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v < 0)
        ctx->result = Value::fromNumber(qSNaN());
    else
        ctx->result = Value::fromNumber(::log(v));
}

void MathObject::method_max(Context *ctx)
{
    double mx = -qInf();
    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        double x = ctx->argument(i).toNumber(ctx);
        if (x > mx || qIsNaN(x))
            mx = x;
    }
    ctx->result = Value::fromNumber(mx);
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
    ctx->result = Value::fromNumber(mx);
}

void MathObject::method_pow(Context *ctx)
{
    double x = ctx->argument(0).toNumber(ctx);
    double y = ctx->argument(1).toNumber(ctx);

    if (qIsNaN(y)) {
        ctx->result = Value::fromNumber(qSNaN());
        return;
    }

    if (y == 0) {
        ctx->result = Value::fromNumber(1);
    } else if (((x == 1) || (x == -1)) && qIsInf(y)) {
        ctx->result = Value::fromNumber(qSNaN());
    } else if (((x == 0) && copySign(1.0, x) == 1.0) && (y < 0)) {
        ctx->result = Value::fromNumber(qInf());
    } else if ((x == 0) && copySign(1.0, x) == -1.0) {
        if (y < 0) {
            if (::fmod(-y, 2.0) == 1.0)
                ctx->result = Value::fromNumber(-qInf());
            else
                ctx->result = Value::fromNumber(qInf());
        } else if (y > 0) {
            if (::fmod(y, 2.0) == 1.0)
                ctx->result = Value::fromNumber(copySign(0, -1.0));
            else
                ctx->result = Value::fromNumber(0);
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
        ctx->result = Value::fromNumber(::pow(x, y));
    }
}

void MathObject::method_random(Context *ctx)
{
    ctx->result = Value::fromNumber(qrand() / (double) RAND_MAX);
}

void MathObject::method_round(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    v = copySign(::floor(v + 0.5), v);
    ctx->result = Value::fromNumber(v);
}

void MathObject::method_sin(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    ctx->result = Value::fromNumber(::sin(v));
}

void MathObject::method_sqrt(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    ctx->result = Value::fromNumber(::sqrt(v));
}

void MathObject::method_tan(Context *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v == 0.0)
        ctx->result = Value::fromNumber(v);
    else
        ctx->result = Value::fromNumber(::tan(v));
}
