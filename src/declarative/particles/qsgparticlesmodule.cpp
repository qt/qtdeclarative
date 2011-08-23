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

#include "qsgangleddirection_p.h"
#include "qsgcustomparticle_p.h"
#include "qsgellipseextruder_p.h"
#include "qsgfollowemitter_p.h"
#include "qsgfriction_p.h"
#include "qsggravity_p.h"
#include "qsgimageparticle_p.h"
#include "qsgitemparticle_p.h"
#include "qsgkill_p.h"
#include "qsglineextruder_p.h"
#include "qsgmaskextruder_p.h"
#include "qsgmodelparticle_p.h"
#include "qsgparticleaffector_p.h"
#include "qsgparticleemitter_p.h"
#include "qsgparticleextruder_p.h"
#include "qsgparticlepainter_p.h"
#include "qsgparticlesmodule_p.h"
#include "qsgparticlesystem_p.h"
#include "qsgpointattractor_p.h"
#include "qsgpointdirection_p.h"
#include "qsgspritegoal_p.h"
#include "qsgstochasticdirection_p.h"
#include "qsgtargeteddirection_p.h"
#include "qsgturbulence_p.h"
#include "qsgwander_p.h"
#include "qsgtargetaffector_p.h"
#include "qsgcumulativedirection_p.h"

QT_BEGIN_NAMESPACE

void QSGParticlesModule::defineModule()
{
    const char* uri = "QtQuick.Particles";

    qmlRegisterType<QSGParticleSystem>(uri, 2, 0, "ParticleSystem");

    qmlRegisterType<QSGImageParticle>(uri, 2, 0, "ImageParticle");
    qmlRegisterType<QSGCustomParticle>(uri, 2, 0, "CustomParticle");
    qmlRegisterType<QSGItemParticle>(uri, 2, 0, "ItemParticle");
    qmlRegisterType<QSGModelParticle>(uri, 2, 0, "ModelParticle");

    qmlRegisterType<QSGParticleEmitter>(uri, 2, 0, "Emitter");
    qmlRegisterType<QSGFollowEmitter>(uri, 2, 0, "FollowEmitter");

    qmlRegisterType<QSGEllipseExtruder>(uri, 2, 0, "EllipseShape");
    qmlRegisterType<QSGLineExtruder>(uri, 2, 0, "LineShape");
    qmlRegisterType<QSGMaskExtruder>(uri, 2, 0, "MaskShape");

    qmlRegisterType<QSGPointDirection>(uri, 2, 0, "PointDirection");
    qmlRegisterType<QSGAngledDirection>(uri, 2, 0, "AngledDirection");
    qmlRegisterType<QSGTargetedDirection>(uri, 2, 0, "TargetedDirection");
    qmlRegisterType<QSGCumulativeDirection>(uri, 2, 0, "CumulativeDirection");

    qmlRegisterType<QSGParticleAffector>(uri, 2, 0, "Affector");//useful for the triggered signal
    qmlRegisterType<QSGWanderAffector>(uri, 2, 0, "Wander");
    qmlRegisterType<QSGFrictionAffector>(uri, 2, 0, "Friction");
    qmlRegisterType<QSGPointAttractorAffector>(uri, 2, 0, "PointAttractor");
    qmlRegisterType<QSGGravityAffector>(uri, 2, 0, "Gravity");
    qmlRegisterType<QSGKillAffector>(uri, 2, 0, "Kill");
    qmlRegisterType<QSGSpriteGoalAffector>(uri, 2, 0, "SpriteGoal");
    qmlRegisterType<QSGTurbulenceAffector>(uri, 2, 0 , "Turbulence");
    qmlRegisterType<QSGTargetAffector>(uri, 2, 0 , "Target");

    //Exposed just for completeness
    qmlRegisterType<QSGParticlePainter>(uri, 2, 0, "ParticlePainter");
    qmlRegisterType<QSGParticleExtruder>(uri, 2, 0, "ParticleExtruder");
    qmlRegisterType<QSGStochasticDirection>(uri, 2, 0, "NullVector");
}

QT_END_NAMESPACE

