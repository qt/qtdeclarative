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
#include <private/qqmlcontextdata_p.h>
#include <private/qqmlboundsignal_p.h>
#include <private/qqmlpreviewobjectpatch_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4executablecompilationunit_p.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <private/qquickitem_p.h>

#include <QtGui/qfont.h>
#include <QtGui/qcolor.h>
#include <QtCore/qpoint.h>

#include <QtQml/qqml.h>

#include <functional>

using namespace QV4::CompiledData;

// A single observable value of a reloaded widget: how to read it, and what it should be
// before the reload, after the reload, and after mutating the source property (to prove the
// re-applied binding is live).
struct DeferredProbe
{
    std::function<qreal(QObject *)> read;
    qreal before;
    qreal afterReload;
    qreal afterLive;
};
using DeferredProbes = QList<DeferredProbe>;
Q_DECLARE_METATYPE(DeferredProbes)

// A type with a deferred property of a *value* type (int). Its deferred binding
// does not create an object; it just assigns a value. Used to verify that the
// preview rebuild re-arms non-object deferred bindings too.
class DeferredIntItem : public QQuickItem
{
    Q_OBJECT
    Q_CLASSINFO("DeferredPropertyNames", "amount")
    Q_PROPERTY(int amount READ amount WRITE setAmount NOTIFY amountChanged)
public:
    int amount() const { return m_amount; }
    void setAmount(int amount)
    {
        if (m_amount == amount)
            return;
        m_amount = amount;
        emit amountChanged();
    }

    void componentComplete() override
    {
        QQuickItem::componentComplete();
        // Opt in to executing the deferred property, like a Control does for its
        // contentItem/background in its own componentComplete().
        qmlExecuteDeferred(this);
    }

Q_SIGNALS:
    void amountChanged();

private:
    int m_amount = -1;
};

// An attached type carrying an observable value. Used to check whether a deferred
// *attached-property* binding survives a preview rebuild. Such a binding resolves to
// no QQmlPropertyData on the host object, so QQmlData::deferData files it under the
// key -1.
class DeferredAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int amount READ amount WRITE setAmount NOTIFY amountChanged)
public:
    DeferredAttached(QObject *parent = nullptr) : QObject(parent) {}

    int amount() const { return m_amount; }
    void setAmount(int amount)
    {
        if (m_amount == amount)
            return;
        m_amount = amount;
        emit amountChanged();
    }

    static DeferredAttached *qmlAttachedProperties(QObject *object)
    {
        return new DeferredAttached(object);
    }

Q_SIGNALS:
    void amountChanged();

private:
    int m_amount = -1;
};
QML_DECLARE_TYPEINFO(DeferredAttached, QML_HAS_ATTACHED_PROPERTIES)

// A type that defers *all* bindings except the few listed, the way prominent Qt types
// (Binding, PropertyChanges) do via ImmediatePropertyNames. Any binding on it that is
// not one of the listed names — an attached-property binding (DeferredAttached.amount)
// or a generalized grouped property whose first chain part is an id (someId.x) — is
// therefore deferred and lands in DeferredData under the key -1.
//
// The "target" property is immediate so an id-bearing object can be assigned to it
// (an id on a deferred object is a compile error), giving the generalized grouped
// property test something to reference by id.
class ImmediateHost : public QQuickItem
{
    Q_OBJECT
    Q_CLASSINFO("ImmediatePropertyNames", "objectName,target,spot")
    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged)
    // px/py are C++ (not VME) properties, so their values survive a preview rebuild that
    // reuses this QObject. That lets a generalized grouped property targeting a surviving
    // object be observed after the rebuild.
    Q_PROPERTY(int px READ px WRITE setPx NOTIFY pxChanged)
    Q_PROPERTY(int py READ py WRITE setPy NOTIFY pyChanged)
    // A value-type property with sub-properties, so "spot.x" is a plain value-type group
    // property (not a generalized grouped property). Its name can be made to clash with an id.
    Q_PROPERTY(QPoint spot READ spot WRITE setSpot NOTIFY spotChanged)
public:
    QQuickItem *target() const { return m_target; }
    void setTarget(QQuickItem *target)
    {
        if (m_target == target)
            return;
        m_target = target;
        emit targetChanged();
    }

    int px() const { return m_px; }
    void setPx(int px)
    {
        if (m_px == px)
            return;
        m_px = px;
        emit pxChanged();
    }

    int py() const { return m_py; }
    void setPy(int py)
    {
        if (m_py == py)
            return;
        m_py = py;
        emit pyChanged();
    }

    QPoint spot() const { return m_spot; }
    void setSpot(QPoint spot)
    {
        if (m_spot == spot)
            return;
        m_spot = spot;
        emit spotChanged();
    }

    void componentComplete() override
    {
        QQuickItem::componentComplete();
        // Opt in to executing the deferred properties, like a Control does for its
        // contentItem/background in its own componentComplete().
        qmlExecuteDeferred(this);
    }

Q_SIGNALS:
    void targetChanged();
    void pxChanged();
    void pyChanged();
    void spotChanged();

private:
    QQuickItem *m_target = nullptr;
    int m_px = 0;
    int m_py = 0;
    QPoint m_spot;
};

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
    void updateObjectsGeneratorMethodChange();
    void updateObjectsFunctionChange();
    void updateObjectsFunctionRemove();

    void scriptBindingChangeDropsCppPropertyOverride();
    void bindingPropertyRename();
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
    void groupPropertyValueTypeOverridePreserved();
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

    // Verify that rebuilding parent+child from the same CU does not leak
    // orphaned QObject children (child rebuilt individually then replaced
    // by parent's repopulateBindings).
    void rebuildDoesNotLeakChildren();

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

    // Reproduces the coffee demo "Out of" label bug: a script binding
    // (visible: (root.cupsLeft != 0) ? false : true) is changed to a constant
    // (visible: true), then changed back. The binding must be re-created so
    // that the visible property responds to cupsLeft changes again.
    void scriptToConstantToScriptRoundtrip();

    // Same as above but the external binding is a script expression (2 + 3)
    // rather than a literal (5). Verifies that external script bindings also
    // survive a rebuild of the target's compilation unit.
    void scriptToConstantToScriptRoundtripExternalScript();

    // SafeArea attached property assertion crash after rebuild.
    // Reproduces ASSERT "item == parent()" in qquicksafearea.cpp when
    // resize event fires after rebuild orphaned the attached SafeArea object.
    void safeAreaAttachedRebuildCrash();

    // External signal handler disconnection: when an object is rebuilt, signal
    // connections made from outside that object are lost. This test exercises
    // the known issue by connecting a signal handler from a separate (non-rebuilt)
    // object and verifying it fails to fire after rebuild.
    void externalSignalHandlerLostOnRebuild();
    void externalSignalHandlerSignalRemoved();

    // Same as above but connects to the signal via grouped property syntax
    // (e.g. "target.onFired: ...") rather than a Connections element.
    void externalSignalHandlerGroupedPropertySyntax();

    // Same as above but uses signal.connect() from JavaScript.
    void externalSignalHandlerJsConnect();

    // Reproduces the coffee demo button bug: the signal handler is on a
    // sub-object (accessed via alias) of the rebuilt component, not on the
    // root. When the component is rebuilt, the sub-object gets replaced and
    // the grouped-property signal handler is lost.
    void externalSignalHandlerOnSubObject();

    // Same as above but the sub-object is accessible only via a list property
    // (property list<Timer>), not a single QObject* alias.
    void externalSignalHandlerOnListChild();

    // Consecutive in-place updates must skip objects whose outer context was
    // invalidated by a prior rebuild. Without the isValid() check in
    // rebuildObject(), the second update crashes accessing a dead context.
    void consecutiveUpdatesDeadContext();

    // Reproduces the assertion failure reported on change 741247: an external
    // property binding installed on a sub-object (accessed via alias) has its
    // targetObject pointing to the old child. After rebuild, refreshObjects()
    // updates m_object to the new child, but restoreExternalState() installs
    // the binding without updating targetObject first, triggering:
    //   ASSERT: "abstractBinding->targetObject() == target.object()"
    void externalBindingOnSubObjectTargetMismatch();

    // Coffee demo regression: signal handler on sub-object is lost when
    // the signal was never fired before rebuild (endpoint stays in todo queue).
    void externalSignalHandlerOnSubObjectUnfired();

    // Coffee demo regression: after rebuilding a composite type, old visual
    // children remain in the scene (parentItem not cleared). The bug was that
    // QMetaProperty::write() with QVariant::fromValue<QObject*>(nullptr) fails
    // silently when the property's metatype is QQuickItem* (type mismatch).
    void compositeRebuildNoVisualChildDuplication();

    void singletonConstantPropertyChange();
    void singletonBindingPropagation();
    void singletonChildObjectPropertyChange();
    void singletonPropertyAddition();
    void singletonConsumerBindingRefresh();

    // Child ordering: binding index shifts (e.g., from property addition)
    // must not reorder or lose children from the visual tree.
    void childOrderBindingShift_data();
    void childOrderBindingShift();

    // Required property supplied via createWithInitialProperties must be
    // preserved across a hot-reload rebuild of that component's CU. Without
    // the fix in stashExternalState(), the VME meta-object reconstruction
    // resets the required property to null and the dependent bindings break.
    void requiredPropertyPreservedOnRebuild();

    // Bindings from a child composite type's own CU on deeply-nested sub-objects
    // (accessed through intermediate C++ parents) must not be incorrectly stashed
    // during a parent form rebuild. The child is fully recreated during the
    // rebuild, so its internal bindings are fresh. Stashing and restoring them
    // overwrites the fresh binding with a stale copy that has the wrong scope.
    void childBindingScopeAfterFormRebuild();

    // Rebuilding a form CU when the top-level object is a derived type (directly
    // inheriting from the form, not instantiated by an outer CU) must not crash.
    // The instance-level VME creation must use the correct CU and objectIndex.
    void topLevelDerivedTypeFormRebuild();

    // Reproduces coffee demo crash 1: adding a new child object with an id to a
    // form causes a heap-buffer-overflow in QQmlContextData::setIdValue because
    // the context's id array was allocated for the old id count.
    void childAddedWithIdCrash();

    // Reproduces coffee demo crash 2: same as above but the form is used as a
    // composite base type (derived type instantiates the form).
    void childAddedWithIdCompositeCrash();

    void insertBinding();

    // Reproduces the calqlatr hot-reload bug: changing the default value of a
    // color property in a composite type (CalculatorButton) that is used for
    // many visual-child instances makes all those instances disappear from the
    // scene instead of just changing color. A sibling of an unrelated composite
    // type (BackspaceButton) must survive untouched.
    void compositePropertyDefaultChangeKeepsVisualChildren();

    // Reproduces the calqlatr hot-reload assert: adding a property (e.g.
    // "property int bla") to a composite button type used only as a base type of
    // derived instances (component DigitButton: CalculatorButton {}) inserts a
    // property and its change signal into the base VME meta-object, shifting the
    // index of the base type's own JS methods (getBackgroundColor()/getTextColor()).
    // The base carries deferred bindings, so the derived instances are recreated by rebuilding the
    // enclosing root; their property caches must be relinked to the new base layout, otherwise a
    // base VME method resolved through them lands at a stale index and asserts
    // "index >= methodOffset()" in QQmlVMEMetaObject::vmeMethod().
    void compositeBaseLayoutChangeRelinksDerivedCaches();

    // A Control-derived composite type appearing only as a composite base of derived instances
    // (component DigitButton: CalculatorButton {}) carries deferred bindings. A structural edit to
    // that base recreates the derived instances via the enclosing root ("go up"), rather than an
    // in-place rebuild that would silently drop their deferred contentItem/background. Verifies the
    // instances are fresh objects and their deferred content survives.
    void deferredDerivedInstanceRecreatedOnStructuralReload();

    // Same, but the derived type also declares its own property (and thus its own
    // change signal) and its own method, exercising the index-shifting of the
    // derived cache's *own* members when the base grows.
    void compositeBaseLayoutChangeRelinksDerivedOwnMembers();

    // Same as above, but the deferred contentItem is an instance of a composite
    // type from a separate .qml file (not an inline component, not a built-in
    // type), and the derived button types are separate .qml files too. The
    // deferred content must still be recreated on reload.
    void compositePropertyDefaultChangeExternalDeferredContent();
    void reloadDeferredContentWithAlias();

    // A deferred binding that must be re-applied when the object is recreated on reload.
    // Data-driven over the shapes such a binding can take:
    //  - a plain value-type property (amount: base), which assigns a value rather than
    //    creating an object;
    //  - an attached-property binding (DeferredAttached.amount: base) and a generalized
    //    grouped-property binding (someId.prop: base), which resolve to no QQmlPropertyData
    //    and land in DeferredData under the key -1 (the case Binding and PropertyChanges
    //    rely on ImmediatePropertyNames for), and whose old id-named binding must reset
    //    without asserting;
    //  - a grouped-property binding whose left-hand side is retargeted (self.px -> self.py),
    //    where the old left-hand side must be left at its C++ default.
    void reloadDeferredProperty_data();
    void reloadDeferredProperty();

    // A value-type group property ("spot.x") whose first chain part is also the id of another
    // object. The patcher must resolve it to the host's value property, not to the id object.
    void groupPropertyNameClashesWithId();

    // A base-type property that un-reloaded derived instances still bind to is
    // removed on reload. Re-resolving the binding target against the relinked
    // cache yields no property, so refreshBindingPropertyData() must clear that
    // binding-target entry. Without the fix it kept the stale (old-layout)
    // property-data pointer, so the surviving binding wrote through the wrong
    // offset (heap-use-after-free / clobbered sibling property).
    void compositeBaseLayoutChangeDropsRemovedBaseProperty();

    // Changing the non-composite (C++) base type of a component root: instead of failing the
    // hot reload, the enclosing component root is rebuilt so the object is recreated with the
    // new base type. Reaching the topmost scope this way still fails.
    void inlineComponentBaseTypeChange();
    void inlineComponentBaseTypeChangeDropsInvalidBinding();
    void crossCompilationUnitBaseTypeChange();
    void topLevelBaseTypeChangeFails();
    void derivedTypeBaseTypeChangeFails();

    // Changing a composite type's non-composite base (Item -> Rectangle) shifts the VME indices of
    // its own members. When it is used as a composite base of derived instances, recreating those
    // instances must relink the derived types' property caches to the new base layout; otherwise a
    // binding calling the shifted VME method lands at a stale index and asserts in vmeMethod().
    void changedBaseTypeRelinksDerivedInstanceCaches();

    // Reproduces coffee demo crash 3: changing a function body in a derived type
    // (that has states with PropertyChanges targeting form aliases) should not
    // trigger an assert in "canGetTypeFromVariant<T>(this)" during rebuild.
    void derivedTypeFunctionChangeCrash();
    void nestedDerivedIdMethodChange();

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

