/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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


void QDeclarativeListModelWorkerAgent::Data::clearChange(int uid)
{
    for (int i=0 ; i < changes.count() ; ++i) {
        if (changes[i].modelUid == uid) {
            changes.removeAt(i);
            --i;
        }
    }
}

void QDeclarativeListModelWorkerAgent::Data::insertChange(int uid, int index, int count)
{
    Change c = { uid, Change::Inserted, index, count, 0, QList<int>() };
    changes << c;
}

void QDeclarativeListModelWorkerAgent::Data::removeChange(int uid, int index, int count)
{
    Change c = { uid, Change::Removed, index, count, 0, QList<int>() };
    changes << c;
}

void QDeclarativeListModelWorkerAgent::Data::moveChange(int uid, int index, int count, int to)
{
    Change c = { uid, Change::Moved, index, count, to, QList<int>() };
    changes << c;
}

void QDeclarativeListModelWorkerAgent::Data::changedChange(int uid, int index, int count, const QList<int> &roles)
{
    Change c = { uid, Change::Changed, index, count, 0, roles };
    changes << c;
}

QDeclarativeListModelWorkerAgent::QDeclarativeListModelWorkerAgent(QDeclarativeListModel *model)
: m_ref(1), m_orig(model), m_copy(new QDeclarativeListModel(model, this))
{
}

QDeclarativeListModelWorkerAgent::~QDeclarativeListModelWorkerAgent()
{
    mutex.lock();
    syncDone.wakeAll();
    mutex.unlock();
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

void QDeclarativeListModelWorkerAgent::remove(QDeclarativeV8Function *args)
{
    m_copy->remove(args);
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

        QHash<int, QDeclarativeListModel *> targetModelDynamicHash;
        QHash<int, ListModel *> targetModelStaticHash;

        Q_ASSERT(m_orig->m_dynamicRoles == s->list->m_dynamicRoles);
        if (m_orig->m_dynamicRoles)
            QDeclarativeListModel::sync(s->list, m_orig, &targetModelDynamicHash);
        else
            ListModel::sync(s->list->m_listModel, m_orig->m_listModel, &targetModelStaticHash);

        for (int ii = 0; ii < changes.count(); ++ii) {
            const Change &change = changes.at(ii);

            QDeclarativeListModel *model = 0;
            if (m_orig->m_dynamicRoles) {
                model = targetModelDynamicHash.value(change.modelUid);
            } else {
                ListModel *lm = targetModelStaticHash.value(change.modelUid);
                if (lm)
                    model = lm->m_modelCache;
            }

            if (model) {
                switch (change.type) {
                case Change::Inserted:
                    emit model->itemsInserted(change.index, change.count);
                    break;
                case Change::Removed:
                    emit model->itemsRemoved(change.index, change.count);
                    break;
                case Change::Moved:
                    emit model->itemsMoved(change.index, change.to, change.count);
                    break;
                case Change::Changed:
                    emit model->itemsChanged(change.index, change.count, change.roles);
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

