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
#include <QtQuickControls/private/qquickexclusivegroup_p.h>
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
}

void QtQuickControls2Plugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(engine);
    Q_UNUSED(uri);
    initResources();
}

QT_END_NAMESPACE

#include "qtquickcontrols2plugin.moc"
