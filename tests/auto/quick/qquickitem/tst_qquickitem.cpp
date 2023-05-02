// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>

#include <QtQml/QQmlComponent>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickview.h>
#include "private/qquickfocusscope_p.h"
#include "private/qquickrectangle_p.h"
#include "private/qquickitem_p.h"
#include <QtGui/private/qevent_p.h>
#include <qpa/qwindowsysteminterface.h>
#ifdef Q_OS_WIN
#include <QOpenGLContext>
#endif
#include <QDebug>
#include <QTimer>
#include <QQmlEngine>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QSignalSpy>
#include <QTranslator>
#include <QtCore/qregularexpression.h>

#ifdef TEST_QTBUG_60123
#include <QWidget>
#include <QMainWindow>
#endif

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

class TestItem : public QQuickItem
{
Q_OBJECT
public:
    TestItem(QQuickItem *parent = nullptr)
        : QQuickItem(parent)
    {
    }

    bool focused = false;
    int pressCount = 0;
    int releaseCount = 0;
    int wheelCount = 0;
    bool acceptIncomingTouchEvents = true;
    bool touchEventReached = false;
    ulong timestamp = 0;
    QPoint lastWheelEventPos;
    QPoint lastWheelEventGlobalPos;
    int languageChangeEventCount = 0;
    int localeChangeEventCount = 0;
protected:
    void focusInEvent(QFocusEvent *event) override {
        qCDebug(lcTests) << objectName() << event;
        Q_ASSERT(!focused);
        focused = true;
    }
    void focusOutEvent(QFocusEvent *event) override {
        qCDebug(lcTests) << objectName() << event;
        Q_ASSERT(focused);
        focused = false;
    }
    void mousePressEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << objectName() << event;
        // Tradition holds that an item or widget does not need to accept a mouse event:
        // it arrives pre-accepted.
        ++pressCount;
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << objectName() << event;
        ++releaseCount;
    }
    void touchEvent(QTouchEvent *event) override {
        qCDebug(lcTests) << objectName() << event;
        touchEventReached = true;
        event->setAccepted(acceptIncomingTouchEvents);
    }
    void wheelEvent(QWheelEvent *event) override {
        qCDebug(lcTests) << objectName() << event;
        event->accept();
        ++wheelCount;
        timestamp = event->timestamp();
        lastWheelEventPos = event->position().toPoint();
        lastWheelEventGlobalPos = event->globalPosition().toPoint();
    }
    bool event(QEvent *e) override
    {
        Q_ASSERT(e->isAccepted()); // every event is constructed with the accept flag initialized to true
        switch (e->type()) {
        case QEvent::LanguageChange:
            ++languageChangeEventCount;
            break;
        case QEvent::LocaleChange:
            ++localeChangeEventCount;
            break;
        default:
            break;
        }
        return QQuickItem::event(e); // default dispatch
    }
};

class TestWindow: public QQuickWindow
{
public:
    TestWindow()
        : QQuickWindow()
    {}

    bool event(QEvent *event) override
    {
        return QQuickWindow::event(event);
    }
};

class TestPolishItem : public QQuickItem
{
Q_OBJECT
public:
    TestPolishItem(QQuickItem *parent = nullptr)
    : QQuickItem(parent), wasPolished(false) {

    }

    bool wasPolished;
    int repolishLoopCount = 0;

protected:
    void updatePolish() override {
        wasPolished = true;
        if (repolishLoopCount > 0) {
            --repolishLoopCount;
            polish();
        }
    }

public slots:
    void doPolish() {
        polish();
    }
};

class TestFocusScope : public QQuickFocusScope
{
Q_OBJECT
public:
    TestFocusScope(QQuickItem *parent = nullptr) : QQuickFocusScope(parent), focused(false) {}

    bool focused;
protected:
    void focusInEvent(QFocusEvent *) override { Q_ASSERT(!focused); focused = true; }
    void focusOutEvent(QFocusEvent *) override { Q_ASSERT(focused); focused = false; }
};

class tst_qquickitem : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickitem();

private slots:
    void initTestCase() override;

    void noWindow();
    void simpleFocus();
    void scopedFocus();
    void addedToWindow();
    void changeParent();
    void multipleFocusClears();
    void focusSubItemInNonFocusScope();
    void parentItemWithFocus();
    void reparentFocusedItem();
    void activeFocusChangedOrder();

    void constructor();
    void setParentItem();

    void visible();
    void enabled();
    void enabledFocus();

    void mouseGrab();
    void mousePropagationToParent();
    void touchEventAcceptIgnore_data();
    void touchEventAcceptIgnore();
    void polishOutsideAnimation();
    void polishOnCompleted();

    void wheelEvent_data();
    void wheelEvent();
    void hoverEvent_data();
    void hoverEvent();
    void hoverEventInParent();

    void paintOrder_data();
    void paintOrder();

    void acceptedMouseButtons();

    void visualParentOwnership();
    void visualParentOwnershipWindow();

    void testSGInvalidate();

    void objectChildTransform();

    void contains_data();
    void contains();
    void containsContainmentMask_data();
    void containsContainmentMask();

    void childAt();

    void ignoreButtonPressNotInAcceptedMouseButtons();

    void shortcutOverride();

#ifdef TEST_QTBUG_60123
    void qtBug60123();
#endif

    void setParentCalledInOnWindowChanged();
    void receivesLanguageChangeEvent();
    void receivesLocaleChangeEvent();
    void polishLoopDetection_data();
    void polishLoopDetection();

    void objectCastInDestructor();
    void listsAreNotLists();

private:

    enum PaintOrderOp {
        NoOp, Append, Remove, StackBefore, StackAfter, SetZ
    };

    bool ensureFocus(QWindow *w) {
        if (w->width() <=0 || w->height() <= 0)
            w->setGeometry(100, 100, 400, 300);
        w->show();
        w->requestActivate();
        return QTest::qWaitForWindowActive(w);
    }
};

tst_qquickitem::tst_qquickitem()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquickitem::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<TestPolishItem>("Qt.test", 1, 0, "TestPolishItem");
}

// Focus still updates when outside a window
void tst_qquickitem::noWindow()
{
    QQuickItem *root = new TestItem;
    QQuickItem *child = new TestItem(root);
    QQuickItem *scope = new TestItem(root);
    QQuickFocusScope *scopedChild = new TestFocusScope(scope);
    QQuickFocusScope *scopedChild2 = new TestFocusScope(scope);

    QCOMPARE(root->hasFocus(), false);
    QCOMPARE(child->hasFocus(), false);
    QCOMPARE(scope->hasFocus(), false);
    QCOMPARE(scopedChild->hasFocus(), false);
    QCOMPARE(scopedChild2->hasFocus(), false);

    root->setFocus(true);
    scope->setFocus(true);
    scopedChild2->setFocus(true);
    QCOMPARE(root->hasFocus(), false);
    QCOMPARE(child->hasFocus(), false);
    QCOMPARE(scope->hasFocus(), false);
    QCOMPARE(scopedChild->hasFocus(), false);
    QCOMPARE(scopedChild2->hasFocus(), true);

    root->setFocus(false);
    child->setFocus(true);
    scopedChild->setFocus(true);
    scope->setFocus(false);
    QCOMPARE(root->hasFocus(), false);
    QCOMPARE(child->hasFocus(), false);
    QCOMPARE(scope->hasFocus(), false);
    QCOMPARE(scopedChild->hasFocus(), true);
    QCOMPARE(scopedChild2->hasFocus(), false);

    delete root;
}

struct FocusData {
    FocusData() : focus(false), activeFocus(false) {}

    void set(bool f, bool af) { focus = f; activeFocus = af; }
    bool focus;
    bool activeFocus;
};
struct FocusState : public QHash<QQuickItem *, FocusData>
{
    FocusState() : activeFocusItem(nullptr) {}
    FocusState &operator<<(QQuickItem *item) {
        insert(item, FocusData());
        return *this;
    }

    void active(QQuickItem *i) {
        activeFocusItem = i;
    }
    QQuickItem *activeFocusItem;
};

#define FVERIFY() \
    do { \
        if (focusState.activeFocusItem) { \
            QCOMPARE(window.activeFocusItem(), focusState.activeFocusItem); \
            if (qobject_cast<TestItem *>(window.activeFocusItem())) \
                QCOMPARE(qobject_cast<TestItem *>(window.activeFocusItem())->focused, true); \
            else if (qobject_cast<TestFocusScope *>(window.activeFocusItem())) \
                QCOMPARE(qobject_cast<TestFocusScope *>(window.activeFocusItem())->focused, true); \
        } else { \
            QCOMPARE(window.activeFocusItem(), window.contentItem()); \
        } \
        for (QHash<QQuickItem *, FocusData>::Iterator iter = focusState.begin(); \
            iter != focusState.end(); \
            iter++) { \
            QCOMPARE(iter.key()->hasFocus(), iter.value().focus); \
            QCOMPARE(iter.key()->hasActiveFocus(), iter.value().activeFocus); \
        } \
    } while (false)

// Tests a simple set of top-level scoped items
void tst_qquickitem::simpleFocus()
{
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));

    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);

    QQuickItem *l1c1 = new TestItem(window.contentItem());
    QQuickItem *l1c2 = new TestItem(window.contentItem());
    QQuickItem *l1c3 = new TestItem(window.contentItem());

    QQuickItem *l2c1 = new TestItem(l1c1);
    QQuickItem *l2c2 = new TestItem(l1c1);
    QQuickItem *l2c3 = new TestItem(l1c3);

    FocusState focusState;
    focusState << l1c1 << l1c2 << l1c3
               << l2c1 << l2c2 << l2c3;
    FVERIFY();

    l1c1->setFocus(true);
    focusState[l1c1].set(true, true);
    focusState.active(l1c1);
    FVERIFY();

    l2c3->setFocus(true);
    focusState[l1c1].set(false, false);
    focusState[l2c3].set(true, true);
    focusState.active(l2c3);
    FVERIFY();

    l1c3->setFocus(true);
    focusState[l2c3].set(false, false);
    focusState[l1c3].set(true, true);
    focusState.active(l1c3);
    FVERIFY();

    l1c2->setFocus(false);
    FVERIFY();

    l1c3->setFocus(false);
    focusState[l1c3].set(false, false);
    focusState.active(nullptr);
    FVERIFY();

    l2c1->setFocus(true);
    focusState[l2c1].set(true, true);
    focusState.active(l2c1);
    FVERIFY();
}

