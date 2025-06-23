// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4variantassociationobject_p.h"

#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

template<typename Return, typename MapCallable, typename HashCallable>
Return visitVariantAssociation(
    const QV4::Heap::VariantAssociationObject* association,
    MapCallable&& mapCallable,
    HashCallable&& hashCallable
) {
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
    QV4::Heap::VariantAssociationObject* association,
    MapCallable&& mapCallable,
    HashCallable&& hashCallable
) {
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
    const QV4::Heap::VariantAssociationObject* association,
    Callable&& callable
) {
    return visitVariantAssociation<Return>(association, callable, callable);
}

template<typename Return, typename Callable>
Return visitVariantAssociation(
    QV4::Heap::VariantAssociationObject* association,
    Callable&& callable
) {
    return visitVariantAssociation<Return>(association, callable, callable);
}

static void mapPropertyKey(std::vector<QString>& mapping, const QString& key) {
    auto it = std::find(mapping.cbegin(), mapping.cend(), key);
    if (it == mapping.cend())
        mapping.emplace_back(key);
}

static int keyToIndex(const std::vector<QString>& mapping, const QString& key) {
    auto it = std::find(mapping.cbegin(), mapping.cend(), key);
    return (it == mapping.cend()) ? -1 : std::distance(mapping.cbegin(), it);
}

static const QString& indexToKey(const std::vector<QString>& mapping, int index) {
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < static_cast<int>(mapping.size()));
    return mapping[index];
}

namespace QV4 {

    DEFINE_OBJECT_VTABLE(VariantAssociationObject);

    ReturnedValue VariantAssociationPrototype::fromQVariantMap(
        ExecutionEngine *engine,
        const QVariantMap& variantMap,
        QV4::Heap::Object* container,
        int property, Heap::ReferenceObject::Flags flags)
    {
        return engine->memoryManager->allocate<VariantAssociationObject>(
            variantMap, container, property, flags)->asReturnedValue();
    }

    ReturnedValue VariantAssociationPrototype::fromQVariantHash(
        ExecutionEngine *engine,
        const QVariantHash& variantHash,
        QV4::Heap::Object* container,
        int property, Heap::ReferenceObject::Flags flags)
    {
        return engine->memoryManager->allocate<VariantAssociationObject>(
            variantHash, container, property, flags)->asReturnedValue();
    }

    namespace Heap {
        void VariantAssociationObject::init(
            const QVariantMap& variantMap,
            QV4::Heap::Object* container,
            int property, Heap::ReferenceObject::Flags flags)
        {
            ReferenceObject::init(container, property, flags);

            new(m_variantAssociation) QVariantMap(variantMap);
            m_type = AssociationType::VariantMap;

            propertyIndexMapping = new std::vector<QString>(variantMap.keyBegin(), variantMap.keyEnd());
        }

        void VariantAssociationObject::init(
            const QVariantHash& variantHash,
            QV4::Heap::Object* container,
            int property, Heap::ReferenceObject::Flags flags)
        {
            ReferenceObject::init(container, property, flags);

            new(m_variantAssociation) QVariantHash(variantHash);
            m_type = AssociationType::VariantHash;

            propertyIndexMapping = new std::vector<QString>(variantHash.keyBegin(), variantHash.keyEnd());
        }

        void VariantAssociationObject::destroy() {
            visitVariantAssociation<void>(
                this,
                std::destroy_at<QVariantMap>,
                std::destroy_at<QVariantHash>);
            delete propertyIndexMapping;
            ReferenceObject::destroy();
        }

        QVariant VariantAssociationObject::toVariant() const
        {
            return visitVariantAssociation<QVariant>(
                this, [](auto association){ return QVariant(*association); });
        }

