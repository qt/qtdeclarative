// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400
    objectName: "Rectangle"

    property alias dialog: dialog
    property alias outsideButton1: outsideButton1
    property alias outsideButton2: outsideButton2
    property alias dialogButton1: dialogButton1
    property alias dialogButton2: dialogButton2

    ColumnLayout {
        Button {
            id: outsideButton1
            text: "Button1"
        }
        Button {
            id: outsideButton2
            text: "Button2"
        }
    }

    Dialog {
        id: dialog
        objectName: "Dialog"
        width: 200
        height: 200
        anchors.centerIn: parent
        visible: true

        ColumnLayout {
            Button {
                id: dialogButton1
                text: "Button3"
            }
            Button {
                id: dialogButton2
                text: "Button4"
            }
        }
    }
}
