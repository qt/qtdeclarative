/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
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
****************************************************************************/

#ifndef QV4ISEL_P_H
#define QV4ISEL_P_H

#include <qglobal.h>
#include <QHash>

namespace QQmlJS {

namespace IR {
struct Function;
struct Module;
} // namespace IR;

namespace VM {
struct ExecutionEngine;
struct Function;
} // namespace VM

class EvalInstructionSelection
{
public:
    EvalInstructionSelection(VM::ExecutionEngine *engine, IR::Module *module);
    virtual ~EvalInstructionSelection() = 0;

    VM::Function *vmFunction(IR::Function *f);

protected:
    VM::Function *createFunctionMapping(VM::ExecutionEngine *engine, IR::Function *irFunction);
    VM::ExecutionEngine *engine() const { return _engine; }
    virtual void run(VM::Function *vmFunction, IR::Function *function) = 0;

private:
    VM::ExecutionEngine *_engine;
    QHash<IR::Function *, VM::Function *> _irToVM;
};

class EvalISelFactory
{
public:
    virtual ~EvalISelFactory() = 0;
    virtual EvalInstructionSelection *create(VM::ExecutionEngine *engine, IR::Module *module) = 0;
};

} // namespace QQmlJS

#endif // QV4ISEL_P_H
