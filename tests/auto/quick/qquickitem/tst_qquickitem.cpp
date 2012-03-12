/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickcanvas.h>
#include <QtQuick/qquickview.h>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include "private/qquickfocusscope_p.h"
#include "private/qquickitem_p.h"
#include <QDebug>
#include <QTimer>
#include "../../shared/util.h"

class TestItem : public QQuickItem
{
Q_OBJECT
public:
    TestItem(QQuickItem *parent = 0)
        : QQuickItem(parent), focused(false), pressCount(0), releaseCount(0)
        , wheelCount(0), acceptIncomingTouchEvents(true)
        , touchEventReached(false) {}

    bool focused;
    int pressCount;
    int releaseCount;
    int wheelCount;
    bool acceptIncomingTouchEvents;
    bool touchEventReached;
protected:
    virtual void focusInEvent(QFocusEvent *) { Q_ASSERT(!focused); focused = true; }
    virtual void focusOutEvent(QFocusEvent *) { Q_ASSERT(focused); focused = false; }
    virtual void mousePressEvent(QMouseEvent *event) { event->accept(); ++pressCount; }
    virtual void mouseReleaseEvent(QMouseEvent *event) { event->accept(); ++releaseCount; }
    virtual void touchEvent(QTouchEvent *event) {
        touchEventReached = true;
        event->setAccepted(acceptIncomingTouchEvents);
    }
    virtual void wheelEvent(QWheelEvent *event) { event->accept(); ++wheelCount; }
};

class TestCanvas: public QQuickCanvas
{
public:
    TestCanvas()
        : QQuickCanvas()
    {}

    virtual bool event(QEvent *event)
    {
        return QQuickCanvas::event(event);
    }
};

class TestPolishItem : public QQuickItem
{
Q_OBJECT
public:
    TestPolishItem(QQuickItem *parent = 0)
    : QQuickItem(parent), wasPolished(false) {

    }

    bool wasPolished;

protected:
    virtual void updatePolish() {
        wasPolished = true;
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
    TestFocusScope(QQuickItem *parent = 0) : QQuickFocusScope(parent), focused(false) {}

    bool focused;
protected:
    virtual void focusInEvent(QFocusEvent *) { Q_ASSERT(!focused); focused = true; }
    virtual void focusOutEvent(QFocusEvent *) { Q_ASSERT(focused); focused = false; }
};

class tst_qquickitem : public QQmlDataTest
{
    Q_OBJECT
public:

private slots:
    void initTestCase();

    void noCanvas();
    void simpleFocus();
    void scopedFocus();
    void addedToCanvas();
    void changeParent();
    void multipleFocusClears();

    void constructor();
    void setParentItem();

    void visible();
    void enabled();
    void enabledFocus();

    void mouseGrab();
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

private:

    enum PaintOrderOp {
        NoOp, Append, Remove, StackBefore, StackAfter, SetZ
    };

    void ensureFocus(QWindow *w) {
        w->show();
        w->requestActivateWindow();
        qApp->processEvents();
    }
};

void tst_qquickitem::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<TestPolishItem>("Qt.test", 1, 0, "TestPolishItem");
}

