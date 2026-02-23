// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qqmlproperty.h>
#include <QtQml/qqmlcontext.h>
#include <QtCore/qdir.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qcoreevent.h>

#include <private/qqmlcomponent_p.h>
#include <private/qqmldata_p.h>
#include <private/qqmlboundsignal_p.h>
#include <private/qqmlpreviewdiff_p.h>
#include <private/qqmlpreviewobjectpatch_p.h>
#include <private/qv4executablecompilationunit_p.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <private/qquickitem_p.h>

#include <QtGui/qfont.h>
#include <QtGui/qcolor.h>

using namespace QV4::CompiledData;

static qsizetype countByType(const CompilationUnitDiff &diff, ChangeType type)
{
    return std::count_if(diff.changes.begin(), diff.changes.end(),
                         [type](const Change &c) { return c.type == type; });
}

class tst_QQmlPreviewObjectPatch : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQmlPreviewObjectPatch();

private slots:
    // Granular live-object update tests
    void granularConstantUpdate();
    void granularConstantUpdatePreservesUserOverride();
    void granularPropertyAdditionWithStash();
    void granularPropertyRemovalWithStash();
    void updateObjectsFunctionAdd();
    void updateObjectsGeneratorFunctionAdd();
    void updateObjectsFunctionChange();
    void updateObjectsFunctionRemove();

    void scriptBindingChangeDropsCppPropertyOverride();
    void reattachLosesListChildUserOverrides();
    void varPropertyStashTypeMismatch();
    void reattachPreservesIdBinding();
    void inPlaceOnChildNeedlesslyReattaches();
    void reattachPreservesExternalBinding();

    // Cross-compilation-unit lookup invalidation
    void enumChangeAcrossComponents();

    // findInnerObjectForChange edge-case tests
    void attachedPropertyValueChange();
    void attachedPropertyAdded();
    void attachedPropertyRemoved();
    void groupPropertyFontChange();
    void groupPropertyAnchorsChange();
    void groupPropertyAnchorsTargetChange();
    void groupPropIndexShift();
    void groupPropChildRemoved();
    void groupPropertyRemoved();
    void attachedPropIndexShift();
    void multiGroupChange();
    void groupAndAttachedChange();
    void inlineComponentInstanceChange();
    void implicitComponentContentChange();
    void explicitComponentContentChange();

    // Signal handler binding resolution during property addition
    void signalHandlerWithPropertyAdd();

    // Verifies that after an in-place constant change, no duplicate child
    // objects survive and all objects reference the new compilation unit.
    void inPlaceUpdateNoObjectDuplication();

    // QTBUG-145905: Changing an individual anchor target (e.g. anchors.top
    // from parent.top to parent.verticalCenter) should update via in-place
    // patching without crashing.
    void anchorsTopIndividualTargetChange();

    // Same as above but with a running Timer and event loop to verify
    // no alternation between old and new values.
    void anchorsTopTimerNoAlternation();

    // VME metaobject rebuild for composite base types: the instance doesn't
    // need its own VME but its composite base type does.
    void compositeVMERebuildFromPlain();
    void compositeVMERebuildICChange();
    void compositeVMERebuildExternal();

    // Composite base type binding evaluation: script bindings (dependent
    // on other properties) must also be installed as live bindings.
    void compositeBindingScriptFromPlain();
    void compositeBindingScriptICChange();
    void compositeBindingScriptExternal();

    // Recursive composite hierarchies: the base type itself inherits from
    // another composite. Both levels must contribute VMEs and have their
    // bindings evaluated.
    void compositeRecursiveConstant();
    void compositeRecursiveScript();
    void compositeRecursiveICExternal();

    // Context rebuild regression tests
    void compositeExternalToAlias();
    void compositeFromPlainToSmallerCU();
    void childConstantChangePreservesRootIds();

    // Alias resolution crash when rebuilding a composite type whose base has
    // aliases referencing internal IDs (reproduces coffee example crash).
    void compositeAliasRebuildCrash();

    // Stress tests: multiple instances, deep hierarchies, dynamic children,
    // signal handlers, Component children, instance-level alias bindings.
    void multipleCompositeInstances();
    void nestedCompositeAlias();
    void compositeSignalHandler();
    void compositeRepeater();
    void instanceBindingReadsAlias();
    void compositeComponent();
    void inlineChildReadsAlias();
    void compositeStates();

    // Context hierarchy corruption when rebuilding a composite type instance
    // from an outer CU. The derived type's state "when" binding references IDs
    // from the base form, which requires the full context chain to be intact.
    // Reproduces the coffee demo crash (Home.qml modifying "when" binding).
    void compositeContextHierarchyCrash();

    // Same bug but with states on the root object (not grouped property).
    // Reproduces the lookupIdObject crash specifically.
    void compositeContextIdLookupCrash();

    // SafeArea attached property assertion crash after rebuild.
    // Reproduces ASSERT "item == parent()" in qquicksafearea.cpp when
    // resize event fires after rebuild orphaned the attached SafeArea object.
    void safeAreaAttachedRebuildCrash();

private:
    QQmlEngine engine;
};

static std::vector<QObject *>
objectsForCompilationUnit(QQmlEngine *engine,
                          const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit)
{
    return engine->handle()->memoryManager->findObjectsForCompilationUnits(
            { unit->baseCompilationUnit() });
}

static bool updateObjects(std::vector<QObject *> &objects,
                          const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                          const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    const auto diff = diffCompilationUnits(oldUnit->baseCompilationUnit()->unitData(),
                                           newUnit->baseCompilationUnit()->unitData());

    // Diff is tested elsewhere.
    Q_ASSERT(diff.success);

    if (QQmlPreview::applyDiff(objects, diff, oldUnit, newUnit)) {
        QQmlMetaType::deepClearCompositeType(oldUnit->baseCompilationUnit());
        QQmlPreview::refreshBindings(oldUnit, newUnit);
        return true;
    }

    return false;
}

tst_QQmlPreviewObjectPatch::tst_QQmlPreviewObjectPatch() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

void tst_QQmlPreviewObjectPatch::granularConstantUpdate()
{
    QQmlComponent oldComponent(&engine, testFileUrl("GranularConstantOld.qml"));
    QVERIFY2(oldComponent.isReady(), qPrintable(oldComponent.errorString()));
    QScopedPointer<QObject> object(oldComponent.create());
    QVERIFY(object);

    QCOMPARE(object->property("count").toInt(), 10);
    QCOMPARE(object->property("label").toString(), QString("hello"));

    QQmlComponent newComponent(&engine, testFileUrl("GranularConstantNew.qml"));
    QVERIFY2(newComponent.isReady(), qPrintable(newComponent.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComponent)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComponent)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    // Verify the diff has the expected structure (test-data sanity check).
    const auto diff = diffCompilationUnits(oldExecUnit->baseCompilationUnit()->unitData(),
                                           newExecUnit->baseCompilationUnit()->unitData());
    QVERIFY(diff.success);
    // Only `count` changed; `label` is identical and must not show as BindingChanged.
    QCOMPARE(countByType(diff, ChangeType::BindingChanged), 1);

    // Apply the update to the live object — no new QObject created.
    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCOMPARE(object->property("count").toInt(), 20); // updated by diff
    QCOMPARE(object->property("label").toString(), QString("hello")); // not invalidated
}

void tst_QQmlPreviewObjectPatch::granularConstantUpdatePreservesUserOverride()
{
    QQmlComponent oldComponent(&engine, testFileUrl("GranularConstantOld.qml"));
    QVERIFY2(oldComponent.isReady(), qPrintable(oldComponent.errorString()));
    QScopedPointer<QObject> object(oldComponent.create());
    QVERIFY(object);
    QCOMPARE(object->property("count").toInt(), 10);

    // The user manually changed `count` after the component loaded — simulate live editing.
    object->setProperty("count", 99);
    QCOMPARE(object->property("count").toInt(), 99);

    QQmlComponent newComponent(&engine, testFileUrl("GranularConstantNew.qml"));
    QVERIFY2(newComponent.isReady(), qPrintable(newComponent.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComponent)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComponent)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    // The user-overridden value (99 ≠ old default 10) must not be touched.
    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));
    QEXPECT_FAIL("", "We do not preserve user overrides", Continue);
    QCOMPARE(object->property("count").toInt(), 99); // preserved
}

void tst_QQmlPreviewObjectPatch::granularPropertyAdditionWithStash()
{
    QQmlComponent oldComponent(&engine, testFileUrl("GranularPropertyAddOld.qml"));
    QVERIFY2(oldComponent.isReady(), qPrintable(oldComponent.errorString()));
    QScopedPointer<QObject> object(oldComponent.create());
    QVERIFY(object);

    QCOMPARE(object->property("count").toInt(), 10);
    QVERIFY(!object->property("newProp").isValid()); // not in old CU

    // Simulate a user-modified property value during a live-preview session.
    object->setProperty("count", 42);

    QQmlComponent newComponent(&engine, testFileUrl("GranularPropertyAddNew.qml"));
    QVERIFY2(newComponent.isReady(), qPrintable(newComponent.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComponent)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComponent)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    // Verify the diff has the expected structure (test-data sanity check).
    const auto diff = diffCompilationUnits(oldExecUnit->baseCompilationUnit()->unitData(),
                                           newExecUnit->baseCompilationUnit()->unitData());
    QVERIFY(diff.success);
    QCOMPARE(countByType(diff, ChangeType::PropertyAdded), 1);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QEXPECT_FAIL("", "We do not preserve user overrides", Continue);
    QCOMPARE(object->property("count").toInt(), 42);
    QCOMPARE(object->property("newProp").toString(), QString("added"));
}

