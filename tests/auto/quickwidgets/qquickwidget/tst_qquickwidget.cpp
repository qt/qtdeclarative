// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <qtesttouch.h>
#include <QtTest/QSignalSpy>
#include <QtTest/private/qtesthelpers_p.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtGui/QWindow>
#include <QtGui/QScreen>
#include <QtGui/QImage>
#include <QtCore/QDebug>
#include <QtQml/qqmlengine.h>

#include <QtCore/QLoggingCategory>
#include <QtGui/qstylehints.h>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/private/qapplication_p.h>

#include <QtQuickWidgets/QQuickWidget>

#if QT_CONFIG(graphicsview)
# include <QtWidgets/QGraphicsView>
# include <QtWidgets/QGraphicsProxyWidget>
#endif
Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

class MouseRecordingQQWidget : public QQuickWidget
{
public:
    explicit MouseRecordingQQWidget(QWidget *parent = nullptr) : QQuickWidget(parent) {
        setAttribute(Qt::WA_AcceptTouchEvents);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << event;
        m_mouseEvents << event->source();
        QQuickWidget::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << event;
        m_mouseEvents << event->source();
        QQuickWidget::mouseMoveEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << event;
        m_mouseEvents << event->source();
        QQuickWidget::mouseReleaseEvent(event);
    }

public:
    QList<Qt::MouseEventSource> m_mouseEvents;
};

class MouseRecordingItem : public QQuickItem
{
public:
    MouseRecordingItem(bool acceptTouch, bool acceptTouchPress, QQuickItem *parent = nullptr)
        : QQuickItem(parent)
        , m_acceptTouchPress(acceptTouchPress)
    {
        setSize(QSizeF(300, 300));
        setAcceptedMouseButtons(Qt::LeftButton);
        setAcceptTouchEvents(acceptTouch);
    }

protected:
    void touchEvent(QTouchEvent* event) override {
        event->setAccepted(m_acceptTouchPress);
        m_touchEvents << event->type();
        qCDebug(lcTests) << "accepted?" << event->isAccepted() << event;
    }
    void mousePressEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << event;
        m_mouseEvents << event->source();
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << event;
        m_mouseEvents << event->source();
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << event;
        m_mouseEvents << event->source();
    }

public:
    QList<Qt::MouseEventSource> m_mouseEvents;
    QList<QEvent::Type> m_touchEvents;

private:
    bool m_acceptTouchPress;
};

class tst_qquickwidget : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickwidget();

private slots:
    void showHide();
    void reparentAfterShow();
    void changeGeometry();
    void resizemodeitem();
    void layoutSizeChange();
    void errors();
    void engine();
    void readback();
    void renderingSignals();
    void grab();
    void grabBeforeShow();
    void reparentToNewWindow();
    void nullEngine();
    void keyEvents();
    void shortcuts();
    void enterLeave();
    void mouseEventWindowPos();
    void synthMouseFromTouch_data();
    void synthMouseFromTouch();
    void touchTapMouseArea();
    void touchTapButton();
    void touchMultipleWidgets();
    void tabKey();
    void resizeOverlay();
    void controls();
    void focusOnClick();
#if QT_CONFIG(graphicsview)
    void focusOnClickInProxyWidget();
#endif
    void focusPreserved();
    void accessibilityHandlesViewChange();

private:
    QPointingDevice *device = QTest::createTouchDevice();
    const QRect m_availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
};

tst_qquickwidget::tst_qquickwidget()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquickwidget::showHide()
{
    QWidget window;

    QQuickWidget *childView = new QQuickWidget(&window);
    childView->setSource(testFileUrl("rectangle.qml"));

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(!childView->quickWindow()->isVisible()); // this window is always not visible see QTBUG-65761
    QVERIFY(childView->quickWindow()->visibility() != QWindow::Hidden);

    window.hide();
    QVERIFY(!childView->quickWindow()->isVisible());
    QCOMPARE(childView->quickWindow()->visibility(), QWindow::Hidden);
}