// Focus has no effect when outside a canvas
void tst_qquickitem::noCanvas()
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
    QCOMPARE(root->hasFocus(), true);
    QCOMPARE(child->hasFocus(), false);
    QCOMPARE(scope->hasFocus(), true);
    QCOMPARE(scopedChild->hasFocus(), false);
    QCOMPARE(scopedChild2->hasFocus(), true);

    root->setFocus(false);
    child->setFocus(true);
    scopedChild->setFocus(true);
    scope->setFocus(false);
    QCOMPARE(root->hasFocus(), false);
    QCOMPARE(child->hasFocus(), true);
    QCOMPARE(scope->hasFocus(), false);
    QCOMPARE(scopedChild->hasFocus(), true);
    QCOMPARE(scopedChild2->hasFocus(), true);

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
    FocusState() : activeFocusItem(0) {}
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
            QCOMPARE(canvas.activeFocusItem(), focusState.activeFocusItem); \
            if (qobject_cast<TestItem *>(canvas.activeFocusItem())) \
                QCOMPARE(qobject_cast<TestItem *>(canvas.activeFocusItem())->focused, true); \
            else if (qobject_cast<TestFocusScope *>(canvas.activeFocusItem())) \
                QCOMPARE(qobject_cast<TestFocusScope *>(canvas.activeFocusItem())->focused, true); \
        } else { \
            QCOMPARE(canvas.activeFocusItem(), canvas.rootItem()); \
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
    QQuickCanvas canvas;
    ensureFocus(&canvas);

#ifdef Q_OS_MAC
    QSKIP("QTBUG-24094: fails on Mac OS X 10.7");
#endif

    QTRY_VERIFY(QGuiApplication::focusWindow() == &canvas);

    QQuickItem *l1c1 = new TestItem(canvas.rootItem());
    QQuickItem *l1c2 = new TestItem(canvas.rootItem());
    QQuickItem *l1c3 = new TestItem(canvas.rootItem());

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
    focusState.active(0);
    FVERIFY();

    l2c1->setFocus(true);
    focusState[l2c1].set(true, true);
    focusState.active(l2c1);
    FVERIFY();
}

