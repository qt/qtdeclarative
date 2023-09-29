// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include <qv4engine_p.h>

#include <private/qv4compileddata_p.h>
#include <private/qv4codegen_p.h>
#include <private/qqmljsdiagnosticmessage_p.h>

#include <QtCore/QTextStream>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#if QT_CONFIG(regularexpression)
#include <QRegularExpression>
#endif
#include <QtCore/QTimeZone>
#include <QtCore/qiterable.h>

#include <qv4qmlcontext_p.h>
#include <qv4value_p.h>
#include <qv4object_p.h>
#include <qv4objectproto_p.h>
#include <qv4objectiterator_p.h>
#include <qv4setiterator_p.h>
#include <qv4mapiterator_p.h>
#include <qv4arrayiterator_p.h>
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
#include "qv4symbol_p.h"
#include "qv4setobject_p.h"
#include "qv4mapobject_p.h"
#include <qv4variantobject_p.h>
#include <qv4runtime_p.h>
#include <private/qv4mm_p.h>
#include <qv4argumentsobject_p.h>
#include <qv4dateobject_p.h>
#include <qv4jsonobject_p.h>
#include <qv4stringobject_p.h>
#include <qv4identifiertable_p.h>
#include "qv4debugging_p.h"
#include "qv4profiling_p.h"
#include "qv4executableallocator_p.h"
#include "qv4iterator_p.h"
#include "qv4stringiterator_p.h"
#include "qv4generatorobject_p.h"
#include "qv4reflect_p.h"
#include "qv4proxy_p.h"
#include "qv4stackframe_p.h"
#include "qv4stacklimits_p.h"
#include "qv4atomics_p.h"
#include "qv4urlobject_p.h"
#include "qv4variantobject_p.h"
#include "qv4sequenceobject_p.h"
#include "qv4qobjectwrapper_p.h"
#include "qv4memberdata_p.h"
#include "qv4arraybuffer_p.h"
#include "qv4dataview_p.h"
#include "qv4promiseobject_p.h"
#include "qv4typedarray_p.h"
#include <private/qjsvalue_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/qqmlvaluetypewrapper_p.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qqmllistwrapper_p.h>
#include <private/qqmllist_p.h>
#include <private/qqmltypeloader_p.h>
#include <private/qqmlbuiltinfunctions_p.h>
#if QT_CONFIG(qml_locale)
#include <private/qqmllocale_p.h>
#endif
#if QT_CONFIG(qml_xml_http_request)
#include <private/qv4domerrors_p.h>
#include <private/qqmlxmlhttprequest_p.h>
#endif
#include <private/qv4sqlerrors_p.h>
#include <qqmlfile.h>
#include <qmetatype.h>
#include <qsequentialiterable.h>

#include <private/qqmlengine_p.h>

#ifdef V4_USE_VALGRIND
#include <valgrind/memcheck.h>
#endif

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(disableDiskCache, QML_DISABLE_DISK_CACHE);
DEFINE_BOOL_CONFIG_OPTION(forceDiskCache, QML_FORCE_DISK_CACHE);

using namespace QV4;

// While engineSerial is odd the statics haven't been initialized. The engine that receives ID 1
// initializes the statics and sets engineSerial to 2 afterwards.
// Each engine does engineSerial.fetchAndAddOrdered(2) on creation. Therefore engineSerial stays
// odd while the statics are being initialized, and stays even afterwards.
// Any further engines created while the statics are being initialized busy-wait until engineSerial
// is even.
static QBasicAtomicInt engineSerial = Q_BASIC_ATOMIC_INITIALIZER(1);
int ExecutionEngine::s_maxCallDepth = -1;
int ExecutionEngine::s_jitCallCountThreshold = 3;
int ExecutionEngine::s_maxJSStackSize = 4 * 1024 * 1024;
int ExecutionEngine::s_maxGCStackSize = 2 * 1024 * 1024;

ReturnedValue throwTypeError(const FunctionObject *b, const QV4::Value *, const QV4::Value *, int)
{
    return b->engine()->throwTypeError();
}


template <typename ReturnType>
ReturnType convertJSValueToVariantType(const QJSValue &value)
{
    const QVariant variant = value.toVariant();
    return variant.metaType() == QMetaType::fromType<QJSValue>()
            ? ReturnType()
            : variant.value<ReturnType>();
}

struct JSArrayIterator {
    QJSValue const* data;
    quint32 index;
};

namespace  {
void createNewIteratorIfNonExisting(void **iterator) {
    if (*iterator == nullptr)
        *iterator = new JSArrayIterator;
}
}

static QtMetaContainerPrivate::QMetaSequenceInterface emptySequenceInterface()
{
    // set up some functions so that non-array QSequentialIterables do not crash
    // but instead appear as an empty sequence

    using namespace QtMetaContainerPrivate;
    QMetaSequenceInterface iface;
    iface.sizeFn = [](const void *) { return qsizetype(0); };
    iface.valueAtIndexFn = [](const void *, qsizetype, void *) {};
    iface.createIteratorFn = [](void *, QMetaSequenceInterface::Position) -> void * {
        return nullptr;
    };
    iface.advanceIteratorFn = [](void *, qsizetype) {};
    iface.compareIteratorFn = [](const void *, const void *) {
        return true; /*all iterators are nullptr*/
    };
    iface.destroyIteratorFn = [](const void *) {};
    iface.copyIteratorFn = [](void *, const void *) {};
    iface.diffIteratorFn = [](const void *, const void *) { return qsizetype(0); };
    return iface;
}

static QtMetaContainerPrivate::QMetaSequenceInterface sequenceInterface()
{
    using namespace QtMetaContainerPrivate;
    QMetaSequenceInterface iface;
    iface.valueMetaType = QtPrivate::qMetaTypeInterfaceForType<QVariant>();
    iface.iteratorCapabilities = RandomAccessCapability | BiDirectionalCapability | ForwardCapability;
    iface.addRemoveCapabilities = CanAddAtEnd;
    iface.sizeFn = [](const void *p) -> qsizetype {
        return static_cast<QJSValue const *>(p)->property(QString::fromLatin1("length")).toInt();
    };

    /* Lifetime management notes:
     * valueAtIndexFn and valueAtIteratorFn return a pointer to a JSValue allocated via
     * QMetaType::create Because we set QVariantConstructionFlags::ShouldDeleteVariantData,
     * QSequentialIterable::at and QSequentialIterable::operator*() will free that memory
     */

    iface.valueAtIndexFn = [](const void *iterable, qsizetype index, void *dataPtr) -> void {
        auto *data = static_cast<QVariant *>(dataPtr);
        *data = static_cast<QJSValue const *>(iterable)->property(quint32(index)).toVariant();
    };
    iface.createIteratorFn = [](void *iterable, QMetaSequenceInterface::Position pos) {
        void *iterator = nullptr;
        createNewIteratorIfNonExisting(&iterator);
        auto jsArrayIterator = static_cast<JSArrayIterator *>(iterator);
        jsArrayIterator->index = 0;
        jsArrayIterator->data = reinterpret_cast<QJSValue const*>(iterable);
        if (pos == QMetaSequenceInterface::AtEnd) {
            auto length = static_cast<QJSValue const *>(iterable)->property(
                        QString::fromLatin1("length")).toInt();
            jsArrayIterator->index = quint32(length);
        }
        return iterator;
    };
    iface.createConstIteratorFn = [](const void *iterable, QMetaSequenceInterface::Position pos) {
        void *iterator = nullptr;
        createNewIteratorIfNonExisting(&iterator);
        auto jsArrayIterator = static_cast<JSArrayIterator *>(iterator);
        jsArrayIterator->index = 0;
        jsArrayIterator->data = reinterpret_cast<QJSValue const*>(iterable);
        if (pos == QMetaSequenceInterface::AtEnd) {
            auto length = static_cast<QJSValue const *>(iterable)->property(
                        QString::fromLatin1("length")).toInt();
            jsArrayIterator->index = quint32(length);
        }
        return iterator;
    };
    iface.advanceIteratorFn = [](void *iterator, qsizetype advanceBy) {
        static_cast<JSArrayIterator *>(iterator)->index += quint32(advanceBy);
    };
    iface.advanceConstIteratorFn = [](void *iterator, qsizetype advanceBy) {
        static_cast<JSArrayIterator *>(iterator)->index += quint32(advanceBy);
    };
    iface.valueAtIteratorFn = [](const void *iterator, void *dataPtr) -> void {
        const auto *arrayIterator = static_cast<const JSArrayIterator *>(iterator);
        const QJSValue *jsArray = arrayIterator->data;
        auto *data = static_cast<QVariant *>(dataPtr);
        *data = jsArray->property(arrayIterator->index).toVariant();
    };
    iface.valueAtConstIteratorFn = [](const void *iterator, void *dataPtr) -> void {
        const auto *arrayIterator = static_cast<const JSArrayIterator *>(iterator);
        const QJSValue *jsArray = arrayIterator->data;
        auto *data = static_cast<QVariant *>(dataPtr);
        *data = jsArray->property(arrayIterator->index).toVariant();
    };
    iface.destroyIteratorFn = [](const void *iterator) {
        delete static_cast<const JSArrayIterator *>(iterator);
    };
    iface.destroyConstIteratorFn = [](const void *iterator) {
        delete static_cast<const JSArrayIterator *>(iterator);
    };
    iface.compareIteratorFn = [](const void *p, const void *other) {
        auto this_ = static_cast<const JSArrayIterator *>(p);
        auto that_ = static_cast<const JSArrayIterator *>(other);
        return this_->index == that_->index && this_->data == that_->data;
    };
    iface.compareConstIteratorFn = [](const void *p, const void *other) {
        auto this_ = static_cast<const JSArrayIterator *>(p);
        auto that_ = static_cast<const JSArrayIterator *>(other);
        return this_->index == that_->index && this_->data == that_->data;
    };
    iface.copyIteratorFn = [](void *iterator, const void *otherIterator) {
        auto *otherIter = (static_cast<JSArrayIterator const *>(otherIterator));
        static_cast<JSArrayIterator *>(iterator)->index = otherIter->index;
        static_cast<JSArrayIterator *>(iterator)->data = otherIter->data;
    };
    iface.copyConstIteratorFn = [](void *iterator, const void *otherIterator) {
        auto *otherIter = (static_cast<JSArrayIterator const *>(otherIterator));
        static_cast<JSArrayIterator *>(iterator)->index = otherIter->index;
        static_cast<JSArrayIterator *>(iterator)->data = otherIter->data;
    };
    iface.diffIteratorFn = [](const void *iterator, const void *otherIterator) -> qsizetype {
        const auto *self = static_cast<const JSArrayIterator *>(iterator);
        const auto *other = static_cast<const JSArrayIterator *>(otherIterator);
        return self->index - other->index;
    };
    iface.diffConstIteratorFn = [](const void *iterator, const void *otherIterator) -> qsizetype {
        const auto *self = static_cast<const JSArrayIterator *>(iterator);
        const auto *other = static_cast<const JSArrayIterator *>(otherIterator);
        return self->index - other->index;
    };
    iface.addValueFn = [](void *iterable, const void *data, QMetaSequenceInterface::Position) {
        auto *jsvalue = static_cast<QJSValue *>(iterable);
        QV4::Scope scope(QJSValuePrivate::engine(jsvalue));
        QV4::ScopedArrayObject a(scope, QJSValuePrivate::asManagedType<QV4::ArrayObject>(jsvalue));
        QV4::ScopedValue v(scope, scope.engine->fromVariant(*static_cast<const QVariant *>(data)));
        if (!a)
            return;
        int len = a->getLength();
        a->setIndexed(len, v, QV4::Object::DoNotThrow);
    };
    return iface;
}

static QSequentialIterable jsvalueToSequence (const QJSValue& value) {
    using namespace QtMetaTypePrivate;
    using namespace QtMetaContainerPrivate;


    if (!value.isArray()) {
        static QMetaSequenceInterface emptySequence = emptySequenceInterface();
        return QSequentialIterable(QMetaSequence(&emptySequence), nullptr);
    }

    static QMetaSequenceInterface sequence = sequenceInterface();
    return QSequentialIterable(QMetaSequence(&sequence), &value);
}

void ExecutionEngine::initializeStaticMembers()
{
    bool ok = false;

    const int envMaxJSStackSize = qEnvironmentVariableIntValue("QV4_JS_MAX_STACK_SIZE", &ok);
    if (ok && envMaxJSStackSize > 0)
        s_maxJSStackSize = envMaxJSStackSize;

    const int envMaxGCStackSize = qEnvironmentVariableIntValue("QV4_GC_MAX_STACK_SIZE", &ok);
    if (ok && envMaxGCStackSize > 0)
        s_maxGCStackSize = envMaxGCStackSize;

    if (qEnvironmentVariableIsSet("QV4_CRASH_ON_STACKOVERFLOW")) {
        s_maxCallDepth = std::numeric_limits<qint32>::max();
    } else {
        ok = false;
        s_maxCallDepth = qEnvironmentVariableIntValue("QV4_MAX_CALL_DEPTH", &ok);
        if (!ok || s_maxCallDepth <= 0)
            s_maxCallDepth = -1;
    }

    ok = false;
    s_jitCallCountThreshold = qEnvironmentVariableIntValue("QV4_JIT_CALL_THRESHOLD", &ok);
    if (!ok)
        s_jitCallCountThreshold = 3;
    if (qEnvironmentVariableIsSet("QV4_FORCE_INTERPRETER"))
        s_jitCallCountThreshold = std::numeric_limits<int>::max();

    qMetaTypeId<QJSValue>();
    qMetaTypeId<QList<int> >();

    if (!QMetaType::hasRegisteredConverterFunction<QJSValue, QVariantMap>())
        QMetaType::registerConverter<QJSValue, QVariantMap>(convertJSValueToVariantType<QVariantMap>);
    if (!QMetaType::hasRegisteredConverterFunction<QJSValue, QVariantList>())
        QMetaType::registerConverter<QJSValue, QVariantList>(convertJSValueToVariantType<QVariantList>);
    if (!QMetaType::hasRegisteredConverterFunction<QJSValue, QStringList>())
        QMetaType::registerConverter<QJSValue, QStringList>(convertJSValueToVariantType<QStringList>);
    if (!QMetaType::hasRegisteredConverterFunction<QJSValue, QSequentialIterable>())
        QMetaType::registerConverter<QJSValue, QSequentialIterable>(jsvalueToSequence);
}

ExecutionEngine::ExecutionEngine(QJSEngine *jsEngine)
    : executableAllocator(new QV4::ExecutableAllocator)
    , regExpAllocator(new QV4::ExecutableAllocator)
    , bumperPointerAllocator(new WTF::BumpPointerAllocator)
    , jsStack(new WTF::PageAllocation)
    , gcStack(new WTF::PageAllocation)
    , globalCode(nullptr)
    , publicEngine(jsEngine)
    , m_engineId(engineSerial.fetchAndAddOrdered(2))
    , regExpCache(nullptr)
    , m_multiplyWrappedQObjects(nullptr)
#if QT_CONFIG(qml_jit)
    , m_canAllocateExecutableMemory(OSAllocator::canAllocateExecutableMemory())
