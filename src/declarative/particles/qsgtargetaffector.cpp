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

#include "qsgtargetaffector_p.h"
#include <QDebug>

QSGTargetAffector::QSGTargetAffector(QSGItem *parent) :
    QSGParticleAffector(parent), m_targetX(0), m_targetY(0),
    m_targetWidth(0), m_targetHeight(0), m_defaultShape(new QSGParticleExtruder(this)),
    m_targetShape(m_defaultShape), m_targetTime(-1)
{
    m_needsReset = true;
}

void QSGTargetAffector::reset(QSGParticleData* d)
{
    QSGParticleAffector::reset(d);
    m_targets[qMakePair<int,int>(d->group, d->index)] = m_targetShape->extrude(QRectF(m_targetX, m_targetY, m_targetWidth, m_targetHeight));
}

bool QSGTargetAffector::affectParticle(QSGParticleData *d, qreal dt)
{
    Q_UNUSED(dt);
    QPointF target = m_targets[qMakePair<int,int>(d->group, d->index)];
    if (target.isNull())
        return false;
    qreal tt = m_targetTime==-1?d->lifeSpan:(m_targetTime / 1000.0);
    qreal t = tt - (d->lifeSpan - d->lifeLeft());
    if (t <= 0)
        return false;
    qreal tx = d->x + d->sx * tt + 0.5 * d->ax * tt * tt;
    qreal ty = d->y + d->sy * tt + 0.5 * d->ay * tt * tt;

    if (QPointF(tx,ty) == target)
        return false;

    qreal vX = (target.x() - d->x) / tt;
    qreal vY = (target.y() - d->y) / tt;

    qreal w = 1 - (t / tt) + 0.05;
    w = qMin(w, 1.0);
    qreal wvX = vX * w + d->sx * (1 - w);
    qreal wvY = vY * w + d->sy * (1 - w);
    //Screws with the acceleration so that the given start pos with the chosen weighted velocity will still end at the target coordinates
    qreal ax = (2*(target.x() - d->x - wvX*tt)) / (tt*tt);
    qreal ay = (2*(target.y() - d->y - wvY*tt)) / (tt*tt);

    d->sx = wvX;
    d->sy = wvY;
    d->ax = ax;
    d->ay = ay;

    return true;
}
