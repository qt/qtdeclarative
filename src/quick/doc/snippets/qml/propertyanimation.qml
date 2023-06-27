// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Row {

//![transition]
Rectangle {
    id: rect
    width: 100; height: 100
    color: "red"

    //! [single state]
    states: State {
        name: "moved"
        PropertyChanges { target: rect; x: 50 }
    }
    //! [single state]

    transitions: Transition {
        PropertyAnimation { properties: "x,y"; easing.type: Easing.InOutQuad }
    }
}
//![transition]

//![behavior]
Rectangle {
    width: 100; height: 100
    color: "red"

    Behavior on x { PropertyAnimation {} }

    MouseArea { anchors.fill: parent; onClicked: parent.x = 50 }
}
//![behavior]

//![propertyvaluesource]
Rectangle {
    width: 100; height: 100
    color: "red"

    SequentialAnimation on x {
        loops: Animation.Infinite
        PropertyAnimation { to: 50 }
        PropertyAnimation { to: 0 }
    }
}
//![propertyvaluesource]

    //![standalone]
    Rectangle {
        id: theRect
        width: 100; height: 100
        color: "red"

        // this is a standalone animation, it's not running by default
        PropertyAnimation { id: animation;
                            target: theRect;
                            property: "width";
                            to: 30;
                            duration: 500 }

        MouseArea { anchors.fill: parent; onClicked: animation.running = true }
    }
    //![standalone]
}
