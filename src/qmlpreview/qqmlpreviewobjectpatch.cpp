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

enum Severity : quint8 {
    Unaffected,
    Rebuild,
    Replace,
};

static void collectGroupAndAttachedPropertySubObjects(
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit, int objectIndex,
        QVarLengthArray<int, 4> &indices)
{
    const QV4::CompiledData::Object *obj = unit->objectAt(objectIndex);
    const QV4::CompiledData::Binding *binding = obj->bindingTable();
    for (quint32 i = 0; i < obj->nBindings; ++i, ++binding) {
        switch (binding->type()) {
        case QV4::CompiledData::Binding::Type_AttachedProperty:
        case QV4::CompiledData::Binding::Type_GroupProperty: {
            const int subIndex = binding->value.objectIndex;
            indices.push_back(subIndex);
            collectGroupAndAttachedPropertySubObjects(unit, subIndex, indices);
            break;
        }
        default:
            break;
        }
    }
}

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

    // Include indices of group property sub-objects (value types like font, anchors margins, etc.)
    // These are virtual objects in the CU that don't correspond to real QObjects.
    const qsizetype count = objectIndices.size();
    for (qsizetype i = 0; i < count; ++i)
        collectGroupAndAttachedPropertySubObjects(oldUnit, objectIndices[i], objectIndices);

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

