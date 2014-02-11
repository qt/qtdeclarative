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
#include <qv4binop_p.h>
#include <qv4assembler_p.h>

#if ENABLE(ASSEMBLER)

using namespace QV4;
using namespace JIT;

using namespace QQmlJS;
using namespace MASM;

namespace {
inline bool isPregOrConst(V4IR::Expr *e)
{
    if (V4IR::Temp *t = e->asTemp())
        return t->kind == V4IR::Temp::PhysicalRegister;
    return e->asConst() != 0;
}
} // anonymous namespace

void Binop::generate(V4IR::Expr *lhs, V4IR::Expr *rhs, V4IR::Temp *target)
{
    if (op != V4IR::OpMod
            && lhs->type == V4IR::DoubleType && rhs->type == V4IR::DoubleType
            && isPregOrConst(lhs) && isPregOrConst(rhs)) {
        doubleBinop(lhs, rhs, target);
        return;
    }
    if (lhs->type == V4IR::SInt32Type && rhs->type == V4IR::SInt32Type) {
        if (int32Binop(lhs, rhs, target))
            return;
    }

    Assembler::Jump done;
    if (lhs->type != V4IR::StringType && rhs->type != V4IR::StringType)
        done = genInlineBinop(lhs, rhs, target);

    // TODO: inline var===null and var!==null
    Assembler::BinaryOperationInfo info = Assembler::binaryOperation(op);

    if (op == V4IR::OpAdd &&
            (lhs->type == V4IR::StringType || rhs->type == V4IR::StringType)) {
        const Assembler::BinaryOperationInfo stringAdd = OPCONTEXT(__qmljs_add_string);
        info = stringAdd;
    }

    if (info.fallbackImplementation) {
        as->generateFunctionCallImp(target, info.name, info.fallbackImplementation,
                                     Assembler::PointerToValue(lhs),
                                     Assembler::PointerToValue(rhs));
    } else if (info.contextImplementation) {
        as->generateFunctionCallImp(target, info.name, info.contextImplementation,
                                     Assembler::ContextRegister,
                                     Assembler::PointerToValue(lhs),
                                     Assembler::PointerToValue(rhs));
    } else {
        assert(!"unreachable");
    }

    if (done.isSet())
        done.link(as);

}

void Binop::doubleBinop(V4IR::Expr *lhs, V4IR::Expr *rhs, V4IR::Temp *target)
{
    Q_ASSERT(lhs->asConst() == 0 || rhs->asConst() == 0);
    Q_ASSERT(isPregOrConst(lhs));
    Q_ASSERT(isPregOrConst(rhs));
    Assembler::FPRegisterID targetReg;
    if (target->kind == V4IR::Temp::PhysicalRegister)
        targetReg = (Assembler::FPRegisterID) target->index;
    else
        targetReg = Assembler::FPGpr0;

    switch (op) {
    case V4IR::OpAdd:
        as->addDouble(as->toDoubleRegister(lhs), as->toDoubleRegister(rhs),
                       targetReg);
        break;
    case V4IR::OpMul:
        as->mulDouble(as->toDoubleRegister(lhs), as->toDoubleRegister(rhs),
                       targetReg);
        break;
    case V4IR::OpSub:
#if CPU(X86) || CPU(X86_64)
        if (V4IR::Temp *rightTemp = rhs->asTemp()) {
            if (rightTemp->kind == V4IR::Temp::PhysicalRegister && rightTemp->index == targetReg) {
                as->moveDouble(targetReg, Assembler::FPGpr0);
                as->moveDouble(as->toDoubleRegister(lhs, targetReg), targetReg);
                as->subDouble(Assembler::FPGpr0, targetReg);
                break;
            }
        } else if (rhs->asConst() && targetReg == Assembler::FPGpr0) {
            Q_ASSERT(lhs->asTemp());
            Q_ASSERT(lhs->asTemp()->kind == V4IR::Temp::PhysicalRegister);
            as->moveDouble(as->toDoubleRegister(lhs, targetReg), targetReg);
            Assembler::FPRegisterID reg = (Assembler::FPRegisterID) lhs->asTemp()->index;
            as->moveDouble(as->toDoubleRegister(rhs, reg), reg);
            as->subDouble(reg, targetReg);
            break;
        }
#endif

        as->subDouble(as->toDoubleRegister(lhs), as->toDoubleRegister(rhs),
                       targetReg);
        break;
    case V4IR::OpDiv:
#if CPU(X86) || CPU(X86_64)
        if (V4IR::Temp *rightTemp = rhs->asTemp()) {
            if (rightTemp->kind == V4IR::Temp::PhysicalRegister && rightTemp->index == targetReg) {
                as->moveDouble(targetReg, Assembler::FPGpr0);
                as->moveDouble(as->toDoubleRegister(lhs, targetReg), targetReg);
                as->divDouble(Assembler::FPGpr0, targetReg);
                break;
            }
        } else if (rhs->asConst() && targetReg == Assembler::FPGpr0) {
            Q_ASSERT(lhs->asTemp());
            Q_ASSERT(lhs->asTemp()->kind == V4IR::Temp::PhysicalRegister);
            as->moveDouble(as->toDoubleRegister(lhs, targetReg), targetReg);
            Assembler::FPRegisterID reg = (Assembler::FPRegisterID) lhs->asTemp()->index;
            as->moveDouble(as->toDoubleRegister(rhs, reg), reg);
            as->divDouble(reg, targetReg);
            break;
        }
#endif
        as->divDouble(as->toDoubleRegister(lhs), as->toDoubleRegister(rhs),
                       targetReg);
        break;
    default: {
        Q_ASSERT(target->type == V4IR::BoolType);
        Assembler::Jump trueCase = as->branchDouble(false, op, lhs, rhs);
        as->storeBool(false, target);
        Assembler::Jump done = as->jump();
        trueCase.link(as);
        as->storeBool(true, target);
        done.link(as);
    } return;
    }

    if (target->kind != V4IR::Temp::PhysicalRegister)
        as->storeDouble(Assembler::FPGpr0, target);
}


