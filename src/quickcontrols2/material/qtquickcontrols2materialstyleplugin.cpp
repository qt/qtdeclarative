// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmaterialstyle_p.h"
#include "qquickmaterialtheme_p.h"

#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickControls2Impl/private/qquickpaddedrectangle_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

QT_BEGIN_NAMESPACE

extern void qml_register_types_QtQuick_Controls_Material();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Controls_Material);

class QtQuickControls2MaterialStylePlugin : public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2MaterialStylePlugin(QObject *parent = nullptr);

    QString name() const override;
    void initializeTheme(QQuickTheme *theme) override;

    QQuickMaterialTheme theme;
};

QtQuickControls2MaterialStylePlugin::QtQuickControls2MaterialStylePlugin(QObject *parent) : QQuickStylePlugin(parent)
{
    volatile auto registration = &qml_register_types_QtQuick_Controls_Material;
    Q_UNUSED(registration);
}

QString QtQuickControls2MaterialStylePlugin::name() const
{
    return QStringLiteral("Material");
}

void QtQuickControls2MaterialStylePlugin::initializeTheme(QQuickTheme *theme)
{
    QQuickMaterialStyle::initGlobals();
    this->theme.initialize(theme);
}

QT_END_NAMESPACE

#include "qtquickcontrols2materialstyleplugin.moc"