void tst_qquickwidget::reparentAfterShow()
{
    QWidget window;

    QQuickWidget *childView = new QQuickWidget(&window);
    childView->setSource(testFileUrl("rectangle.qml"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QScopedPointer<QQuickWidget> toplevelView(new QQuickWidget);
    toplevelView->setParent(&window);
    toplevelView->setSource(testFileUrl("rectangle.qml"));
    toplevelView->show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
}

void tst_qquickwidget::changeGeometry()
{
    QWidget window;

    QQuickWidget *childView = new QQuickWidget(&window);
    childView->setSource(testFileUrl("rectangle.qml"));

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    childView->setGeometry(100,100,100,100);
}

void tst_qquickwidget::resizemodeitem()
{
    QWidget window;
    window.setGeometry(m_availableGeometry.left(), m_availableGeometry.top(), 400, 400);

    QScopedPointer<QQuickWidget> view(new QQuickWidget);
    view->setParent(&window);
    view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    QCOMPARE(QSize(0,0), view->initialSize());
    view->setSource(testFileUrl("resizemodeitem.qml"));
    QQuickItem* item = qobject_cast<QQuickItem*>(view->rootObject());
    QVERIFY(item);
    window.show();

    view->showNormal();
    // initial size from root object
    QCOMPARE(item->width(), 200.0);
    QCOMPARE(item->height(), 200.0);
    QCOMPARE(view->size(), QSize(200, 200));
    QCOMPARE(view->size(), view->sizeHint());
    QCOMPARE(view->size(), view->initialSize());

    // size update from view
    view->resize(QSize(80,100));

    QTRY_COMPARE(item->width(), 80.0);
    QCOMPARE(item->height(), 100.0);
    QCOMPARE(view->size(), QSize(80, 100));
    QCOMPARE(view->size(), view->sizeHint());

    view->setResizeMode(QQuickWidget::SizeViewToRootObject);

    // size update from view disabled
    view->resize(QSize(60,80));
    QCOMPARE(item->width(), 80.0);
    QCOMPARE(item->height(), 100.0);
    QTRY_COMPARE(view->size(), QSize(60, 80));

    // size update from root object
    item->setWidth(250);
    item->setHeight(350);
    QCOMPARE(item->width(), 250.0);
    QCOMPARE(item->height(), 350.0);
    QTRY_COMPARE(view->size(), QSize(250, 350));
    QCOMPARE(view->size(), QSize(250, 350));
    QCOMPARE(view->size(), view->sizeHint());

    // reset window
    window.hide();
    view.reset(new QQuickWidget(&window));
    view->setResizeMode(QQuickWidget::SizeViewToRootObject);
    view->setSource(testFileUrl("resizemodeitem.qml"));
    item = qobject_cast<QQuickItem*>(view->rootObject());
    QVERIFY(item);
    window.show();

    view->showNormal();

    // initial size for root object
    QCOMPARE(item->width(), 200.0);
    QCOMPARE(item->height(), 200.0);
    QCOMPARE(view->size(), view->sizeHint());
    QCOMPARE(view->size(), view->initialSize());

    // size update from root object
    item->setWidth(80);
    item->setHeight(100);
    QCOMPARE(item->width(), 80.0);
    QCOMPARE(item->height(), 100.0);
    QTRY_COMPARE(view->size(), QSize(80, 100));
    QCOMPARE(view->size(), view->sizeHint());

    // size update from root object disabled
    view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    item->setWidth(60);
    item->setHeight(80);
    QCOMPARE(view->width(), 80);
    QCOMPARE(view->height(), 100);
    QCOMPARE(QSize(item->width(), item->height()), view->sizeHint());

    // size update from view
    view->resize(QSize(200,300));
    QTRY_COMPARE(item->width(), 200.0);
    QCOMPARE(item->height(), 300.0);
    QCOMPARE(view->size(), QSize(200, 300));
    QCOMPARE(view->size(), view->sizeHint());

    window.hide();

    // if we set a specific size for the view then it should keep that size
    // for SizeRootObjectToView mode.
    view.reset(new QQuickWidget(&window));
    view->resize(300, 300);
    view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    QCOMPARE(QSize(0,0), view->initialSize());
    view->setSource(testFileUrl("resizemodeitem.qml"));
    view->resize(300, 300);
    item = qobject_cast<QQuickItem*>(view->rootObject());
    QVERIFY(item);
    window.show();

    view->showNormal();

    // initial size from root object
    QCOMPARE(item->width(), 300.0);
    QCOMPARE(item->height(), 300.0);
    QTRY_COMPARE(view->size(), QSize(300, 300));
    QCOMPARE(view->size(), view->sizeHint());
    QCOMPARE(view->initialSize(), QSize(200, 200)); // initial object size
}

void tst_qquickwidget::layoutSizeChange()
{
    QWidget window;
    window.resize(400, 400);

    QVBoxLayout *layout = new QVBoxLayout(&window);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    QScopedPointer<QQuickWidget> view(new QQuickWidget);
    layout->addWidget(view.data());
    QLabel *label = new QLabel("Label");
    layout->addWidget(label);
    layout->addStretch(1);


    view->resize(300,300);
    view->setResizeMode(QQuickWidget::SizeViewToRootObject);
    QCOMPARE(QSize(0,0), view->initialSize());
    view->setSource(testFileUrl("rectangle.qml"));
    QQuickItem* item = qobject_cast<QQuickItem*>(view->rootObject());
    QVERIFY(item);
    QCOMPARE(item->height(), 200.0);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));
    QTRY_COMPARE(view->height(), 200);
    QTRY_COMPARE(label->y(), 200);

    item->setSize(QSizeF(100,100));
    QCOMPARE(item->height(), 100.0);
    QTRY_COMPARE(view->height(), 100);
    QTRY_COMPARE(label->y(), 100);
}

