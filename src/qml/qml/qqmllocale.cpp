// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmllocale_p.h"
#include <private/qqmlcontext_p.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qtimezone.h>

#include <private/qlocale_p.h>
#include <private/qlocale_data_p.h>

#include <private/qv4dateobject_p.h>
#include <private/qv4numberobject_p.h>
#include <private/qv4stringobject_p.h>
#include <private/qqmlvaluetypewrapper_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

#define THROW_ERROR(string) \
    do { \
        return scope.engine->throwError(QString::fromUtf8(string)); \
    } while (false)


#define GET_LOCALE_DATA_RESOURCE(OBJECT) \
    QLocale *r = [&]() { \
        QV4::Scoped<QQmlValueTypeWrapper> r(scope, OBJECT.as<QQmlValueTypeWrapper>()); \
        return r ? r->cast<QLocale>() : nullptr; \
    }(); \
    if (!r) \
        THROW_ERROR("Not a valid Locale object")

static bool isLocaleObject(const QV4::Value &val)
{
    if (const QV4::QQmlValueTypeWrapper *wrapper = val.as<QQmlValueTypeWrapper>())
        return wrapper->type() == QMetaType::fromType<QLocale>();
    return false;
}

//--------------
// Date extension

void QQmlDateExtension::registerExtension(QV4::ExecutionEngine *engine)
{
    engine->datePrototype()->defineDefaultProperty(engine->id_toLocaleString(), method_toLocaleString);
    engine->datePrototype()->defineDefaultProperty(QStringLiteral("toLocaleTimeString"), method_toLocaleTimeString);
    engine->datePrototype()->defineDefaultProperty(QStringLiteral("toLocaleDateString"), method_toLocaleDateString);
    engine->dateCtor()->defineDefaultProperty(QStringLiteral("fromLocaleString"), method_fromLocaleString);
    engine->dateCtor()->defineDefaultProperty(QStringLiteral("fromLocaleTimeString"), method_fromLocaleTimeString);
    engine->dateCtor()->defineDefaultProperty(QStringLiteral("fromLocaleDateString"), method_fromLocaleDateString);
    engine->dateCtor()->defineDefaultProperty(QStringLiteral("timeZoneUpdated"), method_timeZoneUpdated);
}

ReturnedValue QQmlDateExtension::method_toLocaleString(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *argv, int argc)
{
    Scope scope(b);
    if (argc > 2)
        return QV4::DatePrototype::method_toLocaleString(b, thisObject, argv, argc);

    const QV4::DateObject *date = thisObject->as<DateObject>();
    if (!date)
        return QV4::DatePrototype::method_toLocaleString(b, thisObject, argv, argc);

    QDateTime dt = date->toQDateTime();

    if (argc == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        RETURN_RESULT(scope.engine->newString(locale.toString(dt)));
    }

    if (!isLocaleObject(argv[0]))
        return QV4::DatePrototype::method_toLocaleString(b, thisObject, argv, argc); // Use the default Date toLocaleString()

    GET_LOCALE_DATA_RESOURCE(argv[0]);

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QString formattedDt;
    if (argc == 2) {
        if (String *s = argv[1].stringValue()) {
            QString format = s->toQString();
            formattedDt = r->toString(dt, format);
        } else if (argv[1].isNumber()) {
            quint32 intFormat = argv[1].toNumber();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            formattedDt = r->toString(dt, format);
        } else {
            THROW_ERROR("Locale: Date.toLocaleString(): Invalid datetime format");
        }
    } else {
         formattedDt = r->toString(dt, enumFormat);
    }

    RETURN_RESULT(scope.engine->newString(formattedDt));
}

