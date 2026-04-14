import QtQuick

Window {
    id: root
    width: 100
    height: 100
    visible: true

    property int sent: 0
    property int received: 0

    WorkerScript {
        id: keyboardWorker
        source: "properWorkerAgentLifecycle.js"
        onMessage: _ => ++received
    }

    function messageWorker() {
        ++sent
        keyboardWorker.sendMessage({
                                       a: [ [ { label: "0" }, ], [ { label: "A" }, ]],
                                       rowListModel: rowListModel
                                   });
    }

    Timer {
        running: root.sent < 4
        repeat: true
        interval: 50
        onTriggered: messageWorker()
    }

    ListModel {
        id: rowListModel
    }

    Item {
        width: parent.width

        ListView {
            anchors.fill: parent
            model: rowListModel
            delegate:
                Row {
                    id: row
                    required property QtObject modelData
                    Repeater {
                        id: keyboardRowRepeater
                        model: row.modelData.keys
                    }
                }
        }
    }
}
