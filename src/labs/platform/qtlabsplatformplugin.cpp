/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include <QtQml/qqml.h>
#include <QtCore/qloggingcategory.h>

#include "qquicklabsplatformdialog_p.h"
#include "qquicklabsplatformcolordialog_p.h"
#include "qquicklabsplatformfiledialog_p.h"
#include "qquicklabsplatformfolderdialog_p.h"
#include "qquicklabsplatformfontdialog_p.h"
#include "qquicklabsplatformmessagedialog_p.h"

#include "qquicklabsplatformmenu_p.h"
#include "qquicklabsplatformmenubar_p.h"
#include "qquicklabsplatformmenuitem_p.h"
#include "qquicklabsplatformmenuitemgroup_p.h"
#include "qquicklabsplatformmenuseparator_p.h"

#include "qquicklabsplatformstandardpaths_p.h"
#if QT_CONFIG(systemtrayicon)
# include "qquicklabsplatformsystemtrayicon_p.h"
#endif

#include "qquicklabsplatformicon_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qtLabsPlatformDialogs, "qt.labs.platform.dialogs")
Q_LOGGING_CATEGORY(qtLabsPlatformMenus, "qt.labs.platform.menus")
Q_LOGGING_CATEGORY(qtLabsPlatformTray, "qt.labs.platform.tray")

class QtLabsPlatformPlugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtLabsPlatformPlugin(QObject *parent = nullptr);
    void registerTypes(const char *uri) override;
};

QtLabsPlatformPlugin::QtLabsPlatformPlugin(QObject *parent) : QQmlExtensionPlugin(parent)
{
}

void QtLabsPlatformPlugin::registerTypes(const char *uri)
{
    qmlRegisterUncreatableType<QQuickLabsPlatformDialog>(uri, 1, 0, "Dialog", QQuickLabsPlatformDialog::tr("Dialog is an abstract base class"));
    qmlRegisterType<QQuickLabsPlatformColorDialog>(uri, 1, 0, "ColorDialog");
    qmlRegisterType<QQuickLabsPlatformFileDialog>(uri, 1, 0, "FileDialog");
    qmlRegisterAnonymousType<QQuickLabsPlatformFileNameFilter>(uri, 1);
    qmlRegisterType<QQuickLabsPlatformFolderDialog>(uri, 1, 0, "FolderDialog");
    qmlRegisterType<QQuickLabsPlatformFontDialog>(uri, 1, 0, "FontDialog");
    qmlRegisterType<QQuickLabsPlatformMessageDialog>(uri, 1, 0, "MessageDialog");

    qmlRegisterType<QQuickLabsPlatformMenu>(uri, 1, 0, "Menu");
    qmlRegisterType<QQuickLabsPlatformMenuBar>(uri, 1, 0, "MenuBar");
    qmlRegisterType<QQuickLabsPlatformMenuItem>(uri, 1, 0, "MenuItem");
    qmlRegisterType<QQuickLabsPlatformMenuItem, 1>(uri, 1, 1, "MenuItem");
    qmlRegisterType<QQuickLabsPlatformMenuItemGroup>(uri, 1, 0, "MenuItemGroup");
    qmlRegisterType<QQuickLabsPlatformMenuSeparator>(uri, 1, 0, "MenuSeparator");
    qRegisterMetaType<QPlatformMenu::MenuType>();

    qmlRegisterUncreatableType<QPlatformDialogHelper>(uri, 1, 0, "StandardButton", QQuickLabsPlatformDialog::tr("Cannot create an instance of StandardButton"));
    qmlRegisterSingletonType<QQuickLabsPlatformStandardPaths>(uri, 1, 0, "StandardPaths", QQuickLabsPlatformStandardPaths::create);
    qRegisterMetaType<QStandardPaths::StandardLocation>();
    qRegisterMetaType<QStandardPaths::LocateOptions>();

#if QT_CONFIG(systemtrayicon)
    qmlRegisterType<QQuickLabsPlatformSystemTrayIcon>(uri, 1, 0, "SystemTrayIcon");
    qmlRegisterType<QQuickLabsPlatformSystemTrayIcon, 1>(uri, 1, 1, "SystemTrayIcon");
    qRegisterMetaType<QPlatformSystemTrayIcon::ActivationReason>();
    qRegisterMetaType<QPlatformSystemTrayIcon::MessageIcon>();
#endif

    qmlRegisterAnonymousType<QQuickLabsPlatformIcon>(uri, 1);
    qRegisterMetaType<QQuickLabsPlatformIcon>();
}

QT_END_NAMESPACE

#include "qtlabsplatformplugin.moc"
