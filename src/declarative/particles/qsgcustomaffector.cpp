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
#include <QDebug>
QT_BEGIN_NAMESPACE

//TODO: Move docs (and inherit) to real base when docs can propagate
//TODO: Document particle 'type'
/*!
    \qmlsignal QtQuick.Particles2::Affector::affectParticle(particle, dt)

    This handler is called when particles are selected to be affected.

    dt is the time since the last time it was affected. Use dt to normalize
    trajectory manipulations to real time.

    Note that JS is slower to execute, so it is not recommended to use this in
    high-volume particle systems.
*/
QSGCustomAffector::QSGCustomAffector(QSGItem *parent) :
    QSGParticleAffector(parent)
{
}

bool QSGCustomAffector::isAffectConnected()
{
    static int idx = QObjectPrivate::get(this)->signalIndex("affectParticle(QDeclarativeV8Handle,qreal)");
    return QObjectPrivate::get(this)->isSignalConnected(idx);
}

bool QSGCustomAffector::affectParticle(QSGParticleData *d, qreal dt)
{
    if (isAffectConnected()){
        d->update = 0.0;
        emit affectParticle(d->v8Value(), dt);
        return d->update == 1.0;
    }
    return true;
}

QT_END_NAMESPACE
