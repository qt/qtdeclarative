/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qqmllocale_p.h"
#include "qqmlengine_p.h"
#include <private/qqmlcontext_p.h>
#include <private/qjsconverter_impl_p.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qdatetime.h>

#include <private/qlocale_p.h>
#include <private/qlocale_data_p.h>

QT_BEGIN_NAMESPACE

class QV8LocaleDataResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(LocaleDataType)
public:
    QV8LocaleDataResource(QV8Engine *e) : QV8ObjectResource(e) {}
    QLocale locale;
};

#define GET_LOCALE_DATA_RESOURCE(OBJECT) \
QV8LocaleDataResource *r = v8_resource_cast<QV8LocaleDataResource>(OBJECT); \
if (!r) \
    V8THROW_ERROR("Not a valid Locale object")

static bool isLocaleObject(v8::Handle<v8::Value> val)
{
    if (!val->IsObject())
        return false;

    v8::Handle<v8::Object> localeObj = val->ToObject();
    return localeObj->Has(v8::String::New("nativeLanguageName")); //XXX detect locale object properly
}

//--------------
// Date extension

static const char dateToLocaleStringFunction[] =
        "(function(toLocaleStringFunc) { "
        "  var orig_toLocaleString;"
        "  orig_toLocaleString = Date.prototype.toLocaleString;"
        "  Date.prototype.toLocaleString = (function() {"
        "    var val = toLocaleStringFunc.apply(this, arguments);"
        "    if (val == undefined) val = orig_toLocaleString.call(this);"
        "    return val;"
        "  })"
        "})";

static const char dateToLocaleTimeStringFunction[] =
        "(function(toLocaleTimeStringFunc) { "
        "  var orig_toLocaleTimeString;"
        "  orig_toLocaleTimeString = Date.prototype.toLocaleTimeString;"
        "  Date.prototype.toLocaleTimeString = (function() {"
        "    var val = toLocaleTimeStringFunc.apply(this, arguments);"
        "    if (val == undefined) val = orig_toLocaleTimeString.call(this);"
        "    return val;"
        "  })"
        "})";

static const char dateToLocaleDateStringFunction[] =
        "(function(toLocaleDateStringFunc) { "
        "  var orig_toLocaleDateString;"
        "  orig_toLocaleDateString = Date.prototype.toLocaleDateString;"
        "  Date.prototype.toLocaleDateString = (function() {"
        "    var val = toLocaleDateStringFunc.apply(this, arguments);"
        "    if (val == undefined) val = orig_toLocaleDateString.call(this);"
        "    return val;"
        "  })"
        "})";


static const char dateFromLocaleStringFunction[] =
        "(function(fromLocaleStringFunc) { "
        "  Date.fromLocaleString = (function() {"
        "    return fromLocaleStringFunc.apply(null, arguments);"
        "  })"
        "})";

static const char dateFromLocaleTimeStringFunction[] =
        "(function(fromLocaleTimeStringFunc) { "
        "  Date.fromLocaleTimeString = (function() {"
        "    return fromLocaleTimeStringFunc.apply(null, arguments);"
        "  })"
        "})";

static const char dateFromLocaleDateStringFunction[] =
        "(function(fromLocaleDateStringFunc) { "
        "  Date.fromLocaleDateString = (function() {"
        "    return fromLocaleDateStringFunc.apply(null, arguments);"
        "  })"
        "})";

static const char dateTimeZoneUpdatedFunction[] =
        "(function(timeZoneUpdatedFunc) { "
        "  Date.timeZoneUpdated = (function() {"
        "    return timeZoneUpdatedFunc.apply(null, arguments);"
        "  })"
        "})";


static void registerFunction(QV8Engine *engine, const char *script, v8::InvocationCallback func)
{
    v8::Local<v8::Script> registerScript = v8::Script::New(v8::String::New(script), 0, 0, v8::Handle<v8::String>(), v8::Script::NativeMode);
    v8::Local<v8::Value> result = registerScript->Run();
    Q_ASSERT(result->IsFunction());
    v8::Local<v8::Function> registerFunc = v8::Local<v8::Function>::Cast(result);
    v8::Handle<v8::Value> args = V8FUNCTION(func, engine);
    registerFunc->Call(v8::Local<v8::Object>::Cast(registerFunc), 1, &args);
}

void QQmlDateExtension::registerExtension(QV8Engine *engine)
{
    registerFunction(engine, dateToLocaleStringFunction, toLocaleString);
    registerFunction(engine, dateToLocaleTimeStringFunction, toLocaleTimeString);
    registerFunction(engine, dateToLocaleDateStringFunction, toLocaleDateString);
    registerFunction(engine, dateFromLocaleStringFunction, fromLocaleString);
    registerFunction(engine, dateFromLocaleTimeStringFunction, fromLocaleTimeString);
    registerFunction(engine, dateFromLocaleDateStringFunction, fromLocaleDateString);
    registerFunction(engine, dateTimeZoneUpdatedFunction, timeZoneUpdated);
}

