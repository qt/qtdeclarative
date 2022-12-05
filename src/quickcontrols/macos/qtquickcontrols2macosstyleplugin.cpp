// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/qqml.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickControls2/qquickstyle.h>

QT_BEGIN_NAMESPACE

extern void qml_register_types_QtQuick_Controls_macOS();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Controls_macOS);

class QtQuickControls2MacOSStylePlugin : public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2MacOSStylePlugin(QObject *parent = nullptr);
    QString name() const override;
    void initializeTheme(QQuickTheme *theme) override;
};


QtQuickControls2MacOSStylePlugin::QtQuickControls2MacOSStylePlugin(QObject *parent):
    QQuickStylePlugin(parent)
{
    volatile auto registration = &qml_register_types_QtQuick_Controls_macOS;
    Q_UNUSED(registration);
}

QString QtQuickControls2MacOSStylePlugin::name() const
{
    return QStringLiteral("macOS");
}

void QtQuickControls2MacOSStylePlugin::initializeTheme(QQuickTheme */*theme*/)
{
}

QT_END_NAMESPACE

#include "qtquickcontrols2macosstyleplugin.moc"