#endif
#if QT_CONFIG(qml_xml_http_request)
    , m_xmlHttpRequestData(nullptr)
#endif
    , m_qmlEngine(nullptr)
{
    if (m_engineId == 1) {
        initializeStaticMembers();
        engineSerial.storeRelease(2); // make it even
    } else if (Q_UNLIKELY(m_engineId & 1)) {
        // This should be rare. You usually don't create lots of engines at the same time.
        while (engineSerial.loadAcquire() & 1) {
            QThread::yieldCurrentThread();
        }
    }

    if (s_maxCallDepth < 0) {
        const StackProperties stack = stackProperties();
        cppStackBase = stack.base;
        cppStackLimit = stack.softLimit;
    } else {
        callDepth = 0;
    }

    // We allocate guard pages around our stacks.
    const size_t guardPages = 2 * WTF::pageSize();

    memoryManager = new QV4::MemoryManager(this);
    // reserve space for the JS stack
    // we allow it to grow to a bit more than m_maxJSStackSize, as we can overshoot due to ScopedValues
    // allocated outside of JIT'ed methods.
    *jsStack = WTF::PageAllocation::allocate(
                s_maxJSStackSize + 256*1024 + guardPages, WTF::OSAllocator::JSVMStackPages,
                /* writable */ true, /* executable */ false, /* includesGuardPages */ true);
    jsStackBase = (Value *)jsStack->base();
#ifdef V4_USE_VALGRIND
    VALGRIND_MAKE_MEM_UNDEFINED(jsStackBase, m_maxJSStackSize + 256*1024);
#endif

    jsStackTop = jsStackBase;

    *gcStack = WTF::PageAllocation::allocate(
                s_maxGCStackSize + guardPages, WTF::OSAllocator::JSVMStackPages,
                /* writable */ true, /* executable */ false, /* includesGuardPages */ true);

    exceptionValue = jsAlloca(1);
    *exceptionValue = Encode::undefined();
    globalObject = static_cast<Object *>(jsAlloca(1));
    jsObjects = jsAlloca(NJSObjects);
    typedArrayPrototype = static_cast<Object *>(jsAlloca(NTypedArrayTypes));
    typedArrayCtors = static_cast<FunctionObject *>(jsAlloca(NTypedArrayTypes));
    jsStrings = jsAlloca(NJSStrings);
    jsSymbols = jsAlloca(NJSSymbols);

    // set up stack limits
    jsStackLimit = jsStackBase + s_maxJSStackSize/sizeof(Value);

    identifierTable = new IdentifierTable(this);

    memset(classes, 0, sizeof(classes));
    classes[Class_Empty] = memoryManager->allocIC<InternalClass>();
    classes[Class_Empty]->init(this);

    classes[Class_MemberData] = classes[Class_Empty]->changeVTable(QV4::MemberData::staticVTable());
    classes[Class_SimpleArrayData] = classes[Class_Empty]->changeVTable(QV4::SimpleArrayData::staticVTable());
    classes[Class_SparseArrayData] = classes[Class_Empty]->changeVTable(QV4::SparseArrayData::staticVTable());
    classes[Class_ExecutionContext] = classes[Class_Empty]->changeVTable(QV4::ExecutionContext::staticVTable());
    classes[Class_CallContext] = classes[Class_Empty]->changeVTable(QV4::CallContext::staticVTable());
    classes[Class_QmlContext] = classes[Class_Empty]->changeVTable(QV4::QmlContext::staticVTable());

    Scope scope(this);
    Scoped<InternalClass> ic(scope);
    ic = classes[Class_Empty]->changeVTable(QV4::Object::staticVTable());
    jsObjects[ObjectProto] = memoryManager->allocObject<ObjectPrototype>(ic->d());
    classes[Class_Object] = ic->changePrototype(objectPrototype()->d());
    classes[Class_QmlContextWrapper] = classes[Class_Object]->changeVTable(QV4::QQmlContextWrapper::staticVTable());

    ic = newInternalClass(QV4::StringObject::staticVTable(), objectPrototype());
    jsObjects[StringProto] = memoryManager->allocObject<StringPrototype>(ic->d(), /*init =*/ false);
    classes[Class_String] = classes[Class_Empty]->changeVTable(QV4::String::staticVTable())->changePrototype(stringPrototype()->d());
    Q_ASSERT(stringPrototype()->d() && classes[Class_String]->prototype);

    jsObjects[SymbolProto] = memoryManager->allocate<SymbolPrototype>();
    classes[Class_Symbol] = classes[EngineBase::Class_Empty]->changeVTable(QV4::Symbol::staticVTable())->changePrototype(symbolPrototype()->d());

    jsStrings[String_Empty] = newIdentifier(QString());
    jsStrings[String_undefined] = newIdentifier(QStringLiteral("undefined"));
    jsStrings[String_null] = newIdentifier(QStringLiteral("null"));
    jsStrings[String_true] = newIdentifier(QStringLiteral("true"));
    jsStrings[String_false] = newIdentifier(QStringLiteral("false"));
    jsStrings[String_boolean] = newIdentifier(QStringLiteral("boolean"));
    jsStrings[String_number] = newIdentifier(QStringLiteral("number"));
    jsStrings[String_string] = newIdentifier(QStringLiteral("string"));
    jsStrings[String_default] = newIdentifier(QStringLiteral("default"));
    jsStrings[String_symbol] = newIdentifier(QStringLiteral("symbol"));
    jsStrings[String_object] = newIdentifier(QStringLiteral("object"));
    jsStrings[String_function] = newIdentifier(QStringLiteral("function"));
    jsStrings[String_length] = newIdentifier(QStringLiteral("length"));
    jsStrings[String_prototype] = newIdentifier(QStringLiteral("prototype"));
    jsStrings[String_constructor] = newIdentifier(QStringLiteral("constructor"));
    jsStrings[String_arguments] = newIdentifier(QStringLiteral("arguments"));
    jsStrings[String_caller] = newIdentifier(QStringLiteral("caller"));
    jsStrings[String_callee] = newIdentifier(QStringLiteral("callee"));
    jsStrings[String_this] = newIdentifier(QStringLiteral("this"));
    jsStrings[String___proto__] = newIdentifier(QStringLiteral("__proto__"));
    jsStrings[String_enumerable] = newIdentifier(QStringLiteral("enumerable"));
    jsStrings[String_configurable] = newIdentifier(QStringLiteral("configurable"));
    jsStrings[String_writable] = newIdentifier(QStringLiteral("writable"));
    jsStrings[String_value] = newIdentifier(QStringLiteral("value"));
    jsStrings[String_get] = newIdentifier(QStringLiteral("get"));
    jsStrings[String_set] = newIdentifier(QStringLiteral("set"));
    jsStrings[String_eval] = newIdentifier(QStringLiteral("eval"));
    jsStrings[String_uintMax] = newIdentifier(QStringLiteral("4294967295"));
    jsStrings[String_name] = newIdentifier(QStringLiteral("name"));
    jsStrings[String_index] = newIdentifier(QStringLiteral("index"));
    jsStrings[String_input] = newIdentifier(QStringLiteral("input"));
    jsStrings[String_toString] = newIdentifier(QStringLiteral("toString"));
    jsStrings[String_toLocaleString] = newIdentifier(QStringLiteral("toLocaleString"));
    jsStrings[String_destroy] = newIdentifier(QStringLiteral("destroy"));
    jsStrings[String_valueOf] = newIdentifier(QStringLiteral("valueOf"));
    jsStrings[String_byteLength] = newIdentifier(QStringLiteral("byteLength"));
    jsStrings[String_byteOffset] = newIdentifier(QStringLiteral("byteOffset"));
    jsStrings[String_buffer] = newIdentifier(QStringLiteral("buffer"));
    jsStrings[String_lastIndex] = newIdentifier(QStringLiteral("lastIndex"));
    jsStrings[String_next] = newIdentifier(QStringLiteral("next"));
    jsStrings[String_done] = newIdentifier(QStringLiteral("done"));
    jsStrings[String_return] = newIdentifier(QStringLiteral("return"));
    jsStrings[String_throw] = newIdentifier(QStringLiteral("throw"));
    jsStrings[String_global] = newIdentifier(QStringLiteral("global"));
    jsStrings[String_ignoreCase] = newIdentifier(QStringLiteral("ignoreCase"));
    jsStrings[String_multiline] = newIdentifier(QStringLiteral("multiline"));
    jsStrings[String_unicode] = newIdentifier(QStringLiteral("unicode"));
    jsStrings[String_sticky] = newIdentifier(QStringLiteral("sticky"));
    jsStrings[String_source] = newIdentifier(QStringLiteral("source"));
    jsStrings[String_flags] = newIdentifier(QStringLiteral("flags"));

    jsSymbols[Symbol_hasInstance] = Symbol::create(this, QStringLiteral("@Symbol.hasInstance"));
    jsSymbols[Symbol_isConcatSpreadable] = Symbol::create(this, QStringLiteral("@Symbol.isConcatSpreadable"));
    jsSymbols[Symbol_iterator] = Symbol::create(this, QStringLiteral("@Symbol.iterator"));
    jsSymbols[Symbol_match] = Symbol::create(this, QStringLiteral("@Symbol.match"));
    jsSymbols[Symbol_replace] = Symbol::create(this, QStringLiteral("@Symbol.replace"));
    jsSymbols[Symbol_search] = Symbol::create(this, QStringLiteral("@Symbol.search"));
    jsSymbols[Symbol_species] = Symbol::create(this, QStringLiteral("@Symbol.species"));
    jsSymbols[Symbol_split] = Symbol::create(this, QStringLiteral("@Symbol.split"));
    jsSymbols[Symbol_toPrimitive] = Symbol::create(this, QStringLiteral("@Symbol.toPrimitive"));
    jsSymbols[Symbol_toStringTag] = Symbol::create(this, QStringLiteral("@Symbol.toStringTag"));
    jsSymbols[Symbol_unscopables] = Symbol::create(this, QStringLiteral("@Symbol.unscopables"));
    jsSymbols[Symbol_revokableProxy] = Symbol::create(this, QStringLiteral("@Proxy.revokableProxy"));

    ic = newInternalClass(ArrayPrototype::staticVTable(), objectPrototype());
    Q_ASSERT(ic->d()->prototype);
    ic = ic->addMember(id_length()->propertyKey(), Attr_NotConfigurable|Attr_NotEnumerable);
    Q_ASSERT(ic->d()->prototype);
    jsObjects[ArrayProto] = memoryManager->allocObject<ArrayPrototype>(ic->d());
    classes[Class_ArrayObject] = ic->changePrototype(arrayPrototype()->d());
    jsObjects[PropertyListProto] = memoryManager->allocate<PropertyListPrototype>();

    Scoped<InternalClass> argsClass(scope);
    argsClass = newInternalClass(ArgumentsObject::staticVTable(), objectPrototype());
    argsClass = argsClass->addMember(id_length()->propertyKey(), Attr_NotEnumerable);
    argsClass = argsClass->addMember(symbol_iterator()->propertyKey(), Attr_Data|Attr_NotEnumerable);
    classes[Class_ArgumentsObject] = argsClass->addMember(id_callee()->propertyKey(), Attr_Data|Attr_NotEnumerable);
    argsClass = newInternalClass(StrictArgumentsObject::staticVTable(), objectPrototype());
    argsClass = argsClass->addMember(id_length()->propertyKey(), Attr_NotEnumerable);
    argsClass = argsClass->addMember(symbol_iterator()->propertyKey(), Attr_Data|Attr_NotEnumerable);
    classes[Class_StrictArgumentsObject] = argsClass->addMember(id_callee()->propertyKey(), Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);

    *static_cast<Value *>(globalObject) = newObject();
    Q_ASSERT(globalObject->d()->vtable());
    initRootContext();

    ic = newInternalClass(QV4::StringObject::staticVTable(), objectPrototype());
    ic = ic->addMember(id_length()->propertyKey(), Attr_ReadOnly);
    classes[Class_StringObject] = ic->changePrototype(stringPrototype()->d());
    Q_ASSERT(classes[Class_StringObject]->verifyIndex(id_length()->propertyKey(), Heap::StringObject::LengthPropertyIndex));

    classes[Class_SymbolObject] = newInternalClass(QV4::SymbolObject::staticVTable(), symbolPrototype());

    jsObjects[NumberProto] = memoryManager->allocate<NumberPrototype>();
    jsObjects[BooleanProto] = memoryManager->allocate<BooleanPrototype>();
    jsObjects[DateProto] = memoryManager->allocate<DatePrototype>();

#if defined(QT_NO_DEBUG) && !defined(QT_FORCE_ASSERTS)
    InternalClassEntry *index = nullptr;
#else
    InternalClassEntry _index;
    auto *index = &_index;
#endif
    ic = newInternalClass(QV4::FunctionPrototype::staticVTable(), objectPrototype());
    auto addProtoHasInstance = [&] {
        // Add an invalid prototype slot, so that all function objects have the same layout
        // This helps speed up instanceof operations and other things where we need to query
        // prototype property (as we always know it's location)
        ic = ic->addMember(id_prototype()->propertyKey(), Attr_Invalid, index);
        Q_ASSERT(index->index == Heap::FunctionObject::Index_Prototype);
        // add an invalid @hasInstance slot, so that we can quickly track whether the
        // hasInstance method has been reimplemented. This is required for a fast
        // instanceof implementation
        ic = ic->addMember(symbol_hasInstance()->propertyKey(), Attr_Invalid, index);
        Q_ASSERT(index->index == Heap::FunctionObject::Index_HasInstance);
    };
    addProtoHasInstance();
    jsObjects[FunctionProto] = memoryManager->allocObject<FunctionPrototype>(ic->d());
    ic = newInternalClass(FunctionObject::staticVTable(), functionPrototype());
    addProtoHasInstance();
    classes[Class_FunctionObject] = ic->d();
    ic = ic->addMember(id_name()->propertyKey(), Attr_ReadOnly, index);
    Q_ASSERT(index->index == Heap::ArrowFunction::Index_Name);
    ic = ic->addMember(id_length()->propertyKey(), Attr_ReadOnly_ButConfigurable, index);
    Q_ASSERT(index->index == Heap::ArrowFunction::Index_Length);
    classes[Class_ArrowFunction] = ic->changeVTable(ArrowFunction::staticVTable());
    ic = ic->changeVTable(MemberFunction::staticVTable());
    classes[Class_MemberFunction] = ic->d();
    ic = ic->changeVTable(GeneratorFunction::staticVTable());
    classes[Class_GeneratorFunction] = ic->d();
    ic = ic->changeVTable(MemberGeneratorFunction::staticVTable());
    classes[Class_MemberGeneratorFunction] = ic->d();

    ic = ic->changeMember(id_prototype()->propertyKey(), Attr_NotConfigurable|Attr_NotEnumerable);
    ic = ic->changeVTable(ScriptFunction::staticVTable());
    classes[Class_ScriptFunction] = ic->d();
    ic = ic->changeVTable(ConstructorFunction::staticVTable());
    classes[Class_ConstructorFunction] = ic->d();

    classes[Class_ObjectProto] = classes[Class_Object]->addMember(id_constructor()->propertyKey(), Attr_NotEnumerable, index);
    Q_ASSERT(index->index == Heap::FunctionObject::Index_ProtoConstructor);

    jsObjects[GeneratorProto] = memoryManager->allocObject<GeneratorPrototype>(classes[Class_Object]);
    classes[Class_GeneratorObject] = newInternalClass(QV4::GeneratorObject::staticVTable(), generatorPrototype());

    ScopedString str(scope);
    classes[Class_RegExp] = classes[Class_Empty]->changeVTable(QV4::RegExp::staticVTable());
    ic = newInternalClass(QV4::RegExpObject::staticVTable(), objectPrototype());
    ic = ic->addMember(id_lastIndex()->propertyKey(), Attr_NotEnumerable|Attr_NotConfigurable, index);
    Q_ASSERT(index->index == RegExpObject::Index_LastIndex);
    jsObjects[RegExpProto] = memoryManager->allocObject<RegExpPrototype>(classes[Class_Object]);
    classes[Class_RegExpObject] = ic->changePrototype(regExpPrototype()->d());

    ic = classes[Class_ArrayObject]->addMember(id_index()->propertyKey(), Attr_Data, index);
    Q_ASSERT(index->index == RegExpObject::Index_ArrayIndex);
    classes[Class_RegExpExecArray] = ic->addMember(id_input()->propertyKey(), Attr_Data, index);
    Q_ASSERT(index->index == RegExpObject::Index_ArrayInput);

    ic = newInternalClass(ErrorObject::staticVTable(), nullptr);
    ic = ic->addMember((str = newIdentifier(QStringLiteral("stack")))->propertyKey(), Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable, index);
    Q_ASSERT(index->index == ErrorObject::Index_Stack);
    Q_ASSERT(index->setterIndex == ErrorObject::Index_StackSetter);
    ic = ic->addMember((str = newIdentifier(QStringLiteral("fileName")))->propertyKey(), Attr_Data|Attr_NotEnumerable, index);
    Q_ASSERT(index->index == ErrorObject::Index_FileName);
    ic = ic->addMember((str = newIdentifier(QStringLiteral("lineNumber")))->propertyKey(), Attr_Data|Attr_NotEnumerable, index);
    classes[Class_ErrorObject] = ic->d();
    Q_ASSERT(index->index == ErrorObject::Index_LineNumber);
    classes[Class_ErrorObjectWithMessage] = ic->addMember((str = newIdentifier(QStringLiteral("message")))->propertyKey(), Attr_Data|Attr_NotEnumerable, index);
    Q_ASSERT(index->index == ErrorObject::Index_Message);
    ic = newInternalClass(Object::staticVTable(), objectPrototype());
    ic = ic->addMember(id_constructor()->propertyKey(), Attr_Data|Attr_NotEnumerable, index);
    Q_ASSERT(index->index == ErrorPrototype::Index_Constructor);
    ic = ic->addMember((str = newIdentifier(QStringLiteral("message")))->propertyKey(), Attr_Data|Attr_NotEnumerable, index);
    Q_ASSERT(index->index == ErrorPrototype::Index_Message);
    classes[Class_ErrorProto] = ic->addMember(id_name()->propertyKey(), Attr_Data|Attr_NotEnumerable, index);
    Q_ASSERT(index->index == ErrorPrototype::Index_Name);

    classes[Class_ProxyObject] = classes[Class_Empty]->changeVTable(ProxyObject::staticVTable());
    classes[Class_ProxyFunctionObject] = classes[Class_Empty]->changeVTable(ProxyFunctionObject::staticVTable());

    jsObjects[GetStack_Function] = FunctionObject::createBuiltinFunction(this, str = newIdentifier(QStringLiteral("stack")), ErrorObject::method_get_stack, 0);

    jsObjects[ErrorProto] = memoryManager->allocObject<ErrorPrototype>(classes[Class_ErrorProto]);
    ic = classes[Class_ErrorProto]->changePrototype(errorPrototype()->d());
    jsObjects[EvalErrorProto] = memoryManager->allocObject<EvalErrorPrototype>(ic->d());
    jsObjects[RangeErrorProto] = memoryManager->allocObject<RangeErrorPrototype>(ic->d());
    jsObjects[ReferenceErrorProto] = memoryManager->allocObject<ReferenceErrorPrototype>(ic->d());
    jsObjects[SyntaxErrorProto] = memoryManager->allocObject<SyntaxErrorPrototype>(ic->d());
    jsObjects[TypeErrorProto] = memoryManager->allocObject<TypeErrorPrototype>(ic->d());
    jsObjects[URIErrorProto] = memoryManager->allocObject<URIErrorPrototype>(ic->d());

    jsObjects[VariantProto] = memoryManager->allocate<VariantPrototype>();
    Q_ASSERT(variantPrototype()->getPrototypeOf() == objectPrototype()->d());

    ic = newInternalClass(SequencePrototype::staticVTable(), SequencePrototype::defaultPrototype(this));
    jsObjects[SequenceProto] = ScopedValue(scope, memoryManager->allocObject<SequencePrototype>(ic->d()));

    ExecutionContext *global = rootContext();

    jsObjects[Object_Ctor] = memoryManager->allocate<ObjectCtor>(global);
    jsObjects[String_Ctor] = memoryManager->allocate<StringCtor>(global);
    jsObjects[Symbol_Ctor] = memoryManager->allocate<SymbolCtor>(global);
    jsObjects[Number_Ctor] = memoryManager->allocate<NumberCtor>(global);
    jsObjects[Boolean_Ctor] = memoryManager->allocate<BooleanCtor>(global);
    jsObjects[Array_Ctor] = memoryManager->allocate<ArrayCtor>(global);
    jsObjects[Function_Ctor] = memoryManager->allocate<FunctionCtor>(global);
    jsObjects[GeneratorFunction_Ctor] = memoryManager->allocate<GeneratorFunctionCtor>(global);
    jsObjects[Date_Ctor] = memoryManager->allocate<DateCtor>(global);
    jsObjects[RegExp_Ctor] = memoryManager->allocate<RegExpCtor>(global);
    jsObjects[Error_Ctor] = memoryManager->allocate<ErrorCtor>(global);
    jsObjects[EvalError_Ctor] = memoryManager->allocate<EvalErrorCtor>(global);
    jsObjects[RangeError_Ctor] = memoryManager->allocate<RangeErrorCtor>(global);
    jsObjects[ReferenceError_Ctor] = memoryManager->allocate<ReferenceErrorCtor>(global);
    jsObjects[SyntaxError_Ctor] = memoryManager->allocate<SyntaxErrorCtor>(global);
    jsObjects[TypeError_Ctor] = memoryManager->allocate<TypeErrorCtor>(global);
    jsObjects[URIError_Ctor] = memoryManager->allocate<URIErrorCtor>(global);
    jsObjects[IteratorProto] = memoryManager->allocate<IteratorPrototype>();

    ic = newInternalClass(ForInIteratorPrototype::staticVTable(), iteratorPrototype());
    jsObjects[ForInIteratorProto] = memoryManager->allocObject<ForInIteratorPrototype>(ic);
    ic = newInternalClass(SetIteratorPrototype::staticVTable(), iteratorPrototype());
    jsObjects[MapIteratorProto] = memoryManager->allocObject<MapIteratorPrototype>(ic);
    ic = newInternalClass(SetIteratorPrototype::staticVTable(), iteratorPrototype());
    jsObjects[SetIteratorProto] = memoryManager->allocObject<SetIteratorPrototype>(ic);
    ic = newInternalClass(ArrayIteratorPrototype::staticVTable(), iteratorPrototype());
    jsObjects[ArrayIteratorProto] = memoryManager->allocObject<ArrayIteratorPrototype>(ic);
    ic = newInternalClass(StringIteratorPrototype::staticVTable(), iteratorPrototype());
    jsObjects[StringIteratorProto] = memoryManager->allocObject<StringIteratorPrototype>(ic);

    //
    // url
    //

    jsObjects[Url_Ctor] = memoryManager->allocate<UrlCtor>(global);
    jsObjects[UrlProto] = memoryManager->allocate<UrlPrototype>();
    jsObjects[UrlSearchParams_Ctor] = memoryManager->allocate<UrlSearchParamsCtor>(global);
    jsObjects[UrlSearchParamsProto] = memoryManager->allocate<UrlSearchParamsPrototype>();

    str = newString(QStringLiteral("get [Symbol.species]"));
    jsObjects[GetSymbolSpecies] = FunctionObject::createBuiltinFunction(this, str, ArrayPrototype::method_get_species, 0);

    static_cast<ObjectPrototype *>(objectPrototype())->init(this, objectCtor());
    static_cast<StringPrototype *>(stringPrototype())->init(this, stringCtor());
    static_cast<SymbolPrototype *>(symbolPrototype())->init(this, symbolCtor());
    static_cast<NumberPrototype *>(numberPrototype())->init(this, numberCtor());
    static_cast<BooleanPrototype *>(booleanPrototype())->init(this, booleanCtor());
    static_cast<ArrayPrototype *>(arrayPrototype())->init(this, arrayCtor());
    static_cast<PropertyListPrototype *>(propertyListPrototype())->init();
    static_cast<DatePrototype *>(datePrototype())->init(this, dateCtor());
    static_cast<FunctionPrototype *>(functionPrototype())->init(this, functionCtor());
    static_cast<GeneratorPrototype *>(generatorPrototype())->init(this, generatorFunctionCtor());
    static_cast<RegExpPrototype *>(regExpPrototype())->init(this, regExpCtor());
    static_cast<ErrorPrototype *>(errorPrototype())->init(this, errorCtor());
    static_cast<EvalErrorPrototype *>(evalErrorPrototype())->init(this, evalErrorCtor());
    static_cast<RangeErrorPrototype *>(rangeErrorPrototype())->init(this, rangeErrorCtor());
    static_cast<ReferenceErrorPrototype *>(referenceErrorPrototype())->init(this, referenceErrorCtor());
    static_cast<SyntaxErrorPrototype *>(syntaxErrorPrototype())->init(this, syntaxErrorCtor());
    static_cast<TypeErrorPrototype *>(typeErrorPrototype())->init(this, typeErrorCtor());
    static_cast<URIErrorPrototype *>(uRIErrorPrototype())->init(this, uRIErrorCtor());
    static_cast<UrlPrototype *>(urlPrototype())->init(this, urlCtor());
    static_cast<UrlSearchParamsPrototype *>(urlSearchParamsPrototype())->init(this, urlSearchParamsCtor());

    static_cast<IteratorPrototype *>(iteratorPrototype())->init(this);
    static_cast<ForInIteratorPrototype *>(forInIteratorPrototype())->init(this);
    static_cast<MapIteratorPrototype *>(mapIteratorPrototype())->init(this);
    static_cast<SetIteratorPrototype *>(setIteratorPrototype())->init(this);
    static_cast<ArrayIteratorPrototype *>(arrayIteratorPrototype())->init(this);
    static_cast<StringIteratorPrototype *>(stringIteratorPrototype())->init(this);

    static_cast<VariantPrototype *>(variantPrototype())->init();

    sequencePrototype()->cast<SequencePrototype>()->init();

    jsObjects[WeakMap_Ctor] = memoryManager->allocate<WeakMapCtor>(global);
    jsObjects[WeakMapProto] = memoryManager->allocate<WeakMapPrototype>();
    static_cast<WeakMapPrototype *>(weakMapPrototype())->init(this, weakMapCtor());

    jsObjects[Map_Ctor] = memoryManager->allocate<MapCtor>(global);
    jsObjects[MapProto] = memoryManager->allocate<MapPrototype>();
    static_cast<MapPrototype *>(mapPrototype())->init(this, mapCtor());

    jsObjects[WeakSet_Ctor] = memoryManager->allocate<WeakSetCtor>(global);
    jsObjects[WeakSetProto] = memoryManager->allocate<WeakSetPrototype>();
    static_cast<WeakSetPrototype *>(weakSetPrototype())->init(this, weakSetCtor());

    jsObjects[Set_Ctor] = memoryManager->allocate<SetCtor>(global);
    jsObjects[SetProto] = memoryManager->allocate<SetPrototype>();
    static_cast<SetPrototype *>(setPrototype())->init(this, setCtor());

    //
    // promises
    //

    jsObjects[Promise_Ctor] = memoryManager->allocate<PromiseCtor>(global);
    jsObjects[PromiseProto] = memoryManager->allocate<PromisePrototype>();
    static_cast<PromisePrototype *>(promisePrototype())->init(this, promiseCtor());

    // typed arrays

    jsObjects[SharedArrayBuffer_Ctor] = memoryManager->allocate<SharedArrayBufferCtor>(global);
    jsObjects[SharedArrayBufferProto] = memoryManager->allocate<SharedArrayBufferPrototype>();
    static_cast<SharedArrayBufferPrototype *>(sharedArrayBufferPrototype())->init(this, sharedArrayBufferCtor());

    jsObjects[ArrayBuffer_Ctor] = memoryManager->allocate<ArrayBufferCtor>(global);
    jsObjects[ArrayBufferProto] = memoryManager->allocate<ArrayBufferPrototype>();
    static_cast<ArrayBufferPrototype *>(arrayBufferPrototype())->init(this, arrayBufferCtor());

    jsObjects[DataView_Ctor] = memoryManager->allocate<DataViewCtor>(global);
    jsObjects[DataViewProto] = memoryManager->allocate<DataViewPrototype>();
    static_cast<DataViewPrototype *>(dataViewPrototype())->init(this, dataViewCtor());
    jsObjects[ValueTypeProto] = (Heap::Base *) nullptr;
    jsObjects[SignalHandlerProto] = (Heap::Base *) nullptr;

    jsObjects[IntrinsicTypedArray_Ctor] = memoryManager->allocate<IntrinsicTypedArrayCtor>(global);
    jsObjects[IntrinsicTypedArrayProto] = memoryManager->allocate<IntrinsicTypedArrayPrototype>();
    static_cast<IntrinsicTypedArrayPrototype *>(intrinsicTypedArrayPrototype())
            ->init(this, static_cast<IntrinsicTypedArrayCtor *>(intrinsicTypedArrayCtor()));

    for (int i = 0; i < NTypedArrayTypes; ++i) {
        static_cast<Value &>(typedArrayCtors[i]) = memoryManager->allocate<TypedArrayCtor>(global, Heap::TypedArray::Type(i));
        static_cast<Value &>(typedArrayPrototype[i]) = memoryManager->allocate<TypedArrayPrototype>(Heap::TypedArray::Type(i));
        typedArrayPrototype[i].as<TypedArrayPrototype>()->init(this, static_cast<TypedArrayCtor *>(typedArrayCtors[i].as<Object>()));
    }

    //
    // set up the global object
    //
    rootContext()->d()->activation.set(scope.engine, globalObject->d());
    Q_ASSERT(globalObject->d()->vtable());

    globalObject->defineDefaultProperty(QStringLiteral("Object"), *objectCtor());
    globalObject->defineDefaultProperty(QStringLiteral("String"), *stringCtor());
    globalObject->defineDefaultProperty(QStringLiteral("Symbol"), *symbolCtor());
    FunctionObject *numberObject = numberCtor();
    globalObject->defineDefaultProperty(QStringLiteral("Number"), *numberObject);
    globalObject->defineDefaultProperty(QStringLiteral("Boolean"), *booleanCtor());
    globalObject->defineDefaultProperty(QStringLiteral("Array"), *arrayCtor());
    globalObject->defineDefaultProperty(QStringLiteral("Function"), *functionCtor());
    globalObject->defineDefaultProperty(QStringLiteral("Date"), *dateCtor());
    globalObject->defineDefaultProperty(QStringLiteral("RegExp"), *regExpCtor());
    globalObject->defineDefaultProperty(QStringLiteral("Error"), *errorCtor());
    globalObject->defineDefaultProperty(QStringLiteral("EvalError"), *evalErrorCtor());
    globalObject->defineDefaultProperty(QStringLiteral("RangeError"), *rangeErrorCtor());
    globalObject->defineDefaultProperty(QStringLiteral("ReferenceError"), *referenceErrorCtor());
    globalObject->defineDefaultProperty(QStringLiteral("SyntaxError"), *syntaxErrorCtor());
    globalObject->defineDefaultProperty(QStringLiteral("TypeError"), *typeErrorCtor());
    globalObject->defineDefaultProperty(QStringLiteral("URIError"), *uRIErrorCtor());
    globalObject->defineDefaultProperty(QStringLiteral("Promise"), *promiseCtor());
    globalObject->defineDefaultProperty(QStringLiteral("URL"), *urlCtor());
    globalObject->defineDefaultProperty(QStringLiteral("URLSearchParams"), *urlSearchParamsCtor());

    globalObject->defineDefaultProperty(QStringLiteral("SharedArrayBuffer"), *sharedArrayBufferCtor());
    globalObject->defineDefaultProperty(QStringLiteral("ArrayBuffer"), *arrayBufferCtor());
    globalObject->defineDefaultProperty(QStringLiteral("DataView"), *dataViewCtor());
    globalObject->defineDefaultProperty(QStringLiteral("WeakSet"), *weakSetCtor());
    globalObject->defineDefaultProperty(QStringLiteral("Set"), *setCtor());
    globalObject->defineDefaultProperty(QStringLiteral("WeakMap"), *weakMapCtor());
    globalObject->defineDefaultProperty(QStringLiteral("Map"), *mapCtor());

    for (int i = 0; i < NTypedArrayTypes; ++i)
        globalObject->defineDefaultProperty((str = typedArrayCtors[i].as<FunctionObject>()->name()), typedArrayCtors[i]);
    ScopedObject o(scope);
    globalObject->defineDefaultProperty(QStringLiteral("Atomics"), (o = memoryManager->allocate<Atomics>()));
    globalObject->defineDefaultProperty(QStringLiteral("Math"), (o = memoryManager->allocate<MathObject>()));
    globalObject->defineDefaultProperty(QStringLiteral("JSON"), (o = memoryManager->allocate<JsonObject>()));
    globalObject->defineDefaultProperty(QStringLiteral("Reflect"), (o = memoryManager->allocate<Reflect>()));
    globalObject->defineDefaultProperty(QStringLiteral("Proxy"), (o = memoryManager->allocate<Proxy>(rootContext())));

    globalObject->defineReadonlyProperty(QStringLiteral("undefined"), Value::undefinedValue());
    globalObject->defineReadonlyProperty(QStringLiteral("NaN"), Value::fromDouble(std::numeric_limits<double>::quiet_NaN()));
    globalObject->defineReadonlyProperty(QStringLiteral("Infinity"), Value::fromDouble(Q_INFINITY));


    jsObjects[Eval_Function] = memoryManager->allocate<EvalFunction>(global);
    globalObject->defineDefaultProperty(QStringLiteral("eval"), *evalFunction());

    // ES6: 20.1.2.12 &  20.1.2.13:
    // parseInt and parseFloat must be the same FunctionObject on the global &
    // Number object.
    {
        QString piString(QStringLiteral("parseInt"));
        QString pfString(QStringLiteral("parseFloat"));
        Scope scope(this);
        ScopedString pi(scope, newIdentifier(piString));
        ScopedString pf(scope, newIdentifier(pfString));
        ScopedFunctionObject parseIntFn(scope, FunctionObject::createBuiltinFunction(this, pi, GlobalFunctions::method_parseInt, 2));
        ScopedFunctionObject parseFloatFn(scope, FunctionObject::createBuiltinFunction(this, pf, GlobalFunctions::method_parseFloat, 1));
        globalObject->defineDefaultProperty(piString, parseIntFn);
        globalObject->defineDefaultProperty(pfString, parseFloatFn);
        numberObject->defineDefaultProperty(piString, parseIntFn);
        numberObject->defineDefaultProperty(pfString, parseFloatFn);
    }

    globalObject->defineDefaultProperty(QStringLiteral("isNaN"), GlobalFunctions::method_isNaN, 1);
    globalObject->defineDefaultProperty(QStringLiteral("isFinite"), GlobalFunctions::method_isFinite, 1);
    globalObject->defineDefaultProperty(QStringLiteral("decodeURI"), GlobalFunctions::method_decodeURI, 1);
    globalObject->defineDefaultProperty(QStringLiteral("decodeURIComponent"), GlobalFunctions::method_decodeURIComponent, 1);
    globalObject->defineDefaultProperty(QStringLiteral("encodeURI"), GlobalFunctions::method_encodeURI, 1);
    globalObject->defineDefaultProperty(QStringLiteral("encodeURIComponent"), GlobalFunctions::method_encodeURIComponent, 1);
    globalObject->defineDefaultProperty(QStringLiteral("escape"), GlobalFunctions::method_escape, 1);
    globalObject->defineDefaultProperty(QStringLiteral("unescape"), GlobalFunctions::method_unescape, 1);

    ScopedFunctionObject t(scope, memoryManager->allocate<FunctionObject>(rootContext(), nullptr, ::throwTypeError));
    t->defineReadonlyProperty(id_length(), Value::fromInt32(0));
    t->setInternalClass(t->internalClass()->cryopreserved());
    jsObjects[ThrowerObject] = t;

    ScopedProperty pd(scope);
    pd->value = thrower();
    pd->set = thrower();
    functionPrototype()->insertMember(id_caller(), pd, Attr_Accessor|Attr_ReadOnly_ButConfigurable);
    functionPrototype()->insertMember(id_arguments(), pd, Attr_Accessor|Attr_ReadOnly_ButConfigurable);

    QV4::QObjectWrapper::initializeBindings(this);

    m_delayedCallQueue.init(this);
    isInitialized = true;
}

