// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLSACONSTANTS_H
#define QQMLSACONSTANTS_H

#include <QtCore/qfileinfo.h>

QT_BEGIN_NAMESPACE

class QQmlJSScope;

namespace QQmlSA {

enum BindingType : unsigned int {
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
    GroupProperty,
};

enum ScriptBindingKind : unsigned int {
    Script_Invalid,
    Script_PropertyBinding, // property int p: 1 + 1
    Script_SignalHandler, // onSignal: { ... }
    Script_ChangeHandler, // onXChanged: { ... }
};

enum class BindingTargetSpecifier {
    SimplePropertyTarget, // e.g. `property int p: 42`
    ListPropertyTarget, // e.g. `property list<Item> pList: [ Text {} ]`
    UnnamedPropertyTarget // default property bindings, where property name is unspecified
};

enum class ScopeType {
    JSFunctionScope,
    JSLexicalScope,
    QMLScope,
    GroupedPropertyScope,
    AttachedPropertyScope,
    EnumScope
};

enum class QQmlJSMetaMethodAccess { Private, Protected, Public };
} // namespace QQmlSA

QT_END_NAMESPACE

#endif // QQMLSACONSTANTS_H
