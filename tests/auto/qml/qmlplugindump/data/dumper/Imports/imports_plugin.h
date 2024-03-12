// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef IMPORTS_PLUGIN_H
#define IMPORTS_PLUGIN_H

#include <QQmlExtensionPlugin>

class ImportsPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override;
};

#endif // IMPORTS_PLUGIN_H

