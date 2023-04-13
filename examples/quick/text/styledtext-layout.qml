// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: main

    readonly property real margin: 8

    width: 320
    height: 480
    focus: true

    Text {
        id: myText
        anchors.fill: parent
        anchors.margins: 10
        wrapMode: Text.WordWrap
        font.family: "Times New Roman"
        font.pixelSize: 14
        textFormat: Text.StyledText
        horizontalAlignment: Text.AlignJustify

        text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer at ante dui <a href=\"http://www.digia.com\">www.digia.com</a>.<br/>Curabitur ante est, pulvinar quis adipiscing a, iaculis id ipsum. Nunc blandit condimentum odio vel egestas.<br><ul type=\"bullet\"><li>Coffee<ol type=\"a\"><li>Espresso<li>Cappuccino<li>Latte</ol><li>Juice<ol type=\"1\"><li>Orange</li><li>Apple</li><li>Pineapple</li><li>Tomato</li></ol></li></ul><p><font color=\"#434343\"><i>Proin consectetur <b>sapien</b> in ipsum lacinia sit amet mattis orci interdum. Quisque vitae accumsan lectus. Ut nisi turpis, sollicitudin ut dignissim id, fermentum ac est. Maecenas nec libero leo. Sed ac leo eget ipsum ultricies viverra sit amet eu orci. Praesent et tortor risus, viverra accumsan sapien. Sed faucibus eleifend lectus, sed euismod urna porta eu. Quisque vitae accumsan lectus. Ut nisi turpis, sollicitudin ut dignissim id, fermentum ac est. Maecenas nec libero leo. Sed ac leo eget ipsum ultricies viverra sit amet eu orci."

//! [layout]
        onLineLaidOut: (line) => {
            line.width = width / 2  - main.margin

            if (line.y + line.height >= height) {
                line.y -= height - main.margin
                line.x = width / 2 + main.margin
            }

            if (line.isLast) {
                lastLineMarker.x = line.x + line.implicitWidth
                lastLineMarker.y = line.y + (line.height - lastLineMarker.height) / 2
            }
        }
//! [layout]

        Rectangle {
            id: lastLineMarker
            color: "#44cccccc"
            width: theEndText.width + main.margin
            height: theEndText.height + main.margin

            Text {
                id: theEndText
                text: qsTr("THE\nEND")
                anchors.centerIn: parent
                font.pixelSize: myText.font.pixelSize / 2
            }
        }
    }
}
