// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![document]
import QtQuick

//![parent begin]
Rectangle {
//![parent begin]

    id: screen
    width: 400; height: 500


Rectangle {
    id: flag
}
Column {
    spacing: 15
//![signal states]
Rectangle {
    id: signal
    width: 200; height: 200
    state: "NORMAL"

    states: [
        State {
            name: "NORMAL"
            PropertyChanges { target: signal; color: "green"}
            PropertyChanges { target: flag; state: "FLAG_DOWN"}
        },
        State {
            name: "CRITICAL"
            PropertyChanges { target: signal; color: "red"}
            PropertyChanges { target: flag; state: "FLAG_UP"}
        }
    ]
}
//![signal states]

//![switch states]
Rectangle {
    id: signalswitch
    width: 75; height: 75
    color: "blue"

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (signal.state == "NORMAL")
                signal.state = "CRITICAL"
            else
                signal.state = "NORMAL"
        }
    }
}
//![switch states]

//![when property]
Rectangle {
    id: bell
    width: 75; height: 75
    color: "yellow"

    states: State {
                name: "RINGING"
                when: (signal.state == "CRITICAL")
                PropertyChanges {target: speaker; play: "RING!"}
            }
}
//![when property]

Text {
    id: speaker
    property alias play: speaker.text
    text: "NORMAL"
}

} // end of row

//![parent end]
}
//![parent end]

//![document]
