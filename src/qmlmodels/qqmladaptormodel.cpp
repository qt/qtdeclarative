// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmladaptormodel_p.h"

#include <private/qqmldmabstractitemmodeldata_p.h>
#include <private/qqmldmlistaccessordata_p.h>
#include <private/qqmldmobjectdata_p.h>

QT_BEGIN_NAMESPACE

QQmlAdaptorModel::Accessors::~Accessors()
{
}

QQmlAdaptorModel::QQmlAdaptorModel()
    : QQmlGuard<QObject>(QQmlAdaptorModel::objectDestroyedImpl, nullptr)
    , accessors(&m_nullAccessors)
{
}

QQmlAdaptorModel::~QQmlAdaptorModel()
{
    accessors->cleanup(*this);
}

void QQmlAdaptorModel::setModel(const QVariant &variant)
{
    accessors->cleanup(*this);

    // Don't use variant anymore after this. list may transform it.
    list.setList(variant);

    modelStrongReference.clear();

    if (QObject *object = qvariant_cast<QObject *>(list.list())) {
        if (QQmlData *ddata = QQmlData::get(object))
            modelStrongReference = ddata->jsWrapper;
        setObject(object);
        if (qobject_cast<QAbstractItemModel *>(object))
            accessors = new VDMAbstractItemModelDataType(this);
        else
            accessors = new VDMObjectDelegateDataType;
    } else if (list.type() == QQmlListAccessor::ListProperty) {
        auto object = static_cast<const QQmlListReference *>(list.list().constData())->object();
        if (QQmlData *ddata = QQmlData::get(object))
            modelStrongReference = ddata->jsWrapper;
        setObject(object);
        accessors = new VDMObjectDelegateDataType;
    } else if (list.type() == QQmlListAccessor::ObjectList) {
        setObject(nullptr);
        accessors = new VDMObjectDelegateDataType;
    } else if (list.type() != QQmlListAccessor::Invalid
            && list.type() != QQmlListAccessor::Instance) { // Null QObject
        setObject(nullptr);
        accessors = new VDMListDelegateDataType(this);
    } else {
        setObject(nullptr);
        accessors = &m_nullAccessors;
    }
}

void QQmlAdaptorModel::invalidateModel()
{
    accessors->cleanup(*this);
    accessors = &m_nullAccessors;
    // Don't clear the model object as we still need the guard to clear the list variant if the
    // object is destroyed.
}

bool QQmlAdaptorModel::isValid() const
{
    return accessors != &m_nullAccessors;
}

int QQmlAdaptorModel::count() const
{
    return rowCount() * columnCount();
}

int QQmlAdaptorModel::rowCount() const
{
    return qMax(0, accessors->rowCount(*this));
}

int QQmlAdaptorModel::columnCount() const
{
    return qMax(0, accessors->columnCount(*this));
}

int QQmlAdaptorModel::rowAt(int index) const
{
    int count = rowCount();
    return count <= 0 ? -1 : index % count;
}

int QQmlAdaptorModel::columnAt(int index) const
{
    int count = rowCount();
    return count <= 0 ? -1 : index / count;
}

int QQmlAdaptorModel::indexAt(int row, int column) const
{
    return column * rowCount() + row;
}

void QQmlAdaptorModel::useImportVersion(QTypeRevision revision)
{
    modelItemRevision = revision;
}

void QQmlAdaptorModel::objectDestroyedImpl(QQmlGuardImpl *guard)
{
    auto This = static_cast<QQmlAdaptorModel *>(guard);
    This->setModel(QVariant());
}

QT_END_NAMESPACE
