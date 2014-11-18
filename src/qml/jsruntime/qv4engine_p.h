/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4ENGINE_H
#define QV4ENGINE_H

#include "qv4global_p.h"
#include "private/qv4isel_p.h"
#include "qv4managed_p.h"
#include "qv4context_p.h"
#include <private/qintrusivelist_p.h>

namespace WTF {
class BumpPointerAllocator;
class PageAllocation;
}

QT_BEGIN_NAMESPACE

class QV8Engine;
class QQmlError;

namespace QV4 {
namespace Debugging {
class Debugger;
} // namespace Debugging
namespace Profiling {
class Profiler;
} // namespace Profiling
namespace CompiledData {
struct CompilationUnit;
}

#define CHECK_STACK_LIMITS(v4) \
    if ((v4->jsStackTop <= v4->jsStackLimit) && (reinterpret_cast<quintptr>(&v4) >= v4->cStackLimit || v4->recheckCStackLimits())) {}  \
    else \
        return v4->throwRangeError(QStringLiteral("Maximum call stack size exceeded."))


struct Q_QML_EXPORT ExecutionEngine
{
private:
    friend struct ExecutionContextSaver;
    friend struct ExecutionContext;
    friend struct Heap::ExecutionContext;
public:
    ExecutionContext *current;
    ExecutionContext *currentContext() const { return current; }

    Value *jsStackTop;
    quint32 hasException;
    GlobalContext *rootContext;

    MemoryManager *memoryManager;
    ExecutableAllocator *executableAllocator;
    ExecutableAllocator *regExpAllocator;
    QScopedPointer<EvalISelFactory> iselFactory;


    Value *jsStackLimit;
    quintptr cStackLimit;

    WTF::BumpPointerAllocator *bumperPointerAllocator; // Used by Yarr Regex engine.

    enum { JSStackLimit = 4*1024*1024 };
    WTF::PageAllocation *jsStack;
    Value *jsStackBase;

    Value *stackPush(uint nValues) {
        Value *ptr = jsStackTop;
        jsStackTop = ptr + nValues;
        return ptr;
    }
    void stackPop(uint nValues) {
        jsStackTop -= nValues;
    }

    void pushForGC(Heap::Base *m) {
        *jsStackTop = m;
        ++jsStackTop;
    }
    Heap::Base *popForGC() {
        --jsStackTop;
        return jsStackTop->heapObject();
    }

    IdentifierTable *identifierTable;

    QV4::Debugging::Debugger *debugger;
    QV4::Profiling::Profiler *profiler;

    Object *globalObject;

    Function *globalCode;

    QV8Engine *v8Engine;

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
    Value sequencePrototype;
    Value arrayBufferCtor;
    Value dataViewCtor;
    enum { NTypedArrayTypes = 9 }; // avoid header dependency
    Value typedArrayCtors[NTypedArrayTypes];

    InternalClassPool *classPool;
    InternalClass *emptyClass;
    InternalClass *executionContextClass;
    InternalClass *constructClass;
    InternalClass *stringClass;

    InternalClass *objectClass;
    InternalClass *arrayClass;
    InternalClass *simpleArrayDataClass;
    InternalClass *stringObjectClass;
    InternalClass *booleanClass;
    InternalClass *numberClass;
    InternalClass *dateClass;

    InternalClass *functionClass;
    InternalClass *protoClass;

    InternalClass *regExpClass;
    InternalClass *regExpExecArrayClass;
    InternalClass *regExpValueClass;

    InternalClass *errorClass;
    InternalClass *evalErrorClass;
    InternalClass *rangeErrorClass;
    InternalClass *referenceErrorClass;
    InternalClass *syntaxErrorClass;
    InternalClass *typeErrorClass;
    InternalClass *uriErrorClass;
    InternalClass *argumentsObjectClass;
    InternalClass *strictArgumentsObjectClass;

    InternalClass *variantClass;
    InternalClass *memberDataClass;

