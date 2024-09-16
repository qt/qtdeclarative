// Copyright (C) 2016 basysKom GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QQmlEngine>
#include <QLoggingCategory>
#include <QQmlComponent>

#include <private/qv4mm_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qjsvalue_p.h>
#include <private/qqmlengine_p.h>
#include <private/qv4identifiertable_p.h>
#include <private/qv4arraydata_p.h>
#include <private/qqmlcomponentattached_p.h>
#include <private/qv4mapobject_p.h>
#include <private/qv4setobject_p.h>
#if QT_CONFIG(qml_jit)
#include <private/qv4baselinejit_p.h>
#endif

#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <memory>

class tst_qv4mm : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qv4mm();

private slots:
    void gcStats();
    void arrayDataWriteBarrierInteraction();
    void persistentValueMarking_data();
    void persistentValueMarking();
    void multiWrappedQObjects();
    void accessParentOnDestruction();
    void cleanInternalClasses();
    void createObjectsOnDestruction();
    void sharedInternalClassDataMarking();
    void gcTriggeredInOnDestroyed();
    void weakValuesAssignedAfterThePhaseThatShouldHandleWeakValues();
    void mapAndSetKeepValuesAlive();
    void jittedStoreLocalMarksValue();
};

tst_qv4mm::tst_qv4mm()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    QV4::ExecutionEngine engine;
    QV4::Scope scope(engine.rootContext());
}

void tst_qv4mm::gcStats()
{
    QLoggingCategory::setFilterRules("qt.qml.gc.*=true");
    QQmlEngine engine;
    gc(engine);
    QLoggingCategory::setFilterRules("qt.qml.gc.*=false");
}

void tst_qv4mm::arrayDataWriteBarrierInteraction()
{
    QV4::ExecutionEngine engine;
    QCOMPARE(engine.memoryManager->gcBlocked, QV4::MemoryManager::Unblocked);
    engine.memoryManager->gcBlocked = QV4::MemoryManager::InCriticalSection;
    QV4::Heap::Object *unprotectedObject = engine.newObject();
    QV4::Scope scope(&engine);
    QV4::ScopedArrayObject array(scope, engine.newArrayObject());
    constexpr int initialCapacity = 8; // compare qv4arraydata.cpp
    for (int i = 0; i < initialCapacity; ++i) {
        array->push_back(unprotectedObject->asReturnedValue());
    }
    QVERIFY(!unprotectedObject->isMarked());
    engine.memoryManager->gcBlocked = QV4::MemoryManager::Unblocked;

    // initialize gc
    auto sm = engine.memoryManager->gcStateMachine.get();
    sm->reset();
    while (sm->state != QV4::GCState::MarkGlobalObject) {
        QV4::GCStateInfo& stateInfo = sm->stateInfoMap[int(sm->state)];
        sm->state = stateInfo.execute(sm, sm->stateData);
    }

    array->push_back(QV4::Value::fromUInt32(42));
    QVERIFY(!unprotectedObject->isMarked());
    // we should have pushed the new arraydata on the mark stack
    // so if we call drain...
    engine.memoryManager->markStack()->drain();
    // the unprotectedObject should have been marked
    QVERIFY(unprotectedObject->isMarked());
}

enum PVSetOption {
    CopyCtor,
    ValueCtor,
    ObjectCtor,
    ReturnedValueCtor,
    WeakValueAssign,
    ObjectAssign,
};

void tst_qv4mm::persistentValueMarking_data()
{
    QTest::addColumn<PVSetOption>("setOption");

    QTest::addRow("copy") << CopyCtor;
    QTest::addRow("valueCtor") << ValueCtor;
    QTest::addRow("ObjectCtor") << ObjectCtor;
    QTest::addRow("ReturnedValueCtor") << ReturnedValueCtor;
    QTest::addRow("WeakValueAssign") << WeakValueAssign;
    QTest::addRow("ObjectAssign") << ObjectAssign;
}

