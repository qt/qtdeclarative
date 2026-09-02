// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLSACONSTANTS_H
#define QQMLSACONSTANTS_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtdeprecationmarkers.h>

QT_BEGIN_NAMESPACE

namespace QQmlSA {

enum class BindingType : unsigned int {
    Invalid,
    BoolLiteral,
    NumberLiteral,
    StringLiteral,
    RegExpLiteral,
    Null,
    Translation,
    TranslationById,
    Script,
    Object,
    Interceptor,
    ValueSource,
    AttachedProperty,
    GroupedProperty,
#if QT_DEPRECATED_SINCE(6, 14)
    GroupProperty Q_DECL_ENUMERATOR_DEPRECATED_X("Use GroupedProperty instead.") = GroupedProperty,
#endif
};

enum class ScriptBindingKind : unsigned int {
    Invalid,
    PropertyBinding, // property int p: 1 + 1
    SignalHandler, // onSignal: { ... }
    ChangeHandler, // onXChanged: { ... }
};

enum class ScopeType {
    JSFunctionScope,
    JSLexicalScope,
    QMLScope,
    GroupedPropertyScope,
    AttachedPropertyScope,
    EnumScope,
    BindingFunctionScope,
    SignalHandlerFunctionScope,
};

} // namespace QQmlSA

QT_END_NAMESPACE

#endif // QQMLSACONSTANTS_H
