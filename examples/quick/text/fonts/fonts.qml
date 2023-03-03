// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root
    readonly property string myText: qsTr("The quick brown fox jumps over the lazy dog.")

    width: 320
    height: 480
    color: "steelblue"

//! [fontloaderlocal]
    FontLoader {
        id: localFont
        source: "content/fonts/tarzeau_ocr_a.ttf"
    }
//! [fontloaderlocal]
//! [fontloaderremote]
    FontLoader {
        id: webFont
        source: "http://www.princexml.com/fonts/steffmann/Starburst.ttf"
    }
//! [fontloaderremote]

    Column {
        anchors {
            fill: parent
            leftMargin: 10
            rightMargin: 10
            topMargin: 10
        }
        spacing: 15

        Text {
            text: root.myText
            color: "lightsteelblue"
            width: parent.width
            wrapMode: Text.WordWrap
//! [name]
            font.family: "Times"
//! [name]
            font.pixelSize: 20
        }
        Text {
            text: root.myText
            color: "lightsteelblue"
            width: parent.width
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            font {
                family: "Times"
                pixelSize: 20
                capitalization: Font.AllUppercase
            }
        }
        Text {
            text: root.myText
            color: "lightsteelblue"
            width: parent.width
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.WordWrap
            font {
                family: "Courier"
                pixelSize: 20
                weight: Font.Bold
                capitalization: Font.AllLowercase
            }
        }
        Text {
            text: root.myText
            color: "lightsteelblue"
            width: parent.width
            wrapMode: Text.WordWrap
            font {
                family: "Courier"
                pixelSize: 20
                italic: true
                capitalization: Font.SmallCaps
            }
        }
        Text {
            text: root.myText
            color: "lightsteelblue"
            width: parent.width
            wrapMode: Text.WordWrap
            font {
                family: localFont.name
                pixelSize: 20
                capitalization: Font.Capitalize
            }
        }
        Text {
            text: {
                if (webFont.status == FontLoader.Ready) root.myText
                else if (webFont.status == FontLoader.Loading) "Loading..."
                else if (webFont.status == FontLoader.Error) "Error loading font"
            }
            color: "lightsteelblue"
            width: parent.width
            wrapMode: Text.WordWrap
            font.family: webFont.name
            font.pixelSize: 20
        }
    }
}
