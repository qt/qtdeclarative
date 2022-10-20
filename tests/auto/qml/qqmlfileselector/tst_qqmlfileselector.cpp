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
    QQmlApplicationEngine engine;
    engine.addImportPath(dataDirectory());
    engine.load(testFileUrl("qmldirtest/main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    QObject *object = engine.rootObjects().at(0);
    auto color = object->property("color").value<QColor>();
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QCOMPARE(color, QColorConstants::Svg::blue);
#elif defined(Q_OS_DARWIN)
    QCOMPARE(color, QColorConstants::Svg::yellow);
#else
    QCOMPARE(color, QColorConstants::Svg::green);
#endif
}

QTEST_MAIN(tst_qqmlfileselector)

#include "tst_qqmlfileselector.moc"