void tst_qv4mm::persistentValueMarking()
{
    QFETCH(PVSetOption, setOption);
    QV4::ExecutionEngine engine;
    QV4::PersistentValue persistentOrigin; // used for copy ctor
    QV4::Heap::Object *unprotectedObject = engine.newObject();
    {
        QV4::Scope scope(engine.rootContext());
        QV4::ScopedObject object {scope, unprotectedObject};
        persistentOrigin.set(&engine, object);
        QVERIFY(!unprotectedObject->isMarked());
    }
    auto sm = engine.memoryManager->gcStateMachine.get();
    sm->reset();
    while (sm->state != QV4::GCState::MarkGlobalObject) {
        QV4::GCStateInfo& stateInfo = sm->stateInfoMap[int(sm->state)];
        sm->state = stateInfo.execute(sm, sm->stateData);
    }
    QVERIFY(engine.isGCOngoing);
    QVERIFY(!unprotectedObject->isMarked());
    switch (setOption) {
    case CopyCtor: {
        QV4::PersistentValue persistentCopy(persistentOrigin);
        QVERIFY(unprotectedObject->isMarked());
        break;
    }
    case ValueCtor: {
        QV4::Value val = QV4::Value::fromHeapObject(unprotectedObject);
        QV4::PersistentValue persistent(&engine, val);
        QVERIFY(unprotectedObject->isMarked());
        break;
    }
    case ObjectCtor: {
        QV4::Scope scope(&engine);
        QV4::ScopedObject o(scope, unprotectedObject);
        // scoped object without scan shouldn't result in marking
        QVERIFY(!unprotectedObject->isMarked());
        QV4::PersistentValue persistent(&engine, o.getPointer());
        QVERIFY(unprotectedObject->isMarked());
        break;
    }
    case ReturnedValueCtor: {
        QV4::PersistentValue persistent(&engine, unprotectedObject->asReturnedValue());
        QVERIFY(unprotectedObject->isMarked());
        break;
    }
    case WeakValueAssign: {
        QV4::WeakValue wv;
        wv.set(&engine, unprotectedObject);
        QVERIFY(!unprotectedObject->isMarked());
        QV4::PersistentValue persistent;
        persistent = wv;
        break;
    }
    case ObjectAssign: {
        QV4::Scope scope(&engine);
        QV4::ScopedObject o(scope, unprotectedObject);
        // scoped object without scan shouldn't result in marking
        QVERIFY(!unprotectedObject->isMarked());
        QV4::PersistentValue persistent;
        persistent = o;
        QVERIFY(unprotectedObject->isMarked());
        break;
    }
    }
}

void tst_qv4mm::multiWrappedQObjects()
{
    QV4::ExecutionEngine engine1;
    QV4::ExecutionEngine engine2;
    {
        QObject object;
        for (int i = 0; i < 10; ++i)
            QV4::QObjectWrapper::ensureWrapper(i % 2 ? &engine1 : &engine2, &object);

        QCOMPARE(engine1.memoryManager->m_pendingFreedObjectWrapperValue.size(), 0);
        QCOMPARE(engine2.memoryManager->m_pendingFreedObjectWrapperValue.size(), 0);
        {
            QV4::WeakValue value;
            value.set(&engine1, QV4::QObjectWrapper::wrap(&engine1, &object));
        }

        QCOMPARE(engine1.memoryManager->m_pendingFreedObjectWrapperValue.size(), 1);
        QCOMPARE(engine2.memoryManager->m_pendingFreedObjectWrapperValue.size(), 0);

        // The additional WeakValue from m_multiplyWrappedQObjects hasn't been moved
        // to m_pendingFreedObjectWrapperValue yet. It's still alive after all.
        gc(engine1);
        QCOMPARE(engine1.memoryManager->m_pendingFreedObjectWrapperValue.size(), 1);

        // engine2 doesn't own the object as engine1 was the first to wrap it above.
        // Therefore, no effect here.
        gc(engine2);
        QCOMPARE(engine2.memoryManager->m_pendingFreedObjectWrapperValue.size(), 0);
    }

    // Clears m_pendingFreedObjectWrapperValue. Now it's really dead.
    gc(engine1);
    QCOMPARE(engine1.memoryManager->m_pendingFreedObjectWrapperValue.size(), 0);

    gc(engine2);
    QCOMPARE(engine2.memoryManager->m_pendingFreedObjectWrapperValue.size(), 0);
}

