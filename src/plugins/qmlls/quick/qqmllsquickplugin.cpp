// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant reason:default

#include "qqmllsquickplugin_p.h"
#include <QtQmlLS/private/qqmllsutils_p.h>
#include <QtQmlLS/private/qqmllscompletion_p.h>

using namespace QLspSpecification;
using namespace QQmlJS::Dom;

QT_BEGIN_NAMESPACE

std::unique_ptr<QQmlLSCompletionPlugin> QQmlLSQuickPlugin::createCompletionPlugin() const
{
    return std::make_unique<QQmlLSQuickCompletionPlugin>();
}

void QQmlLSQuickCompletionPlugin::suggestSnippetsForLeftHandSideOfBinding(
        const DomItem &itemAtPosition, BackInsertIterator result) const
{
    auto file = itemAtPosition.containingFile().as<QmlFile>();
    if (!file)
        return;

    // check if QtQuick has been imported
    const auto &imports = file->imports();
    auto it = std::find_if(imports.constBegin(), imports.constEnd(), [](const Import &import) {
        return import.uri.moduleUri() == u"QtQuick";
    });
    if (it == imports.constEnd()) {
        return;
    }

    // for default bindings:
    suggestSnippetsForRightHandSideOfBinding(itemAtPosition, result);

    // check if the user already typed some qualifier, remove its dot and compare it to QtQuick's
    // qualified name
    const QString userTypedQualifier = QQmlLSUtils::qualifiersFrom(itemAtPosition);
    if (!userTypedQualifier.isEmpty()
        && !it->importId.startsWith(QStringView(userTypedQualifier).chopped(1))) {
        return;
    }

    const QByteArray prefixForSnippet =
            userTypedQualifier.isEmpty() ? it->importId.toUtf8() : QByteArray();
    const QByteArray prefixWithDotForSnippet =
            it->importId.isEmpty() ? QByteArray() : it->importId.toUtf8().append(u'.');

    auto resolver = file->typeResolver();
    if (!resolver)
        return;
    const auto qquickItemScope = resolver->typeForName(prefixWithDotForSnippet + u"Item"_s);
    const QQmlJSScope::ConstPtr ownerScope = itemAtPosition.qmlObject().semanticScope();
    if (!ownerScope || !qquickItemScope)
        return;

    if (ownerScope->inherits(qquickItemScope)) {
        result = QQmlLSCompletion::makeSnippet(
                "states binding with PropertyChanges in State",
                "states: [\n"
                "\t"_ba.append(prefixWithDotForSnippet)
                        .append("State {\n"
                                "\t\tname: \"${1:name}\"\n"
                                "\t\t"_ba.append(prefixWithDotForSnippet)
                                        .append("PropertyChanges {\n"
                                                "\t\t\ttarget: ${2:object}\n"
                                                "\t\t}\n"
                                                "\t}\n"
                                                "]")));
        result = QQmlLSCompletion::makeSnippet("transitions binding with Transition",
                                               "transitions: [\n"
                                               "\t"_ba.append(prefixWithDotForSnippet)
                                                       .append("Transition {\n"
                                                               "\t\tfrom: \"${1:fromState}\"\n"
                                                               "\t\tto: \"${2:fromState}\"\n"
                                                               "\t}\n"
                                                               "]"));
    }
}

