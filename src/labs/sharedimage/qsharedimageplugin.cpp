/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#include <QtLabsSharedImage/private/qtlabssharedimageglobal_p.h>
#include <QtLabsSharedImage/private/qsharedimageprovider_p.h>

#include <qqmlextensionplugin.h>
#include <qqmlengine.h>

QT_BEGIN_NAMESPACE

class QtQuickSharedImagePlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)
public:
    QtQuickSharedImagePlugin(QObject *parent = nullptr) : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_Qt_labs_sharedimage;
        Q_UNUSED(registration);
    }

    void initializeEngine(QQmlEngine *engine, const char *uri) override
    {
        Q_UNUSED(uri);
        engine->addImageProvider("shared", new SharedImageProvider);
    }
};

QT_END_NAMESPACE

#include "qsharedimageplugin.moc"
