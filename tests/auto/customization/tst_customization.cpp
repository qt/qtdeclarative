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
#include <QtCore/private/qhooks_p.h>
#include <QtCore/qregularexpression.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickControls2/qquickstyle.h>
#include "../shared/visualtestutil.h"

using namespace QQuickVisualTestUtil;

class tst_customization : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void creation_data();
    void creation();

    void comboPopup();

private:
    void reset();
    void addHooks();
    void removeHooks();

    QObject* createControl(const QString &type, QString *error);

    QQmlEngine *engine = nullptr;
};

typedef QHash<QObject *, QString> QObjectNameHash;
Q_GLOBAL_STATIC(QObjectNameHash, qt_objectNames)
Q_GLOBAL_STATIC(QStringList, qt_createdQObjects)
Q_GLOBAL_STATIC(QStringList, qt_destroyedQObjects)

extern "C" Q_DECL_EXPORT void qt_addQObject(QObject *object)
{
    // objectName is not set at construction time
    QObject::connect(object, &QObject::objectNameChanged, [object](const QString &objectName) {
        QString oldObjectName = qt_objectNames()->value(object);
        if (!oldObjectName.isEmpty())
            qt_createdQObjects()->removeOne(oldObjectName);
        if (!objectName.isEmpty()) {
            qt_createdQObjects()->append(objectName);
            qt_objectNames()->insert(object, objectName);
        }
    });
}

extern "C" Q_DECL_EXPORT void qt_removeQObject(QObject *object)
{
    QString objectName = object->objectName();
    if (!objectName.isEmpty())
        qt_destroyedQObjects()->append(objectName);
    qt_objectNames()->remove(object);
}

void tst_customization::init()
{
    engine = new QQmlEngine(this);

    qtHookData[QHooks::AddQObject] = reinterpret_cast<quintptr>(&qt_addQObject);
    qtHookData[QHooks::RemoveQObject] = reinterpret_cast<quintptr>(&qt_removeQObject);
}

void tst_customization::cleanup()
{
    qtHookData[QHooks::AddQObject] = 0;
    qtHookData[QHooks::RemoveQObject] = 0;

    delete engine;
    engine = nullptr;

    qmlClearTypeRegistrations();

    reset();
}

void tst_customization::reset()
{
    qt_createdQObjects()->clear();
    qt_destroyedQObjects()->clear();
}

QObject* tst_customization::createControl(const QString &name, QString *error)
{
    QQmlComponent component(engine);
    component.setData("import QtQuick.Controls 2.2; " + name.toUtf8() + " { }", QUrl());
    QObject *obj = component.create();
    if (!obj)
        *error = component.errorString();
    return obj;
}

