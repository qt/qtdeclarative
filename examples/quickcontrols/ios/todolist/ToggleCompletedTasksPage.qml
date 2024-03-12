// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    ColumnLayout {
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter

        Label {
            id: toggleText
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: toggleTasksSwitch.checked ? qsTr("Completed tasks will be shown.")
                                            : qsTr("Completed tasks will be hidden.")
            font.pointSize: AppSettings.fontSize

            Layout.fillWidth: true
        }

        Switch {
            id: toggleTasksSwitch
            checked: AppSettings.showDoneTasks

            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 10
            Layout.fillWidth: false

            onClicked: AppSettings.showDoneTasks = checked
        }
    }
}