ExecutionEngine::~ExecutionEngine()
{
    modules.clear();
    for (auto val : nativeModules) {
        PersistentValueStorage::free(val);
    }
    nativeModules.clear();
    qDeleteAll(m_extensionData);
    delete m_multiplyWrappedQObjects;
    m_multiplyWrappedQObjects = nullptr;
    delete identifierTable;
    delete memoryManager;

    while (!compilationUnits.isEmpty())
        (*compilationUnits.begin())->unlink();

    delete bumperPointerAllocator;
    delete regExpCache;
    delete regExpAllocator;
    delete executableAllocator;
    jsStack->deallocate();
    delete jsStack;
    gcStack->deallocate();
    delete gcStack;

#if QT_CONFIG(qml_xml_http_request)
    qt_rem_qmlxmlhttprequest(this, m_xmlHttpRequestData);
    m_xmlHttpRequestData = nullptr;
#endif
}

#if QT_CONFIG(qml_debug)
void ExecutionEngine::setDebugger(Debugging::Debugger *debugger)
{
    Q_ASSERT(!m_debugger);
    m_debugger.reset(debugger);
}

void ExecutionEngine::setProfiler(Profiling::Profiler *profiler)
{
    Q_ASSERT(!m_profiler);
    m_profiler.reset(profiler);
}
#endif // QT_CONFIG(qml_debug)

