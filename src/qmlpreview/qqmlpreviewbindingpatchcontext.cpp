// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmlpreviewbindingpatchcontext_p.h"

#include <private/qqmlcomponent_p.h>
#include <private/qqmlnotifier_p.h>
#include <private/qqmlobjectcreator_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlpropertybinding_p.h>
#include <private/qqmlpropertytopropertybinding_p.h>
#include <private/qqmltypeloader_p.h>
#include <private/qqmlvme_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4generatorobject_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qv4resolvedtypereference_p.h>

#include <QtCore/qqueue.h>
#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE

namespace QQmlPreview {


static bool functionBelongsToObject(const QV4::Function *f,
                                    const QQmlRefPointer<QV4::ExecutableCompilationUnit> &cu,
                                    int objectIndex)
{
    if (f->executableCompilationUnit() != cu)
        return false;

    const QV4::CompiledData::Object *obj = cu->objectAt(objectIndex);
    for (auto binding = obj->bindingsBegin(), end = obj->bindingsEnd(); binding != end; ++binding) {
        switch (binding->type()) {
        case QV4::CompiledData::Binding::Type_GroupProperty:
        case QV4::CompiledData::Binding::Type_AttachedProperty:
        case QV4::CompiledData::Binding::Type_Object:
            if (functionBelongsToObject(f, cu, binding->value.objectIndex))
                return true;
            break;
        case QV4::CompiledData::Binding::Type_Script:
            if (cu->runtimeFunctions[binding->value.compiledScriptIndex] == f)
                return true;
        default:
            break;
        }
    }
    return false;
}

// Determines whether a binding on a property is "external", i.e. not from any of the
// compilation units that participate in the rebuild of this object.
// External bindings come from other compilation units (e.g. a parent component setting a
// property binding on a child instance) and must be preserved across rebuilds.
static bool isExternalBinding(const QQmlAnyBinding &binding,
                              const std::vector<CompositeLevel> &internalUnits,
                              QObject *target)
{
    if (!binding)
        return false;

    const QV4::Function *f = nullptr;

    if (const QQmlAbstractBinding *abstractBinding = binding.asAbstractBinding()) {
        // Other kinds of abstract bindings (e.g. ValueTypeProxyBinding) are external
        if (abstractBinding->kind() == QQmlAbstractBinding::QmlBinding)
            f = static_cast<const QQmlBinding *>(abstractBinding)->function();
    } else if (const QPropertyBindingPrivate *priv =
                       QPropertyBindingPrivate::get(binding.asUntypedPropertyBinding());
               priv && priv->isQmlBinding()) {
        // QPropertyBindingPrivate-based binding. Check if it's a QQmlPropertyBinding
        // with a JS expression we can trace back to a CU.

        const auto base = static_cast<const QQmlPropertyBindingBase *>(priv);
        if (base->bindingKind() == QQmlPropertyBindingBase::BindingKind::JavaScript) {
            if (const QQmlPropertyBindingJS *jsExpr =
                        static_cast<const QQmlPropertyBinding *>(base)->jsExpression()) {
                f = jsExpr->function();
            }
        }
    }

    if (!f)
        return true;

    for (const auto &internalUnit : internalUnits) {
        if (functionBelongsToObject(f, internalUnit.oldCu, internalUnit.objectIndex)
            || functionBelongsToObject(f, internalUnit.newCu, internalUnit.objectIndex)) {
            return false;
        }
    }

    const QQmlData *ddata = QQmlData::get(target);
    if (!ddata)
        return true;

    // If the binding lives in an outer context that's still part of the rebuild, it is not
    // actually external since it will be re-recreated.

    for (QQmlRefPointer<QQmlContextData> context = ddata->outerContext; context;
         context = context->parent()) {
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> cu = context->typeCompilationUnit();
        if (!cu)
            continue;

        if (f->executableCompilationUnit() == cu)
            return false;

        if (std::any_of(internalUnits.begin(), internalUnits.end(),
                        [&](const CompositeLevel &level) {
                            return level.oldCu == cu || level.newCu == cu;
                        })) {
            break;
        }
    }

    return true;
}

static QObject *propertyToPropertySource(const QQmlAnyBinding &binding)
{
    if (const QQmlAbstractBinding *abstractBinding = binding.asAbstractBinding()) {
        if (abstractBinding->kind() == QQmlAbstractBinding::PropertyToPropertyBinding) {
            return static_cast<const QQmlPropertyToUnbindablePropertyBinding *>(abstractBinding)
                    ->source();
        }
    } else if (const QPropertyBindingPrivate *priv =
                       QPropertyBindingPrivate::get(binding.asUntypedPropertyBinding());
               priv && priv->isQmlBinding()) {
        const auto base = static_cast<const QQmlPropertyBindingBase *>(priv);
        if (base->bindingKind() == QQmlPropertyBindingBase::BindingKind::PropertyToProperty)
            return static_cast<const QQmlPropertyToBindablePropertyBinding *>(priv)->source();
    }
    return nullptr;
}

void BindingPatchContext::recordBindingValues(
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit, int cuIndex,
        QHash<QString, QVariant> *constantValues, QDuplicateTracker<QObject *> *seenChildren)
{
    Q_ASSERT(constantValues);

    if (!unit || cuIndex >= unit->objectCount())
        return;

    const QV4::CompiledData::Object *obj = unit->objectAt(cuIndex);
    const QQmlPropertyCache::ConstPtr cache = unit->propertyCachesPtr()->at(cuIndex);
    const QString defaultPropertyName = cache ? cache->defaultPropertyName() : QString();

    for (auto binding = obj->bindingsBegin(), end = obj->bindingsEnd(); binding != end; ++binding) {

        const QString name = binding->propertyNameIndex == 0
                ? defaultPropertyName
                : unit->stringAt(binding->propertyNameIndex);
        if (name.isEmpty())
            continue;

        switch (binding->type()) {
        case QV4::CompiledData::Binding::Type_AttachedProperty:
            attachedContext(unit, binding, seenChildren);
            continue;
        case QV4::CompiledData::Binding::Type_GroupProperty:
            childContext(unit, binding, seenChildren);
            continue;
        default:
            break;
        }

        if (binding->isSignalHandler())
            continue;

        if (binding->hasFlag(QV4::CompiledData::Binding::IsCustomParserBinding))
            continue;

        const qsizetype size = constantValues->size();
        QVariant &value = (*constantValues)[name];
        if (constantValues->size() == size)
            continue;

        // Extract constant value from the binding for comparison
        switch (binding->type()) {
        case QV4::CompiledData::Binding::Type_Number: {
            const double d = unit->bindingValueAsNumber(binding);
            value = QV4::Value::isInt32(d) ? QVariant(int(d)) : QVariant(d);
            break;
        }
        case QV4::CompiledData::Binding::Type_Boolean:
            value = QVariant(bool(binding->value.b));
            break;
        case QV4::CompiledData::Binding::Type_Translation:
        case QV4::CompiledData::Binding::Type_TranslationById:
        case QV4::CompiledData::Binding::Type_String:
            value = unit->bindingValueAsString(binding);
            break;
        case QV4::CompiledData::Binding::Type_Null:
            value = QVariant::fromValue(nullptr);
            break;
        default:
            // Script bindings, object bindings, etc.
            // We mark these with an invalid QVariant. They shouldn't be touched since we've
            // just installed the new bindings (unless there is yet another, external binding).
            break;
        }
    }

    for (int propertyIndex = 0, end = obj->propertyCount(); propertyIndex != end; ++propertyIndex) {
        const qsizetype size = constantValues->size();
        QVariant &value =
                (*constantValues)[unit->stringAt(obj->propertyTable()[propertyIndex].nameIndex())];
        if (constantValues->size() != size)
            value = QVariant(cache->property(cache->propertyOffset() + propertyIndex)->propType());
    }
}

void BindingPatchContext::stashExternalState(const std::vector<CompositeLevel> &internalUnits,
                                             QDuplicateTracker<QObject *> *seenChildren)
{
    // Determine which properties are assigned by the CU and their constant values
    QHash<QString, QVariant> constantValues;
    recordBindingValues(unit, objectIndex, &constantValues, seenChildren);

    if (prefix.isEmpty() && QQmlData::get(m_object)->hasVMEMetaObject) {
        for (QQmlVMEMetaObject *vmeMeta =
                     static_cast<QQmlVMEMetaObject *>(QObjectPrivate::get(m_object)->metaObject);
             vmeMeta; vmeMeta = vmeMeta->parentVMEMetaObject()) {
            if (auto cu = vmeMeta->compilationUnit())
                recordBindingValues(cu, vmeMeta->qmlObjectId(), &constantValues, seenChildren);
        }
    }

    // Iterate all properties. For those in the CU's binding table, check if the current state
    // differs from what the CU set (indicating an external override to preserve). For the rest,
    // check for external bindings installed by other components.
    // Additionally, for QObject* properties pointing to QML-created children, register them
    // as child contexts so their external signal handlers are stashed recursively at the end.
    const QMetaObject *mo = m_object->metaObject();
    for (int i = 0, count = mo->propertyCount(); i < count; ++i) {
        const QMetaProperty metaProp = mo->property(i);
        const QString propName = QString::fromUtf8(metaProp.name());

        const QQmlProperty qProp(m_object, propName);
        if (!qProp.isValid())
            continue;

        // Discover QML-created child objects accessible via QObject* properties.
        // Objects without a CU (like lazily-created grouped property objects) survive
        // rebuilds unchanged and don't need stashing.
        if (qProp.propertyMetaType().flags().testFlag(QMetaType::PointerToQObject)) {
            if (QObject *child = qProp.read().value<QObject *>()) {
                if (QQmlData *childDdata = QQmlData::get(child)) {
                    if (const auto &childCU = childDdata->compilationUnit; childCU
                        && std::find_if(internalUnits.begin(), internalUnits.end(),
                                        [&](const CompositeLevel &level) {
                                            return level.newCu == childCU || level.oldCu == childCU;
                                        })
                                != internalUnits.end()) {
                        childContext(propName, child, childCU, childDdata->cuObjectIndex,
                                     seenChildren);
                    }
                }
            }
        }

        const auto it = constantValues.constFind(propName);
        if (it == constantValues.cend()) {
            // Property not in CU's binding table — check for external bindings.
            const QQmlAnyBinding binding = QQmlAnyBinding::ofProperty(qProp);
            if (isExternalBinding(binding, internalUnits, m_object)) {
                QQmlAnyBinding taken = QQmlAnyBinding::takeFrom(qProp);
                m_storedBindings.push_back(
                        { propName, std::move(taken), propertyToPropertySource(binding) });
            }
            continue;
        }

        // Property is in the CU's binding table.
        const QQmlAnyBinding binding = QQmlAnyBinding::ofProperty(qProp);
        if (isExternalBinding(binding, internalUnits, m_object)) {
            QQmlAnyBinding taken = QQmlAnyBinding::takeFrom(qProp);
            m_storedBindings.push_back(
                    { propName, std::move(taken), propertyToPropertySource(binding) });
            continue;
        }

        // Internal binding is still valid. Apparently it doesn't get overridden by an external
        // constant or binding. Nothing to store.
        if (binding)
            continue;

        if (!it->isValid()) {
            // This is potentially an internal binding overridden by an external constant. But
            // it can also be an enum assignment optimized away to omit the binding itself. We
            // can't discern those. So we don't store them for now.
            // TODO: We can probably do better here.
            continue;
        }

        // Two constant values. Figure out if they're the same. If not, store.

        const QMetaType expectedMetaType = qProp.propertyMetaType();
        QVariant expected;
        if (expectedMetaType != QMetaType::fromType<QVariant>()
            && it->metaType() != expectedMetaType) {
            QV4::ExecutionEngine *v4 = unit->engine;
            QV4::Scope scope(v4);
            QV4::ScopedValue v(scope, v4->metaTypeToJS(it->metaType(), it->constData()));
            expected = QVariant(expectedMetaType);
            v4->metaTypeFromJS(v, expectedMetaType, expected.data());
        } else {
            expected = *it;
        }

        if (const QVariant current = qProp.read(); current != expected)
            m_storedValues.push_back({ propName, current });
    }

    const auto stashBoundSignal = [&](QQmlBoundSignal *boundSignal) {
        const QByteArray signature =
                QMetaObjectPrivate::signal(m_object->metaObject(), boundSignal->signalIndex())
                        .methodSignature();
        QQmlNotifierEndpoint *next = boundSignal->nextEndpoint();
        boundSignal->disconnect();
        m_storedSignalHandlers.push_back(
                { QString::fromUtf8(signature), std::unique_ptr<QQmlBoundSignal>(boundSignal) });
        return next;
    };

    // Stash external signal handlers connected to this object's signals.
    // A handler is "internal" only if its function will be recreated during repopulation
    // (i.e., it's a signal handler binding at one of the specific object indices being rebuilt).
    // Only QQmlBoundSignal endpoints are stashed — other notifier endpoints (e.g. alias
    // tracking) are embedded in VME data arrays and cannot be safely owned or relocated.
    if (QQmlNotifyList *list = QQmlData::get(m_object)->notifyList.loadRelaxed()) {
        // Ensure all endpoints are moved from the pending 'todo' list into the
        // laid-out 'notifies' array. Endpoints remain in 'todo' until a signal
        // with a high enough index is actually delivered, so without this call
        // we'd miss handlers for signals that were never fired (e.g. clicked()).
        if (list->todo)
            list->layout();
        for (quint16 i = 0, end = list->notifiesSize; i < end; ++i) {
            for (QQmlNotifierEndpoint *ep = list->notifies[i]; ep;) {
                if (ep->callbackType() != QQmlNotifierEndpoint::QQmlBoundSignal) {
                    ep = ep->nextEndpoint();
                    continue;
                }

                QQmlBoundSignal *boundSignal = static_cast<QQmlBoundSignal *>(ep);
                QQmlBoundSignalExpression *expr = boundSignal->expression();
                if (!expr) {
                    ep = stashBoundSignal(boundSignal);
                    continue;
                }

                const QV4::Function *f = expr->function();
                if (!f) {
                    ep = stashBoundSignal(boundSignal);
                    continue;
                }

                bool isInternal = false;
                for (const CompositeLevel &internalUnit : internalUnits) {
                    if (functionBelongsToObject(f, internalUnit.oldCu, internalUnit.objectIndex)
                        || functionBelongsToObject(f, internalUnit.newCu,
                                                   internalUnit.objectIndex)) {
                        isInternal = true;
                        break;
                    }
                }

                ep = isInternal ? ep->nextEndpoint() : stashBoundSignal(boundSignal);
            }
        }
    }

    // Recurse into child contexts (group properties)
    for (auto &[name, child] : m_children) {
        if (child)
            child->stashExternalState(internalUnits, seenChildren);
    }
}

void BindingPatchContext::refreshObjects()
{
    if (!m_object)
        return;

    // After a rebuild, child objects (accessed via grouped properties) may have
    // been replaced. Re-fetch QObject pointers from the parent's properties so
    // that restoreExternalState() reconnects to the new objects.
    for (auto &[name, child] : m_children) {
        if (!child)
            continue;

        // Children with a non-empty prefix share m_object with their parent
        // (value-type group properties like "font."). Update them to match.
        if (!child->prefix.isEmpty()) {
            child->m_object = m_object;
            child->refreshObjects();
            continue;
        }

        if (QObject *newObj = m_object->property(name.toUtf8()).value<QObject *>())
            child->m_object = newObj;

        child->refreshObjects();
    }
}

void BindingPatchContext::restoreExternalState()
{
    if (!m_object)
        return;

    // Restore external bindings (look up by name since indices may have shifted)
    for (auto &stored : m_storedBindings) {
        if (!stored.binding)
            continue;

        // If this was a property-to-property binding, verify the source object survived the
        // rebuild. Delegate model items (the source for required-property bindings) are
        // destroyed when the delegate is re-instantiated and new bindings are created
        // automatically. Restoring a stale binding would dereference freed memory.
        if (stored.sourceGuard.isNull()) {
            bool isPTP = false;
            if (auto *abstractBinding = stored.binding.asAbstractBinding()) {
                isPTP = abstractBinding->kind()
                        == QQmlAbstractBinding::PropertyToPropertyBinding;
            } else if (const QPropertyBindingPrivate *priv = QPropertyBindingPrivate::get(
                               stored.binding.asUntypedPropertyBinding())) {
                const auto base = static_cast<const QQmlPropertyBindingBase *>(priv);
                isPTP = base->bindingKind()
                        == QQmlPropertyBindingBase::BindingKind::PropertyToProperty;
            }
            if (isPTP)
                continue;
        }

        QQmlProperty qProp(m_object, stored.propertyName);
        if (!qProp.isValid())
            continue;

        // After a rebuild, child objects may have been replaced (refreshObjects).
        // The stashed binding's targetObject still references the old object.
        // Update it to the new object before installing, otherwise installOn()
        // asserts that targetObject() == target.object().
        if (auto *abstractBinding = stored.binding.asAbstractBinding()) {
            if (abstractBinding->targetObject() != qProp.object())
                abstractBinding->setTarget(qProp);
        }

        stored.binding.installOn(qProp);
    }
    m_storedBindings.clear();

    // Restore externally set values (only if no new binding was installed)
    for (auto &stored : m_storedValues) {
        const QMetaObject *mo = m_object->metaObject();
        const int idx = mo->indexOfProperty(stored.propertyName.toUtf8().constData());
        if (idx < 0)
            continue;

        // Don't overwrite if a binding was just installed by repopulateBindings
        QQmlProperty qProp(m_object, stored.propertyName);
        if (!qProp.isValid())
            continue;
        QQmlAnyBinding currentBinding = QQmlAnyBinding::ofProperty(qProp);
        if (currentBinding)
            continue;

        mo->property(idx).write(m_object, stored.value);
    }
    m_storedValues.clear();

    // Restore external signal handlers that were detached during stash.
    // Reconnect them to this object's signals.
    if (!m_storedSignalHandlers.empty()) {
        QQmlEngine *engine = unit->engine->qmlEngine();
        for (auto &stored : m_storedSignalHandlers) {
            const QMetaObject *metaObject = m_object->metaObject();
            const int signalIndex = QMetaObjectPrivate::signalIndex(
                    metaObject->method(metaObject->indexOfSignal(stored.signature.toUtf8())));
            if (signalIndex >= 0)
                QQmlData::connectEndpoint(stored.handler.release(), m_object, signalIndex, engine);
        }
    }
    m_storedSignalHandlers.clear();

    // Recurse into child contexts (group properties)
    for (auto &[name, child] : m_children) {
        if (child)
            child->restoreExternalState();
    }
}

BindingPatchContext *
BindingPatchContext::childContext(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                                  const QV4::CompiledData::Binding *binding,
                                  QDuplicateTracker<QObject *> *seenChildren)
{
    const QString name = unit->stringAt(binding->propertyNameIndex);

    const size_t size = m_children.size();
    std::unique_ptr<BindingPatchContext> &child = m_children[name];
    if (size == m_children.size())
        return child.get();

    if (!seenChildren)
        return nullptr;

    if (QObject *groupObject = m_object->property(name.toUtf8()).value<QObject *>()) {
        if (seenChildren->hasSeen(groupObject))
            return nullptr;
        child = std::make_unique<BindingPatchContext>(groupObject, unit,
                                                      binding->value.objectIndex);
    } else {
        child = std::make_unique<BindingPatchContext>(m_object, unit, binding->value.objectIndex,
                                                      name);
    }
    return child.get();
}

BindingPatchContext *
BindingPatchContext::childContext(const QString &name, QObject *object,
                                  const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                                  int objectIndex, QDuplicateTracker<QObject *> *seenChildren)
{
    const size_t size = m_children.size();
    std::unique_ptr<BindingPatchContext> &child = m_children[name];
    if (size == m_children.size()) {
        Q_ASSERT(!child || child->m_object == object);
        return child.get();
    }

    if (!seenChildren || seenChildren->hasSeen(object))
        return nullptr;

    child = std::make_unique<BindingPatchContext>(object, unit, objectIndex);
    return child.get();
}

BindingPatchContext *
BindingPatchContext::attachedContext(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                                     const QV4::CompiledData::Binding *binding,
                                     QDuplicateTracker<QObject *> *seenChildren)
{
    const QString name = unit->stringAt(binding->propertyNameIndex);

    const size_t size = m_children.size();
    std::unique_ptr<BindingPatchContext> &child = m_children[name];
    if (size == m_children.size())
        return child.get();

    if (!seenChildren)
        return nullptr;

    QV4::ResolvedTypeReference *typeRef = unit->resolvedType(binding->propertyNameIndex);
    Q_ASSERT(typeRef);
    QQmlAttachedPropertiesFunc func =
            typeRef->type().attachedPropertiesFunction(unit->engine->typeLoader());
    Q_ASSERT(func);

    if (QObject *attached = QQmlData::get(m_object)->attachedProperties()->value(func)) {
        if (seenChildren->hasSeen(attached))
            return nullptr;
        child = std::make_unique<BindingPatchContext>(attached, unit, binding->value.objectIndex);
    }
    return child.get();
}

void BindingPatchContext::reset(
        const std::vector<QQmlRefPointer<QV4::ExecutableCompilationUnit>> &unitsToUnparent)
{
    resetBindings(unit, objectIndex);

    QQmlData *ddata = QQmlData::get(m_object);
    for (QQmlVMEMetaObject *vmeMeta = ddata->hasVMEMetaObject
                 ? static_cast<QQmlVMEMetaObject *>(QObjectPrivate::get(m_object)->metaObject)
                 : nullptr;
         vmeMeta; vmeMeta = vmeMeta->parentVMEMetaObject()) {
        resetBindings(vmeMeta->compilationUnit(), vmeMeta->qmlObjectId());
    }

    // Remove remaining composite signal handlers (all internal ones).
    // External handlers were already detached by stashExternalState() and are invisible here.
    // The object creator will recreate the internal handlers when it rebuilds the object.
    while (QQmlBoundSignal *signalHandler = ddata->signalHandlers)
        delete signalHandler;

    const QHash<QQmlAttachedPropertiesFunc, QObject *> *attachedProperties =
            ddata->hasExtendedData() ? ddata->attachedProperties() : nullptr;
    const auto isAttached = [attachedProperties](QObject *child) {
        if (!attachedProperties)
            return false;
        for (QObject *attached : *attachedProperties) {
            if (attached == child)
                return true;
        }
        return false;
    };

    const auto shouldUnparent = [&](const QQmlRefPointer<QV4::ExecutableCompilationUnit> &cu) {
        return cu
                && std::find(unitsToUnparent.begin(), unitsToUnparent.end(), cu)
                != unitsToUnparent.end();
    };

    const QObjectList children = m_object->children();

    for (QObject *child : children) {
        // Objects from the old CU or composite-level CUs will be recreated by
        // repopulateBindings. Unparent them so they don't interfere with the new objects.
        // Attached property objects are reused across rebuilds.
        if (QQmlData *childDdata = QQmlData::get(child);
            childDdata && shouldUnparent(childDdata->compilationUnit)) {
            if (!isAttached(child))
                retireObject(child);
        }
    }
}

// Fully retire an old object that is being replaced by repopulateBindings.
// Recursively removes bindings from all descendants (unlinking expressions
// from context lists), then removes the subtree from the tree and schedules it
// for deletion. compilationUnit is intentionally left intact. The GC needs it.
void BindingPatchContext::retireObject(QObject *object)
{
    // First pass: recursively remove all bindings from the entire subtree.
    // This must happen before any parent changes so that binding
    // evaluations triggered by re-parenting find no live expressions.
    clearBindingsRecursive(object);

    // Remove from parent (in QtQuick "visual parent" or parentItem) via the meta property system.
    // We must not assume any particular property to be the "parent" property here. That's what
    // we have the ParentProperty classInfo for.
    const QMetaObject *mo = object->metaObject();
    if (const int classInfoIndex = mo->indexOfClassInfo("ParentProperty"); classInfoIndex >= 0) {
        const QMetaClassInfo classInfo = mo->classInfo(classInfoIndex);
        if (const int propertyIndex = mo->indexOfProperty(classInfo.value()); propertyIndex >= 0) {
            const QMetaProperty property = mo->property(propertyIndex);
            if ((!property.isResettable() || !property.reset(object)) && property.isWritable())
                property.write(object, QVariant(property.metaType()));
        }
    }

    // Unparent from QObject hierarchy so it no longer appears in
    // parent->children(), then schedule deletion. The destructor will
    // cascade-delete all QObject children (the recursive descendants).
    QQml_setParent_noEvent(object, nullptr);
    object->deleteLater();
}

void BindingPatchContext::clearBindingsRecursive(QObject *object)
{
    QQueue<QObject *> queue;
    queue.enqueue(object);

    while (!queue.isEmpty()) {
        QObject *next = queue.dequeue();
        queue.append(next->children());

        QQmlData *ddata = QQmlData::get(next);
        if (!ddata)
            continue;

        while (ddata->bindings)
            QQmlPropertyPrivate::removeBinding(ddata->bindings);
    }
}

void BindingPatchContext::resetBinding(
        const QV4::CompiledData::Binding *binding, const QString &name,
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit)
{
    if (binding->hasFlag(QV4::CompiledData::Binding::IsCustomParserBinding))
        return;

    QQmlProperty prop(m_object, name);
    QQmlPropertyIndex propIdx = QQmlPropertyPrivate::propertyIndex(prop);
    Q_ASSERT(propIdx.coreIndex() >= 0);

    const QMetaType type = prop.propertyMetaType();

    const QMetaType::TypeFlags flags = type.flags();
    if (flags.testFlag(QMetaType::IsQmlList)) {
        QQmlListReference list = prop.read().value<QQmlListReference>();
        if (list.clear()) {
            return;
        }
    } else if (flags.testFlag(QMetaType::PointerToQObject) && binding->isGroupProperty()) {
        if (BindingPatchContext *child = childContext(oldUnit, binding, nullptr))
            child->resetBindings(oldUnit, binding->value.objectIndex);
        return;
    }

    if ((!prop.isResettable() || !prop.reset()) && prop.isWritable())
        prop.write(QVariant(type));
}

void BindingPatchContext::resetBindings(
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit, int cuIndex)
{
    const QV4::CompiledData::Object *obj = oldUnit->objectAt(cuIndex);
    const QQmlPropertyCache::ConstPtr cache = oldUnit->propertyCachesPtr()->at(cuIndex);
    const QString defaultPropertyName = cache ? cache->defaultPropertyName() : QString();

    for (auto binding = obj->bindingsBegin(), end = obj->bindingsEnd(); binding != end; ++binding) {
        const QString name = binding->propertyNameIndex == 0
                ? defaultPropertyName
                : oldUnit->stringAt(binding->propertyNameIndex);
        if (name.isEmpty())
            continue;

        if (binding->isAttachedProperty()) {
            // Recurse into existing attached objects to reset their bindings.
            // The object creator will reuse them via qmlAttachedPropertiesObject().
            if (!QQmlData::get(m_object)->hasExtendedData())
                continue;

            if (BindingPatchContext *attached = attachedContext(oldUnit, binding, nullptr))
                attached->resetBindings(oldUnit, binding->value.objectIndex);

            continue;
        }

        // Signal handlers are disconnected centrally.
        if (!binding->isSignalHandler())
            resetBinding(binding, prefix + name, oldUnit);
    }
}

} // namespace QQmlPreview

QT_END_NAMESPACE
