// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QtTest/QSignalSpy>

#include <QtGui/qscreen.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickview.h>
#include <private/qquickitem_p.h>
#include <private/qquickrectangle_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickrectangle : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickrectangle();

private slots:
    void color();
    void gradient();
    void gradient_border();
    void gradient_separate();
    void gradient_multiple();
    void gradient_preset();
    void antialiasing();

private:
    QQmlEngine engine;
};

tst_qquickrectangle::tst_qquickrectangle()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquickrectangle::color()
{
#ifdef Q_OS_ANDROID
    QSKIP("Test does not work on Android because of QTBUG-102345");
#endif

    if (QGuiApplication::primaryScreen()->depth() < 24)
        QSKIP("This test does not work at display depths < 24");

    QQuickView view;
    view.setSource(testFileUrl("color.qml"));
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    if (QGuiApplication::platformName() == QLatin1String("minimal"))
        QSKIP("Skipping due to grabWindow not functional on offscreen/minimal platforms");

    QImage image = view.grabWindow();
    QVERIFY(image.pixel(0,0) == QColor("#020202").rgba());
}

void tst_qquickrectangle::gradient()
{
    QQmlComponent component(&engine, testFileUrl("gradient.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(component.create());
    QVERIFY(rect);

    QQuickGradient *grad = qobject_cast<QQuickGradient *>(rect->gradient().toQObject());
    QVERIFY(grad);

    QQmlListProperty<QQuickGradientStop> stops = grad->stops();
    QCOMPARE(stops.count(&stops), 2);
    QCOMPARE(stops.at(&stops, 0)->position(), 0.0);
    QCOMPARE(stops.at(&stops, 0)->color(), QColor("gray"));
    QCOMPARE(stops.at(&stops, 1)->position(), 1.0);
    QCOMPARE(stops.at(&stops, 1)->color(), QColor("white"));

    QGradientStops gradientStops = grad->gradientStops();
    QCOMPARE(gradientStops.size(), 2);
    QCOMPARE(gradientStops.at(0).first, 0.0);
    QCOMPARE(gradientStops.at(0).second, QColor("gray"));
    QCOMPARE(gradientStops.at(1).first, 1.0);
    QCOMPARE(gradientStops.at(1).second, QColor("white"));

    QMetaObject::invokeMethod(rect, "resetGradient");

    grad = qobject_cast<QQuickGradient *>(rect->gradient().toQObject());
    QVERIFY(!grad);

    delete rect;
}

void tst_qquickrectangle::gradient_border()
{
    QQuickView view;
    view.setSource(testFileUrl("gradient-border.qml"));
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));
}

// A gradient not defined inline with the Rectangle using it should still change
// that Rectangle.
void tst_qquickrectangle::gradient_separate()
{
    QQuickView view;
    view.setSource(testFileUrl("gradient-separate.qml"));
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(view.rootObject());
    QVERIFY(rect);

    // Start off clean
    QQuickItemPrivate *rectPriv = QQuickItemPrivate::get(rect);
    QTRY_COMPARE(rectPriv->dirtyAttributes & QQuickItemPrivate::Content, 0u);

    QMetaObject::invokeMethod(rect, "changeGradient");

    // Changing the gradient should have scheduled an update of the item.
    QVERIFY((rectPriv->dirtyAttributes & QQuickItemPrivate::Content) != 0);
}

// When a gradient is changed, every Rectangle connected to it must update.
void tst_qquickrectangle::gradient_multiple()
{
    QQuickView view;
    view.setSource(testFileUrl("gradient-multiple.qml"));
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickRectangle *firstRect = qobject_cast<QQuickRectangle*>(view.rootObject()->property("firstRectangle").value<QObject*>());
    QQuickRectangle *secondRect = qobject_cast<QQuickRectangle*>(view.rootObject()->property("secondRectangle").value<QObject*>());
    QVERIFY(firstRect);
    QVERIFY(secondRect);

    // Start off clean
    QQuickItemPrivate *firstRectPriv = QQuickItemPrivate::get(firstRect);
    QQuickItemPrivate *secondRectPriv = QQuickItemPrivate::get(secondRect);
    QTRY_VERIFY(!(firstRectPriv->dirtyAttributes & QQuickItemPrivate::Content));
    bool secondIsDirty = secondRectPriv->dirtyAttributes & QQuickItemPrivate::Content;
    QVERIFY(!secondIsDirty);

    QMetaObject::invokeMethod(view.rootObject(), "changeGradient");

    // Changing the gradient should have scheduled an update of both items
    QTRY_VERIFY(firstRectPriv->dirtyAttributes & QQuickItemPrivate::Content);
    secondIsDirty = secondRectPriv->dirtyAttributes & QQuickItemPrivate::Content;
    QVERIFY(secondIsDirty);
}

void tst_qquickrectangle::gradient_preset()
{
    QQuickView view;
    view.setSource(testFileUrl("gradient-preset.qml"));
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickRectangle *enumRect = view.rootObject()->findChild<QQuickRectangle *>("enum");
    QVERIFY(enumRect);
    QVERIFY(enumRect->gradient().isNumber());
    QCOMPARE(enumRect->gradient().toUInt(), QGradient::NightFade);

    QQuickRectangle *stringRect = view.rootObject()->findChild<QQuickRectangle *>("string");
    QVERIFY(stringRect);
    QVERIFY(stringRect->gradient().isString());
    QCOMPARE(stringRect->gradient().toString(), QLatin1String("NightFade"));

    for (int i = 1; i <= 5; ++i) {
        QQuickRectangle *invalidRect = view.rootObject()->findChild<QQuickRectangle *>(qPrintable(QString("invalid%1").arg(i)));
        QVERIFY(invalidRect);
        QVERIFY(invalidRect->gradient().isUndefined());
    }
}

void tst_qquickrectangle::antialiasing()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\n Rectangle {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickRectangle *rect = qobject_cast<QQuickRectangle *>(object.data());
    QVERIFY(rect);

    QSignalSpy spy(rect, SIGNAL(antialiasingChanged(bool)));

    QCOMPARE(rect->antialiasing(), false);

    rect->setAntialiasing(true);
    QCOMPARE(rect->antialiasing(), true);
    QCOMPARE(spy.size(), 1);

    rect->setAntialiasing(true);
    QCOMPARE(spy.size(), 1);

    rect->resetAntialiasing();
    QCOMPARE(rect->antialiasing(), false);
    QCOMPARE(spy.size(), 2);

    rect->setRadius(5);
    QCOMPARE(rect->antialiasing(), true);
    QCOMPARE(spy.size(), 3);

    rect->resetAntialiasing();
    QCOMPARE(rect->antialiasing(), true);
    QCOMPARE(spy.size(), 3);

    rect->setRadius(0);
    QCOMPARE(rect->antialiasing(), false);
    QCOMPARE(spy.size(), 4);

    rect->resetAntialiasing();
    QCOMPARE(rect->antialiasing(), false);
    QCOMPARE(spy.size(), 4);

    rect->setRadius(5);
    QCOMPARE(rect->antialiasing(), true);
    QCOMPARE(spy.size(), 5);

    rect->resetAntialiasing();
    QCOMPARE(rect->antialiasing(), true);
    QCOMPARE(spy.size(), 5);

    rect->setAntialiasing(false);
    QCOMPARE(rect->antialiasing(), false);
    QCOMPARE(spy.size(), 6);

    rect->resetAntialiasing();
    QCOMPARE(rect->antialiasing(), true);
    QCOMPARE(spy.size(), 7);
}

QTEST_MAIN(tst_qquickrectangle)

#include "tst_qquickrectangle.moc"
