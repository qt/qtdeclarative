// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

            onClicked: AppSettings.showDoneTasks = checked
        }
    }
}
