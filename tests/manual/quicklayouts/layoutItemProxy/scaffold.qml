// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
// This example was created for the blog post about responsive layouts:
// https://www.qt.io/blog/responsive-layouts-in-qt

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls


Window {
    id: window

    width: 600
    height: 800

    minimumHeight: 500
    minimumWidth: 160

    title: "Window: (" + width + "x" + height + ")"
    visible: true

    FontLoader { id: materialFont; source: "https://github.com/google/material-design-icons/blob/master/font/MaterialIcons-Regular.ttf?raw=true" }

    component MyButton : Rectangle {
        implicitWidth: 32
        implicitHeight: label == "" ? 32 : 32+22
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignHCenter
        property int iconId: 0
        property string label: ""

        Text {
            id: im
            height: 32
            width: 32
            font.family: materialFont.font.family
            font.weight: materialFont.font.weight
            font.pixelSize: 32
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            text: String.fromCodePoint(iconId)
            color: "#555"
        }

        Text {
            text: parent.label
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: im.bottom
            font.pixelSize: 12
            color: "#555"
        }
    }

    LayoutChooser {
        id: layoutChooser
        width: parent.width
        height: parent.height

        layoutChoices: [
            smallLayout,
            mediumLayout,
            largeLayout
        ]

        criteria: [
            width < 700,
            width < 1000,
            true
        ]

        MyButton {
            id: inboxButton
            objectName: "inboxButton"
            iconId: 0xe156 // see https://fonts.google.com/icons
            label: layoutChooser.width <700 ? "Inbox" : ""
        }
        MyButton {
            id: articlesButton
            objectName: "articlesButton"
            iconId: 0xef42 // see https://fonts.google.com/icons
        }
        MyButton {
            id: chatButton
            objectName: "chatButton"
            iconId: 0xe0b7 // see https://fonts.google.com/icons
        }
        MyButton {
            id: videoButton
            objectName: "videoButton"
            iconId: 0xe070 // see https://fonts.google.com/icons
        }

        Rectangle {
            id: bigbox
            color: '#e99ec0'
            implicitHeight: 512
            implicitWidth: 512
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Flickable {
            id: flick
            Layout.fillHeight: true
            Layout.fillWidth: true
            implicitWidth: 512
            contentWidth: width
            contentHeight: gl.height
            GridLayout {
                id: gl
                columns: 2
                width: parent.width
                height: implicitHeight
                columnSpacing: 10
                rowSpacing: 10
                Repeater {
                    model: 12
                    LayoutItemProxy { target: rep.itemAt(index) }
                }
            }
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        Repeater {
            id: rep
            model: 12
            Rectangle {
                objectName: "Rectangle" + index
                color: '#ffc9c5'
                implicitHeight: width
                implicitWidth: 256
                Layout.fillWidth: true
                Layout.fillHeight: true
                Text {
                    anchors.centerIn: parent
                    color: '#e99ec0'
                    text: index
                    font.pixelSize: 64
                }
            }
        }

        property Item smallLayout: ColumnLayout {
            parent: layoutChooser
            height: parent.height
            width: parent.width
            Repeater {
                model: 2
                LayoutItemProxy { target: rep.itemAt(index) }
            }
            RowLayout {
                Layout.fillHeight: false
                Layout.fillWidth: true
                LayoutItemProxy { target: inboxButton }
                LayoutItemProxy { target: articlesButton }
                LayoutItemProxy { target: chatButton }
                LayoutItemProxy { target: videoButton }
            }
        }

        property Item mediumLayout: RowLayout {
            parent: layoutChooser
            height: parent.height
            width: parent.width
            ColumnLayout {
                Layout.fillHeight: false
                Layout.alignment: Qt.AlignTop
                LayoutItemProxy { target: inboxButton }
                LayoutItemProxy { target: articlesButton }
                LayoutItemProxy { target: chatButton }
                LayoutItemProxy { target: videoButton }
            }
            LayoutItemProxy { target: flick }
            LayoutItemProxy { target: bigbox }
        }

        property Item largeLayout: RowLayout {
            parent: layoutChooser
            height: parent.height
            width: parent.width
            GridLayout {
                columns: 2
                Layout.fillHeight: false
                Layout.fillWidth: false
                Layout.alignment: Qt.AlignTop
                LayoutItemProxy { target: inboxButton }
                Text { text: "Inbox"; color: "#555"; font.pixelSize: 20 }
                LayoutItemProxy { target: articlesButton }
                Text { text: "Articles"; color: "#555"; font.pixelSize: 20 }
                LayoutItemProxy { target: chatButton }
                Text { text: "Chat"; color: "#555"; font.pixelSize: 20 }
                LayoutItemProxy { target: videoButton }
                Text { text: "Video"; color: "#555"; font.pixelSize: 20 }
            }
            LayoutItemProxy { target: flick }
            LayoutItemProxy { target: bigbox }
        }
    }
}
