// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Exercises Drag.dragType: Drag.Automatic, i.e. the QDrag::exec() path. tst_qquickdrag
// deliberately covers only Drag.Internal and Drag.None, because exec() runs a nested
// event loop; this test drives that loop instead of avoiding it.

#include <QtCore/qpointer.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qtimer.h>
#include <QtGui/qcursor.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlexpression.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickdrag_p.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

#include <QtQuick/private/qquickitemview_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

#include <qpa/qplatformdrag.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qwindowsysteminterface.h>

#include <functional>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

// QDrag::exec() runs a nested event loop that, on most platforms, only a real user can
// terminate. The offscreen platform installs the in-process QSimpleDrag, which drives
// itself from ordinary mouse and key events, and implements QCursor::setPos(), which
// QSimpleDrag::startDrag() needs to resolve the source window. The CMakeLists.txt for
// this test selects it; "minimal" is not a substitute, as it has no QPlatformCursor.
static bool haveInProcessDrag()
{
    if (QGuiApplication::platformName() != "offscreen"_L1)
        return false;
    // QBasicDrag, which QSimpleDrag derives from, is a QObject: it installs itself as an
    // application event filter to track the mouse. A stub implementation that returns
    // Qt::IgnoreAction without running an event loop is not, so this also skips rather
    // than fails when built against a Qt version whose offscreen plugin still has one.
    return dynamic_cast<QObject *>(QGuiApplicationPrivate::platformIntegration()->drag());
}

#define SKIP_IF_NO_IN_PROCESS_DRAG() \
    do { \
        if (!haveInProcessDrag()) \
            QSKIP("Needs the in-process QSimpleDrag; run with -platform offscreen"); \
    } while (false)

// Input is posted rather than delivered with QTest::mouse*(), which is not usable from
// inside a drag: those go through qt_handleMouseEvent(), which delivers synchronously
// and then calls qApp->processEvents(). That nested processEvents() clears the event
// dispatcher's interrupt flag and drains its wakeup pipe, so if the event ended the
// drag, the QEventLoop::exit() it triggered is swallowed and the drag loop blocks until
// something else happens to wake it. Posting lets the drag's own loop drain the queue.
static void postMouseMove(QWindow *window, const QPoint &local)
{
    QWindowSystemInterface::handleMouseEvent(window, local, window->mapToGlobal(local),
                                            Qt::LeftButton, Qt::NoButton, QEvent::MouseMove);
}

static void postMouseRelease(QWindow *window, const QPoint &local)
{
    QWindowSystemInterface::handleMouseEvent(window, local, window->mapToGlobal(local),
                                            Qt::NoButton, Qt::LeftButton,
                                            QEvent::MouseButtonRelease);
}

class tst_QQuickNativeDrag : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickNativeDrag() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void init() override;

    void completes();
    void delegateDestroyedWhileDragging_data();
    void delegateDestroyedWhileDragging();

private:
    // Starts a native drag on item, invoking insideLoop from within QDrag::exec()'s
    // nested event loop.
    static void runDrag(QQuickView *window, QQuickItem *item, QQuickDragAttached *attached,
                        std::function<void()> insideLoop);
    static QQuickDragAttached *dragAttached(QQuickItem *item)
    {
        return qobject_cast<QQuickDragAttached *>(
                qmlAttachedPropertiesObject<QQuickDrag>(item, /*create*/ true));
    }
    static void evaluate(QObject *scope, const QString &expression)
    {
        QQmlExpression expr(qmlContext(scope), scope, expression);
        const QVariant result = expr.evaluate();
        Q_UNUSED(result);
        QVERIFY2(!expr.hasError(), qPrintable(expr.error().toString()));
    }
};

void tst_QQuickNativeDrag::init()
{
    QQmlDataTest::init();
    // runDrag() warns when its watchdog has to break into a stuck nested event loop.
    QTest::failOnWarning(QRegularExpression(u"^watchdog:"_s));
}

