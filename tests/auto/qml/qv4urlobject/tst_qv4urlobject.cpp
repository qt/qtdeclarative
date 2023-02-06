// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtQml/qjsengine.h>

class tst_urlobject : public QObject
{
    Q_OBJECT

private slots:
    void searchParams_set();
    void searchParams_nullUrlPointer();
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

QTEST_MAIN(tst_urlobject)

#include "tst_qv4urlobject.moc"
