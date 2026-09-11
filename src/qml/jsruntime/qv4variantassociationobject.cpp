// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qv4variantassociationobject_p.h"

#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

/*!
 * \class QV4::VariantAssociationObject
 * \internal
 *
 * A VariantAssociationObject stores the contents of a QVariantMap or QVariantHash
 * and makes them acccessible to the JavaScript engine. It behaves mostly like a
 * regular JavaScript object. The entries of the QVariantMap or QVariantHash are
 * exposed as properties.
 *
 * VariantAssociationObject is a ReferenceObject. Therefore it writes back its contents
 * to the property it was retrieved from whenever it changes. It also re-reads
 * the property whenever that one changes.
 *
 * As long as a VariantAssociationObject is attached to a property this way, it is the
 * responsibility of the property's surrounding (C++) object to keep the contents valid.
 * It has to, for example, track pointers to QObjects potentially deleted in other places
 * so that they don't become dangling.
 *
 * However, the VariantAssociation can also be detached. This happens predominantly by
 * assigning it to a QML-declared property. In that case, it becomes the
 * VariantAssociationObject's responsibility to track its contents. To do so, it does not
 * keep an actual QVariantMap or QVariantHash in this case, but rather stores its contents
 * as actual JavaScript object properties. This includes QObjectWrappers for all QObject
 * pointers it may contain. The contents are then marked like all JavaScript properties
 * when the garbage collector runs, and QObjectWrapper also guards against external
 * deletion. There is no property to read or write back in this case, and neither
 * does the internal QVariantMap or QVariantHash need to be updated. Therefore, the
 * objects stored in the individual properties are also created detached and won't
 * read or write back.
 */

template<typename Return, typename MapCallable, typename HashCallable>
Return visitVariantAssociation(
        const QV4::Heap::VariantAssociationObject *association,
        MapCallable &&mapCallable, HashCallable &&hashCallable)
{
    switch (association->m_type) {
        case QV4::Heap::VariantAssociationObject::AssociationType::VariantMap:
            return std::invoke(
                std::forward<MapCallable>(mapCallable),
                reinterpret_cast<const QVariantMap *>(&association->m_variantAssociation));
        case QV4::Heap::VariantAssociationObject::AssociationType::VariantHash:
            return std::invoke(
                std::forward<HashCallable>(hashCallable),
                reinterpret_cast<const QVariantHash *>(&association->m_variantAssociation));
        default: Q_UNREACHABLE();
    };
}

template<typename Return, typename MapCallable, typename HashCallable>
Return visitVariantAssociation(
        QV4::Heap::VariantAssociationObject *association,
        MapCallable &&mapCallable, HashCallable &&hashCallable)
{
    switch (association->m_type) {
        case QV4::Heap::VariantAssociationObject::AssociationType::VariantMap:
            return std::invoke(
                std::forward<MapCallable>(mapCallable),
                reinterpret_cast<QVariantMap *>(&association->m_variantAssociation));
        case QV4::Heap::VariantAssociationObject::AssociationType::VariantHash:
            return std::invoke(
                std::forward<HashCallable>(hashCallable),
                reinterpret_cast<QVariantHash *>(&association->m_variantAssociation));
        default: Q_UNREACHABLE();
    };
}

template<typename Return, typename Callable>
Return visitVariantAssociation(
        const QV4::Heap::VariantAssociationObject *association, Callable &&callable)
{
    return visitVariantAssociation<Return>(association, callable, callable);
}

template<typename Return, typename Callable>
Return visitVariantAssociation(
        QV4::Heap::VariantAssociationObject *association, Callable &&callable)
{
    return visitVariantAssociation<Return>(association, callable, callable);
}

