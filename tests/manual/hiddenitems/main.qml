// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import HiddenItems

Rectangle {
    id: root
    width: 1280
    height: 800
    color: "white"

    property int cellCount: 3000
    property int toggledCount: 40
    property bool clipPanel: true
    property bool useEffectSource: false

    // Static content. It never changes, but every full batch rebuild in the
    // renderer has to go through all of it again.
    Grid {
        anchors.fill: parent
        columns: Math.floor(width / 20)
        Repeater {
            model: root.cellCount
            Rectangle {
                required property int index
                width: 20
                height: 14
                color: Qt.hsla((index % 97) / 97, 0.5, 0.75, 1)
                border.color: "gray"
                Text {
                    anchors.centerIn: parent
                    text: parent.index % 100
                    font.pixelSize: 8
                }
            }
        }
    }

    // The items that get hidden and shown, inside a view-like clipped panel,
    // similar to delegates in a ListView or pages in a SwipeView.
    Rectangle {
        x: 40
        y: 40
        width: 600
        height: 400
        color: "#c0e0ff"
        border.color: "black"
        clip: root.clipPanel

        Flow {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 4
            Repeater {
                model: root.toggledCount
                Item {
                    id: delegate
                    required property int index
                    width: 64
                    height: 40

                    Rectangle {
                        id: toggled
                        anchors.fill: parent
                        color: "steelblue"
                        Text {
                            anchors.centerIn: parent
                            text: "Item " + delegate.index
                            color: "white"
                        }
                    }

                    // Only references the item in hidesource mode; zero-sized
                    // so that it does not render anything itself.
                    ShaderEffectSource {
                        id: effectSource
                        width: 0
                        height: 0
                        sourceItem: root.useEffectSource ? toggled : null
                    }

                    OpacityAnimator {
                        id: animator
                        target: toggled
                        from: 0.5
                        to: 1
                        duration: 1000000
                    }

                    Component.onCompleted: Benchmark.addToggledItem(toggled, effectSource, animator)
                }
            }
        }
    }
}
