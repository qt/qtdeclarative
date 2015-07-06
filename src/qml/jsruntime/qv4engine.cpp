/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
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
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qv4engine_p.h>
#include <qv4context_p.h>
#include <qv4value_inl_p.h>
#include <qv4object_p.h>
#include <qv4objectproto_p.h>
#include <qv4objectiterator_p.h>
#include <qv4arrayobject_p.h>
#include <qv4booleanobject_p.h>
#include <qv4globalobject_p.h>
#include <qv4errorobject_p.h>
#include <qv4functionobject_p.h>
#include "qv4function_p.h"
#include <qv4mathobject_p.h>
#include <qv4numberobject_p.h>
#include <qv4regexpobject_p.h>
#include <qv4regexp_p.h>
#include <qv4variantobject_p.h>
#include <qv4runtime_p.h>
#include "qv4mm_p.h"
#include <qv4argumentsobject_p.h>
#include <qv4dateobject_p.h>
#include <qv4jsonobject_p.h>
#include <qv4stringobject_p.h>
#include <qv4identifiertable_p.h>
#include "qv4debugging_p.h"
#include "qv4profiling_p.h"
#include "qv4executableallocator_p.h"
#include "qv4sequenceobject_p.h"
#include "qv4qobjectwrapper_p.h"
#include "qv4qmlextensions_p.h"
#include "qv4memberdata_p.h"
#include "qv4arraybuffer_p.h"
#include "qv4dataview_p.h"
#include "qv4typedarray_p.h"
#include <private/qv8engine_p.h>
#include <private/qjsvalue_p.h>
#include <private/qqmlcontextwrapper_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/qqmlvaluetypewrapper_p.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qqmllistwrapper_p.h>
#include <private/qqmllist_p.h>
#include <private/qqmllocale_p.h>

#include <QtCore/QTextStream>
#include <QDateTime>

#ifdef V4_ENABLE_JIT
#include "qv4isel_masm_p.h"
#endif // V4_ENABLE_JIT

#include "qv4isel_moth_p.h"

#if USE(PTHREADS)
#  include <pthread.h>
#  include <sys/resource.h>
#if HAVE(PTHREAD_NP_H)
#  include <pthread_np.h>
#endif
#endif

#ifdef V4_USE_VALGRIND
#include <valgrind/memcheck.h>
#endif

QT_BEGIN_NAMESPACE

using namespace QV4;

static QBasicAtomicInt engineSerial = Q_BASIC_ATOMIC_INITIALIZER(1);

static ReturnedValue throwTypeError(CallContext *ctx)
{
    return ctx->engine()->throwTypeError();
}

const int MinimumStackSize = 256; // in kbytes

QT_WARNING_PUSH
QT_WARNING_DISABLE_MSVC(4172) // MSVC 2015: warning C4172: returning address of local variable or temporary: dummy

