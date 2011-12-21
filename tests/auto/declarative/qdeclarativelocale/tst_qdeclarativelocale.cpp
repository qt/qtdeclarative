/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <QDebug>

#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtCore/QDateTime>
#include <qcolor.h>
#include "../../shared/util.h"

class tst_qdeclarativelocale : public QDeclarativeDataTest
{
    Q_OBJECT
public:
    tst_qdeclarativelocale() { }

private slots:
    void defaultLocale();

    void properties_data();
    void properties();
    void currencySymbol_data();
    void currencySymbol();
    void monthName_data();
    void monthName();
    void standaloneMonthName_data();
    void standaloneMonthName();
    void dayName_data();
    void dayName();
    void standaloneDayName_data();
    void standaloneDayName();
    void weekDays_data();
    void weekDays();
    void dateFormat_data();
    void dateFormat();
    void dateTimeFormat_data();
    void dateTimeFormat();
    void timeFormat_data();
    void timeFormat();

    void dateToLocaleString_data();
    void dateToLocaleString();
    void dateToLocaleStringFormatted_data();
    void dateToLocaleStringFormatted();
    void dateToLocaleDateString_data();
    void dateToLocaleDateString();
    void dateToLocaleDateStringFormatted_data();
    void dateToLocaleDateStringFormatted();
    void dateToLocaleTimeString_data();
    void dateToLocaleTimeString();
    void dateToLocaleTimeStringFormatted_data();
    void dateToLocaleTimeStringFormatted();
    void dateFromLocaleString_data();
    void dateFromLocaleString();
    void dateFromLocaleDateString_data();
    void dateFromLocaleDateString();
    void dateFromLocaleTimeString_data();
    void dateFromLocaleTimeString();

    void numberToLocaleString_data();
    void numberToLocaleString();
    void numberToLocaleCurrencyString_data();
    void numberToLocaleCurrencyString();
    void numberFromLocaleString_data();
    void numberFromLocaleString();
    void numberConstToLocaleString();

private:
    void addPropertyData(const QString &l);
    QVariant getProperty(QObject *obj, const QString &locale, const QString &property);
    void addCurrencySymbolData(const QString &locale);
    void addStandardFormatData();
    void addFormatNameData(const QString &locale);
    void addDateTimeFormatData(const QString &l);
    void addDateFormatData(const QString &l);
    void addTimeFormatData(const QString &l);
    QDeclarativeEngine engine;
};

