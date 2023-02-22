// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ScrollablePage {
    id: page

    readonly property int buttonWidth: Math.max(button.implicitWidth, Math.min(button.implicitWidth * 2, page.availableWidth / 3))

    Column {
        spacing: 40
        width: parent.width

        Label {
            width: parent.width
            wrapMode: Label.Wrap
            horizontalAlignment: Qt.AlignHCenter
            text: "Dialog is a popup that is mostly used for short-term tasks "
                + "and brief communications with the user."
        }

        Button {
            text: "Message"
            anchors.horizontalCenter: parent.horizontalCenter
            width: page.buttonWidth
            onClicked: messageDialog.open()

            Dialog {
                id: messageDialog

                x: (parent.width - width) / 2
                y: (parent.height - height) / 2

                title: "Message"

                Label {
                    text: "Lorem ipsum dolor sit amet..."
                }
            }
        }

        Button {
            id: button
            text: "Confirmation"
            anchors.horizontalCenter: parent.horizontalCenter
            width: page.buttonWidth
            onClicked: confirmationDialog.open()

            Dialog {
                id: confirmationDialog

                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                parent: Overlay.overlay

                modal: true
                title: "Confirmation"
                standardButtons: Dialog.Yes | Dialog.No

                Column {
                    spacing: 20
                    anchors.fill: parent
                    Label {
                        text: "The document has been modified.\nDo you want to save your changes?"
                    }
                    CheckBox {
                        text: "Do not ask again"
                        anchors.right: parent.right
                    }
                }
            }
        }

        Button {
            text: "Content"
            anchors.horizontalCenter: parent.horizontalCenter
            width: page.buttonWidth
            onClicked: contentDialog.open()

            Dialog {
                id: contentDialog

                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                width: Math.min(page.width, page.height) / 3 * 2
                contentHeight: logo.height * 2
                parent: Overlay.overlay

                modal: true
                title: "Content"
                standardButtons: Dialog.Close

                Flickable {
                    id: flickable
                    clip: true
                    anchors.fill: parent
                    contentHeight: column.height

                    Column {
                        id: column
                        spacing: 20
                        width: parent.width

                        Image {
                            id: logo
                            width: parent.width / 2
                            anchors.horizontalCenter: parent.horizontalCenter
                            fillMode: Image.PreserveAspectFit
                            source: "../images/qt-logo.png"
                        }

                        Label {
                            width: parent.width
                            text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc finibus "
                                + "in est quis laoreet. Interdum et malesuada fames ac ante ipsum primis "
                                + "in faucibus. Curabitur eget justo sollicitudin enim faucibus bibendum. "
                                + "Suspendisse potenti. Vestibulum cursus consequat mauris id sollicitudin. "
                                + "Duis facilisis hendrerit consectetur. Curabitur sapien tortor, efficitur "
                                + "id auctor nec, efficitur et nisl. Ut venenatis eros in nunc placerat, "
                                + "eu aliquam enim suscipit."
                            wrapMode: Label.Wrap
                        }
                    }

                    ScrollIndicator.vertical: ScrollIndicator {
                        parent: contentDialog.contentItem
                        anchors.top: flickable.top
                        anchors.bottom: flickable.bottom
                        anchors.right: parent.right
                        anchors.rightMargin: -contentDialog.rightPadding + 1
                    }
                }
            }
        }

        Button {
            text: "Input"
            anchors.horizontalCenter: parent.horizontalCenter
            width: page.buttonWidth
            onClicked: inputDialog.open()

            Dialog {
                id: inputDialog

                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                parent: Overlay.overlay

                focus: true
                modal: true
                title: "Input"
                standardButtons: Dialog.Ok | Dialog.Cancel

                ColumnLayout {
                    spacing: 20
                    anchors.fill: parent
                    Label {
                        elide: Label.ElideRight
                        text: "Please enter the credentials:"
                        Layout.fillWidth: true
                    }
                    TextField {
                        focus: true
                        placeholderText: "Username"
                        Layout.fillWidth: true
                    }
                    TextField {
                        placeholderText: "Password"
                        echoMode: TextField.PasswordEchoOnEdit
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
