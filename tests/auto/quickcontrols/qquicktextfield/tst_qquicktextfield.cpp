// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtesttouch.h>

#include <QtGui/qfontmetrics.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/qtestsupport_gui.h>
#include <QtQuick/qquickview.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>
#include <QtQuickControlsTestUtils/private/qtest_quickcontrols_p.h>

class tst_QQuickTextField : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickTextField();

private slots:
    void initTestCase() override;
    void touchscreenDoesNotSelect_data();
    void touchscreenDoesNotSelect();

private:
    QScopedPointer<QPointingDevice> touchDevice = QScopedPointer<QPointingDevice>(QTest::createTouchDevice());
};

tst_QQuickTextField::tst_QQuickTextField()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickTextField::initTestCase()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() > 23)
        QSKIP("Crashes on Android 7+, figure out why (QTBUG-107028)");
#endif
    QQmlDataTest::initTestCase();
    qputenv("QML_NO_TOUCH_COMPRESSION", "1");
}

void tst_QQuickTextField::touchscreenDoesNotSelect_data()
{
    QTest::addColumn<QUrl>("src");
    QTest::addColumn<bool>("setEnv");
    QTest::addColumn<bool>("selectByMouse");
    QTest::addColumn<bool>("selectByTouch");
    QTest::newRow("new default") << testFileUrl("mouseselection_default.qml") << false << true << false;
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    QTest::newRow("putenv") << testFileUrl("mouseselection_default.qml") << true << false << false;
    QTest::newRow("old_import") << testFileUrl("mouseselection_old_default.qml") << false << true << false;
    QTest::newRow("old+putenv") << testFileUrl("mouseselection_old_default.qml") << true << false << false;
    QTest::newRow("old+putenv+selectByMouse") << testFileUrl("mouseselection_old_overridden.qml") << true << true << true;
#endif
}

void tst_QQuickTextField::touchscreenDoesNotSelect()
{
    QFETCH(QUrl, src);
    QFETCH(bool, setEnv);
    QFETCH(bool, selectByMouse);
    QFETCH(bool, selectByTouch);

    if (setEnv)
        qputenv("QT_QUICK_CONTROLS_TEXT_SELECTION_BEHAVIOR", "old");
    else
        qunsetenv("QT_QUICK_CONTROLS_TEXT_SELECTION_BEHAVIOR");

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, src));

    QQuickTextField *textField = qobject_cast<QQuickTextField *>(window.rootObject());
    QVERIFY(textField != nullptr);
    QCOMPARE(textField->selectByMouse(), selectByMouse);
    textField->setSelectByMouse(true); // enable selection with pre-6.4 import version
    QVERIFY(textField->selectedText().isEmpty());

    if (selectByMouse) {
        // press-drag-and-release from x1 to x2
        const int x1 = textField->leftPadding();
        const int x2 = textField->width() / 2;
        const int y = textField->height() / 2;
        QTest::touchEvent(&window, touchDevice.data()).press(0, QPoint(x1,y), &window);
        QTest::touchEvent(&window, touchDevice.data()).move(0, QPoint(x2,y), &window);
        QTest::touchEvent(&window, touchDevice.data()).release(0, QPoint(x2,y), &window);
        QQuickTouchUtils::flush(&window);
        // if the env var is set, fall back to old behavior: touch swipe _does_ select text if selectByMouse is true
        QCOMPARE(textField->selectedText().isEmpty(), !selectByTouch);
    }
}

QTEST_QUICKCONTROLS_MAIN(tst_QQuickTextField)

#include "tst_qquicktextfield.moc"
