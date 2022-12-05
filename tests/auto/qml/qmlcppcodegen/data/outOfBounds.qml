pragma Strict
import QtQuick

Item {
    property var oob: children[12]
    property QtObject oob2: children[123]
    Component.onCompleted: console.log("oob", children[11])
}