void tst_qdeclarativelocale::defaultLocale()
{
    QDeclarativeComponent c(&engine, testFileUrl("properties.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QCOMPARE(obj->property("name").toString(), QLocale().name());
}

#define LOCALE_PROP(type,prop) { #prop, QVariant(type(qlocale.prop())) }

void tst_qdeclarativelocale::addPropertyData(const QString &l)
{
    QLocale qlocale(l);

    struct {
        const char *name;
        QVariant value;
    }
    values[] = {
        LOCALE_PROP(QString,name),
        LOCALE_PROP(QString,amText),
        LOCALE_PROP(QString,pmText),
        LOCALE_PROP(QString,nativeLanguageName),
        LOCALE_PROP(QString,nativeCountryName),
        LOCALE_PROP(QString,decimalPoint),
        LOCALE_PROP(QString,groupSeparator),
        LOCALE_PROP(QString,percent),
        LOCALE_PROP(QString,zeroDigit),
        LOCALE_PROP(QString,negativeSign),
        LOCALE_PROP(QString,positiveSign),
        LOCALE_PROP(QString,exponential),
        LOCALE_PROP(int,firstDayOfWeek),
        LOCALE_PROP(int,measurementSystem),
        LOCALE_PROP(int,textDirection),
        { 0, QVariant() }
    };

    int i = 0;
    while (values[i].name) {
        QByteArray n = l.toLatin1() + ':' + values[i].name;
        QTest::newRow(n.constData()) << l << QByteArray(values[i].name) << values[i].value;
        ++i;
    }
}

void tst_qdeclarativelocale::properties_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QByteArray>("property");
    QTest::addColumn<QVariant>("value");

    addPropertyData("en_US");
    addPropertyData("de_DE");
    addPropertyData("ar_SA");
    addPropertyData("hi_IN");
    addPropertyData("zh_CN");
    addPropertyData("th_TH");
}

void tst_qdeclarativelocale::properties()
{
    QFETCH(QString, locale);
    QFETCH(QByteArray, property);
    QFETCH(QVariant, value);

    QDeclarativeComponent c(&engine, testFileUrl("properties.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QCOMPARE(obj->property(property), value);

    delete obj;
}

void tst_qdeclarativelocale::addCurrencySymbolData(const QString &l)
{
    QByteArray locale = l.toLatin1();
    QTest::newRow(locale.constData()) << l << -1;
    QByteArray t(locale);
    t += " CurrencyIsoCode";
    QTest::newRow(t.constData()) << l << (int)QLocale::CurrencyIsoCode;
    t = locale + " CurrencySymbol";
    QTest::newRow(t.constData()) << l << (int)QLocale::CurrencySymbol;
    t = locale + " CurrencyDisplayName";
    QTest::newRow(t.constData()) << l << (int)QLocale::CurrencyDisplayName;
}

void tst_qdeclarativelocale::currencySymbol_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<int>("param");

    addCurrencySymbolData("en_US");
    addCurrencySymbolData("de_DE");
    addCurrencySymbolData("ar_SA");
    addCurrencySymbolData("hi_IN");
    addCurrencySymbolData("zh_CN");
    addCurrencySymbolData("th_TH");
}

void tst_qdeclarativelocale::currencySymbol()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QDeclarativeComponent c(&engine, testFileUrl("functions.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;
    QLocale::CurrencySymbolFormat format = QLocale::CurrencySymbol;

    if (param >= 0)
        format = QLocale::CurrencySymbolFormat(param);

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QMetaObject::invokeMethod(obj, "currencySymbol", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(int(format))));

    QCOMPARE(val.toString(), l.currencySymbol(format));

    delete obj;
}

void tst_qdeclarativelocale::addFormatNameData(const QString &l)
{
    QByteArray locale = l.toLatin1();
    QTest::newRow(locale.constData()) << l << -1;
    QByteArray t(locale);
    t += " LongFormat";
    QTest::newRow(t.constData()) << l << (int)QLocale::LongFormat;
    t = locale + " ShortFormat";
    QTest::newRow(t.constData()) << l << (int)QLocale::ShortFormat;
    t = locale + " NarrowFormat";
    QTest::newRow(t.constData()) << l << (int)QLocale::NarrowFormat;
}

void tst_qdeclarativelocale::addStandardFormatData()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<int>("param");

    addFormatNameData("en_US");
    addFormatNameData("de_DE");
    addFormatNameData("ar_SA");
    addFormatNameData("hi_IN");
    addFormatNameData("zh_CN");
    addFormatNameData("th_TH");
}

void tst_qdeclarativelocale::monthName_data()
{
    addStandardFormatData();
}

void tst_qdeclarativelocale::monthName()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QDeclarativeComponent c(&engine, testFileUrl("functions.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;
    QLocale::FormatType format = QLocale::LongFormat;
    if (param >= 0)
        format = QLocale::FormatType(param);

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    for (int i = 0; i <= 11; ++i) {
        QMetaObject::invokeMethod(obj, "monthName", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(i)),
            Q_ARG(QVariant, QVariant(int(format))));

        // QLocale January == 1, JS Date January == 0
        QCOMPARE(val.toString(), l.monthName(i+1, format));
    }

    delete obj;
}

void tst_qdeclarativelocale::standaloneMonthName_data()
{
    addStandardFormatData();
}

void tst_qdeclarativelocale::standaloneMonthName()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QDeclarativeComponent c(&engine, testFileUrl("functions.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;
    QLocale::FormatType format = QLocale::LongFormat;
    if (param >= 0)
        format = QLocale::FormatType(param);

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    for (int i = 0; i <= 11; ++i) {
        QMetaObject::invokeMethod(obj, "standaloneMonthName", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(i)),
            Q_ARG(QVariant, QVariant(int(format))));

        // QLocale January == 1, JS Date January == 0
        QCOMPARE(val.toString(), l.standaloneMonthName(i+1, format));
    }

    delete obj;
}

void tst_qdeclarativelocale::dayName_data()
{
    addStandardFormatData();
}

void tst_qdeclarativelocale::dayName()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QDeclarativeComponent c(&engine, testFileUrl("functions.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;
    QLocale::FormatType format = QLocale::LongFormat;
    if (param >= 0)
        format = QLocale::FormatType(param);

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    for (int i = 1; i <= 7; ++i) {
        QMetaObject::invokeMethod(obj, "dayName", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(i)),
            Q_ARG(QVariant, QVariant(int(format))));

        QCOMPARE(val.toString(), l.dayName(i, format));
    }

    delete obj;
}

void tst_qdeclarativelocale::standaloneDayName_data()
{
    addStandardFormatData();
}

void tst_qdeclarativelocale::standaloneDayName()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QDeclarativeComponent c(&engine, testFileUrl("functions.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;
    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    for (int i = 1; i <= 7; ++i) {
        QMetaObject::invokeMethod(obj, "standaloneDayName", Qt::DirectConnection,
                Q_RETURN_ARG(QVariant, val),
                Q_ARG(QVariant, QVariant(i)),
                Q_ARG(QVariant, QVariant(int(format))));

        QCOMPARE(val.toString(), l.standaloneDayName(i, format));
    }

    delete obj;
}

void tst_qdeclarativelocale::weekDays_data()
{
    QTest::addColumn<QString>("locale");

    QTest::newRow("en_US") << "en_US";
    QTest::newRow("de_DE") << "de_DE";
    QTest::newRow("ar_SA") << "ar_SA";
    QTest::newRow("hi_IN") << "hi_IN";
    QTest::newRow("zh_CN") << "zh_CN";
    QTest::newRow("th_TH") << "th_TH";
}

void tst_qdeclarativelocale::weekDays()
{
    QFETCH(QString, locale);

    QDeclarativeComponent c(&engine, testFileUrl("properties.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QVariant val = obj->property("weekDays");
    QVERIFY(val.type() == QVariant::List);

    QList<QVariant> qmlDays = val.toList();
    QList<Qt::DayOfWeek> days = QLocale(locale).weekdays();

    QVERIFY(days.count() == qmlDays.count());

    for (int i = 0; i < days.count(); ++i) {
        int day = int(days.at(i));
        if (day == 7) // JS Date days in range 0(Sunday) to 6(Saturday)
            day = 0;
        QCOMPARE(day, qmlDays.at(i).toInt());
    }

    delete obj;
}


void tst_qdeclarativelocale::dateTimeFormat_data()
{
    addStandardFormatData();
}

void tst_qdeclarativelocale::dateTimeFormat()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QDeclarativeComponent c(&engine, testFileUrl("functions.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);
    QMetaObject::invokeMethod(obj, "dateTimeFormat", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(param)));

    QCOMPARE(val.toString(), l.dateTimeFormat(format));
}

void tst_qdeclarativelocale::dateFormat_data()
{
    addStandardFormatData();
}

void tst_qdeclarativelocale::dateFormat()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QDeclarativeComponent c(&engine, testFileUrl("functions.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);
    QMetaObject::invokeMethod(obj, "dateFormat", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(param)));

    QCOMPARE(val.toString(), l.dateFormat(format));
}

void tst_qdeclarativelocale::timeFormat_data()
{
    addStandardFormatData();
}

void tst_qdeclarativelocale::timeFormat()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QDeclarativeComponent c(&engine, testFileUrl("functions.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);
    QMetaObject::invokeMethod(obj, "timeFormat", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(param)));

    QCOMPARE(val.toString(), l.timeFormat(format));
}

void tst_qdeclarativelocale::dateToLocaleString_data()
{
    addStandardFormatData();
}

void tst_qdeclarativelocale::dateToLocaleString()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QDeclarativeComponent c(&engine, testFileUrl("date.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QDateTime dt;
    dt.setDate(QDate(2011, 10, 7)); // weirdly, JS Date month range is 0-11
    dt.setTime(QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);

    QVariant val;
    QMetaObject::invokeMethod(obj, "toLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(param)));

    QLocale l(locale);
    QCOMPARE(val.toString(), l.toString(dt, format));
}

void tst_qdeclarativelocale::addDateTimeFormatData(const QString &l)
{
    const char *formats[] = {
        "hh:mm dd.MM.yyyy",
        "h:m:sap ddd MMMM d yy",
        "'The date and time is: 'H:mm:ss:zzz dd/MM/yy",
        "MMM d yyyy HH:mm t",
        0
    };
    QByteArray locale = l.toLatin1();
    int i = 0;
    while (formats[i]) {
        QByteArray t(locale);
        t += " ";
        t += formats[i];
        QTest::newRow(t.constData()) << l << QString(formats[i]);
        ++i;
    }
}

void tst_qdeclarativelocale::dateToLocaleStringFormatted_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("format");

    addDateTimeFormatData("en_US");
    addDateTimeFormatData("de_DE");
    addDateTimeFormatData("ar_SA");
    addDateTimeFormatData("hi_IN");
    addDateTimeFormatData("zh_CN");
    addDateTimeFormatData("th_TH");
}

void tst_qdeclarativelocale::dateToLocaleStringFormatted()
{
    QFETCH(QString, locale);
    QFETCH(QString, format);

    QDeclarativeComponent c(&engine, testFileUrl("date.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QDateTime dt;
    dt.setDate(QDate(2011, 10, 7)); // weirdly, JS Date month range is 0-11
    dt.setTime(QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QVariant val;
    QMetaObject::invokeMethod(obj, "toLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(format)));

    QLocale l(locale);
    QCOMPARE(val.toString(), l.toString(dt, format));
}

void tst_qdeclarativelocale::dateToLocaleDateString_data()
{
    addStandardFormatData();
}

void tst_qdeclarativelocale::dateToLocaleDateString()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QDeclarativeComponent c(&engine, testFileUrl("date.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QDateTime dt;
    dt.setDate(QDate(2011, 10, 7)); // weirdly, JS Date month range is 0-11
    dt.setTime(QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);

    QVariant val;
    QMetaObject::invokeMethod(obj, "toLocaleDateString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(param)));

    QLocale l(locale);
    QCOMPARE(val.toString(), l.toString(dt.date(), format));
}

void tst_qdeclarativelocale::addDateFormatData(const QString &l)
{
    const char *formats[] = {
        "dd.MM.yyyy",
        "ddd MMMM d yy",
        "'The date is: 'dd/MM/yy",
        "MMM d yyyy",
        0
    };
    QByteArray locale = l.toLatin1();
    int i = 0;
    while (formats[i]) {
        QByteArray t(locale);
        t += " ";
        t += formats[i];
        QTest::newRow(t.constData()) << l << QString(formats[i]);
        ++i;
    }
}

void tst_qdeclarativelocale::dateToLocaleDateStringFormatted_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("format");

    addDateFormatData("en_US");
    addDateFormatData("de_DE");
    addDateFormatData("ar_SA");
    addDateFormatData("hi_IN");
    addDateFormatData("zh_CN");
    addDateFormatData("th_TH");
}

void tst_qdeclarativelocale::dateToLocaleDateStringFormatted()
{
    QFETCH(QString, locale);
    QFETCH(QString, format);

    QDeclarativeComponent c(&engine, testFileUrl("date.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QDateTime dt;
    dt.setDate(QDate(2011, 10, 7)); // weirdly, JS Date month range is 0-11
    dt.setTime(QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QVariant val;
    QMetaObject::invokeMethod(obj, "toLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(format)));

    QLocale l(locale);
    QCOMPARE(val.toString(), l.toString(dt.date(), format));
}

void tst_qdeclarativelocale::dateToLocaleTimeString_data()
{
    addStandardFormatData();
}

void tst_qdeclarativelocale::dateToLocaleTimeString()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QDeclarativeComponent c(&engine, testFileUrl("date.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QDateTime dt;
    dt.setDate(QDate(2011, 10, 7)); // weirdly, JS Date month range is 0-11
    dt.setTime(QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);

    QVariant val;
    QMetaObject::invokeMethod(obj, "toLocaleTimeString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(param)));

    QLocale l(locale);
    QCOMPARE(val.toString(), l.toString(dt.time(), format));
}

void tst_qdeclarativelocale::addTimeFormatData(const QString &l)
{
    const char *formats[] = {
        "hh:mm",
        "h:m:sap",
        "'The time is: 'H:mm:ss:zzz",
        "HH:mm t",
        0
    };
    QByteArray locale = l.toLatin1();
    int i = 0;
    while (formats[i]) {
        QByteArray t(locale);
        t += " ";
        t += formats[i];
        QTest::newRow(t.constData()) << l << QString(formats[i]);
        ++i;
    }
}

void tst_qdeclarativelocale::dateToLocaleTimeStringFormatted_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("format");

    addTimeFormatData("en_US");
    addTimeFormatData("de_DE");
    addTimeFormatData("ar_SA");
    addTimeFormatData("hi_IN");
    addTimeFormatData("zh_CN");
    addTimeFormatData("th_TH");
}

void tst_qdeclarativelocale::dateToLocaleTimeStringFormatted()
{
    QFETCH(QString, locale);
    QFETCH(QString, format);

    QDeclarativeComponent c(&engine, testFileUrl("date.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QDateTime dt;
    dt.setDate(QDate(2011, 10, 7)); // weirdly, JS Date month range is 0-11
    dt.setTime(QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QVariant val;
    QMetaObject::invokeMethod(obj, "toLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(format)));

    QLocale l(locale);
    QCOMPARE(val.toString(), l.toString(dt.time(), format));
}

void tst_qdeclarativelocale::dateFromLocaleString_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("format");

    QTest::newRow("en_US 1") << "en_US" << "dddd, MMMM d, yyyy h:mm:ss AP";
    QTest::newRow("en_US long") << "en_US" << QLocale("en_US").dateTimeFormat();
    QTest::newRow("en_US short") << "en_US" << QLocale("en_US").dateTimeFormat(QLocale::ShortFormat);
    QTest::newRow("de_DE long") << "de_DE" << QLocale("de_DE").dateTimeFormat();
    QTest::newRow("de_DE short") << "de_DE" << QLocale("de_DE").dateTimeFormat(QLocale::ShortFormat);
    QTest::newRow("ar_SA long") << "ar_SA" << QLocale("ar_SA").dateTimeFormat();
    QTest::newRow("ar_SA short") << "ar_SA" << QLocale("ar_SA").dateTimeFormat(QLocale::ShortFormat);
    QTest::newRow("zh_CN long") << "zh_CN" << QLocale("zh_CN").dateTimeFormat();
    QTest::newRow("zh_CN short") << "zh_CN" << QLocale("zh_CN").dateTimeFormat(QLocale::ShortFormat);
}

void tst_qdeclarativelocale::dateFromLocaleString()
{
    QFETCH(QString, locale);
    QFETCH(QString, format);

    QDeclarativeComponent c(&engine, testFileUrl("date.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QDateTime dt;
    dt.setDate(QDate(2011, 10, 7));
    dt.setTime(QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale l(locale);
    QVariant val;
    QMetaObject::invokeMethod(obj, "fromLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(l.toString(dt, format))),
        Q_ARG(QVariant, QVariant(format)));

    QDateTime pd = l.toDateTime(l.toString(dt, format), format);
    QCOMPARE(val.toDateTime(), pd);
}

void tst_qdeclarativelocale::dateFromLocaleDateString_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("format");

    QTest::newRow("en_US 1") << "en_US" << "dddd, MMMM d, yyyy h:mm:ss AP";
    QTest::newRow("en_US long") << "en_US" << QLocale("en_US").dateTimeFormat();
    QTest::newRow("en_US short") << "en_US" << QLocale("en_US").dateTimeFormat(QLocale::ShortFormat);
    QTest::newRow("de_DE long") << "de_DE" << QLocale("de_DE").dateTimeFormat();
    QTest::newRow("de_DE short") << "de_DE" << QLocale("de_DE").dateTimeFormat(QLocale::ShortFormat);
    QTest::newRow("ar_SA long") << "ar_SA" << QLocale("ar_SA").dateTimeFormat();
    QTest::newRow("ar_SA short") << "ar_SA" << QLocale("ar_SA").dateTimeFormat(QLocale::ShortFormat);
    QTest::newRow("zh_CN long") << "zh_CN" << QLocale("zh_CN").dateTimeFormat();
    QTest::newRow("zh_CN short") << "zh_CN" << QLocale("zh_CN").dateTimeFormat(QLocale::ShortFormat);
}

void tst_qdeclarativelocale::dateFromLocaleDateString()
{
    QFETCH(QString, locale);
    QFETCH(QString, format);

    QDeclarativeComponent c(&engine, testFileUrl("date.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QDateTime dt;
    dt.setDate(QDate(2011, 10, 7));
    dt.setTime(QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale l(locale);
    QVariant val;
    QMetaObject::invokeMethod(obj, "fromLocaleDateString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(l.toString(dt, format))),
        Q_ARG(QVariant, QVariant(format)));

    QDate pd = l.toDate(l.toString(dt, format), format);
    QCOMPARE(val.toDate(), pd);
}

void tst_qdeclarativelocale::dateFromLocaleTimeString_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("format");

    QTest::newRow("en_US 1") << "en_US" << "dddd, MMMM d, yyyy h:mm:ss AP";
    QTest::newRow("en_US long") << "en_US" << QLocale("en_US").dateTimeFormat();
    QTest::newRow("en_US short") << "en_US" << QLocale("en_US").dateTimeFormat(QLocale::ShortFormat);
    QTest::newRow("de_DE long") << "de_DE" << QLocale("de_DE").dateTimeFormat();
    QTest::newRow("de_DE short") << "de_DE" << QLocale("de_DE").dateTimeFormat(QLocale::ShortFormat);
    QTest::newRow("ar_SA long") << "ar_SA" << QLocale("ar_SA").dateTimeFormat();
    QTest::newRow("ar_SA short") << "ar_SA" << QLocale("ar_SA").dateTimeFormat(QLocale::ShortFormat);
    QTest::newRow("zh_CN long") << "zh_CN" << QLocale("zh_CN").dateTimeFormat();
    QTest::newRow("zh_CN short") << "zh_CN" << QLocale("zh_CN").dateTimeFormat(QLocale::ShortFormat);
}

void tst_qdeclarativelocale::dateFromLocaleTimeString()
{
    QFETCH(QString, locale);
    QFETCH(QString, format);

    QDeclarativeComponent c(&engine, testFileUrl("date.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QDateTime dt;
    dt.setDate(QDate(2011, 10, 7));
    dt.setTime(QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale l(locale);
    QVariant val;
    QMetaObject::invokeMethod(obj, "fromLocaleTimeString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(l.toString(dt, format))),
        Q_ARG(QVariant, QVariant(format)));

    QTime pd = l.toTime(l.toString(dt, format), format);
    QCOMPARE(val.toTime(), pd);
}

void tst_qdeclarativelocale::numberToLocaleString_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<char>("format");
    QTest::addColumn<int>("prec");

    QTest::newRow("en_US 1") << "en_US" << 'f' << 2;
    QTest::newRow("en_US 2") << "en_US" << 'g' << 3;
    QTest::newRow("en_US 3") << "en_US" << 'f' << 0;
    QTest::newRow("en_US 4") << "en_US" << 'f' << -1;
    QTest::newRow("de_DE 1") << "de_DE" << 'f' << 2;
    QTest::newRow("de_DE 2") << "de_DE" << 'g' << 3;
    QTest::newRow("ar_SA 1") << "ar_SA" << 'f' << 2;
    QTest::newRow("ar_SA 2") << "ar_SA" << 'g' << 3;
}

void tst_qdeclarativelocale::numberToLocaleString()
{
    QFETCH(QString, locale);
    QFETCH(char, format);
    QFETCH(int, prec);

    QDeclarativeComponent c(&engine, testFileUrl("number.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    double number = 2344423.3289;

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale l(locale);
    QVariant val;
    QMetaObject::invokeMethod(obj, "toLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(number)),
        Q_ARG(QVariant, QVariant(QString(format))),
        Q_ARG(QVariant, QVariant(prec)));

    if (prec < 0) prec = 2;
    QCOMPARE(val.toString(), l.toString(number, format, prec));
}

void tst_qdeclarativelocale::numberToLocaleCurrencyString_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("symbol");

    QTest::newRow("en_US 1") << "en_US" << QString();
    QTest::newRow("en_US 2") << "en_US" << "USD";
    QTest::newRow("de_DE") << "de_DE" << QString();
    QTest::newRow("ar_SA") << "ar_SA" << QString();
    QTest::newRow("hi_IN") << "hi_IN" << QString();
    QTest::newRow("zh_CN") << "zh_CN" << QString();
    QTest::newRow("th_TH") << "th_TH" << QString();
}

void tst_qdeclarativelocale::numberToLocaleCurrencyString()
{
    QFETCH(QString, locale);
    QFETCH(QString, symbol);

    QDeclarativeComponent c(&engine, testFileUrl("number.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    double number = 2344423.3289;

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale l(locale);
    QVariant val;
    QMetaObject::invokeMethod(obj, "toLocaleCurrencyString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(number)),
        Q_ARG(QVariant, QVariant(symbol)));

    QCOMPARE(val.toString(), l.toCurrencyString(number, symbol));
}

void tst_qdeclarativelocale::numberFromLocaleString_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<double>("number");

    QTest::newRow("en_US 1") << "en_US" << 1234567.2345;
    QTest::newRow("en_US 2") << "en_US" << 0.234;
    QTest::newRow("en_US 3") << "en_US" << 234.0;
    QTest::newRow("de_DE") << "de_DE" << 1234567.2345;
    QTest::newRow("ar_SA") << "ar_SA" << 1234567.2345;
    QTest::newRow("hi_IN") << "hi_IN" << 1234567.2345;
    QTest::newRow("zh_CN") << "zh_CN" << 1234567.2345;
    QTest::newRow("th_TH") << "th_TH" << 1234567.2345;
}

void tst_qdeclarativelocale::numberFromLocaleString()
{
    QFETCH(QString, locale);
    QFETCH(double, number);

    QDeclarativeComponent c(&engine, testFileUrl("number.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QLocale l(locale);
    QString strNumber = l.toString(number, 'f');

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QVariant val;
    QMetaObject::invokeMethod(obj, "fromLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(strNumber)));

    QCOMPARE(val.toDouble(), l.toDouble(strNumber));
}

void tst_qdeclarativelocale::numberConstToLocaleString()
{
    QDeclarativeComponent c(&engine, testFileUrl("number.qml"));

    QObject *obj = c.create();
    QVERIFY(obj);

    QMetaObject::invokeMethod(obj, "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant("en_US")));

    QLocale l("en_US");
    QCOMPARE(obj->property("const1").toString(), l.toString(1234.56, 'f', 2));
    QCOMPARE(obj->property("const2").toString(), l.toString(1234., 'f', 2));
}

QTEST_MAIN(tst_qdeclarativelocale)

#include "tst_qdeclarativelocale.moc"