ReturnedValue QQmlDateExtension::method_toLocaleTimeString(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *argv, int argc)
{
    Scope scope(b);
    if (argc > 2)
        return QV4::DatePrototype::method_toLocaleTimeString(b, thisObject, argv, argc);

    const QV4::DateObject *date = thisObject->as<DateObject>();
    if (!date)
        return QV4::DatePrototype::method_toLocaleTimeString(b, thisObject, argv, argc);

    QDateTime dt = date->toQDateTime();
    QTime time = dt.time();

    if (argc == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        RETURN_RESULT(scope.engine->newString(locale.toString(time)));
    }

    if (!isLocaleObject(argv[0]))
        return QV4::DatePrototype::method_toLocaleTimeString(b, thisObject, argv, argc); // Use the default Date toLocaleTimeString()

    GET_LOCALE_DATA_RESOURCE(argv[0]);

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QString formattedTime;
    if (argc == 2) {
        if (String *s = argv[1].stringValue()) {
            QString format = s->toQString();
            formattedTime = r->toString(time, format);
        } else if (argv[1].isNumber()) {
            quint32 intFormat = argv[1].toNumber();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            formattedTime = r->toString(time, format);
        } else {
            THROW_ERROR("Locale: Date.toLocaleTimeString(): Invalid time format");
        }
    } else {
         formattedTime = r->toString(time, enumFormat);
    }

    RETURN_RESULT(scope.engine->newString(formattedTime));
}

ReturnedValue QQmlDateExtension::method_toLocaleDateString(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *argv, int argc)
{
    Scope scope(b);
    if (argc > 2)
        return QV4::DatePrototype::method_toLocaleDateString(b, thisObject, argv, argc);

    const QV4::DateObject *dateObj = thisObject->as<DateObject>();
    if (!dateObj)
        return QV4::DatePrototype::method_toLocaleDateString(b, thisObject, argv, argc);

    QDateTime dt = dateObj->toQDateTime();
    QDate date = dt.date();

    if (argc == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        RETURN_RESULT(scope.engine->newString(locale.toString(date)));
    }

    if (!isLocaleObject(argv[0]))
        return QV4::DatePrototype::method_toLocaleDateString(b, thisObject, argv, argc); // Use the default Date toLocaleDateString()

    GET_LOCALE_DATA_RESOURCE(argv[0]);

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QString formattedDate;
    if (argc == 2) {
        if (String *s = argv[1].stringValue()) {
            QString format = s->toQString();
            formattedDate = r->toString(date, format);
        } else if (argv[1].isNumber()) {
            quint32 intFormat = argv[1].toNumber();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            formattedDate = r->toString(date, format);
        } else {
            THROW_ERROR("Locale: Date.loLocaleDateString(): Invalid date format");
        }
    } else {
         formattedDate = r->toString(date, enumFormat);
    }

    RETURN_RESULT(scope.engine->newString(formattedDate));
}

ReturnedValue QQmlDateExtension::method_fromLocaleString(const QV4::FunctionObject *b, const QV4::Value *, const QV4::Value *argv, int argc)
{
    QV4::Scope scope(b);
    QV4::ExecutionEngine * const engine = scope.engine;
    if (argc == 1) {
        if (String *s = argv[0].stringValue()) {
            QLocale locale;
            QString dateString = s->toQString();
            QDateTime dt = locale.toDateTime(dateString);
            RETURN_RESULT(engine->newDateObject(dt));
        }
    }

    if (argc < 1 || argc > 3 || !isLocaleObject(argv[0]))
        THROW_ERROR("Locale: Date.fromLocaleString(): Invalid arguments");

    GET_LOCALE_DATA_RESOURCE(argv[0]);

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QDateTime dt;
    QString dateString = argv[1].toQStringNoThrow();
    if (argc == 3) {
        if (String *s = argv[2].stringValue()) {
            QString format = s->toQString();
            dt = r->toDateTime(dateString, format);
        } else if (argv[2].isNumber()) {
            quint32 intFormat = argv[2].toNumber();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            dt = r->toDateTime(dateString, format);
        } else {
            THROW_ERROR("Locale: Date.fromLocaleString(): Invalid datetime format");
        }
    } else {
        dt = r->toDateTime(dateString, enumFormat);
    }

    RETURN_RESULT(engine->newDateObject(dt));
}

