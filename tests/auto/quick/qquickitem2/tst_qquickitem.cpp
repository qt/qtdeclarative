// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QtTest/QSignalSpy>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickitemgrabresult.h>
#include <QtQuick/qquickview.h>
#include <QtGui/private/qinputmethod_p.h>
#include <QtQuick/private/qquickloader_p.h>
#include <QtQuick/private/qquickpalette_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/private/qquicktextinput_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuick/private/qquickanchors_p.h>
#include <QtGui/qstylehints.h>
#include <private/qquickitem_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/platforminputcontext_p.h>
#include <QtTest/private/qpropertytesthelper_p.h>

using namespace QQuickVisualTestUtils;

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

class tst_QQuickItem : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickItem();

private slots:
    void initTestCase() override;
    void cleanup();

    void activeFocusOnTab();
    void activeFocusOnTab2();
    void activeFocusOnTab3();
    void activeFocusOnTab4();
    void activeFocusOnTab5();
    void activeFocusOnTab6();
    void activeFocusOnTab7();
    void activeFocusOnTab8();
    void activeFocusOnTab9();
    void activeFocusOnTab10();
    void activeFocusOnTab_infiniteLoop_data();
    void activeFocusOnTab_infiniteLoop();
    void activeFocusOnTab_infiniteLoopControls();

    void nextItemInFocusChain();
    void nextItemInFocusChain2();
    void nextItemInFocusChain3();

    void tabFence();
    void qtbug_50516();
    void qtbug_50516_2_data();
    void qtbug_50516_2();
    void focusableItemReparentedToLoadedComponent();

    void keys();
#if QT_CONFIG(shortcut)
    void standardKeys_data();
    void standardKeys();
#endif
    void keysProcessingOrder();
    void keysim();
    void keysForward();
    void keyNavigation_data();
    void keyNavigation();
    void keyNavigation_RightToLeft();
    void keyNavigation_skipNotVisible();
    void keyNavigation_implicitSetting();
    void keyNavigation_implicitDestroy();
    void keyNavigation_focusReason();
    void keyNavigation_loop();
    void keyNavigation_repeater();
    void layoutMirroring();
    void layoutMirroringWindow();
    void layoutMirroringIllegalParent();
    void smooth();
    void antialiasing();
    void clip();
    void mapCoordinates();
    void mapCoordinates_data();
    void mapCoordinatesRect();
    void mapCoordinatesRect_data();
    void propertyChanges();
    void nonexistentPropertyConnection();
    void transforms();
    void transforms_data();
    void childrenRect();
    void childrenRectBug();
    void childrenRectBug2();
    void childrenRectBug3();
    void childrenRectBottomRightCorner();

    void childrenProperty();
    void resourcesProperty();
    void bindableProperties_data();
    void bindableProperties();

    void changeListener();
    void transformCrash();
    void implicitSize();
    void qtbug_16871();
    void visibleChildren();
    void parentLoop();
    void contains_data();
    void contains();
    void childAt();
    void isAncestorOf();

    void grab();

    void colorGroup();
    void paletteAllocated();

    void undefinedIsInvalidForWidthAndHeight();

    void viewport_data();
    void viewport();

    void qobject_castOnDestruction();
    void signalsOnDestruction_data();
    void signalsOnDestruction();
    void visibleChanged();

private:
    QQmlEngine engine;
    bool qt_tab_all_widgets() {
        return QGuiApplication::styleHints()->tabFocusBehavior() == Qt::TabFocusAllControls;
    }
};

class KeysTestObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool processLast READ processLast NOTIFY processLastChanged)

public:
    KeysTestObject() : mKey(0), mModifiers(0), mForwardedKey(0), mLast(false), mNativeScanCode(0) {}

    void reset() {
        mKey = 0;
        mText = QString();
        mModifiers = 0;
        mForwardedKey = 0;
        mNativeScanCode = 0;
    }

    bool processLast() const { return mLast; }
    void setProcessLast(bool b) {
        if (b != mLast) {
            mLast = b;
            emit processLastChanged();
        }
    }

public slots:
    void keyPress(int key, QString text, int modifiers) {
        mKey = key;
        mText = text;
        mModifiers = modifiers;
    }
    void keyRelease(int key, QString text, int modifiers) {
        mKey = key;
        mText = text;
        mModifiers = modifiers;
    }
    void forwardedKey(int key) {
        mForwardedKey = key;
    }
    void specialKey(int key, QString text, quint32 nativeScanCode) {
        mKey = key;
        mText = text;
        mNativeScanCode = nativeScanCode;
    }

signals:
    void processLastChanged();

public:
    int mKey;
    QString mText;
    int mModifiers;
    int mForwardedKey;
    bool mLast;
    quint32 mNativeScanCode;

private:
};

class KeyTestItem : public QQuickItem
{
    Q_OBJECT
public:
    KeyTestItem(QQuickItem *parent=nullptr) : QQuickItem(parent), mKey(0) {}

protected:
    void keyPressEvent(QKeyEvent *e) override {
        mKey = e->key();

        if (e->key() == Qt::Key_A)
            e->accept();
        else
            e->ignore();
    }

    void keyReleaseEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_B)
            e->accept();
        else
            e->ignore();
    }

public:
    int mKey;
};

class FocusEventFilter : public QObject
{
protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if ((event->type() == QEvent::FocusIn) ||  (event->type() == QEvent::FocusOut)) {
            QFocusEvent *focusEvent = static_cast<QFocusEvent *>(event);
            lastFocusReason = focusEvent->reason();
            return false;
        } else
            return QObject::eventFilter(watched, event);
    }
public:
    Qt::FocusReason lastFocusReason;
};

QML_DECLARE_TYPE(KeyTestItem);

class HollowTestItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool circle READ isCircle WRITE setCircle NOTIFY circleChanged)
    Q_PROPERTY(qreal holeRadius READ holeRadius WRITE setHoleRadius NOTIFY holeRadiusChanged)

public:
    HollowTestItem(QQuickItem *parent = nullptr)
        : QQuickItem(parent),
          m_isPressed(false),
          m_isHovered(false),
          m_isCircle(false),
          m_holeRadius(50)
    {
        setAcceptHoverEvents(true);
        setAcceptedMouseButtons(Qt::LeftButton);
    }

    bool isPressed() const { return m_isPressed; }
    bool isHovered() const { return m_isHovered; }

    bool isCircle() const { return m_isCircle; }
    void setCircle(bool circle) { m_isCircle = circle; emit circleChanged(); }

    qreal holeRadius() const { return m_holeRadius; }
    void setHoleRadius(qreal radius) { m_holeRadius = radius; emit holeRadiusChanged(); }

    bool contains(const QPointF &point) const override {
        const qreal w = width();
        const qreal h = height();
        const qreal r = m_holeRadius;

        // check boundaries
        if (!QRectF(0, 0, w, h).contains(point))
            return false;

        // square shape
        if (!m_isCircle)
            return !QRectF(w / 2 - r, h / 2 - r, r * 2, r * 2).contains(point);

        // circle shape
        const qreal dx = point.x() - (w / 2);
        const qreal dy = point.y() - (h / 2);
        const qreal dd = (dx * dx) + (dy * dy);
        const qreal outerRadius = qMin<qreal>(w / 2, h / 2);
        return dd > (r * r) && dd <= outerRadius * outerRadius;
    }

signals:
    void circleChanged();
    void holeRadiusChanged();

protected:
    void hoverEnterEvent(QHoverEvent *) override { m_isHovered = true; }
    void hoverLeaveEvent(QHoverEvent *) override { m_isHovered = false; }
    void mousePressEvent(QMouseEvent *) override { m_isPressed = true; }
    void mouseReleaseEvent(QMouseEvent *) override { m_isPressed = false; }

private:
    bool m_isPressed;
    bool m_isHovered;
    bool m_isCircle;
    qreal m_holeRadius;
};

QML_DECLARE_TYPE(HollowTestItem);

class ViewportTestItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QRectF viewport READ viewport NOTIFY viewportChanged)

public:
    ViewportTestItem(QQuickItem *parent = nullptr) : QQuickItem(parent) { }

    QRectF viewport() const { return clipRect(); }

signals:
    void viewportChanged();
};

QML_DECLARE_TYPE(ViewportTestItem);

class TabFenceItem : public QQuickItem
{
    Q_OBJECT

public:
    TabFenceItem(QQuickItem *parent = nullptr)
        : QQuickItem(parent)
    {
        QQuickItemPrivate *d = QQuickItemPrivate::get(this);
        d->isTabFence = true;
    }
};

QML_DECLARE_TYPE(TabFenceItem);

class TabFenceItem2 : public QQuickItem
{
    Q_OBJECT

public:
    TabFenceItem2(QQuickItem *parent = nullptr)
        : QQuickItem(parent)
    {
        QQuickItemPrivate *d = QQuickItemPrivate::get(this);
        d->isTabFence = true;
        setFlag(ItemIsFocusScope);
    }
};

QML_DECLARE_TYPE(TabFenceItem2);

tst_QQuickItem::tst_QQuickItem()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickItem::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<KeyTestItem>("Test",1,0,"KeyTestItem");
    qmlRegisterType<HollowTestItem>("Test", 1, 0, "HollowTestItem");
    qmlRegisterType<ViewportTestItem>("Test", 1, 0, "ViewportTestItem");
    qmlRegisterType<TabFenceItem>("Test", 1, 0, "TabFence");
    qmlRegisterType<TabFenceItem2>("Test", 1, 0, "TabFence2");
}

void tst_QQuickItem::cleanup()
{
    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = nullptr;
}

void tst_QQuickItem::activeFocusOnTab()
{
    if (!qt_tab_all_widgets())
        QSKIP("This function doesn't support NOT iterating all.");

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("activeFocusOnTab.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    // original: button12
    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "button12");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: button12->sub2
    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "sub2");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // Tab: sub2->button21
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "button21");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // Tab: button21->button22
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "button22");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // Tab: button22->edit
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "edit");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }
    // BackTab: edit->button22
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "button22");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }
    // BackTab: button22->button21
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "button21");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // BackTab: button21->sub2
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "sub2");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // BackTab: sub2->button12
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "button12");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // BackTab: button12->button11
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "button11");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // BackTab: button11->edit
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "edit");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    delete window;
}

void tst_QQuickItem::activeFocusOnTab2()
{
    if (!qt_tab_all_widgets())
        QSKIP("This function doesn't support NOT iterating all.");

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("activeFocusOnTab.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    // original: button12
    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "button12");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: button12->button11
    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "button11");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // BackTab: button11->edit
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "edit");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    delete window;
}

void tst_QQuickItem::activeFocusOnTab3()
{
    if (!qt_tab_all_widgets())
        QSKIP("This function doesn't support NOT iterating all.");

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("activeFocusOnTab3.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    // original: button1
    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "button1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // 4 Tabs: button1->button2, through a repeater
    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);;
        for (int i = 0; i < 4; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button2");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // 4 Tabs: button2->button3, through a row
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);;
        for (int i = 0; i < 4; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button3");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // 4 Tabs: button3->button4, through a flow
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);;
        for (int i = 0; i < 4; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button4");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // 4 Tabs: button4->button5, through a focusscope
    // parent is activeFocusOnTab:false, one of children is focus:true
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);;
        for (int i = 0; i < 4; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button5");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // 4 Tabs: button5->button6, through a focusscope
    // parent is activeFocusOnTab:true, one of children is focus:true
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);;
        for (int i = 0; i < 4; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button6");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // 5 Tabs: button6->button7, through a focusscope
    // parent is activeFocusOnTab:true, none of children is focus:true
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);;
        for (int i = 0; i < 5; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button7");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // 4 BackTabs: button7->button6, through a focusscope
    // parent is activeFocusOnTab:true, one of children got focus:true in previous code
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        for (int i = 0; i < 4; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button6");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // 4 Tabs: button6->button7, through a focusscope
    // parent is activeFocusOnTab:true, one of children got focus:true in previous code
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);;
        for (int i = 0; i < 4; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button7");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // 4 BackTabs: button7->button6, through a focusscope
    // parent is activeFocusOnTab:true, one of children got focus:true in previous code
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        for (int i = 0; i < 4; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button6");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // 4 BackTabs: button6->button5, through a focusscope(parent is activeFocusOnTab: false)
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        for (int i = 0; i < 4; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button5");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // 4 BackTabs: button5->button4, through a focusscope(parent is activeFocusOnTab: false)
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        for (int i = 0; i < 4; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button4");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // 4 BackTabs: button4->button3, through a flow
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        for (int i = 0; i < 4; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button3");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // 4 BackTabs: button3->button2, through a row
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        for (int i = 0; i < 4; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button2");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // 4 BackTabs: button2->button1, through a repeater
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        for (int i = 0; i < 4; ++i) {
            QGuiApplication::sendEvent(window, &key);
            QVERIFY(key.isAccepted());
        }

        item = findItem<QQuickItem>(window->rootObject(), "button1");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    delete window;
}

void tst_QQuickItem::activeFocusOnTab4()
{
    if (!qt_tab_all_widgets())
        QSKIP("This function doesn't support NOT iterating all.");

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("activeFocusOnTab4.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    // original: button11
    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "button11");
    item->setActiveFocusOnTab(true);
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: button11->button21
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "button21");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    delete window;
}

void tst_QQuickItem::activeFocusOnTab5()
{
    if (!qt_tab_all_widgets())
        QSKIP("This function doesn't support NOT iterating all.");

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("activeFocusOnTab4.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    // original: button11 in sub1
    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "button11");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    QQuickItem *item2 = findItem<QQuickItem>(window->rootObject(), "sub1");
    item2->setActiveFocusOnTab(true);

    // Tab: button11->button21
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "button21");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    delete window;
}

void tst_QQuickItem::activeFocusOnTab6()
{
    if (qt_tab_all_widgets())
        QSKIP("This function doesn't support iterating all.");

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("activeFocusOnTab6.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    // original: button12
    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "button12");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: button12->edit
    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "edit");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // BackTab: edit->button12
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "button12");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // BackTab: button12->button11
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "button11");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // BackTab: button11->edit
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "edit");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    delete window;
}

void tst_QQuickItem::activeFocusOnTab7()
{
    if (qt_tab_all_widgets())
        QSKIP("This function doesn't support iterating all.");

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(300,300));

    window->setSource(testFileUrl("activeFocusOnTab7.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "button1");
    QVERIFY(item);
    item->forceActiveFocus();
    QVERIFY(item->hasActiveFocus());

    // Tab: button1->button1
    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(!key.isAccepted());

        QVERIFY(item->hasActiveFocus());
    }

    // BackTab: button1->button1
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(!key.isAccepted());

        QVERIFY(item->hasActiveFocus());
    }

    delete window;
}

void tst_QQuickItem::activeFocusOnTab8()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(300,300));

    window->setSource(testFileUrl("activeFocusOnTab8.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QQuickItem *content = window->contentItem();
    QVERIFY(content);
    QVERIFY(content->hasActiveFocus());

    QQuickItem *button1 = findItem<QQuickItem>(window->rootObject(), "button1");
    QVERIFY(button1);
    QVERIFY(!button1->hasActiveFocus());

    QQuickItem *button2 = findItem<QQuickItem>(window->rootObject(), "button2");
    QVERIFY(button2);
    QVERIFY(!button2->hasActiveFocus());

    // Tab: contentItem->button1
    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        QVERIFY(button1->hasActiveFocus());
    }

    // Tab: button1->button2
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        QVERIFY(button2->hasActiveFocus());
        QVERIFY(!button1->hasActiveFocus());
    }

    // BackTab: button2->button1
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        QVERIFY(button1->hasActiveFocus());
        QVERIFY(!button2->hasActiveFocus());
    }

    delete window;
}

