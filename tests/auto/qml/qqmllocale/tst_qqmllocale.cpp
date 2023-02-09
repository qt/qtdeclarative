// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QDebug>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlexpression.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtCore/QDateTime>
#include <QtCore/qtimezone.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qscopedpointer.h>
#include <qcolor.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <time.h>

#undef QT_CAN_CHANGE_SYSTEM_ZONE
/* See QTBUG-56899. We don't (yet) have a proper way to reset the system zone,
   Testing Date.timeZoneUpdated() is only possible on systems where we have a
   way to change the system zone in use.
*/
#ifdef Q_OS_ANDROID
/* Android's time_t-related system functions don't seem to care about the TZ
   environment variable. If we can find a way to change the zone those functions
   use, try implementing it here.
*/
#elif defined(Q_OS_UNIX)
static void setTimeZone(const QByteArray &tz)
{
    if (tz.isEmpty())
        qunsetenv("TZ");
    else
        qputenv("TZ", tz);
    ::tzset();
}
#define QT_CAN_CHANGE_SYSTEM_ZONE
#else
/* On Windows, adjusting the timezone (such that GetTimeZoneInformation() will
   notice) requires additional privileges that aren't normally enabled for a
   process. This can be achieved by calling AdjustTokenPrivileges() and then
   SetTimeZoneInformation(), which will require linking to a different library
   to access that API.
*/
#endif

class tst_qqmllocale : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmllocale() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

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
    void toString_data();
    void toString();
    void firstDayOfWeek_data();
    void firstDayOfWeek();
    void weekDays_data();
    void weekDays();
    void uiLanguages_data();
    void uiLanguages();
    void dateFormat_data();
    void dateFormat();
    void dateTimeFormat_data();
    void dateTimeFormat();
    void timeFormat_data();
    void timeFormat();
#ifdef QT_CAN_CHANGE_SYSTEM_ZONE
    void timeZoneUpdated();
#endif
    void formattedDataSize_data();
    void formattedDataSize();

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
    void numberOptions();

    void stringLocaleCompare_data();
    void stringLocaleCompare();

    void localeAsCppProperty();
private:
    void addPropertyData(const QString &l);
    QVariant getProperty(QObject *obj, const QString &locale, const QString &property);
    void addCurrencySymbolData(const QString &locale);
    void addStandardFormatData();
    void addFormatNameData(const QString &locale);
    void addDateTimeFormatData(const QString &l);
    void addDateFormatData(const QString &l);
    void addTimeFormatData(const QString &l);
    void addFormattedDataSizeDataForLocale(const QString &l);
    void testFunctionCall();
    void addTestFunctionCallDataColumns();
    QQmlEngine engine;
};

void tst_qqmllocale::defaultLocale()
{
    QQmlComponent c(&engine, testFileUrl("properties.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QCOMPARE(obj->property("name").toString(), QLocale().name());
}

#define LOCALE_PROP(type,prop) { #prop, QVariant(type(qlocale.prop())) }

void tst_qqmllocale::addPropertyData(const QString &l)
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
        LOCALE_PROP(QString,nativeTerritoryName),
        LOCALE_PROP(QString,decimalPoint),
        LOCALE_PROP(QString,groupSeparator),
        LOCALE_PROP(QString,percent),
        LOCALE_PROP(QString,zeroDigit),
        LOCALE_PROP(QString,negativeSign),
        LOCALE_PROP(QString,positiveSign),
        LOCALE_PROP(QString,exponential),
        LOCALE_PROP(int,measurementSystem),
        LOCALE_PROP(int,textDirection),
        { nullptr, QVariant() }
    };

    int i = 0;
    while (values[i].name) {
        QByteArray n = l.toLatin1() + ':' + values[i].name;
        QTest::newRow(n.constData()) << l << QByteArray(values[i].name) << values[i].value;
        ++i;
    }
}

void tst_qqmllocale::properties_data()
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

