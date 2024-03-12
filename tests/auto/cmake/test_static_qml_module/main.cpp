// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QObject>
#include <QString>
#include <QTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlExtensionPlugin>
#include <QUrl>

Q_IMPORT_QML_PLUGIN(MyUriPlugin)

using namespace Qt::StringLiterals;

class TestStaticQmlPlugin : public QObject
{
    Q_OBJECT
private slots:
    void ElementFromPluginAvailable();
};

void TestStaticQmlPlugin::ElementFromPluginAvailable()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral(":/qt/qml"));
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/MyAppUri/main.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

QTEST_MAIN(TestStaticQmlPlugin)

#include "main.moc"
