// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtCore
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: window
    title: "Material"
    width: screen.desktopAvailableWidth * 0.8
    height: screen.desktopAvailableHeight * 0.8
    visible: true

    Material.theme: settings.theme

    Settings {
        id: settings

        property alias windowX: window.x
        property alias windowY: window.y
        property alias windowWidth: window.width
        property alias windowHeight: window.height

        property int theme: darkThemeSwitch.checked ? Material.Dark : Material.Light
        property string variant: denseSwitch.checked ? "Dense" : "Normal"

        property alias currentControlIndex: listView.currentIndex
    }

    Shortcut {
        sequences: ["Esc", "Back"]
        onActivated: openDrawerAction.trigger()
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }

    Action {
        id: openDrawerAction
        text: "Controls"
        onTriggered: drawer.open()
    }

    header: ToolBar {
        RowLayout {
            spacing: 20
            anchors.fill: parent

            Material.theme: Material.Dark

            ToolButton {
                action: openDrawerAction
                Layout.fillWidth: false
            }

            Label {
                id: titleLabel
                text: listView.currentItem ? listView.currentItem.text : "Material"
                font.pixelSize: 20
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }

            Item {
                Layout.fillWidth: true
            }

            Switch {
                id: darkThemeSwitch
                text: "Dark"
                checked: settings.theme === Material.Dark
                Layout.fillWidth: false
            }

            Switch {
                id: denseSwitch
                text: "Dense"
                Layout.fillWidth: false
                checked: settings.variant === "Dense"

                ToolTip.text: "Requires restart"
                ToolTip.visible: hovered
            }
        }
    }

    Drawer {
        id: drawer
        width: window.width / 3
        height: window.height
        interactive: stackView.depth === 1

        ListView {
            id: listView
            focus: true
            currentIndex: settings.currentControlIndex
            anchors.fill: parent
            model: ["Button", "DelayButton", "RoundButton", "Switch", "TextArea", "TextField"]
            delegate: ItemDelegate {
                width: listView.width
                text: modelData
                highlighted: ListView.isCurrentItem
                onClicked: listView.currentIndex = index
            }

            ScrollIndicator.vertical: ScrollIndicator { }

            // Need to wait until our count is non-zero before setting a default currentIndex.
            // This also allows us to use an alias for the settings property.
            Component.onCompleted: if (currentIndex === -1) currentIndex = 0

            onCurrentIndexChanged: {
                if (currentIndex >= 0 && currentIndex < count)
                    stackView.replace("qrc:/pages/" + model[currentIndex] + "Page.qml")
                drawer.close()
            }
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
    }
}
