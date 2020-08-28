/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qquickstyle.h"
#include "qquickstyle_p.h"
#include "qquickstyleplugin_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlfile.h>
#include <QtQuickTemplates2/private/qquicktheme_p_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcStylePlugin, "qt.quick.controls.styleplugin")

QQuickStylePlugin::QQuickStylePlugin(QObject *parent)
    : QQmlExtensionPlugin(parent)
{
}

QQuickStylePlugin::~QQuickStylePlugin()
{
}

void QQuickStylePlugin::registerTypes(const char *uri)
{
    qCDebug(lcStylePlugin).nospace() << "registerTypes called with uri " << uri << "; plugin name is " << name();

    auto theme = QQuickTheme::instance();
    if (!theme) {
        qWarning() << "QtQuick.Controls must be imported before importing" << baseUrl().toString();
        return;
    }

    if (name() != QQuickStyle::name()) {
        qCDebug(lcStylePlugin).nospace() << "theme does not belong to current style ("
            << QQuickStyle::name() << "); not calling initializeTheme()";
        return;
    }

    qCDebug(lcStylePlugin) << "theme has not been initialized; calling initializeTheme()";
    initializeTheme(theme);
}

void QQuickStylePlugin::unregisterTypes()
{
    qCDebug(lcStylePlugin) << "unregisterTypes called; plugin name is" << name();
}

QT_END_NAMESPACE
