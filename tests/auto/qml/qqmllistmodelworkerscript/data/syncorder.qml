import QtQuick

Item {
    id: item
    property var model
    property bool done: false

    WorkerScript {
        id: worker
        source: "syncorder.js"
        onMessage: item.done = true
    }

    function fill(names) {
        for (var i = 0; i < names.length; ++i)
            model.append({ "name": names[i] });
    }

    function runOps(ops) {
        done = false;
        worker.sendMessage({ "model": model, "ops": ops });
    }
}
