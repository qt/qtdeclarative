// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Simulates ApplicationFlowForm.ui.qml: a form with internal IDs, aliases,
// StackView with anchors, and Component objects.

import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: root
    width: 400
    height: 600
    property string coffeeName: ""
    property alias toolbar: toolbar
    property alias contentText: contentText
    property alias stack: stack
    property alias choosingCoffee: choosingCoffee

    Item {
        id: toolbar
        objectName: "toolbar"
        height: 35
        anchors.topMargin: parent.height / 80
        width: parent.width
        anchors.top: parent.top
        property real backOpacity: 1
        property bool backEnabled: true
    }
    Text {
        id: contentText
        text: "Coffee Selection"
        font.pixelSize: 24
        anchors.top: toolbar.bottom
        anchors.topMargin: parent.height / 20
    }
    StackView {
        id: stack
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: parent.height / 20
    }

    Component {
        id: choosingCoffee
        Item {
            visible: true
        }
    }
}
