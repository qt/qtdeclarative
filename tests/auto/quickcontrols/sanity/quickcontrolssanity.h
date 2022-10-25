// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QUICKCONTROLSSANITY_H
#define QUICKCONTROLSSANITY_H

#include <QtCore/qplugin.h>
#include <QtQmlCompiler/private/qqmlsa_p.h>

QT_BEGIN_NAMESPACE

class QuickControlsSanityPlugin : public QObject, public QQmlSA::LintPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QmlLintPluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(QQmlSA::LintPlugin)

public:
    void registerPasses(QQmlSA::PassManager *manager, const QQmlSA::Element &rootElement) override;
};

class AnchorsElementPass : public QQmlSA::ElementPass
{
public:
    AnchorsElementPass(QQmlSA::PassManager *manager);

    bool shouldRun(const QQmlSA::Element &element) override;
    void run(const QQmlSA::Element &element) override;

private:
    QQmlSA::Element m_item;
};

class SignalHandlerPass : public QQmlSA::ElementPass
{
public:
    SignalHandlerPass(QQmlSA::PassManager *manager);

    bool shouldRun(const QQmlSA::Element &element) override;
    void run(const QQmlSA::Element &element) override;

private:
    QQmlSA::Element m_qtobject;
};

class FunctionDeclarationPass : public QQmlSA::ElementPass
{
public:
    FunctionDeclarationPass(QQmlSA::PassManager *manager);

    bool shouldRun(const QQmlSA::Element &element) override;
    void run(const QQmlSA::Element &element) override;
};

QT_END_NAMESPACE

#endif // QUICKCONTROLSSANITY_H
