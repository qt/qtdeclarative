// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMASTCREATOR_P_H
#define QQMLDOMASTCREATOR_P_H

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
#include "qqmldomitem_p.h"
#include "qqmldomastcreator_p.h"
#include "qqmldomcomments_p.h"

#include <QtQml/private/qqmljsastvisitor_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

SourceLocation combineLocations(SourceLocation s1, SourceLocation s2);
SourceLocation combineLocations(AST::Node *n);

void createDom(MutableDomItem qmlFile);

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QQMLDOMASTCREATOR_P_H
