// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [document]
import QtQuick


//! [parent begin]
Rectangle {
//! [parent begin]
    width: 200; height: 600
    id: screen

Column {
    spacing: 12
//! [direct property change]
Rectangle {
    id: blob
    width: 75; height: 75
    color: "blue"

    MouseArea {
        anchors.fill: parent
        onClicked: blob.color = "green"
    }
}
//! [direct property change]

//! [property animation]
Rectangle {
    id: flashingblob
    width: 75; height: 75
    color: "blue"
    opacity: 1.0

    MouseArea {
        anchors.fill: parent
        onClicked: {
            animateColor.start()
            animateOpacity.start()
        }
    }

    PropertyAnimation {id: animateColor; target: flashingblob; properties: "color"; to: "green"; duration: 100}

    NumberAnimation {
        id: animateOpacity
        target: flashingblob
        properties: "opacity"
        from: 0.99
        to: 1.0
        loops: Animation.Infinite
        easing {type: Easing.OutBack; overshoot: 500}
   }
}
//! [property animation]

//! [transition animation]
Rectangle {
    width: 75; height: 75
    id: button
    state: "RELEASED"

    MouseArea {
        anchors.fill: parent
        onPressed: button.state = "PRESSED"
        onReleased: button.state = "RELEASED"
    }

    states: [
        State {
            name: "PRESSED"
            PropertyChanges { target: button; color: "lightblue"}
        },
        State {
            name: "RELEASED"
            PropertyChanges { target: button; color: "lightsteelblue"}
        }
    ]

    transitions: [
        Transition {
            from: "PRESSED"
            to: "RELEASED"
            ColorAnimation { target: button; duration: 100}
        },
        Transition {
            from: "RELEASED"
            to: "PRESSED"
            ColorAnimation { target: button; duration: 100}
        }
    ]
}
//! [transition animation]

Rectangle {
    width: 75; height: 75
    id: wildcard
    color: "green"
//! [wildcard animation]
    transitions:
        Transition {
            to: "*"
            ColorAnimation { target: button; duration: 100}
        }
//! [wildcard animation]

    MouseArea {
        anchors.fill: parent
        onPressed: {
            ball.x = 10
            ball.color = "red"
        }
        onReleased: {
            ball.x = screen.width / 2
            ball.color = "salmon"
        }
    }
}

//! [behavior animation]
Rectangle {
    width: 75; height: 75; radius: width
    id: ball
    color: "salmon"

    component BounceAnimation : NumberAnimation {
       easing {
          type: Easing.OutElastic
          amplitude: 1.0
          period: 0.5
       }
    }

    Behavior on x {
        BounceAnimation {}
    }
    Behavior on y {
        BounceAnimation {}
    }
    Behavior on color {
        ColorAnimation { target: ball; duration: 100 }
    }
}
//! [behavior animation]

//! [sequential animation]
Rectangle {
    id: banner
    width: 150; height: 100; border.color: "black"

    Column {
        anchors.centerIn: parent
        Text {
            id: code
            text: "Code less."
            opacity: 0.01
        }
        Text {
            id: create
            text: "Create more."
            opacity: 0.01
        }
        Text {
            id: deploy
            text: "Deploy everywhere."
            opacity: 0.01
        }
    }

    MouseArea {
        anchors.fill: parent
        onPressed: playbanner.start()
    }

    SequentialAnimation {
        id: playbanner
        running: false
        NumberAnimation { target: code; property: "opacity"; to: 1.0; duration: 200}
        NumberAnimation { target: create; property: "opacity"; to: 1.0; duration: 200}
        NumberAnimation { target: deploy; property: "opacity"; to: 1.0; duration: 200}
    }
}
//! [sequential animation]

}//end of col
//! [parent end]
}
//! [parent end]

//! [document]
