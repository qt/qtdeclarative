// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 300
    height: 200
    visible: true

    ColumnLayout {
        anchors.fill: parent
        TextField {
            id: singleline
            text: "Initial Text"
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.margins: 5
            background: Rectangle {
               implicitWidth: 200
               implicitHeight: 40
               border.color: singleline.focus ? "#21be2b" : "lightgray"
               color: singleline.focus ? "lightgray" : "transparent"
            }
        }

        TextArea {
            id: multiline
            placeholderText: "Initial text\n...\n...\n"
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 5
            background: Rectangle {
               implicitWidth: 200
               implicitHeight: 100
               border.color: multiline.focus ? "#21be2b" : "lightgray"
               color: multiline.focus ? "lightgray" : "transparent"
            }
        }
    }
}
//![0]
