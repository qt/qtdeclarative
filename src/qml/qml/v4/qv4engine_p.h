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
#ifndef QV4ENGINE_H
#define QV4ENGINE_H

#include "qv4global_p.h"
#include "qv4isel_p.h"
#include "qv4util_p.h"
#include "qv4context_p.h"
#include "qv4property_p.h"
#include <private/qintrusivelist_p.h>

namespace WTF {
class BumpPointerAllocator;
}

QT_BEGIN_NAMESPACE

class QJSEngine;

namespace QQmlJS {
namespace Debugging {
class Debugger;
} // namespace Debugging
}

namespace QV4 {

struct Value;
struct Function;
struct Object;
struct BooleanObject;
struct NumberObject;
struct StringObject;
struct ArrayObject;
struct DateObject;
struct FunctionObject;
struct BoundFunction;
struct RegExpObject;
struct ErrorObject;
struct ArgumentsObject;
struct ExecutionContext;
struct ExecutionEngine;
class MemoryManager;
class UnwindHelper;
class ExecutableAllocator;

struct ObjectPrototype;
struct StringPrototype;
struct NumberPrototype;
struct BooleanPrototype;
struct ArrayPrototype;
struct FunctionPrototype;
struct DatePrototype;
struct RegExpPrototype;
struct ErrorPrototype;
struct EvalErrorPrototype;
struct RangeErrorPrototype;
struct ReferenceErrorPrototype;
struct SyntaxErrorPrototype;
struct TypeErrorPrototype;
struct URIErrorPrototype;
struct VariantPrototype;
struct SequencePrototype;
struct EvalFunction;
struct Identifiers;
struct InternalClass;

class RegExp;
class RegExpCache;

typedef bool (*ExternalResourceComparison)(const Value &a, const Value &b);

struct Q_QML_EXPORT ExecutionEngine
{
    MemoryManager *memoryManager;
    ExecutableAllocator *executableAllocator;
    ExecutableAllocator *regExpAllocator;
    QScopedPointer<QQmlJS::EvalISelFactory> iselFactory;

    ExecutionContext *current;
    GlobalContext *rootContext;

    WTF::BumpPointerAllocator *bumperPointerAllocator; // Used by Yarr Regex engine.

    Identifiers *identifierCache;

    QQmlJS::Debugging::Debugger *debugger;

    Object *globalObject;

    Function *globalCode;

    QJSEngine *publicEngine;

    Value objectCtor;
    Value stringCtor;
    Value numberCtor;
    Value booleanCtor;
    Value arrayCtor;
    Value functionCtor;
    Value dateCtor;
    Value regExpCtor;
    Value errorCtor;
    Value evalErrorCtor;
    Value rangeErrorCtor;
    Value referenceErrorCtor;
    Value syntaxErrorCtor;
    Value typeErrorCtor;
    Value uRIErrorCtor;

    ObjectPrototype *objectPrototype;
    StringPrototype *stringPrototype;
    NumberPrototype *numberPrototype;
    BooleanPrototype *booleanPrototype;
    ArrayPrototype *arrayPrototype;
    FunctionPrototype *functionPrototype;
    DatePrototype *datePrototype;
    RegExpPrototype *regExpPrototype;
    ErrorPrototype *errorPrototype;
    EvalErrorPrototype *evalErrorPrototype;
    RangeErrorPrototype *rangeErrorPrototype;
    ReferenceErrorPrototype *referenceErrorPrototype;
    SyntaxErrorPrototype *syntaxErrorPrototype;
    TypeErrorPrototype *typeErrorPrototype;
    URIErrorPrototype *uRIErrorPrototype;

    VariantPrototype *variantPrototype;
    SequencePrototype *sequencePrototype;

    QQmlJS::MemoryPool classPool;
    InternalClass *emptyClass;
    InternalClass *arrayClass;

    EvalFunction *evalFunction;

    QVector<Property> argumentsAccessors;

    String *id_undefined;
    String *id_null;
    String *id_true;
    String *id_false;
    String *id_boolean;
    String *id_number;
    String *id_string;
    String *id_object;
    String *id_function;
    String *id_length;
    String *id_prototype;
    String *id_constructor;
    String *id_arguments;
    String *id_caller;
    String *id_this;
    String *id___proto__;
    String *id_enumerable;
    String *id_configurable;
    String *id_writable;
    String *id_value;
    String *id_get;
    String *id_set;
    String *id_eval;
    String *id_uintMax;
    String *id_name;

    mutable QVector<Function *> functions;
    mutable bool functionsNeedSort;

    ExternalResourceComparison externalResourceComparison;

