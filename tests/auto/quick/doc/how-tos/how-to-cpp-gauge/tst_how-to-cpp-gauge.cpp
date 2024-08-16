// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>

#include "tickmarkcontainer.h"

QT_BEGIN_NAMESPACE

using namespace QQuickControlsTestUtils;

class tst_HowToCppGauge : public QObject
{
    Q_OBJECT

public:
    tst_HowToCppGauge();

private slots:
    void example_data();
    void example();
};

tst_HowToCppGauge::tst_HowToCppGauge()
{
}

void tst_HowToCppGauge::example_data()
{
    QTest::addColumn<Qt::Orientation>("orientation");

    QTest::newRow("vertical") << Qt::Vertical;
    QTest::newRow("horizontal") << Qt::Horizontal;
}

void tst_HowToCppGauge::example()
{
    QFETCH(Qt::Orientation, orientation);

    QTest::failOnWarning(QRegularExpression(QStringLiteral(".?")));

    QQmlApplicationEngine engine;
    engine.loadFromModule("GaugeHowTo", "Main");
    QCOMPARE(engine.rootObjects().size(), 1);

    auto *window = qobject_cast<QQuickWindow*>(engine.rootObjects().at(0));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *gauge = window->findChild<QQuickControl*>();
    QVERIFY(gauge);
    const int expectedTickmarkStepSize = 10;
    QCOMPARE(gauge->property("tickmarkStepSize").toInt(), expectedTickmarkStepSize);
    const int expectedTickmarkCount = 11;
    QCOMPARE(gauge->property("tickmarkCount").toInt(), expectedTickmarkCount);
    const int expectedMinorTickmarkCount = 4;
    QCOMPARE(gauge->property("minorTickmarkCount").toInt(), expectedMinorTickmarkCount);
    // Set orientation.
    QVERIFY(gauge->setProperty("orientation", orientation));
    const int expectedTickmarkLength = 16;
    const int expectedTickmarkWidth = 2;
    QCOMPARE(gauge->property("tickmarkLength").toInt(), expectedTickmarkLength);
    QCOMPARE(gauge->property("tickmarkWidth").toInt(), expectedTickmarkWidth);
    const int expectedMinorTickmarkLength = 8;
    const int expectedMinorTickmarkWidth = 1;
    QCOMPARE(gauge->property("minorTickmarkLength").toInt(), expectedMinorTickmarkLength);
    QCOMPARE(gauge->property("minorTickmarkWidth").toInt(), expectedMinorTickmarkWidth);
    const qreal tickmarkSpacing = gauge->property("tickmarkSpacing").toReal();

    const bool vertical = orientation == Qt::Vertical;
    if (!vertical) {
        // If it changes orientation, a polish will occur. We need to wait for
        // that to happen before trying to access delegate items, as those will
        // be recreated in updatePolish.
        QVERIFY(QQuickTest::qIsPolishScheduled(window));
        QVERIFY(QQuickTest::qWaitForPolish(window));
    }

    auto *tickmarkLabelContainer = gauge->findChild<TickmarkContainer *>("tickmarkLabelContainer");
    QVERIFY(tickmarkLabelContainer);
    auto *tickmarkContainer = gauge->findChild<TickmarkContainer *>("tickmarkContainer");
    QVERIFY(tickmarkContainer);
    auto *minorTickmarkContainer = gauge->findChild<TickmarkContainer *>("minorTickmarkContainer");
    QVERIFY(minorTickmarkContainer);
    auto *valueBarItem = gauge->findChild<QQuickItem *>("valueBarItem");
    QVERIFY(valueBarItem);

    // Check that there are as many tickmarks and labels as we expect.
    QCOMPARE(tickmarkLabelContainer->childItems().size(), expectedTickmarkCount);
    QCOMPARE(tickmarkContainer->childItems().size(), expectedTickmarkCount);
    const int expectedTotalMinorTickmarkCount = (expectedTickmarkCount - 1) * expectedMinorTickmarkCount;
    QCOMPARE(minorTickmarkContainer->childItems().size(), expectedTotalMinorTickmarkCount);

    QQuickItem *firstTickmark = tickmarkContainer->delegateItemAt(0);
    QVERIFY(firstTickmark);
    QQuickItem *secondTickmark = tickmarkContainer->delegateItemAt(1);
    QVERIFY(secondTickmark);
    QQuickItem *lastTickmark = tickmarkContainer->delegateItemAt(expectedTickmarkCount - 1);
    QVERIFY(lastTickmark);

    QQuickItem *firstMinorTickmark = minorTickmarkContainer->delegateItemAt(0);
    QVERIFY(firstMinorTickmark);
    QQuickItem *secondMinorTickmark = minorTickmarkContainer->delegateItemAt(1);
    QVERIFY(secondMinorTickmark);
    QQuickItem *lastMinorTickmark = minorTickmarkContainer->delegateItemAt(expectedTotalMinorTickmarkCount - 1);
    QVERIFY(lastMinorTickmark);

    QQuickItem *firstLabel = tickmarkLabelContainer->delegateItemAt(0);
    QVERIFY(firstLabel);
    QQuickItem *secondLabel = tickmarkLabelContainer->delegateItemAt(1);
    QVERIFY(secondLabel);
    QQuickItem *lastLabel = tickmarkLabelContainer->delegateItemAt(expectedTickmarkCount - 1);
    QVERIFY(lastLabel);

    if (vertical) {
        QCOMPARE_GT(gauge->implicitHeight(), gauge->implicitWidth());

        // Check that the items are laid out correctly.
        QCOMPARE_LT(tickmarkLabelContainer->x(), tickmarkContainer->x());
        QCOMPARE_LT(tickmarkContainer->x(), valueBarItem->x());

        // Check that tickmarks have the size they should.
        QCOMPARE(firstTickmark->width(), expectedTickmarkLength);
        QCOMPARE(firstTickmark->height(), expectedTickmarkWidth);
        QCOMPARE(firstMinorTickmark->width(), expectedMinorTickmarkLength);
        QCOMPARE(firstMinorTickmark->height(), expectedMinorTickmarkWidth);

        // Check that some tickmarks are laid out roughly where they should be.
        QCOMPARE_GT(firstTickmark->y(), lastTickmark->y());
        QCOMPARE_LE(secondTickmark->y(), firstTickmark->y() - tickmarkSpacing);
        QCOMPARE_GT(secondTickmark->y(), lastTickmark->y());
        QCOMPARE_GT(firstMinorTickmark->y(), lastMinorTickmark->y());
        QCOMPARE_LE(secondMinorTickmark->y(), firstMinorTickmark->y());
        QCOMPARE_GT(secondMinorTickmark->y(), lastMinorTickmark->y());

        // Check that some labels are laid out roughly where they should be.
        QCOMPARE_GT(firstLabel->y(), lastLabel->y());
        QCOMPARE_LE(secondLabel->y(), firstLabel->y() - tickmarkSpacing);
        QCOMPARE_GT(secondLabel->y(), lastLabel->y());
    } else { // Horizontal
        QCOMPARE_GT(gauge->implicitWidth(), gauge->implicitHeight());

        // Check that the items are laid out correctly.
        QCOMPARE_LT(valueBarItem->y(), tickmarkContainer->y());
        QCOMPARE_LT(tickmarkContainer->y(), tickmarkLabelContainer->y());

        // Check that tickmarks have the size they should.
        QCOMPARE(firstTickmark->width(), expectedTickmarkWidth);
        QCOMPARE(firstTickmark->height(), expectedTickmarkLength);
        QCOMPARE(firstMinorTickmark->width(), expectedMinorTickmarkWidth);
        QCOMPARE(firstMinorTickmark->height(), expectedMinorTickmarkLength);

        // Check that some tickmarks are laid out roughly where they should be.
        QCOMPARE_LT(firstTickmark->x(), lastTickmark->x());
        QCOMPARE_GE(secondTickmark->x(), firstTickmark->x() - tickmarkSpacing);
        QCOMPARE_LT(secondTickmark->x(), lastTickmark->x());
        QCOMPARE_LT(firstMinorTickmark->x(), lastMinorTickmark->x());
        QCOMPARE_GE(secondMinorTickmark->x(), firstMinorTickmark->x());
        QCOMPARE_LT(secondMinorTickmark->x(), lastMinorTickmark->x());

        // // Check that some labels are laid out roughly where they should be.
        QCOMPARE_LT(firstLabel->x(), lastLabel->x());
        QCOMPARE_GE(secondLabel->x(), firstLabel->x() - tickmarkSpacing);
        QCOMPARE_LT(secondLabel->x(), lastLabel->x());
    }

    QCOMPARE(firstTickmark->property("index").toInt(), 0);
    QCOMPARE(firstTickmark->property("value").toInt(), 0);
    QCOMPARE(secondTickmark->property("index").toInt(), 1);
    QCOMPARE(secondTickmark->property("value").toInt(), expectedTickmarkStepSize);
    QCOMPARE(lastTickmark->property("index").toInt(), expectedTickmarkCount - 1);
    QCOMPARE(lastTickmark->property("value").toInt(), expectedTickmarkStepSize * (expectedTickmarkCount - 1));

    QCOMPARE(firstMinorTickmark->property("index").toInt(), 0);
    QCOMPARE(firstMinorTickmark->property("value").toInt(), 2);
    QCOMPARE(secondMinorTickmark->property("index").toInt(), 1);
    QCOMPARE(secondMinorTickmark->property("value").toInt(), 4);
    QCOMPARE(lastMinorTickmark->property("index").toInt(), expectedTotalMinorTickmarkCount - 1);
    QCOMPARE(lastMinorTickmark->property("value").toInt(), 98);

    QCOMPARE(firstLabel->property("index").toInt(), 0);
    QCOMPARE(firstLabel->property("value").toInt(), 0);
    QCOMPARE(secondLabel->property("index").toInt(), 1);
    QCOMPARE(secondLabel->property("value").toInt(), expectedTickmarkStepSize);
    QCOMPARE(lastLabel->property("index").toInt(), expectedTickmarkCount - 1);
    QCOMPARE(lastLabel->property("value").toInt(), expectedTickmarkStepSize * (expectedTickmarkCount - 1));
}

QT_END_NAMESPACE

QTEST_MAIN(tst_HowToCppGauge)

#include "tst_how-to-cpp-gauge.moc"
