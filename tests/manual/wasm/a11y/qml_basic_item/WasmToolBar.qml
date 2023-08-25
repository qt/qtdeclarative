// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    id: mainTooBar
    signal cancelThisMeeting
    signal requestReadReceipt
    property bool cancelMeeting: false
    RowLayout {

        spacing: 10
        Accessible.role: Accessible.ToolBar
        ToolButton {
            id: cancelButton
            text: qsTr("Clear")
            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: text
            onClicked: {
                cancelThisMeeting()
            }
        }
        ToolButton {
            id: readRequestButton
            text: qsTr("Request receipt")
            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: text
            onClicked: {
                requestReadReceipt()
            }
        }
    }
}
