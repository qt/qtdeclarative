// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef LINTPLUGIN_H
#define LINTPLUGIN_H

#include <QtPlugin>
#include <QtCore/qobject.h>
#include <QtQmlCompiler/qqmlsa.h>

class LintPlugin : public QObject, public QQmlSA::LintPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QmlLintPluginInterface_iid FILE "testPlugin.json")
    Q_INTERFACES(QQmlSA::LintPlugin)

public:
    void registerPasses(QQmlSA::PassManager *manager, const QQmlSA::Element &rootElement) override;
};

#endif // LINTPLUGIN_H
