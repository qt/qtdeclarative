// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtQuick/qquickview.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickShapesDesignHelpers/private/qquickstarshape_p.h>
#include <QtQuickShapesDesignHelpers/private/qquickstarshape_p_p.h>

using namespace QQuickVisualTestUtils;

class tst_QQuickStarShape : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickStarShape();

private slots:
    void initTestCase() override;
    void basicShape();
    void setPointCount();
    void setCornerRadius();
    void changeSignals();
    void changeSignals_data();
    void fillItem();

private:
    QScopedPointer<QQuickView> window;
};

tst_QQuickStarShape::tst_QQuickStarShape() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

void tst_QQuickStarShape::initTestCase()
{
    QQmlDataTest::initTestCase();
    window.reset(QQuickViewTestUtils::createView());
}

void tst_QQuickStarShape::basicShape()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("starshape.qml"));
    QQuickStarShape *shape = qobject_cast<QQuickStarShape *>(c.create());
    QVERIFY(shape);

    QCOMPARE(shape->width(), 200);
    QCOMPARE(shape->height(), 200);
    QCOMPARE(shape->cornerRadius(), 10);
    QCOMPARE(shape->pointCount(), 6);
    QCOMPARE(shape->ratio(), 0.5);
    QCOMPARE(shape->strokeWidth(), 1);
    QCOMPARE(shape->dashOffset(), 0);
    QCOMPARE(shape->capStyle(), QQuickShapePath::SquareCap);
    QCOMPARE(shape->joinStyle(), QQuickShapePath::BevelJoin);
    QCOMPARE(shape->strokeStyle(), QQuickShapePath::SolidLine);
    QCOMPARE(shape->fillColor(), QColor(Qt::white));
    QCOMPARE(shape->strokeColor(), QColor(Qt::black));
    QCOMPARE(shape->fillItem(), nullptr);
}

void tst_QQuickStarShape::setPointCount()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("starshape.qml"));
    QQuickStarShape *shape = qobject_cast<QQuickStarShape *>(c.create());
    QVERIFY2(shape, qPrintable(c.errorString()));

    QCOMPARE(shape->pointCount(), 6);

    shape->setPointCount(QQuickStarShapePrivate::pointCountMin - 1);
    QCOMPARE(shape->pointCount(), QQuickStarShapePrivate::pointCountMin);

    shape->setPointCount(QQuickStarShapePrivate::pointCountMax + 1);
    QCOMPARE(shape->pointCount(), QQuickStarShapePrivate::pointCountMax);
}

void tst_QQuickStarShape::setCornerRadius()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("starshape.qml"));
    QQuickStarShape *shape = qobject_cast<QQuickStarShape *>(c.create());
    QVERIFY2(shape, qPrintable(c.errorString()));

    QCOMPARE(shape->cornerRadius(), 10);

    shape->setCornerRadius(-1);
    QCOMPARE(shape->cornerRadius(), 0);
}

void tst_QQuickStarShape::changeSignals_data()
{
    QTest::addColumn<QVariant>("propertyValue");
    QTest::addColumn<QMetaMethod>("changeSignal");

    QTest::newRow("cornerRadius") << QVariant::fromValue(5.0f)
                                  << QMetaMethod::fromSignal(&QQuickStarShape::cornerRadiusChanged);
    QTest::newRow("pointCount") << QVariant::fromValue(7)
                                << QMetaMethod::fromSignal(&QQuickStarShape::pointCountChanged);
    QTest::newRow("ratio") << QVariant::fromValue(0.9)
                           << QMetaMethod::fromSignal(&QQuickStarShape::ratioChanged);
    QTest::newRow("strokeColor") << QVariant::fromValue(QColor(Qt::blue))
                                 << QMetaMethod::fromSignal(&QQuickStarShape::strokeColorChanged);
    QTest::newRow("strokeWidth") << QVariant::fromValue(0)
                                 << QMetaMethod::fromSignal(&QQuickStarShape::strokeWidthChanged);
    QTest::newRow("fillColor") << QVariant::fromValue(QColor(Qt::blue))
                               << QMetaMethod::fromSignal(&QQuickStarShape::fillColorChanged);
    QTest::newRow("joinStyle") << QVariant::fromValue(QQuickShapePath::RoundJoin)
                               << QMetaMethod::fromSignal(&QQuickStarShape::joinStyleChanged);
    QTest::newRow("capStyle") << QVariant::fromValue(QQuickShapePath::RoundCap)
                              << QMetaMethod::fromSignal(&QQuickStarShape::capStyleChanged);
    QTest::newRow("strokeStyle") << QVariant::fromValue(QQuickShapePath::DashLine)
                                 << QMetaMethod::fromSignal(&QQuickStarShape::strokeStyleChanged);
    QTest::newRow("dashOffset") << QVariant::fromValue(4)
                                << QMetaMethod::fromSignal(&QQuickStarShape::dashOffsetChanged);
    QTest::newRow("dashPattern") << QVariant::fromValue(QList<qreal>{ 1, 2 })
                                 << QMetaMethod::fromSignal(&QQuickStarShape::dashPatternChanged);
}

void tst_QQuickStarShape::changeSignals()
{
    window->setSource(testFileUrl("default.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.get()));
    QVERIFY(window->status() == QQuickView::Ready);

    QFETCH(const QVariant, propertyValue);
    QFETCH(const QMetaMethod, changeSignal);

    QQuickStarShape *shape =
            QQuickVisualTestUtils::findItem<QQuickStarShape>(window->rootObject(), "shape");
    QVERIFY(shape);
    const QSignalSpy signalSpy(shape, changeSignal);
    QVERIFY(signalSpy.isValid());
    QVERIFY(shape->setProperty(QTest::currentDataTag(), propertyValue));
    QCOMPARE(signalSpy.count(), 1);
}

void tst_QQuickStarShape::fillItem()
{
    QQuickApplicationHelper helper(this, "fillitem.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *theShape = window->property("theShape").value<QQuickStarShape *>();
    QVERIFY(theShape);

    auto *fillItem = window->findChild<QQuickItem *>("fillItem");
    QVERIFY(fillItem);
}

QTEST_MAIN(tst_QQuickStarShape)

#include "tst_qquickstarshape.moc"
