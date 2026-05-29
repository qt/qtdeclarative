// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmlpreviewbindingpatchcontext_p.h"

#include <private/qqmlboundsignal_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlobjectcreator_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmltypeloader_p.h>
#include <private/qqmlvme_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4generatorobject_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qv4resolvedtypereference_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlPreview {

// Determines whether a binding on a property is "external", i.e. not from any of the
// compilation units that participate in the rebuild of this object.
// External bindings come from other compilation units (e.g. a parent component setting a
// property binding on a child instance) and must be preserved across rebuilds.
static bool
isExternalBinding(const QQmlAnyBinding &binding,
                  const std::vector<QQmlRefPointer<QV4::ExecutableCompilationUnit>> &internalUnits)
{
    if (!binding)
        return false;

    // QPropertyBindingPrivate-based bindings are external.
    if (!binding.isAbstractPropertyBinding())
        return true;

    const QQmlAbstractBinding *abstractBinding = binding.asAbstractBinding();

    // Other kinds of abstract bindings (e.g. ValueTypeProxyBinding) are external
    if (abstractBinding->kind() != QQmlAbstractBinding::QmlBinding)
        return true;

    const QV4::Function *f = static_cast<const QQmlBinding *>(abstractBinding)->function();
    if (!f)
        return false;

    const QQmlRefPointer<QV4::ExecutableCompilationUnit> bindingCU = f->executableCompilationUnit();
    for (const auto &internalUnit : internalUnits) {
        if (bindingCU == internalUnit)
            return false;
    }

    return true;
}

void BindingPatchContext::recordBindingValues(
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit, int cuIndex,
        QHash<QString, QVariant> *constantValues)
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
            attachedContext(unit, binding);
            continue;
        case QV4::CompiledData::Binding::Type_GroupProperty:
            childContext(unit, binding);
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
}

void BindingPatchContext::stashExternalState(
        const std::vector<QQmlRefPointer<QV4::ExecutableCompilationUnit>> &internalUnits)
{
    // Determine which properties are assigned by the CU and their constant values
    QHash<QString, QVariant> constantValues;
    recordBindingValues(unit, objectIndex, &constantValues);

    if (prefix.isEmpty() && QQmlData::get(m_object)->hasVMEMetaObject) {
        for (QQmlVMEMetaObject *vmeMeta =
                     static_cast<QQmlVMEMetaObject *>(QObjectPrivate::get(m_object)->metaObject);
             vmeMeta; vmeMeta = vmeMeta->parentVMEMetaObject()) {
            if (auto cu = vmeMeta->compilationUnit())
                recordBindingValues(cu, vmeMeta->qmlObjectId(), &constantValues);
        }
    }

    // Only examine properties that appear in the CU's binding table.
    // For each such property, check if its current state differs from what the CU set
    // (indicating an external override that needs to be preserved).
    // TODO: There may be other external bindings but we can't easily see whether they change
    //       anything.
    for (auto it = constantValues.cbegin(), end = constantValues.cend(); it != end; ++it) {
        const QString propName = it.key();
        const QQmlProperty qProp(m_object, propName);
        if (!qProp.isValid() || !qProp.isWritable())
            continue;

        const QQmlAnyBinding binding = QQmlAnyBinding::ofProperty(qProp);
        if (isExternalBinding(binding, internalUnits)) {
            m_storedBindings.push_back({ propName, QQmlAnyBinding::takeFrom(qProp) });
            continue;
        }

        // Internal binding is still valid. Apparently it doens't get overridden by an external
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

    // Recurse into child contexts (group properties)
    for (auto &[name, child] : m_children) {
        if (child)
            child->stashExternalState(internalUnits);
    }
}

void BindingPatchContext::restoreExternalState()
{
    // Restore external bindings (look up by name since indices may have shifted)
    for (auto &stored : m_storedBindings) {
        if (!stored.binding)
            continue;
        QQmlProperty qProp(m_object, stored.propertyName);
        if (qProp.isValid())
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

    // Recurse into child contexts (group properties)
    for (auto &[name, child] : m_children) {
        if (child)
            child->restoreExternalState();
    }
}

BindingPatchContext *
BindingPatchContext::childContext(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                                  const QV4::CompiledData::Binding *binding)
{
    const QString name = unit->stringAt(binding->propertyNameIndex);

    const size_t size = m_children.size();
    std::unique_ptr<BindingPatchContext> &child = m_children[name];
    if (size == m_children.size())
        return child.get();

    if (QObject *groupObject = m_object->property(name.toUtf8()).value<QObject *>()) {
        child = std::make_unique<BindingPatchContext>(groupObject, unit,
                                                      binding->value.objectIndex);
    } else {
        child = std::make_unique<BindingPatchContext>(m_object, unit, binding->value.objectIndex,
                                                      name);
    }
    return child.get();
}

BindingPatchContext *
BindingPatchContext::attachedContext(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                                     const QV4::CompiledData::Binding *binding)
{
    const QString name = unit->stringAt(binding->propertyNameIndex);

    const size_t size = m_children.size();
    std::unique_ptr<BindingPatchContext> &child = m_children[name];
    if (size == m_children.size())
        return child.get();

    QV4::ResolvedTypeReference *typeRef = unit->resolvedType(binding->propertyNameIndex);
    Q_ASSERT(typeRef);
    QQmlAttachedPropertiesFunc func =
            typeRef->type().attachedPropertiesFunction(unit->engine->typeLoader());
    Q_ASSERT(func);

    if (QObject *attached = QQmlData::get(m_object)->attachedProperties()->value(func))
        child = std::make_unique<BindingPatchContext>(attached, unit, binding->value.objectIndex);
    return child.get();
}

void BindingPatchContext::reset()
{
    resetBindings(unit, objectIndex);

    QQmlData *ddata = QQmlData::get(m_object);
    for (QQmlVMEMetaObject *vmeMeta = ddata->hasVMEMetaObject
                 ? static_cast<QQmlVMEMetaObject *>(QObjectPrivate::get(m_object)->metaObject)
                 : nullptr;
         vmeMeta; vmeMeta = vmeMeta->parentVMEMetaObject()) {
        resetBindings(vmeMeta->compilationUnit(), vmeMeta->qmlObjectId());
    }

    // Remove all composite signal handlers, no matter where they're from.
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

    const QObjectList children = m_object->children();

    for (QObject *child : children) {
        // Objects from the same CU have likely been created as inner scopes and will be replaced.
        // Unparent the old ones so that the GC can take care of them.
        // But skip attached property objects — they are reused across rebuilds.
        if (QQmlData *childDdata = QQmlData::get(child);
            childDdata && childDdata->compilationUnit == ddata->compilationUnit) {
            if (!isAttached(child))
                child->setParent(nullptr);
        }
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
        if (BindingPatchContext *child = childContext(oldUnit, binding))
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

            if (BindingPatchContext *attached = attachedContext(oldUnit, binding))
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