void tst_qv4mm::accessParentOnDestruction()
{
    QQmlEngine engine;

    QQmlComponent component(&engine, testFileUrl("createdestroy.qml"));
    std::unique_ptr<QObject> obj(component.create());
    QVERIFY(obj);
    QPointer<QObject> timer = qvariant_cast<QObject *>(obj->property("timer"));
    QVERIFY(timer);
    QTRY_VERIFY(!timer->property("running").toBool());
    QCOMPARE(obj->property("iterations").toInt(), 100);
    QCOMPARE(obj->property("creations").toInt(), 100);
    gc(engine); // ensure incremental gc has finished, and collected all objects
    // TODO: investigaet whether we really need two gc rounds for incremental gc
    gc(engine); // ensure incremental gc has finished, and collected all objects
    QCOMPARE(obj->property("destructions").toInt(), 100);
}

void tst_qv4mm::cleanInternalClasses()
{
    QV4::ExecutionEngine engine;
    QV4::Scope scope(engine.rootContext());
    QV4::ScopedObject object(scope, engine.newObject());
    QV4::ScopedObject prototype(scope, engine.newObject());

    // Set a prototype so that we get a unique IC.
    object->setPrototypeOf(prototype);

    QV4::Scoped<QV4::InternalClass> prevIC(scope, object->internalClass());
    QVERIFY(prevIC->d()->transitions.empty());

    uint prevIcChainLength = 0;
    for (QV4::Heap::InternalClass *ic = object->internalClass(); ic; ic = ic->parent)
        ++prevIcChainLength;

    const auto checkICCHainLength = [&]() {
        uint icChainLength = 0;
        for (QV4::Heap::InternalClass *ic = object->internalClass(); ic; ic = ic->parent)
            ++icChainLength;

        const uint redundant = object->internalClass()->numRedundantTransitions;
        QVERIFY(redundant <= QV4::Heap::InternalClass::MaxRedundantTransitions);

        // A removal makes two transitions redundant.
        QVERIFY(icChainLength <= prevIcChainLength + 2 * redundant);
    };

    const uint numTransitions = 16 * 1024;

    // Keep identifiers in a separate array so that we don't have to allocate them in the loop that
    // should test the GC on InternalClass allocations.
    QV4::ScopedArrayObject identifiers(scope, engine.newArrayObject());
    for (uint i = 0; i < numTransitions; ++i) {
        QV4::Scope scope(&engine);
        QV4::ScopedString s(scope);
        s = engine.newIdentifier(QString::fromLatin1("key%1").arg(i));
        identifiers->push_back(s);

        QV4::ScopedValue v(scope);
        v->setDouble(i);
        object->insertMember(s, v);
    }

    // There is a chain of ICs originating from the original class.
    QCOMPARE(prevIC->d()->transitions.size(), 1u);
    QVERIFY(prevIC->d()->transitions.front().lookup != nullptr);

    // When allocating the InternalClass objects required for deleting properties, eventually
    // the IC chain gets truncated, dropping all the removed properties.
    for (uint i = 0; i < numTransitions; ++i) {
        QV4::Scope scope(&engine);
        QV4::ScopedString s(scope, identifiers->get(i));
        QV4::Scoped<QV4::InternalClass> ic(scope, object->internalClass());
        QVERIFY(ic->d()->parent != nullptr);
        QV4::ScopedValue val(scope, object->get(s->toPropertyKey()));
        QCOMPARE(val->toNumber(), double(i));
        QVERIFY(object->deleteProperty(s->toPropertyKey()));
        QVERIFY(!object->hasProperty(s->toPropertyKey()));
        QVERIFY(object->internalClass() != ic->d());
    }

    // None of the properties we've added are left
    for (uint i = 0; i < numTransitions; ++i) {
        QV4::ScopedString s(scope, identifiers->get(i));
        QVERIFY(!object->hasProperty(s->toPropertyKey()));
    }

    // Also no other properties have appeared
    QScopedPointer<QV4::OwnPropertyKeyIterator> iterator(object->ownPropertyKeys(object));
    QVERIFY(!iterator->next(object).isValid());

    checkICCHainLength();

    // Add and remove properties until it clears all remaining redundant ones
    uint i = 0;
    while (object->internalClass()->numRedundantTransitions > 0) {
        i = (i + 1) % numTransitions;
        QV4::ScopedString s(scope, identifiers->get(i));
        QV4::ScopedValue v(scope);
        v->setDouble(i);
        object->insertMember(s, v);
        QVERIFY(object->deleteProperty(s->toPropertyKey()));
    }

    // Make sure that all dangling ICs are actually gone.
    gc(engine);
    // NOTE: If we allocate new ICs during gc (potentially triggered on alloc),
    // then they will survive the previous gc call
    // run gc again to ensure that a full gc cycle happens
    gc(engine);

    // Now the GC has removed the ICs we originally added by adding properties.
    QVERIFY(prevIC->d()->transitions.empty() || prevIC->d()->transitions.front().lookup == nullptr);

    // Same thing with redundant prototypes
    for (uint i = 0; i < numTransitions; ++i) {
        QV4::ScopedObject prototype(scope, engine.newObject());
        object->setPrototypeOf(prototype); // Makes previous prototype redundant
    }

    checkICCHainLength();
}

