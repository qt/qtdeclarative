// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import ".."

Page {
    topPadding: Constants.pageTopPadding

//    Component.onCompleted: Material.background = "red"

    component TextAreaFlow: Flow {
        id: layout
        spacing: 40

        required property int containerStyle

        TextArea {
            Material.containerStyle: layout.containerStyle
        }

        TextArea {
            placeholderText: "placeholderText"

            Material.containerStyle: layout.containerStyle
        }

        TextArea {
            text: "text"

            Material.containerStyle: layout.containerStyle
        }

        TextArea {
            text: "text\nmore text"

            Material.containerStyle: layout.containerStyle
        }

        TextArea {
            text: "text"
            placeholderText: "placeholderText"

            Material.containerStyle: layout.containerStyle
        }

        TextArea {
            text: "text\nmore text"
            placeholderText: "placeholderText"

            Material.containerStyle: layout.containerStyle
        }

        TextArea {
            placeholderText: "Disabled placeholder"
            enabled: false

            Material.containerStyle: layout.containerStyle
        }

        TextArea {
            text: "Disabled text"
            enabled: false

            Material.containerStyle: layout.containerStyle
        }

        TextArea {
            text: "Disabled text\nMore text"
            enabled: false

            Material.containerStyle: layout.containerStyle
        }

        TextArea {
            text: "text"
            placeholderText: "placeholderText"
            enabled: false

            Material.containerStyle: layout.containerStyle
        }

        TextArea {
            text: "text\nmore text"
            placeholderText: "placeholderText"
            enabled: false

            Material.containerStyle: layout.containerStyle
        }
    }

    ColumnLayout {
        width: parent.width

        Label {
            text: "Filled"
        }
        TextAreaFlow {
            containerStyle: Material.Filled

            Layout.fillWidth: true
            Layout.bottomMargin: 40
        }

        Label {
            text: "Outlined"
        }
        TextAreaFlow {
            containerStyle: Material.Outlined

            Layout.fillWidth: true
        }
    }
}
