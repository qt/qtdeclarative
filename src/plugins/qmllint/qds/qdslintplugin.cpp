// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant reason:default

#include "qdslintplugin.h"

#include <QtCore/qlist.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qhash.h>
#include <QtCore/qset.h>
#include <QtCore/qspan.h>

#include <QtQmlCompiler/private/qqmljsscope_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QQmlSA;

// note: is a warning, but is prefixed Err to share the name with its QtC codemodel counterpart.
constexpr LoggerWarningId ErrFunctionsNotSupportedInQmlUi{
    "QtDesignStudio.FunctionsNotSupportedInQmlUi"
};
constexpr LoggerWarningId WarnReferenceToParentItemNotSupportedByVisualDesigner{
    "QtDesignStudio.ReferenceToParentItemNotSupportedByVisualDesigner"
};
constexpr LoggerWarningId WarnImperativeCodeNotEditableInVisualDesigner{
    "QtDesignStudio.ImperativeCodeNotEditableInVisualDesigner"
};
constexpr LoggerWarningId ErrUnsupportedTypeInQmlUi{ "QtDesignStudio.UnsupportedTypeInQmlUi" };
constexpr LoggerWarningId ErrInvalidIdeInVisualDesigner{
    "QtDesignStudio.InvalidIdeInVisualDesigner"
};
constexpr LoggerWarningId ErrUnsupportedRootTypeInQmlUi{
    "QtDesignStudio.UnsupportedRootTypeInQmlUi"
};
constexpr LoggerWarningId qmlTranslationFunctionMismatch{
    "QtDesignStudio.TranslationFunctionMismatch"
};

class FunctionCallValidator : public PropertyPass
{
public:
    FunctionCallValidator(PassManager *manager)
    : PropertyPass(manager)
    , m_connectionsType(resolveType("QtQuick", "Connections")) {}

    void onCall(const Element &element, const QString &propertyName, const Element &readScope,
                SourceLocation location) override;
private:
    Element m_connectionsType;
};

class QdsBindingValidator : public PropertyPass
{
public:
    QdsBindingValidator(PassManager *manager, const Element &) : PropertyPass(manager) { }

    void onRead(const QQmlSA::Element &element, const QString &propertyName,
                const QQmlSA::Element &readScope, QQmlSA::SourceLocation location) override;

    void onWrite(const QQmlSA::Element &element, const QString &propertyName,
                 const QQmlSA::Element &value, const QQmlSA::Element &writeScope,
                 QQmlSA::SourceLocation location) override;
};

class QdsElementValidator : public ElementPass
{
public:
    QdsElementValidator(PassManager *passManager);
    void run(const Element &element) override;

private:
    void complainAboutFunctions(const Element &element);
    static constexpr std::array s_unsupportedElementNames = {
        std::make_pair("QtQuick.Controls"_L1, "ApplicationWindow"_L1),
        std::make_pair("QtQuick.Controls"_L1, "Drawer"_L1),
        std::make_pair("QtQml.Models"_L1, "Package"_L1),
        std::make_pair("QtQuick"_L1, "ShaderEffect"_L1),
    };
    using UnsupportedName = decltype(s_unsupportedElementNames)::value_type;
    std::array<Element, s_unsupportedElementNames.size()> m_unsupportedElements;

    static constexpr std::array s_unsupportedRootNames = {
        std::make_pair("QtQml.Models"_L1, "ListModel"_L1),
        std::make_pair("QtQml.Models"_L1, "Package"_L1),
        std::make_pair("QtQml"_L1, "Timer"_L1),
    };
    std::array<Element, s_unsupportedRootNames.size()> m_unsupportedRootElements;
    std::array<Element, 2> m_supportFunctions;
    Element m_qtObject;
};

class QQmlJSTranslationFunctionMismatchCheck : public QQmlSA::PropertyPass
{
public:
    using QQmlSA::PropertyPass::PropertyPass;

    void onCall(const QQmlSA::Element &element, const QString &propertyName,
                const QQmlSA::Element &readScope, QQmlSA::SourceLocation location) override;

private:
    enum TranslationType : quint8 { None, Normal, IdBased };
    TranslationType m_lastTranslationFunction = None;
};

void QdsBindingValidator::onRead(const QQmlSA::Element &element, const QString &propertyName,
                                 const QQmlSA::Element &readScope, QQmlSA::SourceLocation location)
{
    Q_UNUSED(readScope);

    if (element.isFileRootComponent() && propertyName == u"parent") {
        emitWarning("Referencing the parent of the root item is not supported in a UI file (.ui.qml)",
                    WarnReferenceToParentItemNotSupportedByVisualDesigner, location);
    }
}

