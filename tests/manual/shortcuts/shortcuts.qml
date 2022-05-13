// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.3

ApplicationWindow {
    id: window

    width: 520
    height: 340
    visible: true
    title: "Shortcuts - main"

    menuBar: MenuBar {
        Menu {
            title: "File"
            MenuItem {
                text: "New..."
                shortcut: StandardKey.New
                onTriggered: shortcutWindow.createObject(window)
            }
            MenuItem {
                text: "Quit"
                shortcut: StandardKey.Quit
                onTriggered: Qt.quit()
            }
        }
    }

    Loader {
        anchors.margins: 20
        anchors.fill: parent
        sourceComponent: shortcutColumn
    }

    Component {
        id: shortcutWindow

        ApplicationWindow {
            width: 520
            height: 300
            visible: true
            title: "Shortcuts - child"

            Loader {
                anchors.margins: 20
                anchors.fill: parent
                sourceComponent: shortcutColumn
            }

            onClosing: destroy(1)
        }
    }

    Component {
        id: shortcutColumn

        Column {
            spacing: 20

            Repeater {
                model: ["Esc", "Ctrl+C", "Alt+6", "Shift+F12", "Ctrl+X,Ctrl+C"]

                RowLayout {
                    spacing: 20
                    width: parent.width

                    CheckBox {
                        id: checkbox
                        text: modelData
                        checked: index % 2 == 0
                        Layout.fillWidth: true
                    }

                    ComboBox {
                        id: combobox
                        enabled: checkbox.checked
                        model: ["WindowShortcut", "ApplicationShortcut"]
                    }

                    Shortcut {
                        id: shortcut

                        property int activationCount: 0
                        property int ambiguousActivationCount: 0

                        sequence: modelData
                        enabled: checkbox.checked
                        context: combobox.currentText

                        onActivated: { activationCount++; activationTimer.restart() }
                        onActivatedAmbiguously: { ambiguousActivationCount++; ambiguousActivationTimer.restart() }
                    }

                    Timer { id: activationTimer; interval: 500 }
                    Timer { id: ambiguousActivationTimer; interval: 500 }

                    Column {
                        Text {
                            font.pixelSize: 10
                            text: qsTr("Activated: %1").arg(shortcut.activationCount)
                            color: activationTimer.running ? "red" : checkbox.checked ? "black" : "gray"
                        }
                        Text {
                            font.pixelSize: 10
                            text: qsTr("Ambiguously: %1").arg(shortcut.ambiguousActivationCount)
                            color: ambiguousActivationTimer.running ? "red" : checkbox.checked ? "black" : "gray"
                        }
                    }
                }
            }
        }
    }
}

