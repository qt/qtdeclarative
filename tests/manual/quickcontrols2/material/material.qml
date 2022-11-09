// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: window
    title: "Material"
    width: screen.desktopAvailableWidth * 0.8
    height: screen.desktopAvailableHeight * 0.8
    visible: true

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

            ToolButton {
                action: openDrawerAction
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
        }
    }

    function showPageForControl(controlName, index) {
        listView.currentIndex = index
        stackView.replace("qrc:/pages/" + controlName + "Page.qml")
        drawer.close()
    }

    Drawer {
        id: drawer
        width: window.width / 3
        height: window.height
        interactive: stackView.depth === 1

        ListView {
            id: listView
            focus: true
            currentIndex: -1
            anchors.fill: parent

            model: ["Button"]
            delegate: ItemDelegate {
                width: listView.width
                text: modelData
                highlighted: ListView.isCurrentItem
                onClicked: window.showPageForControl(modelData, index)
            }

            ScrollIndicator.vertical: ScrollIndicator { }
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent

        Component.onCompleted: window.showPageForControl("Button", 0)
    }
}
