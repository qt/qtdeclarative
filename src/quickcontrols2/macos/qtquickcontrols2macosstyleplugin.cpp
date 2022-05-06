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

#include <QtQml/qqml.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickControls2/qquickstyle.h>

extern void qml_register_types_QtQuick_Controls_macOS();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Controls_macOS);

QT_BEGIN_NAMESPACE

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
