import QtQuick 2.0

QtObject {
    id: root
    Component.onCompleted: {
        var a = [1, 2]
        var b = {a: "hello", d: 1 }
        var c
        var d = 12
        var e = function() { return 5;}
        var f = true
        var g = {toString: function() { throw new Error('toString'); }}

        console.profile("profile1")
        console.time("timer1")
        console.log("completed", "ok")
        console.log("completed ok")
        console.debug("completed ok")
        console.warn("completed ok")
        console.error("completed ok")
        console.log(a)
        console.log(b)
        console.log(c)
        console.log(d)
        console.log(e)
        console.log(f)
        console.log(root)
        console.log(g)
        console.log(1, "pong!", new Object)
        console.log(1, ["ping","pong"], new Object, 2)
        console.trace()
        console.timeEnd("timer1")
        console.profileEnd("profile1")
        console.log(exception) //This has to be at the end
    }
}
