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

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>

#include <QtQuickControls2/private/qquickiconlabel_p.h>

#include "../shared/util.h"
#include "../shared/visualtestutil.h"

using namespace QQuickVisualTestUtil;

class tst_qquickiconlabel : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickiconlabel();

private slots:
    void display_data();
    void display();
    void spacingWithOneDelegate_data();
    void spacingWithOneDelegate();
    void emptyIconSource();
};

tst_qquickiconlabel::tst_qquickiconlabel()
{
}

void tst_qquickiconlabel::display_data()
{
    QTest::addColumn<QVector<QQuickIconLabel::Display> >("displayTypes");
    QTest::addColumn<bool>("mirrored");
    QTest::addColumn<qreal>("labelWidth");
    QTest::addColumn<qreal>("labelHeight");
    QTest::addColumn<qreal>("spacing");

    typedef QVector<QQuickIconLabel::Display> DisplayVector;
    QQuickIconLabel::Display IconOnly = QQuickIconLabel::IconOnly;
    QQuickIconLabel::Display TextOnly = QQuickIconLabel::TextOnly;
    QQuickIconLabel::Display TextBesideIcon = QQuickIconLabel::TextBesideIcon;

    QTest::addRow("IconOnly") << (DisplayVector() << IconOnly) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextOnly") << (DisplayVector() << TextOnly) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon") << (DisplayVector() << TextBesideIcon) << false << -1.0 << -1.0 << 10.0;
    QTest::addRow("IconOnly, spacing=10") << (DisplayVector() << IconOnly) << false << -1.0 << -1.0 << 10.0;
    QTest::addRow("TextOnly, spacing=10") << (DisplayVector() << TextOnly) << false << -1.0 << -1.0 << 10.0;
    QTest::addRow("TextBesideIcon, spacing=10") << (DisplayVector() << TextBesideIcon) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon => IconOnly => TextBesideIcon")
        << (DisplayVector() << TextBesideIcon << IconOnly << TextBesideIcon) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon => IconOnly => TextBesideIcon, labelWidth=400")
        << (DisplayVector() << TextBesideIcon << IconOnly << TextBesideIcon) << false << 400.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon => TextOnly => TextBesideIcon")
        << (DisplayVector() << TextBesideIcon << TextOnly << TextBesideIcon) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon => TextOnly => TextBesideIcon, labelWidth=400")
        << (DisplayVector() << TextBesideIcon << TextOnly << TextBesideIcon) << false << 400.0 << -1.0 << 0.0;
    QTest::addRow("IconOnly, mirrored") << (DisplayVector() << IconOnly) << true << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextOnly, mirrored") << (DisplayVector() << TextOnly) << true << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon, mirrored") << (DisplayVector() << TextBesideIcon) << true << -1.0 << -1.0 << 0.0;
}

