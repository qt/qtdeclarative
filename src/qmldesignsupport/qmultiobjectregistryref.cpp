// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmultiobjectregistryref_p.h"

#include <private/qobjectregistrysingleton_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMultiObjectRegistryRef
    \inmodule QtQmlDesignSupport
    \ingroup qmldesignsupport
    \brief This class is used to access objects registered with ObjectRegistry QML type from
           C++ code.

    Multiple objects registered with the same key can be accessed via this reference.

    Example usage:
    \snippet doc_src_objectregistry.cpp 3
    \codeline
    \snippet doc_src_objectregistry.cpp 5

    \sa ObjectRegistry, QAbstractObjectRegistryRef::key
*/

/*!
    \qmltype MultiObjectRegistryRef
    \nativetype QObjectRegistryRef
    \inqmlmodule QtQml.DesignSupport
    \ingroup qmldesignsupport_qml
    \inherits AbstractObjectRegistryRef
    \brief This type is used to access objects registered with ObjectRegistry type.

    Multiple objects registered with the same key can be accessed via this reference.

    Example usage:
    \snippet doc_src_objectregistry.cpp 3
    \codeline
    \snippet doc_src_objectregistry.cpp 4

    \sa ObjectRegistry, AbstractObjectRegistryRef::key
*/

/*!
    \property QMultiObjectRegistryRef::objects

    This property provides the objects referenced by this instance.
    The order of the objects in this list is not guaranteed to match the order of their
    registration.

    \sa objectsList(), ObjectRegistry, AbstractObjectRegistryRef::key
*/

/*!
    \qmlproperty list<QtObject> MultiObjectRegistryRef::objects

    This property provides the objects referenced by this instance.
    The order of the objects in this list is not guaranteed to match the order of their
    registration.

    \sa ObjectRegistry, AbstractObjectRegistryRef::key
*/

/*!
    Constructs QMultiObjectRegistryRef instance with \a parent.

    \note Use this constructor only if the correct QML engine resolves automatically for this
    instance when the key is set. This is typically not the case when constructing an instance
    of this class from C++.
    It is recommended to use one of the constructors that take a QML engine pointer as a parameter.

    \sa qmlEngine(), QAbstractObjectRegistryRef::key
*/
QMultiObjectRegistryRef::QMultiObjectRegistryRef(QObject *parent)
    : QAbstractObjectRegistryRef(*new QMultiObjectRegistryRefPrivate(), parent)
{}

/*!
    Constructs QMultiObjectRegistryRef instance with QML \a engine and \a parent.
    All registrations are scoped to the specified QML engine.

    \sa QAbstractObjectRegistryRef::key
*/
QMultiObjectRegistryRef::QMultiObjectRegistryRef(QQmlEngine *engine, QObject *parent)
    : QAbstractObjectRegistryRef(*new QMultiObjectRegistryRefPrivate(engine), parent)
{}

/*!
    Constructs QMultiObjectRegistryRef instance with QML \a engine, \a key and \a parent.
    All registrations are scoped to the specified QML engine.

    \sa QAbstractObjectRegistryRef::key
*/
QMultiObjectRegistryRef::QMultiObjectRegistryRef(QQmlEngine *engine, const QString &key, QObject *parent)
    : QAbstractObjectRegistryRef(*new QMultiObjectRegistryRefPrivate(engine), parent)
{
    setKey(key);
}

QMultiObjectRegistryRef::~QMultiObjectRegistryRef() = default;

QQmlListProperty<QObject> QMultiObjectRegistryRef::objects()
{
    return { this, nullptr,
            &QMultiObjectRegistryRefPrivate::objectsCount,
            &QMultiObjectRegistryRefPrivate::objectsAt };
}

/*!
    \return the list of registered objects matching the key of this instance.

    This is a method to get the list of registered objects as a QList.

    The order of the objects in this list is not guaranteed to match the order of their
    registration.

    \sa QAbstractObjectRegistryRef::key, objects
*/
QList<QObject *> QMultiObjectRegistryRef::objectsList()
{
    Q_D(const QMultiObjectRegistryRef);

    return d->m_objects;
}

/*!
    \fn void QMultiObjectRegistryRef::objectAdded(QObject *obj)

    This signal is emitted when a new object, \a obj, has been registered with the key of this
    reference.
*/

/*!
    \qmlsignal MultiObjectRegistryRef::objectAdded(QtObject obj)

    This signal is emitted when a new object, \a obj, has been registered with the key of this
    reference.
*/

/*!
    \fn void QMultiObjectRegistryRef::objectRemoved(QObject *obj)

    This signal is emitted when an object, \a obj, registration has been removed with the key of this
    reference.
*/

/*!
    \qmlsignal MultiObjectRegistryRef::objectRemoved(QtObject obj)

    This signal is emitted when an object, \a obj, registration has been removed with the key of this
    reference.

    \note If the registration removal was triggered by the object deletion, the \a obj parameter
    will be null as QML is not able to resolve an object being deleted.
*/

void QMultiObjectRegistryRefPrivate::handleInitialObjects()
{
    Q_Q(QMultiObjectRegistryRef);

    if (!registry())
        return;

    QList<QObject *> oldObjects = m_objects;
    QList<QObject *> newObjects = registry()->objects(key()).values();

    if (oldObjects == newObjects)
        return;

    m_objects = newObjects;

    for (const auto obj : std::as_const(oldObjects))
        newObjects.removeOne(obj);
    for (const auto obj : std::as_const(m_objects))
        oldObjects.removeOne(obj);

    for (const auto obj : std::as_const(oldObjects))
        emit q->objectRemoved(obj);

    for (const auto obj : std::as_const(newObjects))
        emit q->objectAdded(obj);

    emit q->objectsChanged();
}

QMultiObjectRegistryRefPrivate::QMultiObjectRegistryRefPrivate(QQmlEngine *engine)
    : QAbstractObjectRegistryRefPrivate(engine)
{}

void QMultiObjectRegistryRefPrivate::handleObjectAdded(QObject *obj)
{
    Q_Q(QMultiObjectRegistryRef);

    m_objects.append(obj);

    emit q->objectAdded(obj);
    emit q->objectsChanged();
}

void QMultiObjectRegistryRefPrivate::handleObjectRemoved(QObject *obj)
{
    Q_Q(QMultiObjectRegistryRef);

    m_objects.removeOne(obj);

    emit q->objectRemoved(obj);
    emit q->objectsChanged();
}

qsizetype QMultiObjectRegistryRefPrivate::objectsCount(QQmlListProperty<QObject> *l)
{
    return static_cast<QMultiObjectRegistryRef *>(l->object)->d_func()->m_objects.size();
}

QObject *QMultiObjectRegistryRefPrivate::objectsAt(QQmlListProperty<QObject> *l, qsizetype i)
{
    return static_cast<QMultiObjectRegistryRef *>(l->object)->d_func()->m_objects.at(i);
}

QT_END_NAMESPACE
