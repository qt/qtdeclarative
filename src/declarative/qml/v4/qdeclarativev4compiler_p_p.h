/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEV4COMPILER_P_P_H
#define QDECLARATIVEV4COMPILER_P_P_H

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

#include "qdeclarativev4instruction_p.h"
#include "qdeclarativev4ir_p.h"
#include <private/qdeclarativeparser_p.h>
#include <private/qdeclarativeimport_p.h>
#include <private/qdeclarativeengine_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

// NOTE: This is a copy of QDeclarative1AnchorLine: src/qtquick1/graphicsitems/qdeclarativeanchors_p_p.h
class QGraphicsObject;
class QDeclarative1AnchorLine
{
public:
    QDeclarative1AnchorLine() : item(0), anchorLine(Invalid) {}

    enum AnchorLine {
        Invalid = 0x0,
        Left = 0x01,
        Right = 0x02,
        Top = 0x04,
        Bottom = 0x08,
        HCenter = 0x10,
        VCenter = 0x20,
        Baseline = 0x40,
        Horizontal_Mask = Left | Right | HCenter,
        Vertical_Mask = Top | Bottom | VCenter | Baseline
    };

    QGraphicsObject *item;
    AnchorLine anchorLine;
};

inline bool operator==(const QDeclarative1AnchorLine& a, const QDeclarative1AnchorLine& b)
{
    return a.item == b.item && a.anchorLine == b.anchorLine;
}


class QDeclarativeV4CompilerPrivate: protected QDeclarativeJS::IR::ExprVisitor, 
                                     protected QDeclarativeJS::IR::StmtVisitor
{
public:
    QDeclarativeV4CompilerPrivate();

    void resetInstanceState();
    int commitCompile();

    const QDeclarativeV4Compiler::Expression *expression;
    QDeclarativeEnginePrivate *engine;

    QString contextName() const { return QLatin1String("$$$SCOPE_") + QString::number((quintptr)expression->context, 16); }

    bool compile(QDeclarativeJS::AST::Node *);

    QHash<int, QPair<int, int> > registerCleanups;

    int registerLiteralString(quint8 reg, const QStringRef &);
    int registerString(const QString &);
    QHash<QString, QPair<int, int> > registeredStrings;
    QByteArray data;

    bool blockNeedsSubscription(const QStringList &);
    int subscriptionIndex(const QStringList &);
    quint32 subscriptionBlockMask(const QStringList &);

    quint8 exceptionId(quint32 line, quint32 column);
    quint8 exceptionId(QDeclarativeJS::AST::ExpressionNode *);
    QVector<quint64> exceptions;

    QHash<int, quint32> usedSubscriptionIds;

    QHash<QString, int> subscriptionIds;
    QDeclarativeJS::Bytecode bytecode;

    // back patching
    struct Patch {
        QDeclarativeJS::IR::BasicBlock *block; // the basic block
        int offset; // the index of the instruction to patch
        Patch(QDeclarativeJS::IR::BasicBlock *block = 0, int index = -1)
            : block(block), offset(index) {}
    };
    QVector<Patch> patches;
    QDeclarativePool pool;

    // Committed binding data
    struct {
        QList<int> offsets;
        QList<QHash<int, quint32> > dependencies;

        //QDeclarativeJS::Bytecode bytecode;
        QByteArray bytecode;
        QByteArray data;
        QHash<QString, int> subscriptionIds;
        QVector<quint64> exceptions;

        QHash<QString, QPair<int, int> > registeredStrings;

        int count() const { return offsets.count(); }
    } committed;

    QByteArray buildSignalTable() const;
    QByteArray buildExceptionData() const;

    void convertToReal(QDeclarativeJS::IR::Expr *expr, int reg);    
    void convertToInt(QDeclarativeJS::IR::Expr *expr, int reg);
    void convertToBool(QDeclarativeJS::IR::Expr *expr, int reg);
    quint8 instructionOpcode(QDeclarativeJS::IR::Binop *e);

protected:

    //
    // tracing
    //
    void trace(int line, int column);
    void trace(QVector<QDeclarativeJS::IR::BasicBlock *> *blocks);
    void traceExpression(QDeclarativeJS::IR::Expr *e, quint8 r);

    inline void gen(const QDeclarativeJS::Instr &i) { bytecode.append(i); }

    //
    // expressions
    //
    virtual void visitConst(QDeclarativeJS::IR::Const *e);
    virtual void visitString(QDeclarativeJS::IR::String *e);
    virtual void visitName(QDeclarativeJS::IR::Name *e);
    virtual void visitTemp(QDeclarativeJS::IR::Temp *e);
    virtual void visitUnop(QDeclarativeJS::IR::Unop *e);
    virtual void visitBinop(QDeclarativeJS::IR::Binop *e);
    virtual void visitCall(QDeclarativeJS::IR::Call *e);

    //
    // statements
    //
    virtual void visitExp(QDeclarativeJS::IR::Exp *s);
    virtual void visitMove(QDeclarativeJS::IR::Move *s);
    virtual void visitJump(QDeclarativeJS::IR::Jump *s);
    virtual void visitCJump(QDeclarativeJS::IR::CJump *s);
    virtual void visitRet(QDeclarativeJS::IR::Ret *s);

private:
    QStringList _subscribeName;
    QDeclarativeJS::IR::Function *_function;
    QDeclarativeJS::IR::BasicBlock *_block;
    void discard() { _discarded = true; }
    bool _discarded;
    quint8 currentReg;

    bool usedSubscriptionIdsChanged;
    quint32 currentBlockMask;
};


QT_END_NAMESPACE

Q_DECLARE_METATYPE(QDeclarative1AnchorLine)

QT_END_HEADER

#endif // QDECLARATIVEV4COMPILER_P_P_H

