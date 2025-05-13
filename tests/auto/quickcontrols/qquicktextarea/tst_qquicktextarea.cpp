// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtesttouch.h>

#include <QtGui/qclipboard.h>
#include <QtGui/qfontmetrics.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/qtestsupport_gui.h>
#include <QtQuick/qquickview.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTemplates2/private/qquickcontextmenu_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p.h>
#include <QtQuickTemplates2/private/qquickmenuitem_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>
#include <QtQuickTemplates2/private/qquickpopupwindow_p_p.h>
#include <QtQuickTemplates2/private/qquicktextarea_p.h>
#include <QtQuickControlsTestUtils/private/qtest_quickcontrols_p.h>

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
    void contextMenuCut();
    void contextMenuCopy();
    void contextMenuPaste();
    void contextMenuDelete();
    void contextMenuSelectAll();

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

static const auto mementoStr = QLatin1String("Memento");
static const auto moriStr = QLatin1String("mori");
static const auto mementoMoriStr = QLatin1String("Memento mori");

void tst_QQuickTextArea::contextMenuCut()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("contextMenu.qml")));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto *textArea = window.rootObject()->property("textArea").value<QQuickTextArea *>();
    QVERIFY(textArea);
    textArea->forceActiveFocus();

    // Right click on the TextArea to open the context menu.
    // Right-clicking without a selection should result in the Cut menu item being disabled.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    auto *contextMenu = textArea->findChild<QQuickContextMenu *>();
    QVERIFY(contextMenu);
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    auto *cutMenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->menu()->itemAt(0));
    QCOMPARE(cutMenuItem->text(), "Cut");
    QVERIFY(!cutMenuItem->isEnabled());
    QVERIFY(textArea->selectedText().isEmpty());

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Right-clicking with a selection should result in the Cut menu item being enabled
    // if Qt was built with clipboard support.
    textArea->select(mementoStr.length(), textArea->length());
    const QString cutText = QLatin1Char(' ') + moriStr;
    QCOMPARE(textArea->selectedText(), cutText);
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    QCOMPARE(cutMenuItem->isEnabled(), hasClipboardSupport);

    // Click on the Cut menu item (if enabled) and close the menu.
#if QT_CONFIG(clipboard)
    auto *contextMenuPrivate = QQuickPopupPrivate::get(contextMenu->menu());
    QQuickWindow *contextMenuPopupWindow = &window;
    if (contextMenuPrivate->usePopupWindow())
        contextMenuPopupWindow = contextMenuPrivate->popupWindow;
    QTest::mouseClick(contextMenuPopupWindow, Qt::LeftButton, Qt::NoModifier, mapCenterToWindow(cutMenuItem));
    QCOMPARE(textArea->text(), mementoStr);
    QCOMPARE(qGuiApp->clipboard()->text(), cutText);
#else
    QTest::keyClick(&window, Qt::Key_Escape);
#endif
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Make the TextArea read-only. Cut should no longer be enabled.
    textArea->setReadOnly(true);
    textArea->selectAll();
    // Right click on the TextArea to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    QCOMPARE(cutMenuItem->text(), "Cut");
    QVERIFY(!cutMenuItem->isEnabled());

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());
}

void tst_QQuickTextArea::contextMenuCopy()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("contextMenu.qml")));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto *textArea = window.rootObject()->property("textArea").value<QQuickTextArea *>();
    QVERIFY(textArea);
    textArea->forceActiveFocus();

    // Right click on the TextArea to open the context menu.
    // Right-clicking without a selection should result in the Copy menu item being disabled.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    auto *contextMenu = textArea->findChild<QQuickContextMenu *>();
    QVERIFY(contextMenu);
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    auto *copyMenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->menu()->itemAt(1));
    QVERIFY(copyMenuItem);
    QCOMPARE(copyMenuItem->text(), "Copy");
    QVERIFY(!copyMenuItem->isEnabled());

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Right-clicking with a selection should result in the Copy menu item being enabled
    // if Qt was built with clipboard support.
    textArea->select(0, mementoStr.length());
    QCOMPARE(textArea->selectedText(), mementoStr);
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    QCOMPARE(copyMenuItem->isEnabled(), hasClipboardSupport);

    // Click on the Copy menu item (if enabled) and close the menu.
#if QT_CONFIG(clipboard)
    auto *contextMenuPrivate = QQuickPopupPrivate::get(contextMenu->menu());
    QQuickWindow *contextMenuPopupWindow = &window;
    if (contextMenuPrivate->usePopupWindow())
        contextMenuPopupWindow = contextMenuPrivate->popupWindow;
    QTest::mouseClick(contextMenuPopupWindow, Qt::LeftButton, Qt::NoModifier, mapCenterToWindow(copyMenuItem));
    QCOMPARE(textArea->text(), mementoMoriStr);
    const auto *clipboard = QGuiApplication::clipboard();
    QCOMPARE(clipboard->text(), mementoStr);
#else
    QTest::keyClick(&window, Qt::Key_Escape);
#endif
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Make the TextArea read-only. Copy should still be enabled if Qt was built with clipboard support.
    textArea->setReadOnly(true);
    // Right click on the TextArea to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    QCOMPARE(copyMenuItem->text(), "Copy");
    // Select some text.
    textArea->select(0, mementoStr.length());
    QCOMPARE(copyMenuItem->isEnabled(), hasClipboardSupport);

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());
}

