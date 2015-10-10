/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QtTest/QSignalSpy>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qstylehints.h>
#include "../shared/util.h"
#include "../shared/visualtestutil.h"

using namespace QQuickVisualTestUtil;

class tst_activeFocusOnTab : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_activeFocusOnTab();

private slots:
    void initTestCase();
    void cleanup();

    void allControls();
    void textControls();
private:
    QQmlEngine engine;
    bool qt_tab_all_controls() {
        return QGuiApplication::styleHints()->tabFocusBehavior() == Qt::TabFocusAllControls;
    }
};

tst_activeFocusOnTab::tst_activeFocusOnTab()
{
}

void tst_activeFocusOnTab::initTestCase()
{
    QQmlDataTest::initTestCase();
}

void tst_activeFocusOnTab::cleanup()
{
}

void tst_activeFocusOnTab::allControls()
{
    if (!qt_tab_all_controls())
        QSKIP("This platform iterates only text controls. Cannot test iterating all controls.");

    QQuickView *window = new QQuickView(0);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("activeFocusOnTab.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(QGuiApplication::focusWindow() == window);

    // original: button1
    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "button1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: button1->button2
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "button2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: button2->checkbox
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "checkbox");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: checkbox->checkbox1
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "checkbox1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: checkbox1->checkbox2
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "checkbox2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: checkbox2->radiobutton
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "radiobutton");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: radiobutton->radiobutton1
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "radiobutton1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: radiobutton1->radiobutton2
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "radiobutton2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: radiobutton2->rangeslider.first.handle
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "rangeslider");
    QVERIFY(item);
    item = item->property("first").value<QObject*>()->property("handle").value<QQuickItem*>();
    QVERIFY(item->hasActiveFocus());

    // Tab: rangeslider.first.handle->rangeslider.second.handle
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "rangeslider");
    QVERIFY(item);
    item = item->property("second").value<QObject*>()->property("handle").value<QQuickItem*>();
    QVERIFY(item->hasActiveFocus());

    // Tab: rangeslider.second.handle->slider
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "slider");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: slider->spinbox
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "spinbox");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: spinbox->switch
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "switch");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: switch->tabbutton1
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "tabbutton1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: tabbutton1->tabbutton2
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "tabbutton2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: tabbutton2->textfield
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "textfield");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: textfield->toolbutton
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "toolbutton");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: toolbutton->textarea
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "textarea");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: textarea->toolbutton
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "toolbutton");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: toolbutton->textfield
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "textfield");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: textfield->tabbutton2
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "tabbutton2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: tabbutton2->tabbutton2
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "tabbutton1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: tabbutton1->switch
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "switch");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: switch->spinbox
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "spinbox");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: spinbox->slider
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "slider");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: slider->rangeslider.second.handle
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "rangeslider");
    QVERIFY(item);
    item = item->property("second").value<QObject*>()->property("handle").value<QQuickItem*>();
    QVERIFY(item->hasActiveFocus());

    // BackTab: rangeslider.second.handle->rangeslider.first.handle
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "rangeslider");
    QVERIFY(item);
    item = item->property("first").value<QObject*>()->property("handle").value<QQuickItem*>();
    QVERIFY(item->hasActiveFocus());

    // BackTab: rangeslider.first.handle->radiobutton2
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "radiobutton2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: radiobutton2->radiobutton1
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "radiobutton1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: radiobutton1->radiobutton
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "radiobutton");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: radiobutton->checkbox2
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "checkbox2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: checkbox2->checkbox1
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "checkbox1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: checkbox1->checkbox
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "checkbox");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: checkbox->button2
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "button2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: button2->button1
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "button1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: button1->textarea
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "textarea");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: textarea->toolbutton
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "toolbutton");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    delete window;
}

void tst_activeFocusOnTab::textControls()
{
    if (qt_tab_all_controls())
        QSKIP("This platform iterates all controls. Cannot test iterating text controls only.");

    QQuickView *window = new QQuickView(0);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("activeFocusOnTab.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(QGuiApplication::focusWindow() == window);

    // original: textfield
    QQuickItem *item = findItem<QQuickItem>(window->rootObject(), "textfield");
    QVERIFY(item);
    item->forceActiveFocus();
    QVERIFY(item->hasActiveFocus());

    // Tab: textfield->textarea
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "textarea");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: textarea->textfield
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "textfield");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: textfield->spinbox
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "spinbox");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: spinbox->textarea
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->rootObject(), "textarea");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    delete window;
}

QTEST_MAIN(tst_activeFocusOnTab)

#include "tst_activeFocusOnTab.moc"
