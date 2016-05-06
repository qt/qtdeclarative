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

#include <QtLabsTemplates/private/qquickabstractbutton_p.h>
#include <QtLabsTemplates/private/qquickapplicationwindow_p.h>
#include <QtLabsTemplates/private/qquickbusyindicator_p.h>
#include <QtLabsTemplates/private/qquickbutton_p.h>
#include <QtLabsTemplates/private/qquickcheckbox_p.h>
#include <QtLabsTemplates/private/qquickcombobox_p.h>
#include <QtLabsTemplates/private/qquickcontrol_p.h>
#include <QtLabsTemplates/private/qquickcontainer_p.h>
#include <QtLabsTemplates/private/qquickdial_p.h>
#include <QtLabsTemplates/private/qquickdrawer_p.h>
#include <QtLabsTemplates/private/qquickframe_p.h>
#include <QtLabsTemplates/private/qquickgroupbox_p.h>
#include <QtLabsTemplates/private/qquickitemdelegate_p.h>
#include <QtLabsTemplates/private/qquicklabel_p.h>
#include <QtLabsTemplates/private/qquickmenu_p.h>
#include <QtLabsTemplates/private/qquickmenuitem_p.h>
#include <QtLabsTemplates/private/qquickoverlay_p.h>
#include <QtLabsTemplates/private/qquickpage_p.h>
#include <QtLabsTemplates/private/qquickpageindicator_p.h>
#include <QtLabsTemplates/private/qquickpane_p.h>
#include <QtLabsTemplates/private/qquickpopup_p.h>
#include <QtLabsTemplates/private/qquickprogressbar_p.h>
#include <QtLabsTemplates/private/qquickradiobutton_p.h>
#include <QtLabsTemplates/private/qquickrangeslider_p.h>
#include <QtLabsTemplates/private/qquickscrollbar_p.h>
#include <QtLabsTemplates/private/qquickscrollindicator_p.h>
#include <QtLabsTemplates/private/qquickslider_p.h>
#include <QtLabsTemplates/private/qquickspinbox_p.h>
#include <QtLabsTemplates/private/qquickstackview_p.h>
#include <QtLabsTemplates/private/qquickswipeview_p.h>
#include <QtLabsTemplates/private/qquickswitch_p.h>
#include <QtLabsTemplates/private/qquicktabbar_p.h>
#include <QtLabsTemplates/private/qquicktabbutton_p.h>
#include <QtLabsTemplates/private/qquicktextarea_p.h>
#include <QtLabsTemplates/private/qquicktextfield_p.h>
#include <QtLabsTemplates/private/qquicktoolbar_p.h>
#include <QtLabsTemplates/private/qquicktoolbutton_p.h>
#include <QtLabsTemplates/private/qquicktumbler_p.h>

QT_BEGIN_NAMESPACE

class QtLabsTemplatesPlugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri);
};

void QtLabsTemplatesPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<QQuickAbstractButton>(uri, 1, 0, "AbstractButton");
    qmlRegisterType<QQuickApplicationWindow>(uri, 1, 0, "ApplicationWindow");
    qmlRegisterType<QQuickApplicationWindowAttached>();
    qmlRegisterType<QQuickBusyIndicator>(uri, 1, 0, "BusyIndicator");
    qmlRegisterType<QQuickButton>(uri, 1, 0, "Button");
    qmlRegisterType<QQuickCheckBox>(uri, 1, 0, "CheckBox");
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
    qmlRegisterType<QQuickRangeSlider>(uri, 1, 0, "RangeSlider");
    qmlRegisterType<QQuickRangeSliderNode>();
    qmlRegisterType<QQuickScrollBar>(uri, 1, 0, "ScrollBar");
    qmlRegisterType<QQuickScrollIndicator>(uri, 1, 0, "ScrollIndicator");
    qmlRegisterType<QQuickSlider>(uri, 1, 0, "Slider");
    qmlRegisterType<QQuickSpinBox>(uri, 1, 0, "SpinBox");
    qmlRegisterType<QQuickSpinButton>();
    qmlRegisterType<QQuickStackView>(uri, 1, 0, "StackView");
    qmlRegisterType<QQuickSwipeViewAttached>();
    qmlRegisterType<QQuickSwipeView>(uri, 1, 0, "SwipeView");
    qmlRegisterType<QQuickSwitch>(uri, 1, 0, "Switch");
    qmlRegisterType<QQuickTabBar>(uri, 1, 0, "TabBar");
    qmlRegisterType<QQuickTabButton>(uri, 1, 0, "TabButton");
    qmlRegisterType<QQuickTextArea>(uri, 1, 0, "TextArea");
    qmlRegisterType<QQuickTextField>(uri, 1, 0, "TextField");
    qmlRegisterType<QQuickToolBar>(uri, 1, 0, "ToolBar");
    qmlRegisterType<QQuickToolButton>(uri, 1, 0, "ToolButton");
    qmlRegisterType<QQuickTumblerAttached>();
    qmlRegisterType<QQuickTumbler>(uri, 1, 0, "Tumbler");

    qmlRegisterRevision<QQuickItem, 2>(uri, 1, 0);
    qmlRegisterRevision<QQuickWindow, 2>(uri, 1, 0);
    qmlRegisterRevision<QQuickText, 6>(uri, 1, 0);
    qmlRegisterRevision<QQuickTextInput, 6>(uri, 1, 0);
    qmlRegisterRevision<QQuickTextEdit, 6>(uri, 1, 0);
}

QT_END_NAMESPACE

#include "qtlabstemplatesplugin.moc"