void QdsBindingValidator::onWrite(const QQmlSA::Element &, const QString &propertyName,
                                  const QQmlSA::Element &, const QQmlSA::Element &,
                                  QQmlSA::SourceLocation location)
{
    static constexpr std::array forbiddenAssignments = { "baseline"_L1,
                                                         "baselineOffset"_L1,
                                                         "bottomMargin"_L1,
                                                         "centerIn"_L1,
                                                         "color"_L1,
                                                         "fill"_L1,
                                                         "height"_L1,
                                                         "horizontalCenter"_L1,
                                                         "horizontalCenterOffset"_L1,
                                                         "left"_L1,
                                                         "leftMargin"_L1,
                                                         "margins"_L1,
                                                         "mirrored"_L1,
                                                         "opacity"_L1,
                                                         "right"_L1,
                                                         "rightMargin"_L1,
                                                         "rotation"_L1,
                                                         "scale"_L1,
                                                         "topMargin"_L1,
                                                         "verticalCenter"_L1,
                                                         "verticalCenterOffset"_L1,
                                                         "width"_L1,
                                                         "x"_L1,
                                                         "y"_L1,
                                                         "z"_L1 };
    Q_ASSERT(std::is_sorted(forbiddenAssignments.cbegin(), forbiddenAssignments.cend()));
    if (std::find(forbiddenAssignments.cbegin(), forbiddenAssignments.cend(), propertyName)
        != forbiddenAssignments.cend()) {
        emitWarning("Imperative JavaScript assignments can break the visual tooling in Qt Design "
                    "Studio.",
                    WarnImperativeCodeNotEditableInVisualDesigner, location);
    }
}

void QQmlJSTranslationFunctionMismatchCheck::onCall(const QQmlSA::Element &element,
                                                    const QString &propertyName,
                                                    const QQmlSA::Element &readScope,
                                                    QQmlSA::SourceLocation location)
{
    Q_UNUSED(readScope);

    const QQmlSA::Element globalJSObject = resolveBuiltinType(u"GlobalObject");
    if (element != globalJSObject)
        return;

    constexpr std::array translationFunctions = {
        "qsTranslate"_L1,
        "QT_TRANSLATE_NOOP"_L1,
        "qsTr"_L1,
        "QT_TR_NOOP"_L1,
    };

    constexpr std::array idTranslationFunctions = {
        "qsTrId"_L1,
        "QT_TRID_NOOP"_L1,
    };

    const bool isTranslation =
            std::find(translationFunctions.cbegin(), translationFunctions.cend(), propertyName)
            != translationFunctions.cend();
    const bool isIdTranslation =
            std::find(idTranslationFunctions.cbegin(), idTranslationFunctions.cend(), propertyName)
            != idTranslationFunctions.cend();

    if (!isTranslation && !isIdTranslation)
        return;

    const TranslationType current = isTranslation ? Normal : IdBased;

    if (m_lastTranslationFunction == None) {
        m_lastTranslationFunction = current;
        return;
    }

    if (m_lastTranslationFunction != current) {
        emitWarning("Do not mix translation functions", qmlTranslationFunctionMismatch, location);
    }
}

void QmlLintQdsPlugin::registerPasses(PassManager *manager, const Element &rootElement)
{
    if (!rootElement.filePath().endsWith(u".ui.qml"))
        return;

    manager->registerPropertyPass(std::make_shared<FunctionCallValidator>(manager),
                                  QAnyStringView(), QAnyStringView());
    manager->registerPropertyPass(std::make_shared<QdsBindingValidator>(manager, rootElement),
                                  QAnyStringView(), QAnyStringView());
    manager->registerPropertyPass(std::make_unique<QQmlJSTranslationFunctionMismatchCheck>(manager),
                                  QString(), QString(), QString());
    manager->registerElementPass(std::make_unique<QdsElementValidator>(manager));
}

