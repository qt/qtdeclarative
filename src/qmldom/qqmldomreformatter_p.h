// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMREFORMATTER_P
#define QQMLDOMREFORMATTER_P

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

#include "qqmldom_global.h"

#include "qqmldomoutwriter_p.h"
#include "qqmldom_fwd_p.h"
#include "qqmldomcomments_p.h"

#include <QtQml/private/qqmljsast_p.h>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

QMLDOM_EXPORT void reformatAst(OutWriter &lw, std::shared_ptr<AstComments> comments,
                               const std::function<QStringView(SourceLocation)> loc2Str,
                               AST::Node *n);

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif // QQMLDOMREFORMATTER_P
