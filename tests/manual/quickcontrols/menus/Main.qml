// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

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
    }

    menuBar: MenuBar {
        visible: menuBarVisibleSwitch.checked

        Menu {
            id: fileMenu
            objectName: "file"
            title: qsTr("&File")
            popupType: popupTypeCombo.popupType()
            ContextAction { text: qsTr("&New...") }
            ContextMenuItem { text: "menuItem" }
            ContextAction { text: qsTr("&Open...") }
            ContextAction { text: qsTr("&Save") }
            ContextAction { text: qsTr("Save &As...") }
            Menu {
                title: qsTr("Sub...")
                ContextAction { text: qsTr("Sub action 1") }
                ContextAction { text: qsTr("Sub action 2") }
                Menu {
                    title: qsTr("SubSub...")
                    ContextAction { text: qsTr("SubSub action 1") }
                    ContextAction { text: qsTr("SubSub action 2") }
                }
            }
            MenuSeparator { }
            ContextAction {
                text: qsTr("&Quit")
                // This is needed for macOS since it takes priority over the Shortcut.
                onTriggered: Qt.quit()
            }
            Action {
                text: qsTr("Remove menu")
                onTriggered: menuBar.removeMenu(fileMenu)
            }
        }
        Menu {
            id: editMenu
            objectName: "edit"
            title: qsTr("&Edit")
            popupType: popupTypeCombo.popupType()
            ContextAction {
                id: cutAction
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

            MenuSeparator {}

            Action {
                text: qsTr("Checkable menu")
                checkable: true
                checked: true
            }
            Action {
                text: qsTr("Remove menu")
                onTriggered: menuBar.removeMenu(editMenu)
            }
            Menu {
                id: editSubMenu
                title: qsTr("Find / Replace")
                Action { text: qsTr("&Find") }
            }

            MenuSeparator {}

            ContextAction {
                text: qsTr("Dummy Action")
                shortcut: "Ctrl+I"
            }
        }
        MenuBarItem {
            id: explicitMenuBarItem
            menu: Menu {
                id: menuBarItemMenu
                objectName: "MenuBarItem"
                title: "MenuBarItem"
                popupType: popupTypeCombo.popupType()
                ContextAction { text: qsTr("Action") }
                Action {
                    text: qsTr("Remove menu")
                    onTriggered: menuBar.removeMenu(menuBarItemMenu)
                }
            }
        }
    }

    Component {
        id: extraMenuComp
        Menu {
            id: extraMenu
            objectName: "Extra"
            title: qsTr("&Extra")
            ContextAction { text: qsTr("&Trigger") }
            Action {
                text: qsTr("Remove Extra menu")
                onTriggered: menuBar.removeMenu(extraMenu)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent

        Label {
            text: qsTr("Right click on the window background to open a context menu. "
               + "Right click on the TextArea to access its edit context menu.\n\n"
               + "Things to check:\n\n"
               + "- Do the menu items trigger their actions (check console for output)?\n"
               + "- Do checkable menu items work?\n"
               + "- Do the Edit menu items (in the MenuBar menu and edit context menu)"
               + " work as expected with the TextArea?\n"
               + "  - Are they enabled/disabled as expected?\n"
               + "  - Does the TextArea keep focus after interacting with the Edit menu items?\n"
               + "- Does adding and removing menu items work?\n"
               + "- Do the menus in the MenuBar work?\n"
               + "- Can you add and remove menus from the MenuBar?\n"
               + "- Do shortcuts work?")
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap

            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: window.width * 0.5
            Layout.fillHeight: true
        }

        GroupBox {
            title: qsTr("Context menu")

            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent

                RowLayout {
                    Label {
                        text: qsTr("Popup type")
                    }

                    ComboBox {
                        id: popupTypeCombo
                        model: ["Item", "Window", "Native"]
                        onCurrentIndexChanged: CppSettings.popupType = currentIndex
                        currentIndex: CppSettings.popupType

                        function popupType() {
                            if (currentText === "Window")
                                return Popup.Window
                            else if (currentText === "Native")
                                return Popup.Native
                            else
                                return Popup.Item
                        }
                    }
                }

                Row {
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
                }
                Row {
                    Switch {
                        text: qsTr("Don't use native menu windows")
                        checked: CppSettings.dontUseNativeMenuWindows
                        onClicked: CppSettings.dontUseNativeMenuWindows = checked
                    }
                }
            }
        }

        TextArea {
            id: textArea
            text: qsTr("Dummy TextArea to test disabled menu items")

            Layout.fillWidth: true
            Layout.minimumHeight: 100

            TapHandler {
                objectName: "textAreaMouseTapHandler"
                acceptedButtons: Qt.RightButton
                onPressedChanged: if (pressed) editContextMenu.popup()
            }
            TapHandler {
                objectName: "textAreaTouchTapHandler"
                acceptedDevices: PointerDevice.TouchScreen
                onLongPressed: editContextMenu.popup()
            }
        }

        Component {
            id: menuBarItemComp
            MenuBarItem {
            }
        }

        MessageDialog {
            id: restartNeededDialog
            buttons: MessageDialog.Ok
            text: "Your current changes requires a restart to take effect!"
        }

        GroupBox {
            title: qsTr("MenuBar")

            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent

                Row {
                    Switch {
                        text: qsTr("Don't use native menu bar")
                        checked: CppSettings.dontUseNativeMenuBar

                        onClicked: {
                            CppSettings.dontUseNativeMenuBar = checked
                            restartNeededDialog.open()
                        }
                    }
                    Switch {
                        id: menuBarVisibleSwitch
                        text: qsTr("MenuBar visible")
                        checked: true
                    }
                }
                Row {
                    Button {
                        text: "Append menu"
                        onClicked: {
                            let menu = extraMenuComp.createObject(menuBar, { title: "Extra " + menuBar.count })
                            menuBar.addMenu(menu)
                        }
                    }
                    Button {
                        text: "Prepend menu"
                        onClicked: {
                            let menu = extraMenuComp.createObject(menuBar, { title: "Extra " + menuBar.count })
                            menuBar.insertMenu(0, menu)
                        }
                    }
                    Button {
                        text: qsTr("Add file menu")
                        onClicked: menuBar.addMenu(fileMenu)
                    }
                    Button {
                        text: "Change labels"
                        onClicked: {
                            fileMenu.title = "File changed"
                            cutAction.text = "Cut changed"
                        }
                    }
                    Button {
                        text: "toggle delegate"
                        onClicked: menuBar.delegate = menuBar.delegate ? null : menuBarItemComp
                    }
                    Switch {
                        text: "MenuBarItem visible"
                        checked: true
                        onCheckedChanged: explicitMenuBarItem.visible = checked
                    }
                }
            }
        }
    }

    TapHandler {
        objectName: "backgroundTouchTapHandler"
        acceptedDevices: PointerDevice.TouchScreen
        onLongPressed: backgroundContextMenu.popup()
    }

    Component {
        id: actionComponent

        Action {}
    }

    component ContextAction: Action {
        onCheckedChanged: (checked) => print("checked of \"" + text + "\" changed to " + checked)
        onTriggered: print("triggered \"" + text + "\"")
    }

    component ContextMenuItem: MenuItem {
        onCheckedChanged: print("checked of \"" + text + "\" changed to " + checked)
        onTriggered: print("triggered \"" + text + "\"")
    }

    contentItem.ContextMenu.menu: Menu {
        id: backgroundContextMenu
        objectName: "backgroundContextMenu"
        popupType: popupTypeCombo.popupType()

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
            text: qsTr("Context menu item")
            shortcut: "A"
        }
        ContextMenuItem {
            text: qsTr("Checkable context menu item")
            checkable: true
        }
        ContextAction {
            text: qsTr("Checked context menu item")
            checkable: true
            checked: true
            shortcut: "C"
        }
        ContextAction {
            text: qsTr("Disabled context menu item")
            enabled: false
            shortcut: "D"
        }
        ContextAction {
            text: qsTr("Checked and disabled context menu item")
            checkable: true
            checked: true
            enabled: false
            shortcut: "E"
        }

        MenuSeparator {}

        ContextAction {
            text: qsTr("Context menu item with icon (name)")
            icon.name: "mail-send"
        }

        ContextAction {
            text: qsTr("Context menu item with icon (source)")
            icon.source: "qrc:/qt/qml/Menus/icons/warning.png"
        }

        ContextAction {
            text: qsTr("Context menu item with disabled icon (source)")
            icon.source: "qrc:/qt/qml/Menus/icons/warning.png"
            enabled: false
        }

        MenuSeparator {}

        Menu {
            id: subMenu
            title: qsTr("Sub-menu")
            objectName: title
            popupType: popupTypeCombo.popupType()

            function appendAction() {
                let action = actionComponent.createObject(null, { text: qsTr("Extra sub-menu item") })
                subMenu.addAction(action)
            }

            function removeLastAction() {
                subMenu.removeAction(subMenu.actionAt(subMenu.contentData.length - 1))
            }

            ContextAction {
                text: qsTr("Sub-menu item")
            }
            ContextAction {
                text: qsTr("Checkable sub-menu item")
                checkable: true
                shortcut: "G"
            }
            ContextAction {
                text: qsTr("Checked sub-menu item")
                checkable: true
                checked: true
            }

            MenuSeparator {}

            ContextAction {
                text: qsTr("Disabled sub-menu item")
                enabled: false
                shortcut: "I"
            }
            ContextAction {
                text: qsTr("Checked and disabled sub-menu item")
                checkable: true
                checked: true
                enabled: false
                shortcut: "J"
            }
            Menu {
                title: qsTr("SubSub...")
                ContextAction { text: qsTr("SubSub action 1") }
                ContextAction { text: qsTr("SubSub action 2") }
            }
        }
    }

    Menu {
        id: editContextMenu
        objectName: "editContextMenu"

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

