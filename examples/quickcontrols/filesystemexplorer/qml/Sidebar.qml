// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import FileSystemModule

Rectangle {
    id: root
    color: Colors.surface2

    required property ApplicationWindow rootWindow
    property alias currentTabIndex: tabBar.currentIndex

    ColumnLayout {
        anchors.fill: root
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        spacing: 10

        // TabBar is designed to be horizontal, whereas we need a vertical bar.
        // We can easily achieve that by using a Container.
        Container {
            id: tabBar

            Layout.fillWidth: true

            // ButtonGroup ensures that only one button can be checked at a time.
            ButtonGroup {
                buttons: tabBar.contentItem.children
                // We have to manage the currentIndex ourselves, which we do by setting it to the
                // index of the currently checked button.
                // We use setCurrentIndex instead of setting the currentIndex property to avoid breaking bindings.
                // See "Managing the Current Index" in Container's documentation for more information.
                onCheckedButtonChanged: tabBar.setCurrentIndex(Math.max(0, buttons.indexOf(checkedButton)))
            }

            contentItem: ColumnLayout {
                spacing: tabBar.spacing

                Repeater {
                    model: tabBar.contentModel
                }
            }

            component SidebarEntry: Button {
                id: sidebarButton
                icon.color: down || checked ? Colors.iconIndicator : Colors.icon
                icon.width: 35
                icon.height: 35
                leftPadding: 8 + indicator.width

                background: null

                Rectangle {
                    id: indicator
                    x: 4
                    anchors.verticalCenter: parent.verticalCenter
                    width: 4
                    height: sidebarButton.icon.width
                    color: Colors.color1
                    visible: sidebarButton.checked
                }
            }

            // Shows help text when clicked.
            SidebarEntry {
                icon.source: "../icons/light_bulb.svg"
                checkable: true
                checked: true

                Layout.alignment: Qt.AlignHCenter
            }

            // Shows the file system when clicked.
            SidebarEntry {
                icon.source: "../icons/read.svg"
                checkable: true

                Layout.alignment: Qt.AlignHCenter
            }
        }

        // This item acts as a spacer to expand between the checkable and non-checkable buttons.
        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true

            // Make the empty space drag our main window.
            WindowDragHandler { dragWindow: rootWindow }
        }

        // Opens the Qt website in the system's web browser.
        SidebarEntry {
            id: qtWebsiteButton
            icon.source: "../icons/globe.svg"
            checkable: false

            onClicked: Qt.openUrlExternally("https://www.qt.io/")
        }

        // Opens the About Qt Window.
        SidebarEntry {
            id: aboutQtButton
            icon.source: "../icons/info_sign.svg"
            checkable: false

            onClicked: aboutQtWindow.visible = !aboutQtWindow.visible
        }
    }

    About {
        id: aboutQtWindow
        visible: false
    }
}
