// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 320
    height: 480
    visible: true
    title: qsTr("Advanced Text Example")

    FontLoader {
        id: georama
        source: "fonts/Georama-VariableFont_wdth,wght.ttf"
    }

    Column {
        id: column

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width * 0.6
        spacing: 20

        Text {
            width: parent.width
            font.pixelSize: 12
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            text: qsTr("The following text does not fit in its layout. Fine-tune the width and weight to make it fit.")
        }

        TextSample {
            anchors.topMargin: 50
            text: qsTr("Breaking: News!")
            font.pixelSize: 32
        }

        TextSample {
            font.pixelSize: 18
            text: "Lorem ipsum dolor sit amet,\n" +
                  "consectetur adipiscing elit.\n" +
                  "Praesent ornare nunc vel mauris\n" +
                  "bibendum gravida.\n" +
                  "Maecenas sed massa maximus,\n" +
                  "sagittis ex in, tristique ipsum.\n" +
                  "Vestibulum tincidunt est sapien,\n" +
                  "ut venenatis sapien tincidunt ut."
        }
    }
}
