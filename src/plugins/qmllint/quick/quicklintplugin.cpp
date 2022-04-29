/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "quicklintplugin.h"

using namespace Qt::StringLiterals;

ForbiddenChildrenPropertyValidatorPass::ForbiddenChildrenPropertyValidatorPass(
        QQmlSA::PassManager *manager)
    : QQmlSA::ElementPass(manager)
{
}

void ForbiddenChildrenPropertyValidatorPass::addWarning(QAnyStringView moduleName,
                                                        QAnyStringView typeName,
                                                        QAnyStringView propertyName,
                                                        QAnyStringView warning)
{
    auto element = resolveType(moduleName, typeName);
    if (!element.isNull())
        m_types[element].append({ propertyName.toString(), warning.toString() });
}

bool ForbiddenChildrenPropertyValidatorPass::shouldRun(const QQmlSA::Element &element)
{
    if (!element->parentScope())
        return false;

    for (const auto pair : m_types.asKeyValueRange()) {
        if (element->parentScope()->inherits(pair.first))
            return true;
    }

    return false;
}

void ForbiddenChildrenPropertyValidatorPass::run(const QQmlSA::Element &element)
{
    for (const auto elementPair : m_types.asKeyValueRange()) {
        const QQmlSA::Element &type = elementPair.first;
        if (!element->parentScope()->inherits(type))
            continue;

        for (const auto &warning : elementPair.second) {
            if (!element->hasOwnPropertyBindings(warning.propertyName))
                continue;

            auto bindings = element->ownPropertyBindings(warning.propertyName);

            emitWarning(warning.message, bindings.first->sourceLocation());
        }
        break;
    }
}

AttachedPropertyTypeValidatorPass::AttachedPropertyTypeValidatorPass(QQmlSA::PassManager *manager)
    : QQmlSA::ElementPass(manager)
{
}

void AttachedPropertyTypeValidatorPass::addWarning(
        QAnyStringView attachedTypeName,
        QList<AttachedPropertyTypeValidatorPass::TypeDescription> allowedTypes,
        QAnyStringView warning)
{
    AttachedPropertyTypeValidatorPass::Warning warningInfo;
    warningInfo.message = warning.toString();

    for (const TypeDescription &description : allowedTypes) {
        auto type = resolveType(description.module, description.name);

        if (type.isNull())
            continue;

        warningInfo.allowedTypes.push_back(type);
    }

    m_attachedTypes[attachedTypeName.toString()] = warningInfo;
}

bool AttachedPropertyTypeValidatorPass::shouldRun(const QQmlSA::Element &element)
{
    for (const auto pair : m_attachedTypes.asKeyValueRange()) {
        if (element->hasOwnPropertyBindings(pair.first))
            return true;
    }

    return false;
}

void AttachedPropertyTypeValidatorPass::run(const QQmlSA::Element &element)
{
    for (const auto pair : m_attachedTypes.asKeyValueRange()) {
        if (element->hasOwnPropertyBindings(pair.first)) {
            bool hasAllowedType = false;
            for (const QQmlSA::Element &type : pair.second.allowedTypes) {
                if (element->inherits(type)) {
                    hasAllowedType = true;
                    break;
                }
            }
            if (!hasAllowedType) {
                auto binding = *element->ownPropertyBindings(pair.first).first;
                emitWarning(pair.second.message, binding.sourceLocation());
            }
        }
    }
}

ControlsNativeValidatorPass::ControlsNativeValidatorPass(QQmlSA::PassManager *manager)
    : QQmlSA::ElementPass(manager)
{
    m_elements = {
        ControlElement { "Control",
                         QStringList { "background", "contentItem", "leftPadding", "rightPadding",
                                       "topPadding", "bottomPadding", "horizontalPadding",
                                       "verticalPadding", "padding" },
                         false, true },
        ControlElement { "Button", QStringList { "indicator" } },
        ControlElement {
                "ApplicationWindow",
                QStringList { "background", "contentItem", "header", "footer", "menuBar" } },
        ControlElement { "ComboBox", QStringList { "indicator" } },
        ControlElement { "Dial", QStringList { "handle" } },
        ControlElement { "Dialog", QStringList { "header", "footer" } },
        ControlElement { "GroupBox", QStringList { "label" } },
        ControlElement { "$internal$.QQuickIndicatorButton", QStringList { "indicator" }, false },
        ControlElement { "Label", QStringList { "background" } },
        ControlElement { "MenuItem", QStringList { "arrow" } },
        ControlElement { "Page", QStringList { "header", "footer" } },
        ControlElement { "Popup", QStringList { "background", "contentItem" } },
        ControlElement { "RangeSlider", QStringList { "handle" } },
        ControlElement { "Slider", QStringList { "handle" } },
        ControlElement { "$internal$.QQuickSwipe",
                         QStringList { "leftItem", "behindItem", "rightItem" }, false },
        ControlElement { "TextArea", QStringList { "background" } },
        ControlElement { "TextField", QStringList { "background" } },
    };

    for (const QString &module : { u"QtQuick.Controls.macOS"_s, u"QtQuick.Controls.Windows"_s }) {
        if (!manager->hasImportedModule(module))
            continue;

        QQmlSA::Element control = resolveType(module, "Control");

        for (ControlElement &element : m_elements) {
            auto type = resolveType(element.isInModuleControls ? module : "QtQuick.Templates",
                                    element.name);

            if (type.isNull())
                continue;

            element.inheritsControl = !element.isControl && type->inherits(control);
            element.element = type;
        }

        m_elements.removeIf([](const ControlElement &element) { return element.element.isNull(); });

        break;
    }
}