void tst_QQuickNativeDrag::runDrag(QQuickView *window, QQuickItem *item,
                                   QQuickDragAttached *attached,
                                   std::function<void()> insideLoop)
{
    const QPoint pressPos = item->mapToScene(QPointF(10, 10)).toPoint();

    // QSimpleDrag::startDrag() resolves the source window from QCursor::pos() rather
    // than from the last mouse event, so this is needed before the drag starts. On
    // offscreen it takes effect immediately, with no round trip to a compositor.
    QCursor::setPos(window->mapToGlobal(pressPos));
    QCoreApplication::processEvents();
    QCOMPARE(QCursor::pos(), window->mapToGlobal(pressPos));

    postMouseMove(window, pressPos);
    QWindowSystemInterface::handleMouseEvent(window, pressPos, window->mapToGlobal(pressPos),
                                            Qt::LeftButton, Qt::LeftButton,
                                            QEvent::MouseButtonPress);
    QCoreApplication::processEvents();

    // dragStarted() is emitted from QQuickDragAttachedPrivate::startDrag() just before
    // QDrag::exec(), so a queued call from it is delivered inside the nested loop --
    // by which time QBasicDrag has installed the event filter that makes posted input
    // drive the drag.
    if (insideLoop) {
        QObject::connect(attached, &QQuickDragAttached::dragStarted, attached,
                         std::move(insideLoop), Qt::QueuedConnection);
    }

    // Fail rather than hang if nothing terminates the loop.
    QTimer watchdog;
    watchdog.setSingleShot(true);
    watchdog.setInterval(5s);
    QObject::connect(&watchdog, &QTimer::timeout, &watchdog, [] {
        qWarning("watchdog: nested drag event loop did not terminate");
        if (QPlatformDrag *pd = QGuiApplicationPrivate::platformIntegration()->drag())
            pd->cancelDrag();
    });
    watchdog.start();

    QPointer<QQuickDragAttached> guard(attached);
    evaluate(item, u"Drag.active = true"_s);
    // setActive() defers startDrag() to the event loop, and exec() then blocks until the
    // input posted by insideLoop ends the drag -- so spinning here runs the whole thing.
    // The attached object may not survive it, hence the guard.
    QTRY_VERIFY(!guard || !guard->isActive());
}

// Baseline: a native drag from a delegate starts, is seen by the DropArea, and finishes.
void tst_QQuickNativeDrag::completes()
{
    SKIP_IF_NO_IN_PROCESS_DRAG();

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl(u"listViewDelegateDrag.qml"_s)));
    QQuickItemView *view = window.rootObject()->findChild<QQuickItemView *>();
    QVERIFY(view);
    QQuickItem *delegate = QQuickVisualTestUtils::findViewDelegateItem(view, 0);
    QVERIFY(delegate);
    QQuickDragAttached *attached = dragAttached(delegate);
    QVERIFY(attached);
    const QQuickItem *dropArea = window.rootObject()->findChild<QQuickItem *>(u"dropArea"_s);
    QVERIFY(dropArea);

    QSignalSpy startedSpy(attached, SIGNAL(dragStarted()));
    QSignalSpy finishedSpy(attached, SIGNAL(dragFinished(Qt::DropAction)));

    const QPoint dropPos = delegate->mapToScene(QPointF(10, 120)).toPoint();
    runDrag(&window, delegate, attached, [&] {
        postMouseMove(&window, dropPos);
        postMouseRelease(&window, dropPos);
    });

    QCOMPARE(startedSpy.size(), 1);
    QTRY_COMPARE(finishedSpy.size(), 1);
    QVERIFY(!attached->isActive());
    QVERIFY(!attached->target());
}

