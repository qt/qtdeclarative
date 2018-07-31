/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmltableinstancemodel_p.h"
#include "qqmltableinstancemodel_p.h"

#include <QtCore/QTimer>

#include <QtQml/private/qqmlincubator_p.h>
#include <QtQml/private/qqmlchangeset_p.h>
#include <QtQml/private/qqmlcomponent_p.h>

QT_BEGIN_NAMESPACE

const char* kModelItemTag = "_tableinstancemodel_modelItem";

bool QQmlTableInstanceModel::isDoneIncubating(QQmlDelegateModelItem *modelItem)
{
    if (!modelItem->incubationTask)
        return true;

    const auto status = modelItem->incubationTask->status();
    return (status == QQmlIncubator::Ready) || (status == QQmlIncubator::Error);
}

void QQmlTableInstanceModel::deleteModelItemLater(QQmlDelegateModelItem *modelItem)
{
    Q_ASSERT(modelItem);

    delete modelItem->object;
    modelItem->object = nullptr;

    if (modelItem->contextData) {
        modelItem->contextData->invalidate();
        Q_ASSERT(modelItem->contextData->refCount == 1);
        modelItem->contextData = nullptr;
    }

    modelItem->deleteLater();
}

QQmlTableInstanceModel::QQmlTableInstanceModel(QQmlContext *qmlContext, QObject *parent)
    : QQmlInstanceModel(*(new QObjectPrivate()), parent)
    , m_qmlContext(qmlContext)
    , m_metaType(new QQmlDelegateModelItemMetaType(m_qmlContext->engine()->handle(), nullptr, QStringList()))
{
}

QQmlTableInstanceModel::~QQmlTableInstanceModel()
{
    deleteAllFinishedIncubationTasks();

    // If we have model items loaded at this point, it means that
    // the view is still holding references to them. So basically
    // the view needs to be deleted first (and thereby release all
    // its items), before this destructor is run.
    Q_ASSERT(m_modelItems.isEmpty());
}

QObject *QQmlTableInstanceModel::object(int index, QQmlIncubator::IncubationMode incubationMode)
{
    Q_ASSERT(m_delegate);
    Q_ASSERT(index >= 0 && index < m_adaptorModel.count());
    Q_ASSERT(m_qmlContext && m_qmlContext->isValid());

    // Check if we've already created an item for the given index
    QQmlDelegateModelItem *modelItem = m_modelItems.value(index, nullptr);

    if (!modelItem) {
        // Create a new model item
        modelItem = m_adaptorModel.createItem(m_metaType, index);
        if (!modelItem) {
            qWarning() << Q_FUNC_INFO << "Adaptor model failed creating a model item";
            return nullptr;
        }
        m_modelItems.insert(index, modelItem);
    }

    if (modelItem->object) {
        // The model item has already been incubated. So
        // just bump the ref-count and return it.
        modelItem->referenceObject();
        return modelItem->object;
    }

    // Guard the model item temporarily so that it's not deleted from
    // incubatorStatusChanged(), in case the incubation is done synchronously.
    modelItem->scriptRef++;

    if (modelItem->incubationTask) {
        // We're already incubating the model item from a previous request. If the previous call requested
        // the item async, but the current request needs it sync, we need to force-complete the incubation.
        const bool sync = (incubationMode == QQmlIncubator::Synchronous || incubationMode == QQmlIncubator::AsynchronousIfNested);
        if (sync && modelItem->incubationTask->incubationMode() == QQmlIncubator::Asynchronous)
            modelItem->incubationTask->forceCompletion();
    } else {
        modelItem->incubationTask = new QQmlTableInstanceModelIncubationTask(this, modelItem, incubationMode);

        QQmlContextData *ctxt = new QQmlContextData;
        QQmlContext *creationContext = m_delegate->creationContext();
        ctxt->setParent(QQmlContextData::get(creationContext  ? creationContext : m_qmlContext.data()));
        ctxt->contextObject = modelItem;
        modelItem->contextData = ctxt;

        QQmlComponentPrivate::get(m_delegate)->incubateObject(
                    modelItem->incubationTask,
                    m_delegate,
                    m_qmlContext->engine(),
                    ctxt,
                    QQmlContextData::get(m_qmlContext));
    }

    // Remove the temporary guard
    modelItem->scriptRef--;
    Q_ASSERT(modelItem->scriptRef == 0);

    if (!isDoneIncubating(modelItem))
        return nullptr;

    // When incubation is done, the task should be removed
    Q_ASSERT(!modelItem->incubationTask);

    if (!modelItem->object) {
        // The object was incubated synchronously (otherwise we would return above). But since
        // we have no object, the incubation must have failed. And when we have no object, there
        // should be no object references either. And there should also not be any internal script
        // refs at this point. So we delete the model item.
        Q_ASSERT(!modelItem->isObjectReferenced());
        Q_ASSERT(!modelItem->isReferenced());
        m_modelItems.remove(modelItem->index);
        delete modelItem;
        return nullptr;
    }

    // Incubation was completed sync and successful
    modelItem->referenceObject();
    return modelItem->object;
}