bool ControlsNativeValidatorPass::shouldRun(const QQmlSA::Element &element)
{
    for (const ControlElement &controlElement : m_elements) {
        // If our element inherits control, we don't have to individually check for them here.
        if (controlElement.inheritsControl)
            continue;
        if (element->inherits(controlElement.element))
            return true;
    }
    return false;
}

void ControlsNativeValidatorPass::run(const QQmlSA::Element &element)
{
    for (const ControlElement &controlElement : m_elements) {
        if (element->inherits(controlElement.element)) {
            for (const QString &propertyName : controlElement.restrictedProperties) {
                if (element->hasOwnPropertyBindings(propertyName)) {
                    emitWarning(QStringLiteral("Not allowed to override \"%1\" because native "
                                               "styles cannot be customized: See "
                                               "https://doc-snapshots.qt.io/qt6-dev/"
                                               "qtquickcontrols2-customize.html#customization-"
                                               "reference for more information.")
                                        .arg(propertyName),
                                element->sourceLocation());
                }
            }
            // Since all the different types we have rules for don't inherit from each other (except
            // for Control) we don't have to keep checking whether other types match once we've
            // found one that has been inherited from.
            if (!controlElement.isControl)
                break;
        }
    }
}

AnchorsValidatorPass::AnchorsValidatorPass(QQmlSA::PassManager *manager)
    : QQmlSA::ElementPass(manager)
{
    m_item = resolveType("QtQuick", "Item");
}

bool AnchorsValidatorPass::shouldRun(const QQmlSA::Element &element)
{
    return !m_item.isNull() && element->inherits(m_item)
            && element->hasOwnPropertyBindings(u"anchors"_s);
}

void AnchorsValidatorPass::run(const QQmlSA::Element &element)
{
    enum BindingLocation { Exists = 1, Own = (1 << 1) };
    QHash<QString, qint8> bindings;

    const QStringList properties = { u"left"_s,    u"right"_s,  u"horizontalCenter"_s,
                                     u"top"_s,     u"bottom"_s, u"verticalCenter"_s,
                                     u"baseline"_s };

    QList<QQmlJSMetaPropertyBinding> anchorBindings = element->propertyBindings(u"anchors"_s);

    for (qsizetype i = anchorBindings.size() - 1; i >= 0; i--) {
        auto groupType = anchorBindings[i].groupType();
        if (groupType == nullptr)
            continue;

        for (const QString &name : properties) {
            auto pair = groupType->ownPropertyBindings(name);
            if (pair.first == pair.second)
                continue;
            bool isUndefined = false;
            for (auto it = pair.first; it != pair.second; it++) {
                if (it->bindingType() == QQmlJSMetaPropertyBinding::Script
                    && it->scriptValueType() == QQmlJSMetaPropertyBinding::ScriptValue_Undefined) {
                    isUndefined = true;
                    break;
                }
            }

            if (isUndefined)
                bindings[name] = 0;
            else
                bindings[name] |= Exists | ((i == 0) ? Own : 0);
        }
    }

    auto ownSourceLocation = [&](QStringList properties) {
        QQmlJS::SourceLocation warnLoc;
        for (const QString &name : properties) {
            if (bindings[name] & Own) {
                QQmlSA::Element groupType = anchorBindings[0].groupType();
                auto bindingRange = groupType->ownPropertyBindings(name);
                Q_ASSERT(bindingRange.first != bindingRange.second);
                warnLoc = bindingRange.first->sourceLocation();
                break;
            }
        }
        return warnLoc;
    };

    if ((bindings[u"left"_s] & bindings[u"right"_s] & bindings[u"horizontalCenter"_s]) & Exists) {
        QQmlJS::SourceLocation warnLoc =
                ownSourceLocation({ u"left"_s, u"right"_s, u"horizontalCenter"_s });

        if (warnLoc.isValid()) {
            emitWarning(
                    "Cannot specify left, right, and horizontalCenter anchors at the same time.",
                    warnLoc);
        }
    }

    if ((bindings[u"top"_s] & bindings[u"bottom"_s] & bindings[u"verticalCenter"_s]) & Exists) {
        QQmlJS::SourceLocation warnLoc =
                ownSourceLocation({ u"top"_s, u"bottom"_s, u"verticalCenter"_s });
        if (warnLoc.isValid()) {
            emitWarning("Cannot specify top, bottom, and verticalCenter anchors at the same time.",
                        warnLoc);
        }
    }

    if ((bindings[u"baseline"_s] & (bindings[u"bottom"_s] | bindings[u"verticalCenter"_s]))
        & Exists) {
        QQmlJS::SourceLocation warnLoc =
                ownSourceLocation({ u"baseline"_s, u"bottom"_s, u"verticalCenter"_s });
        if (warnLoc.isValid()) {
            emitWarning("Baseline anchor cannot be used in conjunction with top, bottom, or "
                        "verticalCenter anchors.",
                        warnLoc);
        }
    }
}

