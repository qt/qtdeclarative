#ifndef QV4ISEL_P_H
#define QV4ISEL_P_H

#include "qv4ir_p.h"
#include "qmljs_objects.h"

#include <QtCore/QHash>

namespace QQmlJS {

class InstructionSelection: protected IR::StmtVisitor
{
public:
    InstructionSelection(IR::Module *module);
    ~InstructionSelection();

    void visitFunction(IR::Function *function);

protected:
    String *identifier(const QString &s);

    virtual void visitExp(IR::Exp *);
    virtual void visitEnter(IR::Enter *);
    virtual void visitLeave(IR::Leave *);
    virtual void visitMove(IR::Move *);
    virtual void visitJump(IR::Jump *);
    virtual void visitCJump(IR::CJump *);
    virtual void visitRet(IR::Ret *);

private:
    IR::Module *_module;
    IR::Function *_function;
    IR::BasicBlock *_block;
    uchar *_code;
    uchar *_codePtr;
    QHash<IR::BasicBlock *, QVector<uchar *> > _patches;
    QHash<IR::BasicBlock *, uchar *> _addrs;
    QHash<QString, String *> _identifiers;
};

} // end of namespace QQmlJS

#endif // QV4ISEL_P_H
