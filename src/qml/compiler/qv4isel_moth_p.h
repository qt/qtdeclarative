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

#ifndef QV4ISEL_MOTH_P_H
#define QV4ISEL_MOTH_P_H

#include <private/qv4global_p.h>
#include <private/qv4isel_p.h>
#include <private/qv4isel_util_p.h>
#include <private/qv4jsir_p.h>
#include <private/qv4value_def_p.h>
#include "qv4instr_moth_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Moth {

struct CompilationUnit : public QV4::CompiledData::CompilationUnit
{
    virtual ~CompilationUnit();
    virtual void linkBackendToEngine(QV4::ExecutionEngine *engine);

    QVector<QByteArray> codeRefs;

};

class Q_QML_EXPORT InstructionSelection:
        public V4IR::IRDecoder,
        public EvalInstructionSelection
{
public:
    InstructionSelection(QQmlEnginePrivate *qmlEngine, QV4::ExecutableAllocator *execAllocator, V4IR::Module *module, QV4::Compiler::JSUnitGenerator *jsGenerator);
    ~InstructionSelection();

    virtual void run(int functionIndex);

protected:
    virtual QV4::CompiledData::CompilationUnit *backendCompileStep();

    virtual void visitJump(V4IR::Jump *);
    virtual void visitCJump(V4IR::CJump *);
    virtual void visitRet(V4IR::Ret *);

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
    virtual void constructActivationProperty(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void constructProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void constructValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result);
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

private:
    Param binopHelper(V4IR::AluOp oper, V4IR::Expr *leftSource, V4IR::Expr *rightSource, V4IR::Temp *target);

    struct Instruction {
#define MOTH_INSTR_DATA_TYPEDEF(I, FMT) typedef InstrData<Instr::I> I;
    FOR_EACH_MOTH_INSTR(MOTH_INSTR_DATA_TYPEDEF)
#undef MOTH_INSTR_DATA_TYPEDEF
    private:
        Instruction();
    };

    Param getParam(V4IR::Expr *e);

    Param getResultParam(V4IR::Temp *result)
    {
        if (result)
            return getParam(result);
        else
            return Param::createTemp(scratchTempIndex());
    }

    void simpleMove(V4IR::Move *);
    void prepareCallArgs(V4IR::ExprList *, quint32 &, quint32 * = 0);

    int scratchTempIndex() const { return _function->tempCount; }
    int callDataStart() const { return scratchTempIndex() + 1; }
    int outgoingArgumentTempStart() const { return callDataStart() + qOffsetOf(QV4::CallData, args)/sizeof(QV4::SafeValue); }
    int frameSize() const { return outgoingArgumentTempStart() + _function->maxNumberOfArguments; }

    template <int Instr>
    inline ptrdiff_t addInstruction(const InstrData<Instr> &data);
    ptrdiff_t addInstructionHelper(Instr::Type type, Instr &instr);
    void patchJumpAddresses();
    QByteArray squeezeCode() const;

    QQmlEnginePrivate *qmlEngine;

    V4IR::BasicBlock *_block;
    V4IR::BasicBlock *_nextBlock;

    QHash<V4IR::BasicBlock *, QVector<ptrdiff_t> > _patches;
    QHash<V4IR::BasicBlock *, ptrdiff_t> _addrs;

    uchar *_codeStart;
    uchar *_codeNext;
    uchar *_codeEnd;

    QSet<V4IR::Jump *> _removableJumps;
    V4IR::Stmt *_currentStatement;

    CompilationUnit *compilationUnit;
    QHash<V4IR::Function *, QByteArray> codeRefs;
};

class Q_QML_EXPORT ISelFactory: public EvalISelFactory
{
public:
    virtual ~ISelFactory() {}
    virtual EvalInstructionSelection *create(QQmlEnginePrivate *qmlEngine, QV4::ExecutableAllocator *execAllocator, V4IR::Module *module, QV4::Compiler::JSUnitGenerator *jsGenerator)
    { return new InstructionSelection(qmlEngine, execAllocator, module, jsGenerator); }
    virtual bool jitCompileRegexps() const
    { return false; }
};

template<int InstrT>
ptrdiff_t InstructionSelection::addInstruction(const InstrData<InstrT> &data)
{
    Instr genericInstr;
    InstrMeta<InstrT>::setData(genericInstr, data);
    return addInstructionHelper(static_cast<Instr::Type>(InstrT), genericInstr);
}

} // namespace Moth
} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QV4ISEL_MOTH_P_H