void tst_qv4mm::createObjectsOnDestruction()
{
    QQmlEngine engine;

    QQmlComponent component(&engine, testFileUrl("createobjects.qml"));
    std::unique_ptr<QObject> obj(component.create());
    QVERIFY(obj);
    QCOMPARE(obj->property("numChecked").toInt(), 1000);
    QCOMPARE(obj->property("ok").toBool(), true);
}

void tst_qv4mm::sharedInternalClassDataMarking()
{
    QV4::ExecutionEngine engine;
    QV4::Scope scope(engine.rootContext());
    QV4::ScopedObject object(scope, engine.newObject());
    QVERIFY(!engine.memoryManager->gcBlocked);
    // no scoped classes, as that would defeat the point of the test
    // we block the gc instead so that the allocation can't trigger the gc
    engine.memoryManager->gcBlocked = QV4::MemoryManager::InCriticalSection;
    QV4::Heap::String *s = engine.newString(QString::fromLatin1("test"));
    QV4::PropertyKey id = engine.identifierTable->asPropertyKeyImpl(s);
    engine.memoryManager->gcBlocked = QV4::MemoryManager::Unblocked;
    QVERIFY(!id.asStringOrSymbol()->isMarked());

    auto sm = engine.memoryManager->gcStateMachine.get();
    sm->reset();
    while (sm->state != QV4::GCState::MarkGlobalObject) {
        QV4::GCStateInfo& stateInfo = sm->stateInfoMap[int(sm->state)];
        sm->state = stateInfo.execute(sm, sm->stateData);
    }

    // simulate partial marking caused by drain due mark stack running out of space
    // and running out of time during drain phase for complete marking
    // the last part is necessary for us to find not-already marked name/value pair to put into
    // the object

    QVERIFY(engine.memoryManager->markStack()->isEmpty());
    QVERIFY(!id.asStringOrSymbol()->isMarked());
    {

        // for simplcity's sake we create a new PropertyKey - if gc were actually ongoing that would
        // already mark it. In practice we would need to retrieve an existing one from an unmarked
        // object, and then make that object unreachable afterwards.
        object->put(id, QV4::Value::fromUInt32(42));
        engine.memoryManager->markStack()->drain();
        QVERIFY(id.asStringOrSymbol()->isMarked());
    }
    gc(engine);
    // sanity check that we still can lookup the value
    QV4::ScopedString s2(scope, engine.newString(QString::fromLatin1("test")));
    auto val = QV4::Value::fromReturnedValue(object->get(s2->toPropertyKey()));
    QCOMPARE(val.toUInt32(), 42u);
}