v8::Handle<v8::Value> QQmlDateExtension::toLocaleString(const v8::Arguments& args)
{
    if (args.Length() > 2)
        return v8::Undefined();

    if (!args.This()->IsDate())
        return v8::Undefined();

    QDateTime dt = QV8Engine::qtDateTimeFromJsDate(v8::Handle<v8::Date>::Cast(args.This())->NumberValue());

    if (args.Length() == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        return QJSConverter::toString(locale.toString(dt));
    }

    if (!isLocaleObject(args[0]))
        return v8::Undefined(); // Use the default Date toLocaleString()

    GET_LOCALE_DATA_RESOURCE(args[0]->ToObject());

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QString formattedDt;
    if (args.Length() == 2) {
        if (args[1]->IsString()) {
            QString format = r->engine->toVariant(args[1], -1).toString();
            formattedDt = r->locale.toString(dt, format);
        } else if (args[1]->IsNumber()) {
            quint32 intFormat = args[1]->ToNumber()->Value();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            formattedDt = r->locale.toString(dt, format);
        } else {
            V8THROW_ERROR("Locale: Date.toLocaleString(): Invalid datetime format");
        }
    } else {
         formattedDt = r->locale.toString(dt, enumFormat);
    }

    return r->engine->toString(formattedDt);
}

v8::Handle<v8::Value> QQmlDateExtension::toLocaleTimeString(const v8::Arguments& args)
{
    if (args.Length() > 2)
        return v8::Undefined();

    if (!args.This()->IsDate())
        return v8::Undefined();

    QDateTime dt = QV8Engine::qtDateTimeFromJsDate(v8::Handle<v8::Date>::Cast(args.This())->NumberValue());
    QTime time = dt.time();

    if (args.Length() == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        return QJSConverter::toString(locale.toString(time));
    }

    if (!isLocaleObject(args[0]))
        return v8::Undefined(); // Use the default Date toLocaleTimeString()

    GET_LOCALE_DATA_RESOURCE(args[0]->ToObject());

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QString formattedTime;
    if (args.Length() == 2) {
        if (args[1]->IsString()) {
            QString format = r->engine->toVariant(args[1], -1).toString();
            formattedTime = r->locale.toString(time, format);
        } else if (args[1]->IsNumber()) {
            quint32 intFormat = args[1]->ToNumber()->Value();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            formattedTime = r->locale.toString(time, format);
        } else {
            V8THROW_ERROR("Locale: Date.toLocaleTimeString(): Invalid time format");
        }
    } else {
         formattedTime = r->locale.toString(time, enumFormat);
    }

    return r->engine->toString(formattedTime);
}

v8::Handle<v8::Value> QQmlDateExtension::toLocaleDateString(const v8::Arguments& args)
{
    if (args.Length() > 2)
        return v8::Undefined();

    if (!args.This()->IsDate())
        return v8::Undefined();

    QDateTime dt = QV8Engine::qtDateTimeFromJsDate(v8::Handle<v8::Date>::Cast(args.This())->NumberValue());
    QDate date = dt.date();

    if (args.Length() == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        return QJSConverter::toString(locale.toString(date));
    }

    if (!isLocaleObject(args[0]))
        return v8::Undefined(); // Use the default Date toLocaleDateString()

    GET_LOCALE_DATA_RESOURCE(args[0]->ToObject());

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QString formattedDate;
    if (args.Length() == 2) {
        if (args[1]->IsString()) {
            QString format = r->engine->toVariant(args[1], -1).toString();
            formattedDate = r->locale.toString(date, format);
        } else if (args[1]->IsNumber()) {
            quint32 intFormat = args[1]->ToNumber()->Value();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            formattedDate = r->locale.toString(date, format);
        } else {
            V8THROW_ERROR("Locale: Date.loLocaleDateString(): Invalid date format");
        }
    } else {
         formattedDate = r->locale.toString(date, enumFormat);
    }

    return r->engine->toString(formattedDate);
}

v8::Handle<v8::Value> QQmlDateExtension::fromLocaleString(const v8::Arguments& args)
{
    if (args.Length() == 1 && args[0]->IsString()) {
        QLocale locale;
        QString dateString = QJSConverter::toString(args[0]->ToString());
        QDateTime dt = locale.toDateTime(dateString);
        return QJSConverter::toDateTime(dt);
    }

    if (args.Length() < 1 || args.Length() > 3 || !isLocaleObject(args[0]))
        V8THROW_ERROR("Locale: Date.fromLocaleString(): Invalid arguments");

    GET_LOCALE_DATA_RESOURCE(args[0]->ToObject());

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QDateTime dt;
    QString dateString = r->engine->toString(args[1]->ToString());
    if (args.Length() == 3) {
        if (args[2]->IsString()) {
            QString format = r->engine->toString(args[2]->ToString());
            dt = r->locale.toDateTime(dateString, format);
        } else if (args[2]->IsNumber()) {
            quint32 intFormat = args[2]->ToNumber()->Value();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            dt = r->locale.toDateTime(dateString, format);
        } else {
            V8THROW_ERROR("Locale: Date.fromLocaleString(): Invalid datetime format");
        }
    } else {
        dt = r->locale.toDateTime(dateString, enumFormat);
    }

    return QJSConverter::toDateTime(dt);
}

