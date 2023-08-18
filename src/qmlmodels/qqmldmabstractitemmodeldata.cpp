// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qqmldmabstractitemmodeldata_p.h>

QT_BEGIN_NAMESPACE

QQmlDMAbstractItemModelData::QQmlDMAbstractItemModelData(
        const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
        VDMAbstractItemModelDataType *dataType, int index, int row, int column)
    : QQmlDelegateModelItem(metaType, dataType, index, row, column)
    , m_type(dataType)
{
    if (index == -1)
        m_cachedData.resize(m_type->propertyRoles.size());

    QObjectPrivate::get(this)->metaObject = m_type;

    m_type->addref();
}

int QQmlDMAbstractItemModelData::metaCall(QMetaObject::Call call, int id, void **arguments)
{
    if (call == QMetaObject::ReadProperty && id >= m_type->propertyOffset) {
        const int propertyIndex = id - m_type->propertyOffset;
        if (index == -1) {
            if (!m_cachedData.isEmpty())
                *static_cast<QVariant *>(arguments[0]) = m_cachedData.at(propertyIndex);
        } else  if (*m_type->model) {
            *static_cast<QVariant *>(arguments[0]) = value(m_type->propertyRoles.at(propertyIndex));
        }
        return -1;
    } else if (call == QMetaObject::WriteProperty && id >= m_type->propertyOffset) {
        const int propertyIndex = id - m_type->propertyOffset;
        const QMetaObject *meta = metaObject();
        if (index == -1) {
            if (m_cachedData.size() > 1) {
                m_cachedData[propertyIndex] = *static_cast<QVariant *>(arguments[0]);
                QMetaObject::activate(this, meta, propertyIndex, nullptr);
            } else if (m_cachedData.size() == 1) {
                m_cachedData[0] = *static_cast<QVariant *>(arguments[0]);
                QMetaObject::activate(this, meta, 0, nullptr);
            }
        } else if (*m_type->model) {
            QQmlGuard<QQmlDMAbstractItemModelData> guard(this);
            setValue(m_type->propertyRoles.at(propertyIndex), *static_cast<QVariant *>(arguments[0]));
            if (guard.isNull())
              return -1;

            QMetaObject::activate(this, meta, propertyIndex, nullptr);
        }
        emit modelDataChanged();
        return -1;
    } else {
        return qt_metacall(call, id, arguments);
    }
}

void QQmlDMAbstractItemModelData::setValue(const QString &role, const QVariant &value)
{
    // Used only for initialization of the cached data. Does not have to emit change signals.

    if (m_type->propertyRoles.size() == 1
            && (role.isEmpty() || role == QLatin1String("modelData"))) {
        // If the model has only a single role, the modelData is that role.
        m_cachedData[0] = value;
        return;
    }

    QHash<QByteArray, int>::iterator it = m_type->roleNames.find(role.toUtf8());
    if (it != m_type->roleNames.end()) {
        for (int i = 0; i < m_type->propertyRoles.size(); ++i) {
            if (m_type->propertyRoles.at(i) == *it) {
                m_cachedData[i] = value;
                return;
            }
        }
    }
}

bool QQmlDMAbstractItemModelData::resolveIndex(const QQmlAdaptorModel &adaptorModel, int idx)
{
    if (index == -1) {
        Q_ASSERT(idx >= 0);
        m_cachedData.clear();
        setModelIndex(idx, adaptorModel.rowAt(idx), adaptorModel.columnAt(idx));
        const QMetaObject *meta = metaObject();
        const int propertyCount = m_type->propertyRoles.size();
        for (int i = 0; i < propertyCount; ++i)
            QMetaObject::activate(this, meta, i, nullptr);
        emit modelDataChanged();
        return true;
    } else {
        return false;
    }
}

QV4::ReturnedValue QQmlDMAbstractItemModelData::get_property(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *, int)
{
    QV4::Scope scope(b);
    QV4::Scoped<QQmlDelegateModelItemObject> o(scope, thisObject->as<QQmlDelegateModelItemObject>());
    if (!o)
        return scope.engine->throwTypeError(QStringLiteral("Not a valid DelegateModel object"));

    const qsizetype propertyId = static_cast<const QV4::IndexedBuiltinFunction *>(b)->d()->index;

    QQmlDMAbstractItemModelData *modelData = static_cast<QQmlDMAbstractItemModelData *>(o->d()->item);
    if (o->d()->item->index == -1) {
        if (!modelData->m_cachedData.isEmpty())
            return scope.engine->fromVariant(modelData->m_cachedData.at(propertyId));
    } else if (*modelData->m_type->model) {
        return scope.engine->fromVariant(
                modelData->value(modelData->m_type->propertyRoles.at(propertyId)));
    }
    return QV4::Encode::undefined();
}

