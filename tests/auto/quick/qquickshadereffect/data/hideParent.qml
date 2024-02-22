// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12

Item {
    id: root
    width: 640
    height: 480
    objectName: "qtbug86402Container"

    property bool finished

    Item {
        id: popup
        objectName: "popup"
        width: 200
        height: 200

        Rectangle {
            id: rect
            objectName: "rect"
            implicitWidth: 100
            implicitHeight: 100
            color: "blue"

            Item {
                id: ripple
                objectName: "ripple"
                anchors.fill: parent
                visible: false

                Rectangle {
                    id: rippleBox
                    objectName: "rippleBox"
                    property real cx
                    property real cy
                    x: cx - width / 2
                    y: cy - height / 2
                    width: 0
                    height: width
                    radius: width / 2
                    color: Qt.darker("red", 1.8)
                }
                layer.effect: ShaderEffect {
                    id: mask
                    objectName: "shaderEffect"
                    property variant source
                    property variant maskSource: rect

                    fragmentShader: "opacity-mask.frag.qsb"
                }
            }

            SequentialAnimation {
                id: rippleStartAnimation
                running: popup.visible
                onFinished: {
                    popup.parent = null
                    rippleEndAnimation.start()
                }

                ScriptAction {
                    script: {
                        rippleBox.width = 0
                        rippleBox.opacity = 0.3
                        ripple.visible = true
                        ripple.layer.enabled = true
                    }
                }
                NumberAnimation {
                    target: rippleBox
                    property: "width"
                    from: 0
                    to: Math.max(rect.width,
                                 rect.height) * 2.2
                    duration: 100
                }
            }
            SequentialAnimation {
                id: rippleEndAnimation

                onFinished: root.finished = true

                //Causes Crash on QT Versions > 5.12.5
                NumberAnimation {
                    target: rippleBox
                    property: "opacity"
                    to: 0
                    duration: 100
                }
                ScriptAction {
                    script: {
                        rippleBox.opacity = 0
                        ripple.layer.enabled = false
                        ripple.visible = false
                    }
                }
            }
        }
    }
}
