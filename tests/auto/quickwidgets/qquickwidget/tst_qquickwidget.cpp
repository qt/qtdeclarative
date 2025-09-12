// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <qtesttouch.h>
#include <QtTest/QSignalSpy>
#include <QtTest/QTestAccessibility>
#include <QtTest/private/qtesthelpers_p.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickTemplates2/private/qquickoverlay_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtGui/QWindow>
#include <QtGui/QScreen>
#include <QtGui/QAccessible>
#include <QtGui/QImage>
#include <QtGui/qpa/qplatformaccessibility.h>
#include <QtCore/QDebug>
#include <QtQml/qqmlengine.h>

#include <QtCore/QLoggingCategory>
#include <QtGui/qstylehints.h>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/private/qapplication_p.h>

#include <QtQuickWidgets/QQuickWidget>

using namespace Qt::StringLiterals;

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

#if QT_CONFIG(accessibility)
private:
    bool initAccessibility();
#endif

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
    void touchTapHandler_data();
    void touchTapHandler();
    void touchMultipleWidgets();
    void tabKey();
    void focusChain_data();
    void focusChain();
    void resizeOverlay();
    void overlayGeometry_data();
    void overlayGeometry();
    void controls();
    void focusOnClick();
#if QT_CONFIG(graphicsview)
    void focusOnClickInProxyWidget();
#endif
    void focusPreserved();
#if QT_CONFIG(accessibility)
    void accessibilityHandlesViewChange();
    void accessibleParentOfQuickItems();
#endif
    void cleanupRhi();
    void dontRecreateRootElementOnWindowChange();
    void setInitialProperties();
    void fromModuleCtor();
    void loadFromModule_data();
    void loadFromModule();

private:
    QPointingDevice *device = QTest::createTouchDevice();
    const QRect m_availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
};

tst_qquickwidget::tst_qquickwidget()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

#if QT_CONFIG(accessibility)
bool tst_qquickwidget::initAccessibility()
{
    // Copied from tst_QQuickAccessible::initTestCase()
    QTestAccessibility::initialize();
    QPlatformIntegration *pfIntegration = QGuiApplicationPrivate::platformIntegration();
    if (!pfIntegration->accessibility())
        return false;
    pfIntegration->accessibility()->setActive(true);
    return true;
}
#endif

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
    QCOMPARE(view->quickWindow()->contentItem()->size(), view->size());

    // size update from view
    view->resize(QSize(80,100));

    QTRY_COMPARE(item->width(), 80.0);
    QCOMPARE(item->height(), 100.0);
    QCOMPARE(view->size(), QSize(80, 100));
    QCOMPARE(view->size(), view->sizeHint());
    QCOMPARE(view->quickWindow()->contentItem()->size(), view->size());


    view->setResizeMode(QQuickWidget::SizeViewToRootObject);

    // size update from view disabled
    view->resize(QSize(60,80));
    QCOMPARE(item->width(), 80.0);
    QCOMPARE(item->height(), 100.0);
    QTRY_COMPARE(view->size(), QSize(60, 80));
    QCOMPARE(view->quickWindow()->contentItem()->size(), view->size());


    // size update from root object
    item->setWidth(250);
    item->setHeight(350);
    QCOMPARE(item->width(), 250.0);
    QCOMPARE(item->height(), 350.0);
    QTRY_COMPARE(view->size(), QSize(250, 350));
    QCOMPARE(view->size(), QSize(250, 350));
    QCOMPARE(view->size(), view->sizeHint());
    QCOMPARE(view->quickWindow()->contentItem()->size(), view->size());

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
    QCOMPARE(view->quickWindow()->contentItem()->size(), view->size());

    // size update from root object
    item->setWidth(80);
    item->setHeight(100);
    QCOMPARE(item->width(), 80.0);
    QCOMPARE(item->height(), 100.0);
    QTRY_COMPARE(view->size(), QSize(80, 100));
    QCOMPARE(view->size(), view->sizeHint());
    QCOMPARE(view->quickWindow()->contentItem()->size(), view->size());

    // size update from root object disabled
    view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    item->setWidth(60);
    item->setHeight(80);
    QCOMPARE(view->width(), 80);
    QCOMPARE(view->height(), 100);
    QCOMPARE(QSize(item->width(), item->height()), view->sizeHint());
    QCOMPARE(view->quickWindow()->contentItem()->size(), view->size());

    // size update from view
    view->resize(QSize(200,300));
    QTRY_COMPARE(item->width(), 200.0);
    QCOMPARE(item->height(), 300.0);
    QCOMPARE(view->size(), QSize(200, 300));
    QCOMPARE(view->size(), view->sizeHint());
    QCOMPARE(view->quickWindow()->contentItem()->size(), view->size());

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
    QCOMPARE(view->quickWindow()->contentItem()->size(), view->size());

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

    const QMargins safeMargins = window.windowHandle()->safeAreaMargins();
    QTRY_COMPARE(view->height(), 200);
    QTRY_COMPARE(label->y() - safeMargins.top(), 200);

    item->setSize(QSizeF(100,100));
    QCOMPARE(item->height(), 100.0);
    QTRY_COMPARE(view->height(), 100);
    QTRY_COMPARE(label->y() - safeMargins.top(), 100);
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

    QQuickWidget invalidRoot;
    invalidRoot.setSource(testFileUrl("error2.qml")); // don't crash
    QCOMPARE(invalidRoot.status(), QQuickWidget::Error);
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

