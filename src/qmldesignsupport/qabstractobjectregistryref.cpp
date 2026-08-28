// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qabstractobjectregistryref_p.h"

#include <private/qobjectregistrysingleton_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractObjectRegistryRef
    \inmodule QtQmlDesignSupport
    \ingroup qmldesignsupport
    \brief The QAbstractObjectRegistryRef class is a base class for all
           \QDesSup object reference classes.

    This class can't be instantiated on its own, you must use one of the following derived
    classes: QObjectRegistryRef or QMultiObjectRegistryRef

    If multiple objects are registered with the same key, use the derived class
    QMultiObjectRegistryRef. Typically this happens when delegate objects
    instantiated by models are registered. If you know the key is used to register only a single
    object, using the derived class QObjectRegistryRef is recommended.
*/

/*!
    \qmltype AbstractObjectRegistryRef
    \nativetype QAbstractObjectRegistryRef
    \inqmlmodule QtQml.DesignSupport
    \ingroup qmldesignsupport_qml
    \brief Base type for all \QDesSup object reference types.

    This type cannot be instantiated directly. Instead, use one of the following derived types:
    ObjectRegistryRef or MultiObjectRegistryRef.

    If multiple objects are registered with the same key, use the derived type
    MultiObjectRegistryRef. Typically this happens when delegate objects
    instantiated by models are registered. If you know the key is used to register only a single
    object, using the derived type ObjectRegistryRef is recommended.
*/

/*!
    \property QAbstractObjectRegistryRef::key
    \brief The key of the registered object that this reference refers to.
    \sa ObjectRegistry
*/

/*!
    \qmlproperty string AbstractObjectRegistryRef::key
    This property specifies the key of the registered object that this reference refers to.
    \sa ObjectRegistry
*/

/*!
    \property QAbstractObjectRegistryRef::data
    \brief QML children of this type.
    \internal
*/


QAbstractObjectRegistryRef::QAbstractObjectRegistryRef(QAbstractObjectRegistryRefPrivate &dd,
                                                       QObject *parent)
    : QObject(dd, parent)
{}

QAbstractObjectRegistryRef::~QAbstractObjectRegistryRef()
{
    Q_D(QAbstractObjectRegistryRef);

    if (d->m_registry)
        d->m_registry->deregisterRef(d);
}

QString QAbstractObjectRegistryRef::key() const
{
    Q_D(const QAbstractObjectRegistryRef);

    return d->m_key;
}

void QAbstractObjectRegistryRef::setKey(const QString &key)
{
    Q_D(QAbstractObjectRegistryRef);

    if (key == d->m_key)
        return;

    if (!d->m_registry) {
        d->m_registry = QObjectRegistrySingleton::registryForObject(this);
        if (!d->m_registry) {
            qWarning() << "Object registry could not be resolved for ("
                       << key
                       << ") when setting key to QAbstractObjectRegistryRef."
                       << " Most likely reason for this is that QML engine could not be resolved.";
            d->m_key = key;
            emit keyChanged(d->m_key);
            return;
        }
    }

    if (!d->m_key.isEmpty() && d->m_registry)
        d->m_registry->deregisterRef(d);

    d->m_key = key;

    if (!d->m_key.isEmpty() && d->m_registry)
        d->m_registry->registerRef(d);

    d->handleInitialObjects();

    emit keyChanged(d->m_key);
}

QQmlListProperty<QObject> QAbstractObjectRegistryRef::data()
{
    return { this, nullptr,
            &QAbstractObjectRegistryRefPrivate::dataAppend,
            &QAbstractObjectRegistryRefPrivate::dataCount,
            &QAbstractObjectRegistryRefPrivate::dataAt,
            &QAbstractObjectRegistryRefPrivate::dataClear,
            &QAbstractObjectRegistryRefPrivate::dataReplace,
            &QAbstractObjectRegistryRefPrivate::dataRemoveLast };
}

/*!
    \reimp
*/
bool QAbstractObjectRegistryRef::event(QEvent *event)
{
    return QObject::event(event);
}

QAbstractObjectRegistryRefPrivate::QAbstractObjectRegistryRefPrivate(QQmlEngine *engine)
{
    m_registry = QObjectRegistrySingleton::registryForEngine(engine);
}

QString QAbstractObjectRegistryRefPrivate::key() const
{
    return m_key;
}

QObjectRegistrySingleton *QAbstractObjectRegistryRefPrivate::registry() const
{
    return m_registry;
}

void QAbstractObjectRegistryRefPrivate::dataAppend(QQmlListProperty<QObject> *l, QObject *o)
{
    auto *self = static_cast<QAbstractObjectRegistryRef *>(l->object);
    self->d_func()->m_data.append(o);

    emit self->dataChanged();
}

qsizetype QAbstractObjectRegistryRefPrivate::dataCount(QQmlListProperty<QObject> *l)
{
    return static_cast<QAbstractObjectRegistryRef *>(l->object)->d_func()->m_data.size();
}

QObject *QAbstractObjectRegistryRefPrivate::dataAt(QQmlListProperty<QObject> *l, qsizetype i)
{
    return static_cast<QAbstractObjectRegistryRef *>(l->object)->d_func()->m_data.at(i);
}

void QAbstractObjectRegistryRefPrivate::dataClear(QQmlListProperty<QObject> *l)
{
    auto *self = static_cast<QAbstractObjectRegistryRef *>(l->object);

    if (self->d_func()->m_data.isEmpty())
        return;

    self->d_func()->m_data.clear();

    emit self->dataChanged();
}

void QAbstractObjectRegistryRefPrivate::dataReplace(QQmlListProperty<QObject> *l, qsizetype i, QObject *o)
{
    auto *self = static_cast<QAbstractObjectRegistryRef *>(l->object);

    if (o == self->d_func()->m_data.at(i))
        return;

    self->d_func()->m_data.replace(i, o);

    emit self->dataChanged();
}

void QAbstractObjectRegistryRefPrivate::dataRemoveLast(QQmlListProperty<QObject> *l)
{
    auto *self = static_cast<QAbstractObjectRegistryRef *>(l->object);

    if (self->d_func()->m_data.isEmpty())
        return;

    self->d_func()->m_data.removeLast();

    emit self->dataChanged();
}

QT_END_NAMESPACE
