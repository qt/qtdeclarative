/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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
**
****************************************************************************/

#include "qquickdesignercustomparserobject_p.h"

QT_BEGIN_NAMESPACE

QQuickDesignerCustomParserObject::QQuickDesignerCustomParserObject()
{

}

void QQuickDesignerCustomParser::verifyBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &, const QList<const QV4::CompiledData::Binding *> &)
{
    /* Nothing to do we accept anything */
}

void QQuickDesignerCustomParser::applyBindings(QObject *, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &, const QList<const QV4::CompiledData::Binding *> &)
{
    /* Nothing to do we accept anything */
}

QT_END_NAMESPACE

#include "moc_qquickdesignercustomparserobject_p.cpp"