// Items with a focus scope
void tst_qquickitem::scopedFocus()
{
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);

    QQuickItem *l1c1 = new TestItem(window.contentItem());
    QQuickItem *l1c2 = new TestItem(window.contentItem());
    QQuickItem *l1c3 = new TestItem(window.contentItem());

    QQuickItem *l2c1 = new TestItem(l1c1);
    QQuickItem *l2c2 = new TestItem(l1c1);
    QQuickItem *l2c3 = new TestFocusScope(l1c3);

    QQuickItem *l3c1 = new TestItem(l2c3);
    QQuickItem *l3c2 = new TestFocusScope(l2c3);

    QQuickItem *l4c1 = new TestItem(l3c2);
    QQuickItem *l4c2 = new TestItem(l3c2);

    FocusState focusState;
    focusState << l1c1 << l1c2 << l1c3
               << l2c1 << l2c2 << l2c3
               << l3c1 << l3c2
               << l4c1 << l4c2;
    FVERIFY();

    l4c2->setFocus(true);
    focusState[l4c2].set(true, false);
    FVERIFY();

    l4c1->setFocus(true);
    focusState[l4c2].set(false, false);
    focusState[l4c1].set(true, false);
    FVERIFY();

    l1c1->setFocus(true);
    focusState[l1c1].set(true, true);
    focusState.active(l1c1);
    FVERIFY();

    l3c2->setFocus(true);
    focusState[l3c2].set(true, false);
    FVERIFY();

    l2c3->setFocus(true);
    focusState[l1c1].set(false, false);
    focusState[l2c3].set(true, true);
    focusState[l3c2].set(true, true);
    focusState[l4c1].set(true, true);
    focusState.active(l4c1);
    FVERIFY();

    l3c2->setFocus(false);
    focusState[l3c2].set(false, false);
    focusState[l4c1].set(true, false);
    focusState.active(l2c3);
    FVERIFY();

    l3c2->setFocus(true);
    focusState[l3c2].set(true, true);
    focusState[l4c1].set(true, true);
    focusState.active(l4c1);
    FVERIFY();

    l4c1->setFocus(false);
    focusState[l4c1].set(false, false);
    focusState.active(l3c2);
    FVERIFY();

    l1c3->setFocus(true);
    focusState[l1c3].set(true, true);
    focusState[l2c3].set(false, false);
    focusState[l3c2].set(true, false);
    focusState.active(l1c3);
    FVERIFY();
}

// Tests focus corrects itself when a tree is added to a window for the first time
void tst_qquickitem::addedToWindow()
{
    {
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);

    QQuickItem *item = new TestItem;

    FocusState focusState;
    focusState << item;

    item->setFocus(true);
    focusState[item].set(true, false);
    FVERIFY();

    item->setParentItem(window.contentItem());
    focusState[item].set(true, true);
    focusState.active(item);
    FVERIFY();
    }

    {
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);

    QQuickItem *item = new TestItem(window.contentItem());

    QQuickItem *tree = new TestItem;
    QQuickItem *c1 = new TestItem(tree);
    QQuickItem *c2 = new TestItem(tree);

    FocusState focusState;
    focusState << item << tree << c1 << c2;

    item->setFocus(true);
    c1->setFocus(true);
    c2->setFocus(true);
    focusState[item].set(true, true);
    focusState[c1].set(false, false);
    focusState[c2].set(true, false);
    focusState.active(item);
    FVERIFY();

    tree->setParentItem(item);
    focusState[c1].set(false, false);
    focusState[c2].set(false, false);
    FVERIFY();
    }

    {
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);

    QQuickItem *tree = new TestItem;
    QQuickItem *c1 = new TestItem(tree);
    QQuickItem *c2 = new TestItem(tree);

    FocusState focusState;
    focusState << tree << c1 << c2;
    c1->setFocus(true);
    c2->setFocus(true);
    focusState[c1].set(false, false);
    focusState[c2].set(true, false);
    FVERIFY();

    tree->setParentItem(window.contentItem());
    focusState[c1].set(false, false);
    focusState[c2].set(true, true);
    focusState.active(c2);
    FVERIFY();
    }

    {
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
    QQuickItem *tree = new TestFocusScope;
    QQuickItem *c1 = new TestItem(tree);
    QQuickItem *c2 = new TestItem(tree);

    FocusState focusState;
    focusState << tree << c1 << c2;
    c1->setFocus(true);
    c2->setFocus(true);
    focusState[c1].set(false, false);
    focusState[c2].set(true, false);
    FVERIFY();

    tree->setParentItem(window.contentItem());
    focusState[c1].set(false, false);
    focusState[c2].set(true, false);
    FVERIFY();

    tree->setFocus(true);
    focusState[tree].set(true, true);
    focusState[c2].set(true, true);
    focusState.active(c2);
    FVERIFY();
    }

    {
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
    QQuickItem *tree = new TestFocusScope;
    QQuickItem *c1 = new TestItem(tree);
    QQuickItem *c2 = new TestItem(tree);

    FocusState focusState;
    focusState << tree << c1 << c2;
    tree->setFocus(true);
    c1->setFocus(true);
    c2->setFocus(true);
    focusState[tree].set(true, false);
    focusState[c1].set(false, false);
    focusState[c2].set(true, false);
    FVERIFY();

    tree->setParentItem(window.contentItem());
    focusState[tree].set(true, true);
    focusState[c1].set(false, false);
    focusState[c2].set(true, true);
    focusState.active(c2);
    FVERIFY();
    }

    {
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
    QQuickItem *child = new TestItem(window.contentItem());
    QQuickItem *tree = new TestFocusScope;
    QQuickItem *c1 = new TestItem(tree);
    QQuickItem *c2 = new TestItem(tree);

    FocusState focusState;
    focusState << child << tree << c1 << c2;
    child->setFocus(true);
    tree->setFocus(true);
    c1->setFocus(true);
    c2->setFocus(true);
    focusState[child].set(true, true);
    focusState[tree].set(true, false);
    focusState[c1].set(false, false);
    focusState[c2].set(true, false);
    focusState.active(child);
    FVERIFY();

    tree->setParentItem(window.contentItem());
    focusState[tree].set(false, false);
    focusState[c1].set(false, false);
    focusState[c2].set(true, false);
    FVERIFY();

    tree->setFocus(true);
    focusState[child].set(false, false);
    focusState[tree].set(true, true);
    focusState[c2].set(true, true);
    focusState.active(c2);
    FVERIFY();
    }
}

void tst_qquickitem::changeParent()
{
    // Parent to no parent
    {
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
    QQuickItem *child = new TestItem(window.contentItem());

    FocusState focusState;
    focusState << child;
    FVERIFY();

    child->setFocus(true);
    focusState[child].set(true, true);
    focusState.active(child);
    FVERIFY();

    child->setParentItem(nullptr);
    focusState[child].set(true, false);
    focusState.active(nullptr);
    FVERIFY();
    }

    // Different parent, same focus scope
    {
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
    QQuickItem *child = new TestItem(window.contentItem());
    QQuickItem *child2 = new TestItem(window.contentItem());

    FocusState focusState;
    focusState << child << child2;
    FVERIFY();

    child->setFocus(true);
    focusState[child].set(true, true);
    focusState.active(child);
    FVERIFY();

    child->setParentItem(child2);
    FVERIFY();
    }

    // Different parent, different focus scope
    {
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
    QQuickItem *child = new TestItem(window.contentItem());
    QQuickItem *child2 = new TestFocusScope(window.contentItem());
    QQuickItem *item = new TestItem(child);

    FocusState focusState;
    focusState << child << child2 << item;
    FVERIFY();

    item->setFocus(true);
    focusState[item].set(true, true);
    focusState.active(item);
    FVERIFY();

    item->setParentItem(child2);
    focusState[item].set(true, false);
    focusState.active(nullptr);
    FVERIFY();
    }
    {
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
    QQuickItem *child = new TestItem(window.contentItem());
    QQuickItem *child2 = new TestFocusScope(window.contentItem());
    QQuickItem *item = new TestItem(child2);

    FocusState focusState;
    focusState << child << child2 << item;
    FVERIFY();

    item->setFocus(true);
    focusState[item].set(true, false);
    focusState.active(nullptr);
    FVERIFY();

    item->setParentItem(child);
    focusState[item].set(true, true);
    focusState.active(item);
    FVERIFY();
    }
    {
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
    QQuickItem *child = new TestItem(window.contentItem());
    QQuickItem *child2 = new TestFocusScope(window.contentItem());
    QQuickItem *item = new TestItem(child2);

    FocusState focusState;
    focusState << child << child2 << item;
    FVERIFY();

    child->setFocus(true);
    item->setFocus(true);
    focusState[child].set(true, true);
    focusState[item].set(true, false);
    focusState.active(child);
    FVERIFY();

    item->setParentItem(child);
    focusState[item].set(false, false);
    FVERIFY();
    }

    // child has active focus, then its fs parent changes parent to 0, then
    // child is deleted, then its parent changes again to a valid parent
    {
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
    QQuickItem *item = new TestFocusScope(window.contentItem());
    QQuickItem *child = new TestItem(item);
    QQuickItem *child2 = new TestItem;

    FocusState focusState;
    focusState << item << child;
    FVERIFY();

    item->setFocus(true);
    child->setFocus(true);
    focusState[child].set(true, true);
    focusState[item].set(true, true);
    focusState.active(child);
    FVERIFY();

    item->setParentItem(nullptr);
    focusState[child].set(true, false);
    focusState[item].set(true, false);
    focusState.active(nullptr);
    FVERIFY();

    focusState.remove(child);
    delete child;
    item->setParentItem(window.contentItem());
    focusState[item].set(true, true);
    focusState.active(item);
    FVERIFY();
    delete child2;
    }
}

void tst_qquickitem::multipleFocusClears()
{
    //Multiple clears of focus inside a focus scope shouldn't crash. QTBUG-24714
    QQuickView view;
    view.setSource(testFileUrl("multipleFocusClears.qml"));
    view.show();
    QVERIFY(ensureFocus(&view));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &view);
}

void tst_qquickitem::focusSubItemInNonFocusScope()
{
    QQuickView view;
    view.setSource(testFileUrl("focusSubItemInNonFocusScope.qml"));
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickItem *dummyItem = view.rootObject()->findChild<QQuickItem *>("dummyItem");
    QVERIFY(dummyItem);

    QQuickItem *textInput = view.rootObject()->findChild<QQuickItem *>("textInput");
    QVERIFY(textInput);

    QVERIFY(dummyItem->hasFocus());
    QVERIFY(!textInput->hasFocus());
    QVERIFY(dummyItem->hasActiveFocus());

    QVERIFY(QMetaObject::invokeMethod(textInput, "forceActiveFocus"));

    QVERIFY(!dummyItem->hasFocus());
    QVERIFY(textInput->hasFocus());
    QVERIFY(textInput->hasActiveFocus());
}

