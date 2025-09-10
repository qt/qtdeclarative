// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickimaginestyle_p.h"
#include "qquickimaginetheme_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtQml/qqml.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>

QT_BEGIN_NAMESPACE

extern void qml_register_types_QtQuick_Controls_Imagine();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Controls_Imagine);

class QtQuickControls2ImagineStylePlugin : public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2ImagineStylePlugin(QObject *parent = nullptr);

    QString name() const override;
    void initializeTheme(QQuickTheme *theme) override;

    QQuickImagineTheme theme;
};

QtQuickControls2ImagineStylePlugin::QtQuickControls2ImagineStylePlugin(QObject *parent) : QQuickStylePlugin(parent)
{
    volatile auto registration = &qml_register_types_QtQuick_Controls_Imagine;
    Q_UNUSED(registration);
}

QString QtQuickControls2ImagineStylePlugin::name() const
{
    return QStringLiteral("Imagine");
}

void QtQuickControls2ImagineStylePlugin::initializeTheme(QQuickTheme *theme)
{
    this->theme.initialize(theme);
}

QT_END_NAMESPACE

#include "qtquickcontrols2imaginestyleplugin.moc"