v8::Handle<v8::Value> QQmlDateExtension::fromLocaleTimeString(const v8::Arguments& args)
{
    if (args.Length() == 1 && args[0]->IsString()) {
        QLocale locale;
        QString timeString = QJSConverter::toString(args[0]->ToString());
        QTime time = locale.toTime(timeString);
        QDateTime dt = QDateTime::currentDateTime();
        dt.setTime(time);
        return QJSConverter::toDateTime(dt);
    }

    if (args.Length() < 1 || args.Length() > 3 || !isLocaleObject(args[0]))
        V8THROW_ERROR("Locale: Date.fromLocaleTimeString(): Invalid arguments");

    GET_LOCALE_DATA_RESOURCE(args[0]->ToObject());

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QTime tm;
    QString dateString = r->engine->toString(args[1]->ToString());
    if (args.Length() == 3) {
        if (args[2]->IsString()) {
            QString format = r->engine->toString(args[2]->ToString());
            tm = r->locale.toTime(dateString, format);
        } else if (args[2]->IsNumber()) {
            quint32 intFormat = args[2]->ToNumber()->Value();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            tm = r->locale.toTime(dateString, format);
        } else {
            V8THROW_ERROR("Locale: Date.fromLocaleTimeString(): Invalid datetime format");
        }
    } else {
        tm = r->locale.toTime(dateString, enumFormat);
    }

    QDateTime dt = QDateTime::currentDateTime();
    dt.setTime(tm);

    return QJSConverter::toDateTime(dt);
}

v8::Handle<v8::Value> QQmlDateExtension::fromLocaleDateString(const v8::Arguments& args)
{
    if (args.Length() == 1 && args[0]->IsString()) {
        QLocale locale;
        QString dateString = QJSConverter::toString(args[0]->ToString());
        QDate date = locale.toDate(dateString);
        return QJSConverter::toDateTime(QDateTime(date));
    }

    if (args.Length() < 1 || args.Length() > 3 || !isLocaleObject(args[0]))
        V8THROW_ERROR("Locale: Date.fromLocaleDateString(): Invalid arguments");

    GET_LOCALE_DATA_RESOURCE(args[0]->ToObject());

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QDate dt;
    QString dateString = r->engine->toString(args[1]->ToString());
    if (args.Length() == 3) {
        if (args[2]->IsString()) {
            QString format = r->engine->toString(args[2]->ToString());
            dt = r->locale.toDate(dateString, format);
        } else if (args[2]->IsNumber()) {
            quint32 intFormat = args[2]->ToNumber()->Value();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            dt = r->locale.toDate(dateString, format);
        } else {
            V8THROW_ERROR("Locale: Date.fromLocaleDateString(): Invalid datetime format");
        }
    } else {
        dt = r->locale.toDate(dateString, enumFormat);
    }

    return QJSConverter::toDateTime(QDateTime(dt));
}

v8::Handle<v8::Value> QQmlDateExtension::timeZoneUpdated(const v8::Arguments& args)
{
    if (args.Length() != 0)
        V8THROW_ERROR("Locale: Date.timeZoneUpdated(): Invalid arguments");

    v8::Date::DateTimeConfigurationChangeNotification();

    return v8::Undefined();
}

//-----------------
// Number extension

static const char numberToLocaleStringFunction[] =
        "(function(toLocaleStringFunc) { "
        "  var orig_toLocaleString;"
        "  orig_toLocaleString = Number.prototype.toLocaleString;"
        "  Number.prototype.toLocaleString = (function() {"
        "    var val = toLocaleStringFunc.apply(this, arguments);"
        "    if (val == undefined) val = orig_toLocaleString.call(this);"
        "    return val;"
        "  })"
        "})";

static const char numberToLocaleCurrencyStringFunction[] =
        "(function(toLocaleCurrencyStringFunc) { "
        "  Number.prototype.toLocaleCurrencyString = (function() {"
        "    return toLocaleCurrencyStringFunc.apply(this, arguments);"
        "  })"
        "})";

static const char numberFromLocaleStringFunction[] =
        "(function(fromLocaleStringFunc) { "
        "  Number.fromLocaleString = (function() {"
        "    return fromLocaleStringFunc.apply(null, arguments);"
        "  })"
        "})";


void QQmlNumberExtension::registerExtension(QV8Engine *engine)
{
    registerFunction(engine, numberToLocaleStringFunction, toLocaleString);
    registerFunction(engine, numberToLocaleCurrencyStringFunction, toLocaleCurrencyString);
    registerFunction(engine, numberFromLocaleStringFunction, fromLocaleString);
}