void tst_QQuickItem::activeFocusOnTab9()
{
    if (qt_tab_all_widgets())
        QSKIP("This function doesn't support iterating all.");

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(300,300));

    window->setSource(testFileUrl("activeFocusOnTab9.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QQuickItem *content = window->contentItem();
    QVERIFY(content);
    QVERIFY(content->hasActiveFocus());

    QQuickItem *textinput1 = findItem<QQuickItem>(window->rootObject(), "textinput1");
    QVERIFY(textinput1);
    QQuickItem *textedit1 = findItem<QQuickItem>(window->rootObject(), "textedit1");
    QVERIFY(textedit1);

    QVERIFY(!textinput1->hasActiveFocus());
    textinput1->forceActiveFocus();
    QVERIFY(textinput1->hasActiveFocus());

    // Tab: textinput1->textedit1
    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        QVERIFY(textedit1->hasActiveFocus());
    }

    // BackTab: textedit1->textinput1
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        QVERIFY(textinput1->hasActiveFocus());
    }

    // BackTab: textinput1->textedit1
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        QVERIFY(textedit1->hasActiveFocus());
    }

    delete window;
}

void tst_QQuickItem::activeFocusOnTab10()
{
    if (!qt_tab_all_widgets())
        QSKIP("This function doesn't support NOT iterating all.");

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(300,300));

    window->setSource(testFileUrl("activeFocusOnTab9.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QQuickItem *content = window->contentItem();
    QVERIFY(content);
    QVERIFY(content->hasActiveFocus());

    QQuickItem *textinput1 = findItem<QQuickItem>(window->rootObject(), "textinput1");
    QVERIFY(textinput1);
    QQuickItem *textedit1 = findItem<QQuickItem>(window->rootObject(), "textedit1");
    QVERIFY(textedit1);
    QQuickItem *textinput2 = findItem<QQuickItem>(window->rootObject(), "textinput2");
    QVERIFY(textinput2);
    QQuickItem *textedit2 = findItem<QQuickItem>(window->rootObject(), "textedit2");
    QVERIFY(textedit2);

    QVERIFY(!textinput1->hasActiveFocus());
    textinput1->forceActiveFocus();
    QVERIFY(textinput1->hasActiveFocus());

    // Tab: textinput1->textinput2
    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        QVERIFY(textinput2->hasActiveFocus());
    }

    // Tab: textinput2->textedit1
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        QVERIFY(textedit1->hasActiveFocus());
    }

    // BackTab: textedit1->textinput2
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        QVERIFY(textinput2->hasActiveFocus());
    }

    // BackTab: textinput2->textinput1
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        QVERIFY(textinput1->hasActiveFocus());
    }

    // BackTab: textinput1->textedit2
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        QVERIFY(textedit2->hasActiveFocus());
    }

    // BackTab: textedit2->textedit1
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        QVERIFY(textedit1->hasActiveFocus());
    }

    // BackTab: textedit1->textinput2
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        QVERIFY(textinput2->hasActiveFocus());
    }

    delete window;
}

void tst_QQuickItem::activeFocusOnTab_infiniteLoop_data()
{
    QTest::addColumn<QUrl>("source");
    QTest::newRow("infiniteLoop") << testFileUrl("activeFocusOnTab_infiniteLoop.qml");  // QTBUG-68271
    QTest::newRow("infiniteLoop2") << testFileUrl("activeFocusOnTab_infiniteLoop2.qml");// QTBUG-81510
}

void tst_QQuickItem::activeFocusOnTab_infiniteLoop()
{
    QFETCH(QUrl, source);

    // create a window where the currently focused item is not visible
    QScopedPointer<QQuickView>window(new QQuickView());
    window->setSource(source);
    window->show();
    auto *hiddenChild = findItem<QQuickItem>(window->rootObject(), "hiddenChild");
    QVERIFY(hiddenChild);

    // move the focus - this used to result in an infinite loop
    auto *item = hiddenChild->nextItemInFocusChain();
    // focus is moved to the root object since there is no other candidate
    QCOMPARE(item, window->rootObject());
    item = hiddenChild->nextItemInFocusChain(false);
    QCOMPARE(item, window->rootObject());
}


void tst_QQuickItem::activeFocusOnTab_infiniteLoopControls()
{
    auto source = testFileUrl("activeFocusOnTab_infiniteLoop3.qml");
    QScopedPointer<QQuickView>window(new QQuickView());
    window->setSource(source);
    window->show();
    QVERIFY(window->errors().isEmpty());
    QTest::keyClick(window.get(), Qt::Key_Tab); // should not hang
}

void tst_QQuickItem::nextItemInFocusChain()
{
    if (!qt_tab_all_widgets())
        QSKIP("This function doesn't support NOT iterating all.");

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("activeFocusOnTab.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QQuickItem *button11 = findItem<QQuickItem>(window->rootObject(), "button11");
    QVERIFY(button11);
    QQuickItem *button12 = findItem<QQuickItem>(window->rootObject(), "button12");
    QVERIFY(button12);

    QQuickItem *sub2 = findItem<QQuickItem>(window->rootObject(), "sub2");
    QVERIFY(sub2);
    QQuickItem *button21 = findItem<QQuickItem>(window->rootObject(), "button21");
    QVERIFY(button21);
    QQuickItem *button22 = findItem<QQuickItem>(window->rootObject(), "button22");
    QVERIFY(button22);

    QQuickItem *edit = findItem<QQuickItem>(window->rootObject(), "edit");
    QVERIFY(edit);

    QQuickItem *next, *prev;

    next = button11->nextItemInFocusChain(true);
    QVERIFY(next);
    QCOMPARE(next, button12);
    prev = button11->nextItemInFocusChain(false);
    QVERIFY(prev);
    QCOMPARE(prev, edit);

    next = button12->nextItemInFocusChain();
    QVERIFY(next);
    QCOMPARE(next, sub2);
    prev = button12->nextItemInFocusChain(false);
    QVERIFY(prev);
    QCOMPARE(prev, button11);

    next = sub2->nextItemInFocusChain(true);
    QVERIFY(next);
    QCOMPARE(next, button21);
    prev = sub2->nextItemInFocusChain(false);
    QVERIFY(prev);
    QCOMPARE(prev, button12);

    next = button21->nextItemInFocusChain();
    QVERIFY(next);
    QCOMPARE(next, button22);
    prev = button21->nextItemInFocusChain(false);
    QVERIFY(prev);
    QCOMPARE(prev, sub2);

    next = button22->nextItemInFocusChain(true);
    QVERIFY(next);
    QCOMPARE(next, edit);
    prev = button22->nextItemInFocusChain(false);
    QVERIFY(prev);
    QCOMPARE(prev, button21);

    next = edit->nextItemInFocusChain();
    QVERIFY(next);
    QCOMPARE(next, button11);
    prev = edit->nextItemInFocusChain(false);
    QVERIFY(prev);
    QCOMPARE(prev, button22);

    delete window;
}

void tst_QQuickItem::nextItemInFocusChain2()
{
    if (qt_tab_all_widgets())
        QSKIP("This function doesn't support iterating all.");

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("activeFocusOnTab6.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QQuickItem *button11 = findItem<QQuickItem>(window->rootObject(), "button11");
    QVERIFY(button11);
    QQuickItem *button12 = findItem<QQuickItem>(window->rootObject(), "button12");
    QVERIFY(button12);

    QQuickItem *edit = findItem<QQuickItem>(window->rootObject(), "edit");
    QVERIFY(edit);

    QQuickItem *next, *prev;

    next = button11->nextItemInFocusChain(true);
    QVERIFY(next);
    QCOMPARE(next, button12);
    prev = button11->nextItemInFocusChain(false);
    QVERIFY(prev);
    QCOMPARE(prev, edit);

    next = button12->nextItemInFocusChain();
    QVERIFY(next);
    QCOMPARE(next, edit);
    prev = button12->nextItemInFocusChain(false);
    QVERIFY(prev);
    QCOMPARE(prev, button11);

    next = edit->nextItemInFocusChain();
    QVERIFY(next);
    QCOMPARE(next, button11);
    prev = edit->nextItemInFocusChain(false);
    QVERIFY(prev);
    QCOMPARE(prev, button12);

    delete window;
}

void tst_QQuickItem::nextItemInFocusChain3()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("nextItemInFocusChain3.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);
}

void verifyTabFocusChain(QQuickView *window, const char **focusChain, bool forward)
{
    int idx = 0;
    for (const char **objectName = focusChain; *objectName; ++objectName, ++idx) {
        const QString &descrStr = QString("idx=%1 objectName=\"%2\"").arg(idx).arg(*objectName);
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, forward ? Qt::NoModifier : Qt::ShiftModifier);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY2(key.isAccepted(), qPrintable(descrStr));

        QQuickItem *item = findItem<QQuickItem>(window->rootObject(), *objectName);
        QVERIFY2(item, qPrintable(descrStr));
        QVERIFY2(item->hasActiveFocus(), qPrintable(descrStr));
    }
}

void tst_QQuickItem::tabFence()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("tabFence.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(QGuiApplication::focusWindow() == window);
    QVERIFY(window->rootObject()->hasActiveFocus());

    const char *rootTabFocusChain[] = {
          "input1", "input2", "input3", "input1", nullptr
    };
    verifyTabFocusChain(window, rootTabFocusChain, true /* forward */);

    const char *rootBacktabFocusChain[] = {
          "input3", "input2", "input1", "input3", nullptr
    };
    verifyTabFocusChain(window, rootBacktabFocusChain, false /* forward */);

    // Give focus to input11 in fence1
    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "input11");
    item->setFocus(true);
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    const char *fence1TabFocusChain[] = {
          "input12", "input13", "input11", "input12", nullptr
    };
    verifyTabFocusChain(window, fence1TabFocusChain, true /* forward */);

    const char *fence1BacktabFocusChain[] = {
          "input11", "input13", "input12", "input11", nullptr
    };
    verifyTabFocusChain(window, fence1BacktabFocusChain, false /* forward */);
}

void tst_QQuickItem::qtbug_50516()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("qtbug_50516.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(QGuiApplication::focusWindow() == window);
    QVERIFY(window->rootObject()->hasActiveFocus());

    QQuickItem *contentItem = window->rootObject();
    QQuickItem *next = contentItem->nextItemInFocusChain(true);
    QCOMPARE(next, contentItem);
    next = contentItem->nextItemInFocusChain(false);
    QCOMPARE(next, contentItem);

    delete window;
}

void tst_QQuickItem::qtbug_50516_2_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("item1");
    QTest::addColumn<QString>("item2");

    QTest::newRow("FocusScope TabFence with one Item(focused)")
            << QStringLiteral("qtbug_50516_2_1.qml") << QStringLiteral("root") << QStringLiteral("root");
    QTest::newRow("FocusScope TabFence with one Item(unfocused)")
            << QStringLiteral("qtbug_50516_2_2.qml") << QStringLiteral("root") << QStringLiteral("root");
    QTest::newRow("FocusScope TabFence with two Items(focused)")
            << QStringLiteral("qtbug_50516_2_3.qml") << QStringLiteral("root") << QStringLiteral("root");
    QTest::newRow("FocusScope TabFence with two Items(unfocused)")
            << QStringLiteral("qtbug_50516_2_4.qml") << QStringLiteral("root") << QStringLiteral("root");
    QTest::newRow("FocusScope TabFence with one Item and one TextInput(unfocused)")
            << QStringLiteral("qtbug_50516_2_5.qml") << QStringLiteral("item1") << QStringLiteral("item1");
    QTest::newRow("FocusScope TabFence with two TextInputs(unfocused)")
            << QStringLiteral("qtbug_50516_2_6.qml") << QStringLiteral("item1") << QStringLiteral("item2");
}

void tst_QQuickItem::qtbug_50516_2()
{
    QFETCH(QString, filename);
    QFETCH(QString, item1);
    QFETCH(QString, item2);

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl(filename));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(QGuiApplication::focusWindow() == window);
    QVERIFY(window->rootObject()->hasActiveFocus());

    QQuickItem *contentItem = window->rootObject();
    QQuickItem *next = contentItem->nextItemInFocusChain(true);
    QCOMPARE(next->objectName(), item1);
    next = contentItem->nextItemInFocusChain(false);
    QCOMPARE(next->objectName(), item2);

    delete window;
}

void tst_QQuickItem::focusableItemReparentedToLoadedComponent() // QTBUG-89736
{
    QQuickView window;
    window.setSource(testFileUrl("focusableItemReparentedToLoadedComponent.qml"));
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QCOMPARE(QGuiApplication::focusWindow(), &window);
    QQuickLoader *loader = window.rootObject()->findChild<QQuickLoader *>();
    QVERIFY(loader);
    QTRY_VERIFY(loader->status() == QQuickLoader::Ready);
    QQuickTextInput *textInput = window.rootObject()->findChild<QQuickTextInput *>();
    QVERIFY(textInput);

    // click to focus
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, {10, 10});
    QTRY_VERIFY(textInput->hasActiveFocus());

    // unload and reload
    auto component = loader->sourceComponent();
    loader->resetSourceComponent();
    QTRY_VERIFY(loader->status() == QQuickLoader::Null);
    loader->setSourceComponent(component);
    QTRY_VERIFY(loader->status() == QQuickLoader::Ready);

    // click to focus again
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, {10, 10});
    QTRY_VERIFY(textInput->hasActiveFocus());
}

