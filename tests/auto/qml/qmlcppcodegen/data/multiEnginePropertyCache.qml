pragma Strict
import QtQml

QtObject {
    id: root

    property int foo: 0
    onFooChanged: root.close1()

    property int bar: 0
    onBarChanged: close2()

    function close1() {
        console.log("close1")
    }

    function close2() {
        console.log("close2")
    }
}
