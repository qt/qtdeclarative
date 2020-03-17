/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p.h>
#include "../shared/util.h"

#if QT_CONFIG(accessibility)
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/qpa/qplatformaccessibility.h>
#include <QtQuick/private/qquickaccessibleattached_p.h>
#endif

class tst_accessibility : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void a11y_data();
    void a11y();

    void override_data();
    void override();

private:
    QQmlEngine engine;
};

#if QT_CONFIG(accessibility)
static QPlatformAccessibility *platformAccessibility()
{
    QPlatformIntegration *pfIntegration = QGuiApplicationPrivate::platformIntegration();
    return pfIntegration ? pfIntegration->accessibility() : nullptr;
}
#endif

QString adjustFileBaseName(const QString &fileBaseName)
{
#if !QT_CONFIG(accessibility)
    if (fileBaseName == QLatin1Literal("dayofweekrow")
            || fileBaseName == QLatin1Literal("monthgrid")
            || fileBaseName == QLatin1Literal("weeknumbercolumn"))
        return fileBaseName += QLatin1Literal("-2");
#else
    return fileBaseName;
#endif
}

QQuickItem *findItem(QObject *object)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item) {
        QQuickPopup *popup = qobject_cast<QQuickPopup *>(object);
        if (popup)
            item = popup->popupItem();
    }
    return item;
}

void tst_accessibility::a11y_data()
{
    QTest::addColumn<QString>("fileBaseName");
    QTest::addColumn<QAccessible::Role>("role");
    QTest::addColumn<QString>("text");

    QTest::newRow("AbstractButton") << "abstractbutton" << QAccessible::Button << "AbstractButton";
    QTest::newRow("BusyIndicator") << "busyindicator" << QAccessible::Indicator << "";
    QTest::newRow("Button") << "button" << QAccessible::Button << "Button";
    QTest::newRow("CheckBox") << "checkbox" << QAccessible::CheckBox << "CheckBox";
    QTest::newRow("CheckDelegate") << "checkdelegate" << QAccessible::CheckBox << "CheckDelegate";
    QTest::newRow("ComboBox") << "combobox" << QAccessible::ComboBox << "ComboBox";
    QTest::newRow("Container") << "container" << QAccessible::NoRole << "";
    QTest::newRow("Control") << "control" << QAccessible::NoRole << "";
    QTest::newRow("Dial") << "dial" << QAccessible::Dial << "";
    QTest::newRow("Dialog") << "dialog" << QAccessible::Dialog << "Dialog";
    QTest::newRow("Drawer") << "drawer" << QAccessible::Dialog << "";
    QTest::newRow("Frame") << "frame" << QAccessible::Border << "";
    QTest::newRow("GroupBox") << "groupbox" << QAccessible::Grouping << "GroupBox";
    QTest::newRow("ItemDelegate") << "itemdelegate" << QAccessible::ListItem << "ItemDelegate";
    QTest::newRow("Label") << "label" << QAccessible::StaticText << "Label";
    QTest::newRow("Menu") << "menu" << QAccessible::PopupMenu << "";
    QTest::newRow("MenuItem") << "menuitem" << QAccessible::MenuItem << "MenuItem";
    QTest::newRow("Page") << "page" << QAccessible::PageTab << "Page";
    QTest::newRow("PageIndicator") << "pageindicator" << QAccessible::Indicator << "";
    QTest::newRow("Pane") << "pane" << QAccessible::Pane << "";
    QTest::newRow("Popup") << "popup" << QAccessible::Dialog << "";
    QTest::newRow("ProgressBar") << "progressbar" << QAccessible::ProgressBar << "";
    QTest::newRow("RadioButton") << "radiobutton" << QAccessible::RadioButton << "RadioButton";
    QTest::newRow("RadioDelegate") << "radiodelegate" << QAccessible::RadioButton << "RadioDelegate";
    QTest::newRow("RangeSlider") << "rangeslider" << QAccessible::Slider << "";
    QTest::newRow("RoundButton") << "roundbutton" << QAccessible::Button << "RoundButton";
    QTest::newRow("ScrollBar") << "scrollbar" << QAccessible::ScrollBar << "";
    QTest::newRow("ScrollIndicator") << "scrollindicator" << QAccessible::Indicator << "";
    QTest::newRow("Slider") << "slider" << QAccessible::Slider << "";
    QTest::newRow("SpinBox") << "spinbox" << QAccessible::SpinBox << "";
    QTest::newRow("StackView") << "stackview" << QAccessible::LayeredPane << "";
    QTest::newRow("SwipeDelegate") << "swipedelegate" << QAccessible::ListItem << "SwipeDelegate";
    QTest::newRow("SwipeView") << "swipeview" << QAccessible::PageTabList << "";
    QTest::newRow("Switch") << "switch" << QAccessible::CheckBox << "Switch";
    QTest::newRow("SwitchDelegate") << "switchdelegate" << QAccessible::ListItem << "SwitchDelegate";
    QTest::newRow("TabBar") << "tabbar" << QAccessible::PageTabList << "";
    QTest::newRow("TabButton") << "tabbutton" << QAccessible::PageTab << "TabButton";
    QTest::newRow("TextArea") << "textarea" << QAccessible::EditableText << "";
    QTest::newRow("TextField") << "textfield" << QAccessible::EditableText << "";
    QTest::newRow("ToolBar") << "toolbar" << QAccessible::ToolBar << "";
    QTest::newRow("ToolButton") << "toolbutton" << QAccessible::Button << "ToolButton";
    QTest::newRow("ToolTip") << "tooltip" << QAccessible::ToolTip << "ToolTip";
    QTest::newRow("Tumbler") << "tumbler" << QAccessible::NoRole << ""; // TODO

    QTest::newRow("DayOfWeekRow") << "dayofweekrow" << QAccessible::NoRole << "DayOfWeekRow";
    QTest::newRow("MonthGrid") << "monthgrid" << QAccessible::NoRole << "MonthGrid";
    QTest::newRow("WeekNumberColumn") << "weeknumbercolumn" << QAccessible::NoRole << "WeekNumberColumn";
}

