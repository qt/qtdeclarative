// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmltableinstancemodel_p.h"
#include "qqmlabstractdelegatecomponent_p.h"

#include <QtCore/QTimer>

#include <QtQml/private/qqmlincubator_p.h>
#include <QtQmlModels/private/qqmlchangeset_p.h>
#include <QtQml/private/qqmlcomponent_p.h>

QT_BEGIN_NAMESPACE

const char* kModelItemTag = "_tableinstancemodel_modelItem";

bool QQmlTableInstanceModel::isDoneIncubating(QQmlDelegateModelItem *modelItem)
{
    QQDMIncubationTask *incubationTask = modelItem->incubationTask();
    if (!incubationTask)
        return true;

    switch (incubationTask->status()) {
    case QQmlIncubator::Ready:
    case QQmlIncubator::Error:
        return true;
    default:
        break;
    }

    return false;
}

void QQmlTableInstanceModel::deleteModelItemLater(QQmlDelegateModelItem *modelItem)
{
    Q_ASSERT(modelItem);

    modelItem->destroyObject();
    modelItem->deleteLater();
}

QQmlTableInstanceModel::QQmlTableInstanceModel(QQmlContext *qmlContext, QObject *parent)
    : QQmlInstanceModel(*(new QObjectPrivate()), parent)
    , m_qmlContext(qmlContext)
    , m_metaType(QQml::makeRefPointer<QQmlDelegateModelItemMetaType>(
            m_qmlContext->engine()->handle(), this))
{
}

void QQmlTableInstanceModel::useImportVersion(QTypeRevision version)
{
    m_adaptorModel.useImportVersion(version);
}

QQmlTableInstanceModel::~QQmlTableInstanceModel()
{
    for (const auto modelItem : m_modelItems) {
        // No item in m_modelItems should be referenced at this point. The view
        // should release all its items before it deletes this model. Only model items
        // that are still being incubated should be left for us to delete.
        // We can't rely on that, though. So we only check the strong ref.
        Q_ASSERT(modelItem->objectStrongRef() == 0);
        Q_ASSERT(modelItem->incubationTask());
        // Check that we are not being deleted while we're
        // in the process of e.g emitting a created signal.
        Q_ASSERT(modelItem->scriptRef() == 0);

        modelItem->destroyObject();
    }

    deleteAllFinishedIncubationTasks();
    qDeleteAll(m_modelItems);
    drainReusableItemsPool(0);
}

QQmlComponent *QQmlTableInstanceModel::resolveDelegate(int index)
{
    if (m_delegateChooser) {
        const int row = m_adaptorModel.rowAt(index);
        const int column = m_adaptorModel.columnAt(index);
        QQmlComponent *delegate = nullptr;
        QQmlAbstractDelegateComponent *chooser = m_delegateChooser;
        do {
            delegate = chooser->delegate(&m_adaptorModel, row, column);
            chooser = qobject_cast<QQmlAbstractDelegateComponent *>(delegate);
        } while (chooser);
        return delegate;
    }

    return m_delegate;
}

QQmlDelegateModelItem *QQmlTableInstanceModel::resolveModelItem(int index)
{
    // Check if an item for the given index is already loaded and ready
    QQmlDelegateModelItem *modelItem = m_modelItems.value(index, nullptr);
    if (modelItem)
        return modelItem;

    QQmlComponent *delegate = resolveDelegate(index);
    if (!delegate)
        return nullptr;

    // Check if the pool contains an item that can be reused
    modelItem = m_reusableItemsPool.takeItem(delegate, index);
    if (modelItem) {
        reuseItem(modelItem, index);
        m_modelItems.insert(index, modelItem);
        return modelItem;
    }

    // Create a new item from scratch
    modelItem = m_adaptorModel.createItem(m_metaType.data(), index);
    if (modelItem) {
        modelItem->setDelegate(delegate);
        m_modelItems.insert(index, modelItem);
        return modelItem;
    }

    qWarning() << Q_FUNC_INFO << "failed creating a model item for index: " << index;
    return nullptr;
}

