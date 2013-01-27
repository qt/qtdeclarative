#ifndef QV4ISEL_MOTH_P_H
#define QV4ISEL_MOTH_P_H

#include "qv4isel_p.h"
#include "qv4ir_p.h"
#include "qv4object.h"
#include "qv4instr_moth_p.h"

namespace QQmlJS {
namespace Moth {

class InstructionSelection:
        public IR::InstructionSelection,
        public EvalInstructionSelection
{
public:
    InstructionSelection(VM::ExecutionEngine *engine, IR::Module *module);
    ~InstructionSelection();

    virtual void run(VM::Function *vmFunction, IR::Function *function);

protected:
    virtual void visitJump(IR::Jump *);
    virtual void visitCJump(IR::CJump *);
    virtual void visitRet(IR::Ret *);

    virtual void callBuiltinInvalid(IR::Name *func, IR::ExprList *args, IR::Temp *result);
    virtual void callBuiltinTypeofMember(IR::Temp *base, const QString &name, IR::Temp *result);
    virtual void callBuiltinTypeofSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result);
    virtual void callBuiltinTypeofName(const QString &name, IR::Temp *result);
    virtual void callBuiltinTypeofValue(IR::Temp *value, IR::Temp *result);
    virtual void callBuiltinDeleteMember(IR::Temp *base, const QString &name, IR::Temp *result);
    virtual void callBuiltinDeleteSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result);
    virtual void callBuiltinDeleteName(const QString &name, IR::Temp *result);
    virtual void callBuiltinDeleteValue(IR::Temp *result);
    virtual void callBuiltinPostDecrementMember(IR::Temp *base, const QString &name, IR::Temp *result);
    virtual void callBuiltinPostDecrementSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result);
    virtual void callBuiltinPostDecrementName(const QString &name, IR::Temp *result);
    virtual void callBuiltinPostDecrementValue(IR::Temp *value, IR::Temp *result);
    virtual void callBuiltinPostIncrementMember(IR::Temp *base, const QString &name, IR::Temp *result);
    virtual void callBuiltinPostIncrementSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result);
    virtual void callBuiltinPostIncrementName(const QString &name, IR::Temp *result);
    virtual void callBuiltinPostIncrementValue(IR::Temp *value, IR::Temp *result);
    virtual void callBuiltinThrow(IR::Temp *arg);
    virtual void callBuiltinCreateExceptionHandler(IR::Temp *result);
    virtual void callBuiltinDeleteExceptionHandler();
    virtual void callBuiltinGetException(IR::Temp *result);
    virtual void callBuiltinForeachIteratorObject(IR::Temp *arg, IR::Temp *result);
    virtual void callBuiltinForeachNextPropertyname(IR::Temp *arg, IR::Temp *result);
    virtual void callBuiltinPushWithScope(IR::Temp *arg);
    virtual void callBuiltinPopScope();
    virtual void callBuiltinDeclareVar(bool deletable, const QString &name);
    virtual void callBuiltinDefineGetterSetter(IR::Temp *object, const QString &name, IR::Temp *getter, IR::Temp *setter);
    virtual void callBuiltinDefineProperty(IR::Temp *object, const QString &name, IR::Temp *value);
    virtual void callBuiltinDefineArrayProperty(IR::Temp *object, int index, IR::Temp *value);
    virtual void callValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result);
    virtual void callProperty(IR::Temp *base, const QString &name, IR::ExprList *args, IR::Temp *result);
    virtual void callSubscript(IR::Temp *base, IR::Temp *index, IR::ExprList *args, IR::Temp *result);
    virtual void constructActivationProperty(IR::Name *func, IR::ExprList *args, IR::Temp *result);
    virtual void constructProperty(IR::Temp *base, const QString &name, IR::ExprList *args, IR::Temp *result);
    virtual void constructValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result);
    virtual void loadThisObject(IR::Temp *temp);
    virtual void loadConst(IR::Const *sourceConst, IR::Temp *targetTemp);
    virtual void loadString(const QString &str, IR::Temp *targetTemp);
    virtual void loadRegexp(IR::RegExp *sourceRegexp, IR::Temp *targetTemp);
    virtual void getActivationProperty(const QString &name, IR::Temp *temp);
    virtual void setActivationProperty(IR::Expr *source, const QString &targetName);
    virtual void initClosure(IR::Closure *closure, IR::Temp *target);
    virtual void getProperty(IR::Temp *base, const QString &name, IR::Temp *target);
    virtual void setProperty(IR::Expr *source, IR::Temp *targetBase, const QString &targetName);
    virtual void getElement(IR::Temp *base, IR::Temp *index, IR::Temp *target);
    virtual void setElement(IR::Expr *source, IR::Temp *targetBase, IR::Temp *targetIndex);
    virtual void copyValue(IR::Temp *sourceTemp, IR::Temp *targetTemp);
    virtual void unop(IR::AluOp oper, IR::Temp *sourceTemp, IR::Temp *targetTemp);
    virtual void binop(IR::AluOp oper, IR::Expr *leftSource, IR::Expr *rightSource, IR::Temp *target);
    virtual void inplaceNameOp(IR::AluOp oper, IR::Expr *sourceExpr, const QString &targetName);
    virtual void inplaceElementOp(IR::AluOp oper, IR::Expr *sourceExpr, IR::Temp *targetBaseTemp, IR::Temp *targetIndexTemp);
    virtual void inplaceMemberOp(IR::AluOp oper, IR::Expr *source, IR::Temp *targetBase, const QString &targetName);

private:
    struct Instruction {
#define MOTH_INSTR_DATA_TYPEDEF(I, FMT) typedef InstrData<Instr::I> I;
    FOR_EACH_MOTH_INSTR(MOTH_INSTR_DATA_TYPEDEF)
#undef MOTH_INSTR_DATA_TYPEDEF
    private:
        Instruction();
    };

    void simpleMove(IR::Move *);
    void prepareCallArgs(IR::ExprList *, quint32 &, quint32 &);

    int outgoingArgumentTempStart() const { return _function->tempCount; }
    int scratchTempIndex() const { return outgoingArgumentTempStart() + _function->maxNumberOfArguments; }
    int frameSize() const { return scratchTempIndex() + 1 - _function->locals.size(); }

    template <int Instr>
    inline ptrdiff_t addInstruction(const InstrData<Instr> &data);
    ptrdiff_t addInstructionHelper(Instr::Type type, Instr &instr);
    void patchJumpAddresses();
    uchar *squeezeCode() const;

    IR::Function *_function;
    IR::BasicBlock *_block;

    QHash<IR::BasicBlock *, QVector<ptrdiff_t> > _patches;
    QHash<IR::BasicBlock *, ptrdiff_t> _addrs;

    uchar *_codeStart;
    uchar *_codeNext;
    uchar *_codeEnd;
};

class ISelFactory: public EvalISelFactory
{
public:
    virtual ~ISelFactory() {}
    virtual EvalInstructionSelection *create(VM::ExecutionEngine *engine, IR::Module *module)
    { return new InstructionSelection(engine, module); }
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

#endif // QV4ISEL_MOTH_P_H
