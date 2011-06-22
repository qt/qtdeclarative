/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgmodelparticle_p.h"
#include <QtDeclarative/private/qsgvisualitemmodel_p.h>
#include <qsgnode.h>
#include <QTimer>
#include <QDebug>

QT_BEGIN_NAMESPACE

QSGModelParticle::QSGModelParticle(QSGItem *parent) :
    QSGParticlePainter(parent), m_ownModel(false), m_comp(0), m_model(0), m_fade(true), m_modelCount(0)
{
    setFlag(QSGItem::ItemHasContents);
    QTimer* manageDelegates = new QTimer(this);//TODO: don't leak
    connect(manageDelegates, SIGNAL(timeout()),
            this, SLOT(processPending()));
    manageDelegates->setInterval(16);
    manageDelegates->setSingleShot(false);
    manageDelegates->start();
}

QVariant QSGModelParticle::model() const
{
    return m_dataSource;
}

void QSGModelParticle::setModel(const QVariant &arg)
{
    if(arg == m_dataSource)
        return;
    m_dataSource = arg;
    if(qobject_cast<QSGVisualDataModel*>(arg.value<QObject*>())) {
        if(m_ownModel && m_model)
            delete m_model;
        m_model = qobject_cast<QSGVisualDataModel*>(arg.value<QObject*>());
        m_ownModel = false;
    }else{
        if(!m_model || !m_ownModel)
            m_model = new QSGVisualDataModel(qmlContext(this));
        m_model->setModel(m_dataSource);
        m_ownModel = true;
    }
    if(m_comp)
        m_model->setDelegate(m_comp);
    emit modelChanged();
    emit modelCountChanged();
    connect(m_model, SIGNAL(countChanged()),
            this, SIGNAL(modelCountChanged()));
    connect(m_model, SIGNAL(countChanged()),
            this, SLOT(updateCount()));
    updateCount();
}

void QSGModelParticle::updateCount()
{
    int newCount = 0;
    if(m_model)
        newCount = m_model->count();
    if(newCount < 0)
        return;//WTF?
    if(m_modelCount == 0 || newCount == 0){
        m_available.clear();
        for(int i=0; i<newCount; i++)
            m_available << i;
    }else if(newCount < m_modelCount){
        for(int i=newCount; i<m_modelCount; i++) //existing ones must leave normally, but aren't readded
            m_available.removeAll(i);
    }else if(newCount > m_modelCount){
        for(int i=m_modelCount; i<newCount; i++)
            m_available << i;
    }
    m_modelCount = newCount;
}

QDeclarativeComponent *QSGModelParticle::delegate() const
{
    if(m_model)
        return m_model->delegate();
    return 0;
}

void QSGModelParticle::setDelegate(QDeclarativeComponent *comp)
{
    if (QSGVisualDataModel *dataModel = qobject_cast<QSGVisualDataModel*>(m_model))
        if (comp == dataModel->delegate())
            return;
    m_comp = comp;
    if(m_model)
        m_model->setDelegate(comp);
    emit delegateChanged();
}

int QSGModelParticle::modelCount() const
{
    if(m_model)
        const_cast<QSGModelParticle*>(this)->updateCount();//TODO: Investigate why this doesn't get called properly
    return m_modelCount;
}


void QSGModelParticle::freeze(QSGItem* item)
{
    m_stasis << item;
}


void QSGModelParticle::unfreeze(QSGItem* item)
{
    m_stasis.remove(item);
}

void QSGModelParticle::initialize(int idx)
{
    if(!m_model || !m_model->count())
        return;
    if(m_available.isEmpty())
        return;
    m_requests << idx;
    m_activeCount++;
}

void QSGModelParticle::processPending()
{//can't create/delete arbitrary items in the render thread
    foreach(QSGItem* item, m_deletables){
        item->setOpacity(0.);
        m_model->release(item);
    }
    m_deletables.clear();

    foreach(int pos, m_requests){
        if(m_data[pos]->delegate){
            if(m_stasis.contains(m_data[pos]->delegate))
                qWarning() << "Current model particles prefers overwrite:false";
            //remove old item from the particle that is dying to make room for this one
            m_deletables << m_data[pos]->delegate;
            m_available << m_data[pos]->modelIndex;
            m_data[pos]->modelIndex = -1;
            m_data[pos]->delegate = 0;
            m_data[pos] = 0;
            m_activeCount--;
        }

        if(!m_available.isEmpty()){
            m_data[pos]->delegate = m_model->item(m_available.first());
            m_data[pos]->modelIndex = m_available.first();
            m_available.pop_front();
            QSGModelParticleAttached* mpa = qobject_cast<QSGModelParticleAttached*>(qmlAttachedPropertiesObject<QSGModelParticle>(m_data[pos]->delegate));
            if(mpa){
                mpa->m_mp = this;
                mpa->attach();
            }
            m_data[pos]->delegate->setParentItem(this);
        }
    }
    m_requests.clear();
}

void QSGModelParticle::reload(int idx)
{
    //No-op unless we start copying the data.
}

void QSGModelParticle::reset()
{
    QSGParticlePainter::reset();
    //TODO: Cleanup items?
    //m_available.clear();//Should this be reset too?
    //m_pendingItems.clear();//TODO: Should this be done? If so, Emit signal?
}


QSGNode* QSGModelParticle::updatePaintNode(QSGNode* n, UpdatePaintNodeData* d)
{
    //Dummy update just to get painting tick
    if(m_pleaseReset){
        m_pleaseReset = false;
        reset();
    }
    prepareNextFrame();

    update();//Get called again
    if(n)
        n->markDirty(QSGNode::DirtyMaterial);
    return QSGItem::updatePaintNode(n,d);
}

void QSGModelParticle::prepareNextFrame()
{
    qint64 timeStamp = m_system->systemSync(this);
    qreal curT = timeStamp/1000.0;
    qreal dt = curT - m_lastT;
    m_lastT = curT;
    if(!m_activeCount)
        return;

    //TODO: Size, better fade?
    for(int i=0; i<count(); i++){
        QSGParticleData* data = m_data[i];
        if(!data->delegate)
            continue;
        qreal t = ((timeStamp/1000.0) - data->t) / data->lifeSpan;
        if(m_stasis.contains(m_data[i]->delegate)) {
            m_data[i]->t += dt;//Stasis effect
            continue;
        }
        if(t >= 1.0){//Usually happens from load
            m_available << m_data[i]->modelIndex;
            m_deletables << m_data[i]->delegate;
            m_data[i]->modelIndex = -1;
            m_data[i]->delegate = 0;
            m_data[i] = 0;
            m_activeCount--;
        }else{//Fade
            if(m_fade){
                qreal o = 1.;
                if(t<0.2)
                    o = t*5;
                if(t>0.8)
                    o = (1-t)*5;
                m_data[i]->delegate->setOpacity(o);
            }else{
                m_data[i]->delegate->setOpacity(1.);//###Without fade, it's just a binary toggle - if we turn it off we have to turn it back on
            }
        }
        m_data[i]->delegate->setX(data->curX() - m_data[i]->delegate->width()/2  - m_systemOffset.x());
        m_data[i]->delegate->setY(data->curY() - m_data[i]->delegate->height()/2 - m_systemOffset.y());
    }
}

QSGModelParticleAttached *QSGModelParticle::qmlAttachedProperties(QObject *object)
{
    return new QSGModelParticleAttached(object);
}

QT_END_NAMESPACE