void tst_qquickwidget::errors()
{
    QQuickWidget *view = new QQuickWidget;
    QScopedPointer<QQuickWidget> cleanupView(view);
    QVERIFY(view->errors().isEmpty()); // don't crash

    QQmlTestMessageHandler messageHandler;
    view->setSource(testFileUrl("error1.qml"));
    QCOMPARE(view->status(), QQuickWidget::Error);
    QCOMPARE(view->errors().size(), 1);
}

void tst_qquickwidget::engine()
{
    QScopedPointer<QQmlEngine> engine(new QQmlEngine);
    QScopedPointer<QQuickWidget> view(new QQuickWidget(engine.data(), nullptr));
    QScopedPointer<QQuickWidget> view2(new QQuickWidget(view->engine(), nullptr));

    QVERIFY(view->engine());
    QVERIFY(view2->engine());
    QCOMPARE(view->engine(), view2->engine());
}

void tst_qquickwidget::readback()
{
    QScopedPointer<QQuickWidget> view(new QQuickWidget);
    view->setSource(testFileUrl("rectangle.qml"));

    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QImage img = view->grabFramebuffer();
    QVERIFY(!img.isNull());
    QCOMPARE(img.width(), qCeil(view->width() * view->devicePixelRatio()));
    QCOMPARE(img.height(), qCeil(view->height() * view->devicePixelRatio()));

    QRgb pix = img.pixel(5, 5);
    QCOMPARE(pix, qRgb(255, 0, 0));
}

void tst_qquickwidget::renderingSignals()
{
    QQuickWidget widget;
    QQuickWindow *window = widget.quickWindow();
    QVERIFY(window);

    QSignalSpy beforeRenderingSpy(window, &QQuickWindow::beforeRendering);
    QSignalSpy beforeSyncSpy(window, &QQuickWindow::beforeSynchronizing);
    QSignalSpy afterRenderingSpy(window, &QQuickWindow::afterRendering);

    QVERIFY(beforeRenderingSpy.isValid());
    QVERIFY(beforeSyncSpy.isValid());
    QVERIFY(afterRenderingSpy.isValid());

    QCOMPARE(beforeRenderingSpy.size(), 0);
    QCOMPARE(beforeSyncSpy.size(), 0);
    QCOMPARE(afterRenderingSpy.size(), 0);

    widget.setSource(testFileUrl("rectangle.qml"));

    QCOMPARE(beforeRenderingSpy.size(), 0);
    QCOMPARE(beforeSyncSpy.size(), 0);
    QCOMPARE(afterRenderingSpy.size(), 0);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QTRY_VERIFY(beforeRenderingSpy.size() > 0);
    QTRY_VERIFY(beforeSyncSpy.size() > 0);
    QTRY_VERIFY(afterRenderingSpy.size() > 0);
}

void tst_qquickwidget::grab()
{
    QQuickWidget view;
    view.setSource(testFileUrl("rectangle.qml"));
    QPixmap pixmap = view.grab();
    QRgb pixel = pixmap.toImage().pixel(5, 5);
    QCOMPARE(pixel, qRgb(255, 0, 0));
}

// QTBUG-49929, verify that Qt Designer grabbing the contents before drag
// does not crash due to missing GL contexts or similar.
void tst_qquickwidget::grabBeforeShow()
{
    QQuickWidget widget;
    QVERIFY(!widget.grab().isNull());
}

void tst_qquickwidget::reparentToNewWindow()
{
#ifdef Q_OS_ANDROID
    QSKIP("This test crashes on Android (see QTBUG-100173)");
#endif
    QWidget window1;
    QWidget window2;

    QQuickWidget *qqw = new QQuickWidget(&window1);
    qqw->setSource(testFileUrl("rectangle.qml"));
    window1.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window1));
    window2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window2));

    qqw->setParent(&window2);

    QSignalSpy afterRenderingSpy(qqw->quickWindow(), &QQuickWindow::afterRendering);
    qqw->show();

    QTRY_VERIFY(afterRenderingSpy.size() > 0);

    QImage img = qqw->grabFramebuffer();

    QCOMPARE(img.pixel(5, 5), qRgb(255, 0, 0));
}