void tst_qquickwidget::touchTapHandler_data()
{
    QTest::addColumn<bool>("guiSynthMouse"); // AA_SynthesizeMouseForUnhandledTouchEvents
    QTest::addColumn<QQuickTapHandler::GesturePolicy>("gesturePolicy");

    // QTest::newRow("nosynth: passive grab") << false << QQuickTapHandler::DragThreshold; // still failing
    QTest::newRow("nosynth: exclusive grab") << false << QQuickTapHandler::ReleaseWithinBounds;
    QTest::newRow("allowsynth: passive grab") << true << QQuickTapHandler::DragThreshold; // QTBUG-113558
    QTest::newRow("allowsynth: exclusive grab") << true << QQuickTapHandler::ReleaseWithinBounds;
}

void tst_qquickwidget::touchTapHandler()
{
    QFETCH(bool, guiSynthMouse);
    QFETCH(QQuickTapHandler::GesturePolicy, gesturePolicy);

    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, guiSynthMouse);
    QQuickWidget quick;
    if (!quick.testAttribute(Qt::WA_AcceptTouchEvents))
        QSKIP("irrelevant on non-touch platforms");

    quick.setSource(testFileUrl("tapHandler.qml"));
    quick.show();
    QVERIFY(QTest::qWaitForWindowExposed(&quick));

    QQuickItem *rootItem = quick.rootObject();
    QVERIFY(rootItem);
    QQuickTapHandler *th = rootItem->findChild<QQuickTapHandler *>();
    QVERIFY(th);
    th->setGesturePolicy(gesturePolicy);
    QSignalSpy tappedSpy(th, &QQuickTapHandler::tapped);

    const QPoint p(50, 50);
    QTest::touchEvent(&quick, device).press(0, p, &quick);
    QTRY_COMPARE(th->isPressed(), true);
    QTest::touchEvent(&quick, device).release(0, p, &quick);
    QTRY_COMPARE(tappedSpy.size(), 1);
    QCOMPARE(th->isPressed(), false);
}

void tst_qquickwidget::touchMultipleWidgets()
{
    QWidget window;
    QQuickWidget *leftQuick = new QQuickWidget;
    leftQuick->setSource(testFileUrl("button.qml"));
    if (!leftQuick->testAttribute(Qt::WA_AcceptTouchEvents))
        QSKIP("irrelevant on non-touch platforms");

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
    QVERIFY(QTest::qWaitForWindowExposed(&window1, 5000));
    qqw->setFocus();
    QTRY_VERIFY(qqw->hasFocus());
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
    // Focus-in by tab/backtab events makes the first/last focusable item to be focused
    QVERIFY(topItem->property("activeFocus").toBool());
}

