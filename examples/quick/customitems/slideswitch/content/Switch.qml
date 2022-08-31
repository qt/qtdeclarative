// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Item {
    id: toggleswitch
    width: background.width; height: background.height

//![1]
    property bool on: false
//![1]

//![2]
    function toggle() {
        if (toggleswitch.state == "on")
            toggleswitch.state = "off";
        else
            toggleswitch.state = "on";
    }
//![2]

//![3]
    function releaseSwitch() {
        if (knob.x == 1) {
            if (toggleswitch.state == "off") return;
        }
        if (knob.x == 78) {
            if (toggleswitch.state == "on") return;
        }
        toggle();
    }
//![3]

//![4]
    Image {
        id: background
        source: "background.png"
        MouseArea { anchors.fill: parent; onClicked: toggle() }
    }
//![4]

//![5]
    Image {
        id: knob
        x: 1; y: 2
        source: "knob.png"

        MouseArea {
            anchors.fill: parent
            drag.target: knob; drag.axis: Drag.XAxis; drag.minimumX: 1; drag.maximumX: 78
            onClicked: toggle()
            onReleased: releaseSwitch()
        }
    }
//![5]

//![6]
    states: [
        State {
            name: "on"
            PropertyChanges {
                knob.x: 78
                toggleswitch.on: true
            }
        },
        State {
            name: "off"
            PropertyChanges {
                knob.x: 1
                toggleswitch.on: false
            }
        }
    ]
//![6]

//![7]
    transitions: Transition {
        NumberAnimation { properties: "x"; easing.type: Easing.InOutQuad; duration: 200 }
    }
//![7]
}
//![0]
