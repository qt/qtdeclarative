// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMPATHPARSER_P_H
#define QQMLDOMPATHPARSER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQmlDom/private/qqmldompath_p.h>
#include <QtQmlDom/private/qqmldomerrormessage_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

/*! \internal
    Parse a textual path expression like
    \c{$env.qmlFilesByPath["/path/to.qml"].imports} into a Path. This is
    the inverse of Path::toString().
*/
Path parsePath(QStringView s, const ErrorHandler &errorHandler = nullptr);

} // namespace Dom
} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLDOMPATHPARSER_P_H