void tst_customization::creation_data()
{
    QTest::addColumn<QString>("style");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QStringList>("delegates");

    // the "empty" style does not contain any delegates
    QTest::newRow("empty:AbstractButton") << "empty" << "AbstractButton"<< QStringList();
    QTest::newRow("empty:ApplicationWindow") << "empty" << "ApplicationWindow"<< QStringList();
    QTest::newRow("empty:BusyIndicator") << "empty" << "BusyIndicator"<< QStringList();
    QTest::newRow("empty:Button") << "empty" << "Button"<< QStringList();
    QTest::newRow("empty:CheckBox") << "empty" << "CheckBox" << QStringList();
    QTest::newRow("empty:CheckDelegate") << "empty" << "CheckDelegate" << QStringList();
    QTest::newRow("empty:ComboBox") << "empty" << "ComboBox" << QStringList();
    QTest::newRow("empty:Container") << "empty" << "Container"<< QStringList();
    QTest::newRow("empty:Control") << "empty" << "Control"<< QStringList();
    QTest::newRow("empty:DelayButton") << "empty" << "DelayButton"<< QStringList();
    QTest::newRow("empty:Dial") << "empty" << "Dial" << QStringList();
    QTest::newRow("empty:Dialog") << "empty" << "Dialog" << QStringList();
    QTest::newRow("empty:DialogButtonBox") << "empty" << "DialogButtonBox" << QStringList();
    QTest::newRow("empty:Drawer") << "empty" << "Drawer" << QStringList();
    QTest::newRow("empty:Frame") << "empty" << "Frame"<< QStringList();
    QTest::newRow("empty:GroupBox") << "empty" << "GroupBox"<< QStringList();
    QTest::newRow("empty:ItemDelegate") << "empty" << "ItemDelegate" << QStringList();
    QTest::newRow("empty:Label") << "empty" << "Label"<< QStringList();
    QTest::newRow("empty:Menu") << "empty" << "Menu" << QStringList();
    QTest::newRow("empty:MenuItem") << "empty" << "MenuItem"<< QStringList();
    QTest::newRow("empty:MenuSeparator") << "empty" << "MenuSeparator"<< QStringList();
    QTest::newRow("empty:Page") << "empty" << "Page"<< QStringList();
    QTest::newRow("empty:PageIndicator") << "empty" << "PageIndicator"<< QStringList();
    QTest::newRow("empty:Pane") << "empty" << "Pane"<< QStringList();
    QTest::newRow("empty:Popup") << "empty" << "Popup" << QStringList();
    QTest::newRow("empty:ProgressBar") << "empty" << "ProgressBar"<< QStringList();
    QTest::newRow("empty:RadioButton") << "empty" << "RadioButton" << QStringList();
    QTest::newRow("empty:RadioDelegate") << "empty" << "RadioDelegate" << QStringList();
    QTest::newRow("empty:RangeSlider") << "empty" << "RangeSlider" << QStringList();
    QTest::newRow("empty:RoundButton") << "empty" << "RoundButton" << QStringList();
    QTest::newRow("empty:ScrollBar") << "empty" << "ScrollBar"<< QStringList();
    QTest::newRow("empty:ScrollIndicator") << "empty" << "ScrollIndicator"<< QStringList();
    QTest::newRow("empty:ScrollView") << "empty" << "ScrollView"<< QStringList();
    QTest::newRow("empty:Slider") << "empty" << "Slider" << QStringList();
    QTest::newRow("empty:SpinBox") << "empty" << "SpinBox" << QStringList();
    QTest::newRow("empty:StackView") << "empty" << "StackView" << QStringList();
    QTest::newRow("empty:SwipeDelegate") << "empty" << "SwipeDelegate" << QStringList();
    QTest::newRow("empty:SwipeView") << "empty" << "SwipeView" << QStringList();
    QTest::newRow("empty:Switch") << "empty" << "Switch" << QStringList();
    QTest::newRow("empty:SwitchDelegate") << "empty" << "SwitchDelegate" << QStringList();
    QTest::newRow("empty:TabBar") << "empty" << "TabBar"<< QStringList();
    QTest::newRow("empty:TabButton") << "empty" << "TabButton"<< QStringList();
    QTest::newRow("empty:TextField") << "empty" << "TextField"<< QStringList();
    QTest::newRow("empty:TextArea") << "empty" << "TextArea"<< QStringList();
    QTest::newRow("empty:ToolBar") << "empty" << "ToolBar"<< QStringList();
    QTest::newRow("empty:ToolButton") << "empty" << "ToolButton"<< QStringList();
    QTest::newRow("empty:ToolSeparator") << "empty" << "ToolSeparator"<< QStringList();
    QTest::newRow("empty:ToolTip") << "empty" << "ToolTip"<< QStringList();
    // QTest::newRow("empty:Tumbler") << "empty" << "Tumbler"<< QStringList(); // ### TODO: fix crash with contentItem-less Tumbler

    // the "incomplete" style is missing bindings to the delegates (must be created regardless)
    QTest::newRow("incomplete:AbstractButton") << "incomplete" << "AbstractButton" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("incomplete:ApplicationWindow") << "incomplete" << "ApplicationWindow" << (QStringList() << "background");
    QTest::newRow("incomplete:BusyIndicator") << "incomplete" << "BusyIndicator" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:Button") << "incomplete" << "Button" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:CheckBox") << "incomplete" << "CheckBox" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("incomplete:CheckDelegate") << "incomplete" << "CheckDelegate" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("incomplete:ComboBox") << "incomplete" << "ComboBox" << (QStringList() << "background" << "contentItem" << "indicator"); // popup not created until needed
    QTest::newRow("incomplete:Container") << "incomplete" << "Container" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:Control") << "incomplete" << "Control" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:DelayButton") << "incomplete" << "DelayButton" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:Dial") << "incomplete" << "Dial" << (QStringList() << "background" << "contentItem" << "handle");
    QTest::newRow("incomplete:Dialog") << "incomplete" << "Dialog" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:DialogButtonBox") << "incomplete" << "DialogButtonBox" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:Drawer") << "incomplete" << "Drawer" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:Frame") << "incomplete" << "Frame"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:GroupBox") << "incomplete" << "GroupBox"<< (QStringList() << "background" << "contentItem" << "label");
    QTest::newRow("incomplete:ItemDelegate") << "incomplete" << "ItemDelegate" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:Label") << "incomplete" << "Label" << (QStringList() << "background");
    QTest::newRow("incomplete:Menu") << "incomplete" << "Menu" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:MenuItem") << "incomplete" << "MenuItem" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("incomplete:MenuSeparator") << "incomplete" << "MenuSeparator" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:Page") << "incomplete" << "Page" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:PageIndicator") << "incomplete" << "PageIndicator" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:Pane") << "incomplete" << "Pane" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:Popup") << "incomplete" << "Popup" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:ProgressBar") << "incomplete" << "ProgressBar" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:RadioButton") << "incomplete" << "RadioButton" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("incomplete:RadioDelegate") << "incomplete" << "RadioDelegate" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("incomplete:RangeSlider") << "incomplete" << "RangeSlider" << (QStringList() << "background" << "contentItem" << "first.handle" << "second.handle");
    QTest::newRow("incomplete:RoundButton") << "incomplete" << "RoundButton" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:ScrollBar") << "incomplete" << "ScrollBar" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:ScrollIndicator") << "incomplete" << "ScrollIndicator" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:ScrollView") << "incomplete" << "ScrollView" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:Slider") << "incomplete" << "Slider" << (QStringList() << "background" << "contentItem" << "handle");
    QTest::newRow("incomplete:SpinBox") << "incomplete" << "SpinBox" << (QStringList() << "background" << "contentItem" << "up.indicator" << "down.indicator");
    QTest::newRow("incomplete:StackView") << "incomplete" << "StackView" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:SwipeDelegate") << "incomplete" << "SwipeDelegate" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:SwipeView") << "incomplete" << "SwipeView" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:Switch") << "incomplete" << "Switch" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("incomplete:SwitchDelegate") << "incomplete" << "SwitchDelegate" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("incomplete:TabBar") << "incomplete" << "TabBar"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:TabButton") << "incomplete" << "TabButton"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:TextField") << "incomplete" << "TextField" << (QStringList() << "background");
    QTest::newRow("incomplete:TextArea") << "incomplete" << "TextArea" << (QStringList() << "background");
    QTest::newRow("incomplete:ToolBar") << "incomplete" << "ToolBar"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:ToolButton") << "incomplete" << "ToolButton"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:ToolSeparator") << "incomplete" << "ToolSeparator"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:ToolTip") << "incomplete" << "ToolTip" << (QStringList() << "background" << "contentItem");
    QTest::newRow("incomplete:Tumbler") << "incomplete" << "Tumbler"<< (QStringList() << "background" << "contentItem");

    // the "simple" style simulates a proper style and contains bindings to/in delegates
    QTest::newRow("simple:AbstractButton") << "simple" << "AbstractButton" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("simple:ApplicationWindow") << "simple" << "ApplicationWindow" << (QStringList() << "background");
    QTest::newRow("simple:BusyIndicator") << "simple" << "BusyIndicator" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:Button") << "simple" << "Button" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:CheckBox") << "simple" << "CheckBox" << (QStringList() << "contentItem" << "indicator");
    QTest::newRow("simple:CheckDelegate") << "simple" << "CheckDelegate" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("simple:ComboBox") << "simple" << "ComboBox" << (QStringList() << "background" << "contentItem" << "indicator"); // popup not created until needed
    QTest::newRow("simple:Container") << "simple" << "Container" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:Control") << "simple" << "Control" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:DelayButton") << "simple" << "DelayButton" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:Dial") << "simple" << "Dial" << (QStringList() << "background" << "handle");
    QTest::newRow("simple:Dialog") << "simple" << "Dialog" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:DialogButtonBox") << "simple" << "DialogButtonBox" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:Drawer") << "simple" << "Drawer" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:Frame") << "simple" << "Frame"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:GroupBox") << "simple" << "GroupBox"<< (QStringList() << "background" << "contentItem" << "label");
    QTest::newRow("simple:ItemDelegate") << "simple" << "ItemDelegate" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:Label") << "simple" << "Label" << (QStringList() << "background");
    QTest::newRow("simple:Menu") << "simple" << "Menu" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:MenuItem") << "simple" << "MenuItem" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("simple:MenuSeparator") << "simple" << "MenuSeparator" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:Page") << "simple" << "Page" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:PageIndicator") << "simple" << "PageIndicator" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:Pane") << "simple" << "Pane" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:Popup") << "simple" << "Popup" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:ProgressBar") << "simple" << "ProgressBar" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:RadioButton") << "simple" << "RadioButton" << (QStringList() << "contentItem" << "indicator");
    QTest::newRow("simple:RadioDelegate") << "simple" << "RadioDelegate" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("simple:RangeSlider") << "simple" << "RangeSlider" << (QStringList() << "background" << "first.handle" << "second.handle");
    QTest::newRow("simple:RoundButton") << "simple" << "RoundButton" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:ScrollBar") << "simple" << "ScrollBar" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:ScrollIndicator") << "simple" << "ScrollIndicator" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:ScrollView") << "simple" << "ScrollView" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:Slider") << "simple" << "Slider" << (QStringList() << "background" << "handle");
    QTest::newRow("simple:SpinBox") << "simple" << "SpinBox" << (QStringList() << "background" << "contentItem" << "up.indicator" << "down.indicator");
    QTest::newRow("simple:StackView") << "simple" << "StackView" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:SwipeDelegate") << "simple" << "SwipeDelegate" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:SwipeView") << "simple" << "SwipeView" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:Switch") << "simple" << "Switch" << (QStringList() << "contentItem" << "indicator");
    QTest::newRow("simple:SwitchDelegate") << "simple" << "SwitchDelegate" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("simple:TabBar") << "simple" << "TabBar"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:TabButton") << "simple" << "TabButton"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:TextField") << "simple" << "TextField" << (QStringList() << "background");
    QTest::newRow("simple:TextArea") << "simple" << "TextArea" << (QStringList() << "background");
    QTest::newRow("simple:ToolBar") << "simple" << "ToolBar"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:ToolButton") << "simple" << "ToolButton"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:ToolSeparator") << "simple" << "ToolSeparator"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:ToolTip") << "simple" << "ToolTip" << (QStringList() << "background" << "contentItem");
    QTest::newRow("simple:Tumbler") << "simple" << "Tumbler"<< (QStringList() << "background" << "contentItem");

    // the "override" style overrides all delegates in the "simple" style
    QTest::newRow("override:AbstractButton") << "override" << "AbstractButton" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("override:ApplicationWindow") << "override" << "ApplicationWindow" << (QStringList() << "background");
    QTest::newRow("override:BusyIndicator") << "override" << "BusyIndicator" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:Button") << "override" << "Button" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:CheckBox") << "override" << "CheckBox" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("override:CheckDelegate") << "override" << "CheckDelegate" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("override:ComboBox") << "override" << "ComboBox" << (QStringList() << "background" << "contentItem" << "indicator"); // popup not created until needed
    QTest::newRow("override:Container") << "override" << "Container" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:Control") << "override" << "Control" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:DelayButton") << "override" << "DelayButton" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:Dial") << "override" << "Dial" << (QStringList() << "background" << "contentItem" << "handle");
    QTest::newRow("override:Dialog") << "override" << "Dialog" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:DialogButtonBox") << "override" << "DialogButtonBox" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:Drawer") << "override" << "Drawer" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:Frame") << "override" << "Frame"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("override:GroupBox") << "override" << "GroupBox"<< (QStringList() << "background" << "contentItem" << "label");
    QTest::newRow("override:ItemDelegate") << "override" << "ItemDelegate" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:Label") << "override" << "Label" << (QStringList() << "background");
    QTest::newRow("override:Menu") << "override" << "Menu" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:MenuItem") << "override" << "MenuItem" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("override:MenuSeparator") << "override" << "MenuSeparator" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:Page") << "override" << "Page" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:PageIndicator") << "override" << "PageIndicator" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:Pane") << "override" << "Pane" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:Popup") << "override" << "Popup" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:ProgressBar") << "override" << "ProgressBar" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:RadioButton") << "override" << "RadioButton" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("override:RadioDelegate") << "override" << "RadioDelegate" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("override:RangeSlider") << "override" << "RangeSlider" << (QStringList() << "background" << "contentItem" << "first.handle" << "second.handle");
    QTest::newRow("override:RoundButton") << "override" << "RoundButton" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:ScrollBar") << "override" << "ScrollBar" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:ScrollIndicator") << "override" << "ScrollIndicator" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:ScrollView") << "override" << "ScrollView" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:Slider") << "override" << "Slider" << (QStringList() << "background" << "contentItem" << "handle");
    QTest::newRow("override:SpinBox") << "override" << "SpinBox" << (QStringList() << "background" << "contentItem" << "up.indicator" << "down.indicator");
    QTest::newRow("override:StackView") << "override" << "StackView" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:SwipeDelegate") << "override" << "SwipeDelegate" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:SwipeView") << "override" << "SwipeView" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:Switch") << "override" << "Switch" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("override:SwitchDelegate") << "override" << "SwitchDelegate" << (QStringList() << "background" << "contentItem" << "indicator");
    QTest::newRow("override:TabBar") << "override" << "TabBar"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("override:TabButton") << "override" << "TabButton"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("override:TextField") << "override" << "TextField" << (QStringList() << "background");
    QTest::newRow("override:TextArea") << "override" << "TextArea" << (QStringList() << "background");
    QTest::newRow("override:ToolBar") << "override" << "ToolBar"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("override:ToolButton") << "override" << "ToolButton"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("override:ToolSeparator") << "override" << "ToolSeparator"<< (QStringList() << "background" << "contentItem");
    QTest::newRow("override:ToolTip") << "override" << "ToolTip" << (QStringList() << "background" << "contentItem");
    QTest::newRow("override:Tumbler") << "override" << "Tumbler"<< (QStringList() << "background" << "contentItem");
}

