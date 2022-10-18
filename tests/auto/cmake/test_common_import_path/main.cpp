// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

Q_IMPORT_QML_PLUGIN(duck_tickPlugin)
Q_IMPORT_QML_PLUGIN(duck_trickPlugin)
Q_IMPORT_QML_PLUGIN(duck_trackPlugin)

using namespace Qt::StringLiterals;

class test : public QObject
{
    Q_OBJECT
private slots:
    void test_loadable();
};

void test::test_loadable()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral(":/"));
    QQmlComponent c(&engine, QUrl(u"qrc:/duck/main.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("x1").toInt(), 5);
    QCOMPARE(o->property("x2").toInt(), 10);
}

QTEST_MAIN(test)

#include "main.moc"