void ExecutionEngine::initRootContext()
{
    Scope scope(this);
    Scoped<ExecutionContext> r(scope, memoryManager->allocManaged<ExecutionContext>(sizeof(ExecutionContext::Data)));
    r->d_unchecked()->init(Heap::ExecutionContext::Type_GlobalContext);
    r->d()->activation.set(this, globalObject->d());
    jsObjects[RootContext] = r;
    jsObjects[ScriptContext] = r;
    jsObjects[IntegerNull] = Encode((int)0);
}

Heap::InternalClass *ExecutionEngine::newClass(Heap::InternalClass *other)
{
    Heap::InternalClass *ic = memoryManager->allocIC<InternalClass>();
    ic->init(other);
    return ic;
}

Heap::InternalClass *ExecutionEngine::newInternalClass(const VTable *vtable, Object *prototype)
{
    Scope scope(this);
    Scoped<InternalClass> ic(scope, internalClasses(Class_Empty)->changeVTable(vtable));
    return ic->changePrototype(prototype ? prototype->d() : nullptr);
}

Heap::Object *ExecutionEngine::newObject()
{
    return memoryManager->allocate<Object>();
}

Heap::Object *ExecutionEngine::newObject(Heap::InternalClass *internalClass)
{
    return memoryManager->allocObject<Object>(internalClass);
}

Heap::String *ExecutionEngine::newString(const QString &s)
{
    return memoryManager->allocWithStringData<String>(s.size() * sizeof(QChar), s);
}

Heap::String *ExecutionEngine::newIdentifier(const QString &text)
{
    Scope scope(this);
    ScopedString s(scope, memoryManager->allocWithStringData<String>(text.size() * sizeof(QChar), text));
    s->toPropertyKey();
    return s->d();
}

Heap::Object *ExecutionEngine::newStringObject(const String *string)
{
    return memoryManager->allocate<StringObject>(string);
}

Heap::Object *ExecutionEngine::newSymbolObject(const Symbol *symbol)
{
    return memoryManager->allocObject<SymbolObject>(classes[Class_SymbolObject], symbol);
}

Heap::Object *ExecutionEngine::newNumberObject(double value)
{
    return memoryManager->allocate<NumberObject>(value);
}

Heap::Object *ExecutionEngine::newBooleanObject(bool b)
{
    return memoryManager->allocate<BooleanObject>(b);
}

Heap::ArrayObject *ExecutionEngine::newArrayObject(int count)
{
    Scope scope(this);
    ScopedArrayObject object(scope, memoryManager->allocate<ArrayObject>());

    if (count) {
        if (count < 0x1000)
            object->arrayReserve(count);
        object->setArrayLengthUnchecked(count);
    }
    return object->d();
}

Heap::ArrayObject *ExecutionEngine::newArrayObject(const Value *values, int length)
{
    Scope scope(this);
    ScopedArrayObject a(scope, memoryManager->allocate<ArrayObject>());

    if (length) {
        size_t size = sizeof(Heap::ArrayData) + (length-1)*sizeof(Value);
        Heap::SimpleArrayData *d = scope.engine->memoryManager->allocManaged<SimpleArrayData>(size);
        d->init();
        d->type = Heap::ArrayData::Simple;
        d->offset = 0;
        d->values.alloc = length;
        d->values.size = length;
        // this doesn't require a write barrier, things will be ok, when the new array data gets inserted into
        // the parent object
        memcpy(&d->values.values, values, length*sizeof(Value));
        a->d()->arrayData.set(this, d);
        a->setArrayLengthUnchecked(length);
    }
    return a->d();
}

Heap::ArrayObject *ExecutionEngine::newArrayObject(const QStringList &list)
{
    return memoryManager->allocate<ArrayObject>(list);
}

Heap::ArrayObject *ExecutionEngine::newArrayObject(Heap::InternalClass *internalClass)
{
    return memoryManager->allocObject<ArrayObject>(internalClass);
}

Heap::ArrayBuffer *ExecutionEngine::newArrayBuffer(const QByteArray &array)
{
    return memoryManager->allocate<ArrayBuffer>(array);
}

Heap::ArrayBuffer *ExecutionEngine::newArrayBuffer(size_t length)
{
    return memoryManager->allocate<ArrayBuffer>(length);
}

Heap::DateObject *ExecutionEngine::newDateObject(double dateTime)
{
    return memoryManager->allocate<DateObject>(dateTime);
}

Heap::DateObject *ExecutionEngine::newDateObject(const QDateTime &dateTime)
{
    return memoryManager->allocate<DateObject>(dateTime);
}

Heap::DateObject *ExecutionEngine::newDateObject(
        QDate date, Heap::Object *parent, int index, uint flags)
{
    return memoryManager->allocate<DateObject>(
                date, parent, index, Heap::ReferenceObject::Flags(flags));
}

Heap::DateObject *ExecutionEngine::newDateObject(
        QTime time, Heap::Object *parent, int index, uint flags)
{
    return memoryManager->allocate<DateObject>(
                time, parent, index, Heap::ReferenceObject::Flags(flags));
}

Heap::DateObject *ExecutionEngine::newDateObject(
        QDateTime dateTime, Heap::Object *parent, int index, uint flags)
{
    return memoryManager->allocate<DateObject>(
                dateTime, parent, index, Heap::ReferenceObject::Flags(flags));
}

Heap::RegExpObject *ExecutionEngine::newRegExpObject(const QString &pattern, int flags)
{
    Scope scope(this);
    Scoped<RegExp> re(scope, RegExp::create(this, pattern, static_cast<CompiledData::RegExp::Flags>(flags)));
    return newRegExpObject(re);
}

Heap::RegExpObject *ExecutionEngine::newRegExpObject(RegExp *re)
{
    return memoryManager->allocate<RegExpObject>(re);
}

#if QT_CONFIG(regularexpression)
Heap::RegExpObject *ExecutionEngine::newRegExpObject(const QRegularExpression &re)
{
    return memoryManager->allocate<RegExpObject>(re);
}
#endif

Heap::UrlObject *ExecutionEngine::newUrlObject()
{
    return memoryManager->allocate<UrlObject>();
}

Heap::UrlObject *ExecutionEngine::newUrlObject(const QUrl &url)
{
    Scope scope(this);
    Scoped<UrlObject> urlObject(scope, newUrlObject());
    urlObject->setUrl(url);
    return urlObject->d();
}

Heap::UrlSearchParamsObject *ExecutionEngine::newUrlSearchParamsObject()
{
    return memoryManager->allocate<UrlSearchParamsObject>();
}

Heap::Object *ExecutionEngine::newErrorObject(const Value &value)
{
    return ErrorObject::create<ErrorObject>(this, value, errorCtor());
}

Heap::Object *ExecutionEngine::newErrorObject(const QString &message)
{
    return ErrorObject::create<ErrorObject>(this, message);
}

Heap::Object *ExecutionEngine::newSyntaxErrorObject(const QString &message)
{
    return ErrorObject::create<SyntaxErrorObject>(this, message);
}

Heap::Object *ExecutionEngine::newSyntaxErrorObject(const QString &message, const QString &fileName, int line, int column)
{
    return ErrorObject::create<SyntaxErrorObject>(this, message, fileName, line, column);
}


Heap::Object *ExecutionEngine::newReferenceErrorObject(const QString &message)
{
    return ErrorObject::create<ReferenceErrorObject>(this, message);
}

Heap::Object *ExecutionEngine::newReferenceErrorObject(const QString &message, const QString &fileName, int line, int column)
{
    return ErrorObject::create<ReferenceErrorObject>(this, message, fileName, line, column);
}