ReturnedValue QQmlDateExtension::method_fromLocaleTimeString(const QV4::FunctionObject *b, const QV4::Value *, const QV4::Value *argv, int argc)
{
    QV4::Scope scope(b);
    QV4::ExecutionEngine * const engine = scope.engine;

    if (argc == 1) {
        if (String *s = argv[0].stringValue()) {
            QLocale locale;
            QString timeString = s->toQString();
            QTime time = locale.toTime(timeString);
            QDateTime dt = QDateTime::currentDateTime();
            dt.setTime(time);
            RETURN_RESULT(engine->newDateObject(dt));
        }
    }

    if (argc < 1 || argc > 3 || !isLocaleObject(argv[0]))
        THROW_ERROR("Locale: Date.fromLocaleTimeString(): Invalid arguments");

    GET_LOCALE_DATA_RESOURCE(argv[0]);

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QTime tm;
    QString dateString = argv[1].toQStringNoThrow();
    if (argc == 3) {
        if (String *s = argv[2].stringValue()) {
            QString format = s->toQString();
            tm = r->toTime(dateString, format);
        } else if (argv[2].isNumber()) {
            quint32 intFormat = argv[2].toNumber();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            tm = r->toTime(dateString, format);
        } else {
            THROW_ERROR("Locale: Date.fromLocaleTimeString(): Invalid datetime format");
        }
    } else {
        tm = r->toTime(dateString, enumFormat);
    }

    QDateTime dt;
    if (tm.isValid()) {
        dt = QDateTime::currentDateTime();
        dt.setTime(tm);
    }

    RETURN_RESULT(engine->newDateObject(dt));
}

ReturnedValue QQmlDateExtension::method_fromLocaleDateString(const QV4::FunctionObject *b, const QV4::Value *, const QV4::Value *argv, int argc)
{
    QV4::Scope scope(b);
    QV4::ExecutionEngine * const engine = scope.engine;

    if (argc == 1) {
        if (String *s = argv[0].stringValue()) {
            QLocale locale;
            QString dateString = s->toQString();
            QDate date = locale.toDate(dateString);
            RETURN_RESULT(engine->newDateObject(date.startOfDay(QTimeZone::UTC)));
        }
    }

    if (argc < 1 || argc > 3 || !isLocaleObject(argv[0]))
        THROW_ERROR("Locale: Date.fromLocaleDateString(): Invalid arguments");

    GET_LOCALE_DATA_RESOURCE(argv[0]);

    QLocale::FormatType enumFormat = QLocale::LongFormat;
    QDate dt;
    QString dateString = argv[1].toQStringNoThrow();
    if (argc == 3) {
        if (String *s = argv[2].stringValue()) {
            QString format = s->toQString();
            dt = r->toDate(dateString, format);
        } else if (argv[2].isNumber()) {
            quint32 intFormat = argv[2].toNumber();
            QLocale::FormatType format = QLocale::FormatType(intFormat);
            dt = r->toDate(dateString, format);
        } else {
            THROW_ERROR("Locale: Date.fromLocaleDateString(): Invalid datetime format");
        }
    } else {
        dt = r->toDate(dateString, enumFormat);
    }

    RETURN_RESULT(engine->newDateObject(dt.startOfDay(QTimeZone::UTC)));
}

ReturnedValue QQmlDateExtension::method_timeZoneUpdated(const QV4::FunctionObject *b, const QV4::Value *, const QV4::Value *, int argc)
{
    QV4::Scope scope(b);
    if (argc != 0)
        THROW_ERROR("Locale: Date.timeZoneUpdated(): Invalid arguments");

    QV4::DatePrototype::timezoneUpdated(scope.engine);

    RETURN_UNDEFINED();
}

//-----------------
// Number extension

