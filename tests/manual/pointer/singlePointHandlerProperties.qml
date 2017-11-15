/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the manual tests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.9
import Qt.labs.handlers 1.0

Rectangle {
    id: root
    width: 480
    height: 480
    color: "black"

    Item {
        id: crosshairs
        x: dragHandler.point.position.x - width / 2
        y: dragHandler.point.position.y - height / 2
        width: parent.width / 2; height: parent.height / 2
        visible: dragHandler.active
        rotation: dragHandler.point.rotation

        Rectangle {
            color: "goldenrod"
            anchors.centerIn: parent
            width: 2; height: parent.height
            antialiasing: true
        }
        Rectangle {
            color: "goldenrod"
            anchors.centerIn: parent
            width: parent.width; height: 2
            antialiasing: true
        }
        Rectangle {
            color: "goldenrod"
            width: Math.max(2, 50 * dragHandler.point.pressure)
            height: width
            radius: width / 2
            anchors.centerIn: parent
            antialiasing: true
            Rectangle {
                y: -40
                anchors.horizontalCenter: parent.horizontalCenter
                color: "lightsteelblue"
                implicitWidth: label.implicitWidth
                implicitHeight: label.implicitHeight
                Text {
                    id: label
                    text: 'id: ' + dragHandler.point.id.toString(16) + " uid: " + dragHandler.point.uniqueId.numericId +
                        '\npos: (' + dragHandler.point.position.x.toFixed(2) + ', ' + dragHandler.point.position.y.toFixed(2) + ')'
                }
            }
        }
        Rectangle {
            color: "transparent"
            border.color: "white"
            antialiasing: true
            width: dragHandler.point.ellipseDiameters.width
            height: dragHandler.point.ellipseDiameters.height
            radius: Math.min(width / 2, height / 2)
            anchors.centerIn: parent
        }
    }
    Rectangle {
        id: velocityVector
        visible: width > 0
        width: dragHandler.point.velocity.length() * 100
        height: 2
        x: dragHandler.point.position.x
        y: dragHandler.point.position.y
        rotation: Math.atan2(dragHandler.point.velocity.y, dragHandler.point.velocity.x) * 180 / Math.PI
        transformOrigin: Item.BottomLeft
        antialiasing: true

        Image {
            source: "resources/arrowhead.png"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: 16
            height: 12
            antialiasing: true
        }
    }

    Component {
        id: grabbingLocationIndicator
        Image {
            source: "resources/grabbing-location.svg"
            sourceSize.width: 32
            sourceSize.height: 32
        }
    }

    Component {
        id: mouseButtonIndicator
        Image {
            property int buttons
            source: "resources/mouse.png"
            Image {
                source: "resources/mouse_left.png"
                visible: buttons & Qt.LeftButton
            }
            Image {
                source: "resources/mouse_middle.png"
                visible: buttons & Qt.MidButton
            }
            Image {
                source: "resources/mouse_right.png"
                visible: buttons & Qt.RightButton
            }
        }
    }

    DragHandler {
        id: dragHandler
        target: null
        acceptedButtons: Qt.AllButtons
        onGrabChanged: if (active) {    // 'point' is an implicit parameter referencing to a QQuickEventPoint instance
            console.log("grabbed " + point.pointId + " @ " + point.sceneGrabPos)
            grabbingLocationIndicator.createObject(root, {"x": point.sceneGrabPosition.x, "y": point.sceneGrabPosition.y - 16})
        }
        onPointChanged: {
            // Here, 'point' is referring to the property of the DragHandler
            if (point.pressedButtons)
                mouseButtonIndicator.createObject(root, {"x": point.pressPosition.x - 44, "y": point.pressPosition.y - 64, "buttons": point.pressedButtons})
        }
    }
}
