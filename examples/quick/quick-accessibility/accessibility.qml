// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: window

    width: 320; height: 480
    color: "white"

    Column {
        id: column
        spacing: 6
        anchors.fill: parent
        anchors.margins: 10
        width: parent.width

        Row {
            spacing: 6
            width: column.width

            Text {
                id: subjectLabel
                //! [text]
                Accessible.role: Accessible.StaticText
                Accessible.name: text
                //! [text]
                text: "Subject:"
                y: 3
            }

            Rectangle {
                id: subjectBorder
                Accessible.role: Accessible.EditableText
                Accessible.name: subjectEdit.text
                border.width: 1
                border.color: "black"
                height: subjectEdit.height + 6
                width: 240
                TextInput {
                    focus: true
                    y: 3
                    x: 3
                    width: parent.width - 6
                    id: subjectEdit
                    text: "Vacation plans"
                    KeyNavigation.tab: textEdit
                }
            }
        }

        Rectangle {
            id: textBorder
            Accessible.role: Accessible.EditableText
            property alias text: textEdit.text
            border.width: 1
            border.color: "black"
            width: parent.width - 2
            height: 200

            TextEdit {
                id: textEdit
                y: 3
                x: 3
                width: parent.width - 6
                height: parent.height - 6
                text: "Hi, we're going to the Dolomites this summer. Weren't you also going to northern Italy? \n\nBest wishes, your friend Luke"
                wrapMode: TextEdit.WordWrap
                KeyNavigation.tab: sendButton
                KeyNavigation.priority: KeyNavigation.BeforeItem
            }
        }

        Text {
            id : status
            width: column.width
        }

        Row {
            spacing: 6
            Button {
                id: sendButton
                width: 100; height: 20
                text: "Send"
                onClicked: { status.text = "Send" }
                KeyNavigation.tab: discardButton
            }
            Button { id: discardButton
                width: 100; height: 20
                text: "Discard"
                onClicked: { status.text = "Discard" }
                KeyNavigation.tab: checkBox
            }
        }

        Row {
            spacing: 6

            Checkbox {
                id: checkBox
                checked: false
                KeyNavigation.tab: slider
            }

            Slider {
                id: slider
                value: 10
                KeyNavigation.tab: subjectEdit
            }
        }
    }
}