void tst_qv4mm::gcTriggeredInOnDestroyed()
{
    QQmlEngine engine;
    QV4::ExecutionEngine &v4 = *engine.handle();

    QPointer<QObject> testObject = new QObject; // unparented, will be deleted
    auto cleanup = qScopeGuard([&]() {
        if (testObject)
            testObject->deleteLater();
    });

    QQmlComponent component(&engine, testFileUrl("simpleObject.qml"));
    auto toBeCollected = component.create();
    QVERIFY(toBeCollected);
    QJSEngine::setObjectOwnership(toBeCollected, QJSEngine::JavaScriptOwnership);
    QV4::QObjectWrapper::ensureWrapper(&v4, toBeCollected);
    QVERIFY(qmlEngine(toBeCollected));
    QQmlComponentAttached *attached = QQmlComponent::qmlAttachedProperties(toBeCollected);
    QVERIFY(attached);


    QV4::Scope scope(v4.rootContext());
    QCOMPARE(v4.memoryManager->gcBlocked, QV4::MemoryManager::Unblocked);



    // let the gc run up to CallDestroyObjects
    auto sm = v4.memoryManager->gcStateMachine.get();
    sm->reset();
    v4.memoryManager->gcBlocked = QV4::MemoryManager::NormalBlocked;
    while (sm->state != QV4::GCState::CallDestroyObjects && sm->state != QV4::GCState::Invalid) {
        QV4::GCStateInfo& stateInfo = sm->stateInfoMap[int(sm->state)];
        sm->state = stateInfo.execute(sm, sm->stateData);
    }
    QCOMPARE(sm->state, QV4::GCState::CallDestroyObjects);

    QV4::ScopedValue val(scope);
    bool calledOnDestroyed = false;
    auto con = connect(attached, &QQmlComponentAttached::destruction, this, [&]() {
        calledOnDestroyed = true;
        // we trigger uncommon code paths:
        // create ObjectWrapper in destroyed hadnler
        auto ddata = QQmlData::get(testObject.get(), false);
        QVERIFY(!ddata); // we don't have ddata yet (otherwise we'd already have an object wrapper)
        val = QV4::QObjectWrapper::wrap(&v4, testObject.get());
        QJSEngine::setObjectOwnership(testObject, QJSEngine::JavaScriptOwnership);

        // and also try to trigger a force gc completion
        bool gcComplete = v4.memoryManager->tryForceGCCompletion();
        QVERIFY(!gcComplete);
    });
    while (!calledOnDestroyed && sm->state != QV4::GCState::Invalid) {
        QV4::GCStateInfo& stateInfo = sm->stateInfoMap[int(sm->state)];
        sm->state = stateInfo.execute(sm, sm->stateData);
    }
    QVERIFY(!QTest::currentTestFailed());
    QObject::disconnect(con);
    QVERIFY(calledOnDestroyed);

    bool gcComplete = v4.memoryManager->tryForceGCCompletion();
    QVERIFY(gcComplete);
    val = QV4::Value::undefinedValue(); // no longer keep a reference on the stack
    QCOMPARE(sm->state, QV4::GCState::Invalid);
    QVERIFY(testObject); // must not have be deleted, referenced by val

    gc(v4); // run another gc cycle
    QVERIFY(!testObject); // now collcted by gc
}
void tst_qv4mm::weakValuesAssignedAfterThePhaseThatShouldHandleWeakValues()
{
    QObject testObject;
    QV4::ExecutionEngine v4;

    QCOMPARE(v4.memoryManager->gcBlocked, QV4::MemoryManager::Unblocked);



    // let the gc run up to CallDestroyObjects
    auto sm = v4.memoryManager->gcStateMachine.get();
    sm->reset();
    v4.memoryManager->gcBlocked = QV4::MemoryManager::NormalBlocked;


    // run just before the sweeping face
    while (sm->state != QV4::GCState::DoSweep && sm->state != QV4::GCState::Invalid) {
        QV4::GCStateInfo& stateInfo = sm->stateInfoMap[int(sm->state)];
        sm->state = stateInfo.execute(sm, sm->stateData);
    }
    QCOMPARE(sm->state, QV4::GCState::DoSweep);

    {
        // simulate code accessing the object wrapper for an object
        QV4::Scope scope(v4.rootContext());
        QV4::ScopedValue value(scope);
        value = QV4::QObjectWrapper::wrap(&v4, &testObject);
        // let it go out of scope before any stack re-scanning could happen
    }

    bool gcComplete = v4.memoryManager->tryForceGCCompletion();
    QVERIFY(gcComplete);

    auto ddata = QQmlData::get(&testObject);
    QVERIFY(ddata);
    if (ddata->jsWrapper.isUndefined()) {
        // it's in principle valid for the wrapper to be reset, though the current
        // implementation doesn't do it, and it requires some care
        qWarning("Double-check the handling of weak values and object wrappers in the gc");
        return;
    }
    QVERIFY(ddata->jsWrapper.valueRef()->heapObject()->inUse());
}

