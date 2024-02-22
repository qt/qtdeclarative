// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    visible: true
    width: 800
    height: 600
    title: qsTr("Desktop Gallery")

    TabBar {
        id: bar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 20
        anchors.rightMargin: 40

        TabButton {
            text: qsTr("Default controls")
        }

        TabButton {
            text: qsTr("Customized controls")
        }
    }

    StackLayout {
        currentIndex: bar.currentIndex
        anchors.top: bar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 20

        ScrollView {
            contentWidth: availableWidth

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 20

                Buttons { }
                CheckBoxes { }
                RadioButtons { }
                SpinBoxes { }
                TextFields { }
                TextAreas { }
                ComboBoxes { }
                Dials { }
                Frames { }
                ProgressBars { }
                ScrollBars { }
                Sliders { }
                SlidersSmall { }
                SlidersMini { }
            }
        }

        ScrollView {
            contentWidth: availableWidth

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 20

                CustomButtons { }
                CustomCheckBoxes { }
                CustomRadioButtons { }
                CustomSpinBoxes { }
                CustomTextFields { }
                CustomTextAreas { }
                CustomComboBoxes { }
                CustomDials { }
                CustomFrames { }
                CustomProgressBars { }
                CustomScrollBars { }
                CustomSliders { }
            }
        }
    }

}