void tst_qquickwidget::nullEngine()
{
    QQuickWidget widget;
    // Default should have no errors, even with a null qml engine
    QVERIFY(widget.errors().isEmpty());
    QCOMPARE(widget.status(), QQuickWidget::Null);

    // A QML engine should be created lazily.
    QVERIFY(widget.rootContext());
    QVERIFY(widget.engine());
}

class KeyHandlingWidget : public QQuickWidget
{
public:
    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_A)
            ok = true;
    }

    bool ok = false;
};

void tst_qquickwidget::keyEvents()
{
    // A QQuickWidget should behave like a normal widget when it comes to event handling.
    // Verify that key events actually reach the widget. (QTBUG-45757)
    KeyHandlingWidget widget;
    widget.setSource(testFileUrl("rectangle.qml"));
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(widget.window()));

    // Note: send the event to the QWindow, not the QWidget, in order
    // to simulate the full event processing chain.
    QTest::keyClick(widget.window()->windowHandle(), Qt::Key_A);

    QTRY_VERIFY(widget.ok);
}

class ShortcutEventFilter : public QObject
{
public:
    bool eventFilter(QObject *obj, QEvent *e) override {
        if (e->type() == QEvent::ShortcutOverride)
            shortcutOk = true;

        return QObject::eventFilter(obj, e);
    }

    bool shortcutOk = false;
};

void tst_qquickwidget::shortcuts()
{
    // Verify that ShortcutOverride events do not get lost. (QTBUG-60988)
    KeyHandlingWidget widget;
    widget.setSource(testFileUrl("rectangle.qml"));
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(widget.window()));

    // Send to the widget, verify that the QQuickWindow sees it.

    ShortcutEventFilter filter;
    widget.quickWindow()->installEventFilter(&filter);

    QKeyEvent e(QEvent::ShortcutOverride, Qt::Key_A, Qt::ControlModifier);
    QCoreApplication::sendEvent(&widget, &e);

    QTRY_VERIFY(filter.shortcutOk);
}

void tst_qquickwidget::enterLeave()
{
#ifdef Q_OS_ANDROID
    QSKIP("Android has no cursor");
#endif
    QQuickWidget view;
    view.setSource(testFileUrl("enterleave.qml"));

    // Ensure the cursor is away from the window first
    const auto outside = m_availableGeometry.topLeft() + QPoint(50, 50);
    QCursor::setPos(outside);
    QTRY_VERIFY(QCursor::pos() == outside);

    view.move(m_availableGeometry.topLeft() + QPoint(100, 100));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);
    const QPoint frameOffset = view.geometry().topLeft() - view.frameGeometry().topLeft();

    QTRY_VERIFY(!rootItem->property("hasMouse").toBool());
    // Check the enter
    QCursor::setPos(view.pos() + QPoint(50, 50) + frameOffset);
    QTRY_VERIFY(rootItem->property("hasMouse").toBool());
    // Now check the leave
    QCursor::setPos(outside);
    QTRY_VERIFY(!rootItem->property("hasMouse").toBool());
}

void tst_qquickwidget::mouseEventWindowPos()
{
    QWidget widget;
    widget.resize(100, 100);
    QQuickWidget *quick = new QQuickWidget(&widget);
    quick->setSource(testFileUrl("mouse.qml"));
    quick->move(50, 50);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QQuickItem *rootItem = quick->rootObject();
    QVERIFY(rootItem);

    QVERIFY(!rootItem->property("wasClicked").toBool());
    QVERIFY(!rootItem->property("wasDoubleClicked").toBool());
    // Moving an item under the mouse cursor will trigger a mouse move event.
    // The above quick->move() will trigger a mouse move event on macOS.
    // Discard that in order to get a clean slate for the actual tests.
    rootItem->setProperty("wasMoved", QVariant(false));

    QWindow *window = widget.windowHandle();
    QVERIFY(window);

    QTest::mouseMove(window, QPoint(60, 60));
    QTest::mouseClick(window, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(60, 60));
    QTRY_VERIFY(rootItem->property("wasClicked").toBool());
    QTest::mouseDClick(window, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(60, 60));
    QTRY_VERIFY(rootItem->property("wasDoubleClicked").toBool());
    QTest::mouseMove(window, QPoint(70, 70));
    QTRY_VERIFY(rootItem->property("wasMoved").toBool());
}

