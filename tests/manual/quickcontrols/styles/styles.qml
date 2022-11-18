// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ApplicationWindow {
    id: window

    visible: true
    minimumWidth: column.implicitWidth + 80
    minimumHeight: column.implicitHeight + 80
    title: "Qt Quick Controls 2 - Styles"

    // for taking frameless screenshots:
    // flags: Qt.Window | Qt.FramelessWindowHint

    ColumnLayout {
        id: column

        spacing: 20
        anchors.fill: parent
        anchors.margins: 40

        GroupBox {
            title: "Font Size"
            topPadding: 30
            background.visible: false

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 20

                RadioButton { leftPadding: 0; text: "Small" }
                RadioButton { leftPadding: 0; text: "Medium"; checked: true }
                RadioButton { leftPadding: 0; text: "Large" }
            }
        }

        GroupBox {
            title: "Audio"
            topPadding: 30
            background.visible: false

            GridLayout {
                columns: 2
                columnSpacing: 30
                anchors.fill: parent
                anchors.leftMargin: 20

                Label { text: "Volume" }
                Slider { value: 1.0 }
                Label { text: "Bass" }
                Slider { value: 0.75 }
                Label { text: "Treble" }
                Slider { value: 0.5 }
            }
        }

        Button {
            text: "Save"
            Layout.alignment: Qt.AlignRight
        }

        Item { Layout.fillHeight: true }
    }
}
