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

#include "qsgangledirection_p.h"
#include "qsgcustomparticle_p.h"
#include "qsgellipseextruder_p.h"
#include "qsgtrailemitter_p.h"
#include "qsgfriction_p.h"
#include "qsggravity_p.h"
#include "qsgimageparticle_p.h"
#include "qsgitemparticle_p.h"
#include "qsgage_p.h"
#include "qsglineextruder_p.h"
#include "qsgmaskextruder_p.h"
#include "qsgparticleaffector_p.h"
#include "qsgparticleemitter_p.h"
#include "qsgparticleextruder_p.h"
#include "qsgparticlepainter_p.h"
#include "qsgparticlesmodule_p.h"
#include "qsgparticlesystem_p.h"
#include "qsgpointattractor_p.h"
#include "qsgpointdirection_p.h"
#include "qsgspritegoal_p.h"
#include "qsgdirection_p.h"
#include "qsgtargetdirection_p.h"
#include "qsgturbulence_p.h"
#include "qsgwander_p.h"
#include "qsgcumulativedirection_p.h"
#include "qsgcustomaffector_p.h"
#include "qsgrectangleextruder_p.h"
#include "qsgparticlegroup_p.h"
#include "qsggroupgoal_p.h"
#include "qsgmove_p.h"

QT_BEGIN_NAMESPACE

void QSGParticlesModule::defineModule()
{
    const char* uri = "QtQuick.Particles";

    qmlRegisterType<QSGParticleSystem>(uri, 2, 0, "ParticleSystem");
    qmlRegisterType<QSGParticleGroup>(uri, 2, 0, "ParticleGroup");

    qmlRegisterType<QSGImageParticle>(uri, 2, 0, "ImageParticle");
    qmlRegisterType<QSGCustomParticle>(uri, 2, 0, "CustomParticle");
    qmlRegisterType<QQuickItemParticle>(uri, 2, 0, "ItemParticle");

    qmlRegisterType<QSGParticleEmitter>(uri, 2, 0, "Emitter");
    qmlRegisterType<QSGTrailEmitter>(uri, 2, 0, "TrailEmitter");

    qmlRegisterType<QSGEllipseExtruder>(uri, 2, 0, "EllipseShape");
    qmlRegisterType<QSGRectangleExtruder>(uri, 2, 0, "RectangleShape");
    qmlRegisterType<QSGLineExtruder>(uri, 2, 0, "LineShape");
    qmlRegisterType<QSGMaskExtruder>(uri, 2, 0, "MaskShape");

    qmlRegisterType<QSGPointDirection>(uri, 2, 0, "PointDirection");
    qmlRegisterType<QSGAngleDirection>(uri, 2, 0, "AngleDirection");
    qmlRegisterType<QSGTargetDirection>(uri, 2, 0, "TargetDirection");
    qmlRegisterType<QSGCumulativeDirection>(uri, 2, 0, "CumulativeDirection");

    qmlRegisterType<QSGCustomAffector>(uri, 2, 0, "Affector");
    qmlRegisterType<QSGWanderAffector>(uri, 2, 0, "Wander");
    qmlRegisterType<QSGFrictionAffector>(uri, 2, 0, "Friction");
    qmlRegisterType<QSGAttractorAffector>(uri, 2, 0, "Attractor");
    qmlRegisterType<QSGGravityAffector>(uri, 2, 0, "Gravity");
    qmlRegisterType<QSGAgeAffector>(uri, 2, 0, "Age");
    qmlRegisterType<QSGSpriteGoalAffector>(uri, 2, 0, "SpriteGoal");
    qmlRegisterType<QSGGroupGoalAffector>(uri, 2, 0, "GroupGoal");
    qmlRegisterType<QSGTurbulenceAffector>(uri, 2, 0 , "Turbulence");
    qmlRegisterType<QSGMoveAffector>(uri, 2, 0, "Move");

    //Exposed just for completeness
    qmlRegisterUncreatableType<QSGParticleAffector>(uri, 2, 0, "ParticleAffector",
                                                    QStringLiteral("Abstract type. Use one of the inheriting types instead."));
    qmlRegisterUncreatableType<QSGParticlePainter>(uri, 2, 0, "ParticlePainter",
                                                   QStringLiteral("Abstract type. Use one of the inheriting types instead."));
    qmlRegisterUncreatableType<QSGParticleExtruder>(uri, 2, 0, "ParticleExtruder",
                                                    QStringLiteral("Abstract type. Use one of the inheriting types instead."));
    qmlRegisterUncreatableType<QSGDirection>(uri, 2, 0, "NullVector",
                                             QStringLiteral("Abstract type. Use one of the inheriting types instead."));
}

QT_END_NAMESPACE