void QQmlNumberExtension::registerExtension(QV4::ExecutionEngine *engine)
{
    engine->numberPrototype()->defineDefaultProperty(engine->id_toLocaleString(), method_toLocaleString);
    engine->numberPrototype()->defineDefaultProperty(QStringLiteral("toLocaleCurrencyString"), method_toLocaleCurrencyString);
    engine->numberCtor()->defineDefaultProperty(QStringLiteral("fromLocaleString"), method_fromLocaleString);
}

QV4::ReturnedValue QQmlNumberExtension::method_toLocaleString(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *argv, int argc)
{
    QV4::Scope scope(b);
    if (argc > 3)
        THROW_ERROR("Locale: Number.toLocaleString(): Invalid arguments");

    double number = thisObject->toNumber();

    if (argc == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        RETURN_RESULT(scope.engine->newString(locale.toString(number)));
    }

    if (!isLocaleObject(argv[0]))
        return QV4::NumberPrototype::method_toLocaleString(b, thisObject, argv, argc); // Use the default Number toLocaleString()

    GET_LOCALE_DATA_RESOURCE(argv[0]);

    quint16 format = 'f';
    if (argc > 1) {
        if (!argv[1].isString())
            THROW_ERROR("Locale: Number.toLocaleString(): Invalid arguments");
        QString fs = argv[1].toQString();
        if (fs.size())
            format = fs.at(0).unicode();
    }
    int prec = 2;
    if (argc > 2) {
        if (!argv[2].isNumber())
            THROW_ERROR("Locale: Number.toLocaleString(): Invalid arguments");
         prec = argv[2].toInt32();
    }

    RETURN_RESULT(scope.engine->newString(r->toString(number, (char)format, prec)));
}

ReturnedValue QQmlNumberExtension::method_toLocaleCurrencyString(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *argv, int argc)
{
    QV4::Scope scope(b);
    if (argc > 2)
        THROW_ERROR("Locale: Number.toLocaleCurrencyString(): Invalid arguments");

    double number = thisObject->toNumber();

    if (argc == 0) {
        // Use QLocale for standard toLocaleString() function
        QLocale locale;
        RETURN_RESULT(scope.engine->newString(locale.toString(number)));
    }

    if (!isLocaleObject(argv[0]))
        THROW_ERROR("Locale: Number.toLocaleCurrencyString(): Invalid arguments");

    GET_LOCALE_DATA_RESOURCE(argv[0]);

    QString symbol;
    if (argc > 1) {
        if (!argv[1].isString())
            THROW_ERROR("Locale: Number.toLocaleString(): Invalid arguments");
        symbol = argv[1].toQStringNoThrow();
    }

    RETURN_RESULT(scope.engine->newString(r->toCurrencyString(number, symbol)));
}

ReturnedValue QQmlNumberExtension::method_fromLocaleString(const QV4::FunctionObject *b, const QV4::Value *, const QV4::Value *argv, int argc)
{
    QV4::Scope scope(b);
    if (argc < 1 || argc > 2)
        THROW_ERROR("Locale: Number.fromLocaleString(): Invalid arguments");

    int numberIdx = 0;
    QLocale locale;

    if (argc == 2) {
        if (!isLocaleObject(argv[0]))
            THROW_ERROR("Locale: Number.fromLocaleString(): Invalid arguments");

        GET_LOCALE_DATA_RESOURCE(argv[0]);
        locale = *r;

        numberIdx = 1;
    }

    QString ns = argv[numberIdx].toQString();
    if (!ns.size())
        RETURN_RESULT(QV4::Encode(Q_QNAN));

    bool ok = false;
    double val = locale.toDouble(ns, &ok);

    if (!ok)
        THROW_ERROR("Locale: Number.fromLocaleString(): Invalid format");

    RETURN_RESULT(QV4::Encode(val));
}

//--------------
// Locale object

