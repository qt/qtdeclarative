import QtQuick 1.0

Item {
    WorkerScript {
        id: worker
        source: "stressDispose.js"
    }

    Component.onCompleted: {
        worker.sendMessage(10);
    }
}

