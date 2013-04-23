#ifndef QV4ISEL_MOTH_P_H
#define QV4ISEL_MOTH_P_H

#include <private/qv4global_p.h>
#include <private/qv4isel_p.h>
#include <private/qv4jsir_p.h>
#include <private/qv4object_p.h>
#include "qv4instr_moth_p.h"

namespace QQmlJS {
namespace Moth {

class Q_QML_EXPORT InstructionSelection:
        public V4IR::InstructionSelection,
        public EvalInstructionSelection
{
public:
    InstructionSelection(QV4::ExecutionEngine *engine, V4IR::Module *module);
    ~InstructionSelection();

    virtual void run(QV4::Function *vmFunction, V4IR::Function *function);

protected:
    virtual void visitJump(V4IR::Jump *);
    virtual void visitCJump(V4IR::CJump *);
    virtual void visitRet(V4IR::Ret *);
    virtual void visitTry(V4IR::Try *);

    virtual void callBuiltinInvalid(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void callBuiltinTypeofMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinTypeofSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result);
    virtual void callBuiltinTypeofName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinTypeofValue(V4IR::Temp *value, V4IR::Temp *result);
    virtual void callBuiltinDeleteMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinDeleteSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result);
    virtual void callBuiltinDeleteName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinDeleteValue(V4IR::Temp *result);
    virtual void callBuiltinPostDecrementMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinPostDecrementSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result);
    virtual void callBuiltinPostDecrementName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinPostDecrementValue(V4IR::Temp *value, V4IR::Temp *result);
    virtual void callBuiltinPostIncrementMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinPostIncrementSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result);
    virtual void callBuiltinPostIncrementName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinPostIncrementValue(V4IR::Temp *value, V4IR::Temp *result);
    virtual void callBuiltinThrow(V4IR::Temp *arg);
    virtual void callBuiltinFinishTry();
    virtual void callBuiltinForeachIteratorObject(V4IR::Temp *arg, V4IR::Temp *result);
    virtual void callBuiltinForeachNextPropertyname(V4IR::Temp *arg, V4IR::Temp *result);
    virtual void callBuiltinPushWithScope(V4IR::Temp *arg);
    virtual void callBuiltinPopScope();
    virtual void callBuiltinDeclareVar(bool deletable, const QString &name);
    virtual void callBuiltinDefineGetterSetter(V4IR::Temp *object, const QString &name, V4IR::Temp *getter, V4IR::Temp *setter);
    virtual void callBuiltinDefineProperty(V4IR::Temp *object, const QString &name, V4IR::Temp *value);
    virtual void callBuiltinDefineArray(V4IR::Temp *result, V4IR::ExprList *args);
    virtual void callBuiltinDefineObjectLiteral(V4IR::Temp *result, V4IR::ExprList *args);
    virtual void callValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void callProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void callSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void constructActivationProperty(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void constructProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void constructValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void loadThisObject(V4IR::Temp *temp);
    virtual void loadConst(V4IR::Const *sourceConst, V4IR::Temp *targetTemp);
    virtual void loadString(const QString &str, V4IR::Temp *targetTemp);
    virtual void loadRegexp(V4IR::RegExp *sourceRegexp, V4IR::Temp *targetTemp);
    virtual void getActivationProperty(const V4IR::Name *name, V4IR::Temp *temp);
    virtual void setActivationProperty(V4IR::Temp *source, const QString &targetName);
    virtual void initClosure(V4IR::Closure *closure, V4IR::Temp *target);
    virtual void getProperty(V4IR::Temp *base, const QString &name, V4IR::Temp *target);
    virtual void setProperty(V4IR::Temp *source, V4IR::Temp *targetBase, const QString &targetName);
    virtual void getElement(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *target);
    virtual void setElement(V4IR::Temp *source, V4IR::Temp *targetBase, V4IR::Temp *targetIndex);
    virtual void copyValue(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp);
    virtual void unop(V4IR::AluOp oper, V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp);
    virtual void binop(V4IR::AluOp oper, V4IR::Temp *leftSource, V4IR::Temp *rightSource, V4IR::Temp *target);
    virtual void inplaceNameOp(V4IR::AluOp oper, V4IR::Temp *rightSource, const QString &targetName);
    virtual void inplaceElementOp(V4IR::AluOp oper, V4IR::Temp *source, V4IR::Temp *targetBaseTemp, V4IR::Temp *targetIndexTemp);
    virtual void inplaceMemberOp(V4IR::AluOp oper, V4IR::Temp *source, V4IR::Temp *targetBase, const QString &targetName);

private:
    struct Instruction {
#define MOTH_INSTR_DATA_TYPEDEF(I, FMT) typedef InstrData<Instr::I> I;
    FOR_EACH_MOTH_INSTR(MOTH_INSTR_DATA_TYPEDEF)
#undef MOTH_INSTR_DATA_TYPEDEF
    private:
        Instruction();
    };

    Instr::Param getParam(V4IR::Expr *e)
    {
        typedef Instr::Param Param;
        assert(e);

        if (V4IR::Const *c = e->asConst()) {
            return Param::createValue(convertToValue(c));
        } else if (V4IR::Temp *t = e->asTemp()) {
            const int index = t->index;
            if (index < 0) {
                return Param::createArgument(-index - 1, t->scope);
            } else if (!t->scope) {
                const int localCount = _function->locals.size();
                if (index < localCount)
                    return Param::createLocal(index);
                else
                    return Param::createTemp(index - localCount);
            } else {
                return Param::createScopedLocal(t->index, t->scope);
            }
        } else {
            Q_UNIMPLEMENTED();
            return Param();
        }
    }

    Instr::Param getResultParam(V4IR::Temp *result)
    {
        if (result)
            return getParam(result);
        else
            return Instr::Param::createTemp(scratchTempIndex());
    }

    void simpleMove(V4IR::Move *);
    void prepareCallArgs(V4IR::ExprList *, quint32 &, quint32 &);

    int outgoingArgumentTempStart() const { return _function->tempCount - _function->locals.size(); }
    int scratchTempIndex() const { return outgoingArgumentTempStart() + _function->maxNumberOfArguments; }
    int frameSize() const { return scratchTempIndex() + 1; }

    template <int Instr>
    inline ptrdiff_t addInstruction(const InstrData<Instr> &data);
    ptrdiff_t addInstructionHelper(Instr::Type type, Instr &instr);
    void patchJumpAddresses();
    uchar *squeezeCode() const;

    QV4::String *identifier(const QString &s);

    V4IR::Function *_function;
    QV4::Function *_vmFunction;
    V4IR::BasicBlock *_block;

    QHash<V4IR::BasicBlock *, QVector<ptrdiff_t> > _patches;
    QHash<V4IR::BasicBlock *, ptrdiff_t> _addrs;

    uchar *_codeStart;
    uchar *_codeNext;
    uchar *_codeEnd;
};

class Q_QML_EXPORT ISelFactory: public EvalISelFactory
{
public:
    virtual ~ISelFactory() {}
    virtual EvalInstructionSelection *create(QV4::ExecutionEngine *engine, V4IR::Module *module)
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