QObject *QQmlTableInstanceModel::object(int index, QQmlIncubator::IncubationMode incubationMode)
{
    Q_ASSERT(m_delegate);
    Q_ASSERT(index >= 0 && index < m_adaptorModel.count());

    QQmlDelegateModelItem *modelItem = resolveModelItem(index);
    if (!modelItem)
        return nullptr;

    // The model item has already been incubated. So
    // just bump the ref-count and return it.
    if (modelItem->object())
        return modelItem->referenceObjectWeak();

    // The object is not ready, and needs to be incubated
    incubateModelItem(modelItem, incubationMode);
    if (!isDoneIncubating(modelItem))
        return nullptr;

    // Incubation is done, so the task should be removed
    Q_ASSERT(!modelItem->incubationTask());

    // Incubation was completed sync and successful
    if (modelItem->object())
        return modelItem->referenceObjectWeak();

    // The object was incubated synchronously (otherwise we would return above). But since
    // we have no object, the incubation must have failed. And when we have no object, there
    // should be no object references either. And there should also not be any internal script
    // refs at this point. So we delete the model item.
    Q_ASSERT(!modelItem->isObjectReferenced());
    Q_ASSERT(!modelItem->isScriptReferenced());
    m_modelItems.remove(modelItem->modelIndex());
    delete modelItem;
    return nullptr;
}

QQmlInstanceModel::ReleaseFlags QQmlTableInstanceModel::release(QObject *object, ReusableFlag reusable)
{
    Q_ASSERT(object);
    auto modelItem = qvariant_cast<QQmlDelegateModelItem *>(object->property(kModelItemTag));
    Q_ASSERT(modelItem);
    // Ensure that the object was incubated by this QQmlTableInstanceModel
    Q_ASSERT(m_modelItems.contains(modelItem->modelIndex()));
    Q_ASSERT(m_modelItems[modelItem->modelIndex()]->object() == object);

    if (!modelItem->releaseObjectWeak())
        return QQmlDelegateModel::Referenced;

    if (modelItem->isScriptReferenced()) {
        // We still have an internal reference to this object, which means that we are told to release an
        // object while the createdItem signal for it is still on the stack. This can happen when objects
        // are e.g delivered async, and the user flicks back and forth quicker than the loading can catch
        // up with. The view might then find that the object is no longer visible and should be released.
        // We detect this case in incubatorStatusChanged(), and delete it there instead. But from the callers
        // point of view, it should consider it destroyed.
        return QQmlDelegateModel::Destroyed;
    }

    // The item is not referenced by anyone
    m_modelItems.remove(modelItem->modelIndex());

    if (reusable == Reusable && m_reusableItemsPool.insertItem(modelItem)) {
        emit itemPooled(modelItem->modelIndex(), modelItem->object());
        return QQmlInstanceModel::Pooled;
    }

    // The item is not reused or referenced by anyone, so just delete it
    destroyModelItem(modelItem, Deferred);
    return QQmlInstanceModel::Destroyed;
}

void QQmlTableInstanceModel::destroyModelItem(QQmlDelegateModelItem *modelItem, DestructionMode mode)
{
    emit destroyingItem(modelItem->object());
    if (mode == Deferred)
        modelItem->destroyObjectLater();
    else
        modelItem->destroyObject();
    delete modelItem;
}

void QQmlTableInstanceModel::dispose(QObject *object)
{
    Q_ASSERT(object);
    auto modelItem = qvariant_cast<QQmlDelegateModelItem *>(object->property(kModelItemTag));
    Q_ASSERT(modelItem);

    modelItem->releaseObjectWeak();

    // The item is not referenced by anyone
    Q_ASSERT(!modelItem->isObjectReferenced());
    Q_ASSERT(!modelItem->isScriptReferenced());
    // Ensure that the object was incubated by this QQmlTableInstanceModel
    Q_ASSERT(m_modelItems.contains(modelItem->modelIndex()));
    Q_ASSERT(m_modelItems[modelItem->modelIndex()]->object() == object);

    m_modelItems.remove(modelItem->modelIndex());

    emit destroyingItem(object);
    modelItem->destroyObject();

    delete modelItem;
}

void QQmlTableInstanceModel::cancel(int index)
{
    auto modelItem = m_modelItems.value(index);
    Q_ASSERT(modelItem);

    // Since the view expects the item to be incubating, there should be
    // an incubation task. And since the incubation is not done, no-one
    // should yet have received, and therfore hold a reference to, the object.
    Q_ASSERT(modelItem->incubationTask());
    Q_ASSERT(!modelItem->isObjectReferenced());

    m_modelItems.remove(index);

    modelItem->destroyObject();

    // modelItem->incubationTask will be deleted from the modelItems destructor
    delete modelItem;
}

