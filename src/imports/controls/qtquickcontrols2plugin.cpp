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
#include <QtCore/qdir.h>

#include <QtQuickControls/private/qquickexclusivegroup_p.h>
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
#include "qquicktheme_p.h"

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
    qmlRegisterUncreatableType<QQuickThemeAttached>(uri, 2, 0, "Theme", "Theme is an attached property");

    qmlRegisterType<QQuickExclusiveGroup>(uri, 2, 0, "ExclusiveGroup");
    qmlRegisterType<QQuickExclusiveGroupAttached>();

    QDir baseDir(baseUrl().toLocalFile());
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("ApplicationWindow.qml"))), uri, 2, 0, "ApplicationWindow");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("BusyIndicator.qml"))), uri, 2, 0, "BusyIndicator");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("Button.qml"))), uri, 2, 0, "Button");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("CheckBox.qml"))), uri, 2, 0, "CheckBox");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("Frame.qml"))), uri, 2, 0, "Frame");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("GroupBox.qml"))), uri, 2, 0, "GroupBox");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("Label.qml"))), uri, 2, 0, "Label");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("PageIndicator.qml"))), uri, 2, 0, "PageIndicator");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("ProgressBar.qml"))), uri, 2, 0, "ProgressBar");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("RadioButton.qml"))), uri, 2, 0, "RadioButton");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("ScrollBar.qml"))), uri, 2, 0, "ScrollBar");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("ScrollIndicator.qml"))), uri, 2, 0, "ScrollIndicator");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("Slider.qml"))), uri, 2, 0, "Slider");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("StackView.qml"))), uri, 2, 0, "StackView");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("Switch.qml"))), uri, 2, 0, "Switch");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("TabBar.qml"))), uri, 2, 0, "TabBar");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("TabButton.qml"))), uri, 2, 0, "TabButton");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("TextArea.qml"))), uri, 2, 0, "TextArea");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("TextField.qml"))), uri, 2, 0, "TextField");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("ToggleButton.qml"))), uri, 2, 0, "ToggleButton");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("ToolBar.qml"))), uri, 2, 0, "ToolBar");
    qmlRegisterType(QUrl::fromLocalFile(baseDir.filePath(QStringLiteral("ToolButton.qml"))), uri, 2, 0, "ToolButton");
}

void QtQuickControls2Plugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(engine);
    Q_UNUSED(uri);
    initResources();
}

QT_END_NAMESPACE

#include "qtquickcontrols2plugin.moc"
