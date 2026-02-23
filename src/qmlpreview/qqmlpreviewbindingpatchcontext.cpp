// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmlpreviewbindingpatchcontext_p.h"
#include <private/qqmlboundsignal_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlobjectcreator_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmltypeloader_p.h>
#include <private/qqmlvme_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4generatorobject_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qv4resolvedtypereference_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlPreview {

void BindingPatchContext::reset()
{
    // Intentionally take an extra refpointer of the CU.
    // The GC might kick in and remove it from the ddata.
    if (QQmlRefPointer<QV4::ExecutableCompilationUnit> cu = m_ddata->compilationUnit)
        resetBindings(m_object->metaObject(), cu, m_ddata->cuObjectIndex);

    for (QQmlVMEMetaObject *vmeMeta = m_ddata->hasVMEMetaObject
                 ? static_cast<QQmlVMEMetaObject *>(QObjectPrivate::get(m_object)->metaObject)
                 : nullptr;
         vmeMeta; vmeMeta = vmeMeta->parentVMEMetaObject()) {
        resetBindings(vmeMeta->toDynamicMetaObject(m_object), vmeMeta->compilationUnit(),
                      vmeMeta->qmlObjectId());
    }

    // Remove all composite signal handlers, no matter where they're from.
    while (QQmlBoundSignal *signalHandler = m_ddata->signalHandlers)
        delete signalHandler;

    const QHash<QQmlAttachedPropertiesFunc, QObject *> *attachedProperties =
            m_ddata->hasExtendedData() ? m_ddata->attachedProperties() : nullptr;
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
        if (QQmlData *ddata = QQmlData::get(child);
            ddata && ddata->compilationUnit == m_ddata->compilationUnit) {
            if (!isAttached(child))
                child->setParent(nullptr);
        }
    }
}

void BindingPatchContext::resetBinding(
        const QMetaObject *metaObject, const QV4::CompiledData::Binding *binding,
        const QString &defaultPropName,
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit)
{
    if (binding->hasFlag(QV4::CompiledData::Binding::IsCustomParserBinding))
        return;

    const QString name = binding->propertyNameIndex == 0
            ? defaultPropName
            : oldUnit->stringAt(binding->propertyNameIndex);
    if (name.isEmpty())
        return;

    const int propIdx = metaObject->indexOfProperty(name.toUtf8().constData());
    Q_ASSERT(propIdx >= 0);

    if (m_handledProperties.contains(propIdx))
        return;

    const QMetaProperty property = metaObject->property(propIdx);
    const QMetaType type = property.metaType();

    const QMetaType::TypeFlags flags = type.flags();
    if (flags.testFlag(QMetaType::IsQmlList)) {
        QQmlListReference list(m_object, property.name());
        if (list.clear()) {
            (void)m_handledProperties.hasSeen(propIdx);
            return;
        }
    } else if (flags.testFlag(QMetaType::PointerToQObject) && binding->isGroupProperty()) {
        QObject *groupObject = property.read(m_object).value<QObject *>();
        if (!groupObject) {
            (void)m_handledProperties.hasSeen(propIdx);
            return;
        }

        // If it's an object used as group property, reset each of the properties assigned here.
        // This does not reset the whole object, so don't add it to the duplicate tracker
        BindingPatchContext(groupObject)
                .resetBindings(groupObject->metaObject(), oldUnit, binding->value.objectIndex);
        return;
    }

    if ((property.isResettable() && property.reset(m_object))
        || (property.isWritable() && property.write(m_object, QVariant(type)))) {
        (void)m_handledProperties.hasSeen(propIdx);
    }
}

void BindingPatchContext::resetBindings(
        const QMetaObject *metaObject,
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &oldUnit, int cuIndex)
{
    const QV4::CompiledData::Object *obj = oldUnit->objectAt(cuIndex);
    const QQmlPropertyCache::ConstPtr cache = oldUnit->propertyCachesPtr()->at(cuIndex);
    const QString defaultPropertyName = cache ? cache->defaultPropertyName() : QString();

    const QV4::CompiledData::Binding *binding = obj->bindingTable();

    for (quint32 i = 0; i < obj->nBindings; ++i, ++binding) {
        if (binding->isAttachedProperty()) {
            // Recurse into existing attached objects to reset their bindings.
            // The object creator will reuse them via qmlAttachedPropertiesObject().
            if (!m_ddata->hasExtendedData())
                continue;

            QV4::ResolvedTypeReference *typeRef = oldUnit->resolvedType(binding->propertyNameIndex);
            Q_ASSERT(typeRef);
            QQmlAttachedPropertiesFunc func
                    = typeRef->type().attachedPropertiesFunction(oldUnit->engine->typeLoader());
            Q_ASSERT(func);

            if (QObject *attached = m_ddata->attachedProperties()->value(func)) {
                BindingPatchContext(attached).resetBindings(
                        attached->metaObject(), oldUnit,
                        binding->value.objectIndex);
            }
            continue;
        }

        // Signal handlers are disconnected centrally.
        if (binding->isSignalHandler())
            continue;

        resetBinding(metaObject, binding, defaultPropertyName, oldUnit);
    }
}

} // namespace QQmlPreview

QT_END_NAMESPACE