void QQmlTableInstanceModel::drainReusableItemsPool(int maxPoolTime)
{
    m_reusableItemsPool.drain(maxPoolTime, [this](QQmlDelegateModelItem *modelItem) {
        destroyModelItem(modelItem, Immediate);
    });
}

void QQmlTableInstanceModel::reuseItem(QQmlDelegateModelItem *item, int newModelIndex)
{
    // Update the context properties index, row and column on
    // the delegate item, and inform the application about it.
    // Note that we set alwaysEmit to true, to force all bindings
    // to be reevaluated, even if the index didn't change (since
    // the model can have changed size since last usage).
    const bool alwaysEmit = true;
    const int newRow = m_adaptorModel.rowAt(newModelIndex);
    const int newColumn = m_adaptorModel.columnAt(newModelIndex);
    item->setModelIndex(newModelIndex, newRow, newColumn, alwaysEmit);

    // Notify the application that all 'dynamic'/role-based context data has
    // changed as well (their getter function will use the updated index).
    auto const itemAsList = QList<QQmlDelegateModelItem *>() << item;
    auto const updateAllRoles = QList<int>();
    m_adaptorModel.notify(itemAsList, newModelIndex, 1, updateAllRoles);

    // Inform the view that the item is recycled. This will typically result
    // in the view updating its own attached delegate item properties.
    emit itemReused(newModelIndex, item->object());
}

void QQmlTableInstanceModel::incubateModelItem(QQmlDelegateModelItem *modelItem, QQmlIncubator::IncubationMode incubationMode)
{
    // Guard the model item temporarily so that it's not deleted from
    // incubatorStatusChanged(), in case the incubation is done synchronously.
    QQmlDelegateModelItem::ScriptReference scriptRef(modelItem);

    if (QQDMIncubationTask *incubationTask = modelItem->incubationTask()) {
        // We're already incubating the model item from a previous request. If the previous call requested
        // the item async, but the current request needs it sync, we need to force-complete the incubation.
        const bool sync = (incubationMode == QQmlIncubator::Synchronous || incubationMode == QQmlIncubator::AsynchronousIfNested);
        if (sync && incubationTask->incubationMode() == QQmlIncubator::Asynchronous)
            incubationTask->forceCompletion();
    } else if (m_qmlContext && m_qmlContext->isValid()) {
        modelItem->setIncubationTask(
                new QQmlTableInstanceModelIncubationTask(this, modelItem, incubationMode));
        // TODO: In order to retain compatibility, we cannot allow the incubation task to clear the
        //       context object in the presence of required properties. This results in the context
        //       properties still being available in the delegate even though they shouldn't.
        // modelItem->incubationTask->incubating = modelItem;

        QQmlComponent *delegate = modelItem->delegate();
        QQmlContext *creationContext = delegate->creationContext();
        const QQmlRefPointer<QQmlContextData> componentContext
                = QQmlContextData::get(creationContext  ? creationContext : m_qmlContext.data());

        QQmlComponentPrivate *cp = QQmlComponentPrivate::get(delegate);
        if (cp->isBound()) {
            modelItem->setContextData(componentContext);

            // Ignore return value of initProxy. We want to know the proxy when assigning required
            // properties, but we don't want it to pollute our context. The context is bound.
            if (m_adaptorModel.hasProxyObject())
                modelItem->initProxy();

            cp->incubateObject(
                    modelItem->incubationTask(), delegate, m_qmlContext->engine(), componentContext,
                    QQmlContextData::get(m_qmlContext));
        } else {
            QQmlRefPointer<QQmlContextData> ctxt = QQmlContextData::createRefCounted(
                        QQmlContextData::get(creationContext  ? creationContext : m_qmlContext.data()));
            ctxt->setContextObject(modelItem);
            modelItem->setContextData(ctxt);

            // If the model is read-only we cannot just expose the object as context
            // We actually need a separate model object to moderate access.
            if (m_adaptorModel.hasProxyObject()) {
                if (m_adaptorModel.delegateModelAccess == QQmlDelegateModel::ReadOnly)
                    modelItem->initProxy();
                else
                    ctxt = modelItem->initProxy();
            }

            cp->incubateObject(
                    modelItem->incubationTask(), modelItem->delegate(), m_qmlContext->engine(),
                    ctxt, QQmlContextData::get(m_qmlContext));
        }
    }
}