namespace QV4 {

DEFINE_OBJECT_VTABLE(VariantAssociationObject);

ReturnedValue VariantAssociationPrototype::fromQVariantMap(
        ExecutionEngine *engine, const QVariantMap &variantMap, QV4::Heap::Object *container,
        int property, Heap::ReferenceObject::Flags flags)
{
    return engine->memoryManager->allocate<VariantAssociationObject>(
        variantMap, container, property, flags)->asReturnedValue();
}

ReturnedValue VariantAssociationPrototype::fromQVariantHash(
        ExecutionEngine *engine, const QVariantHash &variantHash, QV4::Heap::Object *container,
        int property, Heap::ReferenceObject::Flags flags)
{
    return engine->memoryManager->allocate<VariantAssociationObject>(
        variantHash, container, property, flags)->asReturnedValue();
}

namespace Heap {

void VariantAssociationObject::init(
        const QVariantMap &variantMap, QV4::Heap::Object *container, int property,
        Heap::ReferenceObject::Flags flags)
{
    ReferenceObject::init(container, property, flags);
    m_type = AssociationType::VariantMap;

    if (container)
        new (m_variantAssociation) QVariantMap(variantMap);
    else
        createElementWrappers(variantMap);
}

void VariantAssociationObject::init(
        const QVariantHash &variantHash, QV4::Heap::Object *container, int property,
        Heap::ReferenceObject::Flags flags)
{
    ReferenceObject::init(container, property, flags);
    m_type = AssociationType::VariantHash;

    if (container)
        new (m_variantAssociation) QVariantHash(variantHash);
    else
        createElementWrappers(variantHash);
}

void VariantAssociationObject::destroy()
{
    if (object()) {
        visitVariantAssociation<void>(
                this, std::destroy_at<QVariantMap>, std::destroy_at<QVariantHash>);
    }
    ReferenceObject::destroy();
}

QVariant VariantAssociationObject::toVariant()
{
    if (object()) {
        return visitVariantAssociation<QVariant>(
            this, [](const auto *association){ return QVariant(*association); });
    }

    QV4::Scope scope(internalClass->engine);
    QV4::ScopedObject self(scope, this);

    switch (m_type) {
    case AssociationType::VariantMap:
        return QV4::ExecutionEngine::variantMapFromJS(self);
    case AssociationType::VariantHash:
        return QV4::ExecutionEngine::variantHashFromJS(self);
    default:
        break;
    }

    Q_UNREACHABLE_RETURN(QVariant());
}

bool VariantAssociationObject::setVariant(const QVariant &variant)
{
    // Should only happen from readReference(). Therefore we are attached.
    Q_ASSERT(object());

    auto metatypeId = variant.metaType().id();

    if (metatypeId != QMetaType::QVariantMap && metatypeId != QMetaType::QVariantHash)
        return false;

    if (metatypeId == QMetaType::QVariantMap && m_type == AssociationType::VariantMap) {
        *reinterpret_cast<QVariantMap *>(&m_variantAssociation) = variant.toMap();
    } else if (metatypeId == QMetaType::QVariantMap && m_type == AssociationType::VariantHash) {
        std::destroy_at(reinterpret_cast<QVariantHash *>(&m_variantAssociation));
        new (m_variantAssociation) QVariantMap(variant.toMap());
        m_type = AssociationType::VariantMap;
    } else if (metatypeId == QMetaType::QVariantHash && m_type == AssociationType::VariantHash) {
        *reinterpret_cast<QVariantHash *>(&m_variantAssociation) = variant.toHash();
    } else if (metatypeId == QMetaType::QVariantHash && m_type == AssociationType::VariantMap) {
        std::destroy_at(reinterpret_cast<QVariantMap *>(&m_variantAssociation));
        new (m_variantAssociation) QVariantHash(variant.toHash());
        m_type = AssociationType::VariantHash;
    }

    return true;
}

template<typename Association>
VariantAssociationObject *createDetached(
        QV4::ExecutionEngine *engine, const Association &association)
{
    return engine->memoryManager->allocate<QV4::VariantAssociationObject>(
            association, nullptr, -1, ReferenceObject::Flag::NoFlag);
}

VariantAssociationObject *VariantAssociationObject::detached()
{
    if (object()) {
        return visitVariantAssociation<VariantAssociationObject *>(
                this, [this](const auto *association) {
                    return createDetached(internalClass->engine, *association);
                });
    }

    QV4::Scope scope(internalClass->engine);
    QV4::ScopedObject self(scope, this);

    switch (m_type) {
    case AssociationType::VariantMap:
        return createDetached(scope.engine, QV4::ExecutionEngine::variantMapFromJS(self));
    case AssociationType::VariantHash:
        return createDetached(scope.engine, QV4::ExecutionEngine::variantHashFromJS(self));
    default:
        break;
    }

    Q_UNREACHABLE_RETURN(nullptr);
}

QV4::ReturnedValue VariantAssociationObject::getElement(const QString &key, bool *hasProperty)
{
    // Must be attached. Otherwise we should use memberData/arrayData
    Q_ASSERT(object());
    Q_ASSERT(hasProperty);

    QV4::ReferenceObject::readReference(this);

    return visitVariantAssociation<QV4::ReturnedValue>(this, [&](const auto *association) {
        auto it = association->constFind(key);
        if (it == association->constEnd()) {
            *hasProperty = false;
            return Encode::undefined();
        }

        *hasProperty = true;

        Scope scope(internalClass->engine);
        ScopedString scopedKey(scope, scope.engine->newString(key));

        uint i = 0;
        if (propertyIndexMapping) {
            Scoped<QV4::ArrayData> arrayData(scope, propertyIndexMapping->arrayData);
            const uint end = arrayData->length();
            for (; i < end; ++i) {
                QV4::ScopedString value(scope, arrayData->get(i));
                Q_ASSERT(value);
                if (value->equals(scopedKey))
                    break;
            }

            if (i == end) {
                ScopedArrayObject mapping(scope, propertyIndexMapping);
                mapping->push_back(scopedKey);
            }
        } else {
            propertyIndexMapping.set(scope.engine, scope.engine->newArrayObject(scopedKey, 1));
        }

        return scope.engine->fromVariant(
                *it, this, i,
                ReferenceObject::Flag::CanWriteBack | ReferenceObject::Flag::IsVariant);
    });
}

} // namespace Heap

ReturnedValue VariantAssociationObject::virtualGet(
        const Managed *that, PropertyKey id, const Value *, bool *hasProperty)
{
    Heap::VariantAssociationObject *heapAssociation
            = static_cast<const VariantAssociationObject *>(that)->d();

    // If this is detached we rely on the element wrappers to hold the value
    if (!heapAssociation->object())
        return ReferenceObject::virtualGet(that, id, that, hasProperty);

    bool found = false;
    if (ReturnedValue result = heapAssociation->getElement(id.toQString(), &found); found) {
        if (hasProperty)
            *hasProperty = true;
        return result;
    }

    return ReferenceObject::virtualGet(that, id, that, hasProperty);
}

bool VariantAssociationObject::virtualPut(
        Managed *that, PropertyKey id, const Value &value, Value *)
{
    Heap::VariantAssociationObject *heapAssociation
            = static_cast<VariantAssociationObject *>(that)->d();

    if (!heapAssociation->object())
        return ReferenceObject::virtualPut(that, id, value, that);

    // Convert before dispatching on the association type. toVariant() reads the
    // properties of the value, which runs any accessors it has, which can change
    // the association object itself between QVariantMap and QVariantHash. We
    // can't let that happen while we are holding a pointer to the the map or hash.
    const QString key = id.toQString();
    const QVariant converted
            = heapAssociation->internalClass->engine->toVariant(value, QMetaType(), false);

    visitVariantAssociation<void>(heapAssociation, [&](auto *association) {
        association->insert(key, converted);
    });

    QV4::ReferenceObject::writeBack(heapAssociation);
    return true;
}

bool VariantAssociationObject::virtualDeleteProperty(Managed *that, PropertyKey id)
{
    Heap::VariantAssociationObject *heapAssociation
            = static_cast<VariantAssociationObject *>(that)->d();

    if (!heapAssociation->object())
        return ReferenceObject::virtualDeleteProperty(that, id);

    const QString key = id.toQString();
    if (!visitVariantAssociation<bool>(heapAssociation, [&](auto *association) {
        return association->remove(key);
    })) {
        return false;
    }

    QV4::ReferenceObject::writeBack(heapAssociation);
    return true;
}

OwnPropertyKeyIterator *VariantAssociationObject::virtualOwnPropertyKeys(
        const Object *m, Value *target)
{
    Heap::VariantAssociationObject *heapAssociation
            = static_cast<const VariantAssociationObject *>(m)->d();

    if (!heapAssociation->object())
        return ReferenceObject::virtualOwnPropertyKeys(m, target);

    struct VariantAssociationOwnPropertyKeyIterator : ObjectOwnPropertyKeyIterator
    {
        QStringList keys;

        ~VariantAssociationOwnPropertyKeyIterator() override = default;

        PropertyKey next(
                const Object *o, Property *pd = nullptr,
                PropertyAttributes *attrs = nullptr) override
        {
            Heap::VariantAssociationObject *heapAssociation
                    = static_cast<const VariantAssociationObject *>(o)->d();

            if (memberIndex == 0) {
                keys = visitVariantAssociation<QStringList>(
                        heapAssociation, [](const auto *association) {
                    return association->keys();
                });
                keys.sort();
            }

            if (static_cast<qsizetype>(memberIndex) < keys.count()) {
                Scope scope(heapAssociation->internalClass->engine);
                ScopedString propertyName(scope, scope.engine->newString(keys[memberIndex]));
                ScopedPropertyKey id(scope, propertyName->toPropertyKey());

                if (attrs)
                    *attrs = QV4::Attr_Data;
                if (pd) {
                    bool found = false;
                    pd->value = heapAssociation->getElement(keys[memberIndex], &found);
                    Q_ASSERT(found);
                }

                ++memberIndex;

                return id;
            }

            return PropertyKey::invalid();
        }
    };

    QV4::ReferenceObject::readReference(heapAssociation);

    *target = *m;
    return new VariantAssociationOwnPropertyKeyIterator;
}

PropertyAttributes VariantAssociationObject::virtualGetOwnProperty(
        const Managed *m, PropertyKey id, Property *p)
{
    Heap::VariantAssociationObject *heapAssociation
            = static_cast<const VariantAssociationObject *>(m)->d();

    if (!heapAssociation->object())
        return ReferenceObject::virtualGetOwnProperty(m, id, p);

    bool hasElement = false;
    Scope scope(heapAssociation->internalClass->engine);
    ScopedValue element(scope, heapAssociation->getElement(id.toQString(), &hasElement));

    if (!hasElement)
        return Attr_Invalid;

    if (p)
        p->value = element->asReturnedValue();

    return Attr_Data;
}

int VariantAssociationObject::virtualMetacall(
        Object *object, QMetaObject::Call call, int index, void **a)
{
    Heap::VariantAssociationObject *heapAssociation
            = static_cast<VariantAssociationObject *>(object)->d();

    // We only create attached wrappers if this variant association is itself attached.
    // When detaching, we re-create everything. Therefore, we can't get a metaCall if
    // we are detached.
    Q_ASSERT(heapAssociation->object());

    Q_ASSERT(heapAssociation->propertyIndexMapping);

    Scope scope(heapAssociation->internalClass->engine);
    Scoped<QV4::ArrayData> arrayData(scope, heapAssociation->propertyIndexMapping->arrayData);
    if (index < 0 || uint(index) >= arrayData->length())
        return 0;

    ScopedString scopedKey(scope, arrayData->get(index));

    switch (call) {
    case QMetaObject::ReadProperty: {
        QV4::ReferenceObject::readReference(heapAssociation);

        if (!visitVariantAssociation<bool>(heapAssociation, [&](const auto *association) {
            const auto it = association->constFind(scopedKey->toQString());
            if (it == association->constEnd())
                return false;

            *static_cast<QVariant *>(a[0]) = *it;
            return true;
        })) {
            return 0;
        }

        break;
    }
    case QMetaObject::WriteProperty: {
        if (!visitVariantAssociation<bool>(heapAssociation, [&](auto *association) {
            const auto it = association->find(scopedKey->toQString());
            if (it == association->end())
                return false;

            *it = *static_cast<const QVariant *>(a[0]);
            return true;
        })) {
            return 0;
        }

        QV4::ReferenceObject::writeBack(heapAssociation);
        break;
    }
    default:
        return 0; // not supported
    }

    return -1;
}

} // namespace QV4

QT_END_NAMESPACE
