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

namespace QQmlJS {
namespace MASM {

class Q_QML_EXPORT InstructionSelection:
        protected V4IR::IRDecoder,
        public EvalInstructionSelection
{
public:
    InstructionSelection(QQmlEnginePrivate *qmlEngine, QV4::ExecutableAllocator *execAllocator, V4IR::Module *module, QV4::Compiler::JSUnitGenerator *jsGenerator);
    ~InstructionSelection();

    virtual void run(int functionIndex);

    void *addConstantTable(QVector<QV4::Primitive> *values);
protected:
    virtual QV4::CompiledData::CompilationUnit *backendCompileStep();

    virtual void callBuiltinInvalid(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void callBuiltinTypeofMember(V4IR::Expr *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinTypeofSubscript(V4IR::Expr *base, V4IR::Expr *index, V4IR::Temp *result);
    virtual void callBuiltinTypeofName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinTypeofValue(V4IR::Expr *value, V4IR::Temp *result);
    virtual void callBuiltinDeleteMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinDeleteSubscript(V4IR::Temp *base, V4IR::Expr *index, V4IR::Temp *result);
    virtual void callBuiltinDeleteName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinDeleteValue(V4IR::Temp *result);
    virtual void callBuiltinThrow(V4IR::Expr *arg);
    virtual void callBuiltinReThrow();
    virtual void callBuiltinUnwindException(V4IR::Temp *);
    virtual void callBuiltinPushCatchScope(const QString &exceptionName);
    virtual void callBuiltinForeachIteratorObject(V4IR::Temp *arg, V4IR::Temp *result);
    virtual void callBuiltinForeachNextPropertyname(V4IR::Temp *arg, V4IR::Temp *result);
    virtual void callBuiltinPushWithScope(V4IR::Temp *arg);
    virtual void callBuiltinPopScope();
    virtual void callBuiltinDeclareVar(bool deletable, const QString &name);
    virtual void callBuiltinDefineGetterSetter(V4IR::Temp *object, const QString &name, V4IR::Temp *getter, V4IR::Temp *setter);
    virtual void callBuiltinDefineProperty(V4IR::Temp *object, const QString &name, V4IR::Expr *value);
    virtual void callBuiltinDefineArray(V4IR::Temp *result, V4IR::ExprList *args);
    virtual void callBuiltinDefineObjectLiteral(V4IR::Temp *result, V4IR::ExprList *args);
    virtual void callBuiltinSetupArgumentObject(V4IR::Temp *result);
    virtual void callBuiltinConvertThisToObject();
    virtual void callValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void callProperty(V4IR::Expr *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void callSubscript(V4IR::Expr *base, V4IR::Expr *index, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void convertType(V4IR::Temp *source, V4IR::Temp *target);
    virtual void loadThisObject(V4IR::Temp *temp);
    virtual void loadQmlIdArray(V4IR::Temp *temp);
    virtual void loadQmlImportedScripts(V4IR::Temp *temp);
    virtual void loadQmlContextObject(V4IR::Temp *temp);
    virtual void loadQmlScopeObject(V4IR::Temp *temp);
    virtual void loadQmlSingleton(const QString &name, V4IR::Temp *temp);
    virtual void loadConst(V4IR::Const *sourceConst, V4IR::Temp *targetTemp);
    virtual void loadString(const QString &str, V4IR::Temp *targetTemp);
    virtual void loadRegexp(V4IR::RegExp *sourceRegexp, V4IR::Temp *targetTemp);
    virtual void getActivationProperty(const V4IR::Name *name, V4IR::Temp *temp);
    virtual void setActivationProperty(V4IR::Expr *source, const QString &targetName);
    virtual void initClosure(V4IR::Closure *closure, V4IR::Temp *target);
    virtual void getProperty(V4IR::Expr *base, const QString &name, V4IR::Temp *target);
    virtual void setProperty(V4IR::Expr *source, V4IR::Expr *targetBase, const QString &targetName);
    virtual void setQObjectProperty(V4IR::Expr *source, V4IR::Expr *targetBase, int propertyIndex);
    virtual void getQObjectProperty(V4IR::Expr *base, int propertyIndex, bool captureRequired, int attachedPropertiesId, V4IR::Temp *target);
    virtual void getElement(V4IR::Expr *base, V4IR::Expr *index, V4IR::Temp *target);
    virtual void setElement(V4IR::Expr *source, V4IR::Expr *targetBase, V4IR::Expr *targetIndex);
    virtual void copyValue(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp);
    virtual void swapValues(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp);
    virtual void unop(V4IR::AluOp oper, V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp);
    virtual void binop(V4IR::AluOp oper, V4IR::Expr *leftSource, V4IR::Expr *rightSource, V4IR::Temp *target);

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

    virtual void constructActivationProperty(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void constructProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void constructValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result);

    virtual void visitJump(V4IR::Jump *);
    virtual void visitCJump(V4IR::CJump *);
    virtual void visitRet(V4IR::Ret *);

    Assembler::Jump genTryDoubleConversion(V4IR::Expr *src, Assembler::FPRegisterID dest);
    Assembler::Jump genInlineBinop(V4IR::AluOp oper, V4IR::Expr *leftSource,
                                   V4IR::Expr *rightSource, V4IR::Temp *target);
    void doubleBinop(V4IR::AluOp oper, V4IR::Expr *leftSource, V4IR::Expr *rightSource,
                     V4IR::Temp *target);
    Assembler::Jump branchDouble(bool invertCondition, V4IR::AluOp op, V4IR::Expr *left, V4IR::Expr *right);
    bool visitCJumpDouble(V4IR::AluOp op, V4IR::Expr *left, V4IR::Expr *right,
                          V4IR::BasicBlock *iftrue, V4IR::BasicBlock *iffalse);
    void visitCJumpStrict(V4IR::Binop *binop, V4IR::BasicBlock *trueBlock, V4IR::BasicBlock *falseBlock);
    bool visitCJumpStrictNullUndefined(V4IR::Type nullOrUndef, V4IR::Binop *binop,
                                       V4IR::BasicBlock *trueBlock, V4IR::BasicBlock *falseBlock);
    bool visitCJumpStrictBool(V4IR::Binop *binop, V4IR::BasicBlock *trueBlock, V4IR::BasicBlock *falseBlock);
    bool visitCJumpNullUndefined(V4IR::Type nullOrUndef, V4IR::Binop *binop,
                                 V4IR::BasicBlock *trueBlock, V4IR::BasicBlock *falseBlock);
    void visitCJumpEqual(V4IR::Binop *binop, V4IR::BasicBlock *trueBlock, V4IR::BasicBlock *falseBlock);
    bool int32Binop(V4IR::AluOp oper, V4IR::Expr *leftSource, V4IR::Expr *rightSource,
                    V4IR::Temp *target);

private:
    void convertTypeSlowPath(V4IR::Temp *source, V4IR::Temp *target);
    void convertTypeToDouble(V4IR::Temp *source, V4IR::Temp *target);
    void convertTypeToBool(V4IR::Temp *source, V4IR::Temp *target);
    void convertTypeToSInt32(V4IR::Temp *source, V4IR::Temp *target);
    void convertTypeToUInt32(V4IR::Temp *source, V4IR::Temp *target);

    void convertIntToDouble(V4IR::Temp *source, V4IR::Temp *target)
    {
        if (target->kind == V4IR::Temp::PhysicalRegister) {
            _as->convertInt32ToDouble(_as->toInt32Register(source, Assembler::ScratchRegister),
                                      (Assembler::FPRegisterID) target->index);
        } else {
            _as->convertInt32ToDouble(_as->toInt32Register(source, Assembler::ScratchRegister),
                                      Assembler::FPGpr0);
            _as->storeDouble(Assembler::FPGpr0, _as->stackSlotPointer(target));
        }
    }

    void convertUIntToDouble(V4IR::Temp *source, V4IR::Temp *target)
    {
        Assembler::RegisterID tmpReg = Assembler::ScratchRegister;
        Assembler::RegisterID reg = _as->toInt32Register(source, tmpReg);

        if (target->kind == V4IR::Temp::PhysicalRegister) {
            _as->convertUInt32ToDouble(reg, (Assembler::FPRegisterID) target->index, tmpReg);
        } else {
            _as->convertUInt32ToDouble(_as->toUInt32Register(source, tmpReg),
                                      Assembler::FPGpr0, tmpReg);
            _as->storeDouble(Assembler::FPGpr0, _as->stackSlotPointer(target));
        }
    }

    void convertIntToBool(V4IR::Temp *source, V4IR::Temp *target)
    {
        Assembler::RegisterID reg = target->kind == V4IR::Temp::PhysicalRegister
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

    int prepareVariableArguments(V4IR::ExprList* args);
    int prepareCallData(V4IR::ExprList* args, V4IR::Expr *thisObject);

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

    V4IR::BasicBlock *_block;
    QSet<V4IR::Jump *> _removableJumps;
    Assembler* _as;

    CompilationUnit *compilationUnit;
    QQmlEnginePrivate *qmlEngine;
};

class Q_QML_EXPORT ISelFactory: public EvalISelFactory
{
public:
    virtual ~ISelFactory() {}
    virtual EvalInstructionSelection *create(QQmlEnginePrivate *qmlEngine, QV4::ExecutableAllocator *execAllocator, V4IR::Module *module, QV4::Compiler::JSUnitGenerator *jsGenerator)
    { return new InstructionSelection(qmlEngine, execAllocator, module, jsGenerator); }
    virtual bool jitCompileRegexps() const
    { return true; }
};

} // end of namespace MASM
} // end of namespace QQmlJS

QT_END_NAMESPACE

#endif // ENABLE(ASSEMBLER)

#endif // QV4ISEL_MASM_P_H
