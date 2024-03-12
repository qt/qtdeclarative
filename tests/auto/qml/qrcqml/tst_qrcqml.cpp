// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QDebug>

// Loading QML files from embedded resources is a common QML usecase.
// This test just verifies that it works at a basic level, and with the qrc:/ syntax

class tst_qrcqml : public QObject
{
    Q_OBJECT
public:
    tst_qrcqml();

private slots:
    void basicLoad_data();
    void basicLoad();
    void qrcImport_data();
    void qrcImport();
};

tst_qrcqml::tst_qrcqml()
{
}

void tst_qrcqml::basicLoad_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("token");

    QTest::newRow("simple")
        << "qrc:/data/main.qml"
        << "foo";

    QTest::newRow("aliased")
        << "qrc:/main.qml"
        << "bar";

    QTest::newRow("prefixed")
        << "qrc:/search/main.qml"
        << "baz";

    /* This is not supported:
    QTest::newRow("without qrc scheme")
        << ":/data/main.qml"
        << "hello";
    */
}

void tst_qrcqml::basicLoad()
{
    QFETCH(QString, url);
    QFETCH(QString, token);

    QQmlEngine e;
    QQmlComponent c(&e, QUrl(url));
    QVERIFY(c.isReady());
    std::unique_ptr<QObject> o { c.create() };
    QVERIFY(o.get());
    QCOMPARE(o->property("tokenProperty").toString(), token);
}

void tst_qrcqml::qrcImport_data()
{
    QTest::addColumn<QString>("importPath");
    QTest::addColumn<QString>("token");

    QTest::newRow("qrc path")
        << ":/imports"
        << "foo";

    QTest::newRow("qrc url")
        << "qrc:/imports"
        << "foo";
}

void tst_qrcqml::qrcImport()
{
    QFETCH(QString, importPath);
    QFETCH(QString, token);

    QQmlEngine e;
    e.addImportPath(importPath);
    QQmlComponent c(&e, QUrl("qrc:///importtest.qml"));
    QVERIFY(c.isReady());
    std::unique_ptr<QObject> o { c.create() };
    QVERIFY(o.get());
    QCOMPARE(o->property("tokenProperty").toString(), token);
}

QTEST_MAIN(tst_qrcqml)

#include "tst_qrcqml.moc"
