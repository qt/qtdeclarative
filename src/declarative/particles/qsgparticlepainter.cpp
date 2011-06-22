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

#include "qsgparticlepainter_p.h"
#include <QDebug>
QT_BEGIN_NAMESPACE
QSGParticlePainter::QSGParticlePainter(QSGItem *parent) :
    QSGItem(parent),
    m_system(0), m_count(0), m_lastStart(0), m_sentinel(new QSGParticleData)
{
    connect(this, SIGNAL(xChanged()),
            this, SLOT(calcSystemOffset()));
    connect(this, SIGNAL(yChanged()),
            this, SLOT(calcSystemOffset()));
}

void QSGParticlePainter::componentComplete()
{
    if(!m_system && qobject_cast<QSGParticleSystem*>(parentItem()))
        setSystem(qobject_cast<QSGParticleSystem*>(parentItem()));
    if(!m_system)
        qWarning() << "ParticlePainter created without a particle system specified";//TODO: useful QML warnings, like line number?
    QSGItem::componentComplete();
}


void QSGParticlePainter::setSystem(QSGParticleSystem *arg)
{
    if (m_system != arg) {
        m_system = arg;
        if(m_system){
            m_system->registerParticlePainter(this);
            connect(m_system, SIGNAL(xChanged()),
                    this, SLOT(calcSystemOffset()));
            connect(m_system, SIGNAL(yChanged()),
                    this, SLOT(calcSystemOffset()));
            calcSystemOffset();
        }
        emit systemChanged(arg);
    }
}

void QSGParticlePainter::load(QSGParticleData* d)
{
    int idx = particleTypeIndex(d);
    m_data[idx] = d;
    initialize(idx);
    reload(idx);
}

void QSGParticlePainter::reload(QSGParticleData* d)
{
    reload(particleTypeIndex(d));
}

void QSGParticlePainter::reset()
{
    //Have to every time because what it's emitting may have changed and that affects particleTypeIndex
    if(m_system && !m_inResize)
        resize(0,1);//###Fix this by making resize take sensible arguments
    //###This also means double resets. Make reset not virtual?
}

void QSGParticlePainter::resize(int oldSize, int newSize)
{
    if(newSize == oldSize)//TODO: What if particles switched so indices change but total count is the same?
        return;

    QHash<int, QPair<int, int> > oldStarts(m_particleStarts);
    //Update particle starts datastore
    m_particleStarts.clear();
    m_lastStart = 0;
    QList<int> particleList;
    if(m_particles.isEmpty())
        particleList << 0;
    foreach(const QString &s, m_particles)
        particleList << m_system->m_groupIds[s];
    foreach(int gIdx, particleList){
        QSGParticleGroupData *gd = m_system->m_groupData[gIdx];
        m_particleStarts.insert(gIdx, qMakePair<int, int>(gd->size, m_lastStart));
        m_lastStart += gd->size;
    }

    //Shuffle stuff around
    //TODO: In place shuffling because it's faster
    QVector<QSGParticleData*> oldData(m_data);
    QVector<QObject*> oldAttached(m_attachedData);
    m_data.clear();
    m_data.resize(m_count);
    m_attachedData.resize(m_count);
    foreach(int gIdx, particleList){
        QSGParticleGroupData *gd = m_system->m_groupData[gIdx];
        for(int i=0; i<gd->data.size(); i++){//TODO: When group didn't exist before
            int newIdx = m_particleStarts[gIdx].second + i;
            int oldIdx = oldStarts[gIdx].second + i;
            if(i >= oldStarts[gIdx].first || oldData.size() <= oldIdx){
                m_data[newIdx] = m_sentinel;
            }else{
                m_data[newIdx] = oldData[oldIdx];
                m_attachedData[newIdx] = oldAttached[oldIdx];
            }
        }
    }
    m_inResize = true;
    reset();
    m_inResize = false;
}


void QSGParticlePainter::setCount(int c)
{
    Q_ASSERT(c >= 0); //XXX
    if(c == m_count)
        return;
    int lastCount = m_count;
    m_count = c;
    resize(lastCount, m_count);
    emit countChanged();
}

int QSGParticlePainter::count()
{
    return m_count;
}

int QSGParticlePainter::particleTypeIndex(QSGParticleData* d)
{
    Q_ASSERT(d && m_particleStarts.contains(d->group));//XXX
    int ret = m_particleStarts[d->group].second + d->index;
    Q_ASSERT(ret >=0 && ret < m_count);//XXX:shouldn't assert, but bugs here were hard to find in the past
    return ret;
}


void QSGParticlePainter::calcSystemOffset()
{
    if(!m_system)
        return;
    QPointF lastOffset = m_systemOffset;
    m_systemOffset = -1 * this->mapFromItem(m_system, QPointF());
    if(lastOffset != m_systemOffset){
        //Reload all particles//TODO: Necessary?
        foreach(const QString &g, m_particles){
            int gId = m_system->m_groupIds[g];
            foreach(QSGParticleData* d, m_system->m_groupData[gId]->data)
                reload(d);
        }
    }
}
QT_END_NAMESPACE