static QQmlPreview::PatchResult updateObjects(std::vector<QObject *> &objects,
                          const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                          const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    const QQmlPreview::PatchResult result = QQmlPreview::applyDiff(objects, oldUnit, newUnit);
    if (result != QQmlPreview::PatchResult::Failed) {
        QQmlMetaType::deepClearCompositeType(oldUnit->baseCompilationUnit());
        QQmlPreview::refreshBindings(
                oldUnit, result == QQmlPreview::PatchResult::PatchedInPlace ? newUnit : nullptr);
    }
    return result;
}

tst_QQmlPreviewObjectPatch::tst_QQmlPreviewObjectPatch() : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    QV4::ExecutionEngine::setPreviewing(true);
    qmlRegisterType<DeferredIntItem>("Qt.Test.PreviewDeferred", 1, 0, "DeferredIntItem");
    qmlRegisterType<ImmediateHost>("Qt.Test.PreviewDeferred", 1, 0, "ImmediateHost");
    qmlRegisterType<DeferredAttached>("Qt.Test.PreviewDeferred", 1, 0, "DeferredAttached");
}

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

    // A pure constant change is a trivial diff: it must be patched in place, leaving the same
    // QObject and the same VME meta-object untouched (no rebuild).
    const QMetaObject *metaObjectBefore = object->metaObject();

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE(updateObjects(objects, oldExecUnit, newExecUnit),
             QQmlPreview::PatchResult::PatchedInPlace);
    QCOMPARE(object->metaObject(), metaObjectBefore);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);
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

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // runGen() drives the generator: yields value(3), value*2(6), value*3(9) → sum 18.
    QVERIFY(QMetaObject::invokeMethod(object.data(), "runGen", Q_RETURN_ARG(QVariant, result)));
    QCOMPARE(result.metaType(), QMetaType::fromType<int>());
    QCOMPARE(result.toInt(), 18);
}

void tst_QQmlPreviewObjectPatch::updateObjectsGeneratorMethodChange()
{
    // A generator method (function*) whose body changes must be re-homed in place like any other
    // VME method: the trivial patch's refreshVmeMethods() re-creates it with
    // GeneratorFunction::create(). Regular createScriptFunction() would produce a non-generator and
    // break iteration; simply skipping it would leave the stale old generator in the slot.
    QQmlComponent oldComponent(&engine, testFileUrl("GeneratorMethodOld.qml"));
    QVERIFY2(oldComponent.isReady(), qPrintable(oldComponent.errorString()));
    QScopedPointer<QObject> object(oldComponent.create());
    QVERIFY(object);

    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(object.data(), "first", Q_RETURN_ARG(QVariant, result)));
    QCOMPARE(result.toInt(), 10);

    QQmlComponent newComponent(&engine, testFileUrl("GeneratorMethodNew.qml"));
    QVERIFY2(newComponent.isReady(), qPrintable(newComponent.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComponent)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComponent)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE(updateObjects(objects, oldExecUnit, newExecUnit),
             QQmlPreview::PatchResult::Rebuilt);

    QVERIFY(QMetaObject::invokeMethod(object.data(), "first", Q_RETURN_ARG(QVariant, result)));
    QCOMPARE(result.toInt(), 20);
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

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    QCOMPARE(object->objectName(), QStringLiteral("user-modified"));

    // The new CU's script binding was applied correctly.
    QCOMPARE(object->property("value").toInt(), 2); // aux * 2 = 1 * 2 = 2
}

