// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import SceneGraphRendering

Item {

    id: root
    width: 640
    height: 480

    SplitView {
        anchors.fill: parent

        Text {
            text: qsTr("Direct")
            color: 'white'
            SplitView.preferredWidth: root.width/2

            Loader {
                anchors.fill: parent
                sourceComponent: customComponent
            }
        }

        Text {
            text: qsTr("Layer")
            color: 'white'
            SplitView.preferredWidth: root.width/2

            Loader {
                anchors.fill: parent
                sourceComponent: customComponent
                onLoaded: item.custom.layer.enabled=true
            }
        }
    }

    Text {
        id: description
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        wrapMode: Text.WordWrap
        text: qsTr("This example creates a custom QQuickItem backed by a QSGRenderNode in the scene graph. " +
              "The render node is placed in front of a red rectangle, and behind a white rectangle. " +
              "Rendering is demonstrated directly into the scenegraph, and as a layered, texture-backed item (layer.enabled set to true). " +
              "Opacity and rotation transform changes are exercised as well.")

        Rectangle {
            z:-1
            anchors.fill: parent
            anchors.margins: -10
            color: 'white'
            opacity: 0.5
            radius: 10
        }
    }

    Component {
        id: customComponent

        Item {
            id: componentRoot
            property alias custom: custom

            Rectangle {
                width: parent.width/5
                height: parent.height/5
                anchors.centerIn: parent
                color: '#ff0000'
            }

            SequentialAnimation {
                running: true
                loops: Animation.Infinite

                OpacityAnimator {
                    target: custom
                    from: 0
                    to: 1
                    duration: 3500
                }
                OpacityAnimator {
                    target: custom
                    from: 1
                    to: 0
                    duration: 3500
                }
            }

            RotationAnimation {
                target: custom
                from: 0
                to: 360
                running: true
                loops: Animation.Infinite
                duration: 11000
            }


            CustomRender {
                id: custom
                width: Math.min(parent.width, parent.height)
                height: width
                anchors.centerIn: parent

                property real a: width/2
                property real b: Math.sqrt(3.0)*a/2;
                vertices: [Qt.vector2d(width/2 - a/2, height/2 + b/3),
                    Qt.vector2d(width/2 + a/2, height/2 + b/3),
                    Qt.vector2d(width/2, height/2 - b*2/3)]

            }

            Rectangle {
                width: parent.width/10
                height: parent.height/10
                anchors.centerIn: parent
                color: '#ffffff'
            }
        }
    }
}