Heap::Object *ExecutionEngine::newTypeErrorObject(const QString &message)
{
    return ErrorObject::create<TypeErrorObject>(this, message);
}

Heap::Object *ExecutionEngine::newRangeErrorObject(const QString &message)
{
    return ErrorObject::create<RangeErrorObject>(this, message);
}

Heap::Object *ExecutionEngine::newURIErrorObject(const Value &message)
{
    return ErrorObject::create<URIErrorObject>(this, message, uRIErrorCtor());
}

Heap::PromiseObject *ExecutionEngine::newPromiseObject()
{
    if (!m_reactionHandler) {
        m_reactionHandler.reset(new Promise::ReactionHandler);
    }

    Scope scope(this);
    Scoped<PromiseObject> object(scope, memoryManager->allocate<PromiseObject>(this));
    return object->d();
}

Heap::Object *ExecutionEngine::newPromiseObject(const QV4::FunctionObject *thisObject, const QV4::PromiseCapability *capability)
{
    if (!m_reactionHandler) {
        m_reactionHandler.reset(new Promise::ReactionHandler);
    }

    Scope scope(this);
    Scoped<CapabilitiesExecutorWrapper> executor(scope,  memoryManager->allocate<CapabilitiesExecutorWrapper>());
    executor->d()->capabilities.set(this, capability->d());
    executor->insertMember(id_length(), Primitive::fromInt32(2), Attr_NotWritable|Attr_NotEnumerable);

    ScopedObject object(scope, thisObject->callAsConstructor(executor, 1));
    return object->d();
}

Promise::ReactionHandler *ExecutionEngine::getPromiseReactionHandler()
{
    Q_ASSERT(m_reactionHandler);
    return m_reactionHandler.data();
}

Heap::Object *ExecutionEngine::newURIErrorObject(const QString &message)
{
    return ErrorObject::create<URIErrorObject>(this, message);
}

Heap::Object *ExecutionEngine::newEvalErrorObject(const QString &message)
{
    return ErrorObject::create<EvalErrorObject>(this, message);
}

Heap::Object *ExecutionEngine::newVariantObject(const QMetaType type, const void *data)
{
    return memoryManager->allocate<VariantObject>(type, data);
}

Heap::Object *ExecutionEngine::newForInIteratorObject(Object *o)
{
    Scope scope(this);
    ScopedObject obj(scope, memoryManager->allocate<ForInIteratorObject>(o));
    return obj->d();
}

Heap::Object *ExecutionEngine::newMapIteratorObject(Object *o)
{
    return memoryManager->allocate<MapIteratorObject>(o->d(), this);
}

Heap::Object *ExecutionEngine::newSetIteratorObject(Object *o)
{
    return memoryManager->allocate<SetIteratorObject>(o->d(), this);
}

Heap::Object *ExecutionEngine::newArrayIteratorObject(Object *o)
{
    return memoryManager->allocate<ArrayIteratorObject>(o->d(), this);
}

Heap::QmlContext *ExecutionEngine::qmlContext() const
{
    return currentStackFrame
            ? static_cast<Heap::QmlContext *>(qmlContext(currentContext()->d()))
            : nullptr;
}

QObject *ExecutionEngine::qmlScopeObject() const
{
    Heap::QmlContext *ctx = qmlContext();
    if (!ctx)
        return nullptr;

    return ctx->qml()->scopeObject;
}

QQmlRefPointer<QQmlContextData> ExecutionEngine::callingQmlContext() const
{
    Heap::QmlContext *ctx = qmlContext();
    if (!ctx)
        return nullptr;

    return ctx->qml()->context;
}

StackTrace ExecutionEngine::stackTrace(int frameLimit) const
{
    Scope scope(const_cast<ExecutionEngine *>(this));
    ScopedString name(scope);
    StackTrace stack;

    CppStackFrame *f = currentStackFrame;
    while (f && frameLimit) {
        QV4::StackFrame frame;
        frame.source = f->source();
        frame.function = f->function();
        frame.line = f->lineNumber();
        frame.column = -1;
        stack.append(frame);
        if (f->isJSTypesFrame()) {
            if (static_cast<JSTypesStackFrame *>(f)->isTailCalling()) {
                QV4::StackFrame frame;
                frame.function = QStringLiteral("[elided tail calls]");
                stack.append(frame);
            }
        }
        --frameLimit;
        f = f->parentFrame();
    }

    return stack;
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
    if (context && context->engine()) {
        const QVector<StackFrame> stackTrace = context->engine()->stackTrace(20);
        for (int i = 0; i < stackTrace.size(); ++i) {
            if (i)
                str << ',';
            const QUrl url(stackTrace.at(i).source);
            const QString fileName = url.isLocalFile() ? url.toLocalFile() : url.toString();
            str << "frame={level=\"" << i << "\",func=\"" << stackTrace.at(i).function
                << "\",file=\"" << fileName << "\",fullname=\"" << fileName
                << "\",line=\"" << qAbs(stackTrace.at(i).line) << "\",language=\"js\"}";
        }
    }
    str << ']';
    return qstrdup(result.toLocal8Bit().constData());
}

extern "C" Q_QML_EXPORT char *qt_v4StackTrace(void *executionContext)
{
    return v4StackTrace(reinterpret_cast<const ExecutionContext *>(executionContext));
}

extern "C" Q_QML_EXPORT char *qt_v4StackTraceForEngine(void *executionEngine)
{
    auto engine = (reinterpret_cast<const ExecutionEngine *>(executionEngine));
    return v4StackTrace(engine->currentContext());
}

QUrl ExecutionEngine::resolvedUrl(const QString &file)
{
    QUrl src(file);
    if (!src.isRelative())
        return src;

    QUrl base;
    CppStackFrame *f = currentStackFrame;
    while (f) {
        if (f->v4Function) {
            base = f->v4Function->finalUrl();
            break;
        }
        f = f->parentFrame();
    }

    if (base.isEmpty() && globalCode)
        base = globalCode->finalUrl();

    if (base.isEmpty())
        return src;

    return base.resolved(src);
}

void ExecutionEngine::markObjects(MarkStack *markStack)
{
    for (int i = 0; i < NClasses; ++i) {
        if (Heap::InternalClass *c = classes[i])
            c->mark(markStack);
    }

    identifierTable->markObjects(markStack);

    for (auto compilationUnit: compilationUnits)
        compilationUnit->markObjects(markStack);
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
    *exceptionValue = value;
    QV4::Scope scope(this);
    QV4::Scoped<ErrorObject> error(scope, value);
    if (!!error)
        exceptionStackTrace = *error->d()->stackTrace;
    else
        exceptionStackTrace = stackTrace();

    if (QV4::Debugging::Debugger *debug = debugger())
        debug->aboutToThrow();

    return Encode::undefined();
}

ReturnedValue ExecutionEngine::catchException(StackTrace *trace)
{
    Q_ASSERT(hasException);
    if (trace)
        *trace = exceptionStackTrace;
    exceptionStackTrace.clear();
    hasException = false;
    ReturnedValue res = exceptionValue->asReturnedValue();
    *exceptionValue = Value::emptyValue();
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

ReturnedValue ExecutionEngine::throwReferenceError(const QString &name)
{
    Scope scope(this);
    QString msg = name + QLatin1String(" is not defined");
    ScopedObject error(scope, newReferenceErrorObject(msg));
    return throwError(error);
}

ReturnedValue ExecutionEngine::throwReferenceError(const Value &value)
{
    Scope scope(this);
    ScopedString s(scope, value.toString(this));
    QString msg = s->toQString() + QLatin1String(" is not defined");
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
    QString msg = s->toQString() + QLatin1String(" out of range");
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
    ScopedValue v(scope, newString(QLatin1String("Unimplemented ") + message));
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
        QV4::StackFrame frame = trace.constFirst();
        error.setUrl(QUrl(frame.source));
        error.setLine(qAbs(frame.line));
        error.setColumn(frame.column);
    }
    QV4::Scoped<QV4::ErrorObject> errorObj(scope, exception);
    error.setDescription(exception->toQStringNoThrow());
    return error;
}

// Variant conversion code

typedef QSet<QV4::Heap::Object *> V4ObjectSet;
static QVariant toVariant(
    const QV4::Value &value, QMetaType typeHint, bool createJSValueForObjectsAndSymbols,
    V4ObjectSet *visitedObjects);
static QObject *qtObjectFromJS(const QV4::Value &value);
static QVariant objectToVariant(const QV4::Object *o, V4ObjectSet *visitedObjects = nullptr);
static bool convertToNativeQObject(const QV4::Value &value, QMetaType targetType, void **result);
static QV4::ReturnedValue variantListToJS(QV4::ExecutionEngine *v4, const QVariantList &lst);
static QV4::ReturnedValue sequentialIterableToJS(QV4::ExecutionEngine *v4, const QSequentialIterable &lst);
static QV4::ReturnedValue variantMapToJS(QV4::ExecutionEngine *v4, const QVariantMap &vmap);
static QV4::ReturnedValue variantToJS(QV4::ExecutionEngine *v4, const QVariant &value)
{
    return v4->metaTypeToJS(value.metaType(), value.constData());
}

static QVariant toVariant(
        const QV4::Value &value, QMetaType metaType, bool createJSValueForObjectsAndSymbols,
        V4ObjectSet *visitedObjects)
{
    Q_ASSERT (!value.isEmpty());

    if (const QV4::VariantObject *v = value.as<QV4::VariantObject>())
        return v->d()->data();

    if (metaType == QMetaType::fromType<bool>())
        return QVariant(value.toBoolean());

    if (metaType == QMetaType::fromType<double>())
        return QVariant(value.toNumber());

    if (metaType == QMetaType::fromType<float>())
        return QVariant(float(value.toNumber()));

    if (metaType == QMetaType::fromType<QJsonValue>())
        return QVariant::fromValue(QV4::JsonObject::toJsonValue(value));

    if (metaType == QMetaType::fromType<QJSValue>())
        return QVariant::fromValue(QJSValuePrivate::fromReturnedValue(value.asReturnedValue()));

    if (const QV4::Object *o = value.as<QV4::Object>()) {
        QV4::Scope scope(o->engine());
        QV4::ScopedObject object(scope, o);
        if (metaType == QMetaType::fromType<QJsonObject>()
                   && !value.as<ArrayObject>() && !value.as<FunctionObject>()) {
            return QVariant::fromValue(QV4::JsonObject::toJsonObject(object));
        } else if (QV4::QObjectWrapper *wrapper = object->as<QV4::QObjectWrapper>()) {
            return QVariant::fromValue<QObject *>(wrapper->object());
        } else if (object->as<QV4::QQmlContextWrapper>()) {
            return QVariant();
        } else if (QV4::QQmlTypeWrapper *w = object->as<QV4::QQmlTypeWrapper>()) {
            return w->toVariant();
        } else if (QV4::QQmlValueTypeWrapper *v = object->as<QV4::QQmlValueTypeWrapper>()) {
            return v->toVariant();
        } else if (QV4::QmlListWrapper *l = object->as<QV4::QmlListWrapper>()) {
            return l->toVariant();
        } else if (QV4::Sequence *s = object->as<QV4::Sequence>()) {
            return QV4::SequencePrototype::toVariant(s);
        }
    }

    if (const QV4::ArrayObject *o = value.as<ArrayObject>()) {
        QV4::Scope scope(o->engine());
        QV4::ScopedArrayObject a(scope, o);
        if (metaType == QMetaType::fromType<QList<QObject *>>()) {
            QList<QObject *> list;
            uint length = a->getLength();
            QV4::Scoped<QV4::QObjectWrapper> qobjectWrapper(scope);
            for (uint ii = 0; ii < length; ++ii) {
                qobjectWrapper = a->get(ii);
                if (!!qobjectWrapper) {
                    list << qobjectWrapper->object();
                } else {
                    list << 0;
                }
            }

            return QVariant::fromValue<QList<QObject*> >(list);
        } else if (metaType == QMetaType::fromType<QJsonArray>()) {
            return QVariant::fromValue(QV4::JsonObject::toJsonArray(a));
        }

        QVariant retn = QV4::SequencePrototype::toVariant(value, metaType);
        if (retn.isValid())
            return retn;

        if (metaType.isValid()) {
            retn = QVariant(metaType, nullptr);
            auto retnAsIterable = retn.value<QSequentialIterable>();
            if (retnAsIterable.metaContainer().canAddValue()) {
                QMetaType valueMetaType = retnAsIterable.metaContainer().valueMetaType();
                auto const length = a->getLength();
                QV4::ScopedValue arrayValue(scope);
                for (qint64 i = 0; i < length; ++i) {
                    arrayValue = a->get(i);
                    QVariant asVariant = QQmlValueTypeProvider::createValueType(
                                arrayValue, valueMetaType);
                    if (asVariant.isValid()) {
                        retnAsIterable.metaContainer().addValue(retn.data(), asVariant.constData());
                        continue;
                    }

                    if (QMetaType::canConvert(QMetaType::fromType<QJSValue>(), valueMetaType)) {
                        // before attempting a conversion from the concrete types,
                        // check if there exists a conversion from QJSValue -> out type
                        // prefer that one for compatibility reasons
                        asVariant = QVariant::fromValue(QJSValuePrivate::fromReturnedValue(
                                                            arrayValue->asReturnedValue()));
                        if (asVariant.convert(valueMetaType)) {
                            retnAsIterable.metaContainer().addValue(retn.data(), asVariant.constData());
                            continue;
                        }
                    }

                    asVariant = toVariant(arrayValue, valueMetaType, false, visitedObjects);
                    if (valueMetaType == QMetaType::fromType<QVariant>()) {
                        retnAsIterable.metaContainer().addValue(retn.data(), &asVariant);
                    } else {
                        auto originalType = asVariant.metaType();
                        bool couldConvert = asVariant.convert(valueMetaType);
                        if (!couldConvert) {
                            qWarning().noquote()
                                    << QLatin1String("Could not convert array value "
                                                     "at position %1 from %2 to %3")
                                       .arg(QString::number(i),
                                            QString::fromUtf8(originalType.name()),
                                            QString::fromUtf8(valueMetaType.name()));
                            // create default constructed value
                            asVariant = QVariant(valueMetaType, nullptr);
                        }
                        retnAsIterable.metaContainer().addValue(retn.data(), asVariant.constData());
                    }
                }
                return retn;
            }
        }
    }

    if (value.isUndefined())
        return QVariant();
    if (value.isNull())
        return QVariant::fromValue(nullptr);
    if (value.isBoolean())
        return value.booleanValue();
    if (value.isInteger())
        return value.integerValue();
    if (value.isNumber())
        return value.asDouble();
    if (String *s = value.stringValue()) {
        const QString &str = s->toQString();
        // QChars are stored as a strings
        if (metaType == QMetaType::fromType<QChar>() && str.size() == 1)
            return str.at(0);
        return str;
    }
#if QT_CONFIG(qml_locale)
    if (const QV4::QQmlLocaleData *ld = value.as<QV4::QQmlLocaleData>())
        return *ld->d()->locale;
#endif
    if (const QV4::DateObject *d = value.as<DateObject>()) {
        // NOTE: since we convert QTime to JS Date,
        //       round trip will change the variant type (to QDateTime)!

        if (metaType == QMetaType::fromType<QDate>())
            return DateObject::dateTimeToDate(d->toQDateTime());

        if (metaType == QMetaType::fromType<QTime>())
            return d->toQDateTime().time();

        if (metaType == QMetaType::fromType<QString>())
            return d->toString();

        return d->toQDateTime();
    }
    if (const QV4::UrlObject *d = value.as<UrlObject>())
        return d->toQUrl();
    if (const ArrayBuffer *d = value.as<ArrayBuffer>())
        return d->asByteArray();
    if (const Symbol *symbol = value.as<Symbol>()) {
        return createJSValueForObjectsAndSymbols
            ? QVariant::fromValue(QJSValuePrivate::fromReturnedValue(symbol->asReturnedValue()))
            : symbol->descriptiveString();
    }

    const QV4::Object *object = value.as<QV4::Object>();
    Q_ASSERT(object);
    QV4::Scope scope(object->engine());
    QV4::ScopedObject o(scope, object);

#if QT_CONFIG(regularexpression)
    if (QV4::RegExpObject *re = o->as<QV4::RegExpObject>())
        return re->toQRegularExpression();
#endif

    if (metaType.isValid() && !(metaType.flags() & QMetaType::PointerToQObject)) {
        const QVariant result = QQmlValueTypeProvider::createValueType(value, metaType);
        if (result.isValid())
            return result;
    }

    if (createJSValueForObjectsAndSymbols)
        return QVariant::fromValue(QJSValuePrivate::fromReturnedValue(o->asReturnedValue()));

    return objectToVariant(o, visitedObjects);
}


