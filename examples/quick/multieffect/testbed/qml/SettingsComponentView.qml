// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Column {
    id: rootItem

    property alias text: textItem.text
    property bool show: true

    default property alias contents: contentItem.children
    property real showHideAnimationSpeed: 400

    width: settings.settingsViewWidth

    Component.onCompleted: {
        // Set initial open state
        contentItem.visible = rootItem.show;
        contentItem.opacity = rootItem.show;
        contentItemArea.height = rootItem.show ? contentItem.height : 0;
    }

    Item {
        id: lightsSettings
        width: parent.width
        height: 30
        Rectangle {
            anchors.fill: parent
            color: "#404040"
            border.width: 1
            border.color: "#808080"
            opacity: 0.4
        }
        Image {
            x: 8
            source: "images/arrow.png"
            anchors.verticalCenter: parent.verticalCenter
            rotation: rootItem.show ? 90 : 0
            Behavior on rotation {
                NumberAnimation {
                    duration: showHideAnimationSpeed
                    easing.type: Easing.InOutQuad
                }
            }
        }

        Text {
            id: textItem
            x: 30
            anchors.verticalCenter: parent.verticalCenter
            color: "#f0f0f0"
            font.bold: true
            font.pixelSize: 16 * dp
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                rootItem.show = !rootItem.show;
                if (rootItem.show) {
                    hideAnimation.stop();
                    showAnimation.start();
                } else {
                    showAnimation.stop();
                    hideAnimation.start();
                }

            }
        }
    }

    Item {
        width: 1
        height: 5
    }

    SequentialAnimation {
        id: showAnimation
        ScriptAction {
            script: contentItem.visible = true;
        }
        ParallelAnimation {
            NumberAnimation {
                target: contentItemArea
                property: "height"
                to: contentItem.height
                duration: showHideAnimationSpeed
                easing.type: Easing.InOutQuad
            }
            SequentialAnimation {
                PauseAnimation {
                    duration: showHideAnimationSpeed / 2
                }
                NumberAnimation {
                    target: contentItem
                    property: "opacity"
                    to: 1.0
                    duration: showHideAnimationSpeed / 2
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }

    SequentialAnimation {
        id: hideAnimation
        ParallelAnimation {
            NumberAnimation {
                target: contentItemArea
                property: "height"
                to: 0
                duration: showHideAnimationSpeed
                easing.type: Easing.InOutQuad
            }
            SequentialAnimation {
                NumberAnimation {
                    target: contentItem
                    property: "opacity"
                    to: 0
                    duration: showHideAnimationSpeed / 2
                    easing.type: Easing.InOutQuad
                }
                PauseAnimation {
                    duration: showHideAnimationSpeed / 2
                }
            }
        }
        ScriptAction {
            script: contentItem.visible = false;
        }
    }

    Item {
        id: contentItemArea
        width: parent.width - 10
        x: 5
        Column {
            id: contentItem
            spacing: -10
        }
    }

    Item {
        width: 1
        height: 5
    }
}
