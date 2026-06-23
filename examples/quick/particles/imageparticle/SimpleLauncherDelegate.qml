// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ItemDelegate {
    id: root

    required property var modelData

    width: ListView.view.width

    contentItem: RowLayout {
        spacing: 12
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Label {
                text: root.modelData.name
                font.pixelSize: 18
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Label {
                text: root.modelData.description
                font.pixelSize: 12
                opacity: 0.6
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }
        Image {
            source: "images/back.png"
            mirror: true
            opacity: 0.4
            sourceSize.width: 20
            sourceSize.height: 20
            fillMode: Image.PreserveAspectFit
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: root.palette.mid
    }
}