void tst_QQmlPreviewObjectPatch::granularPropertyRemovalWithStash()
{
    QQmlComponent oldComponent(&engine, testFileUrl("GranularPropertyRemoveOld.qml"));
    QVERIFY2(oldComponent.isReady(), qPrintable(oldComponent.errorString()));
    QScopedPointer<QObject> object(oldComponent.create());
    QVERIFY(object);

    QCOMPARE(object->property("count").toInt(), 10);
    QCOMPARE(object->property("removedProp").toString(), QString("gone"));

    // Simulate a user-modified property value during a live-preview session.
    object->setProperty("count", 77);

    QQmlComponent newComponent(&engine, testFileUrl("GranularPropertyRemoveNew.qml"));
    QVERIFY2(newComponent.isReady(), qPrintable(newComponent.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComponent)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComponent)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    // Verify the diff has the expected structure (test-data sanity check).
    const auto diff = diffCompilationUnits(oldExecUnit->baseCompilationUnit()->unitData(),
                                           newExecUnit->baseCompilationUnit()->unitData());
    QVERIFY(diff.success);
    QCOMPARE(countByType(diff, ChangeType::PropertyRemoved), 1);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QEXPECT_FAIL("", "We do not preserve user overrides", Continue);
    QCOMPARE(object->property("count").toInt(), 77);
    QVERIFY(!object->property("removedProp").isValid());
}

void tst_QQmlPreviewObjectPatch::updateObjectsFunctionAdd()
{
    QQmlComponent oldComponent(&engine, testFileUrl("FunctionAddOld.qml"));
    QVERIFY2(oldComponent.isReady(), qPrintable(oldComponent.errorString()));
    QScopedPointer<QObject> object(oldComponent.create());
    QVERIFY(object);

    QVariant result;
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression("QMetaObject::invokeMethod: No such method "
                                            "FunctionAddOld_QMLTYPE_[0-9]+::compute\\(\\)"));
    QVERIFY(!QMetaObject::invokeMethod(object.data(), "compute", Q_RETURN_ARG(QVariant, result)));
    QVERIFY(!result.isValid());

    QQmlComponent newComponent(&engine, testFileUrl("FunctionAddNew.qml"));
    QVERIFY2(newComponent.isReady(), qPrintable(newComponent.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComponent)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComponent)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    const auto diff = diffCompilationUnits(oldExecUnit->baseCompilationUnit()->unitData(),
                                           newExecUnit->baseCompilationUnit()->unitData());
    QVERIFY(diff.success);
    // One FunctionAdded change at unit level (objectIndex == -1) is expected.
    // The per-object function association is detected via ObjectChanged (which
    // carries the updated functionOffsetTable), not as a separate FunctionAdded event.
    QCOMPARE(countByType(diff, ChangeType::FunctionAdded), 1);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QVERIFY(QMetaObject::invokeMethod(object.data(), "compute", Q_RETURN_ARG(QVariant, result)));
    QCOMPARE(result.metaType(), QMetaType::fromType<int>());
    QCOMPARE(result.toInt(), 20);
}

void tst_QQmlPreviewObjectPatch::updateObjectsGeneratorFunctionAdd()
{
    // Verifies that a generator function (function*) added via live patching is correctly
    // installed in the VME method slots using GeneratorFunction::create(), not
    // FunctionObject::createScriptFunction().
    QQmlComponent oldComponent(&engine, testFileUrl("GeneratorFunctionAddOld.qml"));
    QVERIFY2(oldComponent.isReady(), qPrintable(oldComponent.errorString()));
    QScopedPointer<QObject> object(oldComponent.create());
    QVERIFY(object);

    QTest::ignoreMessage(
            QtWarningMsg,
            QRegularExpression("QMetaObject::invokeMethod: No such method "
                               "GeneratorFunctionAddOld_QMLTYPE_[0-9]+::runGen\\(\\)"));
    QVariant result;
    QVERIFY(!QMetaObject::invokeMethod(object.data(), "runGen", Q_RETURN_ARG(QVariant, result)));

    QQmlComponent newComponent(&engine, testFileUrl("GeneratorFunctionAddNew.qml"));
    QVERIFY2(newComponent.isReady(), qPrintable(newComponent.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComponent)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComponent)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    const auto diff = diffCompilationUnits(oldExecUnit->baseCompilationUnit()->unitData(),
                                           newExecUnit->baseCompilationUnit()->unitData());
    QVERIFY(diff.success);
    // Two functions added (gen + runGen), both at unit level.
    QCOMPARE(countByType(diff, ChangeType::FunctionAdded), 2);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // runGen() drives the generator: yields value(3), value*2(6), value*3(9) → sum 18.
    QVERIFY(QMetaObject::invokeMethod(object.data(), "runGen", Q_RETURN_ARG(QVariant, result)));
    QCOMPARE(result.metaType(), QMetaType::fromType<int>());
    QCOMPARE(result.toInt(), 18);
}

void tst_QQmlPreviewObjectPatch::updateObjectsFunctionChange()
{
    // Verifies that changing a function body via live patching updates the VME method slot
    // so that subsequent calls use the new implementation. ObjectChanged (triggered by
    // objectContentEqual detecting the body change) is the carrier that drives the rebuild.

    QQmlComponent oldComponent(&engine, testFileUrl("FunctionChangeOld.qml"));
    QVERIFY2(oldComponent.isReady(), qPrintable(oldComponent.errorString()));
    QScopedPointer<QObject> object(oldComponent.create());
    QVERIFY(object);

    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(object.data(), "compute", Q_RETURN_ARG(QVariant, result)));
    QCOMPARE(result.toInt(), 20); // value(10) * 2

    QQmlComponent newComponent(&engine, testFileUrl("FunctionChangeNew.qml"));
    QVERIFY2(newComponent.isReady(), qPrintable(newComponent.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComponent)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComponent)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    const auto diff = diffCompilationUnits(oldExecUnit->baseCompilationUnit()->unitData(),
                                           newExecUnit->baseCompilationUnit()->unitData());
    QVERIFY(diff.success);

    // One FunctionChanged at unit level and one ObjectChanged for the object-level function table.
    QCOMPARE(countByType(diff, ChangeType::FunctionChanged), 1);
    QCOMPARE(countByType(diff, ChangeType::ObjectChanged), 1);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QVERIFY(QMetaObject::invokeMethod(object.data(), "compute", Q_RETURN_ARG(QVariant, result)));
    QCOMPARE(result.toInt(), 30); // value(10) * 3
}

void tst_QQmlPreviewObjectPatch::updateObjectsFunctionRemove()
{
    // Verifies that removing a function via live patching removes the method slot so that
    // subsequent invokeMethod calls for the removed function fail.

    QQmlComponent oldComponent(&engine, testFileUrl("FunctionRemoveOld.qml"));
    QVERIFY2(oldComponent.isReady(), qPrintable(oldComponent.errorString()));
    QScopedPointer<QObject> object(oldComponent.create());
    QVERIFY(object);

    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(object.data(), "compute", Q_RETURN_ARG(QVariant, result)));
    QCOMPARE(result.toInt(), 20); // value(10) * 2

    QQmlComponent newComponent(&engine, testFileUrl("FunctionRemoveNew.qml"));
    QVERIFY2(newComponent.isReady(), qPrintable(newComponent.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComponent)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComponent)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    const auto diff = diffCompilationUnits(oldExecUnit->baseCompilationUnit()->unitData(),
                                           newExecUnit->baseCompilationUnit()->unitData());
    QVERIFY(diff.success);
    QCOMPARE(countByType(diff, ChangeType::FunctionRemoved), 1);
    QCOMPARE(countByType(diff, ChangeType::ObjectChanged), 1);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression("QMetaObject::invokeMethod: No such method "
                                            "FunctionRemoveNew_QMLTYPE_[0-9]+::compute\\(\\)"));
    QVERIFY(!QMetaObject::invokeMethod(object.data(), "compute", Q_RETURN_ARG(QVariant, result)));
}

static bool compareQmlObjects(QObject *actual, QObject *expected, QSet<QObject *> &visited,
                              const QString &path, QString &errorMsg);

static bool compareProperty(const QVariant &actual, const QVariant &expected,
                            QSet<QObject *> &visited, const QString &path, QString &errorMsg)
{
    const QMetaType actualMetaType = actual.metaType();
    const QMetaType expectedMetaType = expected.metaType();

    if (actualMetaType.flags().testFlag(QMetaType::PointerToQObject)) {
        return compareQmlObjects(actual.value<QObject *>(), expected.value<QObject *>(), visited,
                                 path, errorMsg);
    }

    if (actualMetaType.flags().testFlag(QMetaType::IsQmlList)) {
        QQmlListReference actualList(actual);
        QQmlListReference expectedList(expected);
        if (!actualList.isValid() || !expectedList.isValid()
            || actualList.size() != expectedList.size()) {
            errorMsg = path + ": lists differ, (actual=" + QString::number(actualList.size())
                    + ", expected=" + QString::number(expectedList.size()) + ")";
            return false;
        }

        for (qsizetype i = 0, end = actualList.size(); i < end; ++i) {
            if (!compareQmlObjects(actualList.at(i), expectedList.at(i), visited, path, errorMsg))
                return false;
        }

        return true;
    }

    if (actualMetaType != expectedMetaType) {
        errorMsg = path + ": types differ, (actual=" + actualMetaType.name()
                + ", expected=" + expectedMetaType.name() + ")";
        return false;
    }

    if (const QMetaObject *metaObject = QQmlMetaType::metaObjectForValueType(actualMetaType)) {
        for (int i = 0, end = metaObject->propertyCount(); i < end; ++i) {
            const QMetaProperty metaProp = metaObject->property(i);
            if (!compareProperty(metaProp.readOnGadget(actual.constData()),
                                 metaProp.readOnGadget(expected.constData()), visited,
                                 path + '.' + metaProp.name(), errorMsg)) {
                return false;
            }
        }

        return true;
    }

    if (actual == expected)
        return true;

    errorMsg = path + ": values differ, (actual=" + actual.toString()
            + ", expected=" + expected.toString() + ")";
    return false;
}

