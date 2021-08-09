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
#include <QtGui/qguiapplication.h>

#include <QtQuickTemplates2/private/qquicktheme_p.h>

#include "qquicknativestyle.h"
#include "qquickcommonstyle.h"

#if defined(Q_OS_MACOS)
#include "qquickmacstyle_mac_p.h"
#include "qquickmacfocusframe.h"
#elif defined(Q_OS_WINDOWS)
# include "qquickwindowsxpstyle_p.h"
#endif

extern void qml_register_types_QtQuick_NativeStyle();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_NativeStyle);

QT_BEGIN_NAMESPACE

using namespace QQC2;

class QtQuickControls2NativeStylePlugin : public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2NativeStylePlugin(QObject *parent = nullptr);
    ~QtQuickControls2NativeStylePlugin() override;

    void initializeEngine(QQmlEngine *engine, const char *uri) override;
    void initializeTheme(QQuickTheme *theme) override;
    QString name() const override;

#if defined(Q_OS_MACOS)
    QScopedPointer<QQuickMacFocusFrame> m_focusFrame;
#endif
};

static void deleteQStyle()
{
    // When we delete QStyle, it will free up it's own internal resources. Especially
    // on macOS, this means releasing a lot of NSViews and NSCells from the QMacStyle
    // destructor. If we did this from ~QtQuickControls2NativeStylePlugin, it would
    // happen when the plugin was unloaded from a Q_DESTRUCTOR_FUNCTION in QLibrary,
    // which is very late in the tear-down process, and after qGuiApp has been set to
    // nullptr, NSApplication has stopped running, and perhaps also other static platform
    // variables (e.g in AppKit?) has been deleted. And to our best guess, this is also why
    // we see a crash in AppKit from the destructor in QMacStyle. So for this reason, we
    // delete QStyle from a post routine rather than from the destructor.
    QQuickNativeStyle::setStyle(nullptr);
}

QtQuickControls2NativeStylePlugin::QtQuickControls2NativeStylePlugin(QObject *parent):
    QQuickStylePlugin(parent)
{
    volatile auto registration = &qml_register_types_QtQuick_NativeStyle;
    Q_UNUSED(registration);
}

QtQuickControls2NativeStylePlugin::~QtQuickControls2NativeStylePlugin()
{
    if (!qGuiApp)
        return;

    // QGuiApplication is still running, so we need to remove the post
    // routine to not be called after we have been unloaded.
    qRemovePostRoutine(deleteQStyle);
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
    QStyle *style = nullptr;
    if (qEnvironmentVariable("QQC2_COMMONSTYLE") == QStringLiteral("true")) {
        style = new QCommonStyle;
    } else {
        const QString envStyle = qEnvironmentVariable("QQC2_STYLE");
        if (!envStyle.isNull()) {
            if (envStyle == QLatin1String("common"))
                style = new QCommonStyle;
#if defined(Q_OS_MACOS)
            else if (envStyle == QLatin1String("mac"))
                style = new QMacStyle;
#endif
#if defined(Q_OS_WINDOWS)
            else if (envStyle == QLatin1String("windows"))
                style = new QWindowsStyle;
            else if (envStyle == QLatin1String("windowsxp"))
                style = new QWindowsXPStyle;
#endif
        }
        if (!style) {
#if defined(Q_OS_MACOS)
            style = new QMacStyle;
#elif defined(Q_OS_WINDOWS)
            style = new QWindowsXPStyle;
#else
            style = new QCommonStyle;
#endif
        }
    }

#if defined(Q_OS_MACOS)
    m_focusFrame.reset(new QQuickMacFocusFrame());
#endif

    qAddPostRoutine(deleteQStyle);
    QQuickNativeStyle::setStyle(style);
}

void QtQuickControls2NativeStylePlugin::initializeTheme(QQuickTheme * /*theme*/)
{
}

QT_END_NAMESPACE

#include "qtquickcontrols2nativestyleplugin.moc"