// Items with a focus scope
void tst_qquickitem::scopedFocus()
{
    QQuickCanvas canvas;
    ensureFocus(&canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &canvas);

    QQuickItem *l1c1 = new TestItem(canvas.rootItem());
    QQuickItem *l1c2 = new TestItem(canvas.rootItem());
    QQuickItem *l1c3 = new TestItem(canvas.rootItem());

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

// Tests focus corrects itself when a tree is added to a canvas for the first time
void tst_qquickitem::addedToCanvas()
{
    {
    QQuickCanvas canvas;
    ensureFocus(&canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &canvas);

    QQuickItem *item = new TestItem;

    FocusState focusState;
    focusState << item;

    item->setFocus(true);
    focusState[item].set(true, false);
    FVERIFY();

    item->setParentItem(canvas.rootItem());
    focusState[item].set(true, true);
    focusState.active(item);
    FVERIFY();
    }

    {
    QQuickCanvas canvas;
    ensureFocus(&canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &canvas);

    QQuickItem *item = new TestItem(canvas.rootItem());

    QQuickItem *tree = new TestItem;
    QQuickItem *c1 = new TestItem(tree);
    QQuickItem *c2 = new TestItem(tree);

    FocusState focusState;
    focusState << item << tree << c1 << c2;

    item->setFocus(true);
    c1->setFocus(true);
    c2->setFocus(true);
    focusState[item].set(true, true);
    focusState[c1].set(true, false);
    focusState[c2].set(true, false);
    focusState.active(item);
    FVERIFY();

    tree->setParentItem(item);
    focusState[c1].set(false, false);
    focusState[c2].set(false, false);
    FVERIFY();
    }

    {
    QQuickCanvas canvas;
    ensureFocus(&canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &canvas);

    QQuickItem *tree = new TestItem;
    QQuickItem *c1 = new TestItem(tree);
    QQuickItem *c2 = new TestItem(tree);

    FocusState focusState;
    focusState << tree << c1 << c2;
    c1->setFocus(true);
    c2->setFocus(true);
    focusState[c1].set(true, false);
    focusState[c2].set(true, false);
    FVERIFY();

    tree->setParentItem(canvas.rootItem());
    focusState[c1].set(true, true);
    focusState[c2].set(false, false);
    focusState.active(c1);
    FVERIFY();
    }

    {
    QQuickCanvas canvas;
    ensureFocus(&canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &canvas);
    QQuickItem *tree = new TestFocusScope;
    QQuickItem *c1 = new TestItem(tree);
    QQuickItem *c2 = new TestItem(tree);

    FocusState focusState;
    focusState << tree << c1 << c2;
    c1->setFocus(true);
    c2->setFocus(true);
    focusState[c1].set(true, false);
    focusState[c2].set(true, false);
    FVERIFY();

    tree->setParentItem(canvas.rootItem());
    focusState[c1].set(true, false);
    focusState[c2].set(false, false);
    FVERIFY();

    tree->setFocus(true);
    focusState[tree].set(true, true);
    focusState[c1].set(true, true);
    focusState.active(c1);
    FVERIFY();
    }

    {
    QQuickCanvas canvas;
    ensureFocus(&canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &canvas);
    QQuickItem *tree = new TestFocusScope;
    QQuickItem *c1 = new TestItem(tree);
    QQuickItem *c2 = new TestItem(tree);

    FocusState focusState;
    focusState << tree << c1 << c2;
    tree->setFocus(true);
    c1->setFocus(true);
    c2->setFocus(true);
    focusState[tree].set(true, false);
    focusState[c1].set(true, false);
    focusState[c2].set(true, false);
    FVERIFY();

    tree->setParentItem(canvas.rootItem());
    focusState[tree].set(true, true);
    focusState[c1].set(true, true);
    focusState[c2].set(false, false);
    focusState.active(c1);
    FVERIFY();
    }

    {
    QQuickCanvas canvas;
    ensureFocus(&canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &canvas);
    QQuickItem *child = new TestItem(canvas.rootItem());
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
    focusState[c1].set(true, false);
    focusState[c2].set(true, false);
    focusState.active(child);
    FVERIFY();

    tree->setParentItem(canvas.rootItem());
    focusState[tree].set(false, false);
    focusState[c1].set(true, false);
    focusState[c2].set(false, false);
    FVERIFY();

    tree->setFocus(true);
    focusState[child].set(false, false);
    focusState[tree].set(true, true);
    focusState[c1].set(true, true);
    focusState.active(c1);
    FVERIFY();
    }
}

void tst_qquickitem::changeParent()
{
    // Parent to no parent
    {
    QQuickCanvas canvas;
    ensureFocus(&canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &canvas);
    QQuickItem *child = new TestItem(canvas.rootItem());

    FocusState focusState;
    focusState << child;
    FVERIFY();

    child->setFocus(true);
    focusState[child].set(true, true);
    focusState.active(child);
    FVERIFY();

    child->setParentItem(0);
    focusState[child].set(true, false);
    focusState.active(0);
    FVERIFY();
    }

    // Different parent, same focus scope
    {
    QQuickCanvas canvas;
    ensureFocus(&canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &canvas);
    QQuickItem *child = new TestItem(canvas.rootItem());
    QQuickItem *child2 = new TestItem(canvas.rootItem());

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
    QQuickCanvas canvas;
    ensureFocus(&canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &canvas);
    QQuickItem *child = new TestItem(canvas.rootItem());
    QQuickItem *child2 = new TestFocusScope(canvas.rootItem());
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
    focusState.active(0);
    FVERIFY();
    }
    {
    QQuickCanvas canvas;
    ensureFocus(&canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &canvas);
    QQuickItem *child = new TestItem(canvas.rootItem());
    QQuickItem *child2 = new TestFocusScope(canvas.rootItem());
    QQuickItem *item = new TestItem(child2);

    FocusState focusState;
    focusState << child << child2 << item;
    FVERIFY();

    item->setFocus(true);
    focusState[item].set(true, false);
    focusState.active(0);
    FVERIFY();

    item->setParentItem(child);
    focusState[item].set(true, true);
    focusState.active(item);
    FVERIFY();
    }
    {
    QQuickCanvas canvas;
    ensureFocus(&canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &canvas);
    QQuickItem *child = new TestItem(canvas.rootItem());
    QQuickItem *child2 = new TestFocusScope(canvas.rootItem());
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

}

void tst_qquickitem::multipleFocusClears()
{
    //Multiple clears of focus inside a focus scope shouldn't crash. QTBUG-24714
    QQuickView *view = new QQuickView;
    view->setSource(testFileUrl("multipleFocusClears.qml"));
    view->show();
    ensureFocus(view);
    QTRY_VERIFY(QGuiApplication::focusWindow() == view);
}

void tst_qquickitem::constructor()
{
    QQuickItem *root = new QQuickItem;
    QVERIFY(root->parent() == 0);
    QVERIFY(root->parentItem() == 0);

    QQuickItem *child1 = new QQuickItem(root);
    QVERIFY(child1->parent() == root);
    QVERIFY(child1->parentItem() == root);
    QCOMPARE(root->childItems().count(), 1);
    QCOMPARE(root->childItems().at(0), child1);

    QQuickItem *child2 = new QQuickItem(root);
    QVERIFY(child2->parent() == root);
    QVERIFY(child2->parentItem() == root);
    QCOMPARE(root->childItems().count(), 2);
    QCOMPARE(root->childItems().at(0), child1);
    QCOMPARE(root->childItems().at(1), child2);

    delete root;
}

void tst_qquickitem::setParentItem()
{
    QQuickItem *root = new QQuickItem;
    QVERIFY(root->parent() == 0);
    QVERIFY(root->parentItem() == 0);

    QQuickItem *child1 = new QQuickItem;
    QVERIFY(child1->parent() == 0);
    QVERIFY(child1->parentItem() == 0);

    child1->setParentItem(root);
    QVERIFY(child1->parent() == 0);
    QVERIFY(child1->parentItem() == root);
    QCOMPARE(root->childItems().count(), 1);
    QCOMPARE(root->childItems().at(0), child1);

    QQuickItem *child2 = new QQuickItem;
    QVERIFY(child2->parent() == 0);
    QVERIFY(child2->parentItem() == 0);
    child2->setParentItem(root);
    QVERIFY(child2->parent() == 0);
    QVERIFY(child2->parentItem() == root);
    QCOMPARE(root->childItems().count(), 2);
    QCOMPARE(root->childItems().at(0), child1);
    QCOMPARE(root->childItems().at(1), child2);

    child1->setParentItem(0);
    QVERIFY(child1->parent() == 0);
    QVERIFY(child1->parentItem() == 0);
    QCOMPARE(root->childItems().count(), 1);
    QCOMPARE(root->childItems().at(0), child2);

    delete root;

    QVERIFY(child1->parent() == 0);
    QVERIFY(child1->parentItem() == 0);
    QVERIFY(child2->parent() == 0);
    QVERIFY(child2->parentItem() == 0);

    delete child1;
    delete child2;
}

void tst_qquickitem::visible()
{
    QQuickItem *root = new QQuickItem;

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
    QQuickCanvas canvas;
    ensureFocus(&canvas);

    QQuickFocusScope root;

    root.setFocus(true);
    root.setEnabled(false);

    QCOMPARE(root.isEnabled(), false);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), false);

    root.setParentItem(canvas.rootItem());

    QCOMPARE(root.isEnabled(), false);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), false);
    QCOMPARE(canvas.activeFocusItem(), canvas.rootItem());

    root.setEnabled(true);
    QCOMPARE(root.isEnabled(), true);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), true);
    QCOMPARE(canvas.activeFocusItem(), static_cast<QQuickItem *>(&root));

    QQuickItem child1;
    child1.setParentItem(&root);

    QCOMPARE(child1.isEnabled(), true);
    QCOMPARE(child1.hasFocus(), false);
    QCOMPARE(child1.hasActiveFocus(), false);
    QCOMPARE(canvas.activeFocusItem(), static_cast<QQuickItem *>(&root));

    QQuickItem child2;
    child2.setFocus(true);
    child2.setParentItem(&root);

    QCOMPARE(root.isEnabled(), true);
    QCOMPARE(root.hasFocus(), true);
    QCOMPARE(root.hasActiveFocus(), true);
    QCOMPARE(child2.isEnabled(), true);
    QCOMPARE(child2.hasFocus(), true);
    QCOMPARE(child2.hasActiveFocus(), true);
    QCOMPARE(canvas.activeFocusItem(), &child2);

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
    QCOMPARE(canvas.activeFocusItem(), static_cast<QQuickItem *>(&root));

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
    QCOMPARE(canvas.activeFocusItem(), static_cast<QQuickItem *>(&root));

    child1.setEnabled(true);
    QCOMPARE(child1.isEnabled(), true);
    QCOMPARE(child1.hasFocus(), true);
    QCOMPARE(child1.hasActiveFocus(), true);
    QCOMPARE(canvas.activeFocusItem(), static_cast<QQuickItem *>(&child1));

    root.setFocus(false);
    QCOMPARE(root.isEnabled(), true);
    QCOMPARE(root.hasFocus(), false);
    QCOMPARE(root.hasActiveFocus(), false);
    QCOMPARE(child1.isEnabled(), true);
    QCOMPARE(child1.hasFocus(), true);
    QCOMPARE(child1.hasActiveFocus(), false);
    QCOMPARE(canvas.activeFocusItem(), canvas.rootItem());

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
    QCOMPARE(canvas.activeFocusItem(), static_cast<QQuickItem *>(&root));

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
    QCOMPARE(canvas.activeFocusItem(), canvas.rootItem());

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
    QCOMPARE(canvas.activeFocusItem(), canvas.rootItem());

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
    QCOMPARE(canvas.activeFocusItem(), static_cast<QQuickItem *>(&child1));
}

