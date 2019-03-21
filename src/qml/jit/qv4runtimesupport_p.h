/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QV4RUNTIMESUPPORT_P_H
#define QV4RUNTIMESUPPORT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qv4runtimeapi_p.h>

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {
namespace RuntimeSupport {

template <typename T>
struct CountArguments {
    static constexpr unsigned count = 0;
};
template <typename RetTy, typename... Args>
struct CountArguments<RetTy (*)(Args... args)> {
    static constexpr unsigned count = sizeof...(Args) ;
};

template<typename M>
static constexpr unsigned argumentCount() {
    using type = decltype(&M::call);
    return CountArguments<type>::count;
}

enum class ArgumentType {
    Invalid,
    Engine,
    Frame,
    Function,
    ValueRef,
    ValueArray,
    ReturnedValue,
    Int,
    Bool,
    Void,
};


template <typename T>
struct JavaScriptType
{
    // No default type. We want to make sure everything we do is actually recognized.
};

template <typename T>
struct ReturnValue
{
    // No default type.
};

template <int I, typename T>
struct Argument
{
    // For simplicity, we add a default here. Otherwise we would need to spell out more
    // combinations of I and number of arguments of T.
    static constexpr ArgumentType type = ArgumentType::Invalid;
};

template <typename RetTy, typename T, typename... Args>
struct Argument<1, RetTy (*)(T, Args... args)> {
    static constexpr ArgumentType type = JavaScriptType<T>::type;
};

template <typename RetTy, typename Arg1, typename T, typename... Args>
struct Argument<2, RetTy (*)(Arg1, T, Args... args)> {
    static constexpr ArgumentType type = JavaScriptType<T>::type;
};

template <typename RetTy, typename Arg1, typename Arg2, typename T,
          typename... Args>
struct Argument<3, RetTy (*)(Arg1, Arg2, T, Args... args)> {
    static constexpr ArgumentType type = JavaScriptType<T>::type;
};

template <typename RetTy, typename Arg1, typename Arg2,
          typename Arg3, typename T, typename... Args>
struct Argument<4, RetTy (*)(Arg1, Arg2, Arg3, T, Args... args)> {
    static constexpr ArgumentType type = JavaScriptType<T>::type;
};

template <typename RetTy, typename Arg1, typename Arg2,
          typename Arg3, typename Arg4, typename T, typename... Args>
struct Argument<5, RetTy (*)(Arg1, Arg2, Arg3, Arg4, T, Args... args)> {
    static constexpr ArgumentType type = JavaScriptType<T>::type;
};

template <typename RetTy, typename Arg1, typename Arg2,
          typename Arg3, typename Arg4, typename Arg5, typename T, typename... Args>
struct Argument<6, RetTy (*)(Arg1, Arg2, Arg3, Arg4, Arg5, T, Args... args)> {
    static constexpr ArgumentType type = JavaScriptType<T>::type;
};

template <typename RetTy, typename... Args>
struct ReturnValue<RetTy (*)(Args... args)> {
    static constexpr ArgumentType type = JavaScriptType<RetTy>::type;
};

template<>
struct JavaScriptType<QV4::ExecutionEngine *>
{
    static constexpr ArgumentType type = ArgumentType::Engine;
};

template<>
struct JavaScriptType<QV4::CppStackFrame *>
{
    static constexpr ArgumentType type = ArgumentType::Frame;
};

template<>
struct JavaScriptType<QV4::Function *>
{
    static constexpr ArgumentType type = ArgumentType::Function;
};

template<>
struct JavaScriptType<const QV4::Value &>
{
    static constexpr ArgumentType type = ArgumentType::ValueRef;
};

template<>
// We need to pass Value * in order to match a parmeter Value[].
struct JavaScriptType<QV4::Value *>
{
    static constexpr ArgumentType type = ArgumentType::ValueArray;
};

template<>
struct JavaScriptType<int>
{
    static constexpr ArgumentType type = ArgumentType::Int;
};

template<>
struct JavaScriptType<QV4::Bool>
{
    static constexpr ArgumentType type = ArgumentType::Bool;
};

template<>
struct JavaScriptType<QV4::ReturnedValue>
{
    static constexpr ArgumentType type = ArgumentType::ReturnedValue;
};

template<>
struct JavaScriptType<void>
{
    static constexpr ArgumentType type = ArgumentType::Void;
};

template<typename M>
static constexpr ArgumentType retType() {
    using Type = decltype(&M::call);
    return ReturnValue<Type>::type;
}

template<typename M>
static constexpr ArgumentType arg1Type() {
    using Type = decltype(&M::call);
    return Argument<1, Type>::type;
}

template<typename M>
static constexpr ArgumentType arg2Type() {
    using Type = decltype(&M::call);
    return Argument<2, Type>::type;
}

template<typename M>
static constexpr ArgumentType arg3Type() {
    using Type = decltype(&M::call);
    return Argument<3, Type>::type;
}

template<typename M>
static constexpr ArgumentType arg4Type() {
    using Type = decltype(&M::call);
    return Argument<4, Type>::type;
}

template<typename M>
static constexpr ArgumentType arg5Type() {
    using Type = decltype(&M::call);
    return Argument<5, Type>::type;
}

template<typename M>
static constexpr ArgumentType arg6Type() {
    using Type = decltype(&M::call);
    return Argument<6, Type>::type;
}

} // namespace RuntimeSupport
} // namespace IR
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4RUNTIMESUPPORT_P_H