void tst_QQuickItem::keys()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(240,320));

    KeysTestObject *testObject = new KeysTestObject;
    window->rootContext()->setContextProperty("keysTestObject", testObject);

    window->rootContext()->setContextProperty("enableKeyHanding", QVariant(true));
    window->rootContext()->setContextProperty("forwardeeVisible", QVariant(true));

    window->setSource(testFileUrl("keystest.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QVERIFY(window->rootObject());
    QCOMPARE(window->rootObject()->property("isEnabled").toBool(), true);

    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "A", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, int(Qt::Key_A));
        QCOMPARE(testObject->mForwardedKey, int(Qt::Key_A));
        QCOMPARE(testObject->mText, QLatin1String("A"));
        QCOMPARE(testObject->mModifiers, int(Qt::NoModifier));
        QVERIFY(!key.isAccepted());
    }

    testObject->reset();

    {
        QKeyEvent key = QKeyEvent(QEvent::KeyRelease, Qt::Key_A, Qt::ShiftModifier, "A", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, int(Qt::Key_A));
        QCOMPARE(testObject->mForwardedKey, int(Qt::Key_A));
        QCOMPARE(testObject->mText, QLatin1String("A"));
        QCOMPARE(testObject->mModifiers, int(Qt::ShiftModifier));
        QVERIFY(key.isAccepted());
    }

    testObject->reset();

    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, int(Qt::Key_Return));
        QCOMPARE(testObject->mForwardedKey, int(Qt::Key_Return));
        QCOMPARE(testObject->mText, QLatin1String("Return"));
        QCOMPARE(testObject->mModifiers, int(Qt::NoModifier));
        QVERIFY(key.isAccepted());
    }

    testObject->reset();

    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_0, Qt::NoModifier, "0", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, int(Qt::Key_0));
        QCOMPARE(testObject->mForwardedKey, int(Qt::Key_0));
        QCOMPARE(testObject->mText, QLatin1String("0"));
        QCOMPARE(testObject->mModifiers, int(Qt::NoModifier));
        QVERIFY(key.isAccepted());
    }

    testObject->reset();

    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_9, Qt::NoModifier, "9", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, int(Qt::Key_9));
        QCOMPARE(testObject->mForwardedKey, int(Qt::Key_9));
        QCOMPARE(testObject->mText, QLatin1String("9"));
        QCOMPARE(testObject->mModifiers, int(Qt::NoModifier));
        QVERIFY(!key.isAccepted());
    }

    testObject->reset();

    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, int(Qt::Key_Tab));
        QCOMPARE(testObject->mForwardedKey, int(Qt::Key_Tab));
        QCOMPARE(testObject->mText, QLatin1String("Tab"));
        QCOMPARE(testObject->mModifiers, int(Qt::NoModifier));
        QVERIFY(key.isAccepted());
    }

    testObject->reset();

    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, int(Qt::Key_Backtab));
        QCOMPARE(testObject->mForwardedKey, int(Qt::Key_Backtab));
        QCOMPARE(testObject->mText, QLatin1String("Backtab"));
        QCOMPARE(testObject->mModifiers, int(Qt::NoModifier));
        QVERIFY(key.isAccepted());
    }

    testObject->reset();

    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_VolumeUp, Qt::NoModifier, 1234, 0, 0);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, int(Qt::Key_VolumeUp));
        QCOMPARE(testObject->mForwardedKey, int(Qt::Key_VolumeUp));
        QCOMPARE(testObject->mModifiers, int(Qt::NoModifier));
        QCOMPARE(testObject->mNativeScanCode, quint32(1234));
        QVERIFY(key.isAccepted());
    }

    testObject->reset();

    {
        window->rootContext()->setContextProperty("forwardeeVisible", QVariant(false));
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "A", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, int(Qt::Key_A));
        QCOMPARE(testObject->mForwardedKey, 0);
        QCOMPARE(testObject->mText, QLatin1String("A"));
        QCOMPARE(testObject->mModifiers, int(Qt::NoModifier));
        QVERIFY(!key.isAccepted());
    }

    testObject->reset();

    {
        window->rootContext()->setContextProperty("enableKeyHanding", QVariant(false));
        QCOMPARE(window->rootObject()->property("isEnabled").toBool(), false);

        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, 0);
        QVERIFY(!key.isAccepted());
    }

    {
        window->rootContext()->setContextProperty("enableKeyHanding", QVariant(true));
        QCOMPARE(window->rootObject()->property("isEnabled").toBool(), true);

        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, int(Qt::Key_Return));
        QVERIFY(key.isAccepted());
    }

    delete window;
    delete testObject;
}

#if QT_CONFIG(shortcut)

Q_DECLARE_METATYPE(QEvent::Type);
Q_DECLARE_METATYPE(QKeySequence::StandardKey);

void tst_QQuickItem::standardKeys_data()
{
    QTest::addColumn<QKeySequence::StandardKey>("standardKey");
    QTest::addColumn<QKeySequence::StandardKey>("contextProperty");
    QTest::addColumn<QEvent::Type>("eventType");
    QTest::addColumn<bool>("pressed");
    QTest::addColumn<bool>("released");

    QTest::newRow("Press: Open") << QKeySequence::Open << QKeySequence::Open << QEvent::KeyPress << true << false;
    QTest::newRow("Press: Close") << QKeySequence::Close << QKeySequence::Close << QEvent::KeyPress << true << false;
    QTest::newRow("Press: Save") << QKeySequence::Save << QKeySequence::Save << QEvent::KeyPress << true << false;
    QTest::newRow("Press: Quit") << QKeySequence::Quit << QKeySequence::Quit << QEvent::KeyPress << true << false;

    QTest::newRow("Release: New") << QKeySequence::New << QKeySequence::New << QEvent::KeyRelease << false << true;
    QTest::newRow("Release: Delete") << QKeySequence::Delete << QKeySequence::Delete << QEvent::KeyRelease << false << true;
    QTest::newRow("Release: Undo") << QKeySequence::Undo << QKeySequence::Undo << QEvent::KeyRelease << false << true;
    QTest::newRow("Release: Redo") << QKeySequence::Redo << QKeySequence::Redo << QEvent::KeyRelease << false << true;

    QTest::newRow("Mismatch: Cut") << QKeySequence::Cut << QKeySequence::Copy << QEvent::KeyPress << false << false;
    QTest::newRow("Mismatch: Copy") << QKeySequence::Copy << QKeySequence::Paste << QEvent::KeyPress << false << false;
    QTest::newRow("Mismatch: Paste") << QKeySequence::Paste << QKeySequence::Cut << QEvent::KeyRelease << false << false;
    QTest::newRow("Mismatch: Quit") << QKeySequence::Quit << QKeySequence::New << QEvent::KeyRelease << false << false;
}

void tst_QQuickItem::standardKeys()
{
    QFETCH(QKeySequence::StandardKey, standardKey);
    QFETCH(QKeySequence::StandardKey, contextProperty);
    QFETCH(QEvent::Type, eventType);
    QFETCH(bool, pressed);
    QFETCH(bool, released);

    QKeySequence keySequence(standardKey);
    if (keySequence.isEmpty())
        QSKIP("Undefined key sequence.");

    QQuickView view;
    view.rootContext()->setContextProperty("standardKey", contextProperty);
    view.setSource(testFileUrl("standardkeys.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickItem *item = qobject_cast<QQuickItem*>(view.rootObject());
    QVERIFY(item);

    const int key = keySequence[0].key();
    Qt::KeyboardModifiers modifiers = keySequence[0].keyboardModifiers();
    QKeyEvent keyEvent(eventType, key, modifiers);
    QGuiApplication::sendEvent(&view, &keyEvent);

    QCOMPARE(item->property("pressed").toBool(), pressed);
    QCOMPARE(item->property("released").toBool(), released);
}

#endif // QT_CONFIG(shortcut)

void tst_QQuickItem::keysProcessingOrder()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(240,320));

    KeysTestObject *testObject = new KeysTestObject;
    window->rootContext()->setContextProperty("keysTestObject", testObject);

    window->setSource(testFileUrl("keyspriority.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    KeyTestItem *testItem = qobject_cast<KeyTestItem*>(window->rootObject());
    QVERIFY(testItem);

    QCOMPARE(testItem->property("priorityTest").toInt(), 0);

    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "A", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, int(Qt::Key_A));
        QCOMPARE(testObject->mText, QLatin1String("A"));
        QCOMPARE(testObject->mModifiers, int(Qt::NoModifier));
        QVERIFY(key.isAccepted());
    }

    testObject->reset();

    testObject->setProcessLast(true);

    QCOMPARE(testItem->property("priorityTest").toInt(), 1);

    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "A", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, 0);
        QVERIFY(key.isAccepted());
    }

    testObject->reset();

    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_B, Qt::NoModifier, "B", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, int(Qt::Key_B));
        QCOMPARE(testObject->mText, QLatin1String("B"));
        QCOMPARE(testObject->mModifiers, int(Qt::NoModifier));
        QVERIFY(!key.isAccepted());
    }

    testObject->reset();

    {
        QKeyEvent key = QKeyEvent(QEvent::KeyRelease, Qt::Key_B, Qt::NoModifier, "B", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QCOMPARE(testObject->mKey, 0);
        QVERIFY(key.isAccepted());
    }

    delete window;
    delete testObject;
}

