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

#ifndef QV4SSA_P_H
#define QV4SSA_P_H

#include "qv4jsir_p.h"

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace V4IR {

class LifeTimeInterval {
    struct Range {
        int start;
        int end;

        Range(int start = Invalid, int end = Invalid)
            : start(start)
            , end(end)
        {}
    };

    Temp _temp;
    QList<Range> _ranges;
    int _reg;

public:
    static const int Invalid = -1;

    LifeTimeInterval()
        : _reg(Invalid)
    {}

    void setTemp(const Temp &temp) { this->_temp = temp; }
    Temp temp() const { return _temp; }

    void setFrom(Stmt *from);
    void addRange(Stmt *from, Stmt *to);

    int start() const { return _ranges.first().start; }
    int end() const { return _ranges.last().end; }

    int reg() const { return _reg; }
    void setReg(int reg) { _reg = reg; }

    void dump() const;
    static bool lessThan(const LifeTimeInterval &r1, const LifeTimeInterval &r2);
};

class Optimizer
{
public:
    struct SSADeconstructionMove
    {
        Expr *source;
        Temp *target;

        bool needsConversion() const
        { return target->type != source->type; }
    };

public:
    Optimizer(Function *function)
        : function(function)
        , inSSA(false)
    {}

    void run();
    void convertOutOfSSA();

    bool isInSSA() const
    { return inSSA; }

    QList<SSADeconstructionMove> ssaDeconstructionMoves(BasicBlock *basicBlock);

    QList<LifeTimeInterval> lifeRanges() const;

private:
    Function *function;
    bool inSSA;
    QHash<BasicBlock *, BasicBlock *> startEndLoops;
};

} // V4IR namespace
} // QQmlJS namespace
QT_END_NAMESPACE

#endif // QV4SSA_P_H