bool Binop::int32Binop(V4IR::Expr *leftSource, V4IR::Expr *rightSource, V4IR::Temp *target)
{
    Q_ASSERT(leftSource->type == V4IR::SInt32Type);
    Assembler::RegisterID targetReg;
    if (target->kind == V4IR::Temp::PhysicalRegister)
        targetReg = (Assembler::RegisterID) target->index;
    else
        targetReg = Assembler::ReturnValueRegister;

    switch (op) {
    case V4IR::OpBitAnd: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);
        if (rightSource->asTemp() && rightSource->asTemp()->kind == V4IR::Temp::PhysicalRegister
                && target->kind == V4IR::Temp::PhysicalRegister
                && target->index == rightSource->asTemp()->index) {
            as->and32(as->toInt32Register(leftSource, Assembler::ScratchRegister),
                       (Assembler::RegisterID) target->index);
            return true;
        }

        as->and32(as->toInt32Register(leftSource, targetReg),
                   as->toInt32Register(rightSource, Assembler::ScratchRegister),
                   targetReg);
        as->storeInt32(targetReg, target);
    } return true;
    case V4IR::OpBitOr: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);
        if (rightSource->asTemp() && rightSource->asTemp()->kind == V4IR::Temp::PhysicalRegister
                && target->kind == V4IR::Temp::PhysicalRegister
                && target->index == rightSource->asTemp()->index) {
            as->or32(as->toInt32Register(leftSource, Assembler::ScratchRegister),
                       (Assembler::RegisterID) target->index);
            return true;
        }

        as->or32(as->toInt32Register(leftSource, targetReg),
                  as->toInt32Register(rightSource, Assembler::ScratchRegister),
                  targetReg);
        as->storeInt32(targetReg, target);
    } return true;
    case V4IR::OpBitXor: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);
        if (rightSource->asTemp() && rightSource->asTemp()->kind == V4IR::Temp::PhysicalRegister
                && target->kind == V4IR::Temp::PhysicalRegister
                && target->index == rightSource->asTemp()->index) {
            as->xor32(as->toInt32Register(leftSource, Assembler::ScratchRegister),
                       (Assembler::RegisterID) target->index);
            return true;
        }

        as->xor32(as->toInt32Register(leftSource, targetReg),
                   as->toInt32Register(rightSource, Assembler::ScratchRegister),
                   targetReg);
        as->storeInt32(targetReg, target);
    } return true;
    case V4IR::OpLShift: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);

        if (V4IR::Const *c = rightSource->asConst()) {
            as->lshift32(as->toInt32Register(leftSource, Assembler::ReturnValueRegister),
                          Assembler::TrustedImm32(int(c->value) & 0x1f), targetReg);
        } else {
            as->move(as->toInt32Register(rightSource, Assembler::ScratchRegister),
                      Assembler::ScratchRegister);
            if (!rightSource->asConst())
                as->and32(Assembler::TrustedImm32(0x1f), Assembler::ScratchRegister);
            as->lshift32(as->toInt32Register(leftSource, targetReg), Assembler::ScratchRegister, targetReg);
        }
        as->storeInt32(targetReg, target);
    } return true;
    case V4IR::OpRShift: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);

        if (V4IR::Const *c = rightSource->asConst()) {
            as->rshift32(as->toInt32Register(leftSource, Assembler::ReturnValueRegister),
                          Assembler::TrustedImm32(int(c->value) & 0x1f), targetReg);
        } else {
            as->move(as->toInt32Register(rightSource, Assembler::ScratchRegister),
                      Assembler::ScratchRegister);
            as->and32(Assembler::TrustedImm32(0x1f), Assembler::ScratchRegister);
            as->rshift32(as->toInt32Register(leftSource, targetReg), Assembler::ScratchRegister, targetReg);
        }
        as->storeInt32(targetReg, target);
    } return true;
    case V4IR::OpURShift:
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);

        if (V4IR::Const *c = rightSource->asConst()) {
            as->urshift32(as->toInt32Register(leftSource, Assembler::ReturnValueRegister),
                           Assembler::TrustedImm32(int(c->value) & 0x1f), targetReg);
        } else {
            as->move(as->toInt32Register(rightSource, Assembler::ScratchRegister),
                      Assembler::ScratchRegister);
            as->and32(Assembler::TrustedImm32(0x1f), Assembler::ScratchRegister);
            as->urshift32(as->toInt32Register(leftSource, targetReg), Assembler::ScratchRegister, targetReg);
        }
        as->storeUInt32(targetReg, target);
        return true;
    case V4IR::OpAdd: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);

        Assembler::RegisterID targetReg;
        if (target->kind == V4IR::Temp::PhysicalRegister)
            targetReg = (Assembler::RegisterID) target->index;
        else
            targetReg = Assembler::ReturnValueRegister;

        as->add32(as->toInt32Register(leftSource, targetReg),
                   as->toInt32Register(rightSource, Assembler::ScratchRegister),
                   targetReg);
        as->storeInt32(targetReg, target);
    } return true;
    case V4IR::OpSub: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);

        if (rightSource->asTemp() && rightSource->asTemp()->kind == V4IR::Temp::PhysicalRegister
                && target->kind == V4IR::Temp::PhysicalRegister
                && target->index == rightSource->asTemp()->index) {
            Assembler::RegisterID targetReg = (Assembler::RegisterID) target->index;
            as->move(targetReg, Assembler::ScratchRegister);
            as->move(as->toInt32Register(leftSource, targetReg), targetReg);
            as->sub32(Assembler::ScratchRegister, targetReg);
            as->storeInt32(targetReg, target);
            return true;
        }

        Assembler::RegisterID targetReg;
        if (target->kind == V4IR::Temp::PhysicalRegister)
            targetReg = (Assembler::RegisterID) target->index;
        else
            targetReg = Assembler::ReturnValueRegister;

        as->move(as->toInt32Register(leftSource, targetReg), targetReg);
        as->sub32(as->toInt32Register(rightSource, Assembler::ScratchRegister), targetReg);
        as->storeInt32(targetReg, target);
    } return true;
    case V4IR::OpMul: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);

        Assembler::RegisterID targetReg;
        if (target->kind == V4IR::Temp::PhysicalRegister)
            targetReg = (Assembler::RegisterID) target->index;
        else
            targetReg = Assembler::ReturnValueRegister;

        as->mul32(as->toInt32Register(leftSource, targetReg),
                   as->toInt32Register(rightSource, Assembler::ScratchRegister),
                   targetReg);
        as->storeInt32(targetReg, target);
    } return true;
    default:
        return false;
    }
}