void tst_QQuickItem::keysim()
{
    PlatformInputContext platformInputContext;
    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = &platformInputContext;

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(240,320));

    window->setSource(testFileUrl("keysim.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QVERIFY(window->rootObject());
    QVERIFY(window->rootObject()->hasFocus() && window->rootObject()->hasActiveFocus());

    QQuickTextInput *input = window->rootObject()->findChild<QQuickTextInput*>();
    QVERIFY(input);

    QInputMethodEvent ev("Hello world!", QList<QInputMethodEvent::Attribute>());
    QGuiApplication::sendEvent(qGuiApp->focusObject(), &ev);

    QEXPECT_FAIL("", "QTBUG-24280", Continue);
    QCOMPARE(input->text(), QLatin1String("Hello world!"));

    delete window;
}

void tst_QQuickItem::keysForward()
{
    QQuickView window;
    window.setBaseSize(QSize(240,320));

    window.setSource(testFileUrl("keysforward.qml"));
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QCOMPARE(QGuiApplication::focusWindow(), &window);

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(window.rootObject());
    QVERIFY(rootItem);
    QQuickItem *sourceItem = rootItem->property("source").value<QQuickItem *>();
    QVERIFY(sourceItem);
    QQuickItem *primaryTarget = rootItem->property("primaryTarget").value<QQuickItem *>();
    QVERIFY(primaryTarget);
    QQuickItem *secondaryTarget = rootItem->property("secondaryTarget").value<QQuickItem *>();
    QVERIFY(secondaryTarget);

    // primary target accepts/consumes Key_P
    QKeyEvent pressKeyP(QEvent::KeyPress, Qt::Key_P, Qt::NoModifier, "P");
    QCoreApplication::sendEvent(sourceItem, &pressKeyP);
    QCOMPARE(rootItem->property("pressedKeys").toList(), QVariantList());
    QCOMPARE(sourceItem->property("pressedKeys").toList(), QVariantList());
    QCOMPARE(primaryTarget->property("pressedKeys").toList(), QVariantList() << Qt::Key_P);
    QCOMPARE(secondaryTarget->property("pressedKeys").toList(), QVariantList() << Qt::Key_P);
    QVERIFY(pressKeyP.isAccepted());

    QKeyEvent releaseKeyP(QEvent::KeyRelease, Qt::Key_P, Qt::NoModifier, "P");
    QCoreApplication::sendEvent(sourceItem, &releaseKeyP);
    QCOMPARE(rootItem->property("releasedKeys").toList(), QVariantList());
    QCOMPARE(sourceItem->property("releasedKeys").toList(), QVariantList());
    QCOMPARE(primaryTarget->property("releasedKeys").toList(), QVariantList() << Qt::Key_P);
    QCOMPARE(secondaryTarget->property("releasedKeys").toList(), QVariantList() << Qt::Key_P);
    QVERIFY(releaseKeyP.isAccepted());

    // secondary target accepts/consumes Key_S
    QKeyEvent pressKeyS(QEvent::KeyPress, Qt::Key_S, Qt::NoModifier, "S");
    QCoreApplication::sendEvent(sourceItem, &pressKeyS);
    QCOMPARE(rootItem->property("pressedKeys").toList(), QVariantList());
    QCOMPARE(sourceItem->property("pressedKeys").toList(), QVariantList());
    QCOMPARE(primaryTarget->property("pressedKeys").toList(), QVariantList() << Qt::Key_P);
    QCOMPARE(secondaryTarget->property("pressedKeys").toList(), QVariantList() << Qt::Key_P << Qt::Key_S);
    QVERIFY(pressKeyS.isAccepted());

    QKeyEvent releaseKeyS(QEvent::KeyRelease, Qt::Key_S, Qt::NoModifier, "S");
    QCoreApplication::sendEvent(sourceItem, &releaseKeyS);
    QCOMPARE(rootItem->property("releasedKeys").toList(), QVariantList());
    QCOMPARE(sourceItem->property("releasedKeys").toList(), QVariantList());
    QCOMPARE(primaryTarget->property("releasedKeys").toList(), QVariantList() << Qt::Key_P);
    QCOMPARE(secondaryTarget->property("releasedKeys").toList(), QVariantList() << Qt::Key_P << Qt::Key_S);
    QVERIFY(releaseKeyS.isAccepted());

    // neither target accepts/consumes Key_Q
    QKeyEvent pressKeyQ(QEvent::KeyPress, Qt::Key_Q, Qt::NoModifier, "Q");
    QCoreApplication::sendEvent(sourceItem, &pressKeyQ);
    QCOMPARE(rootItem->property("pressedKeys").toList(), QVariantList());
    QCOMPARE(sourceItem->property("pressedKeys").toList(), QVariantList() << Qt::Key_Q);
    QCOMPARE(primaryTarget->property("pressedKeys").toList(), QVariantList() << Qt::Key_P << Qt::Key_Q);
    QCOMPARE(secondaryTarget->property("pressedKeys").toList(), QVariantList() << Qt::Key_P << Qt::Key_S << Qt::Key_Q);
    QVERIFY(!pressKeyQ.isAccepted());

    QKeyEvent releaseKeyQ(QEvent::KeyRelease, Qt::Key_Q, Qt::NoModifier, "Q");
    QCoreApplication::sendEvent(sourceItem, &releaseKeyQ);
    QCOMPARE(rootItem->property("releasedKeys").toList(), QVariantList());
    QCOMPARE(sourceItem->property("releasedKeys").toList(), QVariantList() << Qt::Key_Q);
    QCOMPARE(primaryTarget->property("releasedKeys").toList(), QVariantList() << Qt::Key_P << Qt::Key_Q);
    QCOMPARE(secondaryTarget->property("releasedKeys").toList(), QVariantList() << Qt::Key_P << Qt::Key_S << Qt::Key_Q);
    QVERIFY(!releaseKeyQ.isAccepted());
}

QQuickItemPrivate *childPrivate(QQuickItem *rootItem, const char * itemString)
{
    QQuickItem *item = findItem<QQuickItem>(rootItem, QString(QLatin1String(itemString)));
    QQuickItemPrivate* itemPrivate = QQuickItemPrivate::get(item);
    return itemPrivate;
}

QVariant childProperty(QQuickItem *rootItem, const char * itemString, const char * property)
{
    QQuickItem *item = findItem<QQuickItem>(rootItem, QString(QLatin1String(itemString)));
    return item->property(property);
}

bool anchorsMirrored(QQuickItem *rootItem, const char * itemString)
{
    QQuickItem *item = findItem<QQuickItem>(rootItem, QString(QLatin1String(itemString)));
    QQuickItemPrivate* itemPrivate = QQuickItemPrivate::get(item);
    return itemPrivate->anchors()->mirrored();
}

void tst_QQuickItem::layoutMirroring()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setSource(testFileUrl("layoutmirroring.qml"));
    window->show();

    QQuickItem *rootItem = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(rootItem);
    QQuickItemPrivate *rootPrivate = QQuickItemPrivate::get(rootItem);
    QVERIFY(rootPrivate);

    QVERIFY(childPrivate(rootItem, "mirrored1")->effectiveLayoutMirror);
    QVERIFY(childPrivate(rootItem, "mirrored2")->effectiveLayoutMirror);
    QVERIFY(!childPrivate(rootItem, "notMirrored1")->effectiveLayoutMirror);
    QVERIFY(!childPrivate(rootItem, "notMirrored2")->effectiveLayoutMirror);
    QVERIFY(childPrivate(rootItem, "inheritedMirror1")->effectiveLayoutMirror);
    QVERIFY(childPrivate(rootItem, "inheritedMirror2")->effectiveLayoutMirror);

    QVERIFY(anchorsMirrored(rootItem, "mirrored1"));
    QVERIFY(anchorsMirrored(rootItem, "mirrored2"));
    QVERIFY(!anchorsMirrored(rootItem, "notMirrored1"));
    QVERIFY(!anchorsMirrored(rootItem, "notMirrored2"));
    QVERIFY(anchorsMirrored(rootItem, "inheritedMirror1"));
    QVERIFY(anchorsMirrored(rootItem, "inheritedMirror2"));

    QVERIFY(childPrivate(rootItem, "mirrored1")->inheritedLayoutMirror);
    QVERIFY(!childPrivate(rootItem, "mirrored2")->inheritedLayoutMirror);
    QVERIFY(childPrivate(rootItem, "notMirrored1")->inheritedLayoutMirror);
    QVERIFY(!childPrivate(rootItem, "notMirrored2")->inheritedLayoutMirror);
    QVERIFY(childPrivate(rootItem, "inheritedMirror1")->inheritedLayoutMirror);
    QVERIFY(childPrivate(rootItem, "inheritedMirror2")->inheritedLayoutMirror);

    QVERIFY(!childPrivate(rootItem, "mirrored1")->isMirrorImplicit);
    QVERIFY(!childPrivate(rootItem, "mirrored2")->isMirrorImplicit);
    QVERIFY(!childPrivate(rootItem, "notMirrored1")->isMirrorImplicit);
    QVERIFY(childPrivate(rootItem, "notMirrored2")->isMirrorImplicit);
    QVERIFY(childPrivate(rootItem, "inheritedMirror1")->isMirrorImplicit);
    QVERIFY(childPrivate(rootItem, "inheritedMirror2")->isMirrorImplicit);

    QVERIFY(childPrivate(rootItem, "mirrored1")->inheritMirrorFromParent);
    QVERIFY(!childPrivate(rootItem, "mirrored2")->inheritMirrorFromParent);
    QVERIFY(childPrivate(rootItem, "notMirrored1")->inheritMirrorFromParent);
    QVERIFY(!childPrivate(rootItem, "notMirrored2")->inheritMirrorFromParent);
    QVERIFY(childPrivate(rootItem, "inheritedMirror1")->inheritMirrorFromParent);
    QVERIFY(childPrivate(rootItem, "inheritedMirror2")->inheritMirrorFromParent);

    QVERIFY(childPrivate(rootItem, "mirrored1")->inheritMirrorFromItem);
    QVERIFY(!childPrivate(rootItem, "mirrored2")->inheritMirrorFromItem);
    QVERIFY(!childPrivate(rootItem, "notMirrored1")->inheritMirrorFromItem);
    QVERIFY(!childPrivate(rootItem, "notMirrored2")->inheritMirrorFromItem);
    QVERIFY(!childPrivate(rootItem, "inheritedMirror1")->inheritMirrorFromItem);
    QVERIFY(!childPrivate(rootItem, "inheritedMirror2")->inheritMirrorFromItem);

    // load dynamic content using Loader that needs to inherit mirroring
    rootItem->setProperty("state", "newContent");
    QVERIFY(!childPrivate(rootItem, "notMirrored3")->effectiveLayoutMirror);
    QVERIFY(childPrivate(rootItem, "inheritedMirror3")->effectiveLayoutMirror);

    QVERIFY(childPrivate(rootItem, "notMirrored3")->inheritedLayoutMirror);
    QVERIFY(childPrivate(rootItem, "inheritedMirror3")->inheritedLayoutMirror);

    QVERIFY(!childPrivate(rootItem, "notMirrored3")->isMirrorImplicit);
    QVERIFY(childPrivate(rootItem, "inheritedMirror3")->isMirrorImplicit);

    QVERIFY(childPrivate(rootItem, "notMirrored3")->inheritMirrorFromParent);
    QVERIFY(childPrivate(rootItem, "inheritedMirror3")->inheritMirrorFromParent);

    QVERIFY(!childPrivate(rootItem, "notMirrored3")->inheritMirrorFromItem);
    QVERIFY(!childPrivate(rootItem, "notMirrored3")->inheritMirrorFromItem);

    // disable inheritance
    rootItem->setProperty("childrenInherit", false);

    QVERIFY(!childPrivate(rootItem, "inheritedMirror1")->effectiveLayoutMirror);
    QVERIFY(!childPrivate(rootItem, "inheritedMirror2")->effectiveLayoutMirror);
    QVERIFY(childPrivate(rootItem, "mirrored1")->effectiveLayoutMirror);
    QVERIFY(!childPrivate(rootItem, "notMirrored1")->effectiveLayoutMirror);

    QVERIFY(!childPrivate(rootItem, "inheritedMirror1")->inheritedLayoutMirror);
    QVERIFY(!childPrivate(rootItem, "inheritedMirror2")->inheritedLayoutMirror);
    QVERIFY(!childPrivate(rootItem, "mirrored1")->inheritedLayoutMirror);
    QVERIFY(!childPrivate(rootItem, "notMirrored1")->inheritedLayoutMirror);

    // re-enable inheritance
    rootItem->setProperty("childrenInherit", true);

    QVERIFY(childPrivate(rootItem, "inheritedMirror1")->effectiveLayoutMirror);
    QVERIFY(childPrivate(rootItem, "inheritedMirror2")->effectiveLayoutMirror);
    QVERIFY(childPrivate(rootItem, "mirrored1")->effectiveLayoutMirror);
    QVERIFY(!childPrivate(rootItem, "notMirrored1")->effectiveLayoutMirror);

    QVERIFY(childPrivate(rootItem, "inheritedMirror1")->inheritedLayoutMirror);
    QVERIFY(childPrivate(rootItem, "inheritedMirror2")->inheritedLayoutMirror);
    QVERIFY(childPrivate(rootItem, "mirrored1")->inheritedLayoutMirror);
    QVERIFY(childPrivate(rootItem, "notMirrored1")->inheritedLayoutMirror);

    //
    // dynamic parenting
    //
    QQuickItem *parentItem1 = new QQuickItem();
    QQuickItemPrivate::get(parentItem1)->effectiveLayoutMirror = true; // LayoutMirroring.enabled: true
    QQuickItemPrivate::get(parentItem1)->isMirrorImplicit = false;
    QQuickItemPrivate::get(parentItem1)->inheritMirrorFromItem = true; // LayoutMirroring.childrenInherit: true
    QQuickItemPrivate::get(parentItem1)->resolveLayoutMirror();

    // inherit in constructor
    QQuickItem *childItem1 = new QQuickItem(parentItem1);
    QVERIFY(QQuickItemPrivate::get(childItem1)->effectiveLayoutMirror);
    QVERIFY(QQuickItemPrivate::get(childItem1)->inheritMirrorFromParent);

    // inherit through a parent change
    QQuickItem *childItem2 = new QQuickItem();
    QVERIFY(!QQuickItemPrivate::get(childItem2)->effectiveLayoutMirror);
    QVERIFY(!QQuickItemPrivate::get(childItem2)->inheritMirrorFromParent);
    childItem2->setParentItem(parentItem1);
    QVERIFY(QQuickItemPrivate::get(childItem2)->effectiveLayoutMirror);
    QVERIFY(QQuickItemPrivate::get(childItem2)->inheritMirrorFromParent);

    // stop inherting through a parent change
    QQuickItem *parentItem2 = new QQuickItem();
    QQuickItemPrivate::get(parentItem2)->effectiveLayoutMirror = true; // LayoutMirroring.enabled: true
    QQuickItemPrivate::get(parentItem2)->resolveLayoutMirror();
    childItem2->setParentItem(parentItem2);
    QVERIFY(!QQuickItemPrivate::get(childItem2)->effectiveLayoutMirror);
    QVERIFY(!QQuickItemPrivate::get(childItem2)->inheritMirrorFromParent);

    delete parentItem1;
    delete parentItem2;
}

void tst_QQuickItem::layoutMirroringWindow()
{
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("layoutmirroring_window.qml"));
    QScopedPointer<QObject> object(component.create());
    QQuickWindow *window = qobject_cast<QQuickWindow *>(object.data());
    QVERIFY(window);
    window->show();

    QQuickItemPrivate *content = QQuickItemPrivate::get(window->contentItem());
    QVERIFY(content->effectiveLayoutMirror);
    QVERIFY(content->inheritedLayoutMirror);
    QVERIFY(!content->isMirrorImplicit);
    QVERIFY(content->inheritMirrorFromParent);
    QVERIFY(content->inheritMirrorFromItem);
}

void tst_QQuickItem::layoutMirroringIllegalParent()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; QtObject { LayoutMirroring.enabled: true; LayoutMirroring.childrenInherit: true }", QUrl::fromLocalFile(""));
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>:1:21: QML QtObject: LayoutDirection attached property only works with Items and Windows");
    QObject *object = component.create();
    QVERIFY(object != nullptr);
}

void tst_QQuickItem::keyNavigation_data()
{
    QTest::addColumn<QString>("source");
    QTest::newRow("KeyNavigation") << QStringLiteral("keynavigationtest.qml");
    QTest::newRow("KeyNavigation_FocusScope") << QStringLiteral("keynavigationtest_focusscope.qml");
}

void tst_QQuickItem::keyNavigation()
{
    QFETCH(QString, source);

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(240,320));

    window->setSource(testFileUrl(source));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(window->rootObject(), "verify",
            Q_RETURN_ARG(QVariant, result)));
    QVERIFY(result.toBool());

    // right
    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item2");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // down
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item4");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // left
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item3");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // up
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item1");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // tab
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item2");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // backtab
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item1");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    delete window;
}

void tst_QQuickItem::keyNavigation_RightToLeft()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(240,320));

    window->setSource(testFileUrl("keynavigationtest.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QQuickItem *rootItem = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(rootItem);
    QQuickItemPrivate* rootItemPrivate = QQuickItemPrivate::get(rootItem);

    rootItemPrivate->effectiveLayoutMirror = true; // LayoutMirroring.mirror: true
    rootItemPrivate->isMirrorImplicit = false;
    rootItemPrivate->inheritMirrorFromItem = true; // LayoutMirroring.inherit: true
    rootItemPrivate->resolveLayoutMirror();

    QEvent wa(QEvent::WindowActivate);
    QGuiApplication::sendEvent(window, &wa);
    QFocusEvent fe(QEvent::FocusIn);
    QGuiApplication::sendEvent(window, &fe);

    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(window->rootObject(), "verify",
            Q_RETURN_ARG(QVariant, result)));
    QVERIFY(result.toBool());

    // right
    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item2");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // left
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item1");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    delete window;
}

void tst_QQuickItem::keyNavigation_skipNotVisible()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(240,320));

    window->setSource(testFileUrl("keynavigationtest.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Set item 2 to not visible
    item = findItem<QQuickItem>(window->rootObject(), "item2");
    QVERIFY(item);
    item->setVisible(false);
    QVERIFY(!item->isVisible());

    // right
    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item1");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // tab
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item3");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // backtab
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item1");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    //Set item 3 to not visible
    item = findItem<QQuickItem>(window->rootObject(), "item3");
    QVERIFY(item);
    item->setVisible(false);
    QVERIFY(!item->isVisible());

    // tab
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item4");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // backtab
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item1");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    delete window;
}

void tst_QQuickItem::keyNavigation_implicitSetting()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(240,320));

    window->setSource(testFileUrl("keynavigationtest_implicit.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QEvent wa(QEvent::WindowActivate);
    QGuiApplication::sendEvent(window, &wa);
    QFocusEvent fe(QEvent::FocusIn);
    QGuiApplication::sendEvent(window, &fe);

    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(window->rootObject(), "verify",
            Q_RETURN_ARG(QVariant, result)));
    QVERIFY(result.toBool());

    // right
    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item2");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // back to item1
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item1");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // down
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item3");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // move to item4
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item4");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // left
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item3");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // back to item4
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item4");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // up
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item2");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // back to item4
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item4");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // tab
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item1");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // back to item4
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item4");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    // backtab
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());

        item = findItem<QQuickItem>(window->rootObject(), "item3");
        QVERIFY(item);
        QVERIFY(item->hasActiveFocus());
    }

    delete window;
}

// QTBUG-75399
void tst_QQuickItem::keyNavigation_implicitDestroy()
{
    QQuickView view;
    view.setSource(testFileUrl("keynavigationtest_implicitDestroy.qml"));
    view.show();

    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickItem *root = view.rootObject();
    QVERIFY(QMetaObject::invokeMethod(root, "createImplicitKeyNavigation"));

    // process events is necessary to trigger upcoming memory access violation
    QTest::qWait(0);

    QVERIFY(root->hasActiveFocus());

    QKeyEvent keyPress = QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(&view, &keyPress); // <-- access violation happens here
    // this should fail the test, even if the access violation does not occur
    QVERIFY(!keyPress.isAccepted());
}

void tst_QQuickItem::keyNavigation_focusReason()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(240,320));

    FocusEventFilter focusEventFilter;

    window->setSource(testFileUrl("keynavigationtest.qml"));
    window->show();
    window->requestActivate();

    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    // install event filter on first item
    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());
    item->installEventFilter(&focusEventFilter);

    //install event filter on second item
    item = findItem<QQuickItem>(window->rootObject(), "item2");
    QVERIFY(item);
    item->installEventFilter(&focusEventFilter);

    //install event filter on third item
    item = findItem<QQuickItem>(window->rootObject(), "item3");
    QVERIFY(item);
    item->installEventFilter(&focusEventFilter);

    //install event filter on last item
    item = findItem<QQuickItem>(window->rootObject(), "item4");
    QVERIFY(item);
    item->installEventFilter(&focusEventFilter);

    // tab
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());
        QCOMPARE(focusEventFilter.lastFocusReason, Qt::TabFocusReason);
    }

    // backtab
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());
        QCOMPARE(focusEventFilter.lastFocusReason, Qt::BacktabFocusReason);
    }

    // right - it's also one kind of key navigation
    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());
        QCOMPARE(focusEventFilter.lastFocusReason, Qt::TabFocusReason);

        item->setFocus(true, Qt::OtherFocusReason);
        QVERIFY(item->hasActiveFocus());
        QCOMPARE(focusEventFilter.lastFocusReason, Qt::OtherFocusReason);
    }

    delete window;
}

void tst_QQuickItem::keyNavigation_loop()
{
    // QTBUG-47229
    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(240,320));

    window->setSource(testFileUrl("keynavigationtest_loop.qml"));
    window->show();
    window->requestActivate();

    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());
    QVERIFY(item->hasActiveFocus());

    delete window;
}

void tst_QQuickItem::keyNavigation_repeater()
{
    // QTBUG-83356
    QScopedPointer<QQuickView> window(new QQuickView());
    window->setBaseSize(QSize(240,320));

    window->setSource(testFileUrl("keynavigationtest_repeater.qml"));
    window->show();
    window->requestActivate();

    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(window->rootObject(), "verify",
            Q_RETURN_ARG(QVariant, result)));
    QVERIFY(result.toBool());
}