void tst_customization::creation()
{
    QFETCH(QString, style);
    QFETCH(QString, type);
    QFETCH(QStringList, delegates);

    QQuickStyle::setStyle(testFile("styles/" + style));

    QString error;
    QScopedPointer<QObject> control(createControl(type, &error));
    QVERIFY2(control, qPrintable(error));

    QByteArray templateType = "QQuick" + type.toUtf8();
    QVERIFY2(control->inherits(templateType), qPrintable(type + " does not inherit " + templateType + " (" + control->metaObject()->className() + ")"));

    QString controlName = type.toLower() + "-" + style;
    QVERIFY2(qt_createdQObjects()->removeOne(controlName), qPrintable(controlName + " was not created as expected"));

    for (QString delegate : qAsConst(delegates)) {
        if (!delegate.contains("-"))
            delegate.append("-" + style);
        delegate.prepend(type.toLower() + "-");

        QVERIFY2(qt_createdQObjects()->removeOne(delegate), qPrintable(delegate + " was not created as expected"));
    }

    QEXPECT_FAIL("override:Tumbler", "TODO", Abort);

    QVERIFY2(qt_createdQObjects()->isEmpty(), qPrintable("unexpectedly created: " + qt_createdQObjects->join(", ")));
    QVERIFY2(qt_destroyedQObjects()->isEmpty(), qPrintable("unexpectedly destroyed: " + qt_destroyedQObjects->join(", ") + " were unexpectedly destroyed"));
}

