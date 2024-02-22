// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtQml/qjsengine.h>

class tst_urlobject : public QObject
{
    Q_OBJECT

private slots:
    void searchParams_set();
    void searchParams_nullUrlPointer();
    void urlObject_search();
    void urlObject_search_data();
    void urlObject_href();
    void urlObject_href_data();
};

void tst_urlobject::searchParams_set()
{
    QJSEngine engine;
    QJSValue result =
            engine.evaluate(QLatin1String("var url = new URL(\"http://www.google.com/search\");"
                                          "url.href"));
    QVERIFY(!result.isError());
    QCOMPARE(result.toString(), "http://www.google.com/search");

    result = engine.evaluate(QLatin1String("url.toString()"));
    QVERIFY(!result.isError());
    QCOMPARE(result.toString(), "http://www.google.com/search");

    result = engine.evaluate(QLatin1String("url.searchParams.set(\"q\", \"value\");"
                                           "url.href;"));
    QVERIFY(!result.isError());
    QCOMPARE(result.toString(), "http://www.google.com/search?q=value");

    result = engine.evaluate(QLatin1String("url.toString()"));
    QVERIFY(!result.isError());
    QCOMPARE(result.toString(), "http://www.google.com/search?q=value");

    result = engine.evaluate(QLatin1String("url.searchParams.set(\"t\", \"otherValue\");"
                                           "url.href;"));
    QVERIFY(!result.isError());
    QCOMPARE(result.toString(), "http://www.google.com/search?q=value&t=otherValue");

    result = engine.evaluate(QLatin1String("url.toString()"));
    QVERIFY(!result.isError());
    QCOMPARE(result.toString(), "http://www.google.com/search?q=value&t=otherValue");
}

void tst_urlobject::searchParams_nullUrlPointer()
{
    QJSEngine engine;
    QJSValue result = engine.evaluate(QLatin1String("let params = new URLSearchParams();"
                                                    "params.set(\"foo\", \"bar\");"));
    QVERIFY(!result.isError());
}

void tst_urlobject::urlObject_search()
{
    QFETCH(QString, test);
    QFETCH(QString, expected);

    QJSEngine engine;

    QCOMPARE(engine.evaluate(test).toString(), expected);
}

void tst_urlobject::urlObject_search_data()
{
    QTest::addColumn<QString>("test");
    QTest::addColumn<QString>("expected");

    QTest::newRow("base case")
            << "var url = new URL(\"http://www.google.com/search?q=123\");"
                "url.search"
            << "?q=123";
    QTest::newRow("space")
            << "var url = new URL(\"http://www.google.com/search?a=b ~\");"
                "url.search"
            << "?a=b%20~";
    QTest::newRow("empty search")
            << "var url = new URL(\"http://www.google.com/search?\");"
               "url.search"
            << "";
    QTest::newRow("no search")
            << "var url = new URL(\"http://www.google.com/search\");"
               "url.search"
            << "";
    QTest::newRow("Question mark")
            // the embedded ""'s break trigraph sequences:
            << "var url = new URL(\"http://www.google.com/search?""?=?\");"
               "url.search"
            << "?""?=?";
    QTest::newRow("equal sign")
            << "var url = new URL(\"http://www.google.com/search?a==&b=!\");"
               "url.search"
            << "?a==&b=!";
    QTest::newRow("percent sign")
            << "var url = new URL(\"http://www.google.com/search?a=%20\");"
               "url.search"
            << "?a=%20";
    QTest::newRow("multiple key-value pairs")
            << "var url = new URL(\"http://www.google.com/search?a=b&c=d\");"
               "url.search"
            << "?a=b&c=d";
    QTest::newRow("unreserved")
            << "var url = new URL(\"http://www.google.com/search?a=ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~\");"
               "url.search"
            << "?a=ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";
    QTest::newRow("reserved + illegal")
            << "var url = new URL(\"http://google.com/search/?a=!*();:@&=+$,/?#[]\");"
               "url.search"
            << "?a=!*();:@&=+$,/?";
    QTest::newRow("unicode (U+327D)")
            << "var url = new URL(\"http://google.com/search/?a=㉽\");"
               "url.search"
            << "?a=%E3%89%BD";
    QTest::newRow("backslash")
            // The JS string in the C++ source ends in 4 backslashes.
            // The C++ compiler turns that into 2 backslashes.
            // QV4 receives source code containing a string literal ending in two backslashes.
            // The resulting JS string ends in a single backslash.
            << "var url = new URL('http://google.com/search/?q=\\\\');"
               "url.search"
            << "?q=\\";
}

void tst_urlobject::urlObject_href()
{
    QFETCH(QString, test);
    QFETCH(QString, expected);

    QJSEngine engine;

    QCOMPARE(engine.evaluate(test).toString(), expected);
}

void tst_urlobject::urlObject_href_data()
{
    QTest::addColumn<QString>("test");
    QTest::addColumn<QString>("expected");

    QTest::newRow("QTBUG-110454")
            << "var url = new URL(\"https://example.com/?a=b ~\");"
               "url.href"
            << "https://example.com/?a=b%20~";
}

QTEST_MAIN(tst_urlobject)

#include "tst_qv4urlobject.moc"
