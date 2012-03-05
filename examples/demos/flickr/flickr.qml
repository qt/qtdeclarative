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
import "content"

Item {
    id: screen; width: 320; height: 480
    property bool inGridView : true

    Rectangle {
        id: background
        anchors.fill: parent; color: "#343434";

        Image { source: "content/images/stripes.png"; fillMode: Image.Tile; anchors.fill: parent; opacity: 0.3 }
        ParticleSystem {
            id: bgParticles
            anchors.fill: parent
            ImageParticle {
                groups: ["trail"]
                source: "content/images/particle.png"
                color: "#1A1A6F"
                alpha: 0.1
                colorVariation: 0.01
                blueVariation: 0.8
            }
            Emitter {
                group: "drops"
                width: parent.width
                emitRate: 0.5
                lifeSpan: 20000
                startTime: 16000
                speed: PointDirection{
                    y: {screen.height/18}
                }
            }
            TrailEmitter {
                follow: "drops"
                group: "trail"
                emitRatePerParticle: 18
                size: 32
                endSize: 0
                sizeVariation: 4
                lifeSpan: 1200
                anchors.fill: parent
                emitWidth: 16
                emitHeight: 16
                emitShape: EllipseShape{}
            }
        }

        VisualDataModel{
            id: vdm
            delegate: UnifiedDelegate{}
            model: RssModel { id: rssModel }
        }

        Item {
            id: views
            width: parent.width
            anchors.top: titleBar.bottom; anchors.bottom: toolBar.top

            GridView {
                id: photoGridView; model: vdm.parts.grid
                cacheBuffer: 1000
                cellWidth: (parent.width-2)/4; cellHeight: cellWidth; width: parent.width; height: parent.height
            }

            states: State {
                name: "GridView"; when: state.inGridView == true
            }

            transitions: Transition {
                NumberAnimation { properties: "x"; duration: 500; easing.type: Easing.InOutQuad }
            }

            ImageDetails { id: imageDetails; width: parent.width; anchors.left: views.right; height: parent.height }

            Item { id: foreground; anchors.fill: parent }
        }

        TitleBar { id: titleBar; width: parent.width; height: 40; opacity: 0.9 }

        ToolBar {
            id: toolBar
            height: 40; anchors.bottom: parent.bottom; width: parent.width; opacity: 0.9
            button1Label: "Update"; button2Label: "View mode"
            onButton1Clicked: rssModel.reload()
            onButton2Clicked: if (screen.inGridView == true) screen.inGridView = false; else screen.inGridView = true
        }

        Connections {
            target: imageDetails
            onClosed: {
                if (background.state == "DetailedView") {
                    background.state = '';
                    imageDetails.photoUrl = "";
                }
            }
        }

        states: State {
            name: "DetailedView"
            PropertyChanges { target: views; x: -parent.width }
            PropertyChanges { target: toolBar; button1Label: "View..." }
            PropertyChanges {
                target: toolBar
                onButton1Clicked: if (imageDetails.state=='') imageDetails.state='Back'; else imageDetails.state=''
            }
            PropertyChanges { target: toolBar; button2Label: "Back" }
            PropertyChanges { target: toolBar; onButton2Clicked: imageDetails.closed() }
        }

        transitions: Transition {
            NumberAnimation { properties: "x"; duration: 500; easing.type: Easing.InOutQuad }
        }

    }
}
