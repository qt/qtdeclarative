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
#ifndef QV4ISEL_MASM_P_H
#define QV4ISEL_MASM_P_H

#include "private/qv4global_p.h"
#include "private/qv4jsir_p.h"
#include "private/qv4isel_p.h"
#include "private/qv4isel_util_p.h"
#include "private/qv4value_p.h"
#include "private/qv4lookup_p.h"

#include <QtCore/QHash>
#include <QtCore/QStack>
#include <config.h>
#include <wtf/Vector.h>

#include "qv4assembler_p.h"

#if ENABLE(ASSEMBLER)

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace JIT {

class Q_QML_EXPORT InstructionSelection:
        protected IR::IRDecoder,
        public EvalInstructionSelection
{
public:
    InstructionSelection(QQmlEnginePrivate *qmlEngine, QV4::ExecutableAllocator *execAllocator, IR::Module *module, QV4::Compiler::JSUnitGenerator *jsGenerator);
    ~InstructionSelection();

    virtual void run(int functionIndex);

    const void *addConstantTable(QVector<QV4::Primitive> *values);
protected:
    virtual QV4::CompiledData::CompilationUnit *backendCompileStep();

    virtual void callBuiltinInvalid(IR::Name *func, IR::ExprList *args, IR::Temp *result);
    virtual void callBuiltinTypeofMember(IR::Expr *base, const QString &name, IR::Temp *result);
    virtual void callBuiltinTypeofSubscript(IR::Expr *base, IR::Expr *index, IR::Temp *result);
    virtual void callBuiltinTypeofName(const QString &name, IR::Temp *result);
    virtual void callBuiltinTypeofValue(IR::Expr *value, IR::Temp *result);
    virtual void callBuiltinDeleteMember(IR::Temp *base, const QString &name, IR::Temp *result);
    virtual void callBuiltinDeleteSubscript(IR::Temp *base, IR::Expr *index, IR::Temp *result);
    virtual void callBuiltinDeleteName(const QString &name, IR::Temp *result);
    virtual void callBuiltinDeleteValue(IR::Temp *result);
    virtual void callBuiltinThrow(IR::Expr *arg);
    virtual void callBuiltinReThrow();
    virtual void callBuiltinUnwindException(IR::Temp *);
    virtual void callBuiltinPushCatchScope(const QString &exceptionName);
    virtual void callBuiltinForeachIteratorObject(IR::Expr *arg, IR::Temp *result);
    virtual void callBuiltinForeachNextPropertyname(IR::Temp *arg, IR::Temp *result);
    virtual void callBuiltinPushWithScope(IR::Temp *arg);
    virtual void callBuiltinPopScope();
    virtual void callBuiltinDeclareVar(bool deletable, const QString &name);
    virtual void callBuiltinDefineArray(IR::Temp *result, IR::ExprList *args);
    virtual void callBuiltinDefineObjectLiteral(IR::Temp *result, int keyValuePairCount, IR::ExprList *keyValuePairs, IR::ExprList *arrayEntries, bool needSparseArray);
    virtual void callBuiltinSetupArgumentObject(IR::Temp *result);
    virtual void callBuiltinConvertThisToObject();
    virtual void callValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result);
    virtual void callProperty(IR::Expr *base, const QString &name, IR::ExprList *args, IR::Temp *result);
    virtual void callSubscript(IR::Expr *base, IR::Expr *index, IR::ExprList *args, IR::Temp *result);
    virtual void convertType(IR::Temp *source, IR::Temp *target);
    virtual void loadThisObject(IR::Temp *temp);
    virtual void loadQmlIdArray(IR::Temp *temp);
    virtual void loadQmlImportedScripts(IR::Temp *temp);
    virtual void loadQmlContextObject(IR::Temp *temp);
    virtual void loadQmlScopeObject(IR::Temp *temp);
    virtual void loadQmlSingleton(const QString &name, IR::Temp *temp);
    virtual void loadConst(IR::Const *sourceConst, IR::Temp *targetTemp);
    virtual void loadString(const QString &str, IR::Temp *targetTemp);
    virtual void loadRegexp(IR::RegExp *sourceRegexp, IR::Temp *targetTemp);
    virtual void getActivationProperty(const IR::Name *name, IR::Temp *temp);
    virtual void setActivationProperty(IR::Expr *source, const QString &targetName);
    virtual void initClosure(IR::Closure *closure, IR::Temp *target);
    virtual void getProperty(IR::Expr *base, const QString &name, IR::Temp *target);
    virtual void setProperty(IR::Expr *source, IR::Expr *targetBase, const QString &targetName);
    virtual void setQObjectProperty(IR::Expr *source, IR::Expr *targetBase, int propertyIndex);
    virtual void getQObjectProperty(IR::Expr *base, int propertyIndex, bool captureRequired, int attachedPropertiesId, IR::Temp *target);
    virtual void getElement(IR::Expr *base, IR::Expr *index, IR::Temp *target);
    virtual void setElement(IR::Expr *source, IR::Expr *targetBase, IR::Expr *targetIndex);
    virtual void copyValue(IR::Temp *sourceTemp, IR::Temp *targetTemp);
    virtual void swapValues(IR::Temp *sourceTemp, IR::Temp *targetTemp);
    virtual void unop(IR::AluOp oper, IR::Temp *sourceTemp, IR::Temp *targetTemp);
    virtual void binop(IR::AluOp oper, IR::Expr *leftSource, IR::Expr *rightSource, IR::Temp *target);

    typedef Assembler::Address Address;
    typedef Assembler::Pointer Pointer;

#if !defined(ARGUMENTS_IN_REGISTERS)
    Address addressForArgument(int index) const
    {
        // StackFrameRegister points to its old value on the stack, and above
        // it we have the return address, hence the need to step over two
        // values before reaching the first argument.
        return Address(Assembler::StackFrameRegister, (index + 2) * sizeof(void*));
    }
#endif

    Pointer baseAddressForCallArguments()
    {
        return _as->stackLayout().argumentAddressForCall(0);
    }

    Pointer baseAddressForCallData()
    {
        return _as->stackLayout().callDataAddress();
    }

    virtual void constructActivationProperty(IR::Name *func, IR::ExprList *args, IR::Temp *result);
    virtual void constructProperty(IR::Temp *base, const QString &name, IR::ExprList *args, IR::Temp *result);
    virtual void constructValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result);

    virtual void visitJump(IR::Jump *);
    virtual void visitCJump(IR::CJump *);
    virtual void visitRet(IR::Ret *);

    bool visitCJumpDouble(IR::AluOp op, IR::Expr *left, IR::Expr *right,
                          IR::BasicBlock *iftrue, IR::BasicBlock *iffalse);
    void visitCJumpStrict(IR::Binop *binop, IR::BasicBlock *trueBlock, IR::BasicBlock *falseBlock);
    bool visitCJumpStrictNullUndefined(IR::Type nullOrUndef, IR::Binop *binop,
                                       IR::BasicBlock *trueBlock, IR::BasicBlock *falseBlock);
    bool visitCJumpStrictBool(IR::Binop *binop, IR::BasicBlock *trueBlock, IR::BasicBlock *falseBlock);
    bool visitCJumpNullUndefined(IR::Type nullOrUndef, IR::Binop *binop,
                                 IR::BasicBlock *trueBlock, IR::BasicBlock *falseBlock);
    void visitCJumpEqual(IR::Binop *binop, IR::BasicBlock *trueBlock, IR::BasicBlock *falseBlock);

