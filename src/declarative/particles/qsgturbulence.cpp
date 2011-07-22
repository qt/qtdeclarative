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

#include "qsgturbulence_p.h"
#include "qsgparticlepainter_p.h"//TODO: Why was this needed again?
#include <cmath>
#include <cstdlib>
#include <QDebug>
QT_BEGIN_NAMESPACE

/*!
    \qmlclass Turbulence QSGTurbulenceAffector
    \inqmlmodule QtQuick.Particles 2
    \since QtQuick.Particles 2.0
    \inherits Affector
    \brief The TurbulenceAffector is a bit of a hack and probably shouldn't be used yet.

*/
/*!
    \qmlproperty int QtQuick.Particles2::Turbulence::strength
    Maximum magnitude of a point in the vector field.
*/
/*!
    \qmlproperty int QtQuick.Particles2::Turbulence::frequency
    Times per second vector field is perturbed.
*/
/*!
    \qmlproperty int QtQuick.Particles2::Turbulence::gridSize
    Square root of the number of points in the vector field simulcrum.
*/

QSGTurbulenceAffector::QSGTurbulenceAffector(QSGItem *parent) :
    QSGParticleAffector(parent),
    m_strength(10), m_lastT(0), m_frequency(64), m_gridSize(10), m_field(0), m_inited(false)
{
    //TODO: Update grid on size change
}

QSGTurbulenceAffector::~QSGTurbulenceAffector()
{
    if (m_field) {
        for (int i=0; i<m_gridSize; i++)
            free(m_field[i]);
        free(m_field);
    }
}

static qreal magnitude(qreal x, qreal y)
{
    return sqrt(x*x + y*y);
}

void QSGTurbulenceAffector::setSize(int arg)
{
    if (m_gridSize != arg) {
        if (m_field){ //deallocate and then reallocate grid
            for (int i=0; i<m_gridSize; i++)
                free(m_field[i]);
            free(m_field);
            m_system = 0;
        }
        m_gridSize = arg;
        emit sizeChanged(arg);
    }
}

void QSGTurbulenceAffector::ensureInit()
{
    if (m_inited)
        return;
    m_inited = true;
    m_field = (QPointF**)malloc(m_gridSize * sizeof(QPointF*));
    for (int i=0; i<m_gridSize; i++)
        m_field[i]  = (QPointF*)malloc(m_gridSize * sizeof(QPointF));
    for (int i=0; i<m_gridSize; i++)
        for (int j=0; j<m_gridSize; j++)
            m_field[i][j] = QPointF();
    m_spacing = QPointF(width()/m_gridSize, height()/m_gridSize);
    m_magSum = magnitude(m_spacing.x(), m_spacing.y())*2;
}

void QSGTurbulenceAffector::mapUpdate()
{
    QPoint pos(rand() % m_gridSize, rand() % m_gridSize);
    QPointF vector(m_strength  - (((qreal)rand() / RAND_MAX) * m_strength*2),
                   m_strength  - (((qreal)rand() / RAND_MAX) * m_strength*2));
    for (int i = 0; i < m_gridSize; i++){
        for (int j = 0; j < m_gridSize; j++){
            qreal dist = magnitude(i-pos.x(), j-pos.y());
            m_field[i][j] += vector/(dist + 1);
            if (magnitude(m_field[i][j].x(), m_field[i][j].y()) > m_strength){
                //Speed limit
                qreal theta = atan2(m_field[i][j].y(), m_field[i][j].x());
                m_field[i][j].setX(m_strength * cos(theta));
                m_field[i][j].setY(m_strength * sin(theta));
            }
        }
    }
}


void QSGTurbulenceAffector::affectSystem(qreal dt)
{
    if (!m_system || !m_active)
        return;
    ensureInit();
    qreal period = 1.0/m_frequency;
    qreal time = m_system->m_timeInt / 1000.0;
    while ( m_lastT < time ){
        mapUpdate();
        m_lastT += period;
    }

    foreach (QSGParticleGroupData *gd, m_system->m_groupData){
        if (!activeGroup(m_system->m_groupData.key(gd)))//TODO: Surely this can be done better
            return;
        foreach (QSGParticleData *d, gd->data){
            if (!d || !activeGroup(d->group))
                return;
            qreal fx = 0.0;
            qreal fy = 0.0;
            QPointF pos = QPointF(d->curX() - x(), d->curY() - y());//TODO: Offset
            QPointF nodePos = QPointF(pos.x() / m_spacing.x(), pos.y() / m_spacing.y());
            QSet<QPair<int, int> > nodes;
            nodes << qMakePair((int)ceil(nodePos.x()), (int)ceil(nodePos.y()));
            nodes << qMakePair((int)ceil(nodePos.x()), (int)floor(nodePos.y()));
            nodes << qMakePair((int)floor(nodePos.x()), (int)ceil(nodePos.y()));
            nodes << qMakePair((int)floor(nodePos.x()), (int)floor(nodePos.y()));
            typedef QPair<int, int> intPair;
            foreach (const intPair &p, nodes){
                if (!QRect(0,0,m_gridSize-1,m_gridSize-1).contains(QPoint(p.first, p.second)))
                    continue;
                qreal dist = magnitude(pos.x() - p.first*m_spacing.x(), pos.y() - p.second*m_spacing.y());//TODO: Mathematically valid
                fx += m_field[p.first][p.second].x() * ((m_magSum - dist)/m_magSum);//Proportionally weight nodes
                fy += m_field[p.first][p.second].y() * ((m_magSum - dist)/m_magSum);
            }
            if (fx || fy){
                d->setInstantaneousVX(d->curVX()+ fx * dt);
                d->setInstantaneousVY(d->curVY()+ fy * dt);
                m_system->m_needsReset << d;
            }
        }
    }
}

QT_END_NAMESPACE
