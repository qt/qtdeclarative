// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import Qt.labs.qmlmodels
import StorageModel

Window {
    id: window
    width: 480
    height: 300
    visible: true
    color: "darkgray"
    title: "Storage Volumes"

    TableView {
        id: table
        anchors.fill: parent
        anchors.margins: 10
        clip: true
        model: StorageModel { }
        columnSpacing: 1
        rowSpacing: 1
        resizableColumns: true
        delegate: DelegateChooser {
            role: "type"
            DelegateChoice {
                roleValue: "Value"
                delegate: Rectangle {
                    color: "tomato"
                    implicitWidth: Math.max(100, label.implicitWidth + 8)
                    implicitHeight: label.implicitHeight + 4

                    Rectangle {
                        x: parent.width - width
                        width: value * parent.width / valueMax
                        height: parent.height
                        color: "white"
                    }

                    Text {
                        id: label
                        anchors.baseline: parent.bottom
                        anchors.baselineOffset: -4
                        anchors.left: parent.left
                        anchors.leftMargin: 4
                        text: valueDisplay + " of " + valueMaxDisplay + " " + heading
                    }
                }
            }
            DelegateChoice {
                roleValue: "Flag"
                // We could use a checkbox here but that would be another component (e.g. from Controls)
                delegate: Rectangle {
                    implicitWidth: checkBox.implicitWidth + 8
                    implicitHeight: checkBox.implicitHeight + 4
                    Text {
                        id: checkBox
                        anchors.baseline: parent.bottom
                        anchors.baselineOffset: -4
                        anchors.left: parent.left
                        anchors.leftMargin: 4
                        text: (checkState ? "☑ " : "☐ ") + heading
                    }
                }
            }
            DelegateChoice {
                // roleValue: "String" // default delegate
                delegate: Rectangle {
                    implicitWidth: stringLabel.implicitWidth + 8
                    implicitHeight: stringLabel.implicitHeight + 4
                    Text {
                        id: stringLabel
                        anchors.baseline: parent.bottom
                        anchors.baselineOffset: -4
                        anchors.left: parent.left
                        anchors.leftMargin: 4
                        text: display
                    }
                }
            }
        }
    }
}