static inline Assembler::FPRegisterID getFreeFPReg(V4IR::Expr *shouldNotOverlap, unsigned hint)
{
    if (V4IR::Temp *t = shouldNotOverlap->asTemp())
        if (t->type == V4IR::DoubleType)
            if (t->kind == V4IR::Temp::PhysicalRegister)
                if (t->index == hint)
                    return Assembler::FPRegisterID(hint + 1);
    return Assembler::FPRegisterID(hint);
}

Assembler::Jump Binop::genInlineBinop(V4IR::Expr *leftSource, V4IR::Expr *rightSource, V4IR::Temp *target)
{
    Assembler::Jump done;

    // Try preventing a call for a few common binary operations. This is used in two cases:
    // - no register allocation was performed (not available for the platform, or the IR was
    //   not transformed into SSA)
    // - type inference found that either or both operands can be of non-number type, and the
    //   register allocator will have prepared for a call (meaning: all registers that do not
    //   hold operands are spilled to the stack, which makes them available here)
    // Note: FPGPr0 can still not be used, because uint32->double conversion uses it as a scratch
    //       register.
    switch (op) {
    case V4IR::OpAdd: {
        Assembler::FPRegisterID lReg = getFreeFPReg(rightSource, 2);
        Assembler::FPRegisterID rReg = getFreeFPReg(leftSource, 4);
        Assembler::Jump leftIsNoDbl = as->genTryDoubleConversion(leftSource, lReg);
        Assembler::Jump rightIsNoDbl = as->genTryDoubleConversion(rightSource, rReg);

        as->addDouble(rReg, lReg);
        as->storeDouble(lReg, target);
        done = as->jump();

        if (leftIsNoDbl.isSet())
            leftIsNoDbl.link(as);
        if (rightIsNoDbl.isSet())
            rightIsNoDbl.link(as);
    } break;
    case V4IR::OpMul: {
        Assembler::FPRegisterID lReg = getFreeFPReg(rightSource, 2);
        Assembler::FPRegisterID rReg = getFreeFPReg(leftSource, 4);
        Assembler::Jump leftIsNoDbl = as->genTryDoubleConversion(leftSource, lReg);
        Assembler::Jump rightIsNoDbl = as->genTryDoubleConversion(rightSource, rReg);

        as->mulDouble(rReg, lReg);
        as->storeDouble(lReg, target);
        done = as->jump();

        if (leftIsNoDbl.isSet())
            leftIsNoDbl.link(as);
        if (rightIsNoDbl.isSet())
            rightIsNoDbl.link(as);
    } break;
    case V4IR::OpSub: {
        Assembler::FPRegisterID lReg = getFreeFPReg(rightSource, 2);
        Assembler::FPRegisterID rReg = getFreeFPReg(leftSource, 4);
        Assembler::Jump leftIsNoDbl = as->genTryDoubleConversion(leftSource, lReg);
        Assembler::Jump rightIsNoDbl = as->genTryDoubleConversion(rightSource, rReg);

        as->subDouble(rReg, lReg);
        as->storeDouble(lReg, target);
        done = as->jump();

        if (leftIsNoDbl.isSet())
            leftIsNoDbl.link(as);
        if (rightIsNoDbl.isSet())
            rightIsNoDbl.link(as);
    } break;
    case V4IR::OpDiv: {
        Assembler::FPRegisterID lReg = getFreeFPReg(rightSource, 2);
        Assembler::FPRegisterID rReg = getFreeFPReg(leftSource, 4);
        Assembler::Jump leftIsNoDbl = as->genTryDoubleConversion(leftSource, lReg);
        Assembler::Jump rightIsNoDbl = as->genTryDoubleConversion(rightSource, rReg);

        as->divDouble(rReg, lReg);
        as->storeDouble(lReg, target);
        done = as->jump();

        if (leftIsNoDbl.isSet())
            leftIsNoDbl.link(as);
        if (rightIsNoDbl.isSet())
            rightIsNoDbl.link(as);
    } break;
    default:
        break;
    }

    return done;
}

#endif