void tst_qqmllocale::properties()
{
    QFETCH(QString, locale);
    QFETCH(QByteArray, property);
    QFETCH(QVariant, value);

    QQmlComponent c(&engine, testFileUrl("properties.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QCOMPARE(obj->property(property), value);
}

void tst_qqmllocale::addCurrencySymbolData(const QString &l)
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

void tst_qqmllocale::currencySymbol_data()
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

void tst_qqmllocale::currencySymbol()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QQmlComponent c(&engine, testFileUrl("functions.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;
    QLocale::CurrencySymbolFormat format = QLocale::CurrencySymbol;

    if (param >= 0)
        format = QLocale::CurrencySymbolFormat(param);

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QMetaObject::invokeMethod(obj.data(), "currencySymbol", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(int(format))));

    QCOMPARE(val.toString(), l.currencySymbol(format));
}

void tst_qqmllocale::addFormatNameData(const QString &l)
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

void tst_qqmllocale::addStandardFormatData()
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

void tst_qqmllocale::monthName_data()
{
    addStandardFormatData();
}

void tst_qqmllocale::monthName()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QQmlComponent c(&engine, testFileUrl("functions.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;
    QLocale::FormatType format = QLocale::LongFormat;
    if (param >= 0)
        format = QLocale::FormatType(param);

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    for (int i = 0; i <= 11; ++i) {
        QMetaObject::invokeMethod(obj.data(), "monthName", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(i)),
            Q_ARG(QVariant, QVariant(int(format))));

        // QLocale January == 1, JS Date January == 0
        QCOMPARE(val.toString(), l.monthName(i+1, format));
    }
}

void tst_qqmllocale::standaloneMonthName_data()
{
    addStandardFormatData();
}

void tst_qqmllocale::standaloneMonthName()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QQmlComponent c(&engine, testFileUrl("functions.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;
    QLocale::FormatType format = QLocale::LongFormat;
    if (param >= 0)
        format = QLocale::FormatType(param);

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    for (int i = 0; i <= 11; ++i) {
        QMetaObject::invokeMethod(obj.data(), "standaloneMonthName", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(i)),
            Q_ARG(QVariant, QVariant(int(format))));

        // QLocale January == 1, JS Date January == 0
        QCOMPARE(val.toString(), l.standaloneMonthName(i+1, format));
    }
}

void tst_qqmllocale::dayName_data()
{
    addStandardFormatData();
}

void tst_qqmllocale::dayName()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QQmlComponent c(&engine, testFileUrl("functions.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;
    QLocale::FormatType format = QLocale::LongFormat;
    if (param >= 0)
        format = QLocale::FormatType(param);

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    for (int i = 1; i <= 7; ++i) {
        QMetaObject::invokeMethod(obj.data(), "dayName", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(i)),
            Q_ARG(QVariant, QVariant(int(format))));

        QCOMPARE(val.toString(), l.dayName(i, format));
    }
}

void tst_qqmllocale::standaloneDayName_data()
{
    addStandardFormatData();
}

void tst_qqmllocale::standaloneDayName()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QQmlComponent c(&engine, testFileUrl("functions.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;
    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    for (int i = 1; i <= 7; ++i) {
        QMetaObject::invokeMethod(obj.data(), "standaloneDayName", Qt::DirectConnection,
                Q_RETURN_ARG(QVariant, val),
                Q_ARG(QVariant, QVariant(i)),
                Q_ARG(QVariant, QVariant(int(format))));

        QCOMPARE(val.toString(), l.standaloneDayName(i, format));
    }
}

void tst_qqmllocale::toString_data()
{
    addTestFunctionCallDataColumns();

    // Test general error conditions which aren't overload or locale-specific.
    QString functionCallScript = "locale.toString()";
    QTest::newRow(qPrintable(functionCallScript)) << "en_AU" << functionCallScript
        << "QLocale(English, Latin, Australia)" << QString();

    functionCallScript = "locale.toString(1, 2, 3, 4)";
    QString errorMessage = ".*Locale: toString\\(\\): Expected 1-3 arguments, but received 4";
    QTest::newRow(qPrintable(functionCallScript)) << "en_AU" << functionCallScript << QString() << errorMessage;

    // Test toString(int).
    functionCallScript = "locale.toString(123)";
    QTest::newRow(qPrintable(functionCallScript)) << "de_DE" << functionCallScript << "123" << QString();

    // Test toString(double[, char][, int]).
    functionCallScript = "locale.toString(123.456)";
    QTest::newRow(qPrintable(functionCallScript)) << "de_DE" << functionCallScript << "123,456" << QString();

    functionCallScript = "locale.toString(123.456, 'e')";
    QTest::newRow(qPrintable(functionCallScript)) << "de_DE" << functionCallScript << "1,234560E+02" << QString();

    functionCallScript = "locale.toString(123.456, 'e', 1)";
    QTest::newRow(qPrintable(functionCallScript)) << "de_DE" << functionCallScript << "1,2E+02" << QString();

    // Test toString(Date, string) and toString(Date[, FormatType]).
    const QDateTime midnight2000(QDate(2000, 1, 1), QTime(0, 0));
    // 12 AM might not exist in this timezone (some timezones have transitions at midnight).
    if (midnight2000.isValid()) {
        functionCallScript = "locale.toString(new Date(2000, 1, 1))";
        QTest::newRow(qPrintable(functionCallScript)) << "en_AU" << functionCallScript
            << QLatin1String("Tuesday, 1 February 2000 12:00:00 AM ") + midnight2000.timeZoneAbbreviation() << QString();
    }

    functionCallScript = "locale.toString(new Date(2022, 7, 16), [])";
    errorMessage = ".*Locale: the second argument to the toString overloads whose "
        "first argument is a Date should be a string or FormatType";
    QTest::newRow(qPrintable(functionCallScript)) << "ar" << functionCallScript << QString() << errorMessage;

    functionCallScript = "locale.toString([], 'd')";
    errorMessage = ".*Locale: toString\\(\\) expects either an int, double, or Date as its first argument";
    QTest::newRow(qPrintable(functionCallScript)) << "ar" << functionCallScript << QString() << errorMessage;

    functionCallScript = "locale.toString(new Date(2022, 7, 16), 'd')";
    QTest::newRow(qPrintable(functionCallScript)) << "ar" << functionCallScript << "١٦" << QString();

    functionCallScript = "locale.toString(new Date(2022, 7, 16), Locale.ShortFormat)";
    QTest::newRow(qPrintable(functionCallScript)) << "en_AU" << functionCallScript << "16/8/22 12:00 AM" << QString();

    functionCallScript = "locale.toString(new Date(2022, 7, 16, 1, 23, 4), Locale.ShortFormat)";
    QTest::newRow(qPrintable(functionCallScript)) << "en_AU" << functionCallScript << "16/8/22 1:23 AM" << QString();
}

void tst_qqmllocale::toString()
{
    testFunctionCall();
}

void tst_qqmllocale::firstDayOfWeek_data()
{
    QTest::addColumn<QString>("locale");

    QTest::newRow("en_US") << "en_US";
    QTest::newRow("de_DE") << "de_DE";
    QTest::newRow("ar_SA") << "ar_SA";
    QTest::newRow("hi_IN") << "hi_IN";
    QTest::newRow("zh_CN") << "zh_CN";
    QTest::newRow("th_TH") << "th_TH";
}

void tst_qqmllocale::firstDayOfWeek()
{
    QFETCH(QString, locale);

    QQmlComponent c(&engine, testFileUrl("properties.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QVariant val = obj->property("firstDayOfWeek");
    QCOMPARE(val.userType(), QMetaType::Int);

    int day = int(QLocale(locale).firstDayOfWeek());
    if (day == 7) // JS Date days in range 0(Sunday) to 6(Saturday)
        day = 0;
    QCOMPARE(day, val.toInt());
}

void tst_qqmllocale::weekDays_data()
{
    QTest::addColumn<QString>("locale");

    QTest::newRow("en_US") << "en_US";
    QTest::newRow("de_DE") << "de_DE";
    QTest::newRow("ar_SA") << "ar_SA";
    QTest::newRow("hi_IN") << "hi_IN";
    QTest::newRow("zh_CN") << "zh_CN";
    QTest::newRow("th_TH") << "th_TH";
}

void tst_qqmllocale::weekDays()
{
    QFETCH(QString, locale);

    QQmlComponent c(&engine, testFileUrl("properties.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QVariant val = obj->property("weekDays");
    QCOMPARE(val.userType(), qMetaTypeId<QJSValue>());

    QList<QVariant> qmlDays = val.toList();
    QList<Qt::DayOfWeek> days = QLocale(locale).weekdays();

    QCOMPARE(days.size(), qmlDays.size());

    for (int i = 0; i < days.size(); ++i) {
        int day = int(days.at(i));
        if (day == 7) // JS Date days in range 0(Sunday) to 6(Saturday)
            day = 0;
        QCOMPARE(day, qmlDays.at(i).toInt());
    }
}

void tst_qqmllocale::uiLanguages_data()
{
    QTest::addColumn<QString>("locale");

    QTest::newRow("en_US") << "en_US";
    QTest::newRow("de_DE") << "de_DE";
    QTest::newRow("ar_SA") << "ar_SA";
    QTest::newRow("hi_IN") << "hi_IN";
    QTest::newRow("zh_CN") << "zh_CN";
    QTest::newRow("th_TH") << "th_TH";
}

void tst_qqmllocale::uiLanguages()
{
    QFETCH(QString, locale);

    QQmlComponent c(&engine, testFileUrl("properties.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QVariant val = obj->property("uiLanguages");
    QCOMPARE(val.userType(), qMetaTypeId<QJSValue>());

    QList<QVariant> qmlLangs = val.toList();
    QStringList langs = QLocale(locale).uiLanguages();

    QCOMPARE(langs.size(), qmlLangs.size());

    for (int i = 0; i < langs.size(); ++i) {
        QCOMPARE(langs.at(i), qmlLangs.at(i).toString());
    }
}


void tst_qqmllocale::dateTimeFormat_data()
{
    addStandardFormatData();
}

void tst_qqmllocale::dateTimeFormat()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QQmlComponent c(&engine, testFileUrl("functions.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);
    QMetaObject::invokeMethod(obj.data(), "dateTimeFormat", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(param)));

    QCOMPARE(val.toString(), l.dateTimeFormat(format));
}

void tst_qqmllocale::dateFormat_data()
{
    addStandardFormatData();
}

void tst_qqmllocale::dateFormat()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QQmlComponent c(&engine, testFileUrl("functions.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);
    QMetaObject::invokeMethod(obj.data(), "dateFormat", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(param)));

    QCOMPARE(val.toString(), l.dateFormat(format));
}

void tst_qqmllocale::timeFormat_data()
{
    addStandardFormatData();
}

void tst_qqmllocale::timeFormat()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QQmlComponent c(&engine, testFileUrl("functions.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QLocale l(locale);
    QVariant val;

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);
    QMetaObject::invokeMethod(obj.data(), "timeFormat", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, val),
            Q_ARG(QVariant, QVariant(param)));

    QCOMPARE(val.toString(), l.timeFormat(format));
}

void tst_qqmllocale::addFormattedDataSizeDataForLocale(const QString &localeStr)
{
    const QByteArray localeByteArray = localeStr.toLatin1();
    QString functionCallScript;
    QString expectedResult;
    QString expectedErrorMessage;

    const QLocale locale(localeStr);

    const auto makeTag = [&](){
        QRegularExpression argRegex("formattedDataSize\\((.*)\\)");
        QString tag = functionCallScript;
        const auto match = argRegex.match(functionCallScript);
        if (match.hasMatch())
            tag = match.captured(1);
        return localeStr + QLatin1String(", ") + tag;
    };

    functionCallScript = QLatin1String("locale.formattedDataSize(1000000)");
    expectedResult = locale.formattedDataSize(1000000);
    QTest::newRow(qPrintable(makeTag())) << localeStr << functionCallScript << expectedResult << expectedErrorMessage;

    functionCallScript = QLatin1String("locale.formattedDataSize(1000000, 3)");
    expectedResult = locale.formattedDataSize(1000000, 3);
    QTest::newRow(qPrintable(makeTag())) << localeStr << functionCallScript << expectedResult << expectedErrorMessage;

    functionCallScript = QLatin1String("locale.formattedDataSize(1000000, 3, Locale.DataSizeIecFormat)");
    expectedResult = locale.formattedDataSize(1000000, 3, QLocale::DataSizeIecFormat);
    QTest::newRow(qPrintable(makeTag())) << localeStr << functionCallScript << expectedResult << expectedErrorMessage;

    functionCallScript = QLatin1String("locale.formattedDataSize(1000000, 3, Locale.DataSizeTraditionalFormat)");
    expectedResult = locale.formattedDataSize(1000000, 3, QLocale::DataSizeTraditionalFormat);
    QTest::newRow(qPrintable(makeTag())) << localeStr << functionCallScript << expectedResult << expectedErrorMessage;

    functionCallScript = QLatin1String("locale.formattedDataSize(1000000, 3, Locale.DataSizeSIFormat)");
    expectedResult = locale.formattedDataSize(1000000, 3, QLocale::DataSizeSIFormat);
    QTest::newRow(qPrintable(makeTag())) << localeStr << functionCallScript << expectedResult << expectedErrorMessage;
}

#define STOP_ON_FAILURE \
    if (QTest::currentTestFailed()) \
        return;

/*!
    To simplify the tests, this function calls a function and checks the
    return value (a string). If it's expected that it fails, it checks
    that the expected error message was printed.
*/
void tst_qqmllocale::testFunctionCall()
{
    QFETCH(QString, localeStr);
    QFETCH(QString, functionCallScript);
    QFETCH(QString, expectedResult);
    QFETCH(QString, expectedErrorMessagePattern);

    QQmlComponent component(&engine, testFileUrl("functions.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    STOP_ON_FAILURE

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object);
    STOP_ON_FAILURE

    QLocale locale(localeStr);
    QVariant returnValue;

    QVERIFY(QMetaObject::invokeMethod(object.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(localeStr))));
    STOP_ON_FAILURE

    // Make sure that we use the object's context rather than the root context,
    // so that e.g. enums from the Locale type are available (QTBUG-91747).
    QQmlExpression qmlExpression(qmlContext(object.data()), object.data(), functionCallScript);
    const QVariant evaluationResult = qmlExpression.evaluate();
    if (expectedErrorMessagePattern.isEmpty()) {
        QVERIFY2(!qmlExpression.hasError(), qPrintable(qmlExpression.error().toString()));
        STOP_ON_FAILURE
        QVERIFY(evaluationResult.canConvert<QString>());
        STOP_ON_FAILURE

        // We're not interested in whether the spaces in the date/time format
        // are breaking or non-breaking.
        const QString resultWithBreakingSpaces
                = evaluationResult.toString().replace(u'\u202f', u' ');
        const QString expectedWithBreakingSpaces
                = expectedResult.replace(u'\u202f', u' ');

        QCOMPARE(resultWithBreakingSpaces, expectedWithBreakingSpaces);
        STOP_ON_FAILURE
    } else {
        QVERIFY(qmlExpression.hasError());
        STOP_ON_FAILURE
        QRegularExpression errorRegex(expectedErrorMessagePattern);
        QVERIFY(errorRegex.isValid());
        STOP_ON_FAILURE
        QVERIFY2(errorRegex.match(qmlExpression.error().toString()).hasMatch(),
            qPrintable(QString::fromLatin1("Mismatch in actual vs expected error message:\n   Actual: %1\n Expected: %2")
                .arg(qmlExpression.error().toString()).arg(expectedErrorMessagePattern)));
        STOP_ON_FAILURE
    }
}

void tst_qqmllocale::addTestFunctionCallDataColumns()
{
    QTest::addColumn<QString>("localeStr");
    QTest::addColumn<QString>("functionCallScript");
    QTest::addColumn<QString>("expectedResult");
    QTest::addColumn<QString>("expectedErrorMessagePattern");
}

void tst_qqmllocale::formattedDataSize_data()
{
    addTestFunctionCallDataColumns();

    addFormattedDataSizeDataForLocale("en_US");
    addFormattedDataSizeDataForLocale("de_DE");
    addFormattedDataSizeDataForLocale("ar_SA");
    addFormattedDataSizeDataForLocale("hi_IN");
    addFormattedDataSizeDataForLocale("zh_CN");
    addFormattedDataSizeDataForLocale("th_TH");

    // Test error conditions (which aren't locale-specific).
    QString functionCallScript = "locale.formattedDataSize()";
    QString errorMessage = ".*Locale: formattedDataSize\\(\\): Expected 1-3 arguments, but received 0";
    QTest::newRow("too few args") << "en_AU" << functionCallScript << QString() << errorMessage;

    functionCallScript = "locale.formattedDataSize(10, 1, Locale.DataSizeIecFormat, \"foo\")";
    errorMessage = ".*Locale: formattedDataSize\\(\\): Expected 1-3 arguments, but received 4";
    QTest::newRow("too many args") << "en_AU" << functionCallScript << QString() << errorMessage;

    functionCallScript = "locale.formattedDataSize(10, \"no\")";
    errorMessage = ".*Locale: formattedDataSize\\(\\): Invalid argument \\('precision' must be an int\\)";
    QTest::newRow("precision wrong type") << "en_AU" << functionCallScript << QString() << errorMessage;

    functionCallScript = "locale.formattedDataSize(10, 1, \"no\")";
    errorMessage = ".*Locale: formattedDataSize\\(\\): Invalid argument \\('format' must be DataSizeFormat\\)";
    QTest::newRow("format wrong type") << "en_AU" << functionCallScript << QString() << errorMessage;
}

void tst_qqmllocale::formattedDataSize()
{
    testFunctionCall();
}

void tst_qqmllocale::dateToLocaleString_data()
{
    addStandardFormatData();
}

void tst_qqmllocale::dateToLocaleString()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QQmlComponent c(&engine, testFileUrl("date.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    const QDateTime dt(QDate(2011, 10, 7), QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);

    QVariant val;
    QMetaObject::invokeMethod(obj.data(), "toLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(param)));

    QLocale l(locale);
    QCOMPARE(val.toString(), l.toString(dt, format));
}

void tst_qqmllocale::addDateTimeFormatData(const QString &l)
{
    const char *formats[] = {
        "hh:mm dd.MM.yyyy",
        "h:m:sap ddd MMMM d yy",
        "'The date and time is: 'H:mm:ss:zzz dd/MM/yy",
        "MMM d yyyy HH:mm t",
        nullptr
    };
    QByteArray locale = l.toLatin1();
    int i = 0;
    while (formats[i]) {
        QByteArray t(locale);
        t += ' ';
        t += formats[i];
        QTest::newRow(t.constData()) << l << QString(formats[i]);
        ++i;
    }
}

void tst_qqmllocale::dateToLocaleStringFormatted_data()
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

void tst_qqmllocale::dateToLocaleStringFormatted()
{
    QFETCH(QString, locale);
    QFETCH(QString, format);

    QQmlComponent c(&engine, testFileUrl("date.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    const QDateTime dt(QDate(2011, 10, 7), QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QVariant val;
    QMetaObject::invokeMethod(obj.data(), "toLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(format)));

    QLocale l(locale);
    QCOMPARE(val.toString(), l.toString(dt, format));
}

void tst_qqmllocale::dateToLocaleDateString_data()
{
    addStandardFormatData();
}

void tst_qqmllocale::dateToLocaleDateString()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QQmlComponent c(&engine, testFileUrl("date.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    const QDateTime dt(QDate(2011, 10, 7), QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);

    QVariant val;
    QMetaObject::invokeMethod(obj.data(), "toLocaleDateString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(param)));

    QLocale l(locale);
    QCOMPARE(val.toString(), l.toString(dt.date(), format));
}

void tst_qqmllocale::addDateFormatData(const QString &l)
{
    const char *formats[] = {
        "dd.MM.yyyy",
        "ddd MMMM d yy",
        "'The date is: 'dd/MM/yy",
        "MMM d yyyy",
        nullptr
    };
    QByteArray locale = l.toLatin1();
    int i = 0;
    while (formats[i]) {
        QByteArray t(locale);
        t += ' ';
        t += formats[i];
        QTest::newRow(t.constData()) << l << QString(formats[i]);
        ++i;
    }
}

void tst_qqmllocale::dateToLocaleDateStringFormatted_data()
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

void tst_qqmllocale::dateToLocaleDateStringFormatted()
{
    QFETCH(QString, locale);
    QFETCH(QString, format);

    QQmlComponent c(&engine, testFileUrl("date.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    const QDateTime dt(QDate(2011, 10, 7), QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QVariant val;
    QMetaObject::invokeMethod(obj.data(), "toLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(format)));

    QLocale l(locale);
    QCOMPARE(val.toString(), l.toString(dt.date(), format));
}

void tst_qqmllocale::dateToLocaleTimeString_data()
{
    addStandardFormatData();
}

void tst_qqmllocale::dateToLocaleTimeString()
{
    QFETCH(QString, locale);
    QFETCH(int, param);

    QQmlComponent c(&engine, testFileUrl("date.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    const QDateTime dt(QDate(2011, 10, 7), QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale::FormatType format = param < 0 ? QLocale::LongFormat : QLocale::FormatType(param);

    QVariant val;
    QMetaObject::invokeMethod(obj.data(), "toLocaleTimeString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(param)));

    QLocale l(locale);
    QCOMPARE(val.toString(), l.toString(dt.time(), format));
}

void tst_qqmllocale::addTimeFormatData(const QString &l)
{
    const char *formats[] = {
        "hh:mm",
        "h:m:sap",
        "'The time is: 'H:mm:ss:zzz",
        "HH:mm t",
        nullptr
    };
    QByteArray locale = l.toLatin1();
    int i = 0;
    while (formats[i]) {
        QByteArray t(locale);
        t += ' ';
        t += formats[i];
        QTest::newRow(t.constData()) << l << QString(formats[i]);
        ++i;
    }
}

void tst_qqmllocale::dateToLocaleTimeStringFormatted_data()
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

void tst_qqmllocale::dateToLocaleTimeStringFormatted()
{
    QFETCH(QString, locale);
    QFETCH(QString, format);

    QQmlComponent c(&engine, testFileUrl("date.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    const QDateTime dt(QDate(2011, 10, 7), QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QVariant val;
    QMetaObject::invokeMethod(obj.data(), "toLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(format)));

    QLocale l(locale);
    QCOMPARE(val.toString(), l.toString(dt, format));
}

void tst_qqmllocale::dateFromLocaleString_data()
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

void tst_qqmllocale::dateFromLocaleString()
{
    QFETCH(QString, locale);
    QFETCH(QString, format);

    QQmlComponent c(&engine, testFileUrl("date.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    const QDateTime dt(QDate(2011, 10, 7), QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale l(locale);
    const QString localeText(l.toString(dt, format));
    QVariant val;
    QMetaObject::invokeMethod(obj.data(), "fromLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(localeText)),
        Q_ARG(QVariant, QVariant(format)));

    QDateTime pd = l.toDateTime(localeText, format);
    QCOMPARE(val.toDateTime(), pd);
}

void tst_qqmllocale::dateFromLocaleDateString_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("format");

    QTest::newRow("en_US 1") << "en_US" << "dddd, MMMM d, yyyy h:mm:ss AP";
    QTest::newRow("en_US long") << "en_US" << QLocale("en_US").dateFormat();
    QTest::newRow("en_US short") << "en_US" << QLocale("en_US").dateFormat(QLocale::ShortFormat);
    QTest::newRow("de_DE long") << "de_DE" << QLocale("de_DE").dateFormat();
    QTest::newRow("de_DE short") << "de_DE" << QLocale("de_DE").dateFormat(QLocale::ShortFormat);
    QTest::newRow("ar_SA long") << "ar_SA" << QLocale("ar_SA").dateFormat();
    QTest::newRow("ar_SA short") << "ar_SA" << QLocale("ar_SA").dateFormat(QLocale::ShortFormat);
    QTest::newRow("zh_CN long") << "zh_CN" << QLocale("zh_CN").dateFormat();
    QTest::newRow("zh_CN short") << "zh_CN" << QLocale("zh_CN").dateFormat(QLocale::ShortFormat);
}

void tst_qqmllocale::dateFromLocaleDateString()
{
    QFETCH(QString, locale);
    QFETCH(QString, format);

    QQmlComponent c(&engine, testFileUrl("date.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    const QDateTime dt(QDate(2011, 10, 7), QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale l(locale);
    const QString localeText(l.toString(dt, format));
    QVariant val;
    QMetaObject::invokeMethod(obj.data(), "fromLocaleDateString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(localeText)),
        Q_ARG(QVariant, QVariant(format)));

    QDate pd = l.toDate(localeText, format);
    QCOMPARE(val.toDate(), pd);
}

void tst_qqmllocale::dateFromLocaleTimeString_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("format");

    QTest::newRow("en_US 1") << "en_US" << "dddd, MMMM d, yyyy h:mm:ss AP";
    QTest::newRow("en_US long") << "en_US" << QLocale("en_US").timeFormat();
    QTest::newRow("en_US short") << "en_US" << QLocale("en_US").timeFormat(QLocale::ShortFormat);
    QTest::newRow("de_DE long") << "de_DE" << QLocale("de_DE").timeFormat();
    QTest::newRow("de_DE short") << "de_DE" << QLocale("de_DE").timeFormat(QLocale::ShortFormat);
    QTest::newRow("ar_SA long") << "ar_SA" << QLocale("ar_SA").timeFormat();
    QTest::newRow("ar_SA short") << "ar_SA" << QLocale("ar_SA").timeFormat(QLocale::ShortFormat);
    QTest::newRow("zh_CN long") << "zh_CN" << QLocale("zh_CN").timeFormat();
    QTest::newRow("zh_CN short") << "zh_CN" << QLocale("zh_CN").timeFormat(QLocale::ShortFormat);
}

void tst_qqmllocale::dateFromLocaleTimeString()
{
    QFETCH(QString, locale);
    QFETCH(QString, format);

    QQmlComponent c(&engine, testFileUrl("date.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    const QDateTime dt(QDate(2011, 10, 7), QTime(18, 53, 48, 345));

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale l(locale);
    const QString localeText(l.toString(dt, format));
    QVariant val;
    QMetaObject::invokeMethod(obj.data(), "fromLocaleTimeString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(localeText)),
        Q_ARG(QVariant, QVariant(format)));

    QTime pd = l.toTime(localeText, format);
    QCOMPARE(val.toTime(), pd);
}

void tst_qqmllocale::numberToLocaleString_data()
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

void tst_qqmllocale::numberToLocaleString()
{
    QFETCH(QString, locale);
    QFETCH(char, format);
    QFETCH(int, prec);

    QQmlComponent c(&engine, testFileUrl("number.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    double number = 2344423.3289;

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale l(locale);
    QVariant val;
    QMetaObject::invokeMethod(obj.data(), "toLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(number)),
        Q_ARG(QVariant, QVariant(QString(format))),
        Q_ARG(QVariant, QVariant(prec)));

    if (prec < 0) prec = 2;
    QCOMPARE(val.toString(), l.toString(number, format, prec));
}

void tst_qqmllocale::numberToLocaleCurrencyString_data()
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

void tst_qqmllocale::numberToLocaleCurrencyString()
{
    QFETCH(QString, locale);
    QFETCH(QString, symbol);

    QQmlComponent c(&engine, testFileUrl("number.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    double number = 2344423.3289;

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QLocale l(locale);
    QVariant val;
    QMetaObject::invokeMethod(obj.data(), "toLocaleCurrencyString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(number)),
        Q_ARG(QVariant, QVariant(symbol)));

    QCOMPARE(val.toString(), l.toCurrencyString(number, symbol));
}

void tst_qqmllocale::numberFromLocaleString_data()
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

void tst_qqmllocale::numberFromLocaleString()
{
    QFETCH(QString, locale);
    QFETCH(double, number);

    QQmlComponent c(&engine, testFileUrl("number.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QLocale l(locale);
    QString strNumber = l.toString(number, 'f');

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant(locale)));

    QVariant val;
    QMetaObject::invokeMethod(obj.data(), "fromLocaleString", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, val),
        Q_ARG(QVariant, QVariant(strNumber)));

    QCOMPARE(val.toDouble(), l.toDouble(strNumber));
}

void tst_qqmllocale::numberConstToLocaleString()
{
    QQmlComponent c(&engine, testFileUrl("number.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QMetaObject::invokeMethod(obj.data(), "setLocale", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant("en_US")));

    QLocale l("en_US");
    QCOMPARE(obj->property("const1").toString(), l.toString(1234.56, 'f', 2));
    QCOMPARE(obj->property("const2").toString(), l.toString(1234., 'f', 2));
}

void tst_qqmllocale::numberOptions()
{
    QQmlEngine engine;
    QQmlComponent comp(&engine);
    comp.setData(R"(
        import QtQml 2.15
        QtObject {
            id: root
            property string formatted
            property bool caughtException: false
            Component.onCompleted: () => {
                const myLocale = Qt.locale("de_DE")
                myLocale.numberOptions = Locale.OmitGroupSeparator | Locale.RejectTrailingZeroesAfterDot
                root.formatted = Number(10000).toLocaleString(myLocale, 'f', 4)
                try {
                    Number.fromLocaleString(myLocale, "1,10");
                } catch (e) {console.warn(e); root.caughtException = true}
            }
        }
    )", QUrl("testdata"));
    QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                         "Error: Locale: Number.fromLocaleString(): Invalid format");
    QScopedPointer<QObject> root {comp.create()};
    const auto error = comp.errorString();
    if (!error.isEmpty())
        qDebug() << error;
    QVERIFY(root);
    QCOMPARE(root->property("formatted").toString(), QLatin1String("10000,0000"));
    QCOMPARE(root->property("caughtException").toBool(), true);

}

void tst_qqmllocale::stringLocaleCompare_data()
{
    QTest::addColumn<QString>("string1");
    QTest::addColumn<QString>("string2");

    QTest::newRow("before") << "a" << "b";
    QTest::newRow("equal") << "a" << "a";
    QTest::newRow("after") << "b" << "a";

    // Copied from QString::localeAwareCompare tests
    // We don't actually change locale - we just care that String.localeCompare()
    // matches QString::localeAwareCompare();
    QTest::newRow("swedish1") << QString::fromLatin1("\xe5") << QString::fromLatin1("\xe4");
    QTest::newRow("swedish2") << QString::fromLatin1("\xe4") << QString::fromLatin1("\xf6");
    QTest::newRow("swedish3") << QString::fromLatin1("\xe5") << QString::fromLatin1("\xf6");
    QTest::newRow("swedish4") << QString::fromLatin1("z") << QString::fromLatin1("\xe5");

    QTest::newRow("german1") << QString::fromLatin1("z") << QString::fromLatin1("\xe4");
    QTest::newRow("german2") << QString::fromLatin1("\xe4") << QString::fromLatin1("\xf6");
    QTest::newRow("german3") << QString::fromLatin1("z") << QString::fromLatin1("\xf6");
}

void tst_qqmllocale::stringLocaleCompare()
{
    QFETCH(QString, string1);
    QFETCH(QString, string2);

    QQmlComponent c(&engine, testFileUrl("localeCompare.qml"));

    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    obj->setProperty("string1", string1);
    obj->setProperty("string2", string2);

    QCOMPARE(obj->property("comparison").toInt(), QString::localeAwareCompare(string1, string2));
}

class Calendar : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale)
public:
    Calendar() {
    }

    QLocale locale() const {
        return mLocale;
    }

    void setLocale(const QLocale &locale) {
        mLocale = locale;
    }
private:
    QLocale mLocale;
};

void tst_qqmllocale::localeAsCppProperty()
{
    qmlRegisterType<Calendar>("Test", 1, 0, "Calendar");
    QQmlComponent component(&engine, testFileUrl("localeAsCppProperty.qml"));
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QTRY_VERIFY(component.isReady());

    QScopedPointer<QObject> obj(component.create());
    Calendar *item = qobject_cast<Calendar *>(obj.data());
    QCOMPARE(item->property("locale").toLocale().name(), QLatin1String("en_GB"));

    QVariant localeVariant(QLocale("nb_NO"));
    item->setProperty("testLocale", localeVariant);
    QCOMPARE(item->property("testLocale").toLocale().name(), QLatin1String("nb_NO"));
}

#ifdef QT_CAN_CHANGE_SYSTEM_ZONE
class DateFormatter : public QObject
{
    Q_OBJECT
public:
    DateFormatter() {}

    Q_INVOKABLE QString getLocalizedForm(const QString &isoTimestamp);
};

QString DateFormatter::getLocalizedForm(const QString &isoTimestamp)
{
    QDateTime input = QDateTime::fromString(isoTimestamp, Qt::ISODate);
    QLocale locale;
    return locale.toString(input);
}

void tst_qqmllocale::timeZoneUpdated()
{
    QByteArray original(qgetenv("TZ"));
    QScopedPointer<QObject> obj;

    auto cleanup = qScopeGuard([&original, &obj] {
        // Restore to original time zone
        setTimeZone(original);
        QMetaObject::invokeMethod(obj.data(), "resetTimeZone");
    });

    // Set the timezone to Brisbane time, AEST-10:00
    setTimeZone(QByteArray("Australia/Brisbane"));

    DateFormatter formatter;
    QQmlEngine e;
    e.rootContext()->setContextObject(&formatter);

    QQmlComponent c(&e, testFileUrl("timeZoneUpdated.qml"));
    QVERIFY2(!c.isError(), qPrintable(c.errorString()));
    obj.reset(c.create());
    QVERIFY(obj);
    QVERIFY(obj->property("success").toBool());

    // Change to Indian time, IST-05:30
    setTimeZone(QByteArray("Asia/Kolkata"));

    QMetaObject::invokeMethod(obj.data(), "check");
    QVERIFY(obj->property("success").toBool());
}
#endif // QT_CAN_CHANGE_SYSTEM_ZONE

QTEST_MAIN(tst_qqmllocale)

#include "tst_qqmllocale.moc"
