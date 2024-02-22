// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 800
    height: 600
    title: "dialogs - style: " + style
    visible: true

    required property string style

    Component.onCompleted: {
        x = Screen.width / 2 - width / 2
        y = Screen.height / 2 - height / 2
    }

    Settings {
        id: settings

        property alias useNativeDialogs: useNativeDialogsCheckBox.checked
        property alias lastTabBarIndex: tabBar.currentIndex
    }

    Page {
        anchors.fill: parent

        header: TabBar {
            id: tabBar

            TabButton {
                text: qsTr("ColorDialog")
            }
            TabButton {
                text: qsTr("FileDialog")
            }
            TabButton {
                text: qsTr("FolderDialog")
            }
            TabButton {
                text: qsTr("FontDialog")
            }
            TabButton {
                text: qsTr("MessageBox")
            }
        }

        ScrollView {
            id: scrollView
            anchors.fill: parent
            clip: true

            StackLayout {
                id: stackLayout
                currentIndex: tabBar.currentIndex
                width: scrollView.width

                ColorDialogPage {}
                FileDialogPage {}
                FolderDialogPage {}
                FontDialogPage {}
                MessageDialogPage {}
            }
        }
    }

    footer: ToolBar {
        leftPadding: 12
        rightPadding: 12

        RowLayout {
            anchors.fill: parent

            CheckBox {
                id: useNativeDialogsCheckBox
                text: qsTr("Use Native Dialogs (requires restart)")
                checked: settings.useNativeDialogs
                Layout.fillWidth: false
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Open")
                Layout.fillWidth: false

                onClicked: stackLayout.children[stackLayout.currentIndex].dialog.open()
            }
        }
    }
}
