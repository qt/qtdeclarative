/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

Item{
    id: container
    property variant progress: 0

    Rectangle {
        anchors.fill: parent; smooth: true
        border.color: "white"; border.width: 0; radius: height/2 - 2
        gradient: Gradient {
            GradientStop { position: 0; color: "#66343434" }
            GradientStop { position: 1.0; color: "#66000000" }
        }
    }

    ParticleSystem{
        running: container.visible
        id: barSys
    }
    ImageParticle{
        color: "lightsteelblue"
        alpha: 0.1
        colorVariation: 0.05
        source: "images/particle.png"
        system: barSys
    }
    Emitter{
        y: 2; height: parent.height-4;
        x: 2; width: Math.max(parent.width * progress - 4, 0);
        speed: AngleDirection{ angleVariation: 180; magnitudeVariation: 12 }
        system: barSys
        emitRate: width;
        lifeSpan: 1000
        size: 20
        sizeVariation: 4
        endSize: 12
        maximumEmitted: parent.width;
    }

    Text {
        text: Math.round(progress * 100) + "%"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        color: Qt.rgba(1.0, 1.0 - progress, 1.0 - progress,0.9); font.bold: true; font.pixelSize: 15
    }
}
