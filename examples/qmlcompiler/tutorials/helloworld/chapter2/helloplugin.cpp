// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "helloplugin.h"
#include <QDebug>

using namespace Qt::StringLiterals;

static constexpr QQmlSA::LoggerWarningId helloWorld { "Plugin.HelloWorld.hello-world" };

class HelloWorldElementPass : public QQmlSA::ElementPass
{
public:
    HelloWorldElementPass(QQmlSA::PassManager *manager);
    bool shouldRun(const QQmlSA::Element &element) override;
    void run(const QQmlSA::Element &element) override;
private:
    QQmlSA::Element m_textType;
};

HelloWorldElementPass::HelloWorldElementPass(QQmlSA::PassManager *manager)
    : QQmlSA::ElementPass(manager)
{
    m_textType = resolveType("QtQuick", "Text");
}

bool HelloWorldElementPass::shouldRun(const QQmlSA::Element &element)
{
    if (!element.inherits(m_textType))
        return false;
    if (!element.hasOwnPropertyBindings(u"text"_s))
        return false;
    return true;
}

void HelloWorldElementPass::run(const QQmlSA::Element &element)
{
    auto textBindings = element.ownPropertyBindings(u"text"_s);
    for (const auto &textBinding: textBindings) {
        if (textBinding.bindingType() != QQmlSA::BindingType::StringLiteral)
            continue;
        if (textBinding.stringValue() != u"Hello world!"_s)
            emitWarning("Incorrect greeting", helloWorld, textBinding.sourceLocation());
    }
}

void HelloWorldPlugin::registerPasses(QQmlSA::PassManager *manager, const QQmlSA::Element &rootElement)
{
    const bool pluginIsEnabled = manager->isCategoryEnabled(helloWorld);
    qDebug() << "Hello World plugin is" << (pluginIsEnabled ? "enabled" : "disabled");
    if (!pluginIsEnabled)
        return; // skip registration if the plugin is disabled anyway
    manager->registerElementPass(std::make_unique<HelloWorldElementPass>(manager));
}

#include "moc_helloplugin.cpp"
