// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmlpreviewobjectpatch_p.h"

#include <private/qqmlcontextdata_p.h>
#include <private/qqmldata_p.h>
#include <private/qqmljavascriptexpression_p.h>
#include <private/qqmlobjectcreator_p.h>
#include <private/qqmlpreviewbindingpatchcontext_p.h>
#include <private/qqmlscriptdata_p.h>
#include <private/qqmlvme_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qv4resolvedtypereference_p.h>

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

    while (typeRef && (typeRef->type().isComposite() || typeRef->type().isInlineComponentType())) {
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
        if (typeRef->type().isInlineComponentType()) {
            icName = typeRef->type().elementName();
            rootIndex = cu->inlineComponentId(icName);
        } else {
            rootIndex = 0;
        }

        levels.push_back({ oldCu, cu, rootIndex, icName, nullptr });

        if (rootIndex >= cu->objectCount())
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
    patchCtx.reset(unitsToUnparent);

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

    for (auto it = levels.crbegin(), end = levels.crend(); it != end; ++it) {
        it->context->addOwnedObject(ddata);
        QQmlPropertyCache::ConstPtr cache = it->newCu->propertyCachesPtr()->at(it->objectIndex);
        new QQmlVMEMetaObject(v4, object, cache, it->newCu, it->objectIndex);
    }

    outerContext->addOwnedObject(ddata);
    if (QQmlPropertyCacheVector *caches = instanceLevel.newCu->propertyCachesPtr();
        caches->count() > instanceLevel.objectIndex
        && caches->needsVMEMetaObject(instanceLevel.objectIndex)) {
        QQmlPropertyCache::ConstPtr cache = caches->at(instanceLevel.objectIndex);
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

bool applyDiff(std::vector<QObject *> &objects,
               const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
               const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    // Ensure the new CU's runtime data (strings, lookups, functions) is populated.
    if (!newUnit->runtimeStrings)
        newUnit->populate();

    // For now we always rebuild whole component roots: the document root (index 0) and any
    // inline-component roots. We don't touch other objects directly. Rebuilding a root runs a
    // full reset() + repopulateBindings() that cascades down the entire instantiation (and
    // through its composite base levels), recreating every object below it. Since every
    // non-root object is, by construction, a descendant of a component root, this is
    // sufficient.
    //
    // A consequence worth remembering: because every live object is recreated, every
    // QQmlJavaScriptExpression on those objects is recreated too, freshly bound to newUnit. By
    // the time refreshBindings() runs, the only expressions still referencing oldUnit are the
    // dead, detached leftovers of the resets. That is exactly why refreshBindings() can simply
    // disable (null) them instead of remapping them to newUnit.
    //
    // When we switch to partial patching -- leaving objects the diff did not affect in place
    // and only remapping their compilation unit instead of rebuilding them -- this invariant
    // breaks and two things need care:
    //
    //   * Affectedness: we need to classify each object as unaffected (keep it, just remap its
    //     CU), patchable (only constants or bindings have changed; no structural changes),
    //     needing VME rebuild, or having a changed non-composite (C++) base type (cannot
    //     be patched in place; its instantiating component must recreate it). The
    //     CompilationUnitDiff carries this information. We'll need to compute it from the
    //     given compilation units.
    //
    //   * refreshBindings(): an object left in place keeps its original expressions, whose
    //     function() still points into oldUnit->runtimeFunctions. Disabling those would kill
    //     bindings that are supposed to keep working. Each such expression must instead be
    //     remapped to the function at the same index in newUnit->runtimeFunctions, i.e.
    //     refreshBindings() has to take newUnit again and translate functions rather than null
    //     them. As an additional complication, we must recognize that adding or removing functions
    //     shifts function indices. So the function a binding referred to in the old compilation
    //     unit may have a different index in the new one.
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
                return false;
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

    for (QObject *object : objects) {
        QQmlData *ddata = QQmlData::get(object);
        if (ddata->compilationUnit == newUnit)
            continue;
        if (ddata->compilationUnit != oldUnit)
            continue;

        // Only remap objects whose cuObjectIndex is still valid in the new CU.
        // Objects whose index is out of range are obsolete (they belonged to
        // the old type definition and have no counterpart in the new one).
        if (ddata->cuObjectIndex >= newUnit->objectCount()) {
            // Null the VME's compilation unit so stale alias lookups
            // (triggered by refreshBindings) safely return nullptr from
            // findCompiledObject() instead of asserting.
            if (ddata->hasVMEMetaObject) {
                for (auto *vmeMeta = static_cast<QQmlVMEMetaObject *>(
                             QObjectPrivate::get(object)->metaObject);
                     vmeMeta; vmeMeta = vmeMeta->parentVMEMetaObject()) {
                    if (vmeMeta->compilationUnit() == oldUnit)
                        vmeMeta->setCompilationUnit(nullptr);
                }
            }
            continue;
        }

        ddata->compilationUnit = newUnit;
        if (!ddata->hasVMEMetaObject)
            continue;

        for (QQmlVMEMetaObject *vmeMeta =
                     static_cast<QQmlVMEMetaObject *>(QObjectPrivate::get(object)->metaObject);
             vmeMeta; vmeMeta = vmeMeta->parentVMEMetaObject()) {
            if (vmeMeta->compilationUnit() == oldUnit
                && vmeMeta->qmlObjectId() < newUnit->objectCount()) {
                vmeMeta->setCompilationUnit(newUnit);
            }
        }
    }
    return true;
}

// Remove any function of the old CU from a QQmlJavaScriptExpression.
// Returns true if the function has been cleared or there was none to begin with.
static bool
clearOldExpressionFunction(QQmlJavaScriptExpression *expr,
                           const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit)
{
    QV4::Function *f = expr->function();
    if (!f)
        return true;

    if (f->executableCompilationUnit() != oldUnit.data())
        return false;

    expr->setFunction(nullptr);
    return true;
}

static void
cleanAndRefreshExpressionsRecursive(const QQmlRefPointer<QQmlContextData> &context,
                                     const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit)
{
    for (auto child = context->childContexts(); child; child = child->nextChild())
        cleanAndRefreshExpressionsRecursive(child, oldUnit);

    for (auto *expr = context->expressions(); expr; expr = expr->nextExpression()) {
        if (!clearOldExpressionFunction(expr, oldUnit))
            expr->refresh();
    }
}

void refreshBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit)
{
    cleanAndRefreshExpressionsRecursive(
            QQmlContextData::get(oldUnit->engine->qmlEngine()->rootContext()), oldUnit);
}

} // namespace QQmlPreview

QT_END_NAMESPACE
