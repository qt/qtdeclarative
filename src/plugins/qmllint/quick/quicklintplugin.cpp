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

LayoutChildrenValidatorPass::LayoutChildrenValidatorPass(QQmlSA::PassManager *manager)
    : QQmlSA::ElementPass(manager)
{
    m_layout = resolveType("QtQuick.Layouts", "Layout");
}

bool LayoutChildrenValidatorPass::shouldRun(const QQmlSA::Element &element)
{
    return !m_layout.isNull() && element->parentScope()
            && element->parentScope()->inherits(m_layout);
}

void LayoutChildrenValidatorPass::run(const QQmlSA::Element &element)
{
    auto bindings = element->propertyBindings(u"anchors"_qs);
    if (!bindings.empty())
        emitWarning(u"Detected anchors on an item that is managed by a layout. This is undefined "
                    u"behavior; use Layout.alignment instead.",
                    element->sourceLocation());
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

    for (const QString &module : { u"QtQuick.Controls.macOS"_qs, u"QtQuick.Controls.Windows"_qs }) {
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
            && element->hasOwnPropertyBindings(u"anchors"_qs);
}

void AnchorsValidatorPass::run(const QQmlSA::Element &element)
{
    QQmlJS::SourceLocation left, right, hCenter;
    QQmlJS::SourceLocation top, bottom, vCenter;
    QQmlJS::SourceLocation baseline;
    auto bindings = element->ownPropertyBindings(u"anchors"_qs);
    for (auto it = bindings.first; it != bindings.second; it++) {
        for (const auto &groupBinding : it->groupType()->ownPropertyBindings()) {
            const QString propertyName = groupBinding.propertyName();
            const QQmlJS::SourceLocation srcLoc = groupBinding.sourceLocation();
            if (propertyName == u"horizontalCenter")
                hCenter = srcLoc;
            if (propertyName == u"verticalCenter")
                vCenter = srcLoc;
            if (propertyName == u"left")
                left = srcLoc;
            else if (propertyName == u"right")
                right = srcLoc;
            else if (propertyName == u"top")
                top = srcLoc;
            else if (propertyName == u"bottom")
                bottom = srcLoc;
            else if (propertyName == u"baseline")
                baseline = srcLoc;

            if (propertyName == u"horizontalCenter" || propertyName == u"verticalCenter") {
                if (groupBinding.bindingType() == QQmlJSMetaPropertyBinding::Null) {
                    emitWarning("Cannot anchor to a null item.", srcLoc);
                }
            }
        }
    }

    if (left.isValid() && right.isValid() && hCenter.isValid()) {
        emitWarning("Cannot specify left, right, and horizontalCenter anchors at the same time.",
                    hCenter);
    }

    if (top.isValid() && bottom.isValid() && vCenter.isValid()) {
        emitWarning("Cannot specify top, bottom, and verticalCenter anchors at the same time.",
                    vCenter);
    }

    if (baseline.isValid() && (top.isValid() || bottom.isValid() || vCenter.isValid())) {
        emitWarning("Baseline anchor cannot be used in conjunction with top, bottom, or "
                    "verticalCenter anchors.",
                    baseline);
    }
}

void QmlLintQuickPlugin::registerPasses(QQmlSA::PassManager *manager,
                                        const QQmlSA::Element &rootElement)
{
    Q_UNUSED(rootElement);

    if (manager->hasImportedModule("QtQuick"))
        manager->registerElementPass(std::make_unique<AnchorsValidatorPass>(manager));

    if (manager->hasImportedModule(u"QtQuick.Layouts"_qs))
        manager->registerElementPass(std::make_unique<LayoutChildrenValidatorPass>(manager));

    if (manager->hasImportedModule(u"QtQuick.Controls.macOS"_qs)
        || manager->hasImportedModule(u"QtQuick.Controls.Windows"_qs))
        manager->registerElementPass(std::make_unique<ControlsNativeValidatorPass>(manager));
}
