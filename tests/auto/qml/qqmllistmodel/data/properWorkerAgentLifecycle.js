WorkerScript.onMessage = function(msg) {
    console.log("worker modifying & syncing model")
    msg.rowListModel.clear();
    msg.rowListModel.sync();

    for (let i = 0; i < msg.a.length; i++) {
        var row = msg.a[i];
        msg.rowListModel.append({ rowNum: i, keys: row });
        msg.rowListModel.sync();
    }

    WorkerScript.sendMessage({})
}