v8::Handle<v8::Value> QQmlNumberExtension::toLocaleString(const v8::Arguments& args)
{
    if (args.Length() > 3)
        V8THROW_ERROR("Locale: Number.toLocaleString(): Invalid arguments");

    double number = args.This()->ToNumber()->Value();

    if (args.Length() == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        return QJSConverter::toString(locale.toString(number));
    }

    if (!isLocaleObject(args[0]))
        return v8::Undefined(); // Use the default Number toLocaleString()

    GET_LOCALE_DATA_RESOURCE(args[0]->ToObject());

    uint16_t format = 'f';
    if (args.Length() > 1) {
        if (!args[1]->IsString())
            V8THROW_ERROR("Locale: Number.toLocaleString(): Invalid arguments");
        v8::Local<v8::String> fs = args[1]->ToString();
        if (!fs.IsEmpty() && fs->Length()) {
            v8::String::Value value(fs);
            Q_ASSERT(*value != NULL);
            format = **value;
        }
    }
    int prec = 2;
    if (args.Length() > 2) {
        if (!args[2]->IsNumber())
            V8THROW_ERROR("Locale: Number.toLocaleString(): Invalid arguments");
         prec = args[2]->IntegerValue();
    }

    return r->engine->toString(r->locale.toString(number, (char)format, prec));
}

v8::Handle<v8::Value> QQmlNumberExtension::toLocaleCurrencyString(const v8::Arguments& args)
{
    if (args.Length() > 2)
        V8THROW_ERROR("Locale: Number.toLocaleCurrencyString(): Invalid arguments");

    double number = args.This()->ToNumber()->Value();

    if (args.Length() == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        return QJSConverter::toString(locale.toString(number));
    }

    if (!isLocaleObject(args[0]))
        V8THROW_ERROR("Locale: Number.toLocaleCurrencyString(): Invalid arguments");

    GET_LOCALE_DATA_RESOURCE(args[0]->ToObject());

    QString symbol;
    if (args.Length() > 1) {
        if (!args[1]->IsString())
            V8THROW_ERROR("Locale: Number.toLocaleString(): Invalid arguments");
        symbol = r->engine->toString(args[1]->ToString());
    }

    return r->engine->toString(r->locale.toCurrencyString(number, symbol));
}

v8::Handle<v8::Value> QQmlNumberExtension::fromLocaleString(const v8::Arguments& args)
{
    if (args.Length() < 1 || args.Length() > 2)
        V8THROW_ERROR("Locale: Number.fromLocaleString(): Invalid arguments");

    int numberIdx = 0;
    QLocale locale;

    if (args.Length() == 2) {
        if (!isLocaleObject(args[0]))
            V8THROW_ERROR("Locale: Number.fromLocaleString(): Invalid arguments");

        GET_LOCALE_DATA_RESOURCE(args[0]->ToObject());
        locale = r->locale;

        numberIdx = 1;
    }

    v8::Local<v8::String> ns = args[numberIdx]->ToString();
    if (ns.IsEmpty() || ns->Length() == 0)
        return v8::Number::New(Q_QNAN);

    bool ok = false;
    double val = locale.toDouble(QJSConverter::toString(ns), &ok);

    if (!ok)
        V8THROW_ERROR("Locale: Number.fromLocaleString(): Invalid format")

    return v8::Number::New(val);
}

//--------------
// Locale object

static v8::Handle<v8::Value> locale_get_firstDayOfWeek(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    GET_LOCALE_DATA_RESOURCE(info.This());
    int fdow = int(r->locale.firstDayOfWeek());
    if (fdow == 7)
        fdow = 0; // Qt::Sunday = 7, but Sunday is 0 in JS Date
    return v8::Integer::New(fdow);
}

static v8::Handle<v8::Value> locale_get_measurementSystem(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    GET_LOCALE_DATA_RESOURCE(info.This());
    return v8::Integer::New(r->locale.measurementSystem());
}

static v8::Handle<v8::Value> locale_get_textDirection(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    GET_LOCALE_DATA_RESOURCE(info.This());
    return v8::Integer::New(r->locale.textDirection());
}

static v8::Handle<v8::Value> locale_get_weekDays(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    GET_LOCALE_DATA_RESOURCE(info.This());

    QList<Qt::DayOfWeek> days = r->locale.weekdays();

    v8::Handle<v8::Array> result = v8::Array::New(days.size());
    for (int i = 0; i < days.size(); ++i) {
        int day = days.at(i);
        if (day == 7) // JS Date days in range 0(Sunday) to 6(Saturday)
            day = 0;
        result->Set(i, v8::Integer::New(day));
    }

    return result;
}

