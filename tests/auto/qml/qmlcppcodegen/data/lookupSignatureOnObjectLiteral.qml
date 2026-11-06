pragma Strict
import QtQml

QtObject {
    function f() {
        var r = ({ ok: true })
        if (r && r.ok)
            console.log("force if condition eval")
    }
}
