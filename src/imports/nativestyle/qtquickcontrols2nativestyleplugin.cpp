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

#include <QtQml/qqml.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>

#include "qquicknativestyle.h"
#include "qquickcommonstyle.h"

#if defined(Q_OS_MACOS)
#include "qquickmacstyle_mac_p.h"
#endif

QT_BEGIN_NAMESPACE

using namespace QQC2;

class QtQuickControls2NativeStylePlugin : public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    ~QtQuickControls2NativeStylePlugin() override;

    void initializeEngine(QQmlEngine *engine, const char *uri) override;
    void registerTypes(const char *uri) override;
    QString name() const override;
};

QtQuickControls2NativeStylePlugin::~QtQuickControls2NativeStylePlugin()
{
    QQuickNativeStyle::setStyle(nullptr);
}

QString QtQuickControls2NativeStylePlugin::name() const
{
    return QStringLiteral("NativeStyle");
}

void QtQuickControls2NativeStylePlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(engine);
    Q_UNUSED(uri);
    // Enable commonstyle as a reference style while
    // the native styles are under development.
    if (qEnvironmentVariable("QQC2_COMMONSTYLE") == QStringLiteral("true"))
        QQuickNativeStyle::setStyle(new QCommonStyle);
    else
#if defined(Q_OS_MACOS)
        QQuickNativeStyle::setStyle(new QMacStyle);
#elif defined(Q_OS_WINDOWS)
        QQuickNativeStyle::setStyle(new QCommonStyle);
#else
        QQuickNativeStyle::setStyle(new QCommonStyle);
#endif
}

void QtQuickControls2NativeStylePlugin::registerTypes(const char *uri)
{
    Q_UNUSED(uri);
}

QT_END_NAMESPACE

#include "qtquickcontrols2nativestyleplugin.moc"
