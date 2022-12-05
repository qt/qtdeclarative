// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickControls2/private/qquickstyle_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>

using namespace QQuickControlsTestUtils;

class tst_qquickmaterialstyleconf : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qquickmaterialstyleconf();

private slots:
    void conf();
    void variants_data();
    void variants();
};

tst_qquickmaterialstyleconf::tst_qquickmaterialstyleconf()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquickmaterialstyleconf::conf()
{
    QQuickControlsApplicationHelper helper(this, QLatin1String("applicationwindow.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());

    QFont customFont;
    customFont.setFamilies(QStringList{QLatin1String("Courier")});
    customFont.setPixelSize(22);

    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    // We specified a custom background color and font, so the window should have them.
    QCOMPARE(window->property("color").value<QColor>(), QColor("#444444"));
    QCOMPARE(window->property("font").value<QFont>(), customFont);

    // We specified a custom foreground color and font, so the label should have them.
    QQuickItem *label = window->property("label").value<QQuickItem*>();
    QVERIFY(label);
    QCOMPARE(label->property("color").value<QColor>(), QColor("#F44336"));
    QCOMPARE(label->property("font").value<QFont>(), customFont);
}

void tst_qquickmaterialstyleconf::variants_data()
{
    QTest::addColumn<QByteArray>("confPath");
    QTest::addColumn<int>("expectedButtonHeight");
    // Just to ensure that the correct conf is loaded.
    QTest::addColumn<QColor>("expectedColor");

    // (40 button height + 12 touchable area)
    QTest::newRow("normal") << QByteArray(":/variant-normal.conf") << 52 << QColor::fromRgb(0x123456);
    // We specified a custom variant (dense), so the button should be small.
    // (32 button height + 12 touchable area)
    QTest::newRow("dense") << QByteArray(":/variant-dense.conf") << 44 << QColor::fromRgb(0x789abc);
}

void tst_qquickmaterialstyleconf::variants()
{
    QFETCH(QByteArray, confPath);
    QFETCH(int, expectedButtonHeight);
    QFETCH(QColor, expectedColor);

    qmlClearTypeRegistrations();
    QQuickStylePrivate::reset();
    qputenv("QT_QUICK_CONTROLS_CONF", confPath);

    QQuickControlsApplicationHelper helper(this, QLatin1String("applicationwindow.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickItem *label = window->property("label").value<QQuickItem*>();
    QVERIFY(label);
    QCOMPARE(label->property("color").value<QColor>(), expectedColor);

    QQuickItem *button = window->property("button").value<QQuickItem*>();
    QVERIFY(button);
    QCOMPARE(button->height(), expectedButtonHeight);
}

QTEST_MAIN(tst_qquickmaterialstyleconf)

#include "tst_qquickmaterialstyleconf.moc"
