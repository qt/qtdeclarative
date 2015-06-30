/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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
#include "qquicktheme_p.h"

#include <QtQuickControls/private/qquickapplicationwindow_p.h>
#include <QtQuickControls/private/qquickbusyindicator_p.h>
#include <QtQuickControls/private/qquickbutton_p.h>
#include <QtQuickControls/private/qquickcheckbox_p.h>
#include <QtQuickControls/private/qquickframe_p.h>
#include <QtQuickControls/private/qquickgroupbox_p.h>
#include <QtQuickControls/private/qquicklabel_p.h>
#include <QtQuickControls/private/qquickpageindicator_p.h>
#include <QtQuickControls/private/qquickprogressbar_p.h>
#include <QtQuickControls/private/qquickradiobutton_p.h>
#include <QtQuickControls/private/qquickscrollbar_p.h>
#include <QtQuickControls/private/qquickscrollindicator_p.h>
#include <QtQuickControls/private/qquickslider_p.h>
#include <QtQuickControls/private/qquickstackview_p.h>
#include <QtQuickControls/private/qquickswitch_p.h>
#include <QtQuickControls/private/qquicktabbar_p.h>
#include <QtQuickControls/private/qquicktabbutton_p.h>
#include <QtQuickControls/private/qquicktextarea_p.h>
#include <QtQuickControls/private/qquicktextfield_p.h>
#include <QtQuickControls/private/qquicktogglebutton_p.h>
#include <QtQuickControls/private/qquicktoolbar_p.h>
#include <QtQuickControls/private/qquicktoolbutton_p.h>

#include <QtQuickControls/private/qquickcontrol_p.h>
#include <QtQuickControls/private/qquickexclusivegroup_p.h>

void initResources()
{
    Q_INIT_RESOURCE(qtquickcontrols2plugin);
}

QT_BEGIN_NAMESPACE

class QtQuickControls2Plugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface/1.0")

public:
    void registerTypes(const char *uri);
    void initializeEngine(QQmlEngine *engine, const char *uri);
};

void QtQuickControls2Plugin::registerTypes(const char *uri)
{
    qmlRegisterType<QQuickApplicationWindow>(uri, 2, 0, "AbstractApplicationWindow");
    qmlRegisterType<QQuickBusyIndicator>(uri, 2, 0, "AbstractBusyIndicator");
    qmlRegisterType<QQuickButton>(uri, 2, 0, "AbstractButton");
    qmlRegisterType<QQuickCheckBox>(uri, 2, 0, "AbstractCheckBox");
    qmlRegisterType<QQuickFrame>(uri, 2, 0, "AbstractFrame");
    qmlRegisterType<QQuickGroupBox>(uri, 2, 0, "AbstractGroupBox");
    qmlRegisterType<QQuickLabel>(uri, 2, 0, "AbstractLabel");
    qmlRegisterType<QQuickPageIndicator>(uri, 2, 0, "AbstractPageIndicator");
    qmlRegisterType<QQuickProgressBar>(uri, 2, 0, "AbstractProgressBar");
    qmlRegisterType<QQuickRadioButton>(uri, 2, 0, "AbstractRadioButton");
    qmlRegisterType<QQuickScrollBar>(uri, 2, 0, "AbstractScrollBar");
    qmlRegisterType<QQuickScrollIndicator>(uri, 2, 0, "AbstractScrollIndicator");
    qmlRegisterType<QQuickSlider>(uri, 2, 0, "AbstractSlider");
    qmlRegisterType<QQuickStackView>(uri, 2, 0, "AbstractStackView");
    qmlRegisterType<QQuickSwitch>(uri, 2, 0, "AbstractSwitch");
    qmlRegisterType<QQuickTabBar>(uri, 2, 0, "AbstractTabBar");
    qmlRegisterType<QQuickTabButton>(uri, 2, 0, "AbstractTabButton");
    qmlRegisterType<QQuickTextArea>(uri, 2, 0, "AbstractTextArea");
    qmlRegisterType<QQuickTextField>(uri, 2, 0, "AbstractTextField");
    qmlRegisterType<QQuickToggleButton>(uri, 2, 0, "AbstractToggleButton");
    qmlRegisterType<QQuickToolBar>(uri, 2, 0, "AbstractToolBar");
    qmlRegisterType<QQuickToolButton>(uri, 2, 0, "AbstractToolButton");

    qmlRegisterUncreatableType<QQuickStackAttached>(uri, 2, 0, "Stack", "Stack is an attached property");
    qmlRegisterUncreatableType<QQuickThemeAttached>(uri, 2, 0, "Theme", "Theme is an attached property");

    qmlRegisterType<QQuickControl>(uri, 2, 0, "Control");
    qmlRegisterType<QQuickExclusiveGroup>(uri, 2, 0, "ExclusiveGroup");
    qmlRegisterType<QQuickExclusiveGroupAttached>();

    qmlRegisterRevision<QQuickText, 6>(uri, 2, 0);
    qmlRegisterRevision<QQuickTextInput, 6>(uri, 2, 0);
    qmlRegisterRevision<QQuickTextEdit, 6>(uri, 2, 0);
}

void QtQuickControls2Plugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(engine);
    Q_UNUSED(uri);
    initResources();
}

QT_END_NAMESPACE

#include "qtquickcontrols2plugin.moc"