void tst_qquickiconlabel::display()
{
    QFETCH(QVector<QQuickIconLabel::Display>, displayTypes);
    QFETCH(bool, mirrored);
    QFETCH(qreal, labelWidth);
    QFETCH(qreal, labelHeight);
    QFETCH(qreal, spacing);

    QQuickView view(testFileUrl("iconlabel.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);

    QQuickIconLabel *label = qobject_cast<QQuickIconLabel*>(rootItem->childItems().first());
    QVERIFY(label);
    QCOMPARE(label->spacing(), 0.0);
    QCOMPARE(label->display(), QQuickIconLabel::TextBesideIcon);
    QCOMPARE(label->isMirrored(), false);

    // Setting labelWidth allows us to test the issue where the icon's
    // width was not updated after switching between different display types.
    if (!qFuzzyCompare(labelWidth, -1)) {
        label->setWidth(labelWidth);
        QCOMPARE(label->width(), labelWidth);
    }
    if (!qFuzzyCompare(labelHeight, -1)) {
        label->setHeight(labelHeight);
        QCOMPARE(label->height(), labelHeight);
    }

    QQuickItem *icon = label->icon();
    QVERIFY(icon);

    QQuickItem *text = label->label();
    QVERIFY(text);

    label->setMirrored(mirrored);
    QCOMPARE(label->isMirrored(), mirrored);

    label->setSpacing(spacing);
    QCOMPARE(label->spacing(), spacing);

    const qreal horizontalPadding = label->leftPadding() + label->rightPadding();
    const qreal verticalPadding = label->topPadding() + label->bottomPadding();

    // Test that the icon and text are correctly positioned and sized after
    // setting several different display types in succession.
    for (QQuickIconLabel::Display displayType : qAsConst(displayTypes)) {
        label->setDisplay(displayType);
        QCOMPARE(label->display(), displayType);

        const qreal horizontalCenter = label->width() / 2;
        const qreal verticalCenter = label->height() / 2;

        switch (displayType) {
        case QQuickIconLabel::IconOnly:
            QCOMPARE(icon->x(), horizontalCenter - icon->width() / 2);
            QCOMPARE(icon->y(), verticalCenter - icon->height() / 2);
            QCOMPARE(icon->width(), icon->implicitWidth());
            QCOMPARE(icon->height(), icon->implicitHeight());
            QCOMPARE(icon->isVisible(), true);
            QCOMPARE(text->isVisible(), false);
            QCOMPARE(label->implicitWidth(), icon->implicitWidth() + horizontalPadding);
            QCOMPARE(label->implicitHeight(), icon->implicitHeight() + verticalPadding);
            break;
        case QQuickIconLabel::TextOnly:
            QCOMPARE(icon->isVisible(), false);
            QCOMPARE(text->x(), horizontalCenter - text->width() / 2);
            QCOMPARE(text->y(), verticalCenter - text->height() / 2);
            QCOMPARE(text->width(), text->implicitWidth());
            QCOMPARE(text->height(), text->implicitHeight());
            QCOMPARE(text->isVisible(), true);
            QCOMPARE(label->implicitWidth(), text->implicitWidth() + horizontalPadding);
            QCOMPARE(label->implicitHeight(), text->implicitHeight() + verticalPadding);
            break;
        case QQuickIconLabel::TextBesideIcon:
        default:
            const qreal combinedWidth = icon->width() + label->spacing() + text->width();
            const qreal contentX = horizontalCenter - combinedWidth / 2;
            QCOMPARE(icon->x(), contentX + (label->isMirrored() ? text->width() + label->spacing() : 0));
            QCOMPARE(icon->y(), verticalCenter - icon->height() / 2);
            QCOMPARE(icon->width(), icon->implicitWidth());
            QCOMPARE(icon->height(), icon->implicitHeight());
            QCOMPARE(icon->isVisible(), true);
            QCOMPARE(text->x(), contentX + (label->isMirrored() ? 0 : icon->width() + label->spacing()));
            QCOMPARE(text->y(), verticalCenter - text->height() / 2);
            QCOMPARE(text->width(), text->implicitWidth());
            QCOMPARE(text->height(), text->implicitHeight());
            QCOMPARE(text->isVisible(), true);
            QCOMPARE(label->implicitWidth(), combinedWidth + horizontalPadding);
            QCOMPARE(label->implicitHeight(), qMax(icon->implicitHeight(), text->implicitHeight()) + verticalPadding);
            break;
        }
    }
}

void tst_qquickiconlabel::spacingWithOneDelegate_data()
{
    QTest::addColumn<QString>("qmlFileName");

    QTest::addRow("spacingWithOnlyIcon") << QStringLiteral("spacingWithOnlyIcon.qml");
    QTest::addRow("spacingWithOnlyText") << QStringLiteral("spacingWithOnlyText.qml");
}

void tst_qquickiconlabel::spacingWithOneDelegate()
{
    QFETCH(QString, qmlFileName);

    QQuickView view(testFileUrl(qmlFileName));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);

    QQuickIconLabel *label = qobject_cast<QQuickIconLabel*>(rootItem->childItems().first());
    QVERIFY(label);
    QQuickItem *delegate = nullptr;
    if (label->icon()) {
        QVERIFY(!label->label());
        delegate = label->icon();
    } else {
        QVERIFY(!label->icon());
        delegate = label->label();
    }

    QVERIFY(delegate);
    QCOMPARE(delegate->x(), 0.0);
    QCOMPARE(delegate->width(), label->width());
}

void tst_qquickiconlabel::emptyIconSource()
{
    QQuickView view(testFileUrl("iconlabel.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);

    QQuickIconLabel *label = qobject_cast<QQuickIconLabel*>(rootItem->childItems().first());
    QVERIFY(label);
    QCOMPARE(label->spacing(), 0.0);
    QCOMPARE(label->display(), QQuickIconLabel::TextBesideIcon);
    QCOMPARE(label->isMirrored(), false);

    QQuickItem *icon = label->icon();
    QVERIFY(icon);

    QQuickItem *text = label->label();
    QVERIFY(text);
    qreal horizontalCenter = label->width() / 2;
    const qreal combinedWidth = icon->width() + text->width();
    const qreal contentX = horizontalCenter - combinedWidth / 2;
    // The text should be positioned next to an item.
    QCOMPARE(text->x(), contentX + icon->width() + label->spacing());

    // Now give the label an explicit width large enough so that implicit size
    // changes in its children don't affect its implicit size.
    label->setWidth(label->implicitWidth() + 200);
    label->setHeight(label->implicitWidth() + 100);
    QVERIFY(icon->property("source").isValid());
    QVERIFY(icon->setProperty("source", QUrl()));
    horizontalCenter = label->width() / 2;
    QCOMPARE(text->x(), horizontalCenter - text->width() / 2);
}

QTEST_MAIN(tst_qquickiconlabel)

#include "tst_qquickiconlabel.moc"
