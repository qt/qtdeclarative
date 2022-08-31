// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: main
    width: 320; height: 480
    focus: true
    color: "#dedede"

    property int hAlign: Text.AlignLeft

    Flickable {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: col.height + 20

        Column {
            id: col
            x: 10; y: 10
            spacing: 20
            width: parent.width - 20
            property int hAlign: main.hAlign

            TextWithImage {
                text: "This is a <b>happy</b> face<img src=\"images/face-smile.png\">"
            }
            TextWithImage {
                text: "This is a <b>very<img src=\"images/face-smile-big.png\" align=\"middle\"/>happy</b> face vertically aligned in the middle."
            }
            TextWithImage {
                text: "This is a tiny<img src=\"images/face-smile.png\" width=\"15\" height=\"15\">happy face."
            }
            TextWithImage {
                text: "This is a<img src=\"images/starfish_2.png\" width=\"50\" height=\"50\" align=\"top\">aligned to the top and a<img src=\"images/heart200.png\" width=\"50\" height=\"50\">aligned to the bottom."
            }
            TextWithImage {
                text: "Qt logos<img src=\"images/qtlogo.png\" width=\"55\" height=\"60\" align=\"middle\"><img src=\"images/qtlogo.png\" width=\"37\" height=\"40\" align=\"middle\"><img src=\"images/qtlogo.png\" width=\"18\" height=\"20\" align=\"middle\">aligned in the middle with different sizes."
            }
            TextWithImage {
                text: "Some hearts<img src=\"images/heart200.png\" width=\"20\" height=\"20\" align=\"bottom\"><img src=\"images/heart200.png\" width=\"30\" height=\"30\" align=\"bottom\"> <img src=\"images/heart200.png\" width=\"40\" height=\"40\"><img src=\"images/heart200.png\" width=\"50\" height=\"50\" align=\"bottom\">with different sizes."
            }
            TextWithImage {
                text: "Resized image<img width=\"48\" height=\"48\" align=\"middle\" src=\"http://qt-project.org/images/qtdn/doc/qt-logo.png\">from the internet."

            }
            TextWithImage {
                text: "Image<img align=\"middle\" src=\"http://qt-project.org/images/qtdn/doc/qt-logo.png\">from the internet."
            }
            TextWithImage {
                height: 120
                verticalAlignment: Text.AlignVCenter
                text: "This is a <b>happy</b> face<img src=\"images/face-smile.png\"> with an explicit height."
            }
        }
    }

    Keys.onUpPressed: main.hAlign = Text.AlignHCenter
    Keys.onLeftPressed: main.hAlign = Text.AlignLeft
    Keys.onRightPressed: main.hAlign = Text.AlignRight
}
