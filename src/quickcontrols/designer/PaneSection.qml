// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick
import HelperWidgets
import QtQuick.Layouts

Section {
    caption: qsTr("Pane")

    SectionLayout {
        Label {
            text: qsTr("Content Width")
            tooltip: qsTr("Content height used for calculating the total implicit width.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.contentWidth
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Content Height")
            tooltip: qsTr("Content height used for calculating the total implicit height.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.contentHeight
                Layout.fillWidth: true
            }
        }
    }
}