void tst_qquickitem::parentItemWithFocus()
{
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
    {
    QQuickItem parent;
    QQuickItem child;

    FocusState focusState;
    focusState << &parent << &child;
    FVERIFY();

    parent.setFocus(true);
    child.setFocus(true);
    focusState[&parent].set(true, false);
    focusState[&child].set(true, false);
    FVERIFY();

    child.setParentItem(&parent);
    focusState[&parent].set(true, false);
    focusState[&child].set(false, false);
    FVERIFY();

    parent.setParentItem(window.contentItem());
    focusState[&parent].set(true, true);
    focusState[&child].set(false, false);
    focusState.active(&parent);
    FVERIFY();

    child.forceActiveFocus();
    focusState[&parent].set(false, false);
    focusState[&child].set(true, true);
    focusState.active(&child);
    FVERIFY();
    } {
    QQuickItem parent;
    QQuickItem child;
    QQuickItem grandchild(&child);

    FocusState focusState;
    focusState << &parent << &child << &grandchild;
    FVERIFY();

    parent.setFocus(true);
    grandchild.setFocus(true);
    focusState[&parent].set(true, false);
    focusState[&child].set(false, false);
    focusState[&grandchild].set(true, false);
    FVERIFY();

    child.setParentItem(&parent);
    focusState[&parent].set(true, false);
    focusState[&child].set(false, false);
    focusState[&grandchild].set(false, false);
    FVERIFY();

    parent.setParentItem(window.contentItem());
    focusState[&parent].set(true, true);
    focusState[&child].set(false, false);
    focusState[&grandchild].set(false, false);
    focusState.active(&parent);
    FVERIFY();

    grandchild.forceActiveFocus();
    focusState[&parent].set(false, false);
    focusState[&child].set(false, false);
    focusState[&grandchild].set(true, true);
    focusState.active(&grandchild);
    FVERIFY();
    }

    {
    QQuickItem parent;
    QQuickItem child1;
    QQuickItem child2;

    FocusState focusState;
    focusState << &parent << &child1 << &child2;
    parent.setFocus(true);
    child1.setParentItem(&parent);
    child2.setParentItem(&parent);
    focusState[&parent].set(true, false);
    focusState[&child1].set(false, false);
    focusState[&child2].set(false, false);
    FVERIFY();

    child1.setFocus(true);
    focusState[&parent].set(false, false);
    focusState[&child1].set(true, false);
    FVERIFY();

    parent.setFocus(true);
    focusState[&parent].set(true, false);
    focusState[&child1].set(false, false);
    FVERIFY();
    }
}

void tst_qquickitem::reparentFocusedItem()
{
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);

    QQuickItem parent(window.contentItem());
    QQuickItem child(&parent);
    QQuickItem sibling(&parent);
    QQuickItem grandchild(&child);

    FocusState focusState;
    focusState << &parent << &child << &sibling << &grandchild;
    FVERIFY();

    grandchild.setFocus(true);
    focusState[&parent].set(false, false);
    focusState[&child].set(false, false);
    focusState[&sibling].set(false, false);
    focusState[&grandchild].set(true, true);
    focusState.active(&grandchild);
    FVERIFY();

    // Parenting the item to another item within the same focus scope shouldn't change it's focus.
    child.setParentItem(&sibling);
    FVERIFY();
}

void tst_qquickitem::activeFocusChangedOrder()
{
    // This test checks that the activeFocusChanged signal first comes for an
    // object that has lost focus, and only after that - for an object that
    // has received focus.

    {
        // Two FocusScopes inside a Window
        QQuickWindow window;
        QVERIFY(ensureFocus(&window));
        QTRY_COMPARE(QGuiApplication::focusWindow(), &window);

        QQuickFocusScope scope1(window.contentItem());
        QQuickItem scope1Child(&scope1);

        QQuickFocusScope scope2(window.contentItem());
        QQuickItem scope2Child(&scope2);

        scope1Child.forceActiveFocus();
        QTRY_VERIFY(scope1.hasActiveFocus());

        int counter = 0;
        connect(&scope1, &QQuickItem::activeFocusChanged, [&counter, &scope1](bool focus) {
            QCOMPARE(scope1.childItems().front()->hasActiveFocus(), focus);
            QCOMPARE(counter, 0);
            counter++;
        });
        connect(&scope2, &QQuickItem::activeFocusChanged, [&counter, &scope2](bool focus) {
            QCOMPARE(scope2.childItems().front()->hasActiveFocus(), focus);
            QCOMPARE(counter, 1);
            counter++;
        });

        // A guard is needed so that connections are destroyed before the items.
        // Otherwise the slots will be called during destruction, and test will
        // crash (because childItems will be empty).
        auto guard = qScopeGuard([&scope1, &scope2]() {
            scope1.disconnect();
            scope2.disconnect();
        });
        Q_UNUSED(guard)

        scope2Child.forceActiveFocus();
        QTRY_VERIFY(scope2.hasActiveFocus());
        QCOMPARE(counter, 2); // make sure that both signals are received
    }

    {
        // Two Items inside a Window (no explicict FocusScopes)
        QQuickWindow window;
        QVERIFY(ensureFocus(&window));
        QTRY_COMPARE(QGuiApplication::focusWindow(), &window);

        QQuickItem item1(window.contentItem());

        QQuickItem item2(window.contentItem());

        item1.forceActiveFocus();
        QTRY_VERIFY(item1.hasActiveFocus());

        int counter = 0;
        connect(&item1, &QQuickItem::activeFocusChanged, [&counter](bool focus) {
            QVERIFY(!focus);
            QCOMPARE(counter, 0);
            counter++;
        });
        connect(&item2, &QQuickItem::activeFocusChanged, [&counter](bool focus) {
            QVERIFY(focus);
            QCOMPARE(counter, 1);
            counter++;
        });

        // A guard is needed so that connections are destroyed before the items.
        // Otherwise the slots will be called during destruction, and test will
        // fail.
        auto guard = qScopeGuard([&item1, &item2]() {
            item1.disconnect();
            item2.disconnect();
        });
        Q_UNUSED(guard)

        item2.forceActiveFocus();
        QTRY_VERIFY(item2.hasActiveFocus());
        QCOMPARE(counter, 2); // make sure that both signals are received
    }
}

void tst_qquickitem::constructor()
{
    QScopedPointer<QQuickItem> root(new QQuickItem);
    QVERIFY(!root->parent());
    QVERIFY(!root->parentItem());

    QQuickItem *child1 = new QQuickItem(root.data());
    QCOMPARE(child1->parent(), root.data());
    QCOMPARE(child1->parentItem(), root.data());
    QCOMPARE(root->childItems().size(), 1);
    QCOMPARE(root->childItems().at(0), child1);

    QQuickItem *child2 = new QQuickItem(root.data());
    QCOMPARE(child2->parent(), root.data());
    QCOMPARE(child2->parentItem(), root.data());
    QCOMPARE(root->childItems().size(), 2);
    QCOMPARE(root->childItems().at(0), child1);
    QCOMPARE(root->childItems().at(1), child2);
}

void tst_qquickitem::setParentItem()
{
    QQuickItem *root = new QQuickItem;
    QVERIFY(!root->parent());
    QVERIFY(!root->parentItem());

    QQuickItem *child1 = new QQuickItem;
    QVERIFY(!child1->parent());
    QVERIFY(!child1->parentItem());

    child1->setParentItem(root);
    QVERIFY(!child1->parent());
    QCOMPARE(child1->parentItem(), root);
    QCOMPARE(root->childItems().size(), 1);
    QCOMPARE(root->childItems().at(0), child1);

    QQuickItem *child2 = new QQuickItem;
    QVERIFY(!child2->parent());
    QVERIFY(!child2->parentItem());
    child2->setParentItem(root);
    QVERIFY(!child2->parent());
    QCOMPARE(child2->parentItem(), root);
    QCOMPARE(root->childItems().size(), 2);
    QCOMPARE(root->childItems().at(0), child1);
    QCOMPARE(root->childItems().at(1), child2);

    child1->setParentItem(nullptr);
    QVERIFY(!child1->parent());
    QVERIFY(!child1->parentItem());
    QCOMPARE(root->childItems().size(), 1);
    QCOMPARE(root->childItems().at(0), child2);

    delete root;

    QVERIFY(!child1->parent());
    QVERIFY(!child1->parentItem());
    QVERIFY(!child2->parent());
    QVERIFY(!child2->parentItem());

    delete child1;
    delete child2;
}

void tst_qquickitem::visible()
{
    QQuickWindow window;
    QQuickItem *root = new QQuickItem;
    root->setParentItem(window.contentItem());

    QQuickItem *child1 = new QQuickItem;
    child1->setParentItem(root);

    QQuickItem *child2 = new QQuickItem;
    child2->setParentItem(root);

    QVERIFY(child1->isVisible());
    QVERIFY(child2->isVisible());

    root->setVisible(false);
    QVERIFY(!child1->isVisible());
    QVERIFY(!child2->isVisible());

    root->setVisible(true);
    QVERIFY(child1->isVisible());
    QVERIFY(child2->isVisible());

    child1->setVisible(false);
    QVERIFY(!child1->isVisible());
    QVERIFY(child2->isVisible());

    child2->setParentItem(child1);
    QVERIFY(!child1->isVisible());
    QVERIFY(!child2->isVisible());

    child2->setParentItem(root);
    QVERIFY(!child1->isVisible());
    QVERIFY(child2->isVisible());

    delete root;
    delete child1;
    delete child2;
}

void tst_qquickitem::enabled()
{
    QQuickItem *root = new QQuickItem;

    QQuickItem *child1 = new QQuickItem;
    child1->setParentItem(root);

    QQuickItem *child2 = new QQuickItem;
    child2->setParentItem(root);

    QVERIFY(child1->isEnabled());
    QVERIFY(child2->isEnabled());

    root->setEnabled(false);
    QVERIFY(!child1->isEnabled());
    QVERIFY(!child2->isEnabled());

    root->setEnabled(true);
    QVERIFY(child1->isEnabled());
    QVERIFY(child2->isEnabled());

    child1->setEnabled(false);
    QVERIFY(!child1->isEnabled());
    QVERIFY(child2->isEnabled());

    child2->setParentItem(child1);
    QVERIFY(!child1->isEnabled());
    QVERIFY(!child2->isEnabled());

    child2->setParentItem(root);
    QVERIFY(!child1->isEnabled());
    QVERIFY(child2->isEnabled());

    delete root;
    delete child1;
    delete child2;
}

