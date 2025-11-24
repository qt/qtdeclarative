// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtesttouch.h>

#include <QtCore/private/qabstractanimation_p.h>
#include <QtGui/qfontmetrics.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/qtestsupport_gui.h>
#include <QtQuick/qquickview.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>
#include <QtQuickTemplates2/private/qquicktextarea_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControlsTestUtils/private/qtest_quickcontrols_p.h>

using namespace QQuickControlsTestUtils;
using namespace QQuickVisualTestUtils;

class tst_QQuickTextArea : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickTextArea();

private slots:
    void initTestCase() override;
    void touchscreenDoesNotSelect_data();
    void touchscreenDoesNotSelect();
    void touchscreenSetsFocusAndMovesCursor();
    void customContextMenuOnRelease_data();
    void customContextMenuOnRelease();
    void testCursorPositionChangedOnDeleteStartWord();

private:
    QScopedPointer<QPointingDevice> touchDevice = QScopedPointer<QPointingDevice>(QTest::createTouchDevice());

    bool hasClipboardSupport =
#if QT_CONFIG(clipboard)
        true;
#else
        false;
#endif
};

tst_QQuickTextArea::tst_QQuickTextArea()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickTextArea::initTestCase()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() > 23)
        QSKIP("Crashes on Android 7+, figure out why (QTBUG-107028)");
#endif
    QQmlDataTest::initTestCase();
    qputenv("QML_NO_TOUCH_COMPRESSION", "1");
    // Showing a native menu is a blocking call, so the test will timeout.
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuWindows);

    QUnifiedTimer::instance()->setSpeedModifier(10);
}

void tst_QQuickTextArea::touchscreenDoesNotSelect_data()
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

void tst_QQuickTextArea::touchscreenDoesNotSelect()
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

    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(textEditObject != nullptr);
    QCOMPARE(textEditObject->selectByMouse(), selectByMouse);
    textEditObject->setSelectByMouse(true); // enable selection with pre-6.4 import version
    QVERIFY(textEditObject->selectedText().isEmpty());

    if (selectByMouse) {
        // press-drag-and-release from x1 to x2
        const int x1 = textEditObject->leftPadding();
        const int x2 = textEditObject->width() / 2;
        // Account for all styles by being aware of vertical padding.
        // contentHeight / 2 should be half the line height considering that we only have one line of text.
        const int y = textEditObject->topPadding() + textEditObject->contentHeight() / 2;
        QTest::touchEvent(&window, touchDevice.data()).press(0, QPoint(x1,y), &window);
        QTest::touchEvent(&window, touchDevice.data()).move(0, QPoint(x2,y), &window);
        QTest::touchEvent(&window, touchDevice.data()).release(0, QPoint(x2,y), &window);
        QQuickTouchUtils::flush(&window);
        // if the import version is old enough, fall back to old behavior: touch swipe _does_ select text if selectByMouse is true
        QCOMPARE(textEditObject->selectedText().isEmpty(), !selectByTouch);
    }
}

