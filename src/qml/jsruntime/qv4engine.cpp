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
#  else
    void* stackBottom = 0;
    pthread_attr_t attr;
#if HAVE(PTHREAD_NP_H) && OS(FREEBSD)
    if (pthread_attr_get_np(pthread_self(), &attr) == 0) {
#else
    if (pthread_getattr_np(pthread_self(), &attr) == 0) {
#endif
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
    stackLimit = reinterpret_cast<qintptr>(&dummy) - 1024*1024;
#endif

    // 256k slack
    return stackLimit + MinimumStackSize*1024;
}


QJSEngine *ExecutionEngine::jsEngine() const
{
    return v8Engine->publicEngine();
}

ExecutionEngine::ExecutionEngine(EvalISelFactory *factory)
    : current(0)
    , memoryManager(new QV4::MemoryManager)
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

    memoryManager->setExecutionEngine(this);

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
    executionContextClass = InternalClass::create(this, ExecutionContext::staticVTable());
    stringClass = InternalClass::create(this, String::staticVTable());
    regExpValueClass = InternalClass::create(this, RegExp::staticVTable());

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

    memberDataClass = InternalClass::create(this, MemberData::staticVTable());

    objectPrototype = memoryManager->alloc<ObjectPrototype>(InternalClass::create(this, ObjectPrototype::staticVTable()), (QV4::Object *)0);
    objectClass = InternalClass::create(this, Object::staticVTable());
    Q_ASSERT(objectClass->vtable == Object::staticVTable());

    arrayClass = InternalClass::create(this, ArrayObject::staticVTable());
    arrayClass = arrayClass->addMember(id_length, Attr_NotConfigurable|Attr_NotEnumerable);
    arrayPrototype = memoryManager->alloc<ArrayPrototype>(arrayClass, objectPrototype.asObject());

    simpleArrayDataClass = InternalClass::create(this, SimpleArrayData::staticVTable());

    InternalClass *argsClass = InternalClass::create(this, ArgumentsObject::staticVTable());
    argsClass = argsClass->addMember(id_length, Attr_NotEnumerable);
    argumentsObjectClass = argsClass->addMember(id_callee, Attr_Data|Attr_NotEnumerable);
    strictArgumentsObjectClass = argsClass->addMember(id_callee, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
    strictArgumentsObjectClass = strictArgumentsObjectClass->addMember(id_caller, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
    Q_ASSERT(argumentsObjectClass->vtable == ArgumentsObject::staticVTable());
    Q_ASSERT(strictArgumentsObjectClass->vtable == ArgumentsObject::staticVTable());

    m_globalObject = newObject();
    Q_ASSERT(globalObject()->internalClass()->vtable);
    initRootContext();

    stringPrototype = memoryManager->alloc<StringPrototype>(InternalClass::create(this, StringPrototype::staticVTable()), objectPrototype.asObject());
    stringObjectClass = InternalClass::create(this, String::staticVTable());

    numberPrototype = memoryManager->alloc<NumberPrototype>(InternalClass::create(this, NumberPrototype::staticVTable()), objectPrototype.asObject());
    numberClass = InternalClass::create(this, NumberObject::staticVTable());

    booleanPrototype = memoryManager->alloc<BooleanPrototype>(InternalClass::create(this, BooleanPrototype::staticVTable()), objectPrototype.asObject());
    booleanClass = InternalClass::create(this, BooleanObject::staticVTable());

    datePrototype = memoryManager->alloc<DatePrototype>(InternalClass::create(this, DatePrototype::staticVTable()), objectPrototype.asObject());
    dateClass = InternalClass::create(this, DateObject::staticVTable());

    InternalClass *functionProtoClass = InternalClass::create(this, FunctionObject::staticVTable());
    uint index;
    functionProtoClass = functionProtoClass->addMember(id_prototype, Attr_NotEnumerable, &index);
    Q_ASSERT(index == Heap::FunctionObject::Index_Prototype);
    functionPrototype = memoryManager->alloc<FunctionPrototype>(functionProtoClass, objectPrototype.asObject());
    functionClass = InternalClass::create(this, FunctionObject::staticVTable());
    functionClass = functionClass->addMember(id_prototype, Attr_NotEnumerable|Attr_NotConfigurable, &index);
    Q_ASSERT(index == Heap::FunctionObject::Index_Prototype);
    protoClass = objectClass->addMember(id_constructor, Attr_NotEnumerable, &index);
    Q_ASSERT(index == Heap::FunctionObject::Index_ProtoConstructor);

    regExpPrototype = memoryManager->alloc<RegExpPrototype>(this);
    regExpClass = InternalClass::create(this, RegExpObject::staticVTable());
    regExpExecArrayClass = arrayClass->addMember(id_index, Attr_Data, &index);
    Q_ASSERT(index == RegExpObject::Index_ArrayIndex);
    regExpExecArrayClass = regExpExecArrayClass->addMember(id_input, Attr_Data, &index);
    Q_ASSERT(index == RegExpObject::Index_ArrayInput);

    errorPrototype = memoryManager->alloc<ErrorPrototype>(InternalClass::create(this, ErrorObject::staticVTable()), objectPrototype.asObject());
    errorClass = InternalClass::create(this, ErrorObject::staticVTable());
    evalErrorPrototype = memoryManager->alloc<EvalErrorPrototype>(errorClass, errorPrototype.asObject());
    evalErrorClass = InternalClass::create(this, EvalErrorObject::staticVTable());
    rangeErrorPrototype = memoryManager->alloc<RangeErrorPrototype>(errorClass, errorPrototype.asObject());
    rangeErrorClass = InternalClass::create(this, RangeErrorObject::staticVTable());
    referenceErrorPrototype = memoryManager->alloc<ReferenceErrorPrototype>(errorClass, errorPrototype.asObject());
    referenceErrorClass = InternalClass::create(this, ReferenceErrorObject::staticVTable());
    syntaxErrorPrototype = memoryManager->alloc<SyntaxErrorPrototype>(errorClass, errorPrototype.asObject());
    syntaxErrorClass = InternalClass::create(this, SyntaxErrorObject::staticVTable());
    typeErrorPrototype = memoryManager->alloc<TypeErrorPrototype>(errorClass, errorPrototype.asObject());
    typeErrorClass = InternalClass::create(this, TypeErrorObject::staticVTable());
    uRIErrorPrototype = memoryManager->alloc<URIErrorPrototype>(errorClass, errorPrototype.asObject());
    uriErrorClass = InternalClass::create(this, URIErrorObject::staticVTable());

    variantPrototype = memoryManager->alloc<VariantPrototype>(InternalClass::create(this, VariantPrototype::staticVTable()), objectPrototype.asObject());
    variantClass = InternalClass::create(this, VariantObject::staticVTable());
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
    arrayBufferPrototype = memoryManager->alloc<ArrayBufferPrototype>(objectClass, objectPrototype.asObject());
    static_cast<ArrayBufferPrototype *>(arrayBufferPrototype.asObject())->init(this, arrayBufferCtor.asObject());
    arrayBufferClass = InternalClass::create(this, ArrayBuffer::staticVTable());

    dataViewCtor = memoryManager->alloc<DataViewCtor>(global);
    dataViewPrototype = memoryManager->alloc<DataViewPrototype>(objectClass, objectPrototype.asObject());
    static_cast<DataViewPrototype *>(dataViewPrototype.asObject())->init(this, dataViewCtor.asObject());
    dataViewClass = InternalClass::create(this, DataView::staticVTable());

    for (int i = 0; i < Heap::TypedArray::NTypes; ++i) {
        typedArrayCtors[i] = memoryManager->alloc<TypedArrayCtor>(global, Heap::TypedArray::Type(i));
        typedArrayPrototype[i] = memoryManager->alloc<TypedArrayPrototype>(this, Heap::TypedArray::Type(i));
        typedArrayPrototype[i].as<TypedArrayPrototype>()->init(this, static_cast<TypedArrayCtor *>(typedArrayCtors[i].asObject()));
        typedArrayClasses[i] = InternalClass::create(this, TypedArray::staticVTable());
    }

    //
    // set up the global object
    //
    rootContext()->global = globalObject()->d();
    rootContext()->callData->thisObject = globalObject();
    Q_ASSERT(globalObject()->internalClass()->vtable);

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
    Scoped<GlobalContext> r(scope, static_cast<Heap::GlobalContext*>(memoryManager->allocManaged(sizeof(GlobalContext::Data) + sizeof(CallData))));
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
    return ScopedString(scope, memoryManager->alloc<String>(this, s))->d();
}

Heap::String *ExecutionEngine::newIdentifier(const QString &text)
{
    return identifierTable->insertString(text);
}

Heap::Object *ExecutionEngine::newStringObject(const ValueRef value)
{
    Scope scope(this);
    Scoped<StringObject> object(scope, memoryManager->alloc<StringObject>(this, value));
    return object->d();
}

Heap::Object *ExecutionEngine::newNumberObject(const ValueRef value)
{
    Scope scope(this);
    Scoped<NumberObject> object(scope, memoryManager->alloc<NumberObject>(this, value));
    return object->d();
}

Heap::Object *ExecutionEngine::newBooleanObject(const ValueRef value)
{
    Scope scope(this);
    ScopedObject object(scope, memoryManager->alloc<BooleanObject>(this, value));
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


Heap::DateObject *ExecutionEngine::newDateObject(const ValueRef value)
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

Heap::Object *ExecutionEngine::newErrorObject(const ValueRef value)
{
    Scope scope(this);
    ScopedObject object(scope, memoryManager->alloc<ErrorObject>(errorClass, errorPrototype.asObject(), value));
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

Heap::Object *ExecutionEngine::newURIErrorObject(const ValueRef message)
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
    while (c && frameLimit) {
        CallContext *callCtx = c->asCallContext();
        if (callCtx && callCtx->d()->function) {
            StackFrame frame;
            ScopedFunctionObject function(scope, callCtx->d()->function);
            if (function->function())
                frame.source = function->function()->sourceFile();
            name = function->name();
            frame.function = name->toQString();
            frame.line = -1;
            frame.column = -1;

            if (callCtx->d()->function->function)
                // line numbers can be negative for places where you can't set a real breakpoint
                frame.line = qAbs(callCtx->d()->lineNumber);

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
        Q_ASSERT(c->inUse);
        if (!c->markBit) {
            c->markBit = 1;
            c->internalClass->vtable->markObjects(c, this);
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

ReturnedValue ExecutionEngine::throwError(const ValueRef value)
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
    Scoped<Object> error(scope, newSyntaxErrorObject(message, fileName, line, column));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwSyntaxError(const QString &message)
{
    Scope scope(this);
    Scoped<Object> error(scope, newSyntaxErrorObject(message));
    return throwError(error);
}


ReturnedValue ExecutionEngine::throwTypeError()
{
    Scope scope(this);
    Scoped<Object> error(scope, newTypeErrorObject(QStringLiteral("Type error")));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwTypeError(const QString &message)
{
    Scope scope(this);
    Scoped<Object> error(scope, newTypeErrorObject(message));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwReferenceError(const ValueRef value)
{
    Scope scope(this);
    ScopedString s(scope, value->toString(this));
    QString msg = s->toQString() + QStringLiteral(" is not defined");
    Scoped<Object> error(scope, newReferenceErrorObject(msg));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwReferenceError(const QString &message, const QString &fileName, int line, int column)
{
    Scope scope(this);
    QString msg = message;
    Scoped<Object> error(scope, newReferenceErrorObject(msg, fileName, line, column));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwRangeError(const QString &message)
{
    Scope scope(this);
    ScopedObject error(scope, newRangeErrorObject(message));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwRangeError(const ValueRef value)
{
    Scope scope(this);
    ScopedString s(scope, value->toString(this));
    QString msg = s->toQString() + QStringLiteral(" out of range");
    ScopedObject error(scope, newRangeErrorObject(msg));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwURIError(const ValueRef msg)
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

QT_END_NAMESPACE