void tst_qquickitem::mouseGrab()
{
    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(200, 200);
    canvas->show();

    TestItem *child1 = new TestItem;
    child1->setAcceptedMouseButtons(Qt::LeftButton);
    child1->setSize(QSizeF(200, 100));
    child1->setParentItem(canvas->rootItem());

    TestItem *child2 = new TestItem;
    child2->setAcceptedMouseButtons(Qt::LeftButton);
    child2->setY(51);
    child2->setSize(QSizeF(200, 100));
    child2->setParentItem(canvas->rootItem());

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(50,50));
    QTest::qWait(100);
    QVERIFY(canvas->mouseGrabberItem() == child1);
    QTest::qWait(100);

    QCOMPARE(child1->pressCount, 1);
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(50,50));
    QTest::qWait(50);
    QVERIFY(canvas->mouseGrabberItem() == 0);
    QCOMPARE(child1->releaseCount, 1);

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(50,50));
    QTest::qWait(50);
    QVERIFY(canvas->mouseGrabberItem() == child1);
    QCOMPARE(child1->pressCount, 2);
    child1->setEnabled(false);
    QVERIFY(canvas->mouseGrabberItem() == 0);
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(50,50));
    QTest::qWait(50);
    QCOMPARE(child1->releaseCount, 1);
    child1->setEnabled(true);

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(50,50));
    QTest::qWait(50);
    QVERIFY(canvas->mouseGrabberItem() == child1);
    QCOMPARE(child1->pressCount, 3);
    child1->setVisible(false);
    QVERIFY(canvas->mouseGrabberItem() == 0);
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(50,50));
    QCOMPARE(child1->releaseCount, 1);
    child1->setVisible(true);

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(50,50));
    QTest::qWait(50);
    QVERIFY(canvas->mouseGrabberItem() == child1);
    QCOMPARE(child1->pressCount, 4);
    child2->grabMouse();
    QVERIFY(canvas->mouseGrabberItem() == child2);
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(50,50));
    QTest::qWait(50);
    QCOMPARE(child1->releaseCount, 1);
    QCOMPARE(child2->releaseCount, 1);

    child2->grabMouse();
    QVERIFY(canvas->mouseGrabberItem() == child2);
    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(50,50));
    QTest::qWait(50);
    QCOMPARE(child1->pressCount, 4);
    QCOMPARE(child2->pressCount, 1);
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(50,50));
    QTest::qWait(50);
    QCOMPARE(child1->releaseCount, 1);
    QCOMPARE(child2->releaseCount, 2);

    delete child1;
    delete child2;
    delete canvas;
}

