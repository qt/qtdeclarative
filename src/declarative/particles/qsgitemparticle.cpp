/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
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

#include "qsgitemparticle_p.h"
#include <QtDeclarative/private/qsgvisualitemmodel_p.h>
#include <qsgnode.h>
#include <QTimer>
#include <QDeclarativeComponent>
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmlclass ItemParticle QSGItemParticle
    \inqmlmodule QtQuick.Particles 2
    \since QtQuick.Particles 2.0
    \inherits ParticlePainter
    \brief The ItemParticle element allows you to specify your own delegate to paint particles.

*/

QSGItemParticle::QSGItemParticle(QSGItem *parent) :
    QSGParticlePainter(parent), m_fade(true), m_delegate(0)
{
    setFlag(QSGItem::ItemHasContents);
    QTimer* manageDelegates = new QTimer(this);//TODO: don't leak
    connect(manageDelegates, SIGNAL(timeout()),
            this, SLOT(tick()));
    manageDelegates->setInterval(16);
    manageDelegates->setSingleShot(false);
    manageDelegates->start();
}


void QSGItemParticle::freeze(QSGItem* item)
{
    m_stasis << item;
}


void QSGItemParticle::unfreeze(QSGItem* item)
{
    m_stasis.remove(item);
}

void QSGItemParticle::take(QSGItem *item, bool prioritize)
{
    if (prioritize)
        m_pendingItems.push_front(item);
    else
        m_pendingItems.push_back(item);
}

void QSGItemParticle::give(QSGItem *item)
{
    //TODO: This
}

void QSGItemParticle::initialize(int gIdx, int pIdx)
{
    m_loadables << m_system->m_groupData[gIdx]->data[pIdx];//defer to other thread
}

void QSGItemParticle::commit(int, int)
{
}

void QSGItemParticle::tick()
{
    foreach (QSGItem* item, m_deletables){
        if (m_fade)
            item->setOpacity(0.);
        item->setVisible(false);
        QSGItemParticleAttached* mpa;
        if ((mpa = qobject_cast<QSGItemParticleAttached*>(qmlAttachedPropertiesObject<QSGItemParticle>(item))))
            mpa->detach();//reparent as well?
        //TODO: Delete iff we created it
        m_activeCount--;
    }
    m_deletables.clear();

    foreach (QSGParticleData* d, m_loadables){
        if (m_stasis.contains(d->delegate))
            qWarning() << "Current model particles prefers overwrite:false";
        //remove old item from the particle that is dying to make room for this one
        if (d->delegate)
            m_deletables << d->delegate;
        d->delegate = 0;
        if (!m_pendingItems.isEmpty()){
            d->delegate = m_pendingItems.front();
            m_pendingItems.pop_front();
        }else if (m_delegate){
            d->delegate = qobject_cast<QSGItem*>(m_delegate->create(qmlContext(this)));
        }
        if (d->delegate && d){//###Data can be zero if creating an item leads to a reset - this screws things up.
            d->delegate->setX(d->curX() - d->delegate->width()/2);//TODO: adjust for system?
            d->delegate->setY(d->curY() - d->delegate->height()/2);
            QSGItemParticleAttached* mpa = qobject_cast<QSGItemParticleAttached*>(qmlAttachedPropertiesObject<QSGItemParticle>(d->delegate));
            if (mpa){
                mpa->m_mp = this;
                mpa->attach();
            }
            d->delegate->setParentItem(this);
            if (m_fade)
                d->delegate->setOpacity(0.);
            d->delegate->setVisible(false);//Will be set to true when we prepare the next frame
            m_activeCount++;
        }
    }
    m_loadables.clear();
}

void QSGItemParticle::reset()
{
    QSGParticlePainter::reset();
    //TODO: Cleanup items?
    m_loadables.clear();
    //deletables?
}


QSGNode* QSGItemParticle::updatePaintNode(QSGNode* n, UpdatePaintNodeData* d)
{
    //Dummy update just to get painting tick
    if (m_pleaseReset){
        m_pleaseReset = false;
        reset();
    }
    prepareNextFrame();

    update();//Get called again
    if (n)
        n->markDirty(QSGNode::DirtyMaterial);
    return QSGItem::updatePaintNode(n,d);
}

void QSGItemParticle::prepareNextFrame()
{
    if (!m_system)
        return;
    qint64 timeStamp = m_system->systemSync(this);
    qreal curT = timeStamp/1000.0;
    qreal dt = curT - m_lastT;
    m_lastT = curT;
    if (!m_activeCount)
        return;

    //TODO: Size, better fade?
    foreach (const QString &str, m_particles){
        int gIdx = m_system->m_groupIds[str];
        int count = m_system->m_groupData[gIdx]->size();

        for (int i=0; i<count; i++){
            QSGParticleData* data = m_system->m_groupData[gIdx]->data[i];
            QSGItem* item = data->delegate;
            if (!item)
                continue;
            qreal t = ((timeStamp/1000.0) - data->t) / data->lifeSpan;
            if (m_stasis.contains(item)) {
                data->t += dt;//Stasis effect
                continue;
            }
            if (t >= 1.0){//Usually happens from load
                m_deletables << item;
                data->delegate = 0;
            }else{//Fade
                data->delegate->setVisible(true);
                if (m_fade){
                    qreal o = 1.;
                    if (t<0.2)
                        o = t*5;
                    if (t>0.8)
                        o = (1-t)*5;
                    item->setOpacity(o);
                }
            }
            item->setX(data->curX() - item->width()/2 - m_systemOffset.x());
            item->setY(data->curY() - item->height()/2 - m_systemOffset.y());
        }
    }
}

QSGItemParticleAttached *QSGItemParticle::qmlAttachedProperties(QObject *object)
{
    return new QSGItemParticleAttached(object);
}

QT_END_NAMESPACE
