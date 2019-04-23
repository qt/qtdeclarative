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

#ifndef QV4LOWERING_P_H
#define QV4LOWERING_P_H

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

#include <private/qqmljsmemorypool_p.h>
#include <private/qv4global_p.h>
#include <private/qv4ir_p.h>
#include <private/qv4util_p.h>
#include <private/qv4node_p.h>
#include <private/qv4graph_p.h>

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {

// Lowering replaces JS level operations with lower level ones. E.g. a JSAdd is lowered to an AddI32
// if both inputs and the output are 32bit integers, or to a runtime call in all other cases. This
// transforms the graph into something that is closer to actual executable code.


// Last lowering phase: replace all JSOperations that are left with runtime calls. There is nothing
// smart here, all that should have been done before this phase.
class GenericLowering final
{
    Q_DISABLE_COPY(GenericLowering)

public:
    GenericLowering(Function &f);

    void lower();

private:
    void replaceWithCall(Node *n);
    void replaceWithVarArgsCall(Node *n);
    static bool allUsesAsUnboxedBool(Node *n);

    Function &function()
    { return m_function; }

    Graph *graph()
    { return function().graph(); }

    OperationBuilder *opBuilder()
    { return graph()->opBuilder(); }

private:
    Function &m_function;
};

} // namespace IR
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4LOWERING_P_H
