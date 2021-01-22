/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
**
**
**
****************************************************************************/

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>

#include <private/qquickparticlesmodule_p.h>

QT_BEGIN_NAMESPACE

//![class decl]
class QtQuick2ParticlesPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    QtQuick2ParticlesPlugin(QObject *parent = nullptr) : QQmlExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_QtQuick_Particles;
        Q_UNUSED(registration);
    }

    void registerTypes(const char *uri) override
    {
        Q_UNUSED(uri);
        QQuickParticlesModule::defineModule();
    }
};
//![class decl]

QT_END_NAMESPACE

#include "plugin.moc"
