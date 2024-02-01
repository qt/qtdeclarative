// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfusionstyle_p.h"
#include "qquickfusiontheme_p.h"

#include <QtQml/qqml.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>

QT_BEGIN_NAMESPACE

extern void qml_register_types_QtQuick_Controls_Fusion();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Controls_Fusion);

class QtQuickControls2FusionStylePlugin : public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2FusionStylePlugin(QObject *parent = nullptr);

    QString name() const override;
    void initializeTheme(QQuickTheme *theme) override;

    QQuickFusionTheme theme;
};

QtQuickControls2FusionStylePlugin::QtQuickControls2FusionStylePlugin(QObject *parent) : QQuickStylePlugin(parent)
{
    volatile auto registration = &qml_register_types_QtQuick_Controls_Fusion;
    Q_UNUSED(registration);
}

QString QtQuickControls2FusionStylePlugin::name() const
{
    return QStringLiteral("Fusion");
}

void QtQuickControls2FusionStylePlugin::initializeTheme(QQuickTheme *theme)
{
    this->theme.initialize(theme);
}

QT_END_NAMESPACE

#include "qtquickcontrols2fusionstyleplugin.moc"