void tst_qquickwidget::synthMouseFromTouch_data()
{
    QTest::addColumn<bool>("synthMouse"); // AA_SynthesizeMouseForUnhandledTouchEvents
    QTest::addColumn<bool>("acceptTouch"); // QQuickItem::touchEvent: setAccepted()

    QTest::newRow("no synth, accept") << false << true; // suitable for touch-capable UIs
    QTest::newRow("no synth, don't accept") << false << false;
    QTest::newRow("synth and accept") << true << true;
    QTest::newRow("synth, don't accept") << true << false; // the default
}

void tst_qquickwidget::synthMouseFromTouch()
{
    QFETCH(bool, synthMouse);
    QFETCH(bool, acceptTouch);

    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, synthMouse);
    QWidget window;
    window.setAttribute(Qt::WA_AcceptTouchEvents);
    QScopedPointer<MouseRecordingQQWidget> childView(new MouseRecordingQQWidget(&window));
    MouseRecordingItem *item = new MouseRecordingItem(!synthMouse, acceptTouch, nullptr);
    childView->setContent(QUrl(), nullptr, item);
    window.resize(300, 300);
    childView->resize(300, 300);
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QVERIFY(!childView->quickWindow()->isVisible()); // this window is always not visible see QTBUG-65761
    QVERIFY(item->isVisible());

    QPoint p1 = QPoint(20, 20);
    QPoint p2 = QPoint(30, 30);
    QTest::touchEvent(&window, device).press(0, p1, &window);
    QTest::touchEvent(&window, device).move(0, p2, &window);
    QTest::touchEvent(&window, device).release(0, p2, &window);

    qCDebug(lcTests) << item->m_touchEvents << item->m_mouseEvents;
    QCOMPARE(item->m_touchEvents.size(), synthMouse ? 0 : (acceptTouch ? 3 : 1));
    QCOMPARE(item->m_mouseEvents.size(), synthMouse ? 3 : 0);
    QCOMPARE(childView->m_mouseEvents.size(), 0);
    for (const auto &ev : item->m_mouseEvents)
        QCOMPARE(ev, Qt::MouseEventSynthesizedByQt);
}

void tst_qquickwidget::touchTapMouseArea()
{
    QWidget window;
    window.resize(100, 100);
    window.setObjectName("window widget");
    window.setAttribute(Qt::WA_AcceptTouchEvents);
    QVERIFY(QCoreApplication::testAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents));
    QQuickWidget *quick = new QQuickWidget(&window);
    quick->setSource(testFileUrl("mouse.qml"));
    quick->move(50, 50);
    quick->setObjectName("quick widget");
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QQuickItem *rootItem = quick->rootObject();
    QVERIFY(rootItem);
    QQuickMouseArea *ma = rootItem->findChild<QQuickMouseArea *>();
    QVERIFY(ma);

    QPoint p1 = QPoint(70, 70);
    QTest::touchEvent(&window, device).press(0, p1, &window);
    QTRY_COMPARE(ma->isPressed(), true);
    QTest::touchEvent(&window, device).move(0, p1, &window);
    QTest::touchEvent(&window, device).release(0, p1, &window);
    QTRY_COMPARE(ma->isPressed(), false);
    QVERIFY(rootItem->property("wasClicked").toBool());
}

void tst_qquickwidget::touchTapButton()
{
    QWidget window;
    QQuickWidget *quick = new QQuickWidget;
    quick->setSource(testFileUrl("button.qml"));

    QHBoxLayout hbox;
    hbox.addWidget(quick);
    window.setLayout(&hbox);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQuickItem *rootItem = quick->rootObject();
    QVERIFY(rootItem);
    QQuickButton *button = rootItem->findChild<QQuickButton *>("button");
    QVERIFY(button);

    const QPoint point = quick->mapTo(&window, button->mapToScene(button->boundingRect().center()).toPoint());
    QTest::touchEvent(&window, device).press(0, point, &window).commit();
    QTRY_VERIFY(rootItem->property("wasPressed").toBool());
    QTest::touchEvent(&window, device).release(0, point, &window).commit();
    QTRY_VERIFY(rootItem->property("wasReleased").toBool());
    QTRY_VERIFY(rootItem->property("wasClicked").toBool());
}

