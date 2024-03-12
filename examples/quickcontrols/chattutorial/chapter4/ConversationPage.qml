// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import chattutorial

Page {
    id: root

    property string inConversationWith

    header: ToolBar {
        ToolButton {
            text: qsTr("Back")
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            onClicked: root.StackView.view.pop()
        }

        Label {
            id: pageTitle
            text: root.inConversationWith
            font.pixelSize: 20
            anchors.centerIn: parent
        }
    }

    ColumnLayout {
        anchors.fill: parent

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: pane.leftPadding + messageField.leftPadding
            displayMarginBeginning: 40
            displayMarginEnd: 40
            verticalLayoutDirection: ListView.BottomToTop
            spacing: 12
            model: SqlConversationModel {
                recipient: root.inConversationWith
            }
            delegate: Column {
                id: conversationDelegate
                anchors.right: sentByMe ? listView.contentItem.right : undefined
                spacing: 6

                required property string author
                required property string recipient
                required property date timestamp
                required property string message
                readonly property bool sentByMe: recipient !== "Me"

                Row {
                    id: messageRow
                    spacing: 6
                    anchors.right: conversationDelegate.sentByMe ? parent.right : undefined

                    Image {
                        id: avatar
                        source: !conversationDelegate.sentByMe
                            ? "images/" + conversationDelegate.author.replace(" ", "_") + ".png" : ""
                    }

                    Rectangle {
                        width: Math.min(messageText.implicitWidth + 24,
                            listView.width - (!conversationDelegate.sentByMe ? avatar.width + messageRow.spacing : 0))
                        height: messageText.implicitHeight + 24
                        color: conversationDelegate.sentByMe ? "lightgrey" : "steelblue"

                        Label {
                            id: messageText
                            text: conversationDelegate.message
                            color: conversationDelegate.sentByMe ? "black" : "white"
                            anchors.fill: parent
                            anchors.margins: 12
                            wrapMode: Label.Wrap
                        }
                    }
                }

                Label {
                    id: timestampText
                    text: Qt.formatDateTime(conversationDelegate.timestamp, "d MMM hh:mm")
                    color: "lightgrey"
                    anchors.right: conversationDelegate.sentByMe ? parent.right : undefined
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }

        Pane {
            id: pane
            Layout.fillWidth: true
            Layout.fillHeight: false

            RowLayout {
                width: parent.width

                TextArea {
                    id: messageField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Compose message")
                    wrapMode: TextArea.Wrap
                }

                Button {
                    id: sendButton
                    text: qsTr("Send")
                    enabled: messageField.length > 0
                    Layout.fillWidth: false
                    onClicked: {
                        listView.model.sendMessage(root.inConversationWith, messageField.text)
                        messageField.text = ""
                    }
                }
            }
        }
    }
}