    RegExpCache *regExpCache;

    // Scarce resources are "exceptionally high cost" QVariant types where allowing the
    // normal JavaScript GC to clean them up is likely to lead to out-of-memory or other
    // out-of-resource situations.  When such a resource is passed into JavaScript we
    // add it to the scarceResources list and it is destroyed when we return from the
    // JavaScript execution that created it.  The user can prevent this behavior by
    // calling preserve() on the object which removes it from this scarceResource list.
    class ScarceResourceData {
    public:
        ScarceResourceData(const QVariant &data) : data(data) {}
        QVariant data;
        QIntrusiveListNode node;
    };
    QIntrusiveList<ScarceResourceData, &ScarceResourceData::node> scarceResources;

    ExecutionEngine(QQmlJS::EvalISelFactory *iselFactory = 0);
    ~ExecutionEngine();

    WithContext *newWithContext(Object *with);
    CatchContext *newCatchContext(String* exceptionVarName, const QV4::Value &exceptionValue);
    CallContext *newCallContext(FunctionObject *f, const QV4::Value &thisObject, QV4::Value *args, int argc);
    CallContext *newCallContext(void *stackSpace, FunctionObject *f, const QV4::Value &thisObject, QV4::Value *args, int argc);
    CallContext *newQmlContext(FunctionObject *f, Object *qml);
    ExecutionContext *pushGlobalContext();
    void pushContext(SimpleCallContext *context);
    ExecutionContext *popContext();

    Function *newFunction(const QString &name);

    FunctionObject *newBuiltinFunction(ExecutionContext *scope, String *name, Value (*code)(SimpleCallContext *));
    FunctionObject *newScriptFunction(ExecutionContext *scope, Function *function);
    BoundFunction *newBoundFunction(ExecutionContext *scope, FunctionObject *target, Value boundThis, const QVector<Value> &boundArgs);

    Object *newObject();
    Object *newObject(InternalClass *internalClass);

    String *newString(const QString &s);
    String *newIdentifier(const QString &text);

    Object *newStringObject(const Value &value);
    Object *newNumberObject(const Value &value);
    Object *newBooleanObject(const Value &value);

    ArrayObject *newArrayObject();
    ArrayObject *newArrayObject(const QStringList &list);

    DateObject *newDateObject(const Value &value);
    DateObject *newDateObject(const QDateTime &dt);

    RegExpObject *newRegExpObject(const QString &pattern, int flags);
    RegExpObject *newRegExpObject(RegExp* re, bool global);
    RegExpObject *newRegExpObject(const QRegExp &re);

    Object *newErrorObject(const Value &value);
    Object *newSyntaxErrorObject(ExecutionContext *ctx, DiagnosticMessage *message);
    Object *newSyntaxErrorObject(const QString &message);
    Object *newReferenceErrorObject(const QString &message);
    Object *newTypeErrorObject(const QString &message);
    Object *newRangeErrorObject(const QString &message);
    Object *newURIErrorObject(Value message);

    Object *newVariantObject(const QVariant &v);

    Object *newForEachIteratorObject(ExecutionContext *ctx, Object *o);

    Object *qmlContextObject() const;

    struct StackFrame {
        QString source;
        QString function;
        int line;
        int column;
    };
    typedef QVector<StackFrame> StackTrace;
    StackTrace stackTrace(int frameLimit = -1) const;
    StackFrame currentStackFrame() const;

    void requireArgumentsAccessors(int n);

    void markObjects();

    void initRootContext();

    InternalClass *newClass(const InternalClass &other);

    Function *functionForProgramCounter(quintptr pc) const;
};

inline void ExecutionEngine::pushContext(SimpleCallContext *context)
{
    context->parent = current;
    current = context;
    current->currentEvalCode = 0;
}

inline ExecutionContext *ExecutionEngine::popContext()
{
    CallContext *c = current->asCallContext();
    if (c && !c->needsOwnArguments()) {
        c->arguments = 0;
        c->argumentCount = 0;
    }

    current = current->parent;
    return current;
}

struct Q_QML_EXPORT Exception {
    explicit Exception(ExecutionContext *throwingContext, const Value &exceptionValue);
    ~Exception();

    void accept(ExecutionContext *catchingContext);

    void partiallyUnwindContext(ExecutionContext *catchingContext);

    Value value() const { return exception; }

    ExecutionEngine::StackTrace stackTrace() const { return m_stackTrace; }

private:
    ExecutionContext *throwingContext;
    bool accepted;
    PersistentValue exception;
    ExecutionEngine::StackTrace m_stackTrace;
};

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4ENGINE_H
