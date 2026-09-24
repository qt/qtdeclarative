// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtQuickTest/QtQuickTest>

#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlfile.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlapplicationengine.h>

#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickwindowcontainer_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qscopeguard.h>

#define TEST_WINDOW_PARENT 0

class tst_QQuickWindowContainer : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickWindowContainer()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {
    }

private slots:
    void init() override;
    void cleanup();

    void basicFunctionality_data();
    void basicFunctionality();

    void windowDestroyed();
    void windowLifetimeFollowsContainer();

    void focusNavigation();

#if TEST_WINDOW_PARENT
    void deferredVisibilityWithoutWindow();
    void windowComponent();
#endif

    void invisibleChildWindowFocus();

    void updateStackingOrderPerformance();
    void skipStackingOrderUpdateWithoutChildWindows();

private:
    std::unique_ptr<QQmlApplicationEngine> m_engine;
};

void tst_QQuickWindowContainer::init()
{
    QQmlDataTest::init();

    QString testFile = QTest::currentTestFunction();
    if (auto *dataTag = QTest::currentDataTag())
        testFile += QChar('_') % dataTag;
    testFile += ".qml";

    const auto testUrl = testFileUrl(testFile);
    if (QFileInfo::exists(QQmlFile::urlToLocalFileOrQrc(testUrl))) {
        m_engine.reset(new QQmlApplicationEngine(testUrl));
        QVERIFY(m_engine->rootObjects().size() > 0);
    }
}

void tst_QQuickWindowContainer::cleanup()
{
    m_engine.reset(nullptr);
}

void tst_QQuickWindowContainer::basicFunctionality_data()
{
    QTest::addColumn<QPoint>("position");

#if TEST_WINDOW_PARENT
    QTest::newRow("window") << QPoint(100, 100);
    QTest::newRow("item") << QPoint(200, 200);
#endif
    QTest::newRow("container") << QPoint(100, 100);
}

void tst_QQuickWindowContainer::basicFunctionality()
{
    QFETCH(QPoint, position);

    auto *topLevelWindow = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    auto *childWindow = topLevelWindow->findChild<QWindow*>("childWindow");
    QVERIFY(childWindow);

    // The top level isn't visible yet, so there hasn't been any
    // polish, which we rely on for the actual reparenting.
    QCOMPARE(childWindow->parent(), nullptr);

    topLevelWindow->setVisible(true);
    QVERIFY(QQuickTest::qWaitForPolish(topLevelWindow));
    QCOMPARE(childWindow->parent(), topLevelWindow);

    QCOMPARE(childWindow->position(), position);
}

void tst_QQuickWindowContainer::windowDestroyed()
{
    std::unique_ptr<QWindow> window(new QWindow);

    QQuickWindowContainer container;
    QSignalSpy spy(&container, &QQuickWindowContainer::containedWindowChanged);

    container.setContainedWindow(window.get());
    QCOMPARE(container.containedWindow(), window.get());
    QCOMPARE(spy.size(), 1);

    window.reset(nullptr);

    QVERIFY(!container.containedWindow());
    QCOMPARE(spy.size(), 2);
}

void tst_QQuickWindowContainer::windowLifetimeFollowsContainer()
{
    QWindow window;
    QPointer<QWindow> windowGuard = &window;

    QQuickWindowContainer container;
    container.setContainedWindow(&window);

    {
        QQuickWindow quickWindow;
        container.setParentItem(quickWindow.contentItem());
        quickWindow.show();
        QVERIFY(QQuickTest::qWaitForPolish(&quickWindow));
        QCOMPARE(window.parent(), &quickWindow);

        // Decouple container from Quick window
        container.setParentItem(nullptr);
    }

    QVERIFY(windowGuard);
}