void tst_qquickitem::enabledFocus()
{
    QQuickWindow window;
    QVERIFY(ensureFocus(&window));

    QQuickFocusScope root;

    root.setFocus(true);
    root.setEnabled(false);

    QCOMPARE(root.isEnabled(), false);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), false);

    root.setParentItem(window.contentItem());

    QCOMPARE(root.isEnabled(), false);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), false);
    QCOMPARE(window.activeFocusItem(), window.contentItem());

    root.setEnabled(true);
    QCOMPARE(root.isEnabled(), true);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), true);
    QCOMPARE(window.activeFocusItem(), static_cast<QQuickItem *>(&root));

    QQuickItem child1;
    child1.setParentItem(&root);

    QCOMPARE(child1.isEnabled(), true);
    QCOMPARE(child1.hasFocus(), false);
    QCOMPARE(child1.hasActiveFocus(), false);
    QCOMPARE(window.activeFocusItem(), static_cast<QQuickItem *>(&root));

    QQuickItem child2;
    child2.setFocus(true);
    child2.setParentItem(&root);

    QCOMPARE(root.isEnabled(), true);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), true);
    QCOMPARE(child2.isEnabled(), true);
    QCOMPARE(child2.hasFocus(), true);
    QCOMPARE(child2.hasActiveFocus(), true);
    QCOMPARE(window.activeFocusItem(), &child2);

    child2.setEnabled(false);

    QCOMPARE(root.isEnabled(), true);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), true);
    QCOMPARE(child1.isEnabled(), true);
    QCOMPARE(child1.hasFocus(), false);
    QCOMPARE(child1.hasActiveFocus(), false);
    QCOMPARE(child2.isEnabled(), false);
    QCOMPARE(child2.hasFocus(), true);
    QCOMPARE(child2.hasActiveFocus(), false);
    QCOMPARE(window.activeFocusItem(), static_cast<QQuickItem *>(&root));

    child1.setEnabled(false);
    QCOMPARE(child1.isEnabled(), false);
    QCOMPARE(child1.hasFocus(), false);
    QCOMPARE(child1.hasActiveFocus(), false);

    child1.setFocus(true);
    QCOMPARE(child1.isEnabled(), false);
    QCOMPARE(child1.hasFocus(), true);
    QCOMPARE(child1.hasActiveFocus(), false);
    QCOMPARE(child2.isEnabled(), false);
    QCOMPARE(child2.hasFocus(), false);
    QCOMPARE(child2.hasActiveFocus(), false);
    QCOMPARE(window.activeFocusItem(), static_cast<QQuickItem *>(&root));

    child1.setEnabled(true);
    QCOMPARE(child1.isEnabled(), true);
    QCOMPARE(child1.hasFocus(), true);
    QCOMPARE(child1.hasActiveFocus(), true);
    QCOMPARE(window.activeFocusItem(), static_cast<QQuickItem *>(&child1));

    root.setFocus(false);
    QCOMPARE(root.isEnabled(), true);
    QCOMPARE(root.hasFocus(), false);
    QCOMPARE(root.hasActiveFocus(), false);
    QCOMPARE(child1.isEnabled(), true);
    QCOMPARE(child1.hasFocus(), true);
    QCOMPARE(child1.hasActiveFocus(), false);
    QCOMPARE(window.activeFocusItem(), window.contentItem());

    child2.forceActiveFocus();
    QCOMPARE(root.isEnabled(), true);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), true);
    QCOMPARE(child1.isEnabled(), true);
    QCOMPARE(child1.hasFocus(), false);
    QCOMPARE(child1.hasActiveFocus(), false);
    QCOMPARE(child2.isEnabled(), false);
    QCOMPARE(child2.hasFocus(), true);
    QCOMPARE(child2.hasActiveFocus(), false);
    QCOMPARE(window.activeFocusItem(), static_cast<QQuickItem *>(&root));

    root.setEnabled(false);
    QCOMPARE(root.isEnabled(), false);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), false);
    QCOMPARE(child1.isEnabled(), false);
    QCOMPARE(child1.hasFocus(), false);
    QCOMPARE(child1.hasActiveFocus(), false);
    QCOMPARE(child2.isEnabled(), false);
    QCOMPARE(child2.hasFocus(), true);
    QCOMPARE(child2.hasActiveFocus(), false);
    QCOMPARE(window.activeFocusItem(), window.contentItem());

    child1.forceActiveFocus();
    QCOMPARE(root.isEnabled(), false);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), false);
    QCOMPARE(child1.isEnabled(), false);
    QCOMPARE(child1.hasFocus(), true);
    QCOMPARE(child1.hasActiveFocus(), false);
    QCOMPARE(child2.isEnabled(), false);
    QCOMPARE(child2.hasFocus(), false);
    QCOMPARE(child2.hasActiveFocus(), false);
    QCOMPARE(window.activeFocusItem(), window.contentItem());

    root.setEnabled(true);
    QCOMPARE(root.isEnabled(), true);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), true);
    QCOMPARE(child1.isEnabled(), true);
    QCOMPARE(child1.hasFocus(), true);
    QCOMPARE(child1.hasActiveFocus(), true);
    QCOMPARE(child2.isEnabled(), false);
    QCOMPARE(child2.hasFocus(), false);
    QCOMPARE(child2.hasActiveFocus(), false);
    QCOMPARE(window.activeFocusItem(), static_cast<QQuickItem *>(&child1));

    child2.setFocus(true);
    QCOMPARE(root.isEnabled(), true);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), true);
    QCOMPARE(child1.isEnabled(), true);
    QCOMPARE(child1.hasFocus(), false);
    QCOMPARE(child1.hasActiveFocus(), false);
    QCOMPARE(child2.isEnabled(), false);
    QCOMPARE(child2.hasFocus(), true);
    QCOMPARE(child2.hasActiveFocus(), false);
    QCOMPARE(window.activeFocusItem(), static_cast<QQuickItem *>(&root));

    root.setEnabled(false);
    QCOMPARE(root.isEnabled(), false);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), false);
    QCOMPARE(child1.isEnabled(), false);
    QCOMPARE(child1.hasFocus(), false);
    QCOMPARE(child1.hasActiveFocus(), false);
    QCOMPARE(child2.isEnabled(), false);
    QCOMPARE(child2.hasFocus(), true);
    QCOMPARE(child2.hasActiveFocus(), false);
    QCOMPARE(window.activeFocusItem(), window.contentItem());
}

void tst_qquickitem::mouseGrab()
{
    QQuickWindow window;
    window.resize(200, 200);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto devPriv = QPointingDevicePrivate::get(QPointingDevice::primaryPointingDevice());

    auto child1 = new TestItem(window.contentItem());
    child1->setObjectName(QStringLiteral("child1"));
    child1->setAcceptedMouseButtons(Qt::LeftButton);
    child1->setSize(QSizeF(200, 100));

    auto child2 = new TestItem(window.contentItem());
    child2->setObjectName(QStringLiteral("child2"));
    child2->setAcceptedMouseButtons(Qt::LeftButton);
    child2->setY(51);
    child2->setSize(QSizeF(200, 100));

    // click over child1
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50,50));
    QTRY_COMPARE(devPriv->firstPointExclusiveGrabber(), child1);
    QCOMPARE(child1->pressCount, 1);
    QCOMPARE(child2->pressCount, 0);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50,50));
    QCOMPARE(devPriv->firstPointExclusiveGrabber(), nullptr);
    QCOMPARE(child1->releaseCount, 1);
    QCOMPARE(child2->releaseCount, 0);

    // press over child1 and then disable it: it doesn't get the release
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50,50));
    QTRY_COMPARE(devPriv->firstPointExclusiveGrabber(), child1);
    QCOMPARE(child1->pressCount, 2);
    QCOMPARE(child2->pressCount, 0);
    child1->setEnabled(false);
    QCOMPARE(devPriv->firstPointExclusiveGrabber(), nullptr);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50,50));
    QCOMPARE(child1->releaseCount, 1);
    QCOMPARE(child2->releaseCount, 0);
    child1->setEnabled(true);

    // press over child1 and then hide it: it doesn't get the release
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50,50));
    QTRY_COMPARE(devPriv->firstPointExclusiveGrabber(), child1);
    QCOMPARE(child1->pressCount, 3);
    QCOMPARE(child2->pressCount, 0);
    child1->setVisible(false);
    QCOMPARE(devPriv->firstPointExclusiveGrabber(), nullptr);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50,50));
    QCOMPARE(child1->releaseCount, 1);
    QCOMPARE(child2->releaseCount, 0);
    child1->setVisible(true);

    // click in a position over both children: only the top one gets it,
    // because the event is pre-accepted, which implies that the event
    // stops propagating and the top item gets the mouse grab.
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(75,75));
    QCOMPARE(child1->pressCount, 3);
    QCOMPARE(child2->pressCount, 1);
    QCOMPARE(devPriv->firstPointExclusiveGrabber(), child2);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(75,75));
    QCOMPARE(devPriv->firstPointExclusiveGrabber(), nullptr);
    QCOMPARE(child1->releaseCount, 1);
    QCOMPARE(child2->releaseCount, 1);
}

void tst_qquickitem::mousePropagationToParent()
{
    QQuickWindow window;
    window.resize(200, 200);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto devPriv = QPointingDevicePrivate::get(QPointingDevice::primaryPointingDevice());

    auto child1 = new TestItem(window.contentItem());
    child1->setObjectName(QStringLiteral("child1"));
    child1->setAcceptedMouseButtons(Qt::LeftButton);
    child1->setSize(QSizeF(200, 100));

    auto child2 = new TestItem(child1);
    child2->setObjectName(QStringLiteral("child2"));
    child2->setAcceptedMouseButtons(Qt::LeftButton);
    child2->setSize(QSizeF(200, 100));

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50,50));
    // The event is pre-accepted; child2 gets it first because it's on top,
    // and it does NOT call QPointerEvent::accept(), but by tradition,
    // the event stops there anyway: child1 does not see it.
    QTRY_COMPARE(child2->pressCount, 1);
    QCOMPARE(child1->pressCount, 0);
    // child1 also does not explicitly grab, but the grab happens because the event is accepted.
    QCOMPARE(devPriv->firstPointExclusiveGrabber(), child2);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50,50));
    QTRY_COMPARE(child2->releaseCount, 1);
    QTRY_COMPARE(child1->releaseCount, 0);
}

void tst_qquickitem::touchEventAcceptIgnore_data()
{
    QTest::addColumn<bool>("itemAcceptsTouch");
    QTest::addColumn<bool>("itemAcceptsTouchEvents");

    QTest::newRow("accepts touch, accepts events") << true << true;
    QTest::newRow("accepts touch, ignores events") << true << false;
    QTest::newRow("doesn't accept touch, gets no events") << false << false;
}

