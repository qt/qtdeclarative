// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmlpreviewobjectpatch_p.h"

#include <private/qqmlcomponent_p.h>
#include <private/qqmlcontextdata_p.h>
#include <private/qqmldata_p.h>
#include <private/qqmljavascriptexpression_p.h>
#include <private/qqmlobjectcreator_p.h>
#include <private/qqmlpreviewbindingpatchcontext_p.h>
#include <private/qqmlpreviewdiff_p.h>
#include <private/qqmlpropertyresolver_p.h>
#include <private/qqmlscriptdata_p.h>
#include <private/qqmlvme_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qv4resolvedtypereference_p.h>

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlproperty.h>

#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE

namespace QQmlPreview {

// The indices at which object appears in oldUnit: its own (ddata) index plus any indices it
// occupies through composite base levels (the VME meta-object chain).
static QVarLengthArray<int, 4>
objectIndices(QObject *object, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit)
{
    QVarLengthArray<int, 4> objectIndices;

    QQmlData *ddata = QQmlData::get(object);
    if (ddata->compilationUnit == oldUnit)
        objectIndices.push_back(ddata->cuObjectIndex);
    if (ddata->hasVMEMetaObject) {
        for (QQmlVMEMetaObject *vme =
                     static_cast<QQmlVMEMetaObject *>(QObjectPrivate::get(object)->metaObject);
             vme; vme = vme->parentVMEMetaObject()) {
            if (vme->compilationUnit() == oldUnit)
                objectIndices.push_back(vme->qmlObjectId());
        }
    }

    return objectIndices;
}

static QQmlPropertyCache::ConstPtr
nonCompositeBaseType(const QQmlPropertyCache::ConstPtr &propertyCache)
{
    for (QQmlPropertyCache::ConstPtr parent = propertyCache; parent; parent = parent->parent()) {
        if (!parent->isComposite())
            return parent;
    }

    return QQmlPropertyCache::ConstPtr();
}

static bool
hasChangedNonCompositeBaseType(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                               const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit,
                               int objectIndex)
{
    const QV4::CompiledData::Object *oldObj = oldUnit->objectAt(objectIndex);
    const auto *oldTypeRef = oldUnit->resolvedType(oldObj->inheritedTypeNameIndex);
    if (!oldTypeRef)
        return false; // Group property sub-objects have no inherited type.

    const QV4::CompiledData::Object *newObj = newUnit->objectAt(objectIndex);
    const auto *newTypeRef = newUnit->resolvedType(newObj->inheritedTypeNameIndex);
    if (!newTypeRef)
        return true; // Type disappeared — definitely changed.

    return nonCompositeBaseType(oldTypeRef->typePropertyCache())
            != nonCompositeBaseType(newTypeRef->typePropertyCache());
}

// Re-resolve the object's precomputed binding-target table against its relinked property cache.
static void refreshBindingPropertyData(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &cu,
                                       int objectIndex, const QQmlPropertyCache::ConstPtr &cache)
{
    QList<QV4::CompiledData::BindingPropertyData> &table =
            cu->baseCompilationUnit()->bindingPropertyDataPerObject;
    if (objectIndex >= table.size())
        return;

    QV4::CompiledData::BindingPropertyData &bindingData = table[objectIndex];

    const QV4::CompiledData::Object *obj = cu->objectAt(objectIndex);
    const QQmlPropertyResolver resolver(cache);

    const QV4::CompiledData::Binding *binding = obj->bindingTable();
    for (qsizetype i = 0, end = bindingData.size(); i < end; ++i, ++binding) {
        if (!bindingData.at(i))
            continue;

        Q_ASSERT(i < obj->nBindings);
        const QString name = BindingPatchContext::targetPropertyName(cu, objectIndex, binding);
        if (name.isEmpty())
            continue;

        bindingData[i] =
                (binding->hasFlag(QV4::CompiledData::Binding::IsSignalHandlerExpression)
                 || binding->hasFlag(QV4::CompiledData::Binding::IsSignalHandlerObject))
                ? resolver.signal(name, nullptr, QQmlPropertyResolver::IgnoreRevision)
                : resolver.property(name, nullptr, QQmlPropertyResolver::IgnoreRevision);
    }
}

// Ensure the property cache at objectIndex in cu is derived from actualParent, the cache we just
// used for the level below it in the VME chain.
static QQmlPropertyCache::ConstPtr
relinkCache(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &cu, int objectIndex,
            const QQmlPropertyCache::ConstPtr &actualParent)
{
    QQmlPropertyCacheVector *caches = cu->propertyCachesPtr();
    QQmlPropertyCache::ConstPtr cache = caches->at(objectIndex);

    // The bottom-most composite level's own base is a non-composite (C++) type. That base never
    // changes across a reload since a changed non-composite base is rejected by
    // hasChangedNonCompositeBaseType.
    if (!actualParent)
        return cache;

    // A type that needs no VME meta-object of its own reuses its base type's property cache.
    if (!caches->needsVMEMetaObject(objectIndex)) {
        if (cache != actualParent) {
            caches->set(objectIndex, actualParent);
            refreshBindingPropertyData(cu, objectIndex, actualParent);
        }
        return actualParent;
    }

    // A type with its own cache derived from the base: re-derive it from the relinked base so its
    // inherited offsets match the (possibly changed) base layout.
    if (cache->parent() != actualParent) {
        cache = cache->rebased(actualParent);
        caches->set(objectIndex, cache);
        refreshBindingPropertyData(cu, objectIndex, cache);
    }

    return cache;
}

struct ObjectAndIndex
{
    QObject *object = nullptr;
    int index = -1;
};

// Walk the type resolution chain starting from the object at cuIndex in unit,
// collecting all composite (QML-defined) base type levels. Returns them ordered
// deepest-first (e.g., grandparent before parent).
static std::vector<CompositeLevel>
collectCompositeLevels(const CompositeLevel &instanceLevel,
                       const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                       const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    std::vector<CompositeLevel> levels;

    auto currentUnit = instanceLevel.newCu;
    const QV4::CompiledData::Object *obj = currentUnit->objectAt(instanceLevel.objectIndex);
    const QV4::ResolvedTypeReference *typeRef =
            currentUnit->resolvedType(obj->inheritedTypeNameIndex);

    while (typeRef && typeRef->type().isComposite()) {
        QQmlRefPointer<QV4::ExecutableCompilationUnit> cu = typeRef->isSelfReference()
                ? currentUnit
                : currentUnit->engine->executableCompilationUnit(typeRef->compilationUnit());
        Q_ASSERT(cu);

        // Replace the old unit with the new one wherever it occurs
        const auto oldCu = cu;
        if (cu == oldUnit)
            cu = newUnit;

        int rootIndex;
        QString icName;
        if (typeRef->type().isInlineComponent()) {
            icName = typeRef->type().elementName();
            rootIndex = cu->inlineComponentId(icName);
        } else {
            rootIndex = 0;
        }

        levels.push_back({ oldCu, cu, rootIndex, icName, nullptr });

        if (rootIndex < 0 || rootIndex >= cu->objectCount())
            break;

        // Walk deeper into the base type
        const QV4::CompiledData::Object *rootObj = cu->objectAt(rootIndex);
        typeRef = cu->resolvedType(rootObj->inheritedTypeNameIndex);
        currentUnit = cu;
    }

    return levels;
}

// cuIndex is not necessarily the "outermost" index. There may be levels above oldUnit.
// Those have to be preserved/rebuilt.
static void rebuildObject(QObject *object, int cuIndex,
                          const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                          const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    // If the object's index doesn't exist in the new CU, it's obsolete.
    if (cuIndex >= newUnit->objectCount())
        return;

    QQmlData *ddata = QQmlData::get(object);
    Q_ASSERT(ddata);

    QQmlRefPointer<QQmlContextData> outerContext =
            QQmlRefPointer<QQmlContextData>(ddata->outerContext);

    // If the object has no context, or is scheduled for deletion, it's half-dead already.
    if (!ddata->context || !outerContext || !outerContext->isValid() || ddata->isQueuedForDeletion)
        return;

    CompositeLevel instanceLevel{ ddata->compilationUnit,
                                  ddata->compilationUnit == oldUnit ? newUnit
                                                                    : ddata->compilationUnit,
                                  ddata->cuObjectIndex, QString(), outerContext };

    // If the object doesn't exist anymore in the new CU it will be deleted via GC.
    // Nothing to do here.
    if (instanceLevel.objectIndex >= instanceLevel.newCu->objectCount())
        return;

    std::vector<CompositeLevel> levels = collectCompositeLevels(instanceLevel, oldUnit, newUnit);

    // Build the set of compilation units that participate in this rebuild.
    // Bindings from these CUs are "internal" and will be re-created by repopulateBindings.
    std::vector<CompositeLevel> internalUnits;
    internalUnits.push_back(instanceLevel);
    for (const auto &level : levels)
        internalUnits.push_back(level);

    BindingPatchContext patchCtx(object, ddata->compilationUnit, ddata->cuObjectIndex);
    QDuplicateTracker<QObject *> seenChildren;
    patchCtx.stashExternalState(internalUnits, &seenChildren);
    // Collect the CUs whose children will be recreated by repopulateBindings.
    // This is oldUnit (being replaced) and the composite-level CUs.
    // NOT newUnit (already-rebuilt objects pointing here must be preserved)
    // and NOT instanceLevel.cu (its repopulateBindings only sets bindings, not children).
    std::vector<QQmlRefPointer<QV4::ExecutableCompilationUnit>> unitsToUnparent;
    unitsToUnparent.push_back(oldUnit);
    for (const auto &level : levels)
        unitsToUnparent.push_back(level.oldCu);
    patchCtx.reset(unitsToUnparent, internalUnits);

    QV4::ExecutionEngine *v4 = newUnit->engine;
    Q_ASSERT(v4);
    QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(v4);
    Q_ASSERT(enginePrivate);

    if (outerContext->contextObject() == object) {
        outerContext->setContextObject(nullptr);
        outerContext = enginePrivate->createComponentRootContext(
                instanceLevel.newCu, outerContext->parent(), instanceLevel.objectIndex);
        outerContext->setContextObject(object);
        instanceLevel.context = outerContext;
    }

    for (QQmlRefPointer<QQmlContextData> ctx(ddata->context); ctx; ctx = ctx->linkedContext()) {
        if (ctx->contextObject() == object)
            ctx->setContextObject(nullptr);
    }

    ddata->clear();

    QObjectPrivate *objectPrivate = QObjectPrivate::get(object);
    delete std::exchange(objectPrivate->metaObject, nullptr);

    QQmlRefPointer<QQmlContextData> levelContext = outerContext;
    for (qsizetype i = 0, end = levels.size(); i < end; ++i) {
        CompositeLevel &level = levels[i];
        levelContext = level.context = enginePrivate->createComponentRootContext(
                level.newCu, levelContext, level.objectIndex);
        levelContext->setContextObject(object);
    }

    if (outerContext->contextObject() == object) {
        ddata->ownContext = outerContext;
        ddata->context = outerContext.data();
        if (!levels.empty())
            ddata->context->setLinkedContext(levels.front().context);
    } else if (!levels.empty()) {
        ddata->ownContext = levels.back().context;
        ddata->context = ddata->ownContext.data();
        if (levels.size() > 1)
            ddata->context->setLinkedContext(levels.front().context);
    } else {
        // Non-root child: doesn't own a context. Re-link to the parent context.
        ddata->context = outerContext.data();
    }

    // Build the VME meta-object chain base-first, relinking each level's property cache to the
    // actual (possibly freshly reloaded) parent cache we just used, so the whole chain's offsets
    // stay consistent even when a composite base type's layout changed on reload.
    QQmlPropertyCache::ConstPtr parentCache;
    for (auto it = levels.crbegin(), end = levels.crend(); it != end; ++it) {
        it->context->addOwnedObject(ddata);
        QQmlPropertyCache::ConstPtr cache = relinkCache(it->newCu, it->objectIndex, parentCache);
        if (it->newCu->propertyCachesPtr()->needsVMEMetaObject(it->objectIndex))
            new QQmlVMEMetaObject(v4, object, cache, it->newCu, it->objectIndex);
        parentCache = cache;
    }

    outerContext->addOwnedObject(ddata);
    if (QQmlPropertyCacheVector *caches = instanceLevel.newCu->propertyCachesPtr();
        caches->count() > instanceLevel.objectIndex) {
        QQmlPropertyCache::ConstPtr cache =
                relinkCache(instanceLevel.newCu, instanceLevel.objectIndex, parentCache);
        if (caches->needsVMEMetaObject(instanceLevel.objectIndex))
            new QQmlVMEMetaObject(v4, object, cache, instanceLevel.newCu, instanceLevel.objectIndex);
    }

    // Repopulate bindings at each composite level (deepest first).
    // This sets up functions, evaluates bindings (creating child objects), etc.
    // The object may get queued for deletion as result of some bindings.
    // In that case we have to abort.
    for (auto it = levels.crbegin(), end = levels.crend(); it != end && !ddata->isQueuedForDeletion;
         ++it) {
        QQmlObjectCreator creator(it->context, it->newCu, outerContext, it->icName, nullptr);
        creator.repopulateBindings(it->objectIndex, object, it->context,
                                   QQmlObjectCreator::InitFlag::IsContextObject
                                           | QQmlObjectCreator::InitFlag::IsDocumentRoot);

        QQmlInstantiationInterrupt interrupt;
        creator.finalize(interrupt);
    }

    // The object may get queued for deletion as result of some bindings.
    // In that case don't touch it any further.
    if (!ddata->isQueuedForDeletion) {
        // Repopulate bindings at the instance level in the parent CU.
        QQmlObjectCreator creator(instanceLevel.context, instanceLevel.newCu, outerContext, QString(),
                                  nullptr);
        creator.repopulateBindings(instanceLevel.objectIndex, object, outerContext,
                                   QQmlObjectCreator::InitFlag::None);
        QQmlInstantiationInterrupt interrupt;
        creator.finalize(interrupt);

        // Restore externally set bindings and values that were stashed before reset.
        // First refresh child object pointers — they may have been replaced during rebuild.
        patchCtx.refreshObjects();
        patchCtx.restoreExternalState();
    }
}

static bool isPatchableBinding(const QV4::CompiledData::Binding *binding)
{
    // Only plain value and script bindings can be patched in place. A change to the object,
    // group or attached-property binding itself reassigns a sub-object: that is structural.

    switch (binding->type()) {
    case QV4::CompiledData::Binding::Type_Script:
    case QV4::CompiledData::Binding::Type_Number:
    case QV4::CompiledData::Binding::Type_Boolean:
    case QV4::CompiledData::Binding::Type_String:
    case QV4::CompiledData::Binding::Type_Null:
    case QV4::CompiledData::Binding::Type_Translation:
    case QV4::CompiledData::Binding::Type_TranslationById:
        return true;
    default:
        break;
    }

    return false;
}

// A trivial diff is one we can apply without rebuilding any VME meta-object and without
// adding or removing objects: only existing bindings and binding-expression bodies change. Such a
// diff is patched in place (see patchInPlace()). Everything else falls back to rebuilding roots.
static bool changeIsTrivial(const QV4::CompiledData::Change &change,
                            const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                            const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    using ChangeType = QV4::CompiledData::ChangeType;
    switch (change.type) {
    // Source-location-only changes have no runtime effect.
    case ChangeType::AliasLocationChanged:
    case ChangeType::BindingLocationChanged:
    case ChangeType::EnumLocationChanged:
    case ChangeType::FunctionLocationChanged:
    case ChangeType::ImportLocationChanged:
    case ChangeType::InlineComponentLocationChanged:
    case ChangeType::ObjectLocationChanged:
    case ChangeType::PropertyLocationChanged:
    case ChangeType::SignalLocationChanged:
    // Internal table changes that are byproducts of recompiling expressions. They are picked up
    // when we translate functions and remap compilation units; no structure changes.
    case ChangeType::UnitMetadataChanged:
    case ChangeType::ConstantAdded:
    case ChangeType::ConstantChanged:
    case ChangeType::ConstantRemoved:
    case ChangeType::StringDataAdded:
    case ChangeType::StringDataChanged:
    case ChangeType::StringDataRemoved:
    case ChangeType::LookupAdded:
    case ChangeType::LookupChanged:
    case ChangeType::LookupRemoved:
    case ChangeType::RegExpAdded:
    case ChangeType::RegExpChanged:
    case ChangeType::RegExpRemoved:
    case ChangeType::ClassAdded:
    case ChangeType::ClassChanged:
    case ChangeType::ClassRemoved:
    case ChangeType::BlockAdded:
    case ChangeType::BlockChanged:
    case ChangeType::BlockRemoved:
    case ChangeType::TemplateObjectAdded:
    case ChangeType::TemplateObjectChanged:
    case ChangeType::TemplateObjectRemoved:
    case ChangeType::JSClassAdded:
    case ChangeType::JSClassChanged:
    case ChangeType::JSClassRemoved:
    case ChangeType::TranslationDataAdded:
    case ChangeType::TranslationDataChanged:
    case ChangeType::TranslationDataRemoved:
        return true;
    case ChangeType::FunctionChanged:
        // A recompiled function body at a stable index. A binding expression picks up its new body
        // via function translation. (FunctionAdded/Removed shift indices and are rejected below.
        // A changed *method* body additionally produces an ObjectChanged for the object's function
        // offset table, which is rejected below, so method changes still take the rebuild path:
        // their VME function objects are cached and not refreshed by translation.)
        return true;
    case ChangeType::BindingChanged: {
        Q_ASSERT(change.objectIndex >= 0);
        Q_ASSERT(change.objectIndex < oldUnit->objectCount());
        Q_ASSERT(change.objectIndex < newUnit->objectCount());

        const QV4::CompiledData::Object *newObj = newUnit->objectAt(change.objectIndex);
        Q_ASSERT(change.index < newObj->nBindings);

        const QV4::CompiledData::Binding *newBinding = newObj->bindingTable() + change.index;
        if (!isPatchableBinding(newBinding))
            return false;

        // A BindingChanged at a stable index can also mean the binding was moved to a different
        // target property (its propertyNameIndex changed). In-place patching cannot relocate a
        // binding: it would have to remove it from the old property and install it on the new one.
        // That is structural, so fall back to the rebuild path.

        const QV4::CompiledData::Object *oldObj = oldUnit->objectAt(change.objectIndex);
        Q_ASSERT(change.index < oldObj->nBindings);

        const QV4::CompiledData::Binding *oldBinding = oldObj->bindingTable() + change.index;
        if (!isPatchableBinding(oldBinding))
            return false;

        return BindingPatchContext::targetPropertyName(oldUnit, change.objectIndex, oldBinding)
                == BindingPatchContext::targetPropertyName(newUnit, change.objectIndex, newBinding);
    }
    default:
        // ObjectAdded/Removed/Changed, Binding Added/Removed, Property/Signal/Alias/Enum changes,
        // Function Added/Removed, Import/InlineComponent/RequiredPropertyExtraData changes:
        // structural, need a rebuild.
        return false;
    }
}

static bool isTrivialDiff(const QV4::CompiledData::CompilationUnitDiff &diff,
                          const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                          const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    return std::all_of(diff.changes.cbegin(), diff.changes.cend(),
                       [&](const QV4::CompiledData::Change &change) {
                           return changeIsTrivial(change, oldUnit, newUnit);
                       });
}

// Re-apply changed literal bindings. A literal value is written only if the property still holds
// the value the old unit assigned; anything else is an external override we must leave untouched.
// Changed script bindings carry no literal value and are handled by function translation instead.
//
// The binding may live on a value-type group sub-object (font.pixelSize) or an attached object
// (Keys.enabled), which has no standalone QObject at its compilation-unit index. We use
// BindingPatchContext to map every compilation-unit object index reachable from a live instance to
// the owning QObject and the property-name prefix to address it.
static void patchConstantBindings(const std::vector<QObject *> &objects,
                                  const QV4::CompiledData::CompilationUnitDiff &diff,
                                  const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                                  const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    for (QObject *object : objects) {
        // Skip half-dead objects, mirroring rebuildObject(): an invalidated context means the
        // object is on its way out and must not be patched.
        QQmlData *ddata = QQmlData::get(object);
        if (!ddata->context || !ddata->outerContext || !ddata->outerContext->isValid()
            || ddata->isQueuedForDeletion) {
            continue;
        }

        QVarLengthArray<BindingPatchContext, 4> contexts;
        for (int index : objectIndices(object, oldUnit))
            contexts.append(BindingPatchContext(object, oldUnit, index));

        for (const QV4::CompiledData::Change &change : diff.changes) {
            if (change.type != QV4::CompiledData::ChangeType::BindingChanged)
                continue;

            for (BindingPatchContext &context : contexts) {
                if (context.applyBindingChange(newUnit, change))
                    break;
            }
        }
    }
}

// Point every live object that still references oldUnit at newUnit, in both its ddata and its
// VME meta-object chain. Used after both in-place patching and root rebuilds.
//
// The ddata compilation unit and the VME chain are remapped independently: a composite-type
// instance can carry oldUnit in its VME chain while its ddata->compilationUnit is a different
// executable unit (the "topmost", possibly inaddressible, instantiation).
static void remapObjectsToNewUnit(const std::vector<QObject *> &objects,
                                  const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                                  const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    for (QObject *object : objects) {
        QQmlData *ddata = QQmlData::get(object);

        // A QQmlComponent (e.g. an explicit or implicit `delegate:`) caches the compilation unit it
        // instantiates from. Remap it so future create() calls produce instances from newUnit.
        if (QQmlComponent *component = qobject_cast<QQmlComponent *>(object)) {
            QQmlComponentPrivate *cp = QQmlComponentPrivate::get(component);
            if (cp->compilationUnit() == oldUnit)
                cp->setCompilationUnit(newUnit);
        }

        // Remap the ddata compilation unit if it points at oldUnit. An object whose index is out
        // of range in the new CU is obsolete (it belonged to the old type definition); leave its
        // ddata alone and let the VME nulling below retire it.
        if (ddata->compilationUnit == oldUnit && ddata->cuObjectIndex < newUnit->objectCount())
            ddata->compilationUnit = newUnit;

        if (!ddata->hasVMEMetaObject)
            continue;

        // Remap (or retire) every VME meta-object in the chain that points at oldUnit.
        for (QQmlVMEMetaObject *vmeMeta =
                     static_cast<QQmlVMEMetaObject *>(QObjectPrivate::get(object)->metaObject);
             vmeMeta; vmeMeta = vmeMeta->parentVMEMetaObject()) {
            if (vmeMeta->compilationUnit() != oldUnit)
                continue;
            if (vmeMeta->qmlObjectId() < newUnit->objectCount()) {
                vmeMeta->setCompilationUnit(newUnit);
            } else {
                // Obsolete: null it so stale alias lookups (triggered by refreshBindings) safely
                // return nullptr from findCompiledObject() instead of asserting.
                vmeMeta->setCompilationUnit(nullptr);
            }
        }
    }
}

PatchResult applyDiff(std::vector<QObject *> &objects,
                      const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                      const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    // Ensure the new CU's runtime data (strings, lookups, functions) is populated.
    if (!newUnit->runtimeStrings)
        newUnit->populate();

    // Fast path: if only existing bindings changed -- nothing structural about any VME
    // meta-object, and no objects added or removed -- patch the affected objects in place. We
    // leave every object and its VME meta-object where it is, re-apply the changed literal
    // bindings (patchConstantBindings), point the objects at newUnit (remapObjectsToNewUnit), and
    // let the in-place refreshBindings() translate the still-live expressions' functions to
    // newUnit. Because no functions are added or removed in a trivial diff, function indices are
    // stable, which is what makes that same-index translation correct.
    const QV4::CompiledData::CompilationUnitDiff diff =
            QV4::CompiledData::diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    if (diff.success && isTrivialDiff(diff, oldUnit, newUnit)) {
        patchConstantBindings(objects, diff, oldUnit, newUnit);
        remapObjectsToNewUnit(objects, oldUnit, newUnit);
        return PatchResult::PatchedInPlace;
    }

    // Otherwise we rebuild whole component roots: the document root (index 0) and any
    // inline-component roots. We don't touch other objects directly. Rebuilding a root runs a
    // full reset() + repopulateBindings() that cascades down the entire instantiation (and
    // through its composite base levels), recreating every object below it. Since every
    // non-root object is, by construction, a descendant of a component root, this is
    // sufficient.
    //
    // A consequence worth remembering: because every live object is recreated, every
    // QQmlJavaScriptExpression on those objects is recreated too, freshly bound to newUnit. By
    // the time refreshBindings() runs, the only expressions still referencing oldUnit are the
    // dead, detached leftovers of the resets. That is why the rebuild path's refreshBindings()
    // can simply disable (null) them instead of remapping them to newUnit.
    std::vector<ObjectAndIndex> rebuild;

    for (QObject *object : objects) {
        const QVarLengthArray<int, 4> indices = objectIndices(object, oldUnit);
        for (int index : indices) {
            // Objects instantiated by an enclosing component (indices beyond the CU, explicit
            // Component content) are recreated when that enclosing root is rebuilt, so we don't
            // record them here.
            if (index >= oldUnit->objectCount())
                continue;
            const auto flags = oldUnit->objectAt(index)->flags();
            if (index != 0 && !(flags & QV4::CompiledData::Object::IsInlineComponentRoot))
                continue;

            // A component root whose own non-composite (C++) base type changed cannot be
            // rebuilt in place: reset() + repopulateBindings() reuses the same QObject, which is
            // still of the old C++ class. Bail so the caller falls back to a full reload.
            // (If the index no longer exists in the new CU the root is obsolete; rebuildObject()
            // skips it and the remap loop below retires it.)
            if (index < newUnit->objectCount()
                && hasChangedNonCompositeBaseType(oldUnit, newUnit, index)) {
                return PatchResult::Failed;
            }

            rebuild.push_back({ object, indices.first() });
            break;
        }
    }

    // Sort by ascending cuIndex so that parent objects are rebuilt before their
    // children. A parent's reset() properly retires children (they still point to
    // oldUnit) and repopulateBindings recreates them. This avoids leaking orphaned
    // children and naturally handles cases where the context needs more ID slots.
    std::sort(rebuild.begin(), rebuild.end(),
              [](const ObjectAndIndex &a, const ObjectAndIndex &b) { return a.index < b.index; });

    // Pre-compute which objects to skip: if an ancestor is also in the rebuild
    // list, the child will be properly retired and recreated during the ancestor's
    // rebuild. Rebuilding it individually would be wasted work on a stale pointer.
    QSet<QObject *> skip;
    for (const auto &[object, cuIndex] : rebuild) {
        const QObjectList &children = object->children();
        for (QObject *child : children)
            skip.insert(child);
    }

    for (const auto &[object, cuIndex] : rebuild) {
        if (skip.contains(object))
            continue;

        rebuildObject(object, cuIndex, oldUnit, newUnit);
    }

    remapObjectsToNewUnit(objects, oldUnit, newUnit);
    return PatchResult::Rebuilt;
}

// Translate a still-live expression's function from oldUnit to the function at the same index in
// newUnit, or clear it if there is no newUnit. Expressions whose function does not point into
// oldUnit are left untouched. Same-index translation is valid because we only do this with trivial
// diffs that neither add nor remove functions.
// Returns true if the resulting function is valid afterwards (and should be re-evaluated)
static bool
translateExpressionFunction(QQmlJavaScriptExpression *expr,
                            const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                            const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    QV4::Function *f = expr->function();
    if (!f)
        return false;

    if (f->executableCompilationUnit() != oldUnit.data())
        return true;

    if (newUnit) {
        const qsizetype index = oldUnit->runtimeFunctions.indexOf(f);
        if (index >= 0 && index < newUnit->runtimeFunctions.size())
            expr->setFunction(newUnit->runtimeFunctions[index]);
    } else {
        expr->setFunction(nullptr);
        return false;
    }

    return true;
}

static void translateAndRefreshExpressionsRecursive(
        const QQmlRefPointer<QQmlContextData> &context,
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    for (auto child = context->childContexts(); child; child = child->nextChild())
        translateAndRefreshExpressionsRecursive(child, oldUnit, newUnit);

    for (auto *expr = context->expressions(); expr; expr = expr->nextExpression()) {
        if (translateExpressionFunction(expr, oldUnit, newUnit))
            expr->refresh();
    }
}

void refreshBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                     const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    translateAndRefreshExpressionsRecursive(
            QQmlContextData::get(oldUnit->engine->qmlEngine()->rootContext()), oldUnit, newUnit);
}

} // namespace QQmlPreview

QT_END_NAMESPACE
