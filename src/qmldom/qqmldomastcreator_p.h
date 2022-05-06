/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**/
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