QQmlInstanceModel::ReleaseFlags QQmlTableInstanceModel::release(QObject *object)
{
    Q_ASSERT(object);
    auto modelItem = qvariant_cast<QQmlDelegateModelItem *>(object->property(kModelItemTag));
    Q_ASSERT(modelItem);

    if (!modelItem->releaseObject())
        return QQmlDelegateModel::Referenced;

    if (modelItem->isReferenced()) {
        // We still have an internal reference to this object, which means that we are told to release an
        // object while the createdItem signal for it is still on the stack. This can happen when objects
        // are e.g delivered async, and the user flicks back and forth quicker than the loading can catch
        // up with. The view might then find that the object is no longer visible and should be released.
        // We detect this case in incubatorStatusChanged(), and delete it there instead. But from the callers
        // points of view, it should consider it destroyed.
        return QQmlDelegateModel::Destroyed;
    }

    m_modelItems.remove(modelItem->index);
    modelItem->destroyObject();
    emit destroyingItem(object);

    delete modelItem;
    return QQmlInstanceModel::Destroyed;
}

void QQmlTableInstanceModel::cancel(int index)
{
    auto modelItem = m_modelItems.value(index);
    Q_ASSERT(modelItem);

    // Since the view expects the item to be incubating, there should be
    // an incubation task. And since the incubation is not done, no-one
    // should yet have received, and therfore hold a reference to, the object.
    Q_ASSERT(modelItem->incubationTask);
    Q_ASSERT(!modelItem->isObjectReferenced());

    m_modelItems.remove(index);

    if (modelItem->object)
        delete modelItem->object;

    // modelItem->incubationTask will be deleted from the modelItems destructor
    delete modelItem;
}

void QQmlTableInstanceModel::incubatorStatusChanged(QQmlTableInstanceModelIncubationTask *incubationTask, QQmlIncubator::Status status)
{
    QQmlDelegateModelItem *modelItem = incubationTask->modelItemToIncubate;
    Q_ASSERT(modelItem->incubationTask);

    modelItem->incubationTask = nullptr;
    incubationTask->modelItemToIncubate = nullptr;

    if (status == QQmlIncubator::Ready) {
        // Tag the incubated object with the model item for easy retrieval upon release etc.
        modelItem->object->setProperty(kModelItemTag, QVariant::fromValue(modelItem));

        // Emit that the item has been created. What normally happens next is that the view
        // upon receiving the signal asks for the model item once more. And since the item is
        // now in the map, it will be returned directly.
        Q_ASSERT(modelItem->object);
        modelItem->scriptRef++;
        emit createdItem(modelItem->index, modelItem->object);
        modelItem->scriptRef--;
    } else if (status == QQmlIncubator::Error) {
        qWarning() << "Error incubating delegate:" << incubationTask->errors();
    }

    if (!modelItem->isReferenced() && !modelItem->isObjectReferenced()) {
        // We have no internal reference to the model item, and the view has no
        // reference to the incubated object. So just delete the model item.
        // Note that being here means that the object was incubated _async_
        // (otherwise modelItem->isReferenced() would be true).
        m_modelItems.remove(modelItem->index);

        if (modelItem->object) {
            modelItem->scriptRef++;
            emit destroyingItem(modelItem->object);
            modelItem->scriptRef--;
            Q_ASSERT(!modelItem->isReferenced());
        }

        deleteModelItemLater(modelItem);
    }

    deleteIncubationTaskLater(incubationTask);
}

QQmlIncubator::Status QQmlTableInstanceModel::incubationStatus(int index) {
    const auto modelItem = m_modelItems.value(index, nullptr);
    if (!modelItem)
        return QQmlIncubator::Null;

    if (modelItem->incubationTask)
        return modelItem->incubationTask->status();

    // Since we clear the incubation task when we're done
    // incubating, it means that the status is Ready.
    return QQmlIncubator::Ready;
}

void QQmlTableInstanceModel::deleteIncubationTaskLater(QQmlIncubator *incubationTask)
{
    // We often need to post-delete incubation tasks, since we cannot
    // delete them while we're in the middle of an incubation change callback.
    Q_ASSERT(!m_finishedIncubationTasks.contains(incubationTask));
    m_finishedIncubationTasks.append(incubationTask);
    if (m_finishedIncubationTasks.count() == 1)
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

void QQmlTableInstanceModel::setModel(const QVariant &model)
{
    m_adaptorModel.setModel(model, this, m_qmlContext->engine());
}

QQmlComponent *QQmlTableInstanceModel::delegate() const
{
    return m_delegate;
}

void QQmlTableInstanceModel::setDelegate(QQmlComponent *delegate)
{
    m_delegate = delegate;
}

const QAbstractItemModel *QQmlTableInstanceModel::abstractItemModel() const
{
    return m_adaptorModel.adaptsAim() ? m_adaptorModel.aim() : nullptr;
}

// --------------------------------------------------------

void QQmlTableInstanceModelIncubationTask::setInitialState(QObject *object)
{
    modelItemToIncubate->object = object;
    emit tableInstanceModel->initItem(modelItemToIncubate->index, object);
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

