// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qqmldmlistaccessordata_p.h>

QT_BEGIN_NAMESPACE

QQmlDMListAccessorData::QQmlDMListAccessorData(
        const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
        VDMListDelegateDataType *dataType,
        int index, int row, int column, const QVariant &value)
    : QQmlDelegateModelItem(metaType, dataType, index, row, column)
    , cachedData(value)
{
    QObjectPrivate::get(this)->metaObject = dataType;
    dataType->addref();
}

void QQmlDMListAccessorData::setModelData(const QVariant &data) {
    if (data == cachedData)
        return;

    cachedData = data;
    static_cast<const VDMListDelegateDataType *>(QObjectPrivate::get(this)->metaObject)
            ->emitAllSignals(this);
}

void QQmlDMListAccessorData::setValue(const QString &role, const QVariant &value)
{
    // Used only for initialization of the cached data. Does not have to emit change signals.
    if (role == QLatin1String("modelData") || role.isEmpty())
        cachedData = value;
    else
        VDMListDelegateDataType::setValue(&cachedData, role, value);
}

bool QQmlDMListAccessorData::resolveIndex(const QQmlAdaptorModel &model, int idx)
{
    if (index == -1) {
        index = idx;
        setModelData(model.list.at(idx));
        emit modelIndexChanged();
        return true;
    } else {
        return false;
    }
}

void VDMListDelegateDataType::emitAllSignals(QQmlDMListAccessorData *accessor) const
{
    for (int i = propertyOffset, end = propertyCount(); i != end; ++i)
        QMetaObject::activate(accessor, this, i - propertyOffset, nullptr);
    emit accessor->modelDataChanged();
}

int VDMListDelegateDataType::metaCall(
        QObject *object, QMetaObject::Call call, int id, void **arguments)
{
    Q_ASSERT(qobject_cast<QQmlDMListAccessorData *>(object));
    QQmlDMListAccessorData *accessor = static_cast<QQmlDMListAccessorData *>(object);

    switch (call) {
    case QMetaObject::ReadProperty: {
        if (id < propertyOffset)
            break;

        QVariant *result = static_cast<QVariant *>(arguments[0]);
        const QByteArray name = property(id).name();
        const QVariant data = accessor->index == -1
                ? accessor->modelData()
                : model->list.at(accessor->index);
        *result = value(&data, name);
        return -1;
    }
    case QMetaObject::WriteProperty: {
        if (id < propertyOffset)
            break;

        const QVariant &argument = *static_cast<QVariant *>(arguments[0]);
        const QByteArray name = property(id).name();
        QVariant data = accessor->index == -1
                ? accessor->modelData()
                : model->list.at(accessor->index);
        if (argument == value(&data, name))
            return -1;
        setValue(&data, name, argument);
        if (accessor->index == -1)
            accessor->cachedData = data;
        else
            model->list.set(accessor->index, data);
        QMetaObject::activate(accessor, this, id - propertyOffset, nullptr);
        emit accessor->modelDataChanged();
        return -1;
    }
    default:
        break;
    }

    return accessor->qt_metacall(call, id, arguments);
}

int VDMListDelegateDataType::createProperty(const char *name, const char *)
{
    const int propertyIndex = propertyCount() - propertyOffset;

    // We use QVariant because the types may be different in the different objects.
    QQmlAdaptorModelEngineData::addProperty(
            &builder, propertyIndex, name, QByteArrayLiteral("QVariant"));

    metaObject.reset(builder.toMetaObject());
    *static_cast<QMetaObject *>(this) = *metaObject;
    return propertyIndex + propertyOffset;
}

QMetaObject *VDMListDelegateDataType::toDynamicMetaObject(QObject *object)
{
    if (const QQmlRefPointer<QQmlContextData> &contextData
            = static_cast<QQmlDMListAccessorData *>(object)->contextData) {
        if (contextData->contextObject() == object) {
            // We are using context properties. There should be a propertyCache so that row and
            // column are hidden. We shall also return the static metaObject in that case.

            if (!propertyCache) {
                propertyCache = QQmlPropertyCache::createStandalone(
                        &QQmlDMListAccessorData::staticMetaObject, model->modelItemRevision);
                if (QQmlData *ddata = QQmlData::get(object, true))
                    ddata->propertyCache = propertyCache;
            }

            // ### Qt 7: Return const from toDynamicMetaObject() and drop the const_cast.
            return const_cast<QMetaObject *>(&QQmlDMListAccessorData::staticMetaObject);
        }
    }

    // If the context object is not the model object, we are using required properties.
    // In that case, allow the creation of extra properties.
    return this;
}

QT_END_NAMESPACE
