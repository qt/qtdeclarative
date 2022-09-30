// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4REFERENCEOBJECT_P_H
#define QV4REFERENCEOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qv4object_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace Heap {


#define ReferenceObjectMembers(class, Member)

DECLARE_HEAP_OBJECT(ReferenceObject, Object) {
    DECLARE_MARKOBJECTS(ReferenceObject);

    enum Flag : quint8 {
        NoFlag           = 0,
        CanWriteBack     = 1 << 0,
        IsVariant        = 1 << 1,
        EnforcesLocation = 1 << 2,
    };
    Q_DECLARE_FLAGS(Flags, Flag);

    void init()
    {
        Object::init();
        m_object.init();
    }

    void destroy()
    {
        m_object.destroy();
        Object::destroy();
    }

    void setObject(QObject *object) { m_object = object; }
    QObject *object() const { return m_object; }

    void setProperty(int property) { m_property = property; }
    int property() const { return m_property; }

    void setCanWriteBack(bool canWriteBack) { setFlag(CanWriteBack, canWriteBack); }
    bool canWriteBack() const { return hasFlag(CanWriteBack); }

    void setIsVariant(bool isVariant) { setFlag(IsVariant, isVariant); }
    bool isVariant() const { return hasFlag(IsVariant); }


    void setEnforcesLocation(bool enforces) { setFlag(EnforcesLocation, enforces); }
    bool enforcesLocation() const { return hasFlag(EnforcesLocation); }

    void setLocation(const Function *function, quint16 statement)
    {
        m_function = function;
        m_statementIndex = statement;
    }

    const Function *function() const { return m_function; }
    quint16 statementIndex() const { return m_statementIndex; }

    bool isAttachedToProperty() const
    {
        if (enforcesLocation()) {
            if (CppStackFrame *frame = internalClass->engine->currentStackFrame) {
                if (frame->v4Function != function() || frame->statementNumber() != statementIndex())
                    return false;
            } else {
                return false;
            }
        }

        return true;
    }

    bool isReference() const { return m_property >= 0; }

private:

    bool hasFlag(Flag flag) const
    {
        return m_flags & quint8(flag);
    }

    void setFlag(Flag flag, bool set)
    {
        m_flags = set ? (m_flags | quint8(flag)) : (m_flags & ~quint8(flag));
    }

    QV4QPointer<QObject> m_object;
    const Function *m_function;
    int m_property;
    quint16 m_statementIndex;
    quint8 m_flags;
};

} // namespace Heap


struct ReferenceObject : public Object
{
    V4_OBJECT2(ReferenceObject, Object)
    V4_NEEDS_DESTROY

private:
    static bool doRead(QObject *o, int property, void **a)
    {
        return QMetaObject::metacall(o, QMetaObject::ReadProperty, property, a);
    }

    template<typename HeapObject>
    static bool doReadReference(const QMetaObject *metaObject, QObject *object, HeapObject *ref)
    {
        // A reference resource may be either a "true" reference (eg, to a QVector3D property)
        // or a "variant" reference (eg, to a QVariant property which happens to contain
        // a value-type).
        QMetaProperty referenceProperty = metaObject->property(ref->property());
        ref->setCanWriteBack(referenceProperty.isWritable());

        if (referenceProperty.userType() == QMetaType::QVariant) {
            // variant-containing-value-type reference
            QVariant variantReferenceValue;

            void *a[] = { &variantReferenceValue, nullptr };
            if (doRead(object, ref->property(), a)
                    && ref->setVariant(variantReferenceValue)) {
                ref->setIsVariant(true);
                return true;
            }
            return false;
        }

        // value-type reference
        void *args[] = { ref->storagePointer(), nullptr };
        return doRead(object, ref->property(), args);
    }

    template<typename HeapObject>
    static bool readObjectProperty(HeapObject *ref, QObject *object)
    {
        return doReadReference(object->metaObject(), object, ref);
    }

    static bool doWrite(QObject *o, int property, void **a)
    {
        return QMetaObject::metacall(o, QMetaObject::WriteProperty, property, a);
    }

    template<typename HeapObject>
    static bool doWriteBack(const QMetaObject *metaObject, QObject *object, HeapObject *ref)
    {
        const QMetaProperty writebackProperty = metaObject->property(ref->property());
        Q_ASSERT(writebackProperty.isWritable());

        int flags = 0;
        int status = -1;
        if (writebackProperty.metaType() == QMetaType::fromType<QVariant>()) {
            QVariant variantReferenceValue = ref->toVariant();
            void *a[] = { &variantReferenceValue, nullptr, &status, &flags };
            return doWrite(object, ref->property(), a);
        }

        void *a[] = { ref->storagePointer(), nullptr, &status, &flags };
        return doWrite(object, ref->property(), a);
    }

    template<typename HeapObject>
    static bool writeBackObjectProperty(HeapObject *ref, QObject *object)
    {
        return doWriteBack(object->metaObject(), object, ref);
    }

public:

    template<typename HeapObject>
    static bool readReference(HeapObject *self)
    {
        if (!self->object())
            return false;

        return readObjectProperty(self, self->object());
    }

    template<typename HeapObject>
    static bool writeBack(HeapObject *self)
    {
        if (!self->object() || !self->canWriteBack())
            return false;

        return writeBackObjectProperty(self, self->object());
    }
};

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4REFERENCEOBJECT_P_H
