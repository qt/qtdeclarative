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

#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <memory>

class tst_qv4mm : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qv4mm();

private slots:
    void gcStats();
    void persistentValueMarking_data();
    void persistentValueMarking();
    void multiWrappedQObjects();
    void accessParentOnDestruction();
    void cleanInternalClasses();
    void createObjectsOnDestruction();
    void sharedInternalClassDataMarking();
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
    engine.memoryManager->gcBlocked = true;
    QV4::Heap::String *s = engine.newString(QString::fromLatin1("test"));
    QV4::PropertyKey id = engine.identifierTable->asPropertyKeyImpl(s);
    engine.memoryManager->gcBlocked = false;
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

QTEST_MAIN(tst_qv4mm)

#include "tst_qv4mm.moc"
