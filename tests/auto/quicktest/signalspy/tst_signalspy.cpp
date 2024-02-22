// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>

#include <qqmlengine.h>
#include <qquickitem.h>
#include <qquickview.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include "mypropertymap.h"

class tst_SignalSpy : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_SignalSpy();

private slots:
    void testValid();
    void testCount();

private:
    QQmlEngine engine;
};

tst_SignalSpy::tst_SignalSpy()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    qmlRegisterType<MyPropertyMap>("MyImport", 1, 0, "MyPropertyMap");
}

void tst_SignalSpy::testValid()
{
    QQuickView window;
    window.setSource(testFileUrl("signalspy.qml"));
    QVERIFY(window.rootObject() != nullptr);

    QObject *mouseSpy = window.rootObject()->findChild<QObject*>("mouseSpy");
    QVERIFY(mouseSpy->property("valid").toBool());

    QObject *propertyMapSpy = window.rootObject()->findChild<QObject*>("propertyMapSpy");
    QVERIFY(propertyMapSpy->property("valid").toBool());
}

void tst_SignalSpy::testCount()
{
    const QUrl urls[] = {
        testFileUrl("signalspy.qml"),
        testFileUrl("signalspy2.qml"),
    };
    for (const auto &url : urls) {
        QQuickView window;
        window.resize(200, 200);
        window.setSource(url);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(window.rootObject() != nullptr);

        QObject *mouseSpy = window.rootObject()->findChild<QObject *>("mouseSpy");
        QCOMPARE(mouseSpy->property("count").toInt(), 0);

        QObject *propertyMapSpy = window.rootObject()->findChild<QObject *>("propertyMapSpy");
        QCOMPARE(propertyMapSpy->property("count").toInt(), 0);

        QTest::mouseClick(&window, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(100, 100));
        QTRY_COMPARE(mouseSpy->property("count").toInt(), 1);

        MyPropertyMap *propertyMap = static_cast<MyPropertyMap *>(
                window.rootObject()->findChild<QObject *>("propertyMap"));
        Q_EMIT propertyMap->mySignal();
        QCOMPARE(propertyMapSpy->property("count").toInt(), 1);
    }
}

QTEST_MAIN(tst_SignalSpy)

#include "tst_signalspy.moc"