void tst_QQuickItem::smooth()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; Item { smooth: false; }", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QSignalSpy spy(item, SIGNAL(smoothChanged(bool)));

    QVERIFY(item);
    QVERIFY(!item->smooth());

    item->setSmooth(true);
    QVERIFY(item->smooth());
    QCOMPARE(spy.size(),1);
    QList<QVariant> arguments = spy.first();
    QCOMPARE(arguments.size(), 1);
    QVERIFY(arguments.at(0).toBool());

    item->setSmooth(true);
    QCOMPARE(spy.size(),1);

    item->setSmooth(false);
    QVERIFY(!item->smooth());
    QCOMPARE(spy.size(),2);
    item->setSmooth(false);
    QCOMPARE(spy.size(),2);

    delete item;
}

void tst_QQuickItem::antialiasing()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; Item { antialiasing: false; }", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QSignalSpy spy(item, SIGNAL(antialiasingChanged(bool)));

    QVERIFY(item);
    QVERIFY(!item->antialiasing());

    item->setAntialiasing(true);
    QVERIFY(item->antialiasing());
    QCOMPARE(spy.size(),1);
    QList<QVariant> arguments = spy.first();
    QCOMPARE(arguments.size(), 1);
    QVERIFY(arguments.at(0).toBool());

    item->setAntialiasing(true);
    QCOMPARE(spy.size(),1);

    item->setAntialiasing(false);
    QVERIFY(!item->antialiasing());
    QCOMPARE(spy.size(),2);
    item->setAntialiasing(false);
    QCOMPARE(spy.size(),2);

    delete item;
}

void tst_QQuickItem::clip()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\nItem { clip: false\n }", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QSignalSpy spy(item, SIGNAL(clipChanged(bool)));

    QVERIFY(item);
    QVERIFY(!item->clip());

    item->setClip(true);
    QVERIFY(item->clip());

    QList<QVariant> arguments = spy.first();
    QCOMPARE(arguments.size(), 1);
    QVERIFY(arguments.at(0).toBool());

    QCOMPARE(spy.size(),1);
    item->setClip(true);
    QCOMPARE(spy.size(),1);

    item->setClip(false);
    QVERIFY(!item->clip());
    QCOMPARE(spy.size(),2);
    item->setClip(false);
    QCOMPARE(spy.size(),2);

    delete item;
}