// Recursively compare two QML object trees property-by-property.
// \a actual is the patched object; \a expected is the freshly-created reference.
// Only properties from \a expected's meta-object are checked (so extra C++ properties
// on \a actual are silently ignored).  Sub-objects stored in QObject* properties and
// QML-managed children are compared recursively.
// Returns true when all checked values match; otherwise sets \a errorMsg and returns false.
static bool compareQmlObjects(QObject *actual, QObject *expected, QSet<QObject *> &visited,
                              const QString &path, QString &errorMsg)
{
    if (!actual && !expected)
        return true;
    if (!actual || !expected) {
        errorMsg = path + ": one object is null (actual=" + (actual ? "non-null" : "null")
                + ", expected=" + (expected ? "non-null" : "null") + ')';
        return false;
    }
    if (visited.contains(actual))
        return true; // cycle guard
    visited.insert(actual);

    const QMetaObject *expMeta = expected->metaObject();
    const QMetaObject *actMeta = actual->metaObject();

    if (actMeta->propertyCount() != expMeta->propertyCount()) {
        errorMsg = path + ": objects have different number of properties (actual="
                + QString::number(actMeta->propertyCount())
                + ", expected=" + QString::number(expMeta->propertyCount()) + ")";
    }

    for (int i = 0, end = expMeta->propertyCount(); i < end; ++i) {
        const QMetaProperty prop = expMeta->property(i);
        if (!prop.isReadable())
            continue;

        const QByteArray name = prop.name();
        const QString propPath = path.isEmpty() ? QString::fromLatin1(name)
                                                : path + u'.' + QString::fromLatin1(name);

        const QVariant expVal = prop.read(expected);
        const QVariant actVal = actual->property(name.constData());

        if (!compareProperty(actVal, expVal, visited, propPath, errorMsg))
            return false;
    }

    return true;
}

