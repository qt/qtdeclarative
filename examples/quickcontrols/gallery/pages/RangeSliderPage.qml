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
            text: qsTr("RangeSlider is used to select a range specified by two values, by sliding each handle along a track.")
        }

        RangeSlider {
            enabled: !GalleryConfig.disabled
            first.value: 0.25
            second.value: 0.75
            anchors.horizontalCenter: parent.horizontalCenter
            Accessible.name: qsTr("demo range slider")
        }

        RangeSlider {
            enabled: !GalleryConfig.disabled
            orientation: Qt.Vertical
            first.value: 0.25
            second.value: 0.75
            anchors.horizontalCenter: parent.horizontalCenter
            Accessible.name: qsTr("vertical demo slider")
        }
    }
}
