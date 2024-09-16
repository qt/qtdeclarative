// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QDebug>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QtGui/QScreen>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuick/private/qquickscreen_p.h>
#include <QDebug>
class tst_qquickscreen : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickscreen();

private slots:
    void basicProperties();
    void screenOnStartup();
    void fullScreenList();
};

tst_qquickscreen::tst_qquickscreen()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquickscreen::basicProperties()
{
    QQuickView view;
    view.setSource(testFileUrl("screen.qml"));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem* root = view.rootObject();
    QVERIFY(root);

    QScreen* screen = view.screen();
    QVERIFY(screen);

    QCOMPARE(screen->size().width(), root->property("w").toInt());
    QCOMPARE(screen->size().height(), root->property("h").toInt());
    QCOMPARE(int(screen->orientation()), root->property("curOrientation").toInt());
    QCOMPARE(int(screen->primaryOrientation()), root->property("priOrientation").toInt());
    QCOMPARE(screen->devicePixelRatio(), root->property("devicePixelRatio").toReal());
    QVERIFY(screen->devicePixelRatio() >= 1.0);
    QCOMPARE(screen->geometry().x(), root->property("vx").toInt());
    QCOMPARE(screen->geometry().y(), root->property("vy").toInt());

    QVERIFY(root->property("screenCount").toInt() == QGuiApplication::screens().size());
}

void tst_qquickscreen::screenOnStartup()
{
    // We expect QQuickScreen to fall back to the primary screen
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("screen.qml"));

    QScopedPointer<QQuickItem> root(qobject_cast<QQuickItem*>(component.create()));
    QVERIFY(root);

    QScreen* screen = QGuiApplication::primaryScreen();
    QVERIFY(screen);

    QCOMPARE(screen->size().width(), root->property("w").toInt());
    QCOMPARE(screen->size().height(), root->property("h").toInt());
    QCOMPARE(int(screen->orientation()), root->property("curOrientation").toInt());
    QCOMPARE(int(screen->primaryOrientation()), root->property("priOrientation").toInt());
    QCOMPARE(screen->devicePixelRatio(), root->property("devicePixelRatio").toReal());
    QVERIFY(screen->devicePixelRatio() >= 1.0);
    QCOMPARE(screen->geometry().x(), root->property("vx").toInt());
    QCOMPARE(screen->geometry().y(), root->property("vy").toInt());
}

void tst_qquickscreen::fullScreenList()
{
    QQuickView view;
    view.setSource(testFileUrl("screen.qml"));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem* root = view.rootObject();
    QVERIFY(root);

    QJSValue screensArray = root->property("allScreens").value<QJSValue>();
    QVERIFY(screensArray.isArray());
    int length = screensArray.property("length").toInt();
    const QList<QScreen *> screenList = QGuiApplication::screens();
    QVERIFY(length == screenList.size());

    for (int i = 0; i < length; ++i) {
        QQuickScreenInfo *info = qobject_cast<QQuickScreenInfo *>(screensArray.property(i).toQObject());
        QVERIFY(info != nullptr);
        QCOMPARE(screenList[i]->name(), info->name());
        QCOMPARE(screenList[i]->manufacturer(), info->manufacturer());
        QCOMPARE(screenList[i]->model(), info->model());
        QCOMPARE(screenList[i]->serialNumber(), info->serialNumber());
        QCOMPARE(screenList[i]->size().width(), info->width());
        QCOMPARE(screenList[i]->size().height(), info->height());
        QCOMPARE(screenList[i]->availableVirtualGeometry().width(), info->desktopAvailableWidth());
        QCOMPARE(screenList[i]->availableVirtualGeometry().height(), info->desktopAvailableHeight());
        QCOMPARE(screenList[i]->devicePixelRatio(), info->devicePixelRatio());
        QCOMPARE(screenList[i]->geometry().x(), info->virtualX());
        QCOMPARE(screenList[i]->geometry().y(), info->virtualY());
    }
}

QTEST_MAIN(tst_qquickscreen)

#include "tst_qquickscreen.moc"
