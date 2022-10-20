// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "../qqmldom_global.h"
#ifndef QTQMLCOMPILEREXPORTS_P_H
#define QTQMLCOMPILEREXPORTS_P_H

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

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)

#include <QtCore/qstring.h>
namespace Qt {
inline namespace Literals {
inline namespace StringLiterals {

constexpr inline QLatin1String operator"" _L1(const char *str, size_t size) noexcept
{
    return QLatin1String { str, qsizetype(size) };
}

inline QString operator"" _s(const char16_t *str, size_t size) noexcept
{
    return QString(QStringPrivate(nullptr, const_cast<char16_t *>(str), qsizetype(size)));
}

} // StringLiterals
} // Literals
} // Qt

#ifdef Q_QMLCOMPILER_PRIVATE_EXPORT // clashing declaration fixed in ce53e48504f
#undef Q_QMLCOMPILER_PRIVATE_EXPORT
#endif

#endif // 6.4.0

#ifndef Q_UNREACHABLE_RETURN // new in QT_VERSION_CHECK(6, 5, 0)
#  define Q_UNREACHABLE_RETURN(...) do { Q_UNREACHABLE(); return __VA_ARGS__; } while (0)
#endif

#define Q_QMLCOMPILER_PRIVATE_EXPORT QMLDOM_EXPORT
#endif
