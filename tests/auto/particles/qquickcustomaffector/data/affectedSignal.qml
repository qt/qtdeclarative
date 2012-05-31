/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle {
    color: "black"
    width: 320
    height: 320

    ParticleSystem {
        id: sys
        objectName: "system"
        anchors.fill: parent
        property real resultX1: 1234
        property real resultY1: 1234
        property real resultX2: 1234
        property real resultY2: 1234

        ImageParticle {
            source: "../../shared/star.png"
            rotation: 90
        }

        Emitter{
            //0,100 position
            y: 100
            size: 32
            emitRate: 1000
            lifeSpan: 500
        }

        Affector {
            once: true
            onAffected: {//Does nothing else, so should be called for all particles
                sys.resultX1 = x;
                sys.resultY1 = y;
            }
        }

        Affector {
            once: true
            relative: false
            position: PointDirection { x: 0; y: 100; }
            onAffected: {//Does something, so should only be called when it causes a change (it won't)
                sys.resultX2 = x;
                sys.resultY2 = y;
            }
        }
    }
}