void tst_QQuickWindowContainer::focusNavigation()
{
    if (QGuiApplication::styleHints()->tabFocusBehavior() != Qt::TabFocusAllControls)
        QSKIP("Test requires Qt::TabFocusAllControls tab focus behavior");

    auto *topLevelWindow = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    auto *childWindow = topLevelWindow->findChild<QWindow *>("childWindow");
    QVERIFY(childWindow);

    QQuickItem *rect1 = topLevelWindow->findChild<QQuickItem *>("rect1");
    QQuickItem *rect2 = topLevelWindow->findChild<QQuickItem *>("rect2");
    QVERIFY(rect1);
    QVERIFY(rect2);

    QQuickItem *rect1Embedded = childWindow->findChild<QQuickItem *>("rect1embedded");
    QQuickItem *rect2Embedded = childWindow->findChild<QQuickItem *>("rect2embedded");
    QVERIFY(rect1Embedded);
    QVERIFY(rect2Embedded);

    topLevelWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(topLevelWindow));
    if (!QTest::qWaitForWindowFocused(topLevelWindow))
        QSKIP("Window failed to activate and be focused, skipping test");
    rect1->forceActiveFocus();

    auto verifyFocusChain = [](const QList<QQuickItem *> &focusChain, Qt::Key key) {
        for (QQuickItem *expectedFocusTarget : focusChain) {
            const QByteArray objectName = expectedFocusTarget->objectName().toLatin1();
            QTRY_VERIFY2(expectedFocusTarget->hasActiveFocus(), "expectedFocusTarget: " + objectName);
            QTest::keyClick(QGuiApplication::focusWindow(), key);
        }
    };

    verifyFocusChain({rect1, rect2, rect1Embedded, rect2Embedded}, Qt::Key_Tab);
    verifyFocusChain({rect1, rect2Embedded, rect1Embedded, rect2, rect1}, Qt::Key_Backtab);
}

#if TEST_WINDOW_PARENT
void tst_QQuickWindowContainer::deferredVisibilityWithoutWindow()
{
    auto *topLevelWindow = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    auto *childWindow = topLevelWindow->findChild<QQuickWindow*>("childWindow");
    QVERIFY(childWindow);
    QVERIFY(QQuickTest::qWaitForPolish(topLevelWindow));

    QSignalSpy spy(childWindow, &QWindow::visibleChanged);
    m_engine.reset(nullptr);

    // Deleting the engine should only hide the window once,
    // not hide, show, and then hide again, which would be
    // the result of applying visibility without a window.
    QCOMPARE(spy.count(), 1);
}

void tst_QQuickWindowContainer::windowComponent()
{
    auto *root = qobject_cast<QQuickWindow *>(m_engine->rootObjects().first());
    QVERIFY(root);
    auto *itemParent = root->findChild<QQuickItem*>("itemParent");
    auto *windowParent = root->findChild<QQuickWindow*>("windowParent");
    QVERIFY(itemParent);
    QVERIFY(windowParent);

    root->setVisible(true);
    QVERIFY(QQuickTest::qWaitForPolish(root));
    windowParent->setVisible(true);
    QVERIFY(QQuickTest::qWaitForPolish(windowParent));

    QObject *window_item = root->property("window_item").value<QObject*>();
    QObject *window_window = root->property("window_window").value<QObject*>();
    QObject *window_item_parent = root->property("window_item_parent").value<QObject*>();
    QObject *window_window_parent = root->property("window_window_parent").value<QObject*>();

    QVERIFY(window_item);
    QVERIFY(window_window);
    QVERIFY(window_item_parent);
    QVERIFY(window_window_parent);

    QCOMPARE(window_item->parent(), itemParent);
    QCOMPARE(window_window->parent(), windowParent);
    QCOMPARE(window_item_parent->parent(), root);
    QCOMPARE(window_window_parent->parent(), windowParent);

    QCOMPARE(qobject_cast<QQuickWindow *>(window_item)->transientParent(), root);
    QCOMPARE(qobject_cast<QQuickWindow *>(window_window)->transientParent(), windowParent);
    QCOMPARE(qobject_cast<QQuickWindow *>(window_item)->parent(), nullptr);
    QCOMPARE(qobject_cast<QQuickWindow *>(window_window)->parent(), nullptr);

    QEXPECT_FAIL("", "The automatic transient parent logic doesn't account for visual parent", Continue);
    QCOMPARE(qobject_cast<QQuickWindow *>(window_item_parent)->transientParent(), nullptr);
    QEXPECT_FAIL("", "The automatic transient parent logic doesn't account for visual parent", Continue);
    QCOMPARE(qobject_cast<QQuickWindow *>(window_window_parent)->transientParent(), nullptr);
    QCOMPARE(qobject_cast<QQuickWindow *>(window_item_parent)->parent(), root);
    QCOMPARE(qobject_cast<QQuickWindow *>(window_window_parent)->parent(), windowParent);
}
#endif // TEST_WINDOW_PARENT