void QQmlLocaleValueType::formattedDataSize(QQmlV4Function *args) const
{
    QV4::Scope scope(args->v4engine());
    const auto doThrow = [&](const QString &message) {
        args->setReturnValue(scope.engine->throwError(message));
    };

    const int argc = args->length();

    if (argc < 1 || argc > 3) {
        doThrow(QString::fromLatin1(
                        "Locale: formattedDataSize(): Expected 1-3 arguments, but received %1")
                        .arg(argc));
        return;
    }

    QV4::ScopedValue arg0(scope, (*args)[0]);
    bool mismatched0 = false;
    if (!arg0->isNumber()) {
        // ### Qt7: Throw an exception here, so that we don't have to handle mismatched0 below.
        qWarning() << "Locale: formattedDataSize(): Invalid argument ('bytes' should be a number)";
        if (argc == 1) {
            args->setReturnValue(
                    scope.engine->newString(locale.formattedDataSize(qint64(arg0->toInteger())))
                            ->asReturnedValue());
            return;
        }

        mismatched0 = true;
    }

    // Anything can be coerced to a number, for better or worse ...
    Q_ASSERT(argc >= 2);

    QV4::ScopedValue arg1(scope, (*args)[1]);
    if (!arg1->isInteger()) {
        doThrow(QLatin1String(
                "Locale: formattedDataSize(): Invalid argument ('precision' must be an int)"));
        return;
    }

    if (mismatched0) {
        if (argc == 2) {
            const QString result = locale.formattedDataSize(
                    qint64(arg0->toInteger()), arg1->integerValue());
            args->setReturnValue(scope.engine->newString(result)->asReturnedValue());
            return;
        }

        QV4::ScopedValue arg2(scope, (*args)[2]);
        if (arg2->isNumber()) {
            const QString result = locale.formattedDataSize(
                    qint64(arg0->toInteger()), arg1->integerValue(),
                    QLocale::DataSizeFormats(arg2->integerValue()));
            args->setReturnValue(scope.engine->newString(result)->asReturnedValue());
            return;
        }
    }

    Q_ASSERT(argc == 3);
    Q_ASSERT(!QV4::ScopedValue(scope, (*args)[2])->isNumber());

    doThrow(QLatin1String(
            "Locale: formattedDataSize(): Invalid argument ('format' must be DataSizeFormat)"));
}

static QQmlLocale::DayOfWeek qtDayToQmlDay(Qt::DayOfWeek day)
{
    return day == Qt::Sunday ? QQmlLocale::DayOfWeek::Sunday : QQmlLocale::DayOfWeek(day);
}

QQmlLocale::DayOfWeek QQmlLocaleValueType::firstDayOfWeek() const
{
    return qtDayToQmlDay(locale.firstDayOfWeek());
}

QList<QQmlLocale::DayOfWeek> QQmlLocaleValueType::weekDays() const
{
    const QList<Qt::DayOfWeek> days = locale.weekdays();
    QList<QQmlLocale::DayOfWeek> result;
    result.reserve(days.size());
    for (Qt::DayOfWeek day : days)
        result.append(qtDayToQmlDay(day));
    return result;
}