void tst_qv4mm::mapAndSetKeepValuesAlive()
{
    {
        QJSEngine jsEngine;
        QV4::ExecutionEngine &engine = *jsEngine.handle();

        QV4::Scope scope(&engine);
        auto map = jsEngine.evaluate("new Map()");
        QV4::ScopedFunctionObject afunction(scope, engine.memoryManager->alloc<QV4::FunctionObject>()); // hack, we just need about any function object
        QV4::Value thisObject = QJSValuePrivate::asReturnedValue(&map);

        QVERIFY(!engine.memoryManager->gcBlocked);
        // no scoped classes, as that would defeat the point of the test
        // we block the gc instead so that the allocation can't trigger the gc
        engine.memoryManager->gcBlocked = QV4::MemoryManager::InCriticalSection;
        QV4::Heap::String *key = engine.newString(QString::fromLatin1("key"));
        QV4::Heap::String *value = engine.newString(QString::fromLatin1("value"));
        QV4::Value values[2] = { QV4::Value::fromHeapObject(key), QV4::Value::fromHeapObject(value) };
        engine.memoryManager->gcBlocked = QV4::MemoryManager::Unblocked;
        QVERIFY(!key->isMarked());
        QVERIFY(!value->isMarked());

        auto sm = engine.memoryManager->gcStateMachine.get();
        sm->reset();
        while (sm->state != QV4::GCState::HandleQObjectWrappers) {
            QV4::GCStateInfo& stateInfo = sm->stateInfoMap[int(sm->state)];
            sm->state = stateInfo.execute(sm, sm->stateData);
        }
        QV4::MapPrototype::method_set(afunction.getPointer(), &thisObject, values, 2);

        // check that we can still insert primitve values - they don't get marked
        // but they also should not casue any corrpution - note that a weak map
        // only accepts object keys
        values[0] = QV4::Value::fromInt32(12);
        values[1] = QV4::Value::fromInt32(13);
        QV4::MapPrototype::method_set(afunction.getPointer(), &thisObject, values, 2);

        QVERIFY(key->isMarked());
        QVERIFY(value->isMarked());
        bool gcComplete = engine.memoryManager->tryForceGCCompletion();
        QVERIFY(gcComplete);
        QVERIFY(key->inUse());
        QVERIFY(value->inUse());
        gc(engine);
        QCOMPARE(map.property("size").toInt(), 2);
    }
    {
        QJSEngine jsEngine;
        QV4::ExecutionEngine &engine = *jsEngine.handle();

        QV4::Scope scope(&engine);
        auto map = jsEngine.evaluate("new WeakMap()");
        QV4::ScopedFunctionObject afunction(scope, engine.memoryManager->alloc<QV4::FunctionObject>()); // hack, we just need about any function object
        QV4::Value thisObject = QJSValuePrivate::asReturnedValue(&map);

        QVERIFY(!engine.memoryManager->gcBlocked);
        // no scoped classes, as that would defeat the point of the test
        // we block the gc instead so that the allocation can't trigger the gc
        engine.memoryManager->gcBlocked = QV4::MemoryManager::InCriticalSection;
        QV4::Heap::Object *key = engine.newObject();
        QV4::Heap::String *value = engine.newString(QString::fromLatin1("value"));
        QV4::Value values[2] = { QV4::Value::fromHeapObject(key), QV4::Value::fromHeapObject(value) };
        engine.memoryManager->gcBlocked = QV4::MemoryManager::Unblocked;
        QVERIFY(!key->isMarked());
        QVERIFY(!value->isMarked());

        auto sm = engine.memoryManager->gcStateMachine.get();
        sm->reset();
        while (sm->state != QV4::GCState::HandleQObjectWrappers) {
            QV4::GCStateInfo& stateInfo = sm->stateInfoMap[int(sm->state)];
            sm->state = stateInfo.execute(sm, sm->stateData);
        }
        QV4::WeakMapPrototype::method_set(afunction.getPointer(), &thisObject, values, 2);
        QVERIFY(!engine.hasException);
        QVERIFY(key->isMarked());
        QVERIFY(value->isMarked());
        bool gcComplete = engine.memoryManager->tryForceGCCompletion();
        QVERIFY(gcComplete);
        QVERIFY(key->inUse());
        QVERIFY(value->inUse());
        gc(engine);
        QCOMPARE(map.property("size").toInt(), 0);
    }
    {
        QJSEngine jsEngine;
        QV4::ExecutionEngine &engine = *jsEngine.handle();

        QV4::Scope scope(&engine);
        auto map = jsEngine.evaluate("new Set()");
        QV4::ScopedFunctionObject afunction(scope, engine.memoryManager->alloc<QV4::FunctionObject>()); // hack, we just need about any function object
        QV4::Value thisObject = QJSValuePrivate::asReturnedValue(&map);

        QVERIFY(!engine.memoryManager->gcBlocked);
        // no scoped classes, as that would defeat the point of the test
        // we block the gc instead so that the allocation can't trigger the gc
        engine.memoryManager->gcBlocked = QV4::MemoryManager::InCriticalSection;
        QV4::Heap::Object *key = engine.newObject();
        QV4::Value values[1] = { QV4::Value::fromHeapObject(key) };
        engine.memoryManager->gcBlocked = QV4::MemoryManager::Unblocked;
        QVERIFY(!key->isMarked());

        auto sm = engine.memoryManager->gcStateMachine.get();
        sm->reset();
        while (sm->state != QV4::GCState::HandleQObjectWrappers) {
            QV4::GCStateInfo& stateInfo = sm->stateInfoMap[int(sm->state)];
            sm->state = stateInfo.execute(sm, sm->stateData);
        }
        QV4::SetPrototype::method_add(afunction.getPointer(), &thisObject, values, 1);
        values[0] = QV4::Value::fromInt32(13);
        QV4::SetPrototype::method_add(afunction.getPointer(), &thisObject, values, 1);
        QVERIFY(!engine.hasException);
        QVERIFY(key->isMarked());
        bool gcComplete = engine.memoryManager->tryForceGCCompletion();
        QVERIFY(gcComplete);
        QVERIFY(key->inUse());
        gc(engine);
        QCOMPARE(map.property("size").toInt(), 2);
    }
    {
        QJSEngine jsEngine;
        QV4::ExecutionEngine &engine = *jsEngine.handle();

        QV4::Scope scope(&engine);
        auto map = jsEngine.evaluate("new WeakSet()");
        QV4::ScopedFunctionObject afunction(scope, engine.memoryManager->alloc<QV4::FunctionObject>()); // hack, we just need about any function object
        QV4::Value thisObject = QJSValuePrivate::asReturnedValue(&map);

        QVERIFY(!engine.memoryManager->gcBlocked);
        // no scoped classes, as that would defeat the point of the test
        // we block the gc instead so that the allocation can't trigger the gc
        engine.memoryManager->gcBlocked = QV4::MemoryManager::InCriticalSection;
        QV4::Heap::Object *key = engine.newObject();
        QV4::Value values[1] = { QV4::Value::fromHeapObject(key) };
        engine.memoryManager->gcBlocked = QV4::MemoryManager::Unblocked;
        QVERIFY(!key->isMarked());

        auto sm = engine.memoryManager->gcStateMachine.get();
        sm->reset();
        while (sm->state != QV4::GCState::HandleQObjectWrappers) {
            QV4::GCStateInfo& stateInfo = sm->stateInfoMap[int(sm->state)];
            sm->state = stateInfo.execute(sm, sm->stateData);
        }
        QV4::WeakSetPrototype::method_add(afunction.getPointer(), &thisObject, values, 1);
        QVERIFY(!engine.hasException);
        QVERIFY(key->isMarked());
        bool gcComplete = engine.memoryManager->tryForceGCCompletion();
        QVERIFY(gcComplete);
        QVERIFY(key->inUse());
        gc(engine);
        QCOMPARE(map.property("size").toInt(), 0);
    }
}

