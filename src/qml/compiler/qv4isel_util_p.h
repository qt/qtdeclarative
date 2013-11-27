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

#ifndef QV4ISEL_UTIL_P_H
#define QV4ISEL_UTIL_P_H

#include "private/qv4value_def_p.h"
#include "qv4jsir_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {

inline bool canConvertToSignedInteger(double value)
{
    int ival = (int) value;
    // +0 != -0, so we need to convert to double when negating 0
    return ival == value && !(value == 0 && isNegative(value));
}

inline bool canConvertToUnsignedInteger(double value)
{
    unsigned uval = (unsigned) value;
    // +0 != -0, so we need to convert to double when negating 0
    return uval == value && !(value == 0 && isNegative(value));
}

inline QV4::Primitive convertToValue(V4IR::Const *c)
{
    switch (c->type) {
    case V4IR::MissingType:
        return QV4::Primitive::emptyValue();
    case V4IR::NullType:
        return QV4::Primitive::nullValue();
    case V4IR::UndefinedType:
        return QV4::Primitive::undefinedValue();
    case V4IR::BoolType:
        return QV4::Primitive::fromBoolean(c->value != 0);
    case V4IR::SInt32Type:
        return QV4::Primitive::fromInt32(int(c->value));
    case V4IR::UInt32Type:
        return QV4::Primitive::fromUInt32(unsigned(c->value));
    case V4IR::DoubleType:
        return QV4::Primitive::fromDouble(c->value);
    case V4IR::NumberType: {
        int ival = (int)c->value;
        if (canConvertToSignedInteger(c->value)) {
            return QV4::Primitive::fromInt32(ival);
        } else {
            return QV4::Primitive::fromDouble(c->value);
        }
    }
    default:
        Q_UNREACHABLE();
    }
    // unreachable, but the function must return something
    return QV4::Primitive::undefinedValue();
}

class ConvertTemps: protected V4IR::StmtVisitor, protected V4IR::ExprVisitor
{
    int _nextFreeStackSlot;
    QHash<V4IR::Temp, int> _stackSlotForTemp;

    void renumber(V4IR::Temp *t)
    {
        if (t->kind != V4IR::Temp::VirtualRegister)
            return;

        int stackSlot = _stackSlotForTemp.value(*t, -1);
        if (stackSlot == -1) {
            stackSlot = _nextFreeStackSlot++;
            _stackSlotForTemp[*t] = stackSlot;
        }

        t->kind = V4IR::Temp::StackSlot;
        t->index = stackSlot;
    }

public:
    ConvertTemps()
        : _nextFreeStackSlot(0)
    {}

    void toStackSlots(V4IR::Function *function)
    {
        _stackSlotForTemp.reserve(function->tempCount);

        foreach (V4IR::BasicBlock *bb, function->basicBlocks)
            foreach (V4IR::Stmt *s, bb->statements)
                s->accept(this);

        function->tempCount = _nextFreeStackSlot;
    }

protected:
    virtual void visitConst(V4IR::Const *) {}
    virtual void visitString(V4IR::String *) {}
    virtual void visitRegExp(V4IR::RegExp *) {}
    virtual void visitName(V4IR::Name *) {}
    virtual void visitTemp(V4IR::Temp *e) { renumber(e); }
    virtual void visitClosure(V4IR::Closure *) {}
    virtual void visitConvert(V4IR::Convert *e) { e->expr->accept(this); }
    virtual void visitUnop(V4IR::Unop *e) { e->expr->accept(this); }
    virtual void visitBinop(V4IR::Binop *e) { e->left->accept(this); e->right->accept(this); }
    virtual void visitCall(V4IR::Call *e) {
        e->base->accept(this);
        for (V4IR::ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }
    virtual void visitNew(V4IR::New *e) {
        e->base->accept(this);
        for (V4IR::ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }
    virtual void visitSubscript(V4IR::Subscript *e) { e->base->accept(this); e->index->accept(this); }
    virtual void visitMember(V4IR::Member *e) { e->base->accept(this); }
    virtual void visitExp(V4IR::Exp *s) { s->expr->accept(this); }
    virtual void visitMove(V4IR::Move *s) { s->target->accept(this); s->source->accept(this); }
    virtual void visitJump(V4IR::Jump *) {}
    virtual void visitCJump(V4IR::CJump *s) { s->cond->accept(this); }
    virtual void visitRet(V4IR::Ret *s) { s->expr->accept(this); }
    virtual void visitPhi(V4IR::Phi *) { Q_UNREACHABLE(); }
};
} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QV4ISEL_UTIL_P_H