QVariant ExecutionEngine::toVariant(
    const Value &value, QMetaType typeHint, bool createJSValueForObjectsAndSymbols)
{
    return ::toVariant(value, typeHint, createJSValueForObjectsAndSymbols, nullptr);
}

static QVariantMap objectToVariantMap(const QV4::Object *o, V4ObjectSet *visitedObjects)
{
    QVariantMap map;
    QV4::Scope scope(o->engine());
    QV4::ObjectIterator it(scope, o, QV4::ObjectIterator::EnumerableOnly);
    QV4::ScopedValue name(scope);
    QV4::ScopedValue val(scope);
    while (1) {
        name = it.nextPropertyNameAsString(val);
        if (name->isNull())
            break;

        QString key = name->toQStringNoThrow();
        map.insert(key, ::toVariant(
                                val, /*type hint*/ QMetaType {},
                                /*createJSValueForObjectsAndSymbols*/false, visitedObjects));
    }
    return map;
}

static QVariant objectToVariant(const QV4::Object *o, V4ObjectSet *visitedObjects)
{
    Q_ASSERT(o);

    V4ObjectSet recursionGuardSet;
    if (!visitedObjects) {
        visitedObjects = &recursionGuardSet;
    } else if (visitedObjects->contains(o->d())) {
        // Avoid recursion.
        // For compatibility with QVariant{List,Map} conversion, we return an
        // empty object (and no error is thrown).
        if (o->as<ArrayObject>())
            return QVariantList();
        return QVariantMap();
    }
    visitedObjects->insert(o->d());

    QVariant result;

    if (o->as<ArrayObject>()) {
        QV4::Scope scope(o->engine());
        QV4::ScopedArrayObject a(scope, o->asReturnedValue());
        QV4::ScopedValue v(scope);
        QVariantList list;

        int length = a->getLength();
        for (int ii = 0; ii < length; ++ii) {
            v = a->get(ii);
            list << ::toVariant(v, QMetaType {}, /*createJSValueForObjectsAndSymbols*/false,
                                visitedObjects);
        }

        result = list;
    } else if (o->getPrototypeOf() == o->engine()->objectPrototype()->d()) {
        result = objectToVariantMap(o, visitedObjects);
    } else {
        // If it's not a plain object, we can only save it as QJSValue.
        result = QVariant::fromValue(QJSValuePrivate::fromReturnedValue(o->asReturnedValue()));
    }

    visitedObjects->remove(o->d());
    return result;
}

/*!
  \internal

  Transform the given \a metaType and \a ptr into a JavaScript representation.
 */
QV4::ReturnedValue ExecutionEngine::fromData(
        QMetaType metaType, const void *ptr,
        QV4::Heap::Object *container, int property, uint flags)
{
    const int type = metaType.id();
    if (type < QMetaType::User) {
        switch (QMetaType::Type(type)) {
            case QMetaType::UnknownType:
            case QMetaType::Void:
                return QV4::Encode::undefined();
            case QMetaType::Nullptr:
            case QMetaType::VoidStar:
                return QV4::Encode::null();
            case QMetaType::Bool:
                return QV4::Encode(*reinterpret_cast<const bool*>(ptr));
            case QMetaType::Int:
                return QV4::Encode(*reinterpret_cast<const int*>(ptr));
            case QMetaType::UInt:
                return QV4::Encode(*reinterpret_cast<const uint*>(ptr));
            case QMetaType::Long:
                return QV4::Encode((double)*reinterpret_cast<const long *>(ptr));
            case QMetaType::ULong:
                return QV4::Encode((double)*reinterpret_cast<const ulong *>(ptr));
            case QMetaType::LongLong:
                return QV4::Encode((double)*reinterpret_cast<const qlonglong*>(ptr));
            case QMetaType::ULongLong:
                return QV4::Encode((double)*reinterpret_cast<const qulonglong*>(ptr));
            case QMetaType::Double:
                return QV4::Encode(*reinterpret_cast<const double*>(ptr));
            case QMetaType::QString:
                return newString(*reinterpret_cast<const QString*>(ptr))->asReturnedValue();
            case QMetaType::QByteArray:
                return newArrayBuffer(*reinterpret_cast<const QByteArray*>(ptr))->asReturnedValue();
            case QMetaType::Float:
                return QV4::Encode(*reinterpret_cast<const float*>(ptr));
            case QMetaType::Short:
                return QV4::Encode((int)*reinterpret_cast<const short*>(ptr));
            case QMetaType::UShort:
                return QV4::Encode((int)*reinterpret_cast<const unsigned short*>(ptr));
            case QMetaType::Char:
                return QV4::Encode((int)*reinterpret_cast<const char*>(ptr));
            case QMetaType::UChar:
                return QV4::Encode((int)*reinterpret_cast<const unsigned char*>(ptr));
            case QMetaType::SChar:
                return QV4::Encode((int)*reinterpret_cast<const signed char*>(ptr));
            case QMetaType::QChar:
                return newString(*reinterpret_cast<const QChar *>(ptr))->asReturnedValue();
            case QMetaType::Char16:
                return newString(QChar(*reinterpret_cast<const char16_t *>(ptr)))->asReturnedValue();
            case QMetaType::QDateTime:
                return QV4::Encode(newDateObject(
                                       *reinterpret_cast<const QDateTime *>(ptr),
                                       container, property, flags));
            case QMetaType::QDate:
                return QV4::Encode(newDateObject(
                                       *reinterpret_cast<const QDate *>(ptr),
                                       container, property, flags));
            case QMetaType::QTime:
                return QV4::Encode(newDateObject(
                                       *reinterpret_cast<const QTime *>(ptr),
                                       container, property, flags));
#if QT_CONFIG(regularexpression)
            case QMetaType::QRegularExpression:
                return QV4::Encode(newRegExpObject(*reinterpret_cast<const QRegularExpression *>(ptr)));
#endif
            case QMetaType::QObjectStar:
                return QV4::QObjectWrapper::wrap(this, *reinterpret_cast<QObject* const *>(ptr));
            case QMetaType::QStringList:
                {
                QV4::Scope scope(this);
                QV4::ScopedValue retn(scope, QV4::SequencePrototype::fromData(this, metaType, ptr));
                if (!retn->isUndefined())
                    return retn->asReturnedValue();
                return QV4::Encode(newArrayObject(*reinterpret_cast<const QStringList *>(ptr)));
                }
            case QMetaType::QVariantList:
                return variantListToJS(this, *reinterpret_cast<const QVariantList *>(ptr));
            case QMetaType::QVariantMap:
                return variantMapToJS(this, *reinterpret_cast<const QVariantMap *>(ptr));
            case QMetaType::QJsonValue:
                return QV4::JsonObject::fromJsonValue(this, *reinterpret_cast<const QJsonValue *>(ptr));
            case QMetaType::QJsonObject:
                return QV4::JsonObject::fromJsonObject(this, *reinterpret_cast<const QJsonObject *>(ptr));
            case QMetaType::QJsonArray:
                return QV4::JsonObject::fromJsonArray(this, *reinterpret_cast<const QJsonArray *>(ptr));
#if QT_CONFIG(qml_locale)
            case QMetaType::QLocale:
                return QQmlLocale::wrap(this, *reinterpret_cast<const QLocale*>(ptr));
#endif
            case QMetaType::QPixmap:
            case QMetaType::QImage:
                // Scarce value types
                return QV4::Encode(newVariantObject(metaType, ptr));
            default:
                break;
        }

        if (const QMetaObject *vtmo = QQmlMetaType::metaObjectForValueType(metaType)) {
            if (container) {
                return QV4::QQmlValueTypeWrapper::create(
                            this, ptr, vtmo, metaType,
                            container, property, Heap::ReferenceObject::Flags(flags));
            } else {
                return QV4::QQmlValueTypeWrapper::create(this, ptr, vtmo, metaType);
            }
        }

    } else if (!(metaType.flags() & QMetaType::IsEnumeration)) {
        QV4::Scope scope(this);
        if (metaType == QMetaType::fromType<QQmlListReference>()) {
            typedef QQmlListReferencePrivate QDLRP;
            QDLRP *p = QDLRP::get((QQmlListReference*)const_cast<void *>(ptr));
            if (p->object)
                return QV4::QmlListWrapper::create(scope.engine, p->property, p->propertyType);
            else
                return QV4::Encode::null();
        } else if (auto flags = metaType.flags(); flags & QMetaType::IsQmlList) {
            // casting to QQmlListProperty<QObject> is slightly nasty, but it's the
            // same QQmlListReference does.
            const auto *p = static_cast<const QQmlListProperty<QObject> *>(ptr);
            if (p->object)
                return QV4::QmlListWrapper::create(scope.engine, *p, metaType);
            else
                return QV4::Encode::null();
        } else if (metaType == QMetaType::fromType<QJSValue>()) {
            return QJSValuePrivate::convertToReturnedValue(
                        this, *reinterpret_cast<const QJSValue *>(ptr));
        } else if (metaType == QMetaType::fromType<QList<QObject *> >()) {
            // XXX Can this be made more by using Array as a prototype and implementing
            // directly against QList<QObject*>?
            const QList<QObject *> &list = *(const QList<QObject *>*)ptr;
            QV4::ScopedArrayObject a(scope, newArrayObject());
            a->arrayReserve(list.size());
            QV4::ScopedValue v(scope);
            for (int ii = 0; ii < list.size(); ++ii)
                a->arrayPut(ii, (v = QV4::QObjectWrapper::wrap(this, list.at(ii))));
            a->setArrayLengthUnchecked(list.size());
            return a.asReturnedValue();
        } else if (auto flags = metaType.flags(); flags & QMetaType::PointerToQObject) {
            if (flags.testFlag(QMetaType::IsConst))
                return QV4::QObjectWrapper::wrapConst(this, *reinterpret_cast<QObject* const *>(ptr));
            else
                return QV4::QObjectWrapper::wrap(this, *reinterpret_cast<QObject* const *>(ptr));
        } else if (metaType == QMetaType::fromType<QJSPrimitiveValue>()) {
            const QJSPrimitiveValue *primitive = static_cast<const QJSPrimitiveValue *>(ptr);
            switch (primitive->type()) {
            case QJSPrimitiveValue::Boolean:
                return Encode(primitive->asBoolean());
            case QJSPrimitiveValue::Integer:
                return Encode(primitive->asInteger());
            case QJSPrimitiveValue::String:
                return newString(primitive->asString())->asReturnedValue();
            case QJSPrimitiveValue::Undefined:
                return Encode::undefined();
            case QJSPrimitiveValue::Null:
                return Encode::null();
            case QJSPrimitiveValue::Double:
                return Encode(primitive->asDouble());
            }
        }

        QV4::Scoped<Sequence> sequence(scope);
        if (container) {
            sequence = QV4::SequencePrototype::newSequence(
                        this, metaType, ptr,
                        container, property, Heap::ReferenceObject::Flags(flags));
        } else {
            sequence = QV4::SequencePrototype::fromData(this, metaType, ptr);
        }
        if (!sequence->isUndefined())
            return sequence->asReturnedValue();

        if (QMetaType::canConvert(metaType, QMetaType::fromType<QSequentialIterable>())) {
            QSequentialIterable lst;
            QMetaType::convert(metaType, ptr, QMetaType::fromType<QSequentialIterable>(), &lst);
            return sequentialIterableToJS(this, lst);
        }

        if (const QMetaObject *vtmo = QQmlMetaType::metaObjectForValueType(metaType)) {
            if (container) {
                return QV4::QQmlValueTypeWrapper::create(
                            this, ptr, vtmo, metaType,
                            container, property, Heap::ReferenceObject::Flags(flags));
            } else {
                return QV4::QQmlValueTypeWrapper::create(this, ptr, vtmo, metaType);
            }
        }
    }

    // XXX TODO: To be compatible, we still need to handle:
    //    + QObjectList
    //    + QList<int>

    if (metaType.flags() & QMetaType::IsEnumeration)
        return fromData(metaType.underlyingType(), ptr, container, property, flags);

    return QV4::Encode(newVariantObject(metaType, ptr));
}

QV4::ReturnedValue QV4::ExecutionEngine::fromVariant(const QVariant &variant)
{
    return fromData(variant.metaType(), variant.constData());
}

ReturnedValue ExecutionEngine::fromVariant(
        const QVariant &variant, Heap::Object *parent, int property, uint flags)
{
    return fromData(variant.metaType(), variant.constData(), parent, property, flags);
}

QVariantMap ExecutionEngine::variantMapFromJS(const Object *o)
{
    Q_ASSERT(o);
    V4ObjectSet visitedObjects;
    visitedObjects.insert(o->d());
    return objectToVariantMap(o, &visitedObjects);
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

// Converts a QSequentialIterable to JS.
// The result is a new Array object with length equal to the length
// of the QSequentialIterable, and the elements being the QSequentialIterable's
// elements converted to JS, recursively.
static QV4::ReturnedValue sequentialIterableToJS(QV4::ExecutionEngine *v4, const QSequentialIterable &lst)
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
    QV4::ScopedPropertyKey key(scope);
    QV4::ScopedValue v(scope);
    for (QVariantMap::const_iterator it = vmap.constBegin(), cend = vmap.constEnd(); it != cend; ++it) {
        s = v4->newIdentifier(it.key());
        key = s->propertyKey();
        v = variantToJS(v4, it.value());
        if (key->isArrayIndex())
            o->arraySet(key->asArrayIndex(), v);
        else
            o->insertMember(s, v);
    }
    return o.asReturnedValue();
}