void QQmlLocaleValueType::toString(QQmlV4Function *args) const
{
    Scope scope(args->v4engine());
    const auto doThrow = [&](const QString &message) {
        args->setReturnValue(scope.engine->throwError(message));
    };

    const int argc = args->length();

    // toString()
    Q_ASSERT(argc > 0);

    if (argc > 3) {
        doThrow(QString::fromLatin1("Locale: toString(): Expected 1-3 arguments, but received %1")
                        .arg(argc));
        return;
    }

    QV4::ScopedValue arg0(scope, (*args)[0]);
    if (arg0->isNumber()) {

        // toString(int)
        // toString(double)
        Q_ASSERT(argc != 1);

        QV4::ScopedValue arg1(scope, (*args)[1]);
        if (!arg1->isString()) {
            doThrow(QLatin1String("Locale: the second argument to the toString overload "
                                  "whose first argument is a double should be a char"));
            return;
        }

        // toString(double, const QString &)
        // toString(double, const QString &, int)
        Q_ASSERT(argc == 3);
        Q_ASSERT(!QV4::ScopedValue(scope, (*args)[2])->isInteger());

        doThrow(QLatin1String("Locale: the third argument to the toString overload "
                              "whose first argument is a double should be an int"));
        return;
    }

    if (arg0->as<DateObject>()) {
        if (argc > 2) {
            doThrow(QString::fromLatin1(
                            "Locale: the toString() overload that takes a Date as its first "
                            "argument expects 1 or 2 arguments, but received %1").arg(argc));
            return;
        }

        // toString(QDateTime)
        Q_ASSERT(argc == 2);
        QV4::ScopedValue arg1(scope, (*args)[1]);

        // toString(QDateTime, QString)
        Q_ASSERT(!arg1->isString());

        // toString(QDateTime, QLocale::FormatType)
        Q_ASSERT(!arg1->isNumber());

        doThrow(QLatin1String("Locale: the second argument to the toString overloads whose "
                              "first argument is a Date should be a string or FormatType"));
        return;
    }

    doThrow(QLatin1String("Locale: toString() expects either an int, double, "
                          "or Date as its first argument"));
}

/*!
    \qmltype Locale
    //! \instantiates QQmlLocale
    \inqmlmodule QtQml
    \brief Provides locale specific properties and formatted data.

    The Locale object may only be created via the \l{QtQml::Qt::locale()}{Qt.locale()} function.
    It cannot be created directly.

    The \l{QtQml::Qt::locale()}{Qt.locale()} function returns a JS Locale object representing the
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

    \value Locale.LongFormat    The long version of day and month names; for
        example, returning "January" as a month name.
    \value Locale.ShortFormat   The short version of day and month names; for
        example, returning "Jan" as a month name.
    \value Locale.NarrowFormat  A special version of day and month names for
        use when space is limited; for example, returning "J" as a month
        name. Note that the narrow format might contain the same text for
        different months and days or it can even be an empty string if the
        locale doesn't support narrow names, so you should avoid using it
        for date formatting. Also, for the system locale this format is
        the same as ShortFormat.


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

    \sa Date, Number
*/

QV4::ReturnedValue QQmlLocale::locale(ExecutionEngine *engine, const QString &localeName)
{
    if (localeName.isEmpty()) {
        return QQmlValueTypeWrapper::create(
                engine, nullptr, &QQmlLocaleValueType::staticMetaObject,
                QMetaType::fromType<QLocale>());
    }

    QLocale qlocale(localeName);
    return QQmlValueTypeWrapper::create(
            engine, &qlocale, &QQmlLocaleValueType::staticMetaObject,
            QMetaType::fromType<QLocale>());
}

void QQmlLocale::registerStringLocaleCompare(QV4::ExecutionEngine *engine)
{
    engine->stringPrototype()->defineDefaultProperty(QStringLiteral("localeCompare"), method_localeCompare);
}

ReturnedValue QQmlLocale::method_localeCompare(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *argv, int argc)
{
    if (argc != 1 || (!argv[0].isString() && !argv[0].as<StringObject>()))
        return QV4::StringPrototype::method_localeCompare(b, thisObject, argv, argc);

    if (!thisObject->isString() && !thisObject->as<StringObject>())
        return QV4::StringPrototype::method_localeCompare(b, thisObject, argv, argc);

    QString thisString = thisObject->toQStringNoThrow();
    QString thatString = argv[0].toQStringNoThrow();

    return QV4::Encode(QString::localeAwareCompare(thisString, thatString));
}

/*!
    \qmlproperty string QtQml::Locale::name

    Holds the language and territory of this locale as a
    string of the form "language_territory", where
    language is a lowercase, two-letter ISO 639 language code,
    and territory is an uppercase, two- or three-letter ISO 3166 territory code.
*/

/*!
    \qmlproperty string QtQml::Locale::decimalPoint

    Holds the decimal point character of this locale.
*/