void tst_qquickitem::touchEventAcceptIgnore_data()
{
    QTest::addColumn<bool>("itemSupportsTouch");

    QTest::newRow("with touch") << true;
    QTest::newRow("without touch") << false;
}

void tst_qquickitem::touchEventAcceptIgnore()
{
    QFETCH(bool, itemSupportsTouch);

    TestCanvas *canvas = new TestCanvas;
    canvas->resize(100, 100);
    canvas->show();

    TestItem *item = new TestItem;
    item->setSize(QSizeF(100, 100));
    item->setParentItem(canvas->rootItem());
    item->acceptIncomingTouchEvents = itemSupportsTouch;

    static QTouchDevice* device = 0;
    if (!device) {
        device =new QTouchDevice;
        device->setType(QTouchDevice::TouchScreen);
        QWindowSystemInterface::registerTouchDevice(device);
    }

    // Send Begin, Update & End touch sequence
    {
        QTouchEvent::TouchPoint point;
        point.setId(1);
        point.setPos(QPointF(50, 50));
        point.setScreenPos(point.pos());
        point.setState(Qt::TouchPointPressed);

        QTouchEvent event(QEvent::TouchBegin, device,
                          Qt::NoModifier,
                          Qt::TouchPointPressed,
                          QList<QTouchEvent::TouchPoint>() << point);
        event.setAccepted(true);

        item->touchEventReached = false;

        bool accepted = canvas->event(&event);

        QVERIFY(item->touchEventReached);
        QCOMPARE(accepted && event.isAccepted(), itemSupportsTouch);
    }
    {
        QTouchEvent::TouchPoint point;
        point.setId(1);
        point.setPos(QPointF(60, 60));
        point.setScreenPos(point.pos());
        point.setState(Qt::TouchPointMoved);

        QTouchEvent event(QEvent::TouchUpdate, device,
                          Qt::NoModifier,
                          Qt::TouchPointMoved,
                          QList<QTouchEvent::TouchPoint>() << point);
        event.setAccepted(true);

        item->touchEventReached = false;

        bool accepted = canvas->event(&event);

        QCOMPARE(item->touchEventReached, itemSupportsTouch);
        QCOMPARE(accepted && event.isAccepted(), itemSupportsTouch);
    }
    {
        QTouchEvent::TouchPoint point;
        point.setId(1);
        point.setPos(QPointF(60, 60));
        point.setScreenPos(point.pos());
        point.setState(Qt::TouchPointReleased);

        QTouchEvent event(QEvent::TouchEnd, device,
                          Qt::NoModifier,
                          Qt::TouchPointReleased,
                          QList<QTouchEvent::TouchPoint>() << point);
        event.setAccepted(true);

        item->touchEventReached = false;

        bool accepted = canvas->event(&event);

        QCOMPARE(item->touchEventReached, itemSupportsTouch);
        QCOMPARE(accepted && event.isAccepted(), itemSupportsTouch);
    }

    delete item;
    delete canvas;
}

