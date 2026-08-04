// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qset.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlproperty.h>
#include <QtQml/qqmlengine.h>
#include <QtQmlDesignSupport/qmultiobjectregistryref.h>
#include <QtQmlDesignSupport/qobjectregistryref.h>
#include <QtQmlDesignSupport/private/qobjectregistry_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

using namespace Qt::StringLiterals;

class tst_qobjectregistry : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qobjectregistry()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {
    }

private slots:
    void assignmentOrder_data();
    void assignmentOrder();
    void boundKey();
    void cppRegistration();
    void dynamicObjectCreationAndDestruction();
    void imperativeTargetAssignment();
    void keyChange();
    void multiRegistration();
    void noEngine();
    void qmlReferences();
    void refChangedDuringNotification();
    void refDelete();
    void refInitialKeySet_data();
    void refInitialKeySet();
    void registryOutlivesEngine();
    void sameKeyMultipleEngines();
    void sameTargetRegisteredMultipleTimes();
    void singleRegistration_data();
    void singleRegistration();
    void staleTargetRemoval();
    void targetChange();
};

void tst_qobjectregistry::assignmentOrder_data()
{
    QTest::addColumn<bool>("targetFirst");

    QTest::newRow("Key first") << false;
    QTest::newRow("Target first") << true;
}

void tst_qobjectregistry::assignmentOrder()
{
    QFETCH(bool, targetFirst);

    QQmlEngine e;
    QObject target;

    QObjectRegistry registry;
    QQmlEngine::setContextForObject(&registry, e.rootContext());

    if (targetFirst) {
        registry.setTarget(&target);
        registry.setKey("AssignmentOrder");
    } else {
        registry.setKey("AssignmentOrder");
        registry.setTarget(&target);
    }

    QObjectRegistryRef ref(&e, "AssignmentOrder");
    QVERIFY(ref.object() == &target);
}

void tst_qobjectregistry::boundKey()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("BoundKeyRegistration.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    QObject *boundTarget = root->property("boundTarget").value<QObject *>();
    QVERIFY(boundTarget);

    QObjectRegistryRef ref(&e, "BoundKey");
    QVERIFY(ref.object() == boundTarget);
    QCOMPARE(ref.object()->property("testProp").toInt(), 33);

    QSignalSpy spyObjectChanged(&ref, &QObjectRegistryRef::objectChanged);
    root->setProperty("dynamicKey", "BoundKeyChanged");

    QVERIFY(!ref.object());
    QCOMPARE(spyObjectChanged.size(), 1);

    QObjectRegistryRef changedRef(&e, "BoundKeyChanged");
    QVERIFY(changedRef.object() == boundTarget);
}

void tst_qobjectregistry::cppRegistration()
{
    QQmlEngine e;

    QObjectRegistryRef ref(&e, "CppRegistration");
    QVERIFY(!ref.object());

    QObject *target = new QObject;

    QObjectRegistry registry;
    QQmlEngine::setContextForObject(&registry, e.rootContext());
    registry.setKey("CppRegistration");
    registry.setTarget(target);

    QVERIFY(ref.object() == target);
    QVERIFY(registry.target() == target);

    QSignalSpy spyObjectChanged(&ref, &QObjectRegistryRef::objectChanged);
    QSignalSpy spyTargetChanged(&registry, &QObjectRegistry::targetChanged);

    delete target;

    QVERIFY(!ref.object());
    QVERIFY(!registry.target());
    QCOMPARE(spyObjectChanged.size(), 1);
    QCOMPARE(spyTargetChanged.size(), 1);
}