QV4::ReturnedValue QQmlDMAbstractItemModelData::set_property(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *argv, int argc)
{
    QV4::Scope scope(b);
    QV4::Scoped<QQmlDelegateModelItemObject> o(scope, thisObject->as<QQmlDelegateModelItemObject>());
    if (!o)
        return scope.engine->throwTypeError(QStringLiteral("Not a valid DelegateModel object"));
    if (!argc)
        return scope.engine->throwTypeError();

    const qsizetype propertyId = static_cast<const QV4::IndexedBuiltinFunction *>(b)->d()->index;

    if (o->d()->item->index == -1) {
        QQmlDMAbstractItemModelData *modelData = static_cast<QQmlDMAbstractItemModelData *>(o->d()->item);
        if (!modelData->m_cachedData.isEmpty()) {
            if (modelData->m_cachedData.size() > 1) {
                modelData->m_cachedData[propertyId]
                        = QV4::ExecutionEngine::toVariant(argv[0], QMetaType {});
                QMetaObject::activate(o->d()->item, o->d()->item->metaObject(), propertyId, nullptr);
            } else if (modelData->m_cachedData.size() == 1) {
                modelData->m_cachedData[0] = QV4::ExecutionEngine::toVariant(argv[0], QMetaType {});
                QMetaObject::activate(o->d()->item, o->d()->item->metaObject(), 0, nullptr);
            }
            emit modelData->modelDataChanged();
        }
    }
    return QV4::Encode::undefined();
}

QV4::ReturnedValue QQmlDMAbstractItemModelData::get_modelData(
        const QV4::FunctionObject *b, const QV4::Value *thisObject,
        const QV4::Value *argv, int argc)
{
    Q_UNUSED(argv)
    Q_UNUSED(argc)

    QV4::Scope scope(b);
    QV4::Scoped<QQmlDelegateModelItemObject> o(
                scope, thisObject->as<QQmlDelegateModelItemObject>());
    if (!o)
        return scope.engine->throwTypeError(QStringLiteral("Not a valid DelegateModel object"));

    return scope.engine->fromVariant(
                static_cast<QQmlDMAbstractItemModelData *>(o->d()->item)->modelData());
}

QV4::ReturnedValue QQmlDMAbstractItemModelData::set_modelData(
        const QV4::FunctionObject *b, const QV4::Value *thisObject,
        const QV4::Value *argv, int argc)
{
    QV4::Scope scope(b);
    QV4::Scoped<QQmlDelegateModelItemObject> o(scope, thisObject->as<QQmlDelegateModelItemObject>());
    if (!o)
        return scope.engine->throwTypeError(QStringLiteral("Not a valid DelegateModel object"));
    if (!argc)
        return scope.engine->throwTypeError();

    static_cast<QQmlDMAbstractItemModelData *>(o->d()->item)->setModelData(
                QV4::ExecutionEngine::toVariant(argv[0], QMetaType()));

    return QV4::Encode::undefined();
}

QVariant QQmlDMAbstractItemModelData::modelData() const
{
    if (m_type->propertyRoles.size() == 1) {
        // If the model has only a single role, the modelData is that role.
        return index == -1
                ? m_cachedData.isEmpty() ? QVariant() : m_cachedData[0]
                : value(m_type->propertyRoles[0]);
    }

    // If we're using context properties, the model object is also the context object.
    // In that case we cannot provide modelData. Otherwise return the object itself as modelData.
    return (contextData->contextObject() == this)
            ? QVariant()
            : QVariant::fromValue(this);
}

void QQmlDMAbstractItemModelData::setModelData(const QVariant &modelData)
{
    if (m_type->propertyRoles.size() != 1) {
        qWarning() << "Cannot overwrite model object";
        return;
    }

    // If the model has only a single role, the modelData is that role.
    if (index == -1) {
        if (m_cachedData.isEmpty())
            m_cachedData.append(modelData);
        else
            m_cachedData[0] = modelData;
    } else {
        setValue(m_type->propertyRoles[0], modelData);
    }

    QMetaObject::activate(this, metaObject(), 0, nullptr);
    emit modelDataChanged();
}

bool QQmlDMAbstractItemModelData::hasModelChildren() const
{
    if (index >= 0) {
        if (const QAbstractItemModel *const model = m_type->model->aim())
            return model->hasChildren(model->index(row, column, m_type->model->rootIndex));
    }
    return false;
}

QVariant QQmlDMAbstractItemModelData::value(int role) const
{
    if (const QAbstractItemModel *aim = m_type->model->aim())
        return aim->index(row, column, m_type->model->rootIndex).data(role);
    return QVariant();
}

void QQmlDMAbstractItemModelData::setValue(int role, const QVariant &value)
{
    if (QAbstractItemModel *aim = m_type->model->aim())
        aim->setData(aim->index(row, column, m_type->model->rootIndex), value, role);
}

QV4::ReturnedValue QQmlDMAbstractItemModelData::get()
{
    if (m_type->prototype.isUndefined()) {
        QQmlAdaptorModelEngineData * const data = QQmlAdaptorModelEngineData::get(v4);
        m_type->initializeConstructor(data);
    }
    QV4::Scope scope(v4);
    QV4::ScopedObject proto(scope, m_type->prototype.value());
    QV4::ScopedObject o(scope, proto->engine()->memoryManager->allocate<QQmlDelegateModelItemObject>(this));
    o->setPrototypeOf(proto);
    ++scriptRef;
    return o.asReturnedValue();
}

QT_END_NAMESPACE