// Converts the meta-type defined by the given type and data to JS.
// Returns the value if conversion succeeded, an empty handle otherwise.
QV4::ReturnedValue ExecutionEngine::metaTypeToJS(QMetaType type, const void *data)
{
    Q_ASSERT(data != nullptr);

    if (type == QMetaType::fromType<QVariant>()) {
        // unwrap it: this is tested in QJSEngine, and makes the most sense for
        // end-user code too.
        return fromVariant(*reinterpret_cast<const QVariant*>(data));
    } else if (type == QMetaType::fromType<QUrl>()) {
        // Create a proper URL object here, rather than a variant.
        return newUrlObject(*reinterpret_cast<const QUrl *>(data))->asReturnedValue();
    }

    return fromData(type, data);
}

int ExecutionEngine::maxJSStackSize() const
{
    return s_maxJSStackSize;
}

int ExecutionEngine::maxGCStackSize() const
{
    return s_maxGCStackSize;
}

/*!
    \internal
    Returns \a length converted to int if its safe to
    pass to \c Scope::alloc.
    Otherwise it throws a RangeError, and returns 0.
 */
int ExecutionEngine::safeForAllocLength(qint64 len64)
{
    if (len64 < 0ll || len64 > qint64(std::numeric_limits<int>::max())) {
        throwRangeError(QStringLiteral("Invalid array length."));
        return 0;
    }
    if (len64 > qint64(this->jsStackLimit - this->jsStackTop)) {
        throwRangeError(QStringLiteral("Array too large for apply()."));
        return 0;
    }
    return len64;
}

ReturnedValue ExecutionEngine::global()
{
    return globalObject->asReturnedValue();
}

QQmlRefPointer<ExecutableCompilationUnit> ExecutionEngine::compileModule(const QUrl &url)
{
    QQmlMetaType::CachedUnitLookupError cacheError = QQmlMetaType::CachedUnitLookupError::NoError;
    const DiskCacheOptions options = diskCacheOptions();
    if (const QQmlPrivate::CachedQmlUnit *cachedUnit = (options & DiskCache::Aot)
            ? QQmlMetaType::findCachedCompilationUnit(
                url,
                (options & DiskCache::AotByteCode)
                    ? QQmlMetaType::AcceptUntyped
                    : QQmlMetaType::RequireFullyTyped,
                &cacheError)
            : nullptr) {
        return ExecutableCompilationUnit::create(
                    QV4::CompiledData::CompilationUnit(
                        cachedUnit->qmlData, cachedUnit->aotCompiledFunctions,
                        url.fileName(), url.toString()));
    }

    QFile f(QQmlFile::urlToLocalFileOrQrc(url));
    if (!f.open(QIODevice::ReadOnly)) {
        throwError(QStringLiteral("Could not open module %1 for reading").arg(url.toString()));
        return nullptr;
    }

    const QDateTime timeStamp = QFileInfo(f).lastModified();

    const QString sourceCode = QString::fromUtf8(f.readAll());
    f.close();

    return compileModule(url, sourceCode, timeStamp);
}


QQmlRefPointer<ExecutableCompilationUnit> ExecutionEngine::compileModule(
        const QUrl &url, const QString &sourceCode, const QDateTime &sourceTimeStamp)
{
    QList<QQmlJS::DiagnosticMessage> diagnostics;
    auto unit = Compiler::Codegen::compileModule(/*debugMode*/debugger() != nullptr, url.toString(),
                                                 sourceCode, sourceTimeStamp, &diagnostics);
    for (const QQmlJS::DiagnosticMessage &m : diagnostics) {
        if (m.isError()) {
            throwSyntaxError(m.message, url.toString(), m.loc.startLine, m.loc.startColumn);
            return nullptr;
        } else {
            qWarning() << url << ':' << m.loc.startLine << ':' << m.loc.startColumn
                      << ": warning: " << m.message;
        }
    }

    return ExecutableCompilationUnit::create(std::move(unit));
}

void ExecutionEngine::injectCompiledModule(const QQmlRefPointer<ExecutableCompilationUnit> &moduleUnit)
{
    // Injection can happen from the QML type loader thread for example, but instantiation and
    // evaluation must be limited to the ExecutionEngine's thread.
    QMutexLocker moduleGuard(&moduleMutex);
    modules.insert(moduleUnit->finalUrl(), moduleUnit);
}

ExecutionEngine::Module ExecutionEngine::moduleForUrl(
        const QUrl &url, const ExecutableCompilationUnit *referrer) const
{
    QMutexLocker moduleGuard(&moduleMutex);
    const auto nativeModule = nativeModules.find(url);
    if (nativeModule != nativeModules.end())
        return Module { nullptr, *nativeModule };

    const QUrl resolved = referrer
            ? referrer->finalUrl().resolved(QQmlTypeLoader::normalize(url))
            : QQmlTypeLoader::normalize(url);
    auto existingModule = modules.find(resolved);
    if (existingModule == modules.end())
        return Module { nullptr, nullptr };
    return Module { *existingModule, nullptr };
}

ExecutionEngine::Module ExecutionEngine::loadModule(const QUrl &url, const ExecutableCompilationUnit *referrer)
{
    QMutexLocker moduleGuard(&moduleMutex);
    const auto nativeModule = nativeModules.find(url);
    if (nativeModule != nativeModules.end())
        return Module { nullptr, *nativeModule };

    const QUrl resolved = referrer
            ? referrer->finalUrl().resolved(QQmlTypeLoader::normalize(url))
            : QQmlTypeLoader::normalize(url);
    auto existingModule = modules.find(resolved);
    if (existingModule != modules.end())
        return Module { *existingModule, nullptr };

    moduleGuard.unlock();

    auto newModule = compileModule(resolved);
    if (newModule) {
        moduleGuard.relock();
        modules.insert(resolved, newModule);
    }

    return Module { newModule, nullptr };
}

QV4::Value *ExecutionEngine::registerNativeModule(const QUrl &url, const QV4::Value &module)
{
    QMutexLocker moduleGuard(&moduleMutex);
    const auto existingModule = nativeModules.find(url);
    if (existingModule != nativeModules.end())
        return nullptr;

    QV4::Value *val = this->memoryManager->m_persistentValues->allocate();
    *val = module.asReturnedValue();
    nativeModules.insert(url, val);
    return val;
}

static ExecutionEngine::DiskCacheOptions transFormDiskCache(const char *v)
{
    using DiskCache = ExecutionEngine::DiskCache;

    if (v == nullptr)
        return DiskCache::Enabled;

    ExecutionEngine::DiskCacheOptions result = DiskCache::Disabled;
    const QList<QByteArray> options = QByteArray(v).split(',');
    for (const QByteArray &option : options) {
        if (option == "aot-bytecode")
            result |= DiskCache::AotByteCode;
        else if (option == "aot-native")
            result |= DiskCache::AotNative;
        else if (option == "aot")
            result |= DiskCache::Aot;
        else if (option == "qmlc-read")
            result |= DiskCache::QmlcRead;
        else if (option == "qmlc-write")
            result |= DiskCache::QmlcWrite;
        else if (option == "qmlc")
            result |= DiskCache::Qmlc;
        else
            qWarning() << "Ignoring unknown option to QML_DISK_CACHE:" << option;
    }

    return result;
}

ExecutionEngine::DiskCacheOptions ExecutionEngine::diskCacheOptions() const
{
    if (forceDiskCache())
        return DiskCache::Enabled;
    if (disableDiskCache() || debugger())
        return DiskCache::Disabled;
    static const DiskCacheOptions options = qmlGetConfigOption<
            DiskCacheOptions, transFormDiskCache>("QML_DISK_CACHE");
    return options;
}

void ExecutionEngine::callInContext(QV4::Function *function, QObject *self,
                                    QV4::ExecutionContext *context, int argc, void **args,
                                    QMetaType *types)
{
    if (!args) {
        Q_ASSERT(argc == 0);
        void *dummyArgs[] = { nullptr };
        QMetaType dummyTypes[] = { QMetaType::fromType<void>() };
        function->call(self, dummyArgs, dummyTypes, argc, context);
        return;
    }
    Q_ASSERT(types); // both args and types must be present
    // implicitly sets the return value, which is args[0]
    function->call(self, args, types, argc, context);
}

QV4::ReturnedValue ExecutionEngine::callInContext(QV4::Function *function, QObject *self,
                                                  QV4::ExecutionContext *context, int argc,
                                                  const QV4::Value *argv)
{
    QV4::Scope scope(this);
    QV4::ScopedObject jsSelf(scope, QV4::QObjectWrapper::wrap(this, self));
    Q_ASSERT(jsSelf);
    return function->call(jsSelf, argv, argc, context);
}

void ExecutionEngine::initQmlGlobalObject()
{
    initializeGlobal();
    lockObject(*globalObject);
}

void ExecutionEngine::initializeGlobal()
{
    createQtObject();

    QV4::GlobalExtensions::init(globalObject, QJSEngine::AllExtensions);

#if QT_CONFIG(qml_locale)
    QQmlLocale::registerStringLocaleCompare(this);
    QQmlDateExtension::registerExtension(this);
    QQmlNumberExtension::registerExtension(this);
#endif

#if QT_CONFIG(qml_xml_http_request)
    qt_add_domexceptions(this);
    m_xmlHttpRequestData = qt_add_qmlxmlhttprequest(this);
#endif

    qt_add_sqlexceptions(this);

    {
        for (uint i = 0; i < globalObject->internalClass()->size; ++i) {
            if (globalObject->internalClass()->nameMap.at(i).isString()) {
                QV4::PropertyKey id = globalObject->internalClass()->nameMap.at(i);
                m_illegalNames.insert(id.toQString());
            }
        }
    }
}

void ExecutionEngine::createQtObject()
{
    QV4::Scope scope(this);
    QtObject *qtObject = new QtObject(this);
    QJSEngine::setObjectOwnership(qtObject, QJSEngine::JavaScriptOwnership);

    QV4::ScopedObject qtObjectWrapper(
                scope, QV4::QObjectWrapper::wrap(this, qtObject));
    QV4::ScopedObject qtNamespaceWrapper(
                scope, QV4::QMetaObjectWrapper::create(this, &Qt::staticMetaObject));
    QV4::ScopedObject qtObjectProtoWrapper(
                scope, qtObjectWrapper->getPrototypeOf());

    qtNamespaceWrapper->setPrototypeOf(qtObjectProtoWrapper);
    qtObjectWrapper->setPrototypeOf(qtNamespaceWrapper);

    globalObject->defineDefaultProperty(QStringLiteral("Qt"), qtObjectWrapper);
}

const QSet<QString> &ExecutionEngine::illegalNames() const
{
    return m_illegalNames;
}

void ExecutionEngine::setQmlEngine(QQmlEngine *engine)
{
    // Second stage of initialization. We're updating some more prototypes here.
    isInitialized = false;
    m_qmlEngine = engine;
    initQmlGlobalObject();
    isInitialized = true;
}

static void freeze_recursive(QV4::ExecutionEngine *v4, QV4::Object *object)
{
    if (object->as<QV4::QObjectWrapper>() || object->internalClass()->isFrozen())
        return;

    QV4::Scope scope(v4);

    bool instanceOfObject = false;
    QV4::ScopedObject p(scope, object->getPrototypeOf());
    while (p) {
        if (p->d() == v4->objectPrototype()->d()) {
            instanceOfObject = true;
            break;
        }
        p = p->getPrototypeOf();
    }
    if (!instanceOfObject)
        return;

    Heap::InternalClass *frozen = object->internalClass()->frozen();
    object->setInternalClass(frozen); // Immediately assign frozen to prevent it from getting GC'd

    QV4::ScopedObject o(scope);
    for (uint i = 0; i < frozen->size; ++i) {
        if (!frozen->nameMap.at(i).isStringOrSymbol())
            continue;
        o = *object->propertyData(i);
        if (o)
            freeze_recursive(v4, o);
    }
}

void ExecutionEngine::freezeObject(const QV4::Value &value)
{
    QV4::Scope scope(this);
    QV4::ScopedObject o(scope, value);
    freeze_recursive(this, o);
}

void ExecutionEngine::lockObject(const QV4::Value &value)
{
    QV4::Scope scope(this);
    ScopedObject object(scope, value);
    if (!object)
        return;

    std::vector<Heap::Object *> stack { object->d() };

    // Methods meant to be overridden
    const PropertyKey writableMembers[] = {
        id_toString()->propertyKey(),
        id_toLocaleString()->propertyKey(),
        id_valueOf()->propertyKey(),
        id_constructor()->propertyKey()
    };
    const auto writableBegin = std::begin(writableMembers);
    const auto writableEnd = std::end(writableMembers);

    while (!stack.empty()) {
        object = stack.back();
        stack.pop_back();

        if (object->as<QV4::QObjectWrapper>() || object->internalClass()->isLocked())
            continue;

        Scoped<InternalClass> locked(scope, object->internalClass()->locked());
        QV4::ScopedObject member(scope);

        // Taking this copy is cheap. It's refcounted. This avoids keeping a reference
        // to the original IC.
        const SharedInternalClassData<PropertyKey> nameMap = locked->d()->nameMap;

        for (uint i = 0, end = locked->d()->size; i < end; ++i) {
            const PropertyKey key = nameMap.at(i);
            if (!key.isStringOrSymbol())
                continue;
            if ((member = *object->propertyData(i))) {
                stack.push_back(member->d());
                if (std::find(writableBegin, writableEnd, key) == writableEnd) {
                    PropertyAttributes attributes = locked->d()->find(key).attributes;
                    attributes.setConfigurable(false);
                    attributes.setWritable(false);
                    locked = locked->changeMember(key, attributes);
                }
            }
        }

        object->setInternalClass(locked->d());
    }
}

void ExecutionEngine::startTimer(const QString &timerName)
{
    if (!m_time.isValid())
        m_time.start();
    m_startedTimers[timerName] = m_time.elapsed();
}

qint64 ExecutionEngine::stopTimer(const QString &timerName, bool *wasRunning)
{
    if (!m_startedTimers.contains(timerName)) {
        *wasRunning = false;
        return 0;
    }
    *wasRunning = true;
    qint64 startedAt = m_startedTimers.take(timerName);
    return m_time.elapsed() - startedAt;
}

int ExecutionEngine::consoleCountHelper(const QString &file, quint16 line, quint16 column)
{
    const QString key = file + QString::number(line) + QString::number(column);
    int number = m_consoleCount.value(key, 0);
    number++;
    m_consoleCount.insert(key, number);
    return number;
}

void ExecutionEngine::setExtensionData(int index, Deletable *data)
{
    if (m_extensionData.size() <= index)
        m_extensionData.resize(index + 1);

    if (m_extensionData.at(index))
        delete m_extensionData.at(index);

    m_extensionData[index] = data;
}

template<typename Source>
bool convertToIterable(QMetaType metaType, void *data, Source *sequence)
{
    QSequentialIterable iterable;
    if (!QMetaType::view(metaType, data, QMetaType::fromType<QSequentialIterable>(), &iterable))
        return false;

    const QMetaType elementMetaType = iterable.valueMetaType();
    QVariant element(elementMetaType);
    for (qsizetype i = 0, end = sequence->getLength(); i < end; ++i) {
        if (!ExecutionEngine::metaTypeFromJS(sequence->get(i), elementMetaType, element.data()))
            element = QVariant(elementMetaType);
        iterable.addValue(element, QSequentialIterable::AtEnd);
    }
    return true;
}

