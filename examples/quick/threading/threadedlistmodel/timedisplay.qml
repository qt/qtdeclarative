// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    color: "white"
    width: 200
    height: 300

    ListView {
        anchors.fill: parent
        model: listModel
        delegate: Component {
            Text {
                required property string time
                text: time
            }
        }

        ListModel { id: listModel }

        WorkerScript {
            id: worker
            source: "dataloader.mjs"
        }

// ![0]
        Timer {
            id: timer
            interval: 2000; repeat: true
            running: true
            triggeredOnStart: true

            onTriggered: {
                var msg = {'action': 'appendCurrentTime', 'model': listModel};
                worker.sendMessage(msg);
            }
        }
// ![0]
    }
}