void QQmlLSQuickCompletionPlugin::suggestSnippetsForRightHandSideOfBinding(
        const DomItem &itemAtPosition, BackInsertIterator result) const
{
    auto file = itemAtPosition.containingFile().as<QmlFile>();
    if (!file)
        return;

    // check if QtQuick has been imported
    const auto &imports = file->imports();
    auto it = std::find_if(imports.constBegin(), imports.constEnd(), [](const Import &import) {
        return import.uri.moduleUri() == u"QtQuick";
    });
    if (it == imports.constEnd()) {
        return;
    }

    // check if the user already typed some qualifier, remove its dot and compare it to QtQuick's
    // qualified name
    const QString userTypedQualifier = QQmlLSUtils::qualifiersFrom(itemAtPosition);
    if (!userTypedQualifier.isEmpty()
        && !it->importId.startsWith(QStringView(userTypedQualifier).chopped(1))) {
        return;
    }

    const QByteArray prefixForSnippet =
            userTypedQualifier.isEmpty() ? it->importId.toUtf8() : QByteArray();
    const QByteArray prefixWithDotForSnippet =
            it->importId.isEmpty() ? QByteArray() : it->importId.toUtf8().append(u'.');

    // Quick completions from Qt Creator's code model
    result = QQmlLSCompletion::makeSnippet(prefixForSnippet, "BorderImage snippet",
                                           "BorderImage {\n"
                                           "\tid: ${1:name}\n"
                                           "\tsource: \"${2:file}\"\n"
                                           "\twidth: ${3:100}; height: ${4:100}\n"
                                           "\tborder.left: ${5: 5}; border.top: ${5}\n"
                                           "\tborder.right: ${5}; border.bottom: ${5}\n"
                                           "}");
    result = QQmlLSCompletion::makeSnippet(prefixForSnippet, "ColorAnimation snippet",
                                           "ColorAnimation {\n"
                                           "\tfrom: \"${1:white}\"\n"
                                           "\tto: \"${2:black}\"\n"
                                           "\tduration: ${3:200}\n"
                                           "}");
    result = QQmlLSCompletion::makeSnippet(prefixForSnippet, "Image snippet",
                                           "Image {\n"
                                           "\tid: ${1:name}\n"
                                           "\tsource: \"${2:file}\"\n"
                                           "}");
    result = QQmlLSCompletion::makeSnippet(prefixForSnippet, "Item snippet",
                                           "Item {\n"
                                           "\tid: ${1:name}\n"
                                           "}");
    result = QQmlLSCompletion::makeSnippet(prefixForSnippet, "NumberAnimation snippet",
                                           "NumberAnimation {\n"
                                           "\ttarget: ${1:object}\n"
                                           "\tproperty: \"${2:name}\"\n"
                                           "\tduration: ${3:200}\n"
                                           "\teasing.type: "_ba.append(prefixWithDotForSnippet)
                                                   .append("Easing.${4:InOutQuad}\n"
                                                           "}"));
    result = QQmlLSCompletion::makeSnippet(prefixForSnippet, "NumberAnimation with targets snippet",
                                           "NumberAnimation {\n"
                                           "\ttargets: [${1:object}]\n"
                                           "\tproperties: \"${2:name}\"\n"
                                           "\tduration: ${3:200}\n"
                                           "}");
    result = QQmlLSCompletion::makeSnippet(prefixForSnippet, "PauseAnimation snippet",
                                           "PauseAnimation {\n"
                                           "\tduration: ${1:200}\n"
                                           "}");
    result = QQmlLSCompletion::makeSnippet(prefixForSnippet, "PropertyAction snippet",
                                           "PropertyAction {\n"
                                           "\ttarget: ${1:object}\n"
                                           "\tproperty: \"${2:name}\"\n"
                                           "}");
    result = QQmlLSCompletion::makeSnippet(prefixForSnippet, "PropertyAction with targets snippet",
                                           "PropertyAction {\n"
                                           "\ttargets: [${1:object}]\n"
                                           "\tproperties: \"${2:name}\"\n"
                                           "}");
    result = QQmlLSCompletion::makeSnippet(prefixForSnippet, "PropertyChanges snippet",
                                           "PropertyChanges {\n"
                                           "\ttarget: ${1:object}\n"
                                           "}");
    result = QQmlLSCompletion::makeSnippet(prefixForSnippet, "State snippet",
                                           "State {\n"
                                           "\tname: ${1:name}\n"
                                           "\t"_ba.append(prefixWithDotForSnippet)
                                                   .append("PropertyChanges {\n"
                                                           "\t\ttarget: ${2:object}\n"
                                                           "\t}\n"
                                                           "}"));
    result = QQmlLSCompletion::makeSnippet(prefixForSnippet, "Text snippet",
                                           "Text {\n"
                                           "\tid: ${1:name}\n"
                                           "\ttext: qsTr(\"${2:text}\")\n"
                                           "}");
    result = QQmlLSCompletion::makeSnippet(prefixForSnippet, "Transition snippet",
                                           "Transition {\n"
                                           "\tfrom: \"${1:fromState}\"\n"
                                           "\tto: \"${2:toState}\"\n"
                                           "}");
}

QT_END_NAMESPACE
