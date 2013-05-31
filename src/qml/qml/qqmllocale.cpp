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
#include <QtCore/qnumeric.h>
#include <QtCore/qdatetime.h>

#include <private/qlocale_p.h>
#include <private/qlocale_data_p.h>

#include <private/qv4dateobject_p.h>
#include <private/qv4numberobject_p.h>
#include <private/qv4stringobject_p.h>

QT_BEGIN_NAMESPACE

class QV4_JS_CLASS(QQmlLocaleData) : public QV4::Object
{
    Q_MANAGED
    QV4_ANNOTATE(managedTypeName QmlLocale staticInitClass true)
public:
    QQmlLocaleData(QV4::ExecutionEngine *engine)
        : QV4::Object(engine)
    {
        vtbl = &static_vtbl;
        type = Type_Object;
    }

    QLocale locale;

    static QLocale &getThisLocale(QV4::SimpleCallContext *ctx) {
        QQmlLocaleData *thisObject = ctx->thisObject.asObject()->as<QQmlLocaleData>();
        if (!thisObject)
            ctx->throwTypeError();
        return thisObject->locale;
    }

    static void initClass(QV4::ExecutionEngine *engine, const QV4::Value &obj);

    static QV4::Value method_currencySymbol(QV4::SimpleCallContext *ctx);
    static QV4::Value method_dateTimeFormat(QV4::SimpleCallContext *ctx);
    static QV4::Value method_timeFormat(QV4::SimpleCallContext *ctx);
    static QV4::Value method_dateFormat(QV4::SimpleCallContext *ctx);
    static QV4::Value method_monthName(QV4::SimpleCallContext *ctx);
    static QV4::Value method_standaloneMonthName(QV4::SimpleCallContext *ctx);
    static QV4::Value method_dayName(QV4::SimpleCallContext *ctx);
    static QV4::Value method_standaloneDayName(QV4::SimpleCallContext *ctx);

