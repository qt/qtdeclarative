import QtQuick 2.0

WorkerScript {
    id: worker

    property variant response
    property bool readyChangedCalled : false

    signal done()
    signal ready()

    function testSend(value) {
        worker.sendMessage(value)
    }

    function compareLiteralResponse(expected) {
        var e = eval('(' + expected + ')')
        return JSON.stringify(worker.response) == JSON.stringify(e)
    }

    onMessage: {
        worker.response = messageObject
        worker.done()
    }

    onReadyChanged: worker.readyChangedCalled = true
}

