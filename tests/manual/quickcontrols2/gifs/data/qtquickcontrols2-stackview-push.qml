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

    property int itemIndex: 0
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
    }

    Label {
        id: operationLabel
        text: "push(" + itemText(Math.max(0, Math.min(maxDepth - 1, itemIndex - 1))) + ")"
        font.pixelSize: 16
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 10
    }

    Timer {
        id: operationTimer
        running: true
        interval: 1500
        repeat: stackView.depth < maxDepth - 1
        onRepeatChanged: if (!repeat) hideOperationTimer.start()

        onTriggered: stackView.push(labelComponent, { text: itemText(itemIndex++) })
    }

    Timer {
        id: hideOperationTimer
        interval: operationTimer.interval * 2
        onTriggered: operationLabel.visible = false
    }
}
