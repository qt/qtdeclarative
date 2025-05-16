// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qqmljsastvisitor_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS { namespace AST {

Visitor::Visitor(quint16 parentRecursionDepth) : BaseVisitor(parentRecursionDepth)
{
}

BaseVisitor::BaseVisitor(quint16 parentRecursionDepth) : m_recursionDepth(parentRecursionDepth) {}

BaseVisitor::~BaseVisitor()
{
}

} } // namespace QQmlJS::AST

QT_END_NAMESPACE
