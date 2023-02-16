pragma ValueTypeBehavior: Addressable
import QtQml

QtObject {
    property var a
    property rect b: a as rect
    property bool c: a instanceof rect
    property bool d: ({x: 10, y: 20}) instanceof point
    property var e: ({x: 10, y: 20}) as point
}