static v8::Handle<v8::Value> locale_get_uiLanguages(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    GET_LOCALE_DATA_RESOURCE(info.This());

    QStringList langs = r->locale.uiLanguages();
    v8::Handle<v8::Array> result = v8::Array::New(langs.size());
    for (int i = 0; i < langs.size(); ++i) {
        result->Set(i, r->engine->toString(langs.at(i)));
    }

    return result;
}

static v8::Handle<v8::Value> locale_currencySymbol(const v8::Arguments &args)
{
    GET_LOCALE_DATA_RESOURCE(args.This());

    if (args.Length() > 1)
        V8THROW_ERROR("Locale: currencySymbol(): Invalid arguments");

    QLocale::CurrencySymbolFormat format = QLocale::CurrencySymbol;
    if (args.Length() == 1) {
        quint32 intFormat = args[0]->ToNumber()->Value();
        format = QLocale::CurrencySymbolFormat(intFormat);
    }

    return r->engine->toString(r->locale.currencySymbol(format));
}

#define LOCALE_FORMAT(FUNC) \
static v8::Handle<v8::Value> locale_ ##FUNC (const v8::Arguments &args) { \
    GET_LOCALE_DATA_RESOURCE(args.This());\
    if (args.Length() > 1) \
        V8THROW_ERROR("Locale: " #FUNC "(): Invalid arguments"); \
    QLocale::FormatType format = QLocale::LongFormat;\
    if (args.Length() == 1) { \
        quint32 intFormat = args[0]->Uint32Value(); \
        format = QLocale::FormatType(intFormat); \
    } \
    return r->engine->toString(r->locale. FUNC (format)); \
}

LOCALE_FORMAT(dateTimeFormat)
LOCALE_FORMAT(timeFormat)
LOCALE_FORMAT(dateFormat)

// +1 added to idx because JS is 0-based, whereas QLocale months begin at 1.
#define LOCALE_FORMATTED_MONTHNAME(VARIABLE) \
static v8::Handle<v8::Value> locale_ ## VARIABLE (const v8::Arguments &args) {\
    GET_LOCALE_DATA_RESOURCE(args.This()); \
    if (args.Length() < 1 || args.Length() > 2) \
        V8THROW_ERROR("Locale: " #VARIABLE "(): Invalid arguments"); \
    QLocale::FormatType enumFormat = QLocale::LongFormat; \
    int idx = args[0]->IntegerValue() + 1; \
    if (idx < 1 || idx > 12) \
        V8THROW_ERROR("Locale: Invalid month"); \
    QString name; \
    if (args.Length() == 2) { \
        if (args[1]->IsNumber()) { \
            quint32 intFormat = args[1]->IntegerValue(); \
            QLocale::FormatType format = QLocale::FormatType(intFormat); \
            name = r->locale. VARIABLE(idx, format); \
        } else { \
            V8THROW_ERROR("Locale: Invalid datetime format"); \
        } \
    } else { \
        name = r->locale. VARIABLE(idx, enumFormat); \
    } \
    return r->engine->toString(name); \
}

// 0 -> 7 as Qt::Sunday is 7, but Sunday is 0 in JS Date
#define LOCALE_FORMATTED_DAYNAME(VARIABLE) \
static v8::Handle<v8::Value> locale_ ## VARIABLE (const v8::Arguments &args) {\
    GET_LOCALE_DATA_RESOURCE(args.This()); \
    if (args.Length() < 1 || args.Length() > 2) \
        V8THROW_ERROR("Locale: " #VARIABLE "(): Invalid arguments"); \
    QLocale::FormatType enumFormat = QLocale::LongFormat; \
    int idx = args[0]->IntegerValue(); \
    if (idx < 0 || idx > 7) \
        V8THROW_ERROR("Locale: Invalid day"); \
    if (idx == 0) idx = 7; \
    QString name; \
    if (args.Length() == 2) { \
        if (args[1]->IsNumber()) { \
            quint32 intFormat = args[1]->ToNumber()->Value(); \
            QLocale::FormatType format = QLocale::FormatType(intFormat); \
            name = r->locale. VARIABLE(idx, format); \
        } else { \
            V8THROW_ERROR("Locale: Invalid datetime format"); \
        } \
    } else { \
        name = r->locale. VARIABLE(idx, enumFormat); \
    } \
    return r->engine->toString(name); \
}


