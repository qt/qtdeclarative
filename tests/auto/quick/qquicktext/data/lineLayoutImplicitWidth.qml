// Copyright (C) 2019 Jolla Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    id: main
    width: 800; height: 600

    property real off: 0

    Text {
        id: myText
        objectName: "myText"
        wrapMode: Text.WordWrap
        font.pixelSize: 14
        textFormat: Text.PlainText
        focus: true
        anchors.fill: parent

        // The autotest will retrieve these so that it can verify them
        property var lineImplicitWidths: []

        text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nullam quis ante tristique, fermentum magna at, varius lacus. Donec elementum orci sit amet ligula efficitur, eget sodales orci porttitor. Etiam laoreet tellus quis nisi mollis lacinia. Cras vitae nisl sed nunc semper blandit. Duis egestas commodo lacus non congue. Fusce quis rhoncus urna. And magna arcu, sodales vitae nunc vel, rutrum hendrerit magna. Nullam imperdiet porttitor sem at euismod. Morbi faucibus libero sit amet vestibulum aliquam. Duis consectetur lacinia malesuada. Sed quis ante dui. Name dignissim faucibus felis. Quisque dapibus aliquam ante, eu cursus elit dictum in. Mauris placerat efficitur rutrum."

        onLineLaidOut: (line) => {
            var n = line.number

            // Save information about the line so the autotest can retrieve it
            lineImplicitWidths[n] = line.implicitWidth
        }
    }
}
