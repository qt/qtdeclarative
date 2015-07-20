/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.6
import QtQuick.Window 2.2
import Qt.labs.templates 1.0 as T
import Qt.labs.controls 1.0
//import QtQuick.Enterprise.VirtualKeyboard 1.3

ApplicationWindow {
    id: window
    title: "Qt Quick Controls - Pop-o-rama!"
    width: 640
    height: 800
    visible: true

    ContextMenu {
        target: window.activeFocusItem
    }

    Column {
        y: 100
        width: parent.width
        spacing: 16

        Label {
            id: titleLabel
            text: "Welcome to Pop-O-Rama!"
            x: (Window.width - implicitWidth) / 3
            font.bold: true
            font.pixelSize: 24
            height: 2 * font.pixelSize
        }

        Label {
            text: "All the TextField controls in this app have a context menu. Long press to make it appear."
            anchors.left: titleLabel.left
            width: Window.width - x - 12
            wrapMode: Label.WordWrap
        }

        TextField {
            id: someTextField
            focus: true
            width: 200
            anchors.left: titleLabel.left
            placeholderText: "I should lose focus when a popup shows"
        }

        Button {
            id: pb
            text: "Secret Dialog"
            anchors.left: someTextField.left
            onPressed: dialog.show()

            Label {
                text: "Click this button to see our secret dialog."
                anchors.left: parent.right
                anchors.leftMargin: 12
                anchors.baseline: parent.label.baseline
            }

            Dialog {
                id: dialog

                Column {
                    spacing: 12

                    Label {
                        text: "Secret Dialog!"
                        font.pixelSize: 14
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Label {
                        text: "Notice how tab navigation stays in the dialog."
                        width: parent.width
                        wrapMode: Label.WordWrap
                    }

                    TextField {
                        id: firstTextField
                        width: 200
                        focus: true
                        placeholderText: "User name"

                        Keys.forwardTo: [completionPopup.contentItem]
                        CompletionPopup {
                            id: completionPopup
                            input: firstTextField
                            model: ["Eric", "Graham", "John", "Michael", "Terry"]
                        }
                    }

                    TextField {
                        width: 200
                        placeholderText: "Password"
                        echoMode: TextField.Password
                    }

                    Row {
                        spacing: 12
                        anchors.horizontalCenter: parent.horizontalCenter
                        Button {
                            text: "Cancel"
                            width: 80
                            onClicked: dialog.hide()
                        }
                        Button {
                            text: "OK"
                            width: 80
                            onClicked: dialog.hide()
                        }
                    }
                }

                onAboutToShow: firstTextField.focus = true
            }
        }
    }

    NotificationCenter {
        id: notificationCenter

        window: window
    }

    MessageCenter {
        id: messageCenter

        onMessageReceived: notificationCenter.notify(sender, message)
    }

//    T.Panel {
//        id: inputPanel
//        modal: false
//        contentItem: InputPanel {
//            id: actualPanel
//            width: window.width
//            y: window.height - (Qt.inputMethod.visible ? height : 0)
//            Behavior on y { NumberAnimation { duration: 250; easing.type: Easing.InOutQuad } }
//            z: 1000

//            Component.onCompleted: inputPanel.show()

////            Connections {
////                target: Qt.inputMethod
////                onVisibleChanged: { if (visible) inputPanel.show(); else inputPanel.hide() }
////            }
//        }
//    }
}