void tst_qquickitem::polishOutsideAnimation()
{
    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(200, 200);
    canvas->show();

    TestPolishItem *item = new TestPolishItem(canvas->rootItem());
    item->setSize(QSizeF(200, 100));
    QTest::qWait(50);

    QTimer::singleShot(10, item, SLOT(doPolish()));
    QTRY_VERIFY(item->wasPolished);

    delete item;
    delete canvas;
}

void tst_qquickitem::polishOnCompleted()
{
    QQuickView *view = new QQuickView;
    view->setSource(testFileUrl("polishOnCompleted.qml"));
    view->show();

    TestPolishItem *item = qobject_cast<TestPolishItem*>(view->rootObject());
    QVERIFY(item);

#ifdef Q_OS_MAC
    QSKIP("QTBUG-21590 view does not reliably receive polish without a running animation");
#endif

    QTRY_VERIFY(item->wasPolished);

    delete view;
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

    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(200, 200);
    canvas->show();

    TestItem *item = new TestItem;
    item->setSize(QSizeF(200, 100));
    item->setParentItem(canvas->rootItem());

    item->setEnabled(enabled);
    item->setVisible(visible);

    QWheelEvent event(QPoint(100, 50), -120, Qt::NoButton, Qt::NoModifier, Qt::Vertical);
    event.setAccepted(false);
    QGuiApplication::sendEvent(canvas, &event);

    if (shouldReceiveWheelEvents) {
        QVERIFY(event.isAccepted());
        QCOMPARE(item->wheelCount, 1);
    } else {
        QVERIFY(!event.isAccepted());
        QCOMPARE(item->wheelCount, 0);
    }

    delete canvas;
}

