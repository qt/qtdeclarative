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

#include "qsgcustomaffector_p.h"
#include <private/qv8engine_p.h>
#include <private/qdeclarativeengine_p.h>
#include <QDeclarativeEngine>
#include <QDebug>
QT_BEGIN_NAMESPACE

//TODO: Move docs (and inherit) to real base when docs can propagate
/*!
    \qmlsignal QtQuick.Particles2::Affector::affectParticles(Array particles, real dt)

    This handler is called when particles are selected to be affected. particles contains
    an array of particle objects which can be directly manipulated.

    dt is the time since the last time it was affected. Use dt to normalize
    trajectory manipulations to real time.

    Note that JS is slower to execute, so it is not recommended to use this in
    high-volume particle systems.
*/
QSGCustomAffector::QSGCustomAffector(QQuickItem *parent) :
    QSGParticleAffector(parent)
{
}

bool QSGCustomAffector::isAffectConnected()
{
    static int idx = QObjectPrivate::get(this)->signalIndex("affectParticles(QDeclarativeV8Handle,qreal)");
    return QObjectPrivate::get(this)->isSignalConnected(idx);
}

void QSGCustomAffector::affectSystem(qreal dt)
{
    if (!isAffectConnected()) {
        QSGParticleAffector::affectSystem(dt);
        return;
    }
    if (!m_enabled)
        return;
    updateOffsets();

    QList<QSGParticleData*> toAffect;
    foreach (QSGParticleGroupData* gd, m_system->groupData)
        if (activeGroup(m_system->groupData.key(gd)))
            foreach (QSGParticleData* d, gd->data)
                if (shouldAffect(d))
                    toAffect << d;

    v8::HandleScope handle_scope;
    v8::Context::Scope scope(QDeclarativeEnginePrivate::getV8Engine(qmlEngine(this))->context());
    v8::Handle<v8::Array> array = v8::Array::New(toAffect.size());
    for (int i=0; i<toAffect.size(); i++)
        array->Set(i, toAffect[i]->v8Value().toHandle());

    emit affectParticles(QDeclarativeV8Handle::fromHandle(array), dt);

    foreach (QSGParticleData* d, toAffect)
        if (d->update == 1.0)
            postAffect(d);
}

QT_END_NAMESPACE