QV4::ReturnedValue method_force_jit(const QV4::FunctionObject *b, const QV4::Value *, const QV4::Value *argv, int argc)
{
#if QT_CONFIG(qml_jit)
    auto *v4 =b->engine();

    Q_ASSERT(argc == 1);
    QV4::Scope scope(v4);
    QV4::Scoped<QV4::JavaScriptFunctionObject> functionObject(scope, argv[0]);
    auto *func = static_cast<QV4::Heap::JavaScriptFunctionObject *>(functionObject->heapObject())->function;
    Q_ASSERT(func);
    func->interpreterCallCount = std::numeric_limits<int>::max();
    if (!v4->canJIT(func))
        return QV4::StaticValue::fromBoolean(false).asReturnedValue();
    QV4::JIT::BaselineJIT(func).generate();
    return QV4::StaticValue::fromBoolean(true).asReturnedValue();
#else
    return QV4::StaticValue::fromBoolean(false).asReturnedValue();
#endif
}

QV4::ReturnedValue method_setup_gc_for_test(const QV4::FunctionObject *b, const QV4::Value *, const QV4::Value *, int)
{
    auto *v4 =b->engine();
    auto *mm = v4->memoryManager;
    mm->runFullGC();

    auto sm = v4->memoryManager->gcStateMachine.get();
    sm->reset();
    while (sm->state != QV4::GCState::MarkGlobalObject) {
        QV4::GCStateInfo& stateInfo = sm->stateInfoMap[int(sm->state)];
        sm->state = stateInfo.execute(sm, sm->stateData);
    }

    return QV4::Encode::undefined();
}

