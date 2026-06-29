import QtQuick
import QV4DebuggerTestTypes
MyType {
    objectName: "patron"
    Item {
        Component.onCompleted: {
            console.log("Hallo Welt");
        }
    }
}