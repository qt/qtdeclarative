pragma Strict

import QtQml
import QtQuick

Window {
    property var foo
    Component.onCompleted: {
        console.log(foo !== null)
        console.log(foo === null)
        console.log(foo !== undefined)
        console.log(foo === undefined)
    }
}