void tst_qquickwidget::touchMultipleWidgets()
{
    QWidget window;
    QQuickWidget *leftQuick = new QQuickWidget;
    leftQuick->setSource(testFileUrl("button.qml"));
    QQuickWidget *rightQuick = new QQuickWidget;
    rightQuick->setSource(testFileUrl("button.qml"));

    QHBoxLayout hbox;
    hbox.addWidget(leftQuick);
    hbox.addWidget(rightQuick);
    window.setLayout(&hbox);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQuickItem *leftRootItem = leftQuick->rootObject();
    QQuickItem *rightRootItem = rightQuick->rootObject();
    QVERIFY(leftRootItem);
    QVERIFY(rightRootItem);
    QQuickButton *leftButton = leftRootItem->findChild<QQuickButton *>("button");
    QQuickButton *rightButton = rightRootItem->findChild<QQuickButton *>("button");
    QVERIFY(leftButton);
    QVERIFY(rightButton);

    const QPoint leftPoint = leftQuick->mapTo(&window, leftButton->mapToScene(
                                              leftButton->boundingRect().center()).toPoint());
    const QPoint rightPoint = rightQuick->mapTo(&window, rightButton->mapToScene(
                                                rightButton->boundingRect().center()).toPoint());
    QTest::touchEvent(&window, device).press(0, leftPoint, &window).commit();
    QTRY_VERIFY(leftRootItem->property("wasPressed").toBool());
    QTest::touchEvent(&window, device).press(1, rightPoint, &window).commit();
    QTRY_VERIFY(rightRootItem->property("wasPressed").toBool());
    QTest::touchEvent(&window, device).release(1, rightPoint, &window).commit();
    QTRY_VERIFY(rightRootItem->property("wasReleased").toBool());
    QVERIFY(rightRootItem->property("wasClicked").toBool());
    QTest::touchEvent(&window, device).release(0, leftPoint, &window).commit();
    QTRY_VERIFY(leftRootItem->property("wasReleased").toBool());
    QVERIFY(leftRootItem->property("wasClicked").toBool());
}

void tst_qquickwidget::tabKey()
{
    if (QGuiApplication::styleHints()->tabFocusBehavior() != Qt::TabFocusAllControls)
        QSKIP("This function doesn't support NOT iterating all.");
    QWidget window1;
    QQuickWidget *qqw = new QQuickWidget(&window1);
    qqw->setSource(testFileUrl("activeFocusOnTab.qml"));
    QQuickWidget *qqw2 = new QQuickWidget(&window1);
    qqw2->setSource(testFileUrl("noActiveFocusOnTab.qml"));
    qqw2->move(100, 0);
    window1.show();
    qqw->setFocus();
    QVERIFY(QTest::qWaitForWindowExposed(&window1, 5000));
    QVERIFY(qqw->hasFocus());
    QQuickItem *item = qobject_cast<QQuickItem *>(qqw->rootObject());
    QQuickItem *topItem = item->findChild<QQuickItem *>("topRect");
    QQuickItem *middleItem = item->findChild<QQuickItem *>("middleRect");
    QQuickItem *bottomItem = item->findChild<QQuickItem *>("bottomRect");
    topItem->forceActiveFocus();
    QVERIFY(topItem->property("activeFocus").toBool());
    QTest::keyClick(qqw, Qt::Key_Tab);
    QTRY_VERIFY(middleItem->property("activeFocus").toBool());
    QTest::keyClick(qqw, Qt::Key_Tab);
    QTRY_VERIFY(bottomItem->property("activeFocus").toBool());
    QTest::keyClick(qqw, Qt::Key_Backtab);
    QTRY_VERIFY(middleItem->property("activeFocus").toBool());

    qqw2->setFocus();
    QQuickItem *item2 = qobject_cast<QQuickItem *>(qqw2->rootObject());
    QQuickItem *topItem2 = item2->findChild<QQuickItem *>("topRect2");
    QTRY_VERIFY(qqw2->hasFocus());
    QVERIFY(topItem2->property("activeFocus").toBool());
    QTest::keyClick(qqw2, Qt::Key_Tab);
    QTRY_VERIFY(qqw->hasFocus());
    QVERIFY(middleItem->property("activeFocus").toBool());
}

class Overlay : public QQuickItem, public QQuickItemChangeListener
{
    Q_OBJECT

public:
    Overlay() = default;

    ~Overlay()
    {
        QQuickItemPrivate::get(parentItem())->removeItemChangeListener(this, QQuickItemPrivate::Geometry);
    }

    // componentCompleted() is too early to add the listener, as parentItem()
    // is still null by that stage, so we use this function instead.
    void startListening()
    {
        QQuickItemPrivate::get(parentItem())->addItemChangeListener(this, QQuickItemPrivate::Geometry);
    }

private:
    void itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF &/*oldGeometry*/) override
    {
        auto window = QQuickItemPrivate::get(this)->window;
        if (!window)
            return;

        setSize(window->size());
    }
};

