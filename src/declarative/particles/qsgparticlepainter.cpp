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

#include "qsgparticlepainter_p.h"
#include <QDebug>
QT_BEGIN_NAMESPACE
/*!
    \qmlclass ParticlePainter QSGParticlePainter
    \inqmlmodule QtQuick.Particles 2
    \since QtQuick.Particles 2.0
    \inherits ParticlePainter
    \brief ParticlePainter elements allow you to specify how to paint particles.

    The default implementation paints nothing. See the subclasses if you want to
    paint something visible.

*/
/*!
    \qmlproperty ParticleSystem QtQuick.Particles2::ParticlePainter::system
    This is the system whose particles can be painted by the element.
    If the ParticlePainter is a direct child of a ParticleSystem, it will automatically be associated with it.
*/
/*!
    \qmlproperty list<string> QtQuick.Particles2::ParticlePainter::particles
    Which logical particle groups will be painted.

    If empty, it will paint the default particle ("").
*/
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
    m_pendingCommits << qMakePair<int, int>(d->group, d->index);
}

void QSGParticlePainter::reload(QSGParticleData* d)
{
    m_pendingCommits << qMakePair<int, int>(d->group, d->index);
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
typedef QPair<int,int> intPair;
void QSGParticlePainter::performPendingCommits()
{
    foreach (intPair p, m_pendingCommits)
        commit(p.first, p.second);
    m_pendingCommits.clear();
}

QT_END_NAMESPACE