void tst_accessibility::a11y()
{
    QFETCH(QString, fileBaseName);
    QFETCH(QAccessible::Role, role);
    QFETCH(QString, text);

    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("defaults/" + adjustFileBaseName(fileBaseName) + ".qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QQuickItem *item = findItem(object.data());
    QVERIFY(item);

#if QT_CONFIG(accessibility)
    QQuickAccessibleAttached *attached = QQuickAccessibleAttached::attachedProperties(item);
    if (fileBaseName != QLatin1String("dayofweekrow")
            && fileBaseName != QLatin1String("monthgrid")
            && fileBaseName != QLatin1String("weeknumbercolumn")) {
        if (QAccessible::isActive()) {
            QVERIFY(attached);
        } else {
            QVERIFY(!attached);
            QPlatformAccessibility *accessibility = platformAccessibility();
            if (!accessibility)
                QSKIP("No QPlatformAccessibility available.");
            accessibility->setActive(true);
            attached = QQuickAccessibleAttached::attachedProperties(item);
        }
    }
    QVERIFY(attached);
    QCOMPARE(attached->role(), role);
    QCOMPARE(attached->name(), text);
#else
    Q_UNUSED(role)
    Q_UNUSED(text)
#endif
}

void tst_accessibility::override_data()
{
    QTest::addColumn<QAccessible::Role>("role");

    QTest::newRow("AbstractButton") << QAccessible::Button;
    QTest::newRow("BusyIndicator") << QAccessible::Indicator;
    QTest::newRow("Button") << QAccessible::Button;
    QTest::newRow("CheckBox") << QAccessible::CheckBox;
    QTest::newRow("CheckDelegate") << QAccessible::CheckBox;
    QTest::newRow("ComboBox") << QAccessible::ComboBox;
    QTest::newRow("Container") << QAccessible::NoRole;
    QTest::newRow("Control") << QAccessible::NoRole;
    QTest::newRow("Dial") << QAccessible::Dial;
    QTest::newRow("Dialog") << QAccessible::Dialog;
    QTest::newRow("Drawer") << QAccessible::Dialog;
    QTest::newRow("Frame") << QAccessible::Border;
    QTest::newRow("GroupBox") << QAccessible::Grouping;
    QTest::newRow("ItemDelegate") << QAccessible::ListItem;
    QTest::newRow("Label") << QAccessible::StaticText;
    QTest::newRow("Menu") << QAccessible::PopupMenu;
    QTest::newRow("MenuItem") << QAccessible::MenuItem;
    QTest::newRow("Page") << QAccessible::PageTab;
    QTest::newRow("PageIndicator") << QAccessible::Indicator;
    QTest::newRow("Pane") << QAccessible::Pane;
    QTest::newRow("Popup") << QAccessible::Dialog;
    QTest::newRow("ProgressBar") << QAccessible::ProgressBar;
    QTest::newRow("RadioButton") << QAccessible::RadioButton;
    QTest::newRow("RadioDelegate") << QAccessible::RadioButton;
    QTest::newRow("RangeSlider") << QAccessible::Slider;
    QTest::newRow("RoundButton") << QAccessible::Button;
    QTest::newRow("ScrollBar") << QAccessible::ScrollBar;
    QTest::newRow("ScrollIndicator") << QAccessible::Indicator;
    QTest::newRow("Slider") << QAccessible::Slider;
    QTest::newRow("SpinBox") << QAccessible::SpinBox;
    QTest::newRow("StackView") << QAccessible::LayeredPane;
    QTest::newRow("SwipeDelegate") << QAccessible::ListItem;
    QTest::newRow("SwipeView") << QAccessible::PageTabList;
    QTest::newRow("Switch") << QAccessible::CheckBox;
    QTest::newRow("SwitchDelegate") << QAccessible::ListItem;
    QTest::newRow("TabBar") << QAccessible::PageTabList;
    QTest::newRow("TabButton") << QAccessible::PageTab;
    QTest::newRow("TextArea") << QAccessible::EditableText;
    QTest::newRow("TextField") << QAccessible::EditableText;
    QTest::newRow("ToolBar") << QAccessible::ToolBar;
    QTest::newRow("ToolButton") << QAccessible::Button;
    QTest::newRow("ToolTip") << QAccessible::ToolTip;
    QTest::newRow("Tumbler") << QAccessible::NoRole;

    QTest::newRow("DayOfWeekRow") << QAccessible::NoRole;
    QTest::newRow("MonthGrid") << QAccessible::NoRole;
    QTest::newRow("WeekNumberColumn") << QAccessible::NoRole;
}

void tst_accessibility::override()
{
    QFETCH(QAccessible::Role, role);

    const QString name = QTest::currentDataTag();
    const QString fileBaseName = name.toLower();

    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("override/" + adjustFileBaseName(fileBaseName) + ".qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QQuickItem *item = findItem(object.data());
    QVERIFY(item);

#if QT_CONFIG(accessibility)
    QQuickAccessibleAttached *attached = QQuickAccessibleAttached::attachedProperties(item);
    if (fileBaseName != QLatin1String("dayofweekrow")
            && fileBaseName != QLatin1String("monthgrid")
            && fileBaseName != QLatin1String("weeknumbercolumn")) {
        if (QAccessible::isActive()) {
            QVERIFY(attached);
        } else {
            QPlatformAccessibility *accessibility = platformAccessibility();
            if (!accessibility)
                QSKIP("No QPlatformAccessibility available.");
            accessibility->setActive(true);
            if (!attached)
                attached = QQuickAccessibleAttached::attachedProperties(item);
        }
    }

    QVERIFY(attached);
    QCOMPARE(attached->role(), role);
    QCOMPARE(attached->name(), name + "Override");
#else
    Q_UNUSED(role)
    Q_UNUSED(text)
#endif
}

QTEST_MAIN(tst_accessibility)

#include "tst_accessibility.moc"
