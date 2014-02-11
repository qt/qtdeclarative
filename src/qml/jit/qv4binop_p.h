/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4BINOP_P_H
#define QV4BINOP_P_H

#include <qv4jsir_p.h>
#include <qv4isel_masm_p.h>

QT_BEGIN_NAMESPACE

#if ENABLE(ASSEMBLER)

namespace QV4 {
namespace JIT {

struct Binop {
    Binop(QQmlJS::MASM::Assembler *assembler, QQmlJS::V4IR::AluOp operation)
        : as(assembler)
        , op(operation)
    {}

    void generate(QQmlJS::V4IR::Expr *lhs, QQmlJS::V4IR::Expr *rhs, QQmlJS::V4IR::Temp *target);
    void doubleBinop(QQmlJS::V4IR::Expr *lhs, QQmlJS::V4IR::Expr *rhs, QQmlJS::V4IR::Temp *target);
    bool int32Binop(QQmlJS::V4IR::Expr *leftSource, QQmlJS::V4IR::Expr *rightSource, QQmlJS::V4IR::Temp *target);
    QQmlJS::MASM::Assembler::Jump genInlineBinop(QQmlJS::V4IR::Expr *leftSource, QQmlJS::V4IR::Expr *rightSource, QQmlJS::V4IR::Temp *target);

    QQmlJS::MASM::Assembler *as;
    QQmlJS::V4IR::AluOp op;
};

}
}

#endif

QT_END_NAMESPACE

#endif
