/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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

#include <QtCore/private/qfileselector_p.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickchecklabel_p.h>
#include <QtQuickControls2/private/qquickcolor_p.h>
#include <QtQuickControls2/private/qquickcolorimage_p.h>
#include <QtQuickControls2/private/qquickiconimage_p.h>
#include <QtQuickControls2/private/qquickmnemoniclabel_p.h>
#include <QtQuickControls2/private/qquickpaddedrectangle_p.h>
#include <QtQuickControls2/private/qquickplaceholdertext_p.h>
#include <QtQuickControls2/private/qquickiconlabel_p.h>
#include <QtQuickControls2/private/qquickstyle_p.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickControls2/private/qquickstyleselector_p.h>
#if QT_CONFIG(quick_listview) && QT_CONFIG(quick_pathview)
#include <QtQuickControls2/private/qquicktumblerview_p.h>
#endif
#include <QtQuickTemplates2/private/qquickoverlay_p.h>
#include <QtQuickControls2/private/qquickclippedtext_p.h>
#include <QtQuickControls2/private/qquickitemgroup_p.h>

#include "qquickdefaultbusyindicator_p.h"
#include "qquickdefaultdial_p.h"
#include "qquickdefaultprogressbar_p.h"
#include "qquickdefaultstyle_p.h"
#include "qquickdefaulttheme_p.h"

static inline void initResources()
{
    Q_INIT_RESOURCE(qtquickcontrols2plugin);
#ifdef QT_STATIC
    Q_INIT_RESOURCE(qmake_QtQuick_Controls_2);
#endif
}

QT_BEGIN_NAMESPACE

class QtQuickControls2Plugin: public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2Plugin(QObject *parent = nullptr);
    ~QtQuickControls2Plugin();

    void registerTypes(const char *uri) override;

    QString name() const override;
    QQuickProxyTheme *createTheme() const override;
};

QtQuickControls2Plugin::QtQuickControls2Plugin(QObject *parent) : QQuickStylePlugin(parent)
{
    initResources();
}

QtQuickControls2Plugin::~QtQuickControls2Plugin()
{
    QQuickStylePrivate::reset();
}