void tst_QQuickItem::mapCoordinates()
{
    QFETCH(int, x);
    QFETCH(int, y);

    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(300, 300));
    window->setSource(testFileUrl("mapCoordinates.qml"));
    window->show();
    qApp->processEvents();

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);
    QQuickItem *a = findItem<QQuickItem>(window->rootObject(), "itemA");
    QVERIFY(a != nullptr);
    QQuickItem *b = findItem<QQuickItem>(window->rootObject(), "itemB");
    QVERIFY(b != nullptr);

    QVariant result;

    QVERIFY(QMetaObject::invokeMethod(root, "mapAToB",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapToItem(b, QPointF(x, y)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAToBPoint",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapToItem(b, QPointF(x, y)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAFromB",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapFromItem(b, QPointF(x, y)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAFromBPoint",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapFromItem(b, QPointF(x, y)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAToNull",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapToScene(QPointF(x, y)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAFromNull",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapFromScene(QPointF(x, y)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAToGlobal",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapToGlobal(QPointF(x, y)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAToGlobalPoint",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapToGlobal(QPointF(x, y)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAFromGlobal",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapFromGlobal(QPointF(x, y)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAFromGlobalPoint",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapFromGlobal(QPointF(x, y)));

    // for orphans we are primarily testing that we don't crash.
    // when orphaned the final position is the original position of the item translated by x,y
    QVERIFY(QMetaObject::invokeMethod(root, "mapOrphanToGlobal",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), QPointF(150,150) + QPointF(x, y));

    QVERIFY(QMetaObject::invokeMethod(root, "mapOrphanFromGlobal",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), -QPointF(150,150) + QPointF(x, y));

    QRegularExpression warning1 = QRegularExpression(".*Could not convert argument 0 at.*");
    QRegularExpression warning2 = QRegularExpression(".*checkMapA.*Invalid@.*");

    QTest::ignoreMessage(QtWarningMsg, warning1);
    QTest::ignoreMessage(QtWarningMsg, warning2);
    QVERIFY(QMetaObject::invokeMethod(root, "checkMapAToInvalid",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QVERIFY(result.toBool());

    QTest::ignoreMessage(QtWarningMsg, warning1);
    QTest::ignoreMessage(QtWarningMsg, warning2);
    QVERIFY(QMetaObject::invokeMethod(root, "checkMapAFromInvalid",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QVERIFY(result.toBool());

    delete window;
}

void tst_QQuickItem::mapCoordinates_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("y");

    for (int i=-20; i<=20; i+=10)
        QTest::newRow(QTest::toString(i)) << i << i;
}

void tst_QQuickItem::mapCoordinatesRect()
{
    QFETCH(int, x);
    QFETCH(int, y);
    QFETCH(int, width);
    QFETCH(int, height);

    std::unique_ptr<QQuickView> window = std::make_unique<QQuickView>();
    window->setBaseSize(QSize(300, 300));
    window->setSource(testFileUrl("mapCoordinatesRect.qml"));
    window->show();
    qApp->processEvents();

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);
    QQuickItem *a = findItem<QQuickItem>(window->rootObject(), "itemA");
    QVERIFY(a != nullptr);
    QQuickItem *b = findItem<QQuickItem>(window->rootObject(), "itemB");
    QVERIFY(b != nullptr);

    QVariant result;

    QVERIFY(QMetaObject::invokeMethod(root, "mapAToB",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y), Q_ARG(QVariant, width), Q_ARG(QVariant, height)));
    QCOMPARE(result.value<QRectF>(), qobject_cast<QQuickItem*>(a)->mapRectToItem(b, QRectF(x, y, width, height)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAToBRect",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y), Q_ARG(QVariant, width), Q_ARG(QVariant, height)));
    QCOMPARE(result.value<QRectF>(), qobject_cast<QQuickItem*>(a)->mapRectToItem(b, QRectF(x, y, width, height)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAFromB",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y), Q_ARG(QVariant, width), Q_ARG(QVariant, height)));
    QCOMPARE(result.value<QRectF>(), qobject_cast<QQuickItem*>(a)->mapRectFromItem(b, QRectF(x, y, width, height)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAFromBRect",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y), Q_ARG(QVariant, width), Q_ARG(QVariant, height)));
    QCOMPARE(result.value<QRectF>(), qobject_cast<QQuickItem*>(a)->mapRectFromItem(b, QRectF(x, y, width, height)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAToNull",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y), Q_ARG(QVariant, width), Q_ARG(QVariant, height)));
    QCOMPARE(result.value<QRectF>(), qobject_cast<QQuickItem*>(a)->mapRectToScene(QRectF(x, y, width, height)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAFromNull",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y), Q_ARG(QVariant, width), Q_ARG(QVariant, height)));
    QCOMPARE(result.value<QRectF>(), qobject_cast<QQuickItem*>(a)->mapRectFromScene(QRectF(x, y, width, height)));

    QRegularExpression warning1 = QRegularExpression(".*Could not convert argument 0 at.*");
    QRegularExpression warning2 = QRegularExpression(".*checkMapA.*Invalid@.*");

    QTest::ignoreMessage(QtWarningMsg, warning1);
    QTest::ignoreMessage(QtWarningMsg, warning2);
    QVERIFY(QMetaObject::invokeMethod(root, "checkMapAToInvalid",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y), Q_ARG(QVariant, width), Q_ARG(QVariant, height)));
    QVERIFY(result.toBool());

    QTest::ignoreMessage(QtWarningMsg, warning1);
    QTest::ignoreMessage(QtWarningMsg, warning2);
    QVERIFY(QMetaObject::invokeMethod(root, "checkMapAFromInvalid",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y), Q_ARG(QVariant, width), Q_ARG(QVariant, height)));
    QVERIFY(result.toBool());
}

void tst_QQuickItem::mapCoordinatesRect_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("y");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");

    for (int i=-20; i<=20; i+=5)
        QTest::newRow(QTest::toString(i)) << i << i << i << i;
}

void tst_QQuickItem::transforms_data()
{
    QTest::addColumn<QByteArray>("qml");
    QTest::addColumn<QTransform>("transform");
    QTest::newRow("translate") << QByteArray("Translate { x: 10; y: 20 }")
        << QTransform(1,0,0,0,1,0,10,20,1);
    QTest::newRow("matrix4x4") << QByteArray("Matrix4x4 { matrix: Qt.matrix4x4(1,0,0,10, 0,1,0,15, 0,0,1,0, 0,0,0,1) }")
        << QTransform(1,0,0,0,1,0,10,15,1);
    QTest::newRow("rotation") << QByteArray("Rotation { angle: 90 }")
        << QTransform(0,1,0,-1,0,0,0,0,1);
    QTest::newRow("scale") << QByteArray("Scale { xScale: 1.5; yScale: -2  }")
        << QTransform(1.5,0,0,0,-2,0,0,0,1);
    QTest::newRow("sequence") << QByteArray("[ Translate { x: 10; y: 20 }, Scale { xScale: 1.5; yScale: -2  } ]")
        << QTransform(1,0,0,0,1,0,10,20,1) * QTransform(1.5,0,0,0,-2,0,0,0,1);
}

void tst_QQuickItem::transforms()
{
    QFETCH(QByteArray, qml);
    QFETCH(QTransform, transform);
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.3\nItem { transform: "+qml+"}", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(item);
    QCOMPARE(item->itemTransform(nullptr,nullptr), transform);
}

void tst_QQuickItem::childrenProperty()
{
    QQmlComponent component(&engine, testFileUrl("childrenProperty.qml"));

    QObject *o = component.create();
    QVERIFY(o != nullptr);

    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    delete o;
}

void tst_QQuickItem::resourcesProperty()
{
    QQmlComponent component(&engine, testFileUrl("resourcesProperty.qml"));

    QObject *object = component.create();
    QVERIFY(object != nullptr);

    QQmlProperty property(object, "resources", component.creationContext());

    QVERIFY(property.isValid());
    QQmlListReference list = qvariant_cast<QQmlListReference>(property.read());
    QVERIFY(list.isValid());

    QCOMPARE(list.count(), 4);

    QCOMPARE(object->property("test1").toBool(), true);
    QCOMPARE(object->property("test2").toBool(), true);
    QCOMPARE(object->property("test3").toBool(), true);
    QCOMPARE(object->property("test4").toBool(), true);
    QCOMPARE(object->property("test5").toBool(), true);
    QCOMPARE(object->property("test6").toBool(), true);

    QObject *subObject = object->findChild<QObject *>("subObject");

    QVERIFY(subObject);

    QCOMPARE(object, subObject->parent());

    delete subObject;

    QCOMPARE(list.count(), 3);

    delete object;
}

void tst_QQuickItem::bindableProperties_data()
{
    QTest::addColumn<qreal>("initialValue");
    QTest::addColumn<qreal>("newValue");
    QTest::addColumn<QString>("property");

    // can't simply use 3. or 3.0 for the numbers as qreal might
    // be float instead of double...
    QTest::addRow("x") << qreal(3) << qreal(14) << "x";
    QTest::addRow("y") << qreal(10) << qreal(20) << "y";
    QTest::addRow("width") << qreal(100) << qreal(200) << "width";
    QTest::addRow("height") << qreal(50) << qreal(40) << "height";
}

void tst_QQuickItem::bindableProperties()
{
    QQuickItem item;
    QFETCH(qreal, initialValue);
    QFETCH(qreal, newValue);
    QFETCH(QString, property);

    QTestPrivate::testReadWritePropertyBasics(item, initialValue, newValue, property.toUtf8().constData());
}

void tst_QQuickItem::propertyChanges()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(300, 300));
    window->setSource(testFileUrl("propertychanges.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "item");
    QQuickItem *parentItem = findItem<QQuickItem>(window->rootObject(), "parentItem");

    QVERIFY(item);
    QVERIFY(parentItem);

    QSignalSpy parentSpy(item, SIGNAL(parentChanged(QQuickItem*)));
    QSignalSpy widthSpy(item, SIGNAL(widthChanged()));
    QSignalSpy heightSpy(item, SIGNAL(heightChanged()));
    QSignalSpy baselineOffsetSpy(item, SIGNAL(baselineOffsetChanged(qreal)));
    QSignalSpy childrenRectSpy(parentItem, SIGNAL(childrenRectChanged(QRectF)));
    QSignalSpy focusSpy(item, SIGNAL(focusChanged(bool)));
    QSignalSpy wantsFocusSpy(parentItem, SIGNAL(activeFocusChanged(bool)));
    QSignalSpy childrenChangedSpy(parentItem, SIGNAL(childrenChanged()));
    QSignalSpy xSpy(item, SIGNAL(xChanged()));
    QSignalSpy ySpy(item, SIGNAL(yChanged()));

    item->setParentItem(parentItem);
    item->setWidth(100.0);
    item->setHeight(200.0);
    item->setFocus(true);
    item->setBaselineOffset(10.0);

    QCOMPARE(item->parentItem(), parentItem);
    QCOMPARE(parentSpy.size(),1);
    QList<QVariant> parentArguments = parentSpy.first();
    QCOMPARE(parentArguments.size(), 1);
    QCOMPARE(item->parentItem(), qvariant_cast<QQuickItem *>(parentArguments.at(0)));
    QCOMPARE(childrenChangedSpy.size(),1);

    item->setParentItem(parentItem);
    QCOMPARE(childrenChangedSpy.size(),1);

    QCOMPARE(item->width(), 100.0);
    QCOMPARE(widthSpy.size(),1);

    QCOMPARE(item->height(), 200.0);
    QCOMPARE(heightSpy.size(),1);

    QCOMPARE(item->baselineOffset(), 10.0);
    QCOMPARE(baselineOffsetSpy.size(),1);
    QList<QVariant> baselineOffsetArguments = baselineOffsetSpy.first();
    QCOMPARE(baselineOffsetArguments.size(), 1);
    QCOMPARE(item->baselineOffset(), baselineOffsetArguments.at(0).toReal());

    QCOMPARE(parentItem->childrenRect(), QRectF(0.0,0.0,100.0,200.0));
    QCOMPARE(childrenRectSpy.size(),1);
    QList<QVariant> childrenRectArguments = childrenRectSpy.at(0);
    QCOMPARE(childrenRectArguments.size(), 1);
    QCOMPARE(parentItem->childrenRect(), childrenRectArguments.at(0).toRectF());

    QCOMPARE(item->hasActiveFocus(), true);
    QCOMPARE(focusSpy.size(),1);
    QList<QVariant> focusArguments = focusSpy.first();
    QCOMPARE(focusArguments.size(), 1);
    QCOMPARE(focusArguments.at(0).toBool(), true);

    QCOMPARE(parentItem->hasActiveFocus(), false);
    QCOMPARE(parentItem->hasFocus(), false);
    QCOMPARE(wantsFocusSpy.size(),0);

    item->setX(10.0);
    QCOMPARE(item->x(), 10.0);
    QCOMPARE(xSpy.size(), 1);

    item->setY(10.0);
    QCOMPARE(item->y(), 10.0);
    QCOMPARE(ySpy.size(), 1);

    delete window;
}

void tst_QQuickItem::nonexistentPropertyConnection()
{
    // QTBUG-56551: don't crash
    QQmlComponent component(&engine, testFileUrl("nonexistentPropertyConnection.qml"));
    QObject *o = component.create();
    QVERIFY(o);
    delete o;
}

void tst_QQuickItem::childrenRect()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setSource(testFileUrl("childrenRect.qml"));
    window->setBaseSize(QSize(240,320));
    window->show();

    QQuickItem *o = window->rootObject();
    QQuickItem *item = o->findChild<QQuickItem*>("testItem");
    QCOMPARE(item->width(), qreal(0));
    QCOMPARE(item->height(), qreal(0));

    o->setProperty("childCount", 1);
    QCOMPARE(item->width(), qreal(10));
    QCOMPARE(item->height(), qreal(20));

    o->setProperty("childCount", 5);
    QCOMPARE(item->width(), qreal(50));
    QCOMPARE(item->height(), qreal(100));

    o->setProperty("childCount", 0);
    QCOMPARE(item->width(), qreal(0));
    QCOMPARE(item->height(), qreal(0));

    delete o;
    delete window;
}

// QTBUG-11383
void tst_QQuickItem::childrenRectBug()
{
    QQuickView *window = new QQuickView(nullptr);

    QString warning = testFileUrl("childrenRectBug.qml").toString() + ":11:9: QML Item: Binding loop detected for property \"height\"";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));

    window->setSource(testFileUrl("childrenRectBug.qml"));
    window->show();

    QQuickItem *o = window->rootObject();
    QQuickItem *item = o->findChild<QQuickItem*>("theItem");
    QCOMPARE(item->width(), qreal(200));
    QCOMPARE(item->height(), qreal(100));
    QCOMPARE(item->x(), qreal(100));

    delete window;
}

// QTBUG-11465
void tst_QQuickItem::childrenRectBug2()
{
    QQuickView *window = new QQuickView(nullptr);

    QString warning1 = testFileUrl("childrenRectBug2.qml").toString() + ":10:9: QML Item: Binding loop detected for property \"width\"";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));

    QString warning2 = testFileUrl("childrenRectBug2.qml").toString() + ":11:9: QML Item: Binding loop detected for property \"height\"";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    window->setSource(testFileUrl("childrenRectBug2.qml"));
    window->show();

    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(window->rootObject());
    QVERIFY(rect);
    QQuickItem *item = rect->findChild<QQuickItem*>("theItem");
    QCOMPARE(item->width(), qreal(100));
    QCOMPARE(item->height(), qreal(110));
    QCOMPARE(item->x(), qreal(130));

    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect);
    rectPrivate->setState("row");
    QCOMPARE(item->width(), qreal(210));
    QCOMPARE(item->height(), qreal(50));
    QCOMPARE(item->x(), qreal(75));

    delete window;
}

// QTBUG-12722
void tst_QQuickItem::childrenRectBug3()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setSource(testFileUrl("childrenRectBug3.qml"));
    window->show();

    //don't crash on delete
    delete window;
}

// QTBUG-38732
void tst_QQuickItem::childrenRectBottomRightCorner()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setSource(testFileUrl("childrenRectBottomRightCorner.qml"));
    window->show();

    QQuickItem *rect = window->rootObject()->findChild<QQuickItem*>("childrenRectProxy");
    QCOMPARE(rect->x(), qreal(-100));
    QCOMPARE(rect->y(), qreal(-100));
    QCOMPARE(rect->width(), qreal(50));
    QCOMPARE(rect->height(), qreal(50));

    delete window;
}

struct TestListener : public QQuickItemChangeListener
{
    TestListener(bool remove = false) : remove(remove) { }

    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange, const QRectF &oldGeometry) override
    {
        record(item, QQuickItemPrivate::Geometry, oldGeometry);
    }
    void itemSiblingOrderChanged(QQuickItem *item) override
    {
        record(item, QQuickItemPrivate::SiblingOrder);
    }
    void itemVisibilityChanged(QQuickItem *item) override
    {
        record(item, QQuickItemPrivate::Visibility);
    }
    void itemOpacityChanged(QQuickItem *item) override
    {
        record(item, QQuickItemPrivate::Opacity);
    }
    void itemRotationChanged(QQuickItem *item) override
    {
        record(item, QQuickItemPrivate::Rotation);
    }
    void itemImplicitWidthChanged(QQuickItem *item) override
    {
        record(item, QQuickItemPrivate::ImplicitWidth);
    }
    void itemImplicitHeightChanged(QQuickItem *item) override
    {
        record(item, QQuickItemPrivate::ImplicitHeight);
    }
    void itemDestroyed(QQuickItem *item) override
    {
        record(item, QQuickItemPrivate::Destroyed);
    }
    void itemChildAdded(QQuickItem *item, QQuickItem *child) override
    {
        record(item, QQuickItemPrivate::Children, QVariant::fromValue(child));
    }
    void itemChildRemoved(QQuickItem *item, QQuickItem *child) override
    {
        record(item, QQuickItemPrivate::Children, QVariant::fromValue(child));
    }
    void itemParentChanged(QQuickItem *item, QQuickItem *parent) override
    {
        record(item, QQuickItemPrivate::Parent, QVariant::fromValue(parent));
    }

    QQuickAnchorsPrivate *anchorPrivate() override { return nullptr; }

    void record(QQuickItem *item, QQuickItemPrivate::ChangeType change, const QVariant &value = QVariant())
    {
        changes += change;
        values[change] = value;
        // QTBUG-54732
        if (remove)
            QQuickItemPrivate::get(item)->removeItemChangeListener(this, change);
    }

    int count(QQuickItemPrivate::ChangeType change) const
    {
        return changes.count(change);
    }

    QVariant value(QQuickItemPrivate::ChangeType change) const
    {
        return values.value(change);
    }

    bool remove;
    QList<QQuickItemPrivate::ChangeType> changes;
    QHash<QQuickItemPrivate::ChangeType, QVariant> values;
};

void tst_QQuickItem::changeListener()
{
    QQuickWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQuickItem *item = new QQuickItem;
    TestListener itemListener;
    QQuickItemPrivate::get(item)->addItemChangeListener(&itemListener, QQuickItemPrivate::ChangeTypes(0xffff));

    item->setImplicitWidth(10);
    QCOMPARE(itemListener.count(QQuickItemPrivate::ImplicitWidth), 1);
    QCOMPARE(itemListener.count(QQuickItemPrivate::Geometry), 1);
    QCOMPARE(itemListener.value(QQuickItemPrivate::Geometry), QVariant(QRectF(0,0,0,0)));

    item->setImplicitHeight(20);
    QCOMPARE(itemListener.count(QQuickItemPrivate::ImplicitHeight), 1);
    QCOMPARE(itemListener.count(QQuickItemPrivate::Geometry), 2);
    QCOMPARE(itemListener.value(QQuickItemPrivate::Geometry), QVariant(QRectF(0,0,10,0)));

    item->setWidth(item->width() + 30);
    QCOMPARE(itemListener.count(QQuickItemPrivate::Geometry), 3);
    QCOMPARE(itemListener.value(QQuickItemPrivate::Geometry), QVariant(QRectF(0,0,10,20)));

    item->setHeight(item->height() + 40);
    QCOMPARE(itemListener.count(QQuickItemPrivate::Geometry), 4);
    QCOMPARE(itemListener.value(QQuickItemPrivate::Geometry), QVariant(QRectF(0,0,40,20)));

    item->setOpacity(0.5);
    QCOMPARE(itemListener.count(QQuickItemPrivate::Opacity), 1);

    item->setRotation(90);
    QCOMPARE(itemListener.count(QQuickItemPrivate::Rotation), 1);

    item->setParentItem(window.contentItem());
    QCOMPARE(itemListener.count(QQuickItemPrivate::Parent), 1);

    item->setVisible(false);
    QCOMPARE(itemListener.count(QQuickItemPrivate::Visibility), 1);

    QQuickItemPrivate::get(item)->removeItemChangeListener(&itemListener, QQuickItemPrivate::ChangeTypes(0xffff));

    QQuickItem *parent = new QQuickItem(window.contentItem());
    TestListener parentListener;
    QQuickItemPrivate::get(parent)->addItemChangeListener(&parentListener, QQuickItemPrivate::Children);

    QQuickItem *child1 = new QQuickItem;
    QQuickItem *child2 = new QQuickItem;
    TestListener child1Listener;
    TestListener child2Listener;
    QQuickItemPrivate::get(child1)->addItemChangeListener(&child1Listener, QQuickItemPrivate::Parent | QQuickItemPrivate::SiblingOrder | QQuickItemPrivate::Destroyed);
    QQuickItemPrivate::get(child2)->addItemChangeListener(&child2Listener, QQuickItemPrivate::Parent | QQuickItemPrivate::SiblingOrder | QQuickItemPrivate::Destroyed);

    child1->setParentItem(parent);
    QCOMPARE(parentListener.count(QQuickItemPrivate::Children), 1);
    QCOMPARE(parentListener.value(QQuickItemPrivate::Children), QVariant::fromValue(child1));
    QCOMPARE(child1Listener.count(QQuickItemPrivate::Parent), 1);
    QCOMPARE(child1Listener.value(QQuickItemPrivate::Parent), QVariant::fromValue(parent));

    child2->setParentItem(parent);
    QCOMPARE(parentListener.count(QQuickItemPrivate::Children), 2);
    QCOMPARE(parentListener.value(QQuickItemPrivate::Children), QVariant::fromValue(child2));
    QCOMPARE(child2Listener.count(QQuickItemPrivate::Parent), 1);
    QCOMPARE(child2Listener.value(QQuickItemPrivate::Parent), QVariant::fromValue(parent));

    child2->stackBefore(child1);
    QCOMPARE(child1Listener.count(QQuickItemPrivate::SiblingOrder), 1);
    QCOMPARE(child2Listener.count(QQuickItemPrivate::SiblingOrder), 1);

    child1->setParentItem(nullptr);
    QCOMPARE(parentListener.count(QQuickItemPrivate::Children), 3);
    QCOMPARE(parentListener.value(QQuickItemPrivate::Children), QVariant::fromValue(child1));
    QCOMPARE(child1Listener.count(QQuickItemPrivate::Parent), 2);
    QCOMPARE(child1Listener.value(QQuickItemPrivate::Parent), QVariant::fromValue<QQuickItem *>(nullptr));

    delete child1;
    QCOMPARE(child1Listener.count(QQuickItemPrivate::Destroyed), 1);

    delete child2;
    QCOMPARE(parentListener.count(QQuickItemPrivate::Children), 4);
    QCOMPARE(parentListener.value(QQuickItemPrivate::Children), QVariant::fromValue(child2));
    QCOMPARE(child2Listener.count(QQuickItemPrivate::Parent), 2);
    QCOMPARE(child2Listener.value(QQuickItemPrivate::Parent), QVariant::fromValue<QQuickItem *>(nullptr));
    QCOMPARE(child2Listener.count(QQuickItemPrivate::Destroyed), 1);

    QQuickItemPrivate::get(parent)->removeItemChangeListener(&parentListener, QQuickItemPrivate::Children);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), 0);

    // QTBUG-54732: all listeners should get invoked even if they remove themselves while iterating the listeners
    QList<TestListener *> listeners;
    for (int i = 0; i < 5; ++i)
        listeners << new TestListener(true);

    // itemVisibilityChanged x 5
    foreach (TestListener *listener, listeners)
        QQuickItemPrivate::get(parent)->addItemChangeListener(listener, QQuickItemPrivate::Visibility);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), listeners.size());
    parent->setVisible(false);
    foreach (TestListener *listener, listeners)
        QCOMPARE(listener->count(QQuickItemPrivate::Visibility), 1);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), 0);

    // itemRotationChanged x 5
    foreach (TestListener *listener, listeners)
        QQuickItemPrivate::get(parent)->addItemChangeListener(listener, QQuickItemPrivate::Rotation);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), listeners.size());
    parent->setRotation(90);
    foreach (TestListener *listener, listeners)
        QCOMPARE(listener->count(QQuickItemPrivate::Rotation), 1);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), 0);

    // itemOpacityChanged x 5
    foreach (TestListener *listener, listeners)
        QQuickItemPrivate::get(parent)->addItemChangeListener(listener, QQuickItemPrivate::Opacity);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), listeners.size());
    parent->setOpacity(0.5);
    foreach (TestListener *listener, listeners)
        QCOMPARE(listener->count(QQuickItemPrivate::Opacity), 1);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), 0);

    // itemChildAdded() x 5
    foreach (TestListener *listener, listeners)
        QQuickItemPrivate::get(parent)->addItemChangeListener(listener, QQuickItemPrivate::Children);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), listeners.size());
    child1 = new QQuickItem(parent);
    foreach (TestListener *listener, listeners)
        QCOMPARE(listener->count(QQuickItemPrivate::Children), 1);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), 0);

    // itemParentChanged() x 5
    foreach (TestListener *listener, listeners)
        QQuickItemPrivate::get(child1)->addItemChangeListener(listener, QQuickItemPrivate::Parent);
    QCOMPARE(QQuickItemPrivate::get(child1)->changeListeners.size(), listeners.size());
    child1->setParentItem(nullptr);
    foreach (TestListener *listener, listeners)
        QCOMPARE(listener->count(QQuickItemPrivate::Parent), 1);
    QCOMPARE(QQuickItemPrivate::get(child1)->changeListeners.size(), 0);

    // itemImplicitWidthChanged() x 5
    foreach (TestListener *listener, listeners)
        QQuickItemPrivate::get(parent)->addItemChangeListener(listener, QQuickItemPrivate::ImplicitWidth);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), listeners.size());
    parent->setImplicitWidth(parent->implicitWidth() + 1);
    foreach (TestListener *listener, listeners)
        QCOMPARE(listener->count(QQuickItemPrivate::ImplicitWidth), 1);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), 0);

    // itemImplicitHeightChanged() x 5
    foreach (TestListener *listener, listeners)
        QQuickItemPrivate::get(parent)->addItemChangeListener(listener, QQuickItemPrivate::ImplicitHeight);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), listeners.size());
    parent->setImplicitHeight(parent->implicitHeight() + 1);
    foreach (TestListener *listener, listeners)
        QCOMPARE(listener->count(QQuickItemPrivate::ImplicitHeight), 1);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), 0);

    // itemGeometryChanged() x 5
    foreach (TestListener *listener, listeners)
        QQuickItemPrivate::get(parent)->addItemChangeListener(listener, QQuickItemPrivate::Geometry);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), listeners.size());
    parent->setWidth(parent->width() + 1);
    foreach (TestListener *listener, listeners)
        QCOMPARE(listener->count(QQuickItemPrivate::Geometry), 1);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), 0);

    // itemChildRemoved() x 5
    child1->setParentItem(parent);
    foreach (TestListener *listener, listeners)
        QQuickItemPrivate::get(parent)->addItemChangeListener(listener, QQuickItemPrivate::Children);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), listeners.size());
    delete child1;
    foreach (TestListener *listener, listeners)
        QCOMPARE(listener->count(QQuickItemPrivate::Children), 2);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), 0);

    // itemDestroyed() x 5
    foreach (TestListener *listener, listeners)
        QQuickItemPrivate::get(parent)->addItemChangeListener(listener, QQuickItemPrivate::Destroyed);
    QCOMPARE(QQuickItemPrivate::get(parent)->changeListeners.size(), listeners.size());
    delete parent;
    foreach (TestListener *listener, listeners)
        QCOMPARE(listener->count(QQuickItemPrivate::Destroyed), 1);
}