void tst_qquickwidget::focusChain_data()
{
    if (QGuiApplication::styleHints()->tabFocusBehavior() != Qt::TabFocusAllControls)
        QSKIP("This function doesn't support NOT iterating all.");

    QTest::addColumn<bool>("forward");

    QTest::addRow("Forward") << true;
    QTest::addRow("Backward") << false;
}

void tst_qquickwidget::focusChain()
{
    QFETCH(bool, forward);

    QWidget window;
    window.resize(300, 300);
    QHBoxLayout layout(&window);
    QQuickWidget qqw1;
    qqw1.setObjectName("qqw1");
    qqw1.setResizeMode(QQuickWidget::SizeRootObjectToView);
    qqw1.setSource(testFileUrl("activeFocusOnTab.qml"));
    QWidget middleWidget;
    middleWidget.setObjectName("middleWidget");
    middleWidget.setFocusPolicy(Qt::FocusPolicy::TabFocus);
    QQuickWidget qqw2;
    qqw2.setObjectName("qqw2");
    qqw1.setResizeMode(QQuickWidget::SizeRootObjectToView);
    qqw2.setSource(testFileUrl("activeFocusOnTab.qml"));
    layout.addWidget(&qqw1);
    layout.addWidget(&middleWidget);
    layout.addWidget(&qqw2);
    window.show();
    window.windowHandle()->requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQuickItem *root1 = qqw1.rootObject();
    QVERIFY(root1);
    QQuickItem *topItem1 = root1->findChild<QQuickItem *>("topRect");
    topItem1->setObjectName("topRect1");
    QVERIFY(topItem1);
    QQuickItem *middleItem1 = root1->findChild<QQuickItem *>("middleRect");
    middleItem1->setObjectName("middleRect1");
    QVERIFY(middleItem1);
    QQuickItem *bottomItem1 = root1->findChild<QQuickItem *>("bottomRect");
    bottomItem1->setObjectName("bottomRect1");
    QVERIFY(bottomItem1);

    QQuickItem *root2 = qqw2.rootObject();
    QVERIFY(root2);
    QQuickItem *topItem2 = root2->findChild<QQuickItem *>("topRect");
    topItem2->setObjectName("topRect2");
    QVERIFY(topItem2);
    QQuickItem *middleItem2 = root2->findChild<QQuickItem *>("middleRect");
    middleItem2->setObjectName("middleRect2");
    QVERIFY(middleItem2);
    QQuickItem *bottomItem2 = root2->findChild<QQuickItem *>("bottomRect");
    bottomItem2->setObjectName("bottomRect2");
    QVERIFY(bottomItem2);

    qqw1.setFocus();
    QTRY_VERIFY(qqw1.hasFocus());

    auto hasActiveFocus = [](const QObject *targetObject) -> bool
    {
        if (const QQuickItem *targetItem = qobject_cast<const QQuickItem *>(targetObject))
            return targetItem->hasActiveFocus();
        else if (const QWidget *targetWidget = qobject_cast<const QWidget *>(targetObject))
            return targetWidget->hasFocus();
        return false;
    };

    QList<QObject *> expectedFocusChain;
    if (forward) {
        expectedFocusChain << topItem1 << middleItem1 << bottomItem1 << &middleWidget << topItem2
                           << middleItem2 << bottomItem2 << topItem1;
    } else {
        expectedFocusChain << topItem1 << bottomItem2 << middleItem2 << topItem2 << &middleWidget
                           << bottomItem1 << middleItem1 << topItem1;
    }

    const Qt::Key tabKey = forward ? Qt::Key_Tab : Qt::Key_Backtab;

    for (QObject *expectedFocusTarget : expectedFocusChain) {
        const QByteArray objectName = expectedFocusTarget->objectName().toLatin1();
        QTRY_VERIFY2(hasActiveFocus(expectedFocusTarget), "expectedFocusTarget: " + objectName);
        QTest::keyClick(QGuiApplication::focusWindow(), tabKey);
    }
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

    qmlRegisterType<Overlay>("TestModule", 1, 0, "Overlay");

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

/*
    Overlaps with resizeOverlay, but uses a proper
    Qt Quick Controls Overlay.
*/
void tst_qquickwidget::overlayGeometry_data()
{
    QTest::addColumn<QQuickWidget::ResizeMode>("resizeMode");

    QTest::addRow("SizeRootObjectToView") << QQuickWidget::SizeRootObjectToView;
    QTest::addRow("SizeViewToRootObject") << QQuickWidget::SizeViewToRootObject;
}

void tst_qquickwidget::overlayGeometry()
{
    QFETCH(const QQuickWidget::ResizeMode, resizeMode);

    QWidget window;

    QQuickWidget widget(testFileUrl("overlayGeometry.qml"), &window);
    widget.setResizeMode(resizeMode);

    QCOMPARE(widget.status(), QQuickWidget::Ready);
    const auto *rootItem = qobject_cast<QQuickItem*>(widget.rootObject());
    QVERIFY(rootItem);
    const QSizeF preferredSize = rootItem->size();

    auto *popup = rootItem->findChild<QQuickPopup *>("popup");
    QVERIFY(popup);
    popup->open();
    QTRY_VERIFY(popup->isOpened());

    const auto *overlay = QQuickOverlay::overlay(widget.quickWindow());
    QVERIFY(overlay);

    QTestPrivate::androidCompatibleShow(&window);
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCOMPARE(widget.size(), preferredSize.toSize());
    QCOMPARE(overlay->position(), QPointF(0, 0));
    QCOMPARE(overlay->size(), widget.size());

    widget.resize((preferredSize * 2).toSize());
    QCOMPARE(overlay->position(), QPointF(0, 0));
    QCOMPARE(overlay->size(), widget.size());

    // click outside the popup, make sure it closes
    QTest::mouseClick(&widget, Qt::LeftButton, {}, QPoint(10, 10));
    QTRY_VERIFY(!popup->isOpened());
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
    QTRY_VERIFY_ACTIVE_FOCUS(text1);
    QVERIFY(!text2->hasActiveFocus());

    QTest::mouseClick(window, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(75, 75));
    QTRY_VERIFY_ACTIVE_FOCUS(text2);
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
    QTRY_VERIFY_ACTIVE_FOCUS(text1);
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
    QTRY_VERIFY_ACTIVE_FOCUS(text1);
    QVERIFY(!text2->hasActiveFocus());
}
#endif

void tst_qquickwidget::focusPreserved()
{
    SKIP_IF_NO_WINDOW_ACTIVATION;
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

    // added content2 and content3 to test more complicated case
    QScopedPointer<QQuickItem> content2(new QQuickItem());
    content2->setActiveFocusOnTab(true);
    content2->setFocus(true);
    content2->setParentItem(root);
    QScopedPointer<QQuickItem> content3(new QQuickItem());
    content3->setActiveFocusOnTab(true);
    content3->setFocus(true);
    content3->setParentItem(root);

    quick->setGeometry(0, 0, 200, 200);
    quick->show();
    quick->setFocus();
    quick->activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(quick.get()));
    QTRY_VERIFY(quick->hasFocus());
    QTRY_VERIFY(content->hasFocus());
    QTRY_VERIFY_ACTIVE_FOCUS(content.get());

    content2->forceActiveFocus();
    QVERIFY(content2->hasFocus());
    QVERIFY_ACTIVE_FOCUS(content2.get());

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
    QTRY_VERIFY(content2->hasFocus());
    QTRY_VERIFY_ACTIVE_FOCUS(content2.get());
}

