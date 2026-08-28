// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qobjectregistryref_p.h"

#include <private/qobjectregistrysingleton_p.h>

#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*!
    \class QObjectRegistryRef
    \inmodule QtQmlDesignSupport
    \ingroup qmldesignsupport
    \brief This class is used to access an object registered with ObjectRegistry QML type from
           C++ code.

    Example usage:
    \snippet doc_src_objectregistry.cpp 0
    \codeline
    \snippet doc_src_objectregistry.cpp 2

    \sa ObjectRegistry, QAbstractObjectRegistryRef::key
*/

/*!
    \qmltype ObjectRegistryRef
    \nativetype QObjectRegistryRef
    \inqmlmodule QtQml.DesignSupport
    \ingroup qmldesignsupport_qml
    \inherits AbstractObjectRegistryRef
    \brief This type is used to access an object registered with ObjectRegistry type.

    Example usage:
    \snippet doc_src_objectregistry.cpp 0
    \codeline
    \snippet doc_src_objectregistry.cpp 1

    \sa ObjectRegistry, AbstractObjectRegistryRef::key
*/

/*!
    \property QObjectRegistryRef::object
    \brief The object referenced by this instance.

    If multiple objects are registered with the same key, the object referenced by this instance
    is an unspecified one of among them. Use QMultiObjectRegistryRef instead in that case.

    \sa ObjectRegistry, QAbstractObjectRegistryRef::key
*/

/*!
    \qmlproperty QtObject ObjectRegistryRef::object

    This property specifies the object referenced by this instance.

    If multiple objects are registered with the same key, this instance references an unspecified
    object from that group. In this case, use QMultiObjectRegistryRef.

    \sa ObjectRegistry, AbstractObjectRegistryRef::key
*/

/*!
    Constructs QObjectRegistryRef instance with \a parent.

    \note Use this constructor only if the correct QML engine resolves automatically when the key
    is set. This generally does not happen when creating an instance of this class from C++.
    It is recommended to use one of the constructors that take a QML engine pointer as a parameter.

    \sa qmlEngine(), QAbstractObjectRegistryRef::key
*/
QObjectRegistryRef::QObjectRegistryRef(QObject *parent)
    : QAbstractObjectRegistryRef(*new QObjectRegistryRefPrivate(), parent)
{}

/*!
    Constructs QObjectRegistryRef instance with QML \a engine and \a parent.
    All registrations are scoped to the specified QML engine.

    \sa QAbstractObjectRegistryRef::key
*/
QObjectRegistryRef::QObjectRegistryRef(QQmlEngine *engine, QObject *parent)
    : QAbstractObjectRegistryRef(*new QObjectRegistryRefPrivate(engine), parent)
{}

/*!
    Constructs QObjectRegistryRef instance with QML \a engine, \a key and \a parent.
    All registrations are scoped to the specified QML engine.

    \sa QAbstractObjectRegistryRef::key
*/
QObjectRegistryRef::QObjectRegistryRef(QQmlEngine *engine, const QString &key, QObject *parent)
    : QAbstractObjectRegistryRef(*new QObjectRegistryRefPrivate(engine), parent)
{
    setKey(key);
}

QObjectRegistryRef::~QObjectRegistryRef() = default;

QObject *QObjectRegistryRef::object() const
{
    Q_D(const QObjectRegistryRef);

    return d->m_object;
}

/*!
    \reimp
*/
bool QObjectRegistryRef::event(QEvent *event)
{
    return QAbstractObjectRegistryRef::event(event);
}

void QObjectRegistryRefPrivate::handleInitialObjects()
{
    if (!registry())
        return;

    QList<QObject *> objects = registry()->objects(key()).values();

    if (objects.size() > 1)
        printMultiWarning();

    QObject *newObject = nullptr;

    if (!objects.isEmpty())
        newObject = *objects.cbegin();

    setObject(newObject);
}

void QObjectRegistryRefPrivate::handleObjectAdded(QObject *obj)
{
    if (m_object && m_object != obj) {
        printMultiWarning();
        return;
    }
    setObject(obj);
}

void QObjectRegistryRefPrivate::handleObjectRemoved(QObject *obj)
{
    if (m_object != obj)
        return;
    setObject(nullptr);
}

QObjectRegistryRefPrivate::QObjectRegistryRefPrivate(QQmlEngine *engine)
    : QAbstractObjectRegistryRefPrivate(engine)
{}

void QObjectRegistryRefPrivate::setObject(QObject *obj)
{
    Q_Q(QObjectRegistryRef);

    if (obj == m_object)
        return;

    m_object = obj;
    emit q->objectChanged(m_object);
}

void QObjectRegistryRefPrivate::printMultiWarning() const
{
    Q_Q(const QObjectRegistryRef);

    qmlWarning(q) << "ObjectRegistryRef found multiple objects registered with the same key ("
                  << q->key()
                  << "). MultiObjectRegistryRef should be used instead.";
}

QT_END_NAMESPACE