        bool VariantAssociationObject::setVariant(const QVariant &variant)
        {
            auto metatypeId = variant.metaType().id();

            if (metatypeId != QMetaType::QVariantMap && metatypeId != QMetaType::QVariantHash)
                return false;

            if (metatypeId == QMetaType::QVariantMap && m_type == AssociationType::VariantMap) {
                *reinterpret_cast<QVariantMap *>(&m_variantAssociation) = variant.toMap();
            } else if (metatypeId == QMetaType::QVariantMap && m_type == AssociationType::VariantHash) {
                std::destroy_at(reinterpret_cast<QVariantHash *>(&m_variantAssociation));
                new(m_variantAssociation) QVariantMap(variant.toMap());
                m_type = AssociationType::VariantMap;
            } else if (metatypeId == QMetaType::QVariantHash && m_type == AssociationType::VariantHash) {
                *reinterpret_cast<QVariantHash *>(&m_variantAssociation) = variant.toHash();
            } else if (metatypeId == QMetaType::QVariantHash && m_type == AssociationType::VariantMap) {
                std::destroy_at(reinterpret_cast<QVariantMap *>(&m_variantAssociation));
                new(m_variantAssociation) QVariantHash(variant.toHash());
                m_type = AssociationType::VariantHash;
            }

            auto keys = visitVariantAssociation<QStringList>(
                this, [](auto* association){ return association->keys(); });
            for (const QString& key : keys)
                mapPropertyKey(*propertyIndexMapping, key);

            return true;
        }

        VariantAssociationObject *VariantAssociationObject::detached() const
        {
            return visitVariantAssociation<VariantAssociationObject*>(
                this,
                [engine = internalClass->engine](auto association){
                    return engine->memoryManager->allocate<QV4::VariantAssociationObject>(
                        *association, nullptr, -1, ReferenceObject::Flag::NoFlag);
                }
            );
        }

    } // namespace Heap

    ReturnedValue VariantAssociationObject::virtualGet(const Managed *that, PropertyKey id, const Value *, bool * hasProperty)
    {
        QString key = id.toQString();
        const VariantAssociationObject *self = static_cast<const VariantAssociationObject *>(that);

        bool found = false;
        if (ReturnedValue result = self->getElement(key, &found); found) {
            if (hasProperty)
                *hasProperty = true;
            return result;
        }

        return ReferenceObject::virtualGet(that, id, that, hasProperty);
    }

    bool VariantAssociationObject::virtualPut(Managed *that, PropertyKey id, const Value &value, Value *)
    {
        QString key = id.toQString();
        return static_cast<VariantAssociationObject *>(that)->putElement(key, value);
    }

    bool VariantAssociationObject::virtualDeleteProperty(Managed *that, PropertyKey id)
    {
        QString key = id.toQString();
        return static_cast<VariantAssociationObject *>(that)->deleteElement(key);
    }

    OwnPropertyKeyIterator *VariantAssociationObject::virtualOwnPropertyKeys(
        const Object *m, Value *target
    ) {
        struct VariantAssociationOwnPropertyKeyIterator : ObjectOwnPropertyKeyIterator
        {
            QStringList keys;

            ~VariantAssociationOwnPropertyKeyIterator() override = default;

            PropertyKey next(const Object *o, Property *pd = nullptr, PropertyAttributes *attrs = nullptr) override
            {
                const VariantAssociationObject *variantAssociation =
                    static_cast<const VariantAssociationObject *>(o);

                if (memberIndex == 0) {
                    keys = variantAssociation->keys();
                    keys.sort();
                }

                if (static_cast<qsizetype>(memberIndex) < keys.count()) {
                    Scope scope(variantAssociation->engine());
                    ScopedString propertyName(scope, scope.engine->newString(keys[memberIndex]));
                    ScopedPropertyKey id(scope, propertyName->toPropertyKey());

                    if (attrs)
                        *attrs = QV4::Attr_Data;
                    if (pd)
                        pd->value = variantAssociation->getElement(keys[memberIndex]);

                    ++memberIndex;

                    return id;
                }

                return PropertyKey::invalid();
            }
        };

        QV4::ReferenceObject::readReference(static_cast<const VariantAssociationObject *>(m)->d());

        *target = *m;
        return new VariantAssociationOwnPropertyKeyIterator;
    }