    InternalClass *arrayBufferClass;
    InternalClass *dataViewClass;
    InternalClass *typedArrayClasses[NTypedArrayTypes]; // TypedArray::NValues, avoid including the header here

    EvalFunction *evalFunction;
    FunctionObject *thrower;

    Property *argumentsAccessors;
    int nArgumentsAccessors;

    StringValue id_empty;
    StringValue id_undefined;
    StringValue id_null;
    StringValue id_true;
    StringValue id_false;
    StringValue id_boolean;
    StringValue id_number;
    StringValue id_string;
    StringValue id_object;
    StringValue id_function;
    StringValue id_length;
    StringValue id_prototype;
    StringValue id_constructor;
    StringValue id_arguments;
    StringValue id_caller;
    StringValue id_callee;
    StringValue id_this;
    StringValue id___proto__;
    StringValue id_enumerable;
    StringValue id_configurable;
    StringValue id_writable;
    StringValue id_value;
    StringValue id_get;
    StringValue id_set;
    StringValue id_eval;
    StringValue id_uintMax;
    StringValue id_name;
    StringValue id_index;
    StringValue id_input;
    StringValue id_toString;
    StringValue id_destroy;
    StringValue id_valueOf;
    StringValue id_byteLength;
    StringValue id_byteOffset;
    StringValue id_buffer;
    StringValue id_lastIndex;

    QSet<CompiledData::CompilationUnit*> compilationUnits;

    quint32 m_engineId;

    RegExpCache *regExpCache;

    // Scarce resources are "exceptionally high cost" QVariant types where allowing the
    // normal JavaScript GC to clean them up is likely to lead to out-of-memory or other
    // out-of-resource situations.  When such a resource is passed into JavaScript we
    // add it to the scarceResources list and it is destroyed when we return from the
    // JavaScript execution that created it.  The user can prevent this behavior by
    // calling preserve() on the object which removes it from this scarceResource list.
    class ScarceResourceData {
    public:
        ScarceResourceData(const QVariant &data = QVariant()) : data(data) {}
        QVariant data;
        QIntrusiveListNode node;
    };
    QIntrusiveList<ScarceResourceData, &ScarceResourceData::node> scarceResources;

    // Normally the JS wrappers for QObjects are stored in the QQmlData/QObjectPrivate,
    // but any time a QObject is wrapped a second time in another engine, we have to do
    // bookkeeping.
    MultiplyWrappedQObjectMap *m_multiplyWrappedQObjects;

    ExecutionEngine(EvalISelFactory *iselFactory = 0);
    ~ExecutionEngine();

    void enableDebugger();
    void enableProfiler();

    ExecutionContext *pushGlobalContext();
    void pushContext(CallContext *context);
    ExecutionContext *popContext();

    Heap::Object *newObject();
    Heap::Object *newObject(InternalClass *internalClass);

    Heap::String *newString(const QString &s);
    Heap::String *newIdentifier(const QString &text);

    Heap::Object *newStringObject(const ValueRef value);
    Heap::Object *newNumberObject(const ValueRef value);
    Heap::Object *newBooleanObject(const ValueRef value);

    Heap::ArrayObject *newArrayObject(int count = 0);
    Heap::ArrayObject *newArrayObject(const QStringList &list);
    Heap::ArrayObject *newArrayObject(InternalClass *ic);

    Heap::DateObject *newDateObject(const ValueRef value);
    Heap::DateObject *newDateObject(const QDateTime &dt);

    Heap::RegExpObject *newRegExpObject(const QString &pattern, int flags);
    Heap::RegExpObject *newRegExpObject(RegExp *re, bool global);
    Heap::RegExpObject *newRegExpObject(const QRegExp &re);

