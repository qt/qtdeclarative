// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [document]
//contents of Button.qml
import QtQuick

//! [parent begin]
Rectangle {
//! [parent begin]
    id: button
//! [properties]
    width: 145; height: 60
    color: "blue"
    antialiasing: true; radius: 9
    property alias text: label.text
//! [properties]
//! [object alias]
    property alias label: label
//! [object alias]
    border {color: "#B9C5D0"; width: 1}

    gradient: Gradient {
        GradientStop {color: "#CFF7FF"; position: 0.0}
        GradientStop {color: "#99C0E5"; position: 0.57}
        GradientStop {color: "#719FCB"; position: 0.9}
    }
//![text]
    Text {
        id: label
        anchors.centerIn: parent
        text: "Click Me!"
        font.pointSize: 12
        color: "blue"
    }
//![text]
    MouseArea {
        anchors.fill: parent
        onClicked: console.log(text + " clicked")
    }
//! [parent end]
}
//! [parent end]

//! [document]

//! [ellipses]
    //...
//! [ellipses]