void tst_qobjectregistry::dynamicObjectCreationAndDestruction()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("RepeaterRegistration.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    QMultiObjectRegistryRef ref(&e, "RepeatedRect");

    QSignalSpy spyObjectsChanged(&ref, &QMultiObjectRegistryRef::objectsChanged);
    QSignalSpy spyObjectAdded(&ref, &QMultiObjectRegistryRef::objectAdded);
    QSignalSpy spyObjectRemoved(&ref, &QMultiObjectRegistryRef::objectRemoved);

    QVERIFY(ref.objectsList().size() == 6);
    QVERIFY(spyObjectsChanged.size() == 0);
    QVERIFY(spyObjectAdded.size() == 0);
    QVERIFY(spyObjectRemoved.size() == 0);

    QList<QSignalSpy *> spyList;
    QList<QObject *> objects = ref.objectsList();

    for (QObject *obj : std::as_const(objects)) {
        QVERIFY(obj);
        QVERIFY(obj->property("index").toInt() >= 0);
        spyList.append(new QSignalSpy(obj, &QObject::destroyed));
    }

    spyObjectsChanged.clear();
    spyObjectAdded.clear();
    spyObjectRemoved.clear();

    QObject::connect(&ref, &QMultiObjectRegistryRef::objectRemoved, this, [&](QObject *obj) {
        objects.removeOne(obj);
    });

    QList<QObject *> newObjects;
    QObject::connect(&ref, &QMultiObjectRegistryRef::objectAdded, this, [&](QObject *obj) {
        newObjects.append(obj);
        QVERIFY(obj);

        // Verify that object properties have been intialized at this point
        int index = obj->property("index").toInt();
        QVERIFY(index >= 0);
        QVERIFY(obj->property("testValue").toInt() == 9);
        QVERIFY(obj->property("x").toInt() == 10 * index);
        QVERIFY(obj->property("y").toInt() == 20);

        QQmlProperty borderWidth(obj, "border.width", &e);
        QVERIFY(borderWidth.read().toInt() == index);

        QQmlProperty borderColor(obj, "border.color", &e);
        QVERIFY(borderColor.read().value<QColor>() == QColor("#123456"));
    });

    QMetaObject::invokeMethod(root.get(), "resetToFour");

    // Repeater deletes old objects asynchronously, so wait for destructions
    for (auto spy : spyList) {
        if (spy->size() == 1)
            continue;
        spy->wait();
    }

    QVERIFY(objects.size() == 0);
    QVERIFY(spyObjectsChanged.size() == 10); // removes + adds, each triggers objectsChanged
    QVERIFY(spyObjectAdded.size() == 4);
    QVERIFY(spyObjectRemoved.size() == 6);

    QVERIFY(newObjects.size() == ref.objectsList().size());
    for (auto obj : std::as_const(newObjects))
        QVERIFY(ref.objectsList().contains(obj));

    qDeleteAll(spyList);
}

void tst_qobjectregistry::imperativeTargetAssignment()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("ImperativeRegistration.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    QObjectRegistryRef singleRef(&e, "ImperativeSingle");
    QMultiObjectRegistryRef multiRef(&e, "ImperativeMulti");
    QVERIFY(!singleRef.object());
    QVERIFY(multiRef.objectsList().isEmpty());

    QMetaObject::invokeMethod(root.get(), "registerObjects");

    QObject *obj1 = root->property("obj1").value<QObject *>();
    QObject *obj2 = root->property("obj2").value<QObject *>();
    QVERIFY(obj1);
    QVERIFY(obj2);
    QVERIFY(singleRef.object() == obj1);
    QCOMPARE(multiRef.objectsList().size(), 2);
    QVERIFY(multiRef.objectsList().contains(obj1));
    QVERIFY(multiRef.objectsList().contains(obj2));

    QObject *singleReg = root->property("singleReg").value<QObject *>();
    QVERIFY(singleReg);
    QVERIFY(singleReg->property("target").value<QObject *>() == obj1);

    QSignalSpy spyObjectChanged(&singleRef, &QObjectRegistryRef::objectChanged);
    QSignalSpy spyObjectsChanged(&multiRef, &QMultiObjectRegistryRef::objectsChanged);
    QSignalSpy spyObjectRemoved(&multiRef, &QMultiObjectRegistryRef::objectRemoved);
    QSignalSpy spyTargetChanged(singleReg, SIGNAL(targetChanged()));
    QSignalSpy spyDestroyed1(obj1, &QObject::destroyed);
    QSignalSpy spyDestroyed2(obj2, &QObject::destroyed);

    // QML destroy() is deferred, so wait for the destructions
    QMetaObject::invokeMethod(root.get(), "destroyObjects");
    QVERIFY(spyDestroyed1.size() == 1 || spyDestroyed1.wait());
    QVERIFY(spyDestroyed2.size() == 1 || spyDestroyed2.wait());

    QVERIFY(!singleRef.object());
    QCOMPARE(spyObjectChanged.size(), 1);

    QVERIFY(multiRef.objectsList().isEmpty());
    QCOMPARE(spyObjectRemoved.size(), 2);
    QCOMPARE(spyObjectsChanged.size(), 2);

    QVERIFY(!singleReg->property("target").value<QObject *>());
    QCOMPARE(spyTargetChanged.size(), 1);

    QObjectRegistryRef newSingleRef(&e, "ImperativeSingle");
    QMultiObjectRegistryRef newMultiRef(&e, "ImperativeMulti");
    QVERIFY(!newSingleRef.object());
    QVERIFY(newMultiRef.objectsList().isEmpty());
}