void FunctionCallValidator::onCall(const Element &element, const QString &propertyName,
                                   const Element &readScope, SourceLocation location)
{
    auto currentQmlScope = QQmlJSScope::findCurrentQMLScope(QQmlJSScope::scope(readScope));
    // TODO: we would benefit from some public additional public QQmlSA API here.
    //       This should be considered in the context of QTBUG-138360
    if (currentQmlScope && currentQmlScope->inherits(QQmlJSScope::scope(m_connectionsType)))
        return;

    // all math functions are allowed
    const Element globalJSObject = resolveBuiltinType(u"GlobalObject");
    const Element mathObjectType = globalJSObject.property(u"Math"_s).type();
    if (element.inherits(mathObjectType))
        return;

    const Element qjsValue = resolveBuiltinType(u"QJSValue");
    if (element.inherits(qjsValue)) {
        // Workaround because the Date method has methods and those are only represented in
        // QQmlJSTypePropagator as QJSValue.
        // This is an overapproximation and might flag unrelated methods with the same name as ok
        // even if they are not, but this is better than bogus warnings about the valid Date methods.
        const std::array<QStringView, 4> dateMethodmethods{ u"now", u"parse", u"prototype",
                                                            u"UTC" };
        if (auto it = std::find(dateMethodmethods.cbegin(), dateMethodmethods.cend(), propertyName);
            it != dateMethodmethods.cend())
            return;
    }

    static const std::vector<std::pair<Element, std::unordered_set<QString>>>
            whiteListedFunctions = {
                { Element(),
                  {
                          // used on JS objects and many other types
                          u"valueOf"_s,
                          u"toString"_s,
                          u"toLocaleString"_s,
                  } },
                { globalJSObject,
                  {
                          u"isNaN"_s, u"isFinite"_s,
                          u"qsTr"_s, u"qsTrId"_s, u"qsTranslate"_s,
                          u"QT_TRANSLATE_NOOP"_s, u"QT_TRID_NOOP"_s, u"QT_TR_NOOP"_s,
                  }
                },
                { resolveBuiltinType(u"ArrayPrototype"_s), { u"indexOf"_s, u"lastIndexOf"_s } },
                { resolveBuiltinType(u"NumberPrototype"_s),
                  {
                          u"isNaN"_s,
                          u"isFinite"_s,
                          u"toFixed"_s,
                          u"toExponential"_s,
                          u"toPrecision"_s,
                          u"isInteger"_s,
                  } },
                { resolveBuiltinType(u"StringPrototype"_s),
                  {
                          u"arg"_s,
                          u"toLowerCase"_s,
                          u"toLocaleLowerCase"_s,
                          u"toUpperCase"_s,
                          u"toLocaleUpperCase"_s,
                          u"substring"_s,
                          u"charAt"_s,
                          u"charCodeAt"_s,
                          u"concat"_s,
                          u"includes"_s,
                          u"endsWith"_s,
                          u"indexOf"_s,
                          u"lastIndexOf"_s,
                  } },
                { resolveType(u"QtQml"_s, u"Qt"_s),
                  { u"lighter"_s, u"darker"_s, u"rgba"_s, u"tint"_s, u"hsla"_s, u"hsva"_s,
                    u"point"_s, u"rect"_s, u"size"_s, u"vector2d"_s, u"vector3d"_s, u"vector4d"_s,
                    u"quaternion"_s, u"matrix4x4"_s, u"formatDate"_s, u"formatDateTime"_s,
                    u"formatTime"_s, u"resolvedUrl"_s } },
            };

    for (const auto &[currentElement, methods] : whiteListedFunctions) {
        if ((!currentElement || element.inherits(currentElement)) && methods.count(propertyName)) {
            return;
        }
    }

    // all other functions are forbidden
    emitWarning(u"Arbitrary functions and function calls outside of a Connections object are not "
                u"supported in a UI file (.ui.qml)",
                ErrFunctionsNotSupportedInQmlUi, location);
}

QdsElementValidator::QdsElementValidator(PassManager *manager) : ElementPass(manager)
{
    auto loadTypes = [&manager, this](QSpan<const UnsupportedName> names, QSpan<Element> output) {
        for (qsizetype i = 0; i < qsizetype(names.size()); ++i) {
            if (!manager->hasImportedModule(names[i].first))
                continue;
            output[i] = resolveType(names[i].first, names[i].second);
        }
    };
    loadTypes(s_unsupportedElementNames, m_unsupportedElements);
    loadTypes(s_unsupportedRootNames, m_unsupportedRootElements);
    m_qtObject = resolveType("QtQml"_L1, "QtObject"_L1);
    m_supportFunctions = { resolveType("QtQml"_L1, "Connections"_L1),
                           resolveType("QtQuick"_L1, "ScriptAction"_L1) };
}

void QdsElementValidator::complainAboutFunctions(const Element &element)
{
    for (const auto &method : element.ownMethods()) {
        if (method.methodType() != QQmlSA::MethodType::Method)
            continue;

        emitWarning(
                u"Arbitrary functions and function calls outside of a Connections object are not "
                u"supported in a UI file (.ui.qml)",
                ErrFunctionsNotSupportedInQmlUi, method.sourceLocation());
    }

    for (const auto &binding : element.ownPropertyBindings()) {
        if (!binding.hasFunctionScriptValue())
            continue;
        emitWarning(u"Arbitrary functions and function calls outside of a Connections object "
                    u"are not "
                    u"supported in a UI file (.ui.qml)",
                    ErrFunctionsNotSupportedInQmlUi, binding.sourceLocation());
    }
}

