// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/qqml.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qstylehints.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

#include "qquicknativestyle.h"
#include "qquickcommonstyle.h"

#if defined(Q_OS_MACOS)
#include "qquickmacfocusframe.h"
#include "qquickmacstyle_mac_p.h"
#elif defined(Q_OS_WINDOWS)
#include "qquickwindowsfocusframe.h"
#include "qquickwindowsxpstyle_p.h"
#endif

QT_BEGIN_NAMESPACE

extern void qml_register_types_QtQuick_NativeStyle();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_NativeStyle);

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

#if defined(Q_OS_MACOS) || defined (Q_OS_WIN)
    QScopedPointer<QQuickFocusFrame> m_focusFrame;
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
            if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark)
                qobject_cast<QWindowsStyle *>(style)->refreshPalette();
#else
            style = new QCommonStyle;
#endif
        }
    }

#if defined(Q_OS_MACOS)
    m_focusFrame.reset(new QQuickMacFocusFrame());
#elif defined(Q_OS_WIN)
    m_focusFrame.reset(new QQuickWindowsFocusFrame());
#endif

    // The native style plugin is neither the current style or fallback style
    // during QQuickStylePlugin::registerTypes, so it's not given a chance to
    // initialize or update the theme. But since it's used as an implementation
    // detail of some of the other style plugins, it might need to know about
    // theme changes.
    Q_ASSERT(style->thread()->isMainThread());
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
        style, [=]{ style->handleThemeChange(); });

    qAddPostRoutine(deleteQStyle);
    QQuickNativeStyle::setStyle(style);
}

void QtQuickControls2NativeStylePlugin::initializeTheme(QQuickTheme * /*theme*/)
{
}

QT_END_NAMESPACE

#include "qtquickcontrols2nativestyleplugin.moc"
