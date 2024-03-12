// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import ".."

Page {
    topPadding: Constants.pageTopPadding

    component TextFieldFlow: Flow {
        id: layout
        spacing: 40

        required property int containerStyle

        TextField {
            Material.containerStyle: layout.containerStyle
        }

        TextField {
            placeholderText: "placeholderText"

            Material.containerStyle: layout.containerStyle
        }

        TextField {
            text: "text"

            Material.containerStyle: layout.containerStyle
        }

        TextField {
            text: "text"
            placeholderText: "placeholderText"

            Material.containerStyle: layout.containerStyle
        }

        TextField {
            placeholderText: "Disabled placeholder"
            enabled: false

            Material.containerStyle: layout.containerStyle
        }

        TextField {
            text: "Disabled text"
            enabled: false

            Material.containerStyle: layout.containerStyle
        }

        TextField {
            text: "text"
            placeholderText: "placeholderText"
            enabled: false

            Material.containerStyle: layout.containerStyle
        }

        TextField {
            text: "0 leftPadding"
            placeholderText: "placeholderText"
            leftPadding: 0

            Material.containerStyle: layout.containerStyle
        }

        TextField {
            text: "0 rightPadding"
            placeholderText: "placeholderText"
            rightPadding: 0

            Material.containerStyle: layout.containerStyle
        }

        TextField {
            text: "0 topPadding"
            placeholderText: "placeholderText"
            topPadding: 0

            Material.containerStyle: layout.containerStyle
        }

        TextField {
            text: "0 bottomPadding"
            placeholderText: "placeholderText"
            bottomPadding: 0

            Material.containerStyle: layout.containerStyle
        }
    }

    ColumnLayout {
        width: parent.width

        Label {
            text: "Filled"
        }
        TextFieldFlow {
            containerStyle: Material.Filled

            Layout.fillWidth: true
            Layout.bottomMargin: 40
        }

        Label {
            text: "Outlined"
        }
        TextFieldFlow {
            containerStyle: Material.Outlined

            Layout.fillWidth: true
        }
    }
}