void tst_qobjectregistry::keyChange()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("KeyChange.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    QObjectRegistryRef ref1(&e, "SingleBefore");
    QVERIFY(ref1.object());
    QObject *singleObjBefore = ref1.object();

    QObjectRegistryRef ref2(&e, "SingleAfter");
    QVERIFY(!ref2.object());

    QMultiObjectRegistryRef ref3(&e, "MultiBefore");
    QVERIFY(ref3.objectsList().size() == 2);

    // Use sets to compare as order is not deterministic
    QSet<QObject *> objectsBefore;
    for (auto obj : ref3.objectsList())
        objectsBefore.insert(obj);

    QMultiObjectRegistryRef ref4(&e, "MultiAfter");
    QVERIFY(ref4.objectsList().size() == 0);

    QSignalSpy spyObjectChanged1(&ref1, &QObjectRegistryRef::objectChanged);
    QSignalSpy spyObjectChanged2(&ref2, &QObjectRegistryRef::objectChanged);

    QSignalSpy spyObjectsChanged3(&ref3, &QMultiObjectRegistryRef::objectsChanged);
    QSignalSpy spyObjectAdded3(&ref3, &QMultiObjectRegistryRef::objectAdded);
    QSignalSpy spyObjectRemoved3(&ref3, &QMultiObjectRegistryRef::objectRemoved);

    QSignalSpy spyObjectsChanged4(&ref4, &QMultiObjectRegistryRef::objectsChanged);
    QSignalSpy spyObjectAdded4(&ref4, &QMultiObjectRegistryRef::objectAdded);
    QSignalSpy spyObjectRemoved4(&ref4, &QMultiObjectRegistryRef::objectRemoved);

    // Change keys on registration side
    QMetaObject::invokeMethod(root.get(), "triggerKeyChange");

    QVERIFY(!ref1.object());
    QVERIFY(ref2.object() == singleObjBefore);

    QSet<QObject *> objectsAfter4;
    for (auto obj : ref4.objectsList())
        objectsAfter4.insert(obj);

    QVERIFY(ref3.objectsList().size() == 0);
    QVERIFY(objectsBefore == objectsAfter4);

    QVERIFY(spyObjectChanged1.size() == 1);
    QVERIFY(spyObjectChanged2.size() == 1);

    QVERIFY(spyObjectsChanged3.size() == 2);
    QVERIFY(spyObjectAdded3.size() == 0);
    QVERIFY(spyObjectRemoved3.size() == 2);

    QVERIFY(spyObjectsChanged4.size() == 2);
    QVERIFY(spyObjectAdded4.size() == 2);
    QVERIFY(spyObjectRemoved4.size() == 0);

    spyObjectChanged1.clear();

    // Change keys on reference side
    ref1.setKey("SingleAfter");

    QVERIFY(spyObjectChanged1.size() == 1);
    QVERIFY(ref1.object() == ref2.object());

    spyObjectsChanged3.clear();
    spyObjectAdded3.clear();
    spyObjectRemoved3.clear();

    ref3.setKey("MultiAfter");

    QVERIFY(spyObjectsChanged3.size() == 1);
    QVERIFY(spyObjectAdded3.size() == 2);
    QVERIFY(spyObjectRemoved3.size() == 0);

    QSet<QObject *> objectsAfter3;
    for (auto obj : ref3.objectsList())
        objectsAfter3.insert(obj);
    QVERIFY(objectsAfter3 == objectsAfter4);
}

void tst_qobjectregistry::multiRegistration()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("MultiRegistration.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    {
        QQmlTestMessageHandler messageHandler;
        QObjectRegistryRef ref(&e, "MultiRegistration");
        QCOMPARE(messageHandler.messages().size(), 1);
        QVERIFY(messageHandler.messages()[0].contains("ObjectRegistryRef found multiple"));
        QVERIFY(ref.object());
    }

    {
        QObject dummyParent;
        QMultiObjectRegistryRef ref(&e, "MultiRegistration", &dummyParent);
        QVERIFY(ref.parent() == &dummyParent);
        QVERIFY(ref.objectsList().size() == 2);
        QVERIFY(ref.objectsList() == ref.objects().toList<QList<QObject *>>());

        QSignalSpy spyObjectsChanged(&ref, &QMultiObjectRegistryRef::objectsChanged);
        QSignalSpy spyObjectAdded(&ref, &QMultiObjectRegistryRef::objectAdded);
        QSignalSpy spyObjectRemoved(&ref, &QMultiObjectRegistryRef::objectRemoved);

        QObject *o1 = ref.objectsList()[0];
        QObject *o2 = ref.objectsList()[1];
        QVERIFY(o1 != o2);

        // Order of objects is not deterministic
        if (!o1->property("testProp1").isValid())
            qSwap(o1, o2);

        QVERIFY(o1->property("testProp1").toInt() == 1);
        QVERIFY(o2->property("testProp2").toInt() == 2);

        QSignalSpy spyTest1(o1, SIGNAL(testSignal1()));
        QSignalSpy spyTest2(o2, SIGNAL(testSignal2()));
        QMetaObject::invokeMethod(root.get(), "triggerSignal1");
        QVERIFY(spyTest1.size() == 1);
        QMetaObject::invokeMethod(root.get(), "triggerSignal2");
        QVERIFY(spyTest2.size() == 1);

        delete o1;

        QVERIFY(spyObjectsChanged.size() == 1);
        QVERIFY(spyObjectAdded.size() == 0);
        QVERIFY(spyObjectRemoved.size() == 1);
        QVERIFY(ref.objectsList().size() == 1);
        QVERIFY(ref.objectsList()[0] == o2);

        delete o2;

        QVERIFY(spyObjectsChanged.size() == 2);
        QVERIFY(spyObjectAdded.size() == 0);
        QVERIFY(spyObjectRemoved.size() == 2);
        QVERIFY(ref.objectsList().size() == 0);
    }
    {
        // A new ref will not find deleted objects either
        QMultiObjectRegistryRef ref(&e, "MultiRegistration");
        QVERIFY(ref.objectsList().size() == 0);
    }
}