#define LOCALE_REGISTER_FORMATTED_NAME_FUNCTION(FT, VARIABLE, ENGINE) \
    FT->PrototypeTemplate()->Set(v8::String::New( #VARIABLE ), V8FUNCTION(locale_ ## VARIABLE, ENGINE));

LOCALE_FORMATTED_MONTHNAME(monthName)
LOCALE_FORMATTED_MONTHNAME(standaloneMonthName)
LOCALE_FORMATTED_DAYNAME(dayName)
LOCALE_FORMATTED_DAYNAME(standaloneDayName)

#define LOCALE_STRING_PROPERTY(VARIABLE) static v8::Handle<v8::Value> locale_get_ ## VARIABLE (v8::Local<v8::String>, const v8::AccessorInfo &info) \
{ \
    GET_LOCALE_DATA_RESOURCE(info.This()); \
    return r->engine->toString(r->locale. VARIABLE());\
}

#define LOCALE_REGISTER_STRING_ACCESSOR(FT, VARIABLE) \
    FT ->PrototypeTemplate()->SetAccessor( v8::String::New( #VARIABLE ), locale_get_ ## VARIABLE )


LOCALE_STRING_PROPERTY(name)
LOCALE_STRING_PROPERTY(nativeLanguageName)
LOCALE_STRING_PROPERTY(nativeCountryName)
LOCALE_STRING_PROPERTY(decimalPoint)
LOCALE_STRING_PROPERTY(groupSeparator)
LOCALE_STRING_PROPERTY(percent)
LOCALE_STRING_PROPERTY(zeroDigit)
LOCALE_STRING_PROPERTY(negativeSign)
LOCALE_STRING_PROPERTY(positiveSign)
LOCALE_STRING_PROPERTY(exponential)
LOCALE_STRING_PROPERTY(amText)
LOCALE_STRING_PROPERTY(pmText)

class QV8LocaleDataDeletable : public QV8Engine::Deletable
{
public:
    QV8LocaleDataDeletable(QV8Engine *engine);
    ~QV8LocaleDataDeletable();

    v8::Persistent<v8::Function> constructor;
};

QV8LocaleDataDeletable::QV8LocaleDataDeletable(QV8Engine *engine)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope scope(engine->context());

    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);

    LOCALE_REGISTER_STRING_ACCESSOR(ft, name);
    LOCALE_REGISTER_STRING_ACCESSOR(ft, nativeLanguageName);
    LOCALE_REGISTER_STRING_ACCESSOR(ft, nativeCountryName);
    LOCALE_REGISTER_STRING_ACCESSOR(ft, decimalPoint);
    LOCALE_REGISTER_STRING_ACCESSOR(ft, groupSeparator);
    LOCALE_REGISTER_STRING_ACCESSOR(ft, percent);
    LOCALE_REGISTER_STRING_ACCESSOR(ft, zeroDigit);
    LOCALE_REGISTER_STRING_ACCESSOR(ft, negativeSign);
    LOCALE_REGISTER_STRING_ACCESSOR(ft, positiveSign);
    LOCALE_REGISTER_STRING_ACCESSOR(ft, exponential);
    LOCALE_REGISTER_STRING_ACCESSOR(ft, amText);
    LOCALE_REGISTER_STRING_ACCESSOR(ft, pmText);

    ft->PrototypeTemplate()->Set(v8::String::New("currencySymbol"), V8FUNCTION(locale_currencySymbol, engine));

    ft->PrototypeTemplate()->Set(v8::String::New("dateTimeFormat"), V8FUNCTION(locale_dateTimeFormat, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("dateFormat"), V8FUNCTION(locale_dateFormat, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("timeFormat"), V8FUNCTION(locale_timeFormat, engine));

    LOCALE_REGISTER_FORMATTED_NAME_FUNCTION(ft, monthName, engine);
    LOCALE_REGISTER_FORMATTED_NAME_FUNCTION(ft, standaloneMonthName, engine);
    LOCALE_REGISTER_FORMATTED_NAME_FUNCTION(ft, dayName, engine);
    LOCALE_REGISTER_FORMATTED_NAME_FUNCTION(ft, standaloneDayName, engine);

    ft->PrototypeTemplate()->SetAccessor(v8::String::New("firstDayOfWeek"), locale_get_firstDayOfWeek);
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("weekDays"), locale_get_weekDays);
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("measurementSystem"), locale_get_measurementSystem);
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("textDirection"), locale_get_textDirection);
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("uiLanguages"), locale_get_uiLanguages);

    constructor = qPersistentNew(ft->GetFunction());
}

QV8LocaleDataDeletable::~QV8LocaleDataDeletable()
{
    qPersistentDispose(constructor);
}

V8_DEFINE_EXTENSION(QV8LocaleDataDeletable, localeV8Data);

