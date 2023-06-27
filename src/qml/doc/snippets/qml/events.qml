// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![document]
import QtQuick

//![parent begin]
Rectangle {
//![parent begin]

    id: screen
    width: 400; height: 500

//! [signal declaration]
    signal trigger
    signal send(notice: string)
    signal perform(task: string, object: variant)
//! [signal declaration]

//! [signal handler declaration]
onTrigger: console.log("trigger signal emitted")

onSend: (notice)=> {
    console.log("send signal emitted with notice: " + notice)
}

onPerform: console.log("perform signal emitted")
//! [signal handler declaration]

//! [automatic signals]
Rectangle {
    id: sprite
    width: 25; height: 25
    x: 50; y: 15

    onXChanged: console.log("x property changed, emitted xChanged signal")
    onYChanged: console.log("y property changed, emitted yChanged signal")
}
//! [automatic signals]

//! [signal emit]
Rectangle {
    id: messenger

    signal send(person: string, notice: string)

    onSend: (person, notice)=> {
        console.log("For " + person + ", the notice is: " + notice)
    }

    Component.onCompleted: messenger.send("Tom", "the door is ajar.")
}
//! [signal emit]

//! [connect method]
Rectangle {
    id: relay

    signal send(person: string, notice: string)
    onSend: (person, notice)=> console.log("Send signal to: " + person + ", " + notice)

    Component.onCompleted: {
        relay.send.connect(sendToPost)
        relay.send.connect(sendToTelegraph)
        relay.send.connect(sendToEmail)
        relay.send("Tom", "Happy Birthday")
    }

    function sendToPost(person, notice) {
        console.log("Sending to post: " + person + ", " + notice)
    }
    function sendToTelegraph(person, notice) {
        console.log("Sending to telegraph: " + person + ", " + notice)
    }
    function sendToEmail(person, notice) {
        console.log("Sending to email: " + person + ", " + notice)
    }
}
//! [connect method]

//! [forward signal]
Rectangle {
    id: forwarder
    width: 100; height: 100

    signal send()
    onSend: console.log("Send clicked")

    MouseArea {
        id: mousearea
        anchors.fill: parent
        onClicked: console.log("MouseArea clicked")
    }
    Component.onCompleted: {
        mousearea.clicked.connect(send)
    }
}
//! [forward signal]

//! [connect method]
//![parent end]
}
//![parent end]

//![document]