void tst_qobjectregistry::noEngine()
{
    QString errorStr = "Object registry could not be resolved";
    QQmlTestMessageHandler messageHandler;

    QObjectRegistryRef ref;
    ref.setKey("NoEngine");
    QCOMPARE(messageHandler.messages().size(), 1);
    QVERIFY(messageHandler.messages()[0].contains(errorStr));
    QVERIFY(!ref.object());

    QMultiObjectRegistryRef ref2;
    ref2.setKey("NoEngine");
    QCOMPARE(messageHandler.messages().size(), 2);
    QVERIFY(messageHandler.messages()[1].contains(errorStr));
    QVERIFY(ref2.objectsList().isEmpty());
}

void tst_qobjectregistry::qmlReferences()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("QmlReferences.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    // Verify QML side references resolve same objects as C++ references
    // This is especially meaningful for multiref, where qml side objects list is QQmlListProperty
    QObject *singleObj = root->property("singleObj").value<QObject *>();
    QObject *multiObj0 = root->property("multiObj0").value<QObject *>();
    QObject *multiObj1 = root->property("multiObj1").value<QObject *>();

    QVERIFY(singleObj);
    QVERIFY(multiObj0);
    QVERIFY(multiObj1);
    QVERIFY(multiObj0 != multiObj1);

    QObjectRegistryRef ref(&e, "SingleRegistration");
    QVERIFY(ref.object());
    QVERIFY(ref.object() == singleObj);

    QMultiObjectRegistryRef ref2(&e, "MultiRegistration");
    QVERIFY(ref2.objectsList().size() == 2);
    // Order is not deterministic
    QVERIFY(ref2.objectsList()[0] == multiObj0 || ref2.objectsList()[0] == multiObj1);
    QVERIFY(ref2.objectsList()[1] == multiObj0 || ref2.objectsList()[1] == multiObj1);
}

void tst_qobjectregistry::refChangedDuringNotification()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("SingleRegistration.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    QMultiObjectRegistryRef ref1(&e, "SingleRegistration");
    QMultiObjectRegistryRef ref2(&e, "SingleRegistration");
    QCOMPARE(ref1.objectsList().size(), 1);
    QCOMPARE(ref2.objectsList().size(), 1);

    QObject *target = ref1.objectsList()[0];

    QObject::connect(&ref1, &QMultiObjectRegistryRef::objectRemoved, this, [&] {
        ref2.setKey("UnusedKey");
    });
    QObject::connect(&ref2, &QMultiObjectRegistryRef::objectRemoved, this, [&] {
        ref1.setKey("UnusedKey");
    });

    QSignalSpy spyObjectRemoved1(&ref1, &QMultiObjectRegistryRef::objectRemoved);
    QSignalSpy spyObjectRemoved2(&ref2, &QMultiObjectRegistryRef::objectRemoved);

    delete target;

    // Both references lose the object only once:
    // First via the removal notification
    // Second via the key change that the notification triggered
    QCOMPARE(spyObjectRemoved1.size(), 1);
    QCOMPARE(spyObjectRemoved2.size(), 1);
    QVERIFY(ref1.objectsList().isEmpty());
    QVERIFY(ref2.objectsList().isEmpty());
}

