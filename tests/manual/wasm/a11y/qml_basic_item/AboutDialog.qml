
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Dialog {
    id: aboutDialog
    width: 400
    height: 150
    title: "WebAssembly Dialog box"
    Accessible.role: Accessible.Dialog
    Accessible.name: title
    Accessible.description: "About DialogBox."
    modal: true
    Label {
        id: lblInfo
        anchors.centerIn: parent
        text: "Accessibility Demo sample application developed in QML."
        Accessible.role: Accessible.StaticText
        Accessible.name: text
        Accessible.description: "Purpose of this application."
    }

    Button {
        id: closeButton
        text: "Close"
        anchors {
            top: lblInfo.bottom
            topMargin: 10
            horizontalCenter: parent.horizontalCenter
        }

        Accessible.role: Accessible.Button
        Accessible.name: text
        Accessible.description: "To close the About Dialog box."
        onClicked: {
            aboutDialog.close()
        }
    }
}
