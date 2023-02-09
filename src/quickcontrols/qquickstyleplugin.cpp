// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyle.h"
#include "qquickstyle_p.h"
#include "qquickstyleplugin_p.h"

#include <QtCore/private/qfileselector_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtGui/qstylehints.h>
#include <QtGui/qguiapplication.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlfile.h>
#include <QtQml/private/qqmlmetatype_p.h>
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

    const QTypeRevision latestControlsRevision = QQmlMetaType::latestModuleVersion(QLatin1String("QtQuick.Controls"));
    // Use the private function because we don't want to cause resolve() to be called,
    // as the logic that assigns a default style if one wasn't set would interfere with compile-time style selection.
    QString styleName = QQuickStylePrivate::style();
    if (!latestControlsRevision.isValid() && styleName.isEmpty()) {
        // The user hasn't imported QtQuick.Controls, nor set a style via the runtime methods.
        qCDebug(lcStylePlugin).nospace() << uri << " imported before QtQuick.Controls; using compile-time style selection";
        QQuickStyle::setStyle(name());
        styleName = name();
    }

    // Even if this style plugin isn't for the style set by the user,
    // we still want to create the theme object, because that function
    // is also responsible for reading values from qtquickcontrols2.conf.
    // So, even if a style doesn't have a QQuickTheme, it can still have
    // values set for (e.g. fonts and palettes) in qtquickcontrols2.conf.
    const QString effectiveCurrentStyleName = QQuickStylePrivate::effectiveStyleName(styleName);
    auto theme = QQuickTheme::instance();
    if (!theme) {
        qCDebug(lcStylePlugin) << "creating theme";
        theme = createTheme(effectiveCurrentStyleName);
    }

    if (name() != effectiveCurrentStyleName) {
        qCDebug(lcStylePlugin).nospace() << "theme does not belong to current style ("
            << effectiveCurrentStyleName << "); not calling initializeTheme()";
        return;
    }

    qCDebug(lcStylePlugin) << "theme has not yet been initialized; calling initializeTheme()";
    initializeTheme(theme);
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                                     this, &QQuickStylePlugin::updateTheme);

    if (!styleName.isEmpty())
        QFileSelectorPrivate::addStatics(QStringList() << styleName);
}

void QQuickStylePlugin::unregisterTypes()
{
    qCDebug(lcStylePlugin) << "unregisterTypes called; plugin name is" << name();
    if (!QQuickThemePrivate::instance)
        return;

    disconnect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                                        this, &QQuickStylePlugin::updateTheme);

    // Not every style has a plugin - some styles are QML-only. So, we clean this
    // stuff up when the first style plugin is unregistered rather than when the
    // plugin for the current style is unregistered.
    QQuickThemePrivate::instance.reset();
    QQuickStylePrivate::reset();
}

/*!
    \internal

    Responsible for setting the font and palette settings that were specified in the
    qtquickcontrols2.conf file.

    Style-specific settings (e.g. Variant=Dense) are read in the constructor of the
    appropriate style plugin (e.g. QtQuickControls2MaterialStylePlugin).

    Implicit style-specific font and palette values are assigned in the relevant theme
    (e.g. QQuickMaterialTheme).
*/
QQuickTheme *QQuickStylePlugin::createTheme(const QString &name)
{
    qCDebug(lcStylePlugin) << "creating QQuickTheme instance to be initialized by style-specific theme of" << name;

    QQuickTheme *theme = new QQuickTheme;
#if QT_CONFIG(settings)
    QQuickThemePrivate *p = QQuickThemePrivate::get(theme);
    QSharedPointer<QSettings> settings = QQuickStylePrivate::settings(name);
    if (settings) {
        p->defaultFont.reset(QQuickStylePrivate::readFont(settings));
        // Set the default font as the System scope, because that's what
        // QQuickControlPrivate::parentFont() uses as its fallback if no
        // parent item has a font explicitly set. QQuickControlPrivate::parentFont()
        // is used as the starting point for font inheritance/resolution.
        // The same goes for palettes below.
        theme->setFont(QQuickTheme::System, *p->defaultFont);

        p->defaultPalette.reset(QQuickStylePrivate::readPalette(settings));
        theme->setPalette(QQuickTheme::System, *p->defaultPalette);
    }
#endif
    QQuickThemePrivate::instance.reset(theme);
    return theme;
}

QT_END_NAMESPACE

#include "moc_qquickstyleplugin_p.cpp"
