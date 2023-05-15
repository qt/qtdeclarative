pragma Strict
import QtQuick

Item {
    objectName: "a"

    function f(arg: Item) : string {
        // Read arg as QtObject and Item, merged into QtObject.
        console.log(arg)
        return arg.x
    }

    Component.onCompleted: objectName = f(null)
}
