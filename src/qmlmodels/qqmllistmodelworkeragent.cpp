// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmllistmodelworkeragent_p.h"
#include "qqmllistmodel_p_p.h"
#include <private/qqmldata_p.h>
#include <private/qqmlengine_p.h>
#include <qqmlinfo.h>

#include <QtCore/qcoreevent.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>


QT_BEGIN_NAMESPACE

QQmlListModelWorkerAgent::Sync::~Sync()
{
}

QQmlListModelWorkerAgent::QQmlListModelWorkerAgent(QQmlListModel *model)
: m_ref(1), m_orig(model), m_copy(new QQmlListModel(model, this))
{
}

QQmlListModelWorkerAgent::~QQmlListModelWorkerAgent()
{
    mutex.lock();
    syncDone.wakeAll();
    mutex.unlock();
}

QV4::ExecutionEngine *QQmlListModelWorkerAgent::engine() const
{
    return m_copy->m_engine;
}

void QQmlListModelWorkerAgent::setEngine(QV4::ExecutionEngine *eng)
{
    if (eng != m_copy->m_engine) {
        m_copy->m_engine = eng;
        emit engineChanged(eng);
    }
}

void QQmlListModelWorkerAgent::addref()
{
    m_ref.ref();
}

void QQmlListModelWorkerAgent::release()
{
    bool del = !m_ref.deref();

    if (del)
        deleteLater();
}

void QQmlListModelWorkerAgent::modelDestroyed()
{
    m_orig = nullptr;
}

int QQmlListModelWorkerAgent::count() const
{
    return m_copy->count();
}

void QQmlListModelWorkerAgent::clear()
{
    m_copy->clear();
}

void QQmlListModelWorkerAgent::remove(QQmlV4FunctionPtr args)
{
    m_copy->remove(args);
}

void QQmlListModelWorkerAgent::append(QQmlV4FunctionPtr args)
{
    m_copy->append(args);
}

void QQmlListModelWorkerAgent::insert(QQmlV4FunctionPtr args)
{
    m_copy->insert(args);
}

QJSValue QQmlListModelWorkerAgent::get(int index) const
{
    return m_copy->get(index);
}

void QQmlListModelWorkerAgent::set(int index, const QJSValue &value)
{
    m_copy->set(index, value);
}

void QQmlListModelWorkerAgent::setProperty(int index, const QString& property, const QVariant& value)
{
    m_copy->setProperty(index, property, value);
}

void QQmlListModelWorkerAgent::move(int from, int to, int count)
{
    m_copy->move(from, to, count);
}

void QQmlListModelWorkerAgent::sync()
{
    Sync *s = new Sync(m_copy);

    mutex.lock();
    QCoreApplication::postEvent(this, s);
    syncDone.wait(&mutex);
    mutex.unlock();
}

bool QQmlListModelWorkerAgent::event(QEvent *e)
{
    if (e->type() == QEvent::User) {
        bool cc = false;
        QMutexLocker locker(&mutex);
        if (m_orig) {
            Sync *s = static_cast<Sync *>(e);

            cc = (m_orig->count() != s->list->count());

            Q_ASSERT(m_orig->m_dynamicRoles == s->list->m_dynamicRoles);
            if (m_orig->m_dynamicRoles)
                QQmlListModel::sync(s->list, m_orig);
            else
                ListModel::sync(s->list->m_listModel, m_orig->m_listModel);
        }

        syncDone.wakeAll();
        locker.unlock();

        if (cc)
            emit m_orig->countChanged();
        return true;
    }

    return QObject::event(e);
}

QT_END_NAMESPACE

#include "moc_qqmllistmodelworkeragent_p.cpp"