// QTBUG-13893
void tst_QQuickItem::transformCrash()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setSource(testFileUrl("transformCrash.qml"));
    window->show();

    delete window;
}

void tst_QQuickItem::implicitSize()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setSource(testFileUrl("implicitsize.qml"));
    window->show();

    QQuickItem *item = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(item);
    QCOMPARE(item->width(), qreal(80));
    QCOMPARE(item->height(), qreal(60));

    QCOMPARE(item->implicitWidth(), qreal(200));
    QCOMPARE(item->implicitHeight(), qreal(100));

    QMetaObject::invokeMethod(item, "resetSize");

    QCOMPARE(item->width(), qreal(200));
    QCOMPARE(item->height(), qreal(100));

    QMetaObject::invokeMethod(item, "changeImplicit");

    QCOMPARE(item->implicitWidth(), qreal(150));
    QCOMPARE(item->implicitHeight(), qreal(80));
    QCOMPARE(item->width(), qreal(150));
    QCOMPARE(item->height(), qreal(80));

    QMetaObject::invokeMethod(item, "assignImplicitBinding");

    QCOMPARE(item->implicitWidth(), qreal(150));
    QCOMPARE(item->implicitHeight(), qreal(80));
    QCOMPARE(item->width(), qreal(150));
    QCOMPARE(item->height(), qreal(80));

    QMetaObject::invokeMethod(item, "increaseImplicit");

    QCOMPARE(item->implicitWidth(), qreal(200));
    QCOMPARE(item->implicitHeight(), qreal(100));
    QCOMPARE(item->width(), qreal(175));
    QCOMPARE(item->height(), qreal(90));

    QMetaObject::invokeMethod(item, "changeImplicit");

    QCOMPARE(item->implicitWidth(), qreal(150));
    QCOMPARE(item->implicitHeight(), qreal(80));
    QCOMPARE(item->width(), qreal(150));
    QCOMPARE(item->height(), qreal(80));

    QMetaObject::invokeMethod(item, "assignUndefinedBinding");

    QCOMPARE(item->implicitWidth(), qreal(150));
    QCOMPARE(item->implicitHeight(), qreal(80));
    QCOMPARE(item->width(), qreal(150));
    QCOMPARE(item->height(), qreal(80));

    QMetaObject::invokeMethod(item, "increaseImplicit");

    QCOMPARE(item->implicitWidth(), qreal(200));
    QCOMPARE(item->implicitHeight(), qreal(100));
    QCOMPARE(item->width(), qreal(175));
    QCOMPARE(item->height(), qreal(90));

    QMetaObject::invokeMethod(item, "changeImplicit");

    QCOMPARE(item->implicitWidth(), qreal(150));
    QCOMPARE(item->implicitHeight(), qreal(80));
    QCOMPARE(item->width(), qreal(150));
    QCOMPARE(item->height(), qreal(80));

    delete window;
}

void tst_QQuickItem::qtbug_16871()
{
    QQmlComponent component(&engine, testFileUrl("qtbug_16871.qml"));
    QObject *o = component.create();
    QVERIFY(o != nullptr);
    delete o;
}


void tst_QQuickItem::visibleChildren()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setSource(testFileUrl("visiblechildren.qml"));
    window->show();

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root);

    QCOMPARE(root->property("test1_1").toBool(), true);
    QCOMPARE(root->property("test1_2").toBool(), true);
    QCOMPARE(root->property("test1_3").toBool(), true);
    QCOMPARE(root->property("test1_4").toBool(), true);

    QMetaObject::invokeMethod(root, "hideFirstAndLastRowChild");
    QCOMPARE(root->property("test2_1").toBool(), true);
    QCOMPARE(root->property("test2_2").toBool(), true);
    QCOMPARE(root->property("test2_3").toBool(), true);
    QCOMPARE(root->property("test2_4").toBool(), true);

    QMetaObject::invokeMethod(root, "showLastRowChildsLastChild");
    QCOMPARE(root->property("test3_1").toBool(), true);
    QCOMPARE(root->property("test3_2").toBool(), true);
    QCOMPARE(root->property("test3_3").toBool(), true);
    QCOMPARE(root->property("test3_4").toBool(), true);

    QMetaObject::invokeMethod(root, "showLastRowChild");
    QCOMPARE(root->property("test4_1").toBool(), true);
    QCOMPARE(root->property("test4_2").toBool(), true);
    QCOMPARE(root->property("test4_3").toBool(), true);
    QCOMPARE(root->property("test4_4").toBool(), true);

    QString warning1 = testFileUrl("visiblechildren.qml").toString() + ":87: TypeError: Cannot read property 'visibleChildren' of null";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QMetaObject::invokeMethod(root, "tryWriteToReadonlyVisibleChildren");

    QMetaObject::invokeMethod(root, "reparentVisibleItem3");
    QCOMPARE(root->property("test6_1").toBool(), true);
    QCOMPARE(root->property("test6_2").toBool(), true);
    QCOMPARE(root->property("test6_3").toBool(), true);
    QCOMPARE(root->property("test6_4").toBool(), true);

    QMetaObject::invokeMethod(root, "reparentImlicitlyInvisibleItem4_1");
    QCOMPARE(root->property("test7_1").toBool(), true);
    QCOMPARE(root->property("test7_2").toBool(), true);
    QCOMPARE(root->property("test7_3").toBool(), true);
    QCOMPARE(root->property("test7_4").toBool(), true);

    // FINALLY TEST THAT EVERYTHING IS AS EXPECTED
    QCOMPARE(root->property("test8_1").toBool(), true);
    QCOMPARE(root->property("test8_2").toBool(), true);
    QCOMPARE(root->property("test8_3").toBool(), true);
    QCOMPARE(root->property("test8_4").toBool(), true);
    QCOMPARE(root->property("test8_5").toBool(), true);

    delete window;
}

void tst_QQuickItem::parentLoop()
{
    QQuickView *window = new QQuickView(nullptr);

#if QT_CONFIG(regularexpression)
    QRegularExpression msgRegexp = QRegularExpression("QQuickItem::setParentItem: Parent QQuickItem\\(.*\\) is already part of the subtree of QQuickItem\\(.*\\)");
    QTest::ignoreMessage(QtWarningMsg, msgRegexp);
#endif
    window->setSource(testFileUrl("parentLoop.qml"));

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root);

    QQuickItem *item1 = root->findChild<QQuickItem*>("item1");
    QVERIFY(item1);
    QCOMPARE(item1->parentItem(), root);

    QQuickItem *item2 = root->findChild<QQuickItem*>("item2");
    QVERIFY(item2);
    QCOMPARE(item2->parentItem(), item1);

    delete window;
}

void tst_QQuickItem::contains_data()
{
    QTest::addColumn<bool>("circleTest");
    QTest::addColumn<bool>("insideTarget");
    QTest::addColumn<QList<QPoint> >("points");

    QList<QPoint> points;

    points << QPoint(176, 176)
           << QPoint(176, 226)
           << QPoint(226, 176)
           << QPoint(226, 226)
           << QPoint(150, 200)
           << QPoint(200, 150)
           << QPoint(200, 250)
           << QPoint(250, 200);
    QTest::newRow("hollow square: testing points inside") << false << true << points;

    points.clear();
    points << QPoint(162, 162)
           << QPoint(162, 242)
           << QPoint(242, 162)
           << QPoint(242, 242)
           << QPoint(200, 200)
           << QPoint(175, 200)
           << QPoint(200, 175)
           << QPoint(200, 228)
           << QPoint(228, 200)
           << QPoint(200, 122)
           << QPoint(122, 200)
           << QPoint(200, 280)
           << QPoint(280, 200);
    QTest::newRow("hollow square: testing points outside") << false << false << points;

    points.clear();
    points << QPoint(174, 174)
           << QPoint(174, 225)
           << QPoint(225, 174)
           << QPoint(225, 225)
           << QPoint(165, 200)
           << QPoint(200, 165)
           << QPoint(200, 235)
           << QPoint(235, 200);
    QTest::newRow("hollow circle: testing points inside") << true << true << points;

    points.clear();
    points << QPoint(160, 160)
           << QPoint(160, 240)
           << QPoint(240, 160)
           << QPoint(240, 240)
           << QPoint(200, 200)
           << QPoint(185, 185)
           << QPoint(185, 216)
           << QPoint(216, 185)
           << QPoint(216, 216)
           << QPoint(145, 200)
           << QPoint(200, 145)
           << QPoint(255, 200)
           << QPoint(200, 255);
    QTest::newRow("hollow circle: testing points outside") << true << false << points;
}

void tst_QQuickItem::contains()
{
    QFETCH(bool, circleTest);
    QFETCH(bool, insideTarget);
    QFETCH(QList<QPoint>, points);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("hollowTestItem.qml")));
    QQuickItem *root = qobject_cast<QQuickItem *>(window.rootObject());
    QVERIFY(root);

    // Ensure that we don't get extra hover events delivered on the side.
    QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->frameSynchronousHoverEnabled = false;
    // Flush out any mouse events that might be queued up
    qGuiApp->processEvents();

    HollowTestItem *hollowItem = root->findChild<HollowTestItem *>("hollowItem");
    QVERIFY(hollowItem);
    hollowItem->setCircle(circleTest);

    for (const QPoint &point : points) {
        qCDebug(lcTests) << "hover and click @" << point;

        // check mouse hover
        QTest::mouseMove(&window, point);
        QTRY_COMPARE(hollowItem->isHovered(), insideTarget);

        // check mouse press
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);
        QTRY_COMPARE(hollowItem->isPressed(), insideTarget);

        // check mouse release
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
        QTRY_COMPARE(hollowItem->isPressed(), false);
    }
}

void tst_QQuickItem::childAt()
{
    QQuickItem parent;

    QQuickItem child1;
    child1.setX(0);
    child1.setY(0);
    child1.setWidth(100);
    child1.setHeight(100);
    child1.setParentItem(&parent);

    QQuickItem child2;
    child2.setX(50);
    child2.setY(50);
    child2.setWidth(100);
    child2.setHeight(100);
    child2.setParentItem(&parent);

    QQuickItem child3;
    child3.setX(0);
    child3.setY(200);
    child3.setWidth(50);
    child3.setHeight(50);
    child3.setParentItem(&parent);

    QCOMPARE(parent.childAt(0, 0), &child1);
    QCOMPARE(parent.childAt(0, 99), &child1);
    QCOMPARE(parent.childAt(25, 25), &child1);
    QCOMPARE(parent.childAt(25, 75), &child1);
    QCOMPARE(parent.childAt(75, 25), &child1);
    QCOMPARE(parent.childAt(75, 75), &child2);
    QCOMPARE(parent.childAt(149, 149), &child2);
    QCOMPARE(parent.childAt(25, 200), &child3);
    QCOMPARE(parent.childAt(0, 150), static_cast<QQuickItem *>(nullptr));
    QCOMPARE(parent.childAt(300, 300), static_cast<QQuickItem *>(nullptr));
}

void tst_QQuickItem::grab()
{
    if (QGuiApplication::platformName() == QLatin1String("minimal"))
        QSKIP("Skipping due to grabToImage not functional on minimal platforms");

    QQuickView view;
    view.setSource(testFileUrl("grabToImage.qml"));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *root = qobject_cast<QQuickItem *>(view.rootObject());
    QVERIFY(root);
    QQuickItem *item = root->findChild<QQuickItem *>("myItem");
    QVERIFY(item);
#if QT_CONFIG(opengl)
    { // Default size (item is 100x100)
        QSharedPointer<QQuickItemGrabResult> result = item->grabToImage();
        QSignalSpy spy(result.data(), SIGNAL(ready()));
        QTRY_VERIFY(spy.size() > 0);
        QVERIFY(!result->url().isEmpty());
        QImage image = result->image();
        QCOMPARE(image.pixel(0, 0), qRgb(255, 0, 0));
        QCOMPARE(image.pixel(99, 99), qRgb(0, 0, 255));
    }

    { // Smaller size
        QSharedPointer<QQuickItemGrabResult> result = item->grabToImage(QSize(50, 50));
        QVERIFY(!result.isNull());
        QSignalSpy spy(result.data(), SIGNAL(ready()));
        QTRY_VERIFY(spy.size() > 0);
        QVERIFY(!result->url().isEmpty());
        QImage image = result->image();
        QCOMPARE(image.pixel(0, 0), qRgb(255, 0, 0));
        QCOMPARE(image.pixel(49, 49), qRgb(0, 0, 255));
    }
#endif
}

void tst_QQuickItem::isAncestorOf()
{
    QQuickItem parent;

    QQuickItem sub1;
    sub1.setParentItem(&parent);

    QQuickItem child1;
    child1.setParentItem(&sub1);
    QQuickItem child2;
    child2.setParentItem(&sub1);

    QQuickItem sub2;
    sub2.setParentItem(&parent);

    QQuickItem child3;
    child3.setParentItem(&sub2);
    QQuickItem child4;
    child4.setParentItem(&sub2);

    QVERIFY(parent.isAncestorOf(&sub1));
    QVERIFY(parent.isAncestorOf(&sub2));
    QVERIFY(parent.isAncestorOf(&child1));
    QVERIFY(parent.isAncestorOf(&child2));
    QVERIFY(parent.isAncestorOf(&child3));
    QVERIFY(parent.isAncestorOf(&child4));
    QVERIFY(sub1.isAncestorOf(&child1));
    QVERIFY(sub1.isAncestorOf(&child2));
    QVERIFY(!sub1.isAncestorOf(&child3));
    QVERIFY(!sub1.isAncestorOf(&child4));
    QVERIFY(sub2.isAncestorOf(&child3));
    QVERIFY(sub2.isAncestorOf(&child4));
    QVERIFY(!sub2.isAncestorOf(&child1));
    QVERIFY(!sub2.isAncestorOf(&child2));
    QVERIFY(!sub1.isAncestorOf(&sub1));
    QVERIFY(!sub2.isAncestorOf(&sub2));
}

/*
    Verify that a nested item's palette responds to changes of the enabled state
    and of the window's activation state by switching the current color group.
*/
void tst_QQuickItem::colorGroup()
{
    QQuickView view;

    view.setSource(testFileUrl("colorgroup.qml"));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *root = qobject_cast<QQuickItem *>(view.rootObject());
    QQuickItem *background = root->findChild<QQuickItem *>("background");
    QVERIFY(background);
    QQuickItem *foreground = root->findChild<QQuickItem *>("foreground");
    QVERIFY(foreground);

    QQuickPalette *palette = foreground->property("palette").value<QQuickPalette*>();
    QVERIFY(palette);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QCOMPARE(palette->currentColorGroup(), QPalette::Active);
    QCOMPARE(foreground->property("color").value<QColor>(), palette->active()->base());

    background->setEnabled(false);
    QCOMPARE(palette->currentColorGroup(), QPalette::Disabled);
    QCOMPARE(foreground->property("color").value<QColor>(), palette->disabled()->base());

    QWindow activationThief;
    activationThief.show();
    activationThief.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&activationThief));
    QCOMPARE(palette->currentColorGroup(), QPalette::Disabled);
    QCOMPARE(foreground->property("color").value<QColor>(), palette->disabled()->base());

    background->setEnabled(true);
    QCOMPARE(palette->currentColorGroup(), QPalette::Inactive);
    QCOMPARE(foreground->property("color").value<QColor>(), palette->inactive()->base());

    activationThief.hide();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(palette->currentColorGroup(), QPalette::Active);
    QCOMPARE(foreground->property("color").value<QColor>(), palette->active()->base());

    activationThief.show();
    activationThief.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&activationThief));
    QCOMPARE(palette->currentColorGroup(), QPalette::Inactive);
    QCOMPARE(foreground->property("color").value<QColor>(), palette->inactive()->base());
}

