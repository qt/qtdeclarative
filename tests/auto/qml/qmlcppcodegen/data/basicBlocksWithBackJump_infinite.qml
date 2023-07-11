pragma Strict
import QtQml

QtObject {
    function infinite() {
        let foo = false
        if (true) {
            while (true) {}
        } else {
            console.log(foo)
        }
    }
}