// Converts a JS value to a meta-type.
// data must point to a place that can store a value of the given type.
// Returns true if conversion succeeded, false otherwise.
bool ExecutionEngine::metaTypeFromJS(const Value &value, QMetaType metaType, void *data)
{
    // check if it's one of the types we know
    switch (metaType.id()) {
    case QMetaType::Bool:
        *reinterpret_cast<bool*>(data) = value.toBoolean();
        return true;
    case QMetaType::Int:
        *reinterpret_cast<int*>(data) = value.toInt32();
        return true;
    case QMetaType::UInt:
        *reinterpret_cast<uint*>(data) = value.toUInt32();
        return true;
    case QMetaType::Long:
        *reinterpret_cast<long*>(data) = long(value.toInteger());
        return true;
    case QMetaType::ULong:
        *reinterpret_cast<ulong*>(data) = ulong(value.toInteger());
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
        if (value.isUndefined())
            *reinterpret_cast<QString*>(data) = QStringLiteral("undefined");
        else if (value.isNull())
            *reinterpret_cast<QString*>(data) = QStringLiteral("null");
        else
            *reinterpret_cast<QString*>(data) = value.toQString();
        return true;
    case QMetaType::QByteArray:
        if (const ArrayBuffer *ab = value.as<ArrayBuffer>()) {
            *reinterpret_cast<QByteArray*>(data) = ab->asByteArray();
        } else if (const String *string = value.as<String>()) {
            *reinterpret_cast<QByteArray*>(data) = string->toQString().toUtf8();
        } else if (const ArrayObject *ao = value.as<ArrayObject>()) {
            // Since QByteArray is sequentially iterable, we have to construct it from a JS Array.
            QByteArray result;
            const qint64 length = ao->getLength();
            result.reserve(length);
            for (qint64 i = 0; i < length; ++i) {
                char value = 0;
                ExecutionEngine::metaTypeFromJS(ao->get(i), QMetaType::fromType<char>(), &value);
                result.push_back(value);
            }
            *reinterpret_cast<QByteArray*>(data) = std::move(result);
        } else {
            *reinterpret_cast<QByteArray*>(data) = QByteArray();
        }
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
    case QMetaType::SChar:
        *reinterpret_cast<signed char*>(data) = (signed char)(value.toInt32());
        return true;
    case QMetaType::QChar:
        if (String *s = value.stringValue()) {
            QString str = s->toQString();
            *reinterpret_cast<QChar*>(data) = str.isEmpty() ? QChar() : str.at(0);
        } else {
            *reinterpret_cast<QChar*>(data) = QChar(ushort(value.toUInt16()));
        }
        return true;
    case QMetaType::QDateTime:
        if (const QV4::DateObject *d = value.as<DateObject>()) {
            *reinterpret_cast<QDateTime *>(data) = d->toQDateTime();
            return true;
        } break;
    case QMetaType::QDate:
        if (const QV4::DateObject *d = value.as<DateObject>()) {
            *reinterpret_cast<QDate *>(data) = DateObject::dateTimeToDate(d->toQDateTime());
            return true;
        } break;
    case QMetaType::QTime:
        if (const QV4::DateObject *d = value.as<DateObject>()) {
            *reinterpret_cast<QTime *>(data) = d->toQDateTime().time();
            return true;
        } break;
    case QMetaType::QUrl:
        if (String *s = value.stringValue()) {
            *reinterpret_cast<QUrl *>(data) = QUrl(s->toQString());
            return true;
        } else if (const QV4::UrlObject *d = value.as<UrlObject>()) {
            *reinterpret_cast<QUrl *>(data) = d->toQUrl();
            return true;
        } else if (const QV4::VariantObject *d = value.as<VariantObject>()) {
            const QVariant *variant = &d->d()->data();
            if (variant->metaType() == QMetaType::fromType<QUrl>()) {
                *reinterpret_cast<QUrl *>(data)
                        = *reinterpret_cast<const QUrl *>(variant->constData());
                return true;
            }
        }
        break;
#if QT_CONFIG(regularexpression)
    case QMetaType::QRegularExpression:
        if (const QV4::RegExpObject *r = value.as<QV4::RegExpObject>()) {
            *reinterpret_cast<QRegularExpression *>(data) = r->toQRegularExpression();
            return true;
        } break;
#endif
    case QMetaType::QObjectStar: {
        if (value.isNull()) {
            *reinterpret_cast<QObject* *>(data) = nullptr;
            return true;
        }
        if (value.as<QV4::QObjectWrapper>()) {
            *reinterpret_cast<QObject* *>(data) = qtObjectFromJS(value);
            return true;
        }
        break;
    }
    case QMetaType::QStringList: {
        const QV4::ArrayObject *a = value.as<QV4::ArrayObject>();
        if (a) {
            *reinterpret_cast<QStringList *>(data) = a->toQStringList();
            return true;
        }
        break;
    }
    case QMetaType::QVariantList: {
        const QV4::ArrayObject *a = value.as<QV4::ArrayObject>();
        if (a) {
            *reinterpret_cast<QVariantList *>(data) = ExecutionEngine::toVariant(
                        *a, /*typeHint*/QMetaType{}, /*createJSValueForObjectsAndSymbols*/false)
                .toList();
            return true;
        }
        break;
    }
    case QMetaType::QVariantMap: {
        const QV4::Object *o = value.as<QV4::Object>();
        if (o) {
            *reinterpret_cast<QVariantMap *>(data) = o->engine()->variantMapFromJS(o);
            return true;
        }
        break;
    }
    case QMetaType::QVariant:
        if (value.as<QV4::Managed>()) {
            *reinterpret_cast<QVariant*>(data) = ExecutionEngine::toVariant(
                    value, /*typeHint*/QMetaType{}, /*createJSValueForObjectsAndSymbols*/false);
        } else if (value.isNull()) {
            *reinterpret_cast<QVariant*>(data) = QVariant::fromValue(nullptr);
        } else if (value.isUndefined()) {
            *reinterpret_cast<QVariant*>(data) = QVariant();
        } else if (value.isBoolean()) {
            *reinterpret_cast<QVariant*>(data) = QVariant(value.booleanValue());
        } else if (value.isInteger()) {
            *reinterpret_cast<QVariant*>(data) = QVariant(value.integerValue());
        } else if (value.isDouble()) {
            *reinterpret_cast<QVariant*>(data) = QVariant(value.doubleValue());
        }
        return true;
    case QMetaType::QJsonValue:
        *reinterpret_cast<QJsonValue *>(data) = QV4::JsonObject::toJsonValue(value);
        return true;
    case QMetaType::QJsonObject: {
        *reinterpret_cast<QJsonObject *>(data) = QV4::JsonObject::toJsonObject(value.as<Object>());
        return true;
    }
    case QMetaType::QJsonArray: {
        const QV4::ArrayObject *a = value.as<ArrayObject>();
        if (a) {
            *reinterpret_cast<QJsonArray *>(data) = JsonObject::toJsonArray(a);
            return true;
        }
        break;
    }
#if QT_CONFIG(qml_locale)
    case QMetaType::QLocale: {
        if (const QV4::QQmlLocaleData *l = value.as<QQmlLocaleData>()) {
            *reinterpret_cast<QLocale *>(data) = *l->d()->locale;
            return true;
        }
        break;
    }
#endif
    default:
        break;
    }

    if (metaType.flags() & QMetaType::IsEnumeration) {
        *reinterpret_cast<int *>(data) = value.toInt32();
        return true;
    }

    if (metaType == QMetaType::fromType<QQmlListReference>()) {
        if (const QV4::QmlListWrapper *wrapper = value.as<QV4::QmlListWrapper>()) {
            *reinterpret_cast<QQmlListReference *>(data) = wrapper->toListReference();
            return true;
        }
    }

    if (metaType == QMetaType::fromType<QQmlListProperty<QObject>>()) {
        if (const QV4::QmlListWrapper *wrapper = value.as<QV4::QmlListWrapper>()) {
            *reinterpret_cast<QQmlListProperty<QObject> *>(data) = wrapper->d()->property();
            return true;
        }
    }

    if (const QQmlValueTypeWrapper *vtw = value.as<QQmlValueTypeWrapper>()) {
        const QMetaType valueType = vtw->type();
        if (valueType == metaType)
            return vtw->toGadget(data);

        Heap::QQmlValueTypeWrapper *d = vtw->d();
        if (d->isReference())
            d->readReference();

        if (void *gadgetPtr = d->gadgetPtr()) {
            if (QQmlValueTypeProvider::createValueType(metaType, data, valueType, gadgetPtr))
                return true;
            if (QMetaType::canConvert(valueType, metaType))
                return QMetaType::convert(valueType, gadgetPtr, metaType, data);
        } else {
            QVariant empty(valueType);
            if (QQmlValueTypeProvider::createValueType(metaType, data, valueType, empty.data()))
                return true;
            if (QMetaType::canConvert(valueType, metaType))
                return QMetaType::convert(valueType, empty.data(), metaType, data);
        }
    }

    // Try to use magic; for compatibility with qjsvalue_cast.

    if (convertToNativeQObject(value, metaType, reinterpret_cast<void **>(data)))
        return true;

    const bool isPointer = (metaType.flags() & QMetaType::IsPointer);
    const QV4::VariantObject *variantObject = value.as<QV4::VariantObject>();
    if (variantObject) {
        // Actually a reference, because we're poking it for its data() below and we want
        // the _original_ data, not some copy.
        QVariant &var = variantObject->d()->data();

        if (var.metaType() == metaType) {
            metaType.destruct(data);
            metaType.construct(data, var.data());
            return true;
        }

        if (isPointer) {
            const QByteArray pointedToTypeName = QByteArray(metaType.name()).chopped(1);
            const QMetaType valueType = QMetaType::fromName(pointedToTypeName);

            if (valueType == var.metaType()) {
                // ### Qt7: Remove this. Returning pointers to potentially gc'd data is crazy.
                // We have T t, T* is requested, so return &t.
                *reinterpret_cast<const void **>(data) = var.data();
                return true;
            } else if (Object *o = value.objectValue()) {
                // Look in the prototype chain.
                QV4::Scope scope(o->engine());
                QV4::ScopedObject proto(scope, o->getPrototypeOf());
                while (proto) {
                    bool canCast = false;
                    if (QV4::VariantObject *vo = proto->as<QV4::VariantObject>()) {
                        const QVariant &v = vo->d()->data();
                        canCast = (metaType == v.metaType());
                    }
                    else if (proto->as<QV4::QObjectWrapper>()) {
                        QV4::ScopedObject p(scope, proto.getPointer());
                        if (QObject *qobject = qtObjectFromJS(p)) {
                            if (const QMetaObject *metaObject = metaType.metaObject())
                                canCast = metaObject->cast(qobject) != nullptr;
                            else
                                canCast = qobject->qt_metacast(pointedToTypeName);
                        }
                    }
                    if (canCast) {
                        const QMetaType varType = var.metaType();
                        if (varType.flags() & QMetaType::IsPointer) {
                            *reinterpret_cast<const void **>(data)
                                = *reinterpret_cast<void *const *>(var.data());
                        } else {
                            *reinterpret_cast<const void **>(data) = var.data();
                        }
                        return true;
                    }
                    proto = proto->getPrototypeOf();
                }
            }
        } else if (QQmlValueTypeProvider::createValueType(
                       metaType, data, var.metaType(), var.data())) {
            return true;
        }
    } else if (value.isNull() && isPointer) {
        *reinterpret_cast<void* *>(data) = nullptr;
        return true;
    } else if (metaType == QMetaType::fromType<QJSValue>()) {
        QJSValuePrivate::setValue(reinterpret_cast<QJSValue*>(data), value.asReturnedValue());
        return true;
    } else if (metaType == QMetaType::fromType<QJSPrimitiveValue>()) {
        *reinterpret_cast<QJSPrimitiveValue *>(data) = createPrimitive(&value);
        return true;
    } else if (!isPointer) {
        if (QQmlValueTypeProvider::createValueType(metaType, data, value))
            return true;
    }

    if (const QV4::Sequence *sequence = value.as<Sequence>()) {
        const QVariant result = QV4::SequencePrototype::toVariant(sequence);
        if (result.metaType() == metaType) {
            metaType.destruct(data);
            metaType.construct(data, result.constData());
            return true;
        }

        if (convertToIterable(metaType, data, sequence))
            return true;
    }

    if (const QV4::ArrayObject *array = value.as<ArrayObject>()) {
        if (convertToIterable(metaType, data, array))
            return true;
    }

    return false;
}

static bool convertToNativeQObject(const QV4::Value &value, QMetaType targetType, void **result)
{
    if (!(targetType.flags() & QMetaType::IsPointer))
        return false;
    if (QObject *qobject = qtObjectFromJS(value)) {
        // If the target type has a metaObject, use that for casting.
        if (const QMetaObject *targetMetaObject = targetType.metaObject()) {
            if (QObject *instance = targetMetaObject->cast(qobject)) {
                *result = instance;
                return true;
            }
            return false;
        }

        // We have to call the generated qt_metacast rather than metaObject->cast() here so that
        // it works for types without QMetaObject, such as QStandardItem.
        const QByteArray targetTypeName = targetType.name();
        const int start = targetTypeName.startsWith("const ") ? 6 : 0;
        const QByteArray className = targetTypeName.mid(start, targetTypeName.size() - start - 1);
        if (void *instance = qobject->qt_metacast(className)) {
            *result = instance;
            return true;
        }
    }
    return false;
}

static QObject *qtObjectFromJS(const QV4::Value &value)
{
    if (!value.isObject())
        return nullptr;

    QV4::Scope scope(value.as<QV4::Managed>()->engine());
    QV4::Scoped<QV4::VariantObject> v(scope, value);

    if (v) {
        QVariant variant = v->d()->data();
        int type = variant.userType();
        if (type == QMetaType::QObjectStar)
            return *reinterpret_cast<QObject* const *>(variant.constData());
    }
    QV4::Scoped<QV4::QObjectWrapper> wrapper(scope, value);
    if (wrapper)
        return wrapper->object();

    QV4::Scoped<QV4::QQmlTypeWrapper> typeWrapper(scope, value);
    if (typeWrapper)
        return typeWrapper->object();

    return nullptr;
}

struct QV4EngineRegistrationData
{
    QV4EngineRegistrationData() : extensionCount(0) {}

    QMutex mutex;
    int extensionCount;
};
Q_GLOBAL_STATIC(QV4EngineRegistrationData, registrationData);

QMutex *ExecutionEngine::registrationMutex()
{
    return &registrationData()->mutex;
}

int ExecutionEngine::registerExtension()
{
    return registrationData()->extensionCount++;
}

#if QT_CONFIG(qml_network)
QNetworkAccessManager *QV4::detail::getNetworkAccessManager(ExecutionEngine *engine)
{
    return engine->qmlEngine()->networkAccessManager();
}
#endif // qml_network

QT_END_NAMESPACE
