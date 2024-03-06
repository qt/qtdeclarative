// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Dialogs
import Qt.labs.platform as Platform // note: soon to be obsolete

// TODO:
// - make designer-friendly

ApplicationWindow {
    id: window
    width: 1024
    height: 600
    visible: true
    title: textArea.textDocument.source +
           " - Text Editor Example" + (textArea.textDocument.modified ? " *" : "")

    Component.onCompleted: {
        x = Screen.width / 2 - width / 2
        y = Screen.height / 2 - height / 2
    }

    Action {
        id: openAction
        shortcut: StandardKey.Open
        onTriggered: {
            if (textArea.textDocument.modified)
                discardDialog.open()
            else
                openDialog.open()
        }
    }

    Action {
        id: saveAction
        shortcut: StandardKey.Save
        enabled: textArea.textDocument.modified
        onTriggered: textArea.textDocument.save()
    }

    Action {
        id: saveAsAction
        shortcut: StandardKey.SaveAs
        onTriggered: saveDialog.open()
    }

    Action {
        id: quitAction
        shortcut: StandardKey.Quit
        onTriggered: close()
    }

    Action {
        id: copyAction
        shortcut: StandardKey.Copy
        enabled: textArea.selectedText
        onTriggered: textArea.copy()
    }

    Action {
        id: cutAction
        shortcut: StandardKey.Cut
        enabled: textArea.selectedText
        onTriggered: textArea.cut()
    }

    Action {
        id: pasteAction
        shortcut: StandardKey.Paste
        enabled: textArea.canPaste
        onTriggered: textArea.paste()
    }

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
        id: strikeoutAction
        checkable: true
        checked: textArea.cursorSelection.font.strikeout
        onTriggered: textArea.cursorSelection.font = Qt.font({ strikeout: checked })
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

    Platform.MenuBar {
        Platform.Menu {
            title: qsTr("&File")

            Platform.MenuItem {
                text: qsTr("&Open")
                onTriggered: openAction.trigger()
            }
            Platform.MenuItem {
                text: qsTr("&Save…")
                onTriggered: saveAction.trigger()
            }
            Platform.MenuItem {
                text: qsTr("Save &As…")
                onTriggered: saveAsAction.trigger()
            }
            Platform.MenuItem {
                text: qsTr("&Quit")
                onTriggered: quitAction.trigger()
            }
        }

        Platform.Menu {
            title: qsTr("&Edit")

            Platform.MenuItem {
                text: qsTr("&Copy")
                enabled: copyAction.enabled
                onTriggered: copyAction.trigger()
            }
            Platform.MenuItem {
                text: qsTr("Cu&t")
                enabled: cutAction.enabled
                onTriggered: cutAction.trigger()
            }
            Platform.MenuItem {
                text: qsTr("&Paste")
                enabled: pasteAction.enabled
                onTriggered: pasteAction.trigger()
            }
        }

        Platform.Menu {
            title: qsTr("F&ormat")

            Platform.MenuItem {
                text: qsTr("&Bold")
                checkable: true
                checked: boldAction.checked
                onTriggered: boldAction.trigger()
            }
            Platform.MenuItem {
                text: qsTr("&Italic")
                checkable: true
                checked: italicAction.checked
                onTriggered: italicAction.trigger()
            }
            Platform.MenuItem {
                text: qsTr("&Underline")
                checkable: true
                checked: underlineAction.checked
                onTriggered: underlineAction.trigger()
            }
            Platform.MenuItem {
                text: qsTr("&Strikeout")
                checkable: true
                checked: strikeoutAction.checked
                onTriggered: strikeoutAction.trigger()
            }

            Platform.MenuSeparator {}

            Platform.MenuItem {
                text: qsTr("Align &Left")
                checkable: true
                checked: alignLeftAction.checked
                onTriggered: alignLeftAction.trigger()
            }
            Platform.MenuItem {
                text: qsTr("&Center")
                checkable: true
                checked: alignCenterAction.checked
                onTriggered: alignCenterAction.trigger()
            }
            Platform.MenuItem {
                text: qsTr("&Justify")
                checkable: true
                checked: alignJustifyAction.checked
                onTriggered: alignJustifyAction.trigger()
            }
            Platform.MenuItem {
                text: qsTr("Align &Right")
                checkable: true
                checked: alignRightAction.checked
                onTriggered: alignRightAction.trigger()
            }
        }
    }

    FileDialog {
        id: openDialog
        fileMode: FileDialog.OpenFile
        selectedNameFilter.index: 1
        nameFilters: ["Text files (*.txt)", "HTML files (*.html *.htm)", "Markdown files (*.md *.markdown)"]
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: {
            textArea.textDocument.modified = false // we asked earlier, if necessary
            textArea.textDocument.source = selectedFile
        }
    }

    FileDialog {
        id: saveDialog
        fileMode: FileDialog.SaveFile
        nameFilters: openDialog.nameFilters
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: textArea.textDocument.saveAs(selectedFile)
    }

    FontDialog {
        id: fontDialog
        onAccepted: textArea.cursorSelection.font = selectedFont
    }

    ColorDialog {
        id: colorDialog
        selectedColor: "black"
        onAccepted: textArea.cursorSelection.color = selectedColor
    }

    MessageDialog {
        title: qsTr("Error")
        id: errorDialog
    }

    MessageDialog {
        id : quitDialog
        title: qsTr("Quit?")
        text: qsTr("The file has been modified. Quit anyway?")
        buttons: MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function (button, role) {
            if (role === MessageDialog.YesRole) {
                textArea.textDocument.modified = false
                Qt.quit()
            }
        }
    }

    MessageDialog {
        id : discardDialog
        title: qsTr("Discard changes?")
        text: qsTr("The file has been modified. Open a new file anyway?")
        buttons: MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function (button, role) {
            if (role === MessageDialog.YesRole)
                openDialog.open()
        }
    }

    header: ToolBar {
        leftPadding: 8

        Flow {
            id: flow
            width: parent.width

            Row {
                id: fileRow
                ToolButton {
                    id: openButton
                    text: "\uF115" // icon-folder-open-empty
                    font.family: "fontello"
                    action: openAction
                    focusPolicy: Qt.TabFocus
                }
                ToolButton {
                    id: saveButton
                    text: "\uE80A" // icon-floppy-disk
                    font.family: "fontello"
                    action: saveAction
                    focusPolicy: Qt.TabFocus
                }
                ToolSeparator {
                    contentItem.visible: fileRow.y === editRow.y
                }
            }

            Row {
                id: editRow
                ToolButton {
                    id: copyButton
                    text: "\uF0C5" // icon-docs
                    font.family: "fontello"
                    focusPolicy: Qt.TabFocus
                    action: copyAction
                }
                ToolButton {
                    id: cutButton
                    text: "\uE802" // icon-scissors
                    font.family: "fontello"
                    focusPolicy: Qt.TabFocus
                    action: cutAction
                }
                ToolButton {
                    id: pasteButton
                    text: "\uF0EA" // icon-paste
                    font.family: "fontello"
                    focusPolicy: Qt.TabFocus
                    action: pasteAction
                }
                ToolSeparator {
                    contentItem.visible: editRow.y === formatRow.y
                }
            }

            Row {
                id: formatRow
                ToolButton {
                    id: boldButton
                    text: "\uE800" // icon-bold
                    font.family: "fontello"
                    focusPolicy: Qt.TabFocus
                    action: boldAction
                }
                ToolButton {
                    id: italicButton
                    text: "\uE801" // icon-italic
                    font.family: "fontello"
                    focusPolicy: Qt.TabFocus
                    action: italicAction
                }
                ToolButton {
                    id: underlineButton
                    text: "\uF0CD" // icon-underline
                    font.family: "fontello"
                    focusPolicy: Qt.TabFocus
                    action: underlineAction
                }
                ToolButton {
                    id: strikeoutButton
                    text: "\uF0CC"
                    font.family: "fontello"
                    focusPolicy: Qt.TabFocus
                    action: strikeoutAction
                }
                ToolButton {
                    id: fontFamilyToolButton
                    text: qsTr("\uE808") // icon-font
                    font.family: "fontello"
                    font.bold: textArea.cursorSelection.font.bold
                    font.italic: textArea.cursorSelection.font.italic
                    font.underline: textArea.cursorSelection.font.underline
                    font.strikeout: textArea.cursorSelection.font.strikeout
                    focusPolicy: Qt.TabFocus
                    onClicked: function () {
                        fontDialog.selectedFont = textArea.cursorSelection.font
                        fontDialog.open()
                    }
                }
                ToolButton {
                    id: textColorButton
                    text: "\uF1FC" // icon-brush
                    font.family: "fontello"
                    focusPolicy: Qt.TabFocus
                    onClicked: function () {
                        colorDialog.selectedColor = textArea.cursorSelection.color
                        colorDialog.open()
                    }

                    Rectangle {
                        width: aFontMetrics.width + 3
                        height: 2
                        color: textArea.cursorSelection.color
                        parent: textColorButton.contentItem
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.baseline: parent.baseline
                        anchors.baselineOffset: 6

                        TextMetrics {
                            id: aFontMetrics
                            font: textColorButton.font
                            text: textColorButton.text
                        }
                    }
                }
                ToolSeparator {
                    contentItem.visible: formatRow.y === alignRow.y
                }
            }

            Row {
                id: alignRow
                ToolButton {
                    id: alignLeftButton
                    text: "\uE803" // icon-align-left
                    font.family: "fontello"
                    focusPolicy: Qt.TabFocus
                    action: alignLeftAction
                }
                ToolButton {
                    id: alignCenterButton
                    text: "\uE804" // icon-align-center
                    font.family: "fontello"
                    focusPolicy: Qt.TabFocus
                    action: alignCenterAction
                }
                ToolButton {
                    id: alignRightButton
                    text: "\uE805" // icon-align-right
                    font.family: "fontello"
                    focusPolicy: Qt.TabFocus
                    action: alignRightAction
                }
                ToolButton {
                    id: alignJustifyButton
                    text: "\uE806" // icon-align-justify
                    font.family: "fontello"
                    focusPolicy: Qt.TabFocus
                    action: alignJustifyAction
                }
            }
        }
    }

    Flickable {
        id: flickable
        flickableDirection: Flickable.VerticalFlick
        anchors.fill: parent

        ScrollBar.vertical: ScrollBar {}

        TextArea.flickable: TextArea {
            id: textArea
            textFormat: Qt.AutoText
            wrapMode: TextArea.Wrap
            focus: true
            selectByMouse: true
            persistentSelection: true
            // Different styles have different padding and background
            // decorations, but since this editor is almost taking up the
            // entire window, we don't need them.
            leftPadding: 6
            rightPadding: 6
            topPadding: 0
            bottomPadding: 0
            background: null

            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: contextMenu.open()
            }

            onLinkActivated: function (link) {
                Qt.openUrlExternally(link)
            }

            Component.onCompleted: {
                if (Qt.application.arguments.length === 2)
                    textDocument.source = "file:" + Qt.application.arguments[1]
                else
                    textDocument.source = "qrc:/texteditor.html"
            }

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
    }

    Platform.Menu {
        id: contextMenu

        Platform.MenuItem {
            text: qsTr("Copy")
            enabled: copyAction.enabled
            onTriggered: copyAction.trigger()
        }
        Platform.MenuItem {
            text: qsTr("Cut")
            enabled: cutAction.enabled
            onTriggered: cutAction.trigger()
        }
        Platform.MenuItem {
            text: qsTr("Paste")
            enabled: pasteAction.enabled
            onTriggered: pasteAction.trigger()
        }

        Platform.MenuSeparator {}

        Platform.MenuItem {
            text: qsTr("Font...")
            onTriggered: function () {
                fontDialog.selectedFont = textArea.cursorSelection.font
                fontDialog.open()
            }
        }

        Platform.MenuItem {
            text: qsTr("Color...")
            onTriggered: function () {
                colorDialog.selectedColor = textArea.cursorSelection.color
                colorDialog.open()
            }
        }
    }

    onClosing: function (close) {
        if (textArea.textDocument.modified) {
            quitDialog.open()
            close.accepted = false
        }
    }
}
