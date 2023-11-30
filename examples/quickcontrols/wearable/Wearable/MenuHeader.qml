// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import WearableStyle
import QtQuick.Shapes

Item {
    id: header
    property alias title: labelText.text
    signal backClicked()

    height: 50

    Shape {//Shape because Rectangle does not support diagonal gradient
        id: background
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeWidth: 0

            startX: 0
            startY: 0
            PathLine {
                x: header.width
                y: 0
            }
            PathLine {
                x: header.width
                y: header.height
            }
            PathLine {
                x: 0
                y: header.height
            }
            fillGradient: LinearGradient {
                x1: header.width / 3
                y1: 0
                x2: header.width
                y2: 1.3 * header.parent.height
                GradientStop {
                    position: 0.0
                    color: UIStyle.background1
                }
                GradientStop {
                    position: 0.5
                    color: UIStyle.background2
                }
                GradientStop {
                    position: 1.0
                    color: UIStyle.background3
                }
            }
        }
    }

    QQC2.AbstractButton {
        id: backButton
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.topMargin: 0
        anchors.leftMargin: 0

        width: height
        height: 40

        onClicked: header.backClicked()

        Image {
            width: 40
            height: 40
            anchors.centerIn: parent
            source: UIStyle.iconPath("back")
        }
    }

    QQC2.Label {
        id: labelText
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        color: UIStyle.textColor
        font: UIStyle.h2
    }

    transform: Translate {
        Behavior on y { NumberAnimation { } }
        y: header.enabled ? 0 : -52
    }


}
