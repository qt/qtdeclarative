// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtQuick/qquickview.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickShapesDesignHelpers/private/qquickellipseshape_p.h>

class tst_QQuickEllipseShape : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickEllipseShape();

private slots:
    void initTestCase() override;
    void basicShape();
    void changeSignals();
    void changeSignals_data();

private:
    QScopedPointer<QQuickView> window;
};

tst_QQuickEllipseShape::tst_QQuickEllipseShape() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

void tst_QQuickEllipseShape::initTestCase()
{
    QQmlDataTest::initTestCase();
    window.reset(QQuickViewTestUtils::createView());
}

void tst_QQuickEllipseShape::basicShape()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("ellipseshape1.qml"));
    QQuickEllipseShape *ellipseShape = qobject_cast<QQuickEllipseShape *>(c.create());
    QVERIFY(ellipseShape);

    QCOMPARE(ellipseShape->sweepAngle(), 360);
    QCOMPARE(ellipseShape->startAngle(), 0);
    QCOMPARE(ellipseShape->width(), 200);
    QCOMPARE(ellipseShape->height(), 200);
    QCOMPARE(ellipseShape->cornerRadius(), 10);
    QCOMPARE(ellipseShape->innerArcRatio(), 0);
    QCOMPARE(ellipseShape->strokeWidth(), 1);
    QCOMPARE(ellipseShape->dashOffset(), 0);
    QCOMPARE(ellipseShape->capStyle(), QQuickShapePath::SquareCap);
    QCOMPARE(ellipseShape->joinStyle(), QQuickShapePath::BevelJoin);
    QCOMPARE(ellipseShape->strokeStyle(), QQuickShapePath::SolidLine);
    QCOMPARE(ellipseShape->borderMode(), QQuickEllipseShape::BorderMode::Inside);
    QCOMPARE(ellipseShape->strokeColor(), QColor(Qt::black));
    QCOMPARE(ellipseShape->fillColor(), QColor(Qt::white));
}

void tst_QQuickEllipseShape::changeSignals_data()
{
    QTest::addColumn<QVariant>("propertyValue");
    QTest::addColumn<QMetaMethod>("changeSignal");

    QTest::newRow("sweepAngle") << QVariant::fromValue(180)
                                << QMetaMethod::fromSignal(&QQuickEllipseShape::sweepAngleChanged);
    QTest::newRow("startAngle") << QVariant::fromValue(90)
                                << QMetaMethod::fromSignal(&QQuickEllipseShape::startAngleChanged);
    QTest::newRow("cornerRadius") << QVariant::fromValue(
            20) << QMetaMethod::fromSignal(&QQuickEllipseShape::cornerRadiusChanged);
    QTest::newRow("innerArcRatio")
            << QVariant::fromValue(0.5f)
            << QMetaMethod::fromSignal(&QQuickEllipseShape::innerArcRatioChanged);
    QTest::newRow("strokeColor") << QVariant::fromValue(
            QColor(Qt::blue)) << QMetaMethod::fromSignal(&QQuickEllipseShape::strokeColorChanged);
    QTest::newRow("strokeWidth") << QVariant::fromValue(
            0) << QMetaMethod::fromSignal(&QQuickEllipseShape::strokeWidthChanged);
    QTest::newRow("fillColor") << QVariant::fromValue(QColor(Qt::blue))
                               << QMetaMethod::fromSignal(&QQuickEllipseShape::fillColorChanged);
    QTest::newRow("joinStyle") << QVariant::fromValue(QQuickShapePath::RoundJoin)
                               << QMetaMethod::fromSignal(&QQuickEllipseShape::joinStyleChanged);
    QTest::newRow("capStyle") << QVariant::fromValue(QQuickShapePath::RoundCap)
                              << QMetaMethod::fromSignal(&QQuickEllipseShape::capStyleChanged);
    QTest::newRow("strokeStyle") << QVariant::fromValue(QQuickShapePath::DashLine)
                                 << QMetaMethod::fromSignal(
                                            &QQuickEllipseShape::strokeStyleChanged);
    QTest::newRow("dashOffset") << QVariant::fromValue(4)
                                << QMetaMethod::fromSignal(&QQuickEllipseShape::dashOffsetChanged);
    QTest::newRow("dashPattern") << QVariant::fromValue(QList<qreal>{
            1, 2 }) << QMetaMethod::fromSignal(&QQuickEllipseShape::dashPatternChanged);
    QTest::newRow("borderMode") << QVariant::fromValue(QQuickEllipseShape::BorderMode::Outside)
                                << QMetaMethod::fromSignal(&QQuickEllipseShape::borderModeChanged);
}

void tst_QQuickEllipseShape::changeSignals()
{
    window->setSource(testFileUrl("default.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.get()));
    QVERIFY(window->status() == QQuickView::Ready);

    QFETCH(const QVariant, propertyValue);
    QFETCH(const QMetaMethod, changeSignal);

    QQuickEllipseShape *ellipseShape = QQuickVisualTestUtils::findItem<QQuickEllipseShape>(
            window->rootObject(), "ellipseShape");
    QVERIFY(ellipseShape);
    const QSignalSpy signalSpy(ellipseShape, changeSignal);
    QVERIFY(signalSpy.isValid());
    QVERIFY(ellipseShape->setProperty(QTest::currentDataTag(), propertyValue));
    QCOMPARE(signalSpy.count(), 1);
}

QTEST_MAIN(tst_QQuickEllipseShape)

#include "tst_qquickellipseshape.moc"