void QQmlTableInstanceModel::incubatorStatusChanged(QQmlTableInstanceModelIncubationTask *incubationTask, QQmlIncubator::Status status)
{
    QQmlDelegateModelItem *modelItem = incubationTask->modelItemToIncubate;
    Q_ASSERT(modelItem->incubationTask());

    modelItem->clearIncubationTask();
    incubationTask->modelItemToIncubate = nullptr;

    if (status == QQmlIncubator::Ready) {
        QObject *object = modelItem->object();
        Q_ASSERT(object);

        // Tag the incubated object with the model item for easy retrieval upon release etc.
        object->setProperty(kModelItemTag, QVariant::fromValue(modelItem));

        // Emit that the item has been created. What normally happens next is that the view
        // upon receiving the signal asks for the model item once more. And since the item is
        // now in the map, it will be returned directly.
        QQmlDelegateModelItem::ScriptReference scriptRef(modelItem);
        emit createdItem(modelItem->modelIndex(), object);
    } else if (status == QQmlIncubator::Error) {
        qWarning() << "Error incubating delegate:" << incubationTask->errors();
    }

    if (!modelItem->isScriptReferenced() && !modelItem->isObjectReferenced()) {
        // We have no internal reference to the model item, and the view has no
        // reference to the incubated object. So just delete the model item.
        // Note that being here means that the object was incubated _async_
        // (otherwise modelItem->isReferenced() would be true).
        m_modelItems.remove(modelItem->modelIndex());

        if (QObject *object = modelItem->object()) {
            QQmlDelegateModelItem::ScriptReference scriptRef(modelItem);
            emit destroyingItem(object);
        }

        Q_ASSERT(!modelItem->isScriptReferenced());
        deleteModelItemLater(modelItem);
    }

    deleteIncubationTaskLater(incubationTask);
}

QQmlIncubator::Status QQmlTableInstanceModel::incubationStatus(int index) {
    const auto modelItem = m_modelItems.value(index, nullptr);
    if (!modelItem)
        return QQmlIncubator::Null;

    if (QQDMIncubationTask *incubationTask = modelItem->incubationTask())
        return incubationTask->status();

    // Since we clear the incubation task when we're done
    // incubating, it means that the status is Ready.
    return QQmlIncubator::Ready;
}

bool QQmlTableInstanceModel::setRequiredProperty(int index, const QString &name, const QVariant &value)
{
    // This function can be called from the view upon
    // receiving the initItem signal. It can be used to
    // give all required delegate properties used by the
    // view an initial value.
    const auto modelItem = m_modelItems.value(index, nullptr);
    if (!modelItem)
        return false;
    if (!modelItem->object())
        return false;
    if (!modelItem->incubationTask())
        return false;

    bool wasInRequired = false;
    const auto task = QQmlIncubatorPrivate::get(modelItem->incubationTask());
    RequiredProperties *props = task->requiredProperties();
    if (props->empty())
        return false;

    QQmlProperty componentProp = QQmlComponentPrivate::removePropertyFromRequired(
            modelItem->object(), name, props, QQmlEnginePrivate::get(task->enginePriv),
            &wasInRequired);
    if (wasInRequired)
        componentProp.write(value);
    return wasInRequired;
}

QQmlDelegateModelItem *QQmlTableInstanceModel::getModelItem(int index)
{
    return m_modelItems.value(index, nullptr);
}

void QQmlTableInstanceModel::deleteIncubationTaskLater(QQmlIncubator *incubationTask)
{
    // We often need to post-delete incubation tasks, since we cannot
    // delete them while we're in the middle of an incubation change callback.
    Q_ASSERT(!m_finishedIncubationTasks.contains(incubationTask));
    m_finishedIncubationTasks.append(incubationTask);
    if (m_finishedIncubationTasks.size() == 1)
        QTimer::singleShot(1, this, &QQmlTableInstanceModel::deleteAllFinishedIncubationTasks);
}

void QQmlTableInstanceModel::deleteAllFinishedIncubationTasks()
{
    qDeleteAll(m_finishedIncubationTasks);
    m_finishedIncubationTasks.clear();
}

QVariant QQmlTableInstanceModel::model() const
{
    return m_adaptorModel.model();
}

