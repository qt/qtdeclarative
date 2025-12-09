// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant reason:default

#include "quicklintplugin.h"
#include "qquickliteralbindingcheck_p.h"
#include <QtQmlCompiler/private/qqmlsasourcelocation_p.h>
#include <QtQmlCompiler/private/qqmljsutils_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static constexpr QQmlSA::LoggerWarningId quickLayoutPositioning { "Quick.layout-positioning" };
static constexpr QQmlSA::LoggerWarningId quickAttachedPropertyType { "Quick.attached-property-type" };
static constexpr QQmlSA::LoggerWarningId quickControlsNativeCustomize { "Quick.controls-native-customize" };
static constexpr QQmlSA::LoggerWarningId quickAnchorCombinations { "Quick.anchor-combinations" };
static constexpr QQmlSA::LoggerWarningId quickUnexpectedVarType { "Quick.unexpected-var-type" };
static constexpr QQmlSA::LoggerWarningId quickPropertyChangesParsed { "Quick.property-changes-parsed" };
static constexpr QQmlSA::LoggerWarningId quickControlsAttachedPropertyReuse { "Quick.controls-attached-property-reuse" };
static constexpr QQmlSA::LoggerWarningId quickAttachedPropertyReuse { "Quick.attached-property-reuse" };
static constexpr QQmlSA::LoggerWarningId quickColor { "Quick.color" };
static constexpr QQmlSA::LoggerWarningId quickStateNoChildItem { "Quick.state-no-child-item" };

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
    if (!element.parentScope())
        return false;

    for (const auto &pair : std::as_const(m_types).asKeyValueRange()) {
        if (element.parentScope().inherits(pair.first))
            return true;
    }

    return false;
}

void ForbiddenChildrenPropertyValidatorPass::run(const QQmlSA::Element &element)
{
    for (const auto &elementPair : std::as_const(m_types).asKeyValueRange()) {
        const QQmlSA::Element &type = elementPair.first;
        const QQmlSA::Element parentScope = element.parentScope();

        // If the parent's default property is not what we think it is, then we can't say whether
        // the element in question is actually a visual child of the (document) parent scope.
        const QQmlSA::Property defaultProperty
                = parentScope.property(parentScope.defaultPropertyName());
        if (defaultProperty != type.property(type.defaultPropertyName()))
            continue;

        if (!element.parentScope().inherits(type))
            continue;

        for (const auto &warning : elementPair.second) {
            if (!element.hasOwnPropertyBindings(warning.propertyName))
                continue;

            const auto bindings = element.ownPropertyBindings(warning.propertyName);
            const auto firstBinding = bindings.constBegin().value();
            emitWarning(warning.message, quickLayoutPositioning, firstBinding.sourceLocation());
        }
        break;
    }
}

AttachedPropertyTypeValidatorPass::AttachedPropertyTypeValidatorPass(QQmlSA::PassManager *manager)
    : QQmlSA::PropertyPass(manager)
{
}

QString AttachedPropertyTypeValidatorPass::addWarning(TypeDescription attachType,
                                                      QList<TypeDescription> allowedTypes,
                                                      bool allowInDelegate, QAnyStringView warning)
{
    QVarLengthArray<QQmlSA::Element, 4> elements;

    const QQmlSA::Element attachedType = resolveAttached(attachType.module, attachType.name);
    if (!attachedType) {
        emitWarning(
                "Cannot find attached type for %1/%2"_L1.arg(attachType.module, attachType.name),
                quickAttachedPropertyType);
        return QString();
    }

    for (const TypeDescription &desc : allowedTypes) {
        const QQmlSA::Element type = resolveType(desc.module, desc.name);
        if (type.isNull())
            continue;
        elements.push_back(type);
    }

    m_attachedTypes.insert(
            { std::make_pair<>(attachedType.internalId(),
                               Warning{ elements, allowInDelegate, warning.toString() }) });

    return attachedType.internalId();
}

void AttachedPropertyTypeValidatorPass::checkWarnings(const QQmlSA::Element &element,
                                                      const QQmlSA::Element &scopeUsedIn,
                                                      const QQmlSA::SourceLocation &location)
{
    auto warning = m_attachedTypes.constFind(element.internalId());
    if (warning == m_attachedTypes.cend())
        return;
    for (const QQmlSA::Element &type : warning->allowedTypes) {
        if (scopeUsedIn.inherits(type))
            return;
    }
    // You can use e.g. Layout.leftMargin: 4 in PropertyChanges;
    // custom parser can do arbitrary things with their contained bindings
    if ( QQmlJSScope::scope(scopeUsedIn)->isInCustomParserParent() )
        return;

    if (warning->allowInDelegate) {
        if (scopeUsedIn.isPropertyRequired(u"index"_s)
            || scopeUsedIn.isPropertyRequired(u"model"_s))
            return;

        // If the scope is at the root level, we cannot know whether it will be used
        // as a delegate or not.
        if (scopeUsedIn.isFileRootComponent())
            return;

        for (const QQmlSA::Binding &binding :
             scopeUsedIn.parentScope().propertyBindings(u"delegate"_s)) {
            if (!binding.hasObject())
                continue;
            if (binding.objectType() == scopeUsedIn)
                return;
        }
    }

    emitWarning(warning->message, quickAttachedPropertyType, location);
}

