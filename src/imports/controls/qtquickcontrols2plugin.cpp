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
#include <QtCore/qfileselector.h>
#include <QtCore/qurl.h>

#include <QtQuickTemplates/private/qquickexclusivegroup_p.h>

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

    // TODO: read the style from application manifest file
    QFileSelector selector;
    QString base = baseUrl().toString();
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/ApplicationWindow.qml"))), uri, 2, 0, "ApplicationWindow");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/BusyIndicator.qml"))), uri, 2, 0, "BusyIndicator");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/Button.qml"))), uri, 2, 0, "Button");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/CheckBox.qml"))), uri, 2, 0, "CheckBox");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/Dial.qml"))), uri, 2, 0, "Dial");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/Drawer.qml"))), uri, 2, 0, "Drawer");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/Frame.qml"))), uri, 2, 0, "Frame");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/GroupBox.qml"))), uri, 2, 0, "GroupBox");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/Label.qml"))), uri, 2, 0, "Label");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/PageIndicator.qml"))), uri, 2, 0, "PageIndicator");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/ProgressBar.qml"))), uri, 2, 0, "ProgressBar");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/RadioButton.qml"))), uri, 2, 0, "RadioButton");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/ScrollBar.qml"))), uri, 2, 0, "ScrollBar");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/ScrollIndicator.qml"))), uri, 2, 0, "ScrollIndicator");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/Slider.qml"))), uri, 2, 0, "Slider");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/StackView.qml"))), uri, 2, 0, "StackView");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/SwipeView.qml"))), uri, 2, 0, "SwipeView");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/Switch.qml"))), uri, 2, 0, "Switch");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/TabBar.qml"))), uri, 2, 0, "TabBar");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/TabButton.qml"))), uri, 2, 0, "TabButton");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/TextArea.qml"))), uri, 2, 0, "TextArea");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/TextField.qml"))), uri, 2, 0, "TextField");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/ToolBar.qml"))), uri, 2, 0, "ToolBar");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/ToolButton.qml"))), uri, 2, 0, "ToolButton");
    qmlRegisterType(selector.select(QUrl(base + QStringLiteral("/Tumbler.qml"))), uri, 2, 0, "Tumbler");
}

void QtQuickControls2Plugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(engine);
    Q_UNUSED(uri);
    initResources();
}

QT_END_NAMESPACE

#include "qtquickcontrols2plugin.moc"
