// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
    width: column.implicitWidth
    height: column.implicitHeight
    visible: true

    property alias control1: control1
    property alias control2: control2
    property alias control3: control3

    ColumnLayout {
        id: column
        anchors.centerIn: parent

        RadioButton {
            id: control1
            text: qsTr("First")
            checked: true
            Layout.fillWidth: false
        }
        RadioButton {
            id: control2
            text: qsTr("Second")
            Layout.fillWidth: false
        }
        RadioButton {
            id: control3
            text: qsTr("Third")
            Layout.fillWidth: false
        }
    }
}