void tst_QQuickWindowContainer::invisibleChildWindowFocus()
{
    // QTBUG-141450: Test that an invisible child window doesn't steal focus when
    // the stacking order is updated.
    QQuickWindow quickWindow;
    QWindow childWindow;
    QQuickWindowContainer container;

    // Start invisible so childWindow is never shown and never grabs focus.
    container.setVisible(false);
    container.setParentItem(quickWindow.contentItem());
    container.setContainedWindow(&childWindow);

    quickWindow.show();
    QVERIFY(QQuickTest::qWaitForPolish(&quickWindow));
    // Child is parented but not visible
    QCOMPARE(childWindow.parent(), &quickWindow);
    QVERIFY(!childWindow.isVisible());

    if (!QTest::qWaitForWindowActive(&quickWindow))
        QSKIP("Window failed to activate, skipping test");

    // With childWindow invisible, quickWindow must be the focus window
    QTRY_COMPARE(QGuiApplication::focusWindow(), &quickWindow);

    // Trigger a stacking order update. This should not affect which window
    // has focus.
    auto *windowPrivate = QQuickWindowPrivate::get(&quickWindow);
    windowPrivate->updateChildWindowStackingOrder();
    QCoreApplication::processEvents();
    QCOMPARE(QGuiApplication::focusWindow(), &quickWindow);
}

void tst_QQuickWindowContainer::updateStackingOrderPerformance()
{
    QQuickWindow quickWindow;
    auto *contentItem = quickWindow.contentItem();
    for (int i = 0; i < 100; ++i) {
        QQuickItem *item = new QQuickItem(contentItem);
        for (int j = 0; j < 100; ++j)
            item = new QQuickItem(item);
    }

    QVERIFY(QQuickTest::qWaitForPolish(&quickWindow));

    auto *windowPrivate = QQuickWindowPrivate::get(&quickWindow);

    QBENCHMARK {
        contentItem->polish();
        windowPrivate->polishItems();
    }
}

static int stackingOrderUpdateCount = 0;
static QtMessageHandler previousMessageHandler = nullptr;

static void countStackingOrderUpdates(QtMsgType type, const QMessageLogContext &context,
                                      const QString &message)
{
    if (qstrcmp(context.category, "qt.quick.window") == 0) {
        if (message.startsWith(QLatin1String("Updating child window stacking order")))
            ++stackingOrderUpdateCount;
        return;
    }
    previousMessageHandler(type, context, message);
}

void tst_QQuickWindowContainer::skipStackingOrderUpdateWithoutChildWindows()
{
    QLoggingCategory::setFilterRules(QStringLiteral("qt.quick.window.debug=true"));
    previousMessageHandler = qInstallMessageHandler(countStackingOrderUpdates);
    auto restore = qScopeGuard([]{
        qInstallMessageHandler(previousMessageHandler);
        QLoggingCategory::setFilterRules(QString());
    });
    if (!QLoggingCategory("qt.quick.window").isDebugEnabled())
        QSKIP("Debug output for qt.quick.window can not be enabled");

    QQuickWindow quickWindow;
    auto *windowPrivate = QQuickWindowPrivate::get(&quickWindow);

    // Changes the z of a new item's child, which requests a (deferred)
    // stacking order update, and runs the polish pass that handles it.
    auto changeStackingAndPolish = [&] {
        auto *child = new QQuickItem(new QQuickItem(quickWindow.contentItem()));
        child->setZ(1);
        QVERIFY(windowPrivate->needsChildWindowStackingOrderUpdate);
        stackingOrderUpdateCount = 0;
        windowPrivate->polishItems();
        QVERIFY(!windowPrivate->needsChildWindowStackingOrderUpdate);
    };

    // No child windows, nothing to restack
    changeStackingAndPolish();
    if (QTest::currentTestFailed())
        return;
    QCOMPARE(stackingOrderUpdateCount, 0);

    // Child window added by the window container during the same polish pass
    QWindow containedWindow;
    auto container = std::make_unique<QQuickWindowContainer>();
    container->setContainedWindow(&containedWindow);
    container->setParentItem(quickWindow.contentItem());
    changeStackingAndPolish();
    if (QTest::currentTestFailed())
        return;
    QCOMPARE(containedWindow.parent(), &quickWindow);
    QCOMPARE(stackingOrderUpdateCount, 1);

    // The child window is removed along with its container
    container.reset();
    QCOMPARE(containedWindow.parent(), nullptr);
    changeStackingAndPolish();
    if (QTest::currentTestFailed())
        return;
    QCOMPARE(stackingOrderUpdateCount, 0);
}

QTEST_MAIN(tst_QQuickWindowContainer)

#include "tst_qquickwindowcontainer.moc"