// Renaming the property a binding assigns to (at a stable binding-table index) is reported
// by the positional diff as a single BindingChanged. Patching it in place leaves the live
// expression bound to the old property and never installs it on the new one.
void tst_QQmlPreviewObjectPatch::bindingPropertyRename()
{
    QQmlComponent oldComp(&engine, testFileUrl("BindingPropertyRenameOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    // Old source: width is script-bound to base + 10, height is unset.
    QCOMPARE(object->property("width").toReal(), 60.0);
    QCOMPARE(object->property("height").toReal(), 0.0);

    QQmlComponent newComp(&engine, testFileUrl("BindingPropertyRenameNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // New source: the binding moved from width to height. height must now be 60 and width must
    // no longer be bound.
    QCOMPARE(object->property("height").toReal(), 60.0);
    QCOMPARE(object->property("width").toReal(), 0.0);
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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    QCOMPARE(object->property("value").toInt(), 42);

    // Now change __mult to 8.  If the binding is live, value updates to 48.
    // If only the value was stashed (binding lost), value stays at 42.
    engine.rootContext()->setContextProperty("__mult", 8);

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
    QCOMPARE_NE(updateObjects(enumObjects, oldEnumCU, newEnumCU), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    QFont newFont = object->property("font").value<QFont>();
    QCOMPARE(newFont.pixelSize(), 24);
}

// Like granularConstantUpdatePreservesUserOverride, but the user override is on a
// *value-type sub-property* (font.pixelSize) rather than a top-level property. The change is a
// trivial diff, so it is patched in place: the guarded write sees the property no longer holds the
// old default and leaves the override untouched.
void tst_QQmlPreviewObjectPatch::groupPropertyValueTypeOverridePreserved()
{
    QQmlComponent oldComp(&engine, testFileUrl("GroupPropertyFontChangeOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);
    QCOMPARE(object->property("font").value<QFont>().pixelSize(), 12);

    // The user manually changed font.pixelSize after the component loaded.
    QQmlProperty pixelSize(object.data(), "font.pixelSize");
    QVERIFY(pixelSize.isValid());
    QVERIFY(pixelSize.write(99));
    QCOMPARE(object->property("font").value<QFont>().pixelSize(), 99);

    QQmlComponent newComp(&engine, testFileUrl("GroupPropertyFontChangeNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // The user-overridden sub-property (99 != old default 12) survives the update, just like a
    // top-level property override does.
    QCOMPARE(object->property("font").value<QFont>().pixelSize(), 99);
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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);
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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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

    // Apply — this used to assert in QQmlPropertyPrivate::setBinding()
    // because the signal handler binding had a null targetObject.
    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    const size_t objectCountBefore = objects.size();
    QVERIFY(objectCountBefore > 0);

    // Apply the in-place update.
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // 1. Property was updated.
    QCOMPARE(object->property("count").toInt(), 55);

    // 2. No duplicate QML-created children. Lazily-created C++ objects (like
    //    QQuickAnchors) may appear as additional children — that's expected.
    QVERIFY(object->children().size() >= childCountBefore);

    // 3. All live objects now reference the new compilation unit.
    auto liveObjects = objectsForCompilationUnit(&engine, newExecUnit);
    QVERIFY(liveObjects.size() >= objectCountBefore);
    for (QObject *obj : liveObjects) {
        QQmlData *ddata = QQmlData::get(obj);
        QVERIFY(ddata);
        QCOMPARE(ddata->compilationUnit, newExecUnit);
    }

    // 4. No signal handler function should still reference the old CU.
    bool foundSignalHandler = false;
    for (QObject *obj : liveObjects) {
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

    // Find the "target" child Rectangle by type (skip lazily-created C++ objects).
    auto findTarget = [&]() -> QObject * {
        for (QObject *child : object->children()) {
            if (child->inherits("QQuickRectangle"))
                return child;
        }
        return nullptr;
    };

    QObject *target = findTarget();
    QVERIFY(target);
    QCOMPARE(target->property("y").toReal(), 0.0); // anchors.top: parent.top

    QQmlComponent newComp(&engine, testFileUrl("AnchorsTopTargetNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);
    target = findTarget();
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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // After the patch, y should consistently be 100 (parent.verticalCenter
    // with parent height 200). No y=0 should appear.
    QTRY_VERIFY2(object->property("log").toString().contains(QLatin1String("y=100y=100y=100")),
                 qPrintable(QLatin1String("Expected y=100 not found!")));
    QVERIFY2(!object->property("log").toString().contains(QLatin1String("y=0")),
             qPrintable(QLatin1String("Alternation detected!")));
}

void tst_QQmlPreviewObjectPatch::rebuildDoesNotLeakChildren()
{
    QQmlComponent oldComp(&engine, testFileUrl("AnchorsTopTimerOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);
    QVERIFY(object->children().size() > 0);

    QQmlComponent newComp(&engine, testFileUrl("AnchorsTopTimerNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // Process deferred deletes so any properly retired objects are cleaned up.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // No QObject child should still reference the old compilation unit.
    // Such children would be leaked orphans from individually-rebuilt objects
    // that were subsequently replaced by the parent's repopulateBindings.
    int leakedChildren = 0;
    for (QObject *child : object->children()) {
        QQmlData *ddata = QQmlData::get(child);
        if (ddata && ddata->compilationUnit == oldExecUnit)
            ++leakedChildren;
    }
    QCOMPARE(leakedChildren, 0);
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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

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

// Consecutive in-place updates: the first update invalidates the outer context
// of the root object. Without the outerContext->isValid() guard the second
// update tries to use that dead context and crashes.
void tst_QQmlPreviewObjectPatch::consecutiveUpdatesDeadContext()
{
    QQmlComponent v1Comp(&engine, testFileUrl("DeadContextV1.qml"));
    QVERIFY2(v1Comp.isReady(), qPrintable(v1Comp.errorString()));
    QScopedPointer<QObject> object(v1Comp.create());
    QVERIFY(object);
    QCOMPARE(object->property("counter").toInt(), 1);

    QQmlComponent v2Comp(&engine, testFileUrl("DeadContextV2.qml"));
    QVERIFY2(v2Comp.isReady(), qPrintable(v2Comp.errorString()));

    const auto v1Unit = QQmlComponentPrivate::get(&v1Comp)->compilationUnit();
    const auto v2Unit = QQmlComponentPrivate::get(&v2Comp)->compilationUnit();
    QVERIFY(v1Unit && v2Unit);

    // First update: v1 -> v2 (context is valid, succeeds normally).
    auto objects = objectsForCompilationUnit(&engine, v1Unit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, v1Unit, v2Unit), QQmlPreview::PatchResult::Failed);
    QCOMPARE(object->property("counter").toInt(), 2);

    // Simulate a cascading context invalidation between two rapid updates.
    // This happens in practice when a parent context is destroyed while a
    // child object still references it.
    QQmlData *ddata = QQmlData::get(object.data());
    QVERIFY(ddata && ddata->outerContext);
    QQmlRefPointer<QQmlContextData> outerCtx(ddata->outerContext);
    outerCtx->invalidate();

    // Second update: v2 -> v3.  Without the isValid() guard in rebuildObject(),
    // this would crash by trying to create objects into the invalidated context.
    QQmlComponent v3Comp(&engine, testFileUrl("DeadContextV3.qml"));
    QVERIFY2(v3Comp.isReady(), qPrintable(v3Comp.errorString()));

    const auto v3Unit = QQmlComponentPrivate::get(&v3Comp)->compilationUnit();
    QVERIFY(v3Unit);

    objects = objectsForCompilationUnit(&engine, v2Unit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, v2Unit, v3Unit), QQmlPreview::PatchResult::Failed);

    // The object's outer context was invalidated, so rebuildObject skipped it.
    // Counter stays at 2 (not updated to 3).
    QCOMPARE(object->property("counter").toInt(), 2);
}

// Reproduces the coffee demo bug where the "Out of" label stays visible after
// a script binding → constant → script binding roundtrip during in-place preview.
//
// Scenario:
//   1. CoffeeCardForm.ui.qml has:  visible: (root.cupsLeft != 0) ? false : true
//   2. User edits to:              visible: true   (constant)
//   3. User edits back to:         visible: (root.cupsLeft != 0) ? false : true
//
// After step 3, the script binding must be active again: when cupsLeft > 0,
// visible should be false. The bug causes the binding NOT to be re-created,
// so visible stays true regardless of cupsLeft.
//
// This test reproduces the full roundtrip: load the original (script binding),
// patch to the intermediate (constant), then patch back (script binding).
// Uses a composite type hierarchy (Form + Derived + Wrapper) to faithfully
// reproduce the coffee demo structure where CoffeeCardForm.ui.qml is patched
// while instances live inside ChoosingCoffeeForm → CoffeeCard → CoffeeCardForm.
void tst_QQmlPreviewObjectPatch::scriptToConstantToScriptRoundtrip()
{
    // --- Step 1: Load the outer wrapper which instantiates the composite type ---
    // This establishes: OuterWrapper → Wrapper → Derived → FormOld
    // cupsLeft is set from OuterWrapper (like ChoosingCoffee.qml sets it from outside).
    QQmlComponent wrapperComp(&engine, testFileUrl("ScriptToConstantToScriptOuterWrapper.qml"));
    QVERIFY2(wrapperComp.isReady(), qPrintable(wrapperComp.errorString()));
    std::unique_ptr<QObject> wrapper(wrapperComp.create());
    QVERIFY(wrapper);

    // Find the derived/card instance via the wrapper's card alias.
    QObject *card = wrapper->property("card").value<QObject *>();
    QVERIFY(card);
    QCOMPARE(card->property("cupsLeft").toInt(), 5);

    // Get the outOfDialog Rectangle via the alias.
    QObject *outOfDialog = card->property("outOfDialog").value<QObject *>();
    QVERIFY(outOfDialog);

    // cupsLeft = 5, so (cupsLeft != 0) ? false : true → false → Rectangle hidden
    QCOMPARE(outOfDialog->property("visible").toBool(), false);

    // Verify the binding is reactive
    card->setProperty("cupsLeft", 0);
    QCOMPARE(outOfDialog->property("visible").toBool(), true);
    card->setProperty("cupsLeft", 5);
    QCOMPARE(outOfDialog->property("visible").toBool(), false);

    // --- Step 2: Patch the form CU (script binding → constant true) ---
    // Load old and mid form components to get their CUs for diffing.
    QQmlComponent oldFormComp(&engine,
                              testFileUrl("ScriptToConstantToScriptFormOld.qml"));
    QVERIFY2(oldFormComp.isReady(), qPrintable(oldFormComp.errorString()));
    QQmlComponent midFormComp(&engine,
                              testFileUrl("ScriptToConstantToScriptFormMid.qml"));
    QVERIFY2(midFormComp.isReady(), qPrintable(midFormComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldFormComp)->compilationUnit();
    const auto midExecUnit = QQmlComponentPrivate::get(&midFormComp)->compilationUnit();
    QVERIFY(oldExecUnit && midExecUnit);

    // Find objects belonging to the form's CU (the form-level objects inside `card`).
    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, midExecUnit), QQmlPreview::PatchResult::Failed);

    // After patching to constant true, the Rectangle should be visible.
    // Re-fetch outOfDialog since it might have been recreated.
    outOfDialog = card->property("outOfDialog").value<QObject *>();
    QVERIFY(outOfDialog);
    QCOMPARE(outOfDialog->property("visible").toBool(), true);

    // Process events between patches (mimics real scenario where there's time between edits)
    QCoreApplication::processEvents();

    // --- Step 3: Patch the form CU again (constant true → script binding) ---
    QQmlComponent newFormComp(&engine,
                              testFileUrl("ScriptToConstantToScriptFormNew.qml"));
    QVERIFY2(newFormComp.isReady(), qPrintable(newFormComp.errorString()));

    const auto newExecUnit = QQmlComponentPrivate::get(&newFormComp)->compilationUnit();
    QVERIFY(newExecUnit);

    objects = objectsForCompilationUnit(&engine, midExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, midExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // After restoring the script binding, cupsLeft = 5 → visible should be false.
    // BUG: the external cupsLeft binding is lost during rebuild, cupsLeft falls back
    // to 0 (form default), and the binding (root.cupsLeft != 0) ? false : true
    // evaluates to true — the "Out of" label stays visible.
    outOfDialog = card->property("outOfDialog").value<QObject *>();
    QVERIFY(outOfDialog);
    QCOMPARE(outOfDialog->property("visible").toBool(), false);

    // Verify the binding is truly reactive again
    card->setProperty("cupsLeft", 0);
    QCOMPARE(outOfDialog->property("visible").toBool(), true);
    card->setProperty("cupsLeft", 3);
    QCOMPARE(outOfDialog->property("visible").toBool(), false);
}

void tst_QQmlPreviewObjectPatch::scriptToConstantToScriptRoundtripExternalScript()
{
    // Same as scriptToConstantToScriptRoundtrip but the outer wrapper uses a
    // script expression (2 + 3) rather than a literal constant (5).
    QQmlComponent wrapperComp(&engine, testFileUrl("ScriptToConstantToScriptOuterWrapperScript.qml"));
    QVERIFY2(wrapperComp.isReady(), qPrintable(wrapperComp.errorString()));
    std::unique_ptr<QObject> wrapper(wrapperComp.create());
    QVERIFY(wrapper);

    QObject *card = wrapper->property("card").value<QObject *>();
    QVERIFY(card);
    QCOMPARE(card->property("cupsLeft").toInt(), 5);

    QObject *outOfDialog = card->property("outOfDialog").value<QObject *>();
    QVERIFY(outOfDialog);
    QCOMPARE(outOfDialog->property("visible").toBool(), false);

    // Verify the binding is reactive
    card->setProperty("cupsLeft", 0);
    QCOMPARE(outOfDialog->property("visible").toBool(), true);
    card->setProperty("cupsLeft", 5);
    QCOMPARE(outOfDialog->property("visible").toBool(), false);

    // --- Step 2: Patch the form CU (script binding → constant true) ---
    QQmlComponent oldFormComp(&engine,
                              testFileUrl("ScriptToConstantToScriptFormOld.qml"));
    QVERIFY2(oldFormComp.isReady(), qPrintable(oldFormComp.errorString()));
    QQmlComponent midFormComp(&engine,
                              testFileUrl("ScriptToConstantToScriptFormMid.qml"));
    QVERIFY2(midFormComp.isReady(), qPrintable(midFormComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldFormComp)->compilationUnit();
    const auto midExecUnit = QQmlComponentPrivate::get(&midFormComp)->compilationUnit();
    QVERIFY(oldExecUnit && midExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, midExecUnit), QQmlPreview::PatchResult::Failed);

    outOfDialog = card->property("outOfDialog").value<QObject *>();
    QVERIFY(outOfDialog);
    QCOMPARE(outOfDialog->property("visible").toBool(), true);

    QCoreApplication::processEvents();

    // --- Step 3: Patch the form CU again (constant true → script binding) ---
    QQmlComponent newFormComp(&engine,
                              testFileUrl("ScriptToConstantToScriptFormNew.qml"));
    QVERIFY2(newFormComp.isReady(), qPrintable(newFormComp.errorString()));

    const auto newExecUnit = QQmlComponentPrivate::get(&newFormComp)->compilationUnit();
    QVERIFY(newExecUnit);

    objects = objectsForCompilationUnit(&engine, midExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, midExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // After restoring the script binding, the external script expression
    // (2 + 3 = 5) must still be in effect, so visible should be false.
    outOfDialog = card->property("outOfDialog").value<QObject *>();
    QVERIFY(outOfDialog);
    QCOMPARE(outOfDialog->property("visible").toBool(), false);

    // Verify the binding is truly reactive again
    card->setProperty("cupsLeft", 0);
    QCOMPARE(outOfDialog->property("visible").toBool(), true);
    card->setProperty("cupsLeft", 3);
    QCOMPARE(outOfDialog->property("visible").toBool(), false);
}

void tst_QQmlPreviewObjectPatch::externalSignalHandlerLostOnRebuild()
{
    QQmlEngine localEngine;

    QQmlComponent targetComp(&localEngine, testFileUrl("ExternalSignalHandlerOld.qml"));
    QVERIFY2(targetComp.isReady(), qPrintable(targetComp.errorString()));
    std::unique_ptr<QObject> target(targetComp.create());
    QVERIFY(target);
    QCOMPARE(target->property("counter").toInt(), 0);

    // Connect an external signal handler from outside the target object.
    // This simulates a parent/sibling object attaching a handler to the
    // target's signal — the connecting object is not rebuilt.
    // We use the counter property and a Connections-style approach: install an
    // external binding that increments counter when fired() is emitted.
    QQmlComponent observerComp(&localEngine);
    observerComp.setData(
            "import QtQml\n"
            "QtObject {\n"
            "    required property QtObject signalTarget\n"
            "    property int callCount: 0\n"
            "    property var conn1: Connections {\n"
            "        target: signalTarget\n"
            "        function onFired() { callCount++ }\n"
            "    }\n"
            "    property var conn2: Connections {\n"
            "        target: signalTarget\n"
            "        function onFired() { callCount++ }\n"
            "    }\n"
            "}\n",
            QUrl("file:///test_external_signal_observer.qml"));
    QVERIFY2(observerComp.isReady(), qPrintable(observerComp.errorString()));
    std::unique_ptr<QObject> observer(observerComp.createWithInitialProperties(
            {{"signalTarget", QVariant::fromValue(target.get())}}));
    QVERIFY(observer);

    // Sanity: the connection works before rebuild.
    QMetaObject::invokeMethod(target.get(), "fired");
    QCOMPARE(observer->property("callCount").toInt(), 2);

    // Rebuild the target object (simulate hot-reload patching its CU).
    QQmlComponent newComp(&localEngine, testFileUrl("ExternalSignalHandlerNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&targetComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&localEngine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // Fire the signal again after rebuild.
    QMetaObject::invokeMethod(target.get(), "fired");

    // The external signal handler must still be called after rebuild.
    QCOMPARE(observer->property("callCount").toInt(), 4);
}

// Like externalSignalHandlerLostOnRebuild, but the new version of the target
// removes the fired() signal entirely. The stashed external handler therefore
// cannot be reattached: restoreExternalState() looks up the old signature on the
// new metaobject and gets signalIndex < 0. This exercises the failure branch of
// the stashed-handler restore, which the other external-handler tests never hit
// because they keep the signal alive across the rebuild.
void tst_QQmlPreviewObjectPatch::externalSignalHandlerSignalRemoved()
{
    QQmlEngine localEngine;

    QQmlComponent targetComp(&localEngine, testFileUrl("SignalRemovedOld.qml"));
    QVERIFY2(targetComp.isReady(), qPrintable(targetComp.errorString()));
    std::unique_ptr<QObject> target(targetComp.create());
    QVERIFY(target);

    QQmlComponent observerComp(&localEngine);
    observerComp.setData(
            "import QtQml\n"
            "QtObject {\n"
            "    property QtObject signalTarget\n"
            "    property int callCount: 0\n"
            "    property var conn: Connections {\n"
            "        target: signalTarget\n"
            "        function onFired() { callCount++ }\n"
            "    }\n"
            "}\n",
            QUrl("file:///test_external_signal_removed_observer.qml"));
    QVERIFY2(observerComp.isReady(), qPrintable(observerComp.errorString()));
    std::unique_ptr<QObject> observer(observerComp.createWithInitialProperties(
            { { QStringLiteral("signalTarget"), QVariant::fromValue(target.get()) } }));
    QVERIFY(observer);

    // Sanity: the connection works before rebuild.
    QMetaObject::invokeMethod(target.get(), "fired");
    QCOMPARE(observer->property("callCount").toInt(), 1);

    QQmlComponent newComp(&localEngine, testFileUrl("SignalRemovedNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&targetComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&localEngine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // The signal is gone, so the handler is genuinely unrecoverable. The point of
    // this test is that tearing down the engine and observer afterwards must not
    // leak the stashed handler's expression or leave a dangling entry in the
    // owner's signalHandlers list (caught by ASAN/LSan).
    QVERIFY(target->metaObject()->indexOfSignal("fired()") < 0);
}

void tst_QQmlPreviewObjectPatch::externalSignalHandlerGroupedPropertySyntax()
{
    // Load a wrapper component that uses an inline signal handler on a child
    // component instance (like ChoosingCoffee.qml does with "cappuccino.button.onClicked:").
    QQmlEngine localEngine;
    localEngine.addImportPath(dataDirectory());
    QQmlComponent wrapperComp(&localEngine, testFileUrl("GroupedSignalHandlerWrapper.qml"));
    QVERIFY2(wrapperComp.isReady(), qPrintable(wrapperComp.errorString()));
    std::unique_ptr<QObject> wrapper(wrapperComp.create());
    QVERIFY(wrapper);

    QObject *target = wrapper->property("target").value<QObject *>();
    QVERIFY(target);

    // Sanity: the inline onFired: handler works.
    QVERIFY(QMetaObject::invokeMethod(target, "fired"));
    QCOMPARE(wrapper->property("callCount").toInt(), 2);

    // Get the type-level CU for SignalTargetOld (the target's own type definition).
    // The target's ddata->compilationUnit points to the wrapper's CU (which instantiates it),
    // so we load the type separately — the type loader cache ensures we get the same CU.
    QQmlComponent oldTargetComp(&localEngine, testFileUrl("SignalTargetOld.qml"));
    QVERIFY2(oldTargetComp.isReady(), qPrintable(oldTargetComp.errorString()));
    const auto oldExecUnit = QQmlComponentPrivate::get(&oldTargetComp)->compilationUnit();
    QVERIFY(oldExecUnit);

    QQmlComponent newComp(&localEngine, testFileUrl("SignalTargetNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(newExecUnit);

    // Only rebuild the target object (in a real scenario, only its type's file changed).
    std::vector<QObject *> objects = { target };
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // The wrapper's state must not have been affected by the target rebuild.
    QCOMPARE(wrapper->property("callCount").toInt(), 2);

    // Fire the signal again after rebuild.
    QVERIFY(QMetaObject::invokeMethod(target, "fired"));

    // The external signal handler from the wrapper must still fire.
    QCOMPARE(wrapper->property("callCount").toInt(), 4);
}

void tst_QQmlPreviewObjectPatch::externalSignalHandlerJsConnect()
{
    QQmlEngine localEngine;

    QQmlComponent targetComp(&localEngine, testFileUrl("ExternalSignalHandlerGroupedOld.qml"));
    QVERIFY2(targetComp.isReady(), qPrintable(targetComp.errorString()));
    std::unique_ptr<QObject> target(targetComp.create());
    QVERIFY(target);

    // Connect an external signal handler using a direct JavaScript signal.connect() call.
    // This simulates the pattern where a parent directly connects to a child's signal
    // via grouped property syntax (e.g. "target.onFired: ..." in ChoosingCoffee.qml).
    QQmlComponent observerComp(&localEngine);
    observerComp.setData(
            "import QtQuick\n"
            "Item {\n"
            "    required property QtObject targetObj\n"
            "    property int callCount: 0\n"
            "    Component.onCompleted: {\n"
            "        targetObj.fired.connect(function() { callCount++ })\n"
            "        targetObj.fired.connect(function() { callCount++ })\n"
            "    }\n"
            "}\n",
            QUrl("file:///test_external_signal_grouped_observer.qml"));
    QVERIFY2(observerComp.isReady(), qPrintable(observerComp.errorString()));
    std::unique_ptr<QObject> observer(observerComp.createWithInitialProperties(
            {{"targetObj", QVariant::fromValue(target.get())}}));
    QVERIFY(observer);

    // Sanity: the connection works before rebuild.
    QMetaObject::invokeMethod(target.get(), "fired");
    QCOMPARE(observer->property("callCount").toInt(), 2);

    // Rebuild the target object (simulate hot-reload patching its CU).
    QQmlComponent newComp(&localEngine, testFileUrl("ExternalSignalHandlerGroupedNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&targetComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&localEngine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // Fire the signal again after rebuild.
    QMetaObject::invokeMethod(target.get(), "fired");

    // Both external signal handlers must still fire after rebuild.
    QCOMPARE(observer->property("callCount").toInt(), 4);
}

void tst_QQmlPreviewObjectPatch::externalSignalHandlerOnSubObject()
{
    // Reproduces the coffee demo bug: ChoosingCoffee.qml attaches a signal
    // handler to "cappuccino.button.onClicked:" — the handler lives on a
    // sub-object (button) inside CoffeeCardForm.ui.qml, accessed via alias.
    // When CoffeeCardForm.ui.qml is modified and rebuilt, the button child
    // object gets replaced, and the handler attached from outside is lost.

    QQmlEngine localEngine;
    localEngine.addImportPath(dataDirectory());
    QQmlComponent wrapperComp(&localEngine, testFileUrl("SubObjectSignalWrapper.qml"));
    QVERIFY2(wrapperComp.isReady(), qPrintable(wrapperComp.errorString()));
    std::unique_ptr<QObject> wrapper(wrapperComp.create());
    QVERIFY(wrapper);

    QObject *target = wrapper->property("target").value<QObject *>();
    QVERIFY(target);
    QObject *button = target->property("button").value<QObject *>();
    QVERIFY(button);

    // Sanity: the grouped-property signal handler works before rebuild.
    QVERIFY(QMetaObject::invokeMethod(button, "triggered"));
    QCOMPARE(wrapper->property("callCount").toInt(), 1);

    // Get the form's type-level compilation unit.
    QQmlComponent oldFormComp(&localEngine, testFileUrl("SubObjectSignalFormOld.qml"));
    QVERIFY2(oldFormComp.isReady(), qPrintable(oldFormComp.errorString()));
    const auto oldExecUnit = QQmlComponentPrivate::get(&oldFormComp)->compilationUnit();
    QVERIFY(oldExecUnit);

    QQmlComponent newFormComp(&localEngine, testFileUrl("SubObjectSignalFormNew.qml"));
    QVERIFY2(newFormComp.isReady(), qPrintable(newFormComp.errorString()));
    const auto newExecUnit = QQmlComponentPrivate::get(&newFormComp)->compilationUnit();
    QVERIFY(newExecUnit);

    // Rebuild only the form's objects (simulates editing CoffeeCardForm.ui.qml).
    auto objects = objectsForCompilationUnit(&localEngine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // Re-fetch the button alias — it may point to a new object after rebuild.
    button = target->property("button").value<QObject *>();
    QVERIFY(button);

    // Fire the signal on the (possibly new) button after rebuild.
    QVERIFY(QMetaObject::invokeMethod(button, "triggered"));

    // The external signal handler from the wrapper must still fire.
    QCOMPARE(wrapper->property("callCount").toInt(), 2);
}

void tst_QQmlPreviewObjectPatch::externalBindingOnSubObjectTargetMismatch()
{
    // Reproduces the assertion failure from Gerrit change 741247 review:
    // An external script binding is set on target.button.interval from the
    // wrapper CU. When the form CU is rebuilt, the Timer child gets replaced.
    // refreshObjects() updates the child BindingPatchContext's m_object, but
    // the stashed binding's targetObject() still points to the old Timer.
    // restoreExternalState() calls installOn() which asserts:
    //   abstractBinding->targetObject() == target.object()

    QQmlEngine localEngine;
    localEngine.addImportPath(dataDirectory());
    QQmlComponent wrapperComp(&localEngine, testFileUrl("SubObjectBindingWrapper.qml"));
    QVERIFY2(wrapperComp.isReady(), qPrintable(wrapperComp.errorString()));
    std::unique_ptr<QObject> wrapper(wrapperComp.create());
    QVERIFY(wrapper);

    QObject *target = wrapper->property("target").value<QObject *>();
    QVERIFY(target);
    QObject *button = target->property("button").value<QObject *>();
    QVERIFY(button);

    // Sanity: the external binding sets interval = multiplier * 200 = 5 * 200 = 1000.
    QCOMPARE(button->property("interval").toInt(), 1000);

    // Change multiplier to verify the binding is live.
    wrapper->setProperty("multiplier", 3);
    QCOMPARE(button->property("interval").toInt(), 600);

    // Get the form's type-level compilation unit.
    QQmlComponent oldFormComp(&localEngine, testFileUrl("SubObjectBindingFormOld.qml"));
    QVERIFY2(oldFormComp.isReady(), qPrintable(oldFormComp.errorString()));
    const auto oldExecUnit = QQmlComponentPrivate::get(&oldFormComp)->compilationUnit();
    QVERIFY(oldExecUnit);

    QQmlComponent newFormComp(&localEngine, testFileUrl("SubObjectBindingFormNew.qml"));
    QVERIFY2(newFormComp.isReady(), qPrintable(newFormComp.errorString()));
    const auto newExecUnit = QQmlComponentPrivate::get(&newFormComp)->compilationUnit();
    QVERIFY(newExecUnit);

    // Rebuild only the form's objects (simulates editing SubObjectBindingFormOld.qml).
    // This is where the assertion fires without the fix: restoreExternalState()
    // tries to installOn() a binding whose targetObject is the old Timer.
    auto objects = objectsForCompilationUnit(&localEngine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // Re-fetch the button — it should be a new object after rebuild.
    button = target->property("button").value<QObject *>();
    QVERIFY(button);

    // The external binding must still be functional on the new Timer object.
    // multiplier is still 3, so interval should be 600.
    QCOMPARE(button->property("interval").toInt(), 600);

    // Verify the binding is still live by changing multiplier again.
    wrapper->setProperty("multiplier", 7);
    QCOMPARE(button->property("interval").toInt(), 1400);
}

void tst_QQmlPreviewObjectPatch::externalSignalHandlerOnListChild()
{
    // Like externalSignalHandlerOnSubObject, but the child Timer is accessible
    // only via a list property (property list<Timer> timers), not a single
    // QObject* alias. The signal handler is connected via signal.connect() in
    // the wrapper's Component.onCompleted.

    QQmlEngine localEngine;
    localEngine.addImportPath(dataDirectory());
    QQmlComponent wrapperComp(&localEngine, testFileUrl("ListChildWrapper.qml"));
    QVERIFY2(wrapperComp.isReady(), qPrintable(wrapperComp.errorString()));
    std::unique_ptr<QObject> wrapper(wrapperComp.create());
    QVERIFY(wrapper);

    QObject *target = wrapper->property("target").value<QObject *>();
    QVERIFY(target);

    // Read the list property to get the second timer.
    QQmlListReference timersList(target, "timers");
    QVERIFY(timersList.isValid());
    QVERIFY(timersList.count() >= 2);
    QObject *timer = timersList.at(1);
    QVERIFY(timer);

    // Sanity: the signal handler connected via signal.connect() works.
    QVERIFY(QMetaObject::invokeMethod(timer, "triggered"));
    QCOMPARE(wrapper->property("callCount").toInt(), 1);

    // Get the form's type-level compilation unit.
    QQmlComponent oldFormComp(&localEngine, testFileUrl("ListChildFormOld.qml"));
    QVERIFY2(oldFormComp.isReady(), qPrintable(oldFormComp.errorString()));
    const auto oldExecUnit = QQmlComponentPrivate::get(&oldFormComp)->compilationUnit();
    QVERIFY(oldExecUnit);

    QQmlComponent newFormComp(&localEngine, testFileUrl("ListChildFormNew.qml"));
    QVERIFY2(newFormComp.isReady(), qPrintable(newFormComp.errorString()));
    const auto newExecUnit = QQmlComponentPrivate::get(&newFormComp)->compilationUnit();
    QVERIFY(newExecUnit);

    // Rebuild only the form's objects.
    auto objects = objectsForCompilationUnit(&localEngine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // Re-fetch the timer from the list — it may be a new object after rebuild.
    QQmlListReference newTimersList(target, "timers");
    QVERIFY(newTimersList.isValid());
    QVERIFY(newTimersList.count() >= 2);
    timer = newTimersList.at(1);
    QVERIFY(timer);

    // Fire the signal on the (possibly new) timer after rebuild.
    QVERIFY(QMetaObject::invokeMethod(timer, "triggered"));

    // The external signal handler from the wrapper must still fire. The form change is a trivial
    // constant edit, so the objects are patched in place rather than rebuilt; the list child and
    // its externally-connected signal handler are left untouched and keep working.
    QCOMPARE(wrapper->property("callCount").toInt(), 2);
}

void tst_QQmlPreviewObjectPatch::externalSignalHandlerOnSubObjectUnfired()
{
    // Reproduces the coffee demo bug where the button was never clicked before
    // the file edit. The signal handler's QQmlNotifierEndpoint remains in the
    // NotifyList's 'todo' queue because layout() was never triggered for the
    // 'triggered' signal index. The stash code must call layout() to find it.

    QQmlEngine localEngine;
    localEngine.addImportPath(dataDirectory());
    QQmlComponent wrapperComp(&localEngine, testFileUrl("SubObjectSignalUnfiredWrapper.qml"));
    QVERIFY2(wrapperComp.isReady(), qPrintable(wrapperComp.errorString()));
    std::unique_ptr<QObject> wrapper(wrapperComp.create());
    QVERIFY(wrapper);

    QObject *target = wrapper->property("target").value<QObject *>();
    QVERIFY(target);
    QObject *button = target->property("button").value<QObject *>();
    QVERIFY(button);

    // Do NOT fire the signal before rebuild (this is the key difference from
    // externalSignalHandlerOnSubObject). The handler stays in the todo queue.
    QCOMPARE(wrapper->property("callCount").toInt(), 0);

    // Rebuild the form CU (simulates editing a text label in the form).
    QQmlComponent oldFormComp(&localEngine, testFileUrl("SubObjectSignalUnfiredFormOld.qml"));
    QVERIFY2(oldFormComp.isReady(), qPrintable(oldFormComp.errorString()));
    const auto oldExecUnit = QQmlComponentPrivate::get(&oldFormComp)->compilationUnit();
    QVERIFY(oldExecUnit);

    QQmlComponent newFormComp(&localEngine, testFileUrl("SubObjectSignalUnfiredFormNew.qml"));
    QVERIFY2(newFormComp.isReady(), qPrintable(newFormComp.errorString()));
    const auto newExecUnit = QQmlComponentPrivate::get(&newFormComp)->compilationUnit();
    QVERIFY(newExecUnit);

    auto objects = objectsForCompilationUnit(&localEngine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // Re-fetch button (it was replaced during rebuild).
    button = target->property("button").value<QObject *>();
    QVERIFY(button);

    // Fire the signal on the new button AFTER rebuild.
    QVERIFY(QMetaObject::invokeMethod(button, "triggered"));

    // The external signal handler must survive the rebuild.
    QCOMPARE(wrapper->property("callCount").toInt(), 1);
}

// Verifies that after rebuilding a composite type's CU, old visual children
// are properly removed from the scene. Without the fix for QMetaProperty::write
// (using the property's own metatype instead of QObject*), old children remained
// as visual children because setParentItem(nullptr) was never called.
void tst_QQmlPreviewObjectPatch::compositeRebuildNoVisualChildDuplication()
{
    QQmlComponent oldComp(&engine, testFileUrl("MultipleCompositeInstancesOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    // Get the root item and find the composite children.
    auto *rootItem = qobject_cast<QQuickItem *>(object.get());
    QVERIFY(rootItem);

    // Find composite instances (CompositeBaseWithAliases) by checking for "header" property.
    QList<QQuickItem *> composites;
    for (QQuickItem *child : rootItem->childItems()) {
        if (child->property("header").isValid())
            composites.append(child);
    }
    QCOMPARE(composites.size(), 2);

    // Each composite has 2 visual children (headerText + contentArea).
    for (QQuickItem *composite : composites)
        QCOMPARE(composite->childItems().size(), 2);

    // Now rebuild: apply the "new" version of the CU (same structure, different marker value).
    QQmlComponent newComp(&engine, testFileUrl("MultipleCompositeInstancesNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // After rebuild, the marker should be updated.
    QCOMPARE(object->property("marker").toInt(), 2);

    // Re-find the composite instances (they should still be visual children of root).
    composites.clear();
    for (QQuickItem *child : rootItem->childItems()) {
        if (child->property("header").isValid())
            composites.append(child);
    }
    QCOMPARE(composites.size(), 2);

    // CRITICAL: Each composite must STILL have exactly 2 visual children,
    // not 4 (which would indicate old children were not removed).
    for (QQuickItem *composite : composites)
        QCOMPARE(composite->childItems().size(), 2);
}

void tst_QQmlPreviewObjectPatch::singletonConstantPropertyChange()
{
    QQmlEngine engine;

    const QString moduleDir = dataDirectory() + QStringLiteral("/SingletonModule");
    const QString patchedDir = dataDirectory() + QStringLiteral("/SingletonModulePatched");
    engine.addImportPath(dataDirectory());

    // Instantiate a consumer so the singleton is actually created.
    QQmlComponent consumerComp(&engine, QUrl::fromLocalFile(moduleDir + "/Consumer.qml"));
    QVERIFY2(consumerComp.isReady(), qPrintable(consumerComp.errorString()));
    QScopedPointer<QObject> consumer(consumerComp.create());
    QVERIFY(consumer);

    // Verify initial state.
    QCOMPARE(consumer->property("currentBackground").value<QColor>(), QColor("#121212"));
    QCOMPARE(consumer->property("currentTextColor").value<QColor>(), QColor("#FEFEFE"));

    // Get the old and new singleton CUs.
    QQmlComponent oldComp(&engine, QUrl::fromLocalFile(moduleDir + "/Colors.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();

    QQmlComponent newComp(&engine, QUrl::fromLocalFile(patchedDir + "/Colors.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    // Discover and patch singleton objects.
    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    QCOMPARE(consumer->property("currentBackground").value<QColor>(), QColor("#ffffff"));
    QCOMPARE(consumer->property("currentTextColor").value<QColor>(), QColor("#121111"));
}

void tst_QQmlPreviewObjectPatch::singletonBindingPropagation()
{
    QQmlEngine engine;
    QQmlComponent oldComp(&engine, testFileUrl("SingletonScriptBindingOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    const QColor oldBg("#121212");
    QCOMPARE(object->property("background").value<QColor>(), oldBg);
    const QColor oldDerived = object->property("derivedColor").value<QColor>();
    QVERIFY(oldDerived.isValid());

    QQmlComponent newComp(&engine, testFileUrl("SingletonScriptBindingNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();

    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    if (objects.empty())
        objects.push_back(object.data());

    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // background changes to #ffffff.
    QCOMPARE(object->property("background").value<QColor>(), QColor("#ffffff"));

    // derivedColor uses a script binding: Qt.lighter(background, 1.5).
    // The binding must re-evaluate with the new background value.
    const QColor newDerived = object->property("derivedColor").value<QColor>();
    QVERIFY(newDerived != oldDerived);
}

void tst_QQmlPreviewObjectPatch::singletonChildObjectPropertyChange()
{
    QQmlEngine engine;
    QQmlComponent oldComp(&engine, testFileUrl("SingletonInlineCompOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    // Access the currentTheme alias — it should point to darkTheme.
    QObject *currentTheme = qvariant_cast<QObject *>(object->property("currentTheme"));
    QVERIFY(currentTheme);
    QCOMPARE(currentTheme->property("background").value<QColor>(), QColor("#121212"));
    QCOMPARE(currentTheme->property("textColor").value<QColor>(), QColor("#FEFEFE"));

    QQmlComponent newComp(&engine, testFileUrl("SingletonInlineCompNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    if (objects.empty())
        objects.push_back(object.data());

    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // After hot-reload, the inline component child's constants should update.
    // The darkTheme instance should have its new values.
    currentTheme = qvariant_cast<QObject *>(object->property("currentTheme"));
    QVERIFY(currentTheme);
    QCOMPARE(currentTheme->property("background").value<QColor>(), QColor("#1a1a1a"));
    QCOMPARE(currentTheme->property("textColor").value<QColor>(), QColor("#E0E0E0"));
}

void tst_QQmlPreviewObjectPatch::singletonPropertyAddition()
{
    QQmlEngine engine;
    QQmlComponent oldComp(&engine, testFileUrl("SingletonPropertyAddOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QCOMPARE(object->property("background").value<QColor>(), QColor("#121212"));
    QVERIFY(!object->property("borderColor").isValid());

    QQmlComponent newComp(&engine, testFileUrl("SingletonPropertyAddNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    if (objects.empty())
        objects.push_back(object.data());

    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    // The newly added property should become accessible on the singleton.
    QCOMPARE(object->property("borderColor").value<QColor>(), QColor("#3E3E3E"));

    // Existing properties should be preserved.
    QCOMPARE(object->property("background").value<QColor>(), QColor("#121212"));
}

void tst_QQmlPreviewObjectPatch::singletonConsumerBindingRefresh()
{
    QQmlEngine engine;

    const QString moduleDir = dataDirectory() + QStringLiteral("/SingletonModule");
    const QString patchedDir = dataDirectory() + QStringLiteral("/SingletonModulePatched");
    engine.addImportPath(dataDirectory());

    // Load Consumer which binds to Colors.background and Colors.textColor.
    QQmlComponent consumerComp(&engine, QUrl::fromLocalFile(moduleDir + "/Consumer.qml"));
    QVERIFY2(consumerComp.isReady(), qPrintable(consumerComp.errorString()));
    QScopedPointer<QObject> consumer(consumerComp.create());
    QVERIFY(consumer);

    QCOMPARE(consumer->property("currentBackground").value<QColor>(), QColor("#121212"));
    QCOMPARE(consumer->property("currentTextColor").value<QColor>(), QColor("#FEFEFE"));

    // Get old and new Colors singleton CUs.
    QQmlComponent colorsOldComp(&engine, QUrl::fromLocalFile(moduleDir + "/Colors.qml"));
    QVERIFY2(colorsOldComp.isReady(), qPrintable(colorsOldComp.errorString()));
    const auto oldColorsCU = QQmlComponentPrivate::get(&colorsOldComp)->compilationUnit();

    QQmlComponent colorsNewComp(&engine, QUrl::fromLocalFile(patchedDir + "/Colors.qml"));
    QVERIFY2(colorsNewComp.isReady(), qPrintable(colorsNewComp.errorString()));
    const auto newColorsCU = QQmlComponentPrivate::get(&colorsNewComp)->compilationUnit();
    QVERIFY(oldColorsCU && newColorsCU);

    // Patch the singleton.
    auto singletonObjects = objectsForCompilationUnit(&engine, oldColorsCU);
    QVERIFY(!singletonObjects.empty());
    QCOMPARE_NE(updateObjects(singletonObjects, oldColorsCU, newColorsCU),
                QQmlPreview::PatchResult::Failed);

    QCOMPARE(consumer->property("currentBackground").value<QColor>(), QColor("#ffffff"));
    QCOMPARE(consumer->property("currentTextColor").value<QColor>(), QColor("#121111"));
}

// When a property is added or removed, the implicit default-property bindings
// (Type_Object) shift in the binding table. Verify that after patching:
//  (a) all children remain in the visual parent's "data" list, and
//  (b) they appear in the correct order.
void tst_QQmlPreviewObjectPatch::childOrderBindingShift_data()
{
    QTest::addColumn<QString>("oldFile");
    QTest::addColumn<QString>("newFile");
    QTest::addColumn<QStringList>("expectedOrder");

    // Adding a property shifts child bindings to the right.
    QTest::newRow("addProperty") << "ChildOrderShiftOld.qml" << "ChildOrderShiftNew.qml"
                                 << QStringList{ "alpha", "beta" };

    // Removing a property shifts child bindings to the left.
    QTest::newRow("removeProperty") << "ChildOrderShiftNew.qml" << "ChildOrderShiftOld.qml"
                                    << QStringList{ "alpha", "beta" };

    // Inserting a child between existing ones (ObjectChanged + ObjectAdded).
    QTest::newRow("insertChild") << "ChildOrderInsertOld.qml" << "ChildOrderInsertNew.qml"
                                 << QStringList{ "alpha", "beta", "gamma" };

    // Removing a child from the middle (ObjectChanged + ObjectRemoved).
    QTest::newRow("removeChild") << "ChildOrderInsertNew.qml" << "ChildOrderInsertOld.qml"
                                 << QStringList{ "alpha", "gamma" };
}

void tst_QQmlPreviewObjectPatch::childOrderBindingShift()
{
    QFETCH(QString, oldFile);
    QFETCH(QString, newFile);
    QFETCH(QStringList, expectedOrder);

    QQmlEngine engine;
    QQmlComponent oldComp(&engine, testFileUrl(oldFile));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> object(oldComp.create());
    QVERIFY(object);

    QQmlComponent newComp(&engine, testFileUrl(newFile));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // Verify child count and order via the "data" list property.
    QQmlListReference dataList(object.data(), "data");
    QCOMPARE(dataList.count(), expectedOrder.size());
    for (qsizetype i = 0; i < expectedOrder.size(); ++i) {
        QObject *child = dataList.at(i);
        QVERIFY2(
                child,
                qPrintable(
                        QString("data[%1] is null, expected \"%2\"").arg(i).arg(expectedOrder[i])));
        QCOMPARE(child->objectName(), expectedOrder[i]);
    }
}

// Required property supplied via createWithInitialProperties must survive a
// hot-reload rebuild of that component's CU. Before the fix, stashExternalState()
// skipped declared non-alias VME properties that had no binding, so the required
// property was reset to null when the VME meta-object was reconstructed, and any
// dependent bindings (e.g. "value: dependency.answer") evaluated to -1 / null.
void tst_QQmlPreviewObjectPatch::requiredPropertyPreservedOnRebuild()
{
    QQmlEngine localEngine;

    // Provider: a simple object with an "answer" property.
    QQmlComponent providerComp(&localEngine);
    providerComp.setData("import QtQml\nQtObject { property int answer: 42 }",
                         QUrl("file:///required_prop_provider.qml"));
    QVERIFY2(providerComp.isReady(), qPrintable(providerComp.errorString()));
    std::unique_ptr<QObject> provider(providerComp.create());
    QVERIFY(provider);

    // Target: declares "required property QtObject dependency" and a binding
    // "property int value: dependency ? dependency.answer : -1".
    // The required property is set via createWithInitialProperties — it has no
    // binding in the compilation unit, only an externally-supplied value.
    QQmlComponent targetComp(&localEngine, testFileUrl("RequiredPropPreservedOld.qml"));
    QVERIFY2(targetComp.isReady(), qPrintable(targetComp.errorString()));
    std::unique_ptr<QObject> target(targetComp.createWithInitialProperties(
            { { QStringLiteral("dependency"), QVariant::fromValue(provider.get()) } }));
    QVERIFY(target);

    // Sanity: before rebuild, value is read through dependency correctly.
    QCOMPARE(target->property("dependency").value<QObject *>(), provider.get());
    QCOMPARE(target->property("value").toInt(), 42);

    // Simulate a hot-reload: rebuild the target's CU (new version adds "marker: 99").
    QQmlComponent newComp(&localEngine, testFileUrl("RequiredPropPreservedNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&targetComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&localEngine, oldExecUnit);
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // After rebuild, required property must still point to the original provider.
    QCOMPARE(target->property("dependency").value<QObject *>(), provider.get());
    // The script binding "dependency ? dependency.answer : -1" must still evaluate
    // correctly through the restored dependency reference.
    QCOMPARE(target->property("value").toInt(), 42);
    // The new constant property from the rebuild must also be present.
    QCOMPARE(target->property("marker").toInt(), 99);
}

// Reproduces the coffee demo button bug: a child composite type (CoffeeCard/
// ChildBindingScopeWidget) has a binding on a deeply-nested sub-object (the
// button's "enabled" property, set from the widget's own CU). When the parent
// form is rebuilt, the widget and its sub-objects are fully recreated with fresh
// bindings. Without the fix, stashExternalState() incorrectly stashes the old
// "enabled" binding (classifying it as "external" because the widget's CU is
// not in internalUnits). restoreExternalState() then overwrites the fresh binding
// with the stale copy whose scope points to the OLD (retired) widget. This makes
// the binding evaluate against dead data, breaking the "enabled" state.
void tst_QQmlPreviewObjectPatch::childBindingScopeAfterFormRebuild()
{
    QQmlEngine localEngine;
    localEngine.addImportPath(dataDirectory());

    QQmlComponent outerComp(&localEngine, testFileUrl("ChildBindingScopeOuter.qml"));
    QVERIFY2(outerComp.isReady(), qPrintable(outerComp.errorString()));
    std::unique_ptr<QObject> outer(outerComp.create());
    QVERIFY(outer);

    QObject *wrapper = outer->property("inner").value<QObject *>();
    QVERIFY(wrapper);
    QObject *widget = wrapper->property("widget").value<QObject *>();
    QVERIFY(widget);
    QObject *button = widget->property("button").value<QObject *>();
    QVERIFY(button);

    // Sanity: active=true → interval=500 (binding: root.active ? 500 : 1000)
    QCOMPARE(widget->property("active").toBool(), true);
    QCOMPARE(button->property("interval").toInt(), 500);

    // Verify binding is live before rebuild.
    widget->setProperty("active", false);
    QCOMPARE(button->property("interval").toInt(), 1000);
    widget->setProperty("active", true);
    QCOMPARE(button->property("interval").toInt(), 500);

    // Load the form's CU for rebuild.
    QQmlComponent oldFormComp(&localEngine, testFileUrl("ChildBindingScopeFormOld.qml"));
    QVERIFY2(oldFormComp.isReady(), qPrintable(oldFormComp.errorString()));
    const auto oldExecUnit = QQmlComponentPrivate::get(&oldFormComp)->compilationUnit();
    QVERIFY(oldExecUnit);

    QQmlComponent newFormComp(&localEngine, testFileUrl("ChildBindingScopeFormNew.qml"));
    QVERIFY2(newFormComp.isReady(), qPrintable(newFormComp.errorString()));
    const auto newExecUnit = QQmlComponentPrivate::get(&newFormComp)->compilationUnit();
    QVERIFY(newExecUnit);

    // Rebuild the form CU. The widget child is destroyed and recreated.
    auto objects = objectsForCompilationUnit(&localEngine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // Re-fetch — widget and button (Timer) were replaced during rebuild.
    widget = wrapper->property("widget").value<QObject *>();
    QVERIFY(widget);
    button = widget->property("button").value<QObject *>();
    QVERIFY(button);

    // The "interval" binding on the new Timer must read from the NEW widget's
    // "active" property. active defaults to true, so interval must be 500.
    QCOMPARE(button->property("interval").toInt(), 500);

    // Verify the binding is LIVE — changing "active" on the NEW widget must
    // propagate to the timer. If the binding has a stale scope (old widget),
    // this won't update.
    widget->setProperty("active", false);
    QCOMPARE(button->property("interval").toInt(), 1000);

    widget->setProperty("active", true);
    QCOMPARE(button->property("interval").toInt(), 500);

    // Verify the form change took effect.
    QCOMPARE(wrapper->property("label").toString(), QString("hello!"));
}

// When the top-level component directly inherits from the form being rebuilt
// (ddata->compilationUnit != oldUnit and needsVMEMetaObject is true), the
// instance-level VME must be created with the correct CU and objectIndex.
// Previously, it incorrectly used newUnit/cuIndex (the rebuilt form) instead of
// instanceLevel.newCu/instanceLevel.objectIndex (the wrapper's own CU), causing
// a crash in property writes during repopulateBindings.
void tst_QQmlPreviewObjectPatch::topLevelDerivedTypeFormRebuild()
{
    QQmlEngine localEngine;
    localEngine.addImportPath(dataDirectory());

    // Create the wrapper DIRECTLY (no outer instantiation layer).
    // The wrapper inherits from the form and adds its own property (triggerCount).
    QQmlComponent wrapperComp(&localEngine, testFileUrl("ChildBindingScopeWrapper.qml"));
    QVERIFY2(wrapperComp.isReady(), qPrintable(wrapperComp.errorString()));
    std::unique_ptr<QObject> wrapper(wrapperComp.create());
    QVERIFY(wrapper);

    QObject *widget = wrapper->property("widget").value<QObject *>();
    QVERIFY(widget);
    QCOMPARE(wrapper->property("triggerCount").toInt(), 0);
    QCOMPARE(wrapper->property("label").toString(), QString("hello"));

    // Load the form's CU for rebuild.
    QQmlComponent oldFormComp(&localEngine, testFileUrl("ChildBindingScopeFormOld.qml"));
    QVERIFY2(oldFormComp.isReady(), qPrintable(oldFormComp.errorString()));
    const auto oldExecUnit = QQmlComponentPrivate::get(&oldFormComp)->compilationUnit();
    QVERIFY(oldExecUnit);

    QQmlComponent newFormComp(&localEngine, testFileUrl("ChildBindingScopeFormNew.qml"));
    QVERIFY2(newFormComp.isReady(), qPrintable(newFormComp.errorString()));
    const auto newExecUnit = QQmlComponentPrivate::get(&newFormComp)->compilationUnit();
    QVERIFY(newExecUnit);

    // This must not crash. The wrapper is found via its VME chain (which includes
    // the form CU). The instance-level VME is for the wrapper's own CU.
    auto objects = objectsForCompilationUnit(&localEngine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // The wrapper's own property must still be functional.
    QCOMPARE(wrapper->property("triggerCount").toInt(), 0);

    // The form's label change must have taken effect.
    QCOMPARE(wrapper->property("label").toString(), QString("hello!"));

    // The widget must still be accessible.
    widget = wrapper->property("widget").value<QObject *>();
    QVERIFY(widget);
    QObject *button = widget->property("button").value<QObject *>();
    QVERIFY(button);

    // The widget's binding must still work.
    QCOMPARE(button->property("interval").toInt(), 500);
    widget->setProperty("active", false);
    QCOMPARE(button->property("interval").toInt(), 1000);
}

// Reproduces coffee demo crash: adding a new CoffeeCard (composite child with id)
// to ChoosingCoffeeForm.ui.qml. The new id causes registerObjectWithContextById
// to write beyond the context's id array bounds (heap-buffer-overflow in setIdValue).
void tst_QQmlPreviewObjectPatch::childAddedWithIdCrash()
{
    QQmlEngine localEngine;

    QQmlComponent oldComp(&localEngine, testFileUrl("CoffeeFormOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    // Sanity: 4 CoffeeCard children exist via aliases.
    QVERIFY(object->property("cappuccino").value<QObject *>());
    QVERIFY(object->property("macchiato").value<QObject *>());

    QQmlComponent newComp(&localEngine, testFileUrl("CoffeeFormNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    // This crashes with a heap-buffer-overflow in setIdValue when the new
    // CoffeeCard's id (macchiato_for_ulf) is registered beyond the old context bounds.
    auto objects = objectsForCompilationUnit(&localEngine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

// Same as above but the form is used as a composite base type: CoffeeChooser
// derives from CoffeeFormOld, and the form's CU is rebuilt (simulating
// ChoosingCoffee using ChoosingCoffeeForm in the coffee demo).
void tst_QQmlPreviewObjectPatch::childAddedWithIdCompositeCrash()
{
    QQmlEngine localEngine;

    QQmlComponent chooserComp(&localEngine, testFileUrl("CoffeeChooser.qml"));
    QVERIFY2(chooserComp.isReady(), qPrintable(chooserComp.errorString()));
    std::unique_ptr<QObject> chooser(chooserComp.create());
    QVERIFY(chooser);

    // Get the form's compilation unit (the base type).
    QQmlComponent oldFormComp(&localEngine, testFileUrl("CoffeeFormOld.qml"));
    QVERIFY2(oldFormComp.isReady(), qPrintable(oldFormComp.errorString()));
    QQmlComponent newFormComp(&localEngine, testFileUrl("CoffeeFormNew.qml"));
    QVERIFY2(newFormComp.isReady(), qPrintable(newFormComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldFormComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newFormComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    // This crashes with a heap-buffer-overflow in setIdValue when the chooser
    // instance's form context is rebuilt with more ids than originally allocated.
    auto objects = objectsForCompilationUnit(&localEngine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void tst_QQmlPreviewObjectPatch::insertBinding()
{
    QQmlComponent oldComponent(&engine, testFileUrl("InsertBindingOld.qml"));
    QVERIFY2(oldComponent.isReady(), qPrintable(oldComponent.errorString()));
    const auto oldUnit = QQmlComponentPrivate::get(&oldComponent)->compilationUnit();

    std::unique_ptr<QObject> object(oldComponent.create());
    QVERIFY(object);
    QVERIFY2(object->objectName().isEmpty(), qPrintable(object->objectName()));

    QQmlComponent newComponent(&engine, testFileUrl("InsertBindingNew.qml"));
    QVERIFY2(newComponent.isReady(), qPrintable(newComponent.errorString()));
    const auto newUnit = QQmlComponentPrivate::get(&newComponent)->compilationUnit();

    auto objects = objectsForCompilationUnit(&engine, oldUnit);
    QCOMPARE(objects.size(), 2);
    QCOMPARE_NE(updateObjects(objects, oldUnit, newUnit), QQmlPreview::PatchResult::Failed);

    QVERIFY2(object->objectName().isEmpty(), qPrintable(object->objectName()));
}

// Reproduces coffee demo crash 3: changing a function body in a derived type
// (ApplicationFlow extending ApplicationFlowForm) that uses states with
// PropertyChanges targeting form aliases. The rebuild should not trigger
// ASSERT "canGetTypeFromVariant<T>(this)" in qvariant.h.
void tst_QQmlPreviewObjectPatch::derivedTypeFunctionChangeCrash()
{
    QQmlEngine localEngine;
    QQuickWindow window;
    window.resize(400, 600);

    QQmlComponent oldComp(&localEngine, testFileUrl("DerivedFunctionChangeOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    std::unique_ptr<QObject> object(oldComp.create());
    QVERIFY(object);

    // Parent the root item into the window so anchors and StackView resolve.
    QQuickItem *rootItem = qobject_cast<QQuickItem *>(object.get());
    QVERIFY(rootItem);
    rootItem->setParentItem(window.contentItem());
    rootItem->setSize(QSizeF(400, 600));

    // Activate the "Settings" state and push an item onto the StackView
    // (simulating the coffee demo navigating to Settings page).
    QVERIFY(QMetaObject::invokeMethod(object.get(), "selectCoffee"));
    QCOMPARE(object->property("state").toString(), QString("Settings"));

    QObject *stack = object->property("stack").value<QObject *>();
    QVERIFY(stack);

    // Process events to ensure state transitions, layout, and rendering complete.
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();

    QQmlComponent newComp(&localEngine, testFileUrl("DerivedFunctionChangeNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    // This may crash with ASSERT "canGetTypeFromVariant<T>(this)" when the state
    // system tries to backup/restore anchor properties during rebuild while
    // the StackView has active content.
    auto objects = objectsForCompilationUnit(&localEngine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE_NE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Failed);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    // After rebuild, changing state exercises the state machine's property
    // backup/restore mechanism with potentially stale anchor values.
    object->setProperty("state", "Home");
    QCoreApplication::processEvents();
    object->setProperty("state", "Settings");
    QCoreApplication::processEvents();

    // If we survive, verify the function was updated.
    QVERIFY(QMetaObject::invokeMethod(object.get(), "selectCoffee"));
    QCOMPARE(object->property("coffeeName").toString(), QString("Cappuccinooooo"));
}

// Reproduces the coffee demo failure exactly: the derived-with-id-method type (ApplicationFlow)
// is instantiated *nested* inside an outer document (Main.qml). Changing the method body must
// re-home the VME method against the context that actually owns the id so that a self-referencing
// id lookup ("applicationFlow.foo") still resolves. Historically refreshVmeMethods used the wrong
// context here, so after the patch the method threw "applicationFlow is not defined".
void tst_QQmlPreviewObjectPatch::nestedDerivedIdMethodChange()
{
    QQmlComponent wrapper(&engine, testFileUrl("IdMethodWrapper.qml"));
    QVERIFY2(wrapper.isReady(), qPrintable(wrapper.errorString()));
    std::unique_ptr<QObject> root(wrapper.create());
    QVERIFY(root);

    QObject *inner = root->property("inner").value<QObject *>();
    QVERIFY(inner);

    QVERIFY(QMetaObject::invokeMethod(inner, "bump"));
    QCOMPARE(inner->property("value").toInt(), 10);

    QQmlComponent oldComp(&engine, testFileUrl("IdMethodOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QQmlComponent newComp(&engine, testFileUrl("IdMethodNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE(updateObjects(objects, oldExecUnit, newExecUnit),
             QQmlPreview::PatchResult::Rebuilt);

    // The refreshed method must still resolve the "applicationFlow" id and write the new value.
    QVERIFY(QMetaObject::invokeMethod(inner, "bump"));
    QCOMPARE(inner->property("value").toInt(), 20);
}

void tst_QQmlPreviewObjectPatch::compositePropertyDefaultChangeKeepsVisualChildren()
{
    const QString moduleDir = dataDirectory() + QStringLiteral("/HotReloadButton");
    const QString patchedDir = dataDirectory() + QStringLiteral("/HotReloadButtonPatched");

    QQuickWindow window;
    window.resize(400, 400);

    // Instantiate the "number pad": many CalculatorButton instances plus one
    // BackspaceButton, laid out as visual children of a GridLayout.
    QQmlComponent numberPad(&engine, QUrl::fromLocalFile(moduleDir + "/NumberPad.qml"));
    QVERIFY2(numberPad.isReady(), qPrintable(numberPad.errorString()));
    std::unique_ptr<QObject> root(numberPad.create());
    QVERIFY(root);

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(root.get());
    QVERIFY(rootItem);
    rootItem->setParentItem(window.contentItem());
    QCoreApplication::processEvents();

    const QStringList calcButtonNames = { QStringLiteral("d7"), QStringLiteral("d8"),
                                          QStringLiteral("d9"), QStringLiteral("opdiv"),
                                          QStringLiteral("d4"), QStringLiteral("d5"),
                                          QStringLiteral("d6"), QStringLiteral("opmul"),
                                          QStringLiteral("d1") };

    const auto contentItemOf = [](QQuickItem *button) {
        return button->property("contentItem").value<QQuickItem *>();
    };
    const auto backgroundOf = [](QQuickItem *button) {
        return button->property("background").value<QQuickItem *>();
    };

    // Grab a pointer and the visual parent of every CalculatorButton instance
    // before the reload so we can tell afterwards whether it disappeared. A
    // RoundButton with no contentItem/background draws nothing, i.e. it looks
    // like it vanished — so record those too.
    QHash<QString, QPointer<QQuickItem>> buttons;
    QHash<QString, QPointer<QQuickItem>> visualParents;
    for (const QString &name : calcButtonNames) {
        QQuickItem *item = root->findChild<QQuickItem *>(name);
        QVERIFY2(item, qPrintable(name));
        QVERIFY2(item->parentItem(), qPrintable(name));
        QVERIFY2(contentItemOf(item), qPrintable(name + " has no contentItem before reload"));
        QVERIFY2(backgroundOf(item), qPrintable(name + " has no background before reload"));
        buttons.insert(name, item);
        visualParents.insert(name, item->parentItem());
    }

    QQuickItem *backspace = root->findChild<QQuickItem *>(QStringLiteral("backspace"));
    QVERIFY(backspace);
    QPointer<QQuickItem> backspaceGuard(backspace);
    QQuickItem *backspaceParent = backspace->parentItem();
    QVERIFY(backspaceParent);
    QVERIFY(contentItemOf(backspace));

    // Old CalculatorButton CU (shared with the live instances) and the patched one.
    QQmlComponent calcOld(&engine, QUrl::fromLocalFile(moduleDir + "/CalculatorButton.qml"));
    QVERIFY2(calcOld.isReady(), qPrintable(calcOld.errorString()));
    QQmlComponent calcNew(&engine, QUrl::fromLocalFile(patchedDir + "/CalculatorButton.qml"));
    QVERIFY2(calcNew.isReady(), qPrintable(calcNew.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&calcOld)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&calcNew)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    // Every CalculatorButton instance is still alive and still parented into the
    // same GridLayout — the QObject is rebuilt in place, so it never leaves the
    // visual tree at the item level.
    for (const QString &name : calcButtonNames) {
        QVERIFY2(!buttons.value(name).isNull(), qPrintable(name + " was deleted"));
        QQuickItem *item = buttons.value(name);
        QVERIFY2(item->parentItem(), qPrintable(name + " lost its visual parent"));
        QCOMPARE(item->parentItem(), visualParents.value(name).data());
    }

    // The unrelated BackspaceButton must survive untouched.
    QVERIFY(!backspaceGuard.isNull());
    QCOMPARE(backspace->parentItem(), backspaceParent);
    QVERIFY(contentItemOf(backspace));

    // The regression: rebuilding the composite type must re-establish the
    // contentItem/background it assigns via (deferred) object bindings. Otherwise
    // each RoundButton has nothing to draw and appears to vanish.
    for (const QString &name : calcButtonNames) {
        QQuickItem *item = buttons.value(name);
        QVERIFY2(contentItemOf(item), qPrintable(name + " lost its contentItem on reload"));
        QVERIFY2(backgroundOf(item), qPrintable(name + " lost its background on reload"));
    }

    for (const QString &name : { QStringLiteral("d7"), QStringLiteral("d1") }) {
        QQuickItem *content = contentItemOf(buttons.value(name));
        QVERIFY(content);
        QCOMPARE(content->property("color").value<QColor>(), QColor::fromString("green"));
    }

    // The rebuild must recreate only the winning deferred items (CalculatorButton's
    // Text/Rectangle override), not also the shadowed style defaults. So a reloaded
    // button has the same number of direct child items as a freshly-created one.
    const qsizetype freshChildCount =
            backspace->findChildren<QQuickItem *>(Qt::FindDirectChildrenOnly).size();
    for (const QString &name : calcButtonNames) {
        const auto reloadedChildren =
                buttons.value(name)->findChildren<QQuickItem *>(Qt::FindDirectChildrenOnly);
        QCOMPARE(reloadedChildren.size(), freshChildCount);
    }
}

void tst_QQmlPreviewObjectPatch::compositeBaseLayoutChangeRelinksDerivedCaches()
{
    const QString moduleDir = dataDirectory() + QStringLiteral("/HotReloadButtonFunc");
    const QString patchedDir = dataDirectory() + QStringLiteral("/HotReloadButtonFuncPatched");

    QQuickWindow window;
    window.resize(400, 400);

    QQmlComponent numberPad(&engine, QUrl::fromLocalFile(moduleDir + "/NumberPad.qml"));
    QVERIFY2(numberPad.isReady(), qPrintable(numberPad.errorString()));
    std::unique_ptr<QObject> root(numberPad.create());
    QVERIFY(root);

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(root.get());
    QVERIFY(rootItem);
    rootItem->setParentItem(window.contentItem());
    QCoreApplication::processEvents();

    const QStringList calcButtonNames = { QStringLiteral("d7"), QStringLiteral("d8"),
                                          QStringLiteral("d9"), QStringLiteral("opdiv"),
                                          QStringLiteral("d4"), QStringLiteral("d5"),
                                          QStringLiteral("d6"), QStringLiteral("opmul"),
                                          QStringLiteral("d1") };

    const auto contentItemOf = [](QQuickItem *button) {
        return button->property("contentItem").value<QQuickItem *>();
    };
    const auto backgroundOf = [](QQuickItem *button) {
        return button->property("background").value<QQuickItem *>();
    };

    // All instances are derived types (component DigitButton: CalculatorButton {}), so
    // CalculatorButton appears only as a composite base type in their VME chain. Its deferred
    // contentItem/background bindings call the type's own getTextColor()/getBackgroundColor() VME
    // methods.
    for (const QString &name : calcButtonNames) {
        QQuickItem *item = root->findChild<QQuickItem *>(name);
        QVERIFY2(item, qPrintable(name));
        QVERIFY(!item->property("bla").isValid()); // not in the old CU
        QVERIFY2(contentItemOf(item), qPrintable(name + " has no contentItem before reload"));
        QVERIFY2(backgroundOf(item), qPrintable(name + " has no background before reload"));
    }

    QQmlComponent calcOld(&engine, QUrl::fromLocalFile(moduleDir + "/CalculatorButton.qml"));
    QVERIFY2(calcOld.isReady(), qPrintable(calcOld.errorString()));
    QQmlComponent calcNew(&engine, QUrl::fromLocalFile(patchedDir + "/CalculatorButton.qml"));
    QVERIFY2(calcNew.isReady(), qPrintable(calcNew.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&calcOld)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&calcNew)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());

    // Adding "property int bla" to CalculatorButton shifts its VME method indices. Because the base
    // carries deferred bindings, the derived instances are recreated by rebuilding the enclosing
    // NumberPad root, and the derived types' property caches are relinked to the new base layout so
    // getBackgroundColor()/getTextColor() resolve at the correct (shifted) index rather than
    // asserting in vmeMethod().
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    // The buttons were recreated. Re-find them and check the added property is present, the deferred
    // contentItem/background were recreated (not dropped), and the deferred bindings that call the
    // shifted VME methods are live.
    for (const QString &name : calcButtonNames) {
        QQuickItem *item = root->findChild<QQuickItem *>(name);
        QVERIFY2(item, qPrintable(name + " missing after reload"));
        QVERIFY2(item->property("bla").isValid(), qPrintable(name + " lacks the added property"));
        QVERIFY2(contentItemOf(item), qPrintable(name + " lost its contentItem on reload"));
        QVERIFY2(backgroundOf(item), qPrintable(name + " lost its background on reload"));
    }
}

void tst_QQmlPreviewObjectPatch::deferredDerivedInstanceRecreatedOnStructuralReload()
{
    const QString moduleDir = dataDirectory() + QStringLiteral("/HotReloadDeferredDerived");
    const QString patchedDir = dataDirectory() + QStringLiteral("/HotReloadDeferredDerivedPatched");

    QQuickWindow window;
    window.resize(400, 400);

    QQmlComponent numberPad(&engine, QUrl::fromLocalFile(moduleDir + "/NumberPad.qml"));
    QVERIFY2(numberPad.isReady(), qPrintable(numberPad.errorString()));
    std::unique_ptr<QObject> root(numberPad.create());
    QVERIFY(root);

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(root.get());
    QVERIFY(rootItem);
    rootItem->setParentItem(window.contentItem());
    QCoreApplication::processEvents();

    const auto contentItemOf = [](QQuickItem *button) {
        return button->property("contentItem").value<QQuickItem *>();
    };

    // Grab a guard to each derived instance so we can prove it is recreated (not patched in place).
    const QStringList names = { QStringLiteral("d7"), QStringLiteral("opdiv"), QStringLiteral("d1") };
    QHash<QString, QPointer<QQuickItem>> oldButtons;
    for (const QString &name : names) {
        QQuickItem *item = root->findChild<QQuickItem *>(name);
        QVERIFY2(item, qPrintable(name));
        QVERIFY2(contentItemOf(item), qPrintable(name + " has no contentItem before reload"));
        oldButtons.insert(name, item);
    }

    QQmlComponent calcOld(&engine, QUrl::fromLocalFile(moduleDir + "/CalculatorButton.qml"));
    QVERIFY2(calcOld.isReady(), qPrintable(calcOld.errorString()));
    QQmlComponent calcNew(&engine, QUrl::fromLocalFile(patchedDir + "/CalculatorButton.qml"));
    QVERIFY2(calcNew.isReady(), qPrintable(calcNew.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&calcOld)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&calcNew)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Rebuilt);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    for (const QString &name : names) {
        // The base carries deferred bindings, so the derived instance was recreated via the
        // enclosing root rather than rebuilt in place: the old QObject is gone.
        QVERIFY2(oldButtons.value(name).isNull(), qPrintable(name + " was not recreated"));

        // The fresh instance exists, carries the added property, and kept its deferred content.
        QQuickItem *item = root->findChild<QQuickItem *>(name);
        QVERIFY2(item, qPrintable(name + " missing after reload"));
        QVERIFY2(item->property("bla").isValid(), qPrintable(name + " lacks the added property"));
        QVERIFY2(contentItemOf(item), qPrintable(name + " lost its deferred contentItem on reload"));
    }
}

void tst_QQmlPreviewObjectPatch::compositeBaseLayoutChangeRelinksDerivedOwnMembers()
{
    const QString moduleDir = dataDirectory() + QStringLiteral("/HotReloadOwnMembers");
    const QString patchedDir = dataDirectory() + QStringLiteral("/HotReloadOwnMembersPatched");

    QQmlComponent pad(&engine, QUrl::fromLocalFile(moduleDir + "/Pad.qml"));
    QVERIFY2(pad.isReady(), qPrintable(pad.errorString()));
    std::unique_ptr<QObject> root(pad.create());
    QVERIFY(root);

    const QStringList names = { QStringLiteral("w1"), QStringLiteral("w2") };
    QHash<QString, QPointer<QObject>> widgets;
    for (const QString &name : names) {
        QObject *w = root->findChild<QObject *>(name);
        QVERIFY2(w, qPrintable(name));
        // Own property of the derived type, and a method reading the base's property.
        QCOMPARE(w->property("localCount").toInt(), 3);
        QVariant sum;
        QVERIFY(QMetaObject::invokeMethod(w, "combined", Q_RETURN_ARG(QVariant, sum)));
        QCOMPARE(sum.toInt(), 3 + 10); // localCount + base.value
        widgets.insert(name, w);
    }

    QQmlComponent baseOld(&engine, QUrl::fromLocalFile(moduleDir + "/Widget.qml"));
    QVERIFY2(baseOld.isReady(), qPrintable(baseOld.errorString()));
    QQmlComponent baseNew(&engine, QUrl::fromLocalFile(patchedDir + "/Widget.qml"));
    QVERIFY2(baseNew.isReady(), qPrintable(baseNew.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&baseOld)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&baseNew)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    for (const QString &name : names) {
        QVERIFY2(!widgets.value(name).isNull(), qPrintable(name + " was deleted"));
        QObject *w = widgets.value(name);
        // The base gained "extra" (value 5) and its change signal; the derived type's own
        // property/method indices shifted accordingly and must still resolve correctly.
        QVERIFY2(w->property("extra").isValid(), qPrintable(name + " lacks base's new property"));
        QCOMPARE(w->property("localCount").toInt(), 3); // own property still readable
        QVariant sum;
        QVERIFY(QMetaObject::invokeMethod(w, "combined", Q_RETURN_ARG(QVariant, sum)));
        QCOMPARE(sum.toInt(), 3 + 10); // own method still callable, reads base.value
        // Own change signal still wired: localCount is bindable/settable.
        w->setProperty("localCount", 7);
        QCOMPARE(w->property("localCount").toInt(), 7);
        QVERIFY(QMetaObject::invokeMethod(w, "combined", Q_RETURN_ARG(QVariant, sum)));
        QCOMPARE(sum.toInt(), 7 + 10);
    }
}

void tst_QQmlPreviewObjectPatch::compositeBaseLayoutChangeDropsRemovedBaseProperty()
{
    const QString moduleDir = dataDirectory() + QStringLiteral("/HotReloadRemovedBaseProp");
    const QString patchedDir = dataDirectory() + QStringLiteral("/HotReloadRemovedBasePropPatched");

    QQmlComponent pad(&engine, QUrl::fromLocalFile(moduleDir + "/Pad.qml"));
    QVERIFY2(pad.isReady(), qPrintable(pad.errorString()));
    std::unique_ptr<QObject> root(pad.create());
    QVERIFY(root);

    const QStringList names = { QStringLiteral("r1"), QStringLiteral("r2") };
    QHash<QString, QPointer<QObject>> widgets;
    for (const QString &name : names) {
        QObject *w = root->findChild<QObject *>(name);
        QVERIFY2(w, qPrintable(name));
        // Both bindings resolve against the old base layout.
        QCOMPARE(w->property("gone").toInt(), 42);
        QCOMPARE(w->property("stay").toInt(), 7);
        widgets.insert(name, w);
    }

    QQmlComponent baseOld(&engine, QUrl::fromLocalFile(moduleDir + "/BaseWidget.qml"));
    QVERIFY2(baseOld.isReady(), qPrintable(baseOld.errorString()));
    QQmlComponent baseNew(&engine, QUrl::fromLocalFile(patchedDir + "/BaseWidget.qml"));
    QVERIFY2(baseNew.isReady(), qPrintable(baseNew.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&baseOld)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&baseNew)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());

    // Removing "gone" from the base shrinks its meta-object. The derived instances
    // still carry a binding targeting "gone"; re-resolving it against the relinked
    // cache fails, so refreshBindingPropertyData() must clear that entry rather than
    // keep the stale property-data pointer from the old layout.
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    for (const QString &name : names) {
        QVERIFY2(!widgets.value(name).isNull(), qPrintable(name + " was deleted"));
        QObject *w = widgets.value(name);
        // "gone" is no longer part of the type.
        QVERIFY2(!w->property("gone").isValid(), qPrintable(name + " still has removed property"));
        // The surviving "stay" binding still resolved correctly and was not
        // clobbered by the dropped binding writing through a stale offset.
        QCOMPARE(w->property("stay").toInt(), 7);
    }
}

// An inline component's root changes its non-composite base type (Rectangle -> Item). The
// instance cannot be patched in place, but rebuilding the enclosing document root recreates it
// from scratch with the new base type.
void tst_QQmlPreviewObjectPatch::inlineComponentBaseTypeChange()
{
    QQmlComponent oldComp(&engine, testFileUrl("InlineComponentBaseChangeOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> root(oldComp.create());
    QVERIFY(root);
    QObject *inner = root->findChild<QObject *>("inner");
    QVERIFY(inner);
    // Rectangle has a "color" property; Item does not. Use it to tell the base types apart.
    QVERIFY(inner->property("color").isValid());
    QCOMPARE(inner->property("marker").toInt(), 1);

    QQmlComponent newComp(&engine, testFileUrl("InlineComponentBaseChangeNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Rebuilt);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    inner = root->findChild<QObject *>("inner");
    QVERIFY(inner);
    QVERIFY(!inner->property("color").isValid()); // now an Item
    QCOMPARE(inner->property("marker").toInt(), 2);
}

// Like inlineComponentBaseTypeChange(), but the old base type carries a binding on a property
// that only exists on that base type (Rectangle's "color"). After the base type changes to Item
// the binding is no longer valid. Because the object is recreated from scratch off the new unit
// rather than patched in place, the stale binding simply ceases to exist: no leftover expression,
// no crash, and the property is gone with its base type.
void tst_QQmlPreviewObjectPatch::inlineComponentBaseTypeChangeDropsInvalidBinding()
{
    QQmlComponent oldComp(&engine, testFileUrl("InlineComponentBaseChangeInvalidBindingOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> root(oldComp.create());
    QVERIFY(root);
    QObject *inner = root->findChild<QObject *>("inner");
    QVERIFY(inner);
    // The "color" binding is live on the Rectangle base type.
    QCOMPARE(inner->property("color").value<QColor>(), QColor(Qt::blue));
    QCOMPARE(inner->property("marker").toInt(), 1);

    QQmlComponent newComp(&engine, testFileUrl("InlineComponentBaseChangeInvalidBindingNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Rebuilt);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    inner = root->findChild<QObject *>("inner");
    QVERIFY(inner);
    // Now an Item: the "color" property and its binding are gone.
    QVERIFY(!inner->property("color").isValid());
    QCOMPARE(inner->property("marker").toInt(), 2);
}

// An external type (CrossCuInner) is instantiated inside a container in a different compilation
// unit. When CrossCuInner's root changes its base type (Rectangle -> Item), the instance is
// recreated by rebuilding the container root, which lives in the other compilation unit.
void tst_QQmlPreviewObjectPatch::crossCompilationUnitBaseTypeChange()
{
    QQmlComponent container(&engine, testFileUrl("CrossCuContainer.qml"));
    QVERIFY2(container.isReady(), qPrintable(container.errorString()));
    QScopedPointer<QObject> root(container.create());
    QVERIFY(root);
    QObject *inner = root->findChild<QObject *>("inner");
    QVERIFY(inner);
    QVERIFY(inner->property("color").isValid()); // Rectangle
    QCOMPARE(inner->property("marker").toInt(), 1);

    // The inner instance's innermost VME belongs to CrossCuInner's own compilation unit.
    const auto oldExecUnit = QQmlVMEMetaObject::get(inner)->compilationUnit();
    QVERIFY(oldExecUnit);

    QQmlComponent newComp(&engine, testFileUrl("CrossCuInnerNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Rebuilt);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    inner = root->findChild<QObject *>("inner");
    QVERIFY(inner);
    QVERIFY(!inner->property("color").isValid()); // now an Item
    QCOMPARE(inner->property("marker").toInt(), 2);
}

// The document root itself changes its non-composite base type and is loaded directly as the
// preview root. There is no enclosing scope to rebuild, so the reload must fail.
void tst_QQmlPreviewObjectPatch::topLevelBaseTypeChangeFails()
{
    QQmlComponent oldComp(&engine, testFileUrl("BaseTypeOld.qml"));
    QVERIFY2(oldComp.isReady(), qPrintable(oldComp.errorString()));
    QScopedPointer<QObject> root(oldComp.create());
    QVERIFY(root);

    QQmlComponent newComp(&engine, testFileUrl("BaseTypeNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&oldComp)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QCOMPARE(QQmlPreview::applyDiff(objects, oldExecUnit, newExecUnit),
             QQmlPreview::PatchResult::Failed);
}

// A derived type whose composite base type's non-composite base changed, loaded directly as the
// preview root. The derived instance is the topmost object, so there is nothing to rebuild above
// it and the reload must fail.
void tst_QQmlPreviewObjectPatch::derivedTypeBaseTypeChangeFails()
{
    QQmlComponent derComp(&engine, testFileUrl("DerivedFromInner.qml"));
    QVERIFY2(derComp.isReady(), qPrintable(derComp.errorString()));
    QScopedPointer<QObject> root(derComp.create());
    QVERIFY(root);

    // CrossCuInner's own compilation unit is the derived root's base level.
    const auto oldExecUnit = QQmlVMEMetaObject::get(root.get())->compilationUnit();
    QVERIFY(oldExecUnit);

    QQmlComponent newComp(&engine, testFileUrl("CrossCuInnerNew.qml"));
    QVERIFY2(newComp.isReady(), qPrintable(newComp.errorString()));
    const auto newExecUnit = QQmlComponentPrivate::get(&newComp)->compilationUnit();
    QVERIFY(newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE(QQmlPreview::applyDiff(objects, oldExecUnit, newExecUnit),
             QQmlPreview::PatchResult::Failed);
}

void tst_QQmlPreviewObjectPatch::changedBaseTypeRelinksDerivedInstanceCaches()
{
    const QString moduleDir = dataDirectory() + QStringLiteral("/HotReloadBaseChangeDerived");
    const QString patchedDir = dataDirectory() + QStringLiteral("/HotReloadBaseChangeDerivedPatched");

    QQmlComponent host(&engine, QUrl::fromLocalFile(moduleDir + "/Host.qml"));
    QVERIFY2(host.isReady(), qPrintable(host.errorString()));
    std::unique_ptr<QObject> root(host.create());
    QVERIFY(root);

    const QStringList names = { QStringLiteral("d1"), QStringLiteral("d2"), QStringLiteral("d3") };
    for (const QString &name : names) {
        QObject *item = root->findChild<QObject *>(name);
        QVERIFY2(item, qPrintable(name));
        // The "reported: base.tag()" binding ran during construction.
        QCOMPARE(item->property("reported").toInt(), 11);
        QVERIFY(!item->property("color").isValid()); // Item base, no color yet
    }

    QQmlComponent baseOld(&engine, QUrl::fromLocalFile(moduleDir + "/Base.qml"));
    QVERIFY2(baseOld.isReady(), qPrintable(baseOld.errorString()));
    QQmlComponent baseNew(&engine, QUrl::fromLocalFile(patchedDir + "/Base.qml"));
    QVERIFY2(baseNew.isReady(), qPrintable(baseNew.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&baseOld)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&baseNew)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    // Base's non-composite base changed (Item -> Rectangle), so the derived instances are recreated
    // by rebuilding the enclosing Host root. Their property caches must be relinked to the new base
    // layout, or the recreated "reported: base.tag()" binding resolves tag() at the old (unshifted)
    // VME index and asserts "index >= methodOffset()".
    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());
    QCOMPARE(updateObjects(objects, oldExecUnit, newExecUnit), QQmlPreview::PatchResult::Rebuilt);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    for (const QString &name : names) {
        QObject *item = root->findChild<QObject *>(name);
        QVERIFY2(item, qPrintable(name + " missing after reload"));
        // The relinked binding calls the new tag() at its shifted index, and the base is now a
        // Rectangle (so "color" exists).
        QCOMPARE(item->property("reported").toInt(), 22);
        QVERIFY2(item->property("color").isValid(), qPrintable(name + " is not a Rectangle"));
    }
}

void tst_QQmlPreviewObjectPatch::compositePropertyDefaultChangeExternalDeferredContent()
{
    const QString moduleDir = dataDirectory() + QStringLiteral("/HotReloadButtonExt");
    const QString patchedDir = dataDirectory() + QStringLiteral("/HotReloadButtonExtPatched");

    QQuickWindow window;
    window.resize(400, 400);

    QQmlComponent numberPad(&engine, QUrl::fromLocalFile(moduleDir + "/NumberPad.qml"));
    QVERIFY2(numberPad.isReady(), qPrintable(numberPad.errorString()));
    std::unique_ptr<QObject> root(numberPad.create());
    QVERIFY(root);

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(root.get());
    QVERIFY(rootItem);
    rootItem->setParentItem(window.contentItem());
    QCoreApplication::processEvents();

    // Digit buttons use the (patched) default textColor; operator buttons override it.
    const QStringList digitButtons = { QStringLiteral("d7"), QStringLiteral("d8"),
                                       QStringLiteral("d9"), QStringLiteral("d4"),
                                       QStringLiteral("d5"), QStringLiteral("d6"),
                                       QStringLiteral("d1") };
    const QStringList operatorButtons = { QStringLiteral("opdiv"), QStringLiteral("opmul") };
    const QStringList calcButtonNames = digitButtons + operatorButtons;

    const auto contentItemOf = [](QQuickItem *button) {
        return button->property("contentItem").value<QQuickItem *>();
    };

    QHash<QString, QPointer<QQuickItem>> buttons;
    for (const QString &name : calcButtonNames) {
        QQuickItem *item = root->findChild<QQuickItem *>(name);
        QVERIFY2(item, qPrintable(name));
        QQuickItem *content = contentItemOf(item);
        QVERIFY2(content, qPrintable(name + " has no contentItem before reload"));
        // The contentItem really is the Badge from the extra .qml file.
        QVERIFY2(content->property("tint").isValid(), qPrintable(name + " content is not a Badge"));
        buttons.insert(name, item);
    }

    QQuickItem *backspace = root->findChild<QQuickItem *>(QStringLiteral("backspace"));
    QVERIFY(backspace);

    QQmlComponent calcOld(&engine, QUrl::fromLocalFile(moduleDir + "/CalculatorButton.qml"));
    QVERIFY2(calcOld.isReady(), qPrintable(calcOld.errorString()));
    QQmlComponent calcNew(&engine, QUrl::fromLocalFile(patchedDir + "/CalculatorButton.qml"));
    QVERIFY2(calcNew.isReady(), qPrintable(calcNew.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&calcOld)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&calcNew)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    // Every button must keep its (extra-file) Badge contentItem, and only that one.
    const qsizetype freshChildCount =
            backspace->findChildren<QQuickItem *>(Qt::FindDirectChildrenOnly).size();
    for (const QString &name : calcButtonNames) {
        QVERIFY2(!buttons.value(name).isNull(), qPrintable(name + " was deleted"));
        QQuickItem *content = contentItemOf(buttons.value(name));
        QVERIFY2(content, qPrintable(name + " lost its contentItem on reload"));
        QVERIFY2(content->property("tint").isValid(),
                 qPrintable(name + " content is no longer a Badge"));
        const auto children =
                buttons.value(name)->findChildren<QQuickItem *>(Qt::FindDirectChildrenOnly);
        QCOMPARE(children.size(), freshChildCount);
    }

    // Digit buttons show the new default (green); operator buttons keep their override.
    for (const QString &name : digitButtons)
        QCOMPARE(contentItemOf(buttons.value(name))->property("tint").value<QColor>(),
                 QColor::fromString("green"));
    for (const QString &name : operatorButtons)
        QCOMPARE(contentItemOf(buttons.value(name))->property("tint").value<QColor>(),
                 QColor::fromString("#2CDE85"));
}

void tst_QQmlPreviewObjectPatch::reloadDeferredContentWithAlias()
{
    const QString moduleDir = dataDirectory() + QStringLiteral("/HotReloadAlias");
    const QString patchedDir = dataDirectory() + QStringLiteral("/HotReloadAliasPatched");

    QQuickWindow window;
    window.resize(400, 400);

    QQmlComponent numberPad(&engine, QUrl::fromLocalFile(moduleDir + "/NumberPad.qml"));
    QVERIFY2(numberPad.isReady(), qPrintable(numberPad.errorString()));
    std::unique_ptr<QObject> root(numberPad.create());
    QVERIFY(root);

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(root.get());
    QVERIFY(rootItem);
    rootItem->setParentItem(window.contentItem());
    QCoreApplication::processEvents();

    const auto labelOf = [](QQuickItem *button) {
        return button->property("label").value<QQuickItem *>();
    };

    const QStringList names = { QStringLiteral("d7"), QStringLiteral("d8"), QStringLiteral("d9"),
                                QStringLiteral("d4") };
    QHash<QString, QPointer<QQuickItem>> buttons;
    for (const QString &name : names) {
        QQuickItem *item = root->findChild<QQuickItem *>(name);
        QVERIFY2(item, qPrintable(name));
        // The alias resolves to the deferred content's child.
        QVERIFY2(labelOf(item), qPrintable(name + " alias 'label' is null before reload"));
        buttons.insert(name, item);
    }

    // A button whose aliased deferred content is customized from the using file.
    QPointer<QQuickItem> ext(root->findChild<QQuickItem *>(QStringLiteral("ext")));
    QVERIFY(ext);
    QVERIFY(labelOf(ext));
    QCOMPARE(labelOf(ext)->property("color").value<QColor>(), QColor::fromString("cyan"));

    QQmlComponent calcOld(&engine, QUrl::fromLocalFile(moduleDir + "/CalculatorButton.qml"));
    QVERIFY2(calcOld.isReady(), qPrintable(calcOld.errorString()));
    QQmlComponent calcNew(&engine, QUrl::fromLocalFile(patchedDir + "/CalculatorButton.qml"));
    QVERIFY2(calcNew.isReady(), qPrintable(calcNew.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&calcOld)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&calcNew)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    // The alias into the recreated deferred content must resolve to the new child
    // (not dangle), and that child must show the reloaded green color.
    for (const QString &name : names) {
        QVERIFY2(!buttons.value(name).isNull(), qPrintable(name + " was deleted"));
        QQuickItem *label = labelOf(buttons.value(name));
        QVERIFY2(label, qPrintable(name + " alias 'label' dangled after reload"));
        QCOMPARE(label->property("color").value<QColor>(), QColor::fromString("green"));

        // The alias target must be the actual, live contentItem child (the state
        // that targets it via the same id must therefore work too).
        QQuickItem *content = buttons.value(name)->property("contentItem").value<QQuickItem *>();
        QVERIFY(content);
        QCOMPARE(label->parentItem(), content);

        // The recreated deferred content's binding must be live, not frozen: changing
        // the source property propagates to the content.
        buttons.value(name)->setProperty("textColor", QColor(Qt::red));
        QCOMPARE(label->property("color").value<QColor>(), QColor(Qt::red));

        // A state whose PropertyChanges targets the (recreated) deferred content by
        // id must still apply after the reload.
        QCOMPARE(label->property("scale").toReal(), 1.0);
        buttons.value(name)->setProperty("active", true);
        QCOMPARE(label->property("scale").toReal(), 0.5);
        buttons.value(name)->setProperty("active", false);
        QCOMPARE(label->property("scale").toReal(), 1.0);
    }

    // The external customization on the recreated deferred content must survive.
    QVERIFY(!ext.isNull());
    QQuickItem *extLabel = labelOf(ext);
    QVERIFY2(extLabel, "ext alias 'label' dangled after reload");
    QCOMPARE(extLabel->property("color").value<QColor>(), QColor::fromString("cyan"));
}

static int attachedAmount(QObject *object)
{
    auto *attached = qobject_cast<DeferredAttached *>(
            qmlAttachedPropertiesObject<DeferredAttached>(object, false));
    return attached ? attached->amount() : -42;
}

void tst_QQmlPreviewObjectPatch::reloadDeferredProperty_data()
{
    QTest::addColumn<QString>("moduleName");
    QTest::addColumn<DeferredProbes>("probes");

    const auto readProperty = [](const char *name) {
        return [name](QObject *w) -> qreal { return w->property(name).toReal(); };
    };

    // A deferred value-type binding (amount: base) assigns a value rather than creating an
    // object; the reloaded value must be re-applied.
    QTest::newRow("value") << QStringLiteral("HotReloadDeferredValue")
                           << DeferredProbes{ { readProperty("amount"), 100, 200, 333 } };

    // A deferred attached-property binding (DeferredAttached.amount: base) that resolves to no
    // QQmlPropertyData and lands in DeferredData under the key -1.
    QTest::newRow("attached") << QStringLiteral("HotReloadDeferredAttached")
                              << DeferredProbes{ { [](QObject *w) -> qreal {
                                                      return attachedAmount(w);
                                                  },
                                                   100, 200, 333 } };

    // A deferred generalized grouped-property binding (sibling.x: base) whose first chain part
    // is an id. Like the attached case it lands under the key -1; resetting the old id-named
    // binding on rebuild must not assert.
    QTest::newRow("group") << QStringLiteral("HotReloadDeferredGroup")
                           << DeferredProbes{ { [](QObject *w) -> qreal {
                                                   QObject *sibling = w->findChild<QObject *>(
                                                           QStringLiteral("sibling"));
                                                   return sibling ? sibling->property("x").toReal()
                                                                  : -42;
                                               },
                                                100, 200, 333 } };

    // Like "group", but the binding's left-hand side is retargeted (self.px -> self.py). The
    // recreated host applies the new binding (py); the old left-hand side (px) is never
    // assigned and stays at its C++ default (0), before and after the reload.
    QTest::newRow("groupRetargeted") << QStringLiteral("HotReloadDeferredGroupExt")
                                     << DeferredProbes{ { readProperty("px"), 100, 0, 0 },
                                                        { readProperty("py"), 0, 200, 333 } };
}

void tst_QQmlPreviewObjectPatch::reloadDeferredProperty()
{
    QFETCH(QString, moduleName);
    QFETCH(DeferredProbes, probes);

    const QString moduleDir = dataDirectory() + QStringLiteral("/") + moduleName;
    const QString patchedDir = moduleDir + QStringLiteral("Patched");

    QQmlComponent pad(&engine, QUrl::fromLocalFile(moduleDir + "/Pad.qml"));
    QVERIFY2(pad.isReady(), qPrintable(pad.errorString()));
    std::unique_ptr<QObject> root(pad.create());
    QVERIFY(root);

    const QStringList names = { QStringLiteral("w1"), QStringLiteral("w2"), QStringLiteral("w3") };
    for (const QString &name : names) {
        QObject *w = root->findChild<QObject *>(name);
        QVERIFY2(w, qPrintable(name));
        // The deferred binding ran during construction.
        for (const DeferredProbe &probe : probes)
            QCOMPARE(probe.read(w), probe.before);
    }

    QQmlComponent widgetOld(&engine, QUrl::fromLocalFile(moduleDir + "/Widget.qml"));
    QVERIFY2(widgetOld.isReady(), qPrintable(widgetOld.errorString()));
    QQmlComponent widgetNew(&engine, QUrl::fromLocalFile(patchedDir + "/Widget.qml"));
    QVERIFY2(widgetNew.isReady(), qPrintable(widgetNew.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&widgetOld)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&widgetNew)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());

    // If the patched Widget adds a child, that structural change forces a full rebuild rather
    // than an in-place patch, recreating each Widget through the normal creation path (which
    // re-arms and re-executes the deferred binding). The value case has no such change and is
    // patched in place. Either way the deferred binding must end up re-applied.
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    // Re-find the (possibly recreated) widgets. The deferred binding must be re-applied to the
    // reloaded value, and must stay live when the source property changes.
    for (const QString &name : names) {
        QObject *w = root->findChild<QObject *>(name);
        QVERIFY2(w, qPrintable(name + " missing after reload"));
        for (const DeferredProbe &probe : probes)
            QCOMPARE(probe.read(w), probe.afterReload);
        w->setProperty("base", 333);
        for (const DeferredProbe &probe : probes)
            QCOMPARE(probe.read(w), probe.afterLive);
    }
}

void tst_QQmlPreviewObjectPatch::groupPropertyNameClashesWithId()
{
    const QString moduleDir = dataDirectory() + QStringLiteral("/HotReloadGroupIdClash");
    const QString patchedDir = dataDirectory() + QStringLiteral("/HotReloadGroupIdClashPatched");

    QQmlComponent pad(&engine, QUrl::fromLocalFile(moduleDir + "/Pad.qml"));
    QVERIFY2(pad.isReady(), qPrintable(pad.errorString()));
    std::unique_ptr<QObject> root(pad.create());
    QVERIFY(root);

    const QStringList names = { QStringLiteral("w1"), QStringLiteral("w2"), QStringLiteral("w3") };
    QHash<QString, QPointer<QObject>> widgets;
    for (const QString &name : names) {
        QObject *w = root->findChild<QObject *>(name);
        QVERIFY2(w, qPrintable(name));
        // "spot.x: 100" set the x of the host's QPoint "spot" property, not the id object.
        QCOMPARE(w->property("spot").toPoint().x(), 100);
        QObject *spotItem = w->findChild<QObject *>(QStringLiteral("spotItem"));
        QVERIFY(spotItem);
        QCOMPARE(spotItem->property("x").toReal(), 7);
        widgets.insert(name, w);
    }

    QQmlComponent widgetOld(&engine, QUrl::fromLocalFile(moduleDir + "/Widget.qml"));
    QVERIFY2(widgetOld.isReady(), qPrintable(widgetOld.errorString()));
    QQmlComponent widgetNew(&engine, QUrl::fromLocalFile(patchedDir + "/Widget.qml"));
    QVERIFY2(widgetNew.isReady(), qPrintable(widgetNew.errorString()));

    const auto oldExecUnit = QQmlComponentPrivate::get(&widgetOld)->compilationUnit();
    const auto newExecUnit = QQmlComponentPrivate::get(&widgetNew)->compilationUnit();
    QVERIFY(oldExecUnit && newExecUnit);

    auto objects = objectsForCompilationUnit(&engine, oldExecUnit);
    QVERIFY(!objects.empty());
    QVERIFY(updateObjects(objects, oldExecUnit, newExecUnit));

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    for (const QString &name : names) {
        QVERIFY2(!widgets.value(name).isNull(), qPrintable(name + " was deleted"));
        QObject *w = widgets.value(name);

        // The patched "spot.x: 200" must land on the host's value property...
        QCOMPARE(w->property("spot").toPoint().x(), 200);
        // ... and the object with id "spot" must be untouched (its own x stays 7).
        QObject *spotItem = w->findChild<QObject *>(QStringLiteral("spotItem"));
        QVERIFY(spotItem);
        QCOMPARE(spotItem->property("x").toReal(), 7);
    }
}

QTEST_MAIN(tst_QQmlPreviewObjectPatch)

#include "tst_qqmlpreviewobjectpatch.moc"