/*!
    \qmlproperty string QtQml::Locale::groupSeparator

    Holds the group separator character of this locale.
*/

/*!
    \qmlproperty enumeration QtQml::Locale::numberOptions

    Holds a set of options for number-to-string and
    string-to-number conversions.

    \sa Number::toLocaleString()
    \sa Number::fromLocaleString()
*/

/*!
    \qmlproperty string QtQml::Locale::percent

    Holds the percent character of this locale.
*/


/*!
    \qmlproperty string QtQml::Locale::zeroDigit

    Holds Returns the zero digit character of this locale.
*/

/*!
    \qmlproperty string QtQml::Locale::negativeSign

    Holds the negative sign character of this locale.
*/

/*!
    \qmlproperty string QtQml::Locale::positiveSign

    Holds the positive sign character of this locale.
*/

/*!
    \qmlproperty string QtQml::Locale::exponential

    Holds the exponential character of this locale.
*/

/*!
    \qmlmethod string QtQml::Locale::dateTimeFormat(type)

    Returns the date time format used for the current locale.
    \a type specifies the FormatType to return.

    \sa Date
*/

/*!
    \qmlmethod string QtQml::Locale::dateFormat(type)

    Returns the date format used for the current locale.
    \a type specifies the FormatType to return.

    \sa Date
*/

/*!
    \qmlmethod string QtQml::Locale::timeFormat(type)

    Returns the time format used for the current locale.
    \a type specifies the FormatType to return.

    \sa Date
*/

/*!
    \qmlmethod string QtQml::Locale::formattedDataSize(int bytes, int precision, DataSizeFormat format)
    \since 6.2

    Converts a size in \a bytes to a human-readable localized string, comprising a
    number and a quantified unit.

    The \a precision and \a format arguments are optional.

    For more information, see \l QLocale::formattedDataSize().

    \sa QLocale::DataSizeFormats
*/

/*!
    \qmlmethod string QtQml::Locale::monthName(month, type)

    Returns the localized name of \a month (0-11), in the optional
    \l FormatType specified by \a type.

    \note the QLocale C++ API expects a range of (1-12), however Locale.monthName()
    expects 0-11 as per the JS Date object.

    \sa dayName(), standaloneMonthName()
*/

/*!
    \qmlmethod string QtQml::Locale::standaloneMonthName(month, type)

    Returns the localized name of \a month (0-11) that is used as a
    standalone text, in the optional \l FormatType specified by \a type.

    If the locale information doesn't specify the standalone month
    name then return value is the same as in monthName().

    \note the QLocale C++ API expects a range of (1-12), however Locale.standaloneMonthName()
    expects 0-11 as per the JS Date object.

    \sa monthName(), standaloneDayName()
*/

/*!
    \qmlmethod string QtQml::Locale::dayName(day, type)

    Returns the localized name of the \a day (where 0 represents
    Sunday, 1 represents Monday and so on), in the optional
    \l FormatType specified by \a type.

    \sa monthName(), standaloneDayName()
*/

/*!
    \qmlmethod string QtQml::Locale::standaloneDayName(day, type)

    Returns the localized name of the \a day (where 0 represents
    Sunday, 1 represents Monday and so on) that is used as a
    standalone text, in the \l FormatType specified by \a type.

    If the locale information does not specify the standalone day
    name then return value is the same as in dayName().

    \sa dayName(), standaloneMonthName()
*/

/*!
    \qmlproperty enumeration QtQml::Locale::firstDayOfWeek

    Holds the first day of the week according to the current locale.

    \value Locale.Sunday    0
    \value Locale.Monday    1
    \value Locale.Tuesday   2
    \value Locale.Wednesday 3
    \value Locale.Thursday  4
    \value Locale.Friday    5
    \value Locale.Saturday  6

    \note that these values match the JS Date API which is different
    from the Qt C++ API where Qt::Sunday = 7.
*/

