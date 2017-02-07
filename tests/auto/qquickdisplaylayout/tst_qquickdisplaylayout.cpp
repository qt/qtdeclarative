/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qvector.h>

#include <qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>

#include <QtQuickControls2/private/qquickdisplaylayout_p.h>

#include "../shared/util.h"
#include "../shared/visualtestutil.h"

using namespace QQuickVisualTestUtil;

class tst_qquickdisplaylayout : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickdisplaylayout();

private slots:
    void display_data();
    void display();
    void explicitLayoutSize();
};

tst_qquickdisplaylayout::tst_qquickdisplaylayout()
{
}

void tst_qquickdisplaylayout::display_data()
{
    QTest::addColumn<QVector<QQuickDisplayLayout::Display> >("displayTypes");
    QTest::addColumn<bool>("mirrored");
    QTest::addColumn<qreal>("layoutWidth");
    QTest::addColumn<qreal>("layoutHeight");
    QTest::addColumn<qreal>("spacing");

    typedef QVector<QQuickDisplayLayout::Display> DisplayVector;
    QQuickDisplayLayout::Display IconOnly = QQuickDisplayLayout::IconOnly;
    QQuickDisplayLayout::Display TextOnly = QQuickDisplayLayout::TextOnly;
    QQuickDisplayLayout::Display TextBesideIcon = QQuickDisplayLayout::TextBesideIcon;

    QTest::addRow("IconOnly") << (DisplayVector() << IconOnly) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextOnly") << (DisplayVector() << TextOnly) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon") << (DisplayVector() << TextBesideIcon) << false << -1.0 << -1.0 << 10.0;
    QTest::addRow("IconOnly, spacing=10") << (DisplayVector() << IconOnly) << false << -1.0 << -1.0 << 10.0;
    QTest::addRow("TextOnly, spacing=10") << (DisplayVector() << TextOnly) << false << -1.0 << -1.0 << 10.0;
    QTest::addRow("TextBesideIcon, spacing=10") << (DisplayVector() << TextBesideIcon) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon => IconOnly => TextBesideIcon")
        << (DisplayVector() << TextBesideIcon << IconOnly << TextBesideIcon) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon => IconOnly => TextBesideIcon, layoutWidth=400")
        << (DisplayVector() << TextBesideIcon << IconOnly << TextBesideIcon) << false << 400.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon => TextOnly => TextBesideIcon")
        << (DisplayVector() << TextBesideIcon << TextOnly << TextBesideIcon) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon => TextOnly => TextBesideIcon, layoutWidth=400")
        << (DisplayVector() << TextBesideIcon << TextOnly << TextBesideIcon) << false << 400.0 << -1.0 << 0.0;
    QTest::addRow("IconOnly, mirrored") << (DisplayVector() << IconOnly) << true << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextOnly, mirrored") << (DisplayVector() << TextOnly) << true << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon, mirrored") << (DisplayVector() << TextBesideIcon) << true << -1.0 << -1.0 << 0.0;
}

