// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ScrollablePage {
    id: page

    Column {
        spacing: 40
        width: parent.width

        Label {
            width: parent.width
            wrapMode: Label.Wrap
            horizontalAlignment: Qt.AlignHCenter
            text: qsTr("Slider is used to select a value by sliding a handle along a track.")
        }

        Slider {
            enabled: !GalleryConfig.disabled
            value: 0.5
            anchors.horizontalCenter: parent.horizontalCenter
            Accessible.name: qsTr("demo slider")
        }

        Slider {
            enabled: !GalleryConfig.disabled
            orientation: Qt.Vertical
            value: 0.5
            anchors.horizontalCenter: parent.horizontalCenter
            Accessible.name: qsTr("vertical demo slider")
        }
    }
}