#if QT_CONFIG(accessibility)
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

/*
QTBUG-130116
Since the quick widget reports the quick items as its children, then
the quick items should report the quick widget as their parent.
*/
void tst_qquickwidget::accessibleParentOfQuickItems()
{
    if (!initAccessibility())
        QSKIP("This platform does not support accessibility");

    // These lines are copied from accessibilityHandlesViewChange()
    if (QGuiApplication::platformName() == "offscreen")
        QSKIP("Doesn't test anything on offscreen platform.");

    QWidget window;
    window.resize(300, 300);

    QQuickWidget *quickWidget = new QQuickWidget(&window);
    quickWidget->setSource(testFileUrl("button.qml"));

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto *rootItem = quickWidget->rootObject();
    QVERIFY(rootItem);

    QQuickButton *button = rootItem->findChild<QQuickButton *>("button");
    QVERIFY(button);

    QAccessibleInterface *iface_quickWidget = QAccessible::queryAccessibleInterface(quickWidget);
    QVERIFY(iface_quickWidget);

    QAccessibleInterface *iface_button = QAccessible::queryAccessibleInterface(button);
    QVERIFY(iface_button);

    QAccessibleInterface *iface_popup = iface_button->parent();
    QVERIFY(iface_popup);
    QVERIFY(iface_popup->parent());
    QCOMPARE(iface_popup->parent(), iface_quickWidget);
}
#endif

