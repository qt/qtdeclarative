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

#include "qquickcustomaffector_p.h"
#include <private/qv8engine_p.h>
#include <private/qdeclarativeengine_p.h>
#include <QDeclarativeEngine>
#include <QDebug>
QT_BEGIN_NAMESPACE

//TODO: Move docs (and inheritence) to real base when docs can propagate. Currently this pretends to be the base class!
/*!
    \qmlsignal QtQuick.Particles2::Affector::affectParticles(Array particles, real dt)

    This handler is called when particles are selected to be affected. particles contains
    an array of particle objects which can be directly manipulated.

    dt is the time since the last time it was affected. Use dt to normalize
    trajectory manipulations to real time.

    Note that JS is slower to execute, so it is not recommended to use this in
    high-volume particle systems.
*/

/*!
    \qmlproperty StochasticDirection QtQuick.Particles2::Affector::position

    Affected particles will have their position set to this direction,
    relative to the ParticleSystem. When interpreting directions as points,
    imagine it as an arrow with the base at the 0,0 of the ParticleSystem and the
    tip at where the specified position will be.
*/

/*!
    \qmlproperty StochasticDirection QtQuick.Particles2::Affector::speed

    Affected particles will have their speed set to this direction.
*/


/*!
    \qmlproperty StochasticDirection QtQuick.Particles2::Affector::acceleration

    Affected particles will have their acceleration set to this direction.
*/


/*!
    \qmlproperty bool QtQuick.Particles2::Affector::relative

    Whether the affected particles have their existing position/speed/acceleration added
    to the new one.

    Default is true.
*/
QQuickCustomAffector::QQuickCustomAffector(QQuickItem *parent) :
    QQuickParticleAffector(parent)
    , m_position(&m_nullVector)
    , m_speed(&m_nullVector)
    , m_acceleration(&m_nullVector)
    , m_relative(true)
{
}

bool QQuickCustomAffector::isAffectConnected()
{
    static int idx = QObjectPrivate::get(this)->signalIndex("affectParticles(QDeclarativeV8Handle,qreal)");
    return QObjectPrivate::get(this)->isSignalConnected(idx);
}

void QQuickCustomAffector::affectSystem(qreal dt)
{
    if (!isAffectConnected()) {
        QQuickParticleAffector::affectSystem(dt);
        return;
    }
    if (!m_enabled)
        return;
    updateOffsets();

    QList<QQuickParticleData*> toAffect;
    foreach (QQuickParticleGroupData* gd, m_system->groupData)
        if (activeGroup(m_system->groupData.key(gd)))
            foreach (QQuickParticleData* d, gd->data)
                if (shouldAffect(d))
                    toAffect << d;

    if (toAffect.isEmpty())
        return;

    if (m_onceOff)
        dt = 1.0;

    v8::HandleScope handle_scope;
    v8::Context::Scope scope(QDeclarativeEnginePrivate::getV8Engine(qmlEngine(this))->context());
    v8::Handle<v8::Array> array = v8::Array::New(toAffect.size());
    for (int i=0; i<toAffect.size(); i++)
        array->Set(i, toAffect[i]->v8Value().toHandle());

    if (dt >= simulationCutoff || dt <= simulationDelta) {
        affectProperties(toAffect, dt);
        emit affectParticles(QDeclarativeV8Handle::fromHandle(array), dt);
    } else {
        int realTime = m_system->timeInt;
        m_system->timeInt -= dt * 1000.0;
        while (dt > simulationDelta) {
            m_system->timeInt += simulationDelta * 1000.0;
            dt -= simulationDelta;
            affectProperties(toAffect, simulationDelta);
            emit affectParticles(QDeclarativeV8Handle::fromHandle(array), simulationDelta);
        }
        m_system->timeInt = realTime;
        if (dt > 0.0) {
            affectProperties(toAffect, dt);
            emit affectParticles(QDeclarativeV8Handle::fromHandle(array), dt);
        }
    }

    foreach (QQuickParticleData* d, toAffect)
        if (d->update == 1.0)
            postAffect(d);
}

bool QQuickCustomAffector::affectParticle(QQuickParticleData *d, qreal dt)
{
    //This does the property based affecting, called by superclass if signal isn't hooked up.
    bool changed = false;
    QPointF curPos(d->curX(), d->curY());

    if (m_acceleration != &m_nullVector){
        QPointF pos = m_acceleration->sample(curPos);
        if (m_relative) {
            pos *= dt;
            pos += QPointF(d->curAX(), d->curAY());
        }
        d->setInstantaneousAX(pos.x());
        d->setInstantaneousAY(pos.y());
        changed = true;
    }

    if (m_speed != &m_nullVector){
        QPointF pos = m_speed->sample(curPos);
        if (m_relative) {
            pos *= dt;
            pos += QPointF(d->curVX(), d->curVY());
        }
        d->setInstantaneousVX(pos.x());
        d->setInstantaneousVY(pos.y());
        changed = true;
    }

    if (m_position != &m_nullVector){
        QPointF pos = m_position->sample(curPos);
        if (m_relative) {
            pos *= dt;
            pos += curPos;
        }
        d->setInstantaneousX(pos.x());
        d->setInstantaneousY(pos.y());
        changed = true;
    }

    return changed;
}

void QQuickCustomAffector::affectProperties(const QList<QQuickParticleData*> particles, qreal dt)
{
    foreach (QQuickParticleData* d, particles)
        if ( affectParticle(d, dt) )
            d->update = 1.0;
}

QT_END_NAMESPACE
