/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qquickfusionstyle_p.h"
#include "qquickfusiontheme_p.h"

#include <QtQml/qqml.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

extern void qml_register_types_QtQuick_Controls_Fusion();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Controls_Fusion);

QT_BEGIN_NAMESPACE

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
