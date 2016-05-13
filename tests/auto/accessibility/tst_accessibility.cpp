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
#include <QtQuickTemplates2/private/qquickpopup_p.h>
#include "../shared/util.h"
#include "../shared/visualtestutil.h"

#ifndef QT_NO_ACCESSIBILITY
#include <QtQuick/private/qquickaccessibleattached_p.h>
#endif

using namespace QQuickVisualTestUtil;

class tst_accessibility : public QQmlDataTest
{
    Q_OBJECT
public:

private slots:
    void a11y_data();
    void a11y();

private:
    QQmlEngine engine;
};

void tst_accessibility::a11y_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("role");
    QTest::addColumn<QString>("text");

    QTest::newRow("BusyIndicator") << "busyindicator" << 0x00000027 << ""; //QAccessible::Indicator
    QTest::newRow("Button") << "button" << 0x0000002B << "Button"; //QAccessible::Button
    QTest::newRow("CheckBox") << "checkbox" << 0x0000002C << "CheckBox"; //QAccessible::CheckBox
    // Frame
    // GroupBox
    QTest::newRow("Label") << "label" << 0x00000029 << "Label"; //QAccessible::StaticText
    QTest::newRow("Menu") << "menu" << 0x0000000B << ""; //QAccessible::PopupMenu
    QTest::newRow("PageIndicator") << "pageindicator" << 0x00000027 << ""; //QAccessible::Indicator
    QTest::newRow("Popup") << "popup" << 0x00000080 << ""; //QAccessible::LayeredPane
    QTest::newRow("ProgressBar") << "progressbar" << 0x00000030 << ""; //QAccessible::ProgressBar
    QTest::newRow("RadioButton") << "radiobutton" << 0x0000002D << "RadioButton"; //QAccessible::RadioButton
    QTest::newRow("RangeSlider") << "rangeslider" << 0x00000033 << ""; //QAccessible::Slider
    QTest::newRow("ScrollBar") << "scrollbar" << 0x00000003 << ""; //QAccessible::ScrollBar
    QTest::newRow("ScrollIndicator") << "scrollindicator" << 0x00000027 << ""; //QAccessible::Indicator
    QTest::newRow("Slider") << "slider" << 0x00000033 << ""; //QAccessible::Slider
    QTest::newRow("SpinBox") << "spinbox" << 0x00000034 << ""; //QAccessible::SpinBox
    // StackView
    QTest::newRow("Switch") << "switch" << 0x0000002B << "Switch"; //QAccessible::Button
    QTest::newRow("TabBar") << "tabbar" << 0x0000003C << ""; //QAccessible::PageTabList
    QTest::newRow("TabButton") << "tabbutton" << 0x00000025 << "TabButton"; //QAccessible::PageTab
    QTest::newRow("TextArea") << "textarea" << 0x0000002A << ""; //QAccessible::Accessible.EditableText
    QTest::newRow("TextField") << "textfield" << 0x0000002A << ""; //QAccessible::Accessible.EditableText
    QTest::newRow("ToolBar") << "toolbar" << 0x00000016 << ""; //QAccessible::ToolBar
    QTest::newRow("ToolButton") << "toolbutton" << 0x0000002B << "ToolButton"; //QAccessible::Button

    QTest::newRow("Dial") << "dial" << 0x00000031 << ""; //QAccessible::Dial
    // Drawer
    // SwipeView
    // Tumbler

    QTest::newRow("DayOfWeekRow") << "dayofweekrow" << 0x0 << "DayOfWeekRow"; //QAccessible::NoRole
    QTest::newRow("MonthGrid") << "monthgrid" << 0x0 << "MonthGrid"; //QAccessible::NoRole
    QTest::newRow("WeekNumberColumn") << "weeknumbercolumn" << 0x0 << "WeekNumberColumn"; //QAccessible::NoRole
}

static QQuickAccessibleAttached *accessibleAttached(QQuickItem *item)
{
    QQuickAccessibleAttached *acc = qobject_cast<QQuickAccessibleAttached *>(qmlAttachedPropertiesObject<QQuickAccessibleAttached>(item, false));
    if (!acc)
        acc = item->findChild<QQuickAccessibleAttached *>();
    return acc;
}

void tst_accessibility::a11y()
{
    QFETCH(QString, name);
    QFETCH(int, role);
    QFETCH(QString, text);

    QQmlComponent component(&engine);
    QString fn = name;
#ifdef QT_NO_ACCESSIBILITY
    if (name == QLatin1Literal("dayofweekrow")
            || name == QLatin1Literal("monthgrid")
            || name == QLatin1Literal("weeknumbercolumn"))
        fn += QLatin1Literal("-2");
#endif
    component.loadUrl(testFileUrl(fn + ".qml"));
    QObject* created = component.create();
    QVERIFY2(created, qPrintable(component.errorString()));
    QScopedPointer<QObject> cleanup(created);
    QVERIFY(!cleanup.isNull());

    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickItem *item = findItem<QQuickItem>(window->contentItem(), name);
    if (!item) {
        QQuickPopup *popup = window->contentItem()->findChild<QQuickPopup *>(name);
        if (popup)
            item = popup->popupItem();
    }
    QVERIFY(item);

#ifndef QT_NO_ACCESSIBILITY
    QQuickAccessibleAttached *acc = accessibleAttached(item);
    if (name != QLatin1Literal("dayofweekrow")
            && name != QLatin1Literal("monthgrid")
            && name != QLatin1Literal("weeknumbercolumn")) {
        if (QAccessible::isActive()) {
            QVERIFY(acc);
        } else {
            QVERIFY(!acc);
            QAccessible::setActive(true);
            acc = accessibleAttached(item);
        }
    }
    QVERIFY(acc);
    QCOMPARE(acc->role(), (QAccessible::Role)role);
    QCOMPARE(acc->name(), text);
#else
    Q_UNUSED(role)
    Q_UNUSED(text)
    QObject *acc = qmlAttachedPropertiesObject<QObject>(item, false);
    QVERIFY(!acc);
#endif
}

QTEST_MAIN(tst_accessibility)

#include "tst_accessibility.moc"
