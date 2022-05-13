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
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    QQmlFileSelector* selector = QQmlFileSelector::get(&engine);
QT_WARNING_POP
    QVERIFY(selector != nullptr);
    selector->setExtraSelectors(QStringList() << "basic");

    QQmlComponent component(&engine, testFileUrl("basicTest.qml"));
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("value").toString(), QString("selected"));

    delete object;
}

QTEST_MAIN(tst_qqmlfileselector)

#include "tst_qqmlfileselector.moc"