void tst_qobjectregistry::refDelete()
{
    // Another ref with same name after old ref was deleted finds the same object(s)
    {
        QQmlEngine e;
        QQmlComponent component(&e, testFileUrl("SingleRegistration.qml"));
        QVERIFY(!component.isError());
        QScopedPointer<QObject> root(component.create());
        QVERIFY(!root.isNull());

        QObject *target = nullptr;
        {
            QObjectRegistryRef ref(&e, "SingleRegistration");
            QVERIFY(ref.object());
            target = ref.object();
        }
        {
            QObjectRegistryRef ref(&e, "SingleRegistration");
            QVERIFY(ref.object());
            QVERIFY(target == ref.object());
        }
    }
    {
        QQmlEngine e;
        QQmlComponent component(&e, testFileUrl("MultiRegistration.qml"));
        QVERIFY(!component.isError());
        QScopedPointer<QObject> root(component.create());
        QVERIFY(!root.isNull());

        // Use sets to compare as order is not deterministic
        QSet<QObject *> objects;
        QSet<QObject *> objects2;

        {
            QMultiObjectRegistryRef ref(&e, "MultiRegistration");
            QVERIFY(ref.objectsList().size() == 2);
            for (auto obj : ref.objectsList())
                objects.insert(obj);
        }
        {
            QMultiObjectRegistryRef ref(&e, "MultiRegistration");
            QVERIFY(ref.objectsList().size() == 2);
            for (auto obj : ref.objectsList())
                objects2.insert(obj);
        }

        QVERIFY(objects == objects2);
    }
}

void tst_qobjectregistry::refInitialKeySet_data()
{
    QTest::addColumn<bool>("setKeyAtConstruct");
    QTest::addColumn<int>("initialMultiCount");

    // Setting key at constructor: Repeated objects will be immediately available and no added
    // signals are emitted
    QTest::newRow("Set key at construct") << true << 2;

    // Setting key after constructor: Added signals are emitted for each added object after key set
    QTest::newRow("Set key after construct") << false << 0;
}

void tst_qobjectregistry::refInitialKeySet()
{
    QFETCH(bool, setKeyAtConstruct);
    QFETCH(int, initialMultiCount);

    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("SingleAndMultiRegistration.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    {
        QString key = "SingleRegistration";
        QString constructKey;
        if (setKeyAtConstruct)
            constructKey = key;

        QObjectRegistryRef ref(&e, constructKey);

        QSignalSpy spyObjectChanged(&ref, &QObjectRegistryRef::objectChanged);

        QVERIFY(bool(ref.object()) == setKeyAtConstruct);
        QVERIFY(spyObjectChanged.size() == 0);

        if (!setKeyAtConstruct) {
            ref.setKey(key);

            QVERIFY(ref.object());
            QVERIFY(spyObjectChanged.size() == 1);
        }

        QVERIFY(ref.object()->property("testProp1").toInt() == 11);
    }

    {
        QString key = "MultiRegistration";
        QString constructKey;
        if (setKeyAtConstruct)
            constructKey = key;

        QMultiObjectRegistryRef ref(&e, constructKey);

        QSignalSpy spyObjectsChanged(&ref, &QMultiObjectRegistryRef::objectsChanged);
        QSignalSpy spyObjectAdded(&ref, &QMultiObjectRegistryRef::objectAdded);
        QSignalSpy spyObjectRemoved(&ref, &QMultiObjectRegistryRef::objectRemoved);

        QVERIFY(ref.objectsList().size() == initialMultiCount);
        QVERIFY(spyObjectsChanged.size() == 0);
        QVERIFY(spyObjectAdded.size() == 0);
        QVERIFY(spyObjectRemoved.size() == 0);

        if (!setKeyAtConstruct) {
            ref.setKey(key);

            QVERIFY(ref.objectsList().size() == 2);
            QVERIFY(spyObjectsChanged.size() == 1);
            QVERIFY(spyObjectAdded.size() == 2);
            QVERIFY(spyObjectRemoved.size() == 0);
        }

        QObject *o1 = ref.objectsList()[0];
        QObject *o2 = ref.objectsList()[1];
        QVERIFY(o1 != o2);

        // Order of objects is not deterministic
        if (!o1->property("testProp1").isValid())
            qSwap(o1, o2);

        QVERIFY(o1->property("testProp1").toInt() == 1);
        QVERIFY(o2->property("testProp2").toInt() == 2);
    }
}

void tst_qobjectregistry::registryOutlivesEngine()
{
    QObject target;
    QObjectRegistry registry;
    QScopedPointer<QObjectRegistryRef> ref;

    {
        QQmlEngine e;
        QQmlEngine::setContextForObject(&registry, e.rootContext());
        registry.setKey("OutlivingRegistration");
        registry.setTarget(&target);

        ref.reset(new QObjectRegistryRef(&e, "OutlivingRegistration"));
        QVERIFY(ref->object() == &target);
    }

    // The underlying singleton registry was destroyed along with the engine,
    // so ref can no longer be resolved
    {
        QQmlTestMessageHandler messageHandler;
        ref->setKey("OutlivingRegistrationChanged");
        QCOMPARE(messageHandler.messages().size(), 1);
        QVERIFY(messageHandler.messages()[0].contains("Object registry could not be resolved"));
    }

    // The registry object and its target are unaffected by the underlying registry going away
    QVERIFY(registry.target() == &target);
}

