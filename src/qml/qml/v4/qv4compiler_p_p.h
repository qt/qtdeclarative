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

#ifndef QV4COMPILER_P_P_H
#define QV4COMPILER_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qv4instruction_p.h"
#include "qv4ir_p.h"
#include <private/qqmlscript_p.h>
#include <private/qqmlimport_p.h>
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

template <typename _Key, typename _Value>
class QQmlAssociationList
{
public:
    typedef QVarLengthArray<QPair<_Key, _Value>, 8> Container;
    typedef typename Container::const_iterator const_iterator;
    typedef typename Container::const_iterator ConstIterator;

    const_iterator begin() const { return _container.begin(); }
    const_iterator end() const { return _container.end(); }
    int count() const { return _container.count(); }
    void clear() { _container.clear(); }

    _Value *value(const _Key &key) {
        for (int i = 0; i < _container.size(); ++i) {
            QPair<_Key, _Value> &p = _container[i];
            if (p.first == key)
                return &p.second;
        }
        return 0;
    }

    _Value &operator[](const _Key &key) {
        for (int i = 0; i < _container.size(); ++i) {
            QPair<_Key, _Value> &p = _container[i];
            if (p.first == key)
                return p.second;
        }
        int index = _container.size();
        _container.append(qMakePair(key, _Value()));
        return _container[index].second;
    }

    void insert(const _Key &key, _Value &value) {
        for (int i = 0; i < _container.size(); ++i) {
            QPair<_Key, _Value> &p = _container[i];
            if (p.first == key) {
                p.second = value;
                return;
            }
        }
        _container.append(qMakePair(key, value));
    }

private:
    Container _container;
};

class QV4CompilerPrivate: protected QQmlJS::IR::ExprVisitor, 
                                     protected QQmlJS::IR::StmtVisitor
{
public:
    QV4CompilerPrivate();

    void resetInstanceState();
    int commitCompile();

    const QV4Compiler::Expression *expression;
    QQmlEnginePrivate *engine;

    QString contextName() const { return QLatin1String("$$$SCOPE_") + QString::number((quintptr)expression->context, 16); }

    bool compile(QQmlJS::AST::Node *);

    bool isInvalidatable() const { return invalidatable; }

    int registerLiteralString(quint8 reg, const QStringRef &);
    QByteArray data;

    bool blockNeedsSubscription(const QStringList &);
    int subscriptionIndex(const QStringList &);
    quint32 subscriptionBlockMask(const QStringList &);

    quint8 exceptionId(quint16 line, quint16 column);
    quint8 exceptionId(QQmlJS::AST::ExpressionNode *);
    QVector<quint32> exceptions;

    QQmlAssociationList<int, quint32> usedSubscriptionIds;
    int subscriptionOffset;
    QQmlAssociationList<QString, int> subscriptionIds;
    QQmlJS::Bytecode bytecode;

    // back patching
    struct Patch {
        QQmlJS::IR::BasicBlock *block; // the basic block
        int offset; // the index of the instruction to patch
        Patch(QQmlJS::IR::BasicBlock *block = 0, int index = -1)
            : block(block), offset(index) {}
    };
    QVector<Patch> patches;
    QQmlPool pool;

    // Committed binding data
    struct Committed {
        Committed(): subscriptionCount(0) {}
        QList<int> offsets;
        QList<QQmlAssociationList<int, quint32> > dependencies;

        //QQmlJS::Bytecode bytecode;
        QByteArray bytecode;
        QByteArray data;
        QVector<quint32> exceptions;
        int subscriptionCount;
        QList<QQmlAssociationList<QString, int> > subscriptions;

        int count() const { return offsets.count(); }
    } committed;

    QByteArray buildSignalTable() const;
    QByteArray buildExceptionData() const;

    void convertToNumber(QQmlJS::IR::Expr *expr, int reg);
    void convertToInt(QQmlJS::IR::Expr *expr, int reg);
    void convertToBool(QQmlJS::IR::Expr *expr, int reg);
    quint8 instructionOpcode(QQmlJS::IR::Binop *e);

    struct Instr {
#define QML_V4_INSTR_DATA_TYPEDEF(I, FMT) typedef QQmlJS::V4InstrData<QQmlJS::V4Instr::I> I;
    FOR_EACH_V4_INSTR(QML_V4_INSTR_DATA_TYPEDEF)
#undef QML_v4_INSTR_DATA_TYPEDEF
    private:
        Instr();
    };

protected:
    //
    // tracing
    //
    void trace(quint16 line, quint16 column);
    void trace(QVector<QQmlJS::IR::BasicBlock *> *blocks);
    void traceExpression(QQmlJS::IR::Expr *e, quint8 r);

    template <int Instr>
    inline void gen(const QQmlJS::V4InstrData<Instr> &i)
    { bytecode.append(i); }
    inline void gen(QQmlJS::V4Instr::Type type, QQmlJS::V4Instr &instr)
    { bytecode.append(type, instr); }

    inline QQmlJS::V4Instr::Type instructionType(const QQmlJS::V4Instr *i) const
    { return bytecode.instructionType(i); }

    //
    // expressions
    //
    virtual void visitConst(QQmlJS::IR::Const *e);
    virtual void visitString(QQmlJS::IR::String *e);
    virtual void visitName(QQmlJS::IR::Name *e);
    virtual void visitTemp(QQmlJS::IR::Temp *e);
    virtual void visitUnop(QQmlJS::IR::Unop *e);
    virtual void visitBinop(QQmlJS::IR::Binop *e);
    virtual void visitCall(QQmlJS::IR::Call *e);

    //
    // statements
    //
    virtual void visitExp(QQmlJS::IR::Exp *s);
    virtual void visitMove(QQmlJS::IR::Move *s);
    virtual void visitJump(QQmlJS::IR::Jump *s);
    virtual void visitCJump(QQmlJS::IR::CJump *s);
    virtual void visitRet(QQmlJS::IR::Ret *s);

private:
    QStringList _subscribeName;
    QQmlJS::IR::Function *_function;
    QQmlJS::IR::BasicBlock *_block;
    void discard() { _discarded = true; }
    bool _discarded;
    quint8 currentReg;
    quint8 registerCount;

    bool usedSubscriptionIdsChanged;
    quint32 currentBlockMask;
    quint16 bindingLine;
    quint16 bindingColumn;
    bool invalidatable;
};


QT_END_NAMESPACE

#endif // QV4COMPILER_P_P_H

