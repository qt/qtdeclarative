pragma Strict
import QtQml

QtObject {
    property Component shadowable: QtObject {}
    property B b: B { id: theB }
    property rect r: theB.r

    property Component c: Component {
        id: unshadowable
        QtObject {}
    }

    objectName: {
        return shadowable.createObject(this, {objectName: "a"}).objectName
            + " " + unshadowable.createObject(this, {objectName: "b"}).objectName
    }

    Component.onCompleted: b.r = { x: 12, y: 13, width: 14, height: 15 }
}