void tst_qobjectregistry::sameKeyMultipleEngines()
{
    {
        QQmlEngine e1, e2;
        QQmlComponent component1(&e1, testFileUrl("SingleRegistration.qml"));
        QQmlComponent component2(&e2, testFileUrl("SingleRegistration.qml"));
        QVERIFY(!component1.isError());
        QVERIFY(!component2.isError());
        QScopedPointer<QObject> root1(component1.create());
        QScopedPointer<QObject> root2(component2.create());
        QVERIFY(!root1.isNull());
        QVERIFY(!root2.isNull());

        QObjectRegistryRef ref1(&e1, "SingleRegistration");
        QObjectRegistryRef ref2(&e2, "SingleRegistration");
        QVERIFY(ref1.object());
        QVERIFY(ref2.object());
        QVERIFY(ref1.object() != ref2.object());
        QVERIFY(qmlEngine(ref1.object()) == &e1);
        QVERIFY(qmlEngine(ref2.object()) == &e2);
    }
    {
        QQmlEngine e1, e2;
        QQmlComponent component1(&e1, testFileUrl("MultiRegistration.qml"));
        QQmlComponent component2(&e2, testFileUrl("MultiRegistration.qml"));
        QVERIFY(!component1.isError());
        QVERIFY(!component2.isError());
        QScopedPointer<QObject> root1(component1.create());
        QScopedPointer<QObject> root2(component2.create());
        QVERIFY(!root1.isNull());
        QVERIFY(!root2.isNull());

        QSet<QObject *> objects1;
        QSet<QObject *> objects2;

        QMultiObjectRegistryRef ref1(&e1, "MultiRegistration");
        QMultiObjectRegistryRef ref2(&e2, "MultiRegistration");
        QVERIFY(ref1.objectsList().size() == 2);
        QVERIFY(ref2.objectsList().size() == 2);

        for (auto obj : ref1.objectsList()) {
            objects1.insert(obj);
            QVERIFY(qmlEngine(obj) == &e1);
        }
        for (auto obj : ref2.objectsList()) {
            objects2.insert(obj);
            QVERIFY(qmlEngine(obj) == &e2);
        }

        QVERIFY(!objects1.intersects(objects2));
    }
}