    PropertyAttributes VariantAssociationObject::virtualGetOwnProperty(
        const Managed *m, PropertyKey id, Property *p
    ) {
        auto variantAssociation = static_cast<const VariantAssociationObject *>(m);

        bool hasElement = false;
        Scope scope(variantAssociation->engine());
        ScopedValue element(scope, variantAssociation->getElement(id.toQString(), &hasElement));

        if (!hasElement)
            return Attr_Invalid;

        if (p)
            p->value = element->asReturnedValue();

        return Attr_Data;
    }

    int VariantAssociationObject::virtualMetacall(Object *object, QMetaObject::Call call, int index, void **a)
    {
        VariantAssociationObject *variantAssociation = static_cast<VariantAssociationObject *>(object);
        Q_ASSERT(variantAssociation);

        Heap::VariantAssociationObject *heapAssociation = variantAssociation->d();

        switch (call) {
        case QMetaObject::ReadProperty: {
            QV4::ReferenceObject::readReference(heapAssociation);

            if (index < 0 || index >= static_cast<int>(heapAssociation->propertyIndexMapping->size()))
                return 0;

            const QString& key = indexToKey(*heapAssociation->propertyIndexMapping, index);

            if (!visitVariantAssociation<bool>(heapAssociation, [key](auto association) {
                return association->contains(key);
            })) {
                return 0;
            }

            visitVariantAssociation<void>(heapAssociation, [a, key](auto association) {
                *static_cast<QVariant*>(a[0]) = association->value(key);
            });

            break;
        }
        case QMetaObject::WriteProperty: {
            if (index < 0 || index >= static_cast<int>(heapAssociation->propertyIndexMapping->size()))
                return 0;

            const QString& key = indexToKey(*heapAssociation->propertyIndexMapping, index);

            visitVariantAssociation<void>(heapAssociation, [a, key](auto association){
                if (association->contains(key))
                    association->insert(key, *static_cast<QVariant*>(a[0]));
            });

            QV4::ReferenceObject::writeBack(heapAssociation);

            break;
        }
        default:
            return 0; // not supported
        }

        return -1;
    }

    QV4::ReturnedValue VariantAssociationObject::getElement(const QString& key, bool *hasProperty) const {
        QV4::ReferenceObject::readReference(d());

        return visitVariantAssociation<QV4::ReturnedValue>(
            d(),
            [engine = engine(), this, key, hasProperty](auto* association) {
                bool hasElement = association->contains(key);
                if (hasProperty)
                    *hasProperty = hasElement;
                return engine->fromVariant(
                    association->value(key),
                    d(), hasElement ? keyToIndex(*d()->propertyIndexMapping, key) : -1,
                    Heap::ReferenceObject::Flag::CanWriteBack |
                    Heap::ReferenceObject::Flag::IsVariant);
            }
        );
    }

    bool VariantAssociationObject::putElement(const QString& key, const Value& value) {
        Heap::VariantAssociationObject *heapAssociation = d();

        visitVariantAssociation<void>(heapAssociation, [engine = engine(), value, key](auto association){
            association->insert(key, engine->toVariant(value, QMetaType{}, false));
        });

        mapPropertyKey(*heapAssociation->propertyIndexMapping, key);

        QV4::ReferenceObject::writeBack(heapAssociation);
        return true;
    }

    bool VariantAssociationObject::deleteElement(const QString& key) {
        bool result = visitVariantAssociation<bool>(d(), [key](auto association) {
            return association->remove(key);
        });

        if (result)
            QV4::ReferenceObject::writeBack(d());

        return result;
    }

    QStringList VariantAssociationObject::keys() const {
        return visitVariantAssociation<QStringList>(d(), [](auto association){
            return association->keys();
        });
    }
} // namespace QV4

QT_END_NAMESPACE
