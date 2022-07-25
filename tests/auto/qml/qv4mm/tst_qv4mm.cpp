/****************************************************************************
**
** Copyright (C) 2016 basysKom GmbH.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QQmlEngine>
#include <QLoggingCategory>
#include <QQmlComponent>

#include <private/qv4mm_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qjsvalue_p.h>

#include "../../shared/util.h"

#include <memory>

class tst_qv4mm : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void gcStats();
    void multiWrappedQObjects();
    void accessParentOnDestruction();
    void cleanInternalClasses();
};

void tst_qv4mm::gcStats()
{
    QLoggingCategory::setFilterRules("qt.qml.gc.*=true");
    QQmlEngine engine;
    engine.collectGarbage();
}

void tst_qv4mm::multiWrappedQObjects()
{
    QV4::ExecutionEngine engine1;
    QV4::ExecutionEngine engine2;
    {
        QObject object;
        for (int i = 0; i < 10; ++i)
            QV4::QObjectWrapper::wrap(i % 2 ? &engine1 : &engine2, &object);

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
        engine1.memoryManager->runGC();
        QCOMPARE(engine1.memoryManager->m_pendingFreedObjectWrapperValue.size(), 1);

        // engine2 doesn't own the object as engine1 was the first to wrap it above.
        // Therefore, no effect here.
        engine2.memoryManager->runGC();
        QCOMPARE(engine2.memoryManager->m_pendingFreedObjectWrapperValue.size(), 0);
    }

    // Clears m_pendingFreedObjectWrapperValue. Now it's really dead.
    engine1.memoryManager->runGC();
    QCOMPARE(engine1.memoryManager->m_pendingFreedObjectWrapperValue.size(), 0);

    engine2.memoryManager->runGC();
    QCOMPARE(engine2.memoryManager->m_pendingFreedObjectWrapperValue.size(), 0);
}

void tst_qv4mm::accessParentOnDestruction()
{
    QLoggingCategory::setFilterRules("qt.qml.gc.*=false");
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("createdestroy.qml"));
    std::unique_ptr<QObject> obj(component.create());
    QVERIFY(obj);
    QPointer<QObject> timer = qvariant_cast<QObject *>(obj->property("timer"));
    QVERIFY(timer);
    QTRY_VERIFY(!timer->property("running").toBool());
    QCOMPARE(obj->property("iterations").toInt(), 100);
    QCOMPARE(obj->property("creations").toInt(), 100);
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
    scope.engine->memoryManager->runGC();

    // Now the GC has removed the ICs we originally added by adding properties.
    QVERIFY(prevIC->d()->transitions.empty() || prevIC->d()->transitions.front().lookup == nullptr);

    // Same thing with redundant prototypes
    for (uint i = 0; i < numTransitions; ++i) {
        QV4::ScopedObject prototype(scope, engine.newObject());
        object->setPrototypeOf(prototype); // Makes previous prototype redundant
    }

    checkICCHainLength();
}

QTEST_MAIN(tst_qv4mm)

#include "tst_qv4mm.moc"