/*!
    Verify that items don't allocate their own QQuickPalette instance
    unnecessarily.
*/
void tst_QQuickItem::paletteAllocated()
{
    QQuickView view;

    view.setSource(testFileUrl("paletteAllocate.qml"));

    QQuickItem *root = qobject_cast<QQuickItem *>(view.rootObject());
    QQuickItem *background = root->findChild<QQuickItem *>("background");
    QVERIFY(background);
    QQuickItem *foreground = root->findChild<QQuickItem *>("foreground");
    QVERIFY(foreground);

    bool backgroundHasPalette = false;
    bool foregroundHasPalette = false;
    QObject::connect(background, &QQuickItem::paletteCreated, this, [&]{ backgroundHasPalette = true; });
    QObject::connect(foreground, &QQuickItem::paletteCreated, this, [&]{ foregroundHasPalette = true; });

    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QVERIFY(!backgroundHasPalette);
    QVERIFY(!foregroundHasPalette);

    view.close();

    QVERIFY(!backgroundHasPalette);
    QVERIFY(!foregroundHasPalette);

    auto quickpalette = foreground->property("palette").value<QQuickPalette*>();
    QVERIFY(!backgroundHasPalette);
    QVERIFY(quickpalette);
    QVERIFY(foregroundHasPalette);
}

void tst_QQuickItem::undefinedIsInvalidForWidthAndHeight()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("undefinedInvalid.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root);
    auto item = qobject_cast<QQuickItem *>(root.get());
    auto priv = QQuickItemPrivate::get(item);
    QVERIFY(item);
    QCOMPARE(item->height(), 300);
    QCOMPARE(item->width(), 200);
    QVERIFY(!priv->widthValid());
    QVERIFY(!priv->heightValid());
}

void tst_QQuickItem::viewport_data()
{
    QTest::addColumn<bool>("contentObservesViewport");

    QTest::addColumn<bool>("innerClip");
    QTest::addColumn<bool>("innerViewport");
    QTest::addColumn<bool>("innerObservesViewport");

    QTest::addColumn<bool>("outerClip");
    QTest::addColumn<bool>("outerViewport");
    QTest::addColumn<bool>("outerObservesViewport");

    QTest::addColumn<QPoint>("innerViewportOffset");
    QTest::addColumn<QPoint>("outerViewportOffset");

    QTest::addColumn<QRect>("expectedViewportTestRect");
    QTest::addColumn<QRect>("expectedContentClipRect");

    QTest::newRow("default") << false
        << false << false << false
        << false << false << false
        << QPoint() << QPoint() << QRect(0, 0, 290, 290) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp, clipping and observing") << true
        << true << true << true
        << true << true << true
        << QPoint() << QPoint() << QRect(55, 55, 200, 200) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp, clipping and observing; content not observing") << false
        << true << true << true
        << true << true << true
        << QPoint() << QPoint() << QRect(0, 0, 290, 290) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp and observing") << true
        << false << true << true
        << false << true << true
        << QPoint() << QPoint() << QRect(55, 55, 200, 200) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp and observing, inner pos offset") << true
        << false << true << true
        << false << true << true
        << QPoint(120, 120) << QPoint() << QRect(55, 55, 80, 80) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp and observing, inner neg offset") << true
        << false << true << true
        << false << true << true
        << QPoint(-70, -50) << QPoint() << QRect(105, 85, 170, 190) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp and observing, outer pos offset") << true
        << false << true << true
        << false << true << true
         << QPoint() << QPoint(220, 220) << QRect(55, 55, 20, 20) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp and observing, outer neg offset") << true
        << false << true << true
        << false << true << true
        << QPoint() << QPoint(-70, -50) << QRect(65, 55, 190, 200) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp and observing, pos and neg offset") << true
        << false << true << true
        << false << true << true
        << QPoint(150, 150) << QPoint(-170, -150) << QRect(55, 55, 50, 50) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp and observing, neg and pos offset") << true
        << false << true << true
        << false << true << true
        << QPoint(-180, -210) << QPoint(100, 115) << QRect(215, 245, 60, 30) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp not observing") << true
        << false << true << false
        << false << true << false
        << QPoint() << QPoint() << QRect(55, 55, 220, 220) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp not observing, inner pos offset") << true
        << false << true << false
        << false << true << false
        << QPoint(120, 120) << QPoint() << QRect(55, 55, 220, 220) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp not observing, inner neg offset") << true
        << false << true << false
        << false << true << false
        << QPoint(-70, -50) << QPoint() << QRect(55, 55, 220, 220) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp not observing, outer pos offset") << true
        << false << true << false
        << false << true << false
         << QPoint() << QPoint(220, 220) << QRect(55, 55, 220, 220) << QRect(0, 0, 290, 290);
    QTest::newRow("inner and outer: vp not observing, outer neg offset") << true
        << false << true << false
        << false << true << false
        << QPoint() << QPoint(-70, -50) << QRect(55, 55, 220, 220) << QRect(0, 0, 290, 290);
    QTest::newRow("inner clipping and observing") << true
        << true << true << true
        << false << false << false
        << QPoint() << QPoint() << QRect(55, 55, 220, 220) << QRect(0, 0, 290, 290);
    QTest::newRow("inner clipping and observing only outer") << true
        << true << true << true
        << false << true << false
        << QPoint() << QPoint() << QRect(55, 55, 200, 200) << QRect(0, 0, 290, 290);
}

void tst_QQuickItem::viewport()
{
    QFETCH(bool, contentObservesViewport);
    QFETCH(bool, innerClip);
    QFETCH(bool, innerViewport);
    QFETCH(bool, innerObservesViewport);
    QFETCH(bool, outerClip);
    QFETCH(bool, outerViewport);
    QFETCH(bool, outerObservesViewport);
    QFETCH(QPoint, innerViewportOffset);
    QFETCH(QPoint, outerViewportOffset);
    QFETCH(QRect, expectedViewportTestRect);
    QFETCH(QRect, expectedContentClipRect);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("viewports.qml")));

    QQuickItem *root = qobject_cast<QQuickItem *>(window.rootObject());
    QQuickItem *outer = root->findChild<QQuickItem *>("outerViewport");
    QVERIFY(outer);
    QQuickItem *inner = root->findChild<QQuickItem *>("innerViewport");
    QVERIFY(inner);
    QQuickItem *contentItem = root->findChild<QQuickItem *>("innerRect");
    QVERIFY(contentItem);
    ViewportTestItem *viewportTestItem = root->findChild<ViewportTestItem *>();
    QVERIFY(viewportTestItem);

    inner->setPosition(inner->position() + innerViewportOffset);
    outer->setPosition(outer->position() + outerViewportOffset);
    outer->setClip(outerClip);
    QCOMPARE(outer->flags().testFlag(QQuickItem::ItemIsViewport), outerClip);
    outer->setFlag(QQuickItem::ItemIsViewport, outerViewport);
    outer->setFlag(QQuickItem::ItemObservesViewport, outerObservesViewport);
    inner->setClip(innerClip);
    QCOMPARE(inner->flags().testFlag(QQuickItem::ItemIsViewport), innerClip);
    inner->setFlag(QQuickItem::ItemIsViewport, innerViewport);
    inner->setFlag(QQuickItem::ItemObservesViewport, innerObservesViewport);
    viewportTestItem->setFlag(QQuickItem::ItemObservesViewport, contentObservesViewport);
    emit viewportTestItem->viewportChanged();

    if (lcTests().isDebugEnabled())
        QTest::qWait(1000);
    if (contentObservesViewport) {
        if (innerViewport)
            QCOMPARE(viewportTestItem->viewportItem(), inner);
        else if (outerViewport)
            QCOMPARE(viewportTestItem->viewportItem(), outer);
        else
            QCOMPARE(viewportTestItem->viewportItem(), root->parentItem()); // QQuickRootItem
    } else {
        QCOMPARE(viewportTestItem->viewportItem(), root->parentItem()); // QQuickRootItem
    }

    QCOMPARE(contentItem->clipRect().toRect(), expectedContentClipRect);
    QCOMPARE(viewportTestItem->clipRect().toRect(), expectedViewportTestRect);
}

// Test that in a slot connected to destroyed() the emitter is
// is no longer a QQuickItem.
void tst_QQuickItem::qobject_castOnDestruction()
{
    QQuickItem item;
    QObject::connect(&item, &QObject::destroyed, [](QObject *object)
    {
        QVERIFY(!qobject_cast<QQuickItem *>(object));
        QVERIFY(!dynamic_cast<QQuickItem *>(object));
        QVERIFY(!object->isQuickItemType());
    });
}

/*
    Items that are getting destroyed should not emit property change notifications.
*/
void tst_QQuickItem::signalsOnDestruction_data()
{
    QTest::addColumn<bool>("childVisible");
    QTest::addColumn<bool>("grandChildVisible");

    QTest::addRow("Both visible") << true << true;
    QTest::addRow("Child visible") << true << false;
    QTest::addRow("Grand child visible") << false << true;
    QTest::addRow("Both hidden") << false << false;
}

void tst_QQuickItem::signalsOnDestruction()
{
    QFETCH(bool, childVisible);
    QFETCH(bool, grandChildVisible);

    QQuickWindow window;
    window.show();

    int expectedChildVisibleCount = 0;
    int expectedGrandChildVisibleCount = 0;

    // Visual children, but not QObject children.
    // Note: QQuickItem's visible property defaults to true after creation, as visual
    // items are always expected to be added to a visual hierarchy. So for the sake
    // of this test we first add, and then remove the item from a parent. This explicitly
    // sets the effective visibility to false.
    std::unique_ptr<QQuickItem> parent(new QQuickItem(window.contentItem()));
    QVERIFY(parent->isVisible());
    std::unique_ptr<QQuickItem> child(new QQuickItem);
    child->setVisible(childVisible);
    child->setParentItem(parent.get());
    child->setParentItem(nullptr);
    QVERIFY(!child->isVisible());
    std::unique_ptr<QQuickItem> grandChild(new QQuickItem);
    grandChild->setVisible(grandChildVisible);
    grandChild->setParentItem(child.get());
    grandChild->setParentItem(nullptr);
    QVERIFY(!grandChild->isVisible());

    QSignalSpy childrenSpy(parent.get(), &QQuickItem::childrenChanged);
    QSignalSpy visibleChildrenSpy(parent.get(), &QQuickItem::visibleChildrenChanged);
    QSignalSpy childParentSpy(child.get(), &QQuickItem::parentChanged);
    QSignalSpy childVisibleSpy(child.get(), &QQuickItem::visibleChanged);
    QSignalSpy childChildrenSpy(child.get(), &QQuickItem::childrenChanged);
    QSignalSpy childVisibleChildrenSpy(child.get(), &QQuickItem::visibleChildrenChanged);
    QSignalSpy grandChildParentSpy(grandChild.get(), &QQuickItem::parentChanged);
    QSignalSpy grandChildVisibleSpy(grandChild.get(), &QQuickItem::visibleChanged);

    child->setParentItem(parent.get());
    QCOMPARE(childrenSpy.count(), 1);
    if (childVisible)
        ++expectedChildVisibleCount;
    QCOMPARE(visibleChildrenSpy.count(), expectedChildVisibleCount);
    QCOMPARE(childParentSpy.count(), 1);
    QCOMPARE(childVisibleSpy.count(), expectedChildVisibleCount);
    QCOMPARE(childChildrenSpy.count(), 0);
    QCOMPARE(childVisibleChildrenSpy.count(), 0);

    grandChild->setParentItem(child.get());
    QCOMPARE(childrenSpy.count(), 1);
    QCOMPARE(visibleChildrenSpy.count(), expectedChildVisibleCount);
    QCOMPARE(childParentSpy.count(), 1);
    QCOMPARE(childVisibleSpy.count(), expectedChildVisibleCount);
    QCOMPARE(childChildrenSpy.count(), 1);
    if (grandChildVisible && childVisible)
        ++expectedGrandChildVisibleCount;
    QCOMPARE(childVisibleChildrenSpy.count(), expectedGrandChildVisibleCount);
    QCOMPARE(grandChildParentSpy.count(), 1);
    QCOMPARE(grandChildVisibleSpy.count(), expectedGrandChildVisibleCount);

    parent.reset();

    QVERIFY(!child->parentItem());
    QVERIFY(grandChild->parentItem());
    QCOMPARE(childrenSpy.count(), 1);
    QCOMPARE(visibleChildrenSpy.count(), expectedChildVisibleCount);
    QCOMPARE(childParentSpy.count(), 2);
    QCOMPARE(childChildrenSpy.count(), 1);
    if (childVisible)
        ++expectedChildVisibleCount;
    QCOMPARE(childVisibleSpy.count(), expectedChildVisibleCount);
    if (childVisible && grandChildVisible)
        ++expectedGrandChildVisibleCount;
    QCOMPARE(childVisibleChildrenSpy.count(), expectedGrandChildVisibleCount);
    QCOMPARE(grandChildParentSpy.count(), 1);
    QCOMPARE(grandChildVisibleSpy.count(), expectedGrandChildVisibleCount);
}

void tst_QQuickItem::visibleChanged()
{
    QQuickView window;
    window.setSource(testFileUrl("visiblechanged.qml"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQuickItem *root = qobject_cast<QQuickItem*>(window.rootObject());
    QVERIFY(root);

    QPointer<QQuickItem> parentItem = root->findChild<QQuickItem *>("parentItem");
    QPointer<QQuickItem> childItem = root->findChild<QQuickItem *>("childItem");
    QPointer<QQuickItem> loader = root->findChild<QQuickItem *>("loader");
    QPointer<QQuickItem> loaderChild = root->findChild<QQuickItem *>("loaderChild");
    QVERIFY(parentItem);
    QVERIFY(childItem);
    QVERIFY(loader);
    QVERIFY(loaderChild);

    QSignalSpy parentItemSpy(parentItem.data(), &QQuickItem::visibleChanged);
    QSignalSpy childItemSpy(childItem.data(), &QQuickItem::visibleChanged);
    QSignalSpy loaderChildSpy(loaderChild.data(), &QQuickItem::visibleChanged);

    loader->setProperty("active", false);
    QCOMPARE(parentItemSpy.count(), 0);
    QCOMPARE(childItemSpy.count(), 0);
    QVERIFY(!loaderChild->parentItem());
    QCOMPARE(loaderChildSpy.count(), 1);
    QCOMPARE(loaderChild->isVisible(), false);

    delete parentItem.data();
    QVERIFY(!parentItem);
    QVERIFY(childItem);
    QVERIFY(!childItem->parentItem());

    QCOMPARE(parentItemSpy.count(), 0);
    QCOMPARE(childItemSpy.count(), 1);
}

QTEST_MAIN(tst_QQuickItem)

#include "tst_qquickitem.moc"
