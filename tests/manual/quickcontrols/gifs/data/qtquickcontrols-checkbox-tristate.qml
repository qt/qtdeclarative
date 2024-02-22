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

    property alias english: english
    property alias norwegian: norwegian

    ColumnLayout {
        id: column
        anchors.centerIn: parent

        CheckBox {
            text: qsTr("Languages")
            checkState: english.checked && norwegian.checked
                ? Qt.Checked : (english.checked || norwegian.checked) ? Qt.PartiallyChecked : Qt.Unchecked
            tristate: true
            Layout.fillWidth: false
        }
        CheckBox {
            id: english
            text: qsTr("English")
            checked: true
            leftPadding: indicator.width
            Layout.fillWidth: false
        }
        CheckBox {
            id: norwegian
            text: qsTr("Norwegian")
            checked: true
            leftPadding: indicator.width
            Layout.fillWidth: false
        }
    }
}
