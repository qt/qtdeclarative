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
    \inherits Affector
    \brief Turbulence provides fluid like forces based on a noise image.

    The Turbulence Element scales the noise source over the area it affects,
    and uses the curl of that source to generate force vectors.

    Turbulence requires a fixed size. Unlike other affectors, a 0x0 Turbulence element
    will affect no particles.

    The source should be relatively smooth black and white noise, such as perlin noise.
*/
/*!
    \qmlproperty real QtQuick.Particles2::Turbulence::strength

    The magnitude of the velocity vector at any point varies between zero and
    the square root of two. It will then be multiplied by strength to get the
    velocity per second for the particles affected by the turbulence.
*/
/*!
    \qmlproperty url QtQuick.Particles2::Turbulence::noiseSource

    The source image to generate the turbulence from. It will be scaled to the size of the element,
    so equal or larger sizes will give better results. Tweaking this image is the only way to tweak
    behavior such as where vortices are or how many exist.

    The source should be a relatively smooth black and white noise image, such as perlin noise.
    A default image will be used if none is provided.
*/

QSGTurbulenceAffector::QSGTurbulenceAffector(QSGItem *parent) :
    QSGParticleAffector(parent),
    m_strength(10), m_lastT(0), m_gridSize(0), m_field(0), m_vectorField(0), m_inited(false)
{
}

void QSGTurbulenceAffector::geometryChanged(const QRectF &, const QRectF &)
{
    initializeGrid();
}

QSGTurbulenceAffector::~QSGTurbulenceAffector()
{
    if (m_field) {
        for (int i=0; i<m_gridSize; i++)
            free(m_field[i]);
        free(m_field);
    }
    if (m_vectorField) {
        for (int i=0; i<m_gridSize; i++)
            free(m_vectorField[i]);
        free(m_vectorField);
    }
}

static qreal magnitude(qreal x, qreal y)
{
    return sqrt(x*x + y*y);
}

void QSGTurbulenceAffector::initializeGrid()
{
    if (!m_inited)
        return;

    int arg = qMax(width(), height());
    if (m_gridSize != arg) {
        if (m_field){ //deallocate and then reallocate grid
            for (int i=0; i<m_gridSize; i++)
                free(m_field[i]);
            free(m_field);
            m_system = 0;
        }
        if (m_vectorField) {
            for (int i=0; i<m_gridSize; i++)
                free(m_vectorField[i]);
            free(m_vectorField);
        }
        m_gridSize = arg;
    }

    m_field = (qreal**)malloc(m_gridSize * sizeof(qreal*));
    for (int i=0; i<m_gridSize; i++)
        m_field[i] = (qreal*)malloc(m_gridSize * sizeof(qreal));
    m_vectorField = (QPointF**)malloc(m_gridSize * sizeof(QPointF*));
    for (int i=0; i<m_gridSize; i++)
        m_vectorField[i] = (QPointF*)malloc(m_gridSize * sizeof(QPointF));

    QImage image = QImage(m_noiseSource.toLocalFile()).scaled(QSize(m_gridSize, m_gridSize));
    if (image.isNull())
        image = QImage(":defaultshaders/noise.png").scaled(QSize(m_gridSize, m_gridSize));

    for (int i=0; i<m_gridSize; i++)
        for (int j=0; j<m_gridSize; j++)
            m_field[i][j] = qRed(image.pixel(QPoint(i,j)));//Red as proxy for Value
    for (int i=0; i<m_gridSize; i++){
        for (int j=0; j<m_gridSize; j++){
            m_vectorField[i][j].setX(boundsRespectingField(i,j) - boundsRespectingField(i,j-1));
            m_vectorField[i][j].setY(boundsRespectingField(i-1,j) - boundsRespectingField(i,j));
        }
    }
}

qreal QSGTurbulenceAffector::boundsRespectingField(int x, int y)
{
    if (x < 0)
        x = 0;
    if (x >= m_gridSize)
        x = m_gridSize - 1;
    if (y < 0)
        y = 0;
    if (y >= m_gridSize)
        y = m_gridSize - 1;
    return m_field[x][y];
}

void QSGTurbulenceAffector::ensureInit()
{
    if (m_inited)
        return;
    m_inited = true;
    initializeGrid();
}

void QSGTurbulenceAffector::affectSystem(qreal dt)
{
    if (!m_system || !m_enabled)
        return;
    ensureInit();
    updateOffsets();//### Needed if an ancestor is transformed.

    QRectF boundsRect(0, 0, width()-1, height()-1);
    foreach (QSGParticleGroupData *gd, m_system->m_groupData){
        if (!activeGroup(m_system->m_groupData.key(gd)))
            continue;
        foreach (QSGParticleData *d, gd->data){
            if (!shouldAffect(d))
                continue;
            QPoint pos = (QPointF(d->curX(), d->curY()) - m_offset).toPoint();
            qreal fx = 0.0;
            qreal fy = 0.0;
            fx += m_vectorField[pos.x()][pos.y()].x() * m_strength;
            fy += m_vectorField[pos.x()][pos.y()].y() * m_strength;
            if (fx || fy){
                d->setInstantaneousVX(d->curVX()+ fx * dt);
                d->setInstantaneousVY(d->curVY()+ fy * dt);
                postAffect(d);
            }
        }
    }
}

QT_END_NAMESPACE
