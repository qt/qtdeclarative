pragma ValueTypeBehavior: Addressable
import QtQml
import StaticTest

QtObject {
    property var a
    property rect b: a as rect
    property bool c: a instanceof rect
    property bool d: ({x: 10, y: 20}) instanceof point
    property var e: ({x: 10, y: 20}) as point
    property var f: "red" as withString
    property var g: "green" as string
    property rect bb
    property var p: bb as size;
    property var q: this as size;
    property var r: ({}) as size;
    property var s: 11 as size;
    property var t: Component as size;
    property var u: Qt as size;
}
