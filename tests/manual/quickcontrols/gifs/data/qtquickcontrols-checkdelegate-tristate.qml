// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: window
    width: 170
    height: column.implicitHeight
    visible: true

    property alias english: english
    property alias norwegian: norwegian

    ColumnLayout {
        id: column
        anchors.fill: parent

        CheckDelegate {
            text: qsTr("Languages")
            checkState: english.checked && norwegian.checked
                ? Qt.Checked : (english.checked || norwegian.checked) ? Qt.PartiallyChecked : Qt.Unchecked
            tristate: true
            Layout.fillWidth: true
        }
        CheckDelegate {
            id: english
            text: qsTr("English")
            checked: true
            leftPadding: indicator.width + 14
            Layout.fillWidth: true
        }
        CheckDelegate {
            id: norwegian
            text: qsTr("Norwegian")
            checked: true
            leftPadding: indicator.width + 14
            Layout.fillWidth: true
        }
    }
}