private:
    void convertTypeSlowPath(IR::Temp *source, IR::Temp *target);
    void convertTypeToDouble(IR::Temp *source, IR::Temp *target);
    void convertTypeToBool(IR::Temp *source, IR::Temp *target);
    void convertTypeToSInt32(IR::Temp *source, IR::Temp *target);
    void convertTypeToUInt32(IR::Temp *source, IR::Temp *target);

    void convertIntToDouble(IR::Temp *source, IR::Temp *target)
    {
        if (target->kind == IR::Temp::PhysicalRegister) {
            _as->convertInt32ToDouble(_as->toInt32Register(source, Assembler::ScratchRegister),
                                      (Assembler::FPRegisterID) target->index);
        } else {
            _as->convertInt32ToDouble(_as->toInt32Register(source, Assembler::ScratchRegister),
                                      Assembler::FPGpr0);
            _as->storeDouble(Assembler::FPGpr0, _as->stackSlotPointer(target));
        }
    }

    void convertUIntToDouble(IR::Temp *source, IR::Temp *target)
    {
        Assembler::RegisterID tmpReg = Assembler::ScratchRegister;
        Assembler::RegisterID reg = _as->toInt32Register(source, tmpReg);

        if (target->kind == IR::Temp::PhysicalRegister) {
            _as->convertUInt32ToDouble(reg, (Assembler::FPRegisterID) target->index, tmpReg);
        } else {
            _as->convertUInt32ToDouble(_as->toUInt32Register(source, tmpReg),
                                      Assembler::FPGpr0, tmpReg);
            _as->storeDouble(Assembler::FPGpr0, _as->stackSlotPointer(target));
        }
    }

    void convertIntToBool(IR::Temp *source, IR::Temp *target)
    {
        Assembler::RegisterID reg = target->kind == IR::Temp::PhysicalRegister
                ? (Assembler::RegisterID) target->index
                : Assembler::ScratchRegister;

        _as->move(_as->toInt32Register(source, reg), reg);
        _as->compare32(Assembler::NotEqual, reg, Assembler::TrustedImm32(0), reg);
        _as->storeBool(reg, target);
    }

    #define isel_stringIfyx(s) #s
    #define isel_stringIfy(s) isel_stringIfyx(s)

    #define generateFunctionCall(t, function, ...) \
        _as->generateFunctionCallImp(t, isel_stringIfy(function), function, __VA_ARGS__)

    int prepareVariableArguments(IR::ExprList* args);
    int prepareCallData(IR::ExprList* args, IR::Expr *thisObject);

    template <typename Retval, typename Arg1, typename Arg2, typename Arg3>
    void generateLookupCall(Retval retval, uint index, uint getterSetterOffset, Arg1 arg1, Arg2 arg2, Arg3 arg3)
    {
        Assembler::RegisterID lookupRegister;
#if CPU(ARM)
        lookupRegister = JSC::ARMRegisters::r8;
#else
        lookupRegister = Assembler::ReturnValueRegister;
#endif
        Assembler::Pointer lookupAddr(lookupRegister, index * sizeof(QV4::Lookup));

        Assembler::Address getterSetter = lookupAddr;
        getterSetter.offset += getterSetterOffset;

         _as->generateFunctionCallImp(retval, "lookup getter/setter",
                                      RelativeCall(getterSetter), lookupAddr, arg1, arg2, arg3);
    }

    template <typename Retval, typename Arg1, typename Arg2>
    void generateLookupCall(Retval retval, uint index, uint getterSetterOffset, Arg1 arg1, Arg2 arg2)
    {
        generateLookupCall(retval, index, getterSetterOffset, arg1, arg2, Assembler::VoidType());
    }

    IR::BasicBlock *_block;
    QSet<IR::Jump *> _removableJumps;
    Assembler* _as;

    CompilationUnit *compilationUnit;
    QQmlEnginePrivate *qmlEngine;
};

class Q_QML_EXPORT ISelFactory: public EvalISelFactory
{
public:
    virtual ~ISelFactory() {}
    virtual EvalInstructionSelection *create(QQmlEnginePrivate *qmlEngine, QV4::ExecutableAllocator *execAllocator, IR::Module *module, QV4::Compiler::JSUnitGenerator *jsGenerator)
    { return new InstructionSelection(qmlEngine, execAllocator, module, jsGenerator); }
    virtual bool jitCompileRegexps() const
    { return true; }
};

} // end of namespace JIT
} // end of namespace QV4

QT_END_NAMESPACE

#endif // ENABLE(ASSEMBLER)

#endif // QV4ISEL_MASM_P_H
