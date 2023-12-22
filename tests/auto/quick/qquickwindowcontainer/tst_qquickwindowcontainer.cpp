// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtQuickTest/QtQuickTest>

#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlfile.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlapplicationengine.h>

#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qquickwindowcontainer_p.h>

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
    void deferredVisibilityWithoutWindow();

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

    QTest::newRow("window") << QPoint(100, 100);
    QTest::newRow("item") << QPoint(200, 200);
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

QTEST_MAIN(tst_QQuickWindowContainer)

#include "tst_qquickwindowcontainer.moc"
