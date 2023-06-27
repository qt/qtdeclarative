// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Rectangle {
    width: 300; height: 300

    Text {
        id: myText
        text: 'Click anywhere'
    }

    WorkerScript {
        id: myWorker
        source: "script.mjs"

        onMessage: (messageObject)=> myText.text = messageObject.reply
    }

    MouseArea {
        anchors.fill: parent
        onClicked: (mouse)=> myWorker.sendMessage({ 'x': mouse.x, 'y': mouse.y })
    }
}
//![0]