void AttachedPropertyTypeValidatorPass::onBinding(const QQmlSA::Element &element,
                                                  const QString &propertyName,
                                                  const QQmlSA::Binding &binding,
                                                  const QQmlSA::Element &bindingScope,
                                                  const QQmlSA::Element &value)
{
    Q_UNUSED(value)

    // We can only analyze simple attached bindings since we don't see
    // the grouped and attached properties that lead up to this here.
    //
    // TODO: This is very crude.
    //       We should add API for grouped and attached properties.
    if (propertyName.count(QLatin1Char('.')) > 1)
        return;

    checkWarnings(bindingScope.baseType(), element, binding.sourceLocation());
}

void AttachedPropertyTypeValidatorPass::onRead(const QQmlSA::Element &element,
                                               const QString &propertyName,
                                               const QQmlSA::Element &readScope,
                                               QQmlSA::SourceLocation location)
{
    // If the attachment does not have such a property or method then
    // it's either a more general error or an enum. Enums are fine.
    if (element.hasProperty(propertyName) || element.hasMethod(propertyName))
        checkWarnings(element, readScope, location);
}

void AttachedPropertyTypeValidatorPass::onWrite(const QQmlSA::Element &element,
                                                const QString &propertyName,
                                                const QQmlSA::Element &value,
                                                const QQmlSA::Element &writeScope,
                                                QQmlSA::SourceLocation location)
{
    Q_UNUSED(propertyName)
    Q_UNUSED(value)

    checkWarnings(element, writeScope, location);
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

            element.inheritsControl = !element.isControl && type.inherits(control);
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
        if (element.inherits(controlElement.element))
            return true;
    }
    return false;
}

