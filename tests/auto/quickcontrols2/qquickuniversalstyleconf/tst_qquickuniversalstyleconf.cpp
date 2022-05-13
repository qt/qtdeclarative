// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickControls2/private/qquickstyle_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>

using namespace QQuickControlsTestUtils;

class tst_qquickuniversalstyleconf : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qquickuniversalstyleconf();

private slots:
    void conf();
};

tst_qquickuniversalstyleconf::tst_qquickuniversalstyleconf()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquickuniversalstyleconf::conf()
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
    QCOMPARE(label->property("color").value<QColor>(), QColor("#E51400"));
    QCOMPARE(label->property("font").value<QFont>(), customFont);
}

QTEST_MAIN(tst_qquickuniversalstyleconf)

#include "tst_qquickuniversalstyleconf.moc"
