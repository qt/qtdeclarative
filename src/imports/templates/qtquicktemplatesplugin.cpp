/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#include <QtQml/qqmlextensionplugin.h>

#include <QtQuickTemplates/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates/private/qquickbusyindicator_p.h>
#include <QtQuickTemplates/private/qquickbutton_p.h>
#include <QtQuickTemplates/private/qquickcheckbox_p.h>
#include <QtQuickTemplates/private/qquickcheckdelegate_p.h>
#include <QtQuickTemplates/private/qquickcombobox_p.h>
#include <QtQuickTemplates/private/qquickcontrol_p.h>
#include <QtQuickTemplates/private/qquickcontainer_p.h>
#include <QtQuickTemplates/private/qquickdial_p.h>
#include <QtQuickTemplates/private/qquickdrawer_p.h>
#include <QtQuickTemplates/private/qquickframe_p.h>
#include <QtQuickTemplates/private/qquickgroupbox_p.h>
#include <QtQuickTemplates/private/qquickitemdelegate_p.h>
#include <QtQuickTemplates/private/qquicklabel_p.h>
#include <QtQuickTemplates/private/qquickmenu_p.h>
#include <QtQuickTemplates/private/qquickmenuitem_p.h>
#include <QtQuickTemplates/private/qquickoverlay_p.h>
#include <QtQuickTemplates/private/qquickpage_p.h>
#include <QtQuickTemplates/private/qquickpageindicator_p.h>
#include <QtQuickTemplates/private/qquickpane_p.h>
#include <QtQuickTemplates/private/qquickpopup_p.h>
#include <QtQuickTemplates/private/qquickprogressbar_p.h>
#include <QtQuickTemplates/private/qquickradiobutton_p.h>
#include <QtQuickTemplates/private/qquickradiodelegate_p.h>
#include <QtQuickTemplates/private/qquickrangeslider_p.h>
#include <QtQuickTemplates/private/qquickscrollbar_p.h>
#include <QtQuickTemplates/private/qquickscrollindicator_p.h>
#include <QtQuickTemplates/private/qquickslider_p.h>
#include <QtQuickTemplates/private/qquickspinbox_p.h>
#include <QtQuickTemplates/private/qquickstackview_p.h>
#include <QtQuickTemplates/private/qquickswipedelegate_p.h>
#include <QtQuickTemplates/private/qquickswipeview_p.h>
#include <QtQuickTemplates/private/qquickswitch_p.h>
#include <QtQuickTemplates/private/qquicktabbar_p.h>
#include <QtQuickTemplates/private/qquicktabbutton_p.h>
#include <QtQuickTemplates/private/qquicktextarea_p.h>
#include <QtQuickTemplates/private/qquicktextfield_p.h>
#include <QtQuickTemplates/private/qquicktoolbar_p.h>
#include <QtQuickTemplates/private/qquicktoolbutton_p.h>
#include <QtQuickTemplates/private/qquicktooltip_p.h>
#include <QtQuickTemplates/private/qquicktumbler_p.h>

static inline void initResources()
{
#ifdef QT_STATIC
    Q_INIT_RESOURCE(qmake_Qt_labs_templates);
#endif
}

QT_BEGIN_NAMESPACE

class QtQuickTemplatesPlugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface/1.0")

public:
    QtQuickTemplatesPlugin(QObject *parent = nullptr);
    void registerTypes(const char *uri);
};

QtQuickTemplatesPlugin::QtQuickTemplatesPlugin(QObject *parent) : QQmlExtensionPlugin(parent)
{
    initResources();
}

void QtQuickTemplatesPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<QQuickAbstractButton>(uri, 1, 0, "AbstractButton");
    qmlRegisterType<QQuickApplicationWindow>(uri, 1, 0, "ApplicationWindow");
    qmlRegisterType<QQuickApplicationWindowAttached>();
    qmlRegisterType<QQuickBusyIndicator>(uri, 1, 0, "BusyIndicator");
    qmlRegisterType<QQuickButton>(uri, 1, 0, "Button");
    qmlRegisterType<QQuickCheckBox>(uri, 1, 0, "CheckBox");
    qmlRegisterType<QQuickCheckDelegate>(uri, 1, 0, "CheckDelegate");
    qmlRegisterType<QQuickComboBox>(uri, 1, 0, "ComboBox");
    qmlRegisterType<QQuickContainer>(uri, 1, 0, "Container");
    qmlRegisterType<QQuickControl>(uri, 1, 0, "Control");
    qmlRegisterType<QQuickDial>(uri, 1, 0, "Dial");
    qmlRegisterType<QQuickDrawer>(uri, 1, 0, "Drawer");
    qmlRegisterType<QQuickFrame>(uri, 1, 0, "Frame");
    qmlRegisterType<QQuickGroupBox>(uri, 1, 0, "GroupBox");
    qmlRegisterType<QQuickItemDelegate>(uri, 1, 0, "ItemDelegate");
    qmlRegisterType<QQuickLabel>(uri, 1, 0, "Label");
    qmlRegisterType<QQuickMenu>(uri, 1, 0, "Menu");
    qmlRegisterType<QQuickMenuItem>(uri, 1, 0, "MenuItem");
    qmlRegisterType<QQuickOverlay>();
    qmlRegisterType<QQuickPage>(uri, 1, 0, "Page");
    qmlRegisterType<QQuickPageIndicator>(uri, 1, 0, "PageIndicator");
    qmlRegisterType<QQuickPane>(uri, 1, 0, "Pane");
    qmlRegisterType<QQuickPopup>(uri, 1, 0, "Popup");
    qmlRegisterType<QQuickProgressBar>(uri, 1, 0, "ProgressBar");
    qmlRegisterType<QQuickRadioButton>(uri, 1, 0, "RadioButton");
    qmlRegisterType<QQuickRadioDelegate>(uri, 1, 0, "RadioDelegate");
    qmlRegisterType<QQuickRangeSlider>(uri, 1, 0, "RangeSlider");
    qmlRegisterType<QQuickRangeSliderNode>();
    qmlRegisterType<QQuickScrollBar>(uri, 1, 0, "ScrollBar");
    qmlRegisterType<QQuickScrollIndicator>(uri, 1, 0, "ScrollIndicator");
    qmlRegisterType<QQuickSlider>(uri, 1, 0, "Slider");
    qmlRegisterType<QQuickSpinBox>(uri, 1, 0, "SpinBox");
    qmlRegisterType<QQuickSpinButton>();
    qmlRegisterType<QQuickStackView>(uri, 1, 0, "StackView");
    qmlRegisterType<QQuickSwipeDelegate>(uri, 1, 0, "SwipeDelegate");
    qmlRegisterType<QQuickSwipeExposure>();
    qmlRegisterType<QQuickSwipeViewAttached>();
    qmlRegisterType<QQuickSwipeView>(uri, 1, 0, "SwipeView");
    qmlRegisterType<QQuickSwitch>(uri, 1, 0, "Switch");
    qmlRegisterType<QQuickTabBar>(uri, 1, 0, "TabBar");
    qmlRegisterType<QQuickTabButton>(uri, 1, 0, "TabButton");
    qmlRegisterType<QQuickTextArea>(uri, 1, 0, "TextArea");
    qmlRegisterType<QQuickTextField>(uri, 1, 0, "TextField");
    qmlRegisterType<QQuickToolBar>(uri, 1, 0, "ToolBar");
    qmlRegisterType<QQuickToolButton>(uri, 1, 0, "ToolButton");
    qmlRegisterType<QQuickToolTipAttached>();
    qmlRegisterType<QQuickToolTip>(uri, 1, 0, "ToolTip");
    qmlRegisterType<QQuickTumblerAttached>();
    qmlRegisterType<QQuickTumbler>(uri, 1, 0, "Tumbler");

    qmlRegisterRevision<QQuickItem, 2>(uri, 1, 0);
    qmlRegisterRevision<QQuickWindow, 2>(uri, 1, 0);
    qmlRegisterRevision<QQuickText, 6>(uri, 1, 0);
    qmlRegisterRevision<QQuickTextInput, 6>(uri, 1, 0);
    qmlRegisterRevision<QQuickTextEdit, 6>(uri, 1, 0);
}

QT_END_NAMESPACE

#include "qtquicktemplatesplugin.moc"
