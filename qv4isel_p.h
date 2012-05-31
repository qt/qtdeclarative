#ifndef QV4ISEL_H
#define QV4ISEL_H

#include "qv4ir_p.h"

namespace QQmlJS {

class BaseInstructionSelection: protected IR::StmtVisitor
{
protected:
    struct DispatchExp: IR::ExprVisitor {
        BaseInstructionSelection *_isel;
        IR::Exp *_stmt;

        DispatchExp(BaseInstructionSelection *isel)
            : _isel(isel), _stmt(0) {}

        void operator()(IR::Exp *s)
        {
            qSwap(_stmt, s);
            _stmt->expr->accept(this);
            qSwap(_stmt, s);
        }

        virtual void visitConst(IR::Const *) { _isel->genExpConst(_stmt); }
        virtual void visitString(IR::String *) { _isel->genExpString(_stmt); }
        virtual void visitName(IR::Name *) { _isel->genExpName(_stmt); }
        virtual void visitTemp(IR::Temp *) { _isel->genExpTemp(_stmt); }
        virtual void visitClosure(IR::Closure *) { _isel->genExpClosure(_stmt); }
        virtual void visitUnop(IR::Unop *) { _isel->genExpUnop(_stmt); }
        virtual void visitBinop(IR::Binop *) { _isel->genExpBinop(_stmt); }
        virtual void visitCall(IR::Call *) { _isel->genExpCall(_stmt); }
        virtual void visitNew(IR::New *) { _isel->genExpNew(_stmt); }
        virtual void visitSubscript(IR::Subscript *) { _isel->genExpSubscript(_stmt); }
        virtual void visitMember(IR::Member *) { _isel->genExpMember(_stmt); }
    };

    struct DispatchRet: IR::ExprVisitor {
        BaseInstructionSelection *_isel;
        IR::Ret *_stmt;

        DispatchRet(BaseInstructionSelection *isel)
            : _isel(isel), _stmt(0) {}

        void operator()(IR::Ret *s)
        {
            qSwap(_stmt, s);
            _stmt->expr->accept(this);
            qSwap(_stmt, s);
        }

        virtual void visitConst(IR::Const *) { _isel->genRetConst(_stmt); }
        virtual void visitString(IR::String *) { _isel->genRetString(_stmt); }
        virtual void visitName(IR::Name *) { _isel->genRetName(_stmt); }
        virtual void visitTemp(IR::Temp *) { _isel->genRetTemp(_stmt); }
        virtual void visitClosure(IR::Closure *) { _isel->genRetClosure(_stmt); }
        virtual void visitUnop(IR::Unop *) { _isel->genRetUnop(_stmt); }
        virtual void visitBinop(IR::Binop *) { _isel->genRetBinop(_stmt); }
        virtual void visitCall(IR::Call *) { _isel->genRetCall(_stmt); }
        virtual void visitNew(IR::New *) { _isel->genRetNew(_stmt); }
        virtual void visitSubscript(IR::Subscript *) { _isel->genRetSubscript(_stmt); }
        virtual void visitMember(IR::Member *) { _isel->genRetMember(_stmt); }
    };

    struct DispatchMove: IR::ExprVisitor {
        BaseInstructionSelection *_isel;
        IR::Move *_stmt;

        DispatchMove(BaseInstructionSelection *isel)
            : _isel(isel), _stmt(0) {}

        void operator()(IR::Move *stmt)
        {
            qSwap(_stmt, stmt);
            _stmt->target->accept(this);
            qSwap(_stmt, stmt);
        }

        virtual void visitConst(IR::Const *) { Q_UNREACHABLE(); }
        virtual void visitString(IR::String *) { Q_UNREACHABLE(); }
        virtual void visitName(IR::Name *) { _isel->moveName(_stmt); }
        virtual void visitTemp(IR::Temp *) { _isel->moveTemp(_stmt); }
        virtual void visitClosure(IR::Closure *) { Q_UNREACHABLE(); }
        virtual void visitUnop(IR::Unop *) { Q_UNREACHABLE(); }
        virtual void visitBinop(IR::Binop *) { Q_UNREACHABLE(); }
        virtual void visitCall(IR::Call *) { Q_UNREACHABLE(); }
        virtual void visitNew(IR::New *) { Q_UNREACHABLE(); }
        virtual void visitSubscript(IR::Subscript *) { _isel->moveSubscript(_stmt); }
        virtual void visitMember(IR::Member *) { _isel->moveMember(_stmt); }
    };