QV4::ReturnedValue method_is_marked(const QV4::FunctionObject *, const QV4::Value *, const QV4::Value *argv, int argc)
{
    Q_ASSERT(argc == 1);

    auto h = argv[0].heapObject();
    Q_ASSERT(h);
    return QV4::Encode(h->isMarked());
}

void tst_qv4mm::jittedStoreLocalMarksValue()
{
    QQmlEngine engine;

    auto *v4 = engine.handle();
    auto globalObject = v4->globalObject;
    globalObject->defineDefaultProperty(QStringLiteral("__setupGC"), method_setup_gc_for_test);
    globalObject->defineDefaultProperty(QStringLiteral("__forceJit"), method_force_jit);
    globalObject->defineDefaultProperty(QStringLiteral("__isMarked"), method_is_marked);

    QQmlComponent comp(&engine, testFileUrl("storeLocal.qml"));
    QVERIFY(comp.isReady());
    std::unique_ptr<QObject> root {comp.create()};

    QVERIFY(root);
    bool ok = false;
    int result = root->property("result").toInt(&ok);
    QVERIFY(ok);
    if (result == -1)
        QSKIP("Could not run JIT");
    QCOMPARE(result, 0);
}

QTEST_MAIN(tst_qv4mm)

#include "tst_qv4mm.moc"
