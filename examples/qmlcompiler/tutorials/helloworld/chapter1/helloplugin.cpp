// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "helloplugin.h"
#include <QDebug>

using namespace Qt::StringLiterals;

static constexpr QQmlSA::LoggerWarningId helloWorld { "Plugin.HelloWorld.hello-world" };

void HelloWorldPlugin::registerPasses(QQmlSA::PassManager *manager, const QQmlSA::Element &rootElement)
{
    const bool pluginIsEnabled = manager->isCategoryEnabled(helloWorld);
    qDebug() << "Hello World plugin is" << (pluginIsEnabled ? "enabled" : "disabled");
    if (!pluginIsEnabled)
        return; // skip registration if the plugin is disabled anyway
    // here we will later register our passes
}

#include "moc_helloplugin.cpp"