void QdsElementValidator::run(const Element &element)
{
    enum WarningType { ForElements, ForRootElements };
    auto warnIfElementIsUnsupported = [this, &element](WarningType warningType) {
        QSpan<const Element> unsupportedComponents = warningType == ForElements
                ? QSpan<const Element>(m_unsupportedElements)
                : QSpan<const Element>(m_unsupportedRootElements);
        const QStringView message = warningType == ForElements
                ? u"This type (%1) is not supported in a UI file (.ui.qml)."_sv
                : u"This type (%1) is not supported as a root element of a UI file (.ui.qml)."_sv;
        const LoggerWarningId &id = warningType == ForElements ? ErrUnsupportedTypeInQmlUi
                                                               : ErrUnsupportedRootTypeInQmlUi;

        for (const auto &unsupportedElement : unsupportedComponents) {
            if (!element.inherits(unsupportedElement))
                continue;

            emitWarning(message.arg(element.baseTypeName()), id, element.sourceLocation());
            break;
        }

        // special case: we don't want to warn on types indirectly inheriting from QtObject, for
        // example Item.
        if (warningType == ForRootElements && element.baseType() == m_qtObject)
            emitWarning(message.arg(element.baseTypeName()), id, element.sourceLocation());
    };

    if (element.isFileRootComponent())
        warnIfElementIsUnsupported(ForRootElements);
    warnIfElementIsUnsupported(ForElements);

    if (QString id = resolveElementToId(element, element); !id.isEmpty()) {
        static constexpr std::array unsupportedNames = {
            "action"_L1,     "alias"_L1,    "anchors"_L1,   "as"_L1,          "baseState"_L1,
            "bool"_L1,       "border"_L1,   "bottom"_L1,    "break"_L1,       "case"_L1,
            "catch"_L1,      "clip"_L1,     "color"_L1,     "continue"_L1,    "data"_L1,
            "date"_L1,       "debugger"_L1, "default"_L1,   "delete"_L1,      "do"_L1,
            "double"_L1,     "else"_L1,     "enabled"_L1,   "enumeration"_L1, "finally"_L1,
            "flow"_L1,       "focus"_L1,    "font"_L1,      "for"_L1,         "function"_L1,
            "height"_L1,     "id"_L1,       "if"_L1,        "import"_L1,      "in"_L1,
            "instanceof"_L1, "int"_L1,      "item"_L1,      "layer"_L1,       "left"_L1,
            "list"_L1,       "margin"_L1,   "matrix4x4"_L1, "new"_L1,         "opacity"_L1,
            "padding"_L1,    "parent"_L1,   "point"_L1,     "print"_L1,       "quaternion"_L1,
            "real"_L1,       "rect"_L1,     "return"_L1,    "right"_L1,       "scale"_L1,
            "shaderInfo"_L1, "size"_L1,     "source"_L1,    "sprite"_L1,      "spriteSequence"_L1,
            "state"_L1,      "string"_L1,   "switch"_L1,    "text"_L1,        "texture"_L1,
            "this"_L1,       "throw"_L1,    "time"_L1,      "top"_L1,         "try"_L1,
            "typeof"_L1,     "url"_L1,      "var"_L1,       "variant"_L1,     "vector"_L1,
            "vector2d"_L1,   "vector3d"_L1, "vector4d"_L1,  "visible"_L1,     "void"_L1,
            "while"_L1,      "width"_L1,    "with"_L1,      "x"_L1,           "y"_L1,
            "z"_L1,
        };

        Q_ASSERT(std::is_sorted(unsupportedNames.begin(), unsupportedNames.end()));
        if (std::binary_search(unsupportedNames.cbegin(), unsupportedNames.cend(), id)) {
            emitWarning(
                    u"This id (%1) might be ambiguous and is not supported in a UI file (.ui.qml)."_s
                            .arg(id),
                    ErrInvalidIdeInVisualDesigner, element.idSourceLocation());
        }
    }

    if (std::none_of(m_supportFunctions.cbegin(), m_supportFunctions.cend(),
                     [&element](const Element &base) { return element.inherits(base); })) {
        complainAboutFunctions(element);
    }
}

QT_END_NAMESPACE

#include "moc_qdslintplugin.cpp"
