// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfluenttheme_p.h"

#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

QT_BEGIN_NAMESPACE

extern void qml_register_types_QtQuick_Controls_Fluent();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Controls_Fluent);

class QtQuickControls2FluentStylePlugin : public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2FluentStylePlugin(QObject *parent = nullptr);

    QString name() const override;
    void initializeTheme(QQuickTheme *theme) override;
    void updateTheme() override;

    QQuickFluentTheme theme;
};

QtQuickControls2FluentStylePlugin::QtQuickControls2FluentStylePlugin(QObject *parent) : QQuickStylePlugin(parent)
{
    volatile auto registration = &qml_register_types_QtQuick_Controls_Fluent;
    Q_UNUSED(registration);
}

QString QtQuickControls2FluentStylePlugin::name() const
{
    return QStringLiteral("Fluent");
}

void QtQuickControls2FluentStylePlugin::initializeTheme(QQuickTheme *theme)
{
    this->theme.initialize(theme);
}

void QtQuickControls2FluentStylePlugin::updateTheme()
{
    QQuickTheme *theme = QQuickTheme::instance();
    QPalette palette;
    this->theme.updatePalette(palette);
    theme->setPalette(QQuickTheme::System, palette);
}

QT_END_NAMESPACE

#include "qtquickcontrols2fluentstyleplugin.moc"
