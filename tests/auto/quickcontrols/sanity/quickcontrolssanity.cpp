// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quickcontrolssanity.h"

#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <memory>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static constexpr LoggerWarningId qmlControlsSanity{ "QuickControlsSanity.controls-sanity" };

AnchorsElementPass::AnchorsElementPass(QQmlSA::PassManager *manager)
    : QQmlSA::ElementPass(manager), m_item(resolveType("QtQuick"_L1, "Item"_L1))
{
}

bool AnchorsElementPass::shouldRun(const QQmlSA::Element &element)
{
    return !m_item.isNull() && element->inherits(m_item)
            && element->hasOwnPropertyBindings("anchors"_L1);
}

void AnchorsElementPass::run(const QQmlSA::Element &element)
{
    const auto anchorBindings = element->propertyBindings("anchors"_L1);
    for (auto &&anchors : anchorBindings) {
        emitWarning(u"Using anchors here"_s, qmlControlsSanity, anchors.sourceLocation());
    }
}

SignalHandlerPass::SignalHandlerPass(QQmlSA::PassManager *manager)
    : QQmlSA::ElementPass(manager), m_qtobject(resolveType("QtQml"_L1, "QtObject"_L1))
{
}

bool SignalHandlerPass::shouldRun(const QQmlSA::Element &element)
{
    return !m_qtobject.isNull() && element->inherits(m_qtobject);
}

void SignalHandlerPass::run(const QQmlSA::Element &element)
{
    for (auto &&[propertyName, propertyBinding] :
         element->ownPropertyBindings().asKeyValueRange()) {

        // Already script binding, check if the script kind is signal handler
        if (propertyBinding.bindingType() == QQmlJSMetaPropertyBinding::Script) {
            if (propertyBinding.scriptKind() == QQmlJSMetaPropertyBinding::Script_SignalHandler) {
                emitWarning(u"Declared signal handler \"%1\""_s.arg(propertyName),
                            qmlControlsSanity, propertyBinding.sourceLocation());
            }
            continue;
        }

        // Current property is attached property, recursively go through attaching type
        if (propertyBinding.bindingType() == QQmlJSMetaPropertyBinding::AttachedProperty) {
            const auto scope = propertyBinding.attachingType();
            run(scope);
        }
    };
}

FunctionDeclarationPass::FunctionDeclarationPass(QQmlSA::PassManager *manager)
    : QQmlSA::ElementPass(manager)
{
}

bool FunctionDeclarationPass::shouldRun(const QQmlSA::Element &element)
{
    Q_UNUSED(element);
    return true;
}

void FunctionDeclarationPass::run(const QQmlSA::Element &element)
{
    for (auto &&method : element->ownMethods()) {
        if (method.methodType() != QQmlJSMetaMethod::Method)
            continue;

        emitWarning(u"Declared function \"%1\""_s.arg(method.methodName()), qmlControlsSanity,
                    element->sourceLocation());
    }
}

void QuickControlsSanityPlugin::registerPasses(QQmlSA::PassManager *manager,
                                               const QQmlSA::Element &rootElement)
{
    Q_UNUSED(rootElement);
    if (manager->hasImportedModule("QtQuick"_L1)) {
        manager->registerElementPass(std::make_unique<AnchorsElementPass>(manager));
    }

    manager->registerElementPass(std::make_unique<SignalHandlerPass>(manager));
    manager->registerElementPass(std::make_unique<FunctionDeclarationPass>(manager));
}

QT_END_NAMESPACE

#include "moc_quickcontrolssanity.cpp"
