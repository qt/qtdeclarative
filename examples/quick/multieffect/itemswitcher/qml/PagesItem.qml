// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: rootItem

    property Item source
    property bool selected: false
    property string text

    signal clicked

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        border.color: "#f0f0f0"
        opacity: rootItem.selected ? 0.4 : 0
        Behavior on opacity {
            NumberAnimation {
                duration: 400
                easing.type: Easing.InOutQuad
            }
        }
    }

    ShaderEffectSource {
        anchors.fill: parent
        anchors.margins: 10
        sourceItem: rootItem.source
        smooth: true
        mipmap: true
    }
    Text {
        anchors.centerIn: parent
        visible: rootItem.text != ""
        text: rootItem.text
        font.pixelSize: 14 * dp
        color: "#ffffff"
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            rootItem.clicked();
        }
    }
}