/*!
    \qmltype Locale
    \instantiates QQmlLocale
    \inqmlmodule QtQml 2
    \brief Provides locale specific properties and formatted data

    The Locale object may only be created via the \l{QtQml2::Qt::locale()}{Qt.locale()} function.
    It cannot be created directly.

    The \l{QtQml2::Qt::locale()}{Qt.locale()} function returns a JS Locale object representing the
    locale with the specified name, which has the format
    "language[_territory][.codeset][@modifier]" or "C".

    Locale supports the concept of a default locale, which is
    determined from the system's locale settings at application
    startup.  If no parameter is passed to Qt.locale() the default
    locale object is returned.

    The Locale object provides a number of functions and properties
    providing data for the specified locale.

    The Locale object may also be passed to the \l Date and \l Number toLocaleString()
    and fromLocaleString() methods in order to convert to/from strings using
    the specified locale.

    This example shows the current date formatted for the German locale:

    \code
    import QtQuick 2.0

    Text {
        text: "The date is: " + Date().toLocaleString(Qt.locale("de_DE"))
    }
    \endcode

    The following example displays the specified number
    in the correct format for the default locale:

    \code
    import QtQuick 2.0

    Text {
        text: "The value is: " + Number(23443.34).toLocaleString(Qt.locale())
    }
    \endcode

    Qt Quick Locale's data is based on Common Locale Data Repository v1.8.1.


    \target FormatType
    \section2 Locale String Format Types

    The monthName(), standaloneMonthName(), dayName() and standaloneDayName()
    can use the following enumeration values to specify the formatting of
    the string representation for a Date object.

    \list
    \li Locale.LongFormat The long version of day and month names; for
    example, returning "January" as a month name.
    \li Locale.ShortFormat The short version of day and month names; for
    example, returning "Jan" as a month name.
    \li Locale.NarrowFormat A special version of day and month names for
    use when space is limited; for example, returning "J" as a month
    name. Note that the narrow format might contain the same text for
    different months and days or it can even be an empty string if the
    locale doesn't support narrow names, so you should avoid using it
    for date formatting. Also, for the system locale this format is
    the same as ShortFormat.
    \endlist


    Additionally the double-to-string and string-to-double conversion functions are
    covered by the following licenses:

    \legalese
    Copyright (c) 1991 by AT&T.

    Permission to use, copy, modify, and distribute this software for any
    purpose without fee is hereby granted, provided that this entire notice
    is included in all copies of any software which is or includes a copy
    or modification of this software and in all copies of the supporting
    documentation for such software.

    THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
    WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR AT&T MAKES ANY
    REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
    OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.

    This product includes software developed by the University of
    California, Berkeley and its contributors.

    \sa {QtQuick2::Date}{Date}, {QtQuick2::Number}{Number}
*/

QQmlLocale::QQmlLocale()
{
}

QQmlLocale::~QQmlLocale()
{
}

v8::Handle<v8::Value> QQmlLocale::locale(QV8Engine *v8engine, const QString &locale)
{
    QV8LocaleDataDeletable *d = localeV8Data(v8engine);
    v8::Local<v8::Object> v8Value = d->constructor->NewInstance();
    QV8LocaleDataResource *r = new QV8LocaleDataResource(v8engine);
    if (locale.isEmpty())
        r->locale = QLocale();
    else
        r->locale = QLocale(locale);
    v8Value->SetExternalResource(r);

    return v8Value;
}

static const char localeCompareFunction[] =
    "(function(localeCompareFunc) { "
    "  var orig_localeCompare;"
    "  orig_localeCompare = String.prototype.localeCompare;"
    "  String.prototype.localeCompare = (function() {"
    "    var val = localeCompareFunc.apply(this, arguments);"
    "    if (val == undefined) val = orig_localeCompare.call(this);"
    "    return val;"
    "  })"
    "})";

void QQmlLocale::registerStringLocaleCompare(QV8Engine *engine)
{
    registerFunction(engine, localeCompareFunction, localeCompare);
}

v8::Handle<v8::Value> QQmlLocale::localeCompare(const v8::Arguments &args)
{
    if (args.Length() != 1 || (!args[0]->IsString() && !args[0]->IsStringObject()))
        return v8::Undefined();

    if (!args.This()->IsString() && !args.This()->IsStringObject())
        return v8::Undefined();

    QString thisString = QJSConverter::toString(args.This()->ToString());
    QString thatString = QJSConverter::toString(args[0]->ToString());

    return v8::Integer::New(QString::localeAwareCompare(thisString, thatString));
}

/*!
    \qmlproperty string QtQuick2::Locale::name

    Holds the language and country of this locale as a
    string of the form "language_country", where
    language is a lowercase, two-letter ISO 639 language code,
    and country is an uppercase, two- or three-letter ISO 3166 country code.
*/

/*!
    \qmlproperty string QtQuick2::Locale::decimalPoint

    Holds the decimal point character of this locale.
*/

/*!
    \qmlproperty string QtQuick2::Locale::groupSeparator

    Holds the group separator character of this locale.
*/

/*!
    \qmlproperty string QtQuick2::Locale::percent

    Holds the percent character of this locale.
*/


/*!
    \qmlproperty string QtQuick2::Locale::zeroDigit

    Holds Returns the zero digit character of this locale.
*/

/*!
    \qmlproperty string QtQuick2::Locale::negativeSign

    Holds the negative sign character of this locale.
*/

/*!
    \qmlproperty string QtQuick2::Locale::positiveSign

    Holds the positive sign character of this locale.
*/

/*!
    \qmlproperty string QtQuick2::Locale::exponential

    Holds the exponential character of this locale.
*/

/*!
    \qmlmethod string QtQuick2::Locale::dateTimeFormat(type)

    Returns the date time format used for the current locale.
    \a type specifies the FormatType to return.

    \sa {QtQuick2::Date}{Date}
*/