    struct MoveTemp: IR::ExprVisitor {
        BaseInstructionSelection *_isel;
        IR::Move *_stmt;

        MoveTemp(BaseInstructionSelection *isel)
            : _isel(isel), _stmt(0) {}

        void operator()(IR::Move *stmt)
        {
            qSwap(_stmt, stmt);
            _stmt->source->accept(this);
            qSwap(_stmt, stmt);
        }

        virtual void visitConst(IR::Const *) { _isel->genMoveTempConst(_stmt); }
        virtual void visitString(IR::String *) { _isel->genMoveTempString(_stmt); }
        virtual void visitName(IR::Name *) { _isel->genMoveTempName(_stmt); }
        virtual void visitTemp(IR::Temp *) { _isel->genMoveTempTemp(_stmt); }
        virtual void visitClosure(IR::Closure *) { _isel->genMoveTempClosure(_stmt); }
        virtual void visitUnop(IR::Unop *) { _isel->genMoveTempUnop(_stmt); }
        virtual void visitBinop(IR::Binop *) { _isel->genMoveTempBinop(_stmt); }
        virtual void visitCall(IR::Call *) { _isel->genMoveTempCall(_stmt); }
        virtual void visitNew(IR::New *) { _isel->genMoveTempNew(_stmt); }
        virtual void visitSubscript(IR::Subscript *) { _isel->genMoveTempSubscript(_stmt); }
        virtual void visitMember(IR::Member *) { _isel->genMoveTempMember(_stmt); }
    };

    struct MoveName: IR::ExprVisitor {
        BaseInstructionSelection *_isel;
        IR::Move *_stmt;

        MoveName(BaseInstructionSelection *isel)
            : _isel(isel), _stmt(0) {}

        void operator()(IR::Move *stmt)
        {
            qSwap(_stmt, stmt);
            _stmt->source->accept(this);
            qSwap(_stmt, stmt);
        }

        virtual void visitConst(IR::Const *) { _isel->genMoveNameConst(_stmt); }
        virtual void visitString(IR::String *) { _isel->genMoveNameString(_stmt); }
        virtual void visitName(IR::Name *) { _isel->genMoveNameName(_stmt); }
        virtual void visitTemp(IR::Temp *) { _isel->genMoveNameTemp(_stmt); }
        virtual void visitClosure(IR::Closure *) { _isel->genMoveNameClosure(_stmt); }
        virtual void visitUnop(IR::Unop *) { _isel->genMoveNameUnop(_stmt); }
        virtual void visitBinop(IR::Binop *) { _isel->genMoveNameBinop(_stmt); }
        virtual void visitCall(IR::Call *) { _isel->genMoveNameCall(_stmt); }
        virtual void visitNew(IR::New *) { _isel->genMoveNameNew(_stmt); }
        virtual void visitSubscript(IR::Subscript *) { _isel->genMoveNameSubscript(_stmt); }
        virtual void visitMember(IR::Member *) { _isel->genMoveNameMember(_stmt); }
    };

    struct MoveMember: IR::ExprVisitor {
        BaseInstructionSelection *_isel;
        IR::Move *_stmt;

        MoveMember(BaseInstructionSelection *isel)
            : _isel(isel), _stmt(0) {}

        void operator()(IR::Move *stmt)
        {
            qSwap(_stmt, stmt);
            _stmt->source->accept(this);
            qSwap(_stmt, stmt);
        }

        virtual void visitConst(IR::Const *) { _isel->genMoveMemberConst(_stmt); }
        virtual void visitString(IR::String *) { _isel->genMoveMemberString(_stmt); }
        virtual void visitName(IR::Name *) { _isel->genMoveMemberName(_stmt); }
        virtual void visitTemp(IR::Temp *) { _isel->genMoveMemberTemp(_stmt); }
        virtual void visitClosure(IR::Closure *) { _isel->genMoveMemberClosure(_stmt); }
        virtual void visitUnop(IR::Unop *) { _isel->genMoveMemberUnop(_stmt); }
        virtual void visitBinop(IR::Binop *) { _isel->genMoveMemberBinop(_stmt); }
        virtual void visitCall(IR::Call *) { _isel->genMoveMemberCall(_stmt); }
        virtual void visitNew(IR::New *) { _isel->genMoveMemberNew(_stmt); }
        virtual void visitSubscript(IR::Subscript *) { _isel->genMoveMemberSubscript(_stmt); }
        virtual void visitMember(IR::Member *) { _isel->genMoveMemberMember(_stmt); }
    };

