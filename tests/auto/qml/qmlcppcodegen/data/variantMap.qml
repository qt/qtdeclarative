pragma Strict
import QtQml

QtObject {
    property Component shadowable: QtObject {}
    property B b: B { id: theB }
    property rect r: theB.r
    property var v: { "1": null, "25": undefined, "19": "19" }

    property Component c: Component {
        id: unshadowable
        QtObject {}
    }

    // We need this extra function in order to coerce the result of the shadowable
    // method call back to QtObject
    function createShadowable() : QtObject {
        return shadowable.createObject(this, {objectName: "a"})
    }

    objectName: {
        return createShadowable().objectName
            + " " + unshadowable.createObject(this, {objectName: "b"}).objectName
    }

    Component.onCompleted: b.r = { x: 12, y: 13, width: 14, height: 15 }
}
