// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QObject>
#include <QString>
#include <QTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlExtensionPlugin>
#include <QUrl>
#include <QDirIterator>

using namespace Qt::StringLiterals;

class TestSimple : public QObject
{
    Q_OBJECT
private slots:
    void CanRun();
};

void TestSimple::CanRun()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral(":/qt/qml"));
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/MyAppUri/Main.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

QTEST_MAIN(TestSimple)

#include "main.moc"