void QmlLintQuickPlugin::registerPasses(QQmlSA::PassManager *manager,
                                        const QQmlSA::Element &rootElement)
{
    const bool hasQuick = manager->hasImportedModule("QtQuick");
    const bool hasQuickLayouts = manager->hasImportedModule("QtQuick.Layouts");
    const bool hasQuickControls = manager->hasImportedModule("QtQuick.Templates")
            || manager->hasImportedModule("QtQuick.Controls");

    Q_UNUSED(rootElement);

    if (hasQuick) {
        manager->registerElementPass(std::make_unique<AnchorsValidatorPass>(manager));

        auto forbiddenChildProperty =
                std::make_unique<ForbiddenChildrenPropertyValidatorPass>(manager);

        for (const QString &element : { u"Grid"_s, u"Flow"_s }) {
            for (const QString &property : { u"anchors"_s, u"x"_s, u"y"_s }) {
                forbiddenChildProperty->addWarning(
                        "QtQuick", element, property,
                        u"Cannot specify %1 for items inside %2. %2 will not function."_s.arg(
                                property, element));
            }
        }

        if (hasQuickLayouts) {
            forbiddenChildProperty->addWarning(
                    "QtQuick.Layouts", "Layout", "anchors",
                    "Detected anchors on an item that is managed by a layout. This is undefined "
                    u"behavior; use Layout.alignment instead.");
            forbiddenChildProperty->addWarning(
                    "QtQuick.Layouts", "Layout", "x",
                    "Detected x on an item that is managed by a layout. This is undefined "
                    u"behavior; use Layout.leftMargin or Layout.rightMargin instead.");
            forbiddenChildProperty->addWarning(
                    "QtQuick.Layouts", "Layout", "y",
                    "Detected y on an item that is managed by a layout. This is undefined "
                    u"behavior; use Layout.topMargin or Layout.bottomMargin instead.");
            forbiddenChildProperty->addWarning(
                    "QtQuick.Layouts", "Layout", "width",
                    "Detected width on an item that is managed by a layout. This is undefined "
                    u"behavior; use implicitWidth or Layout.preferredWidth instead.");
            forbiddenChildProperty->addWarning(
                    "QtQuick.Layouts", "Layout", "height",
                    "Detected height on an item that is managed by a layout. This is undefined "
                    u"behavior; use implictHeight or Layout.preferredHeight instead.");
        }

        manager->registerElementPass(std::move(forbiddenChildProperty));
    }

    auto attachedPropertyType = std::make_unique<AttachedPropertyTypeValidatorPass>(manager);

    if (hasQuick) {
        attachedPropertyType->addWarning("Accessible", { { "QtQuick", "Item" } },
                                         "Accessible must be attached to an Item");
        attachedPropertyType->addWarning(
                "LayoutMirroring", { { "QtQuick", "Item" }, { "QtQuick", "Window" } },
                "LayoutDirection attached property only works with Items and Windows");
        attachedPropertyType->addWarning("EnterKey", { { "QtQuick", "Item" } },
                                         "EnterKey attached property only works with Items");
    }

    if (hasQuickLayouts) {
        attachedPropertyType->addWarning("Layout", { { "QtQuick", "Item" } },
                                         "Layout must be attached to Item elements");
    }

    if (hasQuickControls) {
        attachedPropertyType->addWarning(
                "ScrollBar", { { "QtQuick", "Flickable" }, { "QtQuick.Templates", "ScrollView" } },
                "ScrollBar must be attached to a Flickable or ScrollView");
        attachedPropertyType->addWarning("ScrollIndicator", { { "QtQuick", "Flickable" } },
                                         "ScrollIndicator must be attached to a Flickable");
        attachedPropertyType->addWarning("SplitView", { { "QtQuick", "Item" } },
                                         "SplitView attached property only works with Items");
        attachedPropertyType->addWarning("StackView", { { "QtQuick", "Item" } },
                                         "StackView attached property only works with Items");
        attachedPropertyType->addWarning("ToolTip", { { "QtQuick", "Item" } },
                                         "ToolTip must be attached to an Item");
    }

    manager->registerElementPass(std::move(attachedPropertyType));

    if (manager->hasImportedModule(u"QtQuick.Controls.macOS"_s)
        || manager->hasImportedModule(u"QtQuick.Controls.Windows"_s))
        manager->registerElementPass(std::make_unique<ControlsNativeValidatorPass>(manager));
}
