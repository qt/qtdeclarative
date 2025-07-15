// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfluentwinui3theme_p.h"

#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

#include <QtQuickControls2FluentWinUI3StyleImpl/private/qquickfluentwinui3focusframe_p.h>

QT_BEGIN_NAMESPACE

extern void qml_register_types_QtQuick_Controls_FluentWinUI3();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Controls_FluentWinUI3);

class QtQuickControls2FluentWinUI3StylePlugin : public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2FluentWinUI3StylePlugin(QObject *parent = nullptr);

    QString name() const override;
    void initializeEngine(QQmlEngine *engine, const char *uri) override;
    void initializeTheme(QQuickTheme *theme) override;
    void updateTheme() override;

    QQuickFluentWinUI3Theme theme;
    QScopedPointer<QQuickFluentWinUI3FocusFrame> m_focusFrame;
};

QtQuickControls2FluentWinUI3StylePlugin::QtQuickControls2FluentWinUI3StylePlugin(QObject *parent) : QQuickStylePlugin(parent)
{
    volatile auto registration = &qml_register_types_QtQuick_Controls_FluentWinUI3;
    Q_UNUSED(registration);
}

QString QtQuickControls2FluentWinUI3StylePlugin::name() const
{
    return QStringLiteral("FluentWinUI3");
}

void QtQuickControls2FluentWinUI3StylePlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    QQuickStylePlugin::initializeEngine(engine, uri);
    m_focusFrame.reset(new QQuickFluentWinUI3FocusFrame());
}

void QtQuickControls2FluentWinUI3StylePlugin::initializeTheme(QQuickTheme *theme)
{
    this->theme.initialize(theme);
}

void QtQuickControls2FluentWinUI3StylePlugin::updateTheme()
{
    auto *theme = QQuickTheme::instance();
    if (theme)
        theme->setPalette(QQuickTheme::System, this->theme.initializeDefaultPalette());
    // TODO: Update the font too if needed
}

QT_END_NAMESPACE

#include "qtquickcontrols2fluentwinui3styleplugin.moc"