class CreateDestroyWidget : public QWidget
{
public:
    using QWidget::create;
    using QWidget::destroy;
};

void tst_qquickwidget::cleanupRhi()
{
    CreateDestroyWidget topLevel;
    QQuickWidget quickWidget(&topLevel);
    quickWidget.setSource(testFileUrl("rectangle.qml"));
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

    topLevel.destroy();
    topLevel.create();
}

void tst_qquickwidget::dontRecreateRootElementOnWindowChange()
{
    auto *quickWidget = new QQuickWidget();
    quickWidget->setSource(testFileUrl("rectangle.qml"));
    QObject *item = quickWidget->rootObject();

    bool wasDestroyed = false;
    QObject::connect(item, &QObject::destroyed, this, [&] { wasDestroyed = true; });

    QEvent event(QEvent::WindowChangeInternal);
    QCoreApplication::sendEvent(quickWidget, &event);

    QVERIFY(!wasDestroyed);
}

void tst_qquickwidget::setInitialProperties()
{
    QQuickWidget widget;
    widget.setInitialProperties({{"z", 4}, {"width", 100}});
    widget.setSource(testFileUrl("resizemodeitem.qml"));
    widget.show();
    QObject *rootObject = widget.rootObject();
    QVERIFY(rootObject);
    QCOMPARE(rootObject->property("z").toInt(), 4);
    QCOMPARE(rootObject->property("width").toInt(), 100);
}

void tst_qquickwidget::fromModuleCtor()
{
    QQuickWidget widget("QtQuick", "Rectangle");
    // creation is always synchronous for C++ defined types, so we don't need _TRY
    QObject *rootObject = widget.rootObject();
    QVERIFY(rootObject);
    QCOMPARE(rootObject->metaObject()->className(), "QQuickRectangle");
}

void tst_qquickwidget::loadFromModule_data()
{
    QTest::addColumn<QString>("module");
    QTest::addColumn<QString>("typeName");
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<QQuickWidget::Status>("status");

    QTest::addRow("Item") << u"QtQuick"_s << u"Item"_s << QUrl() << QQuickWidget::Ready;
    QTest::addRow("composite") << u"test"_s << u"TestQml"_s << QUrl("qrc:/qt/qml/test/data/TestQml.qml") << QQuickWidget::Ready;
    QTest::addRow("nonexistent") << u"missing"_s << u"Type"_s << QUrl() << QQuickWidget::Error;
}

void tst_qquickwidget::loadFromModule()
{
    QFETCH(QString, module);
    QFETCH(QString, typeName);
    QFETCH(QUrl, url);
    QFETCH(QQuickWidget::Status, status);

    QQuickWidget widget;
    widget.loadFromModule(module, typeName);
    QTRY_COMPARE(widget.status(), status);
    QCOMPARE(widget.source(), url);
    if (status == QQuickWidget::Ready) {
        QPointer<QObject> rootObject = widget.rootObject();
        QVERIFY(rootObject);
        // loadFromModule sets the source and deletes
        // any object that was previously created
        widget.loadFromModule("QtTest", "SignalSpy");
        QVERIFY(rootObject.isNull());
        QCOMPARE(widget.status(), QQuickWidget::Ready);
        QVERIFY(widget.source().toString().endsWith("SignalSpy.qml"));
    }
}

QTEST_MAIN(tst_qquickwidget)

#include "tst_qquickwidget.moc"
