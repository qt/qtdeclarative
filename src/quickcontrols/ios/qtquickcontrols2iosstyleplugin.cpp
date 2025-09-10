// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickiostheme_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtQml/qqml.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

QT_BEGIN_NAMESPACE

extern void qml_register_types_QtQuick_Controls_iOS();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Controls_iOS);

class QtQuickControls2IOSStylePlugin : public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2IOSStylePlugin(QObject *parent = nullptr);

    QString name() const override;
    void initializeTheme(QQuickTheme *theme) override;
    void updateTheme() override;

    QQuickIOSTheme m_theme;
};

QtQuickControls2IOSStylePlugin::QtQuickControls2IOSStylePlugin(QObject *parent) : QQuickStylePlugin(parent)
{
    volatile auto registration = &qml_register_types_QtQuick_Controls_iOS;
    Q_UNUSED(registration);
}

QString QtQuickControls2IOSStylePlugin::name() const
{
    return QStringLiteral("iOS");
}

void QtQuickControls2IOSStylePlugin::initializeTheme(QQuickTheme *theme)
{
    m_theme.initialize(theme);
}

void QtQuickControls2IOSStylePlugin::updateTheme()
{
    QQuickTheme *theme = QQuickTheme::instance();
    m_theme.initialize(theme);
}

QT_END_NAMESPACE

#include "qtquickcontrols2iosstyleplugin.moc"
