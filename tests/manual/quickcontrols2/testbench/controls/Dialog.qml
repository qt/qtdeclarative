// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// TODO
QtObject {
    property string customControlName: qsTr("Dialog")

    property var supportedStates: [
        [],
        ["modal"],
        ["dim"]
    ]

    property Component component: Button {
        id: dialogButton
        text: qsTr("Dialog")
        hoverEnabled: true

        onClicked: dialog.open()

        Dialog {
            id: dialog
            x: (window.width - width) / 2
            y: (window.height - height) / 2
            standardButtons: Dialog.Ok | Dialog.Cancel
            parent: window.contentItem
            modal: is("modal")
            dim: is("dim") || is("modal")

            Label {
                text: "Lorem ipsum dolor sit amet, \nconsectetuer adipiscing elit, \n"
                    + "sed diam nonummy nibh euismod tincidunt ut \nlaoreet dolore magna aliquam erat volutpat."
            }
        }
    }
}
