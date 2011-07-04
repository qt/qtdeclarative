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
    m_system(0), m_count(0), m_sentinel(new QSGParticleData(0))
{
    connect(this, SIGNAL(parentChanged(QSGItem*)),
            this, SLOT(calcSystemOffset()));
    connect(this, SIGNAL(xChanged()),
            this, SLOT(calcSystemOffset()));
    connect(this, SIGNAL(yChanged()),
            this, SLOT(calcSystemOffset()));
}

void QSGParticlePainter::componentComplete()
{
    if (!m_system && qobject_cast<QSGParticleSystem*>(parentItem()))
        setSystem(qobject_cast<QSGParticleSystem*>(parentItem()));
    if (!m_system)
        qWarning() << "ParticlePainter created without a particle system specified";//TODO: useful QML warnings, like line number?
    QSGItem::componentComplete();
}


void QSGParticlePainter::setSystem(QSGParticleSystem *arg)
{
    if (m_system != arg) {
        m_system = arg;
        if (m_system){
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
    initialize(d->group, d->index);
    commit(d->group, d->index);
}

void QSGParticlePainter::reload(QSGParticleData* d)
{
    commit(d->group, d->index);
}

void QSGParticlePainter::reset()
{
    calcSystemOffset(true);//In case an ancestor changed in some way
}

void QSGParticlePainter::setCount(int c)//### TODO: some resizeing so that particles can reallocate on size change instead of recreate
{
    Q_ASSERT(c >= 0); //XXX
    if (c == m_count)
        return;
    m_count = c;
    emit countChanged();
    reset();
}

int QSGParticlePainter::count()
{
    return m_count;
}

void QSGParticlePainter::calcSystemOffset(bool resetPending)
{
    if (!m_system || !parentItem())
        return;
    QPointF lastOffset = m_systemOffset;
    m_systemOffset = -1 * this->mapFromItem(m_system, QPointF(0.0, 0.0));
    if (lastOffset != m_systemOffset && !resetPending){
        //Reload all particles//TODO: Necessary?
        foreach (const QString &g, m_particles){
            int gId = m_system->m_groupIds[g];
            foreach (QSGParticleData* d, m_system->m_groupData[gId]->data)
                reload(d);
        }
    }
}
QT_END_NAMESPACE