    Heap::Object *newErrorObject(const ValueRef value);
    Heap::Object *newSyntaxErrorObject(const QString &message, const QString &fileName, int line, int column);
    Heap::Object *newSyntaxErrorObject(const QString &message);
    Heap::Object *newReferenceErrorObject(const QString &message);
    Heap::Object *newReferenceErrorObject(const QString &message, const QString &fileName, int lineNumber, int columnNumber);
    Heap::Object *newTypeErrorObject(const QString &message);
    Heap::Object *newRangeErrorObject(const QString &message);
    Heap::Object *newURIErrorObject(const ValueRef message);

    Heap::Object *newVariantObject(const QVariant &v);

    Heap::Object *newForEachIteratorObject(Object *o);

    Heap::Object *qmlContextObject() const;

    StackTrace stackTrace(int frameLimit = -1) const;
    StackFrame currentStackFrame() const;
    QUrl resolvedUrl(const QString &file);

    void requireArgumentsAccessors(int n);

    void markObjects();

    void initRootContext();

    InternalClass *newClass(const InternalClass &other);

    QmlExtensions *qmlExtensions();

    bool recheckCStackLimits();

    // Exception handling
    Value exceptionValue;
    StackTrace exceptionStackTrace;

    ReturnedValue throwError(const ValueRef value);
    ReturnedValue catchException(StackTrace *trace = 0);

    ReturnedValue throwError(const QString &message);
    ReturnedValue throwSyntaxError(const QString &message);
    ReturnedValue throwSyntaxError(const QString &message, const QString &fileName, int lineNumber, int column);
    ReturnedValue throwTypeError();
    ReturnedValue throwTypeError(const QString &message);
    ReturnedValue throwReferenceError(const ValueRef value);
    ReturnedValue throwReferenceError(const QString &value, const QString &fileName, int lineNumber, int column);
    ReturnedValue throwRangeError(const ValueRef value);
    ReturnedValue throwRangeError(const QString &message);
    ReturnedValue throwURIError(const ValueRef msg);
    ReturnedValue throwUnimplemented(const QString &message);

    // Use only inside catch(...) -- will re-throw if no JS exception
    QQmlError catchExceptionAsQmlError();

private:
    QmlExtensions *m_qmlExtensions;
};

struct ExecutionContextSaver
{
    ExecutionEngine *engine;
    ExecutionContext *savedContext;

    ExecutionContextSaver(ExecutionContext *context)
        : engine(context->d()->engine)
        , savedContext(context)
    {
    }
    ~ExecutionContextSaver()
    {
        engine->current = savedContext;
    }
};

inline void ExecutionEngine::pushContext(CallContext *context)
{
    Q_ASSERT(current && current->d() && context && context->d());
    context->d()->parent = current->d();
    current = context;
}

inline ExecutionContext *ExecutionEngine::popContext()
{
    Q_ASSERT(current->d()->parent);
    // ### GC
    current = reinterpret_cast<ExecutionContext *>(current->d()->parent);
    Q_ASSERT(current && current->d());
    return current;
}

inline
Heap::ExecutionContext::ExecutionContext(ExecutionEngine *engine, ContextType t)
    : Heap::Base(engine->executionContextClass)
    , type(t)
    , strictMode(false)
    , engine(engine)
    , parent(engine->currentContext()->d())
    , outer(0)
    , lookups(0)
    , compilationUnit(0)
    , lineNumber(-1)
{
    // ### GC
    engine->current = reinterpret_cast<QV4::ExecutionContext *>(this);
}


// ### Remove me
inline
void Managed::mark(QV4::ExecutionEngine *engine)
{
    Q_ASSERT(inUse());
    if (markBit())
        return;
    d()->markBit = 1;
    engine->pushForGC(d());
}


inline
void Heap::Base::mark(QV4::ExecutionEngine *engine)
{
    Q_ASSERT(inUse);
    if (markBit)
        return;
    markBit = 1;
    engine->pushForGC(this);
}



} // namespace QV4

QT_END_NAMESPACE

#endif // QV4ENGINE_H
