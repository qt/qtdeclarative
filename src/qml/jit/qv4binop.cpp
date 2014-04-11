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

namespace {
inline bool isPregOrConst(IR::Expr *e)
{
    if (IR::Temp *t = e->asTemp())
        return t->kind == IR::Temp::PhysicalRegister;
    return e->asConst() != 0;
}
} // anonymous namespace


#define OP(op) \
    { isel_stringIfy(op), op, 0, 0, 0 }
#define OPCONTEXT(op) \
    { isel_stringIfy(op), 0, op, 0, 0 }

#define INLINE_OP(op, memOp, immOp) \
    { isel_stringIfy(op), op, 0, memOp, immOp }
#define INLINE_OPCONTEXT(op, memOp, immOp) \
    { isel_stringIfy(op), 0, op, memOp, immOp }

#define NULL_OP \
    { 0, 0, 0, 0, 0 }

const Binop::OpInfo Binop::operations[IR::LastAluOp + 1] = {
    NULL_OP, // OpInvalid
    NULL_OP, // OpIfTrue
    NULL_OP, // OpNot
    NULL_OP, // OpUMinus
    NULL_OP, // OpUPlus
    NULL_OP, // OpCompl
    NULL_OP, // OpIncrement
    NULL_OP, // OpDecrement

    INLINE_OP(Runtime::bitAnd, &Binop::inline_and32, &Binop::inline_and32), // OpBitAnd
    INLINE_OP(Runtime::bitOr, &Binop::inline_or32, &Binop::inline_or32), // OpBitOr
    INLINE_OP(Runtime::bitXor, &Binop::inline_xor32, &Binop::inline_xor32), // OpBitXor

    INLINE_OPCONTEXT(Runtime::add, &Binop::inline_add32, &Binop::inline_add32), // OpAdd
    INLINE_OP(Runtime::sub, &Binop::inline_sub32, &Binop::inline_sub32), // OpSub
    INLINE_OP(Runtime::mul, &Binop::inline_mul32, &Binop::inline_mul32), // OpMul

    OP(Runtime::div), // OpDiv
    OP(Runtime::mod), // OpMod

    INLINE_OP(Runtime::shl, &Binop::inline_shl32, &Binop::inline_shl32), // OpLShift
    INLINE_OP(Runtime::shr, &Binop::inline_shr32, &Binop::inline_shr32), // OpRShift
    INLINE_OP(Runtime::ushr, &Binop::inline_ushr32, &Binop::inline_ushr32), // OpURShift

    OP(Runtime::greaterThan), // OpGt
    OP(Runtime::lessThan), // OpLt
    OP(Runtime::greaterEqual), // OpGe
    OP(Runtime::lessEqual), // OpLe
    OP(Runtime::equal), // OpEqual
    OP(Runtime::notEqual), // OpNotEqual
    OP(Runtime::strictEqual), // OpStrictEqual
    OP(Runtime::strictNotEqual), // OpStrictNotEqual

    OPCONTEXT(Runtime::instanceof), // OpInstanceof
    OPCONTEXT(Runtime::in), // OpIn

    NULL_OP, // OpAnd
    NULL_OP // OpOr
};



void Binop::generate(IR::Expr *lhs, IR::Expr *rhs, IR::Temp *target)
{
    if (op != IR::OpMod
            && lhs->type == IR::DoubleType && rhs->type == IR::DoubleType
            && isPregOrConst(lhs) && isPregOrConst(rhs)) {
        doubleBinop(lhs, rhs, target);
        return;
    }
    if (lhs->type == IR::SInt32Type && rhs->type == IR::SInt32Type) {
        if (int32Binop(lhs, rhs, target))
            return;
    }

    Assembler::Jump done;
    if (lhs->type != IR::StringType && rhs->type != IR::StringType)
        done = genInlineBinop(lhs, rhs, target);

    // TODO: inline var===null and var!==null
    Binop::OpInfo info = Binop::operation(op);

    if (op == IR::OpAdd &&
            (lhs->type == IR::StringType || rhs->type == IR::StringType)) {
        const Binop::OpInfo stringAdd = OPCONTEXT(Runtime::addString);
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
        Q_ASSERT(!"unreachable");
    }

    if (done.isSet())
        done.link(as);

}

