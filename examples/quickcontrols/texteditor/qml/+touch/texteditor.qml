// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// TODO:
// - make designer-friendly

ApplicationWindow {
    id: window
    width: 480
    height: 640
    visible: true
    title: textArea.textDocument.source +
           " - Text Editor Example" + (textArea.textDocument.modified ? " *" : "")

    Action {
        id: boldAction
        shortcut: StandardKey.Bold
        checkable: true
        checked: textArea.cursorSelection.font.bold
        onTriggered: textArea.cursorSelection.font = Qt.font({ bold: checked })
    }

    Action {
        id: italicAction
        shortcut: StandardKey.Italic
        checkable: true
        checked: textArea.cursorSelection.font.italic
        onTriggered: textArea.cursorSelection.font = Qt.font({ italic: checked })
    }

    Action {
        id: underlineAction
        shortcut: StandardKey.Underline
        checkable: true
        checked: textArea.cursorSelection.font.underline
        onTriggered: textArea.cursorSelection.font = Qt.font({ underline: checked })
    }

    Action {
        id: alignLeftAction
        shortcut: "Ctrl+{"
        checkable: true
        checked: textArea.cursorSelection.alignment === Qt.AlignLeft
        onTriggered: textArea.cursorSelection.alignment = Qt.AlignLeft
    }

    Action {
        id: alignCenterAction
        shortcut: "Ctrl+|"
        checkable: true
        checked: textArea.cursorSelection.alignment === Qt.AlignCenter
        onTriggered: textArea.cursorSelection.alignment = Qt.AlignCenter
    }

    Action {
        id: alignRightAction
        shortcut: "Ctrl+}"
        checkable: true
        checked: textArea.cursorSelection.alignment === Qt.AlignRight
        onTriggered: textArea.cursorSelection.alignment = Qt.AlignRight
    }

    Action {
        id: alignJustifyAction
        shortcut: "Ctrl+Alt+}"
        checkable: true
        checked: textArea.cursorSelection.alignment === Qt.AlignJustify
        onTriggered: textArea.cursorSelection.alignment = Qt.AlignJustify
    }

    header: ToolBar {
        leftPadding: 5

        RowLayout {
            anchors.fill: parent
            spacing: 0

            ToolButton {
                id: doneEditingButton
                font.family: "fontello"
                text: "\uE809" // icon-ok
                opacity: !textArea.readOnly ? 1 : 0
                onClicked: textArea.readOnly = true
                Layout.fillWidth: false
            }

            Label {
                text: qsTr("Text Editor Example")
                font.bold: true
                font.pixelSize: 20
                elide: Label.ElideRight
                Layout.fillWidth: true
            }

            ToolButton {
                font.family: "fontello"
                text: "\uF142" // icon-ellipsis-vert
                onClicked: menu.open()
                Layout.fillWidth: false

                Menu {
                    id: menu

                    MenuItem {
                        text: qsTr("About")
                        onTriggered: aboutDialog.open()
                    }
                }
            }
        }
    }

    Flickable {
        id: flickable
        flickableDirection: Flickable.VerticalFlick
        anchors.fill: parent

        TextArea.flickable: TextArea {
            id: textArea
            textFormat: Qt.RichText
            wrapMode: TextArea.Wrap
            readOnly: true
            persistentSelection: true
            // Different styles have different padding and background
            // decorations, but since this editor is almost taking up the
            // entire window, we don't need them.
            leftPadding: 6
            rightPadding: 6
            topPadding: 0
            bottomPadding: 0
            background: null

            onLinkActivated: Qt.openUrlExternally(link)

            Component.onCompleted: textDocument.source = "qrc:/texteditor.html"

            textDocument.onStatusChanged: {
                // a message lookup table using computed properties:
                // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Object_initializer
                const statusMessages = {
                    [ TextDocument.ReadError ]: qsTr("Failed to load “%1”"),
                    [ TextDocument.WriteError ]: qsTr("Failed to save “%1”"),
                    [ TextDocument.NonLocalFileError ]: qsTr("Not a local file: “%1”"),
                }
                const err = statusMessages[textDocument.status]
                if (err) {
                    errorDialog.text = err.arg(textDocument.source)
                    errorDialog.open()
                }
            }
        }

        ScrollBar.vertical: ScrollBar {}
    }

    footer: ToolBar {
        visible: !textArea.readOnly && textArea.activeFocus

        Flickable {
            anchors.fill: parent
            contentWidth: toolRow.implicitWidth
            flickableDirection: Qt.Horizontal
            boundsBehavior: Flickable.StopAtBounds

            Row {
                id: toolRow

                ToolButton {
                    id: boldButton
                    text: "\uE800" // icon-bold
                    font.family: "fontello"
                    // Don't want to close the virtual keyboard when this is clicked.
                    focusPolicy: Qt.NoFocus
                    action: boldAction
                }
                ToolButton {
                    id: italicButton
                    text: "\uE801" // icon-italic
                    font.family: "fontello"
                    focusPolicy: Qt.NoFocus
                    action: italicAction
                }
                ToolButton {
                    id: underlineButton
                    text: "\uF0CD" // icon-underline
                    font.family: "fontello"
                    focusPolicy: Qt.NoFocus
                    action: underlineAction
                }

                ToolSeparator {}

                ToolButton {
                    id: alignLeftButton
                    text: "\uE803" // icon-align-left
                    font.family: "fontello"
                    focusPolicy: Qt.NoFocus
                    action: alignLeftAction
                }
                ToolButton {
                    id: alignCenterButton
                    text: "\uE804" // icon-align-center
                    font.family: "fontello"
                    focusPolicy: Qt.NoFocus
                    action: alignCenterAction
                }
                ToolButton {
                    id: alignRightButton
                    text: "\uE805" // icon-align-right
                    font.family: "fontello"
                    focusPolicy: Qt.NoFocus
                    action: alignRightAction
                }
                ToolButton {
                    id: alignJustifyButton
                    text: "\uE806" // icon-align-justify
                    font.family: "fontello"
                    focusPolicy: Qt.NoFocus
                    action: alignJustifyAction
                }
            }
        }
    }

    RoundButton {
        id: editButton
        font.family: "fontello"
        text: "\uE809" // icon-pencil
        width: 48
        height: width
        // Don't want to use anchors for the y position, because it will anchor
        // to the footer, leaving a large vertical gap.
        y: parent.height - height - 12
        anchors.right: parent.right
        anchors.margins: 12
        visible: textArea.readOnly
        highlighted: true

        onClicked: {
            textArea.readOnly = false
            // Force focus on the text area so the cursor and footer show up.
            textArea.forceActiveFocus()
        }
    }

    Dialog {
        id: aboutDialog
        standardButtons: Dialog.Ok
        modal: true
        x: parent.width / 2 - width / 2
        y: parent.height / 2 - height / 2

        contentItem: Label {
            text: qsTr("Qt Quick Controls - Text Editor Example")
        }
    }
}
