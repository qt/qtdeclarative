// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
