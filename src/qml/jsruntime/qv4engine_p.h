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
#ifndef QV4ENGINE_H
#define QV4ENGINE_H

#include "qv4global_p.h"
#include "private/qv4isel_p.h"
#include "qv4util_p.h"
#include "qv4property_p.h"
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
namespace CompiledData {
struct CompilationUnit;
}
}

namespace QV4 {

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
struct SyntaxErrorObject;
struct ArgumentsObject;
struct ExecutionContext;
struct ExecutionEngine;
class MemoryManager;
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
struct IdentifierTable;
struct InternalClass;
class MultiplyWrappedQObjectMap;
class RegExp;
class RegExpCache;
struct QmlExtensions;
struct Exception;
struct ExecutionContextSaver;

#define CHECK_STACK_LIMITS(v4) \
    if ((v4->jsStackTop <= v4->jsStackLimit) && (reinterpret_cast<quintptr>(&v4) >= v4->cStackLimit || v4->recheckCStackLimits())) {}  \
    else \
        return v4->currentContext()->throwRangeError(QStringLiteral("Maximum call stack size exceeded."))


struct Q_QML_EXPORT ExecutionEngine
{
    MemoryManager *memoryManager;
    ExecutableAllocator *executableAllocator;
    ExecutableAllocator *regExpAllocator;
    QScopedPointer<QQmlJS::EvalISelFactory> iselFactory;

private:
    friend struct ExecutionContextSaver;
    friend struct ExecutionContext;
    ExecutionContext *current;
public:
    ExecutionContext *currentContext() const { return current; }

    GlobalContext *rootContext;

    SafeValue *jsStackTop;
    SafeValue *jsStackLimit;
    quintptr cStackLimit;

    WTF::BumpPointerAllocator *bumperPointerAllocator; // Used by Yarr Regex engine.

    enum { JSStackLimit = 4*1024*1024 };
    WTF::PageAllocation *jsStack;
    SafeValue *jsStackBase;

    SafeValue *stackPush(uint nValues) {
        SafeValue *ptr = jsStackTop;
        jsStackTop = ptr + nValues;
        return ptr;
    }
    void stackPop(uint nValues) {
        jsStackTop -= nValues;
    }

    void pushForGC(Managed *m) {
        *jsStackTop = Value::fromManaged(m);
        ++jsStackTop;
    }
    Managed *popForGC() {
        --jsStackTop;
        return jsStackTop->managed();
    }


    IdentifierTable *identifierTable;

    QV4::Debugging::Debugger *debugger;

    Object *globalObject;

    Function *globalCode;

    QV8Engine *v8Engine;

    SafeValue objectCtor;
    SafeValue stringCtor;
    SafeValue numberCtor;
    SafeValue booleanCtor;
    SafeValue arrayCtor;
    SafeValue functionCtor;
    SafeValue dateCtor;
    SafeValue regExpCtor;
    SafeValue errorCtor;
    SafeValue evalErrorCtor;
    SafeValue rangeErrorCtor;
    SafeValue referenceErrorCtor;
    SafeValue syntaxErrorCtor;
    SafeValue typeErrorCtor;
    SafeValue uRIErrorCtor;
    SafeValue sequencePrototype;

    QQmlJS::MemoryPool classPool;
    InternalClass *emptyClass;
    InternalClass *executionContextClass;
    InternalClass *stringClass;

    InternalClass *objectClass;
    InternalClass *arrayClass;
    InternalClass *stringObjectClass;
    InternalClass *booleanClass;
    InternalClass *numberClass;
    InternalClass *dateClass;

    InternalClass *functionClass;
    InternalClass *functionWithProtoClass;
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

    EvalFunction *evalFunction;
    FunctionObject *thrower;

    QVector<Property> argumentsAccessors;

    SafeString id_undefined;
    SafeString id_null;
    SafeString id_true;
    SafeString id_false;
    SafeString id_boolean;
    SafeString id_number;
    SafeString id_string;
    SafeString id_object;
    SafeString id_function;
    SafeString id_length;
    SafeString id_prototype;
    SafeString id_constructor;
    SafeString id_arguments;
    SafeString id_caller;
    SafeString id_callee;
    SafeString id_this;
    SafeString id___proto__;
    SafeString id_enumerable;
    SafeString id_configurable;
    SafeString id_writable;
    SafeString id_value;
    SafeString id_get;
    SafeString id_set;
    SafeString id_eval;
    SafeString id_uintMax;
    SafeString id_name;
    SafeString id_index;
    SafeString id_input;
    SafeString id_toString;
    SafeString id_valueOf;