void QQmlTableInstanceModel::forceSetModel(const QVariant &model)
{
    // Pooled items are still accessible/alive for the application, and
    // needs to stay in sync with the model. So we need to drain the pool
    // completely when the model changes.
    drainReusableItemsPool(0);
    if (auto const aim = abstractItemModel()) {
        disconnect(aim, &QAbstractItemModel::dataChanged, this, &QQmlTableInstanceModel::dataChangedCallback);
        disconnect(aim, &QAbstractItemModel::modelAboutToBeReset, this, &QQmlTableInstanceModel::modelAboutToBeResetCallback);
    }
    m_adaptorModel.setModel(model);
    if (auto const aim = abstractItemModel()) {
        connect(aim, &QAbstractItemModel::dataChanged, this, &QQmlTableInstanceModel::dataChangedCallback);
        connect(aim, &QAbstractItemModel::modelAboutToBeReset, this, &QQmlTableInstanceModel::modelAboutToBeResetCallback);
    }
}

void QQmlTableInstanceModel::setModel(const QVariant &model)
{
    if (m_adaptorModel.model() == model)
        return;

    forceSetModel(model);

    emit modelChanged();
}

void QQmlTableInstanceModel::dataChangedCallback(const QModelIndex &begin, const QModelIndex &end, const QList<int> &roles)
{
    // This function is called when model data has changed. In that case, we tell the adaptor model
    // to go through all the items we have created, find the ones that are affected, and notify that
    // their model data has changed. This will in turn update QML bindings inside the delegate items.
    int numberOfRowsChanged = end.row() - begin.row() + 1;
    int numberOfColumnsChanged = end.column() - begin.column() + 1;

    for (int column = 0; column < numberOfColumnsChanged; ++column) {
        const int columnIndex = begin.column() + column;
        const int rowIndex = begin.row() + (columnIndex * rows());
        m_adaptorModel.notify(m_modelItems.values(), rowIndex, numberOfRowsChanged, roles);
    }
}

void QQmlTableInstanceModel::modelAboutToBeResetCallback()
{
    // When the model is reset, we can no longer rely on any of the data it has
    // provided us so far. Normally it's enough for the view to recreate all the
    // delegate items in that case, except if the model roles has changed as well
    // (since those are cached by QQmlAdaptorModel / Accessors). For the latter case, we
    // simply set the model once more in the delegate model to rebuild everything.
    auto const aim = abstractItemModel();
    auto oldRoleNames = aim->roleNames();
    QObject::connect(aim, &QAbstractItemModel::modelReset, this, [this, aim, oldRoleNames](){
        if (oldRoleNames != aim->roleNames()) {
            // We refresh the model, but without sending any signals. The actual model object
            // stays the same after all.
            forceSetModel(model());
        }
    }, Qt::SingleShotConnection);
}

QQmlComponent *QQmlTableInstanceModel::delegate() const
{
    return m_delegate;
}

void QQmlTableInstanceModel::setDelegate(QQmlComponent *delegate)
{
    if (m_delegate == delegate)
        return;

    m_delegateChooser = nullptr;
    if (delegate) {
        QQmlAbstractDelegateComponent *adc =
                qobject_cast<QQmlAbstractDelegateComponent *>(delegate);
        if (adc)
            m_delegateChooser = adc;
    }

    m_delegate = delegate;
}

const QAbstractItemModel *QQmlTableInstanceModel::abstractItemModel() const
{
    return m_adaptorModel.adaptsAim() ? m_adaptorModel.aim() : nullptr;
}

// --------------------------------------------------------

void QQmlTableInstanceModelIncubationTask::setInitialState(QObject *object)
{
    initializeRequiredProperties(
            modelItemToIncubate, object, tableInstanceModel->delegateModelAccess());
    modelItemToIncubate->setObject(object);
    emit tableInstanceModel->initItem(modelItemToIncubate->modelIndex(), object);

    if (!QQmlIncubatorPrivate::get(this)->requiredProperties()->empty())
        modelItemToIncubate->destroyObjectLater();
}

void QQmlTableInstanceModelIncubationTask::statusChanged(QQmlIncubator::Status status)
{
    if (!QQmlTableInstanceModel::isDoneIncubating(modelItemToIncubate))
        return;

    // We require the view to cancel any ongoing load
    // requests before the tableInstanceModel is destructed.
    Q_ASSERT(tableInstanceModel);

    tableInstanceModel->incubatorStatusChanged(this, status);
}

QT_END_NAMESPACE

#include "moc_qqmltableinstancemodel_p.cpp"