void tst_QQuickTextArea::touchscreenSetsFocusAndMovesCursor()
{
    SKIP_IF_NO_WINDOW_ACTIVATION;
    qunsetenv("QT_QUICK_CONTROLS_TEXT_SELECTION_BEHAVIOR");

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("twoInAColumn.qml")));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QQuickTextEdit *top = window.rootObject()->findChild<QQuickTextEdit*>("top");
    QVERIFY(top);
    QQuickTextEdit *bottom = window.rootObject()->findChild<QQuickTextEdit*>("bottom");
    QVERIFY(bottom);
    const auto len = bottom->text().size();

    // tap the bottom field
    const qreal yOffset = bottom->topPadding() + 6; // where to tap or drag to hit the text
    QPoint p1 = bottom->mapToScene({60, yOffset}).toPoint();
    QTest::touchEvent(&window, touchDevice.data()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    // text cursor is at 0 by default, on press
    QCOMPARE(bottom->cursorPosition(), 0);
    // the focus changes and the cursor moves after release (not after press, as in TextEdit)
    QTest::touchEvent(&window, touchDevice.data()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(qApp->focusObject(), bottom);
    QTRY_COMPARE_GT(bottom->cursorPosition(), 0);

    // typing a character inserts it at the cursor position
    QVERIFY(!bottom->text().contains('q'));
    QTest::keyClick(&window, Qt::Key_Q);
    QCOMPARE(bottom->text().size(), len + 1);
    QCOMPARE_GT(bottom->text().indexOf('q'), 0);

    // press-drag-and-release from p1 to p2 on the top field
    p1 = top->mapToScene({0, yOffset}).toPoint();
    QPoint p2 = top->mapToScene({76, yOffset}).toPoint();
    QTest::touchEvent(&window, touchDevice.data()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchDevice.data()).move(0, p2, &window);
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchDevice.data()).release(0, p2, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(qApp->focusObject(), top);
    QVERIFY(top->selectedText().isEmpty());
    QCOMPARE_GT(top->cursorPosition(), 0);

    // touch-drag did not select text, but mouse-drag from p2 back to p1
    // does select the first part of the text, and leave the cursor at the beginning
    QTest::mousePress(&window, Qt::LeftButton, {}, p2);
    QTest::mouseMove(&window, p1);
    QTest::mouseRelease(&window, Qt::LeftButton, {}, p1);
    QCOMPARE(top->cursorPosition(), 0);
    QCOMPARE_GT(top->selectedText().size(), 0);
}

void tst_QQuickTextArea::customContextMenuOnRelease_data()
{
    QTest::addColumn<QUrl>("qmlUrl");
    QTest::addColumn<QQuickPopup::PopupType>("popupType");

    QTest::newRow("Item menu instead of attached menu, TapHandler")
            << testFileUrl("customContextMenuNotAttachedOnRelease.qml") << QQuickPopup::Item;
    QTest::newRow("Item menu instead of attached menu, MouseArea")
        << testFileUrl("customContextMenuNotAttachedOnReleaseMouseArea.qml") << QQuickPopup::Item;
    QTest::newRow("Item menu instead of default menu, TapHandler")
            << testFileUrl("customContextMenuOnRelease.qml") << QQuickPopup::Item;
    QTest::newRow("Item menu instead of default menu, MouseArea")
            << testFileUrl("customContextMenuOnReleaseMouseArea.qml") << QQuickPopup::Item;
    QTest::newRow("Sibling item menu instead of default menu, TapHandler")
            << testFileUrl("customContextMenuOnReleaseInSibling.qml") << QQuickPopup::Item;
    QTest::newRow("Sibling item menu instead of default menu, MouseArea")
            << testFileUrl("customContextMenuOnReleaseSiblingMouseArea.qml") << QQuickPopup::Item;
    if (arePopupWindowsSupported()) {
        QTest::newRow("Windowed menu instead of attached menu, TapHandler")
                << testFileUrl("customContextMenuNotAttachedOnRelease.qml") << QQuickPopup::Window;
        QTest::newRow("Windowed menu instead of attached menu, MouseArea")
            << testFileUrl("customContextMenuNotAttachedOnReleaseMouseArea.qml") << QQuickPopup::Window;
        QTest::newRow("Windowed menu instead of default menu, TapHandler")
                << testFileUrl("customContextMenuOnRelease.qml") << QQuickPopup::Window;
        QTest::newRow("Windowed menu instead of default menu, MouseArea")
            << testFileUrl("customContextMenuOnReleaseMouseArea.qml") << QQuickPopup::Window;
        QTest::newRow("Sibling item menu instead of default menu, TapHandler")
                << testFileUrl("customContextMenuOnReleaseInSibling.qml") << QQuickPopup::Window;
        QTest::newRow("Sibling item menu instead of default menu, MouseArea")
                << testFileUrl("customContextMenuOnReleaseSiblingMouseArea.qml") << QQuickPopup::Window;
    }
}

void tst_QQuickTextArea::customContextMenuOnRelease()
{
    QFETCH(QUrl, qmlUrl);
    QFETCH(QQuickPopup::PopupType, popupType);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, qmlUrl));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto *ourContextMenu = window.rootObject()->property("ourContextMenu").value<QQuickMenu *>();
    std::unique_ptr<QSignalSpy> ourMenuOpenedSpy = nullptr;
    if (ourContextMenu) {
        ourContextMenu->setPopupType(popupType);
        ourMenuOpenedSpy = std::make_unique<QSignalSpy>(ourContextMenu, &QQuickMenu::opened);
    }

    // Right click on the TextArea to open the context menu. The user's custom context menu
    // should be visible, but not ours (ContextMenu's).
    auto *textArea = window.rootObject()->property("textArea").value<QQuickTextArea *>();
    QVERIFY(textArea);
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    auto *userContextMenu = window.rootObject()->property("userContextMenu").value<QQuickMenu *>();
    QVERIFY(userContextMenu);
    QTRY_VERIFY(userContextMenu->isOpened());

    // Delivery of the QContextMenuEvent was prevented, so neither the default context menu
    // nor the attached context menu ever even tried to open.
    if (ourContextMenu)
        QCOMPARE(ourMenuOpenedSpy->size(), 0);
    else
        QVERIFY(window.rootObject()->findChildren<QQuickMenu *>().size() <= 1);
}

void tst_QQuickTextArea::testCursorPositionChangedOnDeleteStartWord()
{
    const QString initialText = "The quick brown fox jumps over the lazy dog.";
    const QString expectedText = "The quick brown  jumps over the lazy dog.";
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("mouseselection_default.qml")));

    QQuickTextEdit *textField = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(textField != nullptr);

    textField->setText(initialText);
    textField->setCursorPosition(19);

    QSignalSpy spy(textField, &QQuickTextEdit::cursorPositionChanged);

    textField->forceActiveFocus();

    QTest::keySequence(&window, QKeySequence::DeleteStartOfWord);

    QCOMPARE(spy.count(), 1);

    QCOMPARE(textField->text(), expectedText);

    QCOMPARE(textField->cursorPosition(), 16);
}

QTEST_QUICKCONTROLS_MAIN(tst_QQuickTextArea)

#include "tst_qquicktextarea.moc"
