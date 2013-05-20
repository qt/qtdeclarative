/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
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
#ifndef QV4FUNCTION_H
#define QV4FUNCTION_H

#include "qv4global_p.h"

#include <QtCore/QVector>
#include <QtCore/QByteArray>
#include <QtCore/qurl.h>

#include <config.h>
#include <assembler/MacroAssemblerCodeRef.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Value;
struct String;
struct Function;
struct Object;
struct FunctionObject;
struct ExecutionContext;
struct ExecutionEngine;
class MemoryManager;

struct ObjectPrototype;
struct StringPrototype;
struct NumberPrototype;
struct BooleanPrototype;
struct ArrayPrototype;
struct FunctionPrototype;
struct DatePrototype;
struct ErrorPrototype;
struct EvalErrorPrototype;
struct RangeErrorPrototype;
struct ReferenceErrorPrototype;
struct SyntaxErrorPrototype;
struct TypeErrorPrototype;
struct URIErrorPrototype;
struct InternalClass;
struct Lookup;

struct Function {
    String *name;

    Value (*code)(ExecutionContext *, const uchar *);
    const uchar *codeData;
    JSC::MacroAssemblerCodeRef codeRef;
    quint32 codeSize;

    QVector<String *> formals;
    QVector<String *> locals;
    QVector<Value> generatedValues;
    QVector<String *> identifiers;
    QVector<Function *> nestedFunctions;
    Function *outer;

    Lookup *lookups;

    bool hasNestedFunctions;
    bool hasDirectEval;
    bool usesArgumentsObject;
    bool isStrict;
    bool isNamedExpression;

    QUrl sourceFile;

    Function(String *name)
        : name(name)
        , code(0)
        , codeData(0)
        , codeSize(0)
        , outer(0)
        , lookups(0)
        , hasNestedFunctions(0)
        , hasDirectEval(false)
        , usesArgumentsObject(false)
        , isStrict(false)
        , isNamedExpression(false)
    {}
    ~Function();

    inline bool needsActivation() const { return hasNestedFunctions || hasDirectEval || usesArgumentsObject; }

    void mark();
};

}

QT_END_NAMESPACE

#endif
