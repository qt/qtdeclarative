// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDMLISTACCESSORDATA_P_H
#define QQMLDMLISTACCESSORDATA_P_H

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

#include <private/qqmladaptormodelenginedata_p.h>
#include <private/qqmldelegatemodel_p_p.h>
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QQmlDMListAccessorData : public QQmlDelegateModelItem
{
    Q_OBJECT
    Q_PROPERTY(QVariant modelData READ modelData WRITE setModelData NOTIFY modelDataChanged)
    QT_ANONYMOUS_PROPERTY(QVariant READ modelData WRITE setModelData NOTIFY modelDataChanged)
public:
    QQmlDMListAccessorData(const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
                           QQmlAdaptorModel::Accessors *accessor,
                           int index, int row, int column, const QVariant &value)
        : QQmlDelegateModelItem(metaType, accessor, index, row, column)
        , cachedData(value)
    {
    }

    QVariant modelData() const
    {
        return cachedData;
    }

    void setModelData(const QVariant &data)
    {
        if (data == cachedData)
            return;

        cachedData = data;
        emit modelDataChanged();
    }

    static QV4::ReturnedValue get_modelData(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *, int)
    {
        QV4::ExecutionEngine *v4 = b->engine();
        const QQmlDelegateModelItemObject *o = thisObject->as<QQmlDelegateModelItemObject>();
        if (!o)
            return v4->throwTypeError(QStringLiteral("Not a valid DelegateModel object"));

        return v4->fromVariant(static_cast<QQmlDMListAccessorData *>(o->d()->item)->cachedData);
    }

    static QV4::ReturnedValue set_modelData(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *argv, int argc)
    {
        QV4::ExecutionEngine *v4 = b->engine();
        const QQmlDelegateModelItemObject *o = thisObject->as<QQmlDelegateModelItemObject>();
        if (!o)
            return v4->throwTypeError(QStringLiteral("Not a valid DelegateModel object"));
        if (!argc)
            return v4->throwTypeError();

        static_cast<QQmlDMListAccessorData *>(o->d()->item)->setModelData(
                    QV4::ExecutionEngine::toVariant(argv[0], QMetaType {}));
        return QV4::Encode::undefined();
    }

    QV4::ReturnedValue get() override
    {
        QQmlAdaptorModelEngineData *data = QQmlAdaptorModelEngineData::get(v4);
        QV4::Scope scope(v4);
        QV4::ScopedObject o(scope, v4->memoryManager->allocate<QQmlDelegateModelItemObject>(this));
        QV4::ScopedObject p(scope, data->listItemProto.value());
        o->setPrototypeOf(p);
        ++scriptRef;
        return o.asReturnedValue();
    }

    void setValue(const QString &role, const QVariant &value) override
    {
        if (role == QLatin1String("modelData") || role.isEmpty())
            cachedData = value;
    }

    bool resolveIndex(const QQmlAdaptorModel &model, int idx) override
    {
        if (index == -1) {
            index = idx;
            cachedData = model.list.at(idx);
            emit modelIndexChanged();
            emit modelDataChanged();
            return true;
        } else {
            return false;
        }
    }


Q_SIGNALS:
    void modelDataChanged();

private:
    QVariant cachedData;
};


class VDMListDelegateDataType : public QQmlRefCount, public QQmlAdaptorModel::Accessors
{
public:
    VDMListDelegateDataType(QQmlAdaptorModel *model)
        : QQmlRefCount()
        , QQmlAdaptorModel::Accessors()
    {
        Q_UNUSED(model)
    }

    void cleanup(QQmlAdaptorModel &) const override
    {
        release();
    }

    int rowCount(const QQmlAdaptorModel &model) const override
    {
        return model.list.count();
    }

    int columnCount(const QQmlAdaptorModel &) const override
    {
        return 1;
    }

    QVariant value(const QQmlAdaptorModel &model, int index, const QString &role) const override
    {
        const QVariant entry = model.list.at(index);
        if (role == QLatin1String("modelData") || role.isEmpty())
            return entry;

        const QMetaType type = entry.metaType();
        if (type == QMetaType::fromType<QVariantMap>())
            return entry.toMap().value(role);

        if (type == QMetaType::fromType<QVariantHash>())
            return entry.toHash().value(role);

        const QMetaType::TypeFlags typeFlags = type.flags();
        if (typeFlags & QMetaType::PointerToQObject)
            return entry.value<QObject *>()->property(role.toUtf8());

        const QMetaObject *metaObject = type.metaObject();
        if (!metaObject) {
            // NB: This acquires the lock on QQmlMetaTypeData. If we had a QQmlEngine here,
            //     we could use QQmlGadgetPtrWrapper::instance() to avoid this.
            if (const QQmlValueType *valueType = QQmlMetaType::valueType(type))
                metaObject = valueType->staticMetaObject();
            else
                return QVariant();
        }

        const int propertyIndex = metaObject->indexOfProperty(role.toUtf8());
        return metaObject->property(propertyIndex).readOnGadget(entry.constData());
    }

    QQmlDelegateModelItem *createItem(
            QQmlAdaptorModel &model,
            const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
            int index, int row, int column) override
    {
        if (!propertyCache) {
            propertyCache = QQmlPropertyCache::createStandalone(
                        &QQmlDMListAccessorData::staticMetaObject, model.modelItemRevision);
        }

        return new QQmlDMListAccessorData(
                metaType,
                this,
                index, row, column,
                index >= 0 && index < model.list.count() ? model.list.at(index) : QVariant());
    }

    bool notify(const QQmlAdaptorModel &model, const QList<QQmlDelegateModelItem *> &items, int index, int count, const QVector<int> &) const override
    {
        for (auto modelItem : items) {
            const int modelItemIndex = modelItem->index;
            if (modelItemIndex < index || modelItemIndex >= index + count)
                continue;

            auto listModelItem = static_cast<QQmlDMListAccessorData *>(modelItem);
            QVariant updatedModelData = model.list.at(listModelItem->index);
            listModelItem->setModelData(updatedModelData);
        }
        return true;
    }
};

QT_END_NAMESPACE

#endif // QQMLDMLISTACCESSORDATA_P_H
