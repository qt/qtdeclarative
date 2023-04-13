// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOM_UTILS_P_H
#define QQMLDOM_UTILS_P_H

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

#include <QtCore/qglobal.h>
#include "qqmldomitem_p.h"

QT_BEGIN_NAMESPACE

template<class... Ts>
struct qOverloadedVisitor : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
qOverloadedVisitor(Ts...) -> qOverloadedVisitor<Ts...>;

namespace QQmlJS {
namespace Dom {

void createDom(MutableDomItem qmlFile, DomCreationOptions options = None);

}
}; // namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLDOM_UTILS_P_H