quintptr getStackLimit()
{
    quintptr stackLimit;
#if USE(PTHREADS) && !OS(QNX)
#  if OS(DARWIN)
    pthread_t thread_self = pthread_self();
    void *stackTop = pthread_get_stackaddr_np(thread_self);
    stackLimit = reinterpret_cast<quintptr>(stackTop);
    quintptr size = 0;
    if (pthread_main_np()) {
        rlimit limit;
        getrlimit(RLIMIT_STACK, &limit);
        size = limit.rlim_cur;
    } else
        size = pthread_get_stacksize_np(thread_self);
    stackLimit -= size;
#  elif defined(__hppa)
    // On some architectures the stack grows upwards. All of these are rather exotic, so simply assume
    // everything is fine there.
    // Known examples:
    // -HP PA-RISC
    stackLimit = 0;

#  else
    pthread_attr_t attr;
#if HAVE(PTHREAD_NP_H) && OS(FREEBSD)
    // on FreeBSD pthread_attr_init() must be called otherwise getting the attrs crashes
    if (pthread_attr_init(&attr) == 0 && pthread_attr_get_np(pthread_self(), &attr) == 0) {
#else
    if (pthread_getattr_np(pthread_self(), &attr) == 0) {
#endif
        void *stackBottom = Q_NULLPTR;
        size_t stackSize = 0;

        pthread_attr_getstack(&attr, &stackBottom, &stackSize);
        pthread_attr_destroy(&attr);

#        if defined(Q_OS_ANDROID)
        // Bionic pretends that the main thread has a tiny stack; work around it
        if (gettid() == getpid()) {
            rlimit limit;
            getrlimit(RLIMIT_STACK, &limit);
            stackBottom = reinterpret_cast<void*>(reinterpret_cast<quintptr>(stackBottom) + stackSize - limit.rlim_cur);
        }
#       endif

        stackLimit = reinterpret_cast<quintptr>(stackBottom);
    } else {
        int dummy;
        // this is inexact, as part of the stack is used when being called here,
        // but let's simply default to 1MB from where the stack is right now
        stackLimit = reinterpret_cast<qintptr>(&dummy) - 1024*1024;
    }

#  endif
// This is wrong. StackLimit is the currently committed stack size, not the real end.
// only way to get that limit is apparently by using VirtualQuery (Yuck)
//#elif OS(WINDOWS)
//    PNT_TIB tib = (PNT_TIB)NtCurrentTeb();
//    stackLimit = static_cast<quintptr>(tib->StackLimit);
#else
    int dummy;
    // this is inexact, as part of the stack is used when being called here,
    // but let's simply default to 1MB from where the stack is right now
    // (Note: triggers warning C4172 as of MSVC 2015, returning address of local variable)
    stackLimit = reinterpret_cast<qintptr>(&dummy) - 1024*1024;
#endif

    // 256k slack
    return stackLimit + MinimumStackSize*1024;
}

QT_WARNING_POP

QJSEngine *ExecutionEngine::jsEngine() const
{
    return v8Engine->publicEngine();
}

QQmlEngine *ExecutionEngine::qmlEngine() const
{
    return v8Engine->engine();
}

ExecutionEngine::ExecutionEngine(EvalISelFactory *factory)
    : current(0)
    , memoryManager(new QV4::MemoryManager(this))
    , executableAllocator(new QV4::ExecutableAllocator)
    , regExpAllocator(new QV4::ExecutableAllocator)
    , bumperPointerAllocator(new WTF::BumpPointerAllocator)
    , jsStack(new WTF::PageAllocation)
    , debugger(0)
    , profiler(0)
    , globalCode(0)
    , v8Engine(0)
    , argumentsAccessors(0)
    , nArgumentsAccessors(0)
    , m_engineId(engineSerial.fetchAndAddOrdered(1))
    , regExpCache(0)
    , m_multiplyWrappedQObjects(0)
    , m_qmlExtensions(0)
{
    MemoryManager::GCBlocker gcBlocker(memoryManager);

    exceptionValue = Encode::undefined();
    hasException = false;

    if (!factory) {

#ifdef V4_ENABLE_JIT
        static const bool forceMoth = !qgetenv("QV4_FORCE_INTERPRETER").isEmpty();
        if (forceMoth)
            factory = new Moth::ISelFactory;
        else
            factory = new JIT::ISelFactory;
#else // !V4_ENABLE_JIT
        factory = new Moth::ISelFactory;
#endif // V4_ENABLE_JIT
    }
    iselFactory.reset(factory);

    // reserve space for the JS stack
    // we allow it to grow to 2 times JSStackLimit, as we can overshoot due to garbage collection
    // and ScopedValues allocated outside of JIT'ed methods.
    *jsStack = WTF::PageAllocation::allocate(2 * JSStackLimit, WTF::OSAllocator::JSVMStackPages,
                                             /* writable */ true, /* executable */ false,
                                             /* includesGuardPages */ true);
    jsStackBase = (Value *)jsStack->base();
    jsStackTop = jsStackBase;

#ifdef V4_USE_VALGRIND
    VALGRIND_MAKE_MEM_UNDEFINED(jsStackBase, 2*JSStackLimit);
#endif

    // set up stack limits
    jsStackLimit = jsStackBase + JSStackLimit/sizeof(Value);
    cStackLimit = getStackLimit();
    if (!recheckCStackLimits())
        qFatal("Fatal: Not enough stack space available for QML. Please increase the process stack size to more than %d KBytes.", MinimumStackSize);

    Scope scope(this);

    identifierTable = new IdentifierTable(this);

    classPool = new InternalClassPool;

    emptyClass =  new (classPool) InternalClass(this);

    id_empty = newIdentifier(QString());
    id_undefined = newIdentifier(QStringLiteral("undefined"));
    id_null = newIdentifier(QStringLiteral("null"));
    id_true = newIdentifier(QStringLiteral("true"));
    id_false = newIdentifier(QStringLiteral("false"));
    id_boolean = newIdentifier(QStringLiteral("boolean"));
    id_number = newIdentifier(QStringLiteral("number"));
    id_string = newIdentifier(QStringLiteral("string"));
    id_object = newIdentifier(QStringLiteral("object"));
    id_function = newIdentifier(QStringLiteral("function"));
    id_length = newIdentifier(QStringLiteral("length"));
    id_prototype = newIdentifier(QStringLiteral("prototype"));
    id_constructor = newIdentifier(QStringLiteral("constructor"));
    id_arguments = newIdentifier(QStringLiteral("arguments"));
    id_caller = newIdentifier(QStringLiteral("caller"));
    id_callee = newIdentifier(QStringLiteral("callee"));
    id_this = newIdentifier(QStringLiteral("this"));
    id___proto__ = newIdentifier(QStringLiteral("__proto__"));
    id_enumerable = newIdentifier(QStringLiteral("enumerable"));
    id_configurable = newIdentifier(QStringLiteral("configurable"));
    id_writable = newIdentifier(QStringLiteral("writable"));
    id_value = newIdentifier(QStringLiteral("value"));
    id_get = newIdentifier(QStringLiteral("get"));
    id_set = newIdentifier(QStringLiteral("set"));
    id_eval = newIdentifier(QStringLiteral("eval"));
    id_uintMax = newIdentifier(QStringLiteral("4294967295"));
    id_name = newIdentifier(QStringLiteral("name"));
    id_index = newIdentifier(QStringLiteral("index"));
    id_input = newIdentifier(QStringLiteral("input"));
    id_toString = newIdentifier(QStringLiteral("toString"));
    id_destroy = newIdentifier(QStringLiteral("destroy"));
    id_valueOf = newIdentifier(QStringLiteral("valueOf"));
    id_byteLength = newIdentifier(QStringLiteral("byteLength"));
    id_byteOffset = newIdentifier(QStringLiteral("byteOffset"));
    id_buffer = newIdentifier(QStringLiteral("buffer"));
    id_lastIndex = newIdentifier(QStringLiteral("lastIndex"));

    objectPrototype = memoryManager->alloc<ObjectPrototype>(emptyClass, (QV4::Object *)0);

    arrayClass = emptyClass->addMember(id_length, Attr_NotConfigurable|Attr_NotEnumerable);
    arrayPrototype = memoryManager->alloc<ArrayPrototype>(arrayClass, objectPrototype.asObject());

    InternalClass *argsClass = emptyClass->addMember(id_length, Attr_NotEnumerable);
    argumentsObjectClass = argsClass->addMember(id_callee, Attr_Data|Attr_NotEnumerable);
    strictArgumentsObjectClass = argsClass->addMember(id_callee, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
    strictArgumentsObjectClass = strictArgumentsObjectClass->addMember(id_caller, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);

    m_globalObject = newObject();
    Q_ASSERT(globalObject()->d()->vtable);
    initRootContext();

    stringPrototype = memoryManager->alloc<StringPrototype>(emptyClass, objectPrototype.asObject());
    numberPrototype = memoryManager->alloc<NumberPrototype>(emptyClass, objectPrototype.asObject());
    booleanPrototype = memoryManager->alloc<BooleanPrototype>(emptyClass, objectPrototype.asObject());
    datePrototype = memoryManager->alloc<DatePrototype>(emptyClass, objectPrototype.asObject());

    uint index;
    InternalClass *functionProtoClass = emptyClass->addMember(id_prototype, Attr_NotEnumerable, &index);
    Q_ASSERT(index == Heap::FunctionObject::Index_Prototype);
    functionPrototype = memoryManager->alloc<FunctionPrototype>(functionProtoClass, objectPrototype.asObject());
    functionClass = emptyClass->addMember(id_prototype, Attr_NotEnumerable|Attr_NotConfigurable, &index);
    Q_ASSERT(index == Heap::FunctionObject::Index_Prototype);
    simpleScriptFunctionClass = functionClass->addMember(id_name, Attr_ReadOnly, &index);
    Q_ASSERT(index == Heap::SimpleScriptFunction::Index_Name);
    simpleScriptFunctionClass = simpleScriptFunctionClass->addMember(id_length, Attr_ReadOnly, &index);
    Q_ASSERT(index == Heap::SimpleScriptFunction::Index_Length);
    protoClass = emptyClass->addMember(id_constructor, Attr_NotEnumerable, &index);
    Q_ASSERT(index == Heap::FunctionObject::Index_ProtoConstructor);

    regExpPrototype = memoryManager->alloc<RegExpPrototype>(this);
    regExpExecArrayClass = arrayClass->addMember(id_index, Attr_Data, &index);
    Q_ASSERT(index == RegExpObject::Index_ArrayIndex);
    regExpExecArrayClass = regExpExecArrayClass->addMember(id_input, Attr_Data, &index);
    Q_ASSERT(index == RegExpObject::Index_ArrayInput);

    errorPrototype = memoryManager->alloc<ErrorPrototype>(emptyClass, objectPrototype.asObject());
    evalErrorPrototype = memoryManager->alloc<EvalErrorPrototype>(emptyClass, errorPrototype.asObject());
    rangeErrorPrototype = memoryManager->alloc<RangeErrorPrototype>(emptyClass, errorPrototype.asObject());
    referenceErrorPrototype = memoryManager->alloc<ReferenceErrorPrototype>(emptyClass, errorPrototype.asObject());
    syntaxErrorPrototype = memoryManager->alloc<SyntaxErrorPrototype>(emptyClass, errorPrototype.asObject());
    typeErrorPrototype = memoryManager->alloc<TypeErrorPrototype>(emptyClass, errorPrototype.asObject());
    uRIErrorPrototype = memoryManager->alloc<URIErrorPrototype>(emptyClass, errorPrototype.asObject());

    variantPrototype = memoryManager->alloc<VariantPrototype>(emptyClass, objectPrototype.asObject());
    Q_ASSERT(variantPrototype.asObject()->prototype() == objectPrototype.asObject()->d());

    sequencePrototype = ScopedValue(scope, memoryManager->alloc<SequencePrototype>(arrayClass, arrayPrototype.asObject()));

    ScopedContext global(scope, rootContext());
    objectCtor = memoryManager->alloc<ObjectCtor>(global);
    stringCtor = memoryManager->alloc<StringCtor>(global);
    numberCtor = memoryManager->alloc<NumberCtor>(global);
    booleanCtor = memoryManager->alloc<BooleanCtor>(global);
    arrayCtor = memoryManager->alloc<ArrayCtor>(global);
    functionCtor = memoryManager->alloc<FunctionCtor>(global);
    dateCtor = memoryManager->alloc<DateCtor>(global);
    regExpCtor = memoryManager->alloc<RegExpCtor>(global);
    errorCtor = memoryManager->alloc<ErrorCtor>(global);
    evalErrorCtor = memoryManager->alloc<EvalErrorCtor>(global);
    rangeErrorCtor = memoryManager->alloc<RangeErrorCtor>(global);
    referenceErrorCtor = memoryManager->alloc<ReferenceErrorCtor>(global);
    syntaxErrorCtor = memoryManager->alloc<SyntaxErrorCtor>(global);
    typeErrorCtor = memoryManager->alloc<TypeErrorCtor>(global);
    uRIErrorCtor = memoryManager->alloc<URIErrorCtor>(global);

    static_cast<ObjectPrototype *>(objectPrototype.asObject())->init(this, objectCtor.asObject());
    static_cast<StringPrototype *>(stringPrototype.asObject())->init(this, stringCtor.asObject());
    static_cast<NumberPrototype *>(numberPrototype.asObject())->init(this, numberCtor.asObject());
    static_cast<BooleanPrototype *>(booleanPrototype.asObject())->init(this, booleanCtor.asObject());
    static_cast<ArrayPrototype *>(arrayPrototype.asObject())->init(this, arrayCtor.asObject());
    static_cast<DatePrototype *>(datePrototype.asObject())->init(this, dateCtor.asObject());
    static_cast<FunctionPrototype *>(functionPrototype.asObject())->init(this, functionCtor.asObject());
    static_cast<RegExpPrototype *>(regExpPrototype.asObject())->init(this, regExpCtor.asObject());
    static_cast<ErrorPrototype *>(errorPrototype.asObject())->init(this, errorCtor.asObject());
    static_cast<EvalErrorPrototype *>(evalErrorPrototype.asObject())->init(this, evalErrorCtor.asObject());
    static_cast<RangeErrorPrototype *>(rangeErrorPrototype.asObject())->init(this, rangeErrorCtor.asObject());
    static_cast<ReferenceErrorPrototype *>(referenceErrorPrototype.asObject())->init(this, referenceErrorCtor.asObject());
    static_cast<SyntaxErrorPrototype *>(syntaxErrorPrototype.asObject())->init(this, syntaxErrorCtor.asObject());
    static_cast<TypeErrorPrototype *>(typeErrorPrototype.asObject())->init(this, typeErrorCtor.asObject());
    static_cast<URIErrorPrototype *>(uRIErrorPrototype.asObject())->init(this, uRIErrorCtor.asObject());

    static_cast<VariantPrototype *>(variantPrototype.asObject())->init();
    sequencePrototype.cast<SequencePrototype>()->init();


    // typed arrays

    arrayBufferCtor = memoryManager->alloc<ArrayBufferCtor>(global);
    arrayBufferPrototype = memoryManager->alloc<ArrayBufferPrototype>(emptyClass, objectPrototype.asObject());
    static_cast<ArrayBufferPrototype *>(arrayBufferPrototype.asObject())->init(this, arrayBufferCtor.asObject());

    dataViewCtor = memoryManager->alloc<DataViewCtor>(global);
    dataViewPrototype = memoryManager->alloc<DataViewPrototype>(emptyClass, objectPrototype.asObject());
    static_cast<DataViewPrototype *>(dataViewPrototype.asObject())->init(this, dataViewCtor.asObject());

    for (int i = 0; i < Heap::TypedArray::NTypes; ++i) {
        typedArrayCtors[i] = memoryManager->alloc<TypedArrayCtor>(global, Heap::TypedArray::Type(i));
        typedArrayPrototype[i] = memoryManager->alloc<TypedArrayPrototype>(this, Heap::TypedArray::Type(i));
        typedArrayPrototype[i].as<TypedArrayPrototype>()->init(this, static_cast<TypedArrayCtor *>(typedArrayCtors[i].asObject()));
    }

    //
    // set up the global object
    //
    rootContext()->global = globalObject()->d();
    rootContext()->callData->thisObject = globalObject();
    Q_ASSERT(globalObject()->d()->vtable);

    globalObject()->defineDefaultProperty(QStringLiteral("Object"), objectCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("String"), stringCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("Number"), numberCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("Boolean"), booleanCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("Array"), arrayCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("Function"), functionCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("Date"), dateCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("RegExp"), regExpCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("Error"), errorCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("EvalError"), evalErrorCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("RangeError"), rangeErrorCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("ReferenceError"), referenceErrorCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("SyntaxError"), syntaxErrorCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("TypeError"), typeErrorCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("URIError"), uRIErrorCtor);

    globalObject()->defineDefaultProperty(QStringLiteral("ArrayBuffer"), arrayBufferCtor);
    globalObject()->defineDefaultProperty(QStringLiteral("DataView"), dataViewCtor);
    ScopedString str(scope);
    for (int i = 0; i < Heap::TypedArray::NTypes; ++i)
        globalObject()->defineDefaultProperty((str = typedArrayCtors[i].asFunctionObject()->name())->toQString(), typedArrayCtors[i]);
    ScopedObject o(scope);
    globalObject()->defineDefaultProperty(QStringLiteral("Math"), (o = memoryManager->alloc<MathObject>(this)));
    globalObject()->defineDefaultProperty(QStringLiteral("JSON"), (o = memoryManager->alloc<JsonObject>(this)));

    globalObject()->defineReadonlyProperty(QStringLiteral("undefined"), Primitive::undefinedValue());
    globalObject()->defineReadonlyProperty(QStringLiteral("NaN"), Primitive::fromDouble(std::numeric_limits<double>::quiet_NaN()));
    globalObject()->defineReadonlyProperty(QStringLiteral("Infinity"), Primitive::fromDouble(Q_INFINITY));


    evalFunction = memoryManager->alloc<EvalFunction>(global);
    globalObject()->defineDefaultProperty(QStringLiteral("eval"), (o = evalFunction));

    globalObject()->defineDefaultProperty(QStringLiteral("parseInt"), GlobalFunctions::method_parseInt, 2);
    globalObject()->defineDefaultProperty(QStringLiteral("parseFloat"), GlobalFunctions::method_parseFloat, 1);
    globalObject()->defineDefaultProperty(QStringLiteral("isNaN"), GlobalFunctions::method_isNaN, 1);
    globalObject()->defineDefaultProperty(QStringLiteral("isFinite"), GlobalFunctions::method_isFinite, 1);
    globalObject()->defineDefaultProperty(QStringLiteral("decodeURI"), GlobalFunctions::method_decodeURI, 1);
    globalObject()->defineDefaultProperty(QStringLiteral("decodeURIComponent"), GlobalFunctions::method_decodeURIComponent, 1);
    globalObject()->defineDefaultProperty(QStringLiteral("encodeURI"), GlobalFunctions::method_encodeURI, 1);
    globalObject()->defineDefaultProperty(QStringLiteral("encodeURIComponent"), GlobalFunctions::method_encodeURIComponent, 1);
    globalObject()->defineDefaultProperty(QStringLiteral("escape"), GlobalFunctions::method_escape, 1);
    globalObject()->defineDefaultProperty(QStringLiteral("unescape"), GlobalFunctions::method_unescape, 1);

    ScopedString name(scope, newString(QStringLiteral("thrower")));
    thrower = BuiltinFunction::create(global, name, ::throwTypeError);
}

ExecutionEngine::~ExecutionEngine()
{
    delete debugger;
    debugger = 0;
    delete profiler;
    profiler = 0;
    delete m_multiplyWrappedQObjects;
    m_multiplyWrappedQObjects = 0;
    delete identifierTable;
    delete memoryManager;

    QSet<QV4::CompiledData::CompilationUnit*> remainingUnits;
    qSwap(compilationUnits, remainingUnits);
    foreach (QV4::CompiledData::CompilationUnit *unit, remainingUnits)
        unit->unlink();

    delete m_qmlExtensions;
    emptyClass->destroy();
    delete classPool;
    delete bumperPointerAllocator;
    delete regExpCache;
    delete regExpAllocator;
    delete executableAllocator;
    jsStack->deallocate();
    delete jsStack;
    delete [] argumentsAccessors;
}

void ExecutionEngine::enableDebugger()
{
    Q_ASSERT(!debugger);
    debugger = new Debugging::Debugger(this);
    iselFactory.reset(new Moth::ISelFactory);
}

void ExecutionEngine::enableProfiler()
{
    Q_ASSERT(!profiler);
    profiler = new QV4::Profiling::Profiler(this);
}

void ExecutionEngine::initRootContext()
{
    Scope scope(this);
    Scoped<GlobalContext> r(scope, memoryManager->allocManaged<GlobalContext>(sizeof(GlobalContext::Data) + sizeof(CallData)));
    new (r->d()) GlobalContext::Data(this);
    r->d()->callData = reinterpret_cast<CallData *>(r->d() + 1);
    r->d()->callData->tag = QV4::Value::_Integer_Type;
    r->d()->callData->argc = 0;
    r->d()->callData->thisObject = globalObject();
    r->d()->callData->args[0] = Encode::undefined();

    m_rootContext = r->d();
}

InternalClass *ExecutionEngine::newClass(const InternalClass &other)
{
    return new (classPool) InternalClass(other);
}

Heap::ExecutionContext *ExecutionEngine::pushGlobalContext()
{
    Scope scope(this);
    Scoped<GlobalContext> g(scope, memoryManager->alloc<GlobalContext>(this));
    g->d()->callData = rootContext()->callData;

    Q_ASSERT(currentContext() == g->d());
    return g->d();
}


Heap::Object *ExecutionEngine::newObject()
{
    Scope scope(this);
    ScopedObject object(scope, memoryManager->alloc<Object>(this));
    return object->d();
}

Heap::Object *ExecutionEngine::newObject(InternalClass *internalClass, QV4::Object *prototype)
{
    Scope scope(this);
    ScopedObject object(scope, memoryManager->alloc<Object>(internalClass, prototype));
    return object->d();
}

Heap::String *ExecutionEngine::newString(const QString &s)
{
    Scope scope(this);
    return ScopedString(scope, memoryManager->allocWithStringData<String>(s.length() * sizeof(QChar), s))->d();
}

Heap::String *ExecutionEngine::newIdentifier(const QString &text)
{
    return identifierTable->insertString(text);
}

Heap::Object *ExecutionEngine::newStringObject(const Value &value)
{
    Scope scope(this);
    Scoped<StringObject> object(scope, memoryManager->alloc<StringObject>(this, value));
    return object->d();
}

Heap::Object *ExecutionEngine::newNumberObject(double value)
{
    Scope scope(this);
    Scoped<NumberObject> object(scope, memoryManager->alloc<NumberObject>(this, value));
    return object->d();
}

Heap::Object *ExecutionEngine::newBooleanObject(bool b)
{
    Scope scope(this);
    ScopedObject object(scope, memoryManager->alloc<BooleanObject>(this, b));
    return object->d();
}

Heap::ArrayObject *ExecutionEngine::newArrayObject(int count)
{
    Scope scope(this);
    ScopedArrayObject object(scope, memoryManager->alloc<ArrayObject>(this));

    if (count) {
        if (count < 0x1000)
            object->arrayReserve(count);
        object->setArrayLengthUnchecked(count);
    }
    return object->d();
}

Heap::ArrayObject *ExecutionEngine::newArrayObject(const QStringList &list)
{
    Scope scope(this);
    ScopedArrayObject object(scope, memoryManager->alloc<ArrayObject>(this, list));
    return object->d();
}

Heap::ArrayObject *ExecutionEngine::newArrayObject(InternalClass *ic, Object *prototype)
{
    Scope scope(this);
    ScopedArrayObject object(scope, memoryManager->alloc<ArrayObject>(ic, prototype));
    return object->d();
}

Heap::ArrayBuffer *ExecutionEngine::newArrayBuffer(const QByteArray &array)
{
    Scope scope(this);
    Scoped<ArrayBuffer> object(scope, memoryManager->alloc<ArrayBuffer>(this, array));
    return object->d();
}


Heap::DateObject *ExecutionEngine::newDateObject(const Value &value)
{
    Scope scope(this);
    Scoped<DateObject> object(scope, memoryManager->alloc<DateObject>(this, value));
    return object->d();
}

Heap::DateObject *ExecutionEngine::newDateObject(const QDateTime &dt)
{
    Scope scope(this);
    Scoped<DateObject> object(scope, memoryManager->alloc<DateObject>(this, dt));
    return object->d();
}

Heap::RegExpObject *ExecutionEngine::newRegExpObject(const QString &pattern, int flags)
{
    bool global = (flags & IR::RegExp::RegExp_Global);
    bool ignoreCase = false;
    bool multiline = false;
    if (flags & IR::RegExp::RegExp_IgnoreCase)
        ignoreCase = true;
    if (flags & IR::RegExp::RegExp_Multiline)
        multiline = true;

    Scope scope(this);
    Scoped<RegExp> re(scope, RegExp::create(this, pattern, ignoreCase, multiline));
    return newRegExpObject(re, global);
}

Heap::RegExpObject *ExecutionEngine::newRegExpObject(RegExp *re, bool global)
{
    Scope scope(this);
    Scoped<RegExpObject> object(scope, memoryManager->alloc<RegExpObject>(this, re, global));
    return object->d();
}

Heap::RegExpObject *ExecutionEngine::newRegExpObject(const QRegExp &re)
{
    Scope scope(this);
    Scoped<RegExpObject> object(scope, memoryManager->alloc<RegExpObject>(this, re));
    return object->d();
}

Heap::Object *ExecutionEngine::newErrorObject(const Value &value)
{
    Scope scope(this);
    ScopedObject object(scope, memoryManager->alloc<ErrorObject>(emptyClass, errorPrototype.asObject(), value));
    return object->d();
}

Heap::Object *ExecutionEngine::newSyntaxErrorObject(const QString &message)
{
    Scope scope(this);
    ScopedString s(scope, newString(message));
    ScopedObject error(scope, memoryManager->alloc<SyntaxErrorObject>(this, s));
    return error->d();
}

Heap::Object *ExecutionEngine::newSyntaxErrorObject(const QString &message, const QString &fileName, int line, int column)
{
    Scope scope(this);
    ScopedObject error(scope, memoryManager->alloc<SyntaxErrorObject>(this, message, fileName, line, column));
    return error->d();
}


Heap::Object *ExecutionEngine::newReferenceErrorObject(const QString &message)
{
    Scope scope(this);
    ScopedObject o(scope, memoryManager->alloc<ReferenceErrorObject>(this, message));
    return o->d();
}

Heap::Object *ExecutionEngine::newReferenceErrorObject(const QString &message, const QString &fileName, int lineNumber, int columnNumber)
{
    Scope scope(this);
    ScopedObject o(scope, memoryManager->alloc<ReferenceErrorObject>(this, message, fileName, lineNumber, columnNumber));
    return o->d();
}


Heap::Object *ExecutionEngine::newTypeErrorObject(const QString &message)
{
    Scope scope(this);
    ScopedObject o(scope, memoryManager->alloc<TypeErrorObject>(this, message));
    return o->d();
}

Heap::Object *ExecutionEngine::newRangeErrorObject(const QString &message)
{
    Scope scope(this);
    ScopedObject o(scope, memoryManager->alloc<RangeErrorObject>(this, message));
    return o->d();
}

Heap::Object *ExecutionEngine::newURIErrorObject(const Value &message)
{
    Scope scope(this);
    ScopedObject o(scope, memoryManager->alloc<URIErrorObject>(this, message));
    return o->d();
}

Heap::Object *ExecutionEngine::newVariantObject(const QVariant &v)
{
    Scope scope(this);
    ScopedObject o(scope, memoryManager->alloc<VariantObject>(this, v));
    return o->d();
}

Heap::Object *ExecutionEngine::newForEachIteratorObject(Object *o)
{
    Scope scope(this);
    ScopedObject obj(scope, memoryManager->alloc<ForEachIteratorObject>(this, o));
    return obj->d();
}

Heap::Object *ExecutionEngine::qmlContextObject() const
{
    Heap::ExecutionContext *ctx = currentContext();

    if (ctx->type == Heap::ExecutionContext::Type_SimpleCallContext && !ctx->outer)
        ctx = ctx->parent;

    if (!ctx->outer)
        return 0;

    while (ctx->outer && ctx->outer->type != Heap::ExecutionContext::Type_GlobalContext)
        ctx = ctx->outer;

    Q_ASSERT(ctx);
    if (ctx->type != Heap::ExecutionContext::Type_QmlContext)
        return 0;

    Q_ASSERT(static_cast<Heap::CallContext *>(ctx)->activation);
    return static_cast<Heap::CallContext *>(ctx)->activation;
}

QVector<StackFrame> ExecutionEngine::stackTrace(int frameLimit) const
{
    Scope scope(const_cast<ExecutionEngine *>(this));
    ScopedString name(scope);
    QVector<StackFrame> stack;

    ScopedContext c(scope, currentContext());
    ScopedFunctionObject function(scope);
    while (c && frameLimit) {
        function = c->getFunctionObject();
        if (function) {
            StackFrame frame;
            if (const Function *f = function->function())
                frame.source = f->sourceFile();
            name = function->name();
            frame.function = name->toQString();
            frame.line = -1;
            frame.column = -1;

            if (function->function())
                // line numbers can be negative for places where you can't set a real breakpoint
                frame.line = qAbs(c->d()->lineNumber);

            stack.append(frame);
            --frameLimit;
        }
        c = c->d()->parent;
    }

    if (frameLimit && globalCode) {
        StackFrame frame;
        frame.source = globalCode->sourceFile();
        frame.function = globalCode->name()->toQString();
        frame.line = rootContext()->lineNumber;
        frame.column = -1;

        stack.append(frame);
    }
    return stack;
}

StackFrame ExecutionEngine::currentStackFrame() const
{
    StackFrame frame;
    frame.line = -1;
    frame.column = -1;

    QVector<StackFrame> trace = stackTrace(/*limit*/ 1);
    if (!trace.isEmpty())
        frame = trace.first();

    return frame;
}

/* Helper and "C" linkage exported function to format a GDBMI stacktrace for
 * invocation by a debugger.
 * Sample GDB invocation: print qt_v4StackTrace((void*)0x7fffffffb290)
 * Sample CDB invocation: .call Qt5Qmld!qt_v4StackTrace(0x7fffffffb290) ; gh
 * Note: The helper is there to suppress MSVC warning 4190 about anything
 * with UDT return types in a "C" linkage function. */

static inline char *v4StackTrace(const ExecutionContext *context)
{
    QString result;
    QTextStream str(&result);
    str << "stack=[";
    if (context && context->d()->engine) {
        const QVector<StackFrame> stackTrace = context->d()->engine->stackTrace(20);
        for (int i = 0; i < stackTrace.size(); ++i) {
            if (i)
                str << ',';
            const QUrl url(stackTrace.at(i).source);
            const QString fileName = url.isLocalFile() ? url.toLocalFile() : url.toString();
            str << "frame={level=\"" << i << "\",func=\"" << stackTrace.at(i).function
                << "\",file=\"" << fileName << "\",fullname=\"" << fileName
                << "\",line=\"" << stackTrace.at(i).line << "\",language=\"js\"}";
        }
    }
    str << ']';
    return qstrdup(result.toLocal8Bit().constData());
}

extern "C" Q_QML_EXPORT char *qt_v4StackTrace(void *executionContext)
{
    return v4StackTrace(reinterpret_cast<const ExecutionContext *>(executionContext));
}

QUrl ExecutionEngine::resolvedUrl(const QString &file)
{
    QUrl src(file);
    if (!src.isRelative())
        return src;

    QUrl base;
    Scope scope(this);
    ScopedContext c(scope, currentContext());
    while (c) {
        CallContext *callCtx = c->asCallContext();
        if (callCtx && callCtx->d()->function) {
            if (callCtx->d()->function->function)
                base.setUrl(callCtx->d()->function->function->sourceFile());
            break;
        }
        c = c->d()->parent;
    }

    if (base.isEmpty() && globalCode)
        base.setUrl(globalCode->sourceFile());

    if (base.isEmpty())
        return src;

    return base.resolved(src);
}

void ExecutionEngine::requireArgumentsAccessors(int n)
{
    if (n <= nArgumentsAccessors)
        return;

    Scope scope(this);
    ScopedFunctionObject get(scope);
    ScopedFunctionObject set(scope);

    if (n >= nArgumentsAccessors) {
        Property *oldAccessors = argumentsAccessors;
        int oldSize = nArgumentsAccessors;
        nArgumentsAccessors = qMax(8, n);
        argumentsAccessors = new Property[nArgumentsAccessors];
        if (oldAccessors) {
            memcpy(argumentsAccessors, oldAccessors, oldSize*sizeof(Property));
            delete [] oldAccessors;
        }
        ScopedContext global(scope, scope.engine->rootContext());
        for (int i = oldSize; i < nArgumentsAccessors; ++i) {
            argumentsAccessors[i].value = ScopedValue(scope, memoryManager->alloc<ArgumentsGetterFunction>(global, i));
            argumentsAccessors[i].set = ScopedValue(scope, memoryManager->alloc<ArgumentsSetterFunction>(global, i));
        }
    }
}

void ExecutionEngine::markObjects()
{
    identifierTable->mark(this);

    globalObject()->mark(this);

    for (int i = 0; i < nArgumentsAccessors; ++i) {
        const Property &pd = argumentsAccessors[i];
        if (Heap::FunctionObject *getter = pd.getter())
            getter->mark(this);
        if (Heap::FunctionObject *setter = pd.setter())
            setter->mark(this);
    }

    Heap::ExecutionContext *c = currentContext();
    while (c) {
        Q_ASSERT(c->inUse());
        if (!c->isMarked()) {
            c->setMarkBit();
            c->gcGetVtable()->markObjects(c, this);
        }
        c = c->parent;
    }

    id_empty->mark(this);
    id_undefined->mark(this);
    id_null->mark(this);
    id_true->mark(this);
    id_false->mark(this);
    id_boolean->mark(this);
    id_number->mark(this);
    id_string->mark(this);
    id_object->mark(this);
    id_function->mark(this);
    id_length->mark(this);
    id_prototype->mark(this);
    id_constructor->mark(this);
    id_arguments->mark(this);
    id_caller->mark(this);
    id_callee->mark(this);
    id_this->mark(this);
    id___proto__->mark(this);
    id_enumerable->mark(this);
    id_configurable->mark(this);
    id_writable->mark(this);
    id_value->mark(this);
    id_get->mark(this);
    id_set->mark(this);
    id_eval->mark(this);
    id_uintMax->mark(this);
    id_name->mark(this);
    id_index->mark(this);
    id_input->mark(this);
    id_toString->mark(this);
    id_destroy->mark(this);
    id_valueOf->mark(this);
    id_byteLength->mark(this);
    id_byteOffset->mark(this);
    id_buffer->mark(this);
    id_lastIndex->mark(this);

    objectCtor.mark(this);
    stringCtor.mark(this);
    numberCtor.mark(this);
    booleanCtor.mark(this);
    arrayCtor.mark(this);
    functionCtor.mark(this);
    dateCtor.mark(this);
    regExpCtor.mark(this);
    errorCtor.mark(this);
    evalErrorCtor.mark(this);
    rangeErrorCtor.mark(this);
    referenceErrorCtor.mark(this);
    syntaxErrorCtor.mark(this);
    typeErrorCtor.mark(this);
    uRIErrorCtor.mark(this);
    arrayBufferCtor.mark(this);
    dataViewCtor.mark(this);
    for (int i = 0; i < Heap::TypedArray::NTypes; ++i)
        typedArrayCtors[i].mark(this);

    objectPrototype.mark(this);
    arrayPrototype.mark(this);
    stringPrototype.mark(this);
    numberPrototype.mark(this);
    booleanPrototype.mark(this);
    datePrototype.mark(this);
    functionPrototype.mark(this);
    regExpPrototype.mark(this);
    errorPrototype.mark(this);
    evalErrorPrototype.mark(this);
    rangeErrorPrototype.mark(this);
    referenceErrorPrototype.mark(this);
    syntaxErrorPrototype.mark(this);
    typeErrorPrototype.mark(this);
    uRIErrorPrototype.mark(this);
    variantPrototype.mark(this);
    sequencePrototype.mark(this);

    arrayBufferPrototype.mark(this);
    dataViewPrototype.mark(this);
    for (int i = 0; i < Heap::TypedArray::NTypes; ++i)
        typedArrayPrototype[i].mark(this);

    exceptionValue.mark(this);

    thrower->mark(this);

    if (m_qmlExtensions)
        m_qmlExtensions->markObjects(this);

    classPool->markObjects(this);

    for (QSet<CompiledData::CompilationUnit*>::ConstIterator it = compilationUnits.constBegin(), end = compilationUnits.constEnd();
         it != end; ++it)
        (*it)->markObjects(this);
}

QmlExtensions *ExecutionEngine::qmlExtensions()
{
    if (!m_qmlExtensions)
        m_qmlExtensions = new QmlExtensions;
    return m_qmlExtensions;
}

ReturnedValue ExecutionEngine::throwError(const Value &value)
{
    // we can get in here with an exception already set, as the runtime
    // doesn't check after every operation that can throw.
    // in this case preserve the first exception to give correct error
    // information
    if (hasException)
        return Encode::undefined();

    hasException = true;
    exceptionValue = value;
    QV4::Scope scope(this);
    QV4::Scoped<ErrorObject> error(scope, value);
    if (!!error)
        exceptionStackTrace = error->d()->stackTrace;
    else
        exceptionStackTrace = stackTrace();

    if (debugger)
        debugger->aboutToThrow();

    return Encode::undefined();
}

ReturnedValue ExecutionEngine::catchException(StackTrace *trace)
{
    Q_ASSERT(hasException);
    if (trace)
        *trace = exceptionStackTrace;
    exceptionStackTrace.clear();
    hasException = false;
    ReturnedValue res = exceptionValue.asReturnedValue();
    exceptionValue = Primitive::emptyValue();
    return res;
}

ReturnedValue ExecutionEngine::throwError(const QString &message)
{
    Scope scope(this);
    ScopedValue v(scope, newString(message));
    v = newErrorObject(v);
    return throwError(v);
}

ReturnedValue ExecutionEngine::throwSyntaxError(const QString &message, const QString &fileName, int line, int column)
{
    Scope scope(this);
    ScopedObject error(scope, newSyntaxErrorObject(message, fileName, line, column));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwSyntaxError(const QString &message)
{
    Scope scope(this);
    ScopedObject error(scope, newSyntaxErrorObject(message));
    return throwError(error);
}


ReturnedValue ExecutionEngine::throwTypeError()
{
    Scope scope(this);
    ScopedObject error(scope, newTypeErrorObject(QStringLiteral("Type error")));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwTypeError(const QString &message)
{
    Scope scope(this);
    ScopedObject error(scope, newTypeErrorObject(message));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwReferenceError(const Value &value)
{
    Scope scope(this);
    ScopedString s(scope, value.toString(this));
    QString msg = s->toQString() + QStringLiteral(" is not defined");
    ScopedObject error(scope, newReferenceErrorObject(msg));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwReferenceError(const QString &message, const QString &fileName, int line, int column)
{
    Scope scope(this);
    QString msg = message;
    ScopedObject error(scope, newReferenceErrorObject(msg, fileName, line, column));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwRangeError(const QString &message)
{
    Scope scope(this);
    ScopedObject error(scope, newRangeErrorObject(message));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwRangeError(const Value &value)
{
    Scope scope(this);
    ScopedString s(scope, value.toString(this));
    QString msg = s->toQString() + QStringLiteral(" out of range");
    ScopedObject error(scope, newRangeErrorObject(msg));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwURIError(const Value &msg)
{
    Scope scope(this);
    ScopedObject error(scope, newURIErrorObject(msg));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwUnimplemented(const QString &message)
{
    Scope scope(this);
    ScopedValue v(scope, newString(QStringLiteral("Unimplemented ") + message));
    v = newErrorObject(v);
    return throwError(v);
}


QQmlError ExecutionEngine::catchExceptionAsQmlError()
{
    QV4::StackTrace trace;
    QV4::Scope scope(this);
    QV4::ScopedValue exception(scope, catchException(&trace));
    QQmlError error;
    if (!trace.isEmpty()) {
        QV4::StackFrame frame = trace.first();
        error.setUrl(QUrl(frame.source));
        error.setLine(frame.line);
        error.setColumn(frame.column);
    }
    QV4::Scoped<QV4::ErrorObject> errorObj(scope, exception);
    if (!!errorObj && errorObj->asSyntaxError()) {
        QV4::ScopedString m(scope, newString(QStringLiteral("message")));
        QV4::ScopedValue v(scope, errorObj->get(m));
        error.setDescription(v->toQStringNoThrow());
    } else
        error.setDescription(exception->toQStringNoThrow());
    return error;
}

bool ExecutionEngine::recheckCStackLimits()
{
    int dummy;
#ifdef Q_OS_WIN
    // ### this is only required on windows, where we currently use heuristics to get the stack limit
    if (cStackLimit - reinterpret_cast<quintptr>(&dummy) > 128*1024)
        // we're more then 128k away from our stack limit, assume the thread has changed, and
        // call getStackLimit
#endif
    // this can happen after a thread change
    cStackLimit = getStackLimit();

    return (reinterpret_cast<quintptr>(&dummy) >= cStackLimit);
}


// Variant conversion code

typedef QSet<QV4::Heap::Object *> V4ObjectSet;
static QVariant toVariant(QV4::ExecutionEngine *e, const QV4::Value &value, int typeHint, bool createJSValueForObjects, V4ObjectSet *visitedObjects);
static QObject *qtObjectFromJS(QV4::ExecutionEngine *engine, const QV4::Value &value);
static QVariant objectToVariant(QV4::ExecutionEngine *e, QV4::Object *o, V4ObjectSet *visitedObjects = 0);
static bool convertToNativeQObject(QV4::ExecutionEngine *e, const QV4::Value &value,
                            const QByteArray &targetType,
                            void **result);
static QV4::ReturnedValue variantListToJS(QV4::ExecutionEngine *v4, const QVariantList &lst);
static QV4::ReturnedValue variantMapToJS(QV4::ExecutionEngine *v4, const QVariantMap &vmap);
static QV4::ReturnedValue variantToJS(QV4::ExecutionEngine *v4, const QVariant &value)
{
    return v4->metaTypeToJS(value.userType(), value.constData());
}


QVariant ExecutionEngine::toVariant(const Value &value, int typeHint, bool createJSValueForObjects)
{
    return ::toVariant(this, value, typeHint, createJSValueForObjects, 0);
}


static QVariant toVariant(QV4::ExecutionEngine *e, const QV4::Value &value, int typeHint, bool createJSValueForObjects, V4ObjectSet *visitedObjects)
{
    Q_ASSERT (!value.isEmpty());
    QV4::Scope scope(e);

    if (QV4::VariantObject *v = value.as<QV4::VariantObject>())
        return v->d()->data;

    if (typeHint == QVariant::Bool)
        return QVariant(value.toBoolean());

    if (typeHint == QMetaType::QJsonValue)
        return QVariant::fromValue(QV4::JsonObject::toJsonValue(value));

    if (typeHint == qMetaTypeId<QJSValue>())
        return QVariant::fromValue(QJSValue(e, value.asReturnedValue()));

    if (value.asObject()) {
        QV4::ScopedObject object(scope, value);
        if (typeHint == QMetaType::QJsonObject
                   && !value.asArrayObject() && !value.asFunctionObject()) {
            return QVariant::fromValue(QV4::JsonObject::toJsonObject(object));
        } else if (QV4::QObjectWrapper *wrapper = object->as<QV4::QObjectWrapper>()) {
            return qVariantFromValue<QObject *>(wrapper->object());
        } else if (object->as<QV4::QmlContextWrapper>()) {
            return QVariant();
        } else if (QV4::QmlTypeWrapper *w = object->as<QV4::QmlTypeWrapper>()) {
            return w->toVariant();
        } else if (QV4::QQmlValueTypeWrapper *v = object->as<QV4::QQmlValueTypeWrapper>()) {
            return v->toVariant();
        } else if (QV4::QmlListWrapper *l = object->as<QV4::QmlListWrapper>()) {
            return l->toVariant();
        } else if (object->isListType())
            return QV4::SequencePrototype::toVariant(object);
    }

    if (value.asArrayObject()) {
        QV4::ScopedArrayObject a(scope, value);
        if (typeHint == qMetaTypeId<QList<QObject *> >()) {
            QList<QObject *> list;
            uint length = a->getLength();
            QV4::Scoped<QV4::QObjectWrapper> qobjectWrapper(scope);
            for (uint ii = 0; ii < length; ++ii) {
                qobjectWrapper = a->getIndexed(ii);
                if (!!qobjectWrapper) {
                    list << qobjectWrapper->object();
                } else {
                    list << 0;
                }
            }

            return qVariantFromValue<QList<QObject*> >(list);
        } else if (typeHint == QMetaType::QJsonArray) {
            return QVariant::fromValue(QV4::JsonObject::toJsonArray(a));
        }

        bool succeeded = false;
        QVariant retn = QV4::SequencePrototype::toVariant(value, typeHint, &succeeded);
        if (succeeded)
            return retn;
    }

    if (value.isUndefined())
        return QVariant();
    if (value.isNull())
        return QVariant(QMetaType::VoidStar, (void *)0);
    if (value.isBoolean())
        return value.booleanValue();
    if (value.isInteger())
        return value.integerValue();
    if (value.isNumber())
        return value.asDouble();
    if (value.isString())
        return value.stringValue()->toQString();
    if (QV4::QQmlLocaleData *ld = value.as<QV4::QQmlLocaleData>())
        return ld->d()->locale;
    if (QV4::DateObject *d = value.asDateObject())
        return d->toQDateTime();
    if (QV4::ArrayBuffer *d = value.as<ArrayBuffer>())
        return d->asByteArray();
    // NOTE: since we convert QTime to JS Date, round trip will change the variant type (to QDateTime)!

    QV4::ScopedObject o(scope, value);
    Q_ASSERT(o);

    if (QV4::RegExpObject *re = o->as<QV4::RegExpObject>())
        return re->toQRegExp();

    if (createJSValueForObjects)
        return QVariant::fromValue(QJSValue(scope.engine, o->asReturnedValue()));

    return objectToVariant(e, o, visitedObjects);
}

static QVariant objectToVariant(QV4::ExecutionEngine *e, QV4::Object *o, V4ObjectSet *visitedObjects)
{
    Q_ASSERT(o);

    V4ObjectSet recursionGuardSet;
    if (!visitedObjects) {
        visitedObjects = &recursionGuardSet;
    } else if (visitedObjects->contains(o->d())) {
        // Avoid recursion.
        // For compatibility with QVariant{List,Map} conversion, we return an
        // empty object (and no error is thrown).
        if (o->asArrayObject())
            return QVariantList();
        return QVariantMap();
    }
    visitedObjects->insert(o->d());

    QVariant result;

    if (o->asArrayObject()) {
        QV4::Scope scope(e);
        QV4::ScopedArrayObject a(scope, o->asReturnedValue());
        QV4::ScopedValue v(scope);
        QVariantList list;

        int length = a->getLength();
        for (int ii = 0; ii < length; ++ii) {
            v = a->getIndexed(ii);
            list << ::toVariant(e, v, -1, /*createJSValueForObjects*/false, visitedObjects);
        }

        result = list;
    } else if (!o->asFunctionObject()) {
        QVariantMap map;
        QV4::Scope scope(e);
        QV4::ObjectIterator it(scope, o, QV4::ObjectIterator::EnumerableOnly);
        QV4::ScopedValue name(scope);
        QV4::ScopedValue val(scope);
        while (1) {
            name = it.nextPropertyNameAsString(val);
            if (name->isNull())
                break;

            QString key = name->toQStringNoThrow();
            map.insert(key, ::toVariant(e, val, /*type hint*/-1, /*createJSValueForObjects*/false, visitedObjects));
        }

        result = map;
    }

    visitedObjects->remove(o->d());
    return result;
}

static QV4::ReturnedValue arrayFromVariantList(QV4::ExecutionEngine *e, const QVariantList &list)
{
    QV4::Scope scope(e);
    QV4::ScopedArrayObject a(scope, e->newArrayObject());
    int len = list.count();
    a->arrayReserve(len);
    QV4::ScopedValue v(scope);
    for (int ii = 0; ii < len; ++ii)
        a->arrayPut(ii, (v = scope.engine->fromVariant(list.at(ii))));

    a->setArrayLengthUnchecked(len);
    return a.asReturnedValue();
}

static QV4::ReturnedValue objectFromVariantMap(QV4::ExecutionEngine *e, const QVariantMap &map)
{
    QV4::Scope scope(e);
    QV4::ScopedObject o(scope, e->newObject());
    QV4::ScopedString s(scope);
    QV4::ScopedValue v(scope);
    for (QVariantMap::const_iterator iter = map.begin(), cend = map.end(); iter != cend; ++iter) {
        s = e->newString(iter.key());
        uint idx = s->asArrayIndex();
        if (idx > 16 && (!o->arrayData() || idx > o->arrayData()->length() * 2))
            o->initSparseArray();
        o->put(s, (v = e->fromVariant(iter.value())));
    }
    return o.asReturnedValue();
}

Q_CORE_EXPORT QString qt_regexp_toCanonical(const QString &, QRegExp::PatternSyntax);

QV4::ReturnedValue QV4::ExecutionEngine::fromVariant(const QVariant &variant)
{
    int type = variant.userType();
    const void *ptr = variant.constData();

    if (type < QMetaType::User) {
        switch (QMetaType::Type(type)) {
            case QMetaType::UnknownType:
            case QMetaType::Void:
                return QV4::Encode::undefined();
            case QMetaType::VoidStar:
                return QV4::Encode::null();
            case QMetaType::Bool:
                return QV4::Encode(*reinterpret_cast<const bool*>(ptr));
            case QMetaType::Int:
                return QV4::Encode(*reinterpret_cast<const int*>(ptr));
            case QMetaType::UInt:
                return QV4::Encode(*reinterpret_cast<const uint*>(ptr));
            case QMetaType::LongLong:
                return QV4::Encode((double)*reinterpret_cast<const qlonglong*>(ptr));
            case QMetaType::ULongLong:
                return QV4::Encode((double)*reinterpret_cast<const qulonglong*>(ptr));
            case QMetaType::Double:
                return QV4::Encode(*reinterpret_cast<const double*>(ptr));
            case QMetaType::QString:
                return newString(*reinterpret_cast<const QString*>(ptr))->asReturnedValue();
            case QMetaType::Float:
                return QV4::Encode(*reinterpret_cast<const float*>(ptr));
            case QMetaType::Short:
                return QV4::Encode((int)*reinterpret_cast<const short*>(ptr));
            case QMetaType::UShort:
                return QV4::Encode((int)*reinterpret_cast<const unsigned short*>(ptr));
            case QMetaType::Char:
                return newString(QChar::fromLatin1(*reinterpret_cast<const char *>(ptr)))->asReturnedValue();
            case QMetaType::UChar:
                return newString(QChar::fromLatin1(*reinterpret_cast<const unsigned char *>(ptr)))->asReturnedValue();
            case QMetaType::QChar:
                return newString(*reinterpret_cast<const QChar *>(ptr))->asReturnedValue();
            case QMetaType::QDateTime:
                return QV4::Encode(newDateObject(*reinterpret_cast<const QDateTime *>(ptr)));
            case QMetaType::QDate:
                return QV4::Encode(newDateObject(QDateTime(*reinterpret_cast<const QDate *>(ptr))));
            case QMetaType::QTime:
            return QV4::Encode(newDateObject(QDateTime(QDate(1970,1,1), *reinterpret_cast<const QTime *>(ptr))));
            case QMetaType::QRegExp:
                return QV4::Encode(newRegExpObject(*reinterpret_cast<const QRegExp *>(ptr)));
            case QMetaType::QObjectStar:
                return QV4::QObjectWrapper::wrap(this, *reinterpret_cast<QObject* const *>(ptr));
            case QMetaType::QStringList:
                {
                bool succeeded = false;
                QV4::Scope scope(this);
                QV4::ScopedValue retn(scope, QV4::SequencePrototype::fromVariant(this, variant, &succeeded));
                if (succeeded)
                    return retn->asReturnedValue();
                return QV4::Encode(newArrayObject(*reinterpret_cast<const QStringList *>(ptr)));
                }
            case QMetaType::QVariantList:
                return arrayFromVariantList(this, *reinterpret_cast<const QVariantList *>(ptr));
            case QMetaType::QVariantMap:
                return objectFromVariantMap(this, *reinterpret_cast<const QVariantMap *>(ptr));
            case QMetaType::QJsonValue:
                return QV4::JsonObject::fromJsonValue(this, *reinterpret_cast<const QJsonValue *>(ptr));
            case QMetaType::QJsonObject:
                return QV4::JsonObject::fromJsonObject(this, *reinterpret_cast<const QJsonObject *>(ptr));
            case QMetaType::QJsonArray:
                return QV4::JsonObject::fromJsonArray(this, *reinterpret_cast<const QJsonArray *>(ptr));
            case QMetaType::QLocale:
                return QQmlLocale::wrap(this, *reinterpret_cast<const QLocale*>(ptr));
            default:
                break;
        }

        if (const QMetaObject *vtmo = QQmlValueTypeFactory::metaObjectForMetaType(type))
            return QV4::QQmlValueTypeWrapper::create(this, variant, vtmo, type);
    } else {
        QV4::Scope scope(this);
        if (type == qMetaTypeId<QQmlListReference>()) {
            typedef QQmlListReferencePrivate QDLRP;
            QDLRP *p = QDLRP::get((QQmlListReference*)const_cast<void *>(ptr));
            if (p->object) {
                return QV4::QmlListWrapper::create(scope.engine, p->property, p->propertyType);
            } else {
                return QV4::Encode::null();
            }
        } else if (type == qMetaTypeId<QJSValue>()) {
            const QJSValue *value = reinterpret_cast<const QJSValue *>(ptr);
            return QJSValuePrivate::convertedToValue(this, *value);
        } else if (type == qMetaTypeId<QList<QObject *> >()) {
            // XXX Can this be made more by using Array as a prototype and implementing
            // directly against QList<QObject*>?
            const QList<QObject *> &list = *(const QList<QObject *>*)ptr;
            QV4::ScopedArrayObject a(scope, newArrayObject());
            a->arrayReserve(list.count());
            QV4::ScopedValue v(scope);
            for (int ii = 0; ii < list.count(); ++ii)
                a->arrayPut(ii, (v = QV4::QObjectWrapper::wrap(this, list.at(ii))));
            a->setArrayLengthUnchecked(list.count());
            return a.asReturnedValue();
        } else if (QMetaType::typeFlags(type) & QMetaType::PointerToQObject) {
            return QV4::QObjectWrapper::wrap(this, *reinterpret_cast<QObject* const *>(ptr));
        }

        bool objOk;
        QObject *obj = QQmlMetaType::toQObject(variant, &objOk);
        if (objOk)
            return QV4::QObjectWrapper::wrap(this, obj);

        bool succeeded = false;
        QV4::ScopedValue retn(scope, QV4::SequencePrototype::fromVariant(this, variant, &succeeded));
        if (succeeded)
            return retn->asReturnedValue();

        if (const QMetaObject *vtmo = QQmlValueTypeFactory::metaObjectForMetaType(type))
            return QV4::QQmlValueTypeWrapper::create(this, variant, vtmo, type);
    }

    // XXX TODO: To be compatible, we still need to handle:
    //    + QObjectList
    //    + QList<int>

    return QV4::Encode(newVariantObject(variant));
}

QVariantMap ExecutionEngine::variantMapFromJS(Object *o)
{
    return objectToVariant(this, o).toMap();
}


// Converts a QVariantList to JS.
// The result is a new Array object with length equal to the length
// of the QVariantList, and the elements being the QVariantList's
// elements converted to JS, recursively.
static QV4::ReturnedValue variantListToJS(QV4::ExecutionEngine *v4, const QVariantList &lst)
{
    QV4::Scope scope(v4);
    QV4::ScopedArrayObject a(scope, v4->newArrayObject());
    a->arrayReserve(lst.size());
    QV4::ScopedValue v(scope);
    for (int i = 0; i < lst.size(); i++)
        a->arrayPut(i, (v = variantToJS(v4, lst.at(i))));
    a->setArrayLengthUnchecked(lst.size());
    return a.asReturnedValue();
}

// Converts a QVariantMap to JS.
// The result is a new Object object with property names being
// the keys of the QVariantMap, and values being the values of
// the QVariantMap converted to JS, recursively.
static QV4::ReturnedValue variantMapToJS(QV4::ExecutionEngine *v4, const QVariantMap &vmap)
{
    QV4::Scope scope(v4);
    QV4::ScopedObject o(scope, v4->newObject());
    QV4::ScopedString s(scope);
    QV4::ScopedValue v(scope);
    for (QVariantMap::const_iterator it = vmap.constBegin(), cend = vmap.constEnd(); it != cend; ++it) {
        s = v4->newIdentifier(it.key());
        v = variantToJS(v4, it.value());
        uint idx = s->asArrayIndex();
        if (idx < UINT_MAX)
            o->arraySet(idx, v);
        else
            o->insertMember(s, v);
    }
    return o.asReturnedValue();
}

// Converts the meta-type defined by the given type and data to JS.
// Returns the value if conversion succeeded, an empty handle otherwise.
QV4::ReturnedValue ExecutionEngine::metaTypeToJS(int type, const void *data)
{
    Q_ASSERT(data != 0);

    // check if it's one of the types we know
    switch (QMetaType::Type(type)) {
    case QMetaType::UnknownType:
    case QMetaType::Void:
        return QV4::Encode::undefined();
    case QMetaType::VoidStar:
        return QV4::Encode::null();
    case QMetaType::Bool:
        return QV4::Encode(*reinterpret_cast<const bool*>(data));
    case QMetaType::Int:
        return QV4::Encode(*reinterpret_cast<const int*>(data));
    case QMetaType::UInt:
        return QV4::Encode(*reinterpret_cast<const uint*>(data));
    case QMetaType::LongLong:
        return QV4::Encode(double(*reinterpret_cast<const qlonglong*>(data)));
    case QMetaType::ULongLong:
#if defined(Q_OS_WIN) && defined(_MSC_FULL_VER) && _MSC_FULL_VER <= 12008804
#pragma message("** NOTE: You need the Visual Studio Processor Pack to compile support for 64bit unsigned integers.")
        return QV4::Encode(double((qlonglong)*reinterpret_cast<const qulonglong*>(data)));
#elif defined(Q_CC_MSVC) && !defined(Q_CC_MSVC_NET)
        return QV4::Encode(double((qlonglong)*reinterpret_cast<const qulonglong*>(data)));
#else
        return QV4::Encode(double(*reinterpret_cast<const qulonglong*>(data)));
#endif
    case QMetaType::Double:
        return QV4::Encode(*reinterpret_cast<const double*>(data));
    case QMetaType::QString:
        return newString(*reinterpret_cast<const QString*>(data))->asReturnedValue();
    case QMetaType::Float:
        return QV4::Encode(*reinterpret_cast<const float*>(data));
    case QMetaType::Short:
        return QV4::Encode((int)*reinterpret_cast<const short*>(data));
    case QMetaType::UShort:
        return QV4::Encode((int)*reinterpret_cast<const unsigned short*>(data));
    case QMetaType::Char:
        return QV4::Encode((int)*reinterpret_cast<const char*>(data));
    case QMetaType::UChar:
        return QV4::Encode((int)*reinterpret_cast<const unsigned char*>(data));
    case QMetaType::QChar:
        return QV4::Encode((int)(*reinterpret_cast<const QChar*>(data)).unicode());
    case QMetaType::QStringList:
        return QV4::Encode(newArrayObject(*reinterpret_cast<const QStringList *>(data)));
    case QMetaType::QVariantList:
        return variantListToJS(this, *reinterpret_cast<const QVariantList *>(data));
    case QMetaType::QVariantMap:
        return variantMapToJS(this, *reinterpret_cast<const QVariantMap *>(data));
    case QMetaType::QDateTime:
        return QV4::Encode(newDateObject(*reinterpret_cast<const QDateTime *>(data)));
    case QMetaType::QDate:
        return QV4::Encode(newDateObject(QDateTime(*reinterpret_cast<const QDate *>(data))));
    case QMetaType::QRegExp:
        return QV4::Encode(newRegExpObject(*reinterpret_cast<const QRegExp *>(data)));
    case QMetaType::QObjectStar:
        return QV4::QObjectWrapper::wrap(this, *reinterpret_cast<QObject* const *>(data));
    case QMetaType::QVariant:
        return variantToJS(this, *reinterpret_cast<const QVariant*>(data));
    case QMetaType::QJsonValue:
        return QV4::JsonObject::fromJsonValue(this, *reinterpret_cast<const QJsonValue *>(data));
    case QMetaType::QJsonObject:
        return QV4::JsonObject::fromJsonObject(this, *reinterpret_cast<const QJsonObject *>(data));
    case QMetaType::QJsonArray:
        return QV4::JsonObject::fromJsonArray(this, *reinterpret_cast<const QJsonArray *>(data));
    default:
        if (type == qMetaTypeId<QJSValue>()) {
            return QJSValuePrivate::convertedToValue(this, *reinterpret_cast<const QJSValue*>(data));
        } else {
            QByteArray typeName = QMetaType::typeName(type);
            if (typeName.endsWith('*') && !*reinterpret_cast<void* const *>(data)) {
                return QV4::Encode::null();
            }
            QMetaType mt(type);
            if (mt.flags() & QMetaType::IsGadget) {
                Q_ASSERT(mt.metaObject());
                return QV4::QQmlValueTypeWrapper::create(this, QVariant(type, data), mt.metaObject(), type);
            }
            // Fall back to wrapping in a QVariant.
            return QV4::Encode(newVariantObject(QVariant(type, data)));
        }
    }
    Q_UNREACHABLE();
    return 0;
}

void ExecutionEngine::assertObjectBelongsToEngine(const Value &v)
{
    Q_UNUSED(v);
    Q_ASSERT(!v.isObject() || v.objectValue()->engine() == this);
    Q_UNUSED(v);
}

// Converts a JS value to a meta-type.
// data must point to a place that can store a value of the given type.
// Returns true if conversion succeeded, false otherwise.
bool ExecutionEngine::metaTypeFromJS(const QV4::Value &value, int type, void *data)
{
    QV4::Scope scope(this);

    // check if it's one of the types we know
    switch (QMetaType::Type(type)) {
    case QMetaType::Bool:
        *reinterpret_cast<bool*>(data) = value.toBoolean();
        return true;
    case QMetaType::Int:
        *reinterpret_cast<int*>(data) = value.toInt32();
        return true;
    case QMetaType::UInt:
        *reinterpret_cast<uint*>(data) = value.toUInt32();
        return true;
    case QMetaType::LongLong:
        *reinterpret_cast<qlonglong*>(data) = qlonglong(value.toInteger());
        return true;
    case QMetaType::ULongLong:
        *reinterpret_cast<qulonglong*>(data) = qulonglong(value.toInteger());
        return true;
    case QMetaType::Double:
        *reinterpret_cast<double*>(data) = value.toNumber();
        return true;
    case QMetaType::QString:
        if (value.isUndefined() || value.isNull())
            *reinterpret_cast<QString*>(data) = QString();
        else
            *reinterpret_cast<QString*>(data) = value.toQString();
        return true;
    case QMetaType::Float:
        *reinterpret_cast<float*>(data) = value.toNumber();
        return true;
    case QMetaType::Short:
        *reinterpret_cast<short*>(data) = short(value.toInt32());
        return true;
    case QMetaType::UShort:
        *reinterpret_cast<unsigned short*>(data) = value.toUInt16();
        return true;
    case QMetaType::Char:
        *reinterpret_cast<char*>(data) = char(value.toInt32());
        return true;
    case QMetaType::UChar:
        *reinterpret_cast<unsigned char*>(data) = (unsigned char)(value.toInt32());
        return true;
    case QMetaType::QChar:
        if (value.isString()) {
            QString str = value.stringValue()->toQString();
            *reinterpret_cast<QChar*>(data) = str.isEmpty() ? QChar() : str.at(0);
        } else {
            *reinterpret_cast<QChar*>(data) = QChar(ushort(value.toUInt16()));
        }
        return true;
    case QMetaType::QDateTime:
        if (QV4::DateObject *d = value.asDateObject()) {
            *reinterpret_cast<QDateTime *>(data) = d->toQDateTime();
            return true;
        } break;
    case QMetaType::QDate:
        if (QV4::DateObject *d = value.asDateObject()) {
            *reinterpret_cast<QDate *>(data) = d->toQDateTime().date();
            return true;
        } break;
    case QMetaType::QRegExp:
        if (QV4::RegExpObject *r = value.as<QV4::RegExpObject>()) {
            *reinterpret_cast<QRegExp *>(data) = r->toQRegExp();
            return true;
        } break;
    case QMetaType::QObjectStar: {
        QV4::QObjectWrapper *qobjectWrapper = value.as<QV4::QObjectWrapper>();
        if (qobjectWrapper || value.isNull()) {
            *reinterpret_cast<QObject* *>(data) = qtObjectFromJS(scope.engine, value);
            return true;
        } break;
    }
    case QMetaType::QStringList: {
        QV4::ScopedArrayObject a(scope, value);
        if (a) {
            *reinterpret_cast<QStringList *>(data) = a->toQStringList();
            return true;
        }
        break;
    }
    case QMetaType::QVariantList: {
        QV4::ScopedArrayObject a(scope, value);
        if (a) {
            *reinterpret_cast<QVariantList *>(data) = scope.engine->toVariant(a, /*typeHint*/-1, /*createJSValueForObjects*/false).toList();
            return true;
        }
        break;
    }
    case QMetaType::QVariantMap: {
        QV4::ScopedObject o(scope, value);
        if (o) {
            *reinterpret_cast<QVariantMap *>(data) = variantMapFromJS(o);
            return true;
        }
        break;
    }
    case QMetaType::QVariant:
        *reinterpret_cast<QVariant*>(data) = scope.engine->toVariant(value, /*typeHint*/-1, /*createJSValueForObjects*/false);
        return true;
    case QMetaType::QJsonValue:
        *reinterpret_cast<QJsonValue *>(data) = QV4::JsonObject::toJsonValue(value);
        return true;
    case QMetaType::QJsonObject: {
        QV4::ScopedObject o(scope, value);
        *reinterpret_cast<QJsonObject *>(data) = QV4::JsonObject::toJsonObject(o);
        return true;
    }
    case QMetaType::QJsonArray: {
        QV4::ScopedArrayObject a(scope, value);
        if (a) {
            *reinterpret_cast<QJsonArray *>(data) = QV4::JsonObject::toJsonArray(a);
            return true;
        }
        break;
    }
    default:
    ;
    }

    {
        QV4::Scoped<QV4::QQmlValueTypeWrapper> vtw(scope, value);
        if (vtw && vtw->typeId() == type) {
            return vtw->toGadget(data);
        }
    }

#if 0
    if (isQtVariant(value)) {
        const QVariant &var = variantValue(value);
        // ### Enable once constructInPlace() is in qt master.
        if (var.userType() == type) {
            QMetaType::constructInPlace(type, data, var.constData());
            return true;
        }
        if (var.canConvert(type)) {
            QVariant vv = var;
            vv.convert(type);
            Q_ASSERT(vv.userType() == type);
            QMetaType::constructInPlace(type, data, vv.constData());
            return true;
        }

    }
#endif

    // Try to use magic; for compatibility with qscriptvalue_cast.

    QByteArray name = QMetaType::typeName(type);
    if (convertToNativeQObject(this, value, name, reinterpret_cast<void* *>(data)))
        return true;
    if (value.as<QV4::VariantObject>() && name.endsWith('*')) {
        int valueType = QMetaType::type(name.left(name.size()-1));
        QVariant &var = value.as<QV4::VariantObject>()->d()->data;
        if (valueType == var.userType()) {
            // We have T t, T* is requested, so return &t.
            *reinterpret_cast<void* *>(data) = var.data();
            return true;
        } else if (value.isObject()) {
            // Look in the prototype chain.
            QV4::ScopedObject proto(scope, value.objectValue()->prototype());
            while (proto) {
                bool canCast = false;
                if (QV4::VariantObject *vo = proto->as<QV4::VariantObject>()) {
                    const QVariant &v = vo->d()->data;
                    canCast = (type == v.userType()) || (valueType && (valueType == v.userType()));
                }
                else if (proto->as<QV4::QObjectWrapper>()) {
                    QByteArray className = name.left(name.size()-1);
                    QV4::ScopedObject p(scope, proto.getPointer());
                    if (QObject *qobject = qtObjectFromJS(scope.engine, p))
                        canCast = qobject->qt_metacast(className) != 0;
                }
                if (canCast) {
                    QByteArray varTypeName = QMetaType::typeName(var.userType());
                    if (varTypeName.endsWith('*'))
                        *reinterpret_cast<void* *>(data) = *reinterpret_cast<void* *>(var.data());
                    else
                        *reinterpret_cast<void* *>(data) = var.data();
                    return true;
                }
                proto = proto->prototype();
            }
        }
    } else if (value.isNull() && name.endsWith('*')) {
        *reinterpret_cast<void* *>(data) = 0;
        return true;
    } else if (type == qMetaTypeId<QJSValue>()) {
        *reinterpret_cast<QJSValue*>(data) = QJSValue(this, value.asReturnedValue());
        return true;
    }

    return false;
}

static bool convertToNativeQObject(QV4::ExecutionEngine *e, const Value &value, const QByteArray &targetType, void **result)
{
    if (!targetType.endsWith('*'))
        return false;
    if (QObject *qobject = qtObjectFromJS(e, value)) {
        int start = targetType.startsWith("const ") ? 6 : 0;
        QByteArray className = targetType.mid(start, targetType.size()-start-1);
        if (void *instance = qobject->qt_metacast(className)) {
            *result = instance;
            return true;
        }
    }
    return false;
}

static QObject *qtObjectFromJS(QV4::ExecutionEngine *engine, const Value &value)
{
    if (!value.isObject())
        return 0;

    QV4::Scope scope(engine);
    QV4::Scoped<QV4::VariantObject> v(scope, value);

    if (v) {
        QVariant variant = v->d()->data;
        int type = variant.userType();
        if (type == QMetaType::QObjectStar)
            return *reinterpret_cast<QObject* const *>(variant.constData());
    }
    QV4::Scoped<QV4::QObjectWrapper> wrapper(scope, value);
    if (!wrapper)
        return 0;
    return wrapper->object();
}


QT_END_NAMESPACE
