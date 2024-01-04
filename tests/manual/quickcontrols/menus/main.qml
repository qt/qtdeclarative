// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 800
    height: 600
    visible: true
    title: qsTr("Menus - style: %1").arg(currentStyle)

    required property string currentStyle

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }

    Settings {
        id: settings

        property alias windowX: window.x
        property alias windowY: window.y
        property alias windowWidth: window.width
        property alias windowHeight: window.height
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("Edit")

            Action {
                text: qsTr("Cut")
                enabled: textArea.selectedText.length > 0
            }
            Action {
                text: qsTr("Copy")
                enabled: textArea.selectedText.length > 0
            }
            Action {
                text: qsTr("Paste")
                enabled: textArea.activeFocus
            }

    //        MenuSeparator { }

    //        Menu {
    //            title: "Find/Replace"
    //            Action { text: "Find Next" }
    //            Action { text: "Find Previous" }
    //            Action { text: "Replace" }
    //        }
        }
    }

    ColumnLayout {
        anchors.fill: parent

        Label {
            text: qsTr("Right click on the window background to open a context menu. "
               + "Right click on the TextArea to access its edit context menu.\n\n"
               + "Things to check:\n\n"
               + "- Do the menu items trigger their actions (check console for output)?\n"
               + "- Do the Edit menu items (in the MenuBar menu and edit context menu)"
               + " work as expected with the TextArea?\n"
               + "  - Are they enabled/disabled as expected?\n"
               + "  - Does the TextArea keep focus after interacting with the Edit menu items?\n"
               + "- Does adding and removing menu items work?")
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap

            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: window.width * 0.5
            Layout.fillHeight: true
        }

        GroupBox {
            title: qsTr("Context menu")

            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent

                Button {
                    text: qsTr("Add action")
                    onClicked: backgroundContextMenu.appendAction()
                }
                Button {
                    text: qsTr("Remove action")
                    onClicked: backgroundContextMenu.removeLastAction()
                }

                Button {
                    text: qsTr("Add sub-menu action")
                    onClicked: subMenu.appendAction()
                }
                Button {
                    text: qsTr("Remove sub-menu action")
                    onClicked: subMenu.removeLastAction()
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }

        TextArea {
            id: textArea
            text: qsTr("Dummy TextArea to test disabled menu items")

            Layout.fillWidth: true
            Layout.minimumHeight: 100

            TapHandler {
                objectName: "textAreaTapHandler"
                acceptedButtons: Qt.RightButton
                onTapped: editContextMenu.open()
            }
        }
    }

    TapHandler {
        objectName: "backgroundTapHandler"
        acceptedButtons: Qt.RightButton
        onTapped: backgroundContextMenu.open()
    }

    Component {
        id: actionComponent

        Action {}
    }

    component ContextAction: Action {
        onTriggered: print("triggered", text)
    }

    Menu {
        id: backgroundContextMenu

        function appendAction() {
            let action = actionComponent.createObject(null, { text: qsTr("Extra context menu item") })
            backgroundContextMenu.addAction(action)
        }

        function removeLastAction() {
            // TODO: Can't use count here because it's 0: it uses contentModel->count(), but native menu items
            // are not Qt Quick items, so we either need to document that you should use contentData.count
            // or add an "actions" property. The problem with contentData is that it could contain
            // non-Action objects. Another potential issue is that "It is not re-ordered when items are inserted or moved",
            // making it unreliable as a general purpose container of actions if users add or remove them dynamically.
            backgroundContextMenu.removeAction(backgroundContextMenu.actionAt(backgroundContextMenu.contentData.length - 1))
        }

        ContextAction {
            text: qsTr("Context menu item 1")
        }
        ContextAction {
            text: qsTr("Context menu item 2")
        }
        ContextAction {
            text: qsTr("Context menu item 3")
        }

        // TODO: separator

        Menu {
            id: subMenu
            title: qsTr("Sub-menu")

            function appendAction() {
                let action = actionComponent.createObject(null, { text: qsTr("Extra sub-menu item") })
                subMenu.addAction(action)
            }

            function removeLastAction() {
                subMenu.removeAction(subMenu.actionAt(subMenu.contentData.length - 1))
            }

            ContextAction {
                text: qsTr("Sub-menu item 1")
            }
            ContextAction {
                text: qsTr("Sub-menu item 2")
            }
            ContextAction {
                text: qsTr("Sub-menu item 3")
            }
        }
    }

    Menu {
        id: editContextMenu

        ContextAction {
            text: qsTr("Cut")
            enabled: textArea.selectedText.length > 0
        }
        ContextAction {
            text: qsTr("Copy")
            enabled: textArea.selectedText.length > 0
        }
        ContextAction {
            text: qsTr("Paste")
            enabled: textArea.activeFocus
        }
    }
}