void tst_QQuickNativeDrag::delegateDestroyedWhileDragging_data()
{
    QTest::addColumn<QString>("destroyExpression");
    // The delegate scrolls out of the visible area, and with cacheBuffer 0 the view
    // releases it immediately. This is what QTBUG-124663 does by auto-scrolling.
    QTest::newRow("recycled") << u"list.contentY = 50 * 40"_s;
    // The model rows backing every delegate go away. Nothing item-view-specific is
    // needed to reproduce this -- any delegate model teardown will do.
    QTest::newRow("model reset") << u"list.model = 0"_s;
}

// QTBUG-124663: the delegate that owns the QQuickDragAttached -- and that the QDrag is
// parented to -- is destroyed inside QDrag::exec()'s nested event loop. Must not crash,
// must not leave the drag active, and must not leave the DropArea entered.
void tst_QQuickNativeDrag::delegateDestroyedWhileDragging()
{
    SKIP_IF_NO_IN_PROCESS_DRAG();
    QFETCH(const QString, destroyExpression);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl(u"listViewDelegateDrag.qml"_s)));
    QQuickItemView *view = window.rootObject()->findChild<QQuickItemView *>();
    QVERIFY(view);
    QQuickItem *delegate = QQuickVisualTestUtils::findViewDelegateItem(view, 0);
    QVERIFY(delegate);
    QPointer<QQuickItem> delegateGuard(delegate);
    QPointer<QQuickDragAttached> attached(dragAttached(delegate));
    QVERIFY(attached);
    const QQuickItem *dropArea = window.rootObject()->findChild<QQuickItem *>(u"dropArea"_s);
    QVERIFY(dropArea);

    QSignalSpy startedSpy(attached, SIGNAL(dragStarted()));
    QSignalSpy finishedSpy(attached, SIGNAL(dragFinished(Qt::DropAction)));
    QSignalSpy enteredSpy(dropArea, SIGNAL(entered(QQuickDragEvent*)));
    QSignalSpy exitedSpy(dropArea, SIGNAL(exited()));

    QObject *root = window.rootObject();
    runDrag(&window, delegate, attached, [&] {
        // We are inside QDrag::exec() here.
        evaluate(root, destroyExpression);
        // The view releases the delegate on its next polish, and the delegate model then
        // queues a deferred delete. Let both happen, still inside the nested loop, which
        // is what the stack trace in the bug report shows. Stop as soon as the delegate
        // is gone: processEvents() after the drag has ended would clear the event
        // dispatcher interrupt that unwinds exec().
        for (int i = 0; i < 10 && !delegateGuard.isNull(); ++i) {
            QCoreApplication::processEvents();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
    });

    QVERIFY(delegateGuard.isNull());
    QCOMPARE(startedSpy.size(), 1);

    // The attached object is a QObject child of the delegate, so it is gone too -- but
    // it must have reported the drag as finished on the way out, and the DropArea must
    // not be left thinking a drag is still in progress.
    QCOMPARE(finishedSpy.size(), 1);
    QTRY_COMPARE(exitedSpy.size(), enteredSpy.size());

    // The reported symptom is that the drag is "internally still active" afterwards.
    // Prove the state machine recovered by running a second, uneventful drag. Undo
    // whatever this data row did first, so that there is a delegate to drag again.
    evaluate(root, u"list.model = 100"_s);
    evaluate(root, u"list.contentY = 0"_s);
    QQuickItem *next = nullptr;
    QTRY_VERIFY((next = QQuickVisualTestUtils::findViewDelegateItem(view, 1)));
    QQuickDragAttached *nextAttached = dragAttached(next);
    QVERIFY(nextAttached);
    QSignalSpy secondFinished(nextAttached, SIGNAL(dragFinished(Qt::DropAction)));

    const QPoint dropPos = next->mapToScene(QPointF(10, 30)).toPoint();
    runDrag(&window, next, nextAttached, [&] {
        postMouseMove(&window, dropPos);
        postMouseRelease(&window, dropPos);
    });

    QTRY_COMPARE(secondFinished.size(), 1);
    QVERIFY(!nextAttached->isActive());
}

QTEST_MAIN(tst_QQuickNativeDrag)

#include "tst_qquicknativedrag.moc"