void tst_qquickitem::touchEventAcceptIgnore()
{
    QFETCH(bool, itemAcceptsTouch);
    QFETCH(bool, itemAcceptsTouchEvents);

    TestWindow window;
    window.resize(100, 100);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QScopedPointer<TestItem> item(new TestItem);
    item->setSize(QSizeF(100, 100));
    item->setParentItem(window.contentItem());
    if (itemAcceptsTouch)
        item->setAcceptTouchEvents(itemAcceptsTouch); // it's false by default in Qt 6
    item->acceptIncomingTouchEvents = itemAcceptsTouchEvents;

    static QPointingDevice* device = QTest::createTouchDevice();

    // Send Begin, Update & End touch sequence
    item->touchEventReached = false;
    QTest::touchEvent(&window, device).press(1, QPoint(50, 50), &window);
    QQuickTouchUtils::flush(&window);
    QTRY_COMPARE(item->touchEventReached, itemAcceptsTouch);

    item->touchEventReached = false;
    QTest::touchEvent(&window, device).move(1, QPoint(60, 60), &window);
    QQuickTouchUtils::flush(&window);
    QTRY_COMPARE(item->touchEventReached, itemAcceptsTouchEvents);

    item->touchEventReached = false;
    QTest::touchEvent(&window, device).release(1, QPoint(60, 60), &window);
    QQuickTouchUtils::flush(&window);
    QTRY_COMPARE(item->touchEventReached, itemAcceptsTouchEvents);
}

void tst_qquickitem::polishOutsideAnimation()
{
    QQuickWindow *window = new QQuickWindow;
    window->resize(200, 200);
    window->show();

    TestPolishItem *item = new TestPolishItem(window->contentItem());
    item->setSize(QSizeF(200, 100));
    QTest::qWait(50);

    QTimer::singleShot(10, item, SLOT(doPolish()));
    QTRY_VERIFY(item->wasPolished);

    delete item;
    delete window;
}

void tst_qquickitem::polishOnCompleted()
{
    QQuickView view;
    view.setSource(testFileUrl("polishOnCompleted.qml"));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    TestPolishItem *item = qobject_cast<TestPolishItem*>(view.rootObject());
    QVERIFY(item);

    QTRY_VERIFY(item->wasPolished);
}

struct PolishItemSpan {
    int itemCount;      // Number of items...
    int repolishCount;  // ...repolishing 'repolishCount' times
};

/*
 * For instance, two consecutive spans {99,0} and {1,2000} } instructs to
 * construct 99 items with no repolish, and 1 item with 2000 repolishes (in that sibling order)
 */
typedef QVector<PolishItemSpan> PolishItemSpans;

Q_DECLARE_METATYPE(PolishItemSpan)
Q_DECLARE_METATYPE(PolishItemSpans)

void tst_qquickitem::polishLoopDetection_data()
{
    QTest::addColumn<PolishItemSpans>("listOfItemsToPolish");
    QTest::addColumn<int>("expectedNumberOfWarnings");

    QTest::newRow("test1.100") <<   PolishItemSpans({ {1, 100} }) << 0;
    QTest::newRow("test1.1002") <<  PolishItemSpans({ {1, 1002} }) << 3;
    QTest::newRow("test1.2020") <<  PolishItemSpans({ {1, 2020} }) << 10;

    QTest::newRow("test5.1") <<    PolishItemSpans({ {5, 1} }) << 0;
    QTest::newRow("test5.10") <<   PolishItemSpans({ {5, 10} }) << 0;
    QTest::newRow("test5.100") <<  PolishItemSpans({ {5, 100} }) << 0;
    QTest::newRow("test5.1000") << PolishItemSpans({ {5, 1000} }) << 5;

    QTest::newRow("test1000.1") <<  PolishItemSpans({ {1000,1} }) << 0;
    QTest::newRow("test2000.1") <<  PolishItemSpans({ {2000,1} }) << 0;

    QTest::newRow("test99.0-1.1100") << PolishItemSpans({ {99,0},{1,1100} }) << 5;
    QTest::newRow("test98.0-2.1100") << PolishItemSpans({ {98,0},{2,1100} }) << 5+5;

    // reverse the two above
    QTest::newRow("test1.1100-99.0") << PolishItemSpans({ {1,1100},{99,0} }) << 5;
    QTest::newRow("test2.1100-98.0") << PolishItemSpans({ {2,1100},{98,0} }) << 5+5;
}

void tst_qquickitem::polishLoopDetection()
{
    QFETCH(PolishItemSpans, listOfItemsToPolish);
    QFETCH(int, expectedNumberOfWarnings);

    QQuickWindow window;
    window.resize(200, 200);
    window.show();

    TestPolishItem *item = nullptr;
    int count = 0;
    for (PolishItemSpan s : listOfItemsToPolish) {
        for (int i = 0; i < s.itemCount; ++i) {
            item = new TestPolishItem(window.contentItem());
            item->setSize(QSizeF(200, 100));
            item->repolishLoopCount = s.repolishCount;
            item->setObjectName(QString::fromLatin1("obj%1").arg(count++));
        }
    }

    for (int i = 0; i < expectedNumberOfWarnings; ++i) {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*possible QQuickItem..polish.. loop.*"));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*TestPolishItem.* called polish.. inside updatePolish.. of TestPolishItem.*"));
    }

    QList<QQuickItem*> items = window.contentItem()->childItems();
    for (int i = 0; i < items.size(); ++i) {
        static_cast<TestPolishItem*>(items.at(i))->doPolish();
    }
    item = static_cast<TestPolishItem*>(items.first());
    // item is the last item, so we wait until the last item reached 0
    QVERIFY(QTest::qWaitFor([=](){return item->repolishLoopCount == 0 && item->wasPolished;}));
}

void tst_qquickitem::wheelEvent_data()
{
    QTest::addColumn<bool>("visible");
    QTest::addColumn<bool>("enabled");

    QTest::newRow("visible and enabled") << true << true;
    QTest::newRow("visible and disabled") << true << false;
    QTest::newRow("invisible and enabled") << false << true;
    QTest::newRow("invisible and disabled") << false << false;
}

