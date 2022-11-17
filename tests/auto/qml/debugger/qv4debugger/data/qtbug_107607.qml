import QtQuick
import TestTypes
MyType {
    objectName: "patron"
    Item {
        Component.onCompleted: {
            console.log("Hallo Welt");
        }
    }
}