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

class tst_focus : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void initTestCase();

    void navigation_data();
    void navigation();
};

void tst_focus::initTestCase()
{
    QQmlDataTest::initTestCase();
}

void tst_focus::navigation_data()
{
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<Qt::TabFocusBehavior>("behavior");
    QTest::addColumn<QStringList>("order");

    QTest::newRow("tab-all-controls") << Qt::Key_Tab << QString("activeFocusOnTab.qml") << Qt::TabFocusAllControls << (QStringList() << "button2" << "checkbox" << "checkbox1" << "checkbox2" << "radiobutton" << "radiobutton1" << "radiobutton2" << "rangeslider.first" << "rangeslider.second" << "slider" << "spinbox" << "switch" << "tabbutton1" << "tabbutton2" << "textfield" << "toolbutton" << "textarea" << "button1");
    QTest::newRow("backtab-all-controls") << Qt::Key_Backtab << QString("activeFocusOnTab.qml") << Qt::TabFocusAllControls << (QStringList() << "textarea" << "toolbutton" << "textfield" << "tabbutton2" << "tabbutton1" << "switch" << "spinbox" << "slider" << "rangeslider.second" << "rangeslider.first" << "radiobutton2" << "radiobutton1" << "radiobutton" << "checkbox2" << "checkbox1" << "checkbox" << "button2" << "button1");

    QTest::newRow("tab-text-controls") << Qt::Key_Tab << QString("activeFocusOnTab.qml") << Qt::TabFocusTextControls << (QStringList() << "spinbox" << "textfield" << "textarea");
    QTest::newRow("backtab-text-controls") << Qt::Key_Backtab << QString("activeFocusOnTab.qml") << Qt::TabFocusTextControls << (QStringList() << "textarea" << "textfield" << "spinbox");

    QTest::newRow("key-up") << Qt::Key_Up << QString("keyNavigation.qml") << Qt::TabFocusAllControls << (QStringList() << "textarea" << "toolbutton" << "textfield" << "tabbutton2" << "tabbutton1" << "switch" << "slider" << "rangeslider.first" << "radiobutton2" << "radiobutton1" << "radiobutton" << "checkbox2" << "checkbox1" << "checkbox" << "button2" << "button1");
    QTest::newRow("key-down") << Qt::Key_Down << QString("keyNavigation.qml") << Qt::TabFocusAllControls << (QStringList() << "button2" << "checkbox" << "checkbox1" << "checkbox2" << "radiobutton" << "radiobutton1" << "radiobutton2" << "rangeslider.first" << "slider" << "switch" << "tabbutton1" << "tabbutton2" << "textfield" << "toolbutton" << "textarea" << "button1");
    QTest::newRow("key-left") << Qt::Key_Left << QString("keyNavigation.qml") << Qt::TabFocusAllControls << (QStringList() << "toolbutton" << "tabbutton2" << "tabbutton1" << "switch" << "spinbox" << "radiobutton2" << "radiobutton1" << "radiobutton" << "checkbox2" << "checkbox1" << "checkbox" << "button2" << "button1");
    QTest::newRow("key-right") << Qt::Key_Right << QString("keyNavigation.qml") << Qt::TabFocusAllControls << (QStringList() << "button2" << "checkbox" << "checkbox1" << "checkbox2" << "radiobutton" << "radiobutton1" << "radiobutton2" << "spinbox" << "switch" << "tabbutton1" << "tabbutton2" << "toolbutton" << "button1");
}

void tst_focus::navigation()
{
    QFETCH(Qt::Key, key);
    QFETCH(QString, testFile);
    QFETCH(Qt::TabFocusBehavior, behavior);
    QFETCH(QStringList, order);

    QGuiApplication::styleHints()->setTabFocusBehavior(behavior);

    QQuickView view;
    view.contentItem()->setObjectName("contentItem");

    view.setSource(testFileUrl(testFile));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QVERIFY(QGuiApplication::focusWindow() == &view);

    foreach (const QString &name, order) {
        QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
        QGuiApplication::sendEvent(&view, &event);
        QVERIFY(event.isAccepted());

        QQuickItem *item = findItem<QQuickItem>(view.rootObject(), name);
        QVERIFY2(item, qPrintable(name));
        QVERIFY2(item->hasActiveFocus(), qPrintable(QString("expected: '%1', actual: '%2'").arg(name).arg(view.activeFocusItem() ? view.activeFocusItem()->objectName() : "null")));
    }

    QGuiApplication::styleHints()->setTabFocusBehavior(Qt::TabFocusBehavior(-1));
}

QTEST_MAIN(tst_focus)

#include "tst_focus.moc"