class HoverItem : public QQuickItem
{
Q_OBJECT
public:
    HoverItem(QQuickItem *parent = 0)
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
protected:
    virtual void hoverEnterEvent(QHoverEvent *event) {
        event->accept();
        ++hoverEnterCount;
    }
    virtual void hoverMoveEvent(QHoverEvent *event) {
        event->accept();
        ++hoverMoveCount;
    }
    virtual void hoverLeaveEvent(QHoverEvent *event) {
        event->accept();
        ++hoverLeaveCount;
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

// ### For some unknown reason QTest::mouseMove() isn't working correctly.
static void sendMouseMove(QObject *object, const QPoint &position)
{
    QMouseEvent moveEvent(QEvent::MouseMove, position, Qt::NoButton, Qt::NoButton, 0);
    QGuiApplication::sendEvent(object, &moveEvent);
}

void tst_qquickitem::hoverEvent()
{
    QFETCH(bool, visible);
    QFETCH(bool, enabled);
    QFETCH(bool, acceptHoverEvents);

    QQuickCanvas *canvas = new QQuickCanvas();
    canvas->resize(200, 200);
    canvas->show();

    HoverItem *item = new HoverItem;
    item->setSize(QSizeF(100, 100));
    item->setParentItem(canvas->rootItem());

    item->setEnabled(enabled);
    item->setVisible(visible);
    item->setAcceptHoverEvents(acceptHoverEvents);

    const QPoint outside(150, 150);
    const QPoint inside(50, 50);
    const QPoint anotherInside(51, 51);

    sendMouseMove(canvas, outside);
    item->resetCounters();

    // Enter, then move twice inside, then leave.
    sendMouseMove(canvas, inside);
    sendMouseMove(canvas, anotherInside);
    sendMouseMove(canvas, inside);
    sendMouseMove(canvas, outside);

    const bool shouldReceiveHoverEvents = visible && enabled && acceptHoverEvents;
    if (shouldReceiveHoverEvents) {
        QCOMPARE(item->hoverEnterCount, 1);
        QCOMPARE(item->hoverMoveCount, 2);
        QCOMPARE(item->hoverLeaveCount, 1);
    } else {
        QCOMPARE(item->hoverEnterCount, 0);
        QCOMPARE(item->hoverMoveCount, 0);
        QCOMPARE(item->hoverLeaveCount, 0);
    }

    delete canvas;
}

void tst_qquickitem::hoverEventInParent()
{
    QQuickCanvas *canvas = new QQuickCanvas();
    canvas->resize(200, 200);
    canvas->show();

    HoverItem *parentItem = new HoverItem(canvas->rootItem());
    parentItem->setSize(QSizeF(200, 200));
    parentItem->setAcceptHoverEvents(true);

    HoverItem *leftItem = new HoverItem(parentItem);
    leftItem->setSize(QSizeF(100, 200));
    leftItem->setAcceptHoverEvents(true);

    HoverItem *rightItem = new HoverItem(parentItem);
    rightItem->setSize(QSizeF(100, 200));
    rightItem->setPos(QPointF(100, 0));
    rightItem->setAcceptHoverEvents(true);

    const QPoint insideLeft(50, 100);
    const QPoint insideRight(150, 100);

    sendMouseMove(canvas, insideLeft);
    parentItem->resetCounters();
    leftItem->resetCounters();
    rightItem->resetCounters();

    sendMouseMove(canvas, insideRight);
    QCOMPARE(parentItem->hoverEnterCount, 0);
    QCOMPARE(parentItem->hoverLeaveCount, 0);
    QCOMPARE(leftItem->hoverEnterCount, 0);
    QCOMPARE(leftItem->hoverLeaveCount, 1);
    QCOMPARE(rightItem->hoverEnterCount, 1);
    QCOMPARE(rightItem->hoverLeaveCount, 0);

    sendMouseMove(canvas, insideLeft);
    QCOMPARE(parentItem->hoverEnterCount, 0);
    QCOMPARE(parentItem->hoverLeaveCount, 0);
    QCOMPARE(leftItem->hoverEnterCount, 1);
    QCOMPARE(leftItem->hoverLeaveCount, 1);
    QCOMPARE(rightItem->hoverEnterCount, 1);
    QCOMPARE(rightItem->hoverLeaveCount, 1);

    delete canvas;
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
    for (int i = 0; i < list.count(); ++i)
        items << list.at(i)->objectName();

    QCOMPARE(items, expected);
}


QTEST_MAIN(tst_qquickitem)

#include "tst_qquickitem.moc"