void tst_qobjectregistry::sameTargetRegisteredMultipleTimes()
{
    // Registering same target multiple times with same key is equivalent to registering it once
    // and shouldn't crash if target is deleted even if some of the ObjectRegistry objects remain
    {
        QQmlEngine e;
        QQmlComponent component(&e, testFileUrl("SameTargetRegistration.qml"));
        QVERIFY(!component.isError());
        QScopedPointer<QObject> root(component.create());
        QVERIFY(!root.isNull());
        QVERIFY(!root->property("target1").isNull());
        QVERIFY(!root->property("target2").isNull());

        QMultiObjectRegistryRef ref(&e, "SameTargetMulti");
        QVERIFY(ref.objectsList().size() == 2);

        QObject *o1 = ref.objectsList()[0];
        QObject *o2 = ref.objectsList()[1];
        QVERIFY(o1 != o2);

        // Order of objects is not deterministic
        if (!o1->property("testProp1").isValid())
            qSwap(o1, o2);

        QVERIFY(o1->property("testProp1").toInt() == 1);
        QVERIFY(o2->property("testProp2").toInt() == 2);

        qDeleteAll(ref.objectsList());

        QVERIFY(ref.objectsList().size() == 0);
        QVERIFY(root->property("target1").isNull());
        QVERIFY(root->property("target2").isNull());
    }

    {
        QQmlEngine e;
        QQmlComponent component(&e, testFileUrl("SameTargetRegistration.qml"));
        QVERIFY(!component.isError());
        QScopedPointer<QObject> root(component.create());
        QVERIFY(!root.isNull());
        QVERIFY(!root->property("target3").isNull());

        QObjectRegistryRef ref(&e, "SameTargetSingle");
        QVERIFY(ref.object());
        QVERIFY(ref.object()->property("testProp3").toInt() == 3);

        delete ref.object();

        QVERIFY(!ref.object());
        QVERIFY(root->property("target3").isNull());
    }

    // Registering same target multiple times with different key is comparable to registering two
    // different targets. If target is deleted, remaining registrations properly lose their objects.
    {
        QQmlEngine e;
        QQmlComponent component(&e, testFileUrl("SameTargetRegistration.qml"));
        QVERIFY(!component.isError());
        QScopedPointer<QObject> root(component.create());
        QVERIFY(!root.isNull());
        QVERIFY(!root->property("target4").isNull());
        QVERIFY(!root->property("target5").isNull());

        QMultiObjectRegistryRef ref1(&e, "SameTargetMultiDifferentkey1");
        QMultiObjectRegistryRef ref2(&e, "SameTargetMultiDifferentkey2");
        QVERIFY(ref1.objectsList().size() == 2);
        QVERIFY(ref2.objectsList().size() == 2);

        QObject *o11 = ref1.objectsList()[0];
        QObject *o12 = ref1.objectsList()[1];
        QObject *o21 = ref2.objectsList()[0];
        QObject *o22 = ref2.objectsList()[1];

        // Order of objects is not deterministic
        QVERIFY(o11 == o21 || o11 == o22);
        QVERIFY(o12 == o21 || o12 == o22);
        QVERIFY(o11 != o12 && o21 != o22);
        if (!o11->property("testProp4").isValid())
            qSwap(o11, o12);

        QVERIFY(o11->property("testProp4").toInt() == 4);
        QVERIFY(o12->property("testProp5").toInt() == 5);

        qDeleteAll(ref1.objectsList());

        QVERIFY(ref1.objectsList().size() == 0);
        QVERIFY(ref2.objectsList().size() == 0);

        QVERIFY(root->property("target4").isNull());
        QVERIFY(root->property("target5").isNull());
    }

    {
        QQmlEngine e;
        QQmlComponent component(&e, testFileUrl("SameTargetRegistration.qml"));
        QVERIFY(!component.isError());
        QScopedPointer<QObject> root(component.create());
        QVERIFY(!root.isNull());
        QVERIFY(!root->property("target6").isNull());

        QObjectRegistryRef ref1(&e, "SameTargetDifferentKey1");
        QObjectRegistryRef ref2(&e, "SameTargetDifferentKey2");
        QVERIFY(ref1.object());
        QVERIFY(ref2.object());
        QVERIFY(ref1.object() == ref2.object());
        QVERIFY(ref1.object()->property("testProp6").toInt() == 6);

        delete ref1.object();

        QVERIFY(!ref1.object());
        QVERIFY(!ref2.object());
        QVERIFY(root->property("target6").isNull());
    }
}

void tst_qobjectregistry::singleRegistration_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<QString>("propName");
    QTest::addColumn<QString>("signalTrigger");
    QTest::addColumn<int>("propValue");

    QTest::newRow("Detached registration")
        << "SingleRegistration" << "testProp1" << "triggerSignal1" <<  11;
    QTest::newRow("Attached Item registration")
        << "ItemAttachReg" << "testProp2" << "triggerSignal2" << 22;
    QTest::newRow("Attached QtObject registration")
        << "ObjectAttachReg" << "testProp3" << "triggerSignal3" << 33;
    QTest::newRow("Registration outside target")
        << "RegNotInsideObject" << "testProp4" << "triggerSignal4" << 44;
}

void tst_qobjectregistry::singleRegistration()
{
    QFETCH(QString, key);
    QFETCH(QString, propName);
    QFETCH(QString, signalTrigger);
    QFETCH(int, propValue);

    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("SingleRegistration.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    {
        QObject dummyParent;
        QObjectRegistryRef ref(&e, key, &dummyParent);
        QVERIFY(ref.parent() == &dummyParent);
        QVERIFY(ref.object());
        QVERIFY(ref.object()->property(propName.toUtf8()).toInt() == propValue);

        QSignalSpy spyObjectChanged(&ref, &QObjectRegistryRef::objectChanged);
        QSignalSpy spyTest(ref.object(), SIGNAL(testSignal()));

        QMetaObject::invokeMethod(root.get(), signalTrigger.toUtf8());
        QVERIFY(spyTest.size() == 1);
        QVERIFY(spyObjectChanged.size() == 0);

        delete ref.object();

        QVERIFY(!ref.object());
        QVERIFY(spyObjectChanged.size() == 1);
    }

    {
        // A new ref will not find deleted object either
        QObjectRegistryRef ref(&e, key);
        QVERIFY(!ref.object());
    }
}

