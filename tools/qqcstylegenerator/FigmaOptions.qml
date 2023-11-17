// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

GridLayout {
    implicitWidth: parent.width
    implicitHeight: childrenRect.height
    columns: 2

    Label {
        text: "Target directory"
    }

    RowLayout {
        Layout.fillWidth: true
        TextField {
            id: stylePathInput
            placeholderText: "The path to where you wish to save the style"
            Layout.fillWidth: true
            text: bridge.targetDirectory
            onTextChanged: bridge.targetDirectory = text
        }

        ToolButton {
            icon.source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/folder-icon-round.png"
            onClicked: stylePathDialog.open()
        }

        FolderDialog {
            id: stylePathDialog
            onAccepted: stylePathInput.text = bridge.toLocalFile(selectedFolder)
        }
    }

    Label {
        text: "Figma URL (or file ID)"
    }

    TextField {
        placeholderText: "URL / file ID"
        Layout.fillWidth: true
        text: bridge.figmaUrlOrId
        onTextChanged: bridge.figmaUrlOrId = text
        ToolTip.visible: hovered
        ToolTip.delay: 1000
        ToolTip.text: "How to get the Figma file ID:"
                      + "\n1. Click the 'Share' button in the top right corner in Figma"
                      + "\n2. Click 'Copy link'"
    }

    Label {
        text: "Figma Token"
    }

    TextField {
        placeholderText: "Token"
        Layout.fillWidth: true
        text: bridge.figmaToken
        onTextChanged: bridge.figmaToken = text
        ToolTip.visible: hovered
        ToolTip.delay: 1000
        ToolTip.text: "How to get a Figma token:"
                      + "\n1. Login to your Figma account."
                      + "\n2. Head to Settings from the top-left menu inside Figma."
                      + "\n3. Find the Personal access tokens section."
                      + "\n4. Click Generate new token to open the configuration modal."
                      + "\n5. Click Generate token."
    }
}