void tst_qquickitem::wheelEvent()
{
    QFETCH(bool, visible);
    QFETCH(bool, enabled);

    const bool shouldReceiveWheelEvents = visible && enabled;

    const int width = 200;
    const int height = 200;

    QQuickWindow window;
    window.resize(width, height);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    TestItem *item = new TestItem;
    item->setSize(QSizeF(width, height));
    item->setParentItem(window.contentItem());

    item->setEnabled(enabled);
    item->setVisible(visible);

    QPoint localPoint(width / 2, height / 2);
    QPoint globalPoint = window.mapToGlobal(localPoint);
    QWheelEvent event(localPoint, globalPoint, QPoint(0, 0), QPoint(0, -120),
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    event.setTimestamp(123456UL);
    event.setAccepted(false);
    QGuiApplication::sendEvent(&window, &event);

    if (shouldReceiveWheelEvents) {
        QVERIFY(event.isAccepted());
        QCOMPARE(item->wheelCount, 1);
        QCOMPARE(item->timestamp, 123456UL);
        QCOMPARE(item->lastWheelEventPos, localPoint);
        QCOMPARE(item->lastWheelEventGlobalPos, globalPoint);
    } else {
        QVERIFY(!event.isAccepted());
        QCOMPARE(item->wheelCount, 0);
    }
}

class HoverItem : public QQuickItem
{
Q_OBJECT
public:
    HoverItem(QQuickItem *parent = nullptr)
        : QQuickItem(parent), hoverEnterCount(0), hoverMoveCount(0), hoverLeaveCount(0)
    { }
    void resetCounters() {
        hoverEnterCount = 0;
        hoverMoveCount = 0;
        hoverLeaveCount = 0;
    }
    int hoverEnterCount;
    int hoverMoveCount;
    int hoverLeaveCount;
    QPoint hoverPosition;
    QPoint hoverScenePosition;
    QPoint hoverGlobalPosition;
    QPoint hoverLastGlobalPosition;
protected:
    void hoverEnterEvent(QHoverEvent *event) override {
        qCDebug(lcTests) << static_cast<QSinglePointEvent *>(event) << event->position() << event->scenePosition() << event->globalPosition();
        event->accept();
        ++hoverEnterCount;
        hoverPosition = event->position().toPoint();
        hoverScenePosition = event->scenePosition().toPoint();
        hoverGlobalPosition = event->globalPosition().toPoint();
        hoverLastGlobalPosition = event->points().first().globalLastPosition().toPoint();
    }
    void hoverMoveEvent(QHoverEvent *event) override {
        qCDebug(lcTests) << static_cast<QSinglePointEvent *>(event) << event->position() << event->scenePosition() << event->globalPosition();
        event->accept();
        ++hoverMoveCount;
        hoverPosition = event->position().toPoint();
        hoverScenePosition = event->scenePosition().toPoint();
        hoverGlobalPosition = event->globalPosition().toPoint();
        hoverLastGlobalPosition = event->points().first().globalLastPosition().toPoint();
    }
    void hoverLeaveEvent(QHoverEvent *event) override {
        qCDebug(lcTests) << static_cast<QSinglePointEvent *>(event) << event->position() << event->scenePosition() << event->globalPosition();
        event->accept();
        ++hoverLeaveCount;
        hoverPosition = event->position().toPoint();
        hoverScenePosition = event->scenePosition().toPoint();
        hoverGlobalPosition = event->globalPosition().toPoint();
        hoverLastGlobalPosition = event->points().first().globalLastPosition().toPoint();
    }
};

void tst_qquickitem::hoverEvent_data()
{
    QTest::addColumn<bool>("visible");
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<bool>("acceptHoverEvents");

    QTest::newRow("visible, enabled, accept hover") << true << true << true;
    QTest::newRow("visible, disabled, accept hover") << true << false << true;
    QTest::newRow("invisible, enabled, accept hover") << false << true << true;
    QTest::newRow("invisible, disabled, accept hover") << false << false << true;

    QTest::newRow("visible, enabled, not accept hover") << true << true << false;
    QTest::newRow("visible, disabled, not accept hover") << true << false << false;
    QTest::newRow("invisible, enabled, not accept hover") << false << true << false;
    QTest::newRow("invisible, disabled, not accept hover") << false << false << false;
}

void tst_qquickitem::hoverEvent()
{
    QFETCH(bool, visible);
    QFETCH(bool, enabled);
    QFETCH(bool, acceptHoverEvents);

    QQuickWindow *window = new QQuickWindow();
    window->resize(200, 200);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
#if QT_CONFIG(cursor) // Get the cursor out of the way.
    QCursor::setPos(window->geometry().topRight() + QPoint(100, 100));
#endif

    HoverItem *item = new HoverItem;
    item->setSize(QSizeF(100, 100));
    item->setParentItem(window->contentItem());

    item->setEnabled(enabled);
    item->setVisible(visible);
    item->setAcceptHoverEvents(acceptHoverEvents);

    // Ensure that we don't get extra hover events delivered on the
    // side, since it can affect the number of hover move events we receive below.
    QQuickWindowPrivate::get(window)->deliveryAgentPrivate()->frameSynchronousHoverEnabled = false;
    // And flush out any mouse events that might be queued up
    // in QPA, since QTest::mouseMove() calls processEvents.
    qGuiApp->processEvents();

    const QPoint outside(150, 150);
    const QPoint inside(50, 50);
    const QPoint anotherInside(51, 51);

    QTest::mouseMove(window, outside);
    item->resetCounters();

    auto checkPositions = [=](QPoint pt) {
        QCOMPARE(item->hoverPosition, pt);
        QCOMPARE(item->hoverScenePosition, item->mapToScene(pt).toPoint());
        QCOMPARE(item->hoverGlobalPosition, item->mapToGlobal(pt).toPoint());
        QVERIFY(!item->hoverLastGlobalPosition.isNull());
    };

    // Enter, then move twice inside, then leave.
    const bool shouldReceiveHoverEvents = visible && acceptHoverEvents;
    QTest::mouseMove(window, inside);
    if (shouldReceiveHoverEvents)
        checkPositions(inside);
    QTest::mouseMove(window, anotherInside);
    if (shouldReceiveHoverEvents)
        checkPositions(anotherInside);
    QTest::mouseMove(window, inside);
    if (shouldReceiveHoverEvents)
        checkPositions(inside);
    QTest::mouseMove(window, outside);
    if (shouldReceiveHoverEvents)
        checkPositions(outside);

    if (shouldReceiveHoverEvents) {
        QCOMPARE(item->hoverEnterCount, 1);
        QVERIFY(item->hoverMoveCount >= 2);
        if (item->hoverMoveCount > 2)
            qCDebug(lcTests) << "expected 2 hover move events, but got" << item->hoverMoveCount;
        QCOMPARE(item->hoverLeaveCount, 1);
    } else {
        QCOMPARE(item->hoverEnterCount, 0);
        QCOMPARE(item->hoverMoveCount, 0);
        QCOMPARE(item->hoverLeaveCount, 0);
    }

    delete window;
}

void tst_qquickitem::hoverEventInParent()
{
    QQuickWindow window;
    window.resize(200, 200);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
#if QT_CONFIG(cursor) // Get the cursor out of the way.
    QCursor::setPos(window.geometry().topRight() + QPoint(100, 100));
#endif

    HoverItem *parentItem = new HoverItem(window.contentItem());
    parentItem->setSize(QSizeF(200, 200));
    parentItem->setAcceptHoverEvents(true);

    HoverItem *leftItem = new HoverItem(parentItem);
    leftItem->setSize(QSizeF(100, 200));
    leftItem->setAcceptHoverEvents(true);

    HoverItem *rightItem = new HoverItem(parentItem);
    rightItem->setSize(QSizeF(100, 200));
    rightItem->setPosition(QPointF(100, 0));
    rightItem->setAcceptHoverEvents(true);

    const QPoint insideLeft(50, 100);
    const QPoint insideRight(150, 100);

    QTest::mouseMove(&window, insideLeft);
    parentItem->resetCounters();
    leftItem->resetCounters();
    rightItem->resetCounters();

    QTest::mouseMove(&window, insideRight);
    QCOMPARE(parentItem->hoverEnterCount, 0);
    QCOMPARE(parentItem->hoverLeaveCount, 0);
    QCOMPARE(leftItem->hoverEnterCount, 0);
    QCOMPARE(leftItem->hoverLeaveCount, 1);
    QCOMPARE(rightItem->hoverEnterCount, 1);
    QCOMPARE(rightItem->hoverLeaveCount, 0);

    QTest::mouseMove(&window, insideLeft);
    QCOMPARE(parentItem->hoverEnterCount, 0);
    QCOMPARE(parentItem->hoverLeaveCount, 0);
    QCOMPARE(leftItem->hoverEnterCount, 1);
    QCOMPARE(leftItem->hoverLeaveCount, 1);
    QCOMPARE(rightItem->hoverEnterCount, 1);
    QCOMPARE(rightItem->hoverLeaveCount, 1);
}

void tst_qquickitem::paintOrder_data()
{
    const QUrl order1Url = testFileUrl("order.1.qml");
    const QUrl order2Url = testFileUrl("order.2.qml");

    QTest::addColumn<QUrl>("source");
    QTest::addColumn<int>("op");
    QTest::addColumn<QVariant>("param1");
    QTest::addColumn<QVariant>("param2");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("test 1 noop") << order1Url
        << int(NoOp) << QVariant() << QVariant()
        << (QStringList() << "1" << "2" << "3");
    QTest::newRow("test 1 add") << order1Url
        << int(Append) << QVariant("new") << QVariant()
        << (QStringList() << "1" << "2" << "3" << "new");
    QTest::newRow("test 1 remove") << order1Url
        << int(Remove) << QVariant(1) << QVariant()
        << (QStringList() << "1" << "3");
    QTest::newRow("test 1 stack before") << order1Url
        << int(StackBefore) << QVariant(2) << QVariant(1)
        << (QStringList() << "1" << "3" << "2");
    QTest::newRow("test 1 stack after") << order1Url
        << int(StackAfter) << QVariant(0) << QVariant(1)
        << (QStringList() << "2" << "1" << "3");
    QTest::newRow("test 1 set z") << order1Url
        << int(SetZ) << QVariant(1) << QVariant(qreal(1.))
        << (QStringList() << "1" << "3" << "2");

    QTest::newRow("test 2 noop") << order2Url
        << int(NoOp) << QVariant() << QVariant()
        << (QStringList() << "1" << "3" << "2");
    QTest::newRow("test 2 add") << order2Url
        << int(Append) << QVariant("new") << QVariant()
        << (QStringList() << "1" << "3" << "new" << "2");
    QTest::newRow("test 2 remove 1") << order2Url
        << int(Remove) << QVariant(1) << QVariant()
        << (QStringList() << "1" << "3");
    QTest::newRow("test 2 remove 2") << order2Url
        << int(Remove) << QVariant(2) << QVariant()
        << (QStringList() << "1" << "2");
    QTest::newRow("test 2 stack before 1") << order2Url
        << int(StackBefore) << QVariant(1) << QVariant(0)
        << (QStringList() << "1" << "3" << "2");
    QTest::newRow("test 2 stack before 2") << order2Url
        << int(StackBefore) << QVariant(2) << QVariant(0)
        << (QStringList() << "3" << "1" << "2");
    QTest::newRow("test 2 stack after 1") << order2Url
        << int(StackAfter) << QVariant(0) << QVariant(1)
        << (QStringList() << "1" << "3" << "2");
    QTest::newRow("test 2 stack after 2") << order2Url
        << int(StackAfter) << QVariant(0) << QVariant(2)
        << (QStringList() << "3" << "1" << "2");
    QTest::newRow("test 1 set z") << order1Url
        << int(SetZ) << QVariant(2) << QVariant(qreal(2.))
        << (QStringList() << "1" << "2" << "3");
}

void tst_qquickitem::paintOrder()
{
    QFETCH(QUrl, source);
    QFETCH(int, op);
    QFETCH(QVariant, param1);
    QFETCH(QVariant, param2);
    QFETCH(QStringList, expected);

    QQuickView view;
    view.setSource(source);

    QQuickItem *root = qobject_cast<QQuickItem*>(view.rootObject());
    QVERIFY(root);

    switch (op) {
        case Append: {
                QQuickItem *item = new QQuickItem(root);
                item->setObjectName(param1.toString());
            }
            break;
        case Remove: {
                QQuickItem *item = root->childItems().at(param1.toInt());
                delete item;
            }
            break;
        case StackBefore: {
                QQuickItem *item1 = root->childItems().at(param1.toInt());
                QQuickItem *item2 = root->childItems().at(param2.toInt());
                item1->stackBefore(item2);
            }
            break;
        case StackAfter: {
                QQuickItem *item1 = root->childItems().at(param1.toInt());
                QQuickItem *item2 = root->childItems().at(param2.toInt());
                item1->stackAfter(item2);
            }
            break;
        case SetZ: {
                QQuickItem *item = root->childItems().at(param1.toInt());
                item->setZ(param2.toReal());
            }
            break;
        default:
            break;
    }

    QList<QQuickItem*> list = QQuickItemPrivate::get(root)->paintOrderChildItems();

    QStringList items;
    for (int i = 0; i < list.size(); ++i)
        items << list.at(i)->objectName();

    QCOMPARE(items, expected);
}

void tst_qquickitem::acceptedMouseButtons()
{
    TestItem item;
    QCOMPARE(item.acceptedMouseButtons(), Qt::MouseButtons(Qt::NoButton));

    QQuickWindow window;
    item.setSize(QSizeF(200,100));
    item.setParentItem(window.contentItem());

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QCOMPARE(item.pressCount, 0);
    QCOMPARE(item.releaseCount, 0);

    QTest::mousePress(&window, Qt::RightButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseRelease(&window, Qt::RightButton, Qt::NoModifier, QPoint(50, 50));
    QCOMPARE(item.pressCount, 0);
    QCOMPARE(item.releaseCount, 0);

    QTest::mousePress(&window, Qt::MiddleButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseRelease(&window, Qt::MiddleButton, Qt::NoModifier, QPoint(50, 50));
    QCOMPARE(item.pressCount, 0);
    QCOMPARE(item.releaseCount, 0);

    item.setAcceptedMouseButtons(Qt::LeftButton);
    QCOMPARE(item.acceptedMouseButtons(), Qt::MouseButtons(Qt::LeftButton));

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QCOMPARE(item.pressCount, 1);
    QCOMPARE(item.releaseCount, 1);

    QTest::mousePress(&window, Qt::RightButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseRelease(&window, Qt::RightButton, Qt::NoModifier, QPoint(50, 50));
    QCOMPARE(item.pressCount, 1);
    QCOMPARE(item.releaseCount, 1);

    QTest::mousePress(&window, Qt::MiddleButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseRelease(&window, Qt::MiddleButton, Qt::NoModifier, QPoint(50, 50));
    QCOMPARE(item.pressCount, 1);
    QCOMPARE(item.releaseCount, 1);

    item.setAcceptedMouseButtons(Qt::RightButton | Qt::MiddleButton);
    QCOMPARE(item.acceptedMouseButtons(), Qt::MouseButtons(Qt::RightButton | Qt::MiddleButton));

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QCOMPARE(item.pressCount, 1);
    QCOMPARE(item.releaseCount, 1);

    QTest::mousePress(&window, Qt::RightButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseRelease(&window, Qt::RightButton, Qt::NoModifier, QPoint(50, 50));
    QCOMPARE(item.pressCount, 2);
    QCOMPARE(item.releaseCount, 2);

    QTest::mousePress(&window, Qt::MiddleButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseRelease(&window, Qt::MiddleButton, Qt::NoModifier, QPoint(50, 50));
    QCOMPARE(item.pressCount, 3);
    QCOMPARE(item.releaseCount, 3);
}

static void gc(QQmlEngine &engine)
{
    engine.collectGarbage();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

void tst_qquickitem::visualParentOwnership()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("visualParentOwnership.qml"));

    QScopedPointer<QQuickItem> root(qobject_cast<QQuickItem*>(component.create()));
    QVERIFY(root);

    QVariant newObject;
    {
        QVERIFY(QMetaObject::invokeMethod(root.data(), "createItemWithoutParent", Q_RETURN_ARG(QVariant, newObject)));
        QPointer<QQuickItem> newItem = qvariant_cast<QQuickItem*>(newObject);
        QVERIFY(!newItem.isNull());

        QVERIFY(!newItem->parent());
        QVERIFY(!newItem->parentItem());

        newItem->setParentItem(root.data());

        gc(engine);

        QVERIFY(!newItem.isNull());
        newItem->setParentItem(nullptr);

        gc(engine);
        QVERIFY(newItem.isNull());
    }
    {
        QVERIFY(QMetaObject::invokeMethod(root.data(), "createItemWithoutParent", Q_RETURN_ARG(QVariant, newObject)));
        QPointer<QQuickItem> firstItem = qvariant_cast<QQuickItem*>(newObject);
        QVERIFY(!firstItem.isNull());

        firstItem->setParentItem(root.data());

        QVERIFY(QMetaObject::invokeMethod(root.data(), "createItemWithoutParent", Q_RETURN_ARG(QVariant, newObject)));
        QPointer<QQuickItem> secondItem = qvariant_cast<QQuickItem*>(newObject);
        QVERIFY(!firstItem.isNull());

        secondItem->setParentItem(firstItem);

        gc(engine);

        delete firstItem;

        root->setProperty("keepAliveProperty", newObject);

        gc(engine);
        QVERIFY(!secondItem.isNull());

        root->setProperty("keepAliveProperty", QVariant());

        gc(engine);
        QVERIFY(secondItem.isNull());
    }
}

void tst_qquickitem::visualParentOwnershipWindow()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("visualParentOwnershipWindow.qml"));

    QQuickWindow *window = qobject_cast<QQuickWindow*>(component.create());
    QVERIFY(window);
    QQuickItem *root = window->contentItem();

    QVariant newObject;
    {
        QVERIFY(QMetaObject::invokeMethod(window, "createItemWithoutParent", Q_RETURN_ARG(QVariant, newObject)));
        QPointer<QQuickItem> newItem = qvariant_cast<QQuickItem*>(newObject);
        QVERIFY(!newItem.isNull());

        QVERIFY(!newItem->parent());
        QVERIFY(!newItem->parentItem());

        newItem->setParentItem(root);

        gc(engine);

        QVERIFY(!newItem.isNull());
        newItem->setParentItem(nullptr);

        gc(engine);
        QVERIFY(newItem.isNull());
    }
    {
        QVERIFY(QMetaObject::invokeMethod(window, "createItemWithoutParent", Q_RETURN_ARG(QVariant, newObject)));
        QPointer<QQuickItem> firstItem = qvariant_cast<QQuickItem*>(newObject);
        QVERIFY(!firstItem.isNull());

        firstItem->setParentItem(root);

        QVERIFY(QMetaObject::invokeMethod(window, "createItemWithoutParent", Q_RETURN_ARG(QVariant, newObject)));
        QPointer<QQuickItem> secondItem = qvariant_cast<QQuickItem*>(newObject);
        QVERIFY(!firstItem.isNull());

        secondItem->setParentItem(firstItem);

        gc(engine);

        delete firstItem;

        window->setProperty("keepAliveProperty", newObject);

        gc(engine);
        QVERIFY(!secondItem.isNull());

        window->setProperty("keepAliveProperty", QVariant());

        gc(engine);
        QVERIFY(secondItem.isNull());
    }
}

class InvalidatedItem : public QQuickItem {
    Q_OBJECT
signals:
    void invalidated();
public slots:
    void invalidateSceneGraph() { emit invalidated(); }
};

void tst_qquickitem::testSGInvalidate()
{
    for (int i=0; i<2; ++i) {
        std::unique_ptr<QQuickView> view(new QQuickView());

        InvalidatedItem *item = new InvalidatedItem();

        int expected = 0;
        if (i == 0) {
            // First iteration, item has contents and should get signals
            expected = 1;
            item->setFlag(QQuickItem::ItemHasContents, true);
        } else {
            // Second iteration, item does not have content and will not get signals
        }

        QSignalSpy invalidateSpy(item, SIGNAL(invalidated()));
        item->setParentItem(view->contentItem());
        view->show();

        QVERIFY(QTest::qWaitForWindowExposed(view.get()));

        view.reset();
        QCOMPARE(invalidateSpy.size(), expected);
    }
}

void tst_qquickitem::objectChildTransform()
{
    QQuickView view;
    view.setSource(testFileUrl("objectChildTransform.qml"));

    QQuickItem *root = qobject_cast<QQuickItem*>(view.rootObject());
    QVERIFY(root);

    root->setProperty("source", QString());
    // Shouldn't crash.
}

void tst_qquickitem::contains_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("y");
    QTest::addColumn<bool>("contains");

    QTest::newRow("(0, 0) = false") << 0 << 0 << false;
    QTest::newRow("(50, 0) = false") << 50 << 0 << false;
    QTest::newRow("(0, 50) = false") << 0 << 50 << false;
    QTest::newRow("(50, 50) = true") << 50 << 50 << true;
    QTest::newRow("(99, 99) = true") << 99 << 99 << true;
    QTest::newRow("(100, 100) = false") << 100 << 100 << false;
    QTest::newRow("(150, 150) = false") << 150 << 150 << false;
}

void tst_qquickitem::contains()
{
    // Tests that contains works, but also checks that mapToItem/mapFromItem
    // return the correct type (point or rect, not a JS object with those properties),
    // as this is a common combination of calls.

    QFETCH(int, x);
    QFETCH(int, y);
    QFETCH(bool, contains);

    QQuickView view;
    view.setSource(testFileUrl("contains.qml"));

    QQuickItem *root = qobject_cast<QQuickItem*>(view.rootObject());
    QVERIFY(root);

    QVariant result = false;
    QVERIFY(QMetaObject::invokeMethod(root, "childContainsViaMapToItem",
        Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, qreal(x)), Q_ARG(QVariant, qreal(y))));
    QCOMPARE(result.toBool(), contains);

    result = false;
    QVERIFY(QMetaObject::invokeMethod(root, "childContainsViaMapFromItem",
        Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, qreal(x)), Q_ARG(QVariant, qreal(y))));
    QCOMPARE(result.toBool(), contains);
}

void tst_qquickitem::containsContainmentMask_data()
{
    QTest::addColumn<QPointF>("point");
    QTest::addColumn<bool>("contains");

    QTest::newRow("(-6, -6) = false") << QPointF(-6, -6) << false;
    QTest::newRow("(-5, -5) = true") << QPointF(-5, -5) << true;
    QTest::newRow("(-4, -4) = true") << QPointF(-4, -4) << true;
    QTest::newRow("(-3, -3) = true") << QPointF(-3, -3) << true;
    QTest::newRow("(-2, -2) = true") << QPointF(-2, -2) << true;
    QTest::newRow("(-1, -1) = true") << QPointF(-1, -1) << true;
    QTest::newRow("(0, 0) = true") << QPointF(0, 0) << true;
    QTest::newRow("(1, 1) = true") << QPointF(1, 1) << true;
    QTest::newRow("(2, 2) = true") << QPointF(2, 2) << true;
    QTest::newRow("(3, 3) = true") << QPointF(3, 3) << true;
    QTest::newRow("(4, 4) = true") << QPointF(4, 4) << true;
    QTest::newRow("(5, 5) = false") << QPointF(5, 5) << false;
}

void tst_qquickitem::containsContainmentMask()
{
    QFETCH(QPointF, point);
    QFETCH(bool, contains);

    QQuickView view;
    view.setSource(testFileUrl("containsContainmentMask.qml"));

    QQuickItem *root = qobject_cast<QQuickItem*>(view.rootObject());
    QVERIFY(root);

    QQuickItem *firstItem = root->findChild<QQuickItem*>("firstItem");
    QVERIFY(firstItem);

    QQuickItem *secondItem = root->findChild<QQuickItem*>("secondItem");
    QVERIFY(secondItem);

    QCOMPARE(firstItem->contains(point), contains);
    QCOMPARE(secondItem->contains(point), contains);
}

void tst_qquickitem::childAt()
{
    QQuickView view;
    view.setSource(testFileUrl("childAtRectangle.qml"));
    QQuickItem *root = qobject_cast<QQuickItem*>(view.rootObject());

    int found = 0;
    for (int i = 0; i < 16; i++)
    {
        if (root->childAt(i, 0))
            found++;
    }
    QCOMPARE(found, 16);

    found = 0;
    for (int i = 0; i < 16; i++)
    {
        if (root->childAt(0, i))
            found++;
    }
    QCOMPARE(found, 16);

    found = 0;
    for (int i = 0; i < 2; i++)
    {
        if (root->childAt(18 + i, 0))
            found++;
    }
    QCOMPARE(found, 1);

    found = 0;
    for (int i = 0; i < 16; i++)
    {
        if (root->childAt(18, i))
            found++;
    }
    QCOMPARE(found, 1);

    QVERIFY(!root->childAt(19,19));
}

void tst_qquickitem::ignoreButtonPressNotInAcceptedMouseButtons()
{
    // Verify the fix for QTBUG-31861
    TestItem item;
    QCOMPARE(item.acceptedMouseButtons(), Qt::MouseButtons(Qt::NoButton));

    QQuickWindow window;
    item.setSize(QSizeF(200,100));
    item.setParentItem(window.contentItem());

    item.setAcceptedMouseButtons(Qt::LeftButton);
    QCOMPARE(item.acceptedMouseButtons(), Qt::MouseButtons(Qt::LeftButton));

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mousePress(&window, Qt::RightButton, Qt::NoModifier, QPoint(50, 50)); // ignored because it's not LeftButton
    QTest::mouseRelease(&window, Qt::RightButton, Qt::NoModifier, QPoint(50, 50)); // ignored because it didn't grab the RightButton press
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));

    QCOMPARE(item.pressCount, 1);
    QCOMPARE(item.releaseCount, 1);
}

