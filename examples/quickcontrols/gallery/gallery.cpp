// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QQuickStyle>

#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QGuiApplication>
#include <QIcon>

#include <QSettings>
#include <QOperatingSystemVersion>

using namespace Qt::StringLiterals;

static constexpr auto styleKey = "style"_L1;

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("Gallery"_L1);
    QCoreApplication::setOrganizationName("QtProject"_L1);

    QGuiApplication app(argc, argv);

    QIcon::setThemeName("gallery"_L1);

    QSettings settings;
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle(settings.value(styleKey).toString());

    // If this is the first time we're running the application,
    // we need to set a style in the settings so that the QML
    // can find it in the list of built-in styles.
    const QString styleInSettings = settings.value(styleKey).toString();
    if (styleInSettings.isEmpty())
        settings.setValue(styleKey, QQuickStyle::name());

    QQmlApplicationEngine engine;

    QStringList builtInStyles = { "Basic"_L1, "Fusion"_L1, "Imagine"_L1,
        "Material"_L1, "Universal"_L1, "FluentWinUI3"_L1 };

    if constexpr (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::MacOS)
        builtInStyles << "macOS"_L1 << "iOS"_L1;
    else if constexpr (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::IOS)
        builtInStyles << "iOS"_L1;
    else if constexpr (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows)
        builtInStyles << "Windows"_L1;

    engine.setInitialProperties({{ "builtInStyles"_L1, builtInStyles }});
    engine.load(QUrl("qrc:/gallery.qml"_L1));
    if (engine.rootObjects().isEmpty())
        return -1;

    return QCoreApplication::exec();
}
