/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

import QtQuick 2.0
import QtQuick.Particles 2.0
import "samegame.js" as Logic

Item {
    id: gameCanvas
    property int score: 0
    property int blockSize: 40
    property ParticleSystem ps: particleSystem
    Image {
        id: background
        anchors.fill: parent
        z: -1
        source: "pics/background.png"
        fillMode: Image.PreserveAspectCrop
    }

    width: 480
    height: 800
    MouseArea {
        anchors.fill: parent; onClicked: Logic.handleClick(mouse.x,mouse.y);
    }
    ParticleSystem{ 
        id: particleSystem;
        z:2
        ImageParticle {
            groups: ["red"]
            color: Qt.darker("red");//Actually want desaturated...
            source: "pics/particle.png"
            colorVariation: 0.4
            alpha: 0.1
        }
        ImageParticle {
            groups: ["green"]
            color: Qt.darker("green");//Actually want desaturated...
            source: "pics/particle.png"
            colorVariation: 0.4
            alpha: 0.1
        }
        ImageParticle {
            groups: ["blue"]
            color: Qt.darker("blue");//Actually want desaturated...
            source: "pics/particle.png"
            colorVariation: 0.4
            alpha: 0.1
        }
        anchors.fill: parent
    }
}

