// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickuniversalstyle_p.h"
#include "qquickuniversaltheme_p.h"

#include <QtQuickControls2/private/qquickstyleplugin_p.h>

QT_BEGIN_NAMESPACE

extern void qml_register_types_QtQuick_Controls_Universal();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Controls_Universal);

class QtQuickControls2UniversalStylePlugin : public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2UniversalStylePlugin(QObject *parent = nullptr);

    QString name() const override;
    void initializeTheme(QQuickTheme *theme) override;
    void updateTheme() override;

    QQuickUniversalTheme theme;
};

QtQuickControls2UniversalStylePlugin::QtQuickControls2UniversalStylePlugin(QObject *parent) : QQuickStylePlugin(parent)
{
    volatile auto registration = &qml_register_types_QtQuick_Controls_Universal;
    Q_UNUSED(registration);
}

QString QtQuickControls2UniversalStylePlugin::name() const
{
    return QStringLiteral("Universal");
}

void QtQuickControls2UniversalStylePlugin::initializeTheme(QQuickTheme *theme)
{
    QQuickUniversalStyle::initGlobals();
    this->theme.initialize(theme);
}

void QtQuickControls2UniversalStylePlugin::updateTheme()
{
    theme.updateTheme();
}

QT_END_NAMESPACE

#include "qtquickcontrols2universalstyleplugin.moc"
