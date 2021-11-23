import QtQuick

Item {
    function wait(timeout) {
        if (timeout === undefined)
            timeout = 5000

        var i = 0;
        while (i < timeout) {
            i += 50
        }

        return i
    }

    property var a: wait(10)
    property var b: wait()
}
