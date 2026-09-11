// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef IMAGEPROVIDER_H
#define IMAGEPROVIDER_H

#include <qqmlextensionplugin.h>

class ImageProviderExtensionPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)
public:
    void initializeEngine(QQmlEngine *engine, const char *uri) override;
};

#endif // IMAGEPROVIDER_H
