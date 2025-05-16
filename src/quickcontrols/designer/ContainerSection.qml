// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick
import HelperWidgets
import QtQuick.Layouts

Section {
    caption: qsTr("Container")

    SectionLayout {
        Label {
            text: qsTr("Current")
            tooltip: qsTr("The index of the current item.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                decimals: 0
                backendValue: backendValues.currentIndex
                Layout.fillWidth: true
            }
        }
    }
}