void QtQuickControls2Plugin::registerTypes(const char *uri)
{
    QQuickStylePrivate::init(typeUrl());
    const QString style = QQuickStyle::name();
    if (!style.isEmpty())
        QFileSelectorPrivate::addStatics(QStringList() << style.toLower());

    QQuickStyleSelector selector;
    selector.setBaseUrl(typeUrl());

    qmlRegisterModule(uri, 2, QT_VERSION_MINOR - 7); // Qt 5.7->2.0, 5.8->2.1, 5.9->2.2...

    // QtQuick.Controls 2.0 (originally introduced in Qt 5.7)
    qmlRegisterType(selector.select(QStringLiteral("AbstractButton.qml")), uri, 2, 0, "AbstractButton");
    qmlRegisterType(selector.select(QStringLiteral("ApplicationWindow.qml")), uri, 2, 0, "ApplicationWindow");
    qmlRegisterType(selector.select(QStringLiteral("BusyIndicator.qml")), uri, 2, 0, "BusyIndicator");
    qmlRegisterType(selector.select(QStringLiteral("Button.qml")), uri, 2, 0, "Button");
    qmlRegisterType(selector.select(QStringLiteral("ButtonGroup.qml")), uri, 2, 0, "ButtonGroup");
    qmlRegisterType(selector.select(QStringLiteral("CheckBox.qml")), uri, 2, 0, "CheckBox");
    qmlRegisterType(selector.select(QStringLiteral("CheckDelegate.qml")), uri, 2, 0, "CheckDelegate");
    qmlRegisterType(selector.select(QStringLiteral("ComboBox.qml")), uri, 2, 0, "ComboBox");
    qmlRegisterType(selector.select(QStringLiteral("Container.qml")), uri, 2, 0, "Container");
    qmlRegisterType(selector.select(QStringLiteral("Control.qml")), uri, 2, 0, "Control");
    qmlRegisterType(selector.select(QStringLiteral("Dial.qml")), uri, 2, 0, "Dial");
    qmlRegisterType(selector.select(QStringLiteral("Drawer.qml")), uri, 2, 0, "Drawer");
    qmlRegisterType(selector.select(QStringLiteral("Frame.qml")), uri, 2, 0, "Frame");
    qmlRegisterType(selector.select(QStringLiteral("GroupBox.qml")), uri, 2, 0, "GroupBox");
    qmlRegisterType(selector.select(QStringLiteral("ItemDelegate.qml")), uri, 2, 0, "ItemDelegate");
    qmlRegisterType(selector.select(QStringLiteral("Label.qml")), uri, 2, 0, "Label");
    qmlRegisterType(selector.select(QStringLiteral("Menu.qml")), uri, 2, 0, "Menu");
    qmlRegisterType(selector.select(QStringLiteral("MenuItem.qml")), uri, 2, 0, "MenuItem");
    qmlRegisterType(selector.select(QStringLiteral("Page.qml")), uri, 2, 0, "Page");
    qmlRegisterType(selector.select(QStringLiteral("PageIndicator.qml")), uri, 2, 0, "PageIndicator");
    qmlRegisterType(selector.select(QStringLiteral("Pane.qml")), uri, 2, 0, "Pane");
    qmlRegisterType(selector.select(QStringLiteral("Popup.qml")), uri, 2, 0, "Popup");
    qmlRegisterType(selector.select(QStringLiteral("ProgressBar.qml")), uri, 2, 0, "ProgressBar");
    qmlRegisterType(selector.select(QStringLiteral("RadioButton.qml")), uri, 2, 0, "RadioButton");
    qmlRegisterType(selector.select(QStringLiteral("RadioDelegate.qml")), uri, 2, 0, "RadioDelegate");
    qmlRegisterType(selector.select(QStringLiteral("RangeSlider.qml")), uri, 2, 0, "RangeSlider");
    qmlRegisterType(selector.select(QStringLiteral("ScrollBar.qml")), uri, 2, 0, "ScrollBar");
    qmlRegisterType(selector.select(QStringLiteral("ScrollIndicator.qml")), uri, 2, 0, "ScrollIndicator");
    qmlRegisterType(selector.select(QStringLiteral("Slider.qml")), uri, 2, 0, "Slider");
    qmlRegisterType(selector.select(QStringLiteral("SpinBox.qml")), uri, 2, 0, "SpinBox");
    qmlRegisterType(selector.select(QStringLiteral("StackView.qml")), uri, 2, 0, "StackView");
    qmlRegisterType(selector.select(QStringLiteral("SwipeDelegate.qml")), uri, 2, 0, "SwipeDelegate");
    qmlRegisterType(selector.select(QStringLiteral("SwipeView.qml")), uri, 2, 0, "SwipeView");
    qmlRegisterType(selector.select(QStringLiteral("Switch.qml")), uri, 2, 0, "Switch");
    qmlRegisterType(selector.select(QStringLiteral("SwitchDelegate.qml")), uri, 2, 0, "SwitchDelegate");
    qmlRegisterType(selector.select(QStringLiteral("TabBar.qml")), uri, 2, 0, "TabBar");
    qmlRegisterType(selector.select(QStringLiteral("TabButton.qml")), uri, 2, 0, "TabButton");
    qmlRegisterType(selector.select(QStringLiteral("TextArea.qml")), uri, 2, 0, "TextArea");
    qmlRegisterType(selector.select(QStringLiteral("TextField.qml")), uri, 2, 0, "TextField");
    qmlRegisterType(selector.select(QStringLiteral("ToolBar.qml")), uri, 2, 0, "ToolBar");
    qmlRegisterType(selector.select(QStringLiteral("ToolButton.qml")), uri, 2, 0, "ToolButton");
    qmlRegisterType(selector.select(QStringLiteral("ToolTip.qml")), uri, 2, 0, "ToolTip");
#if QT_CONFIG(quick_listview) && QT_CONFIG(quick_pathview)
    qmlRegisterType(selector.select(QStringLiteral("Tumbler.qml")), uri, 2, 0, "Tumbler");
#endif

    // QtQuick.Controls 2.1 (new types in Qt 5.8)
    qmlRegisterType(selector.select(QStringLiteral("Dialog.qml")), uri, 2, 1, "Dialog");
    qmlRegisterType(selector.select(QStringLiteral("DialogButtonBox.qml")), uri, 2, 1, "DialogButtonBox");
    qmlRegisterType(selector.select(QStringLiteral("MenuSeparator.qml")), uri, 2, 1, "MenuSeparator");
    qmlRegisterType(selector.select(QStringLiteral("RoundButton.qml")), uri, 2, 1, "RoundButton");
    qmlRegisterType(selector.select(QStringLiteral("ToolSeparator.qml")), uri, 2, 1, "ToolSeparator");

    // QtQuick.Controls 2.2 (new types in Qt 5.9)
    qmlRegisterType(selector.select(QStringLiteral("DelayButton.qml")), uri, 2, 2, "DelayButton");
    qmlRegisterType(selector.select(QStringLiteral("ScrollView.qml")), uri, 2, 2, "ScrollView");

    // QtQuick.Controls 2.3 (new types in Qt 5.10)
    qmlRegisterType(selector.select(QStringLiteral("Action.qml")), uri, 2, 3, "Action");
    qmlRegisterType(selector.select(QStringLiteral("ActionGroup.qml")), uri, 2, 3, "ActionGroup");
    qmlRegisterType(selector.select(QStringLiteral("MenuBar.qml")), uri, 2, 3, "MenuBar");
    qmlRegisterType(selector.select(QStringLiteral("MenuBarItem.qml")), uri, 2, 3, "MenuBarItem");
    qmlRegisterUncreatableType<QQuickOverlay>(uri, 2, 3, "Overlay", QStringLiteral("Overlay is only available as an attached property."));

    const QByteArray import = QByteArray(uri) + ".impl";
    qmlRegisterModule(import, 2, QT_VERSION_MINOR - 7); // Qt 5.7->2.0, 5.8->2.1, 5.9->2.2...

    // QtQuick.Controls.impl 2.0 (Qt 5.7)
    qmlRegisterType<QQuickDefaultBusyIndicator>(import, 2, 0, "BusyIndicatorImpl");
    qmlRegisterType<QQuickDefaultDial>(import, 2, 0, "DialImpl");
    qmlRegisterType<QQuickPaddedRectangle>(import, 2, 0, "PaddedRectangle");
    qmlRegisterType<QQuickDefaultProgressBar>(import, 2, 0, "ProgressBarImpl");

    // QtQuick.Controls.impl 2.1 (Qt 5.8)
#if QT_CONFIG(quick_listview) && QT_CONFIG(quick_pathview)
    qmlRegisterType<QQuickTumblerView>(import, 2, 1, "TumblerView");
#endif
    qmlRegisterSingletonType<QQuickDefaultStyle>(import, 2, 1, "Default", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
            Q_UNUSED(engine);
            Q_UNUSED(scriptEngine);
            return new QQuickDefaultStyle;
    });

    // QtQuick.Controls.impl 2.2 (Qt 5.9)
    qmlRegisterType<QQuickClippedText>(import, 2, 2, "ClippedText");
    qmlRegisterType<QQuickItemGroup>(import, 2, 2, "ItemGroup");
    qmlRegisterType<QQuickPlaceholderText>(import, 2, 2, "PlaceholderText");

    // QtQuick.Controls.impl 2.3 (Qt 5.10)
    qmlRegisterType<QQuickColorImage>(import, 2, 3, "ColorImage");
    qmlRegisterType<QQuickIconImage>(import, 2, 3, "IconImage");
    qmlRegisterSingletonType<QQuickColor>(import, 2, 3, "Color", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
            Q_UNUSED(engine);
            Q_UNUSED(scriptEngine);
            return new QQuickColor;
    });
    qmlRegisterType<QQuickIconLabel>(import, 2, 3, "IconLabel");
    qmlRegisterType<QQuickCheckLabel>(import, 2, 3, "CheckLabel");
    qmlRegisterType<QQuickMnemonicLabel>(import, 2, 3, "MnemonicLabel");
    qmlRegisterRevision<QQuickText, 6>(import, 2, 3);
}

QString QtQuickControls2Plugin::name() const
{
    return QStringLiteral("default");
}

QQuickProxyTheme *QtQuickControls2Plugin::createTheme() const
{
    return new QQuickDefaultTheme;
}

QT_END_NAMESPACE

#include "qtquickcontrols2plugin.moc"