void tst_qquickitem::shortcutOverride()
{
    QQuickView view;
    view.setSource(testFileUrl("shortcutOverride.qml"));
    QVERIFY(ensureFocus(&view));

    QCOMPARE(view.rootObject()->property("escapeHandlerActivationCount").toInt(), 0);
    QCOMPARE(view.rootObject()->property("shortcutActivationCount").toInt(), 0);

    QQuickItem *escapeItem = view.rootObject()->property("escapeItem").value<QQuickItem*>();
    QVERIFY(escapeItem);
    QVERIFY(escapeItem->hasActiveFocus());

    // escapeItem's onEscapePressed handler should accept the first escape press event.
    QTest::keyPress(&view, Qt::Key_Escape);
    QCOMPARE(view.rootObject()->property("escapeHandlerActivationCount").toInt(), 1);
    QCOMPARE(view.rootObject()->property("shortcutActivationCount").toInt(), 0);
    // Now it shouldn't have focus, so it can't handle the next escape press event.
    QVERIFY(!escapeItem->hasActiveFocus());

    QTest::keyRelease(&view, Qt::Key_Escape);
    QCOMPARE(view.rootObject()->property("escapeHandlerActivationCount").toInt(), 1);
    QCOMPARE(view.rootObject()->property("shortcutActivationCount").toInt(), 0);

    QTest::keyPress(&view, Qt::Key_Escape);
    QCOMPARE(view.rootObject()->property("escapeHandlerActivationCount").toInt(), 1);
    QCOMPARE(view.rootObject()->property("shortcutActivationCount").toInt(), 1);

    QTest::keyRelease(&view, Qt::Key_Escape);
    QCOMPARE(view.rootObject()->property("escapeHandlerActivationCount").toInt(), 1);
    QCOMPARE(view.rootObject()->property("shortcutActivationCount").toInt(), 1);
}

