// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![entire]
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    visible: true

    //![palette]
    palette {
        buttonText: "red"
        button: "khaki"

        disabled {
            buttonText: "lavender"
            button: "coral"
        }
    }
    //![palette]

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: 3
        Button {
            text: qsTr("Disabled button")
            enabled: false
        }

        Button {
            text: qsTr("Enabled button")
        }

        TextField {
            Layout.fillWidth: true
            placeholderText: "type something here"
        }
    }
}
//![entire]
