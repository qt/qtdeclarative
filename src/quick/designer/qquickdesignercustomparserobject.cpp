// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qquickdesignercustomparserobject_p.h"

QT_BEGIN_NAMESPACE

QQuickDesignerCustomParserObject::QQuickDesignerCustomParserObject() = default;

void QQuickDesignerCustomParser::verifyBindings(
        const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &,
        const QList<const QV4::CompiledData::Binding *> &)
{
    /* Nothing to do we accept anything */
}

void QQuickDesignerCustomParser::applyBindings(
        QObject *, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &,
        const QList<const QV4::CompiledData::Binding *> &)
{
    /* Nothing to do we accept anything */
}

QT_END_NAMESPACE

#include "moc_qquickdesignercustomparserobject_p.cpp"