    QSet<CompiledData::CompilationUnit*> compilationUnits;
    QMap<quintptr, QV4::Function*> allFunctions;

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
        ScarceResourceData(const QVariant &data) : data(data) {}
        QVariant data;
        QIntrusiveListNode node;
    };
    QIntrusiveList<ScarceResourceData, &ScarceResourceData::node> scarceResources;

    // Normally the JS wrappers for QObjects are stored in the QQmlData/QObjectPrivate,
    // but any time a QObject is wrapped a second time in another engine, we have to do
    // bookkeeping.
    MultiplyWrappedQObjectMap *m_multiplyWrappedQObjects;

    ExecutionEngine(QQmlJS::EvalISelFactory *iselFactory = 0);
    ~ExecutionEngine();

    void enableDebugger();

    ExecutionContext *pushGlobalContext();
    void pushContext(CallContext *context);
    ExecutionContext *popContext();

    Returned<FunctionObject> *newBuiltinFunction(ExecutionContext *scope, const StringRef name, ReturnedValue (*code)(CallContext *));
    Returned<BoundFunction> *newBoundFunction(ExecutionContext *scope, FunctionObjectRef target, const ValueRef boundThis, const QVector<SafeValue> &boundArgs);

    Returned<Object> *newObject();
    Returned<Object> *newObject(InternalClass *internalClass);

    Returned<String> *newString(const QString &s);
    String *newIdentifier(const QString &text);

    Returned<Object> *newStringObject(const ValueRef value);
    Returned<Object> *newNumberObject(const ValueRef value);
    Returned<Object> *newBooleanObject(const ValueRef value);

    Returned<ArrayObject> *newArrayObject(int count = 0);
    Returned<ArrayObject> *newArrayObject(const QStringList &list);
    Returned<ArrayObject> *newArrayObject(InternalClass *ic);

    Returned<DateObject> *newDateObject(const ValueRef value);
    Returned<DateObject> *newDateObject(const QDateTime &dt);

    Returned<RegExpObject> *newRegExpObject(const QString &pattern, int flags);
    Returned<RegExpObject> *newRegExpObject(Referenced<RegExp> re, bool global);
    Returned<RegExpObject> *newRegExpObject(const QRegExp &re);

    Returned<Object> *newErrorObject(const ValueRef value);
    Returned<Object> *newSyntaxErrorObject(const QString &message, const QString &fileName, int line, int column);
    Returned<Object> *newSyntaxErrorObject(const QString &message);
    Returned<Object> *newReferenceErrorObject(const QString &message);
    Returned<Object> *newReferenceErrorObject(const QString &message, const QString &fileName, int lineNumber, int columnNumber);
    Returned<Object> *newTypeErrorObject(const QString &message);
    Returned<Object> *newRangeErrorObject(const QString &message);
    Returned<Object> *newURIErrorObject(const ValueRef message);

    Returned<Object> *newVariantObject(const QVariant &v);

    Returned<Object> *newForEachIteratorObject(ExecutionContext *ctx, const ObjectRef o);

    Returned<Object> *qmlContextObject() const;

    StackTrace stackTrace(int frameLimit = -1) const;
    StackFrame currentStackFrame() const;
    QUrl resolvedUrl(const QString &file);

    void requireArgumentsAccessors(int n);

    void markObjects();

    void initRootContext();

    InternalClass *newClass(const InternalClass &other);

    Function *functionForProgramCounter(quintptr pc) const;

    QmlExtensions *qmlExtensions();

    bool recheckCStackLimits();

    // Exception handling
    SafeValue exceptionValue;
    quint32 hasException;
    StackTrace exceptionStackTrace;

    ReturnedValue throwException(const ValueRef value);
    ReturnedValue catchException(ExecutionContext *catchingContext, StackTrace *trace);

    // Use only inside catch(...) -- will re-throw if no JS exception
    static QQmlError catchExceptionAsQmlError(QV4::ExecutionContext *context);

private:
    QmlExtensions *m_qmlExtensions;
};

inline
void Managed::mark(QV4::ExecutionEngine *engine)
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