    static QV4::Value method_get_firstDayOfWeek(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_measurementSystem(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_textDirection(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_weekDays(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_uiLanguages(QV4::SimpleCallContext *ctx);

    static QV4::Value method_get_name(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_nativeLanguageName(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_nativeCountryName(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_decimalPoint(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_groupSeparator(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_percent(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_zeroDigit(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_negativeSign(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_positiveSign(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_exponential(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_amText(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_pmText(QV4::SimpleCallContext *ctx);

private:
    static void destroy(Managed *that)
    {
        static_cast<QQmlLocaleData *>(that)->~QQmlLocaleData();
    }
};

DEFINE_MANAGED_VTABLE(QQmlLocaleData);

#define GET_LOCALE_DATA_RESOURCE(OBJECT) \
    QQmlLocaleData *r = OBJECT.as<QQmlLocaleData>(); \
    if (!r) \
        V4THROW_ERROR("Not a valid Locale object")

static bool isLocaleObject(const QV4::Value &val)
{
    return val.as<QQmlLocaleData>();
}

//--------------
// Date extension

void QQmlDateExtension::registerExtension(QV4::ExecutionEngine *engine)
{
    engine->datePrototype->defineDefaultProperty(engine, QStringLiteral("toLocaleString"), toLocaleString);
    engine->datePrototype->defineDefaultProperty(engine, QStringLiteral("toLocaleTimeString"), toLocaleTimeString);
    engine->datePrototype->defineDefaultProperty(engine, QStringLiteral("toLocaleDateString"), toLocaleDateString);
    engine->dateCtor.objectValue()->defineDefaultProperty(engine, QStringLiteral("fromLocaleString"), fromLocaleString);
    engine->dateCtor.objectValue()->defineDefaultProperty(engine, QStringLiteral("fromLocaleTimeString"), fromLocaleTimeString);
    engine->dateCtor.objectValue()->defineDefaultProperty(engine, QStringLiteral("fromLocaleDateString"), fromLocaleDateString);
    engine->dateCtor.objectValue()->defineDefaultProperty(engine, QStringLiteral("timeZoneUpdated"), timeZoneUpdated);
}

QV4::Value QQmlDateExtension::toLocaleString(QV4::SimpleCallContext *ctx)
{
    if (ctx->argumentCount > 2)
        return QV4::DatePrototype::method_toLocaleString(ctx);

    QV4::DateObject *date = ctx->thisObject.asDateObject();
    if (!date)
        return QV4::DatePrototype::method_toLocaleString(ctx);

    QDateTime dt = date->toQDateTime();

    if (ctx->argumentCount == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        return QV4::Value::fromString(ctx, locale.toString(dt));
    }

    if (!isLocaleObject(ctx->arguments[0]))
        return QV4::DatePrototype::method_toLocaleString(ctx); // Use the default Date toLocaleString()

    GET_LOCALE_DATA_RESOURCE(ctx->arguments[0]);

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QString formattedDt;
    if (ctx->argumentCount == 2) {
        if (ctx->arguments[1].isString()) {
            QString format = ctx->arguments[1].stringValue()->toQString();
            formattedDt = r->locale.toString(dt, format);
        } else if (ctx->arguments[1].isNumber()) {
            quint32 intFormat = ctx->arguments[1].toNumber();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            formattedDt = r->locale.toString(dt, format);
        } else {
            V4THROW_ERROR("Locale: Date.toLocaleString(): Invalid datetime format");
        }
    } else {
         formattedDt = r->locale.toString(dt, enumFormat);
    }

    return QV4::Value::fromString(ctx, formattedDt);
}

QV4::Value QQmlDateExtension::toLocaleTimeString(QV4::SimpleCallContext *ctx)
{
    if (ctx->argumentCount > 2)
        return QV4::DatePrototype::method_toLocaleTimeString(ctx);

    QV4::DateObject *date = ctx->thisObject.asDateObject();
    if (!date)
        return QV4::DatePrototype::method_toLocaleTimeString(ctx);

    QDateTime dt = date->toQDateTime();
    QTime time = dt.time();

    if (ctx->argumentCount == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        return QV4::Value::fromString(ctx, locale.toString(time));
    }

    if (!isLocaleObject(ctx->arguments[0]))
        return QV4::DatePrototype::method_toLocaleTimeString(ctx); // Use the default Date toLocaleTimeString()

    GET_LOCALE_DATA_RESOURCE(ctx->arguments[0]);

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QString formattedTime;
    if (ctx->argumentCount == 2) {
        if (ctx->arguments[1].isString()) {
            QString format = ctx->arguments[1].stringValue()->toQString();
            formattedTime = r->locale.toString(time, format);
        } else if (ctx->arguments[1].isNumber()) {
            quint32 intFormat = ctx->arguments[1].toNumber();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            formattedTime = r->locale.toString(time, format);
        } else {
            V4THROW_ERROR("Locale: Date.toLocaleTimeString(): Invalid time format");
        }
    } else {
         formattedTime = r->locale.toString(time, enumFormat);
    }

    return QV4::Value::fromString(ctx, formattedTime);
}

QV4::Value QQmlDateExtension::toLocaleDateString(QV4::SimpleCallContext *ctx)
{
    if (ctx->argumentCount > 2)
        return QV4::DatePrototype::method_toLocaleDateString(ctx);

    QV4::DateObject *dateObj = ctx->thisObject.asDateObject();
    if (!dateObj)
        return QV4::DatePrototype::method_toLocaleDateString(ctx);

    QDateTime dt = dateObj->toQDateTime();
    QDate date = dt.date();

    if (ctx->argumentCount == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        return QV4::Value::fromString(ctx, locale.toString(date));
    }

    if (!isLocaleObject(ctx->arguments[0]))
        return QV4::DatePrototype::method_toLocaleDateString(ctx); // Use the default Date toLocaleDateString()

    GET_LOCALE_DATA_RESOURCE(ctx->arguments[0]);

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QString formattedDate;
    if (ctx->argumentCount == 2) {
        if (ctx->arguments[1].isString()) {
            QString format = ctx->arguments[1].stringValue()->toQString();
            formattedDate = r->locale.toString(date, format);
        } else if (ctx->arguments[1].isNumber()) {
            quint32 intFormat = ctx->arguments[1].toNumber();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            formattedDate = r->locale.toString(date, format);
        } else {
            V4THROW_ERROR("Locale: Date.loLocaleDateString(): Invalid date format");
        }
    } else {
         formattedDate = r->locale.toString(date, enumFormat);
    }

    return QV4::Value::fromString(ctx, formattedDate);
}

QV4::Value QQmlDateExtension::fromLocaleString(QV4::SimpleCallContext *ctx)
{
    QV4::ExecutionEngine * const engine = ctx->engine;
    if (ctx->argumentCount == 1 && ctx->arguments[0].isString()) {
        QLocale locale;
        QString dateString = ctx->arguments[0].stringValue()->toQString();
        QDateTime dt = locale.toDateTime(dateString);
        return QV4::Value::fromObject(engine->newDateObject(dt));
    }

    if (ctx->argumentCount < 1 || ctx->argumentCount > 3 || !isLocaleObject(ctx->arguments[0]))
        V4THROW_ERROR("Locale: Date.fromLocaleString(): Invalid arguments");

    GET_LOCALE_DATA_RESOURCE(ctx->arguments[0]);

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QDateTime dt;
    QString dateString = ctx->arguments[1].toQString();
    if (ctx->argumentCount == 3) {
        if (ctx->arguments[2].isString()) {
            QString format = ctx->arguments[2].stringValue()->toQString();
            dt = r->locale.toDateTime(dateString, format);
        } else if (ctx->arguments[2].isNumber()) {
            quint32 intFormat = ctx->arguments[2].toNumber();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            dt = r->locale.toDateTime(dateString, format);
        } else {
            V4THROW_ERROR("Locale: Date.fromLocaleString(): Invalid datetime format");
        }
    } else {
        dt = r->locale.toDateTime(dateString, enumFormat);
    }

    return QV4::Value::fromObject(engine->newDateObject(dt));
}

QV4::Value QQmlDateExtension::fromLocaleTimeString(QV4::SimpleCallContext *ctx)
{
    QV4::ExecutionEngine * const engine = ctx->engine;

    if (ctx->argumentCount == 1 && ctx->arguments[0].isString()) {
        QLocale locale;
        QString timeString = ctx->arguments[0].stringValue()->toQString();
        QTime time = locale.toTime(timeString);
        QDateTime dt = QDateTime::currentDateTime();
        dt.setTime(time);
        return QV4::Value::fromObject(engine->newDateObject(dt));
    }

    if (ctx->argumentCount < 1 || ctx->argumentCount > 3 || !isLocaleObject(ctx->arguments[0]))
        V4THROW_ERROR("Locale: Date.fromLocaleTimeString(): Invalid arguments");

    GET_LOCALE_DATA_RESOURCE(ctx->arguments[0]);

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QTime tm;
    QString dateString = ctx->arguments[1].toQString();
    if (ctx->argumentCount == 3) {
        if (ctx->arguments[2].isString()) {
            QString format = ctx->arguments[2].stringValue()->toQString();
            tm = r->locale.toTime(dateString, format);
        } else if (ctx->arguments[2].isNumber()) {
            quint32 intFormat = ctx->arguments[2].toNumber();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            tm = r->locale.toTime(dateString, format);
        } else {
            V4THROW_ERROR("Locale: Date.fromLocaleTimeString(): Invalid datetime format");
        }
    } else {
        tm = r->locale.toTime(dateString, enumFormat);
    }

    QDateTime dt = QDateTime::currentDateTime();
    dt.setTime(tm);

    return QV4::Value::fromObject(engine->newDateObject(dt));
}

QV4::Value QQmlDateExtension::fromLocaleDateString(QV4::SimpleCallContext *ctx)
{
    QV4::ExecutionEngine * const engine = ctx->engine;

    if (ctx->argumentCount == 1 && ctx->arguments[0].isString()) {
        QLocale locale;
        QString dateString = ctx->arguments[0].stringValue()->toQString();
        QDate date = locale.toDate(dateString);
        return QV4::Value::fromObject(engine->newDateObject(QDateTime(date)));
    }

    if (ctx->argumentCount < 1 || ctx->argumentCount > 3 || !isLocaleObject(ctx->arguments[0]))
        V4THROW_ERROR("Locale: Date.fromLocaleDateString(): Invalid arguments");

    GET_LOCALE_DATA_RESOURCE(ctx->arguments[0]);

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QDate dt;
    QString dateString = ctx->arguments[1].toQString();
    if (ctx->argumentCount == 3) {
        if (ctx->arguments[2].isString()) {
            QString format = ctx->arguments[2].stringValue()->toQString();
            dt = r->locale.toDate(dateString, format);
        } else if (ctx->arguments[2].isNumber()) {
            quint32 intFormat = ctx->arguments[2].toNumber();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            dt = r->locale.toDate(dateString, format);
        } else {
            V4THROW_ERROR("Locale: Date.fromLocaleDateString(): Invalid datetime format");
        }
    } else {
        dt = r->locale.toDate(dateString, enumFormat);
    }

    return QV4::Value::fromObject(engine->newDateObject(QDateTime(dt)));
}

QV4::Value QQmlDateExtension::timeZoneUpdated(QV4::SimpleCallContext *ctx)
{
    if (ctx->argumentCount != 0)
        V4THROW_ERROR("Locale: Date.timeZoneUpdated(): Invalid arguments");

    // ### Anything we need to do here?
    //v8::Date::DateTimeConfigurationChangeNotification();

    return QV4::Value::undefinedValue();
}

//-----------------
// Number extension

void QQmlNumberExtension::registerExtension(QV4::ExecutionEngine *engine)
{
    engine->numberPrototype->defineDefaultProperty(engine, QStringLiteral("toLocaleString"), toLocaleString);
    engine->numberPrototype->defineDefaultProperty(engine, QStringLiteral("toLocaleCurrencyString"), toLocaleCurrencyString);
    engine->numberCtor.objectValue()->defineDefaultProperty(engine, QStringLiteral("fromLocaleString"), fromLocaleString);
}

QV4::Value QQmlNumberExtension::toLocaleString(QV4::SimpleCallContext *ctx)
{
    if (ctx->argumentCount > 3)
        V4THROW_ERROR("Locale: Number.toLocaleString(): Invalid arguments");

    double number = ctx->thisObject.toNumber();

    if (ctx->argumentCount == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        return QV4::Value::fromString(ctx, locale.toString(number));
    }

    if (!isLocaleObject(ctx->arguments[0]))
        return QV4::NumberPrototype::method_toLocaleString(ctx); // Use the default Number toLocaleString()

    GET_LOCALE_DATA_RESOURCE(ctx->arguments[0]);

    uint16_t format = 'f';
    if (ctx->argumentCount > 1) {
        if (!ctx->arguments[1].isString())
            V4THROW_ERROR("Locale: Number.toLocaleString(): Invalid arguments");
        QV4::String *fs = ctx->arguments[1].toString(ctx);
        if (!fs->isEmpty())
            format = fs->toQString().at(0).unicode();
    }
    int prec = 2;
    if (ctx->argumentCount > 2) {
        if (!ctx->arguments[2].isNumber())
            V4THROW_ERROR("Locale: Number.toLocaleString(): Invalid arguments");
         prec = ctx->arguments[2].toInt32();
    }

    return QV4::Value::fromString(ctx, r->locale.toString(number, (char)format, prec));
}

QV4::Value QQmlNumberExtension::toLocaleCurrencyString(QV4::SimpleCallContext *ctx)
{
    if (ctx->argumentCount > 2)
        V4THROW_ERROR("Locale: Number.toLocaleCurrencyString(): Invalid arguments");

    double number = ctx->thisObject.toNumber();

    if (ctx->argumentCount == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        return QV4::Value::fromString(ctx, locale.toString(number));
    }

    if (!isLocaleObject(ctx->arguments[0]))
        V4THROW_ERROR("Locale: Number.toLocaleCurrencyString(): Invalid arguments");

    GET_LOCALE_DATA_RESOURCE(ctx->arguments[0]);

    QString symbol;
    if (ctx->argumentCount > 1) {
        if (!ctx->arguments[1].isString())
            V4THROW_ERROR("Locale: Number.toLocaleString(): Invalid arguments");
        symbol = ctx->arguments[1].toQString();
    }

    return QV4::Value::fromString(ctx, r->locale.toCurrencyString(number, symbol));
}

QV4::Value QQmlNumberExtension::fromLocaleString(QV4::SimpleCallContext *ctx)
{
    if (ctx->argumentCount < 1 || ctx->argumentCount > 2)
        V4THROW_ERROR("Locale: Number.fromLocaleString(): Invalid arguments");

    int numberIdx = 0;
    QLocale locale;

    if (ctx->argumentCount == 2) {
        if (!isLocaleObject(ctx->arguments[0]))
            V4THROW_ERROR("Locale: Number.fromLocaleString(): Invalid arguments");

        GET_LOCALE_DATA_RESOURCE(ctx->arguments[0]);
        locale = r->locale;

        numberIdx = 1;
    }

    QV4::String *ns = ctx->arguments[numberIdx].toString(ctx);
    if (ns->isEmpty())
        return QV4::Value::fromDouble(Q_QNAN);

    bool ok = false;
    double val = locale.toDouble(ns->toQString(), &ok);

    if (!ok)
        V4THROW_ERROR("Locale: Number.fromLocaleString(): Invalid format")

    return QV4::Value::fromDouble(val);
}

//--------------
// Locale object

QV4::Value QQmlLocaleData::method_get_firstDayOfWeek(QV4::SimpleCallContext *ctx)
{
    QLocale locale = getThisLocale(ctx);
    int fdow = int(locale.firstDayOfWeek());
    if (fdow == 7)
        fdow = 0; // Qt::Sunday = 7, but Sunday is 0 in JS Date
    return QV4::Value::fromInt32(fdow);
}

QV4::Value QQmlLocaleData::method_get_measurementSystem(QV4::SimpleCallContext *ctx)
{
    QLocale locale = getThisLocale(ctx);
    return QV4::Value::fromInt32(locale.measurementSystem());
}

QV4::Value QQmlLocaleData::method_get_textDirection(QV4::SimpleCallContext *ctx)
{
    QLocale locale = getThisLocale(ctx);
    return QV4::Value::fromInt32(locale.textDirection());
}

QV4::Value QQmlLocaleData::method_get_weekDays(QV4::SimpleCallContext *ctx)
{
    QLocale locale = getThisLocale(ctx);
    QList<Qt::DayOfWeek> days = locale.weekdays();

    QV4::ArrayObject *result = ctx->engine->newArrayObject();
    result->arrayReserve(days.size());
    for (int i = 0; i < days.size(); ++i) {
        int day = days.at(i);
        if (day == 7) // JS Date days in range 0(Sunday) to 6(Saturday)
            day = 0;
        result->arrayData[i].value = QV4::Value::fromInt32(day);
    }
    result->setArrayLengthUnchecked(days.size());

    return QV4::Value::fromObject(result);
}

QV4::Value QQmlLocaleData::method_get_uiLanguages(QV4::SimpleCallContext *ctx)
{
    QLocale locale = getThisLocale(ctx);
    QStringList langs = locale.uiLanguages();
    QV4::ArrayObject *result = ctx->engine->newArrayObject();
    result->arrayReserve(langs.size());
    for (int i = 0; i < langs.size(); ++i)
        result->arrayData[i].value = QV4::Value::fromString(ctx, langs.at(i));
    result->setArrayLengthUnchecked(langs.size());

    return QV4::Value::fromObject(result);
}

QV4::Value QQmlLocaleData::method_currencySymbol(QV4::SimpleCallContext *ctx)
{
    QLocale locale = getThisLocale(ctx);
    if (ctx->argumentCount > 1)
        V4THROW_ERROR("Locale: currencySymbol(): Invalid arguments");

    QLocale::CurrencySymbolFormat format = QLocale::CurrencySymbol;
    if (ctx->argumentCount == 1) {
        quint32 intFormat = ctx->arguments[0].toNumber();
        format = QLocale::CurrencySymbolFormat(intFormat);
    }

    return QV4::Value::fromString(ctx, locale.currencySymbol(format));
}

#define LOCALE_FORMAT(FUNC) \
QV4::Value QQmlLocaleData::method_ ##FUNC (QV4::SimpleCallContext *ctx) { \
    QLocale locale = getThisLocale(ctx); \
    if (ctx->argumentCount > 1) \
        V4THROW_ERROR("Locale: " #FUNC "(): Invalid arguments"); \
    QLocale::FormatType format = QLocale::LongFormat;\
    if (ctx->argumentCount == 1) { \
        quint32 intFormat = ctx->arguments[0].toUInt32(); \
        format = QLocale::FormatType(intFormat); \
    } \
    return QV4::Value::fromString(ctx, locale. FUNC (format)); \
}

LOCALE_FORMAT(dateTimeFormat)
LOCALE_FORMAT(timeFormat)
LOCALE_FORMAT(dateFormat)

// +1 added to idx because JS is 0-based, whereas QLocale months begin at 1.
#define LOCALE_FORMATTED_MONTHNAME(VARIABLE) \
QV4::Value QQmlLocaleData::method_ ## VARIABLE (QV4::SimpleCallContext *ctx) {\
    QLocale locale = getThisLocale(ctx); \
    if (ctx->argumentCount < 1 || ctx->argumentCount > 2) \
        V4THROW_ERROR("Locale: " #VARIABLE "(): Invalid arguments"); \
    QLocale::FormatType enumFormat = QLocale::LongFormat; \
    int idx = ctx->arguments[0].toInt32() + 1; \
    if (idx < 1 || idx > 12) \
        V4THROW_ERROR("Locale: Invalid month"); \
    QString name; \
    if (ctx->argumentCount == 2) { \
        if (ctx->arguments[1].isNumber()) { \
            quint32 intFormat = ctx->arguments[1].toUInt32(); \
            QLocale::FormatType format = QLocale::FormatType(intFormat); \
            name = locale. VARIABLE(idx, format); \
        } else { \
            V4THROW_ERROR("Locale: Invalid datetime format"); \
        } \
    } else { \
        name = locale. VARIABLE(idx, enumFormat); \
    } \
    return QV4::Value::fromString(ctx, name); \
}

// 0 -> 7 as Qt::Sunday is 7, but Sunday is 0 in JS Date
#define LOCALE_FORMATTED_DAYNAME(VARIABLE) \
QV4::Value QQmlLocaleData::method_ ## VARIABLE (QV4::SimpleCallContext *ctx) {\
    QLocale locale = getThisLocale(ctx); \
    if (ctx->argumentCount < 1 || ctx->argumentCount > 2) \
        V4THROW_ERROR("Locale: " #VARIABLE "(): Invalid arguments"); \
    QLocale::FormatType enumFormat = QLocale::LongFormat; \
    int idx = ctx->arguments[0].toInt32(); \
    if (idx < 0 || idx > 7) \
        V4THROW_ERROR("Locale: Invalid day"); \
    if (idx == 0) idx = 7; \
    QString name; \
    if (ctx->argumentCount == 2) { \
        if (ctx->arguments[1].isNumber()) { \
            quint32 intFormat = ctx->arguments[1].toUInt32(); \
            QLocale::FormatType format = QLocale::FormatType(intFormat); \
            name = locale. VARIABLE(idx, format); \
        } else { \
            V4THROW_ERROR("Locale: Invalid datetime format"); \
        } \
    } else { \
        name = locale. VARIABLE(idx, enumFormat); \
    } \
    return QV4::Value::fromString(ctx, name); \
}

LOCALE_FORMATTED_MONTHNAME(monthName)
LOCALE_FORMATTED_MONTHNAME(standaloneMonthName)
LOCALE_FORMATTED_DAYNAME(dayName)
LOCALE_FORMATTED_DAYNAME(standaloneDayName)

#define LOCALE_STRING_PROPERTY(VARIABLE) QV4::Value QQmlLocaleData::method_get_ ## VARIABLE (QV4::SimpleCallContext* ctx) \
{ \
    QLocale locale = getThisLocale(ctx); \
    return QV4::Value::fromString(ctx, locale. VARIABLE());\
}

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

    QV4::PersistentValue prototype;
};

QV8LocaleDataDeletable::QV8LocaleDataDeletable(QV8Engine *engine)
{
    QV4::ExecutionEngine *eng = QV8Engine::getV4(engine);
    prototype = QV4::Value::fromObject(eng->newObject());
    QQmlLocaleData::initClass(eng, prototype.value());
}

QV8LocaleDataDeletable::~QV8LocaleDataDeletable()
{
}

V8_DEFINE_EXTENSION(QV8LocaleDataDeletable, localeV8Data);

/*!
    \qmltype Locale
    \instantiates QQmlLocale
    \inqmlmodule QtQuick 2
    \brief Provides locale specific properties and formatted data

    The Locale object may only be created via the \l{QML:Qt::locale()}{Qt.locale()} function.
    It cannot be created directly.

    The \l{QML:Qt::locale()}{Qt.locale()} function returns a JS Locale object representing the
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

    QtQuick Locale's data is based on Common Locale Data Repository v1.8.1.


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

QV4::Value QQmlLocale::locale(QV8Engine *v8engine, const QString &locale)
{
    QV8LocaleDataDeletable *d = localeV8Data(v8engine);
    QV4::ExecutionEngine *engine = QV8Engine::getV4(v8engine);
    QQmlLocaleData *wrapper = new (engine->memoryManager) QQmlLocaleData(engine);
    if (!locale.isEmpty())
        wrapper->locale = QLocale(locale);
    wrapper->prototype = d->prototype.value().asObject();
    return QV4::Value::fromObject(wrapper);
}

void QQmlLocale::registerStringLocaleCompare(QV4::ExecutionEngine *engine)
{
    engine->stringPrototype->defineDefaultProperty(engine, QStringLiteral("localeCompare"), localeCompare);
}

QV4::Value QQmlLocale::localeCompare(QV4::SimpleCallContext *ctx)
{
    if (ctx->argumentCount != 1 || (!ctx->arguments[0].isString() && !ctx->arguments[0].asStringObject()))
        return QV4::StringPrototype::method_localeCompare(ctx);

    if (!ctx->thisObject.isString() && !ctx->thisObject.asStringObject())
        return QV4::StringPrototype::method_localeCompare(ctx);

    QString thisString = ctx->thisObject.toQString();
    QString thatString = ctx->arguments[0].toQString();

    return QV4::Value::fromInt32(QString::localeAwareCompare(thisString, thatString));
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

#include "qqmllocale_jsclass.cpp"

QT_END_NAMESPACE