    struct MoveSubscript: IR::ExprVisitor {
        BaseInstructionSelection *_isel;
        IR::Move *_stmt;

        MoveSubscript(BaseInstructionSelection *isel)
            : _isel(isel), _stmt(0) {}

        void operator()(IR::Move *stmt)
        {
            qSwap(_stmt, stmt);
            _stmt->source->accept(this);
            qSwap(_stmt, stmt);
        }

        virtual void visitConst(IR::Const *) { _isel->genMoveSubscriptConst(_stmt); }
        virtual void visitString(IR::String *) { _isel->genMoveSubscriptString(_stmt); }
        virtual void visitName(IR::Name *) { _isel->genMoveSubscriptName(_stmt); }
        virtual void visitTemp(IR::Temp *) { _isel->genMoveSubscriptTemp(_stmt); }
        virtual void visitClosure(IR::Closure *) { _isel->genMoveSubscriptClosure(_stmt); }
        virtual void visitUnop(IR::Unop *) { _isel->genMoveSubscriptUnop(_stmt); }
        virtual void visitBinop(IR::Binop *) { _isel->genMoveSubscriptBinop(_stmt); }
        virtual void visitCall(IR::Call *) { _isel->genMoveSubscriptCall(_stmt); }
        virtual void visitNew(IR::New *) { _isel->genMoveSubscriptNew(_stmt); }
        virtual void visitSubscript(IR::Subscript *) { _isel->genMoveSubscriptSubscript(_stmt); }
        virtual void visitMember(IR::Member *) { _isel->genMoveSubscriptMember(_stmt); }
    };

    struct DispatchCJump: IR::ExprVisitor {
        BaseInstructionSelection *_isel;
        IR::CJump *_stmt;

        DispatchCJump(BaseInstructionSelection *isel)
            : _isel(isel), _stmt(0) {}

        void operator()(IR::CJump *s)
        {
            qSwap(_stmt, s);
            _stmt->cond->accept(this);
            qSwap(_stmt, s);
        }

        virtual void visitConst(IR::Const *) { _isel->genCJumpConst(_stmt); }
        virtual void visitString(IR::String *) { _isel->genCJumpString(_stmt); }
        virtual void visitName(IR::Name *) { _isel->genCJumpName(_stmt); }
        virtual void visitTemp(IR::Temp *) { _isel->genCJumpTemp(_stmt); }
        virtual void visitClosure(IR::Closure *) { _isel->genCJumpClosure(_stmt); }
        virtual void visitUnop(IR::Unop *) { _isel->genCJumpUnop(_stmt); }
        virtual void visitBinop(IR::Binop *) { _isel->genCJumpBinop(_stmt); }
        virtual void visitCall(IR::Call *) { _isel->genCJumpCall(_stmt); }
        virtual void visitNew(IR::New *) { _isel->genCJumpNew(_stmt); }
        virtual void visitSubscript(IR::Subscript *) { _isel->genCJumpSubscript(_stmt); }
        virtual void visitMember(IR::Member *) { _isel->genCJumpMember(_stmt); }
    };

    virtual void visitExp(IR::Exp *s)
    {
        dispatchExp(s);
    }

    virtual void visitEnter(IR::Enter *)
    {
        Q_UNREACHABLE();
    }

    virtual void visitLeave(IR::Leave *)
    {
        Q_UNREACHABLE();
    }

    virtual void visitMove(IR::Move *s)
    {
        dispatchMove(s);
    }

    virtual void visitJump(IR::Jump *s)
    {
        genJump(s);
    }

    virtual void visitCJump(IR::CJump *s)
    {
        dispatchCJump(s);
    }

    virtual void visitRet(IR::Ret *s)
    {
        dispatchRet(s);
    }

    DispatchExp dispatchExp;
    DispatchRet dispatchRet;
    DispatchCJump dispatchCJump;
    DispatchMove dispatchMove;
    MoveTemp moveTemp;
    MoveName moveName;
    MoveMember moveMember;
    MoveSubscript moveSubscript;

public:
    BaseInstructionSelection()
        : dispatchExp(this)
        , dispatchRet(this)
        , dispatchCJump(this)
        , dispatchMove(this)
        , moveTemp(this)
        , moveName(this)
        , moveMember(this)
        , moveSubscript(this) {}

    void statement(IR::Stmt *s) { s->accept(this); }