void tst_QQuickTextArea::contextMenuPaste()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("contextMenu.qml")));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto *textArea = window.rootObject()->property("textArea").value<QQuickTextArea *>();
    QVERIFY(textArea);
    textArea->forceActiveFocus();

#if QT_CONFIG(clipboard)
    auto *clipboard = QGuiApplication::clipboard();
    clipboard->setText(" 🫠");
#endif

    // For some reason the cursor is at the beginning of the text, even when
    // right-clicking to the right of it, so first left-click.
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, mapCenterToWindow(textArea));
    // Right click on the TextArea to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    auto *contextMenu = textArea->findChild<QQuickContextMenu *>();
    QVERIFY(contextMenu);
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    auto *pasteMenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->menu()->itemAt(2));
    QVERIFY(pasteMenuItem);
    QCOMPARE(pasteMenuItem->text(), "Paste");
    QCOMPARE(pasteMenuItem->isEnabled(), hasClipboardSupport);

    // Click on the Paste menu item (if enabled) and close the menu.
    auto *contextMenuPrivate = QQuickPopupPrivate::get(contextMenu->menu());
    QQuickWindow *contextMenuPopupWindow = &window;
    if (contextMenuPrivate->usePopupWindow())
        contextMenuPopupWindow = contextMenuPrivate->popupWindow;
    QTest::mouseClick(contextMenuPopupWindow, Qt::LeftButton, Qt::NoModifier, mapCenterToWindow(pasteMenuItem));
#if QT_CONFIG(clipboard)
    QCOMPARE(textArea->text(), mementoMoriStr + clipboard->text());
#else
    QTest::keyClick(&window, Qt::Key_Escape);
#endif
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Make the TextArea read-only. Paste should no longer be enabled.
    textArea->setReadOnly(true);
    // Right click on the TextArea to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    QCOMPARE(pasteMenuItem->text(), "Paste");
    QVERIFY(!pasteMenuItem->isEnabled());

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());
}

void tst_QQuickTextArea::contextMenuDelete()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("contextMenu.qml")));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto *textArea = window.rootObject()->property("textArea").value<QQuickTextArea *>();
    QVERIFY(textArea);
    textArea->forceActiveFocus();

    // Right click on the TextArea to open the context menu.
    // Right-clicking without a selection should result in the Delete menu item being disabled.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    auto *contextMenu = textArea->findChild<QQuickContextMenu *>();
    QVERIFY(contextMenu);
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    auto *deleteMenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->menu()->itemAt(3));
    QCOMPARE(deleteMenuItem->text(), "Delete");
    QVERIFY(!deleteMenuItem->isEnabled());

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Right-clicking with a selection should result in the Delete menu item being enabled.
    textArea->select(mementoStr.length(), textArea->length());
    QCOMPARE(textArea->selectedText(), QLatin1Char(' ') + moriStr);
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    QVERIFY(deleteMenuItem->isEnabled());

    // Click on the Delete menu item and close the menu.
    auto *contextMenuPrivate = QQuickPopupPrivate::get(contextMenu->menu());
    QQuickWindow *contextMenuPopupWindow = &window;
    if (contextMenuPrivate->usePopupWindow())
        contextMenuPopupWindow = contextMenuPrivate->popupWindow;
    QTest::mouseClick(contextMenuPopupWindow, Qt::LeftButton, Qt::NoModifier, mapCenterToWindow(deleteMenuItem));
    QCOMPARE(textArea->text(), mementoStr);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Make the TextArea read-only. Delete should no longer be enabled.
    textArea->setReadOnly(true);
    textArea->selectAll();
    // Right click on the TextArea to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    QCOMPARE(deleteMenuItem->text(), "Delete");
    QVERIFY(!deleteMenuItem->isEnabled());

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());
}

void tst_QQuickTextArea::contextMenuSelectAll()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("contextMenu.qml")));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto *textArea = window.rootObject()->property("textArea").value<QQuickTextArea *>();
    QVERIFY(textArea);
    textArea->forceActiveFocus();

    // Right click on the TextArea to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    auto *contextMenu = textArea->findChild<QQuickContextMenu *>();
    QVERIFY(contextMenu);
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    auto *selectAllMenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->menu()->itemAt(5));
    QVERIFY(selectAllMenuItem);
    QCOMPARE(selectAllMenuItem->text(), "Select All");

    // Click on the Select All menu item and close the menu.
    auto *contextMenuPrivate = QQuickPopupPrivate::get(contextMenu->menu());
    QQuickWindow *contextMenuPopupWindow = &window;
    if (contextMenuPrivate->usePopupWindow())
        contextMenuPopupWindow = contextMenuPrivate->popupWindow;
    QTest::mouseClick(contextMenuPopupWindow, Qt::LeftButton, Qt::NoModifier, mapCenterToWindow(selectAllMenuItem));
    QCOMPARE(textArea->selectedText(), mementoMoriStr);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Make the TextArea read-only. Select All should still be enabled.
    textArea->setReadOnly(true);
    // Right click on the TextArea to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    QCOMPARE(selectAllMenuItem->text(), "Select All");
    QVERIFY(selectAllMenuItem->isEnabled());
    textArea->setReadOnly(false);

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());
}

QTEST_QUICKCONTROLS_MAIN(tst_QQuickTextArea)

#include "tst_qquicktextarea.moc"
