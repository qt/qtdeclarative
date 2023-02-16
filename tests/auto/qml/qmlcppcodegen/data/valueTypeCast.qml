pragma Strict
pragma ValueTypeBehavior: Addressable
import QtQml

QtObject {
    property rect r: Qt.rect(10, 20, 3, 4)
    property var v: r
    property real x: (v as rect).x
}
