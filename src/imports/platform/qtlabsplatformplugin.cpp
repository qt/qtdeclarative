/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qquickplatformmenu_p.h"
#include "qquickplatformmenubar_p.h"
#include "qquickplatformmenuitem_p.h"
#include "qquickplatformmenuitemgroup_p.h"

static inline void initResources()
{
#ifdef QT_STATIC
    Q_INIT_RESOURCE(qmake_Qt_labs_platform);
#endif
}

QT_BEGIN_NAMESPACE

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
    initResources();
}

void QtLabsPlatformPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<QQuickPlatformMenu>(uri, 1, 0, "Menu");
    qmlRegisterType<QQuickPlatformMenuBar>(uri, 1, 0, "MenuBar");
    qmlRegisterType<QQuickPlatformMenuItem>(uri, 1, 0, "MenuItem");
    qmlRegisterType<QQuickPlatformMenuItemGroup>(uri, 1, 0, "MenuItemGroup");
}

QT_END_NAMESPACE

#include "qtlabsplatformplugin.moc"
