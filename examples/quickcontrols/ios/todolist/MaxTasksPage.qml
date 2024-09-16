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
            id: maxTasksText
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("Choose the maximum amount of tasks each project can have.")
            font.pointSize: AppSettings.fontSize

            Layout.fillWidth: true
        }

        SpinBox {
            id: maxTasksSpinbox
            editable: true
            from: 5
            value: AppSettings.maxTasks
            to: 30

            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 10
            Layout.fillWidth: false

            onValueModified: AppSettings.maxTasks = maxTasksSpinbox.value
        }
    }
}
