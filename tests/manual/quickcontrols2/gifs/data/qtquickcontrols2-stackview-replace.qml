// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

ApplicationWindow {
    width: 160
    height: 160
    visible: true
    color: "#eeeeee"

    property int maxDepth: 3

    function itemText(index) {
        return String.fromCharCode(65 + index);
    }

    Component {
        id: labelComponent

        Label {
            font.pixelSize: 60
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent

        Component.onCompleted: {
            for (var i = 0; i < maxDepth; ++i) {
                stackView.push(labelComponent, { text: itemText(i) }, StackView.Immediate);
            }
        }
    }

    Label {
        id: operationLabel
        text: "replace(D)"
        font.pixelSize: 16
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 10
    }

    Timer {
        id: operationTimer
        running: true
        interval: 1500
        onTriggered: {
            stackView.replace(labelComponent, { text: "D" });
            hideOperationTimer.start();
        }
    }

    Timer {
        id: hideOperationTimer
        interval: operationTimer.interval
        onTriggered: operationLabel.visible = false
    }
}