// Test that an item that resizes itself based on the window size can use a
// Geometry item change listener to respond to changes in size. This is a
// simplified test to mimic a use case involving Overlay from Qt Quick Controls 2.
void tst_qquickwidget::resizeOverlay()
{
    QWidget widget;
    auto contentVerticalLayout = new QVBoxLayout(&widget);
    contentVerticalLayout->setContentsMargins(0, 0, 0, 0);

    qmlRegisterType<Overlay>("Test", 1, 0, "Overlay");

    auto quickWidget = new QQuickWidget(testFileUrl("resizeOverlay.qml"), &widget);
    QCOMPARE(quickWidget->status(), QQuickWidget::Ready);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    contentVerticalLayout->addWidget(quickWidget);

    auto rootItem = qobject_cast<QQuickItem*>(quickWidget->rootObject());
    QVERIFY(rootItem);

    auto overlay = rootItem->property("overlay").value<Overlay*>();
    QVERIFY(overlay);
    QVERIFY(overlay->parentItem());
    overlay->startListening();

    widget.resize(200, 200);
    QTestPrivate::androidCompatibleShow(&widget);
    QCOMPARE(rootItem->width(), 200);
    QCOMPARE(rootItem->height(), 200);
    QCOMPARE(overlay->width(), rootItem->width());
    QCOMPARE(overlay->height(), rootItem->height());

    widget.resize(300, 300);
    QCOMPARE(rootItem->width(), 300);
    QCOMPARE(rootItem->height(), 300);
    QCOMPARE(overlay->width(), rootItem->width());
    QCOMPARE(overlay->height(), rootItem->height());
}

void tst_qquickwidget::controls()
{
    // Smoke test for having some basic Quick Controls in a scene in a QQuickWidget.
    QWidget widget;
    QVBoxLayout *contentVerticalLayout = new QVBoxLayout(&widget);
    QQuickWidget *quickWidget = new QQuickWidget(testFileUrl("controls.qml"), &widget);
    QCOMPARE(quickWidget->status(), QQuickWidget::Ready);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    contentVerticalLayout->addWidget(quickWidget);

    widget.resize(400, 400);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(quickWidget->rootObject());
    QVERIFY(rootItem);
    QCOMPARE(rootItem->size(), quickWidget->size());
    QSize oldSize = quickWidget->size();

    // Verify that QTBUG-95937 no longer occurs. (on Windows with the default
    // native windows style this used to assert in debug builds)
    widget.resize(300, 300);
    QTRY_VERIFY(quickWidget->width() < oldSize.width());
    QTRY_COMPARE(rootItem->size(), quickWidget->size());

    widget.hide();
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
}

void tst_qquickwidget::focusOnClick()
{
    QQuickWidget quick;
    quick.setSource(testFileUrl("FocusOnClick.qml"));
    quick.show();
    QVERIFY(QTest::qWaitForWindowExposed(&quick));
    QQuickItem *rootItem = quick.rootObject();
    QVERIFY(rootItem);
    QWindow *window = quick.windowHandle();
    QVERIFY(window);

    QQuickItem *text1 = rootItem->findChild<QQuickItem *>("text1");
    QVERIFY(text1);
    QQuickItem *text2 = rootItem->findChild<QQuickItem *>("text2");
    QVERIFY(text2);

    QTest::mouseClick(window, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(75, 25));
    QTRY_VERIFY(text1->hasActiveFocus());
    QVERIFY(!text2->hasActiveFocus());

    QTest::mouseClick(window, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(75, 75));
    QTRY_VERIFY(text2->hasActiveFocus());
    QVERIFY(!text1->hasActiveFocus());

}

