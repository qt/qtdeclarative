/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
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
#include <QtCore/qurl.h>
#include <QtCore/qcoreapplication.h>

#include <QtLabsTemplates/private/qquickabstractbutton_p.h>
#include <QtLabsTemplates/private/qquickbuttongroup_p.h>
#include <QtLabsTemplates/private/qquickcontainer_p.h>
#include <QtLabsTemplates/private/qquickcontrol_p.h>
#include <QtLabsTemplates/private/qquickpopup_p.h>
#include <QtLabsControls/private/qquickstyleselector_p.h>

#include "qquickbusyindicatorring_p.h"
#include "qquickprogressstrip_p.h"

static inline void initResources()
{
    Q_INIT_RESOURCE(qtlabscontrolsplugin);
}

QT_BEGIN_NAMESPACE

class QtLabsControlsPlugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri);
    void initializeEngine(QQmlEngine *engine, const char *uri);
};

void QtLabsControlsPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<QQuickAbstractButton>(uri, 1, 0, "AbstractButton");
    qmlRegisterType<QQuickButtonGroup>(uri, 1, 0, "ButtonGroup");
    qmlRegisterType<QQuickButtonGroupAttached>();
    qmlRegisterType<QQuickContainer>(uri, 1, 0, "Container");
    qmlRegisterType<QQuickControl>(uri, 1, 0, "Control");

    QQuickStyleSelector selector;
    selector.setBaseUrl(baseUrl());

    qmlRegisterType(selector.select(QStringLiteral("/ApplicationWindow.qml")), uri, 1, 0, "ApplicationWindow");
    qmlRegisterType(selector.select(QStringLiteral("/BusyIndicator.qml")), uri, 1, 0, "BusyIndicator");
    qmlRegisterType(selector.select(QStringLiteral("/Button.qml")), uri, 1, 0, "Button");
    qmlRegisterType(selector.select(QStringLiteral("/CheckBox.qml")), uri, 1, 0, "CheckBox");
    qmlRegisterType(selector.select(QStringLiteral("/ComboBox.qml")), uri, 1, 0, "ComboBox");
    qmlRegisterType(selector.select(QStringLiteral("/Dial.qml")), uri, 1, 0, "Dial");
    qmlRegisterType(selector.select(QStringLiteral("/Drawer.qml")), uri, 1, 0, "Drawer");
    qmlRegisterType(selector.select(QStringLiteral("/Frame.qml")), uri, 1, 0, "Frame");
    qmlRegisterType(selector.select(QStringLiteral("/GroupBox.qml")), uri, 1, 0, "GroupBox");
    qmlRegisterType(selector.select(QStringLiteral("/ItemDelegate.qml")), uri, 1, 0, "ItemDelegate");
    qmlRegisterType(selector.select(QStringLiteral("/Label.qml")), uri, 1, 0, "Label");
    qmlRegisterType(selector.select(QStringLiteral("/Menu.qml")), uri, 1, 0, "Menu");
    qmlRegisterType(selector.select(QStringLiteral("/MenuItem.qml")), uri, 1, 0, "MenuItem");
    qmlRegisterType(selector.select(QStringLiteral("/Page.qml")), uri, 1, 0, "Page");
    qmlRegisterType(selector.select(QStringLiteral("/PageIndicator.qml")), uri, 1, 0, "PageIndicator");
    qmlRegisterType(selector.select(QStringLiteral("/Pane.qml")), uri, 1, 0, "Pane");
    qmlRegisterType(selector.select(QStringLiteral("/Popup.qml")), uri, 1, 0, "Popup");
    qmlRegisterType(selector.select(QStringLiteral("/ProgressBar.qml")), uri, 1, 0, "ProgressBar");
    qmlRegisterType(selector.select(QStringLiteral("/RadioButton.qml")), uri, 1, 0, "RadioButton");
    qmlRegisterType(selector.select(QStringLiteral("/RangeSlider.qml")), uri, 1, 0, "RangeSlider");
    qmlRegisterType(selector.select(QStringLiteral("/ScrollBar.qml")), uri, 1, 0, "ScrollBar");
    qmlRegisterType(selector.select(QStringLiteral("/ScrollIndicator.qml")), uri, 1, 0, "ScrollIndicator");
    qmlRegisterType(selector.select(QStringLiteral("/Slider.qml")), uri, 1, 0, "Slider");
    qmlRegisterType(selector.select(QStringLiteral("/SpinBox.qml")), uri, 1, 0, "SpinBox");
    qmlRegisterType(selector.select(QStringLiteral("/StackView.qml")), uri, 1, 0, "StackView");
    qmlRegisterType(selector.select(QStringLiteral("/SwipeView.qml")), uri, 1, 0, "SwipeView");
    qmlRegisterType(selector.select(QStringLiteral("/Switch.qml")), uri, 1, 0, "Switch");
    qmlRegisterType(selector.select(QStringLiteral("/TabBar.qml")), uri, 1, 0, "TabBar");
    qmlRegisterType(selector.select(QStringLiteral("/TabButton.qml")), uri, 1, 0, "TabButton");
    qmlRegisterType(selector.select(QStringLiteral("/TextArea.qml")), uri, 1, 0, "TextArea");
    qmlRegisterType(selector.select(QStringLiteral("/TextField.qml")), uri, 1, 0, "TextField");
    qmlRegisterType(selector.select(QStringLiteral("/ToolBar.qml")), uri, 1, 0, "ToolBar");
    qmlRegisterType(selector.select(QStringLiteral("/ToolButton.qml")), uri, 1, 0, "ToolButton");
    qmlRegisterType(selector.select(QStringLiteral("/Tumbler.qml")), uri, 1, 0, "Tumbler");
}

void QtLabsControlsPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(engine);
    Q_UNUSED(uri);
    initResources();

    const QByteArray import = QByteArray(uri) + ".impl";
    qmlRegisterType<QQuickBusyIndicatorRing>(import, 1, 0, "BusyRing");
    qmlRegisterType<QQuickBusyIndicatorAnimator>(import, 1, 0, "BusyRingAnimator");
    qmlRegisterType<QQuickProgressStrip>(import, 1, 0, "ProgressStrip");
    qmlRegisterType<QQuickProgressAnimator>(import, 1, 0, "ProgressStripAnimator");
}

QT_END_NAMESPACE

#include "qtlabscontrolsplugin.moc"
