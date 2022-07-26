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

            onValueModified: AppSettings.maxTasks = maxTasksSpinbox.value
        }
    }
}