#if QT_CONFIG(graphicsview)
void tst_qquickwidget::focusOnClickInProxyWidget()
{
    QGraphicsScene scene(0,0,400,400);

    QGraphicsView view1(&scene);
    view1.setFrameStyle(QFrame::NoFrame);
    view1.resize(400,400);
    QTestPrivate::androidCompatibleShow(&view1);



    QQuickWidget* quick = new QQuickWidget(testFileUrl("FocusOnClick.qml"));
    quick->resize(300,100);
    quick->setAttribute(Qt::WA_AcceptTouchEvents);
    QGraphicsProxyWidget* proxy =  scene.addWidget(quick);
    proxy->setAcceptTouchEvents(true);

   // QTRY_VERIFY(quick->rootObject());
    QQuickItem *rootItem = quick->rootObject();
    QVERIFY(rootItem);
    QQuickItem *text1 = rootItem->findChild<QQuickItem *>("text1");
    QVERIFY(text1);
    QQuickItem *text2 = rootItem->findChild<QQuickItem *>("text2");
    QVERIFY(text2);

    QVERIFY(QTest::qWaitForWindowExposed(&view1));
    QWindow *window1 = view1.windowHandle();
    QVERIFY(window1);

    // Click in the QGraphicsView, outside the QuickWidget
    QTest::mouseClick(window1, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(300, 300));
    QTRY_VERIFY(!text1->hasActiveFocus());
    QTRY_VERIFY(!text2->hasActiveFocus());

    // Click on text1
    QTest::mouseClick(window1, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(75, 25));
    QTRY_VERIFY(text1->hasActiveFocus());
    QVERIFY(!text2->hasActiveFocus());


    // Now create a second view and repeat, in order to verify that we handle one QQuickItem being in multiple windows
    QGraphicsView view2(&scene);
    view2.resize(400,400);
    QTestPrivate::androidCompatibleShow(&view2);

    QVERIFY(QTest::qWaitForWindowExposed(&view2));
    QWindow *window2 = view2.windowHandle();
    QVERIFY(window2);

    QTest::mouseClick(window2, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(300, 300));
    QTRY_VERIFY(!text1->hasActiveFocus());
    QTRY_VERIFY(!text2->hasActiveFocus());

    QTest::mouseClick(window2, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(75, 25));
    QTRY_VERIFY(text1->hasActiveFocus());
    QVERIFY(!text2->hasActiveFocus());
}
#endif

void tst_qquickwidget::focusPreserved()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window Activation is not supported.");
    if (QGuiApplication::platformName() == "android")
        QSKIP("Test doesn't exit cleanly on Android and generates many warnings - QTBUG-112696");

    QScopedPointer<QWidget> widget(new QWidget());
    QScopedPointer<QQuickWidget> quick(new QQuickWidget());
    QQuickItem *root = new QQuickItem(); // will be owned by quick after setContent
    QScopedPointer<QQuickItem> content(new QQuickItem());
    content->setActiveFocusOnTab(true);
    content->setFocus(true);
    quick->setFocusPolicy(Qt::StrongFocus);
    quick->setContent(QUrl(), nullptr, root);
    root->setFlag(QQuickItem::ItemHasContents);
    content->setParentItem(root);

    quick->setGeometry(0, 0, 200, 200);
    quick->show();
    quick->setFocus();
    quick->activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(quick.get()));
    QTRY_VERIFY(quick->hasFocus());
    QTRY_VERIFY(content->hasFocus());
    QTRY_VERIFY(content->hasActiveFocus());

    widget->show();
    widget->setFocus();
    widget->activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(widget.get()));
    QTRY_VERIFY(widget->hasFocus());

    quick->setParent(widget.get());

    quick->show();
    quick->setFocus();
    quick->activateWindow();
    QTRY_VERIFY(quick->hasFocus());
    QTRY_VERIFY(content->hasFocus());
    QTRY_VERIFY(content->hasActiveFocus());
}

/*
    Reparenting the QQuickWidget recreates the offscreen QQuickWindow.
    Since the accessible interface that is cached for the QQuickWidget dispatches
    all calls to the offscreen QQuickWindow, it must fix itself when the offscreen
    view changes. QTBUG-108226
*/
void tst_qquickwidget::accessibilityHandlesViewChange()
{
    if (QGuiApplication::platformName() == "offscreen")
        QSKIP("Doesn't test anything on offscreen platform.");
    if (QGuiApplication::platformName() == "android")
        QSKIP("Test doesn't exit cleanly on Android and generates many warnings - QTBUG-112696");

    QWidget window;

    QPointer<QQuickWindow> backingScene;

    QQuickWidget *childView = new QQuickWidget(&window);
    childView->setSource(testFileUrl("rectangle.qml"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    backingScene = childView->quickWindow();
    QVERIFY(backingScene);

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(childView);
    QVERIFY(iface);
    (void)iface->child(0);

    std::unique_ptr<QQuickWidget> quickWidget(childView);
    childView->setParent(nullptr);
    childView->show();
    QVERIFY(QTest::qWaitForWindowExposed(childView));
    QVERIFY(!backingScene); // the old QQuickWindow should be gone now
    QVERIFY(childView->quickWindow()); // long live the new QQuickWindow

    iface = QAccessible::queryAccessibleInterface(childView);
    QVERIFY(iface);
    // this would crash if QAccessibleQuickWidget hadn't repaired itself to
    // delegate calls to the new (or at least not the old, destroyed) QQuickWindow.
    (void)iface->child(0);
}


QTEST_MAIN(tst_qquickwidget)

#include "tst_qquickwidget.moc"