void Binop::doubleBinop(IR::Expr *lhs, IR::Expr *rhs, IR::Temp *target)
{
    Q_ASSERT(lhs->asConst() == 0 || rhs->asConst() == 0);
    Q_ASSERT(isPregOrConst(lhs));
    Q_ASSERT(isPregOrConst(rhs));
    Assembler::FPRegisterID targetReg;
    if (target->kind == IR::Temp::PhysicalRegister)
        targetReg = (Assembler::FPRegisterID) target->index;
    else
        targetReg = Assembler::FPGpr0;

    switch (op) {
    case IR::OpAdd:
        as->addDouble(as->toDoubleRegister(lhs), as->toDoubleRegister(rhs),
                       targetReg);
        break;
    case IR::OpMul:
        as->mulDouble(as->toDoubleRegister(lhs), as->toDoubleRegister(rhs),
                       targetReg);
        break;
    case IR::OpSub:
#if CPU(X86) || CPU(X86_64)
        if (IR::Temp *rightTemp = rhs->asTemp()) {
            if (rightTemp->kind == IR::Temp::PhysicalRegister && rightTemp->index == targetReg) {
                as->moveDouble(targetReg, Assembler::FPGpr0);
                as->moveDouble(as->toDoubleRegister(lhs, targetReg), targetReg);
                as->subDouble(Assembler::FPGpr0, targetReg);
                break;
            }
        } else if (rhs->asConst() && targetReg == Assembler::FPGpr0) {
            Q_ASSERT(lhs->asTemp());
            Q_ASSERT(lhs->asTemp()->kind == IR::Temp::PhysicalRegister);
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
    case IR::OpDiv:
#if CPU(X86) || CPU(X86_64)
        if (IR::Temp *rightTemp = rhs->asTemp()) {
            if (rightTemp->kind == IR::Temp::PhysicalRegister && rightTemp->index == targetReg) {
                as->moveDouble(targetReg, Assembler::FPGpr0);
                as->moveDouble(as->toDoubleRegister(lhs, targetReg), targetReg);
                as->divDouble(Assembler::FPGpr0, targetReg);
                break;
            }
        } else if (rhs->asConst() && targetReg == Assembler::FPGpr0) {
            Q_ASSERT(lhs->asTemp());
            Q_ASSERT(lhs->asTemp()->kind == IR::Temp::PhysicalRegister);
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
        Q_ASSERT(target->type == IR::BoolType);
        Assembler::Jump trueCase = as->branchDouble(false, op, lhs, rhs);
        as->storeBool(false, target);
        Assembler::Jump done = as->jump();
        trueCase.link(as);
        as->storeBool(true, target);
        done.link(as);
    } return;
    }

    if (target->kind != IR::Temp::PhysicalRegister)
        as->storeDouble(Assembler::FPGpr0, target);
}


bool Binop::int32Binop(IR::Expr *leftSource, IR::Expr *rightSource, IR::Temp *target)
{
    Q_ASSERT(leftSource->type == IR::SInt32Type);
    Assembler::RegisterID targetReg = Assembler::ReturnValueRegister;
    if (target->kind == IR::Temp::PhysicalRegister) {
        // We try to load leftSource into the target's register, but we can't do that if
        // the target register is the same as rightSource.
        IR::Temp *rhs = rightSource->asTemp();
        if (!rhs || rhs->kind != IR::Temp::PhysicalRegister || rhs->index != target->index)
            targetReg = (Assembler::RegisterID) target->index;
    }

    switch (op) {
    case IR::OpBitAnd: {
        Q_ASSERT(rightSource->type == IR::SInt32Type);
        if (rightSource->asTemp() && rightSource->asTemp()->kind == IR::Temp::PhysicalRegister
                && target->kind == IR::Temp::PhysicalRegister
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
    case IR::OpBitOr: {
        Q_ASSERT(rightSource->type == IR::SInt32Type);
        if (rightSource->asTemp() && rightSource->asTemp()->kind == IR::Temp::PhysicalRegister
                && target->kind == IR::Temp::PhysicalRegister
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
    case IR::OpBitXor: {
        Q_ASSERT(rightSource->type == IR::SInt32Type);
        if (rightSource->asTemp() && rightSource->asTemp()->kind == IR::Temp::PhysicalRegister
                && target->kind == IR::Temp::PhysicalRegister
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
    case IR::OpLShift: {
        Q_ASSERT(rightSource->type == IR::SInt32Type);

        if (IR::Const *c = rightSource->asConst()) {
            if (int(c->value) == 0)
                as->move(as->toInt32Register(leftSource, Assembler::ReturnValueRegister), targetReg);
            else
                as->lshift32(as->toInt32Register(leftSource, Assembler::ReturnValueRegister),
                             Assembler::TrustedImm32(int(c->value) & 0x1f), targetReg);
        } else {
            as->move(as->toInt32Register(rightSource, Assembler::ScratchRegister),
                      Assembler::ScratchRegister);
#if CPU(ARM) || CPU(X86) || CPU(X86_64)
            // The ARM assembler will generate this for us, and Intel will do it on the CPU.
#else
            as->and32(Assembler::TrustedImm32(0x1f), Assembler::ScratchRegister);
#endif
            as->lshift32(as->toInt32Register(leftSource, targetReg), Assembler::ScratchRegister, targetReg);
        }
        as->storeInt32(targetReg, target);
    } return true;
    case IR::OpRShift: {
        Q_ASSERT(rightSource->type == IR::SInt32Type);

        if (IR::Const *c = rightSource->asConst()) {
            if (int(c->value) == 0)
                as->move(as->toInt32Register(leftSource, Assembler::ReturnValueRegister), targetReg);
            else
                as->rshift32(as->toInt32Register(leftSource, Assembler::ReturnValueRegister),
                             Assembler::TrustedImm32(int(c->value) & 0x1f), targetReg);
        } else {
            as->move(as->toInt32Register(rightSource, Assembler::ScratchRegister),
                      Assembler::ScratchRegister);
#if CPU(ARM) || CPU(X86) || CPU(X86_64)
            // The ARM assembler will generate this for us, and Intel will do it on the CPU.
#else
            as->and32(Assembler::TrustedImm32(0x1f), Assembler::ScratchRegister);
#endif
            as->rshift32(as->toInt32Register(leftSource, targetReg), Assembler::ScratchRegister, targetReg);
        }
        as->storeInt32(targetReg, target);
    } return true;
    case IR::OpURShift:
        Q_ASSERT(rightSource->type == IR::SInt32Type);

        if (IR::Const *c = rightSource->asConst()) {
            if (int(c->value) == 0)
                as->move(as->toInt32Register(leftSource, Assembler::ReturnValueRegister), targetReg);
            else
                as->urshift32(as->toInt32Register(leftSource, Assembler::ReturnValueRegister),
                              Assembler::TrustedImm32(int(c->value) & 0x1f), targetReg);
        } else {
            as->move(as->toInt32Register(rightSource, Assembler::ScratchRegister),
                      Assembler::ScratchRegister);
#if CPU(ARM) || CPU(X86) || CPU(X86_64)
            // The ARM assembler will generate this for us, and Intel will do it on the CPU.
#else
            as->and32(Assembler::TrustedImm32(0x1f), Assembler::ScratchRegister);
#endif
            as->urshift32(as->toInt32Register(leftSource, targetReg), Assembler::ScratchRegister, targetReg);
        }
        as->storeUInt32(targetReg, target);
        return true;
    case IR::OpAdd: {
        Q_ASSERT(rightSource->type == IR::SInt32Type);

        as->add32(as->toInt32Register(leftSource, targetReg),
                   as->toInt32Register(rightSource, Assembler::ScratchRegister),
                   targetReg);
        as->storeInt32(targetReg, target);
    } return true;
    case IR::OpSub: {
        Q_ASSERT(rightSource->type == IR::SInt32Type);

        if (rightSource->asTemp() && rightSource->asTemp()->kind == IR::Temp::PhysicalRegister
                && target->kind == IR::Temp::PhysicalRegister
                && target->index == rightSource->asTemp()->index) {
            Assembler::RegisterID targetReg = (Assembler::RegisterID) target->index;
            as->move(targetReg, Assembler::ScratchRegister);
            as->move(as->toInt32Register(leftSource, targetReg), targetReg);
            as->sub32(Assembler::ScratchRegister, targetReg);
            as->storeInt32(targetReg, target);
            return true;
        }

        as->move(as->toInt32Register(leftSource, targetReg), targetReg);
        as->sub32(as->toInt32Register(rightSource, Assembler::ScratchRegister), targetReg);
        as->storeInt32(targetReg, target);
    } return true;
    case IR::OpMul: {
        Q_ASSERT(rightSource->type == IR::SInt32Type);

        as->mul32(as->toInt32Register(leftSource, targetReg),
                   as->toInt32Register(rightSource, Assembler::ScratchRegister),
                   targetReg);
        as->storeInt32(targetReg, target);
    } return true;
    default:
        return false;
    }
}

static inline Assembler::FPRegisterID getFreeFPReg(IR::Expr *shouldNotOverlap, unsigned hint)
{
    if (IR::Temp *t = shouldNotOverlap->asTemp())
        if (t->type == IR::DoubleType)
            if (t->kind == IR::Temp::PhysicalRegister)
                if (t->index == hint)
                    return Assembler::FPRegisterID(hint + 1);
    return Assembler::FPRegisterID(hint);
}

Assembler::Jump Binop::genInlineBinop(IR::Expr *leftSource, IR::Expr *rightSource, IR::Temp *target)
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
    case IR::OpAdd: {
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
    case IR::OpMul: {
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
    case IR::OpSub: {
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
    case IR::OpDiv: {
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
