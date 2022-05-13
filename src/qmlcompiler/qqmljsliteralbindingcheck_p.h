// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSLITERALBINDINGCHECK_P_H
#define QQMLJSLITERALBINDINGCHECK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/qglobal.h>
#include <private/qtqmlcompilerexports_p.h>

QT_BEGIN_NAMESPACE

class QQmlJSImportVisitor;
class QQmlJSTypeResolver;

class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSLiteralBindingCheck
{
public:
    void run(QQmlJSImportVisitor *visitor, QQmlJSTypeResolver *resolver);
};

QT_END_NAMESPACE

#endif // QQMLJSLITERALBINDINGCHECK_P_H