// Regression test: Patching from a composite-with-script-binding to a type with an
// alias (CompositeBindingScriptExternalNew → AddAliasNew). This used to crash with
// Q_ASSERT(!m_idValues) because rebuildObject reused an already-initialized context.
// The fix creates a fresh context instead of reinitializing the old one.
void tst_QQmlPreviewObjectPatch::compositeExternalToAlias()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeBindingScriptExternalNew.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    // Verify old state: child is ExternalCompositeScript with value/squared.
    QObject *child = object->findChild<QObject *>("child");
    QVERIFY(child);
    QCOMPARE(child->property("value").toInt(), 10);
    QCOMPARE(child->property("squared").toInt(), 100);

    QQmlComponent newComp(&engine, testFileUrl("AddAliasNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    const auto origObjects = objects;
    const bool updated = updateObjects(objects, oldExecUnit, newExecUnit);
    QVERIFY(updated);

    auto root = std::find(origObjects.begin(), origObjects.end(), object.get());
    QObject *replaced = objects[root - origObjects.begin()];
    if (replaced != object.get()) {
        object.release();
        object.reset(replaced);
    }

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // Verify new state: has the alias property from AddAliasNew.
    QVERIFY(object->property("nestedAlias").isValid());
}

// Regression test: Patching from a composite-with-IC-children to a simpler type with
// fewer objects (CompositeBindingScriptFromPlainNew → ExternalCompositeScript). This
// used to crash with Q_ASSERT(m_qmlObjectId < objectCount()) because the VME CU
// remapping loop incorrectly set the VME's compilation unit to the new CU even when
// the object's index was out of range. The fix skips obsolete objects and nulls their
// VME compilation unit so findCompiledObject() safely returns nullptr.
void tst_QQmlPreviewObjectPatch::compositeFromPlainToSmallerCU()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeBindingScriptFromPlainNew.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    // Old state: root has Inner child with value/doubled.
    QObject *child = object->findChild<QObject *>("child");
    QVERIFY(child);
    QCOMPARE(child->property("value").toInt(), 7);
    QCOMPARE(child->property("doubled").toInt(), 14);

    QQmlComponent newComp(&engine, testFileUrl("ExternalCompositeScript.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    const auto origObjects = objects;
    const bool updated = updateObjects(objects, oldExecUnit, newExecUnit);
    QVERIFY(updated);

    auto root = std::find(origObjects.begin(), origObjects.end(), object.get());
    QObject *replaced = objects[root - origObjects.begin()];
    if (replaced != object.get()) {
        object.release();
        object.reset(replaced);
    }

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // New state: ExternalCompositeScript has value=10, squared=100.
    QCOMPARE(object->property("value").toInt(), 10);
    QCOMPARE(object->property("squared").toInt(), 100);

    // Verify the binding is live.
    object->setProperty("value", 5);
    QCOMPARE(object->property("squared").toInt(), 25);
}

// Regression test: Patching a child constant (ChildConstantChangeNew → Old) must
// update the child's property without destroying the root's id registrations.
// This used to fail because child rebuilds shared root's context for id registration,
// overwriting the canonical id values from root's repopulateBindings. The fix creates
// ephemeral contexts for non-root children so their id registrations are isolated.
void tst_QQmlPreviewObjectPatch::childConstantChangePreservesRootIds()
{

    QQmlComponent oldComp(&engine, testFileUrl("ChildConstantChangeNew.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    // Old state: child has value=20.
    QQmlListProperty<QObject> items = object->property("items").value<QQmlListProperty<QObject>>();
    QCOMPARE(items.count(&items), 1);
    QObject *child = items.at(&items, 0);
    QVERIFY(child);
    QCOMPARE(child->property("value").toInt(), 20);

    QQmlComponent newComp(&engine, testFileUrl("ChildConstantChangeOld.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    const bool updated = updateObjects(objects, oldExecUnit, newExecUnit);
    QVERIFY(updated);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // New state: child has value=10 (reverted constant).
    items = object->property("items").value<QQmlListProperty<QObject>>();
    QCOMPARE(items.count(&items), 1);
    child = items.at(&items, 0);
    QVERIFY(child);
    QCOMPARE(child->property("value").toInt(), 10);
}

void tst_QQmlPreviewObjectPatch::scriptBindingChangeDropsCppPropertyOverride()
{

    QQmlComponent oldComp(&engine, testFileUrl("ScriptBindingChangeOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);
    QCOMPARE(object->objectName(), QStringLiteral("original"));

    // User overrides objectName — a C++ (non-dynamic) property.
    object->setObjectName(QStringLiteral("user-modified"));

    QQmlComponent newComp(&engine, testFileUrl("ScriptBindingChangeNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QEXPECT_FAIL("", "We do not preserve user overrides", Continue);
    QCOMPARE(object->objectName(), QStringLiteral("user-modified"));

    // The new CU's script binding was applied correctly.
    QCOMPARE(object->property("value").toInt(), 2); // aux * 2 = 1 * 2 = 2
}

// User-overridden values on children are retained during the patching.
void tst_QQmlPreviewObjectPatch::reattachLosesListChildUserOverrides()
{

    QQmlComponent oldComp(&engine, testFileUrl("ListUserOverrideOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QQmlListReference ref(object.data(), "items");
    QVERIFY(ref.isValid());
    QCOMPARE(ref.count(), 2);

    // User modifies the first child object's property during a live-preview session.
    ref.at(0)->setProperty("val", 99);
    QCOMPARE(ref.at(0)->property("val").toInt(), 99);

    QQmlComponent newComp(&engine, testFileUrl("ListUserOverrideNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    if (!updateObjects(objects, oldExecUnit, newExecUnit))
        QSKIP("not supported yet");

    QCOMPARE(ref.count(), 2);

    QEXPECT_FAIL("", "We do not preserve user overrides", Continue);
    QCOMPARE(ref.at(0)->property("val").toInt(), 99);
}

void tst_QQmlPreviewObjectPatch::varPropertyStashTypeMismatch()
{

    QQmlComponent oldComp(&engine, testFileUrl("VarPropertyReattachOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    // count is at its CU default; the user has NOT overridden it.
    QCOMPARE(object->property("count").toInt(), 10);

    QQmlComponent newComp(&engine, testFileUrl("VarPropertyReattachNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    // The new CU's default for count is 20; since the user has not overridden it,
    // the new default should win.
    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCOMPARE(object->property("count").toInt(), 20);
}

void tst_QQmlPreviewObjectPatch::reattachPreservesIdBinding()
{

    QQmlComponent oldComp(&engine, testFileUrl("ContextIdReattachOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QCOMPARE(object->property("base").toInt(), 5);
    QCOMPARE(object->property("derived").toInt(), 6); // root.base + 1

    QQmlComponent newComp(&engine, testFileUrl("ContextIdReattachNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    // The id `root` must be re-registered so the `derived: root.base + 1`
    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCOMPARE(object->property("base").toInt(), 5);
    QCOMPARE(object->property("derived").toInt(), 6); // root.base + 1
    QVERIFY(object->property("newFlag").toBool());
}

// Children are treated separately, so the child pointer survives and the
// updated value is applied.
void tst_QQmlPreviewObjectPatch::inPlaceOnChildNeedlesslyReattaches()
{

    QQmlComponent oldComp(&engine, testFileUrl("ChildConstantChangeOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QQmlListReference ref(object.data(), "items");
    QVERIFY(ref.isValid());
    QCOMPARE(ref.count(), 1);

    QObject *childBefore = ref.at(0);
    QVERIFY(childBefore);
    QCOMPARE(childBefore->property("value").toInt(), 10);

    QQmlComponent newComp(&engine, testFileUrl("ChildConstantChangeNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // The child's updated value is applied via reinitChildInPlace.
    QCOMPARE(ref.count(), 1);
    QCOMPARE(ref.at(0)->property("value").toInt(), 20);
}

// Test that an externally-assigned binding (installed via Qt.binding() from outside
// the document) is preserved.
//
// Setup:
//   - ExternalBindingOld.qml has "property int value: 5"
//   - From C++, an external binding is installed: value = Qt.binding(() => __mult * 6)
//     with __mult = 7, giving value = 42
//   - ExternalBindingNew.qml adds "property bool newFlag: true"
//
// After patching, changing __mult should re-trigger the binding.
void tst_QQmlPreviewObjectPatch::reattachPreservesExternalBinding()
{

    engine.rootContext()->setContextProperty("__mult", 7);

    QQmlComponent oldComp(&engine, testFileUrl("ExternalBindingOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);
    QCOMPARE(object->property("value").toInt(), 5);

    // Install an external binding via a small inline helper component.
    // The helper runs from a different URL, so the binding's sourceLocation
    // will not match oldUnit's URL — making it detectable as an external binding.
    engine.rootContext()->setContextProperty("__previewTestTarget", object.data());
    QQmlComponent binder(&engine);
    binder.setData("import QtQml 2.0;"
                   "QtObject {"
                   "  Component.onCompleted: {"
                   "    __previewTestTarget.value = Qt.binding(function() { return __mult * 6; });"
                   "  }"
                   "}",
                   QUrl("file:///test_external_binder.qml"));
    QVERIFY2(binder.isReady(), qPrintable(binder.errorString()));
    QScopedPointer<QObject> binderObj(binder.create());
    QVERIFY(binderObj);
    engine.rootContext()->setContextProperty("__previewTestTarget", QVariant());

    // External binding should be active: value = __mult * 6 = 7 * 6 = 42
    QCOMPARE(object->property("value").toInt(), 42);

    QQmlComponent newComp(&engine, testFileUrl("ExternalBindingNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QEXPECT_FAIL("", "We do not preserve user overrides", Continue);
    QCOMPARE(object->property("value").toInt(), 42);

    // Now change __mult to 8.  If the binding is live, value updates to 48.
    // If only the value was stashed (binding lost), value stays at 42.
    engine.rootContext()->setContextProperty("__mult", 8);

    QEXPECT_FAIL("", "We do not preserve user overrides", Continue);
    QCOMPARE(object->property("value").toInt(), 48);
}

// Verify that changing an enum in one component propagates to bindings in
// another component that references that enum.  The in-CU constant
// (EnumType.ownStatus) can be patched directly, but the cross-CU reference
// (Main.directStatus) requires lookup invalidation.
void tst_QQmlPreviewObjectPatch::enumChangeAcrossComponents()
{


    const QString moduleDir = dataDirectory() + QStringLiteral("/EnumModule");
    const QString patchedDir = dataDirectory() + QStringLiteral("/EnumModulePatched");

    // Load Main.qml which implicitly imports EnumType from the same directory.
    QQmlComponent mainComponent(&engine, QUrl::fromLocalFile(moduleDir + "/Main.qml"));
    QVERIFY2(mainComponent.isReady(), qPrintable(mainComponent.errorString()));
    QScopedPointer<QObject> mainObject(mainComponent.create());
    QVERIFY(mainObject);

    // In the old enum, Status.Ready is the 3rd value (index 2).
    QObject *child = mainObject->findChild<QObject *>(QStringLiteral("enumChild"));
    QVERIFY(child);
    QCOMPARE(child->property("ownStatus").toInt(), 2);
    QCOMPARE(mainObject->property("directStatus").toInt(), 2);
    QCOMPARE(mainObject->property("childStatus").toInt(), 2);

    // Get the old EnumType CU (same URL → engine returns the cached unit).
    QQmlComponent enumOldComponent(&engine, QUrl::fromLocalFile(moduleDir + "/EnumType.qml"));
    QVERIFY2(enumOldComponent.isReady(), qPrintable(enumOldComponent.errorString()));
    const auto oldEnumCU = QQmlComponentPrivate::get(&enumOldComponent)->compilationUnit();

    // Get the new EnumType CU from the patched directory.
    QQmlComponent enumNewComponent(&engine, QUrl::fromLocalFile(patchedDir + "/EnumType.qml"));
    QVERIFY2(enumNewComponent.isReady(), qPrintable(enumNewComponent.errorString()));
    const auto newEnumCU = QQmlComponentPrivate::get(&enumNewComponent)->compilationUnit();

    QVERIFY(oldEnumCU && newEnumCU);

    // Patch all live EnumType objects.
    auto enumObjects = objectsForCompilationUnit(&engine, oldEnumCU);
    QVERIFY(!enumObjects.empty());
    const bool patched = updateObjects(enumObjects, oldEnumCU, newEnumCU);
    QVERIFY(patched);

    // In the new enum, Processing was inserted before Ready, so Ready = 3.

    // (a) In-CU binding: the diff detects BindingChanged and EnumChanged; the
    //     re-installed binding evaluates against the replaced enum cache.
    QCOMPARE(child->property("ownStatus").toInt(), 3);

    // (b) Indirect via property binding: childStatus reads child.ownStatus.
    //     The binding is refreshed and reads the updated value from (a).
    QCOMPARE(mainObject->property("childStatus").toInt(), 3);

    // (c) Cross-CU enum reference: directStatus is compiled in Main's CU as a
    //     constant or lookup.  Main's CU is NOT patched, so this value is stale.
    //     This requires cross-CU lookup invalidation which is not yet implemented.
    QEXPECT_FAIL("", "Cross-CU lookup invalidation not yet implemented", Continue);
    QCOMPARE(mainObject->property("directStatus").toInt(), 3);
}

static void compareKeysEnabled(QObject *object, bool enabled)
{
    QQmlProperty keysEnabled(object, QStringLiteral("Keys.enabled"), qmlContext(object));
    QVERIFY(keysEnabled.isValid());
    QCOMPARE(keysEnabled.propertyMetaType(), QMetaType::fromType<bool>());
    QCOMPARE(keysEnabled.read().toBool(), enabled);
}

// Simple attached property value change (Keys.enabled: true → false).
void tst_QQmlPreviewObjectPatch::attachedPropertyValueChange()
{

    QQmlComponent oldComp(&engine, testFileUrl("AttachedPropertyChangeOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    // Keys.enabled is true by default
    compareKeysEnabled(object.data(), true);

    QCOMPARE(object->property("value").toInt(), 10);

    QQmlComponent newComp(&engine, testFileUrl("AttachedPropertyChangeNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCOMPARE(object->property("value").toInt(), 20);

    // Item.enabled must still be true — it was never set in the QML source.
    // If the patching incorrectly applied Keys.enabled: false to the Item
    // itself (instead of the Keys attached object), this would be false.
    QCOMPARE(object->property("enabled").toBool(), true);

    compareKeysEnabled(object.data(), false);
}

// An attached property (Keys.enabled) is added in the new CU that did not
// exist in the old CU.  This exercises installAttachedPropertyBinding():
// the outer Type_AttachedProperty binding must resolve the attached object
// and install the inner constant binding (enabled: false) on it.
void tst_QQmlPreviewObjectPatch::attachedPropertyAdded()
{

    QQmlComponent oldComp(&engine, testFileUrl("AttachedPropertyAddedOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    // Keys.enabled is true by default
    compareKeysEnabled(object.data(), true);

    QQmlComponent newComp(&engine, testFileUrl("AttachedPropertyAddedNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // Keys.enabled: false must have been applied to the attached object.
    compareKeysEnabled(object.data(), false);
}

// Inverse of attachedPropertyAdded: the attached property binding is removed.
// Keys.enabled has no RESET function, so the property value cannot be restored
// to its default. We verify the patch succeeds.
void tst_QQmlPreviewObjectPatch::attachedPropertyRemoved()
{

    QQmlComponent oldComp(&engine, testFileUrl("AttachedPropertyRemovedOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QQmlProperty keysEnabled(object.data(), QStringLiteral("Keys.enabled"),
                             qmlContext(object.data()));
    QVERIFY(keysEnabled.isValid());

    // Keys.enabled was set to false by the old QML.
    QCOMPARE(keysEnabled.read().toBool(), false);

    QQmlComponent newComp(&engine, testFileUrl("AttachedPropertyRemovedNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // After removal, the binding no longer drives the property.
    // But since the property has no RESET, it retains its value.
}

// Group property change on a value type (font.pixelSize: 12 → 24).
void tst_QQmlPreviewObjectPatch::groupPropertyFontChange()
{

    QQmlComponent oldComp(&engine, testFileUrl("GroupPropertyFontChangeOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QFont oldFont = object->property("font").value<QFont>();
    QCOMPARE(oldFont.pixelSize(), 12);

    QQmlComponent newComp(&engine, testFileUrl("GroupPropertyFontChangeNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QFont newFont = object->property("font").value<QFont>();
    QCOMPARE(newFont.pixelSize(), 24);
}

// Group property change on an object type (anchors.leftMargin: 5 → 10).
// anchors is backed by QQuickAnchors*, an actual QObject, unlike font (QFont
// value type).
void tst_QQmlPreviewObjectPatch::groupPropertyAnchorsChange()
{

    QQmlComponent oldComp(&engine, testFileUrl("GroupPropertyAnchorsChangeOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QObject *anchors = qvariant_cast<QObject *>(object->property("anchors"));
    QVERIFY(anchors);
    QCOMPARE(anchors->property("leftMargin").toReal(), 5.0);
    QCOMPARE(object->property("value").toInt(), 10);

    QQmlComponent newComp(&engine, testFileUrl("GroupPropertyAnchorsChangeNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCOMPARE(anchors->property("leftMargin").toReal(), 10.0);
    QCOMPARE(object->property("value").toInt(), 20);
}

// Change anchors.fill from "parent" to "sibling".  The fill binding's bytecode
// is identical in both CUs — only the lookup table entry changes.  This
// exercises three fixes:
//   1. Function pointer update on group property targets (QQuickAnchors).
//   2. Binding refresh on group property targets after resetLookups.
void tst_QQmlPreviewObjectPatch::groupPropertyAnchorsTargetChange()
{

    QQmlComponent oldComp(&engine, testFileUrl("GroupPropertyAnchorsTargetOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    // "target" is the second child (after sibling).
    QObject *target = object->children().at(1);
    QVERIFY(target);
    QCOMPARE(target->property("width").toReal(), 200.0); // fills parent

    QQmlComponent newComp(&engine, testFileUrl("GroupPropertyAnchorsTargetNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // Rebuild replaces inner objects; re-fetch target.
    target = object->children().at(1);
    QVERIFY(target);
    QCOMPARE(target->property("width").toReal(), 100.0); // now fills sibling
    QCOMPARE(target->property("height").toReal(), 100.0);
}

// A child Rectangle is inserted in the new CU before the font group property.
// Object indices are stable, but the *contents* at each index change: index 1
// held the font-group in the old CU, now holds a Rectangle in the new CU,
// and the font-group sits at index 2.
void tst_QQmlPreviewObjectPatch::groupPropIndexShift()
{

    QQmlComponent oldComp(&engine, testFileUrl("GroupPropIndexShiftOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QFont oldFont = object->property("font").value<QFont>();
    QCOMPARE(oldFont.pixelSize(), 12);
    QCOMPARE(object->property("marker").toInt(), 1);

    QQmlComponent newComp(&engine, testFileUrl("GroupPropIndexShiftNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QFont newFont = object->property("font").value<QFont>();
    QCOMPARE(newFont.pixelSize(), 24);
    QCOMPARE(object->property("marker").toInt(), 2);
}

// Reverse of groupPropIndexShift: a child is removed, so the content at
// index 1 changes from Rectangle (old) to font-group (new).
//   Old CU: obj0=Text, obj1=Rectangle, obj2=font-group
//   New CU: obj0=Text, obj1=font-group
void tst_QQmlPreviewObjectPatch::groupPropChildRemoved()
{

    QQmlComponent oldComp(&engine, testFileUrl("GroupPropChildRemovedOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QFont oldFont = object->property("font").value<QFont>();
    QCOMPARE(oldFont.pixelSize(), 12);
    QCOMPARE(object->property("marker").toInt(), 1);

    QQmlComponent newComp(&engine, testFileUrl("GroupPropChildRemovedNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QFont newFont = object->property("font").value<QFont>();
    QCOMPARE(newFont.pixelSize(), 24);
    QCOMPARE(object->property("marker").toInt(), 2);
}

// Full group property removal: anchors.leftMargin is removed entirely.
// anchors.leftMargin should revert to the default (0) via RESET.
void tst_QQmlPreviewObjectPatch::groupPropertyRemoved()
{

    QQmlComponent oldComp(&engine, testFileUrl("GroupPropertyRemovedOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QObject *anchors = qvariant_cast<QObject *>(object->property("anchors"));
    QVERIFY(anchors);
    QCOMPARE(anchors->property("leftMargin").toReal(), 42.0);

    QQmlComponent newComp(&engine, testFileUrl("GroupPropertyRemovedNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));
    QCOMPARE(anchors->property("leftMargin").toReal(), 0.0);
}

// Same scenario as groupPropIndexShift but for an attached property (Keys).
// Content at the Keys object's old index now holds a Rectangle in the new CU.
//   Old CU: obj0=Item, obj1=Keys-attached
//   New CU: obj0=Item, obj1=Rectangle, obj2=Keys-attached
void tst_QQmlPreviewObjectPatch::attachedPropIndexShift()
{

    QQmlComponent oldComp(&engine, testFileUrl("AttachedPropIndexShiftOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QCOMPARE(object->property("marker").toInt(), 1);

    QQmlComponent newComp(&engine, testFileUrl("AttachedPropIndexShiftNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCOMPARE(object->property("marker").toInt(), 2);
}

// Multiple group properties (font + anchors) on the same object, both changing.
void tst_QQmlPreviewObjectPatch::multiGroupChange()
{

    QQmlComponent oldComp(&engine, testFileUrl("MultiGroupChangeOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QFont oldFont = object->property("font").value<QFont>();
    QCOMPARE(oldFont.pixelSize(), 12);
    QCOMPARE(oldFont.bold(), false);

    QObject *anchors = qvariant_cast<QObject *>(object->property("anchors"));
    QVERIFY(anchors);
    QCOMPARE(anchors->property("leftMargin").toReal(), 5.0);

    QQmlComponent newComp(&engine, testFileUrl("MultiGroupChangeNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QFont newFont = object->property("font").value<QFont>();
    QCOMPARE(newFont.pixelSize(), 24);
    QCOMPARE(newFont.bold(), true);
    QCOMPARE(anchors->property("leftMargin").toReal(), 10.0);
}

// Both a group property (font) and an attached property (Keys) on the same
// object, both changing.  Tests that the inner object hash and redirect logic
// handle mixed group+attached correctly.
void tst_QQmlPreviewObjectPatch::groupAndAttachedChange()
{

    QQmlComponent oldComp(&engine, testFileUrl("GroupAndAttachedChangeOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    compareKeysEnabled(object.data(), true);

    QFont oldFont = object->property("font").value<QFont>();
    QCOMPARE(oldFont.pixelSize(), 12);
    QCOMPARE(object->property("marker").toInt(), 1);

    QQmlComponent newComp(&engine, testFileUrl("GroupAndAttachedChangeNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QFont newFont = object->property("font").value<QFont>();
    QCOMPARE(newFont.pixelSize(), 24);
    QCOMPARE(object->property("marker").toInt(), 2);

    // Item.enabled must still be true — only Keys.enabled was changed.
    QCOMPARE(object->property("enabled").toBool(), true);
    compareKeysEnabled(object.data(), false);
}

// Inline component with an instantiated instance; the component definition's
// property changes from 10 to 20. The instance should reflect the new value.
void tst_QQmlPreviewObjectPatch::inlineComponentInstanceChange()
{

    QQmlComponent oldComp(&engine, testFileUrl("InlineComponentInstanceOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QObject *inner = object->findChild<QObject *>("inner");
    QVERIFY(inner);
    QCOMPARE(inner->property("value").toInt(), 10);

    QQmlComponent newComp(&engine, testFileUrl("InlineComponentInstanceNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    inner = object->findChild<QObject *>("inner");
    QVERIFY(inner);
    QCOMPARE(inner->property("value").toInt(), 20);
}

// Implicit Component wrapper: the compiler wraps `Rectangle { property int
// value: 10 }` in an automatic Component.  After patching, instantiating the
// component from C++ should produce the updated value (20).
void tst_QQmlPreviewObjectPatch::implicitComponentContentChange()
{

    QQmlComponent oldComp(&engine, testFileUrl("ImplicitComponentContentOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    // Instantiate the delegate before patching to verify old value.
    QQmlComponent *delegate = qvariant_cast<QQmlComponent *>(object->property("delegate"));
    QVERIFY(delegate);
    QScopedPointer<QObject> instance(delegate->create());
    QVERIFY(instance);
    QCOMPARE(instance->property("value").toInt(), 10);

    QQmlComponent newComp(&engine, testFileUrl("ImplicitComponentContentNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // After patching, newly created instances should reflect the new value.
    delegate = qvariant_cast<QQmlComponent *>(object->property("delegate"));
    QVERIFY(delegate);
    QScopedPointer<QObject> instance2(delegate->create());
    QVERIFY(instance2);
    QCOMPARE(instance2->property("value").toInt(), 20);
}

// Explicit Component wrapper: `Component { id: comp; Rectangle { ... } }`.
// Same verification as the implicit variant.
void tst_QQmlPreviewObjectPatch::explicitComponentContentChange()
{

    QQmlComponent oldComp(&engine, testFileUrl("ExplicitComponentContentOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QQmlComponent *delegate = qvariant_cast<QQmlComponent *>(object->property("delegate"));
    QVERIFY(delegate);
    QScopedPointer<QObject> instance(delegate->create());
    QVERIFY(instance);
    QCOMPARE(instance->property("value").toInt(), 10);

    QQmlComponent newComp(&engine, testFileUrl("ExplicitComponentContentNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    delegate = qvariant_cast<QQmlComponent *>(object->property("delegate"));
    QVERIFY(delegate);
    QScopedPointer<QObject> instance2(delegate->create());
    QVERIFY(instance2);
    QCOMPARE(instance2->property("value").toInt(), 20);
}

// Adding a property to the root object while a child Timer's onTriggered
// signal handler binding also changes. The CU stores the raw signal name
// ("triggered") rather than the handler name ("onTriggered").
void tst_QQmlPreviewObjectPatch::signalHandlerWithPropertyAdd()
{

    QQmlComponent oldComp(&engine, testFileUrl("SignalHandlerWithPropertyAddOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QCOMPARE(object->property("count").toInt(), 10);
    QVERIFY(!object->property("label").isValid());

    QQmlComponent newComp(&engine, testFileUrl("SignalHandlerWithPropertyAddNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    // Verify the diff contains both PropertyAdded and BindingChanged.
    const auto diff = diffCompilationUnits(oldExecUnit->baseCompilationUnit()->unitData(),
                                           newExecUnit->baseCompilationUnit()->unitData());
    QVERIFY(diff.success);
    QVERIFY(countByType(diff, ChangeType::PropertyAdded) >= 1);
    QVERIFY(countByType(diff, ChangeType::BindingChanged) >= 1);

    // Apply — this used to assert in QQmlPropertyPrivate::setBinding()
    // because the signal handler binding had a null targetObject.
    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // The new property was added and initialized.
    QCOMPARE(object->property("label").toString(), QStringLiteral("new"));
    // The original property is preserved.
    QCOMPARE(object->property("count").toInt(), 10);
}

// Equivalent of tst_QQmlPreview::rerunAfterInPlaceUpdate().
// After an in-place constant change (count: 10 → 55), verify that:
//   1. The root's count property is updated.
//   2. No duplicate child objects were created.
//   3. All objects (root and children) reference the new compilation unit.
//   4. Signal handler functions are updated to the new compilation unit.
void tst_QQmlPreviewObjectPatch::inPlaceUpdateNoObjectDuplication()
{

    QQmlComponent oldComponent(&engine, testFileUrl("InplaceUpdateOld.qml"));
    QVERIFY2(oldComponent.isReady(), qPrintable(oldComponent.errorString()));
    QScopedPointer<QObject> object(oldComponent.create());
    QVERIFY(object);

    QCOMPARE(object->property("count").toInt(), 10);

    // Snapshot child count before the update.
    const int childCountBefore = object->children().size();
    QVERIFY(childCountBefore > 0); // Timer child

    QQmlComponent newComponent(&engine, testFileUrl("InplaceUpdateNew.qml"));
    QVERIFY2(newComponent.isReady(), qPrintable(newComponent.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComponent)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComponent)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    const qsizetype objectCountBefore = objects.size();
    QVERIFY(objectCountBefore > 0);

    // Apply the in-place update.
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // 1. Property was updated.
    QCOMPARE(object->property("count").toInt(), 55);

    // 2. No duplicate children — same count as before the patch.
    QCOMPARE(object->children().size(), childCountBefore);

    // 3. All objects now reference the new compilation unit.
    for (QObject *obj : objects) {
        QQmlData *ddata = QQmlData::get(obj);
        QVERIFY(ddata);
        QCOMPARE(ddata->compilationUnit, newExecUnit);
    }

    // 4. No signal handler function should still reference the old CU.
    //    QQmlBoundSignal::m_nextSignal is private, so we only check the head
    //    of each object's handler list.  The Timer has exactly one handler
    //    (onTriggered), so this is sufficient for our test.
    bool foundSignalHandler = false;
    for (QObject *obj : objects) {
        QQmlData *ddata = QQmlData::get(obj);
        if (!ddata || !ddata->signalHandlers)
            continue;
        auto *expr = ddata->signalHandlers->expression();
        if (!expr)
            continue;
        if (const QV4::Function *fn = expr->function()) {
            foundSignalHandler = true;
            QCOMPARE_NE(fn->executableCompilationUnit(), oldExecUnit.data());
        }
    }
    QVERIFY(foundSignalHandler); // Make sure we actually checked something

    // Also verify that findObjectsForCompilationUnits with the OLD unit
    // returns nothing — all objects were migrated to the new unit.
    auto staleObjects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE(staleObjects.size(), 0);
}

// Changing anchors.top target from parent.top to parent.verticalCenter
// should patch without crash and move the child.
void tst_QQmlPreviewObjectPatch::anchorsTopIndividualTargetChange()
{

    QQmlComponent oldComp(&engine, testFileUrl("AnchorsTopTargetOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    // "target" is the second child (index 0 after internal items).
    QObject *target = object->children().at(0);
    QVERIFY(target);
    QCOMPARE(target->property("y").toReal(), 0.0); // anchors.top: parent.top

    QQmlComponent newComp(&engine, testFileUrl("AnchorsTopTargetNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));
    target = object->children().at(0);
    QVERIFY(target);

    // After patch: anchors.top: parent.verticalCenter → y should be 100 (half of 200).
    QCOMPARE(target->property("y").toReal(), 100.0);

    // Run the event loop for several iterations to verify the value stays stable
    // (no alternation between old and new anchor values).
    for (int i = 0; i < 10; ++i) {
        QCoreApplication::processEvents();
        QCOMPARE(target->property("y").toReal(), 100.0);
    }
}

void tst_QQmlPreviewObjectPatch::anchorsTopTimerNoAlternation()
{

    QQmlComponent oldComp(&engine, testFileUrl("AnchorsTopTimerOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    // Let the Timer fire a few times to establish baseline.
    QTRY_VERIFY(object->property("log").toString().contains(QLatin1String("y=0y=0y=0")));

    // Clear the log.
    object->setProperty("log", QString());

    QQmlComponent newComp(&engine, testFileUrl("AnchorsTopTimerNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // After the patch, y should consistently be 100 (parent.verticalCenter
    // with parent height 200). No y=0 should appear.
    QTRY_VERIFY2(object->property("log").toString().contains(QLatin1String("y=100y=100y=100")),
                 qPrintable(QLatin1String("Expected y=100 not found!")));
    QVERIFY2(!object->property("log").toString().contains(QLatin1String("y=0")),
             qPrintable(QLatin1String("Alternation detected!")));
}

// Transition from a plain Item child to an inline component instance.
// The instance itself has no own properties — the VME must be built from the IC root.
void tst_QQmlPreviewObjectPatch::compositeVMERebuildFromPlain()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeVMERebuildFromPlainOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QObject *child = object->findChild<QObject *>("child");
    QVERIFY(child);
    QVERIFY(!child->property("value").isValid()); // no 'value' in old version

    QQmlComponent newComp(&engine, testFileUrl("CompositeVMERebuildFromPlainNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // Flush deferred deletes.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    child = object->findChild<QObject *>("child");
    QVERIFY(child);
    // The property exists (VME built from the IC root) and has the correct default value.
    QCOMPARE(child->property("value").toInt(), 42);
}

// IC type name changes (OldInner → NewInner) while the instance has no own properties.
// The VME is rebuilt from the new IC root.
void tst_QQmlPreviewObjectPatch::compositeVMERebuildICChange()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeVMERebuildICChangeOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QObject *child = object->findChild<QObject *>("child");
    QVERIFY(child);
    QCOMPARE(child->property("value").toInt(), 10);
    QCOMPARE(child->property("label").toString(), QString("old"));
    QVERIFY(!child->property("extra").isValid());

    QQmlComponent newComp(&engine, testFileUrl("CompositeVMERebuildICChangeNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    child = object->findChild<QObject *>("child");
    QVERIFY(child);
    // VME rebuilt from the new IC root: properties have correct defaults.
    QCOMPARE(child->property("value").toInt(), 20);
    QCOMPARE(child->property("label").toString(), QString("new"));
    QCOMPARE(child->property("extra").toDouble(), 3.14);
}

// Transition from a plain Item child to an instance of an external composite type.
// The instance has no own properties — the VME comes from the external QML file's root.
void tst_QQmlPreviewObjectPatch::compositeVMERebuildExternal()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeVMERebuildExternalOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QObject *child = object->findChild<QObject *>("child");
    QVERIFY(child);
    QVERIFY(!child->property("value").isValid());
    QVERIFY(!child->property("label").isValid());

    QQmlComponent newComp(&engine, testFileUrl("CompositeVMERebuildExternalNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // Flush deferred deletes.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    child = object->findChild<QObject *>("child");
    QVERIFY(child);
    // The property has correct default values (VME built from the external composite's root).
    QCOMPARE(child->property("value").toInt(), 100);
    QCOMPARE(child->property("label").toString(), QString("external"));
}

// Script binding evaluation: transition from plain Item to IC instance with
// a script binding that depends on another property declared in the same IC.
void tst_QQmlPreviewObjectPatch::compositeBindingScriptFromPlain()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeBindingScriptFromPlainOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QObject *child = object->findChild<QObject *>("child");
    QVERIFY(child);
    QVERIFY(!child->property("value").isValid());
    QVERIFY(!child->property("doubled").isValid());

    QQmlComponent newComp(&engine, testFileUrl("CompositeBindingScriptFromPlainNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    child = object->findChild<QObject *>("child");
    QVERIFY(child);
    // Constant binding from IC root.
    QCOMPARE(child->property("value").toInt(), 7);
    // Script binding (value * 2) evaluated as a live binding.
    QCOMPARE(child->property("doubled").toInt(), 14);

    // Verify the binding is live: writing value should update doubled.
    child->setProperty("value", 10);
    QCOMPARE(child->property("doubled").toInt(), 20);
}

// Script binding evaluation: IC type changes, old IC had script binding,
// new IC has a different script binding formula.
void tst_QQmlPreviewObjectPatch::compositeBindingScriptICChange()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeBindingScriptICChangeOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QObject *child = object->findChild<QObject *>("child");
    QVERIFY(child);
    QCOMPARE(child->property("value").toInt(), 3);
    QCOMPARE(child->property("doubled").toInt(), 6);

    QQmlComponent newComp(&engine, testFileUrl("CompositeBindingScriptICChangeNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    child = object->findChild<QObject *>("child");
    QVERIFY(child);
    // New IC: value=5, tripled=value*3=15. Old "doubled" should no longer exist.
    QCOMPARE(child->property("value").toInt(), 5);
    QVERIFY(!child->property("doubled").isValid());
    QCOMPARE(child->property("tripled").toInt(), 15);

    // Verify the binding is live.
    child->setProperty("value", 4);
    QCOMPARE(child->property("tripled").toInt(), 12);
}

// Script binding evaluation: transition from plain Item to external composite
// with a script binding.
void tst_QQmlPreviewObjectPatch::compositeBindingScriptExternal()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeVMERebuildExternalOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QObject *child = object->findChild<QObject *>("child");
    QVERIFY(child);
    QVERIFY(!child->property("value").isValid());
    QVERIFY(!child->property("squared").isValid());

    QQmlComponent newComp(&engine, testFileUrl("CompositeBindingScriptExternalNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    child = object->findChild<QObject *>("child");
    QVERIFY(child);
    // Constant binding from external composite.
    QCOMPARE(child->property("value").toInt(), 10);
    // Script binding (value * value) evaluated.
    QCOMPARE(child->property("squared").toInt(), 100);

    // Verify the binding is live.
    child->setProperty("value", 6);
    QCOMPARE(child->property("squared").toInt(), 36);
}

// Recursive composite: ExternalParent : ExternalGrandparent.
// Both levels provide constant bindings that must be evaluated.
void tst_QQmlPreviewObjectPatch::compositeRecursiveConstant()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeRecursiveConstantOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QObject *child = object->findChild<QObject *>("child");
    QVERIFY(child);
    QVERIFY(!child->property("gpValue").isValid());
    QVERIFY(!child->property("gpLabel").isValid());
    QVERIFY(!child->property("parentValue").isValid());
    QVERIFY(!child->property("parentLabel").isValid());

    QQmlComponent newComp(&engine, testFileUrl("CompositeRecursiveConstantNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    child = object->findChild<QObject *>("child");
    QVERIFY(child);
    // Grandparent-level constant bindings.
    QCOMPARE(child->property("gpValue").toInt(), 99);
    QCOMPARE(child->property("gpLabel").toString(), QStringLiteral("grandparent"));
    // Parent-level constant bindings.
    QCOMPARE(child->property("parentValue").toInt(), 50);
    QCOMPARE(child->property("parentLabel").toString(), QStringLiteral("parent"));
}

// Recursive composite with script bindings at both levels.
// ExternalParentScript : ExternalGrandparentScript — both have computed properties.
void tst_QQmlPreviewObjectPatch::compositeRecursiveScript()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeRecursiveScriptOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QObject *child = object->findChild<QObject *>("child");
    QVERIFY(child);
    QVERIFY(!child->property("gpValue").isValid());
    QVERIFY(!child->property("gpDoubled").isValid());
    QVERIFY(!child->property("parentValue").isValid());
    QVERIFY(!child->property("parentTripled").isValid());

    QQmlComponent newComp(&engine, testFileUrl("CompositeRecursiveScriptNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    child = object->findChild<QObject *>("child");
    QVERIFY(child);
    // Grandparent-level: gpValue = 7, gpDoubled = gpValue * 2 = 14.
    QCOMPARE(child->property("gpValue").toInt(), 7);
    QCOMPARE(child->property("gpDoubled").toInt(), 14);
    // Parent-level: parentValue = 3, parentTripled = parentValue * 3 = 9.
    QCOMPARE(child->property("parentValue").toInt(), 3);
    QCOMPARE(child->property("parentTripled").toInt(), 9);

    // Verify grandparent binding is live.
    child->setProperty("gpValue", 10);
    QCOMPARE(child->property("gpDoubled").toInt(), 20);

    // Verify parent binding is live.
    child->setProperty("parentValue", 4);
    QCOMPARE(child->property("parentTripled").toInt(), 12);
}

// Recursive composite: IC 'Inner' derives from ExternalComposite.
// The IC has its own property (extra), and ExternalComposite provides
// value and label. Both levels must have VMEs and binding evaluation.
void tst_QQmlPreviewObjectPatch::compositeRecursiveICExternal()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeRecursiveICExternalOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QObject *child = object->findChild<QObject *>("child");
    QVERIFY(child);
    QVERIFY(!child->property("value").isValid());
    QVERIFY(!child->property("label").isValid());
    QVERIFY(!child->property("extra").isValid());

    QQmlComponent newComp(&engine, testFileUrl("CompositeRecursiveICExternalNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    child = object->findChild<QObject *>("child");
    QVERIFY(child);
    // ExternalComposite (grandparent-level) constant bindings.
    QCOMPARE(child->property("value").toInt(), 100);
    QCOMPARE(child->property("label").toString(), QStringLiteral("external"));
    // IC-level (immediate base) constant binding.
    QCOMPARE(child->property("extra").toInt(), 77);
}

// Reproduces a crash from the coffee example: when an in-place update rebuilds a
// composite type instance whose base type uses IDs referenced by property aliases,
// alias resolution in setupBindings crashes because the context's ID table is
// not populated for the base type's compilation unit.
//
// Structure:
//   CompositeAliasRebuildOld/New.qml (top-level, modified file):
//     CompositeBaseWithAliases { header.color: "blue"/"red" }
//   CompositeBaseWithAliases.qml:
//     Item { id: root; property alias header: headerText; Text { id: headerText } ... }
//
// The crash occurs during rebuildObject at the composite base level
// (CompositeBaseWithAliases), when alias "header" tries to resolve id
// "headerText" via idValue() on a context without IDs.
void tst_QQmlPreviewObjectPatch::compositeAliasRebuildCrash()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeAliasRebuildOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    // Sanity: verify the composite's alias works.
    QObject *header = object->property("header").value<QObject *>();
    QVERIFY(header);
    QCOMPARE(header->property("color").value<QColor>(), QColor("blue"));

    QQmlComponent newComp(&engine, testFileUrl("CompositeAliasRebuildNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // Verify the alias still resolves and the color change was applied.
    header = object->property("header").value<QObject *>();
    QVERIFY(header);
    QCOMPARE(header->property("color").value<QColor>(), QColor("red"));
}

// Two instances of CompositeBaseWithAliases in the same document.
// Both must survive rebuild without alias resolution failures.
void tst_QQmlPreviewObjectPatch::multipleCompositeInstances()
{

    QQmlComponent oldComp(&engine, testFileUrl("MultipleCompositeInstancesOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    // Sanity: marker starts at 1.
    QCOMPARE(object->property("marker").toInt(), 1);

    QQmlComponent newComp(&engine, testFileUrl("MultipleCompositeInstancesNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // Verify the marker property was updated.
    QCOMPARE(object->property("marker").toInt(), 2);

    // Verify both composite instances still resolve aliases.
    int compositesFound = 0;
    for (QObject *child : object->children()) {
        QObject *header = child->property("header").value<QObject *>();
        if (!header)
            continue;
        ++compositesFound;
        QVERIFY(!header->property("text").toString().isEmpty());
    }
    QCOMPARE(compositesFound, 2);
}

// DerivedCompositeWithAliases inherits CompositeBaseWithAliases (3 composite levels:
// Item → CompositeBaseWithAliases → DerivedCompositeWithAliases → instance).
// Tests deep alias chain resolution during rebuild.
void tst_QQmlPreviewObjectPatch::nestedCompositeAlias()
{

    QQmlComponent oldComp(&engine, testFileUrl("NestedCompositeAliasOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    QQmlComponent newComp(&engine, testFileUrl("NestedCompositeAliasNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // Find the DerivedCompositeWithAliases child and verify its aliases resolve.
    QObject *nested = nullptr;
    for (QObject *child : object->children()) {
        if (child->property("counter").isValid()) {
            nested = child;
            break;
        }
    }
    QVERIFY(nested);
    // The counter property must reflect the new value.
    QCOMPARE(nested->property("counter").toInt(), 99);
    // The deep alias chain (headerText → header.text) must resolve.
    QCOMPARE(nested->property("headerText").toString(), QStringLiteral("Deep Nested"));
}

// Composite with signal handler referencing internal IDs.
// The rebuild must not corrupt signal handler bindings.
void tst_QQmlPreviewObjectPatch::compositeSignalHandler()
{
    QQmlEngine engine;

    QQmlComponent oldComp(&engine, testFileUrl("CompositeSignalHandlerOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    QQmlComponent newComp(&engine, testFileUrl("CompositeSignalHandlerNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // Find the CompositeWithSignalHandler child and verify its alias resolves.
    QObject *handler = nullptr;
    for (auto *c : object->children()) {
        if (c->property("indicator").isValid()) {
            handler = c;
            break;
        }
    }
    QVERIFY(handler);

    QObject *indicator = handler->property("indicator").value<QObject *>();
    QVERIFY(indicator);

    // Find the trigger Rectangle and verify its color was updated to "yellow".
    QObject *trigger = nullptr;
    for (auto *c : object->children()) {
        if (c != handler && c->property("color").isValid()) {
            trigger = c;
            break;
        }
    }
    QVERIFY(trigger);
    QCOMPARE(trigger->property("color").value<QColor>(), QColor("yellow"));
}

// Composite with Repeater — dynamic children during rebuild.
void tst_QQmlPreviewObjectPatch::compositeRepeater()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeRepeaterOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    // Sanity: tag starts as "old".
    QCOMPARE(object->property("tag").toString(), QStringLiteral("old"));

    QQmlComponent newComp(&engine, testFileUrl("CompositeRepeaterNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // Verify the tag property was updated.
    QCOMPARE(object->property("tag").toString(), QStringLiteral("new"));
}

// Instance-level binding that reads from a base type alias.
// The binding expression references "header.text" and "header.width"
// which requires alias resolution in the base type context.
//
// BUG: After rebuild, child objects created by repopulateBindings at the
// composite level are owned by V4 GC rather than QObject parent-child.
// BindingPatchContext::reset() clears list properties (e.g. "data") for bindings
// from the composite level. Clearing the data list calls setParentItem(nullptr) on
// all child items, unparenting them. The orphaned children survive via GC ownership.
// We must clear contextObject on the old contexts before this happens to prevent
// use-after-free when GC later collects the orphans.
void tst_QQmlPreviewObjectPatch::instanceBindingReadsAlias()
{

    QQmlComponent oldComp(&engine, testFileUrl("InstanceBindingReadsAliasOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    QVERIFY(!object->property("headerInfo").toString().isEmpty());

    QQmlComponent newComp(&engine, testFileUrl("InstanceBindingReadsAliasNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QVERIFY(!object->property("headerInfo").toString().isEmpty());
    QCOMPARE(object->property("bgColor").value<QColor>(), QColor("red"));
}

// Composite with an explicit Component{} child (creates a sub-context).
// Rebuild should handle the nested context correctly.
void tst_QQmlPreviewObjectPatch::compositeComponent()
{

    QQmlComponent oldComp(&engine, testFileUrl("CompositeComponentOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    // Sanity: marker starts at 1.
    QCOMPARE(object->property("marker").toInt(), 1);

    QQmlComponent newComp(&engine, testFileUrl("CompositeComponentNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // Verify the marker property was updated.
    QCOMPARE(object->property("marker").toInt(), 2);

    // Find the CompositeWithComponent child and verify its label alias.
    QObject *comp = nullptr;
    for (auto *c : object->children()) {
        if (c->property("label").isValid()) {
            comp = c;
            break;
        }
    }
    QVERIFY(comp);
    QObject *label = comp->property("label").value<QObject *>();
    QVERIFY(label);
    QCOMPARE(label->property("text").toString(), QStringLiteral("Hello"));
}

// Inline children that reference the composite base's aliases via dot notation.
// The inner Text reads "mainItem.header.text" which requires alias resolution
// across the instance→base boundary during rebuild.
//
// Same scenario as instanceBindingReadsAlias with inline children.
void tst_QQmlPreviewObjectPatch::inlineChildReadsAlias()
{

    QQmlComponent oldComp(&engine, testFileUrl("InlineChildReadsAliasOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    QQmlComponent newComp(&engine, testFileUrl("InlineChildReadsAliasNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // Verify status property updated.
    QCOMPARE(object->property("status").toString(), QStringLiteral("new"));
}

// Composite with States/Transitions referencing internal IDs via PropertyChanges.
// The State objects hold target references that must survive rebuild.
void tst_QQmlPreviewObjectPatch::compositeStates()
{
    QQmlComponent oldComp(&engine, testFileUrl("CompositeStatesOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    // Sanity: tag starts as "old".
    QCOMPARE(object->property("tag").toString(), QStringLiteral("old"));

    QQmlComponent newComp(&engine, testFileUrl("CompositeStatesNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // Verify the tag property was updated.
    QCOMPARE(object->property("tag").toString(), QStringLiteral("new"));

    // Find the CompositeWithStates child and verify alias + state.
    QObject *stateful = nullptr;
    for (auto *c : object->children()) {
        if (c->property("indicator").isValid()) {
            stateful = c;
            break;
        }
    }
    QVERIFY(stateful);

    QObject *indicator = stateful->property("indicator").value<QObject *>();
    QVERIFY(indicator);
    // In "active" state, PropertyChanges sets color to "green".
    QCOMPARE(indicator->property("color").value<QColor>(), QColor("green"));
}

// Reproduces the context hierarchy corruption crash from the coffee demo:
// Main.qml → ApplicationFlow.qml → Home.qml (extends HomeForm.ui.qml)
// When Home.qml's CU is rebuilt (binding change in the "when" condition),
// the Home instance's context chain loses intermediate levels because
// outerContext points to the instantiating context (ApplicationFlow) and
// rebuildObject does outerContext->parent() which overshoots.
// The State's deferred content then tries to resolve IDs (header, caption)
// from the base form on a context with no IDs → crash in lookupIdObject.
void tst_QQmlPreviewObjectPatch::compositeContextHierarchyCrash()
{
    // Load the wrapper which instantiates CompositeContextDerivedOld inside it.
    // This establishes the outer context (wrapper) → derived → base form chain.
    QQmlComponent wrapperComp(&engine, testFileUrl("CompositeContextHierarchyWrapper.qml"));
    QVERIFY2(wrapperComp.isReady(), qPrintable(wrapperComp.errorString()));
    std::unique_ptr<QObject> wrapper(wrapperComp.create());
    QVERIFY(wrapper);

    // Find the CompositeContextDerived child.
    QObject *derived = nullptr;
    for (auto *c : wrapper->children()) {
        if (c->property("smallMode").isValid()) {
            derived = c;
            break;
        }
    }
    QVERIFY(derived);

    QCOMPARE(derived->property("smallMode").toBool(), true);
    QCOMPARE(derived->property("width").toDouble(), 300);
    QCOMPARE(derived->property("height").toDouble(), 300);

    // Verify the "small" state is active (smallMode: true triggers the when binding).
    QObject *header = derived->property("header").value<QObject *>();
    QVERIFY(header);
    QCOMPARE(header->property("font").value<QFont>().pixelSize(), 28);

    // Now load the "new" version of the derived type's CU (when binding changed).
    QQmlComponent oldDerivedComp(&engine, testFileUrl("CompositeContextDerivedOld.qml"));
    QVERIFY2(oldDerivedComp.isReady(), qPrintable(oldDerivedComp.errorString()));
    QQmlComponent newDerivedComp(&engine, testFileUrl("CompositeContextDerivedNew.qml"));
    QVERIFY2(newDerivedComp.isReady(), qPrintable(newDerivedComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldDerivedComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newDerivedComp)->compilationUnit();

    // Find objects belonging to the derived type's CU.
    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);

    // This triggers the crash: rebuilding the derived CU corrupts the context
    // hierarchy of the composite instance (whose outerContext is the wrapper),
    // and the deferred State content (PropertyChanges targeting IDs from the
    // base form) crashes on lookupIdObject because the context has no ID values.
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCOMPARE(derived->property("smallMode").toBool(), true);
    QCOMPARE(derived->property("width").toDouble(), 300);
    QCOMPARE(derived->property("height").toDouble(), 300);

    // If we survive, verify things are correct.
    header = derived->property("header").value<QObject *>();
    QVERIFY(header);
    QCOMPARE(header->property("font").value<QFont>().pixelSize(), 28);
}

// Reproduces the original lookupIdObject crash: composite type with states
// whose PropertyChanges target IDs from the base form. When the derived CU
// is rebuilt while the instance lives inside an outer wrapper, the context
// hierarchy gets corrupted (missing intermediate levels) and ID lookups
// crash on a context with m_idValues=nullptr.
void tst_QQmlPreviewObjectPatch::compositeContextIdLookupCrash()
{
    // Load the wrapper which instantiates CompositeContextIdLookupOld inside it.
    QQmlComponent wrapperComp(&engine, testFileUrl("CompositeContextIdLookupWrapper.qml"));
    QVERIFY2(wrapperComp.isReady(), qPrintable(wrapperComp.errorString()));
    std::unique_ptr<QObject> wrapper(wrapperComp.create());
    QVERIFY(wrapper);

    // Process events to evaluate bindings and cache lookups (State "when" binding).
    QCoreApplication::processEvents();

    // Find the composite derived child.
    QObject *derived = nullptr;
    for (auto *c : wrapper->children()) {
        if (c->property("smallMode").isValid()) {
            derived = c;
            break;
        }
    }
    QVERIFY(derived);

    // Verify the "small" state is active (smallMode: true).
    QObject *header = derived->property("header").value<QObject *>();
    QVERIFY(header);
    QCOMPARE(header->property("font").value<QFont>().pixelSize(), 28);

    // Load Old and New versions of the derived type's CU for the diff.
    QQmlComponent oldDerivedComp(&engine, testFileUrl("CompositeContextIdLookupOld.qml"));
    QVERIFY2(oldDerivedComp.isReady(), qPrintable(oldDerivedComp.errorString()));
    QQmlComponent newDerivedComp(&engine, testFileUrl("CompositeContextIdLookupNew.qml"));
    QVERIFY2(newDerivedComp.isReady(), qPrintable(newDerivedComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldDerivedComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newDerivedComp)->compilationUnit();

    // Find objects belonging to the derived type's CU.
    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);

    // This crashes: the rebuild corrupts the context hierarchy so that
    // the deferred State/PropertyChanges content tries to look up "header"
    // and "caption" IDs on a context that has no ID values.
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // If we survive, verify correctness.
    header = derived->property("header").value<QObject *>();
    QVERIFY(header);
    QCOMPARE(header->property("font").value<QFont>().pixelSize(), 28);
}

// Reproduces the crash from Backtrace2.txt: ASSERT "item == parent()" in
// qquicksafearea.cpp after in-place preview rebuild orphans the SafeArea
// attached object (via BindingPatchContext::reset line 37-42 calling
// setParent(nullptr) + deleteLater) while it's still registered as a
// Matrix change listener on the item.
void tst_QQmlPreviewObjectPatch::safeAreaAttachedRebuildCrash()
{
    QQuickWindow window;
    window.resize(200, 200);

    QQmlComponent oldComp(&engine, testFileUrl("SafeAreaRebuildOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    // Parent the root item into the window so SafeArea is meaningful.
    QQuickItem *rootItem = qobject_cast<QQuickItem *>(object.get());
    QVERIFY(rootItem);
    rootItem->setParentItem(window.contentItem());

    // Verify the SafeArea attached property is working.
    QVERIFY(object->property("safeLeft").isValid());

    // Load the new version (color changed from blue to red).
    QQmlComponent newComp(&engine, testFileUrl("SafeAreaRebuildNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    // Trigger a resize on the window — this causes transformChanged on
    // QQuickRootItem → notifyChangeListeners → SafeArea::itemTransformChanged
    // which asserts item == parent(). The orphaned SafeArea has parent()==nullptr.
    window.resize(300, 300);
    QCoreApplication::processEvents();

    // Also resize the root item directly for good measure.
    rootItem->setSize(QSizeF(300, 300));
    QCoreApplication::processEvents();

    // If we reach here without crashing, the bug is fixed (or not reproduced).
    // Verify the color was actually updated.
    QQuickItem *background = nullptr;
    for (QQuickItem *child : rootItem->childItems()) {
        if (child->property("color").value<QColor>() == QColor("red")) {
            background = child;
            break;
        }
    }
    QVERIFY(background);
}

QTEST_MAIN(tst_QQmlPreviewObjectPatch)

#include "tst_qqmlpreviewobjectpatch.moc"
