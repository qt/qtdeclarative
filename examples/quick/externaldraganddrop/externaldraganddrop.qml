// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    width: 320
    height: 480

    ColumnLayout {

        anchors.fill: parent
        anchors.margins: 8

        Label {
            Layout.fillWidth: true
            text: "Drag text into, out of, and between the boxes below."
            wrapMode: Text.WordWrap
        }

        DragAndDropTextItem {
            Layout.fillWidth: true
            Layout.fillHeight: true
            display: "Sample Text"
        }

        DragAndDropTextItem {
            Layout.fillWidth: true
            Layout.fillHeight: true
            display: "Option/ctrl drag to copy instead of move text."
        }

        DragAndDropTextItem {
            Layout.fillWidth: true
            Layout.fillHeight: true
            dropEnabled: false
            display: "Drag out into other applications."
        }
    }
}
