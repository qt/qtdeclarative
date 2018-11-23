/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qloggingcategory.h>

#include "qv4vme_moth_p.h"
#include "qv4graphbuilder_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTracing, "qt.v4.tracing")

namespace QV4 {

// This is the entry point for the "tracing JIT". It uses the sea-of-nodes concept as described in
// https://scholarship.rice.edu/bitstream/handle/1911/96451/TR95-252.pdf
//
// The minimal pipeline is as follows:
//  - create the graph for the function
//  - do generic lowering
//  - schedule the nodes
//  - run minimal stack slot allocation (no re-use of slots)
//  - run the assembler
//
// This pipeline has no optimizations, and generates quite inefficient code. It does have the
// advantage that no trace information is used, so it can be used for testing where it replaces
// the baseline JIT. Any optimizations are additions to this pipeline.
//
// Note: generators (or resuming functions in general) are not supported by this JIT.
void Moth::runTracingJit(QV4::Function *function)
{
    IR::Function irFunction(function);
    qCDebug(lcTracing).noquote() << "runTracingJit called for" << irFunction.name() << "...";

    qCDebug(lcTracing).noquote().nospace() << function->traceInfoToString();

    IR::GraphBuilder::buildGraph(&irFunction);
    irFunction.dump(QStringLiteral("initial IR"));
    irFunction.verify();
}

} // QV4 namespace
QT_END_NAMESPACE
