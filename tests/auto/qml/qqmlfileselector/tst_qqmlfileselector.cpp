// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlFileSelector>
#include <QQmlApplicationEngine>
#include <QFileSelector>
#include <QQmlContext>
#include <QLoggingCategory>
#include <qqmlinfo.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qqmlfileselector : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlfileselector() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void basicTest();
    void basicTestCached();
    void applicationEngineTest();
    void qmldirCompatibility();
};

void tst_qqmlfileselector::basicTest()
{
    QQmlEngine engine;
    QQmlFileSelector selector(&engine);
    selector.setExtraSelectors(QStringList() << "basic");

    QQmlComponent component(&engine, testFileUrl("basicTest.qml"));
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("value").toString(), QString("selected"));

    delete object;
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (type == QtDebugMsg
            && QByteArray(context.category) == QByteArray("qt.qml.diskcache")
            && message.contains("QML source file has moved to a different location.")) {
        QFAIL(message.toUtf8());
    }
}

void tst_qqmlfileselector::basicTestCached()
{
    basicTest(); // Seed the cache, in case basicTestCached() is run on its own
    QtMessageHandler defaultHandler = qInstallMessageHandler(&messageHandler);
    QLoggingCategory::setFilterRules("qt.qml.diskcache.debug=true");
    basicTest(); // Run again and check that the file is in the cache now
    QLoggingCategory::setFilterRules(QString());
    qInstallMessageHandler(defaultHandler);
}

void tst_qqmlfileselector::applicationEngineTest()
{
    QQmlApplicationEngine engine;
    engine.setExtraFileSelectors(QStringList() << "basic");
    engine.load(testFileUrl("basicTest.qml"));

    QVERIFY(!engine.rootObjects().isEmpty());
    QObject *object = engine.rootObjects().at(0);
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("value").toString(), QString("selected"));
}

void tst_qqmlfileselector::qmldirCompatibility()
{
    {
        // No error for multiple files with different selectors, and the matching one is chosen
        // for +macos and +linux selectors.
        QQmlApplicationEngine engine;
        engine.addImportPath(dataDirectory());
        engine.load(testFileUrl("qmldirtest/main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        QObject *object = engine.rootObjects().at(0);
        auto color = object->property("color").value<QColor>();
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
        QCOMPARE(object->objectName(), "linux");
        QCOMPARE(color, QColorConstants::Svg::blue);
#elif defined(Q_OS_DARWIN)
        QCOMPARE(object->objectName(), "macos");
        QCOMPARE(color, QColorConstants::Svg::yellow);
#else
        QCOMPARE(object->objectName(), "base");
        QCOMPARE(color, QColorConstants::Svg::green);
#endif
    }

    {
        // If nothing matches, the _base_ file is chosen, not the first or the last one.
        // This also holds when using the implicit import.
        QQmlApplicationEngine engine;
        engine.addImportPath(dataDirectory());
        engine.load(testFileUrl("qmldirtest2/main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        QObject *object = engine.rootObjects().at(0);
        QCOMPARE(object->property("color").value<QColor>(), QColorConstants::Svg::green);

        QEXPECT_FAIL("", "scripts in implicit import are not resolved", Continue);
        QCOMPARE(object->objectName(), "base");
    }
}

QTEST_MAIN(tst_qqmlfileselector)

#include "tst_qqmlfileselector.moc"