    virtual void genExpConst(IR::Exp *) { Q_UNIMPLEMENTED(); }
    virtual void genExpString(IR::Exp *) { Q_UNIMPLEMENTED(); }
    virtual void genExpName(IR::Exp *) { Q_UNIMPLEMENTED(); }
    virtual void genExpTemp(IR::Exp *) { Q_UNIMPLEMENTED(); }
    virtual void genExpClosure(IR::Exp *) { Q_UNIMPLEMENTED(); }
    virtual void genExpUnop(IR::Exp *) { Q_UNIMPLEMENTED(); }
    virtual void genExpBinop(IR::Exp *) { Q_UNIMPLEMENTED(); }
    virtual void genExpCall(IR::Exp *) { Q_UNIMPLEMENTED(); }
    virtual void genExpNew(IR::Exp *) { Q_UNIMPLEMENTED(); }
    virtual void genExpSubscript(IR::Exp *) { Q_UNIMPLEMENTED(); }
    virtual void genExpMember(IR::Exp *) { Q_UNIMPLEMENTED(); }

    virtual void genMoveTempConst(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveTempString(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveTempName(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveTempTemp(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveTempClosure(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveTempUnop(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveTempBinop(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveTempCall(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveTempNew(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveTempSubscript(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveTempMember(IR::Move *) { Q_UNIMPLEMENTED(); }

    virtual void genMoveNameConst(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveNameString(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveNameName(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveNameTemp(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveNameClosure(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveNameUnop(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveNameBinop(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveNameCall(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveNameNew(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveNameSubscript(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveNameMember(IR::Move *) { Q_UNIMPLEMENTED(); }

    virtual void genMoveMemberConst(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveMemberString(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveMemberName(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveMemberTemp(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveMemberClosure(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveMemberUnop(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveMemberBinop(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveMemberCall(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveMemberNew(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveMemberSubscript(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveMemberMember(IR::Move *) { Q_UNIMPLEMENTED(); }

    virtual void genMoveSubscriptConst(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveSubscriptString(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveSubscriptName(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveSubscriptTemp(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveSubscriptClosure(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveSubscriptUnop(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveSubscriptBinop(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveSubscriptCall(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveSubscriptNew(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveSubscriptSubscript(IR::Move *) { Q_UNIMPLEMENTED(); }
    virtual void genMoveSubscriptMember(IR::Move *) { Q_UNIMPLEMENTED(); }

    virtual void genJump(IR::Jump *) { Q_UNIMPLEMENTED(); }

    virtual void genCJumpConst(IR::CJump *) { Q_UNIMPLEMENTED(); }
    virtual void genCJumpString(IR::CJump *) { Q_UNIMPLEMENTED(); }
    virtual void genCJumpName(IR::CJump *) { Q_UNIMPLEMENTED(); }
    virtual void genCJumpTemp(IR::CJump *) { Q_UNIMPLEMENTED(); }
    virtual void genCJumpClosure(IR::CJump *) { Q_UNIMPLEMENTED(); }
    virtual void genCJumpUnop(IR::CJump *) { Q_UNIMPLEMENTED(); }
    virtual void genCJumpBinop(IR::CJump *) { Q_UNIMPLEMENTED(); }
    virtual void genCJumpCall(IR::CJump *) { Q_UNIMPLEMENTED(); }
    virtual void genCJumpNew(IR::CJump *) { Q_UNIMPLEMENTED(); }
    virtual void genCJumpSubscript(IR::CJump *) { Q_UNIMPLEMENTED(); }
    virtual void genCJumpMember(IR::CJump *) { Q_UNIMPLEMENTED(); }

    virtual void genRetConst(IR::Ret *) { Q_UNIMPLEMENTED(); }
    virtual void genRetString(IR::Ret *) { Q_UNIMPLEMENTED(); }
    virtual void genRetName(IR::Ret *) { Q_UNIMPLEMENTED(); }
    virtual void genRetTemp(IR::Ret *) { Q_UNIMPLEMENTED(); }
    virtual void genRetClosure(IR::Ret *) { Q_UNIMPLEMENTED(); }
    virtual void genRetUnop(IR::Ret *) { Q_UNIMPLEMENTED(); }
    virtual void genRetBinop(IR::Ret *) { Q_UNIMPLEMENTED(); }
    virtual void genRetCall(IR::Ret *) { Q_UNIMPLEMENTED(); }
    virtual void genRetNew(IR::Ret *) { Q_UNIMPLEMENTED(); }
    virtual void genRetSubscript(IR::Ret *) { Q_UNIMPLEMENTED(); }
    virtual void genRetMember(IR::Ret *) { Q_UNIMPLEMENTED(); }
};

} // end of namespace QQmlJS

#endif // QV4ISEL_H
