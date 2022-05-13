// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMFUNCTIONREF_P_H
#define QQMLDOMFUNCTIONREF_P_H

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

#include <QtCore/private/qglobal_p.h>

#include <functional>
// function_ref has been proposed for the C++20 standard, see
//    http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0792r2.html
// uses it if available, replace it with a const ref to std::function otherwise

#ifndef __cpp_lib_function_ref

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {
template <typename T>
using function_ref = const std::function<T> &;
} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#else

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {
using std::function_ref;
} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif // __cpp_lib_function_ref

#endif // QQMLDOMFUNCTIONREF_P_H
