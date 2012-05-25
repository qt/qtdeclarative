#ifndef QV4ISEL_P_H
#define QV4ISEL_P_H

#include "qv4ir_p.h"
#include "qmljs_objects.h"

#include <QtCore/QHash>

namespace QQmlJS {
namespace x86_64 {

class InstructionSelection: protected IR::StmtVisitor
{
public:
    InstructionSelection(VM::ExecutionEngine *engine, IR::Module *module, uchar *code);
    ~InstructionSelection();

    void operator()(IR::Function *function);

protected:
    VM::String *identifier(const QString &s);
    void loadTempAddress(int reg, IR::Temp *t);
    void callActivationProperty(IR::Call *call, IR::Temp *result);
    void callProperty(IR::Call *call, IR::Temp *result);
    void constructActivationProperty(IR::New *call, IR::Temp *result);
    void constructProperty(IR::New *ctor, IR::Temp *result);
    void callValue(IR::Call *call, IR::Temp *result);
    void constructValue(IR::New *call, IR::Temp *result);
    void checkExceptions();

    virtual void visitExp(IR::Exp *);
    virtual void visitEnter(IR::Enter *);
    virtual void visitLeave(IR::Leave *);
    virtual void visitMove(IR::Move *);
    virtual void visitJump(IR::Jump *);
    virtual void visitCJump(IR::CJump *);
    virtual void visitRet(IR::Ret *);

private:
    VM::ExecutionEngine *_engine;
    IR::Module *_module;
    IR::Function *_function;
    IR::BasicBlock *_block;
    uchar *_buffer;
    uchar *_code;
    uchar *_codePtr;
    QHash<IR::BasicBlock *, QVector<uchar *> > _patches;
    QHash<IR::BasicBlock *, uchar *> _addrs;
};

} // end of namespace x86_64
} // end of namespace QQmlJS

#endif // QV4ISEL_P_H
