// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQUICKLITERALBINDINGCHECK_H
#define QQUICKLITERALBINDINGCHECK_H

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
#include <QtCore/qstring.h>
#include <private/qqmljsliteralbindingcheck_p.h>

QT_BEGIN_NAMESPACE

class QQmlJSImportVisitor;
class QQmlJSTypeResolver;

class QQuickLiteralBindingCheck: public LiteralBindingCheckBase
{
public:
    using LiteralBindingCheckBase::LiteralBindingCheckBase;

    virtual QQmlJSStructuredTypeError check(const QString &typeName,
                                            const QString &value) const override;
};

QT_END_NAMESPACE

#endif // QQUICKLITERALBINDINGCHECK_H