#ifdef TEST_QTBUG_60123
void tst_qquickitem::qtBug60123()
{
    QMainWindow main;
    main.resize(400, 200);

    QQuickView window;
    QQuickView window2;
    window.setSource(testFileUrl("mainWindowQtBug60123.qml"));
    window2.setSource(testFileUrl("mainWindowQtBug60123.qml"));

    // Create central widget for the main window
    QWidget *baseWidget = new QWidget(&main);
    baseWidget->resize(400, 200);
    baseWidget->setMaximumHeight(200);
    baseWidget->setMaximumWidth(400);
    main.setCentralWidget(baseWidget);

    // Create container widgets for both windows
    QWidget *containers = QWidget::createWindowContainer(&window, baseWidget);
    QWidget *containers2 = QWidget::createWindowContainer(&window2, baseWidget);
    containers->setGeometry(0, 0, 100, 100);
    containers2->setGeometry(100, 100, 100, 100);

    // Show and activate the main window
    main.show();
    QVERIFY(QTest::qWaitForWindowExposed(&main));

    // Activate window, test press and release events
    auto activateWindowAndTestPress = [] (QQuickView* testWindow) {
        testWindow->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(testWindow));
        QTest::mousePress(testWindow, Qt::LeftButton, Qt::NoModifier, QPoint(10, 10));
        QCOMPARE(testWindow->rootObject()->property("lastEvent").toString(), QString("pressed"));
        QTest::mouseRelease(testWindow, Qt::LeftButton, Qt::NoModifier, QPoint(10, 10));
        QCOMPARE(testWindow->rootObject()->property("lastEvent").toString(), QString("released"));
    };

    // First press after switching focus window resulted in cancelled event
    activateWindowAndTestPress(&window);
    activateWindowAndTestPress(&window2);
    activateWindowAndTestPress(&window);
}
#endif
void tst_qquickitem::setParentCalledInOnWindowChanged()
{
    QQuickView view;
    view.setSource(testFileUrl("setParentInWindowChange.qml"));
    QVERIFY(ensureFocus(&view)); // should not crash
}

void tst_qquickitem::receivesLanguageChangeEvent()
{
    QQuickWindow window;
    window.setFramePosition(QPoint(100, 100));
    window.resize(200, 200);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QScopedPointer<TestItem> child1(new TestItem);
    child1->setObjectName(QStringLiteral("child1"));
    child1->setSize(QSizeF(200, 100));
    child1->setParentItem(window.contentItem());

    QScopedPointer<TestItem> child2(new TestItem);
    child2->setObjectName(QStringLiteral("child2"));
    child2->setSize(QSizeF(50, 50));
    child2->setParentItem(child1.data());

    QTranslator t;
    QVERIFY(t.load("hellotr_la.qm", dataDirectory()));
    QVERIFY(QCoreApplication::installTranslator(&t));

    QTRY_COMPARE(child1->languageChangeEventCount, 1);
    QCOMPARE(child2->languageChangeEventCount, 1);
}

void tst_qquickitem::receivesLocaleChangeEvent()
{
    QQuickWindow window;
    window.setFramePosition(QPoint(100, 100));
    window.resize(200, 200);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QScopedPointer<TestItem> child1(new TestItem);
    child1->setObjectName(QStringLiteral("child1"));
    child1->setSize(QSizeF(200, 100));
    child1->setParentItem(window.contentItem());

    QScopedPointer<TestItem> child2(new TestItem);
    child2->setObjectName(QStringLiteral("child2"));
    child2->setSize(QSizeF(50, 50));
    child2->setParentItem(child1.data());

    QEvent e(QEvent::LocaleChange);
    QCoreApplication::sendEvent(&window, &e);

    QTRY_COMPARE(child1->localeChangeEventCount, 1);
    QCOMPARE(child2->localeChangeEventCount, 1);
}

void tst_qquickitem::objectCastInDestructor()
{
    QQuickView view;
    view.setSource(testFileUrl("objectCastInDestructor.qml"));
    view.show();

    QQuickItem *item = view.findChild<QQuickItem *>("testRectangle");
    QVERIFY(item);
    bool destroyed = false;
    connect(item, &QObject::destroyed, [&]{
        destroyed = true;
        QCOMPARE(qobject_cast<QQuickItem *>(item), nullptr);
        QCOMPARE(qobject_cast<QQuickRectangle *>(item), nullptr);
    });

    QQuickItem *loader = view.findChild<QQuickItem *>("loader");
    QVERIFY(loader);
    loader->setProperty("active", false);

    QVERIFY(QTest::qWaitFor([&destroyed]{ return destroyed; }));
}

template<typename T>
void verifyListProperty(const T &data)
{
    QVERIFY(data.object);
    QVERIFY(data.append);
    QVERIFY(data.count);
    QVERIFY(data.at);
    QVERIFY(data.clear);
    QVERIFY(data.removeLast);

    // We must not synthesize the replace and removeLast methods on those properties.
    // They would be even more broken than the explicitly defined methods.
    QVERIFY(!data.replace);
}

void tst_qquickitem::listsAreNotLists()
{
    QQuickItem item;
    QQuickItem child;
    QObject resource;

    QQmlListProperty<QObject> data
        = item.property("data").value<QQmlListProperty<QObject>>();
    QQmlListProperty<QObject> resources
        = item.property("resources").value<QQmlListProperty<QObject>>();
    QQmlListProperty<QQuickItem> children
        = item.property("children").value<QQmlListProperty<QQuickItem>>();

    verifyListProperty(data);
    verifyListProperty(resources);
    verifyListProperty(children);


    QCOMPARE(data.count(&data), 0);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 0);
    children.append(&children, &child);
    QCOMPARE(data.count(&data), 1);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 1);
    children.removeLast(&children);
    QCOMPARE(data.count(&data), 0);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 0);
    data.append(&data, &child);
    QCOMPARE(data.count(&data), 1);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 1);
    data.removeLast(&data);
    QCOMPARE(data.count(&data), 0);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 0);
    children.append(&children, &child);
    QCOMPARE(data.count(&data), 1);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 1);
    data.removeLast(&data);
    QCOMPARE(data.count(&data), 0);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 0);
    data.append(&data, &child);
    QCOMPARE(data.count(&data), 1);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 1);
    children.removeLast(&children);
    QCOMPARE(data.count(&data), 0);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 0);


    resources.append(&resources, &resource);
    QCOMPARE(data.count(&data), 1);
    QCOMPARE(resources.count(&resources), 1);
    QCOMPARE(children.count(&children), 0);
    resources.removeLast(&resources);
    QCOMPARE(data.count(&data), 0);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 0);
    data.append(&data, &resource);
    QCOMPARE(data.count(&data), 1);
    QCOMPARE(resources.count(&resources), 1);
    QCOMPARE(children.count(&children), 0);
    data.removeLast(&data);
    QCOMPARE(data.count(&data), 0);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 0);
    resources.append(&resources, &resource);
    QCOMPARE(data.count(&data), 1);
    QCOMPARE(resources.count(&resources), 1);
    QCOMPARE(children.count(&children), 0);
    data.removeLast(&data);
    QCOMPARE(data.count(&data), 0);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 0);
    data.append(&data, &resource);
    QCOMPARE(data.count(&data), 1);
    QCOMPARE(resources.count(&resources), 1);
    QCOMPARE(children.count(&children), 0);
    resources.removeLast(&resources);
    QCOMPARE(data.count(&data), 0);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 0);


    children.append(&children, &child);
    resources.append(&resources, &resource);
    QCOMPARE(data.count(&data), 2);
    QCOMPARE(resources.count(&resources), 1);
    QCOMPARE(children.count(&children), 1);
    children.removeLast(&children);
    QCOMPARE(data.count(&data), 1);
    QCOMPARE(resources.count(&resources), 1);
    QCOMPARE(children.count(&children), 0);
    resources.removeLast(&resources);
    QCOMPARE(data.count(&data), 0);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 0);


    children.append(&children, &child);
    resources.append(&resources, &resource);
    QCOMPARE(data.count(&data), 2);
    QCOMPARE(resources.count(&resources), 1);
    QCOMPARE(children.count(&children), 1);
    resources.removeLast(&resources);
    QCOMPARE(data.count(&data), 1);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 1);
    children.removeLast(&children);
    QCOMPARE(data.count(&data), 0);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 0);


    data.append(&data, &child);
    data.append(&data, &resource);
    QCOMPARE(data.count(&data), 2);
    QCOMPARE(resources.count(&resources), 1);
    QCOMPARE(children.count(&children), 1);
    data.removeLast(&data);
    QCOMPARE(data.count(&data), 1);
    QCOMPARE(resources.count(&resources), 1);
    QCOMPARE(children.count(&children), 0);
    data.removeLast(&data);
    QCOMPARE(data.count(&data), 0);
    QCOMPARE(resources.count(&resources), 0);
    QCOMPARE(children.count(&children), 0);
}

QTEST_MAIN(tst_qquickitem)

#include "tst_qquickitem.moc"
