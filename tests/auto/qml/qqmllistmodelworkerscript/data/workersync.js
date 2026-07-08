WorkerScript.onMessage = function(msg) {
    if (msg.action == 'addItem') {
        msg.model.get(0).level0.append({ 'level1': 33 });
    } else if (msg.action == 'prepend') {
        msg.model.insert(0, { 'level0': [ { 'level1': 41 } ] });
    } else if (msg.action == 'dosync') {
        msg.model.sync();
    }
    WorkerScript.sendMessage({'done': true})
}
