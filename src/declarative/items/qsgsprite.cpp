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

#include "qsgsprite_p.h"
//TODO: Split out particle system dependency
#include "qsgparticlesystem_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE

QSGSprite::QSGSprite(QObject *parent) :
    QObject(parent)
    , m_generatedCount(0)
    , m_framesPerRow(0)
    , m_frames(1)
    , m_frameHeight(0)
    , m_frameWidth(0)
    , m_duration(1000)
{
}

void redirectError(QDeclarativeListProperty<QObject> *prop, QObject *value)
{
    qWarning() << "Could not add " << value << " to state" << prop->object << "as it is not associated with a particle system.";
}

QDeclarativeListProperty<QObject> QSGSprite::particleChildren()
{
    QSGParticleSystem* system = qobject_cast<QSGParticleSystem*>(parent());
    if (system)
        return QDeclarativeListProperty<QObject>(this, 0, &QSGParticleSystem::stateRedirect);
    else
        return QDeclarativeListProperty<QObject>(this, 0, &redirectError);
}

int QSGSprite::variedDuration() const
{
    return m_duration
            + (m_durationVariance * ((qreal)qrand()/RAND_MAX) * 2)
            - m_durationVariance;
}

QT_END_NAMESPACE