void tst_qobjectregistry::staleTargetRemoval()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("StaleTargetRegistration.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    QMultiObjectRegistryRef ref(&e, "StaleTarget");
    QVERIFY(ref.objectsList().isEmpty());

    QMetaObject::invokeMethod(root.get(), "registerFirst");

    QObject *firstObject = root->property("firstObject").value<QObject *>();
    QObject *firstRegistry = root->property("firstRegistry").value<QObject *>();
    QVERIFY(firstObject);
    QVERIFY(firstRegistry);
    QCOMPARE(ref.objectsList().size(), 1);
    QVERIFY(ref.objectsList()[0] == firstObject);

    // QML destroy() is deferred, so wait for the destructions
    QSignalSpy spyObjectDestroyed(firstObject, &QObject::destroyed);
    QMetaObject::invokeMethod(root.get(), "destroyFirstObject");
    QVERIFY(spyObjectDestroyed.size() == 1 || spyObjectDestroyed.wait());

    QVERIFY(ref.objectsList().isEmpty());
    QVERIFY(!firstRegistry->property("target").value<QObject *>());

    QMetaObject::invokeMethod(root.get(), "registerSecond");

    QObject *secondObject = root->property("secondObject").value<QObject *>();
    QVERIFY(secondObject);
    QCOMPARE(ref.objectsList().size(), 1);
    QVERIFY(ref.objectsList()[0] == secondObject);

    QSignalSpy spyRegistryDestroyed(firstRegistry, &QObject::destroyed);
    QMetaObject::invokeMethod(root.get(), "destroyFirstRegistry");
    QVERIFY(spyRegistryDestroyed.size() == 1 || spyRegistryDestroyed.wait());

    QCOMPARE(ref.objectsList().size(), 1);
    QVERIFY(ref.objectsList()[0] == secondObject);
}

void tst_qobjectregistry::targetChange()
{
    {
        QQmlEngine e;
        QQmlComponent component(&e, testFileUrl("TargetChange.qml"));
        QVERIFY(!component.isError());
        QScopedPointer<QObject> root(component.create());
        QVERIFY(!root.isNull());

        QObjectRegistryRef ref(&e, "ChangingTarget");
        QVERIFY(ref.object());
        QVERIFY(ref.object()->property("testProp").toInt() == 11);
        auto oldObject = ref.object();

        QSignalSpy spyObjectChanged(&ref, &QObjectRegistryRef::objectChanged);
        QMetaObject::invokeMethod(root.get(), "changeTarget");
        // Spy count 2 because remove old reg and add new reg both trigger objectChanged for refs
        QVERIFY(spyObjectChanged.size() == 2);

        QVERIFY(ref.object());
        QVERIFY(ref.object() != oldObject);
        QVERIFY(ref.object()->property("testProp").toInt() == 22);

        QMetaObject::invokeMethod(root.get(), "changeTargetBack");
        QVERIFY(spyObjectChanged.size() == 4);

        QVERIFY(ref.object());
        QVERIFY(ref.object() == oldObject);
        QVERIFY(ref.object()->property("testProp").toInt() == 11);
    }

    {
        QQmlEngine e;
        QQmlComponent component(&e, testFileUrl("TargetChange.qml"));
        QVERIFY(!component.isError());
        QScopedPointer<QObject> root(component.create());
        QVERIFY(!root.isNull());

        QMultiObjectRegistryRef ref(&e, "ChangingTarget");
        QVERIFY(ref.objectsList().size() == 1);
        QVERIFY(ref.objectsList()[0]);
        QVERIFY(ref.objectsList()[0]->property("testProp").toInt() == 11);
        auto oldObject = ref.objectsList()[0];

        QSignalSpy spyObjectsChanged(&ref, &QMultiObjectRegistryRef::objectsChanged);
        QSignalSpy spyObjectAdded(&ref, &QMultiObjectRegistryRef::objectAdded);
        QSignalSpy spyObjectRemoved(&ref, &QMultiObjectRegistryRef::objectRemoved);
        QMetaObject::invokeMethod(root.get(), "changeTarget");
        // Spy count 2 because remove old reg and add new reg both trigger objectsChanged for multirefs
        QVERIFY(spyObjectsChanged.size() == 2);
        QVERIFY(spyObjectAdded.size() == 1);
        QVERIFY(spyObjectRemoved.size() == 1);

        QVERIFY(ref.objectsList().size() == 1);
        QVERIFY(ref.objectsList()[0]);
        QVERIFY(ref.objectsList()[0] != oldObject);
        QVERIFY(ref.objectsList()[0]->property("testProp").toInt() == 22);

        QMetaObject::invokeMethod(root.get(), "changeTargetBack");
        QVERIFY(spyObjectsChanged.size() == 4);
        QVERIFY(spyObjectAdded.size() == 2);
        QVERIFY(spyObjectRemoved.size() == 2);

        QVERIFY(ref.objectsList().size() == 1);
        QVERIFY(ref.objectsList()[0]);
        QVERIFY(ref.objectsList()[0] == oldObject);
        QVERIFY(ref.objectsList()[0]->property("testProp").toInt() == 11);
    }
}

QTEST_MAIN(tst_qobjectregistry)

#include "tst_qobjectregistry.moc"