static Severity objectAffectedByDiff(const QVarLengthArray<int, 4> &objectIndices,
                                     const QV4::CompiledData::CompilationUnitDiff &diff,
                                     const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                                     const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    Severity result = Unaffected;

    for (const QV4::CompiledData::Change &change : diff.changes) {
        switch (change.type) {
        case QV4::CompiledData::ChangeType::RequiredPropertyExtraDataAdded:
        case QV4::CompiledData::ChangeType::RequiredPropertyExtraDataChanged:
        case QV4::CompiledData::ChangeType::RequiredPropertyExtraDataRemoved:
        case QV4::CompiledData::ChangeType::AliasAdded:
        case QV4::CompiledData::ChangeType::AliasChanged:
        case QV4::CompiledData::ChangeType::AliasRemoved:
        case QV4::CompiledData::ChangeType::BindingAdded:
        case QV4::CompiledData::ChangeType::BindingChanged:
        case QV4::CompiledData::ChangeType::BindingRemoved:
        case QV4::CompiledData::ChangeType::PropertyAdded:
        case QV4::CompiledData::ChangeType::PropertyChanged:
        case QV4::CompiledData::ChangeType::PropertyRemoved:
        case QV4::CompiledData::ChangeType::SignalAdded:
        case QV4::CompiledData::ChangeType::SignalChanged:
        case QV4::CompiledData::ChangeType::SignalRemoved:
            // These are object-specific. Only the object in question needs rebuilding.
            if (!objectIndices.contains(change.objectIndex))
                continue;
            if (result == Unaffected)
                result = Rebuild;
            break;
        case QV4::CompiledData::ChangeType::EnumAdded:
        case QV4::CompiledData::ChangeType::EnumChanged:
        case QV4::CompiledData::ChangeType::EnumRemoved:
        case QV4::CompiledData::ChangeType::FunctionAdded:
        case QV4::CompiledData::ChangeType::FunctionChanged:
        case QV4::CompiledData::ChangeType::FunctionRemoved:
            // These are global. All objects created from the CU need rebuilding
            // TODO: Enum changes actually propagate outside the CU. We'd need to rebuild
            //       absolutely everything that can reach the enum.
            if (result == Unaffected)
                result = Rebuild;
            break;
        case QV4::CompiledData::ChangeType::ImportAdded:
        case QV4::CompiledData::ChangeType::ImportChanged:
        case QV4::CompiledData::ChangeType::ImportRemoved:
            // Changing imports can change everything or nothing. Most of the time you have no
            // conflicting type names. Let's assume it's OK.
            // TODO: To get this right, we need to compare all type resolutions in the whole CU
            //       to see if they're still the same.
            continue;
        case QV4::CompiledData::ChangeType::InlineComponentAdded:
        case QV4::CompiledData::ChangeType::InlineComponentRemoved:
        case QV4::CompiledData::ChangeType::InlineComponentChanged:
            // Adding, removing, or changing an inline component cannot change anything by itself.
            // What we care about is the CU object the inline component refers to.
            continue;
        case QV4::CompiledData::ChangeType::ObjectChanged: {
            if (!objectIndices.contains(int(change.index)))
                continue;

            // If the non-composite base type changes, we need to replace the object
            if (hasChangedNonCompositeBaseType(oldUnit, newUnit, change.index))
                return Replace;

            // If only some flags change, rebuilding is enough.
            if (result == Unaffected)
                result = Rebuild;

            break;
        }
        case QV4::CompiledData::ChangeType::ObjectAdded:
        case QV4::CompiledData::ChangeType::ObjectRemoved:
            // Adding or removing an object only takes effect if you also add or remove
            // a binding to that object, and that causes the outer object to be rebuilt.
            continue;
        default:
            continue;
        }
    }

    return result;
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
        QQmlPropertyCache::ConstPtr cache = newUnit->propertyCachesPtr()->at(cuIndex);
        new QQmlVMEMetaObject(v4, object, cache, newUnit, cuIndex);
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

bool applyDiff(std::vector<QObject *> &objects, const QV4::CompiledData::CompilationUnitDiff &diff,
               const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
               const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    // Ensure the new CU's runtime data (strings, lookups, functions) is populated.
    if (!newUnit->runtimeStrings)
        newUnit->populate();

    std::vector<ObjectAndIndex> rebuild;
    std::vector<ObjectAndIndex> componentRoots;
    bool rebuildOuter = false;

    for (QObject *object : objects) {
        bool isComponentRoot = false;
        const QVarLengthArray<int, 4> indices = objectIndices(object, oldUnit);
        for (int index : indices) {
            if (index >= oldUnit->objectCount()) {
                // Implicit component detected. Rebuild the component around it.
                rebuildOuter = true;
                continue;
            }

            const auto flags = oldUnit->objectAt(index)->flags();
            if (flags & QV4::CompiledData::Object::IsComponent) {
                // Explicit component object. Rebuild the component around it.
                rebuildOuter = true;
                continue;
            }

            if (index == 0 || flags & QV4::CompiledData::Object::IsInlineComponentRoot) {
                componentRoots.push_back({ object, indices.first() });
                isComponentRoot = true;
                break;
            }
        }

        switch (objectAffectedByDiff(indices, diff, oldUnit, newUnit)) {
        case Unaffected:
            // TODO: We don't actually need to rebuild here. We only need to update the context,
            //       ddata, etc. to point to the new compilation unit.
            if (isComponentRoot)
                rebuild.push_back({ object, indices.first() });
            continue;
        case Rebuild:
            rebuild.push_back({ object, indices.first() });
            break;
        case Replace:
            // Can't replace the root object of a component
            // TODO: In some cases we can, by rebuilding the object that instantiated it instead.
            if (isComponentRoot)
                return false;
            rebuildOuter = true;
            break;
        }
    }

    if (rebuildOuter)
        rebuild = std::move(componentRoots);

    // Sort by descending cuIndex so that leaf objects (children) are rebuilt
    // before their parents. This prevents reset() from unparenting objects
    // that are about to be rebuilt themselves.
    std::sort(rebuild.begin(), rebuild.end(),
              [](const ObjectAndIndex &a, const ObjectAndIndex &b) {
                  return a.index > b.index;
              });

    QQmlRefPointer<QQmlContextData> rootContext;
    for (const auto &[object, cuIndex] : rebuild) {
        rebuildObject(object, cuIndex, oldUnit, newUnit);
        QQmlData *rootData = QQmlData::get(object);
        if (rootData)
            rootContext = rootData->ownContext;
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

// Update the m_v4Function of a QQmlJavaScriptExpression from the old CU to the
// corresponding function in the new CU.
static void updateExpressionFunction(QQmlJavaScriptExpression *expr,
                                     const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                                     const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    QV4::Function *f = expr->function();
    if (!f || f->executableCompilationUnit() != oldUnit.data())
        return;

    const auto &oldFunctions = oldUnit->runtimeFunctions;
    const int index = oldFunctions.indexOf(f);
    Q_ASSERT(index >= 0);

    expr->setFunction(index < newUnit->runtimeFunctions.size() ? newUnit->runtimeFunctions[index]
                                                               : nullptr);
}

static void
updateAndRefreshExpressionsRecursive(const QQmlRefPointer<QQmlContextData> &context,
                                     const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                                     const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    for (auto child = context->childContexts(); child; child = child->nextChild())
        updateAndRefreshExpressionsRecursive(child, oldUnit, newUnit);

    for (auto *expr = context->expressions(); expr; expr = expr->nextExpression()) {
        updateExpressionFunction(expr, oldUnit, newUnit);
        expr->refresh();
    }
}

void refreshBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit,
                     const QQmlRefPointer<QV4::ExecutableCompilationUnit> &newUnit)
{
    updateAndRefreshExpressionsRecursive(
            QQmlContextData::get(newUnit->engine->qmlEngine()->rootContext()), oldUnit, newUnit);
}

} // namespace QQmlPreview

QT_END_NAMESPACE
