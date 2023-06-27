// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [document]
import QtQuick

//! [parent begin]
Rectangle {
//! [parent begin]
    width: 150; height: 300; color: "white"

//! [model]
ListModel {
    id: nameModel
    ListElement { name: "Alice"; team: "Crypto" }
    ListElement { name: "Bob"; team: "Crypto" }
    ListElement { name: "Jane"; team: "QA" }
    ListElement { name: "Victor"; team: "QA" }
    ListElement { name: "Wendy"; team: "Graphics" }
}
//! [model]

//! [delegate]
Component {
    id: nameDelegate
    Text {
        text: name;
        font.pixelSize: 24
        anchors.left: parent.left
        anchors.leftMargin: 2
    }
}
//! [delegate]

//! [section]
ListView {
    anchors.fill: parent
    model: nameModel
    delegate: nameDelegate
    focus: true
    highlight: Rectangle {
        color: "lightblue"
        width: parent.width
    }
    section {
        property: "team"
        criteria: ViewSection.FullString
        delegate: Rectangle {
            color: "#b0dfb0"
            width: parent.width
            height: childrenRect.height + 4
            Text { anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: 16
                font.bold: true
                text: section
            }
        }
    }
}
//! [section]

//! [parent end]
}
//! [parent end]
//! [document]
