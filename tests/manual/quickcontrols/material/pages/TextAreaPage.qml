// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import ".."

Page {
    id: root
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

        Flickable {
            width: 200
            height: 100

            TextArea.flickable: TextArea {
                placeholderText: "placeholderText"
                text: "a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk\nl\nm\nn"

                Material.containerStyle: layout.containerStyle
            }
        }

        TextArea {
            text: "AlignLeft"
            placeholderText: "AlignLeft"
            horizontalAlignment: TextArea.AlignLeft
            // Make it a big bigger so it's easier to see the effects of alignment.
            width: implicitWidth + 30

            Material.containerStyle: layout.containerStyle
        }

        TextArea {
            text: "AlignHCenter"
            placeholderText: "AlignMiddle"
            horizontalAlignment: TextArea.AlignMiddle
            width: implicitWidth + 30

            Material.containerStyle: layout.containerStyle
        }

        TextArea {
            text: "AlignRight"
            placeholderText: "AlignRight"
            horizontalAlignment: TextArea.AlignRight
            width: implicitWidth + 30

            Material.containerStyle: layout.containerStyle
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent

        ColumnLayout {
            width: scrollView.availableWidth

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
}