void ControlsNativeValidatorPass::run(const QQmlSA::Element &element)
{
    for (const ControlElement &controlElement : m_elements) {
        if (element.inherits(controlElement.element)) {
            for (const QString &propertyName : controlElement.restrictedProperties) {
                if (element.hasOwnPropertyBindings(propertyName)) {
                    emitWarning(QStringLiteral("Not allowed to override \"%1\" because native "
                                               "styles cannot be customized: See "
                                               "https://doc-snapshots.qt.io/qt6-dev/"
                                               "qtquickcontrols-customize.html#customization-"
                                               "reference for more information.")
                                        .arg(propertyName),
                                quickControlsNativeCustomize, element.sourceLocation());
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
    , m_item(resolveType("QtQuick", "Item"))
{
}

bool AnchorsValidatorPass::shouldRun(const QQmlSA::Element &element)
{
    return !m_item.isNull() && element.inherits(m_item)
            && element.hasOwnPropertyBindings(u"anchors"_s);
}

void AnchorsValidatorPass::run(const QQmlSA::Element &element)
{
    enum BindingLocation { Exists = 1, Own = (1 << 1) };
    QHash<QString, qint8> bindings;

    const QStringList properties = { u"left"_s,    u"right"_s,  u"horizontalCenter"_s,
                                     u"top"_s,     u"bottom"_s, u"verticalCenter"_s,
                                     u"baseline"_s };

    QList<QQmlSA::Binding> anchorBindings = element.propertyBindings(u"anchors"_s);

    for (qsizetype i = anchorBindings.size() - 1; i >= 0; i--) {
        auto groupType = anchorBindings[i].groupType();
        if (groupType.isNull())
            continue;

        for (const QString &name : properties) {

            const auto &propertyBindings = groupType.ownPropertyBindings(name);
            if (propertyBindings.begin() == propertyBindings.end())
                continue;

            bool isUndefined = false;
            for (const auto &propertyBinding : propertyBindings) {
                if (propertyBinding.hasUndefinedScriptValue()) {
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

    auto ownSourceLocation = [&](QStringList properties) -> QQmlSA::SourceLocation {
        QQmlSA::SourceLocation warnLoc;

        for (const QString &name : properties) {
            if (bindings[name] & Own) {
                QQmlSA::Element groupType = QQmlSA::Element{ anchorBindings[0].groupType() };
                auto bindings = groupType.ownPropertyBindings(name);
                Q_ASSERT(bindings.begin() != bindings.end());
                warnLoc = bindings.begin().value().sourceLocation();
                break;
            }
        }
        return warnLoc;
    };

    if ((bindings[u"left"_s] & bindings[u"right"_s] & bindings[u"horizontalCenter"_s]) & Exists) {
        QQmlSA::SourceLocation warnLoc =
                ownSourceLocation({ u"left"_s, u"right"_s, u"horizontalCenter"_s });

        if (warnLoc.isValid()) {
            emitWarning(
                    "Cannot specify left, right, and horizontalCenter anchors at the same time.",
                    quickAnchorCombinations, warnLoc);
        }
    }

    if ((bindings[u"top"_s] & bindings[u"bottom"_s] & bindings[u"verticalCenter"_s]) & Exists) {
        QQmlSA::SourceLocation warnLoc =
                ownSourceLocation({ u"top"_s, u"bottom"_s, u"verticalCenter"_s });
        if (warnLoc.isValid()) {
            emitWarning("Cannot specify top, bottom, and verticalCenter anchors at the same time.",
                        quickAnchorCombinations, warnLoc);
        }
    }

    if ((bindings[u"baseline"_s] & (bindings[u"bottom"_s] | bindings[u"verticalCenter"_s]))
        & Exists) {
        QQmlSA::SourceLocation warnLoc =
                ownSourceLocation({ u"baseline"_s, u"bottom"_s, u"verticalCenter"_s });
        if (warnLoc.isValid()) {
            emitWarning("Baseline anchor cannot be used in conjunction with top, bottom, or "
                        "verticalCenter anchors.",
                        quickAnchorCombinations, warnLoc);
        }
    }
}

ControlsSwipeDelegateValidatorPass::ControlsSwipeDelegateValidatorPass(QQmlSA::PassManager *manager)
    : QQmlSA::ElementPass(manager)
    , m_swipeDelegate(resolveType("QtQuick.Controls", "SwipeDelegate"))
{
}

bool ControlsSwipeDelegateValidatorPass::shouldRun(const QQmlSA::Element &element)
{
    return !m_swipeDelegate.isNull() && element.inherits(m_swipeDelegate);
}

void ControlsSwipeDelegateValidatorPass::run(const QQmlSA::Element &element)
{
    for (const auto &property : { u"background"_s, u"contentItem"_s }) {
        for (const auto &binding : element.ownPropertyBindings(property)) {
            if (!binding.hasObject())
                continue;
            const QQmlSA::Element element = QQmlSA::Element{ binding.objectType() };
            const auto &bindings = element.propertyBindings(u"anchors"_s);
            if (bindings.isEmpty())
                continue;

            if (bindings.first().bindingType() != QQmlSA::BindingType::GroupProperty)
                continue;

            auto anchors = bindings.first().groupType();
            for (const auto &disallowed : { u"fill"_s, u"centerIn"_s, u"left"_s, u"right"_s }) {
                if (anchors.hasPropertyBindings(disallowed)) {
                    QQmlSA::SourceLocation location;
                    const auto &ownBindings = anchors.ownPropertyBindings(disallowed);
                    if (ownBindings.begin() != ownBindings.end()) {
                        location = ownBindings.begin().value().sourceLocation();
                    }

                    emitWarning(
                            u"SwipeDelegate: Cannot use horizontal anchors with %1; unable to layout the item."_s
                                    .arg(property),
                            quickAnchorCombinations, location);
                    break;
                }
            }
            break;
        }
    }

    const auto &swipe = element.ownPropertyBindings(u"swipe"_s);
    if (swipe.begin() == swipe.end())
        return;

    const auto firstSwipe = swipe.begin().value();
    if (firstSwipe.bindingType() != QQmlSA::BindingType::GroupProperty)
        return;

    auto group = firstSwipe.groupType();

    const std::array ownDirBindings = { group.ownPropertyBindings(u"right"_s),
                                        group.ownPropertyBindings(u"left"_s),
                                        group.ownPropertyBindings(u"behind"_s) };

    auto ownBindingIterator =
            std::find_if(ownDirBindings.begin(), ownDirBindings.end(),
                         [](const auto &bindings) { return bindings.begin() != bindings.end(); });

    if (ownBindingIterator == ownDirBindings.end())
        return;

    if (group.hasPropertyBindings(u"behind"_s)
        && (group.hasPropertyBindings(u"right"_s) || group.hasPropertyBindings(u"left"_s))) {
        emitWarning("SwipeDelegate: Cannot set both behind and left/right properties",
                    quickAnchorCombinations, ownBindingIterator->begin().value().sourceLocation());
    }
}

VarBindingTypeValidatorPass::VarBindingTypeValidatorPass(
        QQmlSA::PassManager *manager,
        const QMultiHash<QString, TypeDescription> &expectedPropertyTypes)
    : QQmlSA::PropertyPass(manager)
{
    QMultiHash<QString, QQmlSA::Element> propertyTypes;

    for (const auto &pair : expectedPropertyTypes.asKeyValueRange()) {
        const QQmlSA::Element propType = pair.second.module.isEmpty()
                ? resolveBuiltinType(pair.second.name)
                : resolveType(pair.second.module, pair.second.name);
        if (!propType.isNull())
            propertyTypes.insert(pair.first, propType);
    }

    m_expectedPropertyTypes = propertyTypes;
}

void VarBindingTypeValidatorPass::onBinding(const QQmlSA::Element &element,
                                            const QString &propertyName,
                                            const QQmlSA::Binding &binding,
                                            const QQmlSA::Element &bindingScope,
                                            const QQmlSA::Element &value)
{
    Q_UNUSED(element);
    Q_UNUSED(bindingScope);

    const auto range = m_expectedPropertyTypes.equal_range(propertyName);

    if (range.first == range.second)
        return;

    QQmlSA::Element bindingType;

    if (!value.isNull()) {
        bindingType = value;
    } else {
        if (QQmlSA::Binding::isLiteralBinding(binding.bindingType())) {
            bindingType = resolveLiteralType(binding);
        } else {
            switch (binding.bindingType()) {
            case QQmlSA::BindingType::Object:
                bindingType = QQmlSA::Element{ binding.objectType() };
                break;
            case QQmlSA::BindingType::Script:
                break;
            default:
                return;
            }
        }
    }

    if (std::find_if(range.first, range.second,
                     [&](const QQmlSA::Element &scope) { return bindingType.inherits(scope); })
        == range.second) {

        const bool bindingTypeIsComposite = bindingType.isComposite();
        if (bindingTypeIsComposite && !bindingType.baseType()) {
            /* broken module or missing import, there is nothing we
               can really check here, as something is amiss. We
               simply skip this binding, and assume that whatever
               caused the breakage here will already cause another
               warning somewhere else.
             */
            return;
        }
        const QString bindingTypeName =
                bindingTypeIsComposite ? bindingType.baseType().name()
                                       : bindingType.name();
        QStringList expectedTypeNames;

        for (auto it = range.first; it != range.second; it++)
            expectedTypeNames << it.value().name();

        emitWarning(u"Unexpected type for property \"%1\" expected %2 got %3"_s.arg(
                            propertyName, expectedTypeNames.join(u", "_s), bindingTypeName),
                    quickUnexpectedVarType, binding.sourceLocation());
    }
}

class ColorValidatorPass : public QQmlSA::PropertyPass
{
public:
    ColorValidatorPass(QQmlSA::PassManager *manager);

    void onBinding(const QQmlSA::Element &element, const QString &propertyName,
                   const QQmlSA::Binding &binding, const QQmlSA::Element &bindingScope,
                   const QQmlSA::Element &value) override;
private:
    QQmlSA::Element m_colorType;
    // we support both long and short hex codes for both RGB and ARGB,
    // so 3, 4, 6 and 8 hex digits are allowed
    static inline const QRegularExpression s_hexPattern{ "^#((([0-9A-Fa-f]{3}){1,2})|(([0-9A-Fa-f]{4}){1,2}))$"_L1 };
    // list taken from https://doc.qt.io/qt-6/qcolor.html#fromString
    QStringList m_colorNames = {
        u"aliceblue"_s,
        u"antiquewhite"_s,
        u"aqua"_s,
        u"aquamarine"_s,
        u"azure"_s,
        u"beige"_s,
        u"bisque"_s,
        u"black"_s,
        u"blanchedalmond"_s,
        u"blue"_s,
        u"blueviolet"_s,
        u"brown"_s,
        u"burlywood"_s,
        u"cadetblue"_s,
        u"chartreuse"_s,
        u"chocolate"_s,
        u"coral"_s,
        u"cornflowerblue"_s,
        u"cornsilk"_s,
        u"crimson"_s,
        u"cyan"_s,
        u"darkblue"_s,
        u"darkcyan"_s,
        u"darkgoldenrod"_s,
        u"darkgray"_s,
        u"darkgreen"_s,
        u"darkgrey"_s,
        u"darkkhaki"_s,
        u"darkmagenta"_s,
        u"darkolivegreen"_s,
        u"darkorange"_s,
        u"darkorchid"_s,
        u"darkred"_s,
        u"darksalmon"_s,
        u"darkseagreen"_s,
        u"darkslateblue"_s,
        u"darkslategray"_s,
        u"darkslategrey"_s,
        u"darkturquoise"_s,
        u"darkviolet"_s,
        u"deeppink"_s,
        u"deepskyblue"_s,
        u"dimgray"_s,
        u"dimgrey"_s,
        u"dodgerblue"_s,
        u"firebrick"_s,
        u"floralwhite"_s,
        u"forestgreen"_s,
        u"fuchsia"_s,
        u"gainsboro"_s,
        u"ghostwhite"_s,
        u"gold"_s,
        u"goldenrod"_s,
        u"gray"_s,
        u"green"_s,
        u"greenyellow"_s,
        u"grey"_s,
        u"honeydew"_s,
        u"hotpink"_s,
        u"indianred"_s,
        u"indigo"_s,
        u"ivory"_s,
        u"khaki"_s,
        u"lavender"_s,
        u"lavenderblush"_s,
        u"lawngreen"_s,
        u"lemonchiffon"_s,
        u"lightblue"_s,
        u"lightcoral"_s,
        u"lightcyan"_s,
        u"lightgoldenrodyellow"_s,
        u"lightgray"_s,
        u"lightgreen"_s,
        u"lightgrey"_s,
        u"lightpink"_s,
        u"lightsalmon"_s,
        u"lightseagreen"_s,
        u"lightskyblue"_s,
        u"lightslategray"_s,
        u"lightslategrey"_s,
        u"lightsteelblue"_s,
        u"lightyellow"_s,
        u"lime"_s,
        u"limegreen"_s,
        u"linen"_s,
        u"magenta"_s,
        u"maroon"_s,
        u"mediumaquamarine"_s,
        u"mediumblue"_s,
        u"mediumorchid"_s,
        u"mediumpurple"_s,
        u"mediumseagreen"_s,
        u"mediumslateblue"_s,
        u"mediumspringgreen"_s,
        u"mediumturquoise"_s,
        u"mediumvioletred"_s,
        u"midnightblue"_s,
        u"mintcream"_s,
        u"mistyrose"_s,
        u"moccasin"_s,
        u"navajowhite"_s,
        u"navy"_s,
        u"oldlace"_s,
        u"olive"_s,
        u"olivedrab"_s,
        u"orange"_s,
        u"orangered"_s,
        u"orchid"_s,
        u"palegoldenrod"_s,
        u"palegreen"_s,
        u"paleturquoise"_s,
        u"palevioletred"_s,
        u"papayawhip"_s,
        u"peachpuff"_s,
        u"peru"_s,
        u"pink"_s,
        u"plum"_s,
        u"powderblue"_s,
        u"purple"_s,
        u"red"_s,
        u"rosybrown"_s,
        u"royalblue"_s,
        u"saddlebrown"_s,
        u"salmon"_s,
        u"sandybrown"_s,
        u"seagreen"_s,
        u"seashell"_s,
        u"sienna"_s,
        u"silver"_s,
        u"skyblue"_s,
        u"slateblue"_s,
        u"slategray"_s,
        u"slategrey"_s,
        u"snow"_s,
        u"springgreen"_s,
        u"steelblue"_s,
        u"tan"_s,
        u"teal"_s,
        u"thistle"_s,
        u"tomato"_s,
        u"turquoise"_s,
        u"violet"_s,
        u"wheat"_s,
        u"white"_s,
        u"whitesmoke"_s,
        u"yellow"_s,
        u"yellowgreen"_s,
    };
};


ColorValidatorPass::ColorValidatorPass(QQmlSA::PassManager *manager)
    : PropertyPass(manager), m_colorType(resolveType("QtQuick"_L1, "color"_L1))
{
    Q_ASSERT_X(std::is_sorted(m_colorNames.cbegin(), m_colorNames.cend()), "ColorValidatorPass",
               "m_colorNames should be sorted!");
}

void ColorValidatorPass::onBinding(const QQmlSA::Element &element, const QString &propertyName,
                                   const QQmlSA::Binding &binding, const QQmlSA::Element &,
                                   const QQmlSA::Element &)
{
    if (binding.bindingType() != QQmlSA::BindingType::StringLiteral)
        return;
    const auto propertyType = element.property(propertyName).type();
    if (!propertyType || propertyType != m_colorType)
        return;

    QString colorName = binding.stringValue();
    // for "named" colors, QColor::fromString does not care about
    // the case
    if (!colorName.startsWith(u'#'))
        colorName = std::move(colorName).toLower();
    if (s_hexPattern.match(colorName).hasMatch())
        return;

    if (std::binary_search(m_colorNames.cbegin(), m_colorNames.cend(), colorName))
        return;

    if (colorName == u"transparent")
        return;

    auto suggestion = QQmlJSUtils::didYouMean(
            colorName, m_colorNames,
            QQmlSA::SourceLocationPrivate::sourceLocation(binding.sourceLocation()));

    emitWarningWithOptionalFix(*this, "Invalid color \"%1\"."_L1.arg(colorName), quickColor,
                               binding.sourceLocation(), suggestion);
}

void AttachedPropertyReuse::onRead(const QQmlSA::Element &element, const QString &propertyName,
                                   const QQmlSA::Element &readScope,
                                   QQmlSA::SourceLocation location)
{
    const auto range = usedAttachedTypes.equal_range(readScope);
    const auto attachedTypeAndLocation = std::find_if(
                range.first, range.second, [&](const ElementAndLocation &elementAndLocation) {
        return elementAndLocation.element == element;
    });
    if (attachedTypeAndLocation != range.second) {
        const QQmlSA::SourceLocation attachedLocation = attachedTypeAndLocation->location;

        // Ignore enum accesses, as these will not cause the attached object to be created.
        // Also ignore anything we cannot determine.
        if (!element.hasProperty(propertyName) && !element.hasMethod(propertyName))
            return;

        for (QQmlSA::Element scope = readScope.parentScope(); !scope.isNull();
             scope = scope.parentScope()) {
            const auto range = usedAttachedTypes.equal_range(scope);
            bool found = false;
            for (auto it = range.first; it != range.second; ++it) {
                if (it->element == element) {
                    found = true;
                    break;
                }
            }
            if (!found)
                continue;

            const QString id = resolveElementToId(scope, readScope);
            const QQmlSA::SourceLocation idInsertLocation{ attachedLocation.offset(), 0,
                                                           attachedLocation.startLine(),
                                                           attachedLocation.startColumn() };
            QQmlSA::FixSuggestion suggestion{ "Reference it by id instead:"_L1, idInsertLocation,
                                              id.isEmpty() ? u"<id>."_s : (id + '.'_L1) };

            if (id.isEmpty())
                suggestion.setHint("You first have to give the element an id"_L1);
            else
                suggestion.setAutoApplicable();

            emitWarning("Using attached type %1 already initialized in a parent scope."_L1.arg(
                                element.name()),
                        category, attachedLocation, suggestion);
            return;
        }

        return;
    }

    if (element.hasProperty(propertyName))
        return; // an actual property

    QQmlSA::Element type = resolveTypeInFileScope(propertyName);
    QQmlSA::Element attached = resolveAttachedInFileScope(propertyName);
    if (!type || !attached)
        return;

    if (category == quickControlsAttachedPropertyReuse) {
        for (QQmlSA::Element parent = attached; parent; parent = parent.baseType()) {
            // ### TODO: Make it possible to resolve QQuickAttachedPropertyPropagator
            // so that we don't have to compare the internal id
            if (parent.internalId() == "QQuickAttachedPropertyPropagator"_L1) {
                usedAttachedTypes.insert(readScope, {attached, location});
                break;
            }
        }

    } else {
        usedAttachedTypes.insert(readScope, {attached, location});
    }
}

void AttachedPropertyReuse::onWrite(const QQmlSA::Element &element, const QString &propertyName,
                                    const QQmlSA::Element &value, const QQmlSA::Element &writeScope,
                                    QQmlSA::SourceLocation location)
{
    Q_UNUSED(value);
    onRead(element, propertyName, writeScope, location);
}

void QmlLintQuickPlugin::registerPasses(QQmlSA::PassManager *manager,
                                        const QQmlSA::Element &rootElement)
{
    const QQmlSA::LoggerWarningId attachedReuseCategory = [manager]() {
        if (manager->isCategoryEnabled(quickAttachedPropertyReuse))
            return quickAttachedPropertyReuse;
        if (manager->isCategoryEnabled(qmlAttachedPropertyReuse))
            return qmlAttachedPropertyReuse;
        return quickControlsAttachedPropertyReuse;
    }();

    const bool hasQuick = manager->hasImportedModule("QtQuick");
    const bool hasQuickLayouts = manager->hasImportedModule("QtQuick.Layouts");
    const bool hasQuickControls = manager->hasImportedModule("QtQuick.Templates")
            || manager->hasImportedModule("QtQuick.Controls")
            || manager->hasImportedModule("QtQuick.Controls.Basic");

    Q_UNUSED(rootElement);

    if (hasQuick) {
        manager->registerElementPass(std::make_unique<AnchorsValidatorPass>(manager));
        manager->registerElementPass(std::make_unique<PropertyChangesValidatorPass>(manager));
        manager->registerElementPass(std::make_unique<StateNoItemChildrenValidator>(manager));
        manager->registerPropertyPass(std::make_unique<QQuickLiteralBindingCheck>(manager),
                                      QAnyStringView(), QAnyStringView());
        manager->registerPropertyPass(std::make_unique<ColorValidatorPass>(manager),
                                      QAnyStringView(), QAnyStringView());

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

    auto attachedPropertyType = std::make_shared<AttachedPropertyTypeValidatorPass>(manager);

    auto addAttachedWarning = [&](TypeDescription attachedType, QList<TypeDescription> allowedTypes,
                                  QAnyStringView warning, bool allowInDelegate = false) {
        QString attachedTypeName = attachedPropertyType->addWarning(attachedType, allowedTypes,
                                                                    allowInDelegate, warning);
        if (attachedTypeName.isEmpty())
            return;

        manager->registerPropertyPass(attachedPropertyType, attachedType.module,
                                      u"$internal$."_s + attachedTypeName, {}, false);
    };

    auto addVarBindingWarning =
            [&](QAnyStringView moduleName, QAnyStringView typeName,
                const QMultiHash<QString, TypeDescription> &expectedPropertyTypes) {
                auto varBindingType = std::make_shared<VarBindingTypeValidatorPass>(
                        manager, expectedPropertyTypes);
                for (const auto &propertyName : expectedPropertyTypes.uniqueKeys()) {
                    manager->registerPropertyPass(varBindingType, moduleName, typeName,
                                                  propertyName);
                }
            };

    if (hasQuick) {
        addVarBindingWarning("QtQuick", "TableView",
                             { { "columnWidthProvider", { "", "function" } },
                               { "rowHeightProvider", { "", "function" } } });
        addAttachedWarning({ "QtQuick", "Accessible" },
                           { { "QtQuick", "Item" }, { "QtQuick.Templates", "Action" } },
                           "Accessible attached property must be attached to an object deriving "
                           "from Item or Action");
        addAttachedWarning({ "QtQuick", "LayoutMirroring" },
                           { { "QtQuick", "Item" }, { "QtQuick", "Window" } },
                           "LayoutMirroring attached property must be attached to an object deriving from Item or Window");
        addAttachedWarning({ "QtQuick", "EnterKey" }, { { "QtQuick", "Item" } },
                           "EnterKey attached property must be attached to an object deriving from Item");
    }
    if (hasQuickLayouts) {
        addAttachedWarning({ "QtQuick.Layouts", "Layout" }, { { "QtQuick", "Item" } },
                           "Layout attached property must be attached to an object deriving from Item");
        addAttachedWarning({ "QtQuick.Layouts", "StackLayout" }, { { "QtQuick", "Item" } },
                           "StackLayout attached property must be attached to an object deriving from Item");
    }


    if (hasQuickControls) {
        manager->registerElementPass(std::make_unique<ControlsSwipeDelegateValidatorPass>(manager));
        manager->registerPropertyPass(std::make_unique<AttachedPropertyReuse>(
                                          manager, attachedReuseCategory), "", "");

        addAttachedWarning({ "QtQuick.Templates", "ScrollBar" },
                           { { "QtQuick", "Flickable" }, { "QtQuick.Templates", "ScrollView" } },
                           "ScrollBar attached property must be attached to an object deriving from Flickable or ScrollView");
        addAttachedWarning({ "QtQuick.Templates", "ScrollIndicator" },
                           { { "QtQuick", "Flickable" } },
                           "ScrollIndicator attached property must be attached to an object deriving from Flickable");
        addAttachedWarning({ "QtQuick.Templates", "TextArea" }, { { "QtQuick", "Flickable" } },
                           "TextArea attached property must be attached to an object deriving from Flickable");
        addAttachedWarning({ "QtQuick.Templates", "SplitView" }, { { "QtQuick", "Item" } },
                           "SplitView attached property must be attached to an object deriving from Item");
        addAttachedWarning({ "QtQuick.Templates", "StackView" }, { { "QtQuick", "Item" } },
                           "StackView attached property must be attached to an object deriving from Item");
        addAttachedWarning({ "QtQuick.Templates", "ToolTip" }, { { "QtQuick", "Item" } },
                           "ToolTip attached property must be attached to an object deriving from Item");
        addAttachedWarning({ "QtQuick.Templates", "SwipeDelegate" }, { { "QtQuick", "Item" } },
                           "SwipeDelegate attached property must be attached to an object deriving from Item");
        addAttachedWarning({ "QtQuick.Templates", "SwipeView" }, { { "QtQuick", "Item" } },
                           "SwipeView attached property must be attached to an object deriving from Item");
        addVarBindingWarning("QtQuick.Templates", "Tumbler",
                             { { "contentItem", { "QtQuick", "PathView" } },
                               { "contentItem", { "QtQuick", "ListView" } } });
        addVarBindingWarning("QtQuick.Templates", "SpinBox",
                             { { "textFromValue", { "", "function" } },
                               { "valueFromText", { "", "function" } } });
    } else if (attachedReuseCategory != quickControlsAttachedPropertyReuse) {
        manager->registerPropertyPass(std::make_unique<AttachedPropertyReuse>(
                                          manager, attachedReuseCategory), "", "");
    }

    if (manager->hasImportedModule(u"QtQuick.Controls.macOS"_s)
        || manager->hasImportedModule(u"QtQuick.Controls.Windows"_s))
        manager->registerElementPass(std::make_unique<ControlsNativeValidatorPass>(manager));
}

PropertyChangesValidatorPass::PropertyChangesValidatorPass(QQmlSA::PassManager *manager)
    : QQmlSA::ElementPass(manager)
    , m_propertyChanges(resolveType("QtQuick", "PropertyChanges"))
{
}

bool PropertyChangesValidatorPass::shouldRun(const QQmlSA::Element &element)
{
    return !m_propertyChanges.isNull() && element.inherits(m_propertyChanges);
}

void PropertyChangesValidatorPass::run(const QQmlSA::Element &element)
{
    const QQmlSA::Binding::Bindings bindings = element.ownPropertyBindings();

    const auto target =
            std::find_if(bindings.constBegin(), bindings.constEnd(),
                         [](const auto binding) { return binding.propertyName() == u"target"_s; });
    if (target == bindings.constEnd())
        return;

    QString targetId = u"<id>"_s;
    const auto targetLocation = target.value().sourceLocation();
    const QString targetBinding = sourceCode(targetLocation);
    const QQmlSA::Element targetElement = resolveIdToElement(targetBinding, element);
    if (!targetElement.isNull())
        targetId = targetBinding;

    bool hadCustomParsedBindings = false;
    for (auto it = bindings.constBegin(); it != bindings.constEnd(); ++it) {
        const auto &propertyName = it.key();
        const auto &propertyBinding = it.value();
        if (element.hasProperty(propertyName))
            continue;

        const QQmlSA::SourceLocation bindingLocation = propertyBinding.sourceLocation();
        if (!targetElement.isNull() && !targetElement.hasProperty(propertyName)) {
            emitWarning(
                    "Unknown property \"%1\" in PropertyChanges."_L1.arg(propertyName),
                    quickPropertyChangesParsed, bindingLocation);
            continue;
        }

        QString binding = sourceCode(bindingLocation);
        if (binding.length() > 16)
            binding = binding.left(13) + "..."_L1;

        hadCustomParsedBindings = true;
        emitWarning("Property \"%1\" is custom-parsed in PropertyChanges. "
                    "You should phrase this binding as \"%2.%1: %3\""_L1.arg(propertyName, targetId,
                                                                             binding),
                    quickPropertyChangesParsed, bindingLocation);
    }

    if (hadCustomParsedBindings && !targetElement.isNull()) {
        emitWarning("You should remove any bindings on the \"target\" property and avoid "
                    "custom-parsed bindings in PropertyChanges.",
                    quickPropertyChangesParsed, targetLocation);
    }
}

StateNoItemChildrenValidator::StateNoItemChildrenValidator(QQmlSA::PassManager *manager)
    : QQmlSA::ElementPass(manager)
    , m_state(resolveType("QtQuick", "State"))
    , m_anchorChanges(resolveType("QtQuick", "AnchorChanges"))
    , m_parentChanges(resolveType("QtQuick", "ParentChange"))
    , m_propertyChanges(resolveType("QtQuick", "PropertyChanges"))
    , m_stateChangeScript(resolveType("QtQuick", "StateChangeScript"))
{}

bool StateNoItemChildrenValidator::shouldRun(const QQmlSA::Element &element)
{
    return element.inherits(m_state);
}

void StateNoItemChildrenValidator::run(const QQmlSA::Element &element)
{
    const auto &childScopes = QQmlJSScope::scope(element)->childScopes();
    for (const auto &child : childScopes) {
        if (child->scopeType() != QQmlSA::ScopeType::QMLScope)
            continue;

        if (child->inherits(QQmlJSScope::scope(m_anchorChanges))
            || child->inherits(QQmlJSScope::scope(m_parentChanges))
            || child->inherits(QQmlJSScope::scope(m_propertyChanges))
            || child->inherits(QQmlJSScope::scope(m_stateChangeScript))) {
            continue;
        }
        QString msg = "A State cannot have a child item of type %1"_L1.arg(child->baseTypeName());
        auto loc = QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                child->sourceLocation());
        emitWarning(msg, quickStateNoChildItem, loc);
    }
}

QT_END_NAMESPACE

#include "moc_quicklintplugin.cpp"