/*!
    \qmlmethod string QtQuick2::Locale::dateFormat(type)

    Returns the date format used for the current locale.
    \a type specifies the FormatType to return.

    \sa {QtQuick2::Date}{Date}
*/

/*!
    \qmlmethod string QtQuick2::Locale::timeFormat(type)

    Returns the time format used for the current locale.
    \a type specifies the FormatType to return.

    \sa {QtQuick2::Date}{Date}
*/

/*!
    \qmlmethod string QtQuick2::Locale::monthName(month, type)

    Returns the localized name of \a month (0-11), in the optional
    \l FormatType specified by \a type.

    \note the QLocale C++ API expects a range of (1-12), however Locale.monthName()
    expects 0-11 as per the JS Date object.

    \sa dayName(), standaloneMonthName()
*/

/*!
    \qmlmethod string QtQuick2::Locale::standaloneMonthName(month, type)

    Returns the localized name of \a month (0-11) that is used as a
    standalone text, in the optional \l FormatType specified by \a type.

    If the locale information doesn't specify the standalone month
    name then return value is the same as in monthName().

    \note the QLocale C++ API expects a range of (1-12), however Locale.standaloneMonthName()
    expects 0-11 as per the JS Date object.

    \sa monthName(), standaloneDayName()
*/

/*!
    \qmlmethod string QtQuick2::Locale::dayName(day, type)

    Returns the localized name of the \a day (where 0 represents
    Sunday, 1 represents Monday and so on), in the optional
    \l FormatType specified by \a type.

    \sa monthName(), standaloneDayName()
*/

/*!
    \qmlmethod string QtQuick2::Locale::standaloneDayName(day, type)

    Returns the localized name of the \a day (where 0 represents
    Sunday, 1 represents Monday and so on) that is used as a
    standalone text, in the \l FormatType specified by \a type.

    If the locale information does not specify the standalone day
    name then return value is the same as in dayName().

    \sa dayName(), standaloneMonthName()
*/

/*!
    \qmlproperty enumeration QtQuick2::Locale::firstDayOfWeek

    Holds the first day of the week according to the current locale.

    \list
    \li Locale.Sunday = 0
    \li Locale.Monday = 1
    \li Locale.Tuesday = 2
    \li Locale.Wednesday = 3
    \li Locale.Thursday = 4
    \li Locale.Friday = 5
    \li Locale.Saturday = 6
    \endlist

    \note that these values match the JS Date API which is different
    from the Qt C++ API where Qt::Sunday = 7.
*/

/*!
    \qmlproperty Array<int> QtQuick2::Locale::weekDays

    Holds an array of days that are considered week days according to the current locale,
    where Sunday is 0 and Saturday is 6.

    \sa firstDayOfWeek
*/

/*!
    \qmlproperty Array<string> QtQuick2::Locale::uiLanguages

    Returns an ordered list of locale names for translation purposes in
    preference order.

    The return value represents locale names that the user expects to see the
    UI translation in.

    The first item in the list is the most preferred one.
*/

/*!
    \qmlproperty enumeration QtQuick2::Locale::textDirection

    Holds the text direction of the language:
    \list
    \li Qt.LeftToRight
    \li Qt.RightToLeft
    \endlist
*/

/*!
    \qmlproperty string QtQuick2::Locale::amText

    The localized name of the "AM" suffix for times specified using the conventions of the 12-hour clock.
*/

/*!
    \qmlproperty string QtQuick2::Locale::pmText

    The localized name of the "PM" suffix for times specified using the conventions of the 12-hour clock.
*/

/*!
    \qmlmethod string QtQuick2::Locale::currencySymbol(format)

    Returns the currency symbol for the specified \a format:
    \list
    \li Locale.CurrencyIsoCode a ISO-4217 code of the currency.
    \li Locale.CurrencySymbol a currency symbol.
    \li Locale.CurrencyDisplayName a user readable name of the currency.
    \endlist
    \sa Number::toLocaleCurrencyString()
*/

/*!
    \qmlproperty string QtQuick2::Locale::nativeLanguageName

    Holds a native name of the language for the locale. For example
    "Schwiizertüütsch" for Swiss-German locale.

    \sa nativeCountryName
*/

/*!
    \qmlproperty string QtQuick2::Locale::nativeCountryName

    Holds a native name of the country for the locale. For example
    "España" for Spanish/Spain locale.

    \sa nativeLanguageName
*/

/*!
    \qmlproperty enumeration QtQuick2::Locale::measurementSystem

    This property defines which units are used for measurement.

    \list
    \li Locale.MetricSystem This value indicates metric units, such as meters,
        centimeters and millimeters.
    \li Locale.ImperialSystem This value indicates imperial units, such as inches and
        miles. There are several distinct imperial systems in the world; this
        value stands for the official United States imperial units.
    \endlist
*/

QT_END_NAMESPACE
