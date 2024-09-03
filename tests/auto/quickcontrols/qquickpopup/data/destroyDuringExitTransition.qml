// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 400
    height: 400
    title: "destroyDuringExitTransition"

    property Dialog dialog1
    property Dialog dialog2

    Component {
        id: dlg

        Dialog {
            dim: true
            modal: true
            closePolicy: Popup.CloseOnEscape
            visible: true
            popupType: Popup.Item

            property alias button: button

            Column {
                Text {
                    text: "button is " + (button.down ? "down" : "up")
                }

                Button {
                    id: button
                    text: "Try to press this button"
                }
            }
        }
    }

    Component {
        id: brokenDlg
        Dialog {
            dim: true
            modal: true
            focus: true
            closePolicy: Popup.CloseOnEscape
            visible: true
            popupType: Popup.Item

            Text {
                text: "Press Esc key to reject this dialog"
            }

            exit: Transition {
                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 100 }
            }
        }
    }


    Component.onCompleted: {
        dialog1 = dlg.createObject(window)
        dialog2 = brokenDlg.createObject(window)

        dialog2.onRejected.connect(function(){
            dialog2.destroy()
        })
    }
}
