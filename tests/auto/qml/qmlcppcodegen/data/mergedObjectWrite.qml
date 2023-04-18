pragma Strict
import QtQuick

Item {
    objectName: "a"

    function f(arg: Item) : string {
        // Write arg as Item, read it as QtObject.
        arg.x = 5
        console.log(arg)
        return arg.objectName
    }

    Component.onCompleted: objectName = f(null)
}