void tst_qquickdisplaylayout::display()
{
    QFETCH(QVector<QQuickDisplayLayout::Display>, displayTypes);
    QFETCH(bool, mirrored);
    QFETCH(qreal, layoutWidth);
    QFETCH(qreal, layoutHeight);
    QFETCH(qreal, spacing);

    QQuickView view(testFileUrl("layout.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);

    QQuickDisplayLayout *layout = qobject_cast<QQuickDisplayLayout*>(rootItem->childItems().first());
    QVERIFY(layout);
    QCOMPARE(layout->spacing(), 0.0);
    QCOMPARE(layout->display(), QQuickDisplayLayout::TextBesideIcon);
    QCOMPARE(layout->isMirrored(), false);

    // Setting layoutWidth allows us to test the issue where the icon's
    // width was not updated after switching between different display types.
    if (!qFuzzyCompare(layoutWidth, -1)) {
        layout->setWidth(layoutWidth);
        QCOMPARE(layout->width(), layoutWidth);
    }
    if (!qFuzzyCompare(layoutHeight, -1)) {
        layout->setHeight(layoutHeight);
        QCOMPARE(layout->height(), layoutHeight);
    }

    QQuickItem *icon = layout->icon();
    QVERIFY(icon);

    QQuickItem *text = layout->text();
    QVERIFY(text);

    QSignalSpy mirroredSpy(layout, SIGNAL(mirroredChanged()));
    bool expectChange = layout->isMirrored() != mirrored;
    layout->setMirrored(mirrored);
    QCOMPARE(layout->isMirrored(), mirrored);
    QCOMPARE(mirroredSpy.count(), expectChange ? 1 : 0);

    QSignalSpy spacingSpy(layout, SIGNAL(spacingChanged()));
    expectChange = !qFuzzyCompare(layout->spacing(), spacing);
    layout->setSpacing(spacing);
    QCOMPARE(layout->spacing(), spacing);
    QCOMPARE(spacingSpy.count(), expectChange ? 1 : 0);

    const qreal horizontalPadding = layout->leftPadding() + layout->rightPadding();
    const qreal verticalPadding = layout->topPadding() + layout->bottomPadding();

    // Test that the icon and text are correctly positioned and sized after
    // setting several different display types in succession.
    for (QQuickDisplayLayout::Display displayType : qAsConst(displayTypes)) {
        QSignalSpy displaySpy(layout, SIGNAL(displayChanged()));
        const bool expectChange = layout->display() != displayType;
        layout->setDisplay(displayType);
        QCOMPARE(layout->display(), displayType);
        QCOMPARE(displaySpy.count(), expectChange ? 1 : 0);

        const qreal horizontalCenter = layout->width() / 2;
        const qreal verticalCenter = layout->height() / 2;

        switch (displayType) {
        case QQuickDisplayLayout::IconOnly:
            QCOMPARE(icon->x(), horizontalCenter - icon->width() / 2);
            QCOMPARE(icon->y(), verticalCenter - icon->height() / 2);
            QCOMPARE(icon->width(), icon->implicitWidth());
            QCOMPARE(icon->height(), icon->implicitHeight());
            QCOMPARE(icon->isVisible(), true);
            QCOMPARE(text->isVisible(), false);
            QCOMPARE(layout->implicitWidth(), icon->implicitWidth() + horizontalPadding);
            QCOMPARE(layout->implicitHeight(), icon->implicitHeight() + verticalPadding);
            break;
        case QQuickDisplayLayout::TextOnly:
            QCOMPARE(icon->isVisible(), false);
            QCOMPARE(text->x(), horizontalCenter - text->width() / 2);
            QCOMPARE(text->y(), verticalCenter - text->height() / 2);
            QCOMPARE(text->width(), text->implicitWidth());
            QCOMPARE(text->height(), text->implicitHeight());
            QCOMPARE(text->isVisible(), true);
            QCOMPARE(layout->implicitWidth(), text->implicitWidth() + horizontalPadding);
            QCOMPARE(layout->implicitHeight(), text->implicitHeight() + verticalPadding);
            break;
        case QQuickDisplayLayout::TextBesideIcon:
        default:
            const qreal combinedWidth = icon->width() + layout->spacing() + text->width();
            const qreal contentX = horizontalCenter - combinedWidth / 2;
            QCOMPARE(icon->x(), contentX + (layout->isMirrored() ? text->width() + layout->spacing() : 0));
            QCOMPARE(icon->y(), verticalCenter - icon->height() / 2);
            QCOMPARE(icon->width(), icon->implicitWidth());
            QCOMPARE(icon->height(), icon->implicitHeight());
            QCOMPARE(icon->isVisible(), true);
            QCOMPARE(text->x(), contentX + (layout->isMirrored() ? 0 : icon->width() + layout->spacing()));
            QCOMPARE(text->y(), verticalCenter - text->height() / 2);
            QCOMPARE(text->width(), text->implicitWidth());
            QCOMPARE(text->height(), text->implicitHeight());
            QCOMPARE(text->isVisible(), true);
            QCOMPARE(layout->implicitWidth(), combinedWidth + horizontalPadding);
            QCOMPARE(layout->implicitHeight(), qMax(icon->implicitHeight(), text->implicitHeight()) + verticalPadding);
            break;
        }
    }
}

void tst_qquickdisplaylayout::explicitLayoutSize()
{
    QQuickView view(testFileUrl("layout.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);

    QQuickDisplayLayout *layout = qobject_cast<QQuickDisplayLayout*>(rootItem->childItems().first());
    QVERIFY(layout);

    QQuickItem *icon = layout->icon();
    QVERIFY(icon);

    QQuickItem *text = layout->text();
    QVERIFY(text);

    // Make the row larger than its implicit width, and check that
    // the items are still positioned correctly.
    layout->setWidth(layout->implicitWidth() + 100);
    QCOMPARE(icon->x(), 0.0);
    QCOMPARE(text->x(), icon->implicitWidth());

    layout->setMirrored(true);
    QCOMPARE(icon->x(), layout->width() - icon->implicitWidth());
    QCOMPARE(text->x(), 0.0);
}

QTEST_MAIN(tst_qquickdisplaylayout)

#include "tst_qquickdisplaylayout.moc"
