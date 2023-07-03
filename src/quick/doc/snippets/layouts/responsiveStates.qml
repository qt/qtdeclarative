// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Window

Window {
    visible: true
    width: 350
    height: 250
    //! [document]
    GridLayout {
        anchors.fill: parent

        Rectangle {
            id: rectangle1
            color: "tomato"
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        Rectangle {
            id: rectangle2
            color: "lightskyblue"
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        states: [
            State {
                when: width < 300
                PropertyChanges { target: rectangle2; Layout.row: 1 }
                PropertyChanges { target: rectangle2; Layout.column: 0 }
            },
            State {
                when: width >= 300
                PropertyChanges { target: rectangle2; Layout.row: 0 }
                PropertyChanges { target: rectangle2; Layout.column: 1 }
            }
        ]
    }
    //! [document]
}
