/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativelistmodelworkeragent_p.h"
#include "qdeclarativelistmodel_p_p.h"
#include <private/qdeclarativedata_p.h>
#include <private/qdeclarativeengine_p.h>
#include <qdeclarativeinfo.h>

#include <QtCore/qcoreevent.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>


QT_BEGIN_NAMESPACE


void QDeclarativeListModelWorkerAgent::Data::clearChange(QDeclarativeListModel *model)
{
    int uid = model->m_listModel->getUid();

    for (int i=0 ; i < changes.count() ; ++i) {
        if (changes[i].modelUid == uid) {
            changes.removeAt(i);
            --i;
        }
    }
}

void QDeclarativeListModelWorkerAgent::Data::insertChange(QDeclarativeListModel *model, int index, int count)
{
    Change c = { model->m_listModel->getUid(), Change::Inserted, index, count, 0, QList<int>() };
    changes << c;
}

void QDeclarativeListModelWorkerAgent::Data::removeChange(QDeclarativeListModel *model, int index, int count)
{
    Change c = { model->m_listModel->getUid(), Change::Removed, index, count, 0, QList<int>() };
    changes << c;
}

void QDeclarativeListModelWorkerAgent::Data::moveChange(QDeclarativeListModel *model, int index, int count, int to)
{
    Change c = { model->m_listModel->getUid(), Change::Moved, index, count, to, QList<int>() };
    changes << c;
}

void QDeclarativeListModelWorkerAgent::Data::changedChange(QDeclarativeListModel *model, int index, int count, const QList<int> &roles)
{
    Change c = { model->m_listModel->getUid(), Change::Changed, index, count, 0, roles };
    changes << c;
}

QDeclarativeListModelWorkerAgent::QDeclarativeListModelWorkerAgent(QDeclarativeListModel *model)
: m_ref(1), m_orig(model), m_copy(new QDeclarativeListModel(model, this))
{
}

void QDeclarativeListModelWorkerAgent::setV8Engine(QV8Engine *eng)
{
    m_copy->m_engine = eng;
}

void QDeclarativeListModelWorkerAgent::addref()
{
    m_ref.ref();
}

void QDeclarativeListModelWorkerAgent::release()
{
    bool del = !m_ref.deref();

    if (del)
        delete this;
}

int QDeclarativeListModelWorkerAgent::count() const
{
    return m_copy->count();
}

void QDeclarativeListModelWorkerAgent::clear()
{
    m_copy->clear();
}

void QDeclarativeListModelWorkerAgent::remove(int index)
{
    m_copy->remove(index);
}

void QDeclarativeListModelWorkerAgent::append(QDeclarativeV8Function *args)
{
    m_copy->append(args);
}

void QDeclarativeListModelWorkerAgent::insert(QDeclarativeV8Function *args)
{
    m_copy->insert(args);
}

QDeclarativeV8Handle QDeclarativeListModelWorkerAgent::get(int index) const
{
    return m_copy->get(index);
}

void QDeclarativeListModelWorkerAgent::set(int index, const QDeclarativeV8Handle &value)
{
    m_copy->set(index, value);
}

void QDeclarativeListModelWorkerAgent::setProperty(int index, const QString& property, const QVariant& value)
{
    m_copy->setProperty(index, property, value);
}

void QDeclarativeListModelWorkerAgent::move(int from, int to, int count)
{
    m_copy->move(from, to, count);
}

void QDeclarativeListModelWorkerAgent::sync()
{
    Sync *s = new Sync;
    s->data = data;
    s->list = m_copy;
    data.changes.clear();

    mutex.lock();
    QCoreApplication::postEvent(this, s);
    syncDone.wait(&mutex);
    mutex.unlock();
}

bool QDeclarativeListModelWorkerAgent::event(QEvent *e)
{
    if (e->type() == QEvent::User) {

        QMutexLocker locker(&mutex);
        Sync *s = static_cast<Sync *>(e);

        const QList<Change> &changes = s->data.changes;

        bool cc = m_orig->count() != s->list->count();

        QHash<int, ListModel *> targetModelHash;
        ListModel::sync(s->list->m_listModel, m_orig->m_listModel, &targetModelHash);

        for (int ii = 0; ii < changes.count(); ++ii) {
            const Change &change = changes.at(ii);

            ListModel *model = targetModelHash.value(change.modelUid);

            if (model && model->m_modelCache) {
                switch (change.type) {
                case Change::Inserted:
                    emit model->m_modelCache->itemsInserted(change.index, change.count);
                    break;
                case Change::Removed:
                    emit model->m_modelCache->itemsRemoved(change.index, change.count);
                    break;
                case Change::Moved:
                    emit model->m_modelCache->itemsMoved(change.index, change.to, change.count);
                    break;
                case Change::Changed:
                    emit model->m_modelCache->itemsChanged(change.index, change.count, change.roles);
                    break;
                }
            }
        }

        syncDone.wakeAll();
        locker.unlock();

        if (cc)
            emit m_orig->countChanged();
    }

    return QObject::event(e);
}

QT_END_NAMESPACE