/*!
    \qmlproperty Array<int> QtQml::Locale::weekDays

    Holds an array of days that are considered week days according to the current locale,
    where Sunday is 0 and Saturday is 6.

    \sa firstDayOfWeek
*/

/*!
    \qmlmethod string QtQml::Locale::toString(int i)
    \since 6.5

    Returns a localized string representation of \a i.

    \sa QLocale::toString(int)
*/

/*!
    \qmlmethod string QtQml::Locale::toString(double f, char format = 'g', int precision = 6)
    \overload
    \since 6.5

    Returns a string representing the floating-point number \a f.

    The form of the representation is controlled by the optional \a format and
    \a precision parameters.

    See \l {QLocale::toString(double, char, int)} for more information.
*/

/*!
    \qmlmethod string QtQml::Locale::toString(Date date, string format)
    \overload
    \since 6.5

    Returns a localized string representation of the given \a date in the
    specified \a format. If \c format is an empty string, an empty string is
    returned.

    \sa QLocale::toString(QDate, QStringView)
*/

/*!
    \qmlmethod string QtQml::Locale::toString(Date date, FormatType format = LongFormat)
    \overload
    \since 6.5

    Returns a localized string representation of the given \a date in the
    specified \a format. If \c format is omitted, \c Locale.LongFormat is used.

    \sa QLocale::toString(QDate, QLocale::FormatType)
*/

/*!
    \qmlproperty Array<string> QtQml::Locale::uiLanguages

    Returns an ordered list of locale names for translation purposes in
    preference order.

    The return value represents locale names that the user expects to see the
    UI translation in.

    The first item in the list is the most preferred one.
*/

/*!
    \qmlproperty enumeration QtQml::Locale::textDirection

    Holds the text direction of the language:

    \value Qt.LeftToRight   Text normally begins at the left side.
    \value Qt.RightToLeft   Text normally begins at the right side.
*/

/*!
    \qmlproperty string QtQml::Locale::amText

    The localized name of the "AM" suffix for times specified using the conventions of the 12-hour clock.
*/

/*!
    \qmlproperty string QtQml::Locale::pmText

    The localized name of the "PM" suffix for times specified using the conventions of the 12-hour clock.
*/

/*!
    \qmlmethod string QtQml::Locale::currencySymbol(format)

    Returns the currency symbol for the specified \a format:

    \value Locale.CurrencyIsoCode       a ISO-4217 code of the currency.
    \value Locale.CurrencySymbol        a currency symbol.
    \value Locale.CurrencyDisplayName   a user readable name of the currency.

    \sa Number::toLocaleCurrencyString()
*/

/*!
    \qmlproperty string QtQml::Locale::nativeLanguageName

    Holds a native name of the language for the locale. For example
    "Schwiizertüütsch" for Swiss-German locale.

    \sa nativeTerritoryName
*/

/*!
    \qmlproperty string QtQml::Locale::nativeCountryName
    \deprecated [6.4] Use nativeTerritoryName instead.

    Holds a native name of the country for the locale. For example
    "España" for Spanish/Spain locale.

    \sa nativeLanguageName
*/

/*!
    \qmlproperty string QtQml::Locale::nativeTerritoryName

    Holds a native name of the territory for the locale. For example
    "España" for Spanish/Spain locale.

    \sa nativeLanguageName
*/

/*!
    \qmlproperty enumeration QtQml::Locale::measurementSystem

    This property defines which units are used for measurement.

    \value Locale.MetricSystem      This value indicates metric units, such as meters,
                                    centimeters and millimeters.
    \value Locale.ImperialUSSystem  This value indicates imperial units, such as
                                    inches and miles as they are used in the United States.
    \value Locale.ImperialUKSystem  This value indicates imperial units, such as
                                    inches and miles as they are used in the United Kingdom.
    \value Locale.ImperialSystem    Provided for compatibility. The same as Locale.ImperialUSSystem.
*/

QT_END_NAMESPACE

#include "moc_qqmllocale_p.cpp"
