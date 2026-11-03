WorkerScript.onMessage = function(msg) {
    for (var i = 0; i < msg.ops.length; ++i) {
        var op = msg.ops[i];
        if (op.op === "move")
            msg.model.move(op.from, op.to, 1);
        else if (op.op === "insert")
            msg.model.insert(op.index, { "name": op.name });
        else if (op.op === "remove")
            msg.model.remove(op.index, 1);
    }
    msg.model.sync();
    WorkerScript.sendMessage({ "done": true });
}