void tst_customization::comboPopup()
{
    QQuickStyle::setStyle(testFile("styles/simple"));

    {
        // test that ComboBox::popup is created when accessed
        QQmlComponent component(engine);
        component.setData("import QtQuick.Controls 2.2; ComboBox { }", QUrl());
        QScopedPointer<QQuickItem> comboBox(qobject_cast<QQuickItem *>(component.create()));
        QVERIFY(comboBox);

        QVERIFY(!qt_createdQObjects()->contains("combobox-popup-simple"));

        QObject *popup = comboBox->property("popup").value<QObject *>();
        QVERIFY(popup);
        QVERIFY(qt_createdQObjects()->contains("combobox-popup-simple"));
    }

    reset();

    {
        // test that ComboBox::popup is created when it becomes visible
        QQuickWindow window;
        window.resize(300, 300);
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));

        QQmlComponent component(engine);
        component.setData("import QtQuick.Controls 2.2; ComboBox { }", QUrl());
        QScopedPointer<QQuickItem> comboBox(qobject_cast<QQuickItem *>(component.create()));
        QVERIFY(comboBox);

        comboBox->setParentItem(window.contentItem());
        QVERIFY(!qt_createdQObjects()->contains("combobox-popup-simple"));

        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
        QVERIFY(qt_createdQObjects()->contains("combobox-popup-simple"));
    }
}

QTEST_MAIN(tst_customization)

#include "tst_customization.moc"
