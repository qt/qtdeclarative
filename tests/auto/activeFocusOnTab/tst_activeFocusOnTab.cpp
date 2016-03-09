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

private slots:
    void initTestCase();

    void tabNavigation_data();
    void tabNavigation();
};

void tst_activeFocusOnTab::initTestCase()
{
    QQmlDataTest::initTestCase();
}

void tst_activeFocusOnTab::tabNavigation_data()
{
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<Qt::TabFocusBehavior>("behavior");
    QTest::addColumn<QStringList>("order");

    QTest::newRow("tab-all-controls") << Qt::Key_Tab << Qt::TabFocusAllControls << (QStringList() << "button2" << "checkbox" << "checkbox1" << "checkbox2" << "radiobutton" << "radiobutton1" << "radiobutton2" << "rangeslider.first" << "rangeslider.second" << "slider" << "spinbox" << "switch" << "tabbutton1" << "tabbutton2" << "textfield" << "toolbutton" << "textarea" << "button1");
    QTest::newRow("backtab-all-controls") << Qt::Key_Backtab << Qt::TabFocusAllControls << (QStringList() << "textarea" << "toolbutton" << "textfield" << "tabbutton2" << "tabbutton1" << "switch" << "spinbox" << "slider" << "rangeslider.second" << "rangeslider.first" << "radiobutton2" << "radiobutton1" << "radiobutton" << "checkbox2" << "checkbox1" << "checkbox" << "button2" << "button1");

    QTest::newRow("tab-text-controls") << Qt::Key_Tab << Qt::TabFocusTextControls << (QStringList() << "spinbox" << "textfield" << "textarea");
    QTest::newRow("backtab-text-controls") << Qt::Key_Backtab << Qt::TabFocusTextControls << (QStringList() << "textarea" << "textfield" << "spinbox");
}

void tst_activeFocusOnTab::tabNavigation()
{
    QFETCH(Qt::Key, key);
    QFETCH(Qt::TabFocusBehavior, behavior);
    QFETCH(QStringList, order);

    QGuiApplication::styleHints()->setTabFocusBehavior(behavior);

    QQuickView view;
    view.contentItem()->setObjectName("contentItem");

    view.setSource(testFileUrl("activeFocusOnTab.qml"));
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

QTEST_MAIN(tst_activeFocusOnTab)

#include "tst_activeFocusOnTab.moc"
